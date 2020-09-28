//============================================================================
//  allocationtracker.h
//  Copyright (c) 2008, Ensemble Studios
//============================================================================
#pragma once

#ifdef XBOX

#ifndef BUILD_FINAL
//#define ENABLE_ALLOCATION_TRACKER
#endif

#ifdef ENABLE_ALLOCATION_TRACKER

#include "xdb\xstackTrace.h"
#include "threading\criticalsection.h"
#include "containers\hashtable.h"

//============================================================================
//============================================================================
class BAllocationRecord
{
   public:
      BAllocationRecord() : mCallstackHandle(NULL) {}       // bhandle requires explicit construction
      
      DWORD                   mPointer;
      DWORD                   mSize;
      bhandle                 mCallstackHandle;
};


//============================================================================
//============================================================================
class BCallstackRecord
{
   public:
      BXStackTrace            mCallstack;
      DWORD                   mTotalAllocated;
      long                    mAllocatedForSnapshot;
};


//============================================================================
//============================================================================
class BAllocationTracker
{
   public:
      void                    trackNew(void *ptr, DWORD size, long extraStackSkip);
      void                    trackResize(void *oldPtr, void *newPtr, DWORD newSize);
      void                    trackDelete(void *ptr);
      
      bool                    writeStats(const char *filename, bool snapshot);
      void                    clearSnapshot();
   
   protected:
      bhandle                 BAllocationTracker::findCallstack(const BXStackTrace &callstack, DWORD &outCallstackHash);

      BCriticalSection        mMutex;
      DWORD                   mTotalTrackedSize;
      
      BHashTable<BAllocationRecord, DWORD, 65536, hashFast, true> mAllocationRecords;
      BHashTable<BCallstackRecord, DWORD, 32768, hashFast, true> mCallstackRecords;
};

#endif
#endif