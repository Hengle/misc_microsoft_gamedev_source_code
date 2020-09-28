//==============================================================================
// File: lightEffectManager.cpp
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================
#include "xgameRender.h"
#include "lightEffectManager.h"
#include "bfileStream.h"
#include "consoleOutput.h"
#include "reloadManager.h"

//==============================================================================
// Globals
//==============================================================================
BLightEffectManager gLightEffectManager;

//==============================================================================
// BLightEffectVisualInstance::BLightEffectVisualInstance
//==============================================================================
BLightEffectVisualInstance::BLightEffectVisualInstance() :
   mElapsedTime(0.0f)
{
}

//==============================================================================
// BLightEffectVisualInstance::~BLightEffectVisualInstance
//==============================================================================
BLightEffectVisualInstance::~BLightEffectVisualInstance()
{
}

//==============================================================================
// BLightEffectVisualInstance::deinit
//==============================================================================
void BLightEffectVisualInstance::deinit()
{
   mElapsedTime = 0.0f;
}

//==============================================================================
// BLightEffectVisualInstance::update
//==============================================================================
void BLightEffectVisualInstance::update(float elapsedTime, bool synced)
{
   synced;
   mElapsedTime = elapsedTime;
}

//==============================================================================
// BLightEffectVisualInstance::updateWorldMatrix
//==============================================================================
void BLightEffectVisualInstance::updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix)
{
#if 0
#ifndef BUILD_FINAL
   for (uint i = 0; i < 16; i++)
   {
      if (!Math::IsValidFloat( ((const float*)&worldMatrix)[i] ) )
      {
         BASSERT(!"BLightEffectVisualInstance::updateWorldMatrix was passed an invalid matrix!");
         break;
      }
      
      if (pLocalMatrix)
      {
         if (!Math::IsValidFloat( ((const float*)pLocalMatrix)[i] ) )
         {
            BASSERT(!"BLightEffectVisualInstance::updateWorldMatrix was passed an invalid matrix!");
            break;
         }
      }
   }      
#endif   
#endif
   
   XMMATRIX newWorldMatrix;
   
   if (pLocalMatrix)
      newWorldMatrix = XMMatrixMultiply(*pLocalMatrix, worldMatrix);
   else
      newWorldMatrix = worldMatrix;
      
   // This assumes updateWorldMatrix() is only called ONCE per frame. 
   if (*(const uint*)&mElapsedTime)
   {
      mInstance.tick(mElapsedTime, newWorldMatrix, gLightEffectManager.getIntensityScale());
      mElapsedTime = 0.0f;
   }
}

//==============================================================================
// BLightEffectVisualInstance::setInstanceFlag
//==============================================================================
void BLightEffectVisualInstance::setVisibility(bool bState)
{
   mInstance.setVisibility(bState);
}

//==============================================================================
// BLightEffectVisualInstance::render
//==============================================================================
void BLightEffectVisualInstance::render(const BVisualRenderAttributes* pRenderAttributes)
{
}

//==============================================================================
// BLightEffectVisualInstance::computeBoundingBox
//==============================================================================
void BLightEffectVisualInstance::computeBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool initCorners)
{
   pMinCorner->set(0,0,0);
   pMaxCorner->set(1,1,1);
}

//==============================================================================
// BLightEffectVisualInstance::getBoneHandle
//==============================================================================
long BLightEffectVisualInstance::getBoneHandle(const char* pBoneName)
{
   return -1;  
}

//==============================================================================
// BLightEffectVisualInstance::getBone
//==============================================================================
bool BLightEffectVisualInstance::getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix, BBoundingBox* pBox, const BMatrix* pOffsetMatrix, bool applyIK)
{
   return false;
}

//==============================================================================
// BLightEffectManager::BLightEffectManager
//==============================================================================
BLightEffectManager::BLightEffectManager() :
   mDirID(0),
   mIntensityScale(1.0f)
{
}

//==============================================================================
// BLightEffectManager::~BLightEffectManager
//==============================================================================
BLightEffectManager::~BLightEffectManager()
{
}

//==============================================================================
// BLightEffectManager::init
//==============================================================================
void BLightEffectManager::init(long dirID) 
{
   mDirID = dirID;
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif
}

//==============================================================================
// BLightEffectManager::deinit
//==============================================================================
void BLightEffectManager::deinit(void)
{
   releaseAllInstances();
         
   clear();
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
   
   eventReceiverDeinit();
#endif
}

//==============================================================================
// BLightEffectManager::clear
//==============================================================================
void BLightEffectManager::clear(void)
{
   for (uint i = 0; i < mEffects.getSize(); i++)
      BAlignedAlloc::Delete(mEffects[i]);
   
   mEffects.clear();
}

//==============================================================================
//==============================================================================
void BLightEffectManager::update(DWORD gameTime, float elapsedTime)
{
   for (int i = mTimedInstances.getNumber() - 1; i >= 0; i--)
   {
      BTimedLightInstance& timedInst = mTimedInstances[i];
      BLightEffectVisualInstance* pInst = timedInst.mInstance;
      if (pInst)
      {
         if (timedInst.mEndTime < gameTime)
         {
            releaseInstance(pInst);
            mTimedInstances.removeIndex(i);
         }
         else
         {
            pInst->mInstance.tick(elapsedTime, timedInst.mWorldMtx, gLightEffectManager.getIntensityScale());
         }
      }
   }
}

//==============================================================================
// BLightEffectManager::getLightEffect
//==============================================================================
const BLightEffectData* BLightEffectManager::getLightEffect(long dataHandle) const
{
   if (dataHandle >=0 && dataHandle < mEffects.getNumber())
      return mEffects[dataHandle];
   else
      return NULL;
}

//==============================================================================
// BLightEffectManager::getOrCreateData
//==============================================================================
void BLightEffectManager::getOrCreateData(const char* pName, long& dataHandle)
{
   //SCOPEDSAMPLE(BLightEffectManager_getOrCreateData)
   uint i;
   for (i = 0; i < mEffects.getSize(); i++)
   {
      if (mEffects[i]->getName().compare(pName) == 0)
      {
         dataHandle = i;
         return;
      }
   }
   
   BLightEffectData* pEffect = BAlignedAlloc::New<BLightEffectData>();
   
   BFileSystemStream stream;
      
   bool succeeded = false;
   
   BString filename(pName);
   if (!strPathHasExtension(filename, LIGHT_EFFECT_EXTENSION))
      filename.append("." LIGHT_EFFECT_EXTENSION);
   
   if (stream.open(mDirID, filename, cSFReadable | cSFEnableBuffering | cSFDiscardOnClose))
   {
      stream.setLittleEndian(true);
      
      succeeded = pEffect->load(pName, stream);
   }
   
   if (!succeeded)
   {
      gConsoleOutput.output(cMsgError, "BLightEffectManager::init: Failed loading light effect file %s", filename.getPtr());  
   }
   else
   {
      gConsoleOutput.output(cMsgResource, "BLightEffectManager::init: Loaded light effect file %s", filename.getPtr());  
   }
      
   dataHandle = mEffects.getSize();
   mEffects.pushBack(pEffect);
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.registerClient(mDirID, filename, BReloadManager::cFlagSynchronous, mEventHandle, cEventClassReloadNotify, dataHandle);
#endif
}

//==============================================================================
// BLightEffectManager::createInstance
//==============================================================================
BLightEffectVisualInstance* BLightEffectManager::createInstance(int dataHandle, BMatrix matrix)  
{
   BDEBUG_ASSERT((dataHandle >= 0) && (dataHandle < (int)mEffects.getSize()));
   
   BLightEffectVisualInstance* pInstance = BAlignedAlloc::New<BLightEffectVisualInstance>();
   
   pInstance->mInstance.init(*mEffects[dataHandle]);  
            
   mInstances.pushBack(pInstance);
   
   return pInstance;
}

//==============================================================================
// BLightEffectManager::createTimedInstance
//==============================================================================
void BLightEffectManager::createTimedInstance(int dataHandle, BMatrix matrix, DWORD endTime)
{
   BLightEffectVisualInstance* pInst = createInstance(dataHandle, matrix);
   if (pInst)
   {
      BTimedLightInstance timedInst;
      timedInst.mWorldMtx = matrix;
      timedInst.mInstance = pInst;
      timedInst.mEndTime = endTime;
      mTimedInstances.add(timedInst);
   }
}

//==============================================================================
// BLightEffectManager::createCameraInstance
//==============================================================================
BCameraEffectInstance* BLightEffectManager::createCameraInstance(int dataHandle, BMatrix matrix)  
{
   BDEBUG_ASSERT((dataHandle >= 0) && (dataHandle < (int)mEffects.getSize()));

   BCameraEffectInstance* pCameraInstance = BAlignedAlloc::New<BCameraEffectInstance>();

   pCameraInstance->init(*mEffects[dataHandle]);  

   mCameraInstances.pushBack(pCameraInstance);

   return pCameraInstance;
}

//==============================================================================
// BLightEffectManager::releaseInstance
//==============================================================================
void BLightEffectManager::releaseInstance(BLightEffectVisualInstance* pInstance)
{
   if (pInstance)
   {
      const bool succeeded = mInstances.remove(pInstance, false);
      succeeded;
      BDEBUG_ASSERT(succeeded);
   
      BAlignedAlloc::Delete(pInstance);
   }
}

//==============================================================================
// BLightEffectManager::releaseCameraInstance
//==============================================================================
void BLightEffectManager::releaseCameraInstance(BCameraEffectInstance* pInstance)
{
   if (pInstance)
   {
      const bool succeeded = mCameraInstances.remove(pInstance, false);
      if(succeeded)
      {
         BAlignedAlloc::Delete(pInstance);
      }
   }
}


//==============================================================================
// BLightEffectManager::releaseAllInstances
//==============================================================================
void BLightEffectManager::releaseAllInstances(void)
{
   for (uint i = 0; i < mInstances.getSize(); i++)
      BAlignedAlloc::Delete(mInstances[i]);

   mInstances.clear();
   mTimedInstances.clear();

   for (uint i = 0; i < mCameraInstances.getSize(); i++)
      BAlignedAlloc::Delete(mCameraInstances[i]);

   mCameraInstances.clear();
}

//==============================================================================
// BLightEffectManager::reloadEffect
//==============================================================================
void BLightEffectManager::reloadEffect(int dataHandle)
{
   BLightEffectData* pEffect = mEffects[dataHandle];
   
   BFileSystemStream stream;

   bool succeeded = false;
   
   BString name(pEffect->getName());

   BString filename(name);
   if (!strPathHasExtension(filename, LIGHT_EFFECT_EXTENSION))
      filename.append("." LIGHT_EFFECT_EXTENSION);
      
   gConsoleOutput.status("Reloading light effect: %s", filename.getPtr());      

   if (stream.open(mDirID, filename, cSFReadable | cSFEnableBuffering))
   {
      stream.setLittleEndian(true);

      succeeded = pEffect->load(name.getPtr(), stream);
   }

   if (!succeeded)
   {
      gConsoleOutput.output(cMsgError, "BLightEffectManager::init: Failed loading light effect file %s", filename.getPtr());  
   }
   else
   {
      gConsoleOutput.output(cMsgResource, "BLightEffectManager::init: Reloaded light effect file %s", filename.getPtr());  
   }
}

//==============================================================================
// BLightEffectManager::receiveEvent
//==============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BLightEffectManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassReloadNotify:
      {
//-- FIXING PREFIX BUG ID 6439
         const BReloadManager::BNotificationPayload* pPayload = (BReloadManager::BNotificationPayload*)event.mpPayload;
//--
         
         int handle = pPayload->mData;
         if ((handle >= 0) && (handle < (int)mEffects.getSize()))
            reloadEffect(handle);
         
         break;
      }
   }
   
   return false;
}
#endif