//============================================================================
//
//  commandFIFO.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================

#pragma once
//-----------------------------------------------------------------------------
// class BCommandFIFO
//-----------------------------------------------------------------------------
template <class T, int N>
class BCommandFIFO
{
public:
   typedef T Element_Type;
   enum { cMaxEntries = N };

protected:
   int mHead, mTail;
   volatile LONG mCount;
   mutable BCriticalSection mProducerMutex;
   mutable BCriticalSection mConsumerMutex;
   mutable BWin32Semaphore mNumFree;
   mutable BWin32Semaphore mNumUsed;
   T mBuf[cMaxEntries];

#ifdef BUILD_DEBUG   
   bool mHasBackPtr;
   bool mHasFrontPtr;
#endif

   static int wrap(int i)     { return i & (cMaxEntries - 1); }
   static int nextWrap(int i) { return (i + 1) & (cMaxEntries - 1); }
   static int prevWrap(int i) { return (i - 1) & (cMaxEntries - 1); }

   const T& at (int i) const  { return mBuf[debugRangeCheck<int>(i, cMaxEntries)]; }
         T& at (int i)        { return mBuf[debugRangeCheck<int>(i, cMaxEntries)]; }
         
public:
   BCommandFIFO() : 
      mHead(0), 
      mTail(0), 
      mCount(0),
      mNumFree(cMaxEntries, cMaxEntries),
      mNumUsed(0, cMaxEntries)
   { 
      BDEBUG_ASSERT(Math::IsPow2(cMaxEntries)); 

#ifdef BUILD_DEBUG
      mHasBackPtr = false;
      mHasFrontPtr = false;
#endif      
   }

   ~BCommandFIFO()
   {
   }
   
   static int getMaxSize(void) 
   { 
      return cMaxEntries; 
   }
   
   int getCount(void) const
   {
      return mCount;
   }
   
   // Assumes single thread producer only.
   T* getBackPtr(void) 
   {
#ifdef BUILD_DEBUG      
      BASSERT(!mHasBackPtr);
      mHasBackPtr = true;
#endif
   
      BScopedCriticalSection lock(mProducerMutex);

      mNumFree.wait();

      T* pEntry = &at(mHead);
      mHead = nextWrap(mHead);
      
      return pEntry;
   }
   
   // Use in conjunction with getBackPtr(). Assumes single thread producer only.
   bool pushBack(void)
   {
#ifdef BUILD_DEBUG      
      BASSERT(mHasBackPtr);
      mHasBackPtr = false;
#endif

      BScopedCriticalSection lock(mProducerMutex);
      
      mNumUsed.release();

      InterlockedIncrement(&mCount);
      
      return true;
   }

   // Multiple producer threads okay. false on failure.
   bool pushBack(const T& a)
   {
      BScopedCriticalSection lock(mProducerMutex);
      
      mNumFree.wait();

      at(mHead) = a;
      mHead = nextWrap(mHead);
      
      mNumUsed.release();
      
      InterlockedIncrement(&mCount);

      return true;
   }
   
   T* getFrontPtr(DWORD timeout = INFINITE) 
   {
      BScopedCriticalSection lock(mConsumerMutex);

      if (!mNumUsed.wait(timeout))
         return NULL;

      T* pEntry = &at(mTail);
      mTail = nextWrap(mTail);

#ifdef BUILD_DEBUG      
      BASSERT(!mHasFrontPtr);
      mHasFrontPtr = true;
#endif

      return pEntry;
   }
   
   // Use in conjunction with getFrontPtr(). Assumes single consumer thread only. false on failure.
   bool popFront(void)
   {
#ifdef BUILD_DEBUG      
      BASSERT(mHasFrontPtr);
      mHasFrontPtr = false;
#endif
   
      BScopedCriticalSection lock(mConsumerMutex);

      mNumFree.release();

      InterlockedDecrement(&mCount);

      return true;
   }
        
   // Multiple consumer threads okay. false on failure.
   bool popFront(T& obj)
   {
      BScopedCriticalSection lock(mConsumerMutex);

      mNumUsed.wait();
      
      obj = at(mTail);
      mTail = nextWrap(mTail);
      
      mNumFree.release();
      
      InterlockedDecrement(&mCount);
            
      return true;
   }
      
};
