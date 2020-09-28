//============================================================================
//
// File: synchronizedFIFO.h
// rg [2/10/06] - Supports multiple producers/consumers, non-blocking.
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================

#pragma once
//-----------------------------------------------------------------------------
// class BSynchronizedFIFO
//-----------------------------------------------------------------------------
template <class T>
class BSynchronizedFIFO
{
public:
   typedef T ElementType;
   
public:
   BSynchronizedFIFO(uint maxEntries = 0) : 
      mHead(0), 
      mTail(0), 
      mNum(0), 
      mMaxEntries(maxEntries),
      mpBuf(NULL)
   { 
      BDEBUG_ASSERT((mMaxEntries == 0) || Math::IsPow2(mMaxEntries)); 
      
      if (mMaxEntries)
         mpBuf = new T[mMaxEntries];
            
      InitializeCriticalSection(&mCriticalSection);
   }

   ~BSynchronizedFIFO()
   {
      delete[] mpBuf;
      
      DeleteCriticalSection(&mCriticalSection);
   }
   
   void setSpinCount(uint spinCount)
   {
      SetCriticalSectionSpinCount(&mCriticalSection, spinCount);  
   }
      
   void lock(void) const
   {
      EnterCriticalSection(&mCriticalSection);
   }

   void unlock(void) const
   {
      LeaveCriticalSection(&mCriticalSection);
   }

   uint getMaxSize(void) const { return mMaxEntries; }

   // Returns a snapshot only, only useful for throttling! The returned value may/will be outdated by the time you process it.
   uint getSize(void) const 
   { 
      lock();
      const uint res = mNum; 
      unlock();
      return res;
   }

   // Returns a snapshot only, only useful for throttling! The returned value may/will be outdated by the time you process it.
   uint getAvail(void) const 
   { 
      uint res;
      lock();
      res = availNoLock();
      unlock();
      return res;
   }

   // Returns a snapshot only, only useful for throttling! The returned value may/will be outdated by the time you process it.
   bool getEmpty(void) const 
   { 
      bool res;
      lock();
      res = emptyNoLock();
      unlock();
      return res;
   }

   // Returns a snapshot only, only useful for throttling! The returned value may/will be outdated by the time you process it.
   bool getFull(void) const 
   {  
      bool res;
      lock();
      res = fullNoLock();
      unlock();
      return res;
   }

   void clear() 
   {
      lock();

      mHead = mTail = mNum = 0; 

      unlock();
   }
   
   bool resize(uint newMaxEntries)
   {
      BDEBUG_ASSERT((newMaxEntries == 0) || Math::IsPow2(newMaxEntries)); 
      
      bool success = false;

      lock();
      
      if ((newMaxEntries != mMaxEntries) && (mNum <= newMaxEntries))
      {
         const uint origNum = mNum;
         
         T* pBuf = NULL;
         if (newMaxEntries)
         {
            pBuf = new T[newMaxEntries];
            
            if (origNum)
               popFront(origNum, pBuf);
         }
            
         delete[] mpBuf;
                  
         mpBuf = pBuf;

         mNum = origNum;
         mMaxEntries = newMaxEntries;
         mHead = origNum & (newMaxEntries - 1);
         mTail = 0;
         
         success = true;
      }         

      unlock();

      return success;
   }

   // false on failure
   bool pushFront(const T& a)
   {
      bool success = false;

      lock();

      if (!fullNoLock())
      {
         mTail = prevWrap(mTail);
         at(mTail) = a;
         mNum++;

         success = true;
      }

      unlock();

      return success;
   }
   
   // false on failure
   bool pushFront(uint numObjects, const T* pSrcObjects)
   {
      if (!numObjects)
         return true;
         
      bool success = false;

      lock();

      if (numObjects <= availNoLock())
      {
         for (uint i = 0; < numObjects; i++)
         {
            mTail = prevWrap(mTail);
            at(mTail) = pSrcObjects[i];
         }
         
         mNum += numObjects;
         
         success = true;
      }

      unlock();

      return success;
   }
   
   // false on failure
   bool popFront(T& obj)
   {
      bool success = false;

      lock();

      if (!emptyNoLock())
      {
         obj = mpBuf[mTail];
         mTail = nextWrap(mTail);
         mNum--;

         success = true;
      }

      unlock();

      return success;
   }
   
   // Returns number of objects popped.
   uint popFront(uint maxObjects, T* pDstObjects)
   {
      lock();

      const uint numObjectsToCopy = Math::Min<uint>(mNum, maxObjects);
      
      for (uint i = 0; i < numObjectsToCopy; i++)
      {
         pDstObjects[i] = mpBuf[mTail];
         mTail = nextWrap(mTail);
      }
      
      mNum -= numObjectsToCopy;
      
      unlock();

      return numObjectsToCopy;
   }

   // false on failure
   bool pushBack(const T& a)
   {
      bool success = false;

      lock();

      if (!fullNoLock())
      {
         at(mHead) = a;
         mHead = nextWrap(mHead);
         mNum++;

         success = true;
      }

      unlock();

      return success;
   }
   
   // false on failure
   bool pushBackNoLock(const T& a)
   {
      bool success = false;

      if (!fullNoLock())
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

      lock();

      if (numObjects <= availNoLock())
      {
         for (uint i = 0; i < numObjects; i++)
         {
            at(mHead) = pSrcObjects[i];
            mHead = nextWrap(mHead);
         }

         mNum += numObjects;
         
         success = true;
      }

      unlock();

      return success;
   }
   
   // false on failure           
   bool popBack(T& obj)
   {
      bool success = false;

      lock();

      if (!emptyNoLock())
      {
         mHead = prevWrap(mHead);
         obj = mpBuf[mHead];
         mNum--;

         success = true;
      }

      unlock();

      return success;
   }
   
   // Returns number of objects popped.
   uint popBack(uint maxObjects, T* pDstObjects)
   {
      lock();

      const uint numObjectsToCopy = Math::Min<uint>(mNum, maxObjects);

      for (uint i = 0; i < numObjectsToCopy; i++)
      {
         mHead = prevWrap(mHead);
         pDstObjects[i] = mpBuf[mHead];
      }

      mNum -= numObjectsToCopy;
            
      unlock();

      return numObjectsToCopy;
   }
   
protected:
   uint mHead;
   uint mTail; 
   uint mNum;
   uint mMaxEntries;
   T* mpBuf;
   mutable CRITICAL_SECTION mCriticalSection;

   const T& at (uint i) const  { return mpBuf[debugRangeCheck<uint>(i, mMaxEntries)]; }
         T& at (uint i)        { return mpBuf[debugRangeCheck<uint>(i, mMaxEntries)]; }
   
   uint wrap(uint i)     { return i & (mMaxEntries - 1); }
   uint nextWrap(uint i) { return (i + 1) & (mMaxEntries - 1); }
   uint prevWrap(uint i) { return (i - 1) & (mMaxEntries - 1); }

   bool emptyNoLock(void) const { return mNum == 0; }
   bool fullNoLock(void) const { return mNum == mMaxEntries; }
   uint availNoLock(void) const { const uint res = mMaxEntries - mNum; return res; }   
};
