//==============================================================================
// modegame.cpp
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modegame.h"
#include "keyboard.h"
#include "inputsystem.h"

// xgame
#include "configsGame.h"
#include "econfigenum.h"
#include "game.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "scenario.h"
#include "syncmanager.h"
#include "syncmacros.h"
#include "timingtracker.h"
#include "ui.h"
#include "uigame.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "entity.h"
#include "alert.h"
#include "selectionmanager.h"
#include "protoobject.h"
#include "protosquad.h"
#include "tactic.h"
#include "textvisualmanager.h"
#include "HPBar.h"
#include "recordgame.h"
#include "gamedirectories.h"
#include "archiveManager.h"
#include "statsManager.h"
#include "UIGlobals.h"
#include "worldsoundmanager.h"
#include "ScenarioList.h"
#include "tips.h"
#include "campaignmanager.h"
#include "transitionManager.h"
#include "savegame.h"
#include "viewportManager.h"
#include "scoremanager.h"
#include "deathmanager.h"
#include "fileUtils.h"
#include "damagetemplatemanager.h"
#include "terraineffectmanager.h"
#include "modemenu.h"
#include "uiticker.h"
#include "memoryStats.h"

// xgameRender
#include "render.h"
#include "camera.h"
#include "configsgamerender.h"
#include "renderControl.h"
#include "minimap.h"
#include "renderDraw.h"
#include "worldVisibility.h"
#include "lightEffectManager.h"
#include "binkInterface.h"

// xvisual
#include "grannyInstanceRenderer.h"
#include "visual.h"

// xmultiplayer
//#include "mpgameview.h"
#include "liveSystem.h"
#include "commlog.h"

// terrain
#include "TerrainSimRep.h"
#include "terrain.h"

#include "objectmanager.h"

#include "visualinstance.h"
#include "particlegateway.h"

#include "xphysics.h"

// xvince
#include "vincehelper.h"

// xparticles
#include "configsparticles.h"
#include "particleheap.h"

// xflash
#include "uimanager.h"
#include "flashmanager.h"

// xsystem
#include "notification.h"

// xgranny
#include "grannyManager.h"

// xrender
#include "ugxGeomManager.h"

#include "DCBManager.h"
#include "gpuheap.h"

// Watermarking code
#include "uiwatermark.h"

#ifndef BUILD_FINAL
   #include "consoleRender.h"
   #include "barChart.h"
   #include "triggermanager.h"
   #include "memory\allocationLogger.h"
   #include "xfs.h"
#endif

#include "flashbackgroundplayer.h"
#include "mpcommheaders.h"
#include "commlog.h"
#include "file\win32FileStream.h"


#ifdef ENABLE_FPS_LOG_FILE
   // This lib needs to be removed when doing a cert build
   #pragma comment( lib, "xbdm.lib" )
#endif


// Constants
const int cTimingTrackerSkipCount=5; // ignore the first 5 updates
const DWORD cMaxUpdateLength=100; // maximum amount of seconds that can be passed in to ::update()
const float cMaxUpdateLengthFloat=0.1f;


#ifdef ENABLE_FPS_LOG_FILE
// [11/17/2008 xemu] added so that we can deal OK if we are getting crashes that trigger log closure from a separate thread 
BCriticalSection             BModeGame::mFPSLogLock(10000);
bool                         BModeGame::mWriteFPSLog = false;
bool                         BModeGame::mWriteMemLog = false;
bool                         BModeGame::mAutoCopyFPSLog = false;
#endif

//==============================================================================
// BModeGame::BModeGame
//==============================================================================
BModeGame::BModeGame(long modeType) :
   BMode(modeType),
   mpWorld(NULL),
   mTimingTrackerUpdate(0),
   mpTimingTracker(NULL),
   mTimerFrequency(0),
   mFrameStartTime(0),
   mLastUpdateTime(0),
   mCounterOverhead(0),
   mLastUpdateLength(0),
   mLastVideoTime(0),
   mTimerFrequencyFloat(0.0f),
   mUpdateLength(0.0),
   mFrameLength(0.0),
   mFixedUpdateTime(0.0f),
   mSingleStepRepeatTime(0),
   mSubUpdateLastCalcUpdate(0),
   mSubUpdateLastUpdateLength(0.0f),
   mSubUpdateLastUpdateTime(0),
   mSubUpdateLastOutsideUpdateStartTime(0),
   mAllowSingleStep(true),
   mIsHost(FALSE),
   mCivSoundBankID(AK_INVALID_BANK_ID),
   mIsMPRunning(FALSE),
   mSaveGameTime(0),
   mSaveGameSpeedConfig(1.0f),
   mFlagPlayingVideo(false),
   mFlagSavingGame(false),
   mFlagDoSaveNextUpdate(false)
{
#ifndef BUILD_FINAL
   mAllocSnapshotNumber=0;
#endif
#ifdef ENABLE_FPS_LOG_FILE
   mpFPSLogFileStream = NULL;
   mFPSLogBufferOffset = 0;
   mFPSLogNextSample = 0.0f;
   mTimeOverrideText.set("");

   mWriteFPSLog = gConfig.isDefined(cConfigWriteFPSLog);
   mWriteMemLog = gConfig.isDefined(cConfigWriteMemLog);
   mAutoCopyFPSLog = gConfig.isDefined(cConfigAutoCopyFPSLog);
#endif
#ifndef BUILD_FINAL
   mSimBarChartHandle = cInvalidBarChartBarHandle;
   mTriggerObjectsBarChartHandle = cInvalidBarChartBarHandle;
   mTriggerBarChartHandle = cInvalidBarChartBarHandle;   
   mMemoryBarChartHandle = cInvalidBarChartBarHandle;
   mMemoryBaseVal = 0.0f;

   mMemoryBarPrimaryHeap = cInvalidBarChartBarHandle;
   mMemoryBarSimHeap = cInvalidBarChartBarHandle;
   mMemoryBarRendererHeap = cInvalidBarChartBarHandle;
   mMemoryBarParticleHeap = cInvalidBarChartBarHandle;
   mMemoryBarNetSyncHeaps = cInvalidBarChartBarHandle;
   mMemoryBarPhysicalHeaps = cInvalidBarChartBarHandle;
   mMemoryBarOtherHeaps = cInvalidBarChartBarHandle;

#endif
}

//==============================================================================
// BModeGame::~BModeGame
//==============================================================================
BModeGame::~BModeGame()
{
}

//==============================================================================
// BModeGame::setup
//==============================================================================
bool BModeGame::setup()
{
   return BMode::setup();
}

//==============================================================================
// BModeGame::preEnter
//==============================================================================
void BModeGame::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeGame::enter
//==============================================================================
void BModeGame::enter(BMode* lastMode)
{
#ifndef BUILD_FINAL
   mAllocSnapshotNumber++;
   if (gConfig.isDefined("GameModeAllocLog"))
      getAllocationLogger().logSnapshot(mAllocSnapshotNumber);
#endif
   //AllocSnapshot(mAllocSnapshotNumber);

   // Unload the ticker
   if (BUITicker::isEnabled())
      BUIScreen::remove();

#ifdef ENABLE_FPS_LOG_FILE
   if (gConfig.isDefined(cConfigWriteFPSLog))
   {
      clearFPSLogData();
      openFPSLogFile();
   }

   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter A");
#endif

   gSaveGame.setSavedCampaign(false);

   mSaveGameSpeedConfig  = 1.0f;
   gConfig.get(cConfigGameSpeed, &mSaveGameSpeedConfig);

   mSaveGameTime=0;
   mFlagPlayingVideo=false;
   mFlagSavingGame=false;
   mFlagDoSaveNextUpdate=false;

   // throw away the pregame archive now. We are done with it.
   bool success;
   if (lastMode != this)
   {
      success = gArchiveManager.endMenu();
      BVERIFY(success);
   }

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter B");
#endif

   // load up the loading screen
   success = gArchiveManager.beginLoadingMenu();
   BVERIFY(success);

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter C");
#endif

   gRenderControl.enableWorkerRender(true);

   // Tell the render thread to free its frame storage buffers, to free up some memory for the scenario load.
   gRenderThread.beginLevelLoad();

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter D");
#endif

   // Now tell the render thread to reinitialize frame storage.
   const bool initGameResult = initGame();

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter E");
#endif


   success = gArchiveManager.endLoadingMenu();
   BVERIFY(success);

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter G");
#endif

   gRenderThread.endLevelLoad();
   if (initGameResult && gUIManager)
      gUIManager->resetMinimap();

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter F");
#endif



   if (!initGameResult)
   {
      gConsoleOutput.error("BModeGame::enter: initGame() failed!");
      
      setupError();
      return;
   }

   BMode::enter(lastMode);

   gRender.resetAverageFPS();

   gInputSystem.setCaptureInput(true);

#ifdef ENABLE_FPS_LOG_FILE
   gModeManager.getModeGame()->updateFPSLogFile(true, "BGameMode::enter H");
#endif

   mGameType = -1;
//-- FIXING PREFIX BUG ID 4142
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   if(pSettings)
      pSettings->getLong(BGameSettings::cGameType, mGameType);

#ifndef BUILD_FINAL      
   setFlag(cFlagSmallViewport, false);      
   setFlag(cFlagSmallViewportChanged, false);      
#endif   
}

//==============================================================================
// BModeGame::setupError
//==============================================================================
void BModeGame::setupError()
{
   if (gGame.isSplitScreen())
      gGame.setSplitScreen(false);

   BCampaign *pCampaign = gCampaignManager.getCampaign(0);
   if (pCampaign && pCampaign->getPlayContinuous())
      pCampaign->setPlayContinuous(false);

   gModeManager.getModeMenu()->setNextState(BModeMenu::cStateLoadError);
   gModeManager.setMode(BModeManager::cModeMenu);
}

//==============================================================================
// BModeGame::leave
//==============================================================================
void BModeGame::leave(BMode* newMode)
{
   if (mFlagWaitingOnPlayersSpinner)
   {
      BUIGlobals* puiGlobals = gGame.getUIGlobals();
      if (puiGlobals)
         puiGlobals->setWaitDialogVisible(false);

      mFlagWaitingOnPlayers = FALSE;
      mFlagWaitingOnPlayersSpinner = FALSE;
   }

   gInputSystem.setCaptureInput(false);

#ifndef BUILD_FINAL   
   gRenderControl.endVideo();
#endif   

   if(mpWorld!=NULL && mpWorld->getCinematicManager() != NULL)
    mpWorld->getCinematicManager()->cancelPlayback();

   //CLM [10.01.08] we need to ensure that any message we've recieved are finished before
   //we move on to submit more 'delete' commands to the systems
   if(gRenderThread.isInsideLevelLoad())
      gRenderThread.blockUntilWorkerIdle();

   //Start rendering the mini load screen.  This will allow the render thread to automatically
   //draw and call present for the next few seconds while we do a lot of work and are unable
   //to submit present events to it.  This was put in place to satisfy TCR requirements for
   //non-interactive pause (11/21/2008 bug 18642)
   gRenderThread.startMiniLoadRendering();
      
   deinitGame();

   BDEBUG_ASSERT(gLiveSystem);

   // dispose the audio engine used for voice comms
   // but only if we're leaving this mode for something other than cModePartyRoom2
   //
   // also, the BMPSession will have been disposed in deinitGame above
   if (  (newMode == NULL) || 
         (newMode->getModeType() != BModeManager::cModePartyRoom2) )
   {
      // BMPSession makes use of voice and is now on a delayed shutdown
      //gLiveSystem->disposeVoice();
      //BCommLog::closeLogFile();
   }

   //-- Stop the post game music if it's playing
   if (!gConfig.isDefined(cConfigNoMusic))
      gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicStopPostGame);

   if (newMode != this)
   {
      gSoundManager.loadSoundBank(gSoundManager.getPregameBank(), false);
      if (!gConfig.isDefined(cConfigNoMusic))
         gSoundManager.playSoundCueByEnum(BSoundManager::cSoundMusicPlayPreGame);
   }

   if (mFlagSavingGame || mFlagDoSaveNextUpdate)
   {
      gGame.getUIGlobals()->setWaitDialogVisible(false);
      mFlagSavingGame = false;
      mFlagDoSaveNextUpdate = false;
   }

   // Reset physics memory manager
   XPhysicsMemoryDeinit();

   gConfig.set(cConfigGameSpeed, mSaveGameSpeedConfig);

   gSoundManager.setGameStartDelay(false);

   // Close log file
   #ifdef ENABLE_FPS_LOG_FILE
      if (gConfig.isDefined(cConfigWriteFPSLog))
      {
         flushFPSLogDataToFile();
         closeFPSLogFile();
         clearFPSLogData();
      }
   #endif

   //Stop rendering the mini load screen
   gRenderThread.stopMiniLoadRendering();

   BMode::leave(newMode);

#ifndef BUILD_FINAL
   mAllocSnapshotNumber++;
   if (gConfig.isDefined("GameModeAllocLog"))
      getAllocationLogger().logSnapshot(mAllocSnapshotNumber);
#endif
   //AllocSnapshot(mAllocSnapshotNumber);
}

//==============================================================================
// BModeGame::postLeave
//    - this is called AFTER enter is called on the other mode - We will be going 
//       back to a pregame mode from here and the pregame archives should be up.
//==============================================================================
void BModeGame::postLeave(BMode* pNewMode)
{
   if (!pNewMode)
      return;

   long modeType = pNewMode->getModeType();

   bool loadTicker = false;
   switch (modeType)
   {
      case BModeManager::cModeMenu:
      case BModeManager::cModeCampaign2:
      case BModeManager::cModePartyRoom2:
         loadTicker = true;
         break;
   }

   // the load ticker check should not be necessary but at this point, I just want to make sure.
   //load the UI Ticker
   if (loadTicker && !BUITicker::isEnabled())
      BUIScreen::install();

}

//==============================================================================
// BModeGame::update
//==============================================================================
void BModeGame::update()
{
   SCOPEDSAMPLE(BModeGameUpdate)

   if (!getFlag(cFlagGameInit))
      return;

/*
   if (getFlag(cFlagMPGameWait))
   {
      BMPSession* pMPSession = gLiveSystem->getMPSession();
      if (pMPSession->getAreAllPlayersLoaded())
      {
         // everybody is ready now, turn the flag off, pull down the wait dialog and let's go.
         gGame.getUIGlobals()->setWaitDialogVisible(false);
         setFlag(cFlagMPGameWait, false);
      }
      else
      {
         // not ready yet, wait till everybody is loaded.
         return;
      }
   }
*/

   LARGE_INTEGER currentTime;
   QueryPerformanceCounter(&currentTime);
   if (gConfig.isDefined(cConfigSubUpdTrace))
      gConsole.output(cChannelSim, "BModeGame::update at %f -------------------------------------------------------------------------", currentTime.QuadPart/mTimerFrequencyFloat);

   // DON'T ADD ANY NEW CODE HERE UNLESS YOU ARE SURE... 
   // MOST LIKELY YOUR CODE SHOULD GO IN updateGame() INSTEAD.
   BMode::update();
   float currentUpdateLength=0.0f;
   int64 currentUpdateTime=mLastUpdateTime;
   bool retval = true;

   // mFlagWaitingOnPlayers is set to TRUE when we're playing coop campaign
   if (mFlagWaitingOnPlayers)
   {
      if (!mFlagWaitingOnPlayersSpinner && mWaitingOnPlayersSpinnerTime == 0)
      {
         mWaitingOnPlayersSpinnerTime = timeGetTime() + 1000;
      }
      else if (!mFlagWaitingOnPlayersSpinner && timeGetTime() > mWaitingOnPlayersSpinnerTime)
      {
         // display spinner
         BUIGlobals* puiGlobals = gGame.getUIGlobals();
         if (puiGlobals)
            puiGlobals->showWaitDialog(gDatabase.getLocStringFromID(25463));
         mFlagWaitingOnPlayersSpinner = TRUE;
      }
      // query network layer for all clients pending on finalize or network shutdown
      // kill spinner
      BMPSession* pMPSession = gLiveSystem->getMPSession();
      if (pMPSession == NULL || pMPSession->allPlayersLoaded())
      {
         BUIGlobals* puiGlobals = gGame.getUIGlobals();
         if (puiGlobals)
            puiGlobals->setWaitDialogVisible(false);

         mFlagWaitingOnPlayers = FALSE;
         mFlagWaitingOnPlayersSpinner = FALSE;

         BUser* pUser = gUserManager.getPrimaryUser();
         if (pUser)
         {
            pUser->setFlagCameraScrollEnabled(true);
            pUser->setFlagCameraYawEnabled(true);
            pUser->setFlagCameraZoomEnabled(true);
         }
      }
      else
      {
         retval = false;
      }
   }
   else if (!mFlagPlayingVideo)
   {
      if (gEnableSubUpdating)
      {
         // have we run the last update fully?
#ifdef DECOUPLED_UPDATE
         if ((gDecoupledUpdate && mpWorld->getDecoupledUpdateEnd()) || (!gDecoupledUpdate && (mpWorld->getLastUpdateRealtime() == 0 || mSubUpdateLastCalcUpdate != mpWorld->getUpdateNumber())))
#else
         if (mpWorld->getLastUpdateRealtime() == 0 || mSubUpdateLastCalcUpdate != mpWorld->getUpdateNumber())
#endif
         {         
            // if so, calc some new update times
            retval=calcUpdateTimes(currentUpdateTime, currentUpdateLength);
            if (gConfig.isDefined(cConfigSubUpdTrace))
               gConsole.output(cChannelSim, "  calcUpdateTimes returned %d, currentUpdateLength %f", retval, currentUpdateLength);

            if (retval)
            {
               mSubUpdateLastUpdateLength = currentUpdateLength;
               mSubUpdateLastUpdateTime = currentUpdateTime;
               mSubUpdateLastCalcUpdate = mpWorld->getUpdateNumber();
            }                  
         }
         else // if not, let sub-updating keep going with the last update's timing values
         {         
            currentUpdateLength = mSubUpdateLastUpdateLength;
            currentUpdateTime = mSubUpdateLastUpdateTime;
            if (gConfig.isDefined(cConfigSubUpdTrace))
               gConsole.output(cChannelSim, "  using old currentUpdateLength %f", currentUpdateLength);
         }
      }  
      else
         retval=calcUpdateTimes(currentUpdateTime, currentUpdateLength);
   }
   
   LARGE_INTEGER startTime;
   QueryPerformanceCounter(&startTime);
   
   updateGame(currentUpdateTime, currentUpdateLength, retval);

#ifdef DECOUPLED_UPDATE
   if (gDecoupledUpdate && !retval)
      mpWorld->setDecoupledUpdateRender(true);
#endif

   if (retval)
   {      
      LARGE_INTEGER endTime;
      QueryPerformanceCounter(&endTime);
      int64 delta = mLastUpdateLength = (int64)(endTime.QuadPart - startTime.QuadPart - mCounterOverhead);
      mUpdateLength = static_cast<double>(delta/mTimerFrequencyFloat);
      if (!gEnableSubUpdating)
         mpTimingTracker->addLastUpdateLength(mUpdateLength); // otherwise this is done within world->update
   }
}

//==============================================================================
// BModeGame::calcUpdateTimes
//==============================================================================
bool BModeGame::calcUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength)
{
   if(gWorld->getFlagGameOver())
   {
      if(gRecordGame.isPlaying() || gRecordGame.isRecording())
         gRecordGame.stop();

      // SLB: Let the game keep updating after it's over
      //return false;
   }

   float gameSpeed=1.0f;
   gConfig.get(cConfigGameSpeed, &gameSpeed);
   gParticleGateway.setTimeSpeed(gameSpeed);

   if(gRecordGame.isPlaying())
   {
      if(getFlag(cFlagPaused))
      {
         if(getFlag(cFlagSingleStep))
            setFlag(cFlagSingleStep, false);
         else
            return false;
      }
      if (gEnableSubUpdating && !gDecoupledUpdate)
      {
         if (mpWorld->getUpdateNumber() == 0)
            return true;
         currentUpdateLength = mpWorld->getSubUpdateTotalTimeInMsecs()/1000.0f;
         if(gameSpeed!=1.0f)
            currentUpdateLength*=gameSpeed;         
         DWORD len = static_cast<DWORD>(currentUpdateLength * mTimerFrequencyFloat);
         currentUpdateTime=mpWorld->getLastUpdateTime()+len;
         return true;
      }
      return gRecordGame.playUpdateTimes(currentUpdateTime, currentUpdateLength, gWorld->getUpdateNumber(), gWorld->getLastUpdateTime(), mTimerFrequencyFloat);
   }

   int64 lastUpdateTime=mLastUpdateTime;

   if(getFlag(cFlagPaused) && getFlag(cFlagSingleStep))
      currentUpdateTime=lastUpdateTime+(DWORD)(1.0f/30.0f*mTimerFrequencyFloat); // lock at 30 fps when single stepping
   else
   {
      //currentUpdateTime=timeGetTime();
      LARGE_INTEGER time;
      QueryPerformanceCounter(&time);
      currentUpdateTime=time.QuadPart;
   }

   mLastUpdateTime=currentUpdateTime;

   if (mpWorld->getUpdateNumber() == 0)
      return true;

   // SLB: Don't use MPSession's timing after the game is over
   if (mIsMPRunning && !gWorld->getFlagGameOver())
   {
      BMPSession* pMPSession = gLiveSystem->getMPSession();
      if (!pMPSession)
         return false;

      DWORD mpTiming = pMPSession->advanceGameTiming();
      if (mpTiming == 0)
         return false;

      currentUpdateLength = static_cast<float>(mpTiming*0.001f);

      // ajl 11/20/08 - Only cap the update time if subupdating is not on.
      bool capUpdateTime;
      if (gEnableSubUpdating && !gDecoupledUpdate)
         capUpdateTime = false;
      else
         capUpdateTime = true;

      if (capUpdateTime)
      {
         if (currentUpdateLength > cMaxUpdateLengthFloat)
            currentUpdateLength = cMaxUpdateLengthFloat;
      }

      if(gameSpeed!=1.0f)
         currentUpdateLength*=gameSpeed;         

      DWORD len = static_cast<DWORD>(currentUpdateLength * mTimerFrequencyFloat);
      currentUpdateTime=mpWorld->getLastUpdateTime()+len;

      if (getFlag(cFlagPaused))
      {
#ifndef BUILD_FINAL
         if (getFlag(cFlagSingleStep))
            setFlag(cFlagSingleStep, false);
         else
#endif
            return false;
      }
      return true;
   }

   //-- pause the particle system
   // rg [11/22/06] - This is now called in render
   //gParticleGateway.pauseUpdate(getFlag(cFlagPaused));

   if(getFlag(cFlagPaused))
   {
      if(getFlag(cFlagSingleStep))
         setFlag(cFlagSingleStep, false);
      else
         return false;
   }

   if(getFlag(cFlagFixedUpdate))
   {
      currentUpdateLength=mFixedUpdateTime;
      DWORD len = (DWORD)(currentUpdateLength * mTimerFrequencyFloat);
      currentUpdateTime=mpWorld->getLastUpdateTime()+len;
   }
   else if (gEnableSubUpdating && !gDecoupledUpdate)
   {
      if (mpWorld->getDynamicSubUpdateTime())
         currentUpdateLength = (mpWorld->calcAdjustedSubUpdateTime() * mpWorld->getAmountOfSubUpdates()) * 0.001f;
      else
         currentUpdateLength = mpWorld->getSubUpdateTotalTimeInMsecs()*0.001f;

      if(gameSpeed!=1.0f)
         currentUpdateLength*=gameSpeed;         

      DWORD len = static_cast<DWORD>(currentUpdateLength * mTimerFrequencyFloat);
      currentUpdateTime=mpWorld->getLastUpdateTime()+len;
   }
   else
   {
      int64 delta=currentUpdateTime-lastUpdateTime;
      currentUpdateLength = static_cast<float>(delta/mTimerFrequencyFloat);
      if(currentUpdateLength>cMaxUpdateLengthFloat)
         currentUpdateLength=cMaxUpdateLengthFloat;

      if(gameSpeed!=1.0f)
         currentUpdateLength*=gameSpeed;         

      DWORD len = (DWORD)(currentUpdateLength * mTimerFrequencyFloat);
      currentUpdateTime=mpWorld->getLastUpdateTime()+len;
   }

   if (gWorld->getFlagGameOver())
      return !gWorld->adjustGameOverCountdown(currentUpdateLength);

   return true;
}

//==============================================================================
// BModeGame::updateGame
//==============================================================================
void BModeGame::updateGame(int64 currentUpdateTime, float currentUpdateLength, bool updateSim)
{
   SCOPEDSAMPLE(BModeGameUpdateGame);

   // FPS log file
   #ifdef ENABLE_FPS_LOG_FILE
      if (gConfig.isDefined(cConfigWriteFPSLog))
         updateFPSLogFile(false, NULL);
   #endif


#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif

   gpDebugPrimitives->beginFrame(mpWorld->getGametimeFloat());
   
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryPhysics);
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryMovement);

   gUIManager->clear2DPrimitives();   

   gHPBar.prep();
   
   if(updateSim)
   {
      if (mFlagPlayingVideo)
      {
         if( !getPaused() )
         {
            LARGE_INTEGER time;
            QueryPerformanceCounter(&time);
            int64 delta = time.QuadPart - mLastVideoTime;
            float elapsed = static_cast<float>(delta/mTimerFrequencyFloat);
            if (elapsed>cMaxUpdateLengthFloat)
               elapsed=cMaxUpdateLengthFloat;
            mLastVideoTime = time.QuadPart;
            gWorld->getCinematicManager()->update(elapsed);
            mpWorld->getWorldSoundManager()->update(elapsed);
         }
      }
      else
      {
         gGrannyManager.resetFrameStats();
         gpDebugPrimitives->clear(BDebugPrimitives::cCategoryAI);
         gFontManager.onUpdateSimStart();

         if (!gEnableSubUpdating || gDecoupledUpdate)
         {
            if(gRecordGame.isRecording())
            {
               SCOPEDSAMPLE(RecordGameUpdate);
               gRecordGame.recordUpdateTimes(currentUpdateTime, currentUpdateLength, gWorld->getUpdateNumber());
               gRecordGame.recordUser();
            }
            else if(gRecordGame.isPlaying())
            {
               SCOPEDSAMPLE(RecordGamePlay);
               gRecordGame.playUser();
            }                                                              
         }
         else if (mpWorld->getDynamicSubUpdateTime())
         {
            if (gWorld->isOnFirstSubUpdate() && currentUpdateLength > 0.0f)
               gWorld->setAdjustedSubUpdateTime((long)(currentUpdateLength*1000.0f/mpWorld->getAmountOfSubUpdates()));
            if (gRecordGame.isRecording())
               gRecordGame.recordSubUpdateTime();
            else if (gRecordGame.isPlaying())
               gRecordGame.playSubUpdateTime();
         }
      }
   }

   if (gEnableSubUpdating && !gDecoupledUpdate && !mFlagPlayingVideo && updateSim)
   {
      if (gRecordGame.isRecording())
         gRecordGame.recordUser();
      else if (gRecordGame.isPlaying())
         gRecordGame.playUser();
   }

   LARGE_INTEGER currentTime;
   QueryPerformanceCounter(&currentTime);
   bool isRenderFrame = true;
#ifdef DECOUPLED_UPDATE
   if(gDecoupledUpdate && !gWorld->getDecoupledUpdateRender())
      isRenderFrame = false;
#endif
   if (mSubUpdateLastOutsideUpdateStartTime != 0 && mpTimingTracker && isRenderFrame)
   {
      if (gConfig.isDefined(cConfigSubUpdTrace))
         gConsole.output(cChannelSim, "  last outside update time %f", (currentTime.QuadPart - mSubUpdateLastOutsideUpdateStartTime) / mTimerFrequencyFloat);
      mpTimingTracker->addLastOutsideUpdateLength((currentTime.QuadPart - mSubUpdateLastOutsideUpdateStartTime) / mTimerFrequencyFloat);
   }

   if (updateSim && !mFlagPlayingVideo)   
      mpWorld->update(currentUpdateTime, currentUpdateLength, mTimerFrequencyFloat, true, mpTimingTracker);

   QueryPerformanceCounter(&currentTime);
   mSubUpdateLastOutsideUpdateStartTime = currentTime.QuadPart;

   if (updateSim && !mFlagPlayingVideo)
   {
      gVisualManager.update();

      // need a game stats init/reset before we begin a game
      gStatsManager.update();
   }   

   // Reset game start fade up time based on the fade delay. The fade delay is in world time while the actual 
   // fade up time is in real time. This is done so that a fixed amount of world time has to elapse before
   // the fading up starts, and this helps hide initial jerkiness that happens on game start. It also hides
   // the initial frame of that normally gets rendered which shows the full world without fog of war while
   // that system gets initialized.
   float fadeDelay = 0.0f;
   gConfig.get(cConfigGameStartFadeDelay, &fadeDelay);
   if (fadeDelay > 0.0f)
   {
      if (mpWorld->getGametimeFloat() < fadeDelay && gWorld->getTransitionManager()->getTransitionCounter() == 1)
         gWorld->getTransitionManager()->resetTransition();
   }

   // moving this out of the world updatePreAsync so that it still works when the game is over
   gWorld->getTransitionManager()->update();

   //if(!gWorld->isPlayingCinematic())
   //{
   //   if (!gConfig.isDefined(cConfigFlashGameUI))
   //   {
   //      SCOPEDSAMPLE(MiniMapUpdate);
   //      gMiniMap.update();
   //   }
   //}

   {
      SCOPEDSAMPLE(FlashUIUpdate);
      gUIManager->update(gGame.getFrameTime() * 0.001f);
   }

#ifndef BUILD_FINAL
   timer.stop();
   double elapsedUpdateTime =  timer.getElapsedSeconds();
   if (gConfig.isDefined(cConfigEnableFPSLock))
   {      
      float targetFPS = 30.0f;
      gConfig.get(cConfigFPSLockRate, &targetFPS);
      double fpsLockTime = 1.0f / targetFPS;
      double delta = fpsLockTime - elapsedUpdateTime;
      if (delta > 0.0f)
         Sleep((DWORD)(delta * 1000.0f));
   }
#endif

   #ifndef BUILD_FINAL
   if (!getPaused())
   {
      long pauseOnUpdateNumber;
      if (gConfig.get(cConfigPauseOnUpdate, &pauseOnUpdateNumber))
      {
         long updateNumber=gWorld->getUpdateNumber();
         if (updateNumber+1 == pauseOnUpdateNumber)
            setPaused(true);
      }
   }
   #endif

  
}

//==============================================================================
// BModeGame::frameStart
//==============================================================================
void BModeGame::frameStart()
{
   if (!getFlag(cFlagGameInit))
      return;
      
   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   mFrameStartTime=time.QuadPart;
   mUpdateLength=0.0;
   mFrameLength=0.0;
}

//==============================================================================
// BModeGame::frameEnd
//==============================================================================
void BModeGame::frameEnd()
{
   if (!getFlag(cFlagGameInit))
      return;
      
   LARGE_INTEGER frameEndTime;
   QueryPerformanceCounter(&frameEndTime);

   int64 delta = (int64)(frameEndTime.QuadPart - mFrameStartTime - mCounterOverhead - mLastUpdateLength);
   mFrameLength = static_cast<double>(delta/mTimerFrequencyFloat);

   mpTimingTracker->addLastFrameLength(mFrameLength);

   // Handle save/load game requests.
   if (gSaveGame.getDoSaveGame() && gWorld->isOnFirstSubUpdate())
   {
      if (mFlagDoSaveNextUpdate)
      {
         mFlagDoSaveNextUpdate = false;
         mFlagSavingGame = true;
         mSaveGameTime = timeGetTime();

         // Let physics update complete before saving
         BPhysicsWorld* pPhysicsWorld = gWorld->getPhysicsWorld();
         if (pPhysicsWorld)
         {
            pPhysicsWorld->waitForUpdateCompletion();
            pPhysicsWorld->markForRead(true);
         }

         gSaveGame.doSaveGameBegin();

         if (pPhysicsWorld)
            pPhysicsWorld->markForRead(false);
      }
      else
      {
         mFlagDoSaveNextUpdate = true;
         gGame.getUIGlobals()->showWaitDialog(gDatabase.getLocStringFromID(23501));
      }
      return;
   }

   if (mFlagSavingGame)
   {
      if (!gSaveGame.getSaveFileInProgress())
      {
         // Make sure dialog is displayed at least 3 seconds
         if (timeGetTime() - mSaveGameTime < 3000)
            return;
         gGame.getUIGlobals()->setWaitDialogVisible(false);
         mFlagSavingGame = false;
         gSaveGame.doSaveGameEnd();
         if (gUIManager)
            gUIManager->refreshUI();
         return;
      }
   }

   if (gSaveGame.getDoLoadGame())
   {
      gSaveGame.doLoadGame();
      return;
   }

   // Handle delayed scenario re-loading (this is needed since the scenario editor changes multiple files and
   // causes multiple file change events to get sent which could cause multiple reloads.
   if (gScenario.getFlagReload() && !mIsMPRunning)
   {
      DWORD elapsed = timeGetTime() - gScenario.getReloadTime();
      float delay = 0.0f;
      gConfig.get(cConfigReloadScenarioWaitTime, &delay);
      if (elapsed >= (DWORD)(delay * 1000.0f))
      {
         gScenario.load(true, true);
         return;
      }
   }
}

//==============================================================================
// BModeGame::renderUIShowStats
//==============================================================================
void BModeGame::renderUIShowStats(void)
{
#ifndef BUILD_FINAL
   BDisplayStats::show(mpWorld, mpTimingTracker, &gUserManager);

 
#endif
}

//==============================================================================
// BModeGame::RenderVideos
//==============================================================================
void BModeGame::renderVideos()
{
   gBinkInterface.decompressAllVideos();
   gBinkInterface.renderAllVideos();
   gBinkInterface.advanceAllVideos();
}

//==============================================================================
// BModeGame::renderUserUI
//==============================================================================
void BModeGame::renderUserUI(BUser* pUser, int viewportIndex)
{     
   SCOPEDSAMPLE(RenderUserUI);
       

   gRenderDraw.resetMainActiveMatricesAndViewport();
   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.setDefaultSamplerStates();
   
   // ajl fixme 11/27/07 - should only render some UI once for split screen (such as minimap)
   //bool isPrimary = (pUser == gUserManager.getPrimaryUser());

   // Disable Z here because the device's depth buffer isn't even valid due to tiling.
   gRenderDraw.setRenderState(D3DRS_ZENABLE, FALSE);
   gRenderDraw.setDepthStencilSurface(NULL);
   

   //timings
#ifndef BUILD_FINAL
   if(!gConfig.isDefined(cConfigTimingAlertsOFF) && !gConfig.isDefined(cConfigDisableUI) && viewportIndex==0)
   {
      
      const BWorld::BStats &stats = mpWorld->getTimeStats();
      bool paused = getPaused();
      const float updateTime = paused ? 0 : (float)(stats.mTotalUpdateTime * 1000.0f);

      if (mSimBarChartHandle == cInvalidBarChartBarHandle)
         mSimBarChartHandle = gRenderControl.getBarChart()->addBar(L"Sim Update Time(ms)",0xFFaaBBCC);

      gRenderControl.getBarChart()->setBarValue(mSimBarChartHandle,updateTime);

      // Trigger objects update
      if (mTriggerObjectsBarChartHandle == cInvalidBarChartBarHandle)
         mTriggerObjectsBarChartHandle = gRenderControl.getBarChart()->addBar(L"Trigger Objects Perf", 0xFFFF7A14);

      const float triggerObjectsPerf = paused ? 0.0f : (float)(stats.mTriggerObjectsUpdateTime * 1000.0f);
      gRenderControl.getBarChart()->setBarValue(mTriggerObjectsBarChartHandle, triggerObjectsPerf);

      // Trigger system update
      if (mTriggerBarChartHandle == cInvalidBarChartBarHandle)
         mTriggerBarChartHandle = gRenderControl.getBarChart()->addBar(L"Trigger Perf", 0xFFA02DFF);

      float triggerPerf = paused ? 0.0f : (float)(stats.mTriggerUpdateTime * 1000.0f);
      if (gTriggerManager.getFlagPerfSpiked())
         triggerPerf = 100.0f;

      gRenderControl.getBarChart()->setBarValue(mTriggerBarChartHandle, triggerPerf);

      if (mMemoryBarChartHandle == cInvalidBarChartBarHandle)
      {
         mMemoryBarChartHandle = gRenderControl.getBarChartMem()->addBar(L"Mem Growth",0xFFaaBBCC);

         // [11/5/2008 xemu] store off initial value of how much is free 
         // [11/5/2008 xemu] set this as our "max" value, in terms of memory growth (because then we hit zero), but actually scale down so we start
         //          showing that we have problems when we hit 1/4 of memory gone
         // [11/5/2008 xemu] so if we have 30M free at start, we want the bar to go from 0M new allocation to 30M new allocation
         mMemoryBaseVal = getHeapBarChartValue();
         gRenderControl.getBarChartMem()->setBarMaxValue(mMemoryBaseVal * 0.33f);

         mPrimaryHeapBaseSize = gPrimaryHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
         mSimHeapBaseSize = gSimHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
         mRendererHeapBaseSize = gRenderHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
         mParticleHeapBaseSize = gParticleBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
         mNetSyncHeapsBaseSize = gNetworkHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + 
                                 gSyncHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
         mPhysicalHeapsBaseSize = gPhysCachedHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + 
                                  gPhysWriteCombinedHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
         mOtherHeapsBaseSize = gStringHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + 
                               gFileBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) +
                               gCRunTimeHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      }

      float currFreeMem = getHeapBarChartValue();

      // [11/5/2008 xemu] show how far along that progression we are.  If we have 25M free, then we have used 5M. 
      float grownMem = mMemoryBaseVal - currFreeMem;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarChartHandle, grownMem);

      if (mMemoryBarPrimaryHeap == cInvalidBarChartBarHandle)
         mMemoryBarPrimaryHeap = gRenderControl.getBarChartMem()->addBar("Primary",0xFFFF7A14);
      float heapSize = gPrimaryHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mPrimaryHeapBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarPrimaryHeap, grownMem );

      if (mMemoryBarSimHeap == cInvalidBarChartBarHandle)
         mMemoryBarSimHeap = gRenderControl.getBarChartMem()->addBar("Sim",0xFFA02DFF);
      heapSize = gSimHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mSimHeapBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarSimHeap, grownMem);

      if (mMemoryBarRendererHeap == cInvalidBarChartBarHandle)
         mMemoryBarRendererHeap = gRenderControl.getBarChartMem()->addBar("Renderer",0xFFFF7A14);
      heapSize = gRenderHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mRendererHeapBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarRendererHeap, grownMem);

      if (mMemoryBarParticleHeap == cInvalidBarChartBarHandle)
         mMemoryBarParticleHeap = gRenderControl.getBarChartMem()->addBar("Particle",0xFFA02DFF);
      heapSize = gParticleBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mParticleHeapBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarParticleHeap, grownMem);
      
      if (mMemoryBarNetSyncHeaps == cInvalidBarChartBarHandle)
         mMemoryBarNetSyncHeaps = gRenderControl.getBarChartMem()->addBar("NetSync",0xFFFF7A14);
      heapSize = gNetworkHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + gSyncHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mNetSyncHeapsBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarNetSyncHeaps, grownMem);

      if (mMemoryBarPhysicalHeaps == cInvalidBarChartBarHandle)
         mMemoryBarPhysicalHeaps = gRenderControl.getBarChartMem()->addBar("Physical",0xFFA02DFF);
      heapSize = gPhysCachedHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + gPhysWriteCombinedHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mPhysicalHeapsBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarPhysicalHeaps, grownMem);

      // [11/14/2008 xemu] hack-o-matic
      /*
      if (grownMem > 20.0f)
      {
         static int hackSnapshotCount = 1;
         getAllocationLogger().logSnapshot(hackSnapshotCount);
         hackSnapshotCount++;
      }
      */

      if (mMemoryBarOtherHeaps == cInvalidBarChartBarHandle)
         mMemoryBarOtherHeaps = gRenderControl.getBarChartMem()->addBar("Other",0xFFFF7A14);
      heapSize = gStringHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + 
                 gFileBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f) + 
                 gCRunTimeHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
      grownMem = heapSize - mOtherHeapsBaseSize;
      gRenderControl.getBarChartMem()->setBarValue(mMemoryBarOtherHeaps, grownMem);


#ifndef BUILD_DEBUG
      if (BDisplayStats::getMode() == BDisplayStats::cDSMDisabled)
      {
         gRenderControl.getBarChart()->render();
         gRenderControl.getBarChartMem()->render();
      }
#endif

   }
#endif


#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableUI))
#endif   
   {
      if(!gWorld->isPlayingCinematic())
      {
         //-- Text Visuals
         gTextVisualManager.render(viewportIndex, gGame.isSplitScreen());
                  
         pUser->preRenderViewportUI();

         //-- HP Bars
         gHPBar.render(viewportIndex, gGame.isSplitScreen());         

//#ifndef BUILD_FINAL
//         if (!BDisplayStats::isDisplaying())
//#endif      
//         {
//            //-- render the old minimap for if flash is off
//            if (!gConfig.isDefined(cConfigFlashGameUI))
//               gMiniMap.render();
//         }

         if (gConfig.isDefined(cConfigFlashGameUI))
         {         
            SCOPEDSAMPLE(UIManagerRender);
                                    
            gUIManager->setFlag(BUIManager::eFlagWireframe, gRenderControl.getFlag(BRenderControl::cFlagDebugRenderWireframe));
            gUIManager->renderUser();
         }

         pUser->renderViewportUI();
      }
      
      gFontManager.render3D();
   }  
   
   gRenderDraw.unsetTextures();
   
   if (!gViewportManager.isLastViewport())
      gUIManager->releaseGPUHeapResources();
}

//==============================================================================
// BModeGame::renderStartOfFrame
//==============================================================================
void BModeGame::renderStartOfFrame(uint macroTileIndex)
{
   BUser* users[2];
   if (gViewportManager.isSplitScreen())
   {
      BDEBUG_ASSERT(gViewportManager.getNumViewports() == 2);
      users[0] = gUserManager.getUser(BUserManager::cSecondaryUser);
      users[1] = gUserManager.getUser(BUserManager::cPrimaryUser);
   }
   else
   {
      BDEBUG_ASSERT(gViewportManager.getNumViewports() == 1);
      users[0] = gUserManager.getUser(BUserManager::cPrimaryUser);
      users[1] = NULL;
   }
   
   gRenderControl.startOfFrame(gWorld, gViewportManager.getNumViewports(), users, macroTileIndex);
         
   {
      SCOPEDSAMPLE(BModeGame_renderViewport_generateMinimapVisibility);
      gUIManager->generateMinimapVisibility();
      //if (!gConfig.isDefined(cConfigFlashGameUI))
      //   gMiniMap.generateVisibility();
   }         
}

//==============================================================================
// BModeGame::shouldEnableObscuredUnits
//==============================================================================
bool BModeGame::shouldEnableObscuredUnits(BUser* pViewportUser)
{
   bool enableObscuredUnits = true;

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigDisableObscuredUnits))
      enableObscuredUnits = false;
#endif

   if ((pViewportUser->getUserMode() == BUser::cUserModeCinematic) || (mGameType == BGameSettings::cGameTypeCampaign))
      enableObscuredUnits = false;
      
   return enableObscuredUnits;
}

//==============================================================================
// BModeGame::renderViewportDebug
//==============================================================================
void BModeGame::renderViewportDebug()
{
#ifndef BUILD_FINAL   
   if (getFlag(cFlagDebugRenderSimRep))
      gTerrainSimRep.debugRender();

   if (getFlag(cFlagDebugRenderCameraRep) && gTerrainSimRep.cameraHeightsLoaded())
      gTerrainSimRep.debugRenderCameraHeights();

   if (getFlag(cFlagDebugRenderFlightRep) && gTerrainSimRep.flightHeightsLoaded())
      gTerrainSimRep.debugRenderFlightHeights();

   mpWorld->debugRender();

#endif       
}

//==============================================================================
// BModeGame::renderViewport
//==============================================================================
void BModeGame::renderViewport(long user, BUser* pViewportUser, uint macroTileIndex, uint viewportIndex)
{
   SCOPEDSAMPLE(BModeGameRenderViewport);
      
   gRenderControl.beginViewport(user, pViewportUser, viewportIndex, shouldEnableObscuredUnits(pViewportUser), true);
         
#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableUI))
#endif   
   {
      pViewportUser->updateViewportUI();
   }
   
   renderViewportDebug();
                  
   gRenderControl.renderViewport();   
         
   renderUserUI(pViewportUser, viewportIndex);
   
   gRenderControl.endViewport();
}

//==============================================================================
// BModeGame::renderFrameUI
//==============================================================================
void BModeGame::renderFrameUI()
{
   gViewportManager.setCurrentUser(-1);
   gViewportManager.setCurrentViewport(-1);
      
   gRender.beginFullScreenUI();

   BUIWidgets* pWidgets = gUIManager->getWidgetUI();

#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableUI))
#endif   
   {
      // Letterbox
      mpWorld->getCinematicManager()->postRender();
      
      // Non-Game UI (Pause/Objectives/etc.)
      gUIManager->renderNonGameUI();      
      
      // Fade-to-Black
      mpWorld->getTransitionManager()->postRender();

      // Cinematics
      BBinkVideoHandle videoHandle = gWorld->getCinematicManager()->getVideoHandle();
      if (videoHandle != cInvalidVideoHandle)
      {
         gBinkInterface.decompressVideo(videoHandle);
         gBinkInterface.renderVideo(videoHandle);
         gBinkInterface.advanceVideo(videoHandle);
         mpWorld->getCinematicManager()->postRender();
      }

      // Widgets
      gUIManager->renderWidgets();

      // Minimap
      gUIManager->renderMinimap();

      // HUD/Circle Menu
      BUser* pPrimaryUser = gUserManager.getPrimaryUser();
      if (pPrimaryUser && !gUIManager->isNonGameUIVisible())
      {
         pPrimaryUser->renderFrameUI();
      }

      // Talking Head
      if( pWidgets )
      {
         BUIScreen* pTalkingHeadScreen = pWidgets->getTalkingHead().getScreen();
         if (pTalkingHeadScreen)
            pTalkingHeadScreen->render();
         pWidgets->getTalkingHead().renderVideo();
      }

      // Global Dialogs
      BUIGlobals* puiGlobals = gGame.getUIGlobals();
      BASSERT(puiGlobals);
      puiGlobals->render();
   }

   gFontManager.render2D();

#ifndef BUILD_FINAL
   gConsoleRender.renderSubmit();

   if (!gConfig.isDefined(cConfigDisableUI))
#endif   
   {
      if (gConfig.isDefined(cConfigAlpha))
      {
   #ifndef BUILD_FINAL
         if (!gConfig.isDefined(cConfigDisableWatermark))
   #endif   
         {
            if (!gUIManager->isNonGameUIVisible())
               gWatermarkUI.render();
         }            
      }
   }

   renderUIShowStats();

   renderSafeAreas();
   
   gRender.endFullScreenUI();
}

//==============================================================================
// BModeGame::renderEndOfFrame
//==============================================================================
void BModeGame::renderEndOfFrame()
{
   gUIManager->releaseGPUHeapResources();
         
   gRenderControl.endOfFrame();
}

#ifndef BUILD_FINAL
//==============================================================================
// BModeGame::updateSmallViewport
//==============================================================================
void BModeGame::updateSmallViewport()
{
   if (!getFlag(cFlagSmallViewportChanged))
      return;
   setFlag(cFlagSmallViewportChanged, false);
   
   uint smallViewportWidth = 512;
   uint smallViewportHeight = 512;
   while ((smallViewportWidth > gViewportManager.getBackBufferWidth()) || (smallViewportHeight > gViewportManager.getBackBufferHeight()))
   {
      smallViewportWidth >>= 1;
      smallViewportHeight >>= 1;
   }

   if (getFlag(cFlagSmallViewport))
      gViewportManager.set(gViewportManager.isSplitScreen(), gViewportManager.isVerticalSplit(), smallViewportWidth, smallViewportHeight);
   else 
      gViewportManager.set(gViewportManager.isSplitScreen(), gViewportManager.isVerticalSplit(), -1, -1);
}
#endif

//==============================================================================
// BModeGame::render
//==============================================================================
void BModeGame::render()
{  
   if (!getFlag(cFlagGameInit))
      return;

#ifdef DECOUPLED_UPDATE
   if (gDecoupledUpdate && !mpWorld->getDecoupledUpdateRender())
      return;
#endif

   SCOPEDSAMPLE(BModeGameRender);

#ifndef BUILD_FINAL
   updateSmallViewport();
#endif   
   
   const uint numMacroTiles = gRenderControl.getNumMacroTiles();            
   for (uint macroTileIndex = 0; macroTileIndex < numMacroTiles; macroTileIndex++)
   {
      renderStartOfFrame(macroTileIndex);
               
      if (mFlagPlayingVideo)
         gRenderThread.frameBegin();
      else
      {
         for (uint viewportIndex = 0; viewportIndex < gViewportManager.getNumViewports(); viewportIndex++)
         {
            long user = BUserManager::cPrimaryUser;
            if (gViewportManager.isSplitScreen() && !viewportIndex)
               user = BUserManager::cSecondaryUser;
                                 
            BUser* pViewportUser = gUserManager.getUser(user);
            
            renderViewport(user, pViewportUser, macroTileIndex, viewportIndex);         
         }
      }
                  
      renderFrameUI();      
      
      renderEndOfFrame();
   }      
}


//==============================================================================
// BModeGame::handleInput
//==============================================================================
bool BModeGame::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port; event; controlType; detail;

   if (!getFlag(cFlagGameInit))
      return false;
      
   if (gRenderControl.handleInput(port, event, controlType, detail))
      return true;

#ifndef BUILD_FINAL   
   if (BDisplayStats::handleInput(port, event, controlType, detail))
      return true;
#endif   
      
   if(event==cInputEventControlStart)
   {
      switch(controlType)
      {
#ifndef BUILD_FINAL         
         //Halwes - 12/15/2006 - Now handled in BUser::handleInput in conjunction with objective menu logic
         //case cButtonBack:
         //{
         //   if(!(gInputSystem.getGamepad(port).isControlActive(cTriggerLeft) && gInputSystem.getGamepad(port).isControlActive(cTriggerRight)))
         //   {
         //      setFlag(cFlagShowStats, !getFlag(cFlagShowStats));
         //      return true;
         //   }
         //   break;
         //}            
         case cKeyS:
         {
            if ((gInputSystem.getKeyboard()->isKeyActive(cKeyAlt)) && (!gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)))
            {
               if (gInputSystem.getKeyboard()->isKeyActive(cKeyShift))
               {
                  setFlag(cFlagDebugRenderCameraRep, !getFlag(cFlagDebugRenderCameraRep));
                  gConsoleOutput.status("Camera rep debug display %s", getFlag(cFlagDebugRenderCameraRep) ? "Enabled" : "Disabled");
               }
               else
               {
                  setFlag(cFlagDebugRenderSimRep, !getFlag(cFlagDebugRenderSimRep));
                  gConsoleOutput.status("Simrep debug display %s", getFlag(cFlagDebugRenderSimRep) ? "Enabled" : "Disabled");
               }
            }

            if ((gInputSystem.getKeyboard()->isKeyActive(cKeyAlt)) && (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)))
            {
               setFlag(cFlagDebugRenderFlightRep, !getFlag(cFlagDebugRenderFlightRep));
               gConsoleOutput.status("Flight rep debug display %s", getFlag(cFlagDebugRenderFlightRep) ? "Enabled" : "Disabled");
            }
            break;
         }

         case cKeyA:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               long value=0;
               if (gConfig.get(cConfigRenderSimDebug, &value))
               {
                  value++;
                  if (value > 6)
                     value=0;
               }
               else
                  value=1;
               gConfig.set(cConfigRenderSimDebug, value);
               gConsoleOutput.status("Sim debug set to %d", value);
               switch (value)
               {
                  case 0: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 0 (Off)."); break;
                  case 1: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 1 (All)."); break;
                  case 2: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 2 (Platoon Only)."); break;
                  case 3: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 3 (Squad Only)."); break;
                  case 4: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 4 (Unit Only)."); break;
                  case 5: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 5 (Squad Turn Radius)."); break;
                  case 6: gConsole.outputStatusText(1000, 0xffffffff, cMsgDebug, "RenderSimDebug set to 6 (All Non-Human)."); break;
               }
            }
            break;
         }
         
         case cKeyM:
         {
            if ((gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)) && (!gInputSystem.getKeyboard()->isKeyActive(cKeyShift)))
            {
               setFlag(cFlagRandomCameraTest, !getFlag(cFlagRandomCameraTest));
               gConsoleOutput.status("Camera test %s", getFlag(cFlagRandomCameraTest) ? "Enabled" : "Disabled");
            }
            break;
         }
         case cKeyW:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)) 
            {
               setFlag(cFlagSmallViewport, !getFlag(cFlagSmallViewport));
               setFlag(cFlagSmallViewportChanged, true);
               gConsoleOutput.status("Small viewport %s", getFlag(cFlagSmallViewport) ? "Enabled" : "Disabled");
            }
            break;
         }
#endif            
         
         case cKeyPause:
         case cKeyF16:
         {
            gConsoleOutput.status("Game %s", !getPaused() ? "Paused" : "Unpaused");
            setPaused(!getPaused(), true, -1);
            return true;
         }

#ifndef BUILD_FINAL
         case cKeySpace:
         {
            BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
            if(pUser)
            {
               BVector hoverPointPos(50.0f, 0.0f, 50.0f);
               pUser->computeClosestCameraHoverPointVertical(hoverPointPos, hoverPointPos);
               pUser->setCameraHoverPoint(hoverPointPos);
               pUser->setFlagTeleportCamera(true);
            }
            
            gConsoleOutput.status("Game camera reset");
                        
            return true;
         }  
#endif   
      }
   }

#ifndef BUILD_FINAL
   if (controlType==cKeyTab || controlType == cDpadRight)
   {
      if(getFlag(cFlagPaused))
      {
         if(event==cInputEventControlStart)
         {
            //setFlag(cFlagSingleStep, true);
            setSingleStep(true);
            mSingleStepRepeatTime=timeGetTime()+200;
            mAllowSingleStep = false;
         }
         else if(event==cInputEventControlStop)
         {
            //setFlag(cFlagSingleStep, false);
            setSingleStep(false);
            mAllowSingleStep = true;
         }
         else if(event==cInputEventControlRepeat)
         {
            // DLM 2/12/08 - FTLOG, don't do multiple steps on dpad down.
            // Tab key sends start, repeat, and stop.  Dpad sends repeat and stop.
            if (mAllowSingleStep)
            {
               //setFlag(cFlagSingleStep, true);
               setSingleStep(true);
               mAllowSingleStep = false;
            }
         }
      }
   }
#endif
   
   return false;
}

//==============================================================================
// 
//==============================================================================
void BModeGame::setPaused(bool paused, bool requested, PlayerID playerID)
{
   if (paused)
      mPausedByPlayerID = playerID;

   // Reset rumble whenever game is paused to prevent rumble staying on forever.
   gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort()).resetRumble();

   if (requested && mIsMPRunning)
   {
      // [6/25/2008 DPM] Using the stats manager temporarily until some other way comes along to determine the MP game type
      //
      // We will not allow pausing of matchmade games
      // Pause will work in all other MP game modes
      if (gStatsManager.getGameType() == BStats::eMatchMaking)
         return;

      BMPSession* pMPSession = gLiveSystem->getMPSession();
      if (pMPSession && pMPSession->getSimObject())
      {
         pMPSession->getSimObject()->setPaused(paused, playerID);
      }
   }
   else
   {
      setFlag(cFlagPaused, paused);
      if(gWorld && gWorld->getWorldSoundManager())
         gWorld->getWorldSoundManager()->gamePaused(paused);

      if( paused )
         gBinkInterface.pauseAllVideos();
      else
         gBinkInterface.playAllVideos();
   }
}

//==============================================================================
// 
//==============================================================================
void BModeGame::setSingleStep(bool val)
{
   if (mIsMPRunning)
   {
      if (!val)
         return;

      BMPSession* pMPSession = gLiveSystem->getMPSession();
      if (pMPSession && pMPSession->getSimObject())
      {
         pMPSession->getSimObject()->singleStep();
      }
   }
   else
   {
      setFlag(cFlagSingleStep, val);
   }
}

//==============================================================================
// BModeGame::getLocalTiming
//==============================================================================
HRESULT BModeGame::getLocalTiming(uint32& timing, uint32* deviationRemaining)
{
   //if(timing && mpWorld->getUpdateNumber()>cTimingTrackerSkipCount)
   BASSERTM(mpTimingTracker, "Missing timing tracker");
   if (mpTimingTracker == NULL)
      return HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);

   if (gEnableSubUpdating && !gDecoupledUpdate)
   {
      if (mpWorld->getDynamicSubUpdateTime())
         timing = mpWorld->calcAdjustedSubUpdateTime() * mpWorld->getAmountOfSubUpdates();
      else
         timing = mpWorld->getSubUpdateTotalTimeInMsecs();
   }
   else
      timing = mpTimingTracker->getLocalTiming(cMaxUpdateLength, deviationRemaining);

   return S_OK;

   //if(timing)
   //{
   //   *timing=mpTimingTracker->getLocalTiming(cMaxUpdateLength, deviationRemaining);
   //   return S_OK;
   //}
   //else
   //   return HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE);
}

//=============================================================================
// BModeGame::getMSPerFrame
//=============================================================================
float BModeGame::getMSPerFrame()
{
   if(mpWorld->getUpdateNumber()>cTimingTrackerSkipCount)
   {
      return (float)(mpTimingTracker->getFrameAverage()+mpTimingTracker->getFrameDeviation());
   }
   else
      return 33.0f;
}

//=============================================================================
// Called from the network layer when a synchronized pause request
//    has been handled
//=============================================================================
void BModeGame::netSetPaused(bool val, PlayerID lPlayerID)
{
   // look up the player by MPID for the translation
   const BPlayer* pPlayer = gWorld->getPlayerByMPID(lPlayerID);
   setPaused(val, false, (pPlayer ? pPlayer->getMPID() : -1));
}

//=============================================================================
// Called from the network layer when a synchronized single step request
//    has been handled
//=============================================================================
void BModeGame::netSingleStep()
{
   setFlag(cFlagSingleStep, true);
}

//=============================================================================
// BModeGame::playerDisconnected
//=============================================================================
void BModeGame::playerDisconnected(PlayerID playerID, bool userInitiated)
{
   BPlayer* pPlayer = gWorld->getPlayerByMPID(playerID);
   if (pPlayer)
   {
      // if the player disconnected by choice, then we're going to resign them
      if (userInitiated)
         pPlayer->sendResign();
      else
         pPlayer->sendDisconnect();
   }
}

//==============================================================================
// BModeGame::beginScenarioArchiveLoad
//==============================================================================
bool BModeGame::beginScenarioArchiveLoad()
{
   SCOPEDSAMPLE(BModeGame_beginScenarioArchiveLoad)
//-- FIXING PREFIX BUG ID 4146
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--
   BDEBUG_ASSERT(pSettings);

   BSimString mapName;
   pSettings->getString(BGameSettings::cMapName, mapName);
   mapName += ".scn";

   BSimString fullMapName;   
   gFileManager.constructQualifiedPath(cDirScenario, mapName, fullMapName);
   if (!gArchiveManager.beginScenarioPrefetch(fullMapName)) 
      return false;

   if (!gArchiveManager.beginScenarioLoad(fullMapName))
      return false;
   
   return true;
}      

//==============================================================================
// BModeGame::endScenarioArchiveLoad
//==============================================================================
void BModeGame::endScenarioArchiveLoad()
{
   SCOPEDSAMPLE(BModeGame_endScenarioArchiveLoad)
#ifndef BUILD_FINAL
   //gFileManager.dumpAllFileCacheInfo();
#endif
   
   if (!gArchiveManager.endScenarioLoad())
   {
      trace("BModeGame::endScenarioArchiveLoad: endScenarioLoad() failed\n");
   }
}

//==============================================================================
// BModeGame::initGame
//==============================================================================
bool BModeGame::initGame()
{
   SCOPEDSAMPLE(BModeGame_initGame)

   if (gArchiveManager.getArchivesEnabled() && gConfig.isDefined("UnloadRootArchive"))
      gArchiveManager.reloadLocaleArchive();

   initParticleSystemPools();
   
   // Init the physics memory manager
   XPhysicsMemoryInit();

#ifndef BUILD_FINAL
   gFlashGateway.clearInstanceMemoryStats(cFlashAssetCategoryPreGame);

   BTimer initGameTimer;
   initGameTimer.start();
#endif   
   
   BString methodName[6];
   BUString playerName[6];

   BMPSession* pMPSession = gLiveSystem->getMPSession();
   mIsMPRunning = (pMPSession ? pMPSession->isGameRunning() : FALSE);

   mWaitingOnPlayersSpinnerTime = 0;
   mFlagWaitingOnPlayers = FALSE;
   mFlagWaitingOnPlayersSpinner = FALSE;

//-- FIXING PREFIX BUG ID 4149
   const BGameSettings* pSettings = gDatabase.getGameSettings();
//--

   long gameType = -1;
   pSettings->getLong(BGameSettings::cGameType, gameType);

   BYTE loadType = BGameSettings::cLoadTypeNone;
   pSettings->getBYTE(BGameSettings::cLoadType, loadType);
   bool loadGame = (loadType == BGameSettings::cLoadTypeSave);

   if (gGame.isSplitScreenAvailable() && gUserManager.getSecondaryUser()->getFlagUserActive())
      gGame.setSplitScreen(true);

   gGame.getUIGlobals()->setWaitDialogVisible(false);


   BSimString fontConfigFile;
   if (!gConfig.get(cConfigFlashFontsFile, fontConfigFile))
      fontConfigFile.set("FlashFonts.xml");

   gFlashBackgroundPlayer.setPlayPauseSkipStrings(gDatabase.getLocStringFromID(25336), gDatabase.getLocStringFromID(25337), gDatabase.getLocStringFromID(25338));
   gFlashBackgroundPlayer.setCanPause(true);    // by default

   // set our random tips for the loading screen
   gDatabase.getTips().getRandomLoadingScreenTips(gFlashBackgroundPlayer.getTipPointerContainer());

   bool showLoadingScreen=false;
   if (gConfig.isDefined(cConfigEnableBackgroundPlayer))
      showLoadingScreen=true;

   if (showLoadingScreen && ( (gameType == BGameSettings::cGameTypeSkirmish) || 
                              (gameType == BGameSettings::cGameTypeCampaign) || 
                              (gameType == BGameSettings::cGameTypeScenario) ) )
   {

      // get the map we are loading
      BSimString mapName;
      pSettings->getString(BGameSettings::cMapName, mapName);

      BSimString movieName;
      bool cinematic=false;
            
      BSimString loadingScreen="art\\ui\\flash\\game\\labyrinth_load.swf";      

      if (gameType == BGameSettings::cGameTypeCampaign)
      {
         bool isCoop=false;
         pSettings->getBool(BGameSettings::cCoop, isCoop);
         long networkType = -1;
         pSettings->getLong(BGameSettings::cNetworkType, networkType);
         
         // if coop and (live or lan)
         if (  isCoop && ((networkType == BGameSettings::cNetworkTypeLan) || (networkType == BGameSettings::cNetworkTypeLive)) )
            gFlashBackgroundPlayer.setCanPause(false);

         loadingScreen.set("art\\ui\\flash\\load\\CampaignLoad\\CampaignLoadScreen.swf");

         bool playCineOnLoad = false;
         if (gConfig.isDefined("PlayCinematicOnLoad"))
            playCineOnLoad = true;

         if (playCineOnLoad)
         {
            BSimString captionsFile;
            captionsFile.set("");
            BCampaign* pCampaign = gCampaignManager.getCampaign(0);
            BCampaignNode* pNode = NULL;

            if (pCampaign)
               pNode = pCampaign->getNode(mapName.getPtr());

            if (pCampaign)
            {
               if (gConfig.isDefined(cConfigDemo))
                  pNode = pCampaign->getNode("CampaignUNSC\\design\\PHXscn02\\PHXscn02");
               if (pNode)
               {
                  //Update presence (spammy - but won't actually call the API call unless the value changes) - eric
                  gLiveSystem->setPresenceContext(PROPERTY_MISSIONINDEX, pNode->getLeaderboardLevelIndex(), true); 

                  // get the previous node
//-- FIXING PREFIX BUG ID 4147
                  const BCampaignNode* pVideoNode = pCampaign->getVideoNode(pNode);
//--
                  if (pVideoNode && !loadGame && !gConfig.isDefined("SkipCampaignVideo"))
                  {
                     // if it's a cinematic, play it.
                     if (pVideoNode->getFlag(BCampaignNode::cCinematic))
                     {
                        cinematic = true;

                        // grab the video name and play it.
                        movieName = pVideoNode->getFilename();

                        bool showSubtitles=true;
                        // check to see if subtitles are turned on.
                        if (gUserManager.getPrimaryUser())
                           showSubtitles=gUserManager.getPrimaryUser()->getOption_SubtitlesEnabled();

                        if (showSubtitles)
                           captionsFile = pVideoNode->getCaptionsFile();

                        //These lines don't work because when the level loads - gLiveSystem doesn't get an update (via gGame)
                        //gLiveSystem->setPresenceContext(CONTEXT_PRE_CON_MOVIENAME, pVideoNode->getPresenceMovieIndex());
                        //gLiveSystem->setPresenceContext(X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_WATCHINGMOVIE);
                        
                        //So lets call the API directly - eric :|
                        const BUser* user = gUserManager.getPrimaryUser();
                        //--
                        if (user && user->isSignedIn())
                        {
                           XUserSetContext(user->getPort(), CONTEXT_PRE_CON_MOVIENAME, pVideoNode->getPresenceMovieIndex());
                           XUserSetContext(user->getPort(), X_CONTEXT_PRESENCE, CONTEXT_PRESENCE_PRE_MODE_WATCHINGMOVIE);
                        }
                     }
                  }
               }
            }

            //-- Stop sound
            if(cinematic)
            {
               gSoundManager.playSoundCueByEnum(BSoundManager::cSoundStartBlur);
               gSoundManager.update();
            }

            //-- Play Cinematic
            gFlashBackgroundPlayer.loadMovie(loadingScreen.getPtr(), fontConfigFile.getPtr(), movieName.getPtr(), captionsFile.getPtr(), cDirData, false, true);

            if (pNode)
            {
               // do the action script calls
               GFxValue values2[2];

               // - Screen Title
               // - Description Title
               // - Description
               // - Loading Text
               // - Gamer tag?

               // Fill in Map Title
               values2[0].SetString("mMapTitle");
               values2[1].SetStringW(pNode->getDisplayName());
               gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

               // set Map label
               values2[0].SetString("mDescriptionTitle");
               values2[1].SetStringW(gDatabase.getLocStringFromID(25677));
               gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

               // Fill in Map Description
               values2[0].SetString("mDescription");
               values2[1].SetStringW(pNode->getLoadText());
               gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

               // Set the loading image to the correct picture
               values2[0].SetString("mMapImages");
               values2[1].SetString(pNode->getLoadImage().getPtr());
               gFlashBackgroundPlayer.invokeActionScript("setLoadImage", values2, 2);

               // set loading text label
               values2[0].SetString("mLoadingText");
               values2[1].SetStringW(gDatabase.getLocStringFromID(23503));
               gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);
            }


            gFlashBackgroundPlayer.startMovie();
         }
      }
      else
      {
         // Non-campaign types.
         // see if we can find the map info based on the map name.
         const BScenarioMap* pMap = gDatabase.getScenarioList().getMapInfo(mapName.getPtr());
         if (pMap!=NULL)
         {
            if (pMap->getLoadingScreen().length()> 0)
               loadingScreen.set(pMap->getLoadingScreen().getPtr());
         }

         bool testLoadScreen = gConfig.isDefined("SkirmishLoadScreen");
         if (testLoadScreen)
            loadingScreen="art\\ui\\flash\\Load\\SkirmishLoad\\SkirmishLoadScreen.swf";

         gFlashBackgroundPlayer.loadMovie(loadingScreen.getPtr(), fontConfigFile.getPtr(), movieName.getPtr(), NULL, -1, false, true);
         if (pMap && testLoadScreen)  
         {

            GFxValue values2[2];

            // Fill in Map Title
            values2[0].SetString("mMapTitle");
            values2[1].SetStringW(pMap->getMapName().getPtr());
            gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

            // set Map label
            values2[0].SetString("mDescriptionTitle");
            values2[1].SetStringW(gDatabase.getLocStringFromID(25677));
            gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

            // Fill in Map Description
            values2[0].SetString("mDescription");
            values2[1].SetStringW(pMap->getMapDescription().getPtr());
            gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

            // set loading text label
            values2[0].SetString("mLoadingText");
            values2[1].SetStringW(gDatabase.getLocStringFromID(23503));
            gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

            // set the teams
            values2[0].SetString("mTeams.mTeamAlpha");
            values2[1].SetStringW(gDatabase.getLocStringFromID(25296));
            gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

            values2[0].SetString("mTeams.mTeamBravo");
            values2[1].SetStringW(gDatabase.getLocStringFromID(25297));
            gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);

            // Set the loading image to the correct picture
            values2[0].SetString("mMapImages");
            values2[1].SetString(pMap->getLoadingScreen().getPtr() );
            gFlashBackgroundPlayer.invokeActionScript("setLoadImage", values2, 2);


            // Fill in Players
            long numPlayers=0;
            pSettings->getLong(BGameSettings::cPlayerCount, numPlayers);

            int t1Index=0;
            int t2Index=3;
            bool randomTeams = true;
            bool playersOnTeam1 = false;
            bool playersOnTeam2 = false;
            long aiPlayerNumber=1;
            for (int i=0; i<numPlayers; i++)
            {
               long team=0;
               BString playerNameASCII;
               if(!pSettings->getString(PSINDEX(i+1, BGameSettings::cPlayerName), playerNameASCII))
                  playerName[i].format(L"Player %d", i+1);   // this should never happen

               long settingsPlayerType;
               pSettings->getLong( PSINDEX(i+1, BGameSettings::cPlayerType), settingsPlayerType);
               if (settingsPlayerType == BGameSettings::cPlayerComputer)
               {
                  playerName[i].locFormat(gDatabase.getLocStringFromID(25941).getPtr(), aiPlayerNumber);
                  aiPlayerNumber++;    // increment for our next AI player.
                  // playerName[i].set(gDatabase.getLocStringFromID(25940));     // AI Player
               }
               else
               {
                  playerName[i].format(L"%S", playerNameASCII.getPtr());
               }

               pSettings->getLong(PSINDEX(i+1, BGameSettings::cPlayerTeam), team);   // scenario says the team can define the team

               long index=0;
               if (team==1)
               {
                  playersOnTeam1=true;
                  randomTeams=false;
                  index=t1Index++;
               }
               else if (team==2)
               {
                  playersOnTeam2=true;
                  randomTeams=false;
                  index=t2Index++;
               }
               else
               {
                  // random
                  index=t1Index++;
               }

               // invoke action script to set the name of the player.
               methodName[i].format("mPlayer%d", index);
               values2[0].SetString(methodName[i].getPtr());            // the text field
               values2[1].SetStringW(playerName[i].getPtr());         // the name of the player
               gFlashBackgroundPlayer.invokeActionScript("setText", values2, 2);
            }

            bool playersOn2Teams = (playersOnTeam1 && playersOnTeam2);

            if (randomTeams || (!playersOn2Teams))
            {
               values2[0].SetString("mTeams");
               values2[1].SetBoolean(false);
               gFlashBackgroundPlayer.invokeActionScript("setVisible", values2, 2);
            }

            // fix for resolution.
         }
         else if (pMap)
         {
            GFxValue values[1];
            // values[0].SetStringW(pMap->getMapDescription().getPtr());
            const BUString* pString = gDatabase.getTips().getRandomTip();
            if (pString)
               values[0].SetStringW(pString->getPtr());
            else
               values[0].SetStringW(pMap->getMapDescription().getPtr());
            gFlashBackgroundPlayer.invokeActionScript("setMapDescription", values, 1);

            values[0].SetStringW(pMap->getMapName().getPtr());
            gFlashBackgroundPlayer.invokeActionScript("setMapName", values, 1);
         }

         gFlashBackgroundPlayer.startMovie();

      }
   }

   if (gArchiveManager.getArchivesEnabled() && gConfig.isDefined("UnloadRootArchive"))
      gArchiveManager.reloadRootArchive();

   if(gRecordGame.isPlaying() || gRecordGame.isRecording())
      gRecordGame.stop();
      
   setWorldReset(false);
            
   // Clear flags
   mFlags.setAll(0);   

   setFlag(cFlagSaveFogOff, gConfig.isDefined(cConfigNoFogMask));

#ifndef BUILD_FINAL
   BDisplayStats::reset();
   if (gConfig.isDefined(cConfigDisplayStatPage))
   {
      long statPage;
      gConfig.get(cConfigDisplayStatPage, &statPage);
      
      if (gConfig.isDefined(cConfigDisplayRenderStats))
         BDisplayStats::setMode(BDisplayStats::cDSMRender);
      else
         BDisplayStats::setMode(BDisplayStats::cDSMSim);
      BDisplayStats::setType(statPage);
   }
#endif   

   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);
   mTimerFrequency=freq.QuadPart;
   mTimerFrequencyFloat=(double)mTimerFrequency;

   LARGE_INTEGER counter1;
   LARGE_INTEGER counter2;
   QueryPerformanceCounter(&counter1);
   QueryPerformanceCounter(&counter2);
   mCounterOverhead = (int64)(counter2.QuadPart - counter1.QuadPart);

   gSaveGame.reset();

   // See if we are going to play back or record the new game
   bool playGame=false;
   bool recordGame=false;
   if(pSettings)
   {
      playGame = (loadType == BGameSettings::cLoadTypeRecord);
      if (!playGame)
         pSettings->getBool(BGameSettings::cRecordGame, recordGame);

      // If this is a campaign load, make prep the save system for it. If the prep fails then just load the scenario.
      if (loadGame)
      {
         BSimString loadName;
         pSettings->getString(BGameSettings::cLoadName, loadName);
         if (loadName == "campaign.sav" || loadName == "campaign")
         {
            if (!gSaveGame.prepCampaignLoad())
            {
               // SRL 10/29/08 - What we need to do here is fail because we are expecting this to be valid.
               gSaveGame.reset();
               if (showLoadingScreen)
                  gFlashBackgroundPlayer.stopMovie();

               return false;
/*
               BGameSettings* pChangeSettings = const_cast<BGameSettings*>(pSettings);
               pChangeSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
               loadName.empty();
               pChangeSettings->setString(BGameSettings::cLoadName, loadName);
               loadGame = false;
*/
            }
         }
      }
   }

   // Init syncing
   if ((mIsMPRunning || ((recordGame || playGame) && gConfig.isDefined(cConfigRecordGameSync))) && !gConfig.isDefined(cConfigNoSync))
   {
      BSyncManager::getInstance()->setSyncing(true);
      if (!mIsMPRunning)
         BSyncManager::getInstance()->reset();
   }
   else
      BSyncManager::getInstance()->setSyncing(false);

   long playerCount = 0;
   pSettings->getLong(BGameSettings::cPlayerCount, playerCount);
   BSyncManager::getInstance()->setFull1v1((playerCount <= 2)); // Only allow full1v1 in 1v1 or campaign games

   if (!mIsMPRunning)
      mIsHost = TRUE;
   else
      mIsHost = (pMPSession ? pMPSession->isHosting() : FALSE);

   gStatsManager.init((mIsMPRunning && !pMPSession->isInLANMode()));

   gGame.openVinceLog();

   gVisualManager.gameInit();

   // Possibly play a record game or load a game
   // rg [10/8/07] - I'm moving this from BScenario::load() to here, so the database is setup as early as possible. 
   // Otherwise the archive manager doesn't know what scenario archive to load during playback before the UI is initialized.
   if (playGame)
   {
      if (!gRecordGame.play())
      {
         gRecordGame.reset();
         if (showLoadingScreen)
            gFlashBackgroundPlayer.stopMovie();
         return false;
      }
      // Update loadGame variable in case record game was from a save game
      BYTE loadType = BGameSettings::cLoadTypeNone;
      pSettings->getBYTE(BGameSettings::cLoadType, loadType);
      loadGame = (loadType == BGameSettings::cLoadTypeSave);
   }
   else if (recordGame)
   {
      if (!gRecordGame.record())
      {
         if (showLoadingScreen)
            gFlashBackgroundPlayer.stopMovie();
      
         return false;
      }
   }

   if (loadGame)
   {
      if (!gSaveGame.load())
      {
         gSaveGame.reset();
         if (showLoadingScreen)
            gFlashBackgroundPlayer.stopMovie();
         return false;
      }
   }
   
   // Load the scenario archive - must be done after the record game playback starts!
   if (!beginScenarioArchiveLoad())
   {
      if (showLoadingScreen)
         gFlashBackgroundPlayer.stopMovie();

      return false;
   }
   
   // IMPORTANT: endScenarioArchiveLoad() MUST be called no matter how you exit this function.
      
   // Init the game UI
   if(!gUIGame.init())
   {
      if (showLoadingScreen)
         gFlashBackgroundPlayer.stopMovie();

      endScenarioArchiveLoad();
      
      return false;
   }
   setFlag(cFlagUIGameInit, true);
      
   // Init new UI manager   
   BDEBUG_ASSERT(!gUIManager);
   gUIManager = new BUIManager();
   if (!gUIManager->init())
   {
      delete gUIManager;
      return false;
   }

   // Init the game world
   if(!initWorld())
   {
      blogerrortrace("BModeGame::init: initWorld failed");

      if (showLoadingScreen)
         gFlashBackgroundPlayer.stopMovie();

      endScenarioArchiveLoad();
      
      return false;
   }

   if (!loadGame)
   {
      if (!gUIManager->initPlayerSpecific(false))
         return false;
   }
   
   if (loadGame)
   {
      gSaveGame.postUISetupFixup();
      gSaveGame.reset();
   }

   if (gConfig.isDefined(cConfigAlpha))
   {
      gWatermarkUI.setVisible(true);
      gWatermarkUI.initFromMinimap();
   }

   // Reset the death manager
   gDeathManager.reset();

   // Init the timing tracker
   if (mpTimingTracker)
      delete mpTimingTracker;
   mpTimingTracker=new BTimingTracker();
         
   mTimingTrackerUpdate=mpWorld->getUpdateNumber()+cTimingTrackerSkipCount;

   // Hookup to multiplayer
   if (mIsMPRunning && pMPSession->getSimObject())
   {
      mFlagWaitingOnPlayers = TRUE;
      //if (gameType == BGameSettings::cGameTypeCampaign)
      //{
      //   bool isCoop=false;
      //   pSettings->getBool(BGameSettings::cCoop, isCoop);
      //   if (isCoop)
      //      mFlagWaitingOnPlayers = TRUE;
      //}

      pMPSession->getSimObject()->setTimingHandler(this);
      pMPSession->getSimObject()->setPauseHandler(this);
      pMPSession->getSimObject()->setPlayerHandler(this);

      BUser* pUser = gUserManager.getPrimaryUser();
      if (pUser)
      {
         pUser->setFlagCameraScrollEnabled(false);
         pUser->setFlagCameraYawEnabled(false);
         pUser->setFlagCameraZoomEnabled(false);
      }
   }

   // Init the minimap
   //if (!gConfig.isDefined(cConfigFlashGameUI))
   //{
   //   BUIRect*  pMinimapPos=gUIGame.getMinimapPosition();
   //   gMiniMap.init((float)pMinimapPos->mX1, (float)pMinimapPos->mY1, (float)pMinimapPos->mX2, (float)pMinimapPos->mY2, gUIGame.getMinimapBorderMin(), gUIGame.getMinimapBorderMax(), gUIGame.getMinimapOpacity());
   //}
   setFlag(cFlagMinimapInit, true);

   // Init the world visibility renderer
   BWorldVisibility::getInstance().setWorldSize( (float)gTerrainSimRep.getNumXDataTiles() * (float)gTerrainSimRep.getDataTileScale() );

   // CLM [09.17.08] Terrain skirt is not designed to be on during skirmish games w/ the FOW.
   // jce [9/17/2008] -- <3 NoTerrainSkirt and this was overriding it.
   if (!loadGame)
      gTerrain.setRenderSkirt(!gConfig.isDefined(cConfigNoTerrainSkirt));

   // rg [1/8/8] - FIXME: Change this so the HP Bar textures are loaded once from the root archive at init time!
   gHPBar.initScenario();      

   // MS 10/1/2008: need to resolve power textures here, if any, since we know the UI archive is loaded here.
   gDatabase.resolveProtoPowerTextures();
      
   // Default score on by default if this is a single player skirmish game
   bool scoreOn = false;
   if (pSettings && !mIsMPRunning)
   {
      if (gameType == BGameSettings::cGameTypeSkirmish)
         scoreOn = true;
   }

   gUserManager.getPrimaryUser()->setHUDItemEnabled(BUser::cHUDItemScore, scoreOn);
   if (gGame.isSplitScreenAvailable())
      gUserManager.getSecondaryUser()->setHUDItemEnabled(BUser::cHUDItemScore, scoreOn);

   
   gUserManager.getUser(BUserManager::cPrimaryUser)->setFlagGameActive(true);
   gUserManager.getUser(BUserManager::cPrimaryUser)->setFlagGameDoExit(false);

   if (gGame.isSplitScreenAvailable() && gUserManager.getSecondaryUser()->getFlagUserActive())
   {
      gUserManager.getUser(BUserManager::cSecondaryUser)->setFlagGameActive(true);
      gUserManager.getUser(BUserManager::cSecondaryUser)->setFlagGameDoExit(false);
   }

   //-- set the distance fade parameters
   float fadeStartDistance = 100.0f;
   float fadeEndDistance   = 1000.0f;
   gConfig.get(cConfigEmitterFadeStartDistance, &fadeStartDistance);
   gConfig.get(cConfigEmitterFadeEndDistance, &fadeEndDistance);
   gParticleGateway.setDistanceFade(fadeStartDistance, fadeEndDistance);

   // [10/21/2008 xemu] don't reset if we are loading from a savegame 
   if (!loadGame)
   {
      gScoreManager.reset();
      gScoreManager.initializePlayers();
   }

   if (gameType == BGameSettings::cGameTypeCampaign)
   {
      BCampaign* pCampaign = gCampaignManager.getCampaign(0);
      if (pCampaign)
      {
         if (!loadGame)
            pCampaign->incrementSessionID();

         pCampaign->saveToCurrentProfile();
      }
   }

   setFlag(cFlagGameInit, true);

   endScenarioArchiveLoad();
   
   // If archives aren't enabled, nobody has waited until the render thread is idle. So let's wait here so the loading screen stays active.
   if (!gArchiveManager.getArchivesEnabled())
      gRenderThread.blockUntilWorkerIdle();

   if (!gUIManager->initAfterLoad())
      return false;

   if (showLoadingScreen)
   {
      gFlashBackgroundPlayer.stopMovie(true);
   }

   //-- Load Civ Sound Bank
   if(gGame.isSplitScreen())
   {
      //DJBFIXME: load only a sinlge combined bank
      //-- load both banks for now
      uint numCivs = gDatabase.getNumberCivs();
      for(uint i=0; i < numCivs; i++)
      {
         BCiv *pCiv = gDatabase.getCiv(i);
         if(!pCiv)
            continue;

         //-- load the bank
         gSoundManager.loadSoundBank(pCiv->getSoundBank(), false);
      }

   }
   else
   {
      BSimString soundBank = gUserManager.getPrimaryUser()->getPlayer()->getCiv()->getSoundBank();
      if(!soundBank.isEmpty())
         mCivSoundBankID = gSoundManager.loadSoundBank(soundBank, false);   
   }

   // Load sound banks once game finishes loading.
   gScenario.loadSoundBanks();

   //Let session know that I'm all done loading the level
   if (mIsMPRunning)
   {
      pMPSession->gameDoneLoading();
   }

#ifndef BUILD_FINAL     
   const double totalInitGameTime = initGameTimer.getElapsedSeconds();
   trace("BModeGame::initGame() took %3.3f secs", totalInitGameTime);
#endif   

   //-- Unload the pregame bank
   gSoundManager.unloadSoundBank(gSoundManager.getPregameBank(), false);

   //If we are loading a saved campaign game, set the music state to be in game (usually done by a trigger at the beginning). 
   if (loadGame && (gameType == BGameSettings::cGameTypeCampaign))
   {
      //saved games are always in the play game switch
      gSoundManager.playCue("play_in_game");

      //default to standard state
      gSoundManager.playCue("set_state_standard");

      if( mpWorld && mpWorld->getWorldSoundManager() && mpWorld->getWorldSoundManager()->getMusicManager() )
         mpWorld->getWorldSoundManager()->getMusicManager()->setLoadedFromSavegame(true);

      //Storing some sound data in the campaignmanager. 
      BCampaign* pCampaign = gCampaignManager.getCampaign(0);
      if (pCampaign)
      {
         int campaignNodeID = pCampaign->getCurrentNodeID();
         BCampaignNode *node = pCampaign->getNode(campaignNodeID);
         if( node )
         {
            //Set the switch for the world music to use
            switch ( node->getMusicWorld() )
            {
               case BCampaignNode::cWorldHarvest:
                  gSoundManager.playCue("set_switch_music_harvest");
                  break;
               case BCampaignNode::cWorldArcadia:
                  gSoundManager.playCue("set_switch_music_arcadia");
                  break;
               case BCampaignNode::cWorldShieldExt:
                  gSoundManager.playCue("set_switch_music_shield_ext");
                  break;
               case BCampaignNode::cWorldShieldInt:
                  gSoundManager.playCue("set_switch_music_shield_int");
                  break;
            }

            //Start playing the ambient sound for the world
            switch ( node->getAmbientWorld() )
            {
               case BCampaignNode::cWorldHarvest:
                  gSoundManager.playCue("play_amb_harvest");
                  break;
               case BCampaignNode::cWorldArcadia:
                  gSoundManager.playCue("play_amb_arcadia");
                  break;
               case BCampaignNode::cWorldShieldExt:
                  gSoundManager.playCue("play_amb_shield_ext");
                  break;
               case BCampaignNode::cWorldShieldInt:
                  gSoundManager.playCue("play_amb_shield_int");
                  break;
            }
         }
      }
   }

   /*
   // Invalidate campaign save game if user did not just continue from their last save
   if (gameType == BGameSettings::cGameTypeCampaign && !gSaveGame.getSavedCampaign())
   {
      BSimString saveDir;
      gFileManager.getDirListEntry(saveDir, cDirSaveGame);
      BSimString path;
      path.format("%s%scampaign.sav", BFileUtils::getXboxGamePath(), saveDir.getPtr());
      DeleteFile(path);
   }
   */

   // Fade up from black at the start of the game
   float fadeTime = 0.0f;
   float fadeDelay = 0.0f;
   gConfig.get(cConfigGameStartFadeTime, &fadeTime);
   gConfig.get(cConfigGameStartFadeDelay, &fadeDelay);
   float fadeUpTime = fadeTime + fadeDelay;
   if (fadeUpTime)
      gWorld->getTransitionManager()->doFadeUp(fadeUpTime, 0.0f, 0.0f, cColorBlack, NULL);

   mSubUpdateLastCalcUpdate = 0;
   mSubUpdateLastUpdateLength = 0.0f;
   mSubUpdateLastUpdateTime = 0;

/*
   if (gLiveSystem->isMultiplayerGameActive())
   {
      setFlag(cFlagMPGameWait, true);
      gGame.getUIGlobals()->showWaitDialog(gDatabase.getLocStringFromID(25989));
   }
*/

   // reset the XLSP timeouts because we may have outstanding requests
   // that the level load has delayed
   gLSPManager.resetTimeout();

   return true;
}

//==============================================================================
// BModeGame::deinitGame
//==============================================================================
void BModeGame::deinitGame()
{
   // hide any dialog box that may be up
   // fix for PHX-9855
   BUIGlobals* pUIGlobals = gGame.getUIGlobals();
   if (pUIGlobals)
   {
      if (!gLiveSystem->getMPSession() || !gLiveSystem->getMPSession()->doNotDismissDialog())
      {
         // ajl 11/18/08 - Don't hide if this is the dialog warning the user that they removed the default storage device.
         if (pUIGlobals->isYorNBoxVisible() && pUIGlobals->getAllowAutoCloseOnGameDeinit())
            pUIGlobals->hideYorn();
      }
   }

   setWorldReset(true);

   gGame.setSplitScreen(false);

   gUserManager.getUser(BUserManager::cPrimaryUser)->setFlagGameActive(false);
   gUserManager.getUser(BUserManager::cSecondaryUser)->setFlagGameActive(false);

   gUserManager.getUser(BUserManager::cPrimaryUser)->setFlagGameDoExit(false);
   gUserManager.getUser(BUserManager::cSecondaryUser)->setFlagGameDoExit(false);

   //gUserManager.getUser(BUserManager::cSecondaryUser)->setFlagUserActive(false);
   //gUserManager.setUserPort(BUserManager::cSecondaryUser, -1);

   if(gRecordGame.isPlaying() || gRecordGame.isRecording())
      gRecordGame.stop();

   if (gConfig.isDefined(cConfigNoFogMask) != getFlag(cFlagSaveFogOff))
      gConfig.toggleDefine(cConfigNoFogMask);

   BMPSession* pMPSession = gLiveSystem->getMPSession();

   if (pMPSession && pMPSession->getSimObject())
   {
      pMPSession->getSimObject()->setTimingHandler(NULL);
      pMPSession->getSimObject()->setPauseHandler(NULL);
      pMPSession->getSimObject()->setPlayerHandler(NULL);
   }

   if (mIsMPRunning && pMPSession)
   {
      // if I'm in an MP game and the party session is no longer active and I'm NOT joining another game via invite
      // then we need to inform the user that they're no longer connected to the party
      //
      // this means they're being kicked back to the main menu
      if (!gLiveSystem->isPartySessionActive() && !gLiveSystem->isInviteInfoAvailable())
      {
         BUIGlobals* puiGlobals = gGame.getUIGlobals();
         if (puiGlobals)
            puiGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(23494), BUIGlobals::cDialogButtonsOK);
      }

      pMPSession->setGameStateToFinal();
      pMPSession->endActiveGame();
   }

   // Get rid of the game session
   gLiveSystem->disposeGameSession();

   mIsHost = FALSE;

   if (mpTimingTracker)
   {
      delete mpTimingTracker;
      mpTimingTracker=NULL;
   }
      
   deinitWorld();

   gHPBar.deinitScenario();
   gUI.forceUnloadAllTextures();
      
   if(getFlag(cFlagMinimapInit))
   {
      //if (!gConfig.isDefined(cConfigFlashGameUI))
      //   gMiniMap.deinit();

      setFlag(cFlagMinimapInit, false);
   }

   if(getFlag(cFlagUIGameInit))
   {
      gUIGame.deinit();
      setFlag(cFlagUIGameInit, false);
   }
   
   if (gUIManager)
   {      
      gUIManager->deinit();
      delete gUIManager;
   }
   gUIManager = NULL;
    //-- unload Civ Sound Bank
   if(gGame.isSplitScreen())
   {
      //DJBFIXME: unload only a sinlge combined bank
      //-- unload both banks for now
      uint numCivs = gDatabase.getNumberCivs();
      for(uint i=0; i < numCivs; i++)
      {
         BCiv *pCiv = gDatabase.getCiv(i);
         if(!pCiv)
            continue;

         //-- unload the bank
         gSoundManager.unloadSoundBank(pCiv->getSoundBank(), false);
      }
   }
   else
   {
      //-- Unload Civ Specific sound bank
      if(mCivSoundBankID != AK_INVALID_BANK_ID)
      {
         gSoundManager.unloadSoundBank(mCivSoundBankID, false);
         mCivSoundBankID = AK_INVALID_BANK_ID;
      }
   }
   
   gParticleGateway.releaseAllInstances();   
   gLightEffectManager.releaseAllInstances();
   
   gGrannyManager.unloadAll();
   gVisualManager.unloadAll();
   gTerrainEffectManager.unloadAll();

   gUGXGeomInstanceManager.reset();
   gUGXGeomManager.reset();

   BVisual::mFreeList.clear();
   BVisualItem::mFreeList.clear();
   BGrannySampleAnimCache::mFreeList.clear();

   gVisualManager.restoreState();

   deinitParticleSystemPools();

   gLightEffectManager.clear();

   gArchiveManager.endScenario();

   //-- DJBFIXME: Temp fix for mem leak for alpha
   gSoundManager.resetSoundManager();
#ifdef SOUND_RELOAD_BANKS
   gSoundManager.unloadAllSoundBanks();
   gSoundManager.loadStaticSoundBanks(false);
#endif

#ifndef BUILD_FINAL
   gFlashGateway.clearInstanceMemoryStats(cFlashAssetCategoryInGame);   
#endif
   
   verifyAllHeaps();

   setWorldReset(false);

   uint allocs, memory;
   gSoundManager.getDefaultBankMemoryStats(allocs, memory);
   gConsoleOutput.debug("Default Sound Bank memory %d allocs %d bytes", allocs, memory);
}

//==============================================================================
// BModeGame::initWorld
//==============================================================================
bool BModeGame::initWorld()
{
   // Load the map/scenario
   if(!gScenario.load(false, false))
   {
      gConsoleOutput.error("BModeGame::initWorld: gScenario.load() FAILED!\n");
      
      if(gWorld)
      {
         gTerrain.destroy();

         setWorldReset(true);
         gWorld->reset();
         delete gWorld;
         gWorld=NULL;
         setWorldReset(false);
      }
      return false;
   }

   mpWorld=gWorld;

   setFlag(cFlagTerrainInit, true);

   return true;
}

//==============================================================================
// BModeGame::deinitWorld
//==============================================================================
void BModeGame::deinitWorld()
{
   if(mpWorld)
   {
      mpWorld->reset();
      delete mpWorld;
      mpWorld=NULL;
      gWorld=NULL;
   }

   gObsManager.deinitialize();

   gDamageTemplateManager.reset();

   gUserManager.getUser(BUserManager::cPrimaryUser)->gameRelease();

   BUser* pSecondaryUser = gUserManager.getSecondaryUser();
   if (pSecondaryUser)
      pSecondaryUser->gameRelease();

   //-- Unload the scenario
   gScenario.unload();

   if(getFlag(cFlagTerrainInit))
   {
      gTerrainSimRep.destroy();
      gTerrain.destroy();
      setFlag(cFlagTerrainInit, false);
   }
}

//==============================================================================
// BModeGame::initParticleSystem
//==============================================================================
void BModeGame::initParticleSystemPools()
{         
   gParticleGateway.initMemoryPools();
   gParticleGateway.initTextureSystem();
   trace("Particle System Memory Pools Initialized...");
}

//==============================================================================
// BModeGame::deinitParticleSystem
//==============================================================================
void BModeGame::deinitParticleSystemPools()
{  
   gParticleGateway.releaseAllDataSlots();
   trace("Particle Data Slots Destroyed...");
   gParticleGateway.deinitTextureSystem();
   trace("Particle Texture System shut down...");
   gParticleGateway.deinitMemoryPools();
   trace("Particle System Memory Pools Destroyed...");
}

//==============================================================================
// 
//==============================================================================
BOOL BModeGame::getIsHost() const
{
   return mIsHost;
}

//==============================================================================
//==============================================================================
void BModeGame::setFlagPlayingVideo(bool v)
{ 
   mFlagPlayingVideo=v;
   if (v)
   {
      LARGE_INTEGER time;
      QueryPerformanceCounter(&time);
      mLastVideoTime=time.QuadPart;
   }
}

#ifdef ENABLE_FPS_LOG_FILE

   //==============================================================================
   const uint cFPSLogBufferSize = 10000;

   //==============================================================================
   //==============================================================================
   void BModeGame::clearFPSLogData()
   {
      BScopedCriticalSection lock(mFPSLogLock);

      for (uint i = 0; i < mFPSLogBuffers.getSize(); i++)
      {
         HEAP_DELETE_ARRAY(mFPSLogBuffers.get(i), gPrimaryHeap);
      }
      mFPSLogBuffers.clear();
      mFPSLogBufferOffset = 0;
      mFPSLogNextSample = 0.0f;
   }

   //==============================================================================
   //==============================================================================
   void BModeGame::openFPSLogFile()
   {
      BScopedCriticalSection lock(mFPSLogLock);

      // Open log file
      mPerfLogFilename.empty();
      getFPSLogFileName(mPerfLogFilename);

      mpFPSLogFileStream = new BWin32FileStream;
      if (!mpFPSLogFileStream->open(mPerfLogFilename.asNative(), cSFWritable | cSFSeekable | cSFEnableBuffering, &gWin32LowLevelFileIO))
      {
         delete mpFPSLogFileStream;
         mpFPSLogFileStream = NULL;
         return;
      }

      // Write header
      static BString buffer;
      buffer.set("FPS, Frame Time, Update Number, Game Time, Free Mem, primaryHeap, simHeap, networkHeap, stringHeap, renderHeap, particleBlockHeap, fileBlockHeap, physCachedHeap, physWriteCombinedHeap, syncHeap, runTimeHeap \r\n");
      mpFPSLogFileStream->writeBytes(buffer.asNative(), buffer.length());
   }

   //==============================================================================
   //==============================================================================
   void BModeGame::closeFPSLogFile()
   {
      BScopedCriticalSection lock(mFPSLogLock);

      if (!mWriteFPSLog)
         return;


      if (mpFPSLogFileStream)
      {

         mpFPSLogFileStream->close();
         delete mpFPSLogFileStream;
         mpFPSLogFileStream = NULL;

#ifndef BUILD_FINAL
         if (mAutoCopyFPSLog && gXFS.isActive())
         {
            mpFPSLogFileStream = new BWin32FileStream;
            if (mpFPSLogFileStream->open(mPerfLogFilename.asNative(), cSFReadable | cSFSeekable | cSFEnableBuffering, &gWin32LowLevelFileIO))
            {
               static BString sLogNameBuffer;
               sLogNameBuffer.empty();
               getFPSLogFileNameXFS(sLogNameBuffer);

               char destFileName[512];
               gXFS.setFPSLogFileName(sLogNameBuffer.getPtr(), destFileName, sizeof(destFileName));
               //sprintf_s(destFileName, 512, "%s\\Perf\\%s", BXFS::getFailReportDirectory(), sLogNameBuffer.getPtr());
               HANDLE handle = gXFS.createFile(destFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);



               if (handle != INVALID_HANDLE_VALUE)
               {
                  const int cBufSize = 65536*2;
                  BByteArray buf(cBufSize);
                  mpFPSLogFileStream->seek(0);
                  for ( ; ; )
                  {
                     const int n = mpFPSLogFileStream->readBytes(buf.getPtr(), cBufSize);
                     if (n == 0)
                        break;
                     DWORD bytesWritten = 0;
                     gXFS.writeFile(handle, buf.getPtr(), n, &bytesWritten, NULL);
                  }
                  gXFS.closeHandle(handle);
               }
            }

            mpFPSLogFileStream->close();
            delete mpFPSLogFileStream;
            mpFPSLogFileStream = NULL;

         }
#endif

      }
   }

   //==============================================================================
   //==============================================================================
   void BModeGame::flushFPSLogDataToFile()
   {
      BScopedCriticalSection lock(mFPSLogLock);

      if (!mWriteFPSLog)
         return;

      // Make sure there is data to write
      if (mFPSLogBuffers.getSize() == 0)
         return;

      // Make sure the file is open
      if (!mpFPSLogFileStream)
         return;

      // Write buffers
      for (uint bufferIndex = 0; bufferIndex < mFPSLogBuffers.getSize(); bufferIndex++)
      {
         uint numBytesToWrite = cFPSLogBufferSize;

         // If this is the last buffer then only write to the current offset
         if (bufferIndex == (mFPSLogBuffers.getSize() - 1))
            numBytesToWrite = mFPSLogBufferOffset;

         mpFPSLogFileStream->writeBytes(mFPSLogBuffers.get(bufferIndex), numBytesToWrite);
      }

      // Clear log data
      clearFPSLogData();
   }

   //==============================================================================
   //==============================================================================
   void BModeGame::updateFPSLogFile(bool force, const char *timeOverrideText)
   {
      BScopedCriticalSection lock(mFPSLogLock);

      const double cNextSampleTime = 1.0f;
      const uint cLogBufferLimit = 10;

      if (timeOverrideText != NULL)
      {
         mTimeOverrideText.set(timeOverrideText);
      }

      // If the number of buffers exceeds the limit, flush the data.  Yes, I'm quite aware
      // that this will free memory and then realloc it.  But this happens very infrequently
      // and is just a debugging tool.
      if (mFPSLogBuffers.getSize() >= cLogBufferLimit)
         flushFPSLogDataToFile();

      // Create the initial buffer
      if (mFPSLogBuffers.getSize() == 0)
      {
         uchar* pLogBuffer = HEAP_NEW_ARRAY(uchar, cFPSLogBufferSize, gPrimaryHeap);
         mFPSLogBuffers.add(pLogBuffer);
         mFPSLogBufferOffset = 0;

         if (gWorld != NULL)
            mFPSLogNextSample = gWorld->getTotalRealtime() + cNextSampleTime;
         else
            mFPSLogNextSample = 0;
      }

      // Log data
      if (force || ((gWorld != NULL) && (gWorld->getTotalRealtime() > mFPSLogNextSample)))
      {
         // Generate data
         char gameTimeBuffer[20];
         if (mTimeOverrideText.isEmpty())
         {
            DWORD gameTime = 0;
            if (gWorld != NULL)
               gameTime = gWorld->getGametime();
            DWORD secs = gameTime / 1000;
            DWORD mins = secs / 60;
            secs -= mins * 60;
            DWORD hours = mins / 60;
            mins -= hours * 60;
            if (hours > 0)
               sprintf_s(gameTimeBuffer, sizeof(gameTimeBuffer), "%02d:%02d:%02d", hours, mins, secs);
            else
               sprintf_s(gameTimeBuffer, sizeof(gameTimeBuffer), "%02d:%02d", mins, secs);
         }

         static BString buffer;
         if (mWriteMemLog)
         {
            DM_MEMORY_STATISTICS memStats;
            Utils::ClearObj(memStats);
            memStats.cbSize = sizeof(DM_MEMORY_STATISTICS);
            DmQueryTitleMemoryStatistics(&memStats);

            float freeMem = memStats.AvailablePages*4096.0f/(1024.0f*1024.0f);
            float primaryHeap = gPrimaryHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float simHeap = gSimHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float networkHeap = gNetworkHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float stringHeap = gStringHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float renderHeap = gRenderHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float particleBlockHeap = gParticleBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float fileBlockHeap = gFileBlockHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float physCachedHeap = gPhysCachedHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float physWriteCombinedHeap = gPhysWriteCombinedHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float syncHeap = gSyncHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);
            float runTimeHeap = gCRunTimeHeap.getStats().mCurrentAllocationSize/(1024.0f*1024.0f);

            //BGrannyInstanceRenderer::BStats& grannyStats = gGrannyInstanceRenderer.getStats();
            //float grannySize = 0.0f; // grannyStats.mTotalGPUFrameStorage / (1024.0f*1024.0f);
            //BRenderThread::BStats renderStats;
            //gRenderThread.getStats(renderStats);
            //float commandBytes = renderStats.mCommandBytes / (1024.0f*1024.0f);
            //float DCBsize = gDCBManager.getTotalAllocatedBytes() /  (1024.0f*1024.0f);
            //float GPUFrameStorage0 = gRenderThread.mGPUFrameStorageAllocators[0].getTotalBlockBytesAllocated() / (1024.0f*1024.0f);
            //float GPUFrameStorage1 = gRenderThread.mGPUFrameStorageAllocators[1].getTotalBlockBytesAllocated() / (1024.0f*1024.0f);
            //float GPUFrameStorage2 = gRenderThread.mGPUFrameStorageAllocators[2].getTotalBlockBytesAllocated() / (1024.0f*1024.0f);

            long updateNumber = -1;
            if (gWorld != NULL)
               updateNumber = gWorld->getUpdateNumber();

            if (mTimeOverrideText.isEmpty())
            {
               buffer.format("%.2f, %.2f, %d, %s, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r\n", 
                  gRender.getAverageFPS(), gRender.getAverageFrameTime() * 1000.0f, updateNumber, gameTimeBuffer,
                  freeMem, 
                  primaryHeap, simHeap, networkHeap, stringHeap, renderHeap, particleBlockHeap, fileBlockHeap, physCachedHeap, physWriteCombinedHeap, syncHeap, runTimeHeap);
            }
            else
            {
               buffer.format(",,, %s, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r\n", 
                  mTimeOverrideText.getPtr(),
                  freeMem, 
                  primaryHeap, simHeap, networkHeap, stringHeap, renderHeap, particleBlockHeap, fileBlockHeap, physCachedHeap, physWriteCombinedHeap, syncHeap, runTimeHeap);
            }

         }
         else
         {
            buffer.format("%.2f, %.2f, %d, %s\r\n", gRender.getAverageFPS(), gRender.getAverageFrameTime() * 1000.0f, gWorld ? gWorld->getUpdateNumber() : -1, gameTimeBuffer);
         }
         
         // Write the data to a buffer
         BASSERT(mFPSLogBuffers.getSize() > 0);
         uchar *pLogBuffer = mFPSLogBuffers.get(mFPSLogBuffers.getSize() - 1);
         BASSERT(pLogBuffer);

         uint totalNumBytesToWrite = buffer.length();
         uint numBytesToWrite = Math::Min(totalNumBytesToWrite, (cFPSLogBufferSize - mFPSLogBufferOffset));
         if (numBytesToWrite > 0)
         {
            memcpy_s((pLogBuffer + mFPSLogBufferOffset), (cFPSLogBufferSize - mFPSLogBufferOffset), buffer.asNative(), numBytesToWrite);
            mFPSLogBufferOffset += numBytesToWrite;
         }

         // Data remaining so create a new buffer
         if (totalNumBytesToWrite > numBytesToWrite)
         {
            uchar* pLogBuffer = HEAP_NEW_ARRAY(uchar, cFPSLogBufferSize, gPrimaryHeap);
            mFPSLogBuffers.add(pLogBuffer);
            memcpy_s(pLogBuffer, cFPSLogBufferSize, (buffer.asNative() + numBytesToWrite), (totalNumBytesToWrite - numBytesToWrite));
            mFPSLogBufferOffset = (totalNumBytesToWrite - numBytesToWrite);
         }


         // Set next update time
         if (gWorld != NULL)
         {
            mFPSLogNextSample = gWorld->getTotalRealtime() + cNextSampleTime;
         }
         mTimeOverrideText.set("");

      }
   }

   //==============================================================================
   //==============================================================================
   void BModeGame::getFPSLogFileName(BString &fileName)
   {
      const long cXboxNameSize = 256;
      char       pXboxName[cXboxNameSize];
      pXboxName[0] = 0;

      // Get name from xbox
      DmGetXboxName( pXboxName, (LPDWORD)&cXboxNameSize );
      pXboxName[5] = 0;    // truncate so file name doesn't exceed limits

      // Get time from system
      SYSTEMTIME sysTime;
      GetLocalTime( &sysTime );

      // Build type.  Library used to get exe name isn't linked in for final builds so do this rather than adding it.
      char buildType[10];
      #if defined (LTCG)
         strcpy_s(buildType, sizeof(buildType), "LTCG");
      #elif defined (BUILD_FINAL)
         strcpy_s(buildType, sizeof(buildType), "F");
      #elif defined (BUILD_PROFILE)
         strcpy_s(buildType, sizeof(buildType), "PROF");
      #elif defined (BUILD_PLAYTEST)
         strcpy_s(buildType, sizeof(buildType), "P");
      #elif defined (BUILD_CHECKED)
         strcpy_s(buildType, sizeof(buildType), "C");
      #elif defined (BUILD_DEBUG)
         strcpy_s(buildType, sizeof(buildType), "D");
      #else
         strcpy_s(buildType, sizeof(buildType), "U");
      #endif

      // Set file name with time stamp
      fileName.format("game:\\FPS_%s_%s_%d-%d-%d_%d%d%d.csv", buildType, pXboxName, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
   }

   void BModeGame::getFPSLogFileNameXFS(BString &fileName)
   {
      const long cXboxNameSize = 256;
      char       pXboxName[cXboxNameSize];
      pXboxName[0] = 0;

      // Get name from xbox
      DmGetXboxName( pXboxName, (LPDWORD)&cXboxNameSize );

      // Get time from system
      SYSTEMTIME sysTime;
      GetLocalTime( &sysTime );

      // Build type.  Library used to get exe name isn't linked in for final builds so do this rather than adding it.
      char buildType[10];
#if defined (LTCG)
      strcpy_s(buildType, sizeof(buildType), "LTCG");
#elif defined (BUILD_FINAL)
      strcpy_s(buildType, sizeof(buildType), "F");
#elif defined (BUILD_PROFILE)
      strcpy_s(buildType, sizeof(buildType), "PROF");
#elif defined (BUILD_PLAYTEST)
      strcpy_s(buildType, sizeof(buildType), "P");
#elif defined (BUILD_CHECKED)
      strcpy_s(buildType, sizeof(buildType), "C");
#elif defined (BUILD_DEBUG)
      strcpy_s(buildType, sizeof(buildType), "D");
#else
      strcpy_s(buildType, sizeof(buildType), "U");
#endif

      BFixedStringMaxPath mapName;
      //int64 numPlayers = 0;
      BGameSettings* pSettings = gDatabase.getGameSettings();
      if (pSettings)
      {
         pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);
         int slashPos = mapName.findRight("\\");
         if (slashPos >= 0)
         {
            mapName.right(slashPos+1);
         }
      }
      else
         mapName.set("NOMAP");

      // Set file name with time stamp
      fileName.format("FPS_%s_%s_%s", buildType, pXboxName, mapName.getPtr());
   }

#endif
