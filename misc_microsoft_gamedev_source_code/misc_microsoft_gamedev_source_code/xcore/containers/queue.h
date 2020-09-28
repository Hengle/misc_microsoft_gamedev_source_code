//============================================================================
//
// File: queue.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================

#pragma once

//-----------------------------------------------------------------------------
// class BQueue
//-----------------------------------------------------------------------------
template <class T>
class BQueue
{
public:
   typedef T ElementType;

public:
   BQueue(uint maxEntries = 0, BMemoryHeap* pHeap = &gPrimaryHeap) : 
      mHead(0), 
      mTail(0), 
      mNum(0), 
      mMaxEntries(maxEntries),
      mpBuf(NULL),
      mpHeap(pHeap)
   { 
      BDEBUG_ASSERT((mMaxEntries == 0) || Math::IsPow2(mMaxEntries)); 

      if (mMaxEntries)
      {
         if (mpHeap == &gCRunTimeHeap)
         {
            mpBuf = (T*)malloc(sizeof(T) * mMaxEntries);
            Utils::ConstructArrayInPlace(mpBuf, mMaxEntries);
         }
         else
            mpBuf = HEAP_NEW_ARRAY(T, mMaxEntries, *mpHeap);
      }
   }

   ~BQueue()
   {
      destroyBuf();
   }

   uint getMaxSize(void) const { return mMaxEntries; }
   uint getSize(void) const { return mNum; }
   uint getAvail(void) const { return avail(); }
   bool getEmpty(void) const { return empty(); }
   bool getFull(void) const { return full(); }

   void clear() { mHead = mTail = mNum = 0; }
   
   BMemoryHeap* getHeap() const { return mpHeap; }
   void setHeap(BMemoryHeap* pHeap) { resize(0); mpHeap = pHeap; }

   bool resize(uint newMaxEntries)
   {
      BDEBUG_ASSERT((newMaxEntries == 0) || Math::IsPow2(newMaxEntries)); 
      
      if (newMaxEntries == mMaxEntries)
         return true;
      
      if (mNum > newMaxEntries)
         return false;
         
      const uint origNum = mNum;
      
      T* pBuf = NULL;
      if (newMaxEntries)
      {
         if (mpHeap == &gCRunTimeHeap)
         {
            pBuf = (T*)malloc(sizeof(T) * newMaxEntries);
            Utils::ConstructArrayInPlace(pBuf, newMaxEntries);
         }
         else
            pBuf = HEAP_NEW_ARRAY(T, newMaxEntries, *mpHeap);

         if (origNum)
            popFront(origNum, pBuf);
      }

      destroyBuf();
                     
      mpBuf = pBuf;
      
      mNum = origNum;
      mMaxEntries = newMaxEntries;
      mHead = origNum & (newMaxEntries - 1);
      mTail = 0;

      return true;
   }
   
   const T& peekFront(uint index) const  { BDEBUG_ASSERT(index < getSize()); return mpBuf[wrap(mTail + index)]; }
         T  peekFront(uint index)        { BDEBUG_ASSERT(index < getSize()); return mpBuf[wrap(mTail + index)]; }
   
   // false on failure
   bool pushFront(const T& a)
   {
      bool success = false;

      if (!full())
      {
         mTail = prevWrap(mTail);
         at(mTail) = a;
         mNum++;

         success = true;
      }

      return success;
   }

   // false on failure
   bool pushFront(uint numObjects, const T* pSrcObjects)
   {
      if (!numObjects)
         return true;

      bool success = false;

      if (numObjects <= avail())
      {
         for (uint i = 0; < numObjects; i++)
         {
            mTail = prevWrap(mTail);
            at(mTail) = pSrcObjects[i];
         }

         mNum += numObjects;

         success = true;
      }

      return success;
   }
   
   // false on failure
   bool popFront(void)
   {
      bool success = false;

      if (!empty())
      {
         mTail = nextWrap(mTail);
         mNum--;

         success = true;
      }

      return success;
   }

   // false on failure
   bool popFront(T& obj)
   {
      bool success = false;

      if (!empty())
      {
         obj = mpBuf[mTail];
         mTail = nextWrap(mTail);
         mNum--;

         success = true;
      }

      return success;
   }

   // Returns number of objects popped.
   uint popFront(uint maxObjects, T* pDstObjects)
   {
      const uint numObjectsToCopy = Math::Min<uint>(mNum, maxObjects);

      for (uint i = 0; i < numObjectsToCopy; i++)
      {
         pDstObjects[i] = mpBuf[mTail];
         mTail = nextWrap(mTail);
      }

      mNum -= numObjectsToCopy;

      return numObjectsToCopy;
   }
   
   const T& peekBack(uint index) const  { BDEBUG_ASSERT(index < getSize()); return mpBuf[wrap(mHead - 1 - index)]; }
         T  peekBack(uint index)        { BDEBUG_ASSERT(index < getSize()); return mpBuf[wrap(mHead - 1 - index)]; }

   // false on failure
   bool pushBack(const T& a)
   {
      bool success = false;

      if (!full())
      {
         at(mHead) = a;
         mHead = nextWrap(mHead);
         mNum++;

         success = true;
      }

      return success;
   }

   // false on failure
   bool pushBack(uint numObjects, const T* pSrcObjects)
   {
      if (!numObjects)
         return true;

      bool success = false;

      if (numObjects <= avail())
      {
         for (uint i = 0; i < numObjects; i++)
         {
            at(mHead) = pSrcObjects[i];
            mHead = nextWrap(mHead);
         }

         mNum += numObjects;

         success = true;
      }

      return success;
   }

   // false on failure           
   bool popBack(T& obj)
   {
      bool success = false;

      if (!empty())
      {
         mHead = prevWrap(mHead);
         obj = mpBuf[mHead];
         mNum--;

         success = true;
      }

      return success;
   }
   
   // false on failure           
   bool popBack(void)
   {
      bool success = false;

      if (!empty())
      {
         mHead = prevWrap(mHead);
         mNum--;

         success = true;
      }

      return success;
   }

   // Returns number of objects popped.
   uint popBack(uint maxObjects, T* pDstObjects)
   {
      const uint numObjectsToCopy = Math::Min<uint>(mNum, maxObjects);

      for (uint i = 0; i < numObjectsToCopy; i++)
      {
         mHead = prevWrap(mHead);
         pDstObjects[i] = mpBuf[mHead];
      }

      mNum -= numObjectsToCopy;

      return numObjectsToCopy;
   }

protected:
   uint           mHead;
   uint           mTail; 
   uint           mNum;
   uint           mMaxEntries;
   T*             mpBuf;
   BMemoryHeap*   mpHeap;

   const T& at (uint i) const  { return mpBuf[debugRangeCheck<uint>(i, mMaxEntries)]; }
         T& at (uint i)        { return mpBuf[debugRangeCheck<uint>(i, mMaxEntries)]; }

   uint wrap(uint i)     const { return i & (mMaxEntries - 1); }
   uint nextWrap(uint i) const { return (i + 1) & (mMaxEntries - 1); }
   uint prevWrap(uint i) const { return (i - 1) & (mMaxEntries - 1); }

   bool empty(void) const { return mNum == 0; }
   bool full(void) const { return mNum == mMaxEntries; }
   uint avail(void) const { const uint res = mMaxEntries - mNum; return res; }   
   
   void destroyBuf(void)
   {
      if (mpBuf)
      {
         if (mpHeap == &gCRunTimeHeap)
         {
            Utils::DestructArrayInPlace(mpBuf, mMaxEntries);
            free(mpBuf);
         }
         else
            HEAP_DELETE_ARRAY(mpBuf, *mpHeap);
      }            
   }      
};
