//==============================================================================
// perf.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _PERF_H_
#define _PERF_H_

typedef void (CALLBACK fpLogPerfBlock)(long data);
typedef bool (CALLBACK fpGrowMemlogFlagsArray)();

//==============================================================================
// class BPerfFormal
//==============================================================================
class BPerfFormal
{
   public:
      BSimString         mName;
      long            mParentIndex;
      long            mTrackIDCount;
};


//==============================================================================
// helpful macros
//==============================================================================
static const long cPerfNone = -1;

#ifdef BUILD_FINAL
   #define perfBegin(name) ((void)0)
   #define perfBeginNoTimeline(name) ((void)0)

   #define perfSuspend(name) ((void)0)
   #define perfSuspendNoTimeline(name) ((void)0)

   #define perfEnd(name) ((void)0)
   #define perfEndNoTimeline(name) ((void)0)

   #define perfEndID(name, id) ((void)0)
   #define perfEndIDNoTimeline(name, id) ((void)0)

   #define scopedPerf(name) ((void)0)
   #define scopedPerfNoTimeline(name) ((void)0)
   
   #define EXT_PERF(name) extern long cPerf##name; 
   #define DEF_PERF(name) long cPerf##name=-1; 
   #ifdef UNICODE   
      #define ADD_PERF(name, parent, trackIDCount) cPerf##name = gPerf.addFormal(L#name, cPerf##parent, trackIDCount);
   #else
      #define ADD_PERF(name, parent, trackIDCount) cPerf##name = gPerf.addFormal(#name, cPerf##parent, trackIDCount);
   #endif   
#else

#if defined(XBOX) || !defined(ENABLE_TIMELINE_PROFILER)  
// Use the noTimeline variants if you perf anything in a worker thread!
// rg [1/14/05] perf wasn't designed to be thread safe either so using any of this stuff in a worker thread is asking for trouble
   #define perfBegin(name)             do { gPerf.begin(cPerf##name); } while(0)
   #define perfBeginNoTimeline(name)   gPerf.begin(cPerf##name);

   #define perfSuspend(name)           do { gPerf.suspend(cPerf##name); } while(0)
   #define perfSuspendNoTimeline(name) gPerf.suspend(cPerf##name);

   #define perfEnd(name)               do { gPerf.end(cPerf##name); } while(0)
   #define perfEndNoTimeline(name)     gPerf.end(cPerf##name);

   #define perfEndID(name, id)            do { gPerf.end(cPerf##name, id); } while(0)
   #define perfEndIDNoTimeline(name, id)  gPerf.end(cPerf##name, id);

   #define scopedPerf(name)               BScopedPerf __scopedPerf##name(cPerf##name);
   #define scopedPerfNoTimeline(name)     BScopedPerf __scopedPerf##name(cPerf##name);

   #define EXT_PERF(name) extern long cPerf##name; 
   #define DEF_PERF(name) long cPerf##name=-1; 
   #ifdef UNICODE   
      #define ADD_PERF(name, parent, trackIDCount) cPerf##name = gPerf.addFormal(L#name, cPerf##parent, trackIDCount);
   #else
      #define ADD_PERF(name, parent, trackIDCount) cPerf##name = gPerf.addFormal(#name, cPerf##parent, trackIDCount);  
   #endif    
#else
   // Use the noTimeline variants if you perf anything in a worker thread!
   // rg [1/14/05] perf wasn't designed to be thread safe either so using any of this stuff in a worker thread is asking for trouble
   #define perfBegin(name)             do { ProfileManagerStartSample(__gScopedSample##name); gPerf.begin(cPerf##name); } while(0)
   #define perfBeginNoTimeline(name)   gPerf.begin(cPerf##name);

   #define perfSuspend(name)           do { ProfileManagerEndSample(__gScopedSample##name); gPerf.suspend(cPerf##name); } while(0)
   #define perfSuspendNoTimeline(name) gPerf.suspend(cPerf##name);

   #define perfEnd(name)               do { ProfileManagerEndSample(__gScopedSample##name); gPerf.end(cPerf##name); } while(0)
   #define perfEndNoTimeline(name)     gPerf.end(cPerf##name);

   #define perfEndID(name, id)            do { ProfileManagerEndSampleID(__gScopedSample##name, id); gPerf.end(cPerf##name, id); } while(0)
   #define perfEndIDNoTimeline(name, id)  gPerf.end(cPerf##name, id);

   #define scopedPerf(name)               BScopedSample __scopedSample##name(__gScopedSample##name); BScopedPerf __scopedPerf##name(cPerf##name);
   #define scopedPerfNoTimeline(name)     BScopedPerf __scopedPerf##name(cPerf##name);
   
   #define EXT_PERF(name) extern long cPerf##name; extern BProfileSection __gScopedSample##name; 
   #define DEF_PERF(name) long cPerf##name=-1; BProfileSection __gScopedSample##name(#name, true); 

   #ifdef UNICODE
      #define ADD_PERF(name, parent, trackIDCount) cPerf##name = gPerf.addFormal(L#name, cPerf##parent, trackIDCount);
   #else   
      #define ADD_PERF(name, parent, trackIDCount) cPerf##name = gPerf.addFormal(#name, cPerf##parent, trackIDCount);
   #endif   
   
#endif   
#endif

//==============================================================================
// class BPerfIDInfo
//==============================================================================
class BPerfIDInfo
{
   public:
      long                    mID;
      __int64                 mElapsed;
};
typedef BDynamicSimArray<BPerfIDInfo> BPerfIDInfoArray;


//==============================================================================
// class BPerfRecord
//==============================================================================
class BPerfRecord
{
   public:
                              BPerfRecord(void);
      void                    clear(void);
      void                    updateID(long index, long id, __int64 elapsed);

      bool                    mRecordLogged; // Are we logging this to file?

      long                    mCount;
      __int64                 mTotalTime;
      __int64                 mMinTime;
      __int64                 mMaxTime;

      long                    mFrameMaxCount;
      long                    mFrameMinCount;
      __int64                 mFrameMaxTime;
      __int64                 mFrameMinTime;

      BPerfIDInfoArray        mIDInfo;
};


//==============================================================================
// class BPerfBlock
//==============================================================================
class BPerfBlock
{
   public:
      enum
      {
         cClearNever,
         cClearTime,
         cClearFrameCount,
         cClearUpdateCount
      };

                              BPerfBlock(void);
      void                    checkClear(long blockIndex);
      void                    clear(long blockIndex);
      void                    updateFormalCount(void);

      long                    mClearType;
      __int64                 mClearVal;

      DWORD                   mFrameCount;
      DWORD                   mUpdateCount;
      __int64                 mElapsedTime;                   

      DWORD                   mPastFrameCount;
      DWORD                   mPastUpdateCount;
      __int64                 mPastElapsedTime;                   

      char                    mName[256];
      bool                    mClearOnNextFrame;
      bool                    mBlockLogged; // Are we logging this to file?

      BDynamicSimArray<BPerfRecord> mPastRecords;
      BDynamicSimArray<BPerfRecord> mRecords;

      static void setLogPerfBlockFunc(fpLogPerfBlock* pLogPerfBlock){mpLogPerfBlock = pLogPerfBlock;}

   private:

      static fpLogPerfBlock* mpLogPerfBlock;
};


//==============================================================================
// class BPerf
//==============================================================================
class BPerf
{
   friend class BTestHookManager;

   public:

      // Indices for the default blocks we always add.
      enum
      {
         cBlockTotal=0,
         cBlockLastFrame,
         cBlockLastUpdate,
         cBlockLastSecond,
         cBlockWorld,
         cBlockLastFifteenSeconds
      };      

                              BPerf(void);
                              ~BPerf(void);

      bool                    addBlock(const char *name, long clearType, __int64 clearVal);
      bool                    init(void);
      bool                    clearBlock(long blockIndex);
      bool                    clearBlockOnNextFrame(long blockIndex);

      void                    frameComplete(void);
      void                    updateComplete(void);

      void                    begin(long index);
      void                    suspend(long index);
      void                    end(long index, long id=-1);

      long                    getBlockCount(void) const {return(mBlocks.getNumber());}
      long                    getBlockIndex(const char *name) const;
      const char              *getBlockName(long index) const;

      long                    getCount(long blockIndex, long recordIndex);
      float                   getTotalTime(long blockIndex, long recordIndex);
      float                   getTotalPercentOfParent(long blockIndex, long recordIndex);
      float                   getMinTime(long blockIndex, long recordIndex);
      float                   getMaxTime(long blockIndex, long recordIndex);
      float                   getAverageTime(long blockIndex, long recordIndex);

      float                   getAverageCallsPerFrame(long blockIndex, long recordIndex);
      float                   getAverageTimePerFrame(long blockIndex, long recordIndex);
      float                   getAverageCallsPerUpdate(long blockIndex, long recordIndex);
      float                   getAverageTimePerUpdate(long blockIndex, long recordIndex);

      long                    getIDCount(long blockIndex, long recordIndex);
      bool                    getIDInfo(long blockIndex, long recordIndex, long num, long &id, float &elapsed);

      float                   getAverageFramesPerSecond(long blockIndex);
      float                   getAverageUpdatesPerSecond(long blockIndex);
   
      // These give per frame min/maxes.
      long                    getFrameMaxCount(long blockIndex, long recordIndex);
      long                    getFrameMinCount(long blockIndex, long recordIndex);
      float                   getFrameMaxTime(long blockIndex, long recordIndex);
      float                   getFrameMinTime(long blockIndex, long recordIndex);

      void                    dumpToLog(long blockIndex);

      float                   getFrequencyFloat(void) const {return(mFrequencyFloat);}
      __int64                 getFrequency(void) const {return(mFrequency);}

      bool                    isTracking(void) const {return(mTracking>0);}
      void                    startTracking(void) {mTracking++;}
      void                    stopTracking(void) {mTracking--; BASSERTM(mTracking>=0, "Perf tracking ref count went negative.");}

      long                    addFormal(const BSimString &name, long parentIndex, long trackIDCount);
      const BPerfFormal       *getFormals(void) const {return(mFormals.getPtr());}
      long                    getFormalCount(void) const {return(mFormals.getNumber());}

      static void setGrowMemlogFlagsArrayFunc(fpGrowMemlogFlagsArray* pGrowMemlogFlagsArray){mpGrowMemlogFlagsArray = pGrowMemlogFlagsArray;}

   protected:   

      __int64                 getElapsed(void);

      bool                    mInitialized;
      long                    mTracking;
      float                   mFrequencyFloat;
      __int64                 mFrequency;
      __int64                 mLastTime;

      BDynamicSimArray<BPerfFormal> mFormals;

      BDynamicSimArray<__int64>   mStartTime;
      BDynamicSimArray<__int64>   mSuspendTime;

      BDynamicSimArray<BPerfBlock> mBlocks;

      static fpGrowMemlogFlagsArray* mpGrowMemlogFlagsArray;
};

//==============================================================================
// swiped from herb's old profile stuff
void __fastcall GetTimeStamp(__int64 *pdwtime);
double GetFrequency(void);

extern BPerf gPerf;

#ifndef BUILD_FINAL
   //==============================================================================
   // class BScopedPerf
   // This class guarantees that perf sections will always be ended no matter
   // where the function returns.
   //==============================================================================
   class BScopedPerf
   {
   public:
      BScopedPerf(long index) : mIndex(index) { gPerf.begin(index); }
      ~BScopedPerf() { gPerf.end(mIndex); }
   private:
      long mIndex;   
   };
#endif   

//==============================================================================
#endif // _PERF_H_

//==============================================================================
// eof: perf.h
//==============================================================================
