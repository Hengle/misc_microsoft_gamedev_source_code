//============================================================================
// File: readerWriterLock.h
// Copyright (c) 2005-2006, Ensemble Studios
//
// rg [2/17/06] - A 360 reader/writer lock class.
// Considering it takes ~1500 cycles for a uncontested lock/unlock, this seems mostly useless to me. (Unless you've got a ton of contention,
// but we shouldn't have that anyway.)
// See http://msdn2.microsoft.com/en-us/library/ms810427.aspx
// rg [3/27/08] - Updated and documented. Still needs more testing.
//============================================================================
#pragma once

// Private helper class.
class BReaderWriterLockBase
{
public:
   BReaderWriterLockBase()
   {
      BCOMPILETIMEASSERT(sizeof(mLockFields) == sizeof(LONG));
      *(LONG*)&mLockFields = 0;
   }
   
   struct BLockFields
   {
      uint32 mReaderCounter   : 8;
      uint32 mWriterLock      : 8;
      uint32 mWaitingToWrite  : 8;
      uint32 mWriterRover     : 8;
   };

   BLockFields mLockFields;

   // Helpers

#define changeLockAtomic(F) do { \
      BLockFields curValue; \
      BLockFields newValue; \
      for ( ; ; ) { \
         curValue = mLockFields; \
         _ReadBarrier(); \
         newValue = curValue; \
         valueChanged = F(curValue, newValue); \
         if (InterlockedCompareExchange((LONG*)&mLockFields, *(LONG*)&newValue, *(LONG*)&curValue) == *(LONG*)&curValue) break; \
      } \
   } while(0)
      
#ifdef XBOX   
   #define SPINLOOP { \
      for (DWORD spin = 8; spin > 0; spin--) \
      { \
         YieldProcessor(); \
         __asm { or r0,r0,r0 } \
         YieldProcessor(); \
         __asm { or r1,r1,r1 } \
      } \
   } 
#else
   #define SPINLOOP { \
      YieldProcessor(); \
      YieldProcessor(); \
      YieldProcessor(); \
      YieldProcessor(); \
      YieldProcessor(); \
      YieldProcessor(); \
      YieldProcessor(); \
      YieldProcessor(); \
   }      
#endif   

#define spinUntilChangeLockAtomic(F, maxSpinCount, yielding) do { \
      uint curSpinCount = 0; \
      for ( ; ; ) { \
         bool valueChanged; \
         changeLockAtomic(F); \
         if (valueChanged) break; \
         SPINLOOP \
         if ((yielding) && (++curSpinCount >= maxSpinCount)) { curSpinCount--; Sleep(1); } \
      } \
   } while(0)

   // Callbacks
   static __forceinline bool acquireReaderLock(const BLockFields& curValue, BLockFields& newValue)
   {
      if ((!curValue.mWriterLock) && ((!curValue.mWaitingToWrite) || (curValue.mWriterRover >= 240)))
      {
         newValue.mReaderCounter++;
         newValue.mWriterRover++;
         return true;
      }
      return false;
   }

   static __forceinline bool releaseReaderLock(const BLockFields& curValue, BLockFields& newValue)
   {
      curValue;
      BDEBUG_ASSERT(newValue.mReaderCounter != 0);
      newValue.mReaderCounter--;
      return true;
   }

   static __forceinline bool canChangeReaderToWriter(const BLockFields& curValue, BLockFields& newValue)
   {
      if (curValue.mReaderCounter == 1)
      {
         newValue.mReaderCounter = 0;
         newValue.mWriterLock = 1;
         newValue.mWriterRover++;
         return true;
      }
      return false;
   }

   static __forceinline bool acquireWriterLock(const BLockFields& curValue, BLockFields& newValue)
   {
      if ((!curValue.mReaderCounter) && (!curValue.mWriterLock))   
      {
         newValue.mWriterLock = 1;
         BDEBUG_ASSERT(newValue.mWaitingToWrite > 0);
         newValue.mWaitingToWrite--;
         newValue.mWriterRover++;
         return true;
      }
      return false;
   }

   static __forceinline bool releaseWriterLock(const BLockFields& curValue, BLockFields& newValue)
   {
      curValue;
      newValue.mWriterLock = 0;
      BDEBUG_ASSERT(!newValue.mReaderCounter);
      return true;
   }

   static __forceinline bool changeWriterToReader(const BLockFields& curValue, BLockFields& newValue)
   {
      curValue;
      newValue.mWriterLock = 0;
      BDEBUG_ASSERT(!newValue.mReaderCounter);
      newValue.mReaderCounter = 1;
      return true;
   }

   static __forceinline bool incWaitingToWrite(const BLockFields& curValue, BLockFields& newValue)
   {
      curValue;
      newValue.mWaitingToWrite++;
      return true;
   }

   BReaderWriterLockBase(const BReaderWriterLockBase&);
   BReaderWriterLockBase& operator= (const BReaderWriterLockBase&);
};

// This is a NON-recursive reader-writer lock class implemented with a single LONG.
class BReaderWriterLock : private BReaderWriterLockBase
{
public:
   BReaderWriterLock() : BReaderWriterLockBase()
   {
   }
      
   // Do not call lockReader() while currently a writer!
   // This function is non-recursive!
   void lockReader(uint maxSpinCount = 256, bool yielding = true)
   {
      spinUntilChangeLockAtomic(acquireReaderLock, maxSpinCount, yielding);
      
      // Import barrier.
      MemoryBarrier();
   }

   void unlockReader(void)
   {
      BDEBUG_ASSERT(mLockFields.mReaderCounter);
      
      // This memory barrier doesn't seem necessary - we're not writing anything.
      // But, I want to ensure any previous reads are complete before releasing the read lock.
      MemoryBarrier();
      
      bool valueChanged;
      changeLockAtomic(releaseReaderLock);
   }

   // Do not call unless you're currently a reader!
   // Upon return, the calling thread is now a writer. You must call unlockWriter() when done.
   // Important:
   // Returns true if the lock can be ATOMICALLY changed from a reader to a writer. No other writers could have entered the lock in this case, 
   // and the protected object's state is guaranteed to remain unchanged. This case only occurs when there is a single active reader.
   // Returns false if there where multiple readers active when this method was called. In this case, the reader lock is released, 
   // and the writer lock is taken in the usual way. The internal state of your object may be modified if another writer enters the lock first!
   bool promoteReaderToWriter(uint maxSpinCount = 256, bool yielding = true)
   {
      BDEBUG_ASSERT(mLockFields.mReaderCounter);
      
      bool valueChanged;
      changeLockAtomic(canChangeReaderToWriter);
      if (valueChanged)
         return true;
      
      unlockReader();

      lockWriter(maxSpinCount, yielding);
      
      return false;
   }
                   
   // Do not call lockWriter() while currently a reader!
   // This function is non-recursive!
   void lockWriter(uint maxSpinCount = 256, bool yielding = true)
   {
      bool valueChanged;
      changeLockAtomic(incWaitingToWrite);
      
      spinUntilChangeLockAtomic(acquireWriterLock, maxSpinCount, yielding);
      
      MemoryBarrier();      
   }
         
   void unlockWriter(void)
   {
      MemoryBarrier();      
      
      BDEBUG_ASSERT(mLockFields.mWriterLock);
      
      bool valueChanged;
      changeLockAtomic(releaseWriterLock);
   }
   
   void demoteWriterToReader(void)
   {
      MemoryBarrier();      
      
      BDEBUG_ASSERT(mLockFields.mWriterLock);

      bool valueChanged;
      changeLockAtomic(changeWriterToReader);
   }
};

#undef changeLockAtomic
#undef spinUntilChangeLockAtomic















