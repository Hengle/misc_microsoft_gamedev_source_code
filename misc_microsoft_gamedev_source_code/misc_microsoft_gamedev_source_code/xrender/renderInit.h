//==============================================================================
// renderInit.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xrender
#include "renderThread.h"
#include "asyncFileManager.h"

//==============================================================================
// class BRenderInitializer
//==============================================================================
class BRenderInitializer
{
public:
   BRenderInitializer();
   ~BRenderInitializer();
   
   bool init(
      const BD3D::BCreateDeviceParams& initParams, 
      BRenderThread::BD3DDeviceInfo* pDeviceInfo = NULL,
      long textureManagerBaseDirID = 0,
      long effectCompilerDefaultDirID = 0,
      uint frameStorageSize = 0, 
      uint gpuFrameStorage = 0, 
      uint gpuLocalFrameStorage = 0,
      uint gpuFrameHeapSize = 0);
   
   bool deinit(void);
   
private:
   static void renderInit(void* pData);
   static void renderDeinit(void* pData);
};

//==============================================================================
// Externs
//==============================================================================
extern BRenderInitializer gRenderInitializer;
