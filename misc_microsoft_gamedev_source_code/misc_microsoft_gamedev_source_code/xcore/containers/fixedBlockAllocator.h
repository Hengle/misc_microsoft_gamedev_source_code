//============================================================================
//
// File: fixedBlockAllocator.h
// rg [3/11/06] - This freelist allocator uses a bitmap instead of an in-place linked list to track free/allocated items.
// This can result in increased cache efficiency, depending on the load. Also, efficient serialization/deserialization
// is easy because the bitmap indicates which entries are in-use.
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#pragma once

#include "staticArray.h"

template<uint cItemSizeParam, uint cMaxItemsParam>
class BFixedBlockAllocator
{
public:
   enum { cItemSize = cItemSizeParam, cMaxItems = cMaxItemsParam };
      
   BFixedBlockAllocator() :
      mLowestFreeItem(0),
      mHighwaterMark(0),
      mNumFreeItems(cMaxItems)
   {
      mBuf.resize(cMaxItems * cItemSize);
      Utils::FastMemSet(mBitmap, 0xFF, sizeof(mBitmap));
   }
         
   void* alloc(bool fatalIfFailed)
   {
      const int itemIndex = allocIndex(fatalIfFailed);
      if (itemIndex < 0)
         return NULL;
      
      BDEBUG_ASSERT(itemIndex < cMaxItems);
         
      return &mBuf[itemIndex * cItemSize];         
   }
   
   int allocIndex(bool fatalIfFailed)
   {
      BDEBUG_ASSERT((mHighwaterMark <= cMaxItems) && (mLowestFreeItem <= mHighwaterMark));
      uint itemIndex;

      if ((mHighwaterMark < cMaxItems) && (mLowestFreeItem == mHighwaterMark))
      {
         itemIndex = mHighwaterMark;
         
         mBitmap[mHighwaterMark >> 6] &= (~( ((uint64)1) << (63 - (mHighwaterMark & 63))));

         mHighwaterMark++;

         mLowestFreeItem = mHighwaterMark;

         BDEBUG_ASSERT(mNumFreeItems != 0);
         mNumFreeItems--;

         return itemIndex;         
      }

      if (mLowestFreeItem == cMaxItems)
      {
         if (fatalIfFailed)
            BFATAL_FAIL("BFixedBlockAllocator::alloc: Out of blocks");
         return -1;
      }

      uint ofs = mLowestFreeItem >> 6;
      while (ofs < cBitmapSize)
      {
         if (mBitmap[ofs] != 0)
            break;
         ofs++;
      }

      if (ofs == cBitmapSize)
      {
         if (fatalIfFailed)
            BFATAL_FAIL("BFixedBlockAllocator::alloc: Out of blocks");
         return -1;
      }

      uint64 val = mBitmap[ofs];
      const uint lzc = _CountLeadingZeros64(val);

      itemIndex = (ofs << 6) + lzc;

      if (itemIndex >= cMaxItems)
      {
         if (fatalIfFailed)
            BFATAL_FAIL("BFixedBlockAllocator::alloc: Out of blocks");
         return -1;
      }

      BDEBUG_ASSERT(mNumFreeItems != 0);
      mNumFreeItems--;

      mLowestFreeItem = itemIndex + 1;
      mHighwaterMark = Math::Max(mHighwaterMark, mLowestFreeItem);

      val &= ~( ((uint64)1) << (63U - lzc));

      mBitmap[ofs] = val;

      return itemIndex;
   }
   
   void free(void* p)
   {
      if (!p)
         return;
         
      BDEBUG_ASSERT((p >= mBuf.begin()) && (p < mBuf.end()));
                  
      const uint itemIndex = ((uchar*)p - mBuf.begin()) / cItemSize;
      
      BDEBUG_ASSERT(itemIndex < cMaxItems);
      
      freeIndex(itemIndex);
   }
   
   void freeIndex(const uint itemIndex)
   {
      const uint ofs = itemIndex >> 6;
      const uint64 bitMask = ((uint64)1) << (63 - (itemIndex & 63));

      BDEBUG_ASSERT(itemIndex < mHighwaterMark);

      mLowestFreeItem = Math::Min(mLowestFreeItem, itemIndex);

      BDEBUG_ASSERT(0 == (mBitmap[ofs] & bitMask));

      mBitmap[ofs] |= bitMask;

      if (itemIndex == mHighwaterMark - 1)
         mHighwaterMark = itemIndex;

      BDEBUG_ASSERT(mNumFreeItems != cMaxItems);
      mNumFreeItems++;
      if (mNumFreeItems == cMaxItems)
      {
         BDEBUG_ASSERT(0 == mLowestFreeItem);
         mHighwaterMark = 0;
      }
   }
   
   void freeAll(void)
   {
      if (mNumFreeItems != cMaxItems)
      {
         Utils::FastMemSet(mBitmap, 0xFF, ((mHighwaterMark + 63) >> 6) * sizeof(uint64));
         
         mLowestFreeItem = 0;
         mHighwaterMark = 0;
         mNumFreeItems = cMaxItems;
      }         
   }
   
   uint getItemIndex(const void* p) const { BDEBUG_ASSERT((p >= mBuf.begin()) && (p < mBuf.end())); return ((uchar*)p - mBuf.begin()) / cItemSize; }
   
         void* getItem(uint index)        { BDEBUG_ASSERT(index < cMaxItems); return &mBuf[index * cItemSize]; }
   const void* getItem(uint index) const  { BDEBUG_ASSERT(index < cMaxItems); return &mBuf[index * cItemSize]; }
   
   uint getNumFreeItems(void) const { return mNumFreeItems; }
   uint getNumAllocatedItems(void) const { return cMaxItems - mNumFreeItems; }
   uint getMaxAllocatedItems(void) const { return cMaxItems; }
   uint getItemSize(void) const { return cItemSize; }
   uint getHighwaterMark(void) const { return mHighwaterMark; }
      
   bool isItemAllocated(uint itemIndex) const
   {
      BDEBUG_ASSERT(itemIndex < cMaxItems);
      
      const uint ofs = itemIndex >> 6;
      const uint64 bitMask = ((uint64)1) << (63 - (itemIndex & 63));
      
      return 0 == (mBitmap[ofs] & bitMask);
   }
   
   uint getSerializeSize(void) const
   {
      uint size = sizeof(uint) * 3;
      size += ((mHighwaterMark + 63) >> 6) * sizeof(uint64);
      size += getNumAllocatedItems() * cItemSize;
      return size;
   }
   
   uint serialize(uchar* RESTRICT pDst) const
   {
      const uchar* pOrigDst = pDst;
    
      writeObj(pDst, mLowestFreeItem);
      writeObj(pDst, mHighwaterMark);
      writeObj(pDst, mNumFreeItems);
      
      const uint numQWORDs = (mHighwaterMark + 63) >> 6;
      
      Utils::FastMemCpy(pDst, mBitmap, numQWORDs * sizeof(uint64));
      pDst += numQWORDs * sizeof(uint64);
      
      for (uint i = 0; i < mHighwaterMark; i++)
      {
         if (isItemAllocated(i))
         {
            Utils::FastMemCpy(pDst, &mBuf[i * cItemSize], cItemSize);
            pDst += cItemSize;
         }
      }
            
      return pDst - pOrigDst;
   }
   
   uint deserialize(const uchar* RESTRICT pSrc)
   {
      const uchar* pOrigSrc = pSrc;
      
      readObj(pSrc, mLowestFreeItem);
      readObj(pSrc, mHighwaterMark);
      readObj(pSrc, mNumFreeItems);
      
      const uint numQWORDs = (mHighwaterMark + 63) >> 6;
      
      Utils::FastMemCpy(mBitmap, pSrc, numQWORDs * sizeof(uint64));
      pSrc += numQWORDs * sizeof(uint64);
      
      Utils::FastMemSet(mBitmap + numQWORDs, 0xFF, (cBitmapSize - numQWORDs) * sizeof(uint64));
      
      for (uint i = 0; i < mHighwaterMark; i++)
      {
         if (isItemAllocated(i))
         {
            Utils::FastMemCpy(&mBuf[i * cItemSize], pSrc, cItemSize);
            pSrc += cItemSize;
         }
      }
            
      return pSrc - pOrigSrc;
   }
      
private: 
   BStaticArray<uchar, cItemSize * cMaxItems, false, false> mBuf;
   
   uint mLowestFreeItem;
   uint mHighwaterMark;
   uint mNumFreeItems;
   
   enum { cBitmapSize = (cMaxItems + 63) >> 6 };
   uint64 mBitmap[cBitmapSize];
         
   template<class T>
   static void writeObj(uchar* RESTRICT & pDst, const T& obj)
   {
      Utils::FastMemCpy(pDst, &obj, sizeof(obj));
      pDst += sizeof(obj);
   }

   template<class T>
   static void readObj(const uchar* RESTRICT & pSrc, T& obj)
   {
      Utils::FastMemCpy(&obj, pSrc, sizeof(obj));
      pSrc += sizeof(obj);
   }
};




