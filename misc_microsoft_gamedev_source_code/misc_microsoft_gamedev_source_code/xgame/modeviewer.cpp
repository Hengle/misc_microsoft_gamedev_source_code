//==============================================================================
// modeviewer.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modeviewer.h"
#include "keyboard.h"
#include "database.h"

#include "camera.h"

#include "game.h"
#include "gamecallbacks.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "modemenu.h"
#include "timingtracker.h"

#include "xvisual.h"

#include "user.h"
#include "usermanager.h"

#include "render.h"

#include "fontSystem2.h"

#include "D3DTextureLoader.h"

#include "debugprimitives.h"

#include "configsgame.h"
#include "configsgamerender.h"

// xgame
#include "world.h"
#include "scenario.h"
#include "damagetemplatemanager.h"

// xgameRender
#include "renderControl.h"
#include "viewportManager.h"

// terrain
#include "terrain.h"

#include "xphysics.h"

// xvince
#include "vincehelper.h"


#include "uimanager.h"

#include "protoobject.h"

#ifndef BUILD_FINAL
   #include "consoleRender.h"
#endif


// Constants
const DWORD cMaxUpdateLength=100; // maximum amount of seconds that can be passed in to ::update()
const float cMaxUpdateLengthFloat=cMaxUpdateLength*0.001f;

const float cRotateSpeed=45.0f*cRadiansPerDegree;
const float cViewerYawRate=160.0f*cRadiansPerDegree;
const float cViewerPitchRate=160.0f*cRadiansPerDegree;
const float cViewerZoomRate=8.0f;
const float cViewerPanRate=3.5f;

const float cDefaultBoundingBoxMaxSide=50.0f;

//==============================================================================
// BModeViewer::BModeViewer
//==============================================================================
BModeViewer::BModeViewer(long modeType) :
   BMode(modeType),
   mState(-1),
   mNextState(cStateMain),
   mModelMatrix(),
   mModelRotation(0.0f),
   mLookAtPosition(cOriginVector),
   mViewerYaw(0.0f),
   mViewerPitch(0.0f),
   mViewerZoom(0.0f),
   mViewerPanX(0.0f),
   mViewerPanY(0.0f),
   mViewerDistance(0.0f),
   mShowDebugInfo(true),

   mpWorld(NULL),
   mpObject(NULL),
   mpUnit(NULL),
   mpSquad(NULL),
   mCurrentProtoVisualGen(-1),
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
   mSelectedModel(0),
   mSelectedAnim(0),
   mBoundingBoxMaxSide(cDefaultBoundingBoxMaxSide),
   mBackgroundMode(cBackgroundModeGrid),
   mIsMotionExtractionEnabled(true)
{
   // Get model list
   //
   mModelList.clear();

   long count=gDatabase.getNumberProtoObjects();
   for(long i=0; i<count; i++)
   {
      mModelList.add(gDatabase.getGenericProtoObject(i)->getName());
   }

   mModelList.sort();
}

//==============================================================================
// BModeViewer::~BModeViewer
//==============================================================================
BModeViewer::~BModeViewer()
{
}

//==============================================================================
// BModeViewer::setup
//==============================================================================
bool BModeViewer::setup()
{
   return BMode::setup();
}

//==============================================================================
// BModeViewer::preEnter
//==============================================================================
void BModeViewer::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeViewer::enter
//==============================================================================
void BModeViewer::enter(BMode* lastMode)
{
   gConsoleOutput.output(cMsgError, "Made it to BModeViewer::enter");

   BSimString fileName=gModeManager.getModeMenu()->getSelectedFileName();
   if(fileName.isEmpty())
   {
      setupError();
      return;
   }
   
   
   /*
   long protoID = gDatabase.getProtoObject(fileName.getPtr());
   if(protoID == -1)
   */
   mSelectedModel = mModelList.find(fileName.getPtr());
   if(mSelectedModel == -1)
   {
      setupError();
      return;
   }


   mModelRotation=0.0f;
   mShowDebugInfo = true;
   mBackgroundMode = cBackgroundModeGrid;
   mIsMotionExtractionEnabled = true;

   // Set game settings
   //
   BSimString scenarioName;
   scenarioName.set("development\\blank\\blank");

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

   if(!initGameResult)
   {
      setupError();
      return;
   }

   
   // Load model
   //
   initModel();
   initCamera();


   // No terrain
   gRenderControl.setFlag(BRenderControl::cFlagDisableRenderTerrain, true);
   
   // Set FPS shadow mode
   gRenderControl.setShadowMode(BDirShadowManager::cSMFPS);

   // No Fog of war
   gConfig.define(cConfigNoFogMask);


   BMode::enter(lastMode);

   gRender.resetAverageFPS();

#ifndef BUILD_FINAL   
   BDisplayStats::reset();
#endif   
}


//==============================================================================
// BModeViewer::setupError
//==============================================================================
void BModeViewer::setupError()
{
   gModeManager.getModeMenu()->setNextState(BModeMenu::cStateModelView);
   gModeManager.setMode(BModeManager::cModeMenu);
}


//==============================================================================
// BModeViewer::leave
//==============================================================================
void BModeViewer::leave(BMode* newMode)
{
   deinitGame();

   mpObject = NULL;
   mpUnit = NULL;
   mpSquad = NULL;
   mSelectedModel = 0;
   mSelectedAnim = 0;
   mAnimList.clear();


   // Yes - terrain
   gRenderControl.setFlag(BRenderControl::cFlagDisableRenderTerrain, false);

   // Yes - Fog of war
   gConfig.remove(cConfigNoFogMask);

   // Speed - 1.0f
   gConfig.set(cConfigGameSpeed, 1.0f);


   // Reset physics memory manager
   XPhysicsMemoryDeinit();

   return BMode::leave(newMode);
}


//==============================================================================
// BModeViewer::update
//==============================================================================
void BModeViewer::update()
{
   // DON'T ADD ANY NEW CODE HERE UNLESS YOU ARE SURE... 
   // MOST LIKELY YOUR CODE SHOULD GO IN updateGame() INSTEAD.
   SCOPEDSAMPLE(BModeViewerUpdate);
   BMode::update();
   float currentUpdateLength=0.0f;
   int64 currentUpdateTime=mLastUpdateTime;
   bool retval=calcUpdateTimes(currentUpdateTime, currentUpdateLength);

   //DWORD startTime=timeGetTime();
   LARGE_INTEGER startTime;
   QueryPerformanceCounter(&startTime);


   updateGame(currentUpdateTime, currentUpdateLength, retval);


   // Reset object position since the animation may have motion extraction, in which
   // case the object moves out or the viewport and looks bad.
   if(mpObject)
   {
      if(!mIsMotionExtractionEnabled)
         resetObjectPosition();

      // Check if we need to reload
      if(mpObject->getVisual() && mpObject->getVisual()->getProtoVisual())
      {
         long gen = mpObject->getVisual()->getProtoVisual()->getGeneration();
         if(mCurrentProtoVisualGen != gen)
         {
            uint prevAnimCount = mAnimList.getSize();
            long prevSelectedAnim = mSelectedAnim;
            BSimString prevSelectedAnimName;
            if((prevAnimCount > 0) &&
               (mSelectedAnim < static_cast<long>(prevAnimCount)))
            {
               prevSelectedAnimName = mAnimList[mSelectedAnim].mName;
            }

            // update model
            initModel();
            initCamera();

            // Restore selected anim if the number of animations hasn't changed and the current
            // and previous animation names match.
            if((mAnimList.getSize() > 0) &&
               (prevAnimCount == mAnimList.getSize()) &&
               (prevSelectedAnimName.compare(mAnimList[prevSelectedAnim].mName) == 0))
            {
               mSelectedAnim = prevSelectedAnim;

               playSelectedAnimation();
            }
         }
      }
   }

   // Start a playBlockingAnimationAction on the unit if this action is not active
   if(mpUnit && (mAnimList.getSize() > 0))
   {
      BAction *action = mpUnit->getActionByType(BAction::cActionTypeUnitPlayBlockingAnimation);

      if(action == NULL)
      {
         playSelectedAnimation();
      }
   }

   //if(retval && mpWorld->getUpdateNumber()>=mTimingTrackerUpdate)
   if (retval)
   {
      //mUpdateLength=timeGetTime()-startTime;
      LARGE_INTEGER endTime;
      QueryPerformanceCounter(&endTime);
      int64 delta = mLastUpdateLength = (int64)(endTime.QuadPart - startTime.QuadPart - mCounterOverhead);
      mUpdateLength = static_cast<float>(delta/mTimerFrequencyFloat);
      mpTimingTracker->addLastUpdateLength((DWORD)(mUpdateLength*1000.0f+0.5f));
      //mpTimingTracker->addLastUpdateLength2(mUpdateLength);
   }


   float elapsed=(gGame.getFrameTime()*0.001f);
   float sizeSpeedFactor = mBoundingBoxMaxSide * 0.20f;

   BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();

   camera->setFOV(DEGREES_TO_RADIANS(45));

   if(mViewerPanX!=0.0f)
   {
      float dist=mViewerPanX*elapsed*cViewerPanRate*sizeSpeedFactor;
      camera->moveRight(dist);

      mLookAtPosition += dist*camera->getCameraRight();
   }

   if(mViewerPanY!=0.0f)
   {
      float dist=mViewerPanY*elapsed*cViewerPanRate*sizeSpeedFactor;
      camera->moveUp(dist);

      mLookAtPosition += dist*camera->getCameraUp();
   }

   if(mViewerZoom!=0.0f)
   {
      float dist=mViewerZoom*elapsed*cViewerZoomRate*sizeSpeedFactor;
      if(dist<mViewerDistance)
      {
         mViewerDistance-=dist;
         camera->moveForward(dist);
      }
   }

   if(mViewerYaw!=0.0f)
   {
      float yaw=mViewerYaw*cViewerYawRate*elapsed;
      if(yaw>=cTwoPi) yaw-=cTwoPi; else if(yaw<0.0f) yaw+=cTwoPi;
      camera->yawWorldAbout(yaw, mLookAtPosition);
   }

   if(mViewerPitch!=0.0f)
   {
      float pitch=mViewerPitch*cViewerPitchRate*elapsed;
      if(pitch>=cTwoPi) pitch-=cTwoPi; else if(pitch<0.0f) pitch+=cTwoPi;
      camera->pitchWorldAbout(pitch, mLookAtPosition);
   }

   BMode::update();

   if(mNextState!=-1)
   {
      mState=mNextState;
      mNextState=-1;
   }
   else
   {
      switch(mState)
      {
      case cStateExit: 
         {
            mNextState=cStateMain;
            gModeManager.getModeMenu()->setNextState(BModeMenu::cStateModelView);
            gModeManager.setMode(BModeManager::cModeMenu);
            break;
         }
      }
   }
}

//==============================================================================
// BModeViewer::updateGame
//==============================================================================
void BModeViewer::updateGame(int64 currentUpdateTime, float currentUpdateLength, bool updateSim)
{
   SCOPEDSAMPLE(BModeCinematicUpdateGame);

   gpDebugPrimitives->beginFrame(mpWorld->getGametimeFloat());

   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryPhysics);
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryMovement);
   gpDebugPrimitives->clear(BDebugPrimitives::cCategoryCinematics);

   if(updateSim)
      mpWorld->update(currentUpdateTime, currentUpdateLength, mTimerFrequencyFloat, false);

   gVisualManager.update();
}

//==============================================================================
// BModeViewer::frameStart
//==============================================================================
void BModeViewer::frameStart()
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
// BModeViewer::frameEnd
//==============================================================================
void BModeViewer::frameEnd()
{
   if (!getFlag(cFlagGameInit))
      return;

   LARGE_INTEGER frameEndTime;
   QueryPerformanceCounter(&frameEndTime);

   int64 delta = (int64)(frameEndTime.QuadPart - mFrameStartTime - mCounterOverhead - mLastUpdateLength);
   mFrameLength = static_cast<float>(delta/mTimerFrequencyFloat);
   //mFrameLength -= mUpdateLength;

   mpTimingTracker->addLastFrameLength((DWORD)(mFrameLength*1000.0f+0.5f));
   //mpTimingTracker->addLastFrameLength2(mFrameLength);
}

//==============================================================================
// BModeViewer::initGame
//==============================================================================
bool BModeViewer::initGame()
{
   // Init the physics memory manager
   XPhysicsMemoryInit();

   // Clear flags
   mFlags.setAll(0);   
   
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
      blogerrortrace("BModeViewer::init: initWorld failed");
      return false;
   }

   if (!gUIManager->initPlayerSpecific())
      return false;   

   // Init the timing tracker
   if (mpTimingTracker)
      delete mpTimingTracker;
   mpTimingTracker=new BTimingTracker();

   setFlag(cFlagGameInit, true);

   return true;
}


//==============================================================================
// BModeViewer::deinitGame
//==============================================================================
void BModeViewer::deinitGame()
{
   // TEMP CODE ONLY - Add this for the time being until billy changes his code
   gUIGame.deinit();


   deinitWorld();

   if (gUIManager)
   {
      gUIManager->deinit();
      delete gUIManager;
   }
   gUIManager = NULL;
}

//==============================================================================
// BModeViewer::initWorld
//==============================================================================
bool BModeViewer::initWorld()
{
   // Load the map/scenario
   if(!gScenario.load(false, false))
   {
      gConsoleOutput.error("BModeViewer::initWorld: gScenario.load() FAILED!\n");

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
// BModeViewer::deinitWorld
//==============================================================================
void BModeViewer::deinitWorld()
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
// BModeViewer::renderBegin
//==============================================================================
void BModeViewer::renderBegin()
{
}


//==============================================================================
// BModeViewer::render
//==============================================================================
void BModeViewer::render()
{
   const uint numMacroTiles = gRenderControl.getNumMacroTiles();
   for (uint macroTileIndex = 0; macroTileIndex < numMacroTiles; macroTileIndex++)
   {
      BUser* users[1] = { gUserManager.getUser(BUserManager::cPrimaryUser) };
      gRenderControl.startOfFrame(mpWorld, 1, users, macroTileIndex);
      
      gRenderControl.beginViewport(BUserManager::cPrimaryUser, gUserManager.getUser(BUserManager::cPrimaryUser), 0);
      
      if(mBackgroundMode == cBackgroundModeGrid)
         renderGrid();
      
      gRenderControl.renderViewport();
            
      if (numMacroTiles == 1)
      {
         gFontManager.render3D();
         
         renderViewerUI();
            
         gFontManager.render2D();

#ifndef BUILD_FINAL
         gConsoleRender.renderSubmit();
#endif
      }      

      gRenderControl.endViewport();
      
      gRenderControl.endOfFrame();
   }  
}

//==============================================================================
// BModeViewer::renderEnd
//==============================================================================
void BModeViewer::renderEnd()
{
}



//==============================================================================
// BModeViewer::renderGrid
//==============================================================================
void BModeViewer::renderGrid()
{
   const float cGridIncrement = 1.0f;
 
   int numLines = ((int)((mBoundingBoxMaxSide / cGridIncrement) + 1.0f)) * 2 + 1;
   float gridMax = cGridIncrement * ((numLines - 1) / 2);

   DWORD color1 = 0xffe0e0e0;
   DWORD color2 = 0xff1e1e1e;
   //DWORD color2 = 0xff606060;

   BVector point1, point2;

   float middleOfMap = gTerrainVisual.getTileScale() * gTerrainVisual.getNumXVerts() / 2.0f;


   int x, z;
   for (x = 0; x < numLines; x++)
   {
      float xValue = x * cGridIncrement - gridMax;
      uint color;

      if (x == ((numLines - 1) / 2))
      {
         color = color1;
      }
      else
      {
         color = color2;
      }

      point1.set(xValue + middleOfMap, 0.0f, -gridMax + middleOfMap);
      point2.set(xValue + middleOfMap, 0.0f, gridMax + middleOfMap);


      gpDebugPrimitives->addDebugLine(point1, point2, color);
   }

   for (z = 0; z < numLines; z++)
   {
      float zValue = z * cGridIncrement - gridMax;
      uint color;

      if (z == ((numLines - 1) / 2))
      {
         color = color1;
      }
      else
      {
         color = color2;
      }

      point1.set(-gridMax + middleOfMap, 0.0f, zValue + middleOfMap);
      point2.set(gridMax + middleOfMap, 0.0f, zValue + middleOfMap);

      gpDebugPrimitives->addDebugLine(point1, point2, color);
   }
}


//==============================================================================
// BModeViewer::renderViewerUI
//==============================================================================
void BModeViewer::renderViewerUI()
{
#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableUI))
#endif   
   {
      const long cMenuViewRange = 1;
      const float cRowSpacing = 26.0f;

      const float cMarginX = 70.0f;
      const float cMarginY = 40;


      if(mShowDebugInfo)
      {
         BHandle fontHandle;
         fontHandle=gFontManager.getFontDenmark18();
         gFontManager.setFont(fontHandle);

         BSimString text;

         float y = cMarginY + (cMenuViewRange * cRowSpacing);
         float x = cMarginX;

         gFontManager.drawText(fontHandle, x, y, "Model: ", cDWORDGreen);

         x = 170.0f;

         long numModels = mModelList.getSize();
         long startIndex = Math::Clamp(mSelectedModel - cMenuViewRange, (long) 0, numModels - 1);
         long endIndex = Math::Clamp(mSelectedModel + cMenuViewRange, (long) 0, numModels - 1);

         for(long i = startIndex; i <= endIndex; i++)
         {
            float alpha = 1.0f - (Math::fAbs((float)mSelectedModel - i) * 0.45f);
            float temp_y = y + (cRowSpacing * (i - mSelectedModel));

            if(alpha > 0.0f)
            {
               gFontManager.drawText(fontHandle, x, temp_y, mModelList.get(i).getPtr(), cDWORDGreen);
            }
         }


         long numAnims = mAnimList.getSize();
         if(numAnims > 0)
         {
            // Display state and speed settings
            //

            x = cMarginX;
            y = 720 - cMarginY - cRowSpacing;


            BSimString stateText;
            if(getPaused())
               stateText.format("Paused");
            else
               stateText.format("Playing");


            BSimString motionExtractionText;
            if(mIsMotionExtractionEnabled)
               motionExtractionText.format("On");
            else
               motionExtractionText.format("Off");

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
            
            BSimString backgroundModeText;
            switch(mBackgroundMode)
            {
               case cBackgroundModeGrid:
                  backgroundModeText.format("Grid");
                  break;
               case cBackgroundModeTerrain:
                  backgroundModeText.format("Terrain");
                  break;
               case cBackgroundModeEmpty:
                  backgroundModeText.format("Empty");
                  break;
            }



            text.format("State:  %s - Motion Extraction:  %s - Playback Speed:  %s - Background:  %s", stateText.getPtr(), motionExtractionText.getPtr(), speedText.getPtr(), backgroundModeText.getPtr());
            gFontManager.drawText(fontHandle, x, y, text.getPtr(), cDWORDGreen);


            // Display animation clock
            //

            if(mpUnit && mpUnit->getVisual())
            {
               BVisual *pVisual = mpUnit->getVisual();

               float animDuration = pVisual->getAnimationDuration(0);
               float animPosition = pVisual->getAnimationPosition(0);


               x = cMarginX;
               y -= cRowSpacing;

               text.format("Anim Duration:  %1.2f - Anim Time:  %1.2f", animDuration, animPosition);
               gFontManager.drawText(fontHandle, x, y, text.getPtr(), cDWORDGreen);
            }


            // Display animation name
            //

            x = cMarginX;
            y -= cRowSpacing + (cMenuViewRange * cRowSpacing);

            gFontManager.drawText(fontHandle, x, y, "Anim: ", cDWORDGreen);

            x = 150.0f;

            long numAnims = mAnimList.getSize();
            startIndex = Math::Clamp(mSelectedAnim - cMenuViewRange, (long) 0, numAnims - 1);
            endIndex = Math::Clamp(mSelectedAnim + cMenuViewRange, (long) 0, numAnims - 1);

            for(long i = startIndex; i <= endIndex; i++)
            {
               float alpha = 1.0f - (Math::fAbs((float)mSelectedAnim - i) * 0.60f);
               float temp_y = y + (cRowSpacing * (i - mSelectedAnim));

               if(alpha > 0.0f)
               {
                  text.format("(%s) %s", gVisualManager.getAnimName(mAnimList[i].mAnimType), mAnimList[i].mName.getPtr());
                  gFontManager.drawText(fontHandle, x, temp_y, text.getPtr(), cDWORDGreen);
               }
            }
         }
      }
   }

   renderUIShowStats();

   //-- we need to call this to clear out the gpu frame heap allocations.
   gUIManager->releaseGPUHeapResources();
}

//==============================================================================
// BModeViewer::renderUIShowStats
//==============================================================================
void BModeViewer::renderUIShowStats(void)
{
#ifndef BUILD_FINAL
   BDisplayStats::show(mpWorld, mpTimingTracker, NULL);
#endif
}

//==============================================================================
// BModeViewer::handleInput
//==============================================================================
bool BModeViewer::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (gRenderControl.handleInput(port, event, controlType, detail))
      return true;

#ifndef BUILD_FINAL   
   if (BDisplayStats::handleInput(port, event, controlType, detail))
      return true;
#endif   

   if(event==cInputEventControlStart)
   {
      if(controlType==cButtonStart)
      {
         gModeManager.getModeMenu()->setNextState(BModeMenu::cStateModelView);
         gModeManager.setMode(BModeManager::cModeMenu);
         return true;
      }
   }

   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);
   
   if(start)
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

         case cButtonBack:
            {
               gUI.playClickSound();
               mIsMotionExtractionEnabled = !mIsMotionExtractionEnabled;
               resetObjectPosition();

               return true;
            }         

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
               initCamera();
               return true;
            }       
         case cButtonX:
         case cKeySpace:
            {
               gUI.playClickSound();
               mShowDebugInfo = !mShowDebugInfo;
               return true;
            }
         case cButtonY:
            {
               gUI.playClickSound();
               incrementBackgroundMode();
               return true;
            }         
          

         case cTriggerRight:
            {
               gUI.playClickSound();
               incrementSelectedModel();
               return true;
            }     

         case cTriggerLeft:
            {
               gUI.playClickSound();
               decrementSelectedModel();
               return true;
            }   

         case cButtonShoulderRight:
            {
               gUI.playClickSound();
               incrementSelectedAnim();
               return true;
            }     

         case cButtonShoulderLeft:
            {
               gUI.playClickSound();
               decrementSelectedAnim();
               return true;
            }
      }


      // Don't use arrows keys if the console is active
      if (!gConfig.isDefined(cConfigConsoleRenderEnable))
      {
         switch(controlType)
         {
            case cKeyRight:
               {
                  incrementSelectedAnim();
                  return true;
               }     

            case cKeyLeft:
               {
                  decrementSelectedAnim();
                  return true;
               }

            case cKeyUp:            
               {
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
               if (!gConfig.isDefined(cConfigConsoleRenderEnable))
               {
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
         }
      }
   }

   if(start || repeat || stop)
   {
      switch(controlType)
      {      
         case cStickLeft:
            if(stop)
            {
               mViewerYaw=0.0f;
               mViewerPitch=0.0f;
            }
            else
            {
               if(detail.mX<-0.1f)
                  mViewerYaw=-(detail.mX+0.1f)/0.9f;
               else if(detail.mX>0.0f)
                  mViewerYaw=-(detail.mX-0.1f)/0.9f;
               else
                  mViewerYaw=0.0f;

               if(detail.mY<-0.1f)
                  mViewerPitch=-(detail.mY+0.1f)/0.9f;
               else if(detail.mY>0.1f)
                  mViewerPitch=-(detail.mY-0.1f)/0.9f;
               else
                  mViewerPitch=0.0f;
            }
            return true;

         case cStickRight:
            mViewerPanX=0.0f;
            mViewerPanY=0.0f;
            if(stop)
               mViewerZoom=0.0f;
            else
            {
               if(detail.mY<-0.1f)
                  mViewerZoom=-(detail.mY+0.1f)/0.9f;
               else if(detail.mY>0.1f)
                  mViewerZoom=-(detail.mY-0.1f)/0.9f;
               else
                  mViewerZoom=0.0f;
            }
            return true;

         case cDpad:
            mViewerZoom=0.0f;
            if(stop)
            {
               mViewerPanX=0.0f;
               mViewerPanY=0.0f;
            }
            else
            {
               if(detail.mX<-0.1f)
                  mViewerPanX=(detail.mX+0.1f)/0.9f;
               else if(detail.mX>0.1f)
                  mViewerPanX=(detail.mX-0.1f)/0.9f;
               else
                  mViewerPanX=0.0f;

               if(detail.mY<-0.1f)
                  mViewerPanY=-(detail.mY+0.1f)/0.9f;
               else if(detail.mY>0.1f)
                  mViewerPanY=-(detail.mY-0.1f)/0.9f;
               else
                  mViewerPanY=0.0f;
            }
            return true;      

      }
   }


   /*
   if(controlType==cKeyTab || controlType == cDpadRight || controlType == cStickLeftRight || controlType == cTriggerRight)
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
// BModeViewer::calcUpdateTimes
//==============================================================================
bool BModeViewer::calcUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength)
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

   if(mpWorld->getUpdateNumber()==0)
      return true;


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
         currentUpdateLength*=gameSpeed;

      DWORD len = (DWORD)(currentUpdateLength * mTimerFrequencyFloat);
      currentUpdateTime=mpWorld->getLastUpdateTime()+len;
   }

   return true;
}


//==============================================================================
// BModeViewer::initModel()
//==============================================================================
void BModeViewer::initModel()
{
   // Reset state
   //

   // kill original model
   if(mpSquad)
   {
      mpSquad->kill(true);
      
      mpObject = NULL;
      mpUnit = NULL;
      mpSquad = NULL;
   }
   else if(mpUnit)
   {
      mpUnit->kill(true);

      mpObject = NULL;
      mpUnit = NULL;
   }
   else if(mpObject)
   {
      mpObject->kill(true);

      mpObject = NULL;
   }

   // Release all other units
   BEntityHandle handle=cInvalidObjectID;
   for(BUnit* pUnit=gWorld->getNextUnit(handle); pUnit!=NULL; pUnit=gWorld->getNextUnit(handle))
   {
      pUnit->kill(true);
   }

   mSelectedAnim = 0;

   mObjectCenter.set(0, 10, 0);
   mObjectSize.set(10, 20, 10);
   mBoundingBoxMaxSide = cDefaultBoundingBoxMaxSide;

   mAnimList.clear();




   // Create unit
   //

   float middleOfMap = gTerrainVisual.getTileScale() * gTerrainVisual.getNumXVerts() / 2.0f;

   BVector objPosition;
   objPosition.set(middleOfMap, 0.0f, middleOfMap);

   long protoID = gDatabase.getProtoObject(mModelList.get(mSelectedModel).getPtr());

   BEntityID entityId = mpWorld->createEntity(protoID, false, 1, objPosition, cZAxisVector, cXAxisVector, true, false, true);
//-- FIXING PREFIX BUG ID 4875
   const BEntity* entity = gWorld->getEntity(entityId);
//--

   long entityType = entity->getClassType();
   switch(entityType)
   {
      case BEntity::cClassTypeObject:
         mpObject = gWorld->getObject(entityId);
         break;

      case BEntity::cClassTypeUnit:
         mpUnit = gWorld->getUnit(entityId);
         mpObject = mpUnit->getObject();
         break;

      case BEntity::cClassTypeSquad:
         mpSquad = gWorld->getSquad(entityId);
         
         if (mpSquad && mpSquad->getNumberChildren() >= 1)
         {
            mpUnit = gWorld->getUnit(mpSquad->getLeader());
            if(mpUnit)
            {
               mpObject = mpUnit->getObject();
            }

               
            // disable leashing
            mpSquad->setFlagIgnoreLeash(true);
         }
         break;

      case BEntity::cClassTypeDopple:
         break;
      case BEntity::cClassTypeProjectile:
         break;
      case BEntity::cClassTypeArmy:
         break;
   }

   if (mpObject)
   {
      BVisual *pVisual = mpObject->getVisual();

      if(pVisual)
      {
         BProtoVisual* pProtoVisual = pVisual->getProtoVisual();

         if(pProtoVisual)
         {
            mCurrentProtoVisualGen = pProtoVisual->getGeneration();

            // get animation list
            long numModels = pProtoVisual->mModels.getSize();
            //for(long i = 0; i < numModels; i++)
            if(numModels >= 1)
            {
               BProtoVisualModel *pVisualModel = pProtoVisual->mModels[0];

               long numAnimTypes = pVisualModel->mAnims.getSize();
               for(long j = 0; j < numAnimTypes; j++)
               {
                  BProtoVisualAnim *pVisualAnim = &pVisualModel->mAnims[j];

                  long numAnimAssets = pVisualAnim->mAssets.getSize();
                  for(long k = 0; k < numAnimAssets; k++)
                  {
                     BProtoVisualAsset *pVisualAsset = &pVisualAnim->mAssets[k];

                     BAnimationInfo info;
                     info.mName = pVisualAsset->mAssetName;
                     info.mAnimType = pVisualAnim->mAnimType;
                     info.mAnimIndex = k;//pVisualAsset->mAssetIndex;

                     if(pVisualAsset->mAssetIndex != -1)
                        mAnimList.pushBack(info);
                  }
               }
            }

            // get bounding volume
            BVector minCorner = pVisual->getMinCorner();
            BVector maxCorner = pVisual->getMaxCorner();

            if(!minCorner.almostEqual(maxCorner))
            {
               mObjectCenter = (maxCorner + minCorner) / 2.0f;
               mObjectSize = (maxCorner - mObjectCenter) * 2.0f;

               mBoundingBoxMaxSide = Math::Max(mObjectSize.x, mObjectSize.y);
               mBoundingBoxMaxSide = Math::Max(mBoundingBoxMaxSide, mObjectSize.z);

               if(Math::EqualRelTol(mBoundingBoxMaxSide, 0.0f))
               {
                  mBoundingBoxMaxSide = cDefaultBoundingBoxMaxSide;
               }
            }
         }
      }
   }
}


//==============================================================================
// BModeViewer::initCamera()
//==============================================================================
void BModeViewer::initCamera()
{
   // Position camera
   //
   float middleOfMap = gTerrainVisual.getTileScale() * gTerrainVisual.getNumXVerts() / 2.0f;

   BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
   mLookAtPosition.set( middleOfMap + mObjectCenter.x, mObjectCenter.y, middleOfMap + mObjectCenter.z);


   if(Math::EqualRelTol(mBoundingBoxMaxSide, 0.0f))
   {
      mBoundingBoxMaxSide = cDefaultBoundingBoxMaxSide;
   }

   float cameraDistanceXZ = mBoundingBoxMaxSide * 2.0f;
   float cameraDistanceY = mBoundingBoxMaxSide * 1.0f;

   BVector position;
   position.set(mLookAtPosition.x+cameraDistanceXZ, mLookAtPosition.y+cameraDistanceY, mLookAtPosition.z+cameraDistanceXZ);
   camera->setCameraLoc(position);

   BVector forward=mLookAtPosition-position;

   mViewerDistance=forward.length();

   forward.normalize();

   if(Math::EqualRelTol(Math::fAbs(forward.dot(cYAxisVector)), 1.0f))
   {
      mBoundingBoxMaxSide = cDefaultBoundingBoxMaxSide;
   }
   camera->setCameraDir(forward);
   camera->setCameraUp(cYAxisVector);
   camera->calcCameraRight();

   BVector up = camera->getCameraDir().cross(camera->getCameraRight());
   up.normalize();
   camera->setCameraUp(up);
}



//==============================================================================
// BModeViewer::incrementSelectedModel
//==============================================================================
void BModeViewer::incrementSelectedModel()
{
   mSelectedModel++;

   if(mSelectedModel >= (long) mModelList.getSize())
   {
      mSelectedModel = 0;
   }

   // update model
   initModel();
   initCamera();
}


//==============================================================================
// BModeViewer::decrementSelectedModel
//==============================================================================
void BModeViewer::decrementSelectedModel()
{
   mSelectedModel--;

   if(mSelectedModel < 0)
   {
      mSelectedModel = mModelList.getSize() - 1;
   }

   // update model
   initModel();
   initCamera();
}


//==============================================================================
// BModeViewer::playSelectedAnimation
//==============================================================================
void BModeViewer::playSelectedAnimation()
{
   if((mpUnit == NULL) || (mAnimList.getSize() == 0))
      return;

   long animType = mAnimList[mSelectedAnim].mAnimType;
   long animIndex = mAnimList[mSelectedAnim].mAnimIndex;

   BProtoVisualAnimExitAction exitAction;
   exitAction.mExitAction = cAnimExitActionLoop;
   exitAction.mTweenToAnimation = -1;
   exitAction.mTweenTime = 0.0f;

   mpUnit->beginPlayBlockingAnimation(BObjectAnimationState::cAnimationStateMisc, animType, 
                                                   true,       // applyInstantly
                                                   false,      // useMaxHeight
                                                   true,       // forceReset
                                                   (~animIndex) << 16, // forceAnim
                                                   &exitAction);       // exit action

   resetObjectPosition();
}


//==============================================================================
// BModeViewer::incrementSelectedAnim
//==============================================================================
void BModeViewer::incrementSelectedAnim()
{
   mSelectedAnim++;

   if(mSelectedAnim >= (long) mAnimList.getSize())
   {
      mSelectedAnim = 0;
   }

   // update animation
   playSelectedAnimation();
}


//==============================================================================
// BModeViewer::decrementSelectedAnim
//==============================================================================
void BModeViewer::decrementSelectedAnim()
{
   mSelectedAnim--;

   if(mSelectedAnim < 0)
   {
      mSelectedAnim = mAnimList.getSize() - 1;
   }

   // update animation
   playSelectedAnimation();
}

//==============================================================================
// BModeViewer::incrementBackgroundMode
//==============================================================================
void BModeViewer::incrementBackgroundMode()
{
   mBackgroundMode++;

   if(mBackgroundMode >= cBackgroundModeMax)
      mBackgroundMode = 0;

   switch(mBackgroundMode)
   {
      case cBackgroundModeGrid:
         // No terrain
         gRenderControl.setFlag(BRenderControl::cFlagDisableRenderTerrain, true);
         break;

      case cBackgroundModeTerrain:
         // Yes terrain
         gRenderControl.setFlag(BRenderControl::cFlagDisableRenderTerrain, false);
         break;

      case cBackgroundModeEmpty:
         // No terrain
         gRenderControl.setFlag(BRenderControl::cFlagDisableRenderTerrain, true);
         break;
   }
}


//==============================================================================
// BModeViewer::resetObjectPosition
//==============================================================================
void BModeViewer::resetObjectPosition()
{
   if(mpObject)
   {
      float middleOfMap = gTerrainVisual.getTileScale() * gTerrainVisual.getNumXVerts() / 2.0f;

      BVector objPosition;
      objPosition.set(middleOfMap, 0.0f, middleOfMap);

      mpObject->setPosition(objPosition);
      mpObject->setForward(cZAxisVector);
      mpObject->setUp(cYAxisVector);
      mpObject->calcRight();
   }
}