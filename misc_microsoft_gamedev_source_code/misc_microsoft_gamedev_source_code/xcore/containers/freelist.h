//==============================================================================
// File: freelist.h
//
// Copyright (c) 2005-2007, Ensemble Studios
// 
// Internally, the BFreeList class is a "greedy" HAT (Hashed Array Tree)
// template container/allocator with an allocation bitmap. (See
// http://www.ddj.com/184409965?pgno=5 for HAT info.) Here's the latest
// template definition:
// 
// template<class ValueType, uint GroupSizeLog2 = 4> class BFreeList
// 
// BFreeList immediately constructs objects as soon as memory is allocated
// to hold the object. Objects will only be destructed when the BFreeList
// goes out of scope, or if clear() is called, which also frees all memory
// associated with the container. (That's why it's a "greedy" container --
// once it allocates memory to hold an object, it keeps that memory forever
// until the user specifically frees it by calling clear() or by deleting
// the freelist.)
// 
// Here are the important things to know about this class:
// 
// - BFreeList is not thread safe. If you need a thread safe freelist,
// you'll need to wrap it and sync yourself.
// 
// - You can control the heap the freelist class uses for all allocations
// by specifying it in one of the constructors, or by calling setHeap()
// when the freelist is empty. It defaults to the primary heap
// (gPrimaryHeap). If all operations on a freelist will be on the sim
// thread, you should use the sim heap (gSimHeap) for a speed boost.
// 
// - Memory is allocated in chunks of 2^GroupSizeLog2 objects as the
// container grows. So if your object size is 16 bytes, and GroupSizeLog2
// is 4, all memory allocations will be in chunks of 16*16=256 bytes.
// 
// - The init() method accepts an initialSize parameter. This controls the
// number of objects you want to be immediately allocated+constructed. The
// container can still grow as needed. (Note that the objects are
// internally allocated and constructed when init() is called, but you
// still must call acquire() to "check out" objects.)
// 
// - Objects are never moved once they are constructed. So you can call
// acquire() and safely use the returned pointer even as the container
// grows. Once you call clear(), your pointers will be invalid.
// 
// - The class efficiently supports accessing objects by pointers or
// indices. For example, if you know you'll never allocate more than 64k
// objects from a freelist, you can use uint16 indices to refer to acquired
// objects to save memory. The get() and operator[] methods operate in
// constant time. Some things to know:
// 
// Accessing objects by index is slower than by pointer. Converting an
// index to a pointer involves some bit shifting and accessing the "top"
// array of chunk pointers. Releasing objects by pointer is slower than by
// index because the container must resolve the pointer back to an index.
// This involves a scan of the "top" array of chunk pointers to identify
// which chunk the pointer refers to.
// 
// - You can easily iterate over all valid slots in the container and query
// to determine if the slot holds an acquired object or not. See the
// getHighWaterMark() and isInUse() methods.
// 
// - The class now manages the freelist of "unacquired" objects using a
// rover index which tracks the lowest potentially free slot, a highwater
// marker to track the highest ever acquired slot, and (if all else fails)
// a uint64 allocation bitmap. Over time, allocation performance may drop
// as objects are allocated and freed from the container, depending on the
// allocation pattern used. I don't think this will be a problem, but I'll
// be watching this in the future to see how well the algorithm performs.
//==============================================================================
#pragma once

#define FREELIST_MEM_DEBUG 0

#ifdef BUILD_FINAL
   #undef FREELIST_MEM_DEBUG
   #define FREELIST_MEM_DEBUG 0
   
   #undef FREELIST_TRACKING 
   #define FREELIST_TRACKING 0
#endif

#if FREELIST_TRACKING
#pragma warning(disable:4355) // this used in base memory initialize list
#include "containers\linkedList.h"

class BFreeListTracker
{
   BFreeListTracker();
   BFreeListTracker& operator= (const BFreeListTracker&);

public:
   typedef void (*BGetArrayStateCallbackPtr)(void* pFreeList, uint& typeSize, uint& groupSize, uint& numAllocated, uint& numAcquired, uint& totalOperations);

   typedef BFreeListTracker* BFreeListTrackingPtr;
   typedef BLinkedList<BFreeListTrackingPtr> BFreeListTrackerList;

   BFreeListTracker(void* pFreeList, BGetArrayStateCallbackPtr pGetState) :
      mMagic(0),
      mpFreeList(pFreeList),
      mpGetState(pGetState),
      mThreadID(GetCurrentThreadId()),
      mpListEntry(NULL)
   {
   }      

   ~BFreeListTracker()
   {
      BASSERT(!mpListEntry);
   }

   void* getFreeList(void) const { BASSERT(mMagic == cMagic); return mpFreeList; }
   BGetArrayStateCallbackPtr getGetStateCallback(void) const { BASSERT(mMagic == cMagic); return mpGetState; }
   DWORD getThreadID(void) const { BASSERT(mMagic == cMagic); return mThreadID; }

   bool getState(uint& typeSize, uint& groupSize, uint& numAllocated, uint& numAcquired, uint& totalOperations) const
   {
      // This is not thread safe, but it's only for informational purposes!
      __try
      {
         MemoryBarrier();
         
         if (mMagic != cMagic)
            return false;
      
         mpGetState(mpFreeList, typeSize, groupSize, numAllocated, numAcquired, totalOperations);
         
         MemoryBarrier();

         if (mMagic != cMagic)
            return false;
      }
      __except(EXCEPTION_EXECUTE_HANDLER)
      {
         return false;
      }

      return true;
   }

   void add(void)
   {
      BScopedCriticalSection lock(getTrackerListMutex());

      BASSERT(!mpListEntry);

      mMagic = cMagic;

      mpListEntry = getTrackerList().pushBack(this).getItem();
   }

   void remove(void)
   {
      BScopedCriticalSection lock(getTrackerListMutex());

      BASSERT(mpListEntry);

      mMagic = 0;

      getTrackerList().remove(getTrackerList().getIterator(mpListEntry));
      mpListEntry = NULL;
   }

   struct BStats
   {
      uint mTotalFreeLists;

      uint mTotalElementsAllocated;
      uint mTotalElementsAcquired;
      
      uint mTotalBytesAllocated;
      uint mTotalBytesAcquired;
                  
      void clear(void) { Utils::ClearObj(*this); }
   }; 
   
   struct BFreeListDesc
   {
      void* mpFreeList;
      uint mTypeSize;
      uint mGroupSize;
      uint mNumAllocated;
      uint mNumAcquired;
      uint mTotalOperations;
   };
   typedef BDynamicArray<BFreeListDesc> BFreeListDescArray;  
   
   static void getStatistics(BStats& stats, BFreeListDescArray* pFreeLists = NULL);
   
private:
   enum { cMagic = 0x7743E597 };
   DWORD                      mMagic;
   void*                      mpFreeList;
   BGetArrayStateCallbackPtr  mpGetState;
   DWORD                      mThreadID;
   BFreeListTrackerList::valueType* mpListEntry;

   static BFreeListTrackerList& getTrackerList(void)
   {
      static BFreeListTrackerList trackerList;
      return trackerList;
   }

   static BCriticalSection& getTrackerListMutex(void)
   {
      static BCriticalSection mutex;
      return mutex;
   }
};

#endif

//==============================================================================
// class BFreeList
//==============================================================================
template<class ValueType, uint GroupSizeLog2 = 4, bool FreeEmptyGroups = false>
class BFreeList
{
   BFreeList(BFreeList&);
   BFreeList& operator= (const BFreeList&);
   
public:
   enum { cGroupSize = 1U << GroupSizeLog2 };
   enum { cGroupSizeMask = cGroupSize - 1U };

   BFreeList(BMemoryHeap* pHeap = &gPrimaryHeap) :
      mHighWaterMark(0),
      mNumAllocated(0),
      mLowestFreeIndex(0),
      mTotalElements(0),
      mTotalAllocatedGroups(0)
#if FREELIST_TRACKING
      , mTracker(this, getStateCallBack), mTotalOperations(0)
#endif      
   {
#ifndef BUILD_FINAL
      mClearingGroups = false;
#endif      

      setHeap(pHeap);
      
#if FREELIST_TRACKING         
      mTracker.add();
#endif
   }
   
   BFreeList(uint initialSize, BMemoryHeap* pHeap = &gPrimaryHeap) :
      mHighWaterMark(0),
      mNumAllocated(0),
      mLowestFreeIndex(0),
      mTotalElements(0),
      mTotalAllocatedGroups(0)
#if FREELIST_TRACKING
      , mTracker(this, getStateCallBack), mTotalOperations(0)
#endif        
   { 
#ifndef BUILD_FINAL
      mClearingGroups = false;
#endif      

      setHeap(pHeap);
      
      init(initialSize, true);
      
#if FREELIST_TRACKING         
      mTracker.add();
#endif
   }
   
   ~BFreeList() 
   { 
#if FREELIST_TRACKING
      mTracker.remove();
#endif      
   
      clearGroups();
   }
   
   void setHeap(BMemoryHeap* pHeap) 
   {
      if (!pHeap)
         pHeap = &gPrimaryHeap;
      
      clear();
                              
      BASSERT(!mGroupPtrs.getCapacity());
      mGroupPtrs.getAllocator().setHeap(pHeap);
      
      BASSERT(!mInUse.getCapacity());
      mInUse.getAllocator().setHeap(pHeap);
   }
   
   BMemoryHeap* getHeap(void) const { return mGroupPtrs.getAllocator().getHeap(); }

   // initialSize is the initial allocation. The container can still grow larger if needed later on.
   bool init(uint initialSize = 0, bool fatalOnFailure = false)
   {
      clear();

      if (!addElements(initialSize))
      {
         clear();
         
         if (fatalOnFailure)
         {
            BFATAL_FAIL("BFreeList: Out of memory!");
         }
         
         return false;
      }
                        
      return true;
   }
   
   // This completely resets the freelist, freeing all allocations.
   // Use releaseAll() if you want to only release all objects without freeing all allocations.
   void clear(void)
   {
      clearGroups();
      
      mInUse.clear();

      mHighWaterMark = 0;
      mNumAllocated = 0;
      mLowestFreeIndex = 0;
      
      BASSERT(!mTotalAllocatedGroups);
   }
   
   // The total number of elements allocated from the heap.
   // Note: If FreeEmptyGroups is true, for the sake of compatibility this reflects the maximum 
   // possible number of elements that could be currently allocated from the heap.
   uint getSize(void) const { return mTotalElements; }
      
   // The total number of acquired elements (i.e. allocated by the user).
   uint getNumberAllocated(void) const { return mNumAllocated; }
   
   uint getNumberFree(void) const { return mTotalElements - mNumAllocated; }

   // The high water mark is the maximum number of valid indices that have been returned to the caller. 
   // Any index >= the high water mark must be invalid.
   uint getHighWaterMark(void) const { return mHighWaterMark; }
   
   // Returns the total number of allocated groups.
   uint getTotalAllocatedGroups(void) const { return mTotalAllocatedGroups; } 
      
   bool getIndex(const ValueType* pValue, uint& index) const
   {
      index = 0;
      
      if ((!pValue) || (!mNumAllocated))
         return false;
      
      uint groupIndex;
      for (groupIndex = 0; groupIndex < mGroupPtrs.getSize(); groupIndex++)
      {
         if (FreeEmptyGroups)
         {
            if (!mGroupPtrs[groupIndex])
               continue;
         }
            
         if ((pValue >= mGroupPtrs[groupIndex]) && (pValue < (mGroupPtrs[groupIndex] + cGroupSize)))
            break;
      }
      
      if (groupIndex == mGroupPtrs.getSize())
         return false;
         
      index = (groupIndex << GroupSizeLog2) + (pValue - mGroupPtrs[groupIndex]);
      
      if ((index >= mHighWaterMark) || (!isInUse(index)))
         return false;
      
      // rg [5/18/07] - If this assert fires, either your pointer is bogus or this function is buggy.
      BASSERT(getConst(index) == pValue);
      
      return true;
   }
      
   // If the allocation fails, index will be UINT_MAX and the returned pointer will be NULL.
   // Unlike the original freelist class, the allocation will not move as the container grows, 
   // so you can safely keep the pointer around even as additional objects are allocated.
   ValueType* acquire(uint& index, bool fatalOnFailure = false)
   {
      // Can't acquire new objects while the freelist is being cleared!
      BASSERT(!mClearingGroups);
      BASSERT(mHighWaterMark <= mTotalElements);
      BASSERT(mLowestFreeIndex <= mHighWaterMark);

      if (mLowestFreeIndex == mHighWaterMark)
      {
         if (mHighWaterMark == mTotalElements)
         {
            BASSERT(mNumAllocated == mTotalElements);
            
            if (!addElements(1))
            {
               if (fatalOnFailure)
               {
                  BFATAL_FAIL("Freelist allocation failed");
               }
               
               index = UINT_MAX;
               return NULL;
            }
         }

         index = mLowestFreeIndex;
         mLowestFreeIndex++;
      }
      else 
      {
         BASSERT(mLowestFreeIndex < mTotalElements);
         BASSERT(mTotalElements > 0);

         const uint bitMapSize = (mTotalElements + 63U) >> 6U;
                           
         uint ofs = mLowestFreeIndex >> 6U;
         while (ofs < bitMapSize)
         {
            if (mInUse[ofs] != UINT64_MAX)
               break;
               
            ofs++;
         }
         
         index = ofs << 6U;
         
         if (ofs < bitMapSize)
         {
            const uint64 val = ~mInUse[ofs];
            const uint lzc = Utils::CountLeadingZeros64(val);
            index += lzc;
         }
         
         if (index >= mTotalElements)
         {
            BASSERT(mHighWaterMark == mTotalElements);
            BASSERT(mNumAllocated == mTotalElements);
            
            if (!addElements(1))
            {
               if (fatalOnFailure)
               {
                  BFATAL_FAIL("Freelist allocation failed");
               }

               index = UINT_MAX;
               return NULL;
            }
                        
            index = mHighWaterMark;
         }
         
         mLowestFreeIndex = index + 1;
      }
      
      BASSERT(!isInUse(index));
      
      if (FreeEmptyGroups)
      {
         const uint groupIndex = index >> GroupSizeLog2;
         if (!mGroupPtrs[groupIndex])
         {
            if (NULL == (mGroupPtrs[groupIndex] = createGroup()))
            {
               if (fatalOnFailure)
               {
                  BFATAL_FAIL("Freelist allocation failed");
               }

               mLowestFreeIndex = index;
               
               index = UINT_MAX;
               return NULL;
            }
         }
      }
      
      mHighWaterMark = Math::Max(mHighWaterMark, index + 1);
      BASSERT(mLowestFreeIndex <= mHighWaterMark);
      BASSERT(mHighWaterMark <= mTotalElements);
      
      mNumAllocated++;
      BASSERT(mNumAllocated <= mTotalElements);
                  
      setInUse(index, TRUE);

#if FREELIST_TRACKING      
      mTotalOperations++;
#endif      
                                    
      return &getElement(index);
   }
   
   ValueType* acquire(bool fatalOnFailure = false)
   {
      uint index;
      return acquire(index, fatalOnFailure);
   }
   
   // Attempt to acquire using a specific index.
   ValueType* acquireAtIndex(uint index)
   {
      // Can't acquire new objects while the freelist is being cleared!
      BASSERT(!mClearingGroups);
      BASSERT(mHighWaterMark <= mTotalElements);
      BASSERT(mLowestFreeIndex <= mHighWaterMark);

      if (index >= mTotalElements)
         return NULL;

      if (isInUse(index))
         return NULL;

      if (index == mLowestFreeIndex)
         mLowestFreeIndex++;

      mHighWaterMark = Math::Max(mHighWaterMark, index + 1);

      mNumAllocated++;
      BASSERT(mNumAllocated <= mTotalElements);
                  
      setInUse(index, TRUE);

#if FREELIST_TRACKING      
      mTotalOperations++;
#endif      

      return &getElement(index);
   }

   void release(uint index)
   {
      BASSERT(mHighWaterMark <= mTotalElements);
      BASSERT(index < mHighWaterMark);
      BASSERT(mNumAllocated > 0);
      BASSERT(isInUse(index));

      mNumAllocated--;

      setInUse(index, FALSE);

      mLowestFreeIndex = Math::Min(mLowestFreeIndex, index);
      
      if (FreeEmptyGroups)
      {
         const uint groupIndex = index >> GroupSizeLog2;
         const uint firstElementIndex = groupIndex << GroupSizeLog2;
                  
         bool isUsedGroup = false;
         for (uint i = 0; i < cGroupSize; i++)
         {  
            const uint elementIndex = firstElementIndex + i;
            
            if ((elementIndex < mTotalElements) && isInUse(elementIndex))
            {
               isUsedGroup = true;
               break;
            }
         }
         
         bool isUsedGroup2;
         if (GroupSizeLog2 >= 6U)
         {
            const uint first = firstElementIndex >> 6U;
            const uint last = Math::Min<uint>(mInUse.getSize(), first + (cGroupSize >> 6U));
            
            isUsedGroup2 = false;
            for (uint i = first; i < last; i++)
            {
               if (mInUse[i])
               {
                  isUsedGroup2 = true;
                  break;
               }
            }
         }
         else
         {
            const uint first = firstElementIndex >> 6U;
#pragma warning(disable:4293) // warning C4293: '<<' : shift count negative or too big, undefined behavior
            const uint64 bitMask = ( (((uint64)1ULL) << ((uint64)cGroupSize)) - 1ULL) << (64U - cGroupSize - (firstElementIndex & 63U));
#pragma warning(default:4293) 
            
            isUsedGroup2 = (mInUse[first] & bitMask) != 0;
         }
         
         BASSERT(isUsedGroup == isUsedGroup2);
         
         if (!isUsedGroup2)
         {
            clearGroup(groupIndex);
         }               
      }
      
#if FREELIST_TRACKING      
      mTotalOperations++;
#endif      
   }
   
   bool release(ValueType* pValue)
   {
      uint index;
      if (!getIndex(pValue, index))
      {
         BASSERT(false);
         return false;
      }

      release(index);
      return true;
   }
   
   // Quickly releases all acquired objects.
   void releaseAll(void)
   {
      mInUse.setAll(0);
      
      if (FreeEmptyGroups)
      {
#ifndef BUILD_FINAL
         mClearingGroups = true;
#endif      
         for (uint groupIndex = 0; groupIndex < mGroupPtrs.getSize(); groupIndex++)
            clearGroup(groupIndex);
#ifndef BUILD_FINAL
         mClearingGroups = false;
#endif      
         
         BASSERT(!mTotalAllocatedGroups);
      }
      
      mHighWaterMark = 0;
      mNumAllocated = 0;
      mLowestFreeIndex = 0;

#if FREELIST_TRACKING      
      mTotalOperations++;
#endif      
   }

         ValueType* get(uint index)       { BASSERT(isInUse(index)); return &getElement(index); }
   const ValueType* get(uint index) const { BASSERT(isInUse(index)); return &getElement(index); }
   
         ValueType& operator[](uint index)       { return *get(index); }
   const ValueType& operator[](uint index) const { return *get(index); }

   const ValueType* getConst(uint index) const { return get(index); }

   // TRUE if the index refers to an allocated object. 
   // Asserts if index is out of range.
   // Returns BOOL for more efficiency on 360.
   BOOL isInUse(uint index) const 
   { 
      BASSERT(index < mTotalElements);
      return 0U != (mInUse[index >> 6U] & Utils::BBitMasks::get64(63U - (index & 63U)));
   }
   
   // TRUE if the index is valid and refers to an allocated object.
   // Does not assert if index is out of range, returns FALSE instead.
   BOOL isValidIndex(uint index) const 
   { 
      if (index >= mHighWaterMark)
         return FALSE;
      return 0U != (mInUse[index >> 6U] & Utils::BBitMasks::get64(63U - (index & 63U)));
   }
   
   // Intended for debugging/testing - checks the validity of the container.
   bool check(void) const
   {
      if (mHighWaterMark > mTotalElements)
         return false;
      if (mLowestFreeIndex > mHighWaterMark)
         return false;
      
      if (mHighWaterMark == mTotalElements)
      {
         if (mLowestFreeIndex == mHighWaterMark)
         {
            if (mTotalElements != mNumAllocated) 
               return false;
         }
      }
      
      if (mNumAllocated > mHighWaterMark)  
         return false;
            
      if (!mNumAllocated)
      {
         if (mLowestFreeIndex != 0)
            return false;
      }
      
      if ((mInUse.getSize() << 6U) < mTotalElements)
         return false;
      
      for (uint i = 0; i < mLowestFreeIndex; i++)
         if (!isInUse(i))
            return false;
      
      for (uint i = mHighWaterMark; i < mTotalElements; i++)
         if (isInUse(i))
            return false;
            
      uint totalAllocated = 0;
      for (uint i = 0; i < mHighWaterMark; i++)
         if (isInUse(i))
         {
            const uint groupIndex = i >> GroupSizeLog2;
            if (!mGroupPtrs[groupIndex])
               return false;
               
            totalAllocated++;
         }
                 
      if (totalAllocated != mNumAllocated)
         return false;
      
      return true;
   }
   
   const ValueType& getElement(uint index) const { return mGroupPtrs[index >> GroupSizeLog2][index & cGroupSizeMask]; }
   ValueType& getElement(uint index)       { return mGroupPtrs[index >> GroupSizeLog2][index & cGroupSizeMask]; }

protected:
   void setInUse(uint index, BOOL value)
   {
      if (value)
         mInUse[index >> 6U] |= Utils::BBitMasks::get64(63U - (index & 63U));
      else
         mInUse[index >> 6U] &= Utils::BBitMasks::getInverted64(63U - (index & 63U));
   }
   
   ValueType* createGroup(void)
   {
      BASSERT(!mClearingGroups);

      const uint alignment = ALIGN_OF(ValueType);

#if FREELIST_MEM_DEBUG      
      const uint allocSize = Utils::AlignUpValue(sizeof(ValueType) << GroupSizeLog2, 4096);
      ValueType* p = static_cast<ValueType*>(VirtualAlloc(NULL, allocSize, MEM_COMMIT, PAGE_READWRITE));
#else
      const uint allocSize = Utils::AlignUpValue(sizeof(ValueType) << GroupSizeLog2, alignment);
      ValueType* p = static_cast<ValueType*>(getHeap()->New(allocSize, NULL, true));
#endif      
      if (!p)
         return NULL;

      BASSERT(Utils::IsAligned(p, alignment));

      if (!BIsBuiltInType<ValueType>::Flag) 
         Utils::ConstructArrayInPlace(p, cGroupSize);
      else
         Utils::FastMemSet(p, 0, allocSize);

      mTotalAllocatedGroups++;
      
      return p;         
   }   
   
   void clearGroup(uint groupIndex)
   {
      if (!mGroupPtrs[groupIndex])
         return;

      BASSERT(mTotalAllocatedGroups > 0);
      mTotalAllocatedGroups--;
               
      if (!BIsBuiltInType<ValueType>::Flag) 
      {
         for (uint j = 0; j < cGroupSize; j++)
         {
            Utils::DestructInPlace(mGroupPtrs[groupIndex] + j);

#ifdef BUILD_DEBUG               
            // Clear object in case the user tries to access it through a dangling pointer.
            Utils::FastMemSet(mGroupPtrs[groupIndex] + j, 0xFE, sizeof(ValueType));
#endif               

            const uint entryIndex = (groupIndex << GroupSizeLog2) + j;

            if (entryIndex < (mInUse.getSize() << 6U))
               setInUse(entryIndex, FALSE);
         }
      }

#if FREELIST_MEM_DEBUG
      const uint allocSize = Utils::AlignUpValue(sizeof(ValueType) << GroupSizeLog2, 4096);
      DWORD oldProtect;
      BOOL success = VirtualProtect(mGroupPtrs[groupIndex], allocSize, PAGE_NOACCESS, &oldProtect);
      BASSERT(success);
#else  
      bool success = getHeap()->Delete(mGroupPtrs[groupIndex]);
      success;
      BASSERT(success);
#endif
      
      mGroupPtrs[groupIndex] = NULL;
   }
            
   bool addGroup(void)
   {
      ValueType* p = createGroup();
      if (!p)
         return false;
      
      mGroupPtrs.pushBack(p);
            
      return true;      
   }
   
   bool addElements(uint num)
   {
      BASSERT(!mClearingGroups);
      
      if (!num)
         return true;
         
      mTotalElements += num;
      
      if (mTotalElements >= (mInUse.getSize() << 6U))
      {
         uint newBits = Math::Max3<uint>(mTotalElements, 128U, (mInUse.getSize() << 6U) * 2);

         mInUse.resize((newBits + 63U) >> 6U);
      }
            
      const uint newTotalGroups = (mTotalElements + cGroupSize - 1) >> GroupSizeLog2;
            
      const int newNumGroups = newTotalGroups - mGroupPtrs.getSize();
      
      for (int i = 0; i < newNumGroups; i++)
      {
         if (!addGroup())
         {
            mTotalElements = mGroupPtrs.getSize() << GroupSizeLog2;
            return false;
         }
      }
                  
      BASSERT((mGroupPtrs.getSize() << GroupSizeLog2) >= mTotalElements);
            
      return true;      
   }
      
   void clearGroups(void)
   {
      BASSERT(getHeap());
      BASSERT(!mClearingGroups);

      // Must be very careful here, because the ValueType's destructor could try to manipulate the freelist!

#ifndef BUILD_FINAL
      mClearingGroups = true;
#endif      

      for (uint groupIndex = 0; groupIndex < mGroupPtrs.getSize(); groupIndex++)
         clearGroup(groupIndex);

      mGroupPtrs.clear();

      mTotalElements = 0;

#ifndef BUILD_FINAL
      mClearingGroups = false;
#endif      
   }
                    
   // Freelist management
   BDynamicArray<uint64, sizeof(uint64), BDynamicArrayHeapAllocator>          mInUse;           // Bit array indicating which objects have been acquired.
   
   uint                                                                       mHighWaterMark;   // The total number of valid indices ever exposed to the user.
   uint                                                                       mNumAllocated;    // The total number of acquired objects.
   uint                                                                       mLowestFreeIndex; // Index of lowest potentially free object.
            
   // Group management
   BDynamicArray<ValueType*, sizeof(ValueType*), BDynamicArrayHeapAllocator>  mGroupPtrs;       // Array of group pointers.
   uint                                                                       mTotalElements;   // The current maximum number of acquirable objects.
   uint                                                                       mTotalAllocatedGroups; // The total number of allocated groups.

#ifndef BUILD_FINAL   
   bool                                                                       mClearingGroups : 1;
#endif   

#if FREELIST_TRACKING
   BFreeListTracker mTracker;
   uint mTotalOperations;

   static void getStateCallBack(void* pFreeList, uint& typeSize, uint& groupSize, uint& numAllocated, uint& numAcquired, uint& totalOperations)
   {
      const BFreeList* p = static_cast<const BFreeList*>(pFreeList);
      
      typeSize = sizeof(ValueType);
      groupSize = cGroupSize;
      numAllocated = p->mTotalAllocatedGroups << GroupSizeLog2;
      numAcquired = p->mNumAllocated;
      totalOperations = p->mTotalOperations;
   }
#endif 
};

//==============================================================================
// Helper declaration for class-based free list (custom parameter version)
//==============================================================================
#define DECLARE_FREELIST(ValueType, GroupSizeLog2)                   \
static ValueType* getInstance();                                     \
static void releaseInstance(ValueType* pObject);                     \
static uint getNumAllocatedInstances();                              \
static BFreeList<ValueType, GroupSizeLog2> mFreeList;                \

//==============================================================================
// Helper implementation for class-based free list (custom parameter version)
//==============================================================================
#define IMPLEMENT_FREELIST(ValueType, GroupSizeLog2, Heap)           \
BFreeList<ValueType, GroupSizeLog2> ValueType::mFreeList(Heap);      \
ValueType* ValueType::getInstance()                                  \
{                                                                    \
   ValueType* pObject = mFreeList.acquire(true);                     \
   pObject->onAcquire();                                             \
   return pObject;                                                   \
}                                                                    \
void ValueType::releaseInstance(ValueType* pObject)                  \
{                                                                    \
   if (pObject) {                                                    \
      pObject->onRelease();                                          \
      mFreeList.release(pObject);                                    \
   }                                                                 \
}                                                                    \
uint ValueType::getNumAllocatedInstances()                           \
{                                                                    \
   return mFreeList.getNumberAllocated();                            \
}

//==============================================================================
// Helper implementation for class-based free list (threadsafe custom parameter version)
//==============================================================================
#define IMPLEMENT_THREADSAFE_FREELIST(ValueType, GroupSizeLog2, Heap)           \
   BCriticalSection ValueType##_mFreeListMutex;                          \
BFreeList<ValueType, GroupSizeLog2> ValueType::mFreeList(Heap);      \
ValueType* ValueType::getInstance()                                  \
{                                                                    \
   BScopedCriticalSection lock(ValueType##_mFreeListMutex);           \
   ValueType* pObject = mFreeList.acquire(true);                     \
   pObject->onAcquire();                                             \
   return pObject;                                                   \
}                                                                    \
void ValueType::releaseInstance(ValueType* pObject)                  \
{                                                                    \
   BScopedCriticalSection lock(ValueType##_mFreeListMutex);           \
   if (pObject) {                                                    \
      pObject->onRelease();                                          \
      mFreeList.release(pObject);                                    \
   }                                                                 \
}                                                                    \
uint ValueType::getNumAllocatedInstances()                           \
{                                                                    \
   BScopedCriticalSection lock(ValueType##_mFreeListMutex);           \
   return mFreeList.getNumberAllocated();                            \
}


#include "freelist.inl"

extern void freelistTest(void);
