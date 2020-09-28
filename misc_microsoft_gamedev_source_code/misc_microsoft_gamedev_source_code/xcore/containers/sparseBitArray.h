//============================================================================
//
// File: sparseBitArray.h
// Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once
#include "containers\freelist.h"

#define ALL_SET_BUCKET_PTR ((BBitBucket*)1U)

class BSparseBitArray
{
public:
   BSparseBitArray(BMemoryHeap* pHeap = &gPrimaryHeap) : 
      mBucketFreeList(0, pHeap),
      mBitBuckets(BDynamicArrayHeapAllocator<BBitBucket*, sizeof(uint)>(pHeap)),
      mNumBits(0) 
   { 
   }
   
   BSparseBitArray(uint size, BMemoryHeap* pHeap = &gPrimaryHeap) : 
      mBucketFreeList(0, pHeap),
      mBitBuckets(BDynamicArrayHeapAllocator<BBitBucket*, sizeof(uint)>(pHeap)),
      mNumBits(0) 
   { 
      resize(size); 
   }
   
   BSparseBitArray(const BSparseBitArray& other) :
      mBucketFreeList(0, other.getHeap()),
      mBitBuckets(BDynamicArrayHeapAllocator<BBitBucket*, sizeof(uint)>(other.getHeap())),
      mNumBits(0)
   {
      *this = other;
   }
   
   ~BSparseBitArray() 
   { 
      clear();
   }
   
   BMemoryHeap* getHeap() const { return mBucketFreeList.getHeap(); }
   
   void setHeap(BMemoryHeap* pHeap)
   {
      clear();
      
      mBucketFreeList.setHeap(pHeap);
      mBitBuckets.setAllocator(BDynamicArrayHeapAllocator<BBitBucket*, sizeof(uint)>(pHeap));
   }
   
   BSparseBitArray& operator= (const BSparseBitArray& rhs)
   {
      if (this == &rhs)
         return *this;
         
      clear();
      
      mNumBits = rhs.mNumBits;
      mBitBuckets.resize(rhs.mBitBuckets.getSize());
      
      for (uint i = 0; i < rhs.mBitBuckets.getSize(); i++)
      {
         BBitBucket* p = rhs.mBitBuckets[i];
         if (p)
         {
            if (p == ALL_SET_BUCKET_PTR)
               mBitBuckets[i] = ALL_SET_BUCKET_PTR;
            else
               mBitBuckets[i] = newBucket(*p);
         }
      }
           
      return *this;           
   }
      
   void clear() 
   { 
      mNumBits = 0; 
      
      for (uint i = 0; i < mBitBuckets.getSize(); i++)
         if (mBitBuckets[i] > ALL_SET_BUCKET_PTR)
            deleteBucket(mBitBuckets[i]);
            
      mBitBuckets.clear(); 
      
      mBucketFreeList.clear();
   }
   
   uint getNumBitsSet() const
   {
      uint total = 0;
      
      for (uint i = 0; i < mBitBuckets.getSize(); i++)
      {
         const BBitBucket* pBucket = mBitBuckets[i];
         
         if (pBucket == ALL_SET_BUCKET_PTR)  
            total += cBitsPerBitBucket;
         else if (pBucket > ALL_SET_BUCKET_PTR)
            total += pBucket->countBits();
      }
      
      BDEBUG_ASSERT(total <= mNumBits);
        
      return total;
   }
   
   uint getNumBitsClear() const { return mNumBits - getNumBitsSet(); }
         
   void resize(uint newSize)
   {
      if (mNumBits == newSize)
         return;
      
      const uint newNumBuckets = (newSize + cBitsPerBitBucket - 1) >> cBitsPerBitBucketShift;
      
      if (newSize > mNumBits)
      {
         mBitBuckets.resize(newNumBuckets);
         
         mNumBits = newSize;
      }
      else
      {
         for (uint i = newNumBuckets; i < mBitBuckets.getSize(); i++)
            if (mBitBuckets[i] > ALL_SET_BUCKET_PTR)
               deleteBucket(mBitBuckets[i]); 
      
         mBitBuckets.resize(newNumBuckets);
         
         mNumBits = newSize;
         
         cleanLastBucket();
      }
   }
   
   uint getSize() const { return mNumBits; }
   
   void setBit(uint index)
   {
      BDEBUG_ASSERT(index < mNumBits);
      
      const uint bucketIndex = index >> cBitsPerBitBucketShift;
      const uint bitIndex = index & cBitsPerBitBucketMask;
      
      BBitBucket* pBucket = mBitBuckets[bucketIndex];
      
      if (pBucket == ALL_SET_BUCKET_PTR)
         return;
      else if (!pBucket)
      {
         pBucket = newBucket();
         mBitBuckets[bucketIndex] = pBucket;
         pBucket->setBit(bitIndex);
      }
      else
      {
         pBucket->setBit(bitIndex);
         if (pBucket->areAllSet())
         {
            deleteBucket(pBucket);
            mBitBuckets[bucketIndex] = ALL_SET_BUCKET_PTR;
         }
      }
   }
   
   void clearBit(uint index) 
   {
      BDEBUG_ASSERT(index < mNumBits);

      const uint bucketIndex = index >> cBitsPerBitBucketShift;
      
      BBitBucket* pBucket = mBitBuckets[bucketIndex];
      if (pBucket)
      {
         const uint bitIndex = index & cBitsPerBitBucketMask;
         
         if (pBucket == ALL_SET_BUCKET_PTR)
         {
            pBucket = newBucket(UINT64_MAX);
            pBucket->clearBit(bitIndex);
            mBitBuckets[bucketIndex] = pBucket;
         }
         else
         {
            pBucket->clearBit(bitIndex);
            if (pBucket->areAllClear())
            {
               deleteBucket(pBucket);
               mBitBuckets[bucketIndex] = NULL;
            }
         }
      }
   }
   
   bool isBitSet(uint index) const
   {
      BDEBUG_ASSERT(index < mNumBits);

      const uint bucketIndex = index >> cBitsPerBitBucketShift;
      
      const BBitBucket* pBucket = mBitBuckets[bucketIndex];
      if (pBucket == ALL_SET_BUCKET_PTR)
         return true;
      else if (pBucket)
      {     
         const uint bitIndex = index & cBitsPerBitBucketMask;
         return pBucket->isBitSet(bitIndex);
      }
         
      return false;
   }
   
   void pushBack(bool value)
   {
      resize(mNumBits + 1);
      
      BDEBUG_ASSERT(!isBitSet(mNumBits - 1));
      
      if (value)
         setBit(mNumBits - 1);
   }
   
   void setAll(bool value)
   {
      if (mBitBuckets.isEmpty())
         return;
         
      if (value)
      {
         for (uint i = 0; i < mBitBuckets.getSize(); i++)
         {
            BBitBucket* pBucket = mBitBuckets[i];
            if (pBucket != ALL_SET_BUCKET_PTR)
            {
               if (pBucket > ALL_SET_BUCKET_PTR)
                  deleteBucket(pBucket);
               mBitBuckets[i] = ALL_SET_BUCKET_PTR;
            }               
         }
      }
      else
      {
         for (uint i = 0; i < mBitBuckets.getSize(); i++)
         {
            BBitBucket* pBucket = mBitBuckets[i];
            if (pBucket)
            {
               if (pBucket != ALL_SET_BUCKET_PTR)
                  deleteBucket(pBucket);
               mBitBuckets[i] = NULL;
            }
         }
      }
      
      cleanLastBucket();
   }

private:
   uint mNumBits;
   
   enum 
   { 
      // Number of QWORD's per bit bucket log2
      cBucketCountShift       = 1,
      
      // Number of QWORD's per bit bucket
      cBucketCount            = 1 << cBucketCountShift,     

      // Number of bits per bit bucket log2
      cBitsPerBitBucketShift  = 7,
      cBitsPerBitBucket       = 1 << cBitsPerBitBucketShift,
      cBitsPerBitBucketMask   = (cBitsPerBitBucket - 1)
   };
   
   struct BBitBucket
   {
      BBitBucket(uint val = 0) { setAll(val); }
      
      void setAll(uint64 val = 0) { for (uint i = 0; i < cBucketCount; i++) mBits[i] = val; }
      void clear() { setAll(0); }
      
      uint64 mBits[cBucketCount];
      
      bool isBitSet(uint index) const  { BDEBUG_ASSERT(index < cBitsPerBitBucket); return (mBits[index >> 6] & Utils::BBitMasks::get64(index & 63)) != 0; }
      void setBit(uint index)          { BDEBUG_ASSERT(index < cBitsPerBitBucket); mBits[index >> 6] |= Utils::BBitMasks::get64(index & 63); }
      void clearBit(uint index)        { BDEBUG_ASSERT(index < cBitsPerBitBucket); mBits[index >> 6] &= ~Utils::BBitMasks::get64(index & 63); }
      
      bool areAllClear() const
      {
         for (uint i = 0; i < cBucketCount; i++)
            if (mBits[i])
               return false;
         return true;
      }
      
      bool areAllSet() const
      {
         for (uint i = 0; i < cBucketCount; i++)
            if (mBits[i] != UINT64_MAX)
               return false;
         return true;
      }
      
      uint countBits() const
      {
         uint total = 0;
         for (uint i = 0; i < cBucketCount; i++)
         {
            const uint lo = static_cast<uint>(mBits[i]);
            const uint hi = static_cast<uint>(mBits[i] >> 32U);
            total += Utils::CountBits(lo) + Utils::CountBits(hi);
         }
         return total;            
      }
   };
            
   BDynamicArray<BBitBucket*, sizeof(BBitBucket*), BDynamicArrayHeapAllocator> mBitBuckets;
   
   BFreeList<BBitBucket, 5> mBucketFreeList;
   
   void cleanLastBucket()
   {
      if (mNumBits)
      {
         BBitBucket* pLastBucket = mBitBuckets.back();
         if (pLastBucket)
         {
            pLastBucket = makePhysical(mBitBuckets.getSize() - 1);
               
            for (uint i = mNumBits & cBitsPerBitBucketMask; i < cBitsPerBitBucket; i++)
               pLastBucket->clearBit(i);
               
            checkBucket(mBitBuckets.getSize() - 1);
         }
      }               
   }
   
   BBitBucket* makePhysical(uint index)
   {
      BBitBucket* pBucket = mBitBuckets[index];
      if (!pBucket)
      {
         pBucket = newBucket(0);
         mBitBuckets[index] = pBucket;
      }
      else if (pBucket == ALL_SET_BUCKET_PTR)
      {
         pBucket = newBucket(UINT64_MAX);
         mBitBuckets[index] = pBucket;
      }
      return pBucket;         
   }
   
   bool checkBucket(uint index)
   {
      BBitBucket* pBucket = mBitBuckets[index];
      
      if (pBucket > ALL_SET_BUCKET_PTR)
      {
         if (pBucket->areAllSet())
         {
            deleteBucket(pBucket);
            mBitBuckets[index] = ALL_SET_BUCKET_PTR;
            return true;
         }
         else if (pBucket->areAllClear())
         {
            deleteBucket(pBucket);
            mBitBuckets[index] = NULL;
            return true;
         }
      }
      return false;
   }
   
   BBitBucket* newBucket(uint64 val = 0)
   {
      BBitBucket* p = mBucketFreeList.acquire(true);
      p->setAll(val);
      return p;  
   }
   
   BBitBucket* newBucket(const BBitBucket& other)
   {
      BBitBucket* p = mBucketFreeList.acquire(true);
      memcpy(p, &other, sizeof(BBitBucket));
      return p;
   }
   
   void deleteBucket(BBitBucket* p)
   {
      if (!p)
         return;
      
      BDEBUG_ASSERT(p != ALL_SET_BUCKET_PTR);
      mBucketFreeList.release(p);
   }
};

#undef ALL_SET_BUCKET_PTR



