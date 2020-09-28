//============================================================================
//
//  commandFIFO.h
//  Supports single producer/single consumer with blocking.
// 
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
#include "threading\win32Semaphore.h"

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
   T mBuf[cMaxEntries];
   
   int mHead;  // back, read/writen by producer thread only
   int mTail;  // front, read/written by consumer thread only
   // mCount is intended for real-time throttling purposes only!
   volatile LONG mCount;
      
   mutable BWin32Semaphore mNumFree;
   mutable BWin32Semaphore mNumUsed;
      
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
   
   // getCount() is intended for real-time throttling purposes only!
   LONG getCount(void) const
   {
      return mCount;
   }
   
   // rg [1/19/06] - The notion of "back" and "front" is confusing here:
   // Back = producer thread
   // Front = consumer thread
   
   BWin32Semaphore& getBackSemaphore(void) { return mNumFree; }
   BWin32Semaphore& getFrontSemaphore(void) const { return mNumUsed; }
                       
   // Single producer/single consumer methods allow pointer operations for efficiency.
         
   // To use the acquire() methods, you MUST first wait on the back or front semaphore!
   // The other methods will do the wait for you.
      
   // This method assumes the user has successfully waited on the back semaphore!
   T* acquireBackPtr(void)
   {
#ifdef BUILD_DEBUG      
      BASSERT(!mHasBackPtr);
      mHasBackPtr = true;
#endif
      
      T* pEntry = &at(mHead);
      mHead = nextWrap(mHead);

      return pEntry;      
   }
         
   // Assumes single thread producer only.
   T* getBackPtr(void) 
   {
#ifdef BUILD_DEBUG      
      BASSERT(!mHasBackPtr);
      mHasBackPtr = true;
#endif
         
      mNumFree.wait();
            
      T* pEntry = &at(mHead);
      mHead = nextWrap(mHead);
      
      return pEntry;
   }
   
   enum eGetPtrStatus
   {
      eAcquiredPtr,
      eTimedOut,
      eHandleSignaled
   };
         
   // Assumes single thread producer only.
   // Returns NULL if the supplied handle became signaled first.
   eGetPtrStatus getBackPtr(T*& ptr, HANDLE handle, DWORD timeout = INFINITE)
   {
#ifdef BUILD_DEBUG      
      BASSERT(!mHasBackPtr);
      mHasBackPtr = true;
#endif
                        
      const int waitResult = mNumFree.wait2(handle, false, timeout);
                              
      if (waitResult == -1)
         return eTimedOut;
      else if (waitResult == 0)
         return eHandleSignaled;
            
      T* pEntry = &at(mHead);
      mHead = nextWrap(mHead);

      ptr = pEntry;
      
      return eAcquiredPtr;
   }
   
   // Use in conjunction with getBackPtr(). Assumes single thread producer only.
   bool pushBack(void)
   {
#ifdef BUILD_DEBUG      
      BASSERT(mHasBackPtr);
      mHasBackPtr = false;
#endif
            
      mNumUsed.release();

      InterlockedIncrement(&mCount);
      
      return true;
   }
    
   // This method assumes the user has successfully waited on the back semaphore!
   T* acquireFrontPtr(void) 
   {
      T* pEntry = &at(mTail);
      mTail = nextWrap(mTail);

#ifdef BUILD_DEBUG      
      BASSERT(!mHasFrontPtr);
      mHasFrontPtr = true;
#endif

      return pEntry;
   }
     
   T* getFrontPtr(DWORD timeout = INFINITE) 
   {
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
         
   eGetPtrStatus getFrontPtr(const T*& ptr, HANDLE handle, DWORD timeout = INFINITE)
   {
      const int waitResult = mNumUsed.wait2(handle, false, timeout);
                  
      if (waitResult == -1)
         return eTimedOut;
      else if (waitResult == 0)
         return eHandleSignaled;

      T* pEntry = &at(mTail);
      mTail = nextWrap(mTail);

#ifdef BUILD_DEBUG      
      BASSERT(!mHasFrontPtr);
      mHasFrontPtr = true;
#endif

      ptr = pEntry;
      
      return eAcquiredPtr;
   }
   
   // Use in conjunction with getFrontPtr(). Assumes single consumer thread only. false on failure.
   bool popFront(void)
   {
#ifdef BUILD_DEBUG      
      BASSERT(mHasFrontPtr);
      mHasFrontPtr = false;
#endif
         
      mNumFree.release();

      InterlockedDecrement(&mCount);

      return true;
   }
   
};
