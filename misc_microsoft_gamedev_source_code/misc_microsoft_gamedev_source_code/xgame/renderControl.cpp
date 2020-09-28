//============================================================================
//
//  File: renderControl.cpp
//
//  Copyright (c) 2006-2008 Ensemble Studios
//
//============================================================================
#include "common.h"
#include "renderControl.h"
#include "world.h"
#include "userManager.h"
#include "user.h"
#include "workdirsetup.h"
#include "gameDirectories.h"
#include "debugConnection.h"

#include "configsgame.h"
#include "modemanager.h"
#include "modegame.h"
#include "scenario.h"
#include "game.h"
#include "LiveSystem.h"

#include "threading\workDistributor.h"
#include "xcorelib.h"

#include "grannymanager.h"
#include "meshEffectTextures.h"

// xgameRender
#include "viewportManager.h"
#include "debugprimitives.h"
#include "minimap.h"
#include "render.h"
#include "dynamicGPUBuffer.h"
#include "dirShadowManager.h"
#include "localShadowManager.h"
#include "worldVisibility.h"
#include "HPBar.h"
#include "tiledAA.h"
#include "toneMapManager.h"
#include "camera.h"
#include "ugxGeomInstanceManager.h"
#include "ugxGeomUberSectionRenderer.h"
#include "ugxGeomRender.h"
#include "FontSystem2.h"
#include "cubemapGen.h"
#include "gpuHeap.h"
#include "decalManager.h"
#include "renderTime.h"
#include "configsgamerender.h"
#include "waterManager.h"
#include "occlusion.h"
#include "primDraw2D.h"
#include "effectIntrinsicManager.h"
#include "DCBManager.h"
#include "renderHelperThread.h"
#include "flashRender.h"
#include "packedTextureManager.h"
#include "terrainimpactdecal.h"
#include "gpuDXTVideo.h"

// terrain
#include "terrain.h"
#include "TerrainTexturing.h"
#include "TerrainMetric.h"
#include "TerrainHeightField.h"
#include "TerrainVisual.h"
#include "TerrainFoliage.h"
#include "TerrainDynamicAlpha.h"
#include "TerrainDeformer.h"
#include "TerrainLitDecals.h"
#include "TerrainRibbon.h"

// xgranny
#include "grannyInstanceRenderer.h"

// xparticles
#include "configsparticles.h"
#include "particlesystemmanager.h"
#include "particlegateway.h"

// flash
#include "flashmanager.h"
#include "uimanager.h"

// xinput
#include "keyboard.h"

#include "terraineffectmanager.h"

// Maximum height of shadow casting buildings/units relative to the terrain.
// FIXME: This should be derived from the scene's static objects!
//const float cWorldBoundingBoxExpansionDist = 100.0f;
const float cWorldBoundingBoxExpansionDist = 10.0f;
const float cMaxBuildingUnitHeight = 160.0f;  

const float cDefaultNearClipPlane = 0.25f;
const float cDefaultFarClipPlane = 2500.0f;

BRenderControl gRenderControl;

#include "threading\win32WaitableTimer.h"

//--------------------------------------------------------------------------------------
// BRenderControl::BRenderControl
//--------------------------------------------------------------------------------------
BRenderControl::BRenderControl() :
   BEventReceiver(),
   mSimVisMode(cVMDisabled),
   mSimEventHandle(cInvalidEventReceiverHandle),
   mSimGenerateCubemapFaceIndex(0),
   mSimGenerateCubemapFlag(false),
   mpRenderCubemapGen(NULL),
   mSimGenerateCubemapDim(512),
   mSimGenerateCubemapWasUIEnabled(false),
   mSimGenerateCubemapSH(false),
   mSimHDRScreenshotFlag(false),
   mRenderTerrainHeightfield(false),
   mSimNewScenarioFlag(false),
   mSimGenerateHeightfield(false),
   mSimPrevRenderTime(-1.0f),
   mSimNearClipPlane(cDefaultNearClipPlane),
   mSimFarClipPlane(cDefaultFarClipPlane),
   mSimGameTime(0.0f),
   mSimShadowMode(BDirShadowManager::cSMFPS),
   mRenderMeshInstancesForkEventChecked(false),
   mSimMinimapScreenshotFlag(false),
   mSimMinimapScreenshotOldRenderSkirtState(false),
   mSimMinimapScreenshotOldCameraState(false),
   mSimMinimapScreenshotWasUIEnabledFlag(false),
   mpSimWorld(NULL),
   mDoWorkerRender(true),
   mNextWorkerRenderState(true)
{
   Utils::ClearObj(mSimActualTime);

#ifndef BUILD_FINAL   
   mPrevFrameSysTime = 0;
   Utils::ClearObj(mPrevThreadKernelTimes);
   mRenderUpdateBarChartHandle = cInvalidBarChartBarHandle;
   mRenderDrawBarChartHandle = cInvalidBarChartBarHandle;

   mpRenderGPUDXTVideo = NULL;
   mGPUDXTVideoAutoConvert = false;
#endif   
}

//--------------------------------------------------------------------------------------
// BRenderControl::~BRenderControl
//--------------------------------------------------------------------------------------
BRenderControl::~BRenderControl()
{
}

//--------------------------------------------------------------------------------------
// BRenderControl::init
//--------------------------------------------------------------------------------------
void BRenderControl::init(void)
{  
   ASSERT_THREAD(cThreadIndexSim);
   
   commandListenerInit();
   eventReceiverInit(cThreadIndexRender);
   
   if (mSimEventHandle == cInvalidEventReceiverHandle)
      mSimEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);

   Utils::ClearObj(mSimActualTime);
   mpSimWorld = NULL;
   
   mSimGenerateCubemapPos.clear();
   mSimGenerateCubemapFaceIndex = 0;
   mSimGenerateCubemapFlag = false;
   mpRenderCubemapGen = NULL;
   mSimGenerateCubemapWasUIEnabled = false;

#ifndef BUILD_FINAL   
   mTotalGPUPacksAverage.set(60);
   mTotalGPUPackPixelsAverage.set(60);
#endif   

   mSimFLSSaveAsFileName.empty();
   mSimMegaScreenshotFilename.empty();
   mSimMegaScreenshotRows = 0;
   mSimMegaScreenshotCols = 0; 
   mSimMegaScreenshotFlag = false;
   mSimMegaScreenshotWasUIEnabledFlag = false;
   mSimMegaScreenshotOrigJPEGScreenshotFlag = false;
   mSimMegaScreenshotWasPausedFlag = false;
   mSimCreatingMegaScreenshotFlag = false;
   mSimMegaScreenshotUseJitter = false;
   mSimMegaScreenshotJitterAAQuality = 1;
   mpSimMegaScreenshotAAOffsets = NULL;
   mpSimMegaScreenshotOffsets = NULL;
   mSimMacroTileIndex = 0;
   mSimNumMacroTiles = 1;
   mSimGameTime = 0.0f;

   mSimNearClipPlane = cDefaultNearClipPlane;
   mSimFarClipPlane = cDefaultFarClipPlane;

   mSimMinimapScreenshotFlag=false;
   mSimMinimapScreenshotOldRenderSkirtState = false;
   mSimMinimapScreenshotOldCameraState = false;
   mSimMinimapScreenshotWasUIEnabledFlag = false;
   mSimMinimapFilename.empty();

#ifndef BUILD_FINAL   
   if (gConfig.isDefined(cConfigDisableShadowRendering))
      setFlag(cFlagDisableShadowRendering, true);
#endif      

#ifndef BUILD_FINAL   
   mPrevFrameSysTime = 0;
   Utils::ClearObj(mPrevThreadKernelTimes);


   BBarChart::BInitParams barParms;
   barParms.mHeight = 80;
   barParms.mMaxValue = 30;
   barParms.mLocX = 85;
   barParms.mLocY = 600;
   barParms.mRenderOption = BBarChart::eRenderOnlyIfViolators;

   mBarChart.init(cDirFonts,barParms);

   if(mRenderUpdateBarChartHandle == cInvalidBarChartBarHandle)
      mRenderUpdateBarChartHandle = gRenderControl.getBarChart()->addBar(L"Render Update Time(ms)",0xFF881155);

   if(mRenderDrawBarChartHandle == cInvalidBarChartBarHandle)
      mRenderDrawBarChartHandle = gRenderControl.getBarChart()->addBar(L"Render Draw Time(ms)",0xFF118855);

   BBarChart::BInitParams barParms2;
   barParms2.mHeight = 120;
   barParms2.mMaxValue = 2;
   barParms2.mLocX = 85;
   barParms2.mLocY = 450;
   barParms2.mRenderOption = BBarChart::eRenderOnlyIfFirstViolator;

   mBarChartMem.init(cDirFonts,barParms2);

#endif   

   mDoWorkerRender=true;
   mNextWorkerRenderState=true;

   BMeshEffectTextures::getInstance().init();
}

//--------------------------------------------------------------------------------------
// BRenderControl::deinit
//--------------------------------------------------------------------------------------
void BRenderControl::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   gRenderThread.blockUntilGPUIdle();

   eventReceiverDeinit();
   commandListenerDeinit();
   
   BMeshEffectTextures::getInstance().deinit();

   if (mSimEventHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mSimEventHandle, true);
      mSimEventHandle = cInvalidEventReceiverHandle;
   }

   if (mpSimMegaScreenshotAAOffsets)
   {
      delete [] mpSimMegaScreenshotAAOffsets;
      mpSimMegaScreenshotAAOffsets = 0;
   }
   
   if (mpSimMegaScreenshotOffsets)
   {
      delete [] mpSimMegaScreenshotOffsets;
      mpSimMegaScreenshotOffsets = 0;
   }
#ifndef BUILD_FINAL   
   mBarChart.deinit();
   mBarChartMem.deinit();
   
   mRenderUpdateBarChartHandle = cInvalidBarChartBarHandle;
   mRenderDrawBarChartHandle = cInvalidBarChartBarHandle;
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::renderBeginMegascreenshot
//--------------------------------------------------------------------------------------
void BRenderControl::renderBeginMegascreenshot(void* pData)
{
   gTerrainTexturing.setLODEnabled(false);
   gTerrainVisual.setLODEnabled(false);
   gPSManager.setFlagEnableCulling(false);
}

//--------------------------------------------------------------------------------------
// BRenderControl::renderEndMegascreenshot
//--------------------------------------------------------------------------------------
void BRenderControl::renderEndMegascreenshot(void* pData)
{
   gTerrainTexturing.setLODEnabled(true);
   gTerrainVisual.setLODEnabled(true);
   gPSManager.setFlagEnableCulling(true);
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateTime
//--------------------------------------------------------------------------------------
void BRenderControl::updateTime(BWorld* pWorld)
{
   uint64 freq;
   uint64 curTime;
   QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
   QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
   const double invPerfCounterFreq = 1.0f / freq;

   const double curRenderRealtime = curTime * invPerfCounterFreq;



   if ((mSimNewScenarioFlag) || (mSimPrevRenderTime < 0.0f))
      mSimPrevRenderTime = curRenderRealtime;

   const double deltaTRealtime = Math::Clamp<double>(curRenderRealtime - mSimPrevRenderTime, 0.0f, 2.0f);

   mSimPrevRenderTime = curRenderRealtime;

   //const float deltaT         = pWorld ? pWorld->getLastUpdateLengthFloat() : 0.0f;
   const double gameRealTime  = pWorld ? pWorld->getSubTotalRealtime() : 0.0f;
   const double gameTime      = pWorld ? pWorld->getSubGametimeFloat() : 0.0f;

#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigSubUpdTrace))
      trace("totalRealTime=%10.4f, subRealTime=%10.4f, gameTime=%10.4f, subGameTime=%10.4f, update=%u, sub=%u", gWorld->getTotalRealtime(), gameRealTime, gWorld->getGametimeFloat(), gameTime, gWorld->getUpdateNumber(), gWorld->getSubUpdate());
#endif

   gRenderTime.simUpdate(
      pWorld ? pWorld->getUpdateNumber() : 0,
      gModeManager.getModeGame()->getPaused(),
      pWorld ? pWorld->getLastSubUpdateRealtime() * invPerfCounterFreq : 0.0f,
      curRenderRealtime, 
      deltaTRealtime,
      pWorld ? pWorld->getLastSubUpdateLengthFloat() : 0.0f, 
      gameRealTime,
      gameTime);
      
   GetSystemTimeAsFileTime(&mSimActualTime);      
}

//--------------------------------------------------------------------------------------
// BRenderControl::overrideUserCamera
//--------------------------------------------------------------------------------------
void BRenderControl::overrideUserCamera(BUser* pUser)
{
   BCamera* pUserCamera = pUser->getCamera();

   BMatrix userCameraMatrix;
   pUserCamera->getViewMatrix(userCameraMatrix);

   float cameraFOV = pUserCamera->getFOV();
   BVector cameraLoc(pUserCamera->getCameraLoc()); 

   if (mSimGenerateCubemapFlag)
   {
      if (mSimGenerateCubemapFaceIndex == 0)
      {
         mSimGenerateCubemapPos = BVec4(cameraLoc.x, cameraLoc.y, cameraLoc.z, 1.0f);

         mSimGenerateCubemapWasUIEnabled = gConfig.isDefined(cConfigDisableUI);
         gConfig.define(cConfigDisableUI);

         gModeManager.getModeGame()->setPaused(true);
      }

      BVec4 camDir = BVec4::makeAxisVector(mSimGenerateCubemapFaceIndex >> 1, (mSimGenerateCubemapFaceIndex & 1) ? -1.0f : 1.0f);

      float roll = 0.0f;
      switch (mSimGenerateCubemapFaceIndex)
      {
         case D3DCUBEMAP_FACE_NEGATIVE_Y:
         case D3DCUBEMAP_FACE_POSITIVE_Y:
         {
            roll = Math::fDegToRad(-90.0f);
            break;
         }
         case D3DCUBEMAP_FACE_NEGATIVE_Z:
         {
            roll = Math::fDegToRad(-180.0f);
            break;
         }
      }

      const BMatrix44 worldToView(BMatrix44::makeCamera(mSimGenerateCubemapPos, mSimGenerateCubemapPos + camDir, BVec4(0, 1, 0, 0), roll));

      BRenderViewParams viewParams;
      viewParams.setViewMatrix(worldToView.getPtr());

      float l = 0.0f;
      float r = 1.0f;
      float t = 0.0f;
      float b = 1.0f;

      viewParams.setOffCenterFactor(l, t, r, b);

      const float fov = Math::fDegToRad(90.0f);

      viewParams.setViewportAndProjection(0, 0, 512, 512, mSimNearClipPlane, mSimFarClipPlane, fov);

      gViewportManager.setViewportViewParams(0, viewParams);
   }
   else if (mSimCreatingMegaScreenshotFlag)
   {
      BRenderViewParams viewParams;
      viewParams.setViewMatrix(userCameraMatrix);

      float l = 0.0f;
      float r = 1.0f;
      float t = 0.0f;
      float b = 1.0f;

      const bool mSimMegaScreenshotDoJitterAASamples = false;

      if (mSimMacroTileIndex == 0)
      {
         mSimMegaScreenshotWasPausedFlag = gModeManager.getModeGame()->getPaused();
         gModeManager.getModeGame()->setPaused(true);

         mSimMegaScreenshotWasUIEnabledFlag = gConfig.isDefined(cConfigDisableUI);
         gConfig.define(cConfigDisableUI);

#ifndef BUILD_FINAL            
         mSimMegaScreenshotOrigJPEGScreenshotFlag = gConfig.isDefined(cConfigJPEGScreenshots);
         gConfig.remove(cConfigJPEGScreenshots);
#else
         mSimMegaScreenshotOrigJPEGScreenshotFlag = false;
#endif            

         gRenderThread.submitCallback(renderBeginMegascreenshot, NULL);
      }

      if (mSimMegaScreenshotUseJitter)
      {
         // In JITTER mode, mSimNumMacroTiles actually represents the number of per-pixel offset samples to take.
         if (!mpSimMegaScreenshotAAOffsets)
            calculateJitterPoints(mSimMegaScreenshotJitterAAQuality, mSimMegaScreenshotDoJitterAASamples);

         float xo = 0;
         float yo = 0;

         if (mSimMegaScreenshotJitterAAQuality > 1)
         {
            xo = mpSimMegaScreenshotAAOffsets[mSimMacroTileIndex][0];
            yo = mpSimMegaScreenshotAAOffsets[mSimMacroTileIndex][1];
         }
         else
         {
            xo = mpSimMegaScreenshotOffsets[mSimMacroTileIndex][0];
            yo = mpSimMegaScreenshotOffsets[mSimMacroTileIndex][1];
         }

         l = Math::Clamp<float>(xo,            0, 1);
         t = Math::Clamp<float>(1.0f - (1+yo), 0, 1);
         r = Math::Clamp<float>(1 + xo,        0, 1);
         b = Math::Clamp<float>(1.0f - (yo),   0, 1);
      }
      else
      {
         uint col = mSimMacroTileIndex % mSimMegaScreenshotCols;
         uint row = mSimMacroTileIndex / mSimMegaScreenshotCols;
         l = float(col) / float(mSimMegaScreenshotCols);
         t = 1.0f - float(row + 1) / float(mSimMegaScreenshotRows);
         r = float(col + 1) / float(mSimMegaScreenshotCols);
         b = 1.0f - float(row) / float(mSimMegaScreenshotRows);
      }

      viewParams.setOffCenterFactor(l, t, r, b);

      const D3DVIEWPORT9& sceneViewport = gViewportManager.getSceneViewport(0);
      viewParams.setViewportAndProjection(sceneViewport.X, sceneViewport.Y, sceneViewport.Width, sceneViewport.Height, mSimNearClipPlane, mSimFarClipPlane, cameraFOV);

      gViewportManager.setViewportViewParams(0, viewParams);
   }  
}

//--------------------------------------------------------------------------------------
// BRenderControl::sampleUserCameras
//--------------------------------------------------------------------------------------
void BRenderControl::sampleUserCameras(uint numUsers, BUser** ppUsers)
{
   BDEBUG_ASSERT(ppUsers);
   
   const uint numViewports = gViewportManager.getNumViewports();
   BDEBUG_ASSERT(Math::IsInRange<uint>(numViewports, 1, 2) && (numViewports == numUsers));
      
   if ((mSimMinimapScreenshotFlag) && (numViewports == 1))
   {
      float x = gTerrainVisual.getTileScale() * gTerrainVisual.getNumXVerts() * 0.5f;
      
      const float zoomScale = 250.0f;
      float zoom = zoomScale + (mSimMinimapScreenshotZoomLevel * zoomScale);      
      
      BVector cameraDirection(0,-1.0f,0);   
      BVector cameraUp(0.0f,0.0f,1.0f);
      BVector cameraPos(x,zoom,x);     
               
      BCamera* pUserCamera = ppUsers[0]->getCamera();
      pUserCamera->setCameraUp(cameraUp);
      pUserCamera->setCameraDir(cameraDirection);
      pUserCamera->setCameraLoc(cameraPos);
      pUserCamera->calcCameraRight();      
   }
   
   for (uint viewportIndex = 0; viewportIndex < numViewports; viewportIndex++)
   {
      BDEBUG_ASSERT(ppUsers[viewportIndex]);

      BUser* pUser = ppUsers[viewportIndex];
                  
      BCamera* pUserCamera = pUser->getCamera();
                  
      BMatrix userCameraMatrix;
      pUserCamera->getViewMatrix(userCameraMatrix);

      float cameraFOV = pUserCamera->getFOV();
            
      BRenderViewParams renderViewParams;
      renderViewParams.setViewMatrix(userCameraMatrix);
      renderViewParams.setViewportAndProjection(
         0, 0, 
         gViewportManager.getSceneViewport(viewportIndex).Width, 
         gViewportManager.getSceneViewport(viewportIndex).Height, 
         mSimNearClipPlane, mSimFarClipPlane, cameraFOV);
         
      gViewportManager.setViewportViewParams(viewportIndex, renderViewParams);
   }
   
   if (numViewports == 1)
      overrideUserCamera(ppUsers[0]);
}

//--------------------------------------------------------------------------------------
// BRenderControl::startOfFrame
//--------------------------------------------------------------------------------------
void BRenderControl::startOfFrame(BWorld* pWorld, uint numUsers, BUser** ppUsers, uint macroTileIndex)
{
   BDEBUG_ASSERT(macroTileIndex < mSimNumMacroTiles);
   
#ifndef BUILD_FINAL
   //set these values to 0 at the start of the full frame
   //we may call the subisquent leaf node functions multiple times (so update is additive)
   mBarChart.setBarValue(mRenderUpdateBarChartHandle,0);
   mBarChart.setBarValue(mRenderDrawBarChartHandle,0);
#endif
   
   mpSimWorld = pWorld;
   
   if (macroTileIndex == 0)
      mSimCreatingMegaScreenshotFlag = mSimMegaScreenshotFlag;
   
   mSimMacroTileIndex = static_cast<ushort>(macroTileIndex);
   
   sampleUserCameras(numUsers, ppUsers);

   if (macroTileIndex == 0)
      updateTime(pWorld);
      
   gRender.beginFrame(pWorld ? pWorld->getLastSubUpdateLengthFloat() : (1.0f / 30.0f));
   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerStartOfFrame), (void*)mSimGenerateHeightfield);

   mSimGenerateHeightfield = false;
}

//--------------------------------------------------------------------------------------
// BRenderControl::calculateJitterPoints
//--------------------------------------------------------------------------------------
void BRenderControl::calculateJitterPoints(uint AAQuality, bool jitterAASamples)
{
   mpSimMegaScreenshotAAOffsets = new BVec2[mSimMegaScreenshotCols * mSimMegaScreenshotRows * AAQuality];
   mpSimMegaScreenshotOffsets = new BVec2[mSimMegaScreenshotCols * mSimMegaScreenshotRows];

   const float sqrtAA = sqrt((float)AAQuality);
   const float iStride = 1.0f / (float)mSimMegaScreenshotRows;
   const float jStride = 1.0f / (float)mSimMegaScreenshotCols;
   const float iAAStride = iStride / sqrtAA;
   const float jAAStride = jStride / sqrtAA;

   int jCount=0;
   int rCount=0;
   for(uint i = 0; i <mSimMegaScreenshotRows; i++)
   {
      float iStart = i * iStride;
      for(uint j = 0; j < mSimMegaScreenshotCols; j++)
      {
         float jStart = j * jStride;

         // Offsets for NON ANTIALIASED screens (no jitter!)
         mpSimMegaScreenshotOffsets[rCount][1] = iStart / gRender.getWidth();
         mpSimMegaScreenshotOffsets[rCount][0] = jStart / gRender.getHeight();
         rCount++;

         for(uint q = 0; q < sqrtAA; q++)
         {
            float qStart = iStart + q * iAAStride;
            for(uint r = 0; r < sqrtAA; r++)
            {
               float rStart = jStart + r * jAAStride;
               if (jitterAASamples)
               {
                  mpSimMegaScreenshotAAOffsets[jCount][1] = getRandRangeFloat(cUnsyncedRand,qStart,qStart + iAAStride) / gRender.getWidth();
                  mpSimMegaScreenshotAAOffsets[jCount][0] = getRandRangeFloat(cUnsyncedRand,rStart,rStart + jAAStride) / gRender.getHeight();
               }
               else 
               {
                  mpSimMegaScreenshotAAOffsets[jCount][1] = qStart / gRender.getWidth();
                  mpSimMegaScreenshotAAOffsets[jCount][0] = rStart / gRender.getHeight();
               }
               
               jCount++;
            }
         }
      }
   }
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateSceneMetrics
//--------------------------------------------------------------------------------------
void BRenderControl::updateSceneMetrics(BVec3& worldMin, BVec3& worldMax)
{
   ASSERT_THREAD(cThreadIndexSim);

   // Can't shadow the skirt, it's just too big
   //worldMin.setFromPtr(&gTerrain.getMinTotal().x);
   //worldMax.setFromPtr(&gTerrain.getMaxTotal().x);
   worldMin.setFromPtr(&gTerrain.getMin().x);
   worldMax.setFromPtr(&gTerrain.getMax().x);

   worldMin[0] -= cWorldBoundingBoxExpansionDist;
   worldMin[2] -= cWorldBoundingBoxExpansionDist;

   worldMax[0] += cWorldBoundingBoxExpansionDist;
   worldMax[2] += cWorldBoundingBoxExpansionDist;

   // Increase the ceiling a bit to account for buildings and units.
   worldMax[1] += cMaxBuildingUnitHeight;

   // Drop the world bounds a bit to account for units below the terrain.
   worldMin[1] -= 25.0f;

   if (getFlag(cFlagDebugRenderBoundingBoxes))
      gpDebugPrimitives->addDebugBox(BVector(worldMin[0], worldMin[1], worldMin[2]), BVector(worldMax[0], worldMax[1], worldMax[2]), 0xFFFFFFFF);
}

//--------------------------------------------------------------------------------------
// BRenderControl::renderDebugBoundingBoxes
//--------------------------------------------------------------------------------------
void BRenderControl::renderDebugBoundingBoxes(BWorld* pWorld)
{
#if !defined( BUILD_FINAL )
   if( pWorld && getFlag( cFlagDebugRenderBoundingBoxes ) )
   {
      BEntityHandle handle = cInvalidObjectID;
      BUnit*        pUnit  = pWorld->getNextUnit( handle );
      while( pUnit )
      {      
         if (pUnit->getHitZoneList())
         {
            long hitZoneCount = pUnit->getHitZoneList()->getNumber();
            for( long i = 0; i < hitZoneCount; i++ )
            {
               BBoundingBox obb;
               if( pUnit->getHitZoneOBB( i, obb ) )
               {
                  BVector xA = obb.getAxes()[0]; 
                  BVector yA = obb.getAxes()[1];
                  BVector zA = obb.getAxes()[2];

                  BVector pos;
                  pos = obb.getCenter();

                  BVector ext = BVector( obb.getExtents()[0], obb.getExtents()[1], obb.getExtents()[2] );               

                  BMatrix mat;
                  mat.makeIdentity();
                  mat.r[0] = xA;
                  mat.r[1] = yA;
                  mat.r[2] = zA;
                  mat.setTranslation( pos );

                  gpDebugPrimitives->addDebugBox( mat, ext, D3DCOLOR_ARGB( 255, 0, 0, 255 ) );
                  //gpDebugPrimitives->addDebugSphere( pos, 1.0f, D3DCOLOR_ARGB( 255, 0, 255, 0 ) );            
               }
            }
         }

         pUnit = pWorld->getNextUnit( handle );
      }
   }
   
   if (getFlag(cFlagDebugRenderBoundingBoxes))
      gGrannyInstanceRenderer.debugRenderBoundingBoxes();
#endif    
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateUnits
//--------------------------------------------------------------------------------------
void BRenderControl::updateUnits(BUser* pUser, BWorld* pWorld, const BVec3& worldMin, const BVec3& worldMax)
{
   ASSERT_THREAD(cThreadIndexSim);
      
   SCOPEDSAMPLE(BRenderControl_updateUnits);
   // rg [5/25/07] - Hacking this for now to speed up anim sampling
   //mSimInterestVolume.update(gRenderDraw.getMainSceneMatrixTracker(), BVec3(&gSimSceneLightManager.getDirLight(cLCTerrain).mDir.x), cMaxLocalLightRadius, worldMin[1], worldMax[1]);   
   const float localLightRadius = 8.0f;
   mSimInterestVolume.update(gRenderDraw.getMainSceneMatrixTracker(), BVec3(&gSimSceneLightManager.getDirLight(cLCTerrain).mDir.x), localLightRadius, worldMin[1], worldMax[1]);   
   
   if (pWorld)
   {
      BVolumeCuller& activeVolumeCuller = gRenderDraw.getMainActiveVolumeCuller();
      activeVolumeCuller.disableBasePlanes();
      activeVolumeCuller.enableInclusionPlanes(mSimInterestVolume.getAllLightInterestVolume(), mSimInterestVolume.getNumInterestVolumePlanes());
      
      gRenderDraw.setMainActiveVolumeCuller(activeVolumeCuller);

      // Get ready to start queuing visualitems
      gVisualManager.beginRenderPrepare();
      
      gGrannyInstanceRenderer.begin();
      
      //trace("render vis instances time: %d", pWorld->getSubGametime());

      pWorld->renderVisualInstances(pUser);
      pUser->renderVisualInstances();
      
      // jce [11/3/2008] -- New cache functionality to allow deformation to work correctly in interpolated case.  To minimize code change, the existing render calls
      // will queue things into the visual manager which we now must process to do the actual rendering.
      gVisualManager.processVisualRenderQueue();
      
      gSimSceneLightManager.lockLights();   
      gSimSceneLightManager.updateRenderThreadState(&gRenderSceneLightManager);

      gGrannyInstanceRenderer.flushBegin();
      gGrannyInstanceRenderer.flushEnd();
      gGrannyInstanceRenderer.finish();

      // Done with queued visuals.
      gVisualManager.endRenderPrepare();

      activeVolumeCuller.disableInclusionPlanes();
      activeVolumeCuller.enableBasePlanes();
      gRenderDraw.setMainActiveVolumeCuller(activeVolumeCuller);
   }      
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateRenderThread
//--------------------------------------------------------------------------------------
void BRenderControl::updateRenderThread(
   uint viewportIndex, 
   float deltaT, 
   double gameRealTime, 
   double gameTime, 
   FILETIME actualTime, 
   const BVec3& worldMin, 
   const BVec3& worldMax,
   bool enableObscuredUnits, 
   uint numTeamColors, 
   const DWORD* pTeamColors)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BUpdateData* pUpdateData = reinterpret_cast<BUpdateData*>(gRenderThread.allocateFrameStorage(sizeof(BUpdateData)));

   pUpdateData->mDeltaT                   = deltaT;
   pUpdateData->mGameRealTime             = gameRealTime;
   pUpdateData->mGameTime                 = gameTime;
   pUpdateData->mActualTime               = actualTime;
   pUpdateData->mWorldMin                 = worldMin;
   pUpdateData->mWorldMax                 = worldMax;
   pUpdateData->mFlags                    = mSimFlags;
   pUpdateData->mVisMode                  = mSimVisMode;
   pUpdateData->mGenerateCubemapFlag      = mSimGenerateCubemapFlag;
   pUpdateData->mGenerateCubemapFaceIndex = mSimGenerateCubemapFaceIndex;
   pUpdateData->mGenerateCubemapDim       = mSimGenerateCubemapDim;
   pUpdateData->mGenerateCubemapSH        = mSimGenerateCubemapSH;
   pUpdateData->mHDRScreenshotFilename.set(mSimHDRScreenshotFilename);
   pUpdateData->mHDRScreenshotFlag        = mSimHDRScreenshotFlag;
   pUpdateData->mNewScenarioFlag          = mSimNewScenarioFlag;
   pUpdateData->mDisableAdaptation        = mSimGenerateCubemapFlag || mSimCreatingMegaScreenshotFlag || mSimMinimapScreenshotFlag;
   pUpdateData->mMegaScreenshotFlag       = mSimCreatingMegaScreenshotFlag;
   pUpdateData->mShadowMode               = mSimShadowMode;
   pUpdateData->mMinimapScreenShotFlag    = mSimMinimapScreenshotFlag;
      
   pUpdateData->mUserMode                 = mpSimUser ? mpSimUser->getUserMode() : 0;
   
   pUpdateData->mEnableObscuredUnits      = enableObscuredUnits;
   numTeamColors = Math::Min<uint>(numTeamColors, BUpdateData::cMaxTeamColors);
   pUpdateData->mNumTeamColors            = numTeamColors;
   if (numTeamColors)
   {
      BDEBUG_ASSERT(pTeamColors);
      memcpy(pUpdateData->mTeamColors, pTeamColors, numTeamColors * sizeof(DWORD));
   }
         
   mSimHDRScreenshotFilename.empty();
   mSimHDRScreenshotFlag = false;
   mSimNewScenarioFlag = false;
   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerBeginViewport), pUpdateData);
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateDecalManager
//--------------------------------------------------------------------------------------
void BRenderControl::updateDecalManager(double gameTime)
{
   gDecalManager.updateRenderThread(gameTime);

   if (getFlag(BRenderControl::cFlagRandomSplatTest))
   {

      BTerrainImpactDecalHandle handle;
      handle.mFadeOutTime=5;
      handle.mOrientation=0;
      handle.mSizeX=1.0f;
      handle.mSizeZ=1.0f;
      handle.mTimeFullyOpaque=5;
      handle.mImpactTextureName="decals\\artilary_scorch01";


      float maxT = gTerrainSimRep.getNumXDataTiles()*gTerrainSimRep.getDataTileScale();
      BVector bv = BVector(Math::fRand(0.0f, maxT), 0, Math::fRand(0.0f, maxT));

      gImpactDecalManager.createImpactDecal(bv, &handle, bv);
   }
   gLitDecalManager.updateRenderThread(gameTime);
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateUIManager
//--------------------------------------------------------------------------------------
void BRenderControl::updateUIManager(double gameTime)
{
   SCOPEDSAMPLE(BRenderControl_updateUIManager);
   gUIManager->updateRenderThread(gameTime);
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateShadowMode
//--------------------------------------------------------------------------------------
void BRenderControl::updateShadowMode(void)
{
   // Set shadowing mode.
   BDirShadowManager::BShadowMode shadowMode = BDirShadowManager::cSMFPS;

   if ((mpSimUser->getUserMode() != BUser::cUserModeCinematic))// && (!mpSimUser->getFlagNoCameraLimits()))
   {
      shadowMode = BDirShadowManager::cSMRTS;

      BCamera* pUserCamera = mpSimUser->getCamera();

      BMatrix userCameraMatrix;
      pUserCamera->getViewMatrix(userCameraMatrix);

      float y = fabs(userCameraMatrix._23);
      static float yThresh = .45f;
      if (y < yThresh)
         shadowMode = BDirShadowManager::cSMFPS;
   }

#ifndef BUILD_FINAL   
   if (gConfig.isDefined(cConfigHigherQualityShadows))
      shadowMode = BDirShadowManager::cSMHighestQuality;
#endif   

   gRenderControl.setShadowMode(shadowMode);
}

//--------------------------------------------------------------------------------------
// BRenderControl::update
//--------------------------------------------------------------------------------------
void BRenderControl::beginViewport(
   long userIndex,
   BUser* pUser,
   uint viewportIndex, 
   bool enableObscuredUnits, bool enableTeamColors)
{
   ASSERT_THREAD(cThreadIndexSim);
   BDEBUG_ASSERT(pUser && Math::IsInRange<uint>(userIndex, 0, 1));
   userIndex;
            
   SCOPEDSAMPLE(BRenderControl_update);

   // rg FIXME: This is a hack. We need to change the debug primitives class to at least be viewport aware.
   if ((mSimMacroTileIndex > 0) || (viewportIndex > 0))
   {
      gpDebugPrimitives->beginFrame(mpSimWorld->getGametimeFloat());
   }
   
   gViewportManager.setCurrentUser(userIndex);
   gViewportManager.setCurrentViewport(viewportIndex);

   // rg [9/18/06] - NEVER render anything inside here! D3D::BeginScene() has not been called yet!                  
   mpSimUser = pUser;
         
   const float deltaT         = mpSimWorld ? mpSimWorld->getLastSubUpdateLengthFloat() : 0.0f;
   const double gameRealTime  = mpSimWorld ? mpSimWorld->getSubTotalRealtime() : 0.0f;
   const double gameTime      = mpSimWorld ? mpSimWorld->getSubGametimeFloat() : 0.0f;

   mSimGameTime = gameTime;

   gRender.beginViewport(viewportIndex);
   
   BVec3 worldMin, worldMax;
   updateSceneMetrics(worldMin, worldMax);
         
//    gSimSceneLightManager.lockLights();   
//    gSimSceneLightManager.updateRenderThreadState(&gRenderSceneLightManager);
      
   updateUnits(pUser, mpSimWorld, worldMin, worldMax);
   
   renderDebugBoundingBoxes(mpSimWorld);
         
   gParticleGateway.pauseUpdate(gModeManager.getModeGame()->getPaused());
   gParticleGateway.enableDistanceFade(!gConfig.isDefined(cConfigDisableParticleDistanceFade));
   
   updateShadowMode();
            
   uint numTeamColors = 0;
   const DWORD* pTeamColors = NULL;
   if (enableTeamColors)
   {
      DWORD teamColors[cMaximumSupportedPlayers];
      pTeamColors = teamColors;
      
      numTeamColors = cMaximumSupportedPlayers;
      
      for (uint i = 0; i < cMaximumSupportedPlayers; i++)
         teamColors[i] = gWorld->getPlayerColor(i, BWorld::cPlayerColorContextSelection);
   }  
          
   updateRenderThread(viewportIndex, deltaT, gameRealTime, gameTime, mSimActualTime, worldMin, worldMax, enableObscuredUnits, numTeamColors, pTeamColors);
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerEndViewport
//--------------------------------------------------------------------------------------
void BRenderControl::workerEndViewport(void* pData)
{
   if ((gRenderDraw.getNumViewports() > 1) && (gRenderDraw.getViewportIndex() == 0))
   {
      D3DRECT srcRect;
      srcRect.x1 = 0;
      srcRect.y1 = 0;
      srcRect.x2 = gRenderDraw.getViewportDesc(0).mWidth;
      srcRect.y2 = gRenderDraw.getViewportDesc(0).mHeight;

      BD3D::mpDev->Resolve(D3DRESOLVE_ALLFRAGMENTS, &srcRect, gViewportManager.getSplitscreenTex(), NULL, 0, 0, NULL, 1.0f, 0, NULL);

      gRenderDraw.dummyPresent();
   }

   BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);
   BD3D::mpDev->SetRenderTarget(1, NULL);
   BD3D::mpDev->SetRenderTarget(2, NULL);
   BD3D::mpDev->SetRenderTarget(3, NULL);
   BD3D::mpDev->SetDepthStencilSurface(BD3D::mpDevDepthStencil);

   gRenderDraw.setDefaultSamplerStates();
   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.unsetTextures();
}

//--------------------------------------------------------------------------------------
// BRenderControl::endViewport
//--------------------------------------------------------------------------------------
void BRenderControl::endViewport(void)
{
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerEndViewport), NULL);

   gRenderDraw.resetMainActiveMatricesAndViewport();               
   
   gRender.endViewport();
}

//--------------------------------------------------------------------------------------
// BRenderControl::updateWorldVisibility
//--------------------------------------------------------------------------------------
void BRenderControl::updateWorldVisibility()
{
   if (!mpSimWorld)
      return;
      
   bool worldVisibilityEnabled = true;
   if (gConfig.isDefined(cConfigNoFogMask) || (gWorld->getFlagAllVisible() && gWorld->getFlagShowSkirt()))
      worldVisibilityEnabled = false;

#ifndef BUILD_FINAL      
   if (!gConfig.isDefined(cConfigFlashGameUI))      
      worldVisibilityEnabled = false;
#endif   

   if (gViewportManager.getCurrentViewport() >= 0)
   {
      BVec4 playableBounds;
      if (gConfig.isDefined(cConfigBlockOutsideBounds))
         playableBounds = BVec4(mpSimWorld->getSimBoundsMinX(), mpSimWorld->getSimBoundsMinZ(), mpSimWorld->getSimBoundsMaxX(), mpSimWorld->getSimBoundsMaxZ());
      else
      {
         float worldSize = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale();
         playableBounds = BVec4(0.0f, 0.0f, worldSize, worldSize);         
      }
      BWorldVisibility::getInstance().updateVisibility(
         worldVisibilityEnabled, 
         gViewportManager.getCurrentViewport(),
         playableBounds, 
         gFlashMinimapRenderer.getVisibilityTexture());
   }         
}                                                 

//--------------------------------------------------------------------------------------
// BRenderControl::render
//--------------------------------------------------------------------------------------
void BRenderControl::renderViewport()
{
   ASSERT_THREAD(cThreadIndexSim);
   
   gpDebugPrimitives->endFrame();    
   
   updateUIManager(mSimGameTime);
   updateDecalManager(mSimGameTime);
   updateWorldVisibility();
      
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerRenderViewport), NULL);
}

//--------------------------------------------------------------------------------------
// BRenderControl::generateCubemapEndFrame
//--------------------------------------------------------------------------------------
void BRenderControl::generateCubemapEndFrame(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mSimGenerateCubemapFaceIndex++;
   if (mSimGenerateCubemapFaceIndex == 6)
   {
      mSimGenerateCubemapFlag = false;
      mSimGenerateCubemapFaceIndex = 0;
      
      if (!mSimGenerateCubemapWasUIEnabled)
         gConfig.remove(cConfigDisableUI);

      gModeManager.getModeGame()->setPaused(false);
   }
}

//--------------------------------------------------------------------------------------
// BRenderControl::generateMegaScreenshotEndFrame
//--------------------------------------------------------------------------------------
void BRenderControl::generateMegaScreenshotEndFrame(void)
{
   BFixedStringMaxPath filename;
   filename.format("%s_%03i", mSimMegaScreenshotFilename.getPtr(), mSimMacroTileIndex, mSimNumMacroTiles);
   gRender.screenShot(filename, false);
   
   if (mSimMacroTileIndex == mSimNumMacroTiles - 1)
   {
      BFixedString256 execStr;
      if(mSimMegaScreenshotUseJitter)
      {
         execStr.format("<exec>\"tools\\imageStitch\\imageStitch.exe\" -errormessagebox -pauseonwarnings -jitter -jitterAAQual %u -cols %u -rows %u \"screenshots\\%s\" \"screenshots\\%s_final.tga\"</exec>",
            mSimMegaScreenshotJitterAAQuality, 
            mSimMegaScreenshotCols,
            mSimMegaScreenshotRows,
            mSimMegaScreenshotFilename.getPtr(),
            mSimMegaScreenshotFilename.getPtr());
      }
      else
      {
         execStr.format("<exec>\"tools\\imageStitch\\imageStitch.exe\" -errormessagebox -pauseonwarnings -cols %u -rows %u \"screenshots\\%s\" \"screenshots\\%s_final.tga\"</exec>",
            mSimMegaScreenshotCols,
            mSimMegaScreenshotRows,
            mSimMegaScreenshotFilename.getPtr(),
            mSimMegaScreenshotFilename.getPtr());

      }
      
      gDebugConnection.sendOutput(execStr);
      
      if (!mSimMegaScreenshotWasUIEnabledFlag)
         gConfig.remove(cConfigDisableUI);

#ifndef BUILD_FINAL      
      if (mSimMegaScreenshotOrigJPEGScreenshotFlag)
         gConfig.define(cConfigJPEGScreenshots);
#endif         
         
      gModeManager.getModeGame()->setPaused(mSimMegaScreenshotWasPausedFlag);
            
      mSimCreatingMegaScreenshotFlag = false;
      mSimMegaScreenshotFlag = false;
      mSimMegaScreenshotCols = 1;
      mSimMegaScreenshotRows = 1;
      mSimNumMacroTiles = 1;
      mSimMegaScreenshotUseJitter = false;
      
      //free these up!
      if (mpSimMegaScreenshotAAOffsets)
      {
         delete [] mpSimMegaScreenshotAAOffsets;
         mpSimMegaScreenshotAAOffsets = 0;
      }
      if (mpSimMegaScreenshotOffsets)
      {
         delete [] mpSimMegaScreenshotOffsets;
         mpSimMegaScreenshotOffsets = 0;
      }

      gRenderThread.submitCallback(renderEndMegascreenshot, NULL);
   }
}

//--------------------------------------------------------------------------------------
// BRenderControl::generateMinimapScreenshotEndFrame
//--------------------------------------------------------------------------------------
void BRenderControl::generateMinimapScreenshotEndFrame(void)
{ 
   gRender.screenShot(BStrConv::toA(mSimMinimapFilename), false);

   gConsoleOutput.status("Generated Minimap Texture: %s", mSimMinimapFilename.getPtr());

   mpSimUser->resetCameraSettings(false);
   mpSimUser->setFlagNoCameraLimits(mSimMinimapScreenshotOldCameraState);
   if(!mpSimUser->getFlagNoCameraLimits())
   {      
      mpSimUser->applyCameraSettings(true);
   }  

   gTerrain.setRenderSkirt(mSimMinimapScreenshotOldRenderSkirtState);

   gScenario.reloadLightSet();

   if (!mSimMinimapScreenshotWasUIEnabledFlag)
      gConfig.remove(cConfigDisableUI);
   
   if (gConfig.isDefined(cConfigNoFogMask) && (!mSimMinimapScreenshotWasFogMaskDisabledFlag))
      gConfig.toggleDefine(cConfigNoFogMask);

   gToneMapManager.setDOFEnabled(mSimMinimapScreenshotWasDOFEnabled);
   
   mSimMinimapScreenshotFlag = false;
   mSimMinimapScreenshotOldRenderSkirtState = false;
   mSimMinimapScreenshotOldCameraState = false;
   mSimMinimapScreenshotWasUIEnabledFlag = false;
   mSimMinimapFilename.empty();      
}

//--------------------------------------------------------------------------------------
// BRenderControl::endOfFrame
//--------------------------------------------------------------------------------------
void BRenderControl::endOfFrame(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   gSimSceneLightManager.unlockLights();
         
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerEndFrame));
   
   if (mSimGenerateCubemapFlag)
      generateCubemapEndFrame();
   else if (mSimCreatingMegaScreenshotFlag)
      generateMegaScreenshotEndFrame();
   else if (mSimMinimapScreenshotFlag)
      generateMinimapScreenshotEndFrame();
      
   gRender.endFrame();
   
   mpSimWorld = NULL;
}

#ifndef BUILD_FINAL
//--------------------------------------------------------------------------------------
// workerDrawText
//--------------------------------------------------------------------------------------
static void workerDrawText(ATG::Font& font, float x, float& y, DWORD color, const wchar_t* pFmt, ...)
{
   BUString str;

   va_list args;
   va_start(args, pFmt);
   str.formatArgs(pFmt, args);
   va_end(args);

   font.DrawText(x, y, color, str.getPtr());
   y += font.GetFontHeight();
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerResetCPUUtilization
//--------------------------------------------------------------------------------------
void BRenderControl::workerResetCPUUtilization(void)
{
   mPrevFrameSysTime = 0;
   
   Utils::ClearObj(mPrevThreadKernelTimes);
   
   for (uint i = 0; i < cThreadIndexMax; i++)
      mAveThreadCPUUtilization[i].clear();
}      

//--------------------------------------------------------------------------------------
// BRenderControl::workerShowCPUUtilization
//--------------------------------------------------------------------------------------
void BRenderControl::workerShowCPUUtilization(float x, float y)
{
   ATG::Font& font = gRenderControl.mRenderFont;

   BUString str;
   
   FILETIME sysTime;
   GetSystemTimeAsFileTime(&sysTime);
   
   bool firstFrame = false;
   if (mPrevFrameSysTime == 0)
   {
      mPrevFrameSysTime = *(uint64*)&sysTime;
      firstFrame = true;
      
      for (uint i = 0; i < cThreadIndexMax; i++)
      {
         mAveThreadCPUUtilization[i].set(30);
         mAveThreadCPUUtilization[i].clear();
      }
   }
   
   const double cFileTimeToRealTime = .0000001f;
   
   double deltaSysTime = (*(uint64*)&sysTime - mPrevFrameSysTime) * cFileTimeToRealTime;
      
   for (uint i = 0; i < cThreadIndexMax; i++)
   {
      HANDLE threadHandle = OpenThread(0, 0, gEventDispatcher.getThreadId(static_cast<BThreadIndex>(i)));

      FILETIME createTime, exitTime, kernelTime, userTime;
      GetThreadTimes(threadHandle, &createTime, &exitTime, &kernelTime, &userTime);

      if (!firstFrame)      
      {
         double deltaKernelTime = (*(uint64*)&kernelTime - mPrevThreadKernelTimes[i]) * cFileTimeToRealTime;
         
         double cpuUtilization = deltaKernelTime / deltaSysTime * 100.0f;

         mAveThreadCPUUtilization[i].addSample(static_cast<float>(cpuUtilization));
      }
      
      mPrevThreadKernelTimes[i] = *(uint64*)&kernelTime;

      CloseHandle(threadHandle);          
   }          
   
   mPrevFrameSysTime = *(uint64*)&sysTime;
   
   for (uint i = 0; i < cThreadIndexMax; i++)
   {
      str.format(L"%S CPU Ave: %2.1f%% Peak: %2.1f%%", gEventDispatcher.getThreadName(static_cast<eThreadIndex>(i)), mAveThreadCPUUtilization[i].getAverage(), mAveThreadCPUUtilization[i].getMaximum());
      font.DrawText((i & 1) ? (x + BD3D::mD3DPP.BackBufferWidth * .4f) : x, y, D3DCOLOR_ARGB(255, 0, 255, 0), str.getPtr());
      if (i & 1)
         y += font.GetFontHeight();
   }         
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerShowGPUIdleTime
//--------------------------------------------------------------------------------------
void BRenderControl::workerShowGPUIdleTime(void* pData)
{
   float gpuIdle = BD3D::mpDev->GetCounter(D3DCOUNTER_FRAME_GPU_IDLE_PERCENT);  
   
   ATG::Font& font = gRenderControl.mRenderFont;

   BUString str;
   
   static float startX = 85.0f/1280.0f * BD3D::mD3DPP.BackBufferWidth;
   static float startY = 600.0f/720.0f * BD3D::mD3DPP.BackBufferHeight;
   
   float x = startX;
   float y = startY;

   str.format(L"GPU Idle: %02.1f%%", gpuIdle);
   DWORD color = (gpuIdle > 5.0f) ? D3DCOLOR_ARGB(255, 255, 20, 20) : D3DCOLOR_ARGB(255, 20, 255, 20);
   font.DrawText(x, y, color, str.getPtr());
   y += font.GetFontHeight();
      
   gRenderControl.workerShowCPUUtilization(x, y);
}
#endif   

//--------------------------------------------------------------------------------------
// BRenderControl::showPersistentStats
//--------------------------------------------------------------------------------------
void BRenderControl::showPersistentStats(BDebugTextDisplay& debugTextDisplay, bool showGPUIdle)
{
   ASSERT_THREAD(cThreadIndexSim);
   
#ifndef BUILD_FINAL
   // Shader visualization mode
   if (cVMDisabled != mSimVisMode)
   {
      debugTextDisplay.setColor(cColorYellow);
      debugTextDisplay.setLine(debugTextDisplay.getTotalLines() - 8);
      debugTextDisplay.printf("Shader Visualization Mode: %s", gUGXGeomVisModeDesc[mSimVisMode]);
   }
   
   if (showGPUIdle)
      gRenderThread.submitCallback(workerShowGPUIdleTime, NULL);
#endif   
}

#ifndef BUILD_FINAL
//--------------------------------------------------------------------------------------
// BRenderControl::workerShowRenderStats
//--------------------------------------------------------------------------------------
void BRenderControl::workerShowRenderStats(void* pData)
{
   uint page = (uint)pData;
         
   static float x = 60.0f;
   static float startY = 56.0f;
   float y = startY;
   ATG::Font& font = gRenderControl.mRenderFont;
      
   BUString str;

   // This not needed since the caller displays the page number for all pages.
/*
   if (page >= 3)
   {
      str.format(L"Render Stats - Page %u", page);
      font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
      y += font.GetFontHeight();
   }
*/
   switch (page)
   {
      case 3:
      {
         // D3D counter stats
         font.DrawText(x, y, 0xFFFFFFFF, L"D3D Counters:");
         y += font.GetFontHeight();

         struct 
         {
            D3DCOUNTER counterIndex;
            const char* pName;
         } D3DCounters [] =
         {
#define DEFINE_D3D_COUNTER(x) { x, #x },
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAMESPERSECOND)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAME_GPU_IDLE_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAME_GPU_WASTED_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_PRIMITIVES_CULLED_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_PRIMITIVES_CLIPPED_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_PRIMITIVES_VISIBLE_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAMETIME)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAMECOUNT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAME_BLOCKED_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAME_THROTTLED_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_FRAME_ACTIVE_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_XAM_GPU_PERCENT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_BANDWIDTH_SYSTEM)
            DEFINE_D3D_COUNTER(D3DCOUNTER_BANDWIDTH_INDICES)
            DEFINE_D3D_COUNTER(D3DCOUNTER_BANDWIDTH_TEXTURE) 
            DEFINE_D3D_COUNTER(D3DCOUNTER_BANDWIDTH_VERTEX)
            DEFINE_D3D_COUNTER(D3DCOUNTER_BANDWIDTH_RESOLVE)
            DEFINE_D3D_COUNTER(D3DCOUNTER_BANDWIDTH_MEMEXPORT)
            DEFINE_D3D_COUNTER(D3DCOUNTER_XAM_COMMAND_BUFFER_SIZE)

#undef DEFINE_D3D_COUNTER
         };

         const uint cNumD3DCounters = sizeof(D3DCounters)/sizeof(D3DCounters[0]);

         for (uint i = 0; i < cNumD3DCounters; i++)
         {
            float counterValue = BD3D::mpDev->GetCounter(D3DCounters[i].counterIndex);

            str.format(L"%S: %f", D3DCounters[i].pName, counterValue);

            font.DrawText(x, y, 0xFFFFFFFF, str);
            y += font.GetFontHeight();
         }
         break;
      }
      case 4:
      {
         // GPU frame heap and dynamic GPU buffer stats
         str.format(L"GPU Frame Heap: Max Alloc: %u, Max Bytes: %u", gGPUFrameHeap.getAllocator().getMaxAllocationCount(), gGPUFrameHeap.getAllocator().getMaxAllocationBytes());
         font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
         y += font.GetFontHeight() * 2.0f;
                 
         BDynamicGPUBuffer* pDynamicGPUBuffers[2] = { gPSManager.getDynamicGPUBuffer(), BFlashRender::getDynamicGPUBuffer() };
         const char* pDynamicGPUBufferNames[2] = { "Particle", "Flash" };
         const uint cNumDynamicGPUBuffers = sizeof(pDynamicGPUBuffers)/sizeof(pDynamicGPUBuffers[0]);
                  
         for (uint i = 0; i < cNumDynamicGPUBuffers; i++)
         {
            if (!pDynamicGPUBuffers[i])
               continue;
                                    
            BDynamicGPUBuffer::BStats& stats = pDynamicGPUBuffers[i]->getStats();
            
            BUString str;
            str.format(L"%S DynamicGPUBuffer: TotalBlocks: %u, PeakBlockTime: %3.1fms, TotalBlockTime: %3.1fms",
               pDynamicGPUBufferNames[i], stats.mTotalBlocks, stats.mPeakBlockTime * 1000.0f, stats.mTotalBlockTime * 1000.0f);
            font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
            y += font.GetFontHeight();

            str.format(L"   MaxTotalBlocks: %u, MaxPeakBlockTime: %3.1fms, MaxTotalBlockTime: %3.1fms", 
               stats.mMaxEverTotalBlocks, stats.mMaxEverPeakBlockTime * 1000.0f, stats.mMaxEverTotalBlockTime* 1000.0f);
            font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
            y += font.GetFontHeight();

            str.format(L"   NumFenceQueueResizes: %u, TotalLocks: %u, TotalLockBytes: %u",
               stats.mNumFenceQueueResizes, stats.mTotalLocks, stats.mTotalLockBytes);
            font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
            y += font.GetFontHeight();
            
            str.format(L"   MaxTotalLocks: %u, MaxTotalLockBytes: %u",
               stats.mMaxEverTotalLocks, stats.mMaxEverTotalLockBytes);
            font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
            y += font.GetFontHeight();
            
            str.format(L"   PendingFences: %u, MaxPendingFences: %u, MaxOutstandingBytes: %u, MaxEverOutstandingBytes: %u",
               stats.mPendingFences, stats.mMaxEverPendingFences, stats.mMaxOutstandingBytes, stats.mMaxEverOutstandingBytes);
            font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
            y += font.GetFontHeight() * 2.0f;
         }
                     
         break;
      }
      case 5:
      {
         // Scene light manager stats
         str.format(L"RenderSceneLightManager: NumActiveLocalLights: %u", gRenderSceneLightManager.getNumActiveLocalLights());
         font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
         y += font.GetFontHeight();

         str.format(L"BVisibleLightManager: NumPerpixelLights: %u, NumShadowedPerpixelLights: %u, NumLightBufferedLights: %u", 
            gVisibleLightManager.getNumVisibleLights(),
            gVisibleLightManager.getNumShadowedVisibleLights(),
            gVisibleLightManager.getNumVisibleLightBufferedLights());
         font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
         y += font.GetFontHeight();
         break;
      }
      case 6:
      {
         // UGX instance manager stats
         BUGXGeomInstanceManager::BStats& ugxGeomInstanceStats = gUGXGeomInstanceManager.getStats();

         str.format(L"UGXInstanceManager: Total Flushes: %u, Total NonEmpty Flushes: %u, Total Model Batches: %u, Total Instances: %u", 
            ugxGeomInstanceStats.mTotalFlushes, 
            ugxGeomInstanceStats.mTotalNonEmptyFlushes, 
            ugxGeomInstanceStats.mTotalModelBatches, 
            ugxGeomInstanceStats.mTotalInstances);
         font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
         y += font.GetFontHeight();

         ugxGeomInstanceStats.clear();

         BUGXGeomRender::BStats& ugxGeomRenderStats = BUGXGeomRender::getStats();
         str.format(L"BUGXGeomRender: Global Begins: %u, Instance Renders: %u, Large Renders: %u, Draws: %u, Instances: %u", 
            ugxGeomRenderStats.mTotalGlobalBegins,
            ugxGeomRenderStats.mTotalInstanceRenders,
            ugxGeomRenderStats.mTotalLargeRenders,
            ugxGeomRenderStats.mTotalDraws,
            ugxGeomRenderStats.mTotalInstances);
         font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
         y += font.GetFontHeight();

         str.format(L"Instance Sections: %u, Large Nodes Examined: %u, Large Nodes Passed: %u, Large Sections Rendered: %u", 
            ugxGeomRenderStats.mTotalInstanceSectionsRendered,
            ugxGeomRenderStats.mTotalLargeNodeCullTests,
            ugxGeomRenderStats.mTotalLargeNodeCullPasses,
            ugxGeomRenderStats.mTotalLargeSectionsRendered);
         font.DrawText(x, y, 0xFFFFFFFF, str.getPtr());
         y += font.GetFontHeight();

         ugxGeomRenderStats.clear();
         
         break;
      }
      case 7:
      {
         // Packed texture stats
         
         workerDrawText(font, x, y, 0xFFFFFFFF, L"Packed Texture Manager Statistics");
         if (!gpPackedTextureManager)
            workerDrawText(font, x, y, 0xFFFFFFFF, L"Disabled");
         else
         {
            BPackedTextureManager::BStats textureStats;
            gpPackedTextureManager->getStats(textureStats);
            
            workerDrawText(font, x, y, 0xFFFFFFFF, L"BPackedTextureManager:");
            workerDrawText(font, x, y, 0xFFFFFFFF, L"             Total MipAlloc Bytes: %u", textureStats.mTotalMipAllocBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"        Total MipAlloc Used Bytes: %u", textureStats.mTotalMipAllocUsedBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"      Total Prefers Base Textures: %u", textureStats.mTotalPrefersBaseTextures);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"         Total Prefers Base Bytes: %u", textureStats.mTotalPrefersBaseBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"  Total Not Prefers Base Textures: %u", textureStats.mTotalNotPrefersBaseTextures);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"      Total Important Base Reuses: %u", textureStats.mTotalImportantBaseReuses);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"    Total Unimportant Base Reuses: %u", textureStats.mTotalUnimportantBaseReuses);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"  Total Desired New Base Textures: %u", textureStats.mTotalDesiredNewBaseTextures);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"     Total Desired New Base Bytes: %u", textureStats.mTotalDesiredNewBaseBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L" Total Base Textures Still Needed: %u", textureStats.mTotalBaseTexturesStillNeeded);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"    Total Base Bytes Still Needed: %u", textureStats.mTotalBaseBytesStillNeeded);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"Total Unpacked Forced To Use Mips: %u", textureStats.mTotalUnpackedTexturesForcedToUseMips);
            y += font.GetFontHeight();
            
            BPackedObjectManager::BStats objectStats;
            gpPackedTextureManager->getPackedObjectManager().getStats(objectStats);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"BPackedObjectManager:");
            
            workerDrawText(font, x, y, 0xFFFFFFFF, L"           Total Active Objects: %u", objectStats.mTotalActiveObjects);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"             Total Active Bytes: %u", objectStats.mTotalActiveBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"          Total Unpacked Allocs: %u", objectStats.mTotalUnpackedAllocs);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"           Total Unpacked Bytes: %u", objectStats.mTotalUnpackedBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"            Total Packed Allocs: %u", objectStats.mTotalPackedAllocs);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"             Total Packed Bytes: %u", objectStats.mTotalPackedBytes);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"         Total Last Frame Packs: %u", objectStats.mTotalPacks);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"  Total Last Frame Bytes Packed: %u", objectStats.mTotalBytesPacked);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"       Total Last Frame Unpacks: %u", objectStats.mTotalUnpacks);
            workerDrawText(font, x, y, 0xFFFFFFFF, L"Total Last Frame Bytes Unpacked: %u", objectStats.mTotalBytesUnpacked);
         }

         break;
      }
   }
}   
#endif

//--------------------------------------------------------------------------------------
// BRenderControl::showRenderStats
//--------------------------------------------------------------------------------------
void BRenderControl::showRenderStats(BDebugTextDisplay& textDisplay, uint page)
{
   ASSERT_THREAD(cThreadIndexSim);
   
#ifndef BUILD_FINAL
   switch (page)
   {
      case 0:
      {
         BRenderThread::BStats stats;
         gRenderThread.getStats(stats);
         textDisplay.printf("RenderThread: NumCommands: %u, NumCommandKicks: %u, CommandBytes: %u, SegmentsOpened: %u",
            stats.mNumCommands, stats.mNumCommandKicks, stats.mCommandBytes, stats.mSegmentsOpened);
         
         textDisplay.printf("CPU Frame Stack: Allocs: %u, Bytes: %u",
            stats.mMainCPUFrameStorageAllocs, stats.mCPUFrameStorageBytes);
         
         textDisplay.printf("GPU Frame Stack: Allocs: %u, Bytes: %u",
            stats.mMainGPUFrameStorageAllocs, stats.mGPUFrameStorageBytes);
                  
         break;
      }
      case 1:
      case 2:
      {
         BUGXGeomUberSectionRendererManager::displayStats(textDisplay, page - 1);
               
         BGrannyInstanceRenderer::BStats& grannyStats = gGrannyInstanceRenderer.getStats();
                     
         textDisplay.printf("GrannyInstanceRenderer TotalBones: %u, TotalGPUFrameStorage: %u, TotalInstances: %u", 
            grannyStats.mTotalBones,
            grannyStats.mTotalGPUFrameStorage,
            grannyStats.mTotalInstances);
            
         textDisplay.printf("  MaxEverBones: %u, MaxEverGPUFrameStorage: %u, MaxEverInstances: %u, MaxEverModelBones: %u",
            grannyStats.mMaxEverBones,
            grannyStats.mMaxEverGPUFrameStorage,
            grannyStats.mMaxEverInstances,
            grannyStats.mMaxEverModelBones);
         
         textDisplay.printf("SampleAnimCache: %u, FrameGetBone: %d, FrameCached: %d, TotalGetBone: %d, TotalCached: %d",
            gGrannyManager.getSampleAnimCacheRefCount(), 
            gGrannyManager.getStatFrameGetBoneCallCount(), gGrannyManager.getStatFrameGetBoneCachedCount(),
            gGrannyManager.getStatTotalGetBoneCallCount(), gGrannyManager.getStatTotalGetBoneCachedCount());

         grannyStats.mTotalBones = 0;
         grannyStats.mTotalGPUFrameStorage = 0;
         grannyStats.mTotalInstances = 0;   
         
         break;
      }         
   }  
   
   gRenderThread.submitCallback(workerShowRenderStats, (const void*)page);
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::showParticleStats
//--------------------------------------------------------------------------------------
void BRenderControl::showParticleStats(BDebugTextDisplay& textDisplay)
{
   ASSERT_THREAD(cThreadIndexSim);
   
#ifndef BUILD_FINAL
   
   // Render thread stats
   {
      BParticleSystemManager::BStats particleStats;
      gPSManager.getSimStats(particleStats);

      textDisplay.printf("Particle Statistics");
            
      textDisplay.printf("Gateway: Used Data Slots : %u,  %u, Free Instance Slots: %u,  Instance Watermark: %u", 
         gParticleGateway.getNumDataSlotsInUse(),
         gParticleGateway.getNumInstanceSlotsFree(),
         gParticleGateway.getNumInstanceHighWaterMark());

      BColor color;
      color = cColorWhite;
      if (particleStats.mUniqueEmitters > 2048)
         color = cColorRed;
      textDisplay.printf(color, "PSM: Data Files Loaded : %u, Unique Data Emitters: %u", particleStats.mDataCount, particleStats.mUniqueEmitters);
            
      textDisplay.printf("Sim Effects: Total #: %u, Emitters: [Total: %u  Loop: %u  NonLoop: %u  Invalid Data Ptr: %u]", 
         particleStats.mNumActiveEffects, 
         particleStats.mTotalNumActiveEffectEmitters,
         particleStats.mNumActiveLoopingEffectEmitters,
         particleStats.mNumActiveNonLoopEffectEmitters,
         particleStats.mNumActiveInvalidDataEffectEmitters);

      textDisplay.printf("Total # Emitters: %u [Loop=%u, NonLoop=%u], Total # Particles: %u", 
         particleStats.mNumActiveEmitters, 
         particleStats.mNumActiveLoopEmitters, 
         particleStats.mNumActiveNonLoopEmitters,
         particleStats.mNumActiveParticles);
      
      textDisplay.printf("TotalUpdate: %4.6f (ms), Render: %4.6f (ms)", 
         particleStats.mTotalUpdateTime*1000.0f, particleStats.mRenderTime*1000.0f);
            
      textDisplay.printf(cColorWhite, "VMXUpdate: %4.6f (ms)", particleStats.mVMXUpdateTime*1000.0f);
      textDisplay.printf(cColorWhite, "Emission: %4.6f (ms)", particleStats.mEmissionTime*1000.0f);      
      textDisplay.printf(cColorWhite, "  Normal: %4.6f (ms)", particleStats.mEmissionTimeNormal*1000.0f);
      textDisplay.printf(cColorWhite, "  Trail:  %4.6f (ms)", particleStats.mEmissionTimeTrail*1000.0f);
      textDisplay.printf(cColorWhite, "  Beam:   %4.6f (ms)", particleStats.mEmissionTimeBeam*1000.0f);
      
      textDisplay.printf("# Updated Emitters: %u [Phase 1: %u , Phase 2: %u] # Updated Particles: %u [Phase 1: %u, Phase 2: %u]", 
         particleStats.mNumUpdatedEmitters, 
         particleStats.mNumUpdatedEmittersPhase1,
         particleStats.mNumUpdatedEmittersPhase2,                  
         particleStats.mNumUpdatedParticles,
         particleStats.mNumUpdatedParticlesPhase1, 
         particleStats.mNumUpdatedParticlesPhase2);
            
      textDisplay.printf("Total Particles Allocated: %06u (%uMB)", particleStats.mTotalParticlesAllocated, (particleStats.mTotalParticlesAllocated * sizeof(BParticleSystemParticle) + (512*1024)) / (1024*1024));
      
      uint totalKilled = particleStats.mTotalEmittersKilled+particleStats.mTotalEmittersForceKilled;
      textDisplay.printf(cColorWhite, "Total Emitters:  Created: [%04u] Killed: [%04u] Force Killed: [%04u] CreateToKillRatio[%4.6f]", particleStats.mTotalEmittersCreated, particleStats.mTotalEmittersKilled, particleStats.mTotalEmittersForceKilled, totalKilled > 0 ? 100.0f * ((float)particleStats.mTotalEmittersCreated/(float)totalKilled) : 0.0f);

      textDisplay.printf(cColorWhite, "UsedList");
      textDisplay.printf(cColorWhite, "Total Emitters AboutToDie: %04u  [ Loop: %04u  Non Loop: %04u  Unaccounted For: %04u)", particleStats.mTotalEmittersAboutToDie, particleStats.mTotalEmittersAboutToDieLooping, particleStats.mTotalEmittersAboutToDieNonLooping, particleStats.mTotalEmittersAboutDieUnaccountedFor);
      textDisplay.printf(cColorWhite, "Total Emitters Active    : %04u  [ Loop: %04u  Non Loop: %04u  Unaccounted For: %04u)", particleStats.mTotalEmittersActive, particleStats.mTotalEmittersActiveLooping, particleStats.mTotalEmittersActiveNonLooping, particleStats.mTotalEmittersActiveUnaccountedFor);
            
      textDisplay.printf("Total # Rendered Emitters: %u", particleStats.mTotalNumRenderedEmittersAcrossAllTiles);
      
      textDisplay.printf("Total # Rendered Particles: %u", particleStats.mTotalNumRenderedParticlesAcrossAllTiles);
      
      float particlePercentage = 0.0f;
      float emitterPercentage = 0.0f;

      //float lw0=gFontManager.getLineLength("Tile[XX] :", 12);
      //float lw1=gFontManager.getLineLength("# Rendered Emitters: XXXXXX", 30);
      //float lw2=gFontManager.getLineLength("# Rendered Particles: XXXXXX", 30);
      //float lw3=gFontManager.getLineLength("(XXXXXX.XXX)", 13);

      for (long i = 0; i < particleStats.mTileData.getNumber(); i++)
      {
         particlePercentage = 0.0f;
         emitterPercentage = 0.0f;
         if (particleStats.mTotalNumRenderedEmittersAcrossAllTiles > 0)
            emitterPercentage = 100.0f * ((float) particleStats.mTileData[i].mNumEmitters / (float) particleStats.mTotalNumRenderedEmittersAcrossAllTiles);

         if (particleStats.mTotalNumRenderedParticlesAcrossAllTiles > 0)
            particlePercentage = 100.0f * ((float) particleStats.mTileData[i].mNumParticles / (float) particleStats.mTotalNumRenderedParticlesAcrossAllTiles);

         textDisplay.printf("Tile [%l] : ", i);
         
         textDisplay.printf("# Rendered Emitters: %u (%6.2f%%)", particleStats.mTileData[i].mNumEmitters, emitterPercentage);
                           
         textDisplay.printf("# Rendered Particles: %u (%6.2f%%)", particleStats.mTileData[i].mNumParticles, particlePercentage);
      }
      
      textDisplay.printf("Updated Emitter Types:");
      textDisplay.printf("  Billboard        = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eBillBoard], 100.0f * ((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eBillBoard])/particleStats.mNumUpdatedEmitters );
      textDisplay.printf("  Upfacing         = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eUpfacing], 100.0f *((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eUpfacing])/particleStats.mNumUpdatedEmitters );
      textDisplay.printf("  Oriented Axial   = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eOrientedAxialBillboard], 100.0f *((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eOrientedAxialBillboard])/particleStats.mNumUpdatedEmitters );
      textDisplay.printf("  Trail            = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eTrail], 100.0f *((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eTrail])/particleStats.mNumUpdatedEmitters );
      textDisplay.printf("  Trail Cross      = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eTrailCross], 100.0f *((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eTrailCross])/particleStats.mNumUpdatedEmitters );
      textDisplay.printf("  Velocity Aligned = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eVelocityAligned], 100.0f *((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eVelocityAligned])/particleStats.mNumUpdatedEmitters );
      textDisplay.printf("  Beam             = %d (%6.2f%%)", particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eBeam], 100.0f *((float)particleStats.mNumUpdatedEmittersTypes[BEmitterBaseData::eBeam])/particleStats.mNumUpdatedEmitters );      

      textDisplay.printf("Max Scene Stats");
      
      textDisplay.printf("Max Active Effects %u", particleStats.mSceneMaxActiveEffects);
      
      textDisplay.printf("Max Active Emitters: %u    Max Active Particles: %u", particleStats.mSceneMaxActiveEmitters, particleStats.mSceneMaxActiveParticles);
      
      textDisplay.printf("Max Rendered Emitters: %u    Max Rendered Particles: %u", particleStats.mSceneMaxRenderedEmitters, particleStats.mSceneMaxRenderedParticles);

      textDisplay.printf("Num Dropped Effects: %u", particleStats.mNumDroppedEffects);
   }
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::showFlashStats
//--------------------------------------------------------------------------------------
void BRenderControl::showFlashStats(BDebugTextDisplay& textDisplay)
{
   ASSERT_THREAD(cThreadIndexSim);

#ifndef BUILD_FINAL
   BFlashManager::BStats stats;
   gFlashManager.getSimStats(stats);

   textDisplay.printf(cColorWhite, "Flash Stats");

   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "Times");
   textDisplay.printf("Total CPU Time        : %4.3f(ms)", stats.mCPUTimeAdvance+stats.mCPUTimeDisplay);
   textDisplay.printf("      CPU Advance Time: %4.3f(ms)", stats.mCPUTimeAdvance);
   textDisplay.printf("      CPU Display Time: %4.3f(ms)", stats.mCPUTimeDisplay);
  
   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "Render Stats");
   textDisplay.printf("Total Movies Rendered: %u", stats.mMoviesRenderedPerFrame);
   textDisplay.printf("      Decal Movies   : %u", stats.mDecalMoviesRenderedPerFrame);
   textDisplay.printf("Draw Calls Per Frame : %u", stats.mDrawCallsPerFrame);
   textDisplay.printf("Resolves: %u  Pixels : %u", stats.mResolvesPerFrame, stats.mResolvedPixelsPerFrame);
   textDisplay.printf("Primitives: %u  Triangles: %u  Lines: %u", stats.mPrimitives, stats.mTriangles, stats.mLines);

   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "ActionScript Overhead");
   textDisplay.printf("AS Invokes Per Frame: %u", stats.mASInvokesPerFrame);
   textDisplay.printf("AS Set Variable Per Frame: %u", stats.mASSetVariablePerFrame);

   textDisplay.skipLine();
   textDisplay.printf(cColorWhite, "Shader Draw Calls");
   for (int j = 0; j < 6; ++j)
   {
      if (stats.mVSDrawCalls[j] > 0)
         textDisplay.printf("Vertex Shader [%d] : %d", j, stats.mVSDrawCalls[j]);
   }

   for (int i = 0; i < 25; ++i)
   {
      if (stats.mPSDrawCalls[i] > 0)
         textDisplay.printf("Pixel Shader [%d] : %d", i, stats.mPSDrawCalls[i]);
   }   
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::showImpactEffectStats
//--------------------------------------------------------------------------------------
void BRenderControl::showImpactEffectStats(BDebugTextDisplay& textDisplay)
{
   ASSERT_THREAD(cThreadIndexSim);
   
#ifndef BUILD_FINAL
   BFixedString256 t;

   // Projectile stats
   textDisplay.printf(cColorYellow, "Num Projectiles: %d, Num PerturbanceData: %d, Num StickData: %d", 
      gWorld->getObjectManager()->getNumberProjectiles(), BPerturbanceData::mFreeList.getNumberAllocated(), BStickData::mFreeList.getNumberAllocated());

   // Render thread stats
   BTerrainEffectManager::BStats stats;
   gTerrainEffectManager.getStats(stats);

   textDisplay.printf(cColorWhite, "ImpactEffect Sim Stats");
   textDisplay.printf(cColorWhite, "Per Frame Stats");
   textDisplay.printf(cColorWhite, "   Sim Attempts    : %d", stats.mNumImpactsSimTriedToCreate);
   textDisplay.printf(cColorWhite, "   Dropped Impacts : %d", stats.mDroppedImpacts);
   textDisplay.printf(cColorWhite, "   Added Impacts   : %d", stats.mAddedImpacts);

   gTerrainEffectManager.dumpVoxelData(textDisplay);
      
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::showTerrainStats
//--------------------------------------------------------------------------------------
void BRenderControl::showTerrainStats(BDebugTextDisplay& textDisplay)
{
   ASSERT_THREAD(cThreadIndexSim);
   
#ifndef BUILD_FINAL
   // If you modify this, also change dumpTerrainStats() in consolefuncsengine.cpp!
   BFixedString256 t;
   // Render thread stats
   textDisplay.printf("Terrain Stats");
   
   textDisplay.printf("Terrain Memory:");
   
   float mem = (BTerrainMetrics::getTotalGPUMem() + BTerrainMetrics::mCPUMemCount) / float(1024*1024);
   textDisplay.printf(cColorRed, "Total CPU+GPU: %4.3fMB", mem);
      
   mem = BTerrainMetrics::mCPUMemCount / float(1024*1024);
   textDisplay.printf(cColorRed, " Total CPU: %4.3fMB", mem);

   mem = BTerrainMetrics::getTotalGPUMem() / float(1024*1024);
   textDisplay.printf(cColorRed, " Total GPU: %4.3fMB", mem);
   
   textDisplay.skipLine(1);   

   textDisplay.printf("GPU: %4.3fMB", BTerrainMetrics::getTotalGPUMem());

   mem = BTerrainMetrics::mTerrainGPUMemCount / float(1024*1024);
   textDisplay.printf("   Vertex/Normal/AO/Alpha GPU: %4.3fMB", mem);
   
   mem = BTerrainMetrics::mAlphaTextureGPUCount / float(1024*1024);
   textDisplay.printf("   Terrain Blend Textures GPU: %4.3fMB", mem);

   mem = BTerrainMetrics::mUniqueAlbedoGPUCount / float(1024*1024);
   textDisplay.printf("   Unique Albedo Texture GPU: %4.3fMB", mem);

   mem = BTerrainMetrics::mFoliageGPUCount / float(1024*1024);
   textDisplay.printf("   Foliage Data GPU: %4.3fMB", mem);

   mem = BTerrainMetrics::mTesselationGPUCount / float(1024*1024);
   textDisplay.printf("   Tessellation LOD GPU: %4.3fMB", mem);
   
   mem = BTerrainMetrics::mPrecomputedLightingGPUCount / float(1024*1024);
   textDisplay.printf("   Precomputed Lighting GPU: %4.3fMB", mem);

   mem = BTerrainMetrics::getTotalCacheGPUMem() / float(1024*1024);      
   textDisplay.printf(" Cache Textures: %4.3fMB", mem);
   for(int i=0;i<cTextureTypeMax;i++)
   {
      mem = BTerrainMetrics::mCacheTextureGPUMemCount[i] / float(1024*1024);
      textDisplay.printf("      TextureType %i GPU: %4.3fMB",i, mem);
   }
           
   mem = BTerrainMetrics::getTotalArtistsTexGPUMem() / float(1024*1024);
   textDisplay.printf("   Artist Textures: %4.3fMB",mem);
   for(int i=0;i<cTextureTypeMax;i++)
   {
      if(BTerrainMetrics::mArtistTerrainTextureGPUMem[i]==0)
         mem=0;
      else
         mem = BTerrainMetrics::mArtistTerrainTextureGPUMem[i] / float(1024*1024);
      textDisplay.printf("     TextureType %i GPU: %4.3fMB",i, mem);
   }


   mem = BTerrainMetrics::mCacheMemCPUCount / float(1024*1024);
   textDisplay.printf("Cache CPU: %4.3fMB", mem);
   
   textDisplay.skipLine(1);
   textDisplay.printf("GPU Packs:");
   uint totalPacks = gTerrainTexturing.getGPUPackCount();
   uint totalPixels = gTerrainTexturing.getGPUPackPixels();
   textDisplay.printf("Total Packs this Frame: %u, Total Pixels: %u", totalPacks, totalPixels);
      
   mTotalGPUPacksAverage.addSample((float)totalPacks);
   mTotalGPUPackPixelsAverage.addSample((float)totalPixels);
   
   textDisplay.printf("Ave packs: %.1f, Ave pixels: %.1f", mTotalGPUPacksAverage.getAverage(), mTotalGPUPackPixelsAverage.getAverage());
   textDisplay.printf("Max packs: %.1f, Max pixels: %.1f", mTotalGPUPacksAverage.getMaximum(), mTotalGPUPackPixelsAverage.getMaximum());
      
   gTerrainTexturing.resetGPUPackStats();
#endif
}
//--------------------------------------------------------------------------------------
// class BHandleInputData
//--------------------------------------------------------------------------------------
class BHandleInputData
{
public:
   long mPort;
   long mEvent;
   long mControlType;
   BInputEventDetail mDetail;
   bool mControlPressed : 1;
   bool mShiftPressed : 1;
   bool mAltPressed : 1;
};

//--------------------------------------------------------------------------------------
// BRenderControl::workerHandleInput
//--------------------------------------------------------------------------------------
void BRenderControl::workerHandleInput(void* pData)
{
   pData;
   
#ifndef BUILD_FINAL
   ASSERT_THREAD(cThreadIndexRender);
   const BHandleInputData& inputData = *static_cast<const BHandleInputData*>(pData);
      
   // DO NOT call anything in gInputSystem from here - it's not thread safe! Use inputData instead.
   
   if (inputData.mEvent == cInputEventControlStart)
   {
      switch (inputData.mControlType)
      {
         case cKeyZ:
         {
            if (inputData.mAltPressed)
            {
               if (!gpPackedTextureManager)
               {
                  gConsoleOutput.status("Packed Texture Cache Disabled");
               }
               else
               {
                  BPackedTextureManager::eMode curMode = gpPackedTextureManager->getMode();
                  curMode = static_cast<BPackedTextureManager::eMode>(curMode + 1);
                  if (curMode >= BPackedTextureManager::cModeNum)
                     curMode = BPackedTextureManager::cModeCaching;
                  
                  gpPackedTextureManager->setMode(curMode);
                  
                  gConsoleOutput.status("Packed Texture Cache Mode Set To: %s", BPackedTextureManager::getModeDesc(curMode));
               }                  
            }
            
            break;
         }
         case cKeyW:
         {
            if (inputData.mShiftPressed)
            {
               mRenderTerrainHeightfield = !mRenderTerrainHeightfield;
               gConsoleOutput.status("Terrain heightfield %s", mRenderTerrainHeightfield ? "Enabled" : "Disabled");
            }
            else if (!inputData.mControlPressed)
            {
               int textureMode = BUGXGeomRender::getTextureMode();
               
               textureMode += (inputData.mAltPressed ? -1 : 1);
               
               if (textureMode < 0)
                  textureMode = cTMNum - 1;
               else if (textureMode >= cTMNum)
                  textureMode = cTMNormal;
                  
               BUGXGeomRender::setTextureMode(static_cast<eUGXGeomTextureMode>(textureMode));
               BTerrain::setTextureMode(static_cast<eUGXGeomTextureMode>(textureMode));
               
               gConsoleOutput.status("Setting Texture Mode: %s", getUGXGeomTextureModeDesc(static_cast<eUGXGeomTextureMode>(textureMode)));
            }
            break;
         }
         case cKeyV:
         {
            if ((inputData.mAltPressed) && (inputData.mControlPressed))
            {
               const bool lightBufferingEnabled = !gVisibleLightManager.getLightBufferingEnabled();
               gVisibleLightManager.setLightBufferingEnabled(lightBufferingEnabled);

               gConsoleOutput.status("Light Buffering %s", lightBufferingEnabled ? "Enabled" : "Disabled");
            }
            else if ((!inputData.mAltPressed) && (!inputData.mControlPressed))
            {
               const bool localLightingEnabled = !gVisibleLightManager.getLocalLightingEnabled();
               gVisibleLightManager.setLocalLightingEnabled(localLightingEnabled);
               
               gConsoleOutput.status("Local Lighting %s", localLightingEnabled ? "Enabled" : "Disabled");
            }
            break;
         }
      }
   }
   
   gDirShadowManager.debugHandleInput(inputData.mEvent, inputData.mControlType, inputData.mAltPressed, inputData.mShiftPressed, inputData.mControlPressed);
   gLocalShadowManager.debugHandleInput(inputData.mEvent, inputData.mControlType, inputData.mAltPressed, inputData.mShiftPressed, inputData.mControlPressed);
   gRenderSceneLightManager.debugHandleInput(inputData.mEvent, inputData.mControlType, inputData.mAltPressed, inputData.mShiftPressed, inputData.mControlPressed);
   gWaterManager.debugHandleInput(inputData.mEvent, inputData.mControlType, inputData.mAltPressed, inputData.mShiftPressed, inputData.mControlPressed);
#endif      
}
//--------------------------------------------------------------------------------------
// BRenderControl::generateCubeMap
//--------------------------------------------------------------------------------------
void BRenderControl::generateCubeMap(const char* pFLSOutputFilename)
{
   if (gViewportManager.isSplitScreen())
      return;
   
   mSimGenerateCubemapFlag = true;
   mSimGenerateCubemapFaceIndex = 0;
   const uint cGenerateCubemapDim = 256;
   mSimGenerateCubemapDim = cGenerateCubemapDim;
   mSimGenerateCubemapSH = true;//(controlType == cKeyF11);
   mSimFLSSaveAsFileName = pFLSOutputFilename;

   BVector l(mpSimUser->getCamera()->getCameraLoc());

   gConsoleOutput.status("Generating HDR cubemap at %f, %f, %f", l.x,l.y,l.z);
}

//--------------------------------------------------------------------------------------
// BRenderControl::generateMinimap
//--------------------------------------------------------------------------------------
void BRenderControl::generateMinimap(const char* pFilename, int zoomLevel)
{
   if ((mSimMegaScreenshotFlag) || (mSimGenerateCubemapFlag) || (gViewportManager.isSplitScreen()))
      return;

   mSimMinimapScreenshotOldRenderSkirtState = gTerrain.getRenderSkirt();
   gTerrain.setRenderSkirt(false);
   
   mSimMinimapScreenshotOldCameraState = mpSimUser->getFlagNoCameraLimits();
   mpSimUser->setFlagNoCameraLimits(true);

   mSimMinimapScreenshotWasUIEnabledFlag = gConfig.isDefined(cConfigDisableUI);
   mSimMinimapScreenshotWasFogMaskDisabledFlag = gConfig.isDefined(cConfigNoFogMask);

   //-- disable Depth of Field
   mSimMinimapScreenshotWasDOFEnabled = gToneMapManager.getDOFEnabled();
   gToneMapManager.setDOFEnabled(false);

   gConfig.define(cConfigDisableUI);

   mSimMinimapScreenshotFlag = true;
   mSimMinimapScreenshotZoomLevel = zoomLevel;

   if (!gConfig.isDefined(cConfigNoFogMask))
      gConfig.toggleDefine(cConfigNoFogMask);

   BString filename;
   strPathGetFilename(gWorld->getTerrainName(), filename);

   SYSTEMTIME systemTime;
   GetLocalTime(&systemTime);
      
   char systemName[256] = "Unknown ";
#ifndef BUILD_FINAL
      DWORD size = sizeof(systemName);
      DmGetXboxName(systemName, &size);
#endif
   
   mSimMinimapFilename.format("%s-%02d%02u%02u-%02u%02u%02u", filename.getPtr(), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

   BFogParams fogParams;
   fogParams.mZColor = BVec3(0,0,0); 
   fogParams.mZStart = 1000000.0f;
   fogParams.mZDensity = 0.0f;   

   fogParams.mPlanarColor = BVec3(0,0,0);
   fogParams.mPlanarStart = 0.0f;
   fogParams.mPlanarDensity = 0.0f;

   gSimSceneLightManager.setFogParams(cLCTerrain, fogParams);
   gSimSceneLightManager.setFogParams(cLCUnits, fogParams);   
}

#ifndef BUILD_FINAL
struct BStartVideoParams
{
   BFixedString256 mFilename;
   float mFPSLockRate;
   bool mDownsample;
   bool mAutoConvert;
   bool mRaw;
};

//--------------------------------------------------------------------------------------
// BRenderControl::startVideo
//--------------------------------------------------------------------------------------
void BRenderControl::startVideo(const char* pFilename, bool downsample, bool autoConvert, float fpsLockRate, bool raw)
{
   BStartVideoParams* p = gRenderThread.allocateFrameStorageObj<BStartVideoParams, true>(1);
   //p->mFilename.format("e:\\%s", pFilename);
   p->mFilename.set(pFilename);
   p->mDownsample = downsample;
   p->mFPSLockRate = fpsLockRate;
   p->mAutoConvert = autoConvert;
   p->mRaw = raw;
   
#if 0   
   // This isn't accurate, because the game uses Sleep() to slow down when it runs too fast. 
   // It causes the video to play back too quickly when the game runs too slowly.
   if (gConfig.isDefined(cConfigEnableFPSLock))
   {      
      float targetFPS = 30.0f;
      gConfig.get(cConfigFPSLockRate, &targetFPS);
      p->mFPSLockRate = targetFPS;
   } 
#endif

#if 0
   // This doesn't work either, because the cursor is still updated in real-time, unlike the sim which is ticked at a fixed rate.
   if (gModeManager.getModeGame())        
   {
      float fixedUpdateTime = gModeManager.getModeGame()->getFixedUpdate();
      if (fixedUpdateTime > 0.0f)
      {
         p->mFPSLockRate = 1.0f / fixedUpdateTime;
      }
   }
#endif   
   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerStartVideo), p);
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerStartVideo
//--------------------------------------------------------------------------------------
void BRenderControl::workerStartVideo(void* pData)
{
   pData;

   workerEndVideo(NULL);
   
   const BStartVideoParams* pParams = static_cast<const BStartVideoParams*>(pData);
         
   mpRenderGPUDXTVideo = HEAP_NEW(BGPUDXTVideo, gRenderHeap);
   mGPUDXTVideoAutoConvert = pParams->mAutoConvert;
   mGPUDXTVideoFilename = pParams->mFilename;

   DmMapDevkitDrive();

   mpRenderGPUDXTVideo->init(BFixedString256(cVarArg, "e:\\%s", pParams->mFilename.getPtr()), pParams->mDownsample, pParams->mFPSLockRate, pParams->mRaw, &gWin32LowLevelFileIO);
}

//--------------------------------------------------------------------------------------
// BRenderControl::endVideo
//--------------------------------------------------------------------------------------
void BRenderControl::endVideo(void)
{
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerEndVideo));
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerEndVideo
//--------------------------------------------------------------------------------------
void BRenderControl::workerEndVideo(void* pData)
{
   pData;

   if (!mpRenderGPUDXTVideo)
      return;
    
   mpRenderGPUDXTVideo->deinit();
   
   if (mGPUDXTVideoAutoConvert)
   {
      mGPUDXTVideoAutoConvert = false;
      
      BFixedString256 execStr;
      
      execStr.format("<exec>\"tools\\vidToAvi\\vidToAvi.exe\" \"xe:\\%s\" \"screenshots\\%s.wmv\" -outputExactFrames</exec>",
         mGPUDXTVideoFilename.getPtr(),
         mGPUDXTVideoFilename.getPtr() );

      gDebugConnection.sendOutput(execStr);
   }
   
   HEAP_DELETE(mpRenderGPUDXTVideo, gRenderHeap);
   mpRenderGPUDXTVideo = NULL;   
}

#endif // BUILD_FINAL

//--------------------------------------------------------------------------------------
// BRenderControl::handleInput
//--------------------------------------------------------------------------------------
bool BRenderControl::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BHandleInputData* pInputData = static_cast<BHandleInputData*>(gRenderThread.allocateFrameStorage(sizeof(BUpdateData)));

   pInputData->mPort = port;
   pInputData->mEvent = event;
   pInputData->mControlType = controlType;
   pInputData->mDetail = detail;
   pInputData->mControlPressed = gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl);
   pInputData->mShiftPressed = gInputSystem.getKeyboard()->isKeyActive(cKeyShift);
   pInputData->mAltPressed = gInputSystem.getKeyboard()->isKeyActive(cKeyAlt);
   
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BRenderControl::workerHandleInput), pInputData);
      
#ifndef BUILD_FINAL   
   if(event == cInputEventControlStart)
   {
      switch (controlType)
      {
         case cKeyF12:
         case cKeyF11:
         {
            if ((!mSimGenerateCubemapFlag) && (!mSimCreatingMegaScreenshotFlag) && (!mSimMegaScreenshotFlag) && (!gLiveSystem->isMultiplayerGameActive()))
            {
               if ((gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)) && (controlType == cKeyF11))
               {
                  gConsoleOutput.status("Resetting SH fill lights");

                  gSimSceneLightManager.resetSHFillLights();
               }
               else if ((gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)) && (controlType == cKeyF12))
               {
                  gConsoleOutput.status("Saving SH fill lights");

                  gScenario.saveSHFillLight(0);
               }
               else
               {
                  generateCubeMap(0);
               }                  
            }
            break;
         }
         case cKeyF3:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyAlt))
            {
               setFlag(cFlagDebugRenderUIWireframe, !getFlag(cFlagDebugRenderUIWireframe));
               gFlashGateway.setEnableWireframe(getFlag(cFlagDebugRenderUIWireframe));
               gConsoleOutput.status("UI Wireframe %s", getFlag(cFlagDebugRenderUIWireframe) ? "Enabled" : "Disabled");
            }
            else if(!gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl) && !gInputSystem.getKeyboard()->isKeyActive(cKeyShift))
            {
               setFlag(cFlagDebugRenderWireframe, !getFlag(cFlagDebugRenderWireframe));
               gConsoleOutput.status("Wireframe %s", getFlag(cFlagDebugRenderWireframe) ? "Enabled" : "Disabled");
            }

            return true;
         }
         case cKeyM:
         {
            if(gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl) && gInputSystem.getKeyboard()->isKeyActive(cKeyShift))
            {
               if (!mpSimUser)
                  return true;
               BVector intPt = mpSimUser->getHoverPoint();

               //get our XZ in relative position of the map
               float xRadius = 20;   //in world space
               float zRadius = 10;   //in world space
               float desHeight =intPt.y;  //in world space

               float minX = (intPt.x - xRadius)/ gTerrain.getMax().x;
               float maxX = (intPt.x + xRadius)/ gTerrain.getMax().x;
               float minZ = (intPt.z - zRadius)/ gTerrain.getMax().z;
               float maxZ = (intPt.z + zRadius)/ gTerrain.getMax().z;

               gTerrainSimRep.flattenInstant(minX,maxX,minZ,maxZ,desHeight);
               
               return true;
            }
            
            break;
         }
         case cKeyN:
         {
            if (!mpSimUser)
               return true;
            BVector intPt = mpSimUser->getHoverPoint();

            //get our XZ in relative position of the map
            float xRadius = 10;   //in world space
            float zRadius = 10;   //in world space
            bool value = false;

            //float cX = (intPt.x )/ gTerrain.getMax().x;
            //float cY = (intPt.z )/ gTerrain.getMax().z;

            float minX = (intPt.x - xRadius)/ gTerrain.getMax().x;
            float maxX = (intPt.x + xRadius)/ gTerrain.getMax().x;
            float minZ = (intPt.z - zRadius)/ gTerrain.getMax().z;
            float maxZ = (intPt.z + zRadius)/ gTerrain.getMax().z;

            gTerrainDynamicAlpha.setToRegionToValueRectangleAligned(minX,minZ,maxX,maxZ,value);
            //gTerrainDynamicAlpha.setToRegionToValueCircle(cX,cY,xRadius,value);
            return true;
         }
         case cKeyB:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               setFlag(cFlagDebugRenderBoundingBoxes, !getFlag(cFlagDebugRenderBoundingBoxes));

               gParticleGateway.enableBBoxRendering(getFlag(cFlagDebugRenderBoundingBoxes));
               
               gConsoleOutput.status("Bounding box rendering %s", getFlag(cFlagDebugRenderBoundingBoxes) ? "Enabled" : "Disabled");
               return true;
            }
            break;
         }
         case cKeyU:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               if (gInputSystem.getKeyboard()->isKeyActive(cKeyAlt))
               {
                  setFlag(cFlagDisableRenderSky, !getFlag(cFlagDisableRenderSky));
                  gConsoleOutput.status("Sky rendering %s", getFlag(cFlagDisableRenderSky) ? "Disabled" : "Enabled");
               }
               else
               {
                  setFlag(cFlagDisableRenderUnits, !getFlag(cFlagDisableRenderUnits));
                  gConsoleOutput.status("Unit rendering %s", getFlag(cFlagDisableRenderUnits) ? "Disabled" : "Enabled");
               }
                  
               return true;
            }
            break;
         }
         case cKeyT:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               setFlag(cFlagDisableRenderTerrain, !getFlag(cFlagDisableRenderTerrain));
               gConsoleOutput.status("Terrain rendering %s", getFlag(cFlagDisableRenderTerrain) ? "Disabled" : "Enabled");
               return true;
            }
            else if (gInputSystem.getKeyboard()->isKeyActive(cKeyShift))
            {
               gTerrain.toggleRenderVisGrid();
               gConsoleOutput.status("Terrain VisGrid rendering %s", gTerrain.isVisGridEnabled() ? "Enabled" : "Disabled");
               return true;
            }
            else if (gInputSystem.getKeyboard()->isKeyActive(cKeyAlt))
            {
               setFlag(cFlagRandomSplatTest, !getFlag(cFlagRandomSplatTest));
               gConsoleOutput.status("Random Splat Test %s", getFlag(cFlagDisableRenderTerrain) ? "Disabled" : "Enabled");
               return true;
            }
            break;
         }  
         case cKeyL:
         {
            if(gInputSystem.getKeyboard()->isKeyActive((cKeyShift)))
            {

               float maxT = gTerrainSimRep.getNumXDataTiles()*gTerrainSimRep.getDataTileScale();

               int numRandomLights = 50;
               for(int i=0;i<numRandomLights;i++)
               {
                   XMFLOAT3 bv = XMFLOAT3(Math::fRand(0.0f, maxT), 5, Math::fRand(0.0f, maxT));

                  static bool spotLight = false;
                  int lightHandle = gSimSceneLightManager.createLocalLight();

                  gSimSceneLightManager.setLocalLightRadius(lightHandle, 10.0f);
                  gSimSceneLightManager.setLocalLightShadows(lightHandle, true);
                  BVec3 at(0,0,1);
                  BVec3 right(1,0,0);
                  gSimSceneLightManager.setLocalLightRight(lightHandle, (XMFLOAT3*)&right);
                  gSimSceneLightManager.setLocalLightAt(lightHandle, (XMFLOAT3*)&at);
                  gSimSceneLightManager.setLocalLightSpotInner(lightHandle, Math::fDegToRad(70.0f));
                  gSimSceneLightManager.setLocalLightSpotOuter(lightHandle, Math::fDegToRad(10.0f));      
                  gSimSceneLightManager.setLocalLightPos(lightHandle, &bv);
                  gSimSceneLightManager.setLocalLightEnabled(lightHandle,true);

                  BLocalLightParams& lightParams = gSimSceneLightManager.getLocalLightParams(lightHandle);
                  lightParams.setColor(XMVectorSet(3.0f, 2.8f, 3.0f, 0.0f));
                  lightParams.setDecayDist(250.0f);
                  lightParams.setFarAttenStart(.25f);
                  lightParams.setType(spotLight ? cLTSpot : cLTOmni);
                  lightParams.setSpecular(false);
                  lightParams.setLightBuffered(false);

                  gSimSceneLightManager.enforceLimits(lightHandle);
               }  
            }

               
            
            break;
         }
         case cKeyC:
         case cKeyX:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               int visMode = mSimVisMode + ((controlType == cKeyX) ? -1 : 1);
               if (visMode < cVMDisabled)
                  visMode = cVMNum - 1;
               else if (visMode >= cVMNum)
                  visMode = cVMDisabled;
               mSimVisMode = static_cast<eUGXGeomVisMode>(visMode);
               return true;
            }
            break;
         }
         case cKeyF:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               setFlag(cFlagDisableRenderParticles, !getFlag(cFlagDisableRenderParticles));
               gConsoleOutput.status("Particle rendering %s", getFlag(cFlagDisableRenderParticles) ? "Disabled" : "Enabled");
               return true;
            }
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyShift))
            {
               setFlag(cFlagDebugRenderParticleMagnets, !getFlag(cFlagDebugRenderParticleMagnets));
               gConsoleOutput.status("Particle Magnet rendering %s", getFlag(cFlagDebugRenderParticleMagnets) ? "Disabled" : "Enabled");
               gParticleGateway.enableMagnetRendering(getFlag(cFlagDebugRenderParticleMagnets));
               return true;
            }
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyAlt))
            {
               gTerrain.enableRenderFoliage(!gTerrain.isRenderFoliageEnable());
               gConsoleOutput.status("Foliage rendering %s", gTerrain.isRenderFoliageEnable() ? "Disabled" : "Enabled");
               return true;
            }
            break;
         }
         case cKeyO:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               gTerrainTexturing.toggleVisualizeCache();
               gConsoleOutput.status("Toggled terrain texture cache visualization");
               return true;
            }
            break;
         }
         case cKey0:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               gOcclusionManager.toggleDebugDraw();
               gConsoleOutput.status("Occlusion Buffer Debug Rendering %s", gOcclusionManager.isDebugDrawEnabled() ?  "Enabled" : "Disabled");
               return true;
            }
            else
            {
               gOcclusionManager.toggleOcclusionTestEnabled();
               
               gConsoleOutput.status("Occlusion Testing %s", gOcclusionManager.isOcclusionTestEnabled() ?  "Enabled" : "Disabled");
               return true;
            }
            break;
         }
         case cKeyK:
         {
            if (gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl))
            {
               gTerrain.toggleRenderSkirt();
               gConsoleOutput.status("Toggled skirt rendering");
               return true;
            }
            break;
         }
         case cKeyS:
         {
            if ((gInputSystem.getKeyboard()->isKeyActive(cKeyCtrl)) && (gInputSystem.getKeyboard()->isKeyActive(cKeyAlt)))
            {
               setFlag(cFlagDisableShadowRendering, !getFlag(cFlagDisableShadowRendering));
               gConsoleOutput.status("Shadow rendering %s", getFlag(cFlagDisableShadowRendering) ? "Disabled" : "Enabled");
            }
            break;
         }
      }
   }
#endif   
   
   return false;
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerUpdateTerrain
//--------------------------------------------------------------------------------------
void BRenderControl::workerUpdateTerrain(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if(!mDoWorkerRender)
      return;

   // rg [9/18/06] - NEVER render anything inside here! D3D::BeginScene() has not been called yet!

   // clm [05.17.07] These calls are NO LONGER IMMEDIATE!!!
   // they calls now batch up work to the worker pool. 
   // gTerrain.joinBatchWork() must be called before gTerrain.renderTile(..) so that the work is finished!!!!
   gTerrainVisual.beginEdgeTesselationCalc();
   
   gTerrain.evalSceneNodesLODDeferred(
      BTerrain::cRPVisible, 
      gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4(), 
      gRenderDraw.getWorkerSceneVolumeCuller(),
      true);

   //MUST be called to initiate the BCountDownEvent timer associated with this work!
   gTerrain.beginBatchShadowVis(gDirShadowManager.getNumPasses() + gLocalShadowManager.getNumPasses());

   for (uint shadowPassIndex = 0; shadowPassIndex < gDirShadowManager.getNumPasses(); shadowPassIndex++)
   {
      gTerrain.evalSceneNodesLODDeferred(
         static_cast<BTerrain::eRenderPass>(shadowPassIndex), 
         gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4(), 
         gDirShadowManager.getPassVolumeCuller(shadowPassIndex),
         false);
   }

   for (uint shadowPassIndex = 0; shadowPassIndex < gLocalShadowManager.getNumPasses(); shadowPassIndex++)
   {
      gTerrain.evalSceneNodesLODDeferred(
         BTerrain::cRPLocalShadowBuffer, 
         gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4(), 
         gLocalShadowManager.getPassVolumeCuller(shadowPassIndex),
         false);
   }

   gFoliageManager.update();
   gTerrainRibbonManager.update();
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerBeginViewport
//--------------------------------------------------------------------------------------
void BRenderControl::workerBeginViewport(void* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if(!mDoWorkerRender)
      return;

   // rg [9/18/06] - NEVER render anything inside here! D3D::BeginScene() has not been called yet!
      
   mUpdateData = *reinterpret_cast<const BUpdateData*>(pData);
   
#ifndef BUILD_FINAL

   mRenderThreadTimer.start();
  
   if (mUpdateData.mNewScenarioFlag)
      workerResetCPUUtilization();
#endif   


   // jce [11/4/2008] -- Moving particle update here, because workerStartOfFrame was too early with subupdating because
   // the interpolated positions were not updated yet.  Putting it here wouldn't be correct
   // if we still had split screen and we'd have to do something more complicated.
#ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderParticles))
#endif
   {
      gPSManager.updatePSysManTime(gRenderTime.getTotalGametime());
      gPSManager.updateFrame();
   }


#ifndef BUILD_FINAL
   if (mUpdateData.mGenerateCubemapFlag)
      workerGenerateCubemapBeginViewport();
#endif      
   
   if (mUpdateData.mGenerateCubemapFlag)
   {
      uint dimension = 512;
      if ((gTiledAAManager.getWidth() < 512) || (gTiledAAManager.getHeight() < 512))
         dimension = 256;
         
      gTiledAAManager.enableTiling(false, dimension, dimension);
   }
   else if (gRenderDraw.getNumViewports() > 1)
      gTiledAAManager.enableTiling(false, gRenderDraw.getViewportDesc(0).mWidth, gRenderDraw.getViewportDesc(0).mHeight, D3DMULTISAMPLE_2_SAMPLES); 
   else
   {
      const uint viewportWidth = gRenderDraw.getWorkerSceneRenderViewport().getViewport().Width;
      const uint viewportHeight = gRenderDraw.getWorkerSceneRenderViewport().getViewport().Height;
      
      if ( (viewportWidth != gTiledAAManager.getTilingWidth()) || 
           (viewportHeight != gTiledAAManager.getTilingHeight()) )
      {
         gTiledAAManager.enableTiling(false, viewportWidth, viewportHeight); 
      }
      else           
         gTiledAAManager.enableTiling(true);
   }
         
   //gpDebugPrimitives->addDebugLine(BVector(0,0,0), BVector(30,30,30), 0xFFFFFFFF, 0xFFFFFFFF, BDebugPrimitives::cCategoryNone, -1.0f);
   //gpDebugPrimitives->addDebugBox(BVector(0,0,0), BVector(20,20,20), 0xFFFFFFFF);
   
#if 0
static bool record = false;
static XMVECTOR baseMin, baseMax;
if (record)
{
   record = false;
   baseMin = gRenderDraw.getWorkerSceneVolumeCuller().getBaseMin();
   baseMax = gRenderDraw.getWorkerSceneVolumeCuller().getBaseMax();
}   
gpDebugPrimitives->addDebugBox(baseMin, baseMax);
#endif
     
   gTiledAAManager.beginFrame();

   gVisibleLightManager.update(mUpdateData.mWorldMin[1], mUpdateData.mWorldMax[1]);
         
   gUGXGeomInstanceManager.renderUpdate(mUpdateData.mGameTime);
         
   gUGXGeomInstanceManager.renderCreateSortableAttributes();
   
   gLocalShadowManager.shadowGenPrep(mUpdateData.mWorldMin, mUpdateData.mWorldMax, mUpdateData.mDeltaT);
               
   gVisibleLightManager.updateLightTextures();
   
   gUGXGeomInstanceManager.renderUpdateLocalShadowStatus();

   gUGXGeomInstanceManager.renderUpdateLocalReflectionStatus();
   
   gUGXGeomInstanceManager.renderSort();
   
   gUGXGeomInstanceManager.renderUpdatePackedTextures();

   BDirShadowManager::BShadowMode shadowMode = mUpdateData.mShadowMode;
   if ((mUpdateData.mMegaScreenshotFlag) && (shadowMode != BDirShadowManager::cSMHighestQuality))
      shadowMode = BDirShadowManager::cSMMegaScreenshot;
      
   gDirShadowManager.shadowGenPrep(mUpdateData.mWorldMin, mUpdateData.mWorldMax, shadowMode, !renderGetFlag(cFlagDisableShadowRendering));
      
   workerUpdateTerrain();
   
   BUGXGeomRender::setVisMode(mUpdateData.mVisMode);
      
#ifndef BUILD_FINAL
   BTerrain::setVisMode(mUpdateData.mVisMode);
   gRenderSceneLightManager.debugAddDebugPrims();   
#endif

   BUGXGeomRender::tickDebugBoundingBoxes(renderGetFlag(cFlagDebugRenderBoundingBoxes));
   
#ifndef BUILD_FINAL
   mRenderThreadTimer.stop();
   double statTime = mRenderThreadTimer.getElapsedMilliseconds();
   mBarChart.addBarValue(mRenderUpdateBarChartHandle,(float)statTime);
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerGenerateCubemapBeginViewport
//--------------------------------------------------------------------------------------
void BRenderControl::workerGenerateCubemapBeginViewport(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   if (mUpdateData.mGenerateCubemapFaceIndex == 0)
   {
      if (mpRenderCubemapGen)
         delete mpRenderCubemapGen;
         
      mpRenderCubemapGen = new BCubemapGen;
      mpRenderCubemapGen->init(512, 512, mUpdateData.mGenerateCubemapDim);
   }
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerGenerateHeightfield
//--------------------------------------------------------------------------------------
void BRenderControl::workerGenerateHeightfield(void)
{
   gTerrainHeightField.computeHeightField(1024, 1024);
         
   uint width = gTerrainHeightField.getMapWidth();
   uint height = gTerrainHeightField.getMapHeight();
   
   if ((!gTerrainHeightField.getHeightFieldTex()) || (!width) || (!height))
      return;
      
   BFile fileL;
   if (fileL.openWriteable(cDirProduction, "screenshots\\heightLow.r32", BFILE_OPEN_NORMAL | BFILE_OPEN_ENABLE_BUFFERING))
   {
      BFile fileH;
      if (fileH.openWriteable(cDirProduction, "screenshots\\heightHigh.r32", BFILE_OPEN_NORMAL | BFILE_OPEN_ENABLE_BUFFERING))
      {
         for (uint y = 0; y < height; y++)
         {
            for (uint x = 0; x < width; x++)
            {
               float lowNormZ = 0.0f, highNormZ = 0.0f;
               bool success = gTerrainHeightField.sampleHeights((int)x, (int)y, lowNormZ, highNormZ);
               success;

               if (cBigEndianNative)               
               {
                  EndianSwitchDWords(reinterpret_cast<DWORD*>(&lowNormZ), 1);
                  EndianSwitchDWords(reinterpret_cast<DWORD*>(&highNormZ), 1);
               }
               
               fileL.write(&lowNormZ, sizeof(lowNormZ));
               fileH.write(&highNormZ, sizeof(highNormZ));
            }
         }
      }         
   }      
   
   gTerrainHeightField.computeHeightField(256, 256);
   
   gConsoleOutput.output(cMsgConsole, "Wrote %ix%i terrain heightfield to screenshots\\heightLow.r32 and screenshots\\heightHigh.r32", width, height);
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerStartOfFrame
//--------------------------------------------------------------------------------------
void BRenderControl::workerStartOfFrame(void* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   //CLM [11.06.08]
   //Cinimatics need the ability to enable/disable rendering of render thread items at will
   //because they may interrupt us mid-frame, we defer the state change until the beginning of our frame
   //the below does that.
   InterlockedExchange((volatile LONG*)&mDoWorkerRender,mNextWorkerRenderState);
  
   // DO NOT access mUpdateData here yet. This is updated in workerBeginViewport.
      
   // Nothing should be allocated on the GPU frame heap here. If this assert fails we've leaked somewhere!
   BDEBUG_ASSERT(gGPUFrameHeap.getTotalResources() == 0);
   
#ifndef BUILD_FINAL
   //CLM [11.06.08] All maps should get this data via EXPORT, not runtime gen.
   if (pData)
      workerGenerateHeightfield();
#endif
                  
   gRenderDraw.beginScene();
   
   gRenderDraw.setDefaultRenderStates();
   gRenderDraw.setDefaultSamplerStates();
   
   if(mDoWorkerRender)
   {
      // Reset the shared intrinsic regs in case somebody overwrote them.
      gEffectIntrinsicManager.updateAllSharedIntrinsics();
      
      gpDebugPrimitives->beginFrame(gRenderTime.getTotalGametime());

      // jce [11/4/2008] -- particles used to update here, but it's too early with subupdating because
      // the interpolated positions are not updated yet.  It is now in workerBeginViewport which wouldn't be correct
      // if we still had split screen and we'd have to do something more complicated.
   }
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderReflection
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderReflection(void)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   if(!mDoWorkerRender)
      return;

   SCOPEDSAMPLE(WorkerRenderReflection);

   if (!gWaterManager.isReflectionBufferUpdateEnabled())
      return;
      



   // Debug disabling for perf investigation
   #ifndef BUILD_FINAL
      BWaterManager::eRenderMode reflRenderMode = gWaterManager.getRenderMode();
      if (reflRenderMode == BWaterManager::cRenderNone)
         return;
   #endif

   // Save off current fog params
   BFogParams oldTerrainFogParams, oldUnitsFogParams;
   oldTerrainFogParams = gRenderSceneLightManager.getFogParams(cLCTerrain);
   oldUnitsFogParams = gRenderSceneLightManager.getFogParams(cLCUnits);

   // Disable fog during reflection rendering
   BFogParams fogParams;
   fogParams.mZColor = BVec3(0,0,0); 
   fogParams.mZStart = 1000000.0f;
   fogParams.mZDensity = 0.0f;   
   fogParams.mPlanarColor = BVec3(0,0,0);
   fogParams.mPlanarStart = 0.0f;
   fogParams.mPlanarDensity = 0.0f;
   gRenderSceneLightManager.setFogParams(cLCTerrain, fogParams);
   gRenderSceneLightManager.setFogParams(cLCUnits, fogParams);

   // Setup
   gWaterManager.allocateReflectionBuffer();

   // Begin reflection rendering
   gWaterManager.reflectionRenderBegin();

   // Start terrain setup
   gTerrain.renderBegin(cTRP_Reflect);

   // Render reflected terrain
   #ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderTerrain) && (reflRenderMode & BWaterManager::cRenderTerrain))
   #endif
   {
      gRenderSceneLightManager.updateLightIntrinsics(cLCTerrain);

      gTerrain.renderCustomNoTile(
         cTRP_Reflect, 
         BTerrain::cRPVisible, 
         &gRenderDraw.getWorkerActiveMatrixTracker().getWorldCamPosVec4(), 
         &gRenderDraw.getWorkerActiveVolumeCuller(),
         false, //true, CLM [05.17.05] Reflections no longer consider skirts.
         false,
         2);   // LOD level of 2 will set the terrain to small chunks
   }

   // Render reflected units
   #ifndef BUILD_FINAL      
   if (!renderGetFlag(cFlagDisableRenderUnits) && (reflRenderMode & BWaterManager::cRenderUnits))
   #endif
   {
      gRenderSceneLightManager.updateLightIntrinsics(cLCUnits);
            
      gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
      gUGXGeomInstanceManager.renderFlushVisiblePrep(&gRenderDraw.getWorkerActiveVolumeCuller());
      
      #ifndef BUILD_FINAL      
      if (!renderGetFlag(cFlagDisableRenderSky))
      #endif
      {
         // Turn off alpha rendering
         BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE); 

         workerRenderFarLayerUnits(cUGXGeomPassMainReflect);

         // Restore alpha rendering
         BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL); 
      }
                     
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOpaque, cUGXGeomPassMainReflect, cUGXGeomLayerOpaque, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupAdditive, cUGXGeomPassMainReflect, cUGXGeomLayerAdditive, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOver, cUGXGeomPassMainReflect, cUGXGeomLayerOver, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOverallAlpha, cUGXGeomPassOverallAlphaReflect, cUGXGeomLayerAllVisible, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
   }

   // End terrain setup
   gTerrain.renderEnd(cTRP_Reflect);

   // End reflection rendering
   gWaterManager.reflectionRenderEnd();

   // Restore fog settings
   gRenderSceneLightManager.setFogParams(cLCTerrain, oldTerrainFogParams);
   gRenderSceneLightManager.setFogParams(cLCUnits, oldUnitsFogParams);
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderFlashComponents
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderFlashComponents(void)
{

   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(WorkerRenderFlashComponents);

   gUIManager->workerRenderMinimapIconResources();

   if(!mDoWorkerRender)
      return;
  
   gUIManager->workerRenderDecals();
   
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderShadow
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderShadow(void)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   if(!mDoWorkerRender)
      return;

   SCOPEDSAMPLE(WorkerRenderShadow);

   gTerrain.renderBegin(cTRP_ShadowGen);   
            
   if (gDirShadowManager.getNumPasses())
   {
      gDirShadowManager.allocateShadowBuffers();
      
      gDirShadowManager.shadowGenInit();
                  
      for (uint shadowPassIndex = 0; shadowPassIndex < gDirShadowManager.getNumPasses(); shadowPassIndex++)
      {
         PIXBeginNamedEventNoColor("DirShadowPass%i", shadowPassIndex);

         gDirShadowManager.shadowGenBeginPass(shadowPassIndex);

         gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   #ifndef BUILD_FINAL      
         if ((!renderGetFlag(cFlagDisableRenderUnits)) && (!renderGetFlag(cFlagDisableShadowRendering)))
   #endif      
         {
            gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
            gUGXGeomInstanceManager.renderFlush(cUGXGeomPassShadowGen, cUGXGeomLayerOpaque | cUGXGeomLayerOver, &gDirShadowManager.getPassVolumeCuller(shadowPassIndex));
         }

         gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   #ifndef BUILD_FINAL      
         if ((!renderGetFlag(cFlagDisableRenderTerrain)) && (!renderGetFlag(cFlagDisableShadowRendering)))
   #endif      
         {
            gTerrain.renderCustomNoTile(cTRP_ShadowGen, (BTerrain::eRenderPass)shadowPassIndex, NULL, NULL, false, false);
         }

         gDirShadowManager.shadowGenEndPass(shadowPassIndex);

         PIXEndNamedEvent();
      }
      
      gDirShadowManager.shadowGenDeinit();
      
      BD3D::mpDev->InsertFence();
   }      

   if (gLocalShadowManager.getNumPasses())
   {
      gLocalShadowManager.shadowGenInit();

      for (uint shadowPassIndex = 0; shadowPassIndex < gLocalShadowManager.getNumPasses(); shadowPassIndex++)
      {
         PIXBeginNamedEventNoColor("LocalShadowPass%i", shadowPassIndex);

         bool dualParaboloid;
         gLocalShadowManager.shadowGenBegin(shadowPassIndex, dualParaboloid);

         gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

#ifndef BUILD_FINAL      
         if ((!renderGetFlag(cFlagDisableRenderUnits)) && (!renderGetFlag(cFlagDisableShadowRendering)))
#endif
         {
            gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
            gUGXGeomInstanceManager.renderFlushLocalLightInstances(
               dualParaboloid ? cUGXGeomPassDPShadowGen : cUGXGeomPassShadowGen, 
               cUGXGeomLayerOpaque | cUGXGeomLayerOver, 
               shadowPassIndex);
         }

         gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
         
         // rg [6/27/07] - Need to update terrain renderer to support DP shadowgen.
         if (!dualParaboloid)
         {
   #ifndef BUILD_FINAL      
            if ((!renderGetFlag(cFlagDisableRenderTerrain)) && (!renderGetFlag(cFlagDisableShadowRendering)))
   #endif
            {
               gTerrain.renderCustomNoTile(
                  cTRP_ShadowGen, 
                  BTerrain::cRPLocalShadowBuffer, 
                  &gRenderDraw.getWorkerSceneMatrixTracker().getWorldCamPosVec4(), 
                  &gLocalShadowManager.getPassVolumeCuller(shadowPassIndex),
                  false,
                  true);
            }
         }            

         gLocalShadowManager.shadowGenEnd(shadowPassIndex);

         PIXEndNamedEvent();
      }

      gLocalShadowManager.shadowGenDeinit();
      
      BD3D::mpDev->InsertFence();
   }
   
   gTerrain.renderEnd(cTRP_ShadowGen);
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderFarLayerUnits
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderFarLayerUnits(eUGXGeomPass pass)
{
   SCOPEDSAMPLE(workerRenderFarLayerUnits);
   
   if(!mDoWorkerRender)
      return;

   gRenderDraw.setRenderState(D3DRS_ZENABLE, TRUE);
   gRenderDraw.setRenderState(D3DRS_ZWRITEENABLE, FALSE);

   D3DVIEWPORT9 curViewport;
   BD3D::mpDev->GetViewport(&curViewport);
   
   // Hack the viewport so anything rendered in here will always be very far away, behind everything else.
   D3DVIEWPORT9 newViewport(curViewport);
   newViewport.MinZ = 65535.0/65536.0;
   BD3D::mpDev->SetViewport(&newViewport);

   gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
   gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupFar, pass, cUGXGeomLayerOpaque, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
   gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupFar, pass, cUGXGeomLayerBlended, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);

   BD3D::mpDev->SetViewport(&curViewport);

   gRenderDraw.setRenderState(D3DRS_ZENABLE, TRUE);
   gRenderDraw.setRenderState(D3DRS_ZWRITEENABLE, TRUE);
}   

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderNearLayer
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderNearLayer(uint tileIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   SCOPEDSAMPLE(WorkerRenderNearLayer);

   if(!mDoWorkerRender)
      return;

   if (gUGXGeomInstanceManager.getNumNearLayerInstances() <= 0)
      return;

#if 0
   // Set origin camera (world matrix)
   BMatrixTracker& matrixTracker = gRenderDraw.getWorkerActiveMatrixTracker();
   XMMATRIX prevWorldMtx = matrixTracker.getMatrix(cMTWorldToView);
   XMMATRIX worldToView = XMMatrixIdentity();
   matrixTracker.setMatrix(cMTWorldToView, worldToView);
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
#endif   

   // Clear depth / stencil
   gRenderDraw.clear(D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER);

#ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderUnits))
#endif   
   {
      // Set UI lighting up
      gRenderSceneLightManager.updateLightIntrinsics(cLCUnits);

      // Render units
      gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupNear, cUGXGeomPassMain, cUGXGeomLayerOpaque, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupNear, cUGXGeomPassMain, cUGXGeomLayerBlended, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAll, 0);
   }      
   
   // Render particles
#ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderParticles))
#endif
   {      
      gPSManager.renderNearLayer(tileIndex);            
   }

#if 0
   // Restore original world matrix
   matrixTracker.setMatrix(cMTWorldToView, prevWorldMtx);
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
#endif   
}   

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderVisibleMeshes
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderVisibleMeshes(IDirect3DDevice9* pDev, bool setCommandBufferPredication)
{
   if(!mDoWorkerRender)
      return;

   const DWORD flags = setCommandBufferPredication ? BUGXGeomInstanceManager::cRFFSetCommandBufferRunPredication : 0;
   
   if (!gRenderControl.mUpdateData.mEnableObscuredUnits)
   {
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOpaque,       cUGXGeomPassMain,          cUGXGeomLayerOpaque,     BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAboveDecals, flags);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupAdditive,     cUGXGeomPassMain,          cUGXGeomLayerAdditive,   BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAboveDecals, flags);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOver,         cUGXGeomPassMain,          cUGXGeomLayerOver,       BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAboveDecals, flags);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOverallAlpha, cUGXGeomPassOverallAlpha,  cUGXGeomLayerAllVisible, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDAboveDecals, flags);
   }      
   else
   {
      //pDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
      //pDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
      //pDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_DESTALPHA); 
      
      
      // Clear stencil because we use it for decals.
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOpaque,       cUGXGeomPassMain,          cUGXGeomLayerOpaque,     BUGXGeomInstanceManager::cOFNotObscurable, BUGXGeomInstanceManager::cBDAboveDecals, flags);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupAdditive,     cUGXGeomPassMain,          cUGXGeomLayerAdditive,   BUGXGeomInstanceManager::cOFNotObscurable, BUGXGeomInstanceManager::cBDAboveDecals, flags);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOver,         cUGXGeomPassMain,          cUGXGeomLayerOver,       BUGXGeomInstanceManager::cOFNotObscurable, BUGXGeomInstanceManager::cBDAboveDecals, flags);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOverallAlpha, cUGXGeomPassOverallAlpha,  cUGXGeomLayerAllVisible, BUGXGeomInstanceManager::cOFNotObscurable, BUGXGeomInstanceManager::cBDAboveDecals, flags);

      

      //CLM [11.15.08] There's some wierdness
      if (gUGXGeomInstanceManager.getNumObscurableInstances() )
      {
         // Render obscurable instances.
         pDev->Clear(0, NULL, D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);
         
         pDev->SetRenderState(D3DRS_STENCILENABLE, TRUE);                      
         pDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
         pDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
         pDev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_REPLACE);
         pDev->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

         pDev->SetRenderState(D3DRS_HISTENCILENABLE, FALSE);
         pDev->SetRenderState(D3DRS_HISTENCILWRITEENABLE, TRUE);
         pDev->SetRenderState(D3DRS_HISTENCILREF, 0); 
         pDev->SetRenderState(D3DRS_HISTENCILFUNC, D3DHSCMP_EQUAL);
         pDev->SetRenderState(D3DRS_HISTENCILFUNC, D3DHSCMP_EQUAL);
         
         pDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
         pDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
         pDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO);
         pDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO); 

         //const uint totalDrawCountStart = BUGXGeomRender::getTotalDraws();
         gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOpaque,       cUGXGeomPassMain,          cUGXGeomLayerOpaque,     BUGXGeomInstanceManager::cOFObscurable, BUGXGeomInstanceManager::cBDAboveDecals, BUGXGeomInstanceManager::cRFFSetStencilRefToTeamColor | flags);
         gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupAdditive,     cUGXGeomPassMain,          cUGXGeomLayerAdditive,   BUGXGeomInstanceManager::cOFObscurable, BUGXGeomInstanceManager::cBDAboveDecals, BUGXGeomInstanceManager::cRFFSetStencilRefToTeamColor | flags);
         gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOver,         cUGXGeomPassMain,          cUGXGeomLayerOver,       BUGXGeomInstanceManager::cOFObscurable, BUGXGeomInstanceManager::cBDAboveDecals, BUGXGeomInstanceManager::cRFFSetStencilRefToTeamColor | flags);

         // Don't draw obscured overall alpha instances into the stencil buffer (i.e. don't actually obscure them, just draw them here for the purposes of proper sorting)
         pDev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
         gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOverallAlpha, cUGXGeomPassOverallAlpha,  cUGXGeomLayerAllVisible, BUGXGeomInstanceManager::cOFObscurable, BUGXGeomInstanceManager::cBDAboveDecals, BUGXGeomInstanceManager::cRFFSetStencilRefToTeamColor | flags);
      
         //const uint totalDrawCountEnd = BUGXGeomRender::getTotalDraws();
         // MPB [11/13/2008] reviewed/checked in this change from Mikey (mikey@xbox.com).
         // HACK: Bogus draw call which forces D3D to flush pending render states
         //       Otherwise, the Clear call above will corrupt state IFF it’s the last
         //       command in our command buffer
         // CLM [[11.15.08] For some reason having this call be the first one ALWAYS
         // after the clear caused some GPU deadlock issues to arise. I've moved it here
         // to be called ONLY if nothing was rendered during the obstruction pass. This appears
         // to fix the GPU deadlocks, and the stencil tiling issues.
         // MPB [11/18/2008] - Our bogus draw call needs proper vertex/pixel shaders set up here,
         // along with a vertex declaration to go with it.  It was using whatever shaders
         // were set up before, and when the vertex shader was using memexport this was causing
         // a GPU hang when using command buffers, and D3D errors plus a crash when not using
         // command buffers.
         // MPB [12/5/2008] - Removed this dummy draw call because a hopefully more correct fix
         // was put in place.  In the BeginCommandBuffer call we added the D3DBEGINCB_RECORD_ALL_SET_STATE
         // flag so that state settings after the final draw call will be recorded and played back.  This
         // way it won't leave stuff in a messed up state even if a draw didn't happen after the clear
         // above.  UPDATE - One thing was found to be off even with the flag and that was viewport height
         // of the second (upper) tile.  This was corrected by an additional SetViewport call after the command
         // buffer is run (in dcbManager).
         /*
         if(totalDrawCountStart == totalDrawCountEnd)
         {
            BPVertex* pVB;
            pDev->SetVertexDeclaration(BPVertex::msVertexDecl);

            pDev->SetVertexShader(gFixedFuncShaders.workerGetVertexShader(cPosVS));
            pDev->SetPixelShader(gFixedFuncShaders.workerGetPixelShader(cWhitePS));

            pDev->BeginVertices(D3DPT_TRIANGLELIST, 3, sizeof(BPVertex), (void **) &pVB);
               memset(pVB, 0, 3 * sizeof(BPVertex));
            pDev->EndVertices();

            pDev->SetVertexShader(NULL);
            pDev->SetPixelShader(NULL);
            pDev->SetVertexDeclaration(NULL);
         }
         */
      } 
   }      
}   

//#define DEBUG_TRACE_RECORD
#ifdef DEBUG_TRACE_RECORD
#include "tracerecording.h"
#include "xbdm.h"
#pragma comment( lib, "tracerecording.lib" )
#endif

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderMeshInstancesForkCallback
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderMeshInstancesForkCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   privateData0;
   privateData1;
   workBucketIndex;
   lastWorkEntryInBucket;
   
   gUGXGeomInstanceManager.renderFlushVisiblePrep(&gRenderDraw.getWorkerSceneVolumeCuller());
   
   IDirect3DDevice9* pDev = gUGXGeomInstanceManager.renderFlushDCBForkCallbackBegin();
   
#ifdef DEBUG_TRACE_RECORD
   static bool bTraceDump = false;
   if (bTraceDump)
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\psUpdate.bin" );
   }
#endif   
   
   gRenderControl.workerRenderVisibleMeshes(pDev, true);

#ifdef DEBUG_TRACE_RECORD
   if (bTraceDump)
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDump = false;
   }
#endif
         
   gUGXGeomInstanceManager.renderFlushDCBForkCallbackEnd();
   
   gRenderControl.mRenderMeshInstancesForkFinished.set();
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderMeshInstancesBeginFrame
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderMeshInstancesBeginFrame(void)
{
   if(!mDoWorkerRender)
      return;

#ifndef BUILD_FINAL      
   if (renderGetFlag(cFlagDisableRenderUnits))
      return;
#endif
   
   if (renderGetFlag(cFlagDisableMeshDCBRendering))
      return;
   
   gRenderSceneLightManager.updateLightIntrinsics(cLCUnits);
         
   if (mUpdateData.mEnableObscuredUnits)
      BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE |  D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
   else
      BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
      
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, renderGetFlag(cFlagDebugRenderWireframe) ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
   
   gUGXGeomInstanceManager.renderFlushDCBBeginFork(gTiledAAManager.getTileRenderTarget(), gTiledAAManager.getTileDepthStencilSurface(), true);
   
   BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
   
   mRenderMeshInstancesForkEventChecked = false;
      
   gWorkDistributor.flush();
   gWorkDistributor.queue(workerRenderMeshInstancesForkCallback, NULL, 0);
   gWorkDistributor.flush();
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderMeshInstancesJoin
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderMeshInstancesJoin(void)
{
   if(!mDoWorkerRender)
      return;

   if (renderGetFlag(cFlagDisableMeshDCBRendering))
      return;
      
   if (!mRenderMeshInstancesForkEventChecked)
   {
      mRenderMeshInstancesForkEventChecked = true;
      
      SCOPEDSAMPLE(BRenderControl_renderMeshInstancesJoin)
      
      BD3D::mpDev->InsertFence();
      
      gWorkDistributor.waitSingle(mRenderMeshInstancesForkFinished);
      
      gUGXGeomInstanceManager.renderFlushDCBEndFork();
   }   
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderMeshInstancesEndFrame
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderMeshInstancesEndFrame(void)
{
   if(!mDoWorkerRender)
      return;

#ifndef BUILD_FINAL      
   if (renderGetFlag(cFlagDisableRenderUnits))
      return;
#endif

   if (renderGetFlag(cFlagDisableMeshDCBRendering))
      return;
      
   BDEBUG_ASSERT(mRenderMeshInstancesForkEventChecked);
     
   gUGXGeomInstanceManager.renderFlushDCBEnd();
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderMeshInstances
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderMeshInstances(uint tileIndex, bool belowDecals)
{
   if(!mDoWorkerRender)
      return;

#ifndef BUILD_FINAL      
   if (renderGetFlag(cFlagDisableRenderUnits))
      return;
#endif

   SCOPEDSAMPLE(WorkerRenderMeshInstances);

   BDEBUG_ASSERT(mRenderMeshInstancesForkEventChecked);
   
   gRenderSceneLightManager.updateLightIntrinsics(cLCUnits);

   if (belowDecals)
   {
#ifndef BUILD_FINAL      
      if (!renderGetFlag(cFlagDisableRenderSky))
#endif
      {
         workerRenderFarLayerUnits(cUGXGeomPassMain);
      }  
       
      gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOpaque,       cUGXGeomPassMain,          cUGXGeomLayerOpaque,     BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDBelowDecals, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupAdditive,     cUGXGeomPassMain,          cUGXGeomLayerAdditive,   BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDBelowDecals, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOver,         cUGXGeomPassMain,          cUGXGeomLayerOver,       BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDBelowDecals, 0);
      gUGXGeomInstanceManager.renderFlushVisibleGroup(BUGXGeomInstanceManager::cGroupOverallAlpha, cUGXGeomPassOverallAlpha,  cUGXGeomLayerAllVisible, BUGXGeomInstanceManager::cOFAll, BUGXGeomInstanceManager::cBDBelowDecals, 0); 
   }
   else
   {
      if (renderGetFlag(cFlagDisableMeshDCBRendering))
         workerRenderVisibleMeshes(BD3D::mpDev, false);   
      else
         gUGXGeomInstanceManager.renderFlushDCBRun(tileIndex);
            
      if ((mUpdateData.mEnableObscuredUnits) && (gUGXGeomInstanceManager.getNumObscurableInstances()))
      {
         // Render obscurable instances.
                  
         //BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
         BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE |  D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
         BD3D::mpDev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);   

         BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_DESTALPHA);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVDESTALPHA);   
         BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, TRUE);

         BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, TRUE);                      
         BD3D::mpDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILWRITEENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILREF, 0);
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILFUNC, D3DHSCMP_EQUAL);

         BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);       

         // rg [10/4/07] - I know this code causes a PIX error- I asked xds about it, and they should we should be OK.
         BD3D::mpDev->FlushHiZStencil(D3DFHZS_SYNCHRONOUS);

         const RECT& rect = gTiledAAManager.getTileRect(tileIndex);

         for (uint playerColorIndex = gUGXGeomInstanceManager.getMinPlayerColorIndex(); playerColorIndex <= gUGXGeomInstanceManager.getMaxPlayerColorIndex(); playerColorIndex++)
         {
            if (!gUGXGeomInstanceManager.getPlayerColorIndexUsed(playerColorIndex))
               continue;

            BD3D::mpDev->SetRenderState(D3DRS_STENCILREF, playerColorIndex + 1);

            DWORD playerColor = 0xFFFFFFFF;
            if (playerColorIndex < mUpdateData.mNumTeamColors)
               playerColor = mUpdateData.mTeamColors[playerColorIndex];

            BPrimDraw2D::drawSolidRect2D(0, 0, rect.right - rect.left + 1, rect.bottom - rect.top + 1, 0.0f, 0.0f, 1.0f, 1.0f, playerColor, 0, cPosDiffuseVS, cDiffusePS);
         }            

         BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
         BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
         BD3D::mpDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);  

         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILWRITEENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILREF, 0);
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILFUNC, D3DHSCMP_EQUAL);               
         BD3D::mpDev->SetRenderState(D3DRS_HISTENCILWRITEENABLE, FALSE);

         BD3D::mpDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILREF, 0);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
         BD3D::mpDev->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);

         BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
         BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
         BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);   
         BD3D::mpDev->SetRenderState(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE);
      }         
   }      
}   

//--------------------------------------------------------------------------------------
// BRenderControl::workerRenderVisible
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderVisible(void)
{

   ASSERT_THREAD(cThreadIndexRender);
   
   if(!mDoWorkerRender)
      return;

   SCOPEDSAMPLE(WorkerRenderVisible);

#if defined(ENABLE_TIMELINE_PROFILER) && defined(ENABLE_GPU_PROFILER)
   if (gBProfileSystem.isGPUSamplingEnabled() == false)
   {            
      gBProfileSystem.setDevice(BD3D::mpDev);     
   }
#endif

      workerRenderMeshInstancesBeginFrame();

   if (mUpdateData.mUserMode != BUser::cUserModeCinematic)
      workerRenderFlashComponents();


   gTiledAAManager.beginTiling();
         
   gTerrain.renderBegin(cTRP_Full);

   // CLM do ONE composite pass for the entire screen..
   gTerrain.compositeVisibleNodes();
         
#ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderParticles))
#endif
   {      
      gPSManager.updateRenderList(gViewportManager.getCurrentViewport());

      gPSManager.renderBegin();
      gPSManager.renderNearLayerBegin();
   }
   
   gDecalManager.renderSetZBiases(0.0f, -0.00199f);
   gDecalManager.renderDrawDecalsBegin();
      
   const BVec3 backgroundColorVec3(gRenderSceneLightManager.getBackgroundColor() * gRenderSceneLightManager.getBackgroundIntensity());
   
   // Alpha is .25f for the obscured unit pass.
   const D3DVECTOR4 backgroundColor = XMVectorSet(backgroundColorVec3[0], backgroundColorVec3[1], backgroundColorVec3[2], .25f);
   
   const DWORD cStencilClearValue = 0;

   for (uint tileIndex = 0; tileIndex < gTiledAAManager.getNumTiles(); tileIndex++)
   {
#ifndef BUILD_FINAL   
      BFixedString64 buf(cVarArg, "RenderTile %u", tileIndex);
      BScopedPIXNamedEvent PIXNamedEvent(buf);
#endif      
      
      gTiledAAManager.beginTile(tileIndex, &backgroundColor, cStencilClearValue);
      
      if (mUpdateData.mEnableObscuredUnits)
         BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE |  D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
         
      gRenderDraw.setRenderState(D3DRS_FILLMODE, renderGetFlag(cFlagDebugRenderWireframe) ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
                 
#ifndef BUILD_FINAL      
      if (!renderGetFlag(cFlagDisableRenderTerrain))
#endif
      {
         gRenderSceneLightManager.updateLightIntrinsics(cLCTerrain);
         gTerrain.renderTileBegin(cTRP_Full, tileIndex);
      }
      
      workerRenderMeshInstancesJoin();
      
      gUGXGeomInstanceManager.renderFlushVisiblePrep(&gRenderDraw.getWorkerActiveVolumeCuller());

#ifndef BUILD_FINAL            
      if (!renderGetFlag(cFlagDisableRenderTerrain))
#endif      
      {
         gRenderSceneLightManager.updateLightIntrinsics(cLCTerrain);
         gTerrain.renderTileEnd(cTRP_Full, tileIndex);
      }
      
      // Render meshes that appear below decals.
      workerRenderMeshInstances(tileIndex, true);
      
      // Render lit decals that conform to the terrain
      gLitDecalManager.renderDrawLitDecalsBeginTile(tileIndex);
      gLitDecalManager.renderDrawAlphaLitDecals(tileIndex, -1, BLitDecalManager::cRFConformToTerrain, false);
      gLitDecalManager.renderDrawLitDecalsEndTile(tileIndex);


      // Render opaque decals, and all decals that conform to the terrain.
      gDecalManager.renderDrawDecalsBeginTile(tileIndex);
      gDecalManager.renderDrawNormalDecals(tileIndex, -1, BDecalManager::cRFAll, false);
      gDecalManager.renderDrawAlphaDecals(tileIndex, -1, BDecalManager::cRFConformToTerrain, false);
      gDecalManager.renderDrawStencilDecals(tileIndex, -1, BDecalManager::cRFConformToTerrain, false);
      gDecalManager.renderDrawDecalsEndTile(tileIndex);


      // Render meshes that appear above decals.
      workerRenderMeshInstances(tileIndex, false);

      BD3D::mpDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
      
      // Render non-conforming decals.
      gDecalManager.renderDrawDecalsBeginTile(tileIndex);
      gDecalManager.renderDrawAlphaDecals(tileIndex, -1, BDecalManager::cRFNonConformToTerrain, false);
      gDecalManager.renderDrawStencilDecals(tileIndex, -1, BDecalManager::cRFNonConformToTerrain, false);
      gDecalManager.renderDrawDecalsEndTile(tileIndex);
      
      if (!tileIndex)
         gRenderThread.blockUntilGPUPresent();
            
      gRenderDraw.setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
                  
      // Render debug primitives
      gpDebugPrimitives->endFrame();
      gpDebugPrimitives->workerRender();

#ifndef BUILD_FINAL
      if (mRenderTerrainHeightfield)
      {
         // Render terrain heightfield
         gRenderDraw.setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
         
         gTerrainHeightField.renderDebugHeightField();
         
         gRenderDraw.setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
      }
#endif         

      // Resolve depth
      gTiledAAManager.endTileDepth(tileIndex, NULL, true);

       // Render particles                              
#ifndef BUILD_FINAL
      if (!renderGetFlag(cFlagDisableRenderParticles))
#endif
      {
         gPSManager.render(tileIndex);
      }

      // Render near layer with cleared out depth
	   workerRenderNearLayer(tileIndex);
            
      // Resolve color
      gTiledAAManager.endTileScene(tileIndex, NULL, &backgroundColor, true, cStencilClearValue);
   }
   
   workerRenderMeshInstancesEndFrame();
   
   gDecalManager.renderDrawDecalsEnd();

#ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderParticles))
#endif
   {
      gPSManager.renderNearLayerEnd();
      gPSManager.renderEnd();
   }

   gTiledAAManager.endTiling();

#if !DIR_SHADOW_MANAGER_DEBUG
   gDirShadowManager.releaseShadowBuffers();
#endif   

   //free the cache handles for our terrain.
   gTerrain.freeHeldCompositeTextures();   
   gTerrain.renderEnd(cTRP_Full);

   #ifndef BUILD_FINAL
      if (!gWaterManager.isDebugDrawEnabled())
   #endif
         gWaterManager.releaseReflectionBuffer();
   
   if (mUpdateData.mHDRScreenshotFlag)
   {
      gRender.screenShot(mUpdateData.mHDRScreenshotFilename.c_str(), true);
      mUpdateData.mHDRScreenshotFlag = false;
   }
      
   if (mUpdateData.mGenerateCubemapFlag)
      workerGenerateCubemapCaptureColorBuffer();

#ifndef BUILD_FINAL
   if (!renderGetFlag(cFlagDisableRenderParticles))
#endif
   {
      //-- Render non tiled particles here
      if (gPSManager.getRenderNoTilingParticles())
      {
         gPSManager.renderNoTilingBegin();
         gPSManager.renderNoTiling();
         gPSManager.renderNoTilingEnd();
      }
   }

   const BOOL bDistortionEnabled = gPSManager.isDistortionEnabled() || 
      ((gUGXGeomInstanceManager.getSceneLayerFlags() & cUGXGeomLayerDistortion) != 0);
      
   if (bDistortionEnabled)
   {
      // Distortion Particle Rendering
      gPSManager.renderDistortionBegin();
      
      if (!renderGetFlag(cFlagDisableRenderParticles))
         gPSManager.renderDistortion();
      
      gUGXGeomInstanceManager.renderFlushUpdateIntrinsics();
      gUGXGeomInstanceManager.renderFlush(cUGXGeomPassDistortion, cUGXGeomLayerDistortion, &gRenderDraw.getWorkerActiveVolumeCuller(), false, false);
      
      gPSManager.renderDistortionEnd();
   }
            
   const bool resetAdaptation = mUpdateData.mNewScenarioFlag;
   const bool disableAdaptation = mUpdateData.mDisableAdaptation;
   bool unswizzleRadianceBuffer = gPSManager.getFlagUseAliasedFillSurface() && gPSManager.getRenderNoTilingParticles();

#ifndef BUILD_FINAL
   if (renderGetFlag(cFlagDisableRenderParticles))
      unswizzleRadianceBuffer = false;
#endif
     
   gToneMapManager.tonemap(bDistortionEnabled ? gPSManager.getDistortionTexture() : NULL, resetAdaptation, disableAdaptation, 0, 0, unswizzleRadianceBuffer, gRenderDraw.getViewportIndex());
   
   gPSManager.releaseGPUFrameHeapResources();
   
   if (gRenderDraw.getViewportIndex() > 0)
   {
      BD3D::mpDev->SetTexture(0, gViewportManager.getSplitscreenTex());
      BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
      BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
      BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
      BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

      gRenderDraw.pushAndSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);      
      gRenderDraw.pushAndSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
      gRenderDraw.pushAndSetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
      gRenderDraw.pushAndSetRenderState(D3DRS_ZENABLE, FALSE);
      gRenderDraw.pushAndSetRenderState(D3DRS_HALFPIXELOFFSET, FALSE);
      
      const BRenderDraw::BViewportDesc& viewportDesc = gRenderDraw.getViewportDesc(1);

      BPrimDraw2D::drawSolidRect2D(viewportDesc.mXOfs, viewportDesc.mYOfs, viewportDesc.mXOfs + viewportDesc.mWidth, viewportDesc.mYOfs + viewportDesc.mHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF);
            
      BD3D::mpDev->SetTexture(0, NULL);
      
      gRenderDraw.popRenderState(D3DRS_HALFPIXELOFFSET);
      gRenderDraw.popRenderState(D3DRS_ZENABLE);
      gRenderDraw.popRenderState(D3DRS_ALPHATESTENABLE);
      gRenderDraw.popRenderState(D3DRS_ALPHABLENDENABLE);
      gRenderDraw.popRenderState(D3DRS_CULLMODE);
   }
                        
   //mRenderFont.DrawText(60, 60, 0xFFFFFFFF, L"Hello, world!");

#ifndef BUILD_FINAL
   gTerrain.debugDraw();
   gTerrainTexturing.debugDraw(mRenderFont);
   gDirShadowManager.debugDraw(mRenderFont);
   gLocalShadowManager.debugDraw(mRenderFont);
   gRenderSceneLightManager.debugDraw(mRenderFont);
   if (gWaterManager.isDebugDrawEnabled())
   {
      gWaterManager.debugDraw(mRenderFont);
      gWaterManager.releaseReflectionBuffer();
   }
  // gOcclusionManager.debugDraw(mRenderFont);
#endif

#if DIR_SHADOW_MANAGER_DEBUG   
   // If dir shadow manager debugging is enabled, release the buffers here because we're done rendering them to the screen as sprites.
   gDirShadowManager.releaseShadowBuffers();
#endif
}

//--------------------------------------------------------------------------------------
// BRenderControl::workerRender
//--------------------------------------------------------------------------------------
void BRenderControl::workerRenderViewport(void* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
      
   if(!mDoWorkerRender)
   {
      //CLM [11.18.08] Insert a black clear here so that we don't get corruption between transitions in/out of WOW moments.
      gRenderDraw.clear(D3DCLEAR_TARGET,0x00000000);
      return;
   }

   SCOPEDSAMPLE(WorkerRender);
#ifndef BUILD_FINAL
   mRenderThreadTimer.start();
#endif

   //gOcclusionManager.generateOcclusionBuffer();
      
   {
      SCOPEDSAMPLE(BRenderControl_terrainJoinBatchWork);
      gTerrain.defragmentCaches();
      gTerrain.joinBatchWork();           
   }
                        
   workerRenderShadow();
   
   workerRenderReflection();

   workerRenderVisible();
   
#ifndef BUILD_FINAL
   mRenderThreadTimer.stop();
   double statTime = mRenderThreadTimer.getElapsedMilliseconds();
   mBarChart.addBarValue(mRenderDrawBarChartHandle,(float)statTime);
#endif
}

//--------------------------------------------------------------------------------------
// class BChangeSHFillLightEventPayload
//--------------------------------------------------------------------------------------
class BChangeSHFillLightEventPayload : public BEventPayload
{
   BSpectralSHCoefficients mSHCoeffs;
   
public:
   BChangeSHFillLightEventPayload(const BSpectralSHCoefficients& shCoeffs)
   {
      memcpy(&mSHCoeffs, shCoeffs, sizeof(mSHCoeffs));
   }
   
   const BSpectralSHCoefficients& getSHCoeffs(void) const { return mSHCoeffs; }

   virtual void deleteThis(bool delivered)
   {
      delete this;
   }
};

//--------------------------------------------------------------------------------------
// BRenderControl::workerGenerateCubemapCaptureColorBuffer
//--------------------------------------------------------------------------------------
void BRenderControl::workerGenerateCubemapCaptureColorBuffer(void)
{
   BDEBUG_ASSERT(mpRenderCubemapGen);
   mpRenderCubemapGen->captureFace(gTiledAAManager.getColorTexture(), mUpdateData.mGenerateCubemapFaceIndex);
   
   if (mUpdateData.mGenerateCubemapFaceIndex == 5)
   {
      SYSTEMTIME systemTime;
      GetLocalTime(&systemTime);

      char systemName[256] = "F ";
#ifndef BUILD_FINAL
      DWORD size = sizeof(systemName);
      DmGetXboxName(systemName, &size);
#endif
      
      BFixedString256 filename;
      filename.format("screenshots\\%s %02u-%02u %02u-%02u-%02u.hdr", systemName, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
      
      bool success = mpRenderCubemapGen->saveToHDRFile(0, filename);
      if (success)
         gConsoleOutput.resource("Saved HDR cubemap to file: %s.", filename.c_str());
      else
         gConsoleOutput.error("Failed writing HDR cubemap!");
         
      if (mUpdateData.mGenerateCubemapSH)
      {
         SphericalHarmonic::Vec9Vector shCoeffs;
         mpRenderCubemapGen->calculateSHCoefficients(shCoeffs);
         
         SphericalHarmonic::Vec9Vector irradSHCoeffs(SphericalHarmonic::cosineConvolution(shCoeffs, false));
         mpRenderCubemapGen->unprojectSHCoefficients(irradSHCoeffs, 64);
         
         filename.format("screenshots\\%s %02u-%02u %02u-%02u-%02u irrad.hdr", systemName, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);

         success = mpRenderCubemapGen->saveToHDRFile(0, filename);
         if (success)
            gConsoleOutput.status("Saved irradiance HDR cubemap to file: %s.", filename.c_str());
         else
            gConsoleOutput.error("Failed writing HDR irradiance cubemap!");
            
         BSpectralSHCoefficients spectralSHCoeffs;
         spectralSHCoeffs[0] = shCoeffs[0];
         spectralSHCoeffs[1] = shCoeffs[1];
         spectralSHCoeffs[2] = shCoeffs[2];
         BChangeSHFillLightEventPayload* pPayload = new BChangeSHFillLightEventPayload(spectralSHCoeffs);
         gEventDispatcher.send(mEventHandle, mSimEventHandle, cEventClassChangeSHFillLight, 0, 0, pPayload, BEventDispatcher::cSendSynchronousDispatch);

         gEventDispatcher.send(mEventHandle, mSimEventHandle, cEventClassSaveSHFillLight, 0, 0, 0, BEventDispatcher::cSendSynchronousDispatch);
      }         
                                    
      delete mpRenderCubemapGen;
      mpRenderCubemapGen = NULL;
   }
}

#ifndef BUILD_FINAL
//--------------------------------------------------------------------------------------
// BRenderControl::workerTickVideoCapture
//--------------------------------------------------------------------------------------
void BRenderControl::workerTickVideoCapture(void)
{
   if (mpRenderGPUDXTVideo)
   {
      mpRenderGPUDXTVideo->capture();
   }
}
#endif // BUILD_FINAL

//--------------------------------------------------------------------------------------
// BRenderControl::workerEndFrame
//--------------------------------------------------------------------------------------
void BRenderControl::workerEndFrame(void* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
   pData;

   if(mDoWorkerRender)
   {
      
   #ifndef BUILD_FINAL
      if (!renderGetFlag(cFlagDisableRenderParticles))
   #endif
      {
         gPSManager.renderEndOfFrame();
         gPSManager.renderDistortionEndOfFrame();
         gPSManager.renderNearLayerEndOfFrame();
         gPSManager.renderNoTilingEndOfFrame();   
      
         gPSManager.releaseDynamicGPUResources();
      }      
     
   }
   BD3D::mpDev->UnsetAll();
             
   gRenderDraw.endScene();
   
#ifndef BUILD_FINAL            
   workerTickVideoCapture();
#endif      
      
   gRenderDraw.present(NULL, NULL);
      
   // CLM wait until the GPU is idle to do these commands
   gTerrainDeformer.flushQueuedFlattenCommands();

#ifndef BUILD_FINAL
   gFlashManager.refreshStats();
#endif


}

//--------------------------------------------------------------------------------------
// BRenderControl::screenshot
//--------------------------------------------------------------------------------------
void BRenderControl::screenshot(const char* pFilename, bool hdr)
{
   if (!hdr)
      gRender.screenShot(pFilename, false);
   else
   {
      mSimHDRScreenshotFilename.set(pFilename);
      mSimHDRScreenshotFlag = true;
   }
}

//--------------------------------------------------------------------------------------
// BRenderControl::megaScreenshot
//--------------------------------------------------------------------------------------
bool BRenderControl::megaScreenshot(const char* pFilename, uint cols, uint rows, bool doJitterVersion, uint jitterAAQuality)
{
   if ((mSimMegaScreenshotFlag) || (mSimGenerateCubemapFlag) || (gViewportManager.isSplitScreen()))
      return false;
      
   cols = Math::Clamp<uint>(cols, 1, 24);
   rows = Math::Clamp<uint>(rows, 1, 24);      

   BDEBUG_ASSERT(pFilename);
   mSimMegaScreenshotFilename.set(pFilename);
   mSimMegaScreenshotCols = static_cast<ushort>(cols);
   mSimMegaScreenshotRows = static_cast<ushort>(rows);
   mSimNumMacroTiles = static_cast<ushort>(cols * rows);
   mSimMegaScreenshotFlag = true;

   mSimMegaScreenshotJitterAAQuality = jitterAAQuality;
   mSimMegaScreenshotUseJitter = doJitterVersion;
   if(mSimMegaScreenshotUseJitter)
      mSimNumMacroTiles = (ushort)(mSimNumMacroTiles*mSimMegaScreenshotJitterAAQuality);   //CLM in jitter mode, add more render passes

   return true;
}

//--------------------------------------------------------------------------------------
// BRenderControl::generateHeightfield
//--------------------------------------------------------------------------------------
void BRenderControl::generateHeightfield(void)
{
   mSimGenerateHeightfield = true;
}

//--------------------------------------------------------------------------------------
// BRenderControl::newScenario
//--------------------------------------------------------------------------------------
void BRenderControl::newScenario(void)
{
   mSimNewScenarioFlag = true;
}

//--------------------------------------------------------------------------------------
// BRenderControl::initDeviceData
//--------------------------------------------------------------------------------------
void BRenderControl::initDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   gpDebugPrimitives->init();
         
   mRenderFont.Create(cDirFonts, "courier_10.xpr");
}

//--------------------------------------------------------------------------------------
// BRenderControl::frameBegin
//--------------------------------------------------------------------------------------
void BRenderControl::frameBegin(void)
{
   ASSERT_THREAD(cThreadIndexRender);
}

//--------------------------------------------------------------------------------------
// BRenderControl::processCommand
//--------------------------------------------------------------------------------------
void BRenderControl::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
}

//--------------------------------------------------------------------------------------
// BRenderControl::frameEnd
//--------------------------------------------------------------------------------------
void BRenderControl::frameEnd(void)
{
   ASSERT_THREAD(cThreadIndexRender);

#ifndef BUILD_FINAL   
   BDynamicGPUBuffer* pDynamicGPUBuffers[2] = { gPSManager.getDynamicGPUBuffer(), BFlashRender::getDynamicGPUBuffer() };
   const uint cNumDynamicGPUBuffers = sizeof(pDynamicGPUBuffers)/sizeof(pDynamicGPUBuffers[0]);

   BASSERT(gPSManager.getDynamicGPUBuffer()->getLocked() != TRUE);
   BASSERT(BFlashRender::getDynamicGPUBuffer()->getLocked() != TRUE);

   for (uint i = 0; i < cNumDynamicGPUBuffers; i++)
   {
      if (!pDynamicGPUBuffers[i])
         continue;

      BDynamicGPUBuffer::BStats& stats = pDynamicGPUBuffers[i]->getStats();
      stats.updateMaxEverStats();
      stats.clearFrameStats();
   }
#endif   
}

//--------------------------------------------------------------------------------------
// BRenderControl::deinitDeviceData
//--------------------------------------------------------------------------------------
void BRenderControl::deinitDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   delete mpRenderCubemapGen;
   mpRenderCubemapGen = NULL;
   
   mRenderFont.Destroy();
   
   gpDebugPrimitives->deInit();
      
#ifndef BUILD_FINAL   
   if (mpRenderGPUDXTVideo)
   {
      HEAP_DELETE(mpRenderGPUDXTVideo, gRenderHeap);
      mpRenderGPUDXTVideo = NULL;
   }

#endif   


}

//--------------------------------------------------------------------------------------
// BRenderControl::receiveEvent
//--------------------------------------------------------------------------------------
bool BRenderControl::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch(threadIndex)
   {
      case cThreadIndexRender:
      {
         break;
      }
      
      case cThreadIndexSim:
      {
         switch (event.mEventClass)
         {  
            case cEventClassChangeSHFillLight:
            {
               const BChangeSHFillLightEventPayload* pPayload = reinterpret_cast<const BChangeSHFillLightEventPayload*>(event.mpPayload);
               
               BSHLightParams shLightParams;
               shLightParams.set(pPayload->getSHCoeffs());
               
               gSimSceneLightManager.setSHFillLight(cLCTerrain, shLightParams);
               gSimSceneLightManager.setSHFillLight(cLCUnits, shLightParams);
               
               break;
            }
            case cEventClassSaveSHFillLight:
            {
               gScenario.saveSHFillLight(mSimFLSSaveAsFileName);
            }
            break;
         }
         
         break;
      }
   }
   return false;
}


