//==============================================================================
// visualmanager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "visualmanager.h"
#include "grannymanager.h"
#include "protovisual.h"
#include "visual.h"
#include "reloadManager.h"
#include "config.h"
#include "econfigenum.h"
#include "grannyinstance.h"
#include "threading\workDistributor.h"

#include "../xgame/archiveManager.h"
#include "../xgame/savegame.h"

// Global gVisualManager
BVisualManager gVisualManager;

extern char *gAnimTypeNames[];

#define DEBUG_VM_INTRUSIVE_SYNCING

//==============================================================================
// BVisualManager::BVisualManager
//==============================================================================
BVisualManager::BVisualManager() :
   mDirID(-1),
   mProtoVisualList(),
   mProtoVisualTable(),
   mVisualHandlers(),
   mProtoVisualHandlers(),
   mAnimTypeTable(),
   mAttachmentTypeTable(),
   mNextAnimType(0),
   mNextAttachmentType(0),
   mpDamageTemplateInterface(NULL)
   // SLB: This is made redundant by Sergio's changes.
//    mSavedState_ProtoVisualListCount(-1),
//    mSavedState_AnimTypeCount(-1),
//    mSavedState_AttachmentTypeCount(-1)
{
}

//==============================================================================
// BVisualManager::~BVisualManager
//==============================================================================
BVisualManager::~BVisualManager()
{
}

//==============================================================================
// BVisualManager::saveState
//==============================================================================
void BVisualManager::saveState()
{
   // SLB: This is made redundant by Sergio's changes.
//    mSavedState_ProtoVisualListCount = mProtoVisualList.getNumber();
//    mSavedState_AnimTypeCount = mNextAnimType;
//    mSavedState_AttachmentTypeCount = mNextAttachmentType;
}

//==============================================================================
// BVisualManager::restoreState
//==============================================================================
void BVisualManager::restoreState()
{
   // SLB: This is made redundant by Sergio's changes.
//    // Remove attachment types that were added after the state save
//    long attachmentCount = mAttachmentTypeTable.numTags();
//    const BStringTableLong::BTagsArrayType& attachmentTagArray = mAttachmentTypeTable.getTags();
//    for (long i = attachmentCount - 1; i >= mSavedState_AttachmentTypeCount; i--)
//       mAttachmentTypeTable.remove(attachmentTagArray[i]);
//    mNextAttachmentType = mSavedState_AttachmentTypeCount;
//    BASSERT(mSavedState_AttachmentTypeCount == mAttachmentTypeTable.numTags());
// 
//    // Remove anim types that were added after the state save
//    long animCount = mAnimTypeTable.numTags();
//    const BStringTableLong::BTagsArrayType& animTagArray = mAnimTypeTable.getTags();
//    for (long i = animCount - 1; i >= mSavedState_AnimTypeCount; i--)
//       mAnimTypeTable.remove(animTagArray[i]);
//    mNextAnimType = mSavedState_AnimTypeCount;
//    BASSERT(mSavedState_AnimTypeCount == mAnimTypeTable.numTags());
// 
//    // Remove proto visuals that were added after the state save
//    const BStringTableLong::BTagsArrayType& protoVisualTagArray = mProtoVisualTable.getTags();
//    for (long i = mProtoVisualList.getNumber() - 1; i >= mSavedState_ProtoVisualListCount; i--)
//    {
//       mProtoVisualTable.remove(protoVisualTagArray[i]);
//       mProtoVisualList.removeIndex(i);
//    }
//    BASSERT(mSavedState_ProtoVisualListCount == mProtoVisualList.getNumber());
//    BASSERT(mSavedState_ProtoVisualListCount == mProtoVisualTable.numTags());
}

//==============================================================================
// BVisualManager::init
//==============================================================================
bool BVisualManager::init(long dirID, const BCHAR_T* pDirName)
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif

   mDirID=dirID;
   if(!gGrannyManager.init(pDirName))
      return false;

   // Define the hard-coded anim types.
   BSimString name;
   for(long i=0; i<cAnimTypeMaxCount; i++)
   {
      name=gAnimTypeNames[i];
      getAnimType(name);
   }

   // Callback Data   
   mpDamageTemplateInterface = NULL;
#ifdef ENABLE_RELOAD_MANAGER
   // Setup reloading of vis files
   BReloadManager::BPathArray paths;
   paths.pushBack("*.vis");
   gReloadManager.registerClient(paths, BReloadManager::cFlagSubDirs | BReloadManager::cFlagSynchronous, mEventHandle);
#endif
   return true;
}

//==============================================================================
// BVisualManager::deinit
//==============================================================================
void BVisualManager::deinit()
{
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
#endif

   gGrannyManager.deinit();

#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit();
#endif
}

//============================================================================
//============================================================================
void BVisualManager::gameInit()
{
   gGrannyManager.resetTotalStats();

   if (gConfig.isDefined(cConfigGrannySampleAnimCache))
      gGrannyManager.setEnableSampleAnimCache(true);
   else
      gGrannyManager.setEnableSampleAnimCache(false);

   gGrannyManager.gameInit();
}

//============================================================================
// BVisualManager::update
//============================================================================
void BVisualManager::update(void)
{
   SCOPEDSAMPLE(VisualManagerUpdate);

   if (gConfig.isDefined(cConfigGrannySampleAnimCache))
      gGrannyManager.setEnableSampleAnimCache(true);
   else
      gGrannyManager.setEnableSampleAnimCache(false);

   if (gGrannyManager.getBoundingBoxesDirty())
   {
      recomputeProtoVisualBoundingBoxes();   
   }
}

//============================================================================
// BVisualManager::getOrCreateProtoVisual
//============================================================================
long BVisualManager::getOrCreateProtoVisual(const BCHAR_T* pName, bool loadFile)
{
   //SCOPEDSAMPLE(BVisualManager_getOrCreateProtoVisual)
   if(!pName || !pName[0])
      return -1;

   #ifdef SYNC_World
      syncWorldData("BVisualManager::getOrCreateProtoVisual pName", pName);
      syncWorldData("BVisualManager::getOrCreateProtoVisual loadFile", loadFile);
   #endif
            
   BFixedString256 name(pName);
   name.removeExtension();
   name.standardizePath();
   
   long index=findProtoVisual(name);

   #ifdef SYNC_World
      syncWorldData("BVisualManager::getOrCreateProtoVisual index", index);
   #endif

   // The loadFile parameter means different things in different contexts.  On loose files builds (non archive builds)
   // it's whether the VIS file and all its dependent assets should be loaded.  On archive builds it means whether the
   // dependent assets should be loaded or not.

   if (gArchiveManager.getArchivesEnabled())
   {
      #ifdef SYNC_World
         syncWorldCode("BVisualManager::getOrCreateProtoVisual archive");
      #endif
      if(index<0)
      {
         index=createProtoVisual(name);

         #ifdef SYNC_World
            syncWorldData("BVisualManager::getOrCreateProtoVisual created proto visual with index", index);
         #endif

         BProtoVisual* pProtoVisual = getProtoVisual(index, false);
         BASSERT(pProtoVisual);

         pProtoVisual->load(); 
         handleProtoVisualLoaded(pProtoVisual);
      }

      BASSERT(index != -1);

      if(loadFile)
      {
         BProtoVisual* pProtoVisual = getProtoVisual(index, false);
         BASSERT(pProtoVisual);

         if (pProtoVisual->isLoaded() && !pProtoVisual->areAllAssetsLoaded())
         {
            pProtoVisual->loadAllAssets();
         }
      }
   }
   else
   {
      #ifdef SYNC_World
         syncWorldCode("BVisualManager::getOrCreateProtoVisual non-archive");
      #endif

      if(index<0)
         index=createProtoVisual(name);

      #ifdef SYNC_World
         syncWorldData("BVisualManager::getOrCreateProtoVisual created proto visual with index", index);
      #endif

      BASSERT(index != -1);

      if(loadFile)
      {
         BProtoVisual* pProtoVisual = getProtoVisual(index, false);
         BASSERT(pProtoVisual);

         if (!pProtoVisual->isLoaded() && !pProtoVisual->loadFailed())
         {
            pProtoVisual->load();
            handleProtoVisualLoaded(pProtoVisual);

            pProtoVisual->loadAllAssets();
         }
      }
   }

   return index;
}

//============================================================================
// BVisualManager::findProtoVisual
//============================================================================
long BVisualManager::findProtoVisual(const BCHAR_T* pName)
{
   BFixedString256 name(pName);
   name.removeExtension();
   name.standardizePath();
   
   long index=-1;
   if(mProtoVisualTable.find(name, &index))
      return index;
   return -1;
}

//============================================================================
// BVisualManager::createProtoVisual
//============================================================================
long BVisualManager::createProtoVisual(const BCHAR_T* pName)
{
   // ATTENTION: When running with archives don't allow any creation after BDatabase::setup
   //
   if (gArchiveManager.getArchivesEnabled() && gArchiveManager.getIsGameInitialized() && !gSaveGame.isLoading())
   {
      BSimString messageString;
      messageString.format("BVisualManager::createProtoVisual - Loading a visual after BDatabase::setup is not supported when running with archives.  Visual name: %s", pName);
      BASSERTM(false, messageString.getPtr());

      return -1;
   }


   //SCOPEDSAMPLE(BProtoVisual_createProtoVisual)
   BFixedString256 name(pName);
   name.removeExtension();
   name.standardizePath();
   
   BProtoVisual* pProtoVisual = new BProtoVisual;
   if(!pProtoVisual)
      return -1;

   long index = mProtoVisualList.getNumber();

   #ifdef SYNC_World
      syncWorldData("BVisualManager::createProtoVisual Initing protovisual", name);
      syncWorldData("BVisualManager::createProtoVisual Initing protovisual with index", index);
   #endif
   pProtoVisual->init(name, index);

   mProtoVisualList.setNumber(index + 1);
   mProtoVisualList[index] = pProtoVisual;

   mProtoVisualTable.add(name, index);
      
   return index;
}

//============================================================================
// BVisualManager::getProtoVisual
//============================================================================
BProtoVisual* BVisualManager::getProtoVisual(long index, bool ensureLoaded)
{
   if(index<0 || index>=mProtoVisualList.getNumber())
      return NULL;

   BProtoVisual* pProtoVisual = mProtoVisualList[index];


   if (gArchiveManager.getArchivesEnabled())
   {
      if(ensureLoaded && pProtoVisual && pProtoVisual->isLoaded() && !pProtoVisual->areAllAssetsLoaded())
      {
         pProtoVisual->loadAllAssets();
      }
   }
   else
   {
      if (ensureLoaded && pProtoVisual)
      {
         if (!pProtoVisual->isLoaded())
         {
            if(pProtoVisual->loadFailed())
               return NULL;

            if(!pProtoVisual->load())
               return NULL;

            handleProtoVisualLoaded(pProtoVisual);
         }

         if (!pProtoVisual->areAllAssetsLoaded())
         {
            pProtoVisual->loadAllAssets();
         }
      }
   }

   return pProtoVisual;
}

//============================================================================
// BVisualManager::createVisual
//============================================================================
BVisual* BVisualManager::createVisual(long protoIndex, bool synced, int64 userData, DWORD tintColor, const BMatrix& worldMatrix, int displayPriority)
{
   BProtoVisual* pProtoVisual=getProtoVisual(protoIndex, true);
   if(!pProtoVisual)
      return NULL;

   BVisual* pVisual=BVisual::getInstance();
   if(!pVisual)
      return NULL;

   if(!pVisual->init(pProtoVisual, userData, synced, tintColor, worldMatrix, displayPriority))
   {
      BVisual::releaseInstance(pVisual);
      return NULL;
   }

   return pVisual;
}

//============================================================================
// BVisualManager::createVisual
//============================================================================
BVisual* BVisualManager::createVisual(const BVisual* pSource, bool synced, int64 userData, DWORD tintColor, const BMatrix& worldMatrix)
{
   BVisual* pVisual=BVisual::getInstance();
   if(!pVisual)
      return NULL;

   if(!pVisual->clone(pSource, synced, userData, false, tintColor, worldMatrix))
   {
      BVisual::releaseInstance(pVisual);
      return NULL;
   }

   return pVisual;
}

//============================================================================
// BVisualManager::releaseVisual
//============================================================================
void BVisualManager::releaseVisual(BVisual* pVisual)
{
   BVisual::releaseInstance(pVisual);
}

//==============================================================================
// BVisualManager::getNumVisuals
//==============================================================================
long BVisualManager::getNumVisuals() const 
{ 
   return BVisual::getNumAllocatedInstances();
}

//==============================================================================
// BVisualManager::handleAnimEvent
//==============================================================================
bool BVisualManager::handleAnimEvent(long attachmentHandle, long animType, BVisual* pVisual, BProtoVisualTag* pTag)
{
   long eventType=pTag->mEventType;
   int64 userData=pVisual->getUserData();

   /*
   //-- first let the visual have a first try at handling this
   //-- anim event if not then let the visual handlers do the work
   if (pVisual->handleAnimEvent(attachmentHandle, animType, eventType, pTag))
      return true;
   */

   for(long i=mVisualHandlers.getNumber()-1; i>=0; i--)
   {
      if(mVisualHandlers[i]->handleAnimEvent(attachmentHandle, animType, eventType, userData, pTag, pVisual))
         return true;
   }
   return false;
}

//==============================================================================
// BVisualManager::handleSetAnimSync
//==============================================================================
void BVisualManager::handleSetAnimSync(BVisual* pVisual, long animationTrack, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction)
{
   int64 userData=pVisual->getUserData();
   for(long i=mVisualHandlers.getNumber()-1; i>=0; i--)
   {
      if (mVisualHandlers[i]->handleSetAnimSync(userData, animationTrack, animType, applyInstantly, timeIntoAnimation, forceAnimID, reset, startOnThisAttachment, pOverrideExitAction, pVisual))
         return;
   }
}

//==============================================================================
// BVisualManager::handleVisualLogic
//==============================================================================
long BVisualManager::handleVisualLogic(BProtoVisualLogicNode* pLogicNode, long randomTag, int64 userData, BProtoVisual* pProtoVisual)
{
   #if defined DEBUG_VM_INTRUSIVE_SYNCING && defined SYNC_Anim
      bool sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         if (pVisualItem && pVisualItem->mpName && pVisualItem->mpName->getPtr())
         {
            syncAnimData("BVisualManager::handleVisualLogic pVisualItem name", pVisualItem->mpName->getPtr());
         }
         if (pLogicNode)
         {
            syncAnimData("BVisualManager::handleVisualLogic pLogicNode->mLogicType", pLogicNode->mLogicType);
         }
         syncAnimData("BVisualManager::handleVisualLogic mVisualHandlers.getNumber()", mVisualHandlers.getNumber());
      }
   #endif

   for(long i=mVisualHandlers.getNumber()-1; i>=0; i--)
   {
      long index=mVisualHandlers[i]->handleVisualLogic(pLogicNode, randomTag, userData, pProtoVisual);
      if(index!=-1)
      {
         #if defined DEBUG_VM_INTRUSIVE_SYNCING && defined SYNC_Anim
            bool sync = gRandomManager.getSync(randomTag);
            if (sync)
            {
               syncAnimData("BVisualManager::handleVisualLogic return index", index);
            }
         #endif
         return index;
      }
   }
   return -1;
}

//==============================================================================
// BVisualManager::handleVisualPoint
//==============================================================================
long BVisualManager::handleVisualPoint(long pointType, const BSimString& data)
{
   for(long i=mVisualHandlers.getNumber()-1; i>=0; i--)
   {
      long index=mVisualHandlers[i]->handleVisualPoint(pointType, data);
      if(index!=-1)
         return index;
   }
   return -1;
}

//==============================================================================
// BVisualManager::handleProtoVisualLoaded
//==============================================================================
void BVisualManager::handleProtoVisualLoaded(BProtoVisual* pProtoVisual)
{
   for(long i=mProtoVisualHandlers.getNumber()-1; i>=0; i--)
      mProtoVisualHandlers[i]->handleProtoVisualLoaded(pProtoVisual);
}

//==============================================================================
// BVisualManager::getRandomValue
//==============================================================================
long BVisualManager::getRandomValue(long randomTag, long minVal, long maxVal)
{
   if (mVisualHandlers.getNumber() > 0)
      return mVisualHandlers[0]->getRandomValue(randomTag, minVal, maxVal);
   else
      return minVal;
}

//==============================================================================
// BVisualManager::getVisualLogicValue
//==============================================================================
bool BVisualManager::getVisualLogicValue(long logicType, const char* pName, DWORD& valDword, float& valFloat) const
{
   for(long i=mProtoVisualHandlers.getNumber()-1; i>=0; i--)
   {
      if(mProtoVisualHandlers[i]->getVisualLogicValue(logicType, pName, valDword, valFloat))
         return true;
   }
   return false;
}

//==============================================================================
// BVisualManager::getAnimType
//==============================================================================
long BVisualManager::getAnimType(const BSimString& name)
{
   bool found;
   return (getAnimType(name, found));
}

//==============================================================================
// BVisualManager::getAnimType
//==============================================================================
long BVisualManager::getAnimType(const BSimString& name, bool &found)
{
   found = false;
   if(name.isEmpty())
      return cAnimTypeIdle;
   long index=-1;
   if(mAnimTypeTable.find(name, &index))
   {
      found = true;
      return index;
   }

   // ATTENTION: When running with archives don't allow any creation after BDatabase::setup
   //
   if (gArchiveManager.getArchivesEnabled() && gArchiveManager.getIsGameInitialized() && !gSaveGame.isLoading())
   {
      /*
      BSimString messageString;
      messageString.format("BVisualManager::getAnimType - Animation type table is restricted from growing after BDatabase::setup when running with archives.  Anim type: %s", name.getPtr());
      BASSERTM(false, messageString.getPtr());
      */

      return cAnimTypeIdle;
   }

   BASSERT(mNextAnimType<1023); // the BObjectAnimationState class assumes no more than 1024 anim types
   found = true;
   index=mNextAnimType;
   mNextAnimType++;
   mAnimTypeTable.add(name, index);
   return index;
}

//==============================================================================
// BVisualManager::getAnimName
//==============================================================================
const char* BVisualManager::getAnimName(long type) const
{
   const BStringTableLong::BTagsArrayType& tags=mAnimTypeTable.getTags();
   long count=tags.getNumber();
   if(type<0 || type>=count)
      return "";
   else
      return tags[type].getPtr();
}

//==============================================================================
// BVisualManager::getAttachmentType
//==============================================================================
long BVisualManager::getAttachmentType(const BSimString& name)
{
   if(name.isEmpty())
      return -1;
   long index=-1;
   if(mAttachmentTypeTable.find(name, &index))
      return index;

   // ATTENTION: When running with archives don't allow any creation after BDatabase::setup
   //
   if (gArchiveManager.getArchivesEnabled() && gArchiveManager.getIsGameInitialized() && !gSaveGame.isLoading())
   {
      BSimString messageString;      
      messageString.format("BVisualManager::getAttachmentType - Attachment type table is restricted from growing after BDatabase::setup when running with archives.  Attachment type: %s", name.getPtr());
      BASSERTM(false, messageString.getPtr());

      return -1;
   }

   BASSERT(mNextAttachmentType<2047); // the BObjectAnimationState class assumes no more than 2048 attachment types
   index=mNextAttachmentType;
   mNextAttachmentType++;
   mAttachmentTypeTable.add(name, index);
   return index;
}

//==============================================================================
// BVisualManager::getAttachmentName
//==============================================================================
const char* BVisualManager::getAttachmentName(long type) const
{
   const BStringTableLong::BTagsArrayType& tags=mAttachmentTypeTable.getTags();
   long count=tags.getNumber();
   if(type<0 || type>=count)
      return "";
   else
      return tags[type].getPtr();
}

//==============================================================================
// BVisualManager::recomputeBoundingBoxes
//==============================================================================
void BVisualManager::recomputeProtoVisualBoundingBoxes(void)
{
   for (uint i = 0; i < mProtoVisualList.getSize(); i++)
   {
      if (mProtoVisualList[i])
      {
         mProtoVisualList[i]->recomputeBoundingBox();
      }
   }
}

//==============================================================================
// BVisualManager::receiveEvent
//==============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BVisualManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
//-- FIXING PREFIX BUG ID 7402
      const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--

      BString filepath(pPayload->mPath);
      filepath.findAndReplace("art\\", "");
      long protoIndex = findProtoVisual(filepath);
      if (protoIndex >= 0)
      {
         BProtoVisual* pProtoVis = getProtoVisual(protoIndex, false);
         if (pProtoVis)
         {
            gConsoleOutput.status("Reloading vis file: %s", pPayload->mPath.getPtr());
            pProtoVis->reload();

            pProtoVis->loadAllAssets();
         }
      }
   }

   return false;
}
#endif
//============================================================================
// BVisualManager::unloadAll
//============================================================================
bool BVisualManager::unloadAll()
{
   for (uint i = 0; i < mProtoVisualList.getSize(); i++)
   {
      // SLB: That doesn't seem right. Unload regardless.
      //if (mProtoVisualList[i]->areAllAssetsLoaded())
      {
         mProtoVisualList[i]->unloadAllAssets();
      }
   }

   return(true);
}

#ifndef BUILD_FINAL
   //============================================================================
   // BVisualManager::ensureUnloaded
   //============================================================================
   void BVisualManager::ensureUnloaded() const
   {
      for (uint i = 0; i < mProtoVisualList.getSize(); i++)
      {
         mProtoVisualList[i]->ensureUnloaded();
      }
   }

   //============================================================================
   // BVisualManager::getDamageTemplateCount
   //============================================================================
   long BVisualManager::getDamageTemplateCount() const
   {
      long count = 0;

      for (uint i = 0; i < mProtoVisualList.getSize(); i++)
      {
         count += mProtoVisualList[i]->getDamageTemplateCount();
      }

      return count;
   }
#endif

//============================================================================
//============================================================================
bool BVisualManager::saveVisual(BStream* pStream, int saveType, const BVisual* pVisual) const
{
   if (pVisual && pVisual->getProtoVisualID() != -1)
   {
      GFWRITEVAL(pStream, long, pVisual->getProtoVisualID());
      GFWRITECLASSPTR(pStream, saveType, pVisual);
   }
   else
      GFWRITEVAL(pStream, long, -1);
   return true;
}

//============================================================================
//============================================================================
bool BVisualManager::loadVisual(BStream* pStream, int saveType, BVisual** ppVisual, BMatrix& worldMatrix, DWORD tintColor)
{
   *ppVisual = NULL;

   long protoVisualID = -1;
   GFREADVAR(pStream, long, protoVisualID);
   if (protoVisualID == -1)
      return true;
   gSaveGame.remapProtoVisualID(protoVisualID);

   BProtoVisual* pProtoVisual = getProtoVisual(protoVisualID, true);

   BVisual* pVisual=BVisual::getInstance();
   if (!pVisual)
   {
      {GFERROR("GameFile Error: BVisual::getInstance failed");}
      return false;
   }

   if (!pVisual->load(pStream, saveType, pProtoVisual, worldMatrix, tintColor))
   {
      {GFERROR("GameFile Error: pVisual->load failed");}
      BVisual::releaseInstance(pVisual);
      return false;
   }

   *ppVisual = pVisual;

   return true;
}


//============================================================================
//============================================================================
void BVisualManager::queueVisualItemForRender(BVisualItem *item, BVisualRenderAttributes* renderAttributes)
{
   // Sanity.
   if(!item || !renderAttributes)
   {
      BFAIL("Trying to queue null visual info");
      return;
   }
   
//    for(long i=0; i<mVisualRenderQueue.getNumber(); i++)
//    {
//       BASSERT(item != mVisualRenderQueue[i].mItem);
//    }
   
   // Add to the list.
   BVisualRenderEntry &entry = mVisualRenderQueue.grow();
   entry.mItem = item;
   entry.mVisualRenderAttributes = *renderAttributes;
   entry.mWorldMatrix = gRender.getWorldBMatrix();
}


//============================================================================
//============================================================================
void BVisualManager::queueGrannyInstanceForSampling(BGrannyInstance *instance)
{
   // Sanity.
   if(!instance)
   {
      BFAIL("Trying to queue null granny instance");
      return;
   }
   
   // Add to the queue.
   mVisualRenderGrannyQueue.add(instance);
}


//============================================================================
//============================================================================
/*static*/ void BVisualManager::grannyPoseCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(grannyPoseCallback);

   // Cast parameter to the proper type.
   BGrannyPoseWorkEntry *workEntry = static_cast<BGrannyPoseWorkEntry*>(privateData0);

   for(uint i=0; i<workEntry->mInstances.getSize(); i++)
   {
      // Get instance.
      BGrannyInstance *instance = workEntry->mInstances[i];
      
      // Pose it.
      instance->renderPrepareSampleAnims();
   }

   if (lastWorkEntryInBucket)
      gVisualManager.mGrannyPoseRemainingBuckets.decrement();
}


//============================================================================
//============================================================================
void BVisualManager::processVisualRenderQueue()
{
   SCOPEDSAMPLE(processVisualRenderQueue);

   // Do the prepare on all of them.
   long count = mVisualRenderQueue.getNumber();
   {
      SCOPEDSAMPLE(renderQueuePrepare);

      for(long i=0; i<count; i++)
      {
         // Get entry.
         BVisualRenderEntry &entry = mVisualRenderQueue[i];
         
         // Prepare.
         entry.mItem->renderPrepare();
      }
   }
   
   // Pose them all.  This will eventually go multi-threaded.
   static bool useThreads = true;
   if(useThreads)
   {
      SCOPEDSAMPLE(ThreadedGrannyPose);

      // jce [11/4/2008] -- Mostly cut & paste/adapted from world's async update stuff.
      
      // Flush any work that is in the pipe right now
      {
         SCOPEDSAMPLE(ThreadedGrannyPose_WaitForFlush);
         gWorkDistributor.flush();
      }

      // Calculate optimal bucket and work sizes
      const uint cTotalNumTasks = mVisualRenderGrannyQueue.getSize();
      const uint cNumTasksPerWorkEntryLog2 = 2;
      const uint cNumTasksPerWorkEntry = 1U << cNumTasksPerWorkEntryLog2;
      const uint cTotalNumWorkEntries = (cTotalNumTasks >> cNumTasksPerWorkEntryLog2) + 1;

      uint cNumWorkEntriesPerBucketLog2 = 4;
      if (cTotalNumWorkEntries <= 8)
         cNumWorkEntriesPerBucketLog2 = 1;
      else if (cTotalNumWorkEntries <= 16)
         cNumWorkEntriesPerBucketLog2 = 2;
      else if (cTotalNumWorkEntries <= 32)
         cNumWorkEntriesPerBucketLog2 = 3;
         
      const uint cNumWorkEntriesPerBucket = 1U << cNumWorkEntriesPerBucketLog2;
      BDEBUG_ASSERT(cNumWorkEntriesPerBucket <= gWorkDistributor.getWorkEntryBucketSize());  

      const uint totalBuckets = (cTotalNumWorkEntries + cNumWorkEntriesPerBucket - 1) >> cNumWorkEntriesPerBucketLog2;
      mGrannyPoseRemainingBuckets.set(totalBuckets);

      
      //allocate thread local storage space for our data
      mGrannyPoseWorkEntries.resize(cTotalNumWorkEntries);
      BGrannyPoseWorkEntry *pNextWorkEntry = mGrannyPoseWorkEntries.getPtr();  //so we'll be filling our work entries.

      uint numBucketWorkEntries = 0;
      uint runningBucketCount = 0;
      uint workUnitIndex = 0;
      for(uint entryIndex = 0; entryIndex < cTotalNumWorkEntries; entryIndex ++)
      {
         //create a bucket entry
         BGrannyPoseWorkEntry *currEntry = pNextWorkEntry;
         currEntry->mInstances.setNumber(0);

         //fill the work entry with tasks
         for(uint k=0; k<cNumTasksPerWorkEntry;k++)
         {
            if(workUnitIndex >= cTotalNumTasks)
               break;

            currEntry->mInstances.push_back(mVisualRenderGrannyQueue[workUnitIndex++]); 
         }

         //queue up the work entry to the bucket
         gWorkDistributor.queue(grannyPoseCallback, currEntry, 0);

         //handle bucket management
         numBucketWorkEntries++;
         if (numBucketWorkEntries == cNumWorkEntriesPerBucket)
         {
            runningBucketCount++;
            numBucketWorkEntries = 0;            
            gWorkDistributor.flush();
         }

         //increment our frame storage pointers
         pNextWorkEntry++;
      }
      

      //flush whatever is left over
      if(numBucketWorkEntries)
      {
         runningBucketCount++;
         gWorkDistributor.flush();
         numBucketWorkEntries = 0;
      }

      BASSERT(runningBucketCount == totalBuckets);
      BDEBUG_ASSERT((pNextWorkEntry - mGrannyPoseWorkEntries.getPtr()) <= static_cast<int>(mGrannyPoseWorkEntries.getSize()));

      gWorkDistributor.waitSingle(mGrannyPoseRemainingBuckets);
   }
   else
   {
      SCOPEDSAMPLE(UnthreadedGrannyPose);
      
      long grannyCount = mVisualRenderGrannyQueue.getNumber();
      for(long i=0; i<grannyCount; i++)
      {
         mVisualRenderGrannyQueue[i]->renderPrepareSampleAnims();
      }
   }
   
   
   // Now that all the granny models have a local pose, use that to really do the render.
   {
      SCOPEDSAMPLE(renderQueueInternalRender);

      for(long i=0; i<count; i++)
      {
         // Get entry.
         BVisualRenderEntry &entry = mVisualRenderQueue[i];
         
         // Render.
         gRender.setWorldMatrix(entry.mWorldMatrix);
         entry.mItem->internalRender(&entry.mVisualRenderAttributes);
      }
   }
   
   // Clear out the queues for this frame.
   mVisualRenderQueue.setNumber(0);
   mVisualRenderGrannyQueue.setNumber(0);
}


//============================================================================
//============================================================================
void BVisualManager::beginRenderPrepare()
{
   BASSERT(mVisualRenderQueue.getNumber() == 0);
   BASSERT(mVisualRenderGrannyQueue.getNumber() == 0);
   
   gGrannyManager.beginRenderPrepare();
}


//============================================================================
//============================================================================
void BVisualManager::endRenderPrepare()
{
   gGrannyManager.endRenderPrepare();
}
