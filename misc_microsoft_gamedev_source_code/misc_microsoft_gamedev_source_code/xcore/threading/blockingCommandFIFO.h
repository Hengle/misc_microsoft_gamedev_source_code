//============================================================================
//
//  blockingCommandFIFO.h
//  Supports multiple producers/consumers with blocking.
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//-----------------------------------------------------------------------------
// class BBlockingCommandFIFO
//-----------------------------------------------------------------------------
template <class T, int N>
class BBlockingCommandFIFO
{
public:
   typedef T Element_Type;
   enum { cMaxEntries = N };

protected:
   int mHead, mTail;
   // mCount is intended for real-time throttling purposes only!
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
   BBlockingCommandFIFO() : 
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

   ~BBlockingCommandFIFO()
   {
   }
   
   static int getMaxSize(void) 
   { 
      return cMaxEntries; 
   }
   
   // getCount() is intended for real-time throttling purposes only!
   LONG getCount(void) const
   {
      return mCount;
   }
   
   // Atomic push/pops support multiple producer/consumbers, but only support atomic copies (no pointer methods).
   
   // Multiple producer threads okay. false on failure.
   bool pushBack(const T& a)
   {
      BScopedCriticalSection lock(mProducerMutex);

      mNumFree.wait();

      at(mHead) = a;
      mHead = nextWrap(mHead);

      mNumUsed.release();

      // Using an interlocked operation here because this class uses separate producer/consumer mutexes.
      InterlockedIncrement(&mCount);

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
