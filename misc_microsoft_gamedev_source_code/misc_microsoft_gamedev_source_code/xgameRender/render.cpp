//==============================================================================
// render.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xgameRender.h"
#include "render.h"

// xcore   
#include "debugText.h"
#include "xexception\xdebugtext.h"
#include "memory\alignedAlloc.h"
#include "math\runningAverage.h"
#include "globalObjects.h"
#include "viewportManager.h"

// xsystem
#include "bfileStream.h"
#include "reloadManager.h"
#include "configsgamerender.h"

// xrender
#include "renderInit.h"
#include "effectIntrinsicManager.h"

// ximage
#include "writeHDR.h"

// local
#include "vertexTypes.h"
#include "debugprimitives.h"
#include "sceneLightManager.h"
#include "visibleLightManager.h"
#include "dirShadowManager.h"
#include "localShadowManager.h"
#include "ugxGeomManager.h"
#include "ugxGeomInstanceManager.h"
#include "binkInterface.h"
#include "flashbackgroundplayer.h"
#include "miniLoadManager.h"
#include "render2Dprimitiveutility.h"

#include "consoleOutput.h"
#include "tiledAA.h"
#include "gammaRamp.h"
#include "toneMapManager.h"
#include "decalManager.h"
#include "occlusion.h"
#include "worldVisibility.h"
#include "FontSystem2.h"
#include "lightVisualManager.h"

// terrain
#include "terrain.h"

// xinput
#include "inputsystem.h"
#include "keyboard.h"

BRender gRender;

const float cNearClip = .25f;
const float cFarClip = 32000.0f;

//#define DISABLE_DEBUG_PRINTING

#if defined(BUILD_FINAL) || defined(DISABLE_DEBUG_PRINTING)
   #define DBG_PRINT_INIT(l)
   #define DBG_PRINT(s) 
#else
   #define DBG_PRINT_INIT(l) uint dbgPrintY = l;
   #define DBG_PRINT(s) do { gConsoleOutput.output(cMsgDebug, s); BDebugText::renderRaw(0, dbgPrintY, s); dbgPrintY++; } while(0)
#endif

// GPU frame heap size
#if DIR_SHADOW_MANAGER_DEBUG
   const uint cGPUFrameHeapSize = 19*1024*1024;
#else
   const uint cGPUFrameHeapSize = 15*1024*1024; //13
#endif

//============================================================================
// BRender::BRender
//============================================================================
BRender::BRender() :
   mpLastFrameLengthFilter(NULL)
{
   clear();
}

//============================================================================
// BRender::~BRender
//============================================================================
BRender::~BRender()
{
}

//============================================================================
// BRender::clear
//============================================================================
void BRender::clear(void)
{
   Utils::ClearObj(mDeviceInfo);

   mInitialized = false;
   mStarted = false;
   
   mTextureManagerBaseDirID = 0;
   mEffectCompilerDefaultDirID = 0;   
   
   mTimeLoop = 0.0f;
      
   mWorldMatrix = XMMatrixIdentity();
   
   QueryPerformanceCounter((LARGE_INTEGER*)&mLastBeginFrameTime);
   mLastFrameLength = 0.0f;
   
   if (mpLastFrameLengthFilter)
      delete mpLastFrameLengthFilter;
   mpLastFrameLengthFilter = NULL;
   
   mDebugPrintY = 0;
}

//============================================================================
// BRender::create
//============================================================================
void BRender::create(void)
{
   destroy();
   
   mViewParams.clear();

   mInitialized = true;
}

//============================================================================
// BRender::destroy
//============================================================================
void BRender::destroy(void)
{
   if (!mInitialized)
      return;
      
   MEMORYSTATUS status;
   GlobalMemoryStatus( &status );
   char str[256];
   //sprintf_s(str, sizeof(str), "BRender::destroy: Free physical: %4.1fMB", status.dwAvailPhys / (1024.0f * 1024.0f));
   sprintf_s(str, sizeof(str), "Free physical: %4.1fMB, Total Allocated: %4.1fMB", status.dwAvailPhys / (1024.0f * 1024.0f), (gInitialKernelMemoryTracker.getInitialPhysicalFree() - status.dwAvailPhys)  / (1024.0f * 1024.0f) );

   startupStatus(str);
   
   DBG_PRINT_INIT(0);
     
   DBG_PRINT("BRender::destroy: Teardown 0"); 
   
   gRenderThread.submitCallback(renderThreadDestroy);
   gRenderThread.blockUntilWorkerIdle();
   
   gLightVisualManager.deinit();

   gFontManager.destroyAllMaterials();
   gFontManager.destroyAllFonts();
   
   BWorldVisibility::destroyInstance();
         
   DBG_PRINT("BRender::destroy: Teardown 1"); 
   
   if (mStarted)
   {      
      if (mpLastFrameLengthFilter)
      {
         delete mpLastFrameLengthFilter;
         mpLastFrameLengthFilter = NULL;
      }
      
      DBG_PRINT("BRender::destroy: Teardown 2");  
                        
      gGammaRamp.deinit();
      
      gToneMapManager.deinit();
                        
      gTiledAAManager.deinit();
      
      gLocalShadowManager.deinit();
      
      gDirShadowManager.deinit();
      
      DBG_PRINT("BRender::destroy: Teardown 3");  
      
      gVisibleLightManager.deinit();
      gSimSceneLightManager.deinit();
      gRenderSceneLightManager.deinit();
            
      DBG_PRINT("BRender::destroy: Teardown 4");  
      
      gDecalManager.deinit();
      
      gTerrain.deinit();

      gFlashBackgroundPlayer.deinit();
      gMiniLoadManager.deinit();

      gBinkInterface.deinit();

      gOcclusionManager.destroy();
            
      gUGXGeomInstanceManager.deinit();
      gUGXGeomManager.deinit();
                        
      DBG_PRINT("BRender::destroy: Teardown 5");  

      if (gpDebugPrimitives)
      {
         gpDebugPrimitives->deInit();

         delete gpDebugPrimitives;
         gpDebugPrimitives = NULL;
      }

      gRender2DPrimitiveUtility.deInit();
      
      DBG_PRINT("BRender::destroy: Teardown 6");                   
      
      BVertexTypes::releaseVertexDeclarations();
      
      gViewportManager.deinit();
      
      commandListenerDeinit();
      
      DBG_PRINT("BRender::destroy: Teardown 7"); 
            
      gRenderInitializer.deinit();
   }
   
   mInitialized = false;   
   mStarted = false;
}

//============================================================================
// BRender::start
//============================================================================
bool BRender::start(
   const BD3D::BCreateDeviceParams& createDeviceParams,
   long startupDirID,
   long textureManagerBaseDirID, 
   long effectCompilerDefaultDirID,
   long fontsDirID,
   long dataDirID)
{
   BDEBUG_ASSERT(mInitialized);
   
   if (mStarted)
      return true; 
      
   mStarted = true;
   
   TRACEMEM
      
   mTextureManagerBaseDirID = textureManagerBaseDirID;
   mEffectCompilerDefaultDirID = effectCompilerDefaultDirID;   
   mFontsDirID = fontsDirID;
   
   uint gpuFrameHeapSize = cGPUFrameHeapSize;
#ifndef BUILD_FINAL   
   if (gConfig.isDefined(cConfigHigherQualityShadows))
      gpuFrameHeapSize += 18U*1024U*1024U;
#endif      
   
   gRenderInitializer.init(createDeviceParams, &mDeviceInfo, textureManagerBaseDirID, effectCompilerDefaultDirID, 0, 0, 0, gpuFrameHeapSize);
      
   commandListenerInit();
   
   gViewportManager.init();
                        
   BVertexTypes::createVertexDeclarations();
   
   if (!gpDebugPrimitives)
   {
      gpDebugPrimitives = new BDebugPrimitives;
      gpDebugPrimitives->init();
   }

   gRender2DPrimitiveUtility.init();
   
   TRACEMEM
   
   gUGXGeomManager.init(effectCompilerDefaultDirID);
   
   TRACEMEM
   
   gUGXGeomInstanceManager.init();
   
   TRACEMEM
   
   gTerrain.init();
   
   TRACEMEM

   gDecalManager.init();
   
   TRACEMEM
   
   gSimSceneLightManager.init();
   
   TRACEMEM
   
   gRenderSceneLightManager.init();
   
   TRACEMEM
   
   gVisibleLightManager.init();
   
   TRACEMEM
         
   gDirShadowManager.init();
   
   TRACEMEM
   
   gLocalShadowManager.init();
   
   TRACEMEM

   gBinkInterface.init(mFontsDirID);

   TRACEMEM

   gFlashBackgroundPlayer.init(mFontsDirID);

   TRACEMEM   

   // 3 tiles = 4x MSAA
   uint numAATiles = 3; 
   if (gConfig.isDefined(cConfigNumAATiles))
   {
      long configVal = (long)numAATiles;
      gConfig.get(cConfigNumAATiles, &configVal);
      numAATiles = configVal;
   }
   const D3DFORMAT edramFormat = D3DFMT_A2B10G10R10F_EDRAM; //D3DFMT_A16B16G16R16_EDRAM,
   const D3DFORMAT colorFormat = D3DFMT_A16B16G16R16F_EXPAND;
   const D3DFORMAT depthFormat = D3DFMT_D24S8;
   gTiledAAManager.init(
      createDeviceParams.mBackBufferWidth, 
      createDeviceParams.mBackBufferHeight, 
      numAATiles, 
      edramFormat, 
      colorFormat, 
      depthFormat);
   
   TRACEMEM
   
   gToneMapManager.init();
   
   TRACEMEM
   
   gGammaRamp.init();
               
   gGammaRamp.load(startupDirID, "gammaRamp.xml");
                             
   // Wait for subsystems to finish initializing themselves on the worker thread.
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpUntilThreadQueueEmpty(cThreadIndexRender);
   
   const float cFOV = 45.0f * cRadiansPerDegree;
   mViewParams.setViewportAndProjection(0, 0, createDeviceParams.mBackBufferWidth, createDeviceParams.mBackBufferHeight, cNearClip, cFarClip, cFOV);   
   mAspectRatioMode = createDeviceParams.mAspectRatioMode;
   
   if (mpLastFrameLengthFilter)
      delete mpLastFrameLengthFilter;
   mpLastFrameLengthFilter = new BRunningAverageD(60);
   
   TRACEMEM
   
   BWorldVisibility::createInstance();
   BWorldVisibility::getInstance().init(dataDirID);      
   
   TRACEMEM
   
   // Font manager
   gFontManager.setBaseDirectoryID(fontsDirID);  
   gFontManager.setDefaultFont("Arial 10");
   gFontManager.createFontsFromXML();
   gFontManager.initFontHandles();

   TRACEMEM

   gLightVisualManager.init();
   
   TRACEMEM
   
   gRenderThread.submitCallback(renderThreadStart);
   gRenderThread.blockUntilWorkerIdle();
   
   TRACEMEM
         
   return true;   
}

//============================================================================
// BRender::renderThreadStart
//============================================================================
void BRender::renderThreadStart(void* pData)
{
   BGPUDXTPack::createInstance();
}

//============================================================================
// BRender::renderThreadDestroy
//============================================================================
void BRender::renderThreadDestroy(void* pData)
{
   BGPUDXTPack::destroyInstance();
}

//============================================================================
// BRender::screenShot
//============================================================================
bool BRender::screenShot(const char* pFilename, bool hdr)
{
#ifdef BUILD_FINAL         
   return false;
#else   
   if (!gRender.getInitialized())
      return false;
      
   BString filename;

   if ((!pFilename) || (strlen(pFilename) == 0))
   {
      SYSTEMTIME systemTime;
      GetLocalTime(&systemTime);
      
      char systemName[256] = "Unknown ";
#ifndef BUILD_FINAL
      DWORD size = sizeof(systemName);
      DmGetXboxName(systemName, &size);
#endif
            
      filename.format("screenshots\\%s %02u-%02u %02u-%02u-%02u", systemName, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
   }
   else
   {
      filename.set("screenshots\\");
      filename += pFilename;
   }

   filename.toLower();
   
   BString name, ext;
   strPathGetFilename(filename, name);
   strPathGetExtension(name, ext);
   if (ext.isEmpty())
   {
      if (hdr)
         filename += ".hdr";
      else if (gConfig.isDefined(cConfigJPEGScreenshots))
         filename += ".jpg";
      else
         filename += ".tga";
   }      
         
   if (hdr)
   {
      if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
      {
         const char* pFilenamePtr = filename.getPtr();
         gRenderThread.submitCommand(mCommandHandle, cRenderCommandHDRScreenshot, sizeof(const char*), &pFilenamePtr); 
      
         gRenderThread.blockUntilWorkerIdle();
      }
      else
         workerSaveHDRScreenshot(filename);
   }
   else
      gRenderDraw.saveScreenshot(filename);
         
   return true;
#endif   
}

//============================================================================
// BRender::updateIntrinsics
//============================================================================
void BRender::updateIntrinsics(float deltaT)
{
   // Update timeloop intrinsic.
   mTimeLoop += deltaT;
   mTimeLoop = fmod(mTimeLoop, 100.0f);

   float timeloopIntrinsic = mTimeLoop * 1.0f/100.0f;

   gEffectIntrinsicManager.set(cIntrinsicTimeLoop, &timeloopIntrinsic, cIntrinsicTypeFloat);
}

//============================================================================
// BRender::updateDebugKeys
//============================================================================
void BRender::updateDebugKeys(void)
{
#ifndef BUILD_FINAL   
   // rg - Many hacks here.
//-- FIXING PREFIX BUG ID 6641
   const BKeyboard* pKeyboard = gInputSystem.getKeyboard();
//--

   if (!pKeyboard)
      return;
   
   float yaw = 0.0f;
   float pitch = 0.0f;
   float inten = 0.0f;

   if (pKeyboard->isKeyActive(cKey4))
      yaw = -1;
   else if (pKeyboard->isKeyActive(cKey5))
      yaw = 1;

   if (pKeyboard->isKeyActive(cKey6))
      pitch = -1;
   else if (pKeyboard->isKeyActive(cKey7))
      pitch = 1;

   if (pKeyboard->isKeyActive(cKey8))
      inten = -.05f;
   else if (pKeyboard->isKeyActive(cKey9))
      inten = .05f;

   if ((yaw) || (pitch))
   {
      static float yawSpeed = 1.2f;
      static float pitchSpeed = .5f;
      
      yaw *= yawSpeed;
      pitch *= pitchSpeed;
      
      XMMATRIX yawMat = XMMatrixRotationY(Math::fDegToRad(yaw));
            
      for (uint i = cLCTerrain; i <= cLCUnits; i++)
      {
         BDirLightParams params = gSimSceneLightManager.getDirLight(static_cast<eLightCategory>(i));
      
         XMVECTOR sunDir = params.mDir;
         
         sunDir = XMVector3TransformNormal(sunDir, yawMat);
         
         XMVECTOR up = XMVectorSet(0,1,0,0);
         float d = XMVector3Dot(sunDir, up).x;
         if (fabs(d) >= .99f)
            up = XMVectorSet(1,0,0,0);
         
         XMVECTOR axis = XMVector3Normalize(XMVector3Cross(sunDir, up));
         
         XMMATRIX pitchMat = XMMatrixRotationNormal(axis, Math::fDegToRad(pitch));
         
         sunDir = XMVector3TransformNormal(sunDir, pitchMat);
         
         params.mDir = sunDir;
         
         gSimSceneLightManager.setDirLight(static_cast<eLightCategory>(i), params);
      }   
   }
   
   if (inten)
   {
      for (uint i = cLCTerrain; i <= cLCUnits; i++)
      {
         BDirLightParams params = gSimSceneLightManager.getDirLight(static_cast<eLightCategory>(i));
         params.mColor += XMVectorReplicate(inten);
         params.mColor = XMVectorClamp(params.mColor, XMVectorZero(), XMVectorReplicate(250.0f));
         gSimSceneLightManager.setDirLight(static_cast<eLightCategory>(i), params);
      }   
   }
#endif   
}

//============================================================================
// BRender::updateSceneMatricesAndViewport
// viewportIndex==-1 for full-screen UI
//============================================================================
void BRender::updateSceneMatricesAndViewport(int viewportIndex)
{
   gRenderDraw.setViewportIndex(viewportIndex);

   if (viewportIndex < 0)
   {
      BMatrix cameraMatrix;
      cameraMatrix.makeIdentity();
      mViewParams.setViewMatrix(cameraMatrix);
      mViewParams.setOffCenterFactor(0.0f, 0.0f, 1.0f, 1.0f);

      const D3DVIEWPORT9& sceneViewport = gViewportManager.getBackBufferViewport();
      mViewParams.setViewportAndProjection(sceneViewport.X, sceneViewport.Y, sceneViewport.Width, sceneViewport.Height, cNearClip, cFarClip, 45.0f * cRadiansPerDegree);
      BMatrixTracker& matrixTracker = gRenderDraw.getMainSceneMatrixTracker();

      matrixTracker.setMatrix(cMTWorldToView, XMLoadFloat4x4((const XMFLOAT4X4*)&mViewParams.getViewMatrix()));
      matrixTracker.setMatrix(cMTViewToProj, XMLoadFloat4x4((const XMFLOAT4X4*)&mViewParams.getProjectionMatrix()));
      matrixTracker.setViewport(mViewParams.getDeviceViewport());

      BRenderViewport& renderViewport = gRenderDraw.getMainSceneRenderViewport();
      renderViewport.setViewport(mViewParams.getDeviceViewport());
      renderViewport.setSurf(0, gRenderDraw.getDevBackBuffer());
      renderViewport.setDepthStencilSurf(gRenderDraw.getDevDepthStencil());
   }
   else
   {
      mViewParams = gViewportManager.getViewportViewParams(viewportIndex);
      
      const BRenderDraw::BViewportDesc& viewportDesc = gRenderDraw.getViewportDesc(viewportIndex);
      
      // The scene matrix tracker/viewport should only be updated once per frame (or multiple times for split screen).
      // This also updates the effect intrinsics and active culling manager on the main/worker threads.
      gRenderDraw.setMainSceneMatrixTracker(viewportDesc.mMatrixTracker);
      gRenderDraw.setMainSceneRenderViewport(viewportDesc.mRenderViewport);
      gRenderDraw.setMainSceneVolumeCuller(viewportDesc.mVolumeCuller);
   }  
   
   gRenderDraw.resetMainActiveMatricesAndViewport();
      
   // This is a dummy world matrix used to help legacy code that assumes the device tracks a global world matrix.   
   mWorldMatrix = XMMatrixIdentity();
   
#ifdef BUILD_DEBUG
   const float* pMatrix = mViewParams.getViewMatrix();
   for (uint i = 0; i < 16; i++)
   {
      BDEBUG_ASSERT(Math::IsValidFloat(pMatrix[i]));
   }

   pMatrix = mViewParams.getProjectionMatrix();
   for (uint i = 0; i < 16; i++)
   {
      BDEBUG_ASSERT(Math::IsValidFloat(pMatrix[i]));
   }
#endif
}

//============================================================================
// BRender::updateAverageFPS
//============================================================================
void BRender::updateAverageFPS(void)
{
   uint64 curTime;
   QueryPerformanceCounter((LARGE_INTEGER*)&curTime);
   
   uint64 freq;
   QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
   mLastFrameLength = (curTime - mLastBeginFrameTime) / (double)freq;
   
   mLastBeginFrameTime = curTime;
   
   if (mpLastFrameLengthFilter)
      mpLastFrameLengthFilter->addSample(mLastFrameLength);
}

//============================================================================
// BRender::resetAverageFPS
//============================================================================
void BRender::resetAverageFPS(void)
{
   if (mpLastFrameLengthFilter)
      mpLastFrameLengthFilter->clear();
   
   QueryPerformanceCounter((LARGE_INTEGER*)&mLastBeginFrameTime);
}

//============================================================================
// BRender::updateViewports
//============================================================================
void BRender::updateViewports(void)
{
   const uint numViewports = gViewportManager.getNumViewports();
   BRenderDraw::BViewportDesc viewports[BViewportManager::cMaxViewports];
   
   BDEBUG_ASSERT(Math::IsInRange<uint>(numViewports, 1, 2));
   
   for (uint viewportIndex = 0; viewportIndex < numViewports; viewportIndex++)
   {
      BMatrixTracker matrixTracker;
      BRenderViewport renderViewport;
      BVolumeCuller volumeCuller;
      
      const BRenderViewParams& viewParams = gViewportManager.getViewportViewParams(viewportIndex);
      const D3DVIEWPORT9& sceneViewport = gViewportManager.getSceneViewport(viewportIndex);
      const D3DVIEWPORT9& uiViewport = gViewportManager.getUIViewport(viewportIndex);
      sceneViewport;
                  
      matrixTracker.setMatrix(cMTWorldToView, XMLoadFloat4x4((const XMFLOAT4X4*)&viewParams.getViewMatrix()));
      matrixTracker.setMatrix(cMTViewToProj, XMLoadFloat4x4((const XMFLOAT4X4*)&viewParams.getProjectionMatrix()));
      matrixTracker.setViewport(viewParams.getDeviceViewport());
            
      renderViewport.setViewport(viewParams.getDeviceViewport());
      renderViewport.setSurf(0, gRenderDraw.getDevBackBuffer());
      renderViewport.setDepthStencilSurf(gRenderDraw.getDevDepthStencil());
      
      volumeCuller.setBasePlanes(matrixTracker.getWorldFrustum());
                  
      BRenderDraw::BViewportDesc& desc = viewports[viewportIndex];
      desc.mXOfs = uiViewport.X;
      desc.mYOfs = uiViewport.Y;
      desc.mWidth = uiViewport.Width;
      desc.mHeight = uiViewport.Height;
      desc.mRenderViewport = renderViewport;
      desc.mMatrixTracker = matrixTracker;
      desc.mVolumeCuller = volumeCuller;
   }
   
   gRenderDraw.setViewportIndex(0);   
   gRenderDraw.setViewports(numViewports, gViewportManager.isVerticalSplit(), viewports);
}   

//============================================================================
// BRender::beginFrame
//============================================================================
void BRender::beginFrame(float deltaT)
{
   SCOPEDSAMPLE(BRender_beginFrame);
   
   updateViewports();
   
   updateAverageFPS();
      
   {
      // rg [6/22/06] - I don't like putting this here. Should be handled via a waitable timer somewhere.
      
      SCOPEDSAMPLE(BRender_beginFrame_ReloadLogs);
      gReloadManager.flushNotifications();
      gFileManager.flushLogMessages();
   }
         
   updateDebugKeys();
   
   updateIntrinsics(deltaT);
}

//============================================================================
// BRender::beginViewport
//============================================================================
void BRender::beginViewport(int viewportIndex)
{
   gRenderThread.frameBegin();
   
   updateSceneMatricesAndViewport(viewportIndex);
}

//============================================================================
// BRender::endViewport
//============================================================================
void BRender::endViewport()
{
   if ((gViewportManager.getCurrentViewport() >= 0) && (!gViewportManager.isLastViewport()))
      gRenderThread.frameEnd();

   gRenderThread.kickCommands();
}

//============================================================================
// BRender::beginFullScreenUI
//============================================================================
void BRender::beginFullScreenUI()
{
   updateSceneMatricesAndViewport(-1);
}

//============================================================================
// BRender::endFullScreenUI
//============================================================================
void BRender::endFullScreenUI()
{
   if (gViewportManager.getNumViewports())
   {
      // Set viewport 0 as active so the sim can query it on the next frame.
      updateSceneMatricesAndViewport(gViewportManager.getUserViewportIndex(0));
   }
}

//============================================================================
// BRender::frameEnd
//============================================================================
void BRender::endFrame(void)
{
   gRenderThread.frameEnd();
      
   gRenderThread.kickCommands();
}

//============================================================================
// BRender::startupStatus
//============================================================================
void BRender::startupStatus(const BCHAR_T *fmt, ...)
{
#if !defined(BUILD_FINAL) && !defined(DISABLE_DEBUG_PRINTING)
   BString temp;
   va_list va;
   va_start(va, fmt);
   temp.formatArgs(fmt, va);
   va_end(va);
      
   trace(temp);

#if 0   
   while (static_cast<uint>(temp.length()) < G_debug_text.get_num_cells_x())
      temp += B(" ");

   if (gRenderThread.getHasD3DOwnership())
      gRenderThread.blockUntilGPUIdle();

   BDebugText::renderRaw(0, 0, temp.getPtr());   
#endif   

#endif   
}

//============================================================================
// BRender::debugPrintClear
//============================================================================
void BRender::debugPrintClear(void)
{
   gRenderThread.blockUntilGPUIdle();
   
   gRenderThread.frameBegin();
   gRenderDraw.beginScene();
   gRenderDraw.clear(D3DCLEAR_TARGET, 0, 1.0f, 0);
   gRenderDraw.present();
   gRenderDraw.endScene();
   gRenderThread.frameEnd();
   
   mDebugPrintY = 0;
}

//============================================================================
// BRender::debugPrintSetY
//============================================================================
void BRender::debugPrintSetY(uint y)
{
   mDebugPrintY = y;
}

//============================================================================
// BRender::debugPrintf
//============================================================================
void BRender::debugPrintf(const BCHAR_T *fmt, ...)
{
#if !defined(BUILD_FINAL) && !defined(DISABLE_DEBUG_PRINTING)
   if (!mStarted)
      return;

   BString temp;
   va_list va;
   va_start(va, fmt);
   temp.formatArgs(fmt, va);
   va_end(va);
   
   gRenderThread.blockUntilGPUIdle();

   BDebugText::renderRaw(0, mDebugPrintY, temp.getPtr());   
   
   mDebugPrintY++;
#endif   
}

//============================================================================
// BRender::getAverageFPS
//============================================================================
double BRender::getAverageFPS(void)
{
   if (!mpLastFrameLengthFilter)
      return 0.0f;
   
   const double t = mpLastFrameLengthFilter->getAverage();
   if (t)
      return 1.0f / t;
      
   return 0.0f;
}

//============================================================================
// BRender::getAverageFrameTime
//============================================================================
double BRender::getAverageFrameTime(void)
{
   if (!mpLastFrameLengthFilter)
      return 0.0f;

   return mpLastFrameLengthFilter->getAverage();
}

//============================================================================
// BRender::getAverageFrameTime
//============================================================================
double BRender::getMaxFrameTime(void)
{
   if (!mpLastFrameLengthFilter)
      return 0.0f;

   return mpLastFrameLengthFilter->getMaximum();
}

//============================================================================
// BRender::initDeviceData
//============================================================================
void BRender::initDeviceData(void)
{
   if (!gpDebugPrimitives)
   {
      gpDebugPrimitives = new BDebugPrimitives;
      gpDebugPrimitives->init();

      // make a copy of the render thread's debug primitive pointer so we can use it in helper threads
      gpRenderDebugPrimitives = gpDebugPrimitives;
   }
}

//============================================================================
// BRender::workerSaveHDRScreenshot
//============================================================================
bool BRender::workerSaveHDRScreenshot(const char* pFilename)
{
   const bool status = workerSaveHDRScreenshotInternal(pFilename);
   
   if (status)
      gConsoleOutput.output(cMsgConsole, "Saved HDR screenshot to: %s\n", pFilename);
   else
      gConsoleOutput.output(cMsgConsole, "Error: Unable to save HDR screenshot!");
      
   return status;
}      

//============================================================================
// BRender::workerSaveHDRScreenshotInternal
//============================================================================
bool BRender::workerSaveHDRScreenshotInternal(const char* pFilename)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (!gTiledAAManager.isInitialized())
      return false;
   
   if (gTiledAAManager.getColorFormat() != D3DFMT_A16B16G16R16F_EXPAND)      
      return false;
         
   IDirect3DTexture9* pRadianceBuf = gTiledAAManager.getColorTexture();
   if (!pRadianceBuf)
      return false;
      
   BFixedStringMaxPath hdrFilename;
   hdrFilename.set(pFilename);
   
   BFileSystemStream dstStream;
   if (!dstStream.open(-1, hdrFilename.c_str(), cSFWritable | cSFSeekable | cSFEnableBuffering))
      return false;

   BHDRWriter hdrWriter;
   if (!hdrWriter.open(dstStream, gTiledAAManager.getWidth(), gTiledAAManager.getHeight()))
      return false;
              
   gRenderThread.blockUntilGPUIdle();
   
   D3DLOCKED_RECT rect;
   pRadianceBuf->LockRect(0, &rect, NULL, 0);

   const uint64* pSurf = static_cast<const uint64*>(rect.pBits);
   
   pRadianceBuf->UnlockRect(0);
      
   BDynamicRenderArray<BRGBAColorF, 16> scanlineBuf(gTiledAAManager.getWidth());
   
   for (uint y = 0; y < gTiledAAManager.getHeight(); y++)
   {
      for (uint x = 0; x < gTiledAAManager.getWidth(); x++)
      {
         const uint ofs = XGAddress2DTiledOffset(x, y, gTiledAAManager.getWidth(), sizeof(uint64));
         
         // pSurf points to uncached memory, so load it into a temp before converting it.
         const uint64 p = pSurf[ofs];
         
         XMStoreFloat4A(reinterpret_cast<XMFLOAT4A*>(scanlineBuf.getPtr() + x), XMLoadHalf4(reinterpret_cast<const XMHALF4*>(&p)));
      }
      
      if (!hdrWriter.writeLine(scanlineBuf.getPtr(), BHDRWriter::cInputRGBF))
         return false;
   }
   
   if (!hdrWriter.close())
      return false;
      
   if (!dstStream.close())      
      return false;

   return true;   
}

//============================================================================
// BRender::processCommand
//============================================================================
void BRender::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cRenderCommandHDRScreenshot:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(const char* const*));
         const char* pFilename = *reinterpret_cast<const char* const *>(pData);
         BDEBUG_ASSERT(pFilename);
         
         workerSaveHDRScreenshot(pFilename);
            
         break;
      }
   }      
}

//============================================================================
// BRender::deinitDeviceData
//============================================================================
void BRender::deinitDeviceData(void)
{
   if (gpDebugPrimitives)
   {
      gpDebugPrimitives->deInit();

      delete gpDebugPrimitives;
      gpDebugPrimitives = NULL;
      gpRenderDebugPrimitives = NULL;
   }
}

