//============================================================================
//
//  ugxGeomUberEffectManager.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"
#include "ugxGeomUberEffectManager.h"

// xcore
#include "asyncFileManager.h"
#include "reloadManager.h"
#include "consoleOutput.h"

// xgameRender/xrender
#include "BD3D.h"

//============================================================================
// Globals
//============================================================================
BUGXGeomUberEffectManager gUGXGeomUberEffectManager;

//============================================================================
// BUGXGeomUberEffectManager::BUGXGeomUberEffectManager
//============================================================================
BUGXGeomUberEffectManager::BUGXGeomUberEffectManager() :
   BEventReceiver(),
   mLoadCount(0),
   mpIntrinsicPool(NULL)
{
}

//============================================================================
// BUGXGeomUberEffectManager::~BUGXGeomUberEffectManager
//============================================================================
BUGXGeomUberEffectManager::~BUGXGeomUberEffectManager()
{
}

//============================================================================
// BUGXGeomUberEffectManager::init
//============================================================================
void BUGXGeomUberEffectManager::init(long effectDirID, BFXLEffectIntrinsicPool* pIntrinsicPool)
{
   ASSERT_THREAD(cThreadIndexSim);

   mEffectDirID = effectDirID;
   mpIntrinsicPool = pIntrinsicPool;

   eventReceiverInit(cThreadIndexRender);
}

//============================================================================
// BUGXGeomUberEffectManager::deinit
//============================================================================
void BUGXGeomUberEffectManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   eventReceiverDeinit();
   
   mpIntrinsicPool = NULL;
}

//============================================================================
// BUGXGeomUberEffectManager::changeDevice
//============================================================================
void BUGXGeomUberEffectManager::changeDevice(IDirect3DDevice9* pDev)
{
   for (uint i = 0; i < cMaxEffects; i++)
   {
      if (mEffects[i].getEffect())
         mEffects[i].getEffect()->ChangeDevice(pDev);
   }
}

//============================================================================
// BUGXGeomUberEffectManager::getEffect
//============================================================================
BFXLEffect& BUGXGeomUberEffectManager::getEffect(bool bumpMapped, bool disableDirShadowReception) 
{ 
   uint effectIndex = bumpMapped;
#if 0   
   if (disableDirShadowReception)
      effectIndex += 2;
#endif      
   disableDirShadowReception;
   
   BDEBUG_ASSERT(effectIndex < cMaxEffects); 
   return mEffects[effectIndex]; 
}

//============================================================================
// BUGXGeomUberEffectManager::loadEffects
//============================================================================
void BUGXGeomUberEffectManager::loadEffects(int index)
{
   ASSERT_THREAD(cThreadIndexRender);

   int first = 0;
   int last = cMaxEffects;
   if (index >= 0)
   {
      first = index;
      last = first + 1;
   }

   for (int i = first; i < last; i++)
   {
      BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

      BFixedString256 filename;
      filename.format("ugx\\parametricShader%i.bin", i);

      pPacket->setFilename(filename);
      pPacket->setDirID(mEffectDirID);
      pPacket->setReceiverHandle(mEventHandle);
      pPacket->setPrivateData0(i);
      pPacket->setSynchronousReply(true);
      pPacket->setDiscardOnClose(true);

      gAsyncFileManager.submitRequest(pPacket);
   }
}

//============================================================================
// BUGXGeomUberEffectManager::receiveEvent
//============================================================================
bool BUGXGeomUberEffectManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         loadEffects();

         BString filename;
         eFileManagerError result = gFileManager.getDirListEntry(filename, mEffectDirID);
         BVERIFY(result == cFME_SUCCESS);

         strPathAddBackSlash(filename);

         filename += "ugx\\parametricShader*.bin";

         BReloadManager::BPathArray paths;
         paths.pushBack(filename);
         gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, cUGXGeomReloadEventClass);

         break;
      }
      case cEventClassClientRemove:
      {
         for (uint i = 0; i < cMaxEffects; i++)
            mEffects[i].clear();

         gReloadManager.deregisterClient(mEventHandle);

         break;
      }
      case cEventClassAsyncFile:
      {
         BAsyncFileManager::BRequestPacket* pFileRequestPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket*>(event.mpPayload);
         uint slot = pFileRequestPacket->getPrivateData0();

         if (mEffects[slot].getEffect())
            break;

         if (pFileRequestPacket->getSucceeded())
         {
            HRESULT hres = mEffects[slot].createFromCompiledData(BD3D::mpDev, (void*)pFileRequestPacket->getData(), mpIntrinsicPool);
            if (FAILED(hres))
            {
               BFATAL_FAIL("BUGXGeomUberEffectManager::receiveEvent: BFXLEffect::createFromCompiledData failed");
            }
            else
            {
               gConsoleOutput.output(cMsgResource, "BUGXGeomUberEffectManager: Load successful: %s", pFileRequestPacket->getFilename().c_str());
            }
         }
         else
         {
            BString msg;
            msg.format("BUGXGeomUberEffectManager::receiveEvent: Unable to read shader file: %s", pFileRequestPacket->getFilename().c_str());
            BFATAL_FAIL(msg.getPtr());
         }

         if (mEffects[slot].getNumTechniques() < 1)
         {
            BFATAL_FAIL("BUGXGeomRenderSection::finishEffectInit: At least one technique required");
         }

         if (mEffects[slot].getNumTechniques() < cUberEffectTechniqueNum)
         {
            BFATAL_FAIL("BUGXGeomRenderSection::finishEffectInit: Not enough techniques");
         }

         mEffects[slot].setAllUserParamsToManualRegUpdate();
         
         mLoadCount++;

         break;
      }
      case cUGXGeomReloadEventClass:
      {
//-- FIXING PREFIX BUG ID 6435
         const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--

         BString filename(pPayload->mPath);
         filename.toLower();
         const int charIndex = filename.findLeft("parametricshader");
         if (charIndex >= 0)
         {
            const int effectIndex = atoi(filename.getPtr() + charIndex + strlen("parametricShader"));
            if ((effectIndex >= 0) && (effectIndex < cMaxEffects))
            {
               mEffects[effectIndex].clear();
               loadEffects(effectIndex);      
            }
         }

         break;
      }
   }

   return false;
}
