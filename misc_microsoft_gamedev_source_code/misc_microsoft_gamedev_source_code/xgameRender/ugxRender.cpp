#if 0
//============================================================================
//
//  ugxRender.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "ugxRender.h"

#include "granny.h"

#include "grannyToUnigeom.h"
#include "ugxInstancer.h"
#include "renderThread.h"
#include "renderDraw.h"

#include "effectCompiler.h"

BUGXGeomManager gUGXGeomManager;

BUGXGeom::BUGXGeom(BEventReceiverHandle ownerHandle) :
   mStatus(cUGXGeomStatusInvalid),
   mOwnerEventHandle(ownerHandle),
   mNumEffectsRemaining(0),
   mNumTexturesRemaining(0)
{
   ASSERT_MAIN_THREAD
      
   mEventHandle = gEventDispatcher.addClient(this, cThreadIndexRender);
}

BEventReceiverHandle BUGXGeom::getEventHandle(void) const 
{ 
   return mEventHandle; 
}

void BUGXGeom::setEventHandle(BEventReceiverHandle handle) 
{ 
   mEventHandle = handle;
}
 
BEventReceiverHandle BUGXGeom::getOwnerEventHandle(void) const 
{ 
   ASSERT_WORKER_THREAD
   return mOwnerEventHandle; 
}

void BUGXGeom::setOwnerEventHandle(BEventReceiverHandle handle) 
{ 
   ASSERT_WORKER_THREAD
   mOwnerEventHandle = handle;
}

eUGXGeomStatus BUGXGeom::getStatus(void)
{
   ASSERT_WORKER_THREAD;
   return mStatus;
}
      
BUGXGeom::~BUGXGeom()
{
   ASSERT_WORKER_THREAD
   
   changeStatus(cUGXGeomStatusInvalid);
         
   gEventDispatcher.removeClient(mEventHandle);
}

void BUGXGeom::changeStatus(eUGXGeomStatus newStatus)
{
   if (mStatus == newStatus)
      return;

   trace("BUGXGeom::changeStatus: %i", newStatus);
            
   mStatus = newStatus;
   
   if (cInvalidEventReceiverHandle != mOwnerEventHandle)
      gEventDispatcher.send(mEventHandle, mOwnerEventHandle, cRenderEventClassUGXGeomStatusChanged, newStatus);
   
   gEventDispatcher.send(mEventHandle, gUGXGeomManager.getEventHandle(), cRenderEventClassUGXGeomStatusChanged, newStatus);
}
   
void BUGXGeom::checkIfReady(void)
{
   ASSERT_WORKER_THREAD
   
   if (mStatus != cUGXGeomStatusPending)
      return;
   
   if ((mNumEffectsRemaining == 0) && (mNumTexturesRemaining == 0))
      changeStatus(cUGXGeomStatusReady);
}

bool BUGXGeom::init(granny_file_info* pFileInfo, long effectDirID)
{
   ASSERT_WORKER_THREAD
   
   BASSERT(mStatus == cUGXGeomStatusInvalid);
   BASSERT(pFileInfo);
      
   if ((!pFileInfo->ModelCount) || (!pFileInfo->MeshCount))
   {
      changeStatus(cUGXGeomStatusFailed);
      return false;
   }

   BTraceTextDispatcher textDispatcher;
   GrannyToUnigeom grannyToUnigeom(pFileInfo, true, true, &textDispatcher);
   if (!grannyToUnigeom.getSuccess())
   {
      changeStatus(cUGXGeomStatusFailed);
      return false;
   }

   const Unigeom::Geom& ugfGeom = grannyToUnigeom.getGeom();

   UGXGeomInstancer ugxGeom(ugfGeom, &textDispatcher, true);

   mGeomRenderBase.setGeom(ugxGeom.getGeom());
               
   mTextureHandle = gpThreadTextureManager->getOrCreate("warthog_diffuse", cTRTStatic, mEventHandle);
   if (mTextureHandle == BThreadTextureManager::cInvalidHandle)
   {
      changeStatus(cUGXGeomStatusFailed);
      return false;
   }
   
   gpThreadTextureManager->load(mTextureHandle);
                           
   mNumTexturesRemaining = 1;
                     
   mNumEffectsRemaining = 0;

   BEffectCompilerRequestPacket* pEffectRequest = gAsyncEffectCompiler.newRequestPacket();
   pEffectRequest->setFilename("testShader.fx");
   pEffectRequest->setDirID(effectDirID);
   pEffectRequest->setEventHandle(mEventHandle);
                           
   if (!gAsyncEffectCompiler.submitRequest(pEffectRequest))
   {
      changeStatus(cUGXGeomStatusFailed);
      return false;
   }
   
   mNumEffectsRemaining = 1;
   
   changeStatus(cUGXGeomStatusPending);
         
   return true;
}
   
void BUGXGeom::render(const BMatrix44& matrix, const BVec4& color)
{
   ASSERT_WORKER_THREAD
   
   if (mStatus != cUGXGeomStatusReady)
      return;
   
   mGeomRenderBase.setIndexBuffer();
   
   // World matrix (identity in this sample)
   XMMATRIX matWorld = XMLoadFloat4x4((const XMFLOAT4X4*)matrix.getPtr());
   XMMATRIX matView  = XMLoadFloat4x4((const XMFLOAT4X4*)&gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cWorldToView, false));
   XMMATRIX matProj = XMLoadFloat4x4((const XMFLOAT4X4*)&gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cViewToProj, false));

   // World*view*projection
   XMMATRIX matWVP = matWorld * matView * matProj;
   
   BFXLEffectParam param;
   param = mEffect("matWVP");
   param = matWVP;

   mEffect("color") = color;
   
   mEffect("DiffuseSampler") = gpThreadTextureManager->get(mTextureHandle, true)->getD3DTexture().getTexture();

   BFXLEffectTechnique technique(mEffect.getTechniqueFromIndex(0));   
   technique.begin();
   technique.beginPass();

   technique.commit();         

   for (uint i = 0; i < mGeomRenderBase.getNumSections(); i++)
      mGeomRenderBase.drawSection(i);
      
   technique.endPass();
   technique.end();            
}
   
bool BUGXGeom::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_WORKER_THREAD
      
   switch (event.mEventClass)
   {
      case cRenderEventClassUGXGeomInit:
      {
         trace("BUGXGeom::receiveEvent: cRenderEventClassUGXGeomInit");
         
         init(reinterpret_cast<granny_file_info*>(event.mPrivateData), static_cast<long>(event.mPrivateData2));
         break;
      }
      case cRenderEventClassTextureStatusChanged:
      {
         const eTextureStatus status = static_cast<eTextureStatus>(event.mPrivateData2);
         
         trace("BUGXGeom::receiveEvent: cRenderEventClassTextureStatusChanged %i", status);
         
         switch (status)
         {
            case cTextureStatusInvalid:
            case cTextureStatusLoadFailed:
            {
               changeStatus(cUGXGeomStatusFailed);
               break;
            }
            case cTextureStatusReady:
            {
               mNumTexturesRemaining--;
               break;      
            }
            case cTextureStatusNotReady:
            case cTextureStatusLoadPending:
            {
               break;
            }
            default:
            {
               BFAIL("Unexpected texture status");
            }
         }
         
         checkIfReady();                        
         break;
      }
      case cRenderEventClassEffectCompileResults:
      {
         BEffectCompilerRequestPacket* pEffectPacket = reinterpret_cast<BEffectCompilerRequestPacket*>(event.mpPayload);
         
         trace("BUGXGeom::receiveEvent: cRenderEventClassEffectCompileResults %i", pEffectPacket->getSucceeded());
         
         if (!pEffectPacket->getSucceeded())
         {
            changeStatus(cUGXGeomStatusFailed);
         }
         else
         {
            HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, (void*)pEffectPacket->getCompiledData().getPtr());
            if (FAILED(hres))
            {
               changeStatus(cUGXGeomStatusFailed);
            }
            else
            {
               mNumEffectsRemaining--;
            }
         }
         
         checkIfReady();
         break;
      }
      default:
      {
         BFAIL("BUGXGeom::receiveEvent: Invalid event class");
      }
   }

   return false;
}

BUGXGeomManager::BUGXGeomManager() :
   mEffectDirID(0),
   mTotalModels(0),
   mEventHandle(cInvalidEventReceiverHandle),
   mCommandListenerHandle(cInvalidCommandListenerHandle)
{
   mGeomSlots.reserve(128);
   
   Utils::ClearObj(mModelStats);
}
   
BUGXGeomManager::~BUGXGeomManager()
{
}
   
void BUGXGeomManager::init(void)
{
   ASSERT_MAIN_THREAD   
   
   if (cInvalidCommandListenerHandle != mCommandListenerHandle)
      return;
   
   mEffectDirID = 0;
   
   mTotalModels = 0;
   Utils::ClearObj(mModelStats);
   
   mModelStats[cUGXGeomStatusInvalid] = mGeomSlots.size();
   
   for (uint i = 0; i < mGeomSlots.size(); i++)
      mGeomSlots[i].clear();
            
   mEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);
   mCommandListenerHandle = gRenderThread.registerCommandListener(this);
}

void BUGXGeomManager::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   if (cInvalidCommandListenerHandle == mCommandListenerHandle)
      return;
   
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
   while (getTotalModels() > 0)
   {
      gEventDispatcher.wait(0, NULL, 10);         
   }
                                 
   gRenderThread.blockUntilWorkerIdle();
   
   gRenderThread.freeCommandListener(mCommandListenerHandle);
   mCommandListenerHandle = cInvalidCommandListenerHandle;
   
   gEventDispatcher.removeClient(mEventHandle);
   mEventHandle = cInvalidEventReceiverHandle;
}
   
void BUGXGeomManager::setEffectDirID(long dirID)
{
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);
   
   mEffectDirID = dirID;
}
         
BEventReceiverHandle BUGXGeomManager::addGeom(granny_file_info* pFileInfo, BEventReceiverHandle ownerHandle)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);
   
   const uint slotIndex = allocSlot();
                           
   BUGXGeom* pGeom = new BUGXGeom(ownerHandle);
   
   mGeomSlots[slotIndex].mpGeom = pGeom;
   mGeomSlots[slotIndex].mStatus = cUGXGeomStatusInvalid;
   mGeomSlots[slotIndex].mBeingDeleted = false;
   
   mTotalModels++;
      
   pGeom->setEventHandle(gEventDispatcher.setHandlePrivateData(pGeom->getEventHandle(), slotIndex));
   
   gEventDispatcher.send(mEventHandle, pGeom->getEventHandle(), cRenderEventClassUGXGeomInit, (uint)pFileInfo, (uint)mEffectDirID);      
   
   return pGeom->getEventHandle();
}
   
void BUGXGeomManager::removeGeom(BEventReceiverHandle handle)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);
   
   const uint slotIndex = gEventDispatcher.getHandlePrivateData(handle);
   BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(!mGeomSlots[slotIndex].mBeingDeleted);
   
   mGeomSlots[slotIndex].mBeingDeleted = true;
   
   // Holy fucking shit, we've got to make a full round trip to the worker thread and back 
   // in case this model is being rendered. 
   gRenderThread.submitCommand(mCommandListenerHandle, cRCDeinit, mGeomSlots[slotIndex].mpGeom);
   
   gRenderThread.kickCommands();
}
      
void BUGXGeomManager::renderGeom(BEventReceiverHandle handle, const BMatrix44& matrix, const BVec4& color)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);
   
   const uint slotIndex = gEventDispatcher.getHandlePrivateData(handle);
   BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(!mGeomSlots[slotIndex].mBeingDeleted);            

   BRenderInstanceData instanceData;
   instanceData.mpGeom = mGeomSlots[slotIndex].mpGeom;
   instanceData.mMatrix = matrix;
   instanceData.mColor = color;
         
   gRenderThread.submitCommand(mCommandListenerHandle, cRCRender, instanceData);
}

eUGXGeomStatus BUGXGeomManager::getLastRecordedStatus(BEventReceiverHandle handle)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(cInvalidCommandListenerHandle != mCommandListenerHandle);
   
   const uint slotIndex = gEventDispatcher.getHandlePrivateData(handle);
   BDEBUG_ASSERT((slotIndex < mGeomSlots.size()) && (mGeomSlots[slotIndex].mpGeom));
   BDEBUG_ASSERT(!mGeomSlots[slotIndex].mBeingDeleted);            
   
   return mGeomSlots[slotIndex].mStatus;
}
   
uint BUGXGeomManager::getTotalModels(void) const
{
   ASSERT_MAIN_THREAD
   
   return mTotalModels;
}

uint BUGXGeomManager::getModelStats(eUGXGeomStatus status) const
{
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT((status >= 0) && (status < cUGXGeomStatusMax));
   
   return mModelStats[status];
}
  
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
   
void BUGXGeomManager::freeSlot(uint i)
{
   ASSERT_MAIN_THREAD
   
   BDEBUG_ASSERT(i < mGeomSlots.size());
   mGeomSlots[i].mpGeom = NULL;
   mGeomSlots[i].mStatus = cUGXGeomStatusInvalid;
   mGeomSlots[i].mBeingDeleted = false;
}
   
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
      
void BUGXGeomManager::initDeviceData(void) 
{
}

void BUGXGeomManager::frameBegin(void) 
{
}
      
void BUGXGeomManager::processCommand(const BRenderCommandHeader& header, const uchar* pData) 
{
   ASSERT_WORKER_THREAD
   
   switch (header.mType)
   {
      case cRCDeinit:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(const BUGXGeom*));
         const BUGXGeom* pGeom = *reinterpret_cast<const BUGXGeom* const *>(pData);
         BDEBUG_ASSERT(pGeom);
                                             
         // BUGXGeom will now send us an event in the main thread to change its status, which will free the slot.
         // Yes this is insane.
         delete pGeom;
                                 
         break;
      }
      case cRCRender:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderInstanceData));
         
         const BRenderInstanceData* pInstanceData = reinterpret_cast<const BRenderInstanceData*>(pData);
                     
         pInstanceData->mpGeom->render(pInstanceData->mMatrix, pInstanceData->mColor);
         
         break;
      }
   }
}

void BUGXGeomManager::frameEnd(void) 
{
}

void BUGXGeomManager::deinitDeviceData(void) 
{
}

bool BUGXGeomManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_MAIN_THREAD
   
   if (event.mEventClass == cRenderEventClassUGXGeomStatusChanged)
   {
      // Finally, process the model deinit in the main thread.
      updateGeomStatus(event.mFromHandle, static_cast<eUGXGeomStatus>(event.mPrivateData));
   }
   else
   {
      BFAIL("BUGXGeomManager::receiveEvent: Invalid event class");
   }
   
   return false;
}

void BUGXGeomManager::waitForAllPending(void)
{
   ASSERT_MAIN_THREAD
   
   uint startTime = GetTickCount();
   
   for ( ; ; )
   {
      if ((mModelStats[cUGXGeomStatusReady] + mModelStats[cUGXGeomStatusFailed]) == mTotalModels)
         break;
         
      gEventDispatcher.wait(0, NULL, 16);
   }
   
   trace("BUGXGeomManager::waitForAllPending: waited %ims", GetTickCount() - startTime);
}
#endif