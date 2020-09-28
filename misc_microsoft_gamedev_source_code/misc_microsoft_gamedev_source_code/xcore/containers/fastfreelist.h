//==============================================================================
// fastfreelist.h
//
// Copyright (c) 2005-2007, Ensemble Studios
//==============================================================================
#pragma once

#if 0
typedef int BFreeListValueHandle;
enum { cInvalidFreeListValueHandle = -1 };

// ------------------------------------------------------------------------------------------------------------------------
// class BFastFreeList
// rg [1/20/06] - Untested!
// ------------------------------------------------------------------------------------------------------------------------
template<class ValueType, bool UseConstructorDestructor = true, template<class, uint> class Allocator = BDynamicArrayDefaultAllocator> 
class BFastFreeList
{
public:
   BFastFreeList(uint size = 0) : 
      mNumFree(0),
      mHighWaterMarkIndex(0),
      mFirstFreeIndex(-1)
   {
      BCOMPILETIMEASSERT(sizeof(ValueType) >= sizeof(DWORD));
      
      if (size)
         setSize(size);
   }
   
   void setSize(uint size)
   {
      freeAll();
      
      mData.resize(size);
   }

   void clear(void)
   {
      freeAll();
      
      mData.clear();
   }
   
   void freeAll(void)
   {  
      mNumFree = 0;
      mHighWaterMarkIndex = 0;
      mFirstFreeIndex = -1;
   }
      
   uint getNumFree(void) const { return (mData.getSize() - mHighWaterMarkIndex) + mNumFree; }
   uint getNumAllocated(void) const { return mData.getSize() - getNumFree(); }
   
   const ValueType& getValue(BFreeListValueHandle index) const { return mData[index]; }
         ValueType& getValue(BFreeListValueHandle index)       { return mData[index]; }
         
   BFreeListValueHandle getHandle(ValueType* pVal) const
   {
      if (mData.empty())
         return cInvalidFreeListValueHandle;
      
      if ((pVal < mData.getPtr()) || (pVal >= mData.getPtr() + mData.getSize()))
         return cInvalidFreeListValueHandle;
      
      return static_cast<BFreeListValueHandle>(pVal - mData.getPtr());
   }         

   BFreeListValueHandle alloc(void)
   {
      uint slotIndex = mHighWaterMarkIndex;
      
      if (mNumFree)
      {
         BDEBUG_ASSERT(mFirstFreeIndex != -1);
         slotIndex = mFirstFreeIndex;
         BDEBUG_ASSERT(slotIndex < mHighWaterMarkIndex);
         
         mFirstFreeIndex = getLink(mFirstFreeIndex);
         
         if (mFirstFreeIndex != -1)
         {
            BDEBUG_ASSERT((uint)mFirstFreeIndex != slotIndex);
         }

         mNumFree--;
      }      
      else
      {
         if (mHighWaterMarkIndex == mData.getSize())
         {
            return cInvalidFreeListValueHandle;
         }

         mHighWaterMarkIndex++;
      }
      
      if (UseConstructorDestructor)
         Utils::ConstructInPlace(&mData[slotIndex]);
      
      return static_cast<BFreeListValueHandle>(slotIndex);
   }

   void free(BFreeListValueHandle index)
   {
      if (index == cInvalidFreeListValueHandle)
         return;
      
      BDEBUG_ASSERT((index < mData.getSize()) && (index < mHighWaterMarkIndex));
      
      if (UseConstructorDestructor)
         Utils::DestructInPlace(&mData[index]);
      
      if (index == (mHighWaterMarkIndex - 1))
         mHighWaterMarkIndex--;
      else
      {
         setLink(index, mFirstFreeIndex);
         mFirstFreeIndex = index;

         mNumFree++;
      }         
   }

protected:
   template<class ValueType, uint Alignment>
   struct BDynamicArrayOptions : BDynamicArrayDefaultOptions<ValueType, Alignment>
   {
      enum 
      {
         cUseConstructorDestructor  = false, 
         cGrowable                  = false, 
         cClearNewObjects           = false
      };
   };
   
   BDynamicArray<ValueType, ALIGN_OF(ValueType), Allocator, BDynamicArrayOptions> mData;
   
   uint mHighWaterMarkIndex;
   int mFirstFreeIndex;
   uint mNumFree;
   
   void setLink(uint index, int link) { *reinterpret_cast<int*>(&mData[index]) = link; }
   int getLink(uint index) const { return *reinterpret_cast<const int*>(&mData[index]); }
};

#include "fastfreelist.inl"
#endif