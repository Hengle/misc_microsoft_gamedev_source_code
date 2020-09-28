//============================================================================
//
// File: lightWeightMutex.h
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#pragma once

#include "threading\interlocked.h"
 
//============================================================================
// class BLightWeightMutex
// rg [2/16/06] - lock()/unlock() costs 486 cycles vs. BCriticalSection's 624, 
//and is only 4 bytes instead of 24. Supports recursion (15 levels).
// This object is purposely copyable, but you obviously shouldn't do 
// this on locked objects.
// Important: This class has not been tested on the PC yet!
//============================================================================
class BLightWeightMutex
{
   //BLightWeightMutex(const BLightWeightMutex&);
   //BLightWeightMutex& operator= (const BLightWeightMutex&);
   
public:
   BLightWeightMutex() : 
      mOwnerThreadId(0)
   {
   }
   
#ifdef XBOX   
   enum { cThreadIdRecursionMask = 0xF0000, cThreadIdRecursionAdd = 0x10000 };
#else   
   enum { cThreadIdRecursionMask = 0xF0000000, cThreadIdRecursionAdd = 0x10000000 };
#endif   

   // If yielding is false this is equivalent to a spinlock.
   // Do not set importBarrier to false unless you are VERY sure you know what you are doing.
   void lock(DWORD maxSpinCount = 256, bool importBarrier = true, bool yielding = true) 
   {
      const DWORD currentThreadId = GetCurrentThreadId();
      assert((currentThreadId & cThreadIdRecursionMask) == 0);
      
      DWORD prevOwnerThreadId;

      DWORD spinCount = 0;

      // Loop until we acquire the lock.
      for ( ; ; )
      {
         DWORD ownerThreadIdSnapshot;

         // Loop until we successfully change mOwnerThreadId in an atomic manner.
         do
         {
            // Grab a snapshot of the current owner.
            ownerThreadIdSnapshot = mOwnerThreadId;

            // Decide on the new value.         
            DWORD newValue = ownerThreadIdSnapshot;
            
            if (ownerThreadIdSnapshot == 0)
            {
               newValue = currentThreadId + cThreadIdRecursionAdd;
            }
            else if ((ownerThreadIdSnapshot & ~cThreadIdRecursionMask) == (currentThreadId & ~cThreadIdRecursionMask))
            {
               assert( (ownerThreadIdSnapshot & cThreadIdRecursionMask) < cThreadIdRecursionMask );
               newValue = ownerThreadIdSnapshot + cThreadIdRecursionAdd;
            }

            // Atomically exchange mOwnerThreadId with our desired new value if our original snapshot is still valid.
            prevOwnerThreadId = (DWORD)InterlockedCompareExchange(&mOwnerThreadId, newValue, ownerThreadIdSnapshot);

            // Loop if mOwnerThreadIs has changed since the snapshot.
         } while (prevOwnerThreadId != ownerThreadIdSnapshot);            

         // Break out of the loop if we've acquired the lock, or if we already have it.
         if ( (prevOwnerThreadId == 0) || ((prevOwnerThreadId & ~cThreadIdRecursionMask) == (currentThreadId & ~cThreadIdRecursionMask)) )
            break;

#ifdef XBOX       
         for (DWORD spin = 8; spin > 0; spin--)
         {
            YieldProcessor(); 
            __asm { or r0,r0,r0 } 
            YieldProcessor(); 
            __asm { or r1,r1,r1 }
         }                        
#else         
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
#endif

         spinCount++;
         if ((yielding) && (spinCount >= maxSpinCount))
         {
            spinCount = maxSpinCount;

            // Yield a bit.
            Sleep(1);
         }
      }

      if ((0 == prevOwnerThreadId) && (importBarrier))
      {
         // Import barrier.
         MemoryBarrier();
      }
   }

   // Do not set exportBarrier to false unless you are VERY sure you know what you are doing.
   void unlock(bool exportBarrier = true)
   {
      assert((mOwnerThreadId & cThreadIdRecursionMask) >= cThreadIdRecursionAdd);
      assert((mOwnerThreadId & ~cThreadIdRecursionMask) == (LONG)GetCurrentThreadId());
      
      mOwnerThreadId = mOwnerThreadId - cThreadIdRecursionAdd;
      
      if ((mOwnerThreadId & cThreadIdRecursionMask) == 0)
      {
         if (exportBarrier)
         {
            // Export barrier.
            MemoryBarrier();
         }

         mOwnerThreadId = 0;
      }
   }

private:
   volatile LONG mOwnerThreadId;
};

//============================================================================
// class BScopedLightWeightMutex
//============================================================================
class BScopedLightWeightMutex
{
   BScopedLightWeightMutex(const BScopedLightWeightMutex&);
   BScopedLightWeightMutex& operator= (const BScopedLightWeightMutex&);
   
public:
   BScopedLightWeightMutex(BLightWeightMutex& mutex) : mMutex(mutex)
   {
      mMutex.lock();
   }
   
   ~BScopedLightWeightMutex()
   {
      mMutex.unlock();
   }
   
private:
   BLightWeightMutex& mMutex;   
};

//============================================================================
// class BNonRecursiveSpinlock
// This object is purposely copyable, but you obviously shouldn't do 
// this on locked objects.
//============================================================================
class BNonRecursiveSpinlock
{
public:
   BNonRecursiveSpinlock() : 
      mLocked(FALSE)
   {
   }

   void lock(DWORD maxSpinCount = 256, bool yielding = true, bool memoryBarrier = true) 
   {
      DWORD spinCount = 0;

      // Loop until we acquire the lock.
      for ( ; ; )
      {
         if (!InterlockedExchange(&mLocked, TRUE))
            break;

#ifdef XBOX
         for (DWORD spin = 8; spin > 0; spin--)
         {
            YieldProcessor(); 
            __asm { or r0,r0,r0 } 
            YieldProcessor(); 
            __asm { or r1,r1,r1 }
         }                        
#else                                                                       
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
#endif
                              
         spinCount++;
         if ((yielding) && (spinCount >= maxSpinCount))
         {
            spinCount = maxSpinCount;

            // Yield a bit.
            Sleep(1);
         }
      }

      if (memoryBarrier)
      {
         // Import barrier.
         MemoryBarrier();
      }
   }
   
   BOOL tryLock(bool memoryBarrier = true)
   {
      if (!InterlockedExchange(&mLocked, TRUE))
      {
         if (memoryBarrier)
         {
            // Import barrier.
            MemoryBarrier();
         }
         
         return TRUE;
      }
      
      return FALSE;
   }

   void unlock(bool memoryBarrier = true)
   {
      if (memoryBarrier)
      {
         // Export barrier.
         MemoryBarrier();
      }

      mLocked = FALSE;
   }
   
   BOOL isLocked(void) 
   { 
      return InterlockedCompareExchange(&mLocked, mLocked, mLocked);
   }

private:
   volatile LONG mLocked;
};

// Idea adapted from http://lwn.net/Articles/7388/
// Or http://lwn.net/Articles/20976/
// Fast reader writer lock with no writer starvation. 
// Readers never block, but may need to retry if a write occurs during the read.
//
// Basic usage:
//
// BLockSequence seq;
// do 
// { 
//    seq = readBegin(); 
//    ...Read protected data - except for pointers! A writer may be active inside here.
// } while (!readEnd(Seq));
//
// Caution: Do not use this to protect data containing pointers!
class BFastReaderWriterLock
{
public:
   BFastReaderWriterLock() : 
      mSequence(0) 
   {
   }     
   
   typedef LONG BLockSequence;
         
   BLockSequence readBegin(void)
   {
      // Import barrier.
      MemoryBarrier();
      
      return mSequence;
   }
   
   // Returns TRUE if the read can proceed. 
   // Returns FALSE if a write is currently active.
   BOOL readBegin(BLockSequence& sequence)
   {
      // Import barrier.
      MemoryBarrier();

      const LONG curSequence = mSequence;
      if (curSequence & 1)
         return FALSE;

      sequence = curSequence;
      
      return TRUE;
   }
   
   // Spins if a write is currently taking place.
   BLockSequence readBeginWait(DWORD maxSpinCount = 256, bool yielding = true)
   {  
      unsigned int spinCount = 0;
      for ( ; ; )
      {
         const LONG curSequence = InterlockedCompareExchange(&mSequence, mSequence, mSequence);
         if ((curSequence & 1) == 0)
            break;

#ifdef XBOX
         for (DWORD spin = 8; spin > 0; spin--)
         {
            YieldProcessor(); 
            __asm { or r0,r0,r0 } 
            YieldProcessor(); 
            __asm { or r1,r1,r1 }
         }                        
#else                                                                       
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
         YieldProcessor();
#endif

         spinCount++;
         if ((yielding) && (spinCount >= maxSpinCount))
         {
            spinCount = maxSpinCount;

            // Yield a bit.
            Sleep(1);
         }
      }
      
      // Import barrier.
      MemoryBarrier();

      return mSequence;
   }
   
   // Returns FALSE if a write occurred while reading. In this case, try the read again.
   // Returns TRUE if no writes have occurred while reading.
   BOOL readEnd(BLockSequence sequence)
   {
      // Ensure any reads are complete before we check the sequence.
      MemoryBarrier();
      
      const LONG curSequence = mSequence;
      
      return (curSequence == sequence) && ((curSequence & 1) == 0);
   }
   
   void writeBegin(DWORD maxSpinCount = 256, bool yielding = true)
   {
      mLock.lock(maxSpinCount, yielding, false);
      
      mSequence++;
      
      // Import/export barrier.
      MemoryBarrier();
   }
   
   BOOL tryWriteBegin(void)
   {
      if (!mLock.tryLock(false))
         return FALSE;
         
      mSequence++;
      
      // Import/export barrier.
      MemoryBarrier();
      
      return TRUE;
   }
   
   void writeEnd(void)
   {
      // Export barrier.
      MemoryBarrier();
      
      InterlockedIncrement(&mSequence);
      
      mLock.unlock(false);
   }

private:
   BNonRecursiveSpinlock   mLock;
   volatile LONG           mSequence;
};

