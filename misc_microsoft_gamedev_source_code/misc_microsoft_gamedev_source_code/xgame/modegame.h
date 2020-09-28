//==============================================================================
// modegame.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"
#include "mpSimDataObject.h"
#include "displayStats.h"
#include "UIGlobals.h"

#ifndef BUILD_FINAL
#include "barChart.h"
#endif

// [11/5/2008 xemu] disable this in memoryheap as well when taking this out before ship...
#ifndef BUILD_FINAL
#define ENABLE_FPS_LOG_FILE
#endif


// Forward declarations
class BGameSettings;
class BTimingTracker;
class BWorld;
class BDebugTextDisplay;
class BUser;
class BWin32FileStream;

//==============================================================================
// BModeGame
//==============================================================================
class BModeGame : 
   public BMode, 
   public BMPSimDataObject::BMPTimingHandler,
   public BMPSimDataObject::BMPPauseHandler,
   public BMPSimDataObject::BMPPlayerHandler
{
   public:
      enum
      {
         cFlagGameInit,
         cFlagTerrainInit,
         cFlagUIGameInit,
         cFlagMinimapInit,
         cFlagPaused,
         cFlagSingleStep,
         cFlagFixedUpdate,
         cFlagRandomCameraTest,
         cFlagDebugRenderSimRep,
         cFlagDebugRenderCameraRep,
         cFlagDebugRenderFlightRep,
         cFlagSaveFogOff,
         cFlagMPGameWait
#ifndef BUILD_FINAL         
         , cFlagSmallViewport
         , cFlagSmallViewportChanged
#endif         
      };
      
                        BModeGame(long modeType);
      virtual           ~BModeGame();

      virtual bool      setup();

      virtual void      preEnter(BMode* lastMode);
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);
      virtual void      postLeave(BMode* pNewMode);

      virtual void      render();
      virtual void      update();
      virtual void      frameStart();
      virtual void      frameEnd();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      // Timing stuff
      void              setPaused(bool paused, bool requested=true, PlayerID playerID=-1);
      bool              getPaused() const { return getFlag(cFlagPaused); }
      PlayerID          getPausedPlayerID() const { return mPausedByPlayerID; }
      void              setSingleStep(bool val);
      void              setFixedUpdate(float time) { setFlag(cFlagFixedUpdate, true); mFixedUpdateTime=time; }
      void              clearFixedUpdate() { setFlag(cFlagFixedUpdate, false); }
      float             getFixedUpdate() const { if (!getFlag(cFlagFixedUpdate)) return -1.0f; else return mFixedUpdateTime; }
            
      // BMPTimingHandler
      virtual HRESULT   getLocalTiming(uint32& timing, uint32* deviationRemaining);
      virtual float     getMSPerFrame();

      // BMPSimObject::BMPPauseHandler
      virtual void      netSetPaused(bool val, PlayerID lPlayerID);
      virtual void      netSingleStep();

      // BMPSimObject::BMPPlayerHandler
      virtual void      playerDisconnected(PlayerID playerID, bool userInitiated);

      BOOL              getIsHost() const;

      bool              getFlagPlayingVideo() const { return mFlagPlayingVideo; }
      void              setFlagPlayingVideo(bool v);

      bool              getFlagWaitingOnPlayers() const { return mFlagWaitingOnPlayers; }

      // [11/14/2008 xemu] made these public so we can call them from the various crash handlers 
#ifdef ENABLE_FPS_LOG_FILE
      void              closeFPSLogFile();
      void              flushFPSLogDataToFile();
      void              openFPSLogFile();
      void              updateFPSLogFile(bool force, const char *timeOverrideText);
#endif

   protected:
      bool              initGame();
      void              deinitGame();

      bool              initWorld();
      void              deinitWorld();

      void              initParticleSystemPools();
      void              deinitParticleSystemPools();

      void              setupError();

      bool              calcUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength);      
      void              updateGame(int64 currentUpdateTime, float currentUpdateLength, bool updateSim);      

      void              renderUIShowStats();
      void              renderUserUI(BUser* pUser, int viewportIndex);
      void              renderFrameUI();
      void              renderStartOfFrame(uint macroTileIndex);
      void              renderEndOfFrame();
      bool              shouldEnableObscuredUnits(BUser* pViewportUser);
      void              renderVideos();


   private:
      bool              beginScenarioArchiveLoad();
      void              endScenarioArchiveLoad();
      void              updateSmallViewport();
      void              renderViewportDebug();
      void              renderViewport(long user, BUser* pViewportUser, uint macroTileIndex, uint viewportIndex);
#ifdef ENABLE_FPS_LOG_FILE
      void              clearFPSLogData();

      void              getFPSLogFileName(BString &fileName);
      void              getFPSLogFileNameXFS(BString &fileName);
#endif
            
      BWorld*           mpWorld;

      DWORD             mTimingTrackerUpdate;
      BTimingTracker*   mpTimingTracker;

      int64             mTimerFrequency;
      int64             mFrameStartTime;
      int64             mLastUpdateTime;
      int64             mCounterOverhead;
      int64             mLastUpdateLength;
      int64             mLastVideoTime;

      double            mTimerFrequencyFloat;
      double            mUpdateLength;
      double            mFrameLength;
      float             mFixedUpdateTime;

      uint32            mCivSoundBankID;

      PlayerID          mPausedByPlayerID;

#ifndef BUILD_FINAL
      BBarChartBarHandle mSimBarChartHandle;
      BBarChartBarHandle mTriggerObjectsBarChartHandle;
      BBarChartBarHandle mTriggerBarChartHandle;      
      BBarChartBarHandle mMemoryBarChartHandle;
      BBarChartBarHandle mMemoryBarPrimaryHeap;
      BBarChartBarHandle mMemoryBarSimHeap;
      BBarChartBarHandle mMemoryBarRendererHeap;
      BBarChartBarHandle mMemoryBarParticleHeap;
      BBarChartBarHandle mMemoryBarNetSyncHeaps;
      BBarChartBarHandle mMemoryBarPhysicalHeaps;
      BBarChartBarHandle mMemoryBarOtherHeaps;

      float              mMemoryBaseVal;
      float              mPrimaryHeapBaseSize;
      float              mSimHeapBaseSize;
      float              mRendererHeapBaseSize;
      float              mParticleHeapBaseSize;
      float              mNetSyncHeapsBaseSize;
      float              mPhysicalHeapsBaseSize;
      float              mOtherHeapsBaseSize;

#endif
      
      DWORD             mSingleStepRepeatTime;

      DWORD             mSubUpdateLastCalcUpdate;
      float             mSubUpdateLastUpdateLength;
      int64             mSubUpdateLastUpdateTime;
      int64             mSubUpdateLastOutsideUpdateStartTime;

      // Used during game startup to identify as host - doesn't set *actual* host status
      BOOL              mIsHost;

      BOOL              mIsMPRunning;

      long              mGameType;

      DWORD             mSaveGameTime;

      float             mSaveGameSpeedConfig;

      BOOL              mFlagWaitingOnPlayers;
      BOOL              mFlagWaitingOnPlayersSpinner;
      uint              mWaitingOnPlayersSpinnerTime;

      // FPS log file data
#ifdef ENABLE_FPS_LOG_FILE
      BDynamicArray<uchar*> mFPSLogBuffers;
      BWin32FileStream* mpFPSLogFileStream;
      uint              mFPSLogBufferOffset;
      double            mFPSLogNextSample;
      BString           mPerfLogFilename;
      BString           mTimeOverrideText;

      static BCriticalSection  mFPSLogLock;
      static bool              mWriteFPSLog;
      static bool              mWriteMemLog;
      static bool              mAutoCopyFPSLog;

#endif

#ifndef BUILD_FINAL
      int               mAllocSnapshotNumber;
#endif



      bool              mAllowSingleStep : 1;        // Used for singlestep updating
      bool              mFlagPlayingVideo : 1;
      bool              mFlagDoSaveNextUpdate : 1;
      bool              mFlagSavingGame : 1;
};
