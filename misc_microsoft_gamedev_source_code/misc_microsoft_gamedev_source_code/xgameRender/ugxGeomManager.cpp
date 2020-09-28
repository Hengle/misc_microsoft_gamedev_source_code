//============================================================================
//
//  File: ugxGeomManager.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "ugxGeomManager.h"
#include "ugxGeomUberEffectManager.h"
#include "renderThread.h"
#include "renderEventClasses.h"
#include "config.h"
#include "configsGameRender.h"

#include "..\shaders\ugx\vShaderRegs.inc"
#include "..\shaders\ugx\pShaderRegs.inc"

//============================================================================
// Globals
//============================================================================
BUGXGeomManager      gUGXGeomManager;
BRenderStateFilter*  gpUGXGeomRSFilter;

//============================================================================
// BUGXGeomManager::BUGXGeomManager
//============================================================================
BUGXGeomManager::BUGXGeomManager() :
   mEffectDirID(0),
   mTotalModels(0),
   mEventHandle(cInvalidEventReceiverHandle),
   mCommandListenerHandle(cInvalidCommandListenerHandle),
   mpCommandBufferDevice(NULL)
{
   mGeomSlots.reserve(128);

   Utils::ClearObj(mModelStats);
}

//============================================================================
// BUGXGeomManager::~BUGXGeomManager
//============================================================================
BUGXGeomManager::~BUGXGeomManager()
{
}

//============================================================================
// BUGXGeomManager::init
//============================================================================
void BUGXGeomManager::init(long effectDirID)
{
   ASSERT_MAIN_THREAD   

   if (cInvalidCommandListenerHandle != mCommandListenerHandle)
      return;
      
   gpUGXD3DDev = BD3D::mpDev;

   mEffectDirID = effectDirID;

   mTotalModels = 0;
   Utils::ClearObj(mModelStats);

   mModelStats[cUGXGeomStatusInvalid] = mGeomSlots.size();

   for (uint i = 0; i < mGeomSlots.size(); i++)
      mGeomSlots[i].clear();

   mSimOptions.clear();
   mRenderOptions.clear();
   
   mEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);
   mCommandListenerHandle = gRenderThread.registerCommandListener(this);
   
   gRenderThread.blockUntilWorkerIdle();
         
   gUGXGeomUberEffectManager.init(mEffectDirID, &mIntrinsicPool);
   
   gpUGXGeomRSFilter = new BRenderStateFilter(BD3D::mpDev, FIRST_VSHADER_MATERIAL_REG, NUM_VSHADER_MATERIAL_REGS, FIRST_PSHADER_MATERIAL_REG, NUM_PSHADER_MATERIAL_REGS);
}

//============================================================================
// BUGXGeomManager::reset
//============================================================================
void BUGXGeomManager::reset()
{
   ASSERT_MAIN_THREAD

   if (cInvalidCommandListenerHandle == mCommandListenerHandle)
      return;

   gRenderThread.blockUntilGPUIdle();      

   for (uint i = 0; i < mGeomSlots.size(); i++)
   {
      if ((mGeomSlots[i].mpGeom) && (!mGeomSlots[i].mBeingDeleted))
      {
         mGeomSlots[i].mBeingDeleted = true;
         gRenderThread.submitCommand(mCommandListenerHandle, cRCDeinit, mGeomSlots[i].mpGeom);
      }
   }

   gRenderThread.blockUntilWorkerIdle();

   // Now wait for the round trips to complete. 
   // If this loop locks up, it probably means the UGX geom object status change events are not being sent back to us somehow.
   while (getTotalModels() > 0)
   {
      gEventDispatcher.pumpThreadAndWait(cThreadIndexRender, 33, 8);
   }

   gRenderThread.blockUntilWorkerIdle();

   // Free the array.
   mGeomSlots.clear();
}

//============================================================================
// BUGXGeomManager::deinit
//============================================================================
void BUGXGeomManager::deinit(void)
{
   ASSERT_MAIN_THREAD

   if (cInvalidCommandListenerHandle == mCommandListenerHandle)
      return;
         
   gRenderThread.blockUntilGPUIdle();      
   
   delete gpUGXGeomRSFilter;
   gpUGXGeomRSFilter = NULL;
   
   gUGXGeomUberEffectManager.deinit();
         
   for (uint i = 0; i < mGeomSlots.size(); i++)
   {
      if ((mGeomSlots[i].mpGeom) && (!mGeomSlots[i].mBeingDeleted))
      {
         mGeomSlots[i].mBeingDeleted = true;
         gRenderThread.submitCommand(mCommandListenerHandle, cRCDeinit, mGeomSlots[i].mpGeom);
      }
   }

   gRenderThread.blockUntilWorkerIdle();

   // Now wait for the round trips to complete. 
   // If this loop locks up, it probably means the UGX geom object status change events are not being sent back to us somehow.
   while (getTotalModels() > 0)
   {
      gEventDispatcher.pumpThreadAndWait(cThreadIndexRender, 33, 8);
   }

   gRenderThread.blockUntilWorkerIdle();

   gRenderThread.freeCommandListener(mCommandListenerHandle);
   mCommandListenerHandle = cInvalidCommandListenerHandle;

   gEventDispatcher.removeClientDeferred(mEventHandle, true);
   mEventHandle = cInvalidEventReceiverHandle;

   // Free the array.
   mGeomSlots.clear();
}

//============================================================================
// BUGXGeomManager::setOptions
//============================================================================
void BUGXGeomManager::setOptions(const BNameValueMap& nameValueMap)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);
   
   mSimOptions = nameValueMap;
   
   uint serializeSize = nameValueMap.getSerializeSize();
   
   void* pDst = gRenderThread.submitCommandBegin(mCommandListenerHandle, cRCSetOptions, serializeSize);
   
   nameValueMap.serialize(pDst, true);
   
   gRenderThread.submitCommandEnd(serializeSize);
}

//============================================================================
// BUGXGeomManager::getOptions
//============================================================================
const BNameValueMap& BUGXGeomManager::getOptions(void) const
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
      return mRenderOptions;
   else
      return mSimOptions;
}

//============================================================================
// BUGXGeomManager::addGeom
//============================================================================
BEventReceiverHandle BUGXGeomManager::addGeom(BECFFileData* pECFFileData, BEventReceiverHandle ownerHandle, eUGXGeomRenderFlags renderFlags)
{
  //SCOPEDSAMPLE(BUGXGeomManager_addGeom);
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);

   const uint slotIndex = allocSlot();

   BUGXGeomRender* pGeom = BAlignedAlloc::New<BUGXGeomRender>(ALIGN_OF(BUGXGeomRender), gRenderHeap, ownerHandle, mEventHandle);

   mGeomSlots[slotIndex].mpGeom = pGeom;
   mGeomSlots[slotIndex].mStatus = cUGXGeomStatusInvalid;
   mGeomSlots[slotIndex].mBeingDeleted = false;

   mTotalModels++;

   pGeom->setEventHandle(gEventDispatcher.setHandlePrivateData(pGeom->getEventHandle(), slotIndex));

   // FIXME: We're going to leak this block if this message is discarded.
   gEventDispatcher.send(mEventHandle, pGeom->getEventHandle(), cRenderEventClassUGXGeomInit, (uint)pECFFileData, (uint)renderFlags, NULL, BEventDispatcher::cSendSynchronousDispatch);

   return pGeom->getEventHandle();
}

//============================================================================
// BUGXGeomManager::remove
//============================================================================
void BUGXGeomManager::remove(BEventReceiverHandle handle)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);

   const uint slotIndex = gEventDispatcher.getHandlePrivateData(handle);
   BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(!mGeomSlots[slotIndex].mBeingDeleted);

   mGeomSlots[slotIndex].mBeingDeleted = true;

   // We've got to make a full round trip to the worker thread and back in case this model is being rendered.
   gRenderThread.submitCommand(mCommandListenerHandle, cRCDeinit, mGeomSlots[slotIndex].mpGeom);

   gRenderThread.kickCommands();
}

//============================================================================
// BUGXGeomManager::getLastRecordedStatus
//============================================================================
eUGXGeomStatus BUGXGeomManager::getLastRecordedStatus(BEventReceiverHandle handle)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);

   const uint slotIndex = gEventDispatcher.getHandlePrivateData(handle);
   BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(!mGeomSlots[slotIndex].mBeingDeleted);            

   return mGeomSlots[slotIndex].mStatus;
}

//============================================================================
// BUGXGeomManager::getGeomRenderByHandle
//============================================================================
BUGXGeomRender* BUGXGeomManager::getGeomRenderByHandle(BEventReceiverHandle handle) const
{
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);

   const uint slotIndex = gEventDispatcher.getHandlePrivateData(handle);
   // This shouldn't happen normally, but reloading arghhhh
   //BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(slotIndex < mGeomSlots.size());
   
   return mGeomSlots[slotIndex].mpGeom;
}

//============================================================================
// BUGXGeomManager::getGeomRenderByIndex
//============================================================================
BUGXGeomRender* BUGXGeomManager::getGeomRenderByIndex(uint slotIndex) const
{
   //ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);

   //BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(slotIndex < mGeomSlots.size());

   return mGeomSlots[slotIndex].mpGeom;
}

//============================================================================
// BUGXGeomManager::getTotalModels
//============================================================================
uint BUGXGeomManager::getTotalModels(void) const
{
   ASSERT_MAIN_THREAD

   return mTotalModels;
}

//============================================================================
// BUGXGeomManager::getModelStats
//============================================================================
uint BUGXGeomManager::getModelStats(eUGXGeomStatus status) const
{
   ASSERT_MAIN_THREAD

   BDEBUG_ASSERT((status >= 0) && (status < cUGXGeomStatusMax));

   return mModelStats[status];
}

//============================================================================
// BUGXGeomManager::allocSlot
//============================================================================
uint BUGXGeomManager::allocSlot(void)
{
   ASSERT_MAIN_THREAD

   uint i;
   for (i = 0; i < mGeomSlots.size(); i++)
   {
      if (!mGeomSlots[i].mpGeom)
         break;
   }

   if (i == mGeomSlots.size())
   {
      mGeomSlots.resize(mGeomSlots.size() + 1);

      mModelStats[cUGXGeomStatusInvalid]++;
   }

   return i;
}      

//============================================================================
// BUGXGeomManager::freeSlot
//============================================================================
void BUGXGeomManager::freeSlot(uint i)
{
   ASSERT_MAIN_THREAD

   BDEBUG_ASSERT(i < mGeomSlots.size());
   mGeomSlots[i].mpGeom = NULL;
   mGeomSlots[i].mStatus = cUGXGeomStatusInvalid;
   mGeomSlots[i].mBeingDeleted = false;
}

//============================================================================
// BUGXGeomManager::updateGeomStatus
//============================================================================
void BUGXGeomManager::updateGeomStatus(BEventReceiverHandle handle, eUGXGeomStatus newStatus)
{
   ASSERT_MAIN_THREAD

   const uint slot = gEventDispatcher.getHandlePrivateData(handle);
   BDEBUG_ASSERT((slot < mGeomSlots.size()) && (mGeomSlots[slot].mpGeom));

   BDEBUG_ASSERT( (mGeomSlots[slot].mStatus >= 0) && (mGeomSlots[slot].mStatus <= cUGXGeomStatusMax) );
   BDEBUG_ASSERT( (newStatus >= 0) && (newStatus <= cUGXGeomStatusMax) );

   BDEBUG_ASSERT(mModelStats[mGeomSlots[slot].mStatus] >= 1);

   mModelStats[mGeomSlots[slot].mStatus]--;
   mModelStats[newStatus]++;

   mGeomSlots[slot].mStatus = newStatus;
   if (newStatus == cUGXGeomStatusInvalid)
   {
      freeSlot(slot);

      mTotalModels--;
   }
}

//============================================================================
// BUGXGeomManager::initDeviceData
//============================================================================
void BUGXGeomManager::initDeviceData(void) 
{
   Direct3D_CreateDevice(0, D3DDEVTYPE_COMMAND_BUFFER, NULL, 0, NULL, &mpCommandBufferDevice);
   
   mIntrinsicPool.create();
}

//============================================================================
// BUGXGeomManager::frameBegin
//============================================================================
void BUGXGeomManager::frameBegin(void) 
{
   //gUGXGeomEffectManager.frameBegin();
}

//============================================================================
// BUGXGeomManager::processCommand
//============================================================================
void BUGXGeomManager::processCommand(const BRenderCommandHeader& header, const uchar* pData) 
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cRCDeinit:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(const BUGXGeomRender*));
         BUGXGeomRender* pGeom = *reinterpret_cast<BUGXGeomRender* const *>(pData);
         BDEBUG_ASSERT(pGeom);

         // BUGXGeomRender will now send us an event in the main thread to change its status, which will free the slot.
         BAlignedAlloc::Delete(pGeom, gRenderHeap);

         break;
      }
      case cRCSetOptions:
      {
         bool success = mRenderOptions.deserialize(pData, header.mLen);
         BDEBUG_ASSERT(success);
         success;
                  
         break;
      }
   }
}

//============================================================================
// BUGXGeomManager::frameEnd
//============================================================================
void BUGXGeomManager::frameEnd(void) 
{
}

//============================================================================
// BUGXGeomManager::deinitDeviceData
//============================================================================
void BUGXGeomManager::deinitDeviceData(void) 
{
   if (mpCommandBufferDevice)
   {
      if (gpUGXD3DDev == mpCommandBufferDevice)
         gpUGXD3DDev = BD3D::mpDev;

      mpCommandBufferDevice->Release();
      mpCommandBufferDevice = NULL;
   }
   
   mIntrinsicPool.clear();
}

//============================================================================
// BUGXGeomManager::receiveEvent
//============================================================================
bool BUGXGeomManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_MAIN_THREAD

   if (event.mEventClass == cRenderEventClassUGXGeomStatusChanged)
   {
      // Finally, process the model deinit in the main thread.
      updateGeomStatus(event.mFromHandle, static_cast<eUGXGeomStatus>(event.mPrivateData));
   }
  
   return false;
}

//============================================================================
// BUGXGeomManager::waitForAllPending
//============================================================================
void BUGXGeomManager::waitForAllPending(void)
{
   ASSERT_MAIN_THREAD

   uint startTime = GetTickCount();

   for ( ; ; )
   {
      if ((mModelStats[cUGXGeomStatusReady] + mModelStats[cUGXGeomStatusFailed]) == mTotalModels)
         break;

      gEventDispatcher.pumpThreadAndWait(cThreadIndexRender, 8, 4);
   }

   trace("BUGXGeomManager::waitForAllPending: waited %ims", GetTickCount() - startTime);
}

