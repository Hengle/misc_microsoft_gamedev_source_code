//============================================================================
//
// File: effectSharedParams.cpp
// rg [2/18/06] - All FXLEffect objects should use the effect pool managed by this object, otherwise correlation will be lost.
// Also, be sure to include shared\sharedParams.inc in all .fx files.
// 
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "effectGlobalParams.h"
#include "renderDraw.h"

BEffectSharedParamPool gEffectSharedParamPool;

BEffectSharedParamPool::BEffectSharedParamPool() :
   BRenderCommandListener(),
   mWorldSunDir(cInvalidFXLHandle),
   mViewSunDir(cInvalidFXLHandle)
{
   Utils::ClearObj(mMatrixTrackerParams);
   
#ifdef BUILD_DEBUG   
   FXL__EnforceSharedCorrelation = TRUE;
#endif   
}

BEffectSharedParamPool::~BEffectSharedParamPool()
{
}

void BEffectSharedParamPool::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (cInvalidCommandListenerHandle != mCommandHandle)
      return;

   Utils::ClearObj(mMatrixTrackerParams);       
   
   commandListenerInit();      
}

void BEffectSharedParamPool::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (cInvalidCommandListenerHandle == mCommandHandle)
      return;
      
   commandListenerDeinit();
}

void BEffectSharedParamPool::updateMatrices(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   gRenderThread.submitCommand(*this, cCommandUpdateMatrices);
}

void BEffectSharedParamPool::initDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   mEffectPool.create();
}

void BEffectSharedParamPool::frameBegin(void)
{
   ASSERT_THREAD(cThreadIndexRender);
}

void BEffectSharedParamPool::setupPoolParameters(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   if (mMatrixTrackerParams[0])
      return;
         
   XMMATRIX identity = XMMatrixIdentity();

   for (uint matrixIndex = 0; matrixIndex < cNumMTMatrices; matrixIndex++)
   {
      const BFixedString64 name(cVarArg, "g%s", BMatrixTracker::getMatrixEffectName((eMTMatrix)matrixIndex) );

      BFXLEffectPoolParam& param = mEffectPool(name);
      BDEBUG_ASSERT(param.getValid());

      mMatrixTrackerParams[matrixIndex] = param.getHandle();

      mEffectPool.getEffectPool()->SetMatrixRawF4x4A(mMatrixTrackerParams[matrixIndex], (const FXLFLOATA*)&identity );
   }
   
   mWorldSunDir = mEffectPool("gWorldSunVec").getHandle(); BDEBUG_ASSERT(mWorldSunDir != cInvalidFXLHandle);
   mViewSunDir = mEffectPool("gViewSunVec").getHandle(); BDEBUG_ASSERT(mViewSunDir != cInvalidFXLHandle);
}   

void BEffectSharedParamPool::processUpdateMatricesCommand(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   setupPoolParameters();
         
   const BMatrixTracker& matrixTracker = gRenderDraw.getWorkerActiveMatrixTracker();
         
   for (uint matrixIndex = 0; matrixIndex < cNumMTMatrices; matrixIndex++)
      mEffectPool.getEffectPool()->SetMatrixRawF4x4A(mMatrixTrackerParams[matrixIndex], (const FXLFLOATA*)&matrixTracker.getMatrix((eMTMatrix)matrixIndex, true) );
      
   // hack hack
   const BMatrix44& worldToView = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cWorldToView, false);
   static BVec4 worldLightVec(1,1,1,0);
   worldLightVec.normalize();
   
   const BVec4 viewLightVec(worldLightVec * worldToView);
   
   mEffectPool.getEffectPool()->SetVectorF(mWorldSunDir, worldLightVec.getPtr());
   mEffectPool.getEffectPool()->SetVectorF(mViewSunDir, viewLightVec.getPtr());
}

void BEffectSharedParamPool::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   switch (header.mType)
   {
      case cCommandUpdateMatrices:
      {
         processUpdateMatricesCommand();
         break;
      }
   }
}

void BEffectSharedParamPool::frameEnd(void)
{
   ASSERT_THREAD(cThreadIndexRender);
}

void BEffectSharedParamPool::deinitDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   mEffectPool.clear();
}

