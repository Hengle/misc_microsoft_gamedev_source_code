//==============================================================================
// modecinematic.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modecinematic.h"
#include "database.h"
#include "fontSystem2.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "modemenu.h"
#include "render.h"
#include "ui.h"
#include "configsgame.h"
#include "workdirsetup.h"
#include "reloadManager.h"
#include "camera.h"
#include "keyboard.h"
#include "timingtracker.h"
#include "particlegateway.h"
#include "cinematic.h"

// xcore
#include "math/eulerAngles.h"

// xgame
#include "world.h"
#include "scenario.h"
#include "user.h"
#include "usermanager.h"
#include "transitionManager.h"
#include "scoremanager.h"
#include "damagetemplatemanager.h"

// xgameRender
#include "renderControl.h"
#include "worldVisibility.h"
#include "minimap.h"
#include "viewportManager.h"
 
// terrain
#include "terrain.h"

#include "xphysics.h"

// xvince
#include "vincehelper.h"

#include "uimanager.h"


// Constants
const DWORD cMaxUpdateLength=100; // maximum amount of seconds that can be passed in to ::update()
const float cMaxUpdateLengthFloat=cMaxUpdateLength*0.001f;

const float cCinematicNearClipPlane = 0.05f;
const float cCinematicFarClipPlane = 900.0f;


//==============================================================================
// BModeCinematic::BModeCinematic
//==============================================================================
BModeCinematic::BModeCinematic(long modeType) :
   BMode(modeType),
   mState(-1),
   mNextState(cStateMain),
   mLoaded(false),
   mpWorld(NULL),
   mpTimingTracker(NULL),
   mTimerFrequency(0),
   mFrameStartTime(0),
   mLastUpdateTime(0),
   mCounterOverhead(0),
   mLastUpdateLength(0),
   mTimerFrequencyFloat(0.0f),
   mUpdateLength(0.0f),
   mFrameLength(0.0f),
   mFixedUpdateTime(0.0f),
   mShowDebugInfo(true),
   mShowErrorMsgInfo(true),
   mpFreeCamera(NULL),
   mFreeCameraMovement(0.0f),
   mFreeCameraRotation(0.0f),
   mFreeCameraLModifier(false),
   mFreeCameraRModifier(false),
   mFreeCameraReset(false),
   mFreeCameraCurLoc(0.0f),
   mFreeCameraCurRot(0.0f),
   mSingleStepRepeatTime(0),
   mpCinematicManager(NULL),
   mCurrentCinematicIndex(-1),
   mStoredNearClipPlane(1.0f),
   mStoredFarClipPlane(100.0f)
{
   ASSERT_MAIN_THREAD
}

//==============================================================================
// BModeCinematic::~BModeCinematic
//==============================================================================
BModeCinematic::~BModeCinematic()
{
   ASSERT_MAIN_THREAD
}

//==============================================================================
// BModeCinematic::setup
//==============================================================================
bool BModeCinematic::setup()
{
   ASSERT_MAIN_THREAD
   
   // Free camera   
   mpFreeCamera = new BCamera();
   mpFreeCamera->setZoomLimitOn(false);
   mpFreeCamera->setPitchLimitOn(false);

   return BMode::setup();
}

//==============================================================================
// BModeCinematic::shutdown
//==============================================================================
void BModeCinematic::shutdown()
{
   ASSERT_MAIN_THREAD

   if (mpFreeCamera)
   {
      delete mpFreeCamera;
      mpFreeCamera=NULL;
   }
   
   BMode::shutdown();
}

//==============================================================================
// BModeCinematic::preEnter
//==============================================================================
void BModeCinematic::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeCinematic::enter
//==============================================================================
void BModeCinematic::enter(BMode* lastMode)
{
   ASSERT_MAIN_THREAD

   BSimString fileName=gModeManager.getModeMenu()->getSelectedFileName();
   if(fileName.isEmpty())
      BASSERT(false);


   // Create new cinematic
   BCinematic * pCinematic = new BCinematic();

   pCinematic->init(cDirProduction, fileName.getPtr());
   bool success = pCinematic->load(&mLoadErrorsMsg);

   if (!success)
   {
      //mNextState = cStateExit;
   }
   else
   {
      mLoaded = true;

      // Set game settings
      //
      BSimString scenarioName = pCinematic->getScenarioName();

      gDatabase.resetGameSettings();
      BGameSettings* pSettings = gDatabase.getGameSettings();
      if(pSettings)
      {
         BSimString gameID;
         MVince_CreateGameID(gameID);
         pSettings->setLong(BGameSettings::cPlayerCount, 2);
         pSettings->setString(BGameSettings::cMapName, scenarioName);
         pSettings->setLong(BGameSettings::cMapIndex, -1);
         pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
         pSettings->setString(BGameSettings::cGameID, gameID);
         pSettings->setBool(BGameSettings::cRecordGame, false);
         pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
         pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeScenario);
         pSettings->setLong(BGameSettings::cGameMode, 0);
         pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);
      }

      // Tell the render thread to free its frame storage buffers, to free up some memory for the scenario load.
      gRenderThread.beginLevelLoad();

      // Now tell the render thread to reinitialize frame storage.
      const bool initGameResult = initGame();

      gRenderThread.endLevelLoad();


      if (!initGameResult)
      {
         return;
      }


      // Find cinematic in the cinematic manager list or add cinematic
      //
      mpCinematicManager = mpWorld->getCinematicManager();
      mCurrentCinematicIndex = -1;

      int numCinematics = mpCinematicManager->getNumCinematics();
      for(int i = 0; i < numCinematics; i++)
      {
         BCinematic *pCine = mpCinematicManager->getCinematic(i);

         if(fileName.compare(pCine->getFilename(), false) == 0)
         {
            mCurrentCinematicIndex = i;
            break;
         }
      }

      if(mCurrentCinematicIndex == -1)
      {
         mCurrentCinematicIndex = mpCinematicManager->addCinematic(pCinematic, -1);
      }
      else
      {
         delete pCinematic;
      }

      // start cinematic
      mpCinematicManager->playCinematic(mCurrentCinematicIndex);
   }



   // Set clip planes distances
   mStoredNearClipPlane = gRenderControl.getNearClipPlane();
   mStoredFarClipPlane = gRenderControl.getFarClipPlane();
   gRenderControl.setNearFarClipPlanes(cCinematicNearClipPlane, cCinematicFarClipPlane);
   
   // Set FPS shadow mode
   gRenderControl.setShadowMode(BDirShadowManager::cSMFPS);

#ifndef BUILD_FINAL
   BDisplayStats::reset();
#endif   

   return BMode::enter(lastMode);
}

//==============================================================================
// BModeCinematic::leave
//==============================================================================
void BModeCinematic::leave(BMode* newMode)
{
   ASSERT_MAIN_THREAD

   mLoaded = false;

   // Empty error list
   mLoadErrorsMsg.empty();

   // Restore clip plane distances
   gRenderControl.setNearFarClipPlanes(mStoredNearClipPlane, mStoredFarClipPlane);
      
   deinitGame();

   
   // Yes - Fog of war
   gConfig.remove(cConfigNoFogMask);

   // Speed - 1.0f
   gConfig.set(cConfigGameSpeed, 1.0f);


   // Reset physics memory manager
   XPhysicsMemoryDeinit();

   return BMode::leave(newMode);
}

//==============================================================================
// BModeCinematic::update
//==============================================================================
void BModeCinematic::update()
{
   ASSERT_MAIN_THREAD


   if(mLoaded)
   {
      // DON'T ADD ANY NEW CODE HERE UNLESS YOU ARE SURE... 
      // MOST LIKELY YOUR CODE SHOULD GO IN updateGame() INSTEAD.
      SCOPEDSAMPLE(BModeCinematicUpdate);
      BMode::update();
      float currentUpdateLength=0.0f;
      int64 currentUpdateTime=mLastUpdateTime;
      bool retval=calcUpdateTimes(currentUpdateTime, currentUpdateLength);

      //DWORD startTime=timeGetTime();
      LARGE_INTEGER startTime;
      QueryPerformanceCounter(&startTime);

      updateGame(currentUpdateTime, currentUpdateLength, retval);

      // Restart cinematic if done
      if(!mpCinematicManager->isPlayingCinematic())
      {
         mpCinematicManager->playCinematic(mCurrentCinematicIndex);
      }

      BCinematic *pCinematic = mpCinematicManager->getCinematic(mCurrentCinematicIndex);
/*
      if(pCinematic->getState() == BCinematic::cStateEnded)
      {
         pCinematic->rewindToStart();
         pCinematic->play();
      }
*/

      // Update free camera
      if(pCinematic->getCameraMode() == BCinematic::cCameraModeFree)
      {
         float elapsedTime=(gGame.getFrameTime()*0.001f);

         updateFreeCamera(elapsedTime);

         // Set camera
         BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();

         camera->setCameraRight(mpFreeCamera->getCameraRight());
         camera->setCameraDir(mpFreeCamera->getCameraDir());
         camera->setCameraUp(mpFreeCamera->getCameraUp());
         camera->setCameraLoc(mpFreeCamera->getCameraLoc());
         camera->setFOV(mpFreeCamera->getFOV());
      }


      //if(retval && mpWorld->getUpdateNumber()>=mTimingTrackerUpdate)
      if (retval)
      {
         //mUpdateLength=timeGetTime()-startTime;
         LARGE_INTEGER endTime;
         QueryPerformanceCounter(&endTime);
         int64 delta = mLastUpdateLength = (int64)(endTime.QuadPart - startTime.QuadPart - mCounterOverhead);
         mUpdateLength = static_cast<float>(delta/mTimerFrequencyFloat);
         if (mpTimingTracker)
            mpTimingTracker->addLastUpdateLength((DWORD)(mUpdateLength*1000.0f+0.5f));
         //mpTimingTracker->addLastUpdateLength2(mUpdateLength);
      }
   }

   if(mNextState!=-1)
   {
      mState=mNextState;
      mNextState=-1;

      BHandle fontHandle;
      fontHandle=gFontManager.getFontDenmark24();
      gFontManager.setFont(fontHandle);

      float yh=gFontManager.getLineHeight();

      mList.setFont(fontHandle);
      mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
      mList.setRowSpacing(yh);
      mList.setColors(cColorWhite, cColorCyan);
      mList.setColumnWidth(gUI.mfSafeWidth/3.0f);
      mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-3);
      mList.setMultiColumn(true);
      mList.setJustifyCenter(false);

      mList.clearItems();
      mList.addItem("Start - Exit");
   }
   else
   {
      switch(mState)
      {
         case cStateMain : 
         {
            mList.update();
            break;
         }
         case cStateExit: 
         {
            mNextState=cStateMain;
            // make sure we go back to the listing.
            gModeManager.getModeMenu()->setNextState(BModeMenu::cStateCinematic);
            gModeManager.setMode(BModeManager::cModeMenu);
            break;
         }
      }
   }
}



//==============================================================================
// BModeCinematic::updateGame
//==============================================================================
void BModeCinematic::updateGame(int64 currentUpdateTime, float currentUpdateLength, bool updateSim)
{
   SCOPEDSAMPLE(BModeCinematicUpdateGame);

#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif

   gpDebugPrimitives->beginFrame(mpWorld ? mpWorld->getGametimeFloat() : 1.0f/30.0f);

   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryPhysics);
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryMovement);
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryCinematics);
   
   if((updateSim) && (mpWorld))
      mpWorld->update(currentUpdateTime, currentUpdateLength, mTimerFrequencyFloat, false);

   // moving this out of the world updatePreAsync so that it still works when the game is over
   gWorld->getTransitionManager()->update();

   gVisualManager.update();

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
}

//==============================================================================
// BModeCinematic::frameStart
//==============================================================================
void BModeCinematic::frameStart()
{
   if (!getFlag(cFlagGameInit))
      return;

   LARGE_INTEGER time;
   QueryPerformanceCounter(&time);
   mFrameStartTime=time.QuadPart;
   mUpdateLength=0.0f;
   mFrameLength=0.0f;
}

//==============================================================================
// BModeCinematic::frameEnd
//==============================================================================
void BModeCinematic::frameEnd()
{
   if (!getFlag(cFlagGameInit))
      return;

   LARGE_INTEGER frameEndTime;
   QueryPerformanceCounter(&frameEndTime);

   int64 delta = (int64)(frameEndTime.QuadPart - mFrameStartTime - mCounterOverhead - mLastUpdateLength);
   mFrameLength = static_cast<float>(delta/mTimerFrequencyFloat);
   //mFrameLength -= mUpdateLength;

   if (mpTimingTracker)
      mpTimingTracker->addLastFrameLength((DWORD)(mFrameLength*1000.0f+0.5f));
   //mpTimingTracker->addLastFrameLength2(mFrameLength);
}

//==============================================================================
// BModeCinematic::initGame
//==============================================================================
bool BModeCinematic::initGame()
{
   // Init the physics memory manager
   XPhysicsMemoryInit();

   // Clear flags
   mFlags.setAll(0);   

   gParticleGateway.initMemoryPools();
   gParticleGateway.initTextureSystem();

   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);
   mTimerFrequency=freq.QuadPart;
   mTimerFrequencyFloat=(double)mTimerFrequency;

   LARGE_INTEGER counter1;
   LARGE_INTEGER counter2;
   QueryPerformanceCounter(&counter1);
   QueryPerformanceCounter(&counter2);
   mCounterOverhead = (int64)(counter2.QuadPart - counter1.QuadPart);

   gUIGame.init();

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
      blogerrortrace("BModeCinematic::init: initWorld failed");
      return false;
   }

   if (!gUIManager->initPlayerSpecific())
      return false;
   
   // Init the timing tracker
   if (mpTimingTracker)
      delete mpTimingTracker;
   mpTimingTracker=new BTimingTracker();


   // No Fog of war
   gConfig.define(cConfigNoFogMask);

   gScoreManager.reset();
   gScoreManager.initializePlayers();

   setFlag(cFlagGameInit, true);

   return true;
}


//==============================================================================
// BModeCinematic::deinitGame
//==============================================================================
void BModeCinematic::deinitGame()
{
   deinitWorld();

   if (gUIManager)
   {
      gUIManager->deinit();
      delete gUIManager;
   }
   gUIManager = NULL;

   gParticleGateway.deinitTextureSystem();
   gParticleGateway.deinitMemoryPools();
}

//==============================================================================
// BModeCinematic::initWorld
//==============================================================================
bool BModeCinematic::initWorld()
{
   // Load the map/scenario
   if(!gScenario.load(false, false))
   {
      gConsoleOutput.error("BModeCinematic::initWorld: gScenario.load() FAILED!\n");

      if(gWorld)
      {
         gTerrain.destroy();

         gWorld->reset();
         delete gWorld;
         gWorld=NULL;
      }
      return false;
   }

   mpWorld=gWorld;

   setFlag(cFlagTerrainInit, true);

   return true;
}

//==============================================================================
// BModeCinematic::deinitWorld
//==============================================================================
void BModeCinematic::deinitWorld()
{
   if(mpWorld)
   {
      mpWorld->reset();
      delete mpWorld;
      mpWorld=NULL;
      gWorld=NULL;
   }


   gDamageTemplateManager.reset();

   gUserManager.getUser(BUserManager::cPrimaryUser)->gameRelease();

   //-- Unload the scenario
   gScenario.unload();

   if(getFlag(cFlagTerrainInit))
   {
      gTerrain.destroy();
      setFlag(cFlagTerrainInit, false);
   }
}

//==============================================================================
// BModeCinematic::renderBegin
//==============================================================================
void BModeCinematic::renderBegin()
{
}


//==============================================================================
// BModeCinematic::render
//==============================================================================
void BModeCinematic::render()
{
   ASSERT_MAIN_THREAD

   if(mLoaded)
   {
      SCOPEDSAMPLE(BModeCinematicRender);
            
      const uint numMacroTiles = gRenderControl.getNumMacroTiles();
      for (uint macroTileIndex = 0; macroTileIndex < numMacroTiles; macroTileIndex++)
      {
         BUser* users[1] = { gUserManager.getUser(BUserManager::cPrimaryUser) };
         gRenderControl.startOfFrame(mpWorld, 1, users, macroTileIndex);
         
         gRenderControl.beginViewport(BUserManager::cPrimaryUser, gUserManager.getUser(BUserManager::cPrimaryUser), 0);
                  
         gRenderControl.renderViewport();
         
         if (numMacroTiles == 1)
         {
            gFontManager.render3D();
         
            mpWorld->getCinematicManager()->postRender();

            renderCinematicUI();
               
            mpWorld->getTransitionManager()->postRender();
         
            gFontManager.render2D();
         }            
         
         gRenderControl.endViewport();
         
         gRenderControl.endOfFrame();
      }  
   }
   else
   {
      gRender.beginFrame(1.0f/30.0f);
      gRender.beginViewport(-1);
      
      gRenderDraw.beginScene();
      gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);

      renderCinematicUI();

      gFontManager.render2D();

      gRenderDraw.endScene();
      gRenderDraw.present(NULL, NULL);
      
      gRender.endViewport();
      gRender.endFrame();
   }

   gEventDispatcher.sleep(16);
}

//==============================================================================
// BModeCinematic::renderEnd
//==============================================================================
void BModeCinematic::renderEnd()
{
}


//==============================================================================
// BModeCinematic::renderCinematicUI
//==============================================================================
void BModeCinematic::renderCinematicUI()
{
   BHandle fontHandle;
   fontHandle=gFontManager.getFontDenmark18();
   gFontManager.setFont(fontHandle);


   BCinematic *pCinematic = mpCinematicManager->getCinematic(mCurrentCinematicIndex);

   if(mShowDebugInfo)
   {
      BSimString text;

      float y = 580.0f;
      float x = 70.0f;
      float rowSpacing = 26.0f;

      text.format("Cinematic:  %s", gModeManager.getModeMenu()->getSelectedFileName().getPtr());
      gFontManager.drawText(fontHandle, x, y, text.getPtr(), cDWORDGreen);
      y += rowSpacing;

      text.format("Total Duration:  %1.2f - Current Time:  %1.2f", pCinematic->getDuration(), pCinematic->getCurrentTime());
      gFontManager.drawText(fontHandle, x, y, text.getPtr(), cDWORDGreen);
      y += rowSpacing;

      // Shot info
      const BShot *pShot = pCinematic->getCurrentShot();
      if(pShot)
      {
         text.format("Current Shot:  %s - Shot Duration:  %1.2f - Shot Time:  %1.2f", pShot->mName.getPtr(), pShot->mDuration, pCinematic->getCurrentTime() - pShot->mStartTime);
         gFontManager.drawText(fontHandle, x, y, text.getPtr(), cDWORDGreen);
         y += rowSpacing;
      }


      BSimString stateText;
      if(getPaused())
         stateText.format("Paused");
      else
         stateText.format("Playing");

      BSimString modeText;
      if(pCinematic->getCameraMode() == BCinematic::cCameraModeLocked)
         modeText.format("Locked");
      else
         modeText.format("Free");

      BSimString speedText;
    
      float gameSpeed = 1.0f;
      gConfig.get(cConfigGameSpeed, &gameSpeed);

      if(gameSpeed == 16.0f) speedText.format("16");
      else if(gameSpeed == 8.0f) speedText.format("8");
      else if(gameSpeed == 4.0f) speedText.format("4");
      else if(gameSpeed == 2.0f) speedText.format("2");
      else if(gameSpeed == 1.0f) speedText.format("1");
      else if(gameSpeed == 0.5f) speedText.format("1/2");
      else if(gameSpeed == 0.25f) speedText.format("1/4");
      else if(gameSpeed == 0.125f) speedText.format("1/8");
      else if(gameSpeed == 0.0625f) speedText.format("1/16");
      else speedText.format("%3.1f", gameSpeed);
      

      text.format("State:  %s - Camera Mode:  %s - Playback Speed:  %s", stateText.getPtr(), modeText.getPtr(), speedText.getPtr());
      gFontManager.drawText(fontHandle, x, y, text.getPtr(), cDWORDGreen);
      y += rowSpacing;



      // Display error messages
      if(mShowErrorMsgInfo)
      {
         BHandle fontHandle;
         fontHandle=gFontManager.getFontCourier10();
         gFontManager.setFont(fontHandle);

         gFontManager.drawText(fontHandle, x, 40.0f, mLoadErrorsMsg.getPtr());
      }
   }

   renderUIShowStats();

   //-- we need to call this to clear out the gpu frame heap allocations.
   gUIManager->releaseGPUHeapResources();
}

//==============================================================================
// BModeCinematic::renderUIShowStats
//==============================================================================
void BModeCinematic::renderUIShowStats(void)
{
#ifndef BUILD_FINAL
   BDisplayStats::show(mpWorld, mpTimingTracker, NULL);
#endif
}


//==============================================================================
// BModeCinematic::handleInput
//==============================================================================
bool BModeCinematic::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   ASSERT_MAIN_THREAD

   if (gRenderControl.handleInput(port, event, controlType, detail))
      return true;

#ifndef BUILD_FINAL
   if (BDisplayStats::handleInput(port, event, controlType, detail))
      return true;
#endif   

   BCinematic *pCinematic = mpCinematicManager->getCinematic(mCurrentCinematicIndex);

   if(pCinematic->getCameraMode() == BCinematic::cCameraModeFree)
   {
      handleInputFreeCamera(port, event, controlType, detail);
   }
      
   if(event==cInputEventControlStart)
   {
      switch(controlType)
      {
         case cKeyPause:
         case cKeyF16:
            {
               setPaused(!getPaused());
               return true;
            }

         case cKey1:
            gConfig.set(cConfigGameSpeed, 0.25f);
            break;
         case cKey2:
            gConfig.set(cConfigGameSpeed, 0.5f);
            break;
         case cKey3:
            gConfig.set(cConfigGameSpeed, 1.0f);
            break;
         case cKey4:
            gConfig.set(cConfigGameSpeed, 2.0f);
            break;
         case cKey5:
            gConfig.set(cConfigGameSpeed, 4.0f);
            break;


         case cButtonStart:
         case cKeyEscape:
            {
               gUI.playClickSound();
               mNextState = cStateExit;

               return true;
            }
         case cButtonA:
            {
               gUI.playClickSound();
               setPaused(!getPaused());
               return true;
            }
         case cButtonB:
            {
               gUI.playClickSound();
               pCinematic->rewindToStart();

               return true;
            }
         case cButtonX:
         case cKeySpace:
            {
               gUI.playClickSound();
               toggleShowDebugInfo();
               break;
            }
         case cButtonY:
         case cKeyF7:
            {
               gUI.playClickSound();
               pCinematic->toggleCameraMode();



               if(pCinematic->getCameraMode() == BCinematic::cCameraModeFree)
               {
//-- FIXING PREFIX BUG ID 4977
                  const BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
//--

                  BVector loc = camera->getCameraLoc();
                  BVector dir = camera->getCameraDir();
                  BVector up = camera->getCameraUp();
                  BVector right = camera->getCameraRight();

                  BMatrix mat;
                  mat.makeOrient(dir, up, right);
                  mat.setTranslation(loc);

                  BEulerAngles euler(EulOrdXYZr);
                  euler.setFromMatrix(mat);

                  mFreeCameraCurLoc.set(loc.x, loc.y, loc.z);
                  //mFreeCameraCurRot.set(euler.x, euler.y, 0.0f);
                  mFreeCameraCurRot.set(0.0f, 0.0f, 0.0f);

                  mFreeCameraMovement.clear();
                  mFreeCameraRotation.clear();
               }
               return true;
            }

         case cKeyUp:
         case cButtonShoulderRight:
            {
               gUI.playClickSound();

               float gameSpeed = 1.0f;
               gConfig.get(cConfigGameSpeed, &gameSpeed);

               if(gameSpeed <= 0.0625f) gameSpeed = 0.125f;
               else if(gameSpeed <= 0.125f) gameSpeed = 0.25f;
               else if(gameSpeed <= 0.25f) gameSpeed = 0.5f;
               else if(gameSpeed <= 0.5f) gameSpeed = 1.0f;
               else if(gameSpeed <= 1.0f) gameSpeed = 2.0f;
               else if(gameSpeed <= 2.0f) gameSpeed = 4.0f;
               else if(gameSpeed <= 4.0f) gameSpeed = 8.0f;
               else if(gameSpeed <= 8.0f) gameSpeed = 16.0f;

               gConfig.set(cConfigGameSpeed, gameSpeed);
               return true;
            }

         case cKeyDown:
         case cButtonShoulderLeft:
            {
               gUI.playClickSound();

               float gameSpeed = 1.0f;
               gConfig.get(cConfigGameSpeed, &gameSpeed);


               if(gameSpeed >= 16.0f) gameSpeed = 8.0f;
               else if(gameSpeed >= 8.0f) gameSpeed = 4.0f;
               else if(gameSpeed >= 4.0f) gameSpeed = 2.0f;
               else if(gameSpeed >= 2.0f) gameSpeed = 1.0f;
               else if(gameSpeed >= 1.0f) gameSpeed = 0.5f;
               else if(gameSpeed >= 0.5f) gameSpeed = 0.25f;
               else if(gameSpeed >= 0.25f) gameSpeed = 0.125f;
               else if(gameSpeed >= 0.125f) gameSpeed = 0.0625f;

               gConfig.set(cConfigGameSpeed, gameSpeed);
               return true;
            }

         case cDpadRight:
         case cKeyRight:
            {
               gUI.playClickSound();
               pCinematic->forwardToNextShot();

               return true;
            }
         case cDpadLeft:
         case cKeyLeft:
            {
               gUI.playClickSound();
               pCinematic->rewindToPreviousShot();

               return true;
            }
      }
   }

   /*
   if (controlType==cKeyTab || controlType == cDpadRight)
   {
      if(getFlag(cFlagPaused))
      {
         if(event==cInputEventControlStart)
         {
            setFlag(cFlagSingleStep, true);
            mSingleStepRepeatTime=timeGetTime()+200;
         }
         else if(event==cInputEventControlStop)
            setFlag(cFlagSingleStep, false);
         else if(event==cInputEventControlRepeat)
         {
            if(timeGetTime()>=mSingleStepRepeatTime)
               setFlag(cFlagSingleStep, true);
         }
      }
   }
   */

   return false;
}

//==============================================================================
// BModeCinematic::handleInputFreeCamera
//==============================================================================
bool BModeCinematic::handleInputFreeCamera(long port, long event, long controlType, BInputEventDetail& detail)
{
   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);

   if(start || repeat || stop)
   {
      switch(controlType)
      {
         case cTriggerLeft:
         {
            mFreeCameraLModifier = stop ? false : true;
            return true;
         }
         case cTriggerRight:
         {
            mFreeCameraRModifier = stop ? false : true;
            return true;
         }
         // Move forward/backward, left/right
         case cStickLeft:
         {
            if(stop)
               mFreeCameraMovement.clear();
            else
            {
               static float cMoveDefMultiplier = 35.0f;
               static float cMoveExtraMultiplier = 3.0f;

               if (mFreeCameraLModifier)
                  mFreeCameraMovement.set(detail.mX, -detail.mY, 0.0f);
               else
                  mFreeCameraMovement.set(detail.mX, 0.0f, -detail.mY);

               mFreeCameraMovement *= cMoveDefMultiplier;

               if (mFreeCameraRModifier)
                  mFreeCameraMovement *= cMoveExtraMultiplier;
            }
            return true;
         }
         // Yaw/pitch
         case cStickRight:
         {
            static float cRadPerSec = 50.0f;
            static float cRotMultiplier = 2.0f;
            if(stop)
               mFreeCameraRotation.clear();
            else
               mFreeCameraRotation.set(-detail.mY * cRadPerSec, detail.mX * cRadPerSec, 0.0f);

            if (mFreeCameraRModifier)
               mFreeCameraRotation *= cRotMultiplier;
            return true;
         }            
         case cKeyHome:
         {
            if (!stop)
               mFreeCameraReset = true;
            return true;
         }
      }
   }
   return false;
}


//==============================================================================
// BModeCinematic::updateFreeCamera
//==============================================================================
void BModeCinematic::updateFreeCamera(float elapsedTime)
{
   if (mFreeCameraReset)
   {
      mFreeCameraReset = false;

      mFreeCameraCurRot.clear();
   }

   BMatrix44 x(BMatrix44::makeRotate(0, mFreeCameraCurRot[0]));
   BMatrix44 y(BMatrix44::makeRotate(1, mFreeCameraCurRot[1]));
   BMatrix44 z(BMatrix44::makeRotate(2, mFreeCameraCurRot[2]));
   BMatrix44 t(BMatrix44::makeTranslate(BVec4(-mFreeCameraCurLoc, 1.0f)));
      
   BMatrix44 worldToView(t * y * x * z);
   
   mpFreeCamera->setCameraDir(BVector(worldToView.getColumn(2)[0], worldToView.getColumn(2)[1], worldToView.getColumn(2)[2]));
   mpFreeCamera->setCameraUp(BVector(worldToView.getColumn(1)[0], worldToView.getColumn(1)[1], worldToView.getColumn(1)[2]));
   mpFreeCamera->calcCameraRight();
   
   mpFreeCamera->setCameraLoc(BVector(mFreeCameraCurLoc[0], mFreeCameraCurLoc[1], mFreeCameraCurLoc[2]));
         
   mFreeCameraCurRot += mFreeCameraRotation * cRadiansPerDegree * elapsedTime;
      
   BMatrix44 viewToWorld(worldToView);
   viewToWorld.invert();
   
   BVec4 worldMovement(BVec4(mFreeCameraMovement, 0.0f) * viewToWorld);
   
   mFreeCameraCurLoc += worldMovement * elapsedTime;
}


//==============================================================================
// BModeCinematic::calcUpdateTimes
//==============================================================================
bool BModeCinematic::calcUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength)
{
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

   if (mpWorld)
   {
      if(mpWorld->getUpdateNumber()==0)
         return true;
   }


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
   else
   {
      int64 delta=currentUpdateTime-lastUpdateTime;
      currentUpdateLength = static_cast<float>(delta/mTimerFrequencyFloat);
      if(currentUpdateLength>cMaxUpdateLengthFloat)
         currentUpdateLength=cMaxUpdateLengthFloat;

      float gameSpeed=1.0f;
      gConfig.get(cConfigGameSpeed, &gameSpeed);
      if(gameSpeed!=1.0f)
      {
         currentUpdateLength*=gameSpeed;         
      }
      gParticleGateway.setTimeSpeed(gameSpeed);

      DWORD len = (DWORD)(currentUpdateLength * mTimerFrequencyFloat);
      if (mpWorld)
         currentUpdateTime=mpWorld->getLastUpdateTime()+len;
   }

   return true;
}
