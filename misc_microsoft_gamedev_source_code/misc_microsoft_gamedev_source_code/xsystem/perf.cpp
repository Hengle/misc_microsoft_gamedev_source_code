//==============================================================================
// perf.cpp
//
// Copyright (c) 2000-2001 Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "perf.h"
#include "win32registry.h"
#include "econfigenum.h"
//#include "strHelper.h"

// Static data
fpLogPerfBlock* BPerfBlock::mpLogPerfBlock = NULL;
fpGrowMemlogFlagsArray* BPerf::mpGrowMemlogFlagsArray = NULL;

//==============================================================================
// GetFrequency
// jce [8/21/2003] -- stolen from herb's old perf stuff
//==============================================================================
double GetFrequency(void)
{
   static double ffreq = 0.0;
   static bool bcalculated = false;

#ifdef XBOX

   if (bcalculated)
      if (ffreq > 0.0)
         return ffreq;

   LARGE_INTEGER temp;
   if(QueryPerformanceFrequency(&temp))
      ffreq=(double)temp.QuadPart;

   return ffreq;

#else

   HANDLE hSemaphore;
   __int64 mqwStart, mqwEnd;
   HANDLE hthread;
   HANDLE hprocess;
   long nOldPriority;
   DWORD dwOldClass;
   HCURSOR hcursor;

   if (bcalculated)
      if (ffreq > 0.0)
         return ffreq;

   hcursor = SetCursor(LoadCursor(0, IDC_WAIT));

   hthread = GetCurrentThread();

   hprocess = GetCurrentProcess();

//   hSemaphore = CreateSemaphore(0, 1, 1, 0);
   hSemaphore = CreateSemaphore(0, 0, 1, 0);
   if(hSemaphore == NULL)
   {
      BFAIL("Could not create semaphore");
      return(0.0);
   }

//   WaitForSingleObject(hSemaphore, 1000);

 

   nOldPriority = GetThreadPriority(hthread);

   dwOldClass = GetPriorityClass(hprocess);
   SetPriorityClass(hprocess, REALTIME_PRIORITY_CLASS);
  
   SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL);


   GetTimeStamp(&mqwStart);

   WaitForSingleObject(hSemaphore, 5000);

   GetTimeStamp(&mqwEnd);


   SetPriorityClass(hprocess, dwOldClass);

   SetThreadPriority(hthread, nOldPriority);


   ReleaseSemaphore(hSemaphore, 1, 0);

   CloseHandle(hSemaphore);

//   ffreq = (double) (mqwEnd - mqwStart) / 5000000.0;
   ffreq = (double) (mqwEnd - mqwStart) / 5.0;

   bcalculated = true;

   SetCursor(hcursor);

   return ffreq;
#endif

} // GetFrequency


//==============================================================================
// GetTimeStamp
// jce [8/21/2003] -- stolen from herb's old profile stuff. 
//==============================================================================
#ifdef XBOX
   void __fastcall GetTimeStamp(__int64 *pdwtime)
   {
      LARGE_INTEGER temp;
      if(QueryPerformanceCounter(&temp))
         *pdwtime=temp.QuadPart;
   }
#else
// 23jun02 - ham - some debugging tools can cause __fastcall to be ignored, or otherwise screwed-up, so just pass the parameter
//#ifdef _DEBUG

// jce [1/15/2004] -- in .NET2003, the lower version seems to cause the optimizer to freak out and emit bad code. 
   #if 1
   void __fastcall GetTimeStamp(__int64 *pdwtime)
   {
      DWORD   dwLoDword;
      DWORD   dwHiDword;
           
      _asm 
      {
         _emit 0x0f                      // "Fake" RDTSC instruction (0x0f31)
         _emit 0x31
         mov dwLoDword, eax              // Move Lo Timestamp into mem
         mov dwHiDword, edx              // Move Hi Timestamp into mem
      }

      *pdwtime = dwHiDword;
      *pdwtime = *pdwtime << 32;
      *pdwtime |= dwLoDword;
   } // GetTimeStamp
   #else
   void __fastcall GetTimeStamp(__int64 * /*pdwtime*/)
   {
      // pdwtime passed in through ecx
      _asm 
      {
         _emit 0x0f           // "Fake" RDTSC instruction (0x0f31)
         _emit 0x31

         mov [ecx], eax       // Move Lo Timestamp into mem
         mov [ecx+4], edx     // Move Hi Timestamp into mem
      }
   } // GetTimeStamp
   #endif
#endif




//==============================================================================
// BPerfRecord::BPerfRecord
//==============================================================================
BPerfRecord::BPerfRecord(void) :
   mRecordLogged(false)
{
   clear();
}


//==============================================================================
// BPerfRecord::clear
//==============================================================================
void BPerfRecord::clear()
{
   mCount=0;
   mTotalTime=0;
   mMinTime=_I64_MAX;
   mMaxTime=_I64_MIN;

   mFrameMaxCount=0;
   mFrameMinCount=0;
   mFrameMaxTime=_I64_MIN;
   mFrameMinTime=_I64_MAX;

   mIDInfo.setNumber(0);
}


//==============================================================================
// BPerfRecord::updateID
//==============================================================================
void BPerfRecord::updateID(long index, long id, __int64 elapsed)
{
   // Don't check index, we are trusting the caller to be doing this.

      // Get number of tracked IDs.
   long numToTrack=gPerf.getFormals()[index].mTrackIDCount;
   // Bail if none.
   if(numToTrack<=0)
      return;

   // Get current count.
   long currNum=mIDInfo.getNumber();

   // Look backward through the list for a spot.
   long i;
   for(i=currNum-1; i>=0; i--)
   {
      if(elapsed<mIDInfo[i].mElapsed)
         break;
   }

   // We insert at the spot following the one we found.
   long insertAt=i+1;
   // If this is off the end of the largest allowed list, bail now.
   if(insertAt>=numToTrack)
      return;

   // Do some dupe checking... this could probably be more clever...

   // See if this id already exists earlier in the list.  If so, we can
   // bail out because we already have a bigger max for this id.
   for(i=0; i<insertAt; i++)
   {
      if(mIDInfo[i].mID==id)
         return;
   }

   // See if this id already exists later in the list.  If so, nuke it.
   for(i=insertAt; i<currNum; i++)
   {
      if(id==mIDInfo[i].mID)
      {
         for(long j=i; j<currNum-1; j++)
            mIDInfo[j]=mIDInfo[j+1];
         currNum--;
         mIDInfo.setNumber(currNum);
         break;
      }
   }

   
   // Grow for new entry if needed.
   if(currNum<numToTrack)
      mIDInfo.setNumber(currNum+1);

   // Shuffle down.
   for(i=mIDInfo.getNumber()-1; i>insertAt; i--)
      mIDInfo[i]=mIDInfo[i-1];

   // Add.
   mIDInfo[insertAt].mID=id;
   mIDInfo[insertAt].mElapsed=elapsed;
}


//==============================================================================
// BPerfBlock::BPerfBlock
//==============================================================================
BPerfBlock::BPerfBlock(void) :
   mClearType(cClearNever),
   mClearVal(0),
   mFrameCount(0),
   mUpdateCount(0),
   mElapsedTime(0),
   mPastFrameCount(0),
   mPastUpdateCount(0),
   mPastElapsedTime(0),
   mClearOnNextFrame(false),
   mBlockLogged(false)
{
   StringCchCopyA(mName, countof(mName), "");
   updateFormalCount();
}


//==============================================================================
// BPerfBlock::checkClear
//==============================================================================
void BPerfBlock::checkClear(long blockIndex)
{
   // Bail if it's not time to clear yet.
   if(mClearType==cClearNever)
      return;
   else if(mClearType==cClearTime)
   {
      if(mElapsedTime<mClearVal)
         return;
   }
   else if(mClearType==cClearFrameCount)
   {
      if(mFrameCount<mClearVal)
         return;
   }
   else if(mClearType==cClearUpdateCount)
   {
      if(mUpdateCount<mClearVal)
         return;
   }
   else
   {
      BASSERT(0);
      return;
   }

   // If we made it here, it's time to clear.
   // (If the plant ye wish to flee, go to sector 7B)
   clear(blockIndex);
}


//==============================================================================
// BPerfBlock::clear
//==============================================================================
void BPerfBlock::clear(long blockIndex)
{
   if (mpLogPerfBlock)
      mpLogPerfBlock(blockIndex);

   // Save old times/counts.
   mPastFrameCount=mFrameCount;
   mPastUpdateCount=mUpdateCount;
   mPastElapsedTime=mElapsedTime;

   // Clear times/counts.
   mFrameCount=0;
   mUpdateCount=0;
   mElapsedTime=0;

   // Rip through records.
   for(long i=0; i<mRecords.getNumber(); i++)
   {
      // Copy current into past.
      mPastRecords[i]=mRecords[i];

      // Clear current.
      mRecords[i].clear();
   }
}


//==============================================================================
// BPerfBlock::updateFormalCount
//==============================================================================
void BPerfBlock::updateFormalCount(void)
{
   mPastRecords.setNumber(gPerf.getFormalCount());
   mRecords.setNumber(gPerf.getFormalCount());
}






//==============================================================================
// BPerf::BPerf
//==============================================================================
BPerf::BPerf(void) :
   mInitialized(false),
   mFrequency(0),
   mFrequencyFloat(0.0f)
{   
}


//==============================================================================
// BPerf::~BPerf
//==============================================================================
BPerf::~BPerf(void)
{
}


//==============================================================================
// BPerf::init
//==============================================================================
bool BPerf::init(void)
{
   // Set ref-count to 1 if always tracking is on so that theoretically we
   // should never stop tracking if other ref count users do the right thing.
   #ifndef BUILD_FINAL
   if (!gConfig.isDefined(cConfigDontTrackPerf))
      mTracking = 1;
   else
   #endif
      mTracking = 0;

#ifndef XBOX
   // get CPU speed from registry and multiply by 1M
   
   DWORD dwMhz=0;

   DWORD dwbytes=0;
   DWORD disposition;
   HKEY hKey;
   DWORD dwtype;
   
   LONG result=RegCreateKeyEx(HKEY_LOCAL_MACHINE, B("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, 0,
                       REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE,
                       0, &hKey, &disposition );

   if(result==ERROR_SUCCESS)
   {
      dwbytes=4;
      result=RegQueryValueEx(hKey, B("~MHz"), 0, &dwtype, (unsigned char *) &dwMhz, &dwbytes);
      if(result!=ERROR_SUCCESS)
         dwMhz=0; // set the value back for good luck in case the failed query mucked with it.

      RegCloseKey(hKey);
   }

   mFrequency = (__int64) dwMhz * (__int64) 1000000;  // just in case this decides to inadvertantly round on us
#endif   

   // if not in the registry (eg: win9x) then calculate ourselves

   if (!mFrequency)
   {
      blog("Processor frequency computed...");
      mFrequency = (__int64) GetFrequency();
   }
   else
      blog("Processor frequency from registry...");

   mFrequencyFloat=float(mFrequency);
   blog("Processor frequency: %f", mFrequencyFloat);


   // current time
   GetTimeStamp(&mLastTime);

   // Make sure we set up some blocks.  Otherwise invalid.
   if(mBlocks.getNumber()<0)
   {
      {setBlogError(4069); blogerror("BPerf::init -- must add blocks before initializing");}
      BASSERT(0);
      return(false);
   }

   // Add the default blocks.
   // Add a "total" block that tracks totals for all of execution.
   bool ok=gPerf.addBlock("Total", BPerfBlock::cClearNever, -1);
   if(!ok)
      return(false);
   // "last frame" block that tracks frame by frame (instantaneous)
   ok=gPerf.addBlock("Last frame", BPerfBlock::cClearFrameCount, 1);
   if(!ok)
      return(false);
   // "last frame" block that tracks frame by frame (instantaneous)
   ok=gPerf.addBlock("Last update", BPerfBlock::cClearUpdateCount, 1);
   if(!ok)
      return(false);
   // "last second" block that tracks stats for the past second.
   ok=gPerf.addBlock("Last second", BPerfBlock::cClearTime, 1000);
   if(!ok)
      return(false);
   // "world" block that tracks totals for given "world" incarnation.
   ok=gPerf.addBlock("World", BPerfBlock::cClearNever, -1);
   if(!ok)
      return(false);
   // "last 15 seconds block."
   ok=gPerf.addBlock("Last fifteen seconds", BPerfBlock::cClearTime, 15000);
   if(!ok)
      return(false);

   // Success.
   return(true);
}


//==============================================================================
// BPerf::addBlock
//==============================================================================
bool BPerf::addBlock(const char *name, long clearType, __int64 clearVal)
{
   // Must add all blocks before initializing.
   if(mInitialized)
   {
      {setBlogError(4070); blogerror("BPerf::addBlock -- must add blocks before initializing");}
      BASSERT(0);
      return(false);
   }

   // Grow block array.
   long newIndex=mBlocks.getNumber();
   bool ok=mBlocks.setNumber(newIndex+1);
   if (!ok)
   {
      BASSERT(0);
      return(false);
   }

   // Set name.
   if(name)
      StringCchCopyA(mBlocks[newIndex].mName, countof(mBlocks[newIndex].mName), name);
   else
      StringCchCopyA(mBlocks[newIndex].mName, countof(mBlocks[newIndex].mName), "");

   // Set clear info.
   mBlocks[newIndex].mClearType=clearType;

   // Se the clear value.  If we have a time, convert it from milliseconds to counter ticks.
   if(clearType==BPerfBlock::cClearTime)
      mBlocks[newIndex].mClearVal=(clearVal*mFrequency)/1000;
   else
      mBlocks[newIndex].mClearVal=clearVal;

   // Success.
   return(true);
}


//==============================================================================
// BPerf::clearBlock
//==============================================================================
bool BPerf::clearBlock(long blockIndex)
{
   // Check params.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(false);
   }

   // Clear.
   mBlocks[blockIndex].clear(blockIndex);

   // Success.
   return(true);
}


//==============================================================================
// BPerf::clearBlockOnNextFrame
//==============================================================================
bool BPerf::clearBlockOnNextFrame(long blockIndex)
{
   // Check params.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(false);
   }

   // Set up to clear on next frame.
   mBlocks[blockIndex].mClearOnNextFrame=true;

   // Success.
   return(true);
}


//==============================================================================
// BPerf::frameComplete
//==============================================================================
void BPerf::frameComplete(void)
{  
   // Bail if not tracking.
   if(mTracking<=0)
      return;
   
   __int64 elapsed=getElapsed();

   // Update frame min/maxes.
   for(long i=0; i<mBlocks.getNumber(); i++)
   {
      // Don't do this for the last frame block because that's our data source.
      if(i==cBlockLastFrame)
         continue;

      // Run through each record.
      for(long j=0; j<mFormals.getNumber(); j++)
      {
         // Update counts.
         long count=mBlocks[cBlockLastFrame].mRecords[j].mCount;
         if(count > mBlocks[i].mRecords[j].mFrameMaxCount)
            mBlocks[i].mRecords[j].mFrameMaxCount=count;
         if(count < mBlocks[i].mRecords[j].mFrameMinCount)
            mBlocks[i].mRecords[j].mFrameMinCount=count;

         // Update times.
         __int64 frameTime=mBlocks[cBlockLastFrame].mRecords[j].mTotalTime;
         if(frameTime > mBlocks[i].mRecords[j].mFrameMaxTime)
            mBlocks[i].mRecords[j].mFrameMaxTime=frameTime;
         if(frameTime < mBlocks[i].mRecords[j].mFrameMinTime)
            mBlocks[i].mRecords[j].mFrameMinTime=frameTime;
      }
   }

   // Run through the blocks and tell them a frame passed.
   for(long i=0; i<mBlocks.getNumber(); i++)
   {
      // Increment frame counter.
      mBlocks[i].mFrameCount++;

      // Increment time.
      mBlocks[i].mElapsedTime+=elapsed;

      // Clear if appropriate.
      mBlocks[i].checkClear(i);

      // If forcing a clear on the next frame, clear.
      if(mBlocks[i].mClearOnNextFrame)
      {
         mBlocks[i].clear(i);
         mBlocks[i].mClearOnNextFrame=false;
      }
   }
}


//==============================================================================
// BPerf::updateComplete
//==============================================================================
void BPerf::updateComplete(void)
{
   // Bail if not tracking.
   if(mTracking<=0)
      return;

   // Run through the active blocks.
   for(long i=0; i<mBlocks.getNumber(); i++)
   {
      // Increment frame counter.
      mBlocks[i].mUpdateCount++;

      // Clear if appropriate.
      mBlocks[i].checkClear(i);
   }
}


//==============================================================================
// BPerf::getElapsed
//==============================================================================
__int64 BPerf::getElapsed(void)
{
   __int64 qtime;
   GetTimeStamp(&qtime);

   // Compute elapsed
   __int64 elapsed=qtime-mLastTime;

   // Save this time off.
   mLastTime=qtime;

   // Done.
   return(elapsed);
}


//==============================================================================
// BPerf::begin
//==============================================================================
void BPerf::begin(long index)
{
   // Bail if not tracking.
   if(mTracking<=0)
      return;

   // Index should be in-range.
   if(index<0 || index>=mFormals.getNumber())
   {
      BFAIL("Perf index out of range.  Did you forget to add the formals?");
      return;
   }

   // Shouldn't begin twice without calling end/suspend in between.
   // If you get this assert, look at which perfBegin* thing is calling this and look
   // for ways it might be getting two begins without an intervening end (or suspend). 
   BASSERTM(mStartTime[index]<0 || mSuspendTime[index]>=0, "perfBeginX called without matching perfEndX (possible early return without end?)");

   // Get time.
   __int64 qtime;
   GetTimeStamp(&qtime);

   // If we're re-beginning from suspension, compensate for that.  Otherwise,
   // just store off the start time.
   if(mSuspendTime[index]>=0)
   {
      // Offset start time by suspended interval.
      __int64 suspendedElapsed=qtime-mSuspendTime[index];
      mStartTime[index]+=suspendedElapsed;

      // Mark as unsuspended
      mSuspendTime[index]=-1;
   }
   else
      mStartTime[index]=qtime;
}


//==============================================================================
// BPerf::suspend
//==============================================================================
void BPerf::suspend(long index)
{
   // Bail if not tracking.
   if(mTracking<=0)
      return;

   // Index should be in-range.
   if(index<0 || index>=mFormals.getNumber())
   {
      BFAIL("Perf index out of range.  Did you forget to add the formals?");
      return;
   }

   // Don't suspend without beginning and don't suspend while already suspended.
   BASSERT(mStartTime[index]>=0 && mSuspendTime[index]<0);   

   // Get time.
   __int64 qtime;
   GetTimeStamp(&qtime);

   // Mark down suspend time.
   mSuspendTime[index]=qtime;
}


//==============================================================================
// BPerf::end
//==============================================================================
//#pragma optimize( "", off )

void BPerf::end(long index, long id)
{
   // Bail if not tracking.
   if(mTracking<=0)
      return;

   // Index should be in-range.
   if(index<0 || index>=mFormals.getNumber())
   {
      BFAIL("Perf index out of range.  Did you forget to add the formals?");
      return;
   }

   // Shouldn't end twice without calling start in between.  Also, shouldn't end while suspended.
   BASSERT(mStartTime[index]>=0 && mSuspendTime[index]<0);

   // Get elapsed time.
   __int64 qtime;
   GetTimeStamp(&qtime);

   __int64 elapsed=qtime-mStartTime[index];

   // Update all the blocks.
   for(long i=mBlocks.getNumber()-1; i>=0; i--)
   {
      BPerfRecord &record=mBlocks[i].mRecords[index];
      // Increment call counter.
      record.mCount++;

      // Increment total time.
      record.mTotalTime+=elapsed;

      // Update min/max.
      if(elapsed<record.mMinTime)
         record.mMinTime=elapsed;
      if(elapsed>record.mMaxTime)
         record.mMaxTime=elapsed;

      // Update ID based stuff.
      record.updateID(index, id, elapsed);
   }

   // Clear start time.
   mStartTime[index]=-1;
}
//#pragma optimize( "", on )


//==============================================================================
// BPerf::getCount
//==============================================================================
long BPerf::getCount(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
      return(mBlocks[blockIndex].mRecords[recordIndex].mCount);
   else
      return(mBlocks[blockIndex].mPastRecords[recordIndex].mCount);
}


//==============================================================================
// BPerf::getTotalTime
//==============================================================================
float BPerf::getTotalTime(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
      return(float(mBlocks[blockIndex].mRecords[recordIndex].mTotalTime)/mFrequencyFloat);
   else
      return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mTotalTime)/mFrequencyFloat);
}


//==============================================================================
// BPerf::getTotalPercentOfParent
//==============================================================================
float BPerf::getTotalPercentOfParent(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // If no parent, return 100 percent.
   if(gPerf.getFormals()[recordIndex].mParentIndex<0)
      return(100.0f);

   // If parent has no time, return 0 percent.
   float parentTime=getTotalTime(blockIndex, gPerf.getFormals()[recordIndex].mParentIndex);
   if(parentTime<cFloatCompareEpsilon)
      return(0.0f);

   // Get our time.
   float ourTime=getTotalTime(blockIndex, recordIndex);

   // Return percentage.
   return(100.0f*ourTime/parentTime);
}


//==============================================================================
// BPerf::getMinTime
//==============================================================================
float BPerf::getMinTime(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mRecords[recordIndex].mMinTime==_I64_MAX)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mRecords[recordIndex].mMinTime)/mFrequencyFloat);
   }
   else
   {
      if(mBlocks[blockIndex].mPastRecords[recordIndex].mMinTime==_I64_MAX)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mMinTime)/mFrequencyFloat);
   }
}


//==============================================================================
// BPerf::getMaxTime
//==============================================================================
float BPerf::getMaxTime(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mRecords[recordIndex].mMaxTime==_I64_MIN)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mRecords[recordIndex].mMaxTime)/mFrequencyFloat);
   }
   else
   {
      if(mBlocks[blockIndex].mPastRecords[recordIndex].mMaxTime==_I64_MIN)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mMaxTime)/mFrequencyFloat);
   }
}


//==============================================================================
// BPerf::getAverageTime
//==============================================================================
float BPerf::getAverageTime(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mRecords[recordIndex].mCount>0)
         return((float(mBlocks[blockIndex].mRecords[recordIndex].mTotalTime)/mFrequencyFloat)/float(mBlocks[blockIndex].mRecords[recordIndex].mCount));
      else
         return(0.0f);
   }
   else
   {
      if(mBlocks[blockIndex].mPastRecords[recordIndex].mCount>0)
         return((float(mBlocks[blockIndex].mPastRecords[recordIndex].mTotalTime)/mFrequencyFloat)/float(mBlocks[blockIndex].mPastRecords[recordIndex].mCount));
      else
         return(0.0f);
   }
}


//==============================================================================
// BPerf::getAverageCallsPerFrame
//==============================================================================
float BPerf::getAverageCallsPerFrame(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mFrameCount==0)
         return(0.0f);
      return(float(mBlocks[blockIndex].mRecords[recordIndex].mCount)/float(mBlocks[blockIndex].mFrameCount));
   }
   else
   {
      if(mBlocks[blockIndex].mPastFrameCount==0)
         return(0.0f);
      return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mCount)/float(mBlocks[blockIndex].mPastFrameCount));
   }
}


//==============================================================================
// BPerf::getAverageTimePerFrame
//==============================================================================
float BPerf::getAverageTimePerFrame(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mFrameCount==0)
         return(0.0f);
      return((float(mBlocks[blockIndex].mRecords[recordIndex].mTotalTime)/mFrequencyFloat)/float(mBlocks[blockIndex].mFrameCount));
   }
   else
   {
      if(mBlocks[blockIndex].mPastFrameCount==0)
         return(0.0f);
      return((float(mBlocks[blockIndex].mPastRecords[recordIndex].mTotalTime)/mFrequencyFloat)/float(mBlocks[blockIndex].mPastFrameCount));
   }
}


//==============================================================================
// BPerf::getAverageCallsPerUpdate
//==============================================================================
float BPerf::getAverageCallsPerUpdate(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mUpdateCount==0)
         return(0.0f);
      return(float(mBlocks[blockIndex].mRecords[recordIndex].mCount)/float(mBlocks[blockIndex].mUpdateCount));
   }
   else
   {
      if(mBlocks[blockIndex].mPastUpdateCount==0)
         return(0.0f);
      return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mCount)/float(mBlocks[blockIndex].mPastUpdateCount));
   }
}


//==============================================================================
// BPerf::getAverageTimePerUpdate
//==============================================================================
float BPerf::getAverageTimePerUpdate(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mUpdateCount==0)
         return(0.0f);
      return((float(mBlocks[blockIndex].mRecords[recordIndex].mTotalTime)/mFrequencyFloat)/float(mBlocks[blockIndex].mUpdateCount));
   }
   else
   {
      if(mBlocks[blockIndex].mPastUpdateCount==0)
         return(0.0f);
      return((float(mBlocks[blockIndex].mPastRecords[recordIndex].mTotalTime)/mFrequencyFloat)/float(mBlocks[blockIndex].mPastUpdateCount));
   }
}


//==============================================================================
// BPerf::getIDCount
//==============================================================================
long BPerf::getIDCount(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0);
   }
   
   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
      return(mBlocks[blockIndex].mRecords[recordIndex].mIDInfo.getNumber());
   else
      return(mBlocks[blockIndex].mPastRecords[recordIndex].mIDInfo.getNumber());
}


//==============================================================================
// BPerf::getIDInfo
//==============================================================================
bool BPerf::getIDInfo(long blockIndex, long recordIndex, long num, long &id, float &elapsed)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(false);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(false);
   }

   // Negative num?
   if(num<0)
      return(false);

   // Look up count from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      // Num too big?
      if(num>=mBlocks[blockIndex].mRecords[recordIndex].mIDInfo.getNumber())
         return(false);

      elapsed=float(mBlocks[blockIndex].mRecords[recordIndex].mIDInfo[num].mElapsed)/mFrequencyFloat;
      id=mBlocks[blockIndex].mRecords[recordIndex].mIDInfo[num].mID;
   }
   else
   {
      // Num too big?
      if(num>=mBlocks[blockIndex].mPastRecords[recordIndex].mIDInfo.getNumber())
         return(false);

      elapsed=float(mBlocks[blockIndex].mPastRecords[recordIndex].mIDInfo[num].mElapsed)/mFrequencyFloat;
      id=mBlocks[blockIndex].mPastRecords[recordIndex].mIDInfo[num].mID;
   }


   return(true);
}


//==============================================================================
// BPerf::getAverageFramesPerSecond
//==============================================================================
float BPerf::getAverageFramesPerSecond(long blockIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      float fTime=float(mBlocks[blockIndex].mElapsedTime)/float(mFrequency);
      // If no time passed, just return 0.
      if(fTime<cFloatCompareEpsilon)
         return(0.0f);

      return(float(mBlocks[blockIndex].mFrameCount)/fTime);
   }
   else
   {
      float fTime=float(mBlocks[blockIndex].mPastElapsedTime)/float(mFrequency);
      // If no time passed, just return 0.
      if(fTime<cFloatCompareEpsilon)
         return(0.0f);

      return(float(mBlocks[blockIndex].mPastFrameCount)/fTime);
   }
}


//==============================================================================
// BPerf::getAverageUpdatesPerSecond
//==============================================================================
float BPerf::getAverageUpdatesPerSecond(long blockIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      float fTime=float(mBlocks[blockIndex].mElapsedTime)/float(mFrequency);
      // If no time passed, just return 0.
      if(fTime<cFloatCompareEpsilon)
         return(0.0f);

      return(float(mBlocks[blockIndex].mUpdateCount)/fTime);
   }
   else
   {
      float fTime=float(mBlocks[blockIndex].mPastElapsedTime)/float(mFrequency);
      // If no time passed, just return 0.
      if(fTime<cFloatCompareEpsilon)
         return(0.0f);

      return(float(mBlocks[blockIndex].mPastUpdateCount)/fTime);
   }
}


//==============================================================================
// BPerf::getFrameMaxCount
//==============================================================================
long BPerf::getFrameMaxCount(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // If this is the "last frame" block, just return basic values.
   if(blockIndex==cBlockLastFrame)
      return(getCount(blockIndex, recordIndex));

   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
      return(mBlocks[blockIndex].mRecords[recordIndex].mFrameMaxCount);
   else
      return(mBlocks[blockIndex].mPastRecords[recordIndex].mFrameMaxCount);
}


//==============================================================================
// BPerf::getFrameMinCount
//==============================================================================
long BPerf::getFrameMinCount(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0);
   }

   // If this is the "last frame" block, just return basic values.
   if(blockIndex==cBlockLastFrame)
      return(getCount(blockIndex, recordIndex));

   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
      return(mBlocks[blockIndex].mRecords[recordIndex].mFrameMinCount);
   else
      return(mBlocks[blockIndex].mPastRecords[recordIndex].mFrameMinCount);
}


//==============================================================================
// BPerf::getFrameMaxTime
//==============================================================================
float BPerf::getFrameMaxTime(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // If this is the "last frame" block, just return basic values.
   if(blockIndex==cBlockLastFrame)
      return(getMaxTime(blockIndex, recordIndex));

   // Look up from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mRecords[recordIndex].mFrameMaxTime==_I64_MAX)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mRecords[recordIndex].mFrameMaxTime)/mFrequencyFloat);
   }
   else
   {
      if(mBlocks[blockIndex].mPastRecords[recordIndex].mFrameMaxTime==_I64_MAX)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mFrameMaxTime)/mFrequencyFloat);
   }
}


//==============================================================================
// BPerf::getFrameMinTime
//==============================================================================
float BPerf::getFrameMinTime(long blockIndex, long recordIndex)
{
   // Check for bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }

   // Check for bad record index.
   if(recordIndex<0 || recordIndex>=mFormals.getNumber())
   {
      BASSERT(0);
      return(0.0f);
   }
   
   // If this is the "last frame" block, just return basic values.
   if(blockIndex==cBlockLastFrame)
      return(getMinTime(blockIndex, recordIndex));

   // Look up from record.
   if(mBlocks[blockIndex].mClearType==BPerfBlock::cClearNever)
   {
      if(mBlocks[blockIndex].mRecords[recordIndex].mFrameMinTime==_I64_MAX)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mRecords[recordIndex].mFrameMinTime)/mFrequencyFloat);
   }
   else
   {
      if(mBlocks[blockIndex].mPastRecords[recordIndex].mFrameMinTime==_I64_MAX)
         return(0.0f);
      else
         return(float(mBlocks[blockIndex].mPastRecords[recordIndex].mFrameMinTime)/mFrequencyFloat);
   }
}


//==============================================================================
// BPerf::getBlockIndex
//==============================================================================
long BPerf::getBlockIndex(const char *name) const
{
   // Bail on bad param.
   if(!name)
      return(-1);

   // Run through blocks.
   for(long i=0; i<mBlocks.getNumber(); i++)
   {
      // If the names match, return index.
      if(stricmp(name, mBlocks[i].mName)==0)
         return(i);
   }

   // Nothing found...
   return(-1);
}


//==============================================================================
// BPerf::getBlockName
//==============================================================================
const char *BPerf::getBlockName(long index) const 
{
   // Return null on bad index.
   if(index<0 || index>=mBlocks.getNumber())
      return(NULL);

   // Grab name from block.
   return(mBlocks[index].mName);
}


//==============================================================================
// BPerf::dumpToLog
//==============================================================================
void BPerf::dumpToLog(long blockIndex)
{
   // Bail on bad block index.
   if(blockIndex<0 || blockIndex>=mBlocks.getNumber())
   {
      BASSERT(0);
      return;
   }

   blog("------------------------------------------------------------------------");
   blog("Performance dump for '%s'", mBlocks[blockIndex].mName);
   blog("------------------------------------------------------------------------");

   // Rip through each record.
   for(long i=0; i<mFormals.getNumber()-1; i++)
   {
      if(gPerf.getFormals()[i].mParentIndex>=0)
      {
         blog("%s (parent=%s)", mFormals[i].mName, mFormals[mFormals[i].mParentIndex].mName);
         blog("   total time=%0.1f ms (%0.1f%% of parent)", 1000.0f*getTotalTime(blockIndex, i), 
            getTotalPercentOfParent(blockIndex, i));
      }
      else
      {
         blog("%s (root)", mFormals[i].mName);
         blog("   time(ms): total=%0.1f     min=%0.1f   max=%0.1f   mean=%0.1f    count:%ld", 
              1000.0f*getTotalTime(blockIndex, i),
              1000.0f*getMinTime(blockIndex, i),
              1000.0f*getMaxTime(blockIndex, i),
              1000.0f*getAverageTime(blockIndex, i),
              getCount(blockIndex, i));
      }
   }
}


//==============================================================================
// BPerf::addFormal
//==============================================================================
long BPerf::addFormal(const BSimString &name, long parentIndex, long trackIDCount)
{
   // Get the new index.
   long newIndex = mFormals.getNumber();

   // Make room.
   mFormals.setNumber(newIndex+1);

   // Fill in data.
   mFormals[newIndex].mName = name;
   mFormals[newIndex].mParentIndex = parentIndex;
   mFormals[newIndex].mTrackIDCount = trackIDCount;

   // Sanity check for parentIndex ... it shouldn't be out of range.
   BASSERTM(parentIndex<newIndex, "Invalid parent index when adding a perf formal.");

   // Make space in the timing arrays.
   mStartTime.setNumber(newIndex+1);
   mSuspendTime.setNumber(newIndex+1);
   
   // Fill in sentinel value.
   mStartTime[newIndex] = -1;
   mSuspendTime[newIndex] = -1;

   // Update blocks.
   for(long i=0; i<mBlocks.getNumber(); i++)
      mBlocks[i].updateFormalCount();

   // Give back index.
   return(newIndex);
}


//==============================================================================
// eof: perf.cpp
//==============================================================================
