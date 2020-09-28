// File: dynamicArray.cpp
#include "xcore.h"
#include "math\random.h"
#include "consoleOutput.h"

uint gDynamicArrayDummy;

static inline void* getRaw(void* pArrayPtr, uint64 arrayAttributes)
{
   arrayAttributes;
   return *(void**)(pArrayPtr);
}

static inline void setRaw(void* pArrayPtr, uint64 arrayAttributes, void* pNewRaw)
{
   arrayAttributes;
   *(void**)(pArrayPtr) = pNewRaw;
}

static inline void* getAligned(void* pArrayPtr, uint64 arrayAttributes)
{
   return ((void**)(pArrayPtr))[(arrayAttributes >> 26U) & 1U];
}

static inline void setAligned(void* pArrayPtr, uint64 arrayAttributes, void* pNewAligned)
{
   if ((arrayAttributes >> 26U) & 1U)
   {
      ((void**)(pArrayPtr))[1] = pNewAligned;
   }
}

static inline uint getSize(void* pArrayPtr, uint64 arrayAttributes)
{
   uint ofs = (uint)((arrayAttributes >> 27U) & 31U);
   if (ofs < 2)
      ofs = 0;
      
   ofs += ((arrayAttributes >> 26U) & 1U) ? sizeof(uint)*2 : sizeof(uint);
      
   if ((arrayAttributes >> 25U) & 1U)
      return *(uint*)((BYTE*)pArrayPtr + ofs);
   else
      return *(uint16*)((BYTE*)pArrayPtr + ofs);
}

static inline void setSize(void* pArrayPtr, uint64 arrayAttributes, uint newSize)
{
   uint ofs = (uint)((arrayAttributes >> 27U) & 31U);
   if (ofs < 2)
      ofs = 0;

   ofs += ((arrayAttributes >> 26U) & 1U) ? sizeof(uint)*2 : sizeof(uint);

   if ((arrayAttributes >> 25U) & 1U)
      *(uint*)((BYTE*)pArrayPtr + ofs) = newSize;
   else
      *(uint16*)((BYTE*)pArrayPtr + ofs) = (uint16)newSize;
}

static inline uint getCapacity(void* pArrayPtr, uint64 arrayAttributes)
{
   uint ofs = (uint)((arrayAttributes >> 27U) & 31U);
   if (ofs < 2)
      ofs = 0;

   ofs += ((arrayAttributes >> 26U) & 1U) ? sizeof(uint)*2 : sizeof(uint);
   ofs += ((arrayAttributes >> 25U) & 1U) ? sizeof(uint) : sizeof(uint16);
      
   if ((arrayAttributes >> 25U) & 1U)
      return *(uint*)((BYTE*)pArrayPtr + ofs);
   else
      return *(uint16*)((BYTE*)pArrayPtr + ofs);   
}

static inline void setCapacity(void* pArrayPtr, uint64 arrayAttributes, uint newCapacity)
{
   uint ofs = (uint)((arrayAttributes >> 27U) & 31U);
   if (ofs < 2)
      ofs = 0;

   ofs += ((arrayAttributes >> 26U) & 1U) ? sizeof(uint)*2 : sizeof(uint);
   ofs += ((arrayAttributes >> 25U) & 1U) ? sizeof(uint) : sizeof(uint16);

   if ((arrayAttributes >> 25U) & 1U)
      *(uint*)((BYTE*)pArrayPtr + ofs) = newCapacity;
   else
      *(uint16*)((BYTE*)pArrayPtr + ofs) = (uint16)newCapacity;
}

static inline uint getSizeMaxValue(uint64 arrayAttributes)
{
   return ((arrayAttributes >> 25U) & 1U) ? INT_MAX : USHRT_MAX;
}

static inline void debugCheck(uint64 arrayAttributes)
{
   const uint sizeofArray = (uint)(arrayAttributes >> 40U); 
   sizeofArray;
   
   uint ofs = (uint)((arrayAttributes >> 27U) & 31U);
   if (ofs < 2)
      ofs = 0;

   ofs += ((arrayAttributes >> 26U) & 1U) ? sizeof(uint)*2 : sizeof(uint);
   ofs += ((arrayAttributes >> 25U) & 1U) ? sizeof(uint)*2 : sizeof(uint16)*2;
   
   BASSERT(ofs == sizeofArray);
}

void DynamicArrayChangeCapacity(
   void* pArrayPtr, uint64 arrayAttributes, BDynamicArrayAllocCallback pAllocFunc, BDynamicArrayEntryCallback pEntryFunc,
   uint newCapacity, bool tryResizing, uint minNewCapacity)
{
   BASSERT(newCapacity <= getSizeMaxValue(arrayAttributes));

   const uint valueTypeSize = (uint)(arrayAttributes & 0xFFFFFU);
   const uint alignment = (uint)(arrayAttributes >> 32U) & 255U;
   const BOOL canResizeBuf = (BOOL)((arrayAttributes >> 23U) & 1U);
   const uint sizeofArray = (uint)(arrayAttributes >> 40U); 
   sizeofArray;

#ifdef BUILD_DEBUG  
   BDEBUG_ASSERT(sizeofArray && valueTypeSize);
   BDEBUG_ASSERT(Math::IsPow2(alignment));
   BDEBUG_ASSERT( getCapacity(pArrayPtr, arrayAttributes) >= getSize(pArrayPtr, arrayAttributes) );
   BDEBUG_ASSERT( getRaw(pArrayPtr, arrayAttributes) <= getAligned(pArrayPtr, arrayAttributes) );
   debugCheck(arrayAttributes);
#endif   

   BDEBUG_ASSERT( ((uint64)newCapacity * sizeofArray) <= 0x7FFFFFFF);

   tryResizing;

   BDEBUG_ASSERT(getSize(pArrayPtr, arrayAttributes) <= getCapacity(pArrayPtr, arrayAttributes));

   if (newCapacity == getCapacity(pArrayPtr, arrayAttributes))
      return;

   if (!newCapacity)
   {
      (*pEntryFunc)(pArrayPtr, cDADestruct, 0, getSize(pArrayPtr, arrayAttributes), NULL, NULL);
      
      (*pAllocFunc)(pArrayPtr, cDAFree, getRaw(pArrayPtr, arrayAttributes), getCapacity(pArrayPtr, arrayAttributes), 0, 0, NULL);
      
      setRaw(pArrayPtr, arrayAttributes, NULL);
      setAligned(pArrayPtr, arrayAttributes, NULL);
      setCapacity(pArrayPtr, arrayAttributes, 0);
      setSize(pArrayPtr, arrayAttributes, 0);
      
      return;
   }

   if (newCapacity < getSize(pArrayPtr, arrayAttributes))
   {
      (*pEntryFunc)(pArrayPtr, cDADestruct, newCapacity, getSize(pArrayPtr, arrayAttributes) - newCapacity, NULL, NULL);
      setSize(pArrayPtr, arrayAttributes, newCapacity);
   }

   if (!minNewCapacity)
      minNewCapacity = newCapacity;

   void* pNewRaw = NULL;
   void* pNewAligned = NULL;
   uint actualNewCapacity = 0;

   // First try to resize the existing block.
   if (getRaw(pArrayPtr, arrayAttributes))
   {
      if ((canResizeBuf) && (tryResizing))  
      {
         uint newBlockSize;
         pNewRaw = (*pAllocFunc)(pArrayPtr, cDAResize, getRaw(pArrayPtr, arrayAttributes), getCapacity(pArrayPtr, arrayAttributes), newCapacity, minNewCapacity, &newBlockSize);

         if (pNewRaw)
         {
            pNewAligned = reinterpret_cast<void*>(Utils::AlignUp(pNewRaw, alignment));
            const uint bytesNeededToAlignUp = reinterpret_cast<uchar*>(pNewAligned) - reinterpret_cast<uchar*>(pNewRaw);
            BDEBUG_ASSERT(newBlockSize > bytesNeededToAlignUp);

            actualNewCapacity = (newBlockSize - bytesNeededToAlignUp) / valueTypeSize;
            actualNewCapacity = Math::Min(actualNewCapacity, getSizeMaxValue(arrayAttributes));

            if ( 
               (actualNewCapacity >= newCapacity) || 
               ((newCapacity > getCapacity(pArrayPtr, arrayAttributes)) && (actualNewCapacity > getCapacity(pArrayPtr, arrayAttributes)) && (actualNewCapacity >= minNewCapacity)) 
               )
            {
               if (pNewRaw == getRaw(pArrayPtr, arrayAttributes))
               {
                  // Block resized in-place.
                  BDEBUG_ASSERT(pNewAligned == getAligned(pArrayPtr, arrayAttributes));   
                  BDEBUG_ASSERT(actualNewCapacity >= getSize(pArrayPtr, arrayAttributes));
                  setCapacity(pArrayPtr, arrayAttributes, actualNewCapacity);
                  return;   
               }
            }
            else
            {
               // Block isn't big enough.
               if (pNewRaw != getRaw(pArrayPtr, arrayAttributes))
               {
                  BFATAL_FAIL("BDynamicArray::changeCapacity: resizeBuf() didn't return a big enough block");
               }
               else
               {
                  // No need to free pNewRaw here, it will be freed below because pNewRaw equals mpRaw.
               }

               pNewRaw = NULL;
               pNewAligned = NULL;
               actualNewCapacity = 0;
            }
         }
      }
   }         

   if (!pNewRaw)
   {
      // Allocate new block
      // Overflow check
      BASSERT(newCapacity <= (UINT_MAX - alignment + 1)/valueTypeSize);  

      pNewRaw = (*pAllocFunc)(pArrayPtr, cDAAlloc, NULL, 0, newCapacity, 0, NULL);
      if (!pNewRaw)
      {
         BFATAL_FAIL("BDynamicArray::changeCapacity: Out of memory (allocBuf() failed)");
      }

      pNewAligned = reinterpret_cast<void*>(Utils::AlignUp(pNewRaw, alignment));

      actualNewCapacity = newCapacity;
   }         

   if (getSize(pArrayPtr, arrayAttributes))
   {
      BDEBUG_ASSERT(getSize(pArrayPtr, arrayAttributes) <= newCapacity);

      if (!getAligned(pArrayPtr, arrayAttributes))
      {
         // Initialize new objects.
         (*pEntryFunc)(pArrayPtr, cDAConstruct, 0, getSize(pArrayPtr, arrayAttributes), pNewAligned, NULL);
      }
      else
      {
         // Migrate existing objects into new block.
         (*pEntryFunc)(pArrayPtr, cDAMove, 0, getSize(pArrayPtr, arrayAttributes), pNewAligned, getAligned(pArrayPtr, arrayAttributes));
      }
   }

   (*pAllocFunc)(pArrayPtr, cDAFree, getRaw(pArrayPtr, arrayAttributes), getCapacity(pArrayPtr, arrayAttributes), 0, 0, NULL);

   setRaw(pArrayPtr, arrayAttributes, pNewRaw);

   setAligned(pArrayPtr, arrayAttributes, pNewAligned);          

   setCapacity(pArrayPtr, arrayAttributes, actualNewCapacity); 
}

// numNewEntries is the number of objects to be added, relative to mSize, not mCapacity.
// The capacity will be increased as needed for at least numNewEntries.
void DynamicArrayMakeRoom(
   void* pArrayPtr, uint64 arrayAttributes, BDynamicArrayAllocCallback pAllocFunc, BDynamicArrayEntryCallback pEntryFunc,
   uint numNewEntries, bool growableCheck)
{
   const uint valueTypeSize = (uint)(arrayAttributes & 0xFFFFFU);
   const BOOL growable = (BOOL)((arrayAttributes >> 24U) & 1U);
   
   BDEBUG_ASSERT(getSize(pArrayPtr, arrayAttributes) <= getCapacity(pArrayPtr, arrayAttributes));
   uint roomLeft = getCapacity(pArrayPtr, arrayAttributes) - getSize(pArrayPtr, arrayAttributes);

   if (numNewEntries > roomLeft)
   {
      if ((growableCheck) && (!growable))
      {
         BFATAL_FAIL("BDynamicArray::changeCapacity: Attempt to grow an ungrowable array!");
      }

      const uint minCapacity = Math::Max<uint>(1, 4 / valueTypeSize);
      BVERIFY(getCapacity(pArrayPtr, arrayAttributes) < 0x3FFFFFFF);
      uint newCapacity = Math::Max3(getCapacity(pArrayPtr, arrayAttributes) * 2, minCapacity, getSize(pArrayPtr, arrayAttributes) + numNewEntries);

      newCapacity = Math::Min(newCapacity, getSizeMaxValue(arrayAttributes));

      // First try taking the largest available block by resizing in-place.
      DynamicArrayChangeCapacity(pArrayPtr, arrayAttributes, pAllocFunc, pEntryFunc, newCapacity, true, getSize(pArrayPtr, arrayAttributes) + numNewEntries);
      roomLeft = getCapacity(pArrayPtr, arrayAttributes) - getSize(pArrayPtr, arrayAttributes);

      if (numNewEntries > roomLeft)
      {
         // In-place resize failed, acquire new block.
         DynamicArrayChangeCapacity(pArrayPtr, arrayAttributes, pAllocFunc, pEntryFunc, newCapacity, false);
         roomLeft = getCapacity(pArrayPtr, arrayAttributes) - getSize(pArrayPtr, arrayAttributes);
      }

      if (numNewEntries > roomLeft)
      {
         BFATAL_FAIL("BDynamicArray::changeCapacity: Grow failed");
      }
   }
}

void DynamicArrayResize(
   void* pArrayPtr, uint64 arrayAttributes, BDynamicArrayAllocCallback pAllocFunc, BDynamicArrayEntryCallback pEntryFunc,
   uint newSize, bool growableCheck) 
{
   const uint valueTypeSize = (uint)(arrayAttributes & 0xFFFFFU);
   BDEBUG_ASSERT( ((uint64)newSize * valueTypeSize) <= 0x7FFFFFFF);

#ifdef BUILD_DEBUG   
   debugCheck(arrayAttributes);
#endif   

   uint curSize = getSize(pArrayPtr, arrayAttributes);
   if (newSize < curSize)
   {
      // Caller wants to decrease the number of live objects. Call the destructors on those objects going away.
      (*pEntryFunc)(pArrayPtr, cDADestruct, newSize, curSize - newSize, NULL, NULL);
   }
   else if (newSize > curSize)
   {
      // Caller wants to increase the number of live objects - see if capacity is high enough.
      if (newSize > getCapacity(pArrayPtr, arrayAttributes))
      {
         // Make room for new objects by growing the container's capacity.
         const uint numNewEntries = newSize - curSize;            
         DynamicArrayMakeRoom(pArrayPtr, arrayAttributes, pAllocFunc, pEntryFunc, numNewEntries, growableCheck);

         curSize = getSize(pArrayPtr, arrayAttributes);
         
         BDEBUG_ASSERT(getCapacity(pArrayPtr, arrayAttributes) >= newSize);
      }
      
      // Construct new objects starting at the end.
      (*pEntryFunc)(pArrayPtr, cDAConstruct, 0, newSize - curSize, (BYTE*)getAligned(pArrayPtr, arrayAttributes) + curSize * valueTypeSize, NULL);
   }

   setSize(pArrayPtr, arrayAttributes, newSize);
}

#if DYNAMIC_ARRAY_TRACKING
void BDynamicArrayTracker::getStatistics(BStats& stats)
{
   getTrackerListMutex().lock();
   
   const BDynamicArrayTrackerList& objectList = getTrackerList();
   
   stats.mTotalArrays = objectList.getSize();
         
   stats.mTotalSize = 0;
   stats.mTotalSizeInBytes = 0;
   
   stats.mTotalCapacity = 0;
   stats.mTotalCapacityInBytes = 0;
   
   for (BDynamicArrayTrackerList::constIterator it = objectList.begin(); it != objectList.end(); ++it)
   {
      const BDynamicArrayTracker* ppTracker = *it;
   
      uint typeSize, size, capacity;
      void* pAllocation;
      if (!ppTracker->getState(typeSize, size, capacity, pAllocation))
         continue;
      
      uint sizeInBytes = typeSize * size;
      uint capacityInBytes = typeSize * capacity;
      
      stats.mTotalSize            += size;
      stats.mTotalSizeInBytes     += sizeInBytes;
      stats.mTotalCapacity        += capacity;
      stats.mTotalCapacityInBytes += capacityInBytes;
   }
   
   getTrackerListMutex().unlock();
}
#endif


#if 0
void testDynamicArray(void)
{
   Random rand;

   typedef BDynamicArray<DWORD, 4, BDynamicArrayVirtualAllocator> Array;
   BDynamicArray<Array, 4, BDynamicArrayWin32HeapAllocator> jaggedArray;

   const uint cNumArrays = 1024;
   jaggedArray.resize(cNumArrays);
   for ( ; ; )
   {
      static bool blah;
      if (blah)
      {
         dumpMemoryMap();
         blah = false;
      }

      uint arrayIndex = rand.iRand(0, cNumArrays);

      Array& array = jaggedArray[arrayIndex];

      if (rand.iRand(0, 1000) < 50)
      {
         array.clear();

         if (rand.iRand(0, 1000) < 200)
            array.virtualReserve(rand.iRand(1, 10000));
      }

      if (rand.iRand(0, 1000) < 10)
      {
         array.virtualReserve(0);
      }

      uint op = rand.iRand(0, 5);
      switch (op)
      {
         case 0:
         {
            uint n = rand.iRand(1, 10);
            for (uint i = 0; i < n; i++)
               array.pushBack(rand.iRand(0, INT_MAX));
            break;
         }            
         case 1:
         {
            array.resize(rand.iRand(0, 50000));
            break;
         }
         case 2:
         {
            array.reserve(rand.iRand(0, 50000));
            break;
         }
         case 3:
         {
            if (array.size())
               array.erase(rand.iRand(0, array.size()));
            break;
         }
         case 4:
         {
            array.insert(rand.iRand(0, array.size()+1), (DWORD)rand.iRand(0, INT_MAX));
            break;
         }
      }
   }
}
#endif
