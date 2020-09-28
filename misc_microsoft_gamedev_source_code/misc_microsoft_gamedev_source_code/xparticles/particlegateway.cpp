//============================================================================
// File: particlegateway.cpp
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#include "xparticlescommon.h"
#include "particlegateway.h"
#include "particleemitter.h"
#include "ParticleSystemManager.h"
#include "debugprimitives.h"
#include "config.h"
#include "econfigenum.h"

//#define DEBUG_WORLDMTX_UPDATE

//============================================================================
// Globals
//============================================================================
BParticleGateway gParticleGateway;

//============================================================================
// BParticleInstance::BParticleInstance
//============================================================================
BParticleInstance::BParticleInstance() :
   mInstanceSlotIndex(-1)
{
}

//============================================================================
// BParticleInstance::~BParticleInstance
//============================================================================
BParticleInstance::~BParticleInstance()
{
}

//============================================================================
// BParticleInstance::deinit
//============================================================================
void BParticleInstance::deinit()
{
}

//============================================================================
// BParticleInstance::update
//============================================================================
void BParticleInstance::update(float elapsedTime, bool synced)
{
   elapsedTime;
   synced;
}

//============================================================================
// BParticleInstance::updateWorldMatrix
//============================================================================
void BParticleInstance::updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix)
{
   //SCOPEDSAMPLE(BParticleInstance_updateWorldMatrix);  

   if (-1 == mInstanceSlotIndex)
      return;

   /*      
   BVector pos;
   worldMatrix.getTranslation(pos);
   trace("gateway %p %0.2f %0.2f %0.2f", this, pos.x, pos.y, pos.z);
   */
      
   gParticleGateway.updateInstanceMatrix(this, &worldMatrix, pLocalMatrix);
}

//============================================================================
// BParticleInstance::setSecondaryMatrix
//============================================================================
void BParticleInstance::setSecondaryMatrix(BMatrix matrix)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gParticleGateway.setSecondaryInstanceMatrix(this, &matrix);
}

//============================================================================
//============================================================================
void BParticleInstance::setTintColor(DWORD color)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gParticleGateway.setTintColor(this, color);
}

//============================================================================
// BParticleInstance::setVisibility
//============================================================================
void BParticleInstance::setVisibility(bool bState)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gParticleGateway.setInstanceFlag(this, BParticleEmitter::eFlagVisible, bState);
}

//============================================================================
// BParticleInstance::setNearLayer()
//============================================================================
void BParticleInstance::setNearLayer(bool bState)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gParticleGateway.setInstanceFlag(this, BParticleEmitter::eFlagNearLayer, bState);
}

//============================================================================
// BParticleInstance::render
//============================================================================
void BParticleInstance::render(const BVisualRenderAttributes* pRenderAttributes)
{
}

//============================================================================
// BParticleInstance::computeBoundingBox
//============================================================================
void BParticleInstance::computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners)
{
   pMinCorner->set(0,0,0);
   pMaxCorner->set(1,1,1);
}

//============================================================================
// BParticleInstance::getBoneHandle
//============================================================================
long BParticleInstance::getBoneHandle(const char* pBoneName)
{
   return -1;
}

//============================================================================
// BParticleInstance::getBone
//============================================================================
bool BParticleInstance::getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix, BBoundingBox* pBox, const BMatrix* pOffsetMatrix, bool applyIK)
{
   return false;
}




//============================================================================
// BParticleGateway::BParticleGateway
//============================================================================
BParticleGateway::BParticleGateway() :
   BEventReceiver(),
   mNumDataSlotsInUse(0), 
   mNumInstanceSlotsFree(0),
   mInstanceSlotsHighWaterMark(0),
   mFirstFreeInstanceSlotIndex(-1),
   mTotalActiveInstances(0),
   mInitialized(false)
{

}

//============================================================================
// BParticleGateway::~BParticleGateway
//============================================================================
BParticleGateway::~BParticleGateway()
{
}

//============================================================================
// BParticleGateway::init
//============================================================================
void BParticleGateway::init(void)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(!mInitialized);
   
   clear();

   commandListenerInit();
   
   eventReceiverInit();
   
   mInitialized = true;
}

//============================================================================
// BParticleGateway::deinit
//============================================================================
void BParticleGateway::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   if (!mInitialized)
      return;
   
   releaseAllInstances();      
   
   gRenderThread.blockUntilGPUIdle();
         
   commandListenerDeinit();

   eventReceiverDeinit();
   
   mInitialized = false;   
}

//============================================================================
// BParticleGateway::update
//============================================================================
void BParticleGateway::update()
{
   // Release expired instances
   for (int i = mAutoReleaseInstances.getNumber() - 1; i >= 0; i--)
   {
      BInstanceSlot& slot = mInstanceSlots[mAutoReleaseInstances[i]];
      if (slot.mpInstance)
         releaseInstance(slot.mpInstance, false, true);
      mAutoReleaseInstances.removeIndex(i);
   }
}

//============================================================================
// BParticleGateway::clear
//============================================================================
void BParticleGateway::clear(void)
{
   ASSERT_MAIN_THREAD
   
   for (uint i = 0; i < cMaxDataSlots; i++)
   {
      BDataSlot& dataSlot = mDataSlots[i];
      dataSlot.mName.setNULL();
      dataSlot.mRenderDataIndex = cInvalidIndex;
      dataSlot.mStatus = cSSFree;
   }

   for (uint i = 0; i < cMaxInstanceSlots; i++)
   {
      BInstanceSlot& instanceSlot = mInstanceSlots[i];
      instanceSlot.mDataSlotIndex = (BParticleEffectDataHandle)cInvalidIndex;
      instanceSlot.mpInstance = NULL;
      instanceSlot.mpRenderEffect = NULL;
      instanceSlot.mStatus = cSSFree;
      instanceSlot.mBeingDeleted = false;
   }

   mNumDataSlotsInUse = 0;
   mNumInstanceSlotsFree = 0;
   mInstanceSlotsHighWaterMark = 0;
   mFirstFreeInstanceSlotIndex = -1;
   mTotalActiveInstances = 0;
}

//============================================================================
// BParticleGateway::getDataName
//============================================================================
const BSimString* BParticleGateway::getDataName(BParticleEffectDataHandle dataHandle) const
{
   if (dataHandle >= 0 && dataHandle < (int)mNumDataSlotsInUse)
      return &(mDataSlots[dataHandle].mName);
   else
      return NULL;
}

//============================================================================
// BParticleGateway::getOrCreateData
//============================================================================
void BParticleGateway::getOrCreateData(const char* pName, BParticleEffectDataHandle& dataHandle)
{
   //SCOPEDSAMPLE(BParticleGateway_getOrCreateData)
   ASSERT_MAIN_THREAD

   if (gConfig.isDefined(cConfigNoParticles))
   {
      dataHandle = -1;
      return;
   }

   uint index;
   for (index = 0; index < mNumDataSlotsInUse; index++)
   {
      if (mDataSlots[index].mStatus != cSSFree)
      {
         if (stricmp(pName, mDataSlots[index].mName.getPtr()) == 0)
            break;
      }
   }
   
   if (index < mNumDataSlotsInUse)
   {
      dataHandle = (BParticleEffectDataHandle)index;
      return;
   }
   
   if (mNumDataSlotsInUse == cMaxDataSlots)
   {
      BFATAL_FAIL("BParticleGateway::getOrCreateData: Increase cMaxDataSlots!");
   }
   
   dataHandle = (BParticleEffectDataHandle)mNumDataSlotsInUse;
   mNumDataSlotsInUse++;
   
   mDataSlots[dataHandle].mName.set(pName);
   mDataSlots[dataHandle].mRenderDataIndex = -1;
   mDataSlots[dataHandle].mStatus = cSSPending;
   
   gRenderThread.submitCommand(*this, cRCInitDataSlot, dataHandle);
}

//============================================================================
// BParticleGateway::createInstance
//============================================================================
BParticleInstance* BParticleGateway::createInstance(const BParticleCreateParams& params)
{
   ASSERT_MAIN_THREAD
   
   if (gConfig.isDefined(cConfigNoParticles))
      return NULL;

   debugRangeCheck(params.mDataHandle, cMaxDataSlots);
   debugRangeCheck(params.mDataHandle, mNumDataSlotsInUse);
   BDEBUG_ASSERT(mDataSlots[params.mDataHandle].mStatus != cSSFree);
   
   uint slotIndex = mInstanceSlotsHighWaterMark;      
   
   if (mNumInstanceSlotsFree)
   {
      slotIndex = mFirstFreeInstanceSlotIndex;
      BDEBUG_ASSERT(slotIndex < mInstanceSlotsHighWaterMark);
      BDEBUG_ASSERT(mInstanceSlots[slotIndex].mStatus == cSSFree);
            
      mFirstFreeInstanceSlotIndex = mInstanceSlots[mFirstFreeInstanceSlotIndex].mNextFreeInstanceSlotIndex;
      if (mFirstFreeInstanceSlotIndex != -1)
      {
         BDEBUG_ASSERT((uint)mFirstFreeInstanceSlotIndex != slotIndex);
         BDEBUG_ASSERT(mInstanceSlots[mFirstFreeInstanceSlotIndex].mStatus == cSSFree);
      }
      
      mNumInstanceSlotsFree--;
   }      
   else
   {
      if (mInstanceSlotsHighWaterMark == cMaxInstanceSlots)
      {
         BFATAL_FAIL("BParticleGateway::createInstance: Increase cMaxInstanceSlots!");
      }
      
      mInstanceSlotsHighWaterMark++;
   }
   
   BParticleInstance* pInstance = BAlignedAlloc::New<BParticleInstance>(ALIGN_OF(BParticleInstance), gSimHeap);   
   pInstance->mInstanceSlotIndex = slotIndex;
   
   BInstanceSlot& slot = mInstanceSlots[slotIndex];
   BDEBUG_ASSERT((slot.mpInstance == NULL) && (!slot.mBeingDeleted));
   
   slot.mDataSlotIndex = params.mDataHandle;
   slot.mpInstance = pInstance;
   slot.mpRenderEffect = NULL;
   slot.mStatus = cSSPending;
      
   BInitInstanceSlotData* pInitData = (BInitInstanceSlotData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInitInstanceSlot, sizeof(BInitInstanceSlotData), 16);   
   pInitData->mInstanceSlotIndex = slotIndex;
   pInitData->mParams = params;   
   gRenderThread.submitCommandEnd(sizeof(BInitInstanceSlotData), 16);
   
   mTotalActiveInstances++;
      
   return pInstance;
}

//============================================================================
//============================================================================
void BParticleGateway::createAutoReleaseInstance(const BParticleCreateParams& params)
{
//-- FIXING PREFIX BUG ID 7235
   const BParticleInstance* pInst = createInstance(params);
//--
   if (pInst)
   {
      mAutoReleaseInstances.add(pInst->mInstanceSlotIndex);
   }
}

//============================================================================
// BParticleGateway::releaseInstance
//============================================================================
void BParticleGateway::releaseInstance(BParticleInstance* pInstance, bool bKillImmediately, bool bAutoRelease)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pInstance);
   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   BDEBUG_ASSERT(mInstanceSlots[instanceSlotIndex].mStatus != cSSFree);
   BDEBUG_ASSERT(pInstance == mInstanceSlots[instanceSlotIndex].mpInstance);
   BDEBUG_ASSERT(!mInstanceSlots[instanceSlotIndex].mBeingDeleted);
   
   mInstanceSlots[instanceSlotIndex].mBeingDeleted = true;
   
   BAlignedAlloc::Delete<BParticleInstance>(mInstanceSlots[instanceSlotIndex].mpInstance, gSimHeap);
   mInstanceSlots[instanceSlotIndex].mpInstance = NULL;


   BReleaseInstanceData* pData = (BReleaseInstanceData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCDeleteInstanceSlot, sizeof(BReleaseInstanceData), 16);   
   pData->mInstanceSlotIndex = instanceSlotIndex;
   pData->mbKillImmediately = bKillImmediately;
   pData->mbAutoRelease = bAutoRelease;
   gRenderThread.submitCommandEnd(sizeof(BReleaseInstanceData), 16);
   
   mTotalActiveInstances--;
}

//============================================================================
// BParticleGateway::pauseUpdate
//============================================================================
void BParticleGateway::pauseUpdate(bool pause)
{
   gRenderThread.submitCommand(*this, cRCPauseUpdate, pause ? 1 : 0);
}

//============================================================================
// BParticleGateway::setTimeSpeed
//============================================================================
void BParticleGateway::setTimeSpeed(float speed)
{
   gRenderThread.submitCommand(*this, cRCSetTimeSpeed, speed);
}

//============================================================================
// BParticleGateway::setDistanceFade
//============================================================================
void BParticleGateway::setDistanceFade(float startDistance, float endDistance)
{
   BSetDistanceFadeData* pData = (BSetDistanceFadeData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetDistanceFade, sizeof(BSetDistanceFadeData), 16);   
   pData->mStartDistance = startDistance;
   pData->mEndDistance = endDistance;
   gRenderThread.submitCommandEnd(sizeof(BSetDistanceFadeData), 16);
}

//============================================================================
// BParticleGateway::enableBBoxRendering
//============================================================================
void BParticleGateway::enableBBoxRendering(bool enable)
{
   gRenderThread.submitCommand(*this, cRCEnableBBoxRendering, enable ? 1 : 0);
}

//============================================================================
// BParticleGateway::enableMagnetRendering
//============================================================================
void BParticleGateway::enableMagnetRendering(bool enable)
{
   gRenderThread.submitCommand(*this, cRCEnableMagnetRendering, enable ? 1 : 0);
}

//============================================================================
// BParticleGateway::enableCulling
//============================================================================
void BParticleGateway::enableCulling(bool enable)
{
   gRenderThread.submitCommand(*this, cRCEnableCulling, enable ? 1 : 0);
}

//============================================================================
// BParticleGateway::enableDistanceFade
//============================================================================
void BParticleGateway::enableDistanceFade(bool enable)
{
   gRenderThread.submitCommand(*this, cRCEnableDistanceFade, enable ? 1 : 0);
}

//============================================================================
// BParticleGateway::updateInstanceMatrix
//============================================================================
void BParticleGateway::updateInstanceMatrix(const BParticleInstance* pInstance, const BMatrix* pMatrix, const BMatrix* pLocalMatrix)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pInstance && pMatrix);

#ifdef BUILD_DEBUG
   const float* pFloats = reinterpret_cast<const float*> (pMatrix);
   for (uint i = 0; i < 16; ++i)
   {
      BDEBUG_ASSERT(Math::IsValidFloat(pFloats[i]) && "BParticleGateway::updateInstanceMatrix() ERROR: Invalid Matrix!");
   }
   
   if (pLocalMatrix)
   {
      const float* pLocalFloats = reinterpret_cast<const float*> (pLocalMatrix);
      for (int j = 0; j< 16; ++j)
      {
         BDEBUG_ASSERT(Math::IsValidFloat(pLocalFloats[j]) && "BParticleGateway::updateInstanceMatrix() ERROR: Invalid Local Matrix!");
      }
   }
#endif
   
   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   BDEBUG_ASSERT(mInstanceSlots[instanceSlotIndex].mStatus != cSSFree);
   BDEBUG_ASSERT(!mInstanceSlots[instanceSlotIndex].mBeingDeleted);
      
   BUpdateInstanceMatrixData* pUpdateData = (BUpdateInstanceMatrixData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCUpdateInstanceMatrix, sizeof(BUpdateInstanceMatrixData), 16);
    
   pUpdateData->mMatrix = *pMatrix;
   if (pLocalMatrix)
      pUpdateData->mLocalMatrix = *pLocalMatrix;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;
   pUpdateData->mHasLocalMatrix = (pLocalMatrix != NULL);

#ifdef DEBUG_WORLDMTX_UPDATE
   static bool bRenderSphere = false;
   if (bRenderSphere)
   {
      BMatrix newMatrix;
      if (pLocalMatrix)
         newMatrix = XMMatrixMultiply(*pLocalMatrix, *pMatrix);
      else
         newMatrix = *pMatrix;
      gpDebugPrimitives->addDebugSphere(newMatrix, 0.15f, cDWORDOrange, BDebugPrimitives::cCategoryTest);
   }
#endif

   
   gRenderThread.submitCommandEnd(sizeof(BUpdateInstanceMatrixData), 16);
}

//============================================================================
// BParticleGateway::setSecondaryInstanceMatrix
//============================================================================
void BParticleGateway::setSecondaryInstanceMatrix(const BParticleInstance* pInstance, BMatrix* pMatrix)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pInstance && pMatrix);

#ifdef BUILD_DEBUG
   const float* pFloats = reinterpret_cast<const float*> (pMatrix);
   for (uint i = 0; i < 16; ++i)
   {
      BDEBUG_ASSERT(Math::IsValidFloat(pFloats[i]) && "BParticleGateway::updateInstanceMatrix() ERROR: Invalid Matrix!");
   }   
#endif

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   BDEBUG_ASSERT(mInstanceSlots[instanceSlotIndex].mStatus != cSSFree);
   BDEBUG_ASSERT(!mInstanceSlots[instanceSlotIndex].mBeingDeleted);

   BSetSecondaryInstanceMatrixData* pUpdateData = (BSetSecondaryInstanceMatrixData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetSecondaryMatrix, sizeof(BSetSecondaryInstanceMatrixData), 16);

   pUpdateData->mMatrix = *pMatrix;   
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;   

#ifdef DEBUG_WORLDMTX_UPDATE
   BMatrix newMatrix = *pMatrix;      
   gpDebugPrimitives->addDebugSphere(newMatrix, 0.15f, cDWORDGreen, BDebugPrimitives::cCategoryTest, 2.5f);
#endif


   gRenderThread.submitCommandEnd(sizeof(BSetSecondaryInstanceMatrixData), 16);
}

//============================================================================
// BParticleGateway::setTintColor
//============================================================================
void BParticleGateway::setTintColor(BParticleInstance* pInstance, DWORD color)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(pInstance);

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   BDEBUG_ASSERT(mInstanceSlots[instanceSlotIndex].mStatus != cSSFree);
   BDEBUG_ASSERT(!mInstanceSlots[instanceSlotIndex].mBeingDeleted);

   BSetTintColorData* pUpdateData = (BSetTintColorData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetTintColor, sizeof(BSetTintColorData), 16);

   pUpdateData->mColor = color;   
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;   

   gRenderThread.submitCommandEnd(sizeof(BSetTintColorData), 16);
}

//============================================================================
// BParticleGateway::setInstanceFlag
//============================================================================
void BParticleGateway::setInstanceFlag(const BParticleInstance* pInstance, int flag, bool bState)
{
   BDEBUG_ASSERT(pInstance);

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   BDEBUG_ASSERT(mInstanceSlots[instanceSlotIndex].mStatus != cSSFree);
   BDEBUG_ASSERT(!mInstanceSlots[instanceSlotIndex].mBeingDeleted);

   BUpdateInstanceFlagData* pUpdateData = (BUpdateInstanceFlagData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetInstanceFlag, sizeof(BUpdateInstanceFlagData), 16);

   pUpdateData->mFlag   = flag;
   pUpdateData->mbState = bState;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BUpdateInstanceFlagData), 16);
}


//============================================================================
// BParticleGateway::releaseAllInstances
//============================================================================
void BParticleGateway::releaseAllInstances(void)
{
   ASSERT_MAIN_THREAD
         
   for (uint i = 0; i < mInstanceSlotsHighWaterMark; i++)
   {
      BInstanceSlot& slot = mInstanceSlots[i];
      
      if ((slot.mStatus != cSSFree) && (!slot.mBeingDeleted))
      {
         BDEBUG_ASSERT(slot.mpInstance);
         releaseInstance(slot.mpInstance, true);
      }
   }

   gRenderThread.submitCommand(*this, cRCDestroyAllParticleInstances, 0);
   
   gRenderThread.blockUntilWorkerIdle();
   
   for ( ; ; )
   {
      if (mNumInstanceSlotsFree >= mInstanceSlotsHighWaterMark)
         break;
         
      gEventDispatcher.sleep(4);
   }
         
   mInstanceSlotsHighWaterMark = 0;
   mFirstFreeInstanceSlotIndex = -1;
   mNumInstanceSlotsFree = 0;
}

//============================================================================
// BParticleGateway::releaseAllDataSlots
//============================================================================
void BParticleGateway::releaseAllDataSlots(void)
{
   ASSERT_MAIN_THREAD
   for (uint i = 0; i < cMaxDataSlots; i++)
   {
      BDataSlot& dataSlot = mDataSlots[i];
      dataSlot.mName.setNULL();
      dataSlot.mRenderDataIndex = cInvalidIndex;
      dataSlot.mStatus = cSSFree;
   }

   gRenderThread.submitCommand(*this, cRCDestroyAllParticleData, 0);
   gRenderThread.blockUntilWorkerIdle();

   mNumDataSlotsInUse = 0;   
}

//============================================================================
// BParticleGateway::initMemoryPools
//============================================================================
void BParticleGateway::initMemoryPools()
{
   gRenderThread.submitCommand(*this, cRCInitMemoryPools, 0);
   gRenderThread.blockUntilWorkerIdle();
}

//============================================================================
// BParticleGateway::deinitMemoryPools
//============================================================================
void BParticleGateway::deinitMemoryPools()
{
   gRenderThread.submitCommand(*this, cRCDeinitMemoryPools, 0);
   gRenderThread.blockUntilWorkerIdle();
}

//============================================================================
// BParticleGateway::initTextureSystem
//============================================================================
void BParticleGateway::initTextureSystem()
{
   gRenderThread.submitCommand(*this, cRCInitTextureSystem, 0);
   gRenderThread.blockUntilWorkerIdle();
}

//============================================================================
// BParticleGateway::deinitTextureSystem
//============================================================================
void BParticleGateway::deinitTextureSystem()
{
   gRenderThread.submitCommand(*this, cRCDeinitTextureSystem, 0);
   gRenderThread.blockUntilWorkerIdle();
}

//============================================================================
// BParticleGateway::receiveEvent
//============================================================================
bool BParticleGateway::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_MAIN_THREAD
   
   switch (event.mEventClass)
   {
      case cECInitDataSlotReply:
      {
         const uint dataSlotIndex = debugRangeCheck<uint, uint>(event.mPrivateData, cMaxDataSlots);
         BDataSlot& dataSlot = mDataSlots[dataSlotIndex];

         BDEBUG_ASSERT(dataSlot.mStatus == cSSPending);                  
         if (event.mPrivateData2)
            dataSlot.mStatus = cSSValid;
         else
            dataSlot.mStatus = cSSFailed;
         
         break;
      }
      case cECInitInstanceSlotReply:
      {
         const uint instanceSlotIndex = debugRangeCheck<uint, uint>(event.mPrivateData, cMaxInstanceSlots);
         BInstanceSlot& instanceSlot = mInstanceSlots[instanceSlotIndex];
         
         BDEBUG_ASSERT(instanceSlot.mStatus == cSSPending);                  
         if (event.mPrivateData2)
            instanceSlot.mStatus = cSSValid;
         else
            instanceSlot.mStatus = cSSFailed;
         
         break;
      }
      case cECDeleteInstanceSlotReply:
      {
         const uint instanceSlotIndex = debugRangeCheck<uint, uint>(event.mPrivateData, cMaxInstanceSlots);
         BInstanceSlot& instanceSlot = mInstanceSlots[instanceSlotIndex];
         
         BDEBUG_ASSERT(instanceSlot.mBeingDeleted);
         
         instanceSlot.mStatus = cSSFree;
         instanceSlot.mBeingDeleted = false;
                           
         instanceSlot.mNextFreeInstanceSlotIndex = mFirstFreeInstanceSlotIndex;
         mFirstFreeInstanceSlotIndex = instanceSlotIndex;
                  
         mNumInstanceSlotsFree++;
         
         break;
      }
   }

   return false;
}

//============================================================================
// BParticleGateway::initDeviceData
//============================================================================
void BParticleGateway::initDeviceData(void)
{
}

//============================================================================
// BParticleGateway::frameBegin
//============================================================================
void BParticleGateway::frameBegin(void)
{
}

//============================================================================
// BParticleGateway::processCommand
//============================================================================
void BParticleGateway::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD
   
   switch (header.mType)
   {
      case cRCInitDataSlot:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         
         const uint dataSlotIndex = debugRangeCheck<uint, uint>(*reinterpret_cast<const uint*>(pData), cMaxDataSlots);
         BDataSlot& dataSlot = mDataSlots[dataSlotIndex];
         
         gPSManager.getData(dataSlot.mName.getPtr(), &dataSlot.mRenderDataIndex);
         
         gEventDispatcher.send(mEventHandle, mEventHandle, cECInitDataSlotReply, dataSlotIndex, (dataSlot.mRenderDataIndex >= 0));
                           
         break;
      }
      case cRCInitInstanceSlot:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BInitInstanceSlotData));
         const BInitInstanceSlotData* pInitData = reinterpret_cast<const BInitInstanceSlotData*>(Utils::AlignUp(pData, 16));
         
         BInstanceSlot& instanceSlot = mInstanceSlots[debugRangeCheck<uint, uint>(pInitData->mInstanceSlotIndex, cMaxInstanceSlots)];
         debugRangeCheck<uint, uint>(instanceSlot.mDataSlotIndex, cMaxDataSlots);
         const BDataSlot& dataSlot = mDataSlots[instanceSlot.mDataSlotIndex];
         
         instanceSlot.mpRenderEffect = NULL;
         if (dataSlot.mRenderDataIndex != -1)
         {
            BParticleCreateParams params;
            params.mDataHandle = dataSlot.mRenderDataIndex;
            params.mMatrix     = pInitData->mParams.mMatrix;
            params.mNearLayerEffect = pInitData->mParams.mNearLayerEffect;
            params.mPriority = pInitData->mParams.mPriority;
            params.mTintColor = pInitData->mParams.mTintColor;
            //instanceSlot.mpRenderEffect = gPSManager.createEffect(dataSlot.mRenderDataIndex, pInitData->mMatrix);
            instanceSlot.mpRenderEffect = gPSManager.createEffect(params);
         }
         
         gEventDispatcher.send(mEventHandle, mEventHandle, cECInitInstanceSlotReply, pInitData->mInstanceSlotIndex, instanceSlot.mpRenderEffect != NULL);
         
         break;
      }
      case cRCUpdateInstanceMatrix:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BUpdateInstanceMatrixData));
         const BUpdateInstanceMatrixData* pUpdateData = reinterpret_cast<const BUpdateInstanceMatrixData*>(Utils::AlignUp(pData, 16));
         
         BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];
         
         if (instanceSlot.mpRenderEffect)
            instanceSlot.mpRenderEffect->updateWorldMatrix(pUpdateData->mMatrix, pUpdateData->mHasLocalMatrix ? &pUpdateData->mLocalMatrix : NULL);
         
         break;
      }
      case cRCSetSecondaryMatrix:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BSetSecondaryInstanceMatrixData));
         const BSetSecondaryInstanceMatrixData* pUpdateData = reinterpret_cast<const BSetSecondaryInstanceMatrixData*>(Utils::AlignUp(pData, 16));

         BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

         if (instanceSlot.mpRenderEffect)
            instanceSlot.mpRenderEffect->setSecondaryTransform(pUpdateData->mMatrix);

         break;
      }
      case cRCSetTintColor:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BSetTintColorData));
         const BSetTintColorData* pUpdateData = reinterpret_cast<const BSetTintColorData*>(Utils::AlignUp(pData, 16));

         BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

         if (instanceSlot.mpRenderEffect)
            instanceSlot.mpRenderEffect->setTintColor(pUpdateData->mColor);

         break;
      }
      case cRCSetInstanceFlag:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BUpdateInstanceFlagData));
         const BUpdateInstanceFlagData* pUpdateData = reinterpret_cast<const BUpdateInstanceFlagData*>(Utils::AlignUp(pData, 16));

         BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

         if (instanceSlot.mpRenderEffect)
            instanceSlot.mpRenderEffect->setFlag(pUpdateData->mFlag, pUpdateData->mbState);

         break;
      }
      case cRCPauseUpdate:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint value = *reinterpret_cast<const uint*>(pData);
         bool paused = value > 0 ? true : false;
         gPSManager.setPaused(paused);
         break;
      }

      case cRCSetTimeSpeed:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const float value = *reinterpret_cast<const float*>(pData);         
         gPSManager.setTimeSpeed(value);
         break;
      }

      case cRCSetDistanceFade:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BSetDistanceFadeData));
            const BSetDistanceFadeData* pFadeData = reinterpret_cast<const BSetDistanceFadeData*>(Utils::AlignUp(pData, 16));
            gPSManager.setDistanceFade(pFadeData->mStartDistance, pFadeData->mEndDistance);
            break;
         }

      case cRCEnableBBoxRendering:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint value = *reinterpret_cast<const uint*>(pData);
         bool enable = value > 0 ? true : false;
         gPSManager.enableBBoxRendering(enable);
         break;
      }
      case cRCEnableMagnetRendering:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint value = *reinterpret_cast<const uint*>(pData);
         bool enable = value > 0 ? true : false;
         gPSManager.setFlagRenderMagnets(enable);
         break;
      }
      case cRCEnableCulling:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint value = *reinterpret_cast<const uint*>(pData);
         bool enable = value > 0 ? true : false;
         gPSManager.setFlagEnableCulling(enable);
         break;
      }
      case cRCEnableDistanceFade:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         const uint value = *reinterpret_cast<const uint*>(pData);
         bool enable = value > 0 ? true : false;
         gPSManager.enableDistanceFade(enable);
         break;
      }
      case cRCDeleteInstanceSlot:
      {
         BDEBUG_ASSERT(header.mLen >= sizeof(BReleaseInstanceData));
         const BReleaseInstanceData* pReleaseData = reinterpret_cast<const BReleaseInstanceData*>(Utils::AlignUp(pData, 16));
         debugRangeCheck<uint, uint>(pReleaseData->mInstanceSlotIndex, cMaxInstanceSlots);
         
         BInstanceSlot& instanceSlot = mInstanceSlots[pReleaseData->mInstanceSlotIndex];
         BDEBUG_ASSERT(instanceSlot.mBeingDeleted);
         
         if (instanceSlot.mpRenderEffect)
         {  
            bool releaseEmitters = pReleaseData->mbKillImmediately || !pReleaseData->mbAutoRelease;
            gPSManager.releaseEffect(instanceSlot.mpRenderEffect, pReleaseData->mbKillImmediately, releaseEmitters);
            instanceSlot.mpRenderEffect = NULL;
         }
                  
         gEventDispatcher.send(mEventHandle, mEventHandle, cECDeleteInstanceSlotReply, pReleaseData->mInstanceSlotIndex);
               
         break;
      }      
      case cRCDestroyAllParticleInstances:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));         
         gPSManager.destroyAllInstances();
         break;
      }
      case cRCDestroyAllParticleData:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));         
         gPSManager.destroyAllData();
         break;
      }

      case cRCInitMemoryPools:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         gPSManager.initMemoryPools();
         break;
      }
      case cRCDeinitMemoryPools:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         gPSManager.deinitMemoryPools();
         break;
      }

      case cRCInitTextureSystem:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         gPSManager.initTextureSystem();
         break;
      }
      case cRCDeinitTextureSystem:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         gPSManager.deinitTextureSystem();
         break;
      }
   }   
}

//============================================================================
// BParticleGateway::frameEnd
//============================================================================
void BParticleGateway::frameEnd(void)
{
}

//============================================================================
// BParticleGateway::deinitDeviceData
//============================================================================
void BParticleGateway::deinitDeviceData(void)
{
}

//============================================================================
// BParticleGateway::getDataHandleForInstance
//============================================================================
long BParticleGateway::getDataHandleForInstance(const BParticleInstance* pInstance)
{
   if (!pInstance)
      return -1;

   if (-1 == pInstance->mInstanceSlotIndex)
      return -1;

//-- FIXING PREFIX BUG ID 7240
   const BInstanceSlot& slot = mInstanceSlots[pInstance->mInstanceSlotIndex];
//--
   return(slot.mDataSlotIndex);
}