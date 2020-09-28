//==============================================================================
// renderInit.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xrender.h"
#include "renderInit.h"
#include "config.h"
#include "renderDraw.h"
#include "fixedFuncShaders.h"
#include "D3DTextureManager.h"
#include "D3DTextureManager.h"
#include "renderHelperThread.h"
#include "effectIntrinsicManager.h"
#include "debugText.h"
#include "gpuHeap.h"
#include "xboxTextureHeap.h"
#include "packedTextureManager.h"
#include "memory\XMemory.h"
#include "DCBManager.h"

// xcore
#include "xexception\xdebugtext.h"

// Hardcore/desperate debugging only.
#define RENDER_SINGLESTEP 0

//==============================================================================
// Globals
//==============================================================================
BRenderInitializer gRenderInitializer;

//#define DISABLE_DEBUG_PRINTING

#if defined(BUILD_FINAL) || defined(DISABLE_DEBUG_PRINTING)
   #define DBG_PRINT_INIT(l)
   #define DBG_PRINT(s) 
#else
   #define DBG_PRINT_INIT(l) uint dbgPrintY = l;
   #define DBG_PRINT(s) do { trace(s); BDebugText::renderRaw(0, dbgPrintY, s); dbgPrintY++; } while(0)
#endif

//==============================================================================
// BRenderInitializer::BRenderInitializer
//==============================================================================
BRenderInitializer::BRenderInitializer()
{
}

//==============================================================================
// BRenderInitializer::~BRenderInitializer
//==============================================================================
BRenderInitializer::~BRenderInitializer()
{
}

struct BRenderInitData
{
   uint mGPUFrameHeapSize;
   long mTextureManagerBaseDirID;
   long mEffectCompilerDefaultDirID;
   bool mDisableMeshTextureCache;
   bool mBackgroundTextureLoading;
};

//==============================================================================
// BRenderInitializer::renderInit
//==============================================================================
void BRenderInitializer::renderInit(void* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
      
   const BRenderInitData& data = *static_cast<const BRenderInitData*>(pData);
         
   gGPUFrameHeap.init(data.mGPUFrameHeapSize, 0, PAGE_READWRITE | PAGE_WRITECOMBINE);
   
   if (!data.mDisableMeshTextureCache)
   {
      BDEBUG_ASSERT(!gpPackedTextureManager);
      gpPackedTextureManager = HEAP_NEW(BPackedTextureManager, gRenderHeap);
      static uint cPackedTextureCacheSize = (uint)(8.0f * 1024U * 1024U);
      gpPackedTextureManager->init(8192, cPackedTextureCacheSize);
   }      

   gD3DTextureManager.init(data.mTextureManagerBaseDirID, data.mBackgroundTextureLoading);
   
   gDCBManager.init();
}

//==============================================================================
// BRenderInitializer::init
//==============================================================================
bool BRenderInitializer::init(
   const BD3D::BCreateDeviceParams& initParams, 
   BRenderThread::BD3DDeviceInfo* pDeviceInfo,
   long textureManagerBaseDirID,
   long effectCompilerDefaultDirID,
   uint frameStorageSize, 
   uint gpuFrameStorage, 
   uint gpuLocalFrameStorage,
   uint gpuFrameHeapSize)
{
   DBG_PRINT_INIT(2);
   
   trace("init: start");
   
   TRACEMEM
   
   if (gRenderThread.getInitialized())
      gRenderThread.deinit();
   
   XSetProcessQuantumLength(10);
   
   bool enableHeapInterception = true;
#ifndef BUILD_FINAL
   if (gConfig.isDefined("DisableTextureRegionHeap"))
      enableHeapInterception = false;
#endif   

   bool disableMeshTextureCache = false;
#ifndef BUILD_FINAL
   disableMeshTextureCache = gConfig.isDefined("DisableMeshTextureCache");
#endif   
   
   gpXboxTextureHeap = new BXboxTextureHeap;
   gpXboxTextureHeap->init(enableHeapInterception, !disableMeshTextureCache);
      
   gRenderThread.init(initParams, pDeviceInfo);
   
   TRACEMEM
   
#ifndef BUILD_FINAL
// Hardcore/desperate debugging only.
#if RENDER_SINGLESTEP
   gRenderThread.setSingleStep(true);
   trace("Warning: Render thread single step enabled!");
#endif   
#endif   

   gRenderThread.blockUntilProcessingCommands();

#ifndef BUILD_FINAL   
   BDebugText::init();
#endif   

   DBG_PRINT("init: 1");
         
   gRenderHelperThread.init(cThreadIndexRenderHelper, 3, "RenderHelper", NULL, 0, BEventDispatcher::cDefaultEventQueueSize, BEventDispatcher::cDefaultSyncEventQueueSize, true);
         
   gSimHelperThread.init(cThreadIndexSimHelper, 1, "SimHelper", NULL, 0, BEventDispatcher::cDefaultEventQueueSize, BEventDispatcher::cDefaultSyncEventQueueSize, true);
      
   //gRenderIdleThread.init(cThreadIndexRenderIdle, 2, "RenderIdle", NULL, 0, BEventDispatcher::cDefaultEventQueueSize, BEventDispatcher::cDefaultSyncEventQueueSize, true);
   //SetThreadPriority(gRenderIdleThread.getThreadHandle(), THREAD_PRIORITY_BELOW_NORMAL);
         
   TRACEMEM
         
   DBG_PRINT("init: 2");

   gRenderDraw.init();
         
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
         
   TRACEMEM
   
   DBG_PRINT("init: 3");

   gFixedFuncShaders.init();
         
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
   
   TRACEMEM
   
   DBG_PRINT("init: 4");
            
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
   
   TRACEMEM
   
   DBG_PRINT("init: 5");
   
   gEffectIntrinsicManager.init();

   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);

   TRACEMEM
   
   DBG_PRINT("init: 6");
   
   BRenderInitData renderInitData;
   renderInitData.mGPUFrameHeapSize = gpuFrameHeapSize;
   renderInitData.mTextureManagerBaseDirID = textureManagerBaseDirID;
   renderInitData.mEffectCompilerDefaultDirID = effectCompilerDefaultDirID;
   renderInitData.mDisableMeshTextureCache = disableMeshTextureCache;
   renderInitData.mBackgroundTextureLoading = false;
#if defined(BUILD_DEBUG) || defined(BUILD_CHECKED)
   if ( ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) == 0) && (gConfig.isDefined("BackgroundTextureLoading")) )
   {
      // Enable background texture load if archives are not enabled.
      renderInitData.mBackgroundTextureLoading = true;
   }
#endif   
#ifndef BUILD_FINAL
   if ( ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) == 0) && (gConfig.isDefined("ForceBackgroundTextureLoading")) )
      renderInitData.mBackgroundTextureLoading = true;
#endif
   gRenderThread.submitCallback(renderInit, &renderInitData);
               
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
   
   if (gpPackedTextureManager)
      gpPackedTextureManager->simInit();
      
   TRACEMEM
               
   DBG_PRINT("init: OK");
            
   return true;
}

//==============================================================================
// BRenderInitializer::renderInit
//==============================================================================
void BRenderInitializer::renderDeinit(void* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
   pData;
   
   gDCBManager.deinit();
   
   gD3DTextureManager.deinit();
   
   if (gpPackedTextureManager)
   {
      gpPackedTextureManager->deinit();
      HEAP_DELETE(gpPackedTextureManager, gRenderHeap);
      gpPackedTextureManager = NULL;
   }

   gGPUFrameHeap.deinit();
}
   
//==============================================================================
// BRenderInitializer::deinit
//==============================================================================   
bool BRenderInitializer::deinit(void)
{
   DBG_PRINT_INIT(0);
   
   DBG_PRINT("deinit: start");
   
   // First, lets stop all producers of new work.
   gRenderThread.blockUntilGPUIdle();
   
   gEventDispatcher.pumpAllThreads(2000);
   
   DBG_PRINT("deinit: 1");
         
   if (gpPackedTextureManager)
      gpPackedTextureManager->simDeinit();
   
   gRenderThread.submitCallback(renderDeinit);
      
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
         
   gEffectIntrinsicManager.deinit();

   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);

   DBG_PRINT("deinit: 2");
      
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
   
   DBG_PRINT("deinit: 3");
   
   gFixedFuncShaders.deinit();
   
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(50);
   
   DBG_PRINT("deinit: 4");
   
   gRenderDraw.deinit();
   
   gRenderThread.blockUntilWorkerIdle();
   gEventDispatcher.pumpAllThreads(200);
   
   DBG_PRINT("deinit: 5");

#ifndef BUILD_FINAL   
   BDebugText::deinit();
#endif   
   
   gRenderThread.deinit();
   
   gEventDispatcher.pumpAllThreads(50);
   
   DBG_PRINT("deinit: 6");
   
   //gRenderIdleThread.deinit();
   
   gSimHelperThread.deinit();
   gRenderHelperThread.deinit();
   
   // We can't deinit the xbox texture heap, because it must stay active during shutdown to free allocations!
   gpXboxTextureHeap->freeUnusedValleys();
      
   gEventDispatcher.pumpAllThreads(50);
         
   DBG_PRINT("deinit: OK");
   
   return true;
}
