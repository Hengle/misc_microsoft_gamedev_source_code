//============================================================================
//  allocationtracker.cpp
//  Copyright (c) 2008, Ensemble Studios
//============================================================================
#include "xcore.h"

#undef XPhysicalAlloc
#undef XPhysicalAllocEx
#undef XPhysicalFree
#undef malloc
#undef calloc
#undef free
#undef realloc
#undef _expand

#ifdef XBOX
#include "allocationtracker.h"
#include "xdb\xdbmanager.h"
#include "file\win32filestream.h"
#include "stream\bufferstream.h"

#ifdef ENABLE_ALLOCATION_TRACKER



//============================================================================
//============================================================================
void BAllocationTracker::trackNew(void *ptr, DWORD size, long extraStackSkip)
{
   // Only one thread at a time.
   BScopedCriticalSection crit(mMutex);
   
   // Get the callstack.
   BXStackTrace callstack;
   bool ok = callstack.capture(2+extraStackSkip);
   if(!ok)
   {
      BFAIL("couldn't get callstack");
      return;
   }

   // See if there is an existing callstack entry.
   DWORD callstackHash;
   bhandle handle = findCallstack(callstack, callstackHash);
   
   // If we couldn't find an existing record for this callstack, we need to create one.
   if(!handle)
   {
      // Fill out the new record.
      BCallstackRecord stackRecord;
      stackRecord.mCallstack = callstack;
      stackRecord.mTotalAllocated = size;
      stackRecord.mAllocatedForSnapshot = size;
      
      // Put it in the table.
      handle = mCallstackRecords.add(callstackHash, stackRecord);
   }
   else
   {
      // Get the callstack record.  And in glorious awesomeness, cast away const.
      BCallstackRecord &stackRecord = (BCallstackRecord &)mCallstackRecords.get(handle);
      
      // This stack's allocation is now higher.
      stackRecord.mTotalAllocated += size;
      stackRecord.mAllocatedForSnapshot += size;
   }
   
   // Fill out a new allocation record.
   BAllocationRecord record;
   record.mPointer = (DWORD)ptr;
   record.mSize = size;
   record.mCallstackHandle = handle;
   
   // Put it into the hash table.
   mAllocationRecords.add((DWORD)ptr, record);
   
   // Bump up total tracked size.
   mTotalTrackedSize += size;   
}


//============================================================================
//============================================================================
void BAllocationTracker::trackResize(void *oldPtr, void *newPtr, DWORD newSize)
{
   // Just treat this as a delete followed by a new for simplicity.
   trackDelete(oldPtr);
   trackNew(newPtr, newSize, 1);
}



//============================================================================
//============================================================================
void BAllocationTracker::trackDelete(void *ptr)
{
   // Only one thread at a time.
   BScopedCriticalSection crit(mMutex);
   
   // Find allocation record.
   bhandle handle = mAllocationRecords.find((DWORD)ptr);
   if(!handle)
   {
      BFAIL("Deleting pointer that isn't currently known to be allocated");
      return;
   }
   
   // Get the record.
   const BAllocationRecord &record = mAllocationRecords.get(handle);
   
   // Get the callstack record.  And in glorious awesomeness, cast away const.
   BCallstackRecord &stackRecord = (BCallstackRecord &)mCallstackRecords.get(record.mCallstackHandle);
   
   // This stack's allocation is now lower.
   stackRecord.mTotalAllocated -= record.mSize;
   stackRecord.mAllocatedForSnapshot -= record.mSize;
   
   // Total tracked size is now lower.
   mTotalTrackedSize -= record.mSize;
   
   // Remove the record.
   mAllocationRecords.remove(handle);

}


//============================================================================
//============================================================================
bhandle BAllocationTracker::findCallstack(const BXStackTrace &callstack, DWORD &outCallstackHash)
{
   // Callstack hash is our key into the hash table, which confusingly means it will be hashed there too.
   outCallstackHash = callstack.getHash();

   // Find the callstack.
   bhandle handle = mCallstackRecords.find(outCallstackHash);
   while(handle)
   {
      // Get this record.
      const BCallstackRecord &stackRecord = mCallstackRecords.get(handle);
      
      // It matches our hash, but does it actually match the callstack?
      if(stackRecord.mCallstack == callstack)
      {
         // Yep, we have a winner.
         return(handle);
      }
      
      // Try the next record.
      handle = mCallstackRecords.find(outCallstackHash, handle);
   }
   
   return(bhandle(NULL));
}


//============================================================================
//============================================================================
bool BAllocationTracker::writeStats(const char *filename, bool snapshot)
{
   // jce [11/19/2008] -- File I/O and the classes we're using to do it can allocate, which in turn will
   // end up taking a lock in one of the memory heaps.  We are going to take the lock on ALL the heaps here to
   // make sure that no other thread is trying to allocate when we do this.  Otherwise a deadlock can occur
   // where one thread is blocked on our mutex while we're block on the memory heap one.  This is probably
   // excessive but since we're already writing a big log file, a few more ms to get some locks isn't going to hurt us.
   extern BMemoryHeap* gpRockallHeaps[];
   extern uint cNumRockallHeapsEx;
   for(uint i=0; i<cNumRockallHeapsEx; i++)
      gpRockallHeaps[i]->claimLock();
   
   // Only one thread at a time.
   BScopedCriticalSection crit(mMutex);

   // Open the file.
   BWin32FileStream win32Stream;
   bool ok = win32Stream.open(filename, cSFWritable | cSFEnableBuffering);
   if(!ok)
      return(false);

   // Wrap it in a buffering stream.      
   BBufferStream stream(win32Stream);
   
   // xex checksum.
   DWORD xexChecksum = gXDBManager.getInitialized() ? gXDBManager.getXEXChecksum() : 0;
   stream.writeBytes(&xexChecksum, sizeof(xexChecksum));

   // How many callstacks?
   long count = mCallstackRecords.getCount();
   stream.writeBytes(&count, sizeof(count));
   
   // Iterate the callstacks.
   BHashTableIter iter;
   bhandle handle = mCallstackRecords.iterStart(iter);
   long iterCount = 0;
   while(handle)
   {
      iterCount++;
      
      // Get the record.
      const BCallstackRecord &record = mCallstackRecords.get(handle);
      
      // Write the size.
      long totalAllocated;
      if(snapshot)
         totalAllocated = record.mAllocatedForSnapshot;
      else
         totalAllocated = record.mTotalAllocated;
      stream.writeBytes(&totalAllocated, sizeof(record.mTotalAllocated));
      
      // Write the stack.
      // First get the callstack.
      const BXStackTrace &stack = record.mCallstack;
      
      // Write the number of levels.
      uint levels = stack.getNumLevels();
      BASSERT(levels > 0);
      stream.writeBytes(&levels, sizeof(levels));
      
      // Write the address for each level.
      for(uint i=0; i<levels; i++)
      {
         DWORD address = stack.getLevelAddr(i);
         stream.writeBytes(&address, sizeof(address));
      }
      
      // Get next entry.
      handle = mCallstackRecords.iterNext(iter);
   }
   
   // Sanity check that we wrote the right number of things out.
   if(iterCount != count)
   {
      BFAIL("something wrong with stack count");
      return(false);
   }
   
   // jce [11/18/2008] -- Force close the buffered stream since it doesn't do this automatically and therefore
   // will not finish writing the whole file.
   stream.close(true);

   // Release the heap locks.   
   for(uint i=0; i<cNumRockallHeapsEx; i++)
      gpRockallHeaps[i]->releaseLock();

   // Success.
   return(true);
}


//============================================================================
//============================================================================
void BAllocationTracker::clearSnapshot()
{
   // Only one thread at a time.
   BScopedCriticalSection crit(mMutex);

   // Walk each callstack entry.
   BHashTableIter iter;
   bhandle handle = mCallstackRecords.iterStart(iter);
   while(handle)
   {
      // Get the callstack record.  And in glorious awesomeness, cast away const.
      BCallstackRecord &stackRecord = (BCallstackRecord &)mCallstackRecords.get(handle);
      
      // Clear the snapshot size back to 0.
      stackRecord.mAllocatedForSnapshot = 0;

      // Get next entry.
      handle = mCallstackRecords.iterNext(iter);
   }
}


#endif
#endif

//============================================================================
//============================================================================
