//==============================================================================
// protovisual.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "protovisual.h"
#include "grannyinstance.h"
#include "grannymanager.h"
#include "grannymodel.h"
#include "visual.h"
#include "visualmanager.h"
#include "xmlreader.h"
#include "particlegateway.h"
#include "lightEffectManager.h"
#include "consoleOutput.h"
#include "..\xinputsystem\gamepad.h"
#include "..\xsound\xsound.h"
#include "econfigenum.h"

//xgame
#include "..\xgame\terraineffectmanager.h"
#include "..\xgame\database.h"


//#define SAVE_VIS

#ifdef SAVE_VIS
   #include "xmlwriter.h"
#endif

// attachment types
enum
{
   cAttachmentParticleFile,
   cAttachmentModelFile,
   cAttachmentModelRef,
   cAttachmentLightFile,
   cAttachmentTerrainEffect
};

bool BProtoVisual::mGenerationChanged = false;

#define DEBUG_PROTO_INTRUSIVE_SYNCING

//==============================================================================
// BProtoVisual::BProtoVisual
//==============================================================================
BProtoVisual::BProtoVisual() :
   mFlags(),
   mID(-1),
   mName(),
   mGeneration(1),
   mpLogicNode(NULL), 
   mDefaultModelIndex(-1)
{  
}

//==============================================================================
// BProtoVisual::~BProtoVisual
//==============================================================================
BProtoVisual::~BProtoVisual()
{
   deinit();
}

//==============================================================================
// BProtoVisual::init
//==============================================================================
bool BProtoVisual::init(const BCHAR_T* pName, long id)
{
   mID = id;
   mName=pName;
   return true;
}

//==============================================================================
// BProtoVisual::deinit
//==============================================================================
void BProtoVisual::deinit()
{
   for(long i=mModels.getNumber()-1; i>=0; i--)
      deinitModel(mModels[i]);
   mModels.clear();

   if(mpLogicNode)
   {
      for(long i=mpLogicNode->mLogicValues.getNumber()-1; i>=0; i--)
      {
         BProtoVisualLogicValue& logicValue=mpLogicNode->mLogicValues[i];
         if(logicValue.mpModel)
            deinitModel(logicValue.mpModel);
      }
      delete mpLogicNode;
      mpLogicNode = NULL;
   }


   setFlag(cFlagAreAllAssetsLoaded, false);
}

//==============================================================================
// BProtoVisual::deinitModel
//==============================================================================
void BProtoVisual::deinitModel(BProtoVisualModel* pModel)
{
   deinitAsset(&pModel->mAsset);

   BProtoVisualLogicNode* pLogicNode=pModel->mpLogicNode;
   if(pLogicNode)
   {
      for(long i=pLogicNode->mLogicValues.getNumber()-1; i>=0; i--)
      {
         BProtoVisualLogicValue& logicValue=pLogicNode->mLogicValues[i];
         if(logicValue.mpModel)
            deinitModel(logicValue.mpModel);
      }
      delete pLogicNode;
      pLogicNode = NULL;
   }

   for(long i=pModel->mAnims.getNumber()-1; i>=0; i--)
   {
      BProtoVisualAnim *pModelAnim = &pModel->mAnims[i];
      for(long j=pModelAnim->mAssets.getNumber()-1; j>=0; j--)
      {
         deinitAsset(&pModelAnim->mAssets[j]);
      }
   }

   delete pModel;
   pModel = NULL;
}

//==============================================================================
// BProtoVisual::deinitAsset
//==============================================================================
void BProtoVisual::deinitAsset(BProtoVisualAsset* pAsset)
{
   if(pAsset->mpOpacityProgression)
   {
      delete pAsset->mpOpacityProgression;
      pAsset->mpOpacityProgression = NULL;
   }
}


//==============================================================================
// BProtoVisual::load
//==============================================================================
bool BProtoVisual::load()
{
   //SCOPEDSAMPLE(BProtoVisual_load)
   if(getFlag(cFlagLoadFailed))
      return false;

   // Load the visual's VIS file
   if(!loadVIS())
   {
      setFlag(cFlagLoadFailed, true);
      return false;
   }

   // Post load processing
   for(long i=0; i<mModels.getNumber(); i++)
      postLoadModel(mModels[i]);

   if(mpLogicNode)
   {
      for(long i=0; i<mpLogicNode->mLogicValues.getNumber(); i++)
      {
         BProtoVisualLogicValue& logicValue=mpLogicNode->mLogicValues[i];
         if(logicValue.mpModel)
            postLoadModel(logicValue.mpModel);
      }
   }

#ifdef SAVE_VIS
   saveVIS();
#endif
   
   setFlag(cFlagIsLoaded, true);

   return true;
}

//==============================================================================
// BProtoVisual::reload
//==============================================================================
bool BProtoVisual::reload()
{
   if (mName.isEmpty())
      return false;
      
   mGeneration++;      
   BProtoVisual::mGenerationChanged = true;
      
   deinit();

   return load();   
}


//==============================================================================
// BProtoVisual::postLoadModel
//==============================================================================
void BProtoVisual::postLoadModel(BProtoVisualModel* pModel)
{
   BProtoVisualLogicNode* pLogicNode=pModel->mpLogicNode;
   if(pLogicNode)
   {
      for(long i=0; i<pLogicNode->mLogicValues.getNumber(); i++)
      {
         BProtoVisualLogicValue& logicValue=pLogicNode->mLogicValues[i];
         if(logicValue.mpModel)
            postLoadModel(logicValue.mpModel);
      }
   }

   for(long i=0; i<pModel->mAttachmentLinks.getNumber(); i++)
      postLoadAttachmentLinks(pModel, &(pModel->mAttachmentLinks[i]));

   for(long i=0; i<pModel->mAnims.getNumber(); i++)
   {
      BProtoVisualAnim* pAnim=&(pModel->mAnims[i]);
      for(long j=0; j<pAnim->mAttachmentLinks.getNumber(); j++)
         postLoadAttachmentLinks(pModel, &(pAnim->mAttachmentLinks[j]));
   }

   if(!pModel->mRefModelName.isEmpty())
   {
      for(long i=0; i<mModels.getNumber(); i++)
      {
         if(mModels[i]->mModelName==pModel->mRefModelName)
         {
            pModel->mRefModelIndex=i;
            break;
         }
      }
   }
}

//==============================================================================
// BProtoVisual::postLoadAttachmentLinks
//==============================================================================
void BProtoVisual::postLoadAttachmentLinks(BProtoVisualModel* pModel, BProtoVisualAttachment* pLink)
{
   // Exit if attachment is already linked (this is the case when the attachment type
   // is ModelFile or ParticleFile
   if(pLink->mAttachmentIndex != -1)
      return;

   for(long k=0; k<mModels.getNumber(); k++)
   {
      if(mModels[k]->mModelName==pLink->mAttachmentName)
      {
         pLink->mAttachmentIndex=k;
         break;
      }
   }
}


//==============================================================================
// BProtoVisual::loadAllAssets
//==============================================================================
void BProtoVisual::loadAllAssets()
{
   SCOPEDSAMPLE(BProtoVisual_loadAllAssets)
   if(getFlag(cFlagAreAllAssetsLoaded))
      return;

   #ifdef SYNC_World
      if (getName().getPtr())
      {
         syncWorldData("BProtoVisual::loadAllAssets for", getName().getPtr());
      }
   #endif

   for (uint i = 0; i < mModels.getSize(); i++)
   {
     // SCOPEDSAMPLE(BProtoVisual_loadAllAssets_allModels)
      BProtoVisualModel* pModel = mModels[i];
      if (pModel)
      {
         // If this is a granny model, get it for use below
         ensureAssetIsLoaded(&pModel->mAsset);
         BGrannyModel* pGrannyModel = NULL;
         if (pModel->mAsset.mAssetType == cVisualAssetGrannyModel)
         {
            pGrannyModel = gGrannyManager.getModel(pModel->mAsset.mAssetIndex);
         }
         
         if(!pModel->mPointsInitialized)
            initPoints(pModel, &pModel->mAsset);

         lookupAttachmentBoneHandles(pModel->mAsset, pModel->mAttachmentLinks);

         for (uint j = 0; j < pModel->mAnims.getSize(); j++)
         {
        //    SCOPEDSAMPLE(BProtoVisual_loadAllAssets_allAnims)
            BProtoVisualAnim& anim = pModel->mAnims[j];

            for (uint k = 0; k < anim.mAssets.getSize(); k++)
            {
             //  SCOPEDSAMPLE(BProtoVisual_loadAllAssets_allAnimAssets)
               BProtoVisualAsset& animAsset = anim.mAssets[k];
               
               ensureAssetIsLoaded(&animAsset);

               // Loop through all tags
               for (uint l = 0; l < animAsset.mTags.getSize(); l++)
               {
                //  SCOPEDSAMPLE(BProtoVisual_loadAllAssets_allTags)
                  BProtoVisualTag& tag = animAsset.mTags[l];
                  
                  ensureTagIsLoaded(&tag);

                  if(!tag.mToBoneName.isEmpty() && (tag.mToBoneHandle == -1 || tag.mToBoneModelIndex != pModel->mAsset.mAssetIndex))
                  {
                     tag.mToBoneHandle=getBoneHandle(&(pModel->mAsset), tag.mToBoneName);
                     tag.mToBoneModelIndex=pModel->mAsset.mAssetIndex;
                  }
               }
            }

            // Get optional mesh indices from mesh names
            if (pGrannyModel)
            {
               for (uint k = 0; k < anim.mOptionalMeshes.getSize(); k++)
               {
                  BProtoVisualOptionalMesh& mesh = anim.mOptionalMeshes[k];
                  mesh.mMeshIndex = pGrannyModel->getMeshIndex(mesh.mMeshName);
#ifndef BUILD_FINAL
                  long meshCount = pGrannyModel->getNumMeshes();
                  if ((mesh.mMeshIndex <= -1) || (mesh.mMeshIndex >= meshCount))
                  {
                     char buf[1024 * 5];
                     sprintf_s(buf, sizeof(buf), "BProtoVisual::loadAllAssets (Invalid optional mesh index): meshIndex = %i count = %i meshName = %s protoVisual = %s", mesh.mMeshIndex, meshCount, mesh.mMeshName.getPtr(), getName().getPtr());
                     BFAIL(buf);
                  }
#endif
               }
            }

            lookupAttachmentBoneHandles(pModel->mAsset, anim.mAttachmentLinks);
         }

         if(pModel->mpLogicNode)
            loadAllLogicNodeAssetsRecurse(pModel->mpLogicNode);
      }
   }
       
   if (mpLogicNode)
   {
    //  SCOPEDSAMPLE(BProtoVisual_loadAllAssets_logicNode)
      loadAllLogicNodeAssetsRecurse(mpLogicNode);
   }

   // Compute the max bounding box
   {
      SCOPEDSAMPLE(BProtoVisual_loadAllAssets_computeBoundingBox)
      computeBoundingBox();
   }
   

   setFlag(cFlagAreAllAssetsLoaded, true);
}

//==============================================================================
//==============================================================================
void BProtoVisual::lookupAttachmentBoneHandles(BProtoVisualAsset& parentAsset, BSmallDynamicSimArray<BProtoVisualAttachment>& attachments)
{
   uint count = attachments.size();
   for (uint i=0; i<count; i++)
   {
      BProtoVisualAttachment& attachment = attachments[i];

      if(attachment.mFromBoneHandle==-1)
      {
         if (attachment.mAttachmentIndex != -1)
         {
            BProtoVisualModel* pAttachmentModel = mModels[attachment.mAttachmentIndex];
            attachment.mFromBoneHandle = getBoneHandle(&(pAttachmentModel->mAsset), attachment.mFromBoneName);
         }
      }

      if(attachment.mToBoneHandle==-1)
         attachment.mToBoneHandle = getBoneHandle(&parentAsset, attachment.mToBoneName);
   }
}

//==============================================================================
// BProtoVisual::loadAllLogicNodeAssetsRecurse
//==============================================================================
void BProtoVisual::loadAllLogicNodeAssetsRecurse(BProtoVisualLogicNode* pLogicNode)
{
   if (pLogicNode == NULL)
      return;

   //  SCOPEDSAMPLE(BProtoVisual_loadAllLogicNodeAssetsRecurse)
   for (uint i = 0; i < pLogicNode->mLogicValues.getSize(); i++)
   {
      BProtoVisualModel* pModel = pLogicNode->mLogicValues[i].mpModel;
      if(pModel)
      {
         if (-1 != pModel->mAsset.mAssetType)
            ensureAssetIsLoaded(&pModel->mAsset);

         if(!pModel->mPointsInitialized)
            initPoints(pModel, &pModel->mAsset);

         lookupAttachmentBoneHandles(pModel->mAsset, pModel->mAttachmentLinks);

         if(pModel->mpLogicNode)
            loadAllLogicNodeAssetsRecurse(pModel->mpLogicNode);
      }
   }
}

//==============================================================================
// BProtoVisual::unloadAllAssets
//==============================================================================
void BProtoVisual::unloadAllAssets()
{
   for (uint i = 0; i < mModels.getSize(); i++)
   {
      BProtoVisualModel* pModel = mModels[i];
      if (pModel)
      {
         ensureAssetIsUnloaded(&pModel->mAsset, &pModel->mAttachmentLinks);
         
         pModel->mPointsInitialized = false;

         for (uint j = 0; j < pModel->mAnims.getSize(); j++)
         {
            BProtoVisualAnim& anim = pModel->mAnims[j];
            
            for (uint k = 0; k < anim.mAssets.getSize(); k++)
            {
               BProtoVisualAsset& animAsset = anim.mAssets[k];
               
               ensureAssetIsUnloaded(&animAsset, &anim.mAttachmentLinks);

               /*
               // Loop through all tags
               for (uint l = 0; l < animAsset.mTags.getSize(); l++)
               {
                  BProtoVisualTag& tag = animAsset.mTags[l];
                  
                  ensureTagIsUnloaded(&tag);
               }
               */
            }
         }

         if(pModel->mpLogicNode)
            unloadAllLogicNodeAssetsRecurse(pModel->mpLogicNode);
      }
   }
      
   if (mpLogicNode)
   {
      unloadAllLogicNodeAssetsRecurse(mpLogicNode);
   }

   setFlag(cFlagAreAllAssetsLoaded, false);
}

//==============================================================================
// BProtoVisual::unloadAllLogicNodeAssetsRecurse
//==============================================================================
void BProtoVisual::unloadAllLogicNodeAssetsRecurse(BProtoVisualLogicNode* pLogicNode)
{
   if (pLogicNode == NULL)
      return;

   //  SCOPEDSAMPLE(BProtoVisual_loadAllLogicNodeAssetsRecurse)
   for (uint i = 0; i < pLogicNode->mLogicValues.getSize(); i++)
   {
      BProtoVisualModel* pModel = pLogicNode->mLogicValues[i].mpModel;
      if(pModel)
      {
         // SLB: That doesn't seem right. Unload regardless.
         //if (-1 != pModel->mAsset.mAssetType)
            ensureAssetIsUnloaded(&pModel->mAsset, &pModel->mAttachmentLinks);

         pModel->mPointsInitialized = false;

         if(pModel->mpLogicNode)
            unloadAllLogicNodeAssetsRecurse(pModel->mpLogicNode);
      }
   }
}

//==============================================================================
// BProtoVisual::ensureAssetIsLoadedStub
//==============================================================================
void BProtoVisual::ensureAssetIsLoadedStub(BProtoVisualAsset* pAsset, bool synced)
{
   // This needs to go away IMO.  Removing it breaks non-archive builds though.  SAT
   ensureAssetIsLoaded(pAsset, synced);
}

//==============================================================================
// BProtoVisual::ensureAssetIsLoaded
//==============================================================================
void BProtoVisual::ensureAssetIsLoaded(BProtoVisualAsset* pAsset, bool synced)
{
   //SCOPEDSAMPLE(BProtoVisual_ensureAssetIsLoaded)
   if (pAsset->mLoadFailed)
      return;

   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_World
      if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncWorldData("BProtoVisual::ensureAssetIsLoaded assetType", pAsset->mAssetType);
         syncWorldData("BProtoVisual::ensureAssetIsLoaded (-1 != pAsset->mAssetIndex)", (-1 != pAsset->mAssetIndex));
         syncWorldData("BProtoVisual::ensureAssetIsLoaded pAsset->mLoadFailed", pAsset->mLoadFailed);
      }
   #endif

   if (-1 != pAsset->mAssetIndex)
   {
      // Ensure any asset data that is unloaded at the end of a scenario is loaded. This needs to be fast.
      switch (pAsset->mAssetType)
      {
         case cVisualAssetGrannyAnim:
         {
            if (!gGrannyManager.getAnimation(pAsset->mAssetIndex, true, synced))
               pAsset->mLoadFailed = true;  
            break;
         }
         case cVisualAssetGrannyModel:
         {
            if (!gGrannyManager.getModel(pAsset->mAssetIndex, true))
               pAsset->mLoadFailed = true;
            break;
         }            
      }
   }
   else
   {
      switch (pAsset->mAssetType)
      {
         case cVisualAssetGrannyAnim:
         {
            pAsset->mAssetIndex=gGrannyManager.getOrCreateAnimation(pAsset->mAssetName.getPtr(), true, synced);
            break;
         }
         case cVisualAssetGrannyModel:
         {
            pAsset->mAssetIndex=gGrannyManager.getOrCreateModel(pAsset->mAssetName.getPtr(), true);

            if(!pAsset->mDamageAssetName.isEmpty())
            {
               #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_World
                  if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
                  {
                     syncWorldData("Checking damage template", pAsset->mDamageAssetName.getPtr());
                  }
               #endif

               pAsset->mDamageAssetIndex = gVisualManager.getDamageTemplateInterface()->getOrCreateDamageTemplate(pAsset->mDamageAssetName.getPtr(), pAsset->mAssetIndex);
            }
            break;
         }            
         case cVisualAssetParticleSystem:
         {
            gParticleGateway.getOrCreateData(pAsset->mAssetName.getPtr(), (BParticleEffectDataHandle&)pAsset->mAssetIndex);
            setFlag(cFlagHasParticleSystem, true);
            break;
         }         
         case cVisualAssetLight:
         {
            gLightEffectManager.getOrCreateData(pAsset->mAssetName.getPtr(), pAsset->mAssetIndex);
            setFlag(cFlagHasLight, true);
            break;
         }
         case cVisualAssetTerrainEffect:
         {
            pAsset->mAssetIndex = gTerrainEffectManager.getOrCreateTerrainEffect(pAsset->mAssetName.getPtr(), true);
            setFlag(cFlagHasParticleSystem, true);
            break;
         }
      }

      if (pAsset->mAssetIndex==-1)
         pAsset->mLoadFailed = true;
   }

   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_World
      if (synced && (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
      {
         syncWorldData("BProtoVisual::ensureAssetIsLoaded (-1 != pAsset->mAssetIndex)", (-1 != pAsset->mAssetIndex));
         syncWorldData("BProtoVisual::ensureAssetIsLoaded pAsset->mLoadFailed", pAsset->mLoadFailed);
      }
   #endif
}

//==============================================================================
// BProtoVisual::ensureAssetIsUnloaded
//==============================================================================
void BProtoVisual::ensureAssetIsUnloaded(BProtoVisualAsset* pAsset, BSmallDynamicSimArray<BProtoVisualAttachment>* pAttachments)
{
   setFlag(cFlagAreAllAssetsLoaded, false);

   // SLB: That doesn't seem right. Unload regardless.
#if 1
   pAsset->mAssetIndex = -1;
   pAsset->mDamageAssetIndex = -1;
   pAsset->mLoadFailed = false;
#else
   if (pAsset->mLoadFailed)
      return;

   if (-1 != pAsset->mAssetIndex)
   {
      pAsset->mAssetIndex = -1;
   }

   if (-1 != pAsset->mDamageAssetIndex)
   {
      pAsset->mDamageAssetIndex = -1;
   }
#endif

   // Clear out bone handles since the model is going away and the bone hnadle could change
   uint tagCount = pAsset->mTags.size();
   for (uint i=0; i<tagCount; i++)
      pAsset->mTags[i].mToBoneHandle = -1;

   uint attachmentCount = pAttachments->size();
   for (uint i=0; i<attachmentCount; i++)
   {
      BProtoVisualAttachment& attachment = pAttachments->get(i);
      attachment.mToBoneHandle = -1;
      attachment.mToBoneFailed = false;
      attachment.mFromBoneHandle = -1;
      attachment.mFromBoneFailed = false;
   }
}

//==============================================================================
// BProtoVisual::ensureTagIsLoaded
//==============================================================================
void BProtoVisual::ensureTagIsLoaded(BProtoVisualTag* pTag)
{
   switch(pTag->mEventType)
   {
      case cAnimEventAttack:
         break;
      case cAnimEventSound:
         break;
      case cAnimEventParticles:
         gParticleGateway.getOrCreateData(pTag->mName, (BParticleEffectDataHandle)pTag->mData0);
         break;
      case cAnimEventTerrainEffect:
         pTag->mData0 = gTerrainEffectManager.getOrCreateTerrainEffect(pTag->mName.getPtr(), true);
         break;
      case cAnimEventCameraShake:
         break;
      case cAnimEventLoop:
         break;
      case cAnimEventLight:
         gLightEffectManager.getOrCreateData(pTag->mName, (long&)pTag->mData0);
         break;
      case cAnimEventGroundIK:
         break;
      case cAnimEventAttachTarget:
         break;
      case cAnimEventSweetSpot:
         break;
      case cAnimEventAlphaTerrain:
         break;
      case cAnimEventUVOffset:
         break;
      case cAnimEventRumble:
         break;
      case cAnimEventKillAndThrow:
         break;
      case cAnimEventPhysicsImpulse:
         break;
   }
}


//==============================================================================
// BProtoVisual::getBoneHandle
//==============================================================================
long BProtoVisual::getBoneHandle(BProtoVisualAsset* pAsset, const char* pBoneName)
{
   if(pAsset->mAssetIndex==-1)
      return -1;

   switch(pAsset->mAssetType)
   {
      case cVisualAssetGrannyModel:
      {
         BGrannyModel* pGrannyModel=gGrannyManager.getModel(pAsset->mAssetIndex);
         if(pGrannyModel)
            return pGrannyModel->getBoneHandle(pBoneName);
         break;
      }
   }

   return -1;
}

//==============================================================================
// BProtoVisual::calcState
//==============================================================================
void BProtoVisual::calcState(long animationTrack, long animType, long randomTag, int64 userData, BVisualItem* pState, BProtoVisualAnim** ppAnimOut, BVisualItem* pVisualItem, long forceAnimID)
{
   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      bool sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         syncAnimData("BProtoVisual::calcState animationTrack", animationTrack);
         syncAnimData("BProtoVisual::calcState anim", gVisualManager.getAnimName(animType));
         syncAnimData("BProtoVisual::calcState userData", (DWORD)userData);
         syncAnimData("BProtoVisual::calcState mModels.getNumber()", mModels.getNumber());
         syncAnimData("BProtoVisual::calcState mpLogicNode != NULL", (mpLogicNode != NULL));
         if (pVisualItem && pVisualItem->mpName && pVisualItem->mpName->getPtr())
         {
            syncAnimData("BProtoVisual::calcState pVisualItem name", pVisualItem->mpName->getPtr());
         }
      }
   #endif

   if(mModels.getNumber()==0)
      return;

   BProtoVisualModel* pBaseModel = NULL;

   // Calculate the base model
   if(mpLogicNode)
   {
      long valueIndex=gVisualManager.handleVisualLogic(mpLogicNode, randomTag, userData, this);
      #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
         bool sync = gRandomManager.getSync(randomTag);
         if (sync)
         {
            syncAnimData("BProtoVisual::calcState valueIndex", valueIndex);
         }
      #endif
      if(valueIndex!=-1)
      {
         BProtoVisualLogicValue* pLogicValue=&(mpLogicNode->mLogicValues[valueIndex]);
         if(pLogicValue->mpModel)
            calcBaseModel(pLogicValue->mpModel, animType, randomTag, userData, &pBaseModel);
      }
   }
   else
   {
      if(mDefaultModelIndex != -1)
         //calcBaseModel(mModels[mDefaultModelIndex], animType, randomTag, userData, &pBaseModel);
         pBaseModel=mModels[mDefaultModelIndex];
   }

   if(!pBaseModel)
   {
      #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
         bool sync = gRandomManager.getSync(randomTag);
         if (sync)
         {
            syncAnimCode("BProtoVisual::calcState no base model");
         }
      #endif
      return;
   }

   long animAssetIndex = -1;

   // The forceAnimID can mean two different things.  It can be an animation ID to override the
   // current anim and play and animation that is not on the visual file (cinematics do this).  Or
   // it can also be and animation index which specifies which animation to play for an animation
   // state (the model viewer uses this).  When the forceAnimID is used as an animation index then
   // top 2 bytes are use, else the lower 2.  When forceAnimID is -1 then it isn't used.  (SAT)
   if((forceAnimID != -1) && (forceAnimID & 0xffff0000))
   {
      animAssetIndex = ~(forceAnimID >> 16);
      forceAnimID = -1;
   }

   // Calculate the state for the base model
   calcModelState(animationTrack, pBaseModel, NULL, NULL, animType, randomTag, userData, animAssetIndex, pState, ppAnimOut, pVisualItem, forceAnimID);
}

//==============================================================================
// BProtoVisual::calcModelState
//==============================================================================
bool BProtoVisual::calcModelState(
   long animationTrack, 
   BProtoVisualModel* pModel, 
   BProtoVisualAsset* pParentModelAsset, 
   BProtoVisualAttachment* pAttachment, 
   long animType, 
   long randomTag, 
   int64 userData, 
   long animAssetIndex, 
   BVisualItem* pModelState, 
   BProtoVisualAnim** ppAnimOut, 
   BVisualItem* pVisualItem, 
   long forceAnimID)
{
   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      bool sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         syncAnimData("BProtoVisual::calcModelState animationTrack", animationTrack);
         syncAnimData("BProtoVisual::calcModelState animType", gVisualManager.getAnimName(animType));
         if (pModel)
         {
            syncAnimData("BProtoVisual::calcModelState pModel->mpLogicNode != NULL", (pModel->mpLogicNode != NULL));
         }
      }
   #endif

   BVisualAnimationData &animation = pModelState->mAnimationTrack[animationTrack];

   // Figure out the visual model to use for the model asset
   BProtoVisualModel* pUseModel=pModel;
   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         if (pModel && pModel->mpLogicNode)
         {
            syncAnimCode("BProtoVisual::calcModelState has logic node");
            syncAnimData("BProtoVisual::calcModelState pModel->mpLogicNode->mLogicType", pModel->mpLogicNode->mLogicType);
         }
         if (pVisualItem)
         {
            syncAnimData("BProtoVisual::calcModelState pVisualItem->mModelVariationIndex", pVisualItem->mModelVariationIndex);
         }
      }
   #endif


   if(pModel->mpLogicNode)
   {
      long valueIndex;
      if(pModel->mpLogicNode->mLogicType == cVisualLogicVariation)
      {
         if(pVisualItem->mModelVariationIndex != -1)
            valueIndex = pVisualItem->mModelVariationIndex;
         else
            valueIndex = pVisualItem->mModelVariationIndex = gVisualManager.handleVisualLogic(pModel->mpLogicNode, randomTag, userData, this);
      }
      else
      {
         valueIndex = gVisualManager.handleVisualLogic(pModel->mpLogicNode, randomTag, userData, this);
      }

      if(valueIndex!=-1)
      {
         BProtoVisualLogicValue* pLogicValue=&(pModel->mpLogicNode->mLogicValues[valueIndex]);
         if(pLogicValue->mpModel)
            calcBaseModel(pLogicValue->mpModel, animType, randomTag, userData, &pUseModel);
      }
   }

   // Set the model asset
   BProtoVisualAsset* pModelAsset=&(pUseModel->mAsset);

   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         syncAnimData("BProtoVisual::calcModelState pModelAsset->mAssetType", pModelAsset->mAssetType);
      }
   #endif

   if(pModelAsset->mAssetType==-1)
      return false;
   ensureAssetIsLoadedStub(pModelAsset, gRandomManager.getSync(randomTag));

   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         syncAnimData("BProtoVisual::calcModelState pModelAsset->mLoadFailed", pModelAsset->mLoadFailed);
         syncAnimData("BProtoVisual::calcModelState pModelAsset->mAssetType", pModelAsset->mAssetType);
      }
   #endif

   pModelState->mModelAsset.mType=pModelAsset->mAssetType;
   pModelState->mModelAsset.mIndex=pModelAsset->mAssetIndex;
   pModelState->mModelUVOffsets=pModelAsset->mModelUVOffsets;

//    #ifdef SYNC_Anim
//       if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) && !(gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles))
//       {
//          syncAnimData("BProtoVisual::calcModelState mDamageAssetIndex", pModelAsset->mDamageAssetIndex);
//       }
//    #endif

   pModelState->mDamageTemplateIndex = pModelAsset->mDamageAssetIndex;

   // Set the anim asset
   BProtoVisualAnim* pCurrentAnim=NULL;
   BProtoVisualAsset* pAnimAsset=calcAnimAsset(pModel, pModelAsset, animType, randomTag, animAssetIndex, &(animation.mAnimType), &pCurrentAnim, &animAssetIndex);
   if(pAnimAsset)
   {
      #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
         sync = gRandomManager.getSync(randomTag);
         if (sync)
         {
            syncAnimCode("BProtoVisual::calcModelState pAnimAsset");
            syncAnimData("BProtoVisual::calcModelState pAnimAsset->mAssetName", pAnimAsset->mAssetName);
            syncAnimData("BProtoVisual::calcModelState pAnimAsset->mMinCorner", pAnimAsset->mMinCorner);
            syncAnimData("BProtoVisual::calcModelState pAnimAsset->mMaxCorner", pAnimAsset->mMaxCorner);
            syncAnimData("BProtoVisual::calcModelState pAnimAsset->mAssetType", pAnimAsset->mAssetType);
            syncAnimData("BProtoVisual::calcModelState forceAnimID == -1", (forceAnimID == -1));
         }
      #endif

      animation.mAnimAsset.mType=pAnimAsset->mAssetType;
      if(forceAnimID == -1)
      {
         animation.mAnimAsset.mIndex=pAnimAsset->mAssetIndex;
         animation.mpTags = (pAnimAsset->mTags.getNumber() > 0) ? &(pAnimAsset->mTags) : NULL;
         animation.mpOpacityProgression =pAnimAsset->mpOpacityProgression;
      }
      else
      {
         animation.mAnimAsset.mIndex=forceAnimID;
         animation.mpTags=NULL;
         animation.mpOpacityProgression=NULL;
      }
   }

   // Get the bounding box
   getBoundingBox(pModelAsset, pAnimAsset, &(pModelState->mMinCorner), &(pModelState->mMaxCorner));

   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         if (pModelState)
         {
            syncAnimData("BProtoVisual::calcModelState pModelState->mMinCorner", pModelState->mMinCorner);
            syncAnimData("BProtoVisual::calcModelState pModelState->mMaxCorner", pModelState->mMaxCorner);
         }
      }
   #endif

   // Initialize point data
   if(!pModel->mPointsInitialized)
      initPoints(pModel, pModelAsset);
   animation.mpPoints=(pModel->mPoints.getNumber() > 0 ? &(pModel->mPoints) : NULL);

   // Attachment bone handles
   if(pAttachment)
   {
      if(pAttachment->mFromBoneHandle==-1 && !pAttachment->mFromBoneFailed)
         pAttachment->mFromBoneHandle=getBoneHandle(pModelAsset, pAttachment->mFromBoneName);

      if(pAttachment->mToBoneHandle==-1 && !pAttachment->mToBoneFailed)
         pAttachment->mToBoneHandle=getBoneHandle(pParentModelAsset, pAttachment->mToBoneName);

      pModelState->mFromBoneHandle=pAttachment->mFromBoneHandle;
      pModelState->mToBoneHandle=pAttachment->mToBoneHandle;
      pModelState->mpName=&(pAttachment->mAttachmentName);
      pModelState->mIndex=pAttachment->mAttachmentIndex;
      pModelState->mAttachmentHandle=pAttachment->mAttachmentHandle;
      pModelState->mAttachmentType=pAttachment->mAttachmentType;

      pModelState->setFlag(BVisualItem::cFlagSyncAnims, pAttachment->mSyncAnims);
      pModelState->setFlag(BVisualItem::cFlagDisregardOrient, pAttachment->mDisregardOrient);
   }

   // Set the model and anim attachments
   bool noParticles = gConfig.isDefined(cConfigNoParticles);
   for(long a=0; a<2; a++)
   {
      if(a==1 && !pCurrentAnim)
         break;
      long count=(a==0?pModel->mAttachmentLinks.getNumber():pCurrentAnim->mAttachmentLinks.getNumber());
      for(long i=0; i<count; i++)
      {
         BProtoVisualAttachment* pAttachment=(a==0?&(pModel->mAttachmentLinks[i]):&(pCurrentAnim->mAttachmentLinks[i]));
         if(pAttachment->mAttachmentIndex==-1)
            continue;

         if(noParticles && pAttachment->mAttachmentType == cAttachmentParticleFile)
            continue;

         BVisualItem* pAttachmentState=BVisualItem::getInstance();
         if(!pAttachmentState)
            continue;

         BProtoVisualModel* pAttachmentModel=mModels[pAttachment->mAttachmentIndex];

         if(!calcModelState(animationTrack, pAttachmentModel, pModelAsset, pAttachment, animType, randomTag, userData, (pAttachment->mSyncAnims ? animAssetIndex : -1), pAttachmentState, NULL, pVisualItem, forceAnimID))
         {
            BVisualItem::releaseInstance(pAttachmentState);
            continue;
         }

         pModelState->mAttachments.add(pAttachmentState);
      }
   }

   if (ppAnimOut)
      *ppAnimOut = pCurrentAnim;

   return true;
}

//==============================================================================
// BProtoVisual::calcBaseModel
//==============================================================================
void BProtoVisual::calcBaseModel(BProtoVisualModel* pModel, long animType, long randomTag, int64 userData, BProtoVisualModel** ppCurrentModel)
{
   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      bool sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         syncAnimData("BProtoVisual::calcBaseModel anim", gVisualManager.getAnimName(animType));
         if (pModel && pModel->mModelName.getPtr())
         {
            syncAnimData("BProtoVisual::calcState pModel name", pModel->mModelName.getPtr());
         }
      }
   #endif

   if(pModel->mAsset.mAssetType!=-1)
   {
      *ppCurrentModel=pModel;
      #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
         bool sync = gRandomManager.getSync(randomTag);
         if (sync)
         {
            syncAnimCode("BProtoVisual::calcBaseModel *ppCurrentModel=pModel");
         }
      #endif
   }

   if(pModel->mRefModelIndex!=-1)
   {
      //calcBaseModel(mModels[pModel->mRefModelIndex], animType, randomTag, userData, ppCurrentModel);
      *ppCurrentModel=mModels[pModel->mRefModelIndex];
      #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
         bool sync = gRandomManager.getSync(randomTag);
         if (sync)
         {
            syncAnimCode("BProtoVisual::calcBaseModel *ppCurrentModel=mModels[pModel->mRefModelIndex]");
         }
      #endif
   }

   BProtoVisualLogicNode* pLogicNode=pModel->mpLogicNode;
   if(pLogicNode)
   {
      long valueIndex=gVisualManager.handleVisualLogic(pLogicNode, randomTag, userData, this);
      {
         #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
            bool sync = gRandomManager.getSync(randomTag);
            if (sync)
            {
               syncAnimData("BProtoVisual::calcBaseModel valueIndex", valueIndex);
            }
         #endif
      }
      if(valueIndex!=-1)
      {
         BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[valueIndex]);
         if(pLogicValue->mpModel)
            calcBaseModel(pLogicValue->mpModel, animType, randomTag, userData, ppCurrentModel);
      }
   }
}

//==============================================================================
// BProtoVisual::calcAnimAsset
//==============================================================================
BProtoVisualAsset* BProtoVisual::calcAnimAsset(BProtoVisualModel* pModel, BProtoVisualAsset* pModelAsset, long animType, long randomTag, long animAssetIndex, long* pAnimTypeOut, BProtoVisualAnim** pAnimOut, long* pAnimAssetIndexOut)
{
   long checkSubstitute=0;

   #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
      bool sync = gRandomManager.getSync(randomTag);
      if (sync)
      {
         syncAnimData("BProtoVisual::calcAnimAsset pModel->mAnims.getNumber()", pModel->mAnims.getNumber());
      }
   #endif

   for(;;)
   {
      for(long j=0; j<pModel->mAnims.getNumber(); j++)
      {
         BProtoVisualAnim* pAnim=&(pModel->mAnims[j]);
         if(pAnim->mAnimType==animType)
         {
            if(pAnimTypeOut)
               *pAnimTypeOut=animType;
            if(pAnimOut)
               *pAnimOut=pAnim;
            long pickedAnimAssetIndex;
            if(animAssetIndex>=0 && animAssetIndex<pAnim->mAssets.getNumber())
               pickedAnimAssetIndex=animAssetIndex;
            else
               pickedAnimAssetIndex=pickRandomAnimAsset(pAnim, randomTag);
            if(*pAnimAssetIndexOut)
               *pAnimAssetIndexOut=pickedAnimAssetIndex;
            BProtoVisualAsset* pAnimAsset = (pickedAnimAssetIndex == -1 ? NULL : &(pAnim->mAssets[pickedAnimAssetIndex]));
            #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
               bool sync = gRandomManager.getSync(randomTag);
               if (sync)
               {
                  syncAnimData("BProtoVisual::calcAnimAsset pAnimAsset->mAssetName", pAnimAsset->mAssetName.getPtr());
               }
            #endif
            if(pAnimAsset && pAnimAsset->mAssetType!=-1)
            {
               ensureAssetIsLoadedStub(pAnimAsset, gRandomManager.getSync(randomTag));

               #if defined DEBUG_PROTO_INTRUSIVE_SYNCING && defined SYNC_Anim
                  bool sync = gRandomManager.getSync(randomTag);
                  if (sync)
                  {
                     syncAnimData("BProtoVisual::calcAnimAsset pAnimAsset->mLoadFailed", pAnimAsset->mLoadFailed);
                  }
               #endif

               // Find bone handle for all anim tags that have bone names
               long count=pAnimAsset->mTags.getNumber();
               for(long i=0; i<count; i++)
               {
                  BProtoVisualTag* pTag = &(pAnimAsset->mTags[i]);
                  if(!pTag->mToBoneName.isEmpty() && (pTag->mToBoneHandle == -1 || pTag->mToBoneModelIndex != pModelAsset->mAssetIndex))
                  {
                     pTag->mToBoneHandle=getBoneHandle(pModelAsset, pTag->mToBoneName);
                     pTag->mToBoneModelIndex=pModelAsset->mAssetIndex;
#ifndef BUILD_FINAL
                     // Validate that model variations have same bone structure
                     if(pTag->mToBoneHandle!=-1 && pModel->mpLogicNode && pModel->mpLogicNode->mLogicType==cVisualLogicVariation)
                     {
                        for(long j=0; j<pModel->mpLogicNode->mLogicValues.getNumber(); j++)
                        {
                           BProtoVisualLogicValue& logicValue=pModel->mpLogicNode->mLogicValues[j];
                           BProtoVisualAsset* pModelAsset2=&(logicValue.mpModel->mAsset);
                           
                           if(pModelAsset2->mAssetType!=-1)
                              ensureAssetIsLoadedStub(pModelAsset2);
                              
                           long boneHandle2=getBoneHandle(pModelAsset2, pTag->mToBoneName);
                           if(boneHandle2!=pTag->mToBoneHandle)
                              gConsoleOutput.output(cMsgError, "Bone mismatch for visual '%s' and bone '%s'", this->mName.getPtr(), pTag->mToBoneName.getPtr());
                        }
                     }
#endif
                  }
               }
            }
            return pAnimAsset;
         }
      }

      // Substitute the Run animation if the requested Sprint wasn't found
      if(animType==cAnimTypeSprint)
      {
         animType=cAnimTypeRun;
         continue;
      }

      // Substitute the Idle animation if the requested Recover wasn't found
      if(animType==cAnimTypeRecover)
      {
         animType=cAnimTypeIdle;
         continue;
      }

      // Substitute the Walk animation if the requested Run or Jog wasn't found
      if(animType==cAnimTypeRun || animType==cAnimTypeJog)
      {
         checkSubstitute++;
         if(checkSubstitute==1)
         {
            animType=cAnimTypeWalk;
            continue;
         }
      }

      // Substitute the Idle animation if the requested Research or Train wasn't found
      if(animType==cAnimTypeResearch || animType==cAnimTypeTrain)
      {
         checkSubstitute++;
         if(checkSubstitute==1)
         {
            animType=cAnimTypeIdle;
            continue;
         }
      }

      return NULL;
   }
}

//==============================================================================
// BProtoVisual::calcAttachmentAnimState
//==============================================================================
void BProtoVisual::calcAttachmentAnimState(long animationTrack, long attachmentModelIndex, long animType, long randomTag, BVisualItem* pAttachmentState)
{
   BProtoVisualModel* pAttachmentModel=mModels[attachmentModelIndex];
   if(!pAttachmentModel)
      return;

   BProtoVisualAsset* pAttachmentModelAsset=&(pAttachmentModel->mAsset);

   BProtoVisualAsset* pAttachmentAnimAsset=calcAnimAsset(pAttachmentModel, pAttachmentModelAsset, animType, randomTag, -1, NULL, NULL, NULL);

   getBoundingBox(pAttachmentModelAsset, pAttachmentAnimAsset, &(pAttachmentState->mMinCorner), &(pAttachmentState->mMaxCorner));

   if(pAttachmentAnimAsset)
   {
      BVisualAnimationData &animation = pAttachmentState->mAnimationTrack[animationTrack];

      animation.mAnimAsset.mType=pAttachmentAnimAsset->mAssetType;
      animation.mAnimAsset.mIndex=pAttachmentAnimAsset->mAssetIndex;
      animation.mpTags = (pAttachmentAnimAsset->mTags.getNumber() > 0) ? &(pAttachmentAnimAsset->mTags) : NULL;
   }
}

//==============================================================================
// BProtoVisual::initPoints
//==============================================================================
void BProtoVisual::initPoints(BProtoVisualModel* pModel, BProtoVisualAsset* pAsset)
{
   for(long i=0; i<pModel->mPoints.getNumber(); i++)
   {
      BProtoVisualPoint* pPoint=&(pModel->mPoints[i]);
      pPoint->mBoneHandle=getBoneHandle(pAsset, pPoint->mBoneName);
      pPoint->mPointDataValue=gVisualManager.handleVisualPoint(pPoint->mPointType, pPoint->mPointDataName);
   }
   pModel->mPointsInitialized=true;
}

//==============================================================================
// BProtoVisual::pickRandomAnimAsset
//==============================================================================
long BProtoVisual::pickRandomAnimAsset(BProtoVisualAnim* pAnim, long randomTag) const
{
   long count=pAnim->mAssets.getNumber();
   if(count==0)
      return -1;
   
   if(count>1)
   {
      // Add up all the weights
      long weightSum=0;
      for(long i=0; i<count; i++)
         weightSum+=pAnim->mAssets[i].mWeight;

      if(weightSum>0)
      {
         // Generate a random version using the weights
         long randWeight=gVisualManager.getRandomValue(randomTag, 0, weightSum);

         // Go through the anims again, adding the individual weights until
         // the temp version index has been exceeded to get the version index.
         weightSum=0;
         for(long i=0; i<count; i++)
         {
            weightSum+=pAnim->mAssets[i].mWeight;
            if(weightSum>randWeight)
               return i;
         }
      }
   }

   return 0;
}

//==============================================================================
// BProtoVisual::getBoundingBox
//==============================================================================
void BProtoVisual::getBoundingBox(BProtoVisualAsset* pModelAsset, BProtoVisualAsset* pAnimAsset, BVector* pMinCorner, BVector* pMaxCorner)
{
   if(pAnimAsset)
   {
      *pMinCorner=pAnimAsset->mMinCorner;
      *pMaxCorner=pAnimAsset->mMaxCorner;
   }
   else
   {
      *pMinCorner=pModelAsset->mMinCorner;
      *pMaxCorner=pModelAsset->mMaxCorner;
   }
}

//==============================================================================
// BProtoVisual::recomputeBoundingBox
//==============================================================================
void BProtoVisual::recomputeBoundingBox()
{
   computeBoundingBox();
   mGeneration++;
   BProtoVisual::mGenerationChanged = true;
}

//==============================================================================
// BProtoVisual::computeBoundingBox
//==============================================================================
void BProtoVisual::computeBoundingBox()
{
   long modelCount=mModels.getNumber();

   for(long i=0; i<modelCount; i++)
   {
      BProtoVisualModel* pModel=mModels[i];
      computeBoundingBox(pModel);
   }
}

//==============================================================================
// BProtoVisual::computeBoundingBox
//==============================================================================
void BProtoVisual::computeBoundingBox(BProtoVisualModel* pModel)
{
   // First handle logic nodes
   BProtoVisualLogicNode* pLogicNode=pModel->mpLogicNode;
   if(pLogicNode)
   {
      for(long i=0; i<pLogicNode->mLogicValues.getNumber(); i++)
      {
         BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
         if(pLogicValue->mpModel)
            computeBoundingBox(pLogicValue->mpModel);
      }
   }

   // Finally handle the main model and animation attachments
   BProtoVisualAsset* pModelAsset=&(pModel->mAsset);
   if(pModelAsset->mAssetType==cVisualAssetGrannyModel)
   {
      ensureAssetIsLoadedStub(pModelAsset);
//-- FIXING PREFIX BUG ID 7406
      const BGrannyModel* pGrannyModel=gGrannyManager.getModel(pModelAsset->mAssetIndex);
//--
      if(!pGrannyModel)
         return;
      BGrannyInstance* pGrannyInstance=gGrannyManager.createInstance();
      if(!pGrannyInstance)
         return;
      if(pGrannyInstance->init(pModelAsset->mAssetIndex, &pModelAsset->mModelUVOffsets))
      {
         pGrannyInstance->computeBoundingBox(&(pModelAsset->mMinCorner), &(pModelAsset->mMaxCorner), true);
         long animCount=pModel->mAnims.getNumber();
         for(long j=0; j<animCount; j++)
         {
            BProtoVisualAnim* pAnim=&(pModel->mAnims[j]);

            // Animation assets
            long animAssetCount=pAnim->mAssets.getNumber();
            for(long k=0; k<animAssetCount; k++)
            {
               BProtoVisualAsset* pAnimAsset=&(pAnim->mAssets[k]);
               pAnimAsset->mMinCorner=pModelAsset->mMinCorner;
               pAnimAsset->mMaxCorner=pModelAsset->mMaxCorner;
               if(pAnimAsset->mAssetType!=cVisualAssetGrannyAnim)
                  continue;
               ensureAssetIsLoadedStub(pAnimAsset);
               BGrannyAnimation* pGrannyAnim=gGrannyManager.getAnimation(pAnimAsset->mAssetIndex);
               if(!pGrannyAnim)
                  continue;

               if(pGrannyInstance->playAnimation(cActionAnimationTrack, pAnimAsset->mAssetIndex, 1.0f, 0.0f, 0, 0.0f, 0.0f) && 
                  pGrannyInstance->playAnimation(cMovementAnimationTrack, pAnimAsset->mAssetIndex, 1.0f, 0.0f, 0, 0.0f, 0.0f))
               {
                  float length=pGrannyAnim->getDuration();

                  // Precalculate total motion extraction
                  if (pGrannyInstance->hasMotionExtraction())
                  {
                     BMatrix extractedMotion;
                     extractedMotion.makeIdentity();
                     pGrannyInstance->update(length);
                     pGrannyInstance->getExtractedMotion(length, extractedMotion);
                     pGrannyAnim->setTotalMotionExtraction(extractedMotion.getRow(3));
                     // SLB: This was to work around a bug that doesn't exist anymore
                     //if (XMMatrixIsIdentity(extractedMotion))
                     //{
                     //   pGrannyAnim->setMotionExtractionMode(BGrannyAnimation::cNoExtraction);
                     //   gConsoleOutput.warning("BProtoVisual::computeBoundingBox:: zero motion extraction in %s", pGrannyAnim->getFilename().getPtr());
                     //}
                  }

                  float step=0.25f;
                  for(float time=0.0f; ; time+=step)
                  {
                     if(time>length)
                        time=length;
                     pGrannyInstance->update(time);
                     pGrannyInstance->computeBoundingBox(&(pAnimAsset->mMinCorner), &(pAnimAsset->mMaxCorner), false);
                     if(time>=length)
                        break;
                  }
               }

               // Animation Tags
               long numTags = pAnimAsset->mTags.getNumber();
               for (long tagIndex = 0; tagIndex < numTags; tagIndex++)
               {
                  BProtoVisualTag& tag = pAnimAsset->mTags[tagIndex];
                  // Compute velocity of Kill&Throw tags
                  if (tag.mEventType == cAnimEventKillAndThrow)
                  {
                     if(pGrannyInstance->playAnimation(cActionAnimationTrack, pAnimAsset->mAssetIndex, 1.0f, 0.0f, 0, 0.0f, 0.0f) && 
                        pGrannyInstance->playAnimation(cMovementAnimationTrack, pAnimAsset->mAssetIndex, 1.0f, 0.0f, 0, 0.0f, 0.0f))
                     {
                        // Calculate time of tag in seconds, and lookahead delta
                        float duration = pGrannyAnim->getDuration();
                        float time1 = tag.mPosition * duration;
                        float time2 = Math::Min((time1 + tag.mLifespan), duration);
                        float delta = time2 - time1;

                        // Prevent division by 0
                        if (delta <= cFloatCompareEpsilon)
                           continue;

                        long toBoneHandle = getBoneHandle(pModelAsset, tag.mToBoneName);
                        BVector position1(cOriginVector);
                        BVector position2(cOriginVector);
                        BVector velocity(cOriginVector);
                        BMatrix matrix1;
                        BMatrix matrix2;
                        matrix1.makeIdentity();

                        // Get position of bone at tag time
                        pGrannyInstance->update(time1);
                        if (pGrannyInstance->hasMotionExtraction())
                           pGrannyInstance->getExtractedMotion(time1, matrix1);
                        bool foundBone = pGrannyInstance->getBone(toBoneHandle, &position1, NULL, NULL, &matrix1, false);
                        if (!foundBone)
                           matrix1.getTranslation(position1);

                        // Get position of bone at lookahead time
                        matrix2 = matrix1;
                        pGrannyInstance->update(delta);
                        if (pGrannyInstance->hasMotionExtraction())
                           pGrannyInstance->getExtractedMotion(delta, matrix2);
                        foundBone = pGrannyInstance->getBone(toBoneHandle, &position2, NULL, NULL, &matrix2, false);
                        if (!foundBone)
                           matrix2.getTranslation(position2);

                        // Compute velocity relative to model space at beginning of animation
                        velocity.assignDifference(position2, position1);
                        velocity /= delta;

                        // Transform velocity to be in model space at tag time
                        matrix1.invert();
                        matrix1.transformVector(velocity, velocity);

                        tag.mValue0 = velocity.x;
                        tag.mValue1 = velocity.y;
                        tag.mValue2 = velocity.z;
                     }
                  }
               }
            }
         }
      }
      gGrannyManager.releaseInstance(pGrannyInstance);
   }
}

//==============================================================================
// BProtoVisualTag::computeAttackInfo
// SLB: FIXME
// This should be moved to a build tool when we get one, there is no reason
// to do these calculations in-game since all the data is static!
//==============================================================================
void BProtoVisual::computeReloadInfo(long animType, float& reloadTime)
{
   //-- Find the default model
   long modelCount = mModels.getNumber();
   bool foundAnimType = false;
   for (long defaultModel = 0; (defaultModel < modelCount) && !foundAnimType; defaultModel++)
   {
      BProtoVisualModel* pDefaultModel = mModels[defaultModel];
      //-- Find the anim specified
      long animCount = pDefaultModel->mAnims.getNumber();
      for (long anim = 0; anim < animCount; anim++)
      {
         BProtoVisualAnim* pDefaultAnim = &(pDefaultModel->mAnims[anim]);
         //-- Make sure this is the specified anim
         if (pDefaultAnim->mAnimType == animType)
         {
            foundAnimType = true;

            BSmallDynamicSimArray<float> lengths;
            BSmallDynamicSimArray<float> weights;
            float weightTotal = 0.0f;

            //-- Animation assets
            long animAssetCount = pDefaultAnim->mAssets.getNumber();
            for (long animAsset = 0; animAsset < animAssetCount; animAsset++)
            {
               BProtoVisualAsset* pAnimAsset = &(pDefaultAnim->mAssets[animAsset]);
               if (pAnimAsset->mAssetType != cVisualAssetGrannyAnim)
                  continue;

               //-- Get the duration of the anim
               ensureAssetIsLoadedStub(pAnimAsset);
//-- FIXING PREFIX BUG ID 7408
               const BGrannyAnimation* pGrannyAnim = gGrannyManager.getAnimation(pAnimAsset->mAssetIndex);
//--
               if (!pGrannyAnim)
                  continue;

               float length = pGrannyAnim->getDuration();                             

               weights.add((float)pAnimAsset->mWeight);
               lengths.add(length);

               weightTotal += pAnimAsset->mWeight;
            }

            //-- Normalize the weights
            long numAnims = weights.getNumber();
            for (long anim = 0; anim < numAnims; anim++)
               weights[anim] = weights[anim] / weightTotal;

            //-- Calc weighted lengths
            for (long anim = 0; anim < numAnims; anim++)
               reloadTime += lengths[anim] * weights[anim];

            return;
         }
      }
   }
}

//==============================================================================
// BProtoVisualTag::computeAttackInfo
// DJBFIXME:
// This should be moved to a build tool when we get one, there is no reason
// to do these calculations in-game since all the data is static!
//==============================================================================
void BProtoVisual::computeAttackInfo(long protoVisualModelIndex, long animType, float damagePerSecond, bool useDPSasDPA, float cooldownAverage, float& damagePerAttackOut, long& maxNumAttacksPerAnim)
{
   //-- Find the default model
   BSmallDynamicSimArray<float> damagePerAttackForAnimType;

   // Use the default model index if none is specified
   if (protoVisualModelIndex == -1)
      protoVisualModelIndex = mDefaultModelIndex;

   //-- Run through all the anims of this type and make sure at least one of them has an attack tag. If not then we ignore them all
   bool doesHaveAttackTags = hasAttackTags(protoVisualModelIndex, animType);            
   if (doesHaveAttackTags == true)
   {
      long modelCount = mModels.getNumber();
      bool foundAnimType = false;
      // Check the anims of the correct model rather than searching through all of them.  hasAttackTags will only return
      // true if it found attack tags for this model.
      if ((0 <= protoVisualModelIndex) && (protoVisualModelIndex < modelCount))
      {
         BProtoVisualModel* pDefaultModel = mModels[protoVisualModelIndex];
         //-- Find the anim specified
         long animCount = pDefaultModel->mAnims.getNumber();
         for (long anim = 0; anim < animCount; anim++)
         {
            BProtoVisualAnim* pDefaultAnim = &(pDefaultModel->mAnims[anim]);
            //-- Make sure this is the specified anim
            if (pDefaultAnim->mAnimType == animType)
            {
               foundAnimType = true;

               BSmallDynamicSimArray<float> attacks;
               BSmallDynamicSimArray<float> lengths;
               BSmallDynamicSimArray<float> weights;
               float weightTotal = 0.0f;

               //-- Animation assets
               long animAssetCount = pDefaultAnim->mAssets.getNumber();
               for (long animAsset = 0; animAsset < animAssetCount; animAsset++)
               {
                  BProtoVisualAsset* pAnimAsset = &(pDefaultAnim->mAssets[animAsset]);
                  if (pAnimAsset->mAssetType != cVisualAssetGrannyAnim)
                     continue;

                  //-- How many attack tags?
                  long numAttackTags = getNumAttackTags(protoVisualModelIndex, animType, animAssetCount, animAsset);

                  //-- Store off the max number of attack tags for all attack animations of this type
                  maxNumAttacksPerAnim = max(numAttackTags, maxNumAttacksPerAnim);

                  //-- Get the duration of the anim
                  ensureAssetIsLoadedStub(pAnimAsset);
//-- FIXING PREFIX BUG ID 7411
                  const BGrannyAnimation* pGrannyAnim = gGrannyManager.getAnimation(pAnimAsset->mAssetIndex);
//--
                  if (!pGrannyAnim)
                     continue;

                  float length = pGrannyAnim->getDuration();       

                  //-- Modify the length, by the average cooldown time
                  if(cooldownAverage != 0.0f)
                     length += cooldownAverage;            

                  weights.add((float)pAnimAsset->mWeight);
                  lengths.add(length);
                  // rg [3/1/07] - Is this cast correct??
                  attacks.add((float)numAttackTags);

                  weightTotal += pAnimAsset->mWeight;
               }

               //-- Normalize the weights
               long numAnims = weights.getNumber();
               for (long anim = 0; anim < numAnims; anim++)
                  weights[anim] = weights[anim] / weightTotal;

               //-- Calc weighted lengths
               float weightedLength = 0;
               for (long anim = 0; anim < numAnims; anim++)
                  weightedLength += lengths[anim] * weights[anim];

               //-- Calc weighted attack #
               float weightedAttackNum = 0;
               for (long anim = 0; anim < numAnims; anim++)
                  weightedAttackNum += attacks[anim] * weights[anim];

               float dpa;
               if (useDPSasDPA)
                  dpa = damagePerSecond / max(weightedAttackNum, 0.1f);
               else
                  dpa = damagePerSecond * weightedLength / max(weightedAttackNum, 0.1f);

               if (dpa != 0.0f)
                  damagePerAttackForAnimType.add(dpa);

               break;
            }
         }
      }
   }

   //-- Average all the dpa for each animation together. 
   //-- This handles the case where multiple anims are playing at the same time for the same attack.
   float totalDamagePerAttack = 0.0f;
   long numAnimsOfType = damagePerAttackForAnimType.getNumber();
   for (long i = 0; i < numAnimsOfType; i++)
   {
      totalDamagePerAttack += damagePerAttackForAnimType[i];
   }
   if (numAnimsOfType != 0)
      totalDamagePerAttack = totalDamagePerAttack / numAnimsOfType;

   damagePerAttackOut = totalDamagePerAttack;
}

//==============================================================================
//BProtoVisual::hasAttackTags
//==============================================================================
bool BProtoVisual::hasAttackTags(long protoVisualModelIndex, long animType)
{
   bool hasAttackTags = false;
   long modelCount = mModels.getNumber();
   for (long model = 0; (model < modelCount); model++)
   {
      BProtoVisualModel* pModel = mModels[model];
      long animCount = pModel->mAnims.getNumber();
      for (long anim = 0; (anim < animCount); anim++)
      {
         BProtoVisualAnim* pAnim = &(pModel->mAnims[anim]);
         if (pAnim->mAnimType == animType)
         {
            long animAssetCount = pAnim->mAssets.getNumber();
            for (long animAsset = 0;  (animAsset < animAssetCount); animAsset++)
            {
               BProtoVisualAsset* pAnimAsset = &(pAnim->mAssets[animAsset]);
               if (pAnimAsset->mAssetType != cVisualAssetGrannyAnim)
                  continue;

               //-- How many attack tags?                  
               long numTags = pAnimAsset->mTags.getNumber();
               for (long tagIndex = 0; tagIndex < numTags; tagIndex++)
               {
                  if (pAnimAsset->mTags[tagIndex].mEventType == cAnimEventAttack)
                  {
                     if (model == protoVisualModelIndex)
                     {
                        hasAttackTags = true;
                        break;
                     }
                     else
                     {
//-- FIXING PREFIX BUG ID 7412
                        const BProtoVisualModel* pTargetModel = mModels[protoVisualModelIndex];
//--
                        gConsoleOutput.warning("BProtoVisual::computeAttackInfo - %s has an attack tag at the wrong place. It's under %s when it should be under %s.", getName().getPtr(), pModel->mModelName.getPtr(), pTargetModel->mModelName.getPtr());
                        BASSERTM(0, "BProtoVisual::computeAttackInfo - A vis file has an attack tag at the wrong place. It will be ignored. Check the warnings log in XFS for more information. You can press A to continue.");
                     }
                  }
               }
            }
            break;
         }
      }
   }
   return hasAttackTags;
}

//==============================================================================
//BProtoVisual::getNumAttackTags
//==============================================================================
long BProtoVisual::getNumAttackTags(long protoVisualModelIndex, long animType, long animAssetCount, long animAsset)
{
   long numAttackTags=0;

   // Accumulate attack tags for the anim asset
   long modelCount = mModels.getNumber();
   for (long model = 0; model < modelCount; model++)
   {
      BProtoVisualModel* pModel = mModels[model];
      long animCount = pModel->mAnims.getNumber();
      for (long anim = 0; anim < animCount; anim++)
      {
         BProtoVisualAnim* pAnim = &(pModel->mAnims[anim]);
         if (pAnim->mAnimType == animType)
         {
            BASSERT(animAssetCount == pAnim->mAssets.getNumber());

            BProtoVisualAsset* pAnimAsset2 = &(pAnim->mAssets[animAsset]);
            BASSERT(pAnimAsset2->mAssetType == cVisualAssetGrannyAnim);
            //BASSERT(pAnimAsset2->mAssetIndex == pAnimAsset->mAssetIndex);

            long numTags = pAnimAsset2->mTags.getNumber();
            for (long tagIndex = 0; tagIndex < numTags; tagIndex++)
            {
               if (pAnimAsset2->mTags[tagIndex].mEventType == cAnimEventAttack)
               {
                  if (model == protoVisualModelIndex)
                  {
                     numAttackTags++;
                  }
               }
            }
            break;
         }
      }
   }
   return numAttackTags;
}

//==============================================================================
// BProtoVisualTag::BProtoVisualTag
//==============================================================================
BProtoVisualTag::BProtoVisualTag() :
   mFlags(),
   mEventType(-1), 
   mPosition(0.0f),
   mValue0(0.0f),
   mValue1(0.0f),
   mValue2(0.0f),
   mName(),
   mToBoneName(),
   mToBoneHandle(-1),
   mToBoneModelIndex(-1),
   mData0(-1),
   mLifespan(0.25f),
   mBoolValue0(false)
{
}

//==============================================================================
// BProtoVisualTag::operator=
//==============================================================================
BProtoVisualTag& BProtoVisualTag::operator=(const BProtoVisualTag& source)
{
   if(this==&source)
      return *this;
   mFlags=source.mFlags;
   mEventType=source.mEventType;
   mPosition=source.mPosition;
   mValue0=source.mValue0;
   mValue1=source.mValue1;
   mValue2=source.mValue2;
   mName=source.mName;
   mToBoneName=source.mToBoneName;
   mToBoneHandle=source.mToBoneHandle;
   mToBoneModelIndex=source.mToBoneModelIndex;
   mData0 = source.mData0;
   mLifespan = source.mLifespan;
   mBoolValue0 = source.mBoolValue0;
   
   return *this;
}

//=============================================================================
// BProtoVisualTag::save
//=============================================================================
bool BProtoVisualTag::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int8, mEventType);
   GFWRITEVAR(pStream, float, mPosition);
   GFWRITEVAR(pStream, int, mValueInt0);
   GFWRITEVAR(pStream, int, mValueInt1);
   GFWRITEVAR(pStream, int, mValueInt2);
   GFWRITESTRING(pStream, BSimString, mName, 100);
   GFWRITESTRING(pStream, BSimString, mToBoneName, 100);
   GFWRITEVAR(pStream, long, mToBoneHandle);
   GFWRITEVAR(pStream, int, mToBoneModelIndex);
   GFWRITEVAR(pStream, int, mData0);
   GFWRITEVAR(pStream, float, mLifespan);
   GFWRITEVAR(pStream, bool, mBoolValue0);
   GFWRITEUTBITVECTOR(pStream, mFlags, uint8, 8);

   return true;
}

//=============================================================================
// BProtoVisualTag::load
//=============================================================================
bool BProtoVisualTag::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int8, mEventType);
   GFREADVAR(pStream, float, mPosition);
   GFREADVAR(pStream, int, mValueInt0);
   GFREADVAR(pStream, int, mValueInt1);
   GFREADVAR(pStream, int, mValueInt2);
   GFREADSTRING(pStream, BSimString, mName, 100);
   GFREADSTRING(pStream, BSimString, mToBoneName, 100);
   GFREADVAR(pStream, long, mToBoneHandle);
   GFREADVAR(pStream, int, mToBoneModelIndex);
   GFREADVAR(pStream, int, mData0);
   GFREADVAR(pStream, float, mLifespan);
   GFREADVAR(pStream, bool, mBoolValue0);
   GFREADUTBITVECTOR(pStream, mFlags, uint8, 8);

   // Need to invalidate bone handle on load because it should be looked up again since it can change
   mToBoneHandle = -1;

   return true;
}

//==============================================================================
// BProtoVisualAsset::BProtoVisualAsset
//==============================================================================
BProtoVisualAsset::BProtoVisualAsset() : 
   mAssetType(-1), 
   mAssetName(),
   mAssetIndex(-1), 
   mDamageAssetName(),
   mDamageAssetIndex(-1), 
   mLoadFailed(false), 
   mWeight(1), 
   mTags(),
   mpOpacityProgression(NULL),
   mMinCorner(cOriginVector), 
   mMaxCorner(cOriginVector)
{
   Utils::ClearObj(mModelUVOffsets);
}

//==============================================================================
// BProtoVisualAsset::operator=
//==============================================================================
BProtoVisualAsset& BProtoVisualAsset::operator=(const BProtoVisualAsset& source)
{
   if(this==&source)
      return *this;
   mAssetType=source.mAssetType;
   mAssetName=source.mAssetName;
   mAssetIndex=source.mAssetIndex;
   mDamageAssetName=source.mDamageAssetName;
   mDamageAssetIndex=source.mDamageAssetIndex;
   mLoadFailed=source.mLoadFailed;
   mWeight=source.mWeight;
   mTags=source.mTags;
   mpOpacityProgression=source.mpOpacityProgression;
   mMinCorner=source.mMinCorner;
   mMaxCorner=source.mMaxCorner;
   mModelUVOffsets=source.mModelUVOffsets;
   return *this;
}

//==============================================================================
// BProtoVisualAttachment::BProtoVisualAttachment
//==============================================================================
BProtoVisualAttachment::BProtoVisualAttachment() : 
   mAttachmentName(),
   mAttachmentHandle(-1),
   mAttachmentType(-1),
   mAttachmentIndex(-1),
   mFromBoneName(),
   mFromBoneHandle(-1), 
   mToBoneName(),
   mToBoneHandle(-1), 
   mFromBoneFailed(false), 
   mToBoneFailed(false), 
   mSyncAnims(false),
   mDisregardOrient(false)
{
}

//==============================================================================
// BProtoVisualAttachment::operator=
//==============================================================================
BProtoVisualAttachment& BProtoVisualAttachment::operator=(const BProtoVisualAttachment& source)
{
   if(this==&source)
      return *this;
   mAttachmentName=source.mAttachmentName;
   mAttachmentHandle=source.mAttachmentHandle;
   mAttachmentType=source.mAttachmentType;
   mAttachmentIndex=source.mAttachmentIndex;
   mFromBoneName=source.mFromBoneName;
   mFromBoneHandle=source.mFromBoneHandle;
   mToBoneName=source.mToBoneName;
   mToBoneHandle=source.mToBoneHandle;
   mFromBoneFailed=source.mFromBoneFailed;
   mToBoneFailed=source.mToBoneFailed;
   mSyncAnims=source.mSyncAnims;
   mDisregardOrient=source.mDisregardOrient;
   return *this;
}

//==============================================================================
// BProtoVisualPoint::BProtoVisualPoint
//==============================================================================
BProtoVisualPoint::BProtoVisualPoint() :
   mPointType(-1),
   mPointDataName(),
   mPointDataValue(0),
   mBoneName(),
   mBoneHandle(-1)
{
}

//==============================================================================
// BProtoVisualPoint::operator=
//==============================================================================
BProtoVisualPoint& BProtoVisualPoint::operator=(const BProtoVisualPoint& source)
{
   if(this==&source)
      return *this;
   mPointType=source.mPointType;
   mPointDataName=source.mPointDataName;
   mPointDataValue=source.mPointDataValue;
   mBoneName=source.mBoneName;
   mBoneHandle=source.mBoneHandle;
   return *this;
}

//==============================================================================
// BProtoVisualLogicValue::BProtoVisualLogicValue
//==============================================================================
BProtoVisualLogicValue::BProtoVisualLogicValue() :
   mValueName(),
   mValueDWORD(0),
   mValueFloat(0.0f),
   mpModel(NULL)
{
}

//==============================================================================
// BProtoVisualLogicValue::operator=
//==============================================================================
BProtoVisualLogicValue& BProtoVisualLogicValue::operator=(const BProtoVisualLogicValue& source)
{
   if(this==&source)
      return *this;
   mValueName=source.mValueName;
   mValueDWORD=source.mValueDWORD;
   mValueFloat=source.mValueFloat;
   mpModel=source.mpModel;
   return *this;
}

//==============================================================================
// BProtoVisualLogicNode::BProtoVisualLogicNode
//==============================================================================
BProtoVisualLogicNode::BProtoVisualLogicNode() :
   mLogicType(-1),
   mLogicValues()
{
}

//==============================================================================
// BProtoVisualLogicNode::operator=
//==============================================================================
BProtoVisualLogicNode& BProtoVisualLogicNode::operator=(const BProtoVisualLogicNode& source)
{
   if(this==&source)
      return *this;
   mLogicType=source.mLogicType;
   mLogicValues=source.mLogicValues;
   return *this;
}

//==============================================================================
//==============================================================================
BProtoVisualOptionalMesh::BProtoVisualOptionalMesh() :
   mMeshName(),
   mMeshIndex(-1)
{
}

//==============================================================================
//==============================================================================
BProtoVisualOptionalMesh& BProtoVisualOptionalMesh::operator=(const BProtoVisualOptionalMesh& source)
{
   mMeshName = source.mMeshName;
   mMeshIndex = source.mMeshIndex;
   return *this;
}

//==============================================================================
// BProtoVisualAnim::BProtoVisualAnim
//==============================================================================
BProtoVisualAnim::BProtoVisualAnim() : 
   mAnimType(-1),
   mAssets(),
   mAttachmentLinks(),
   mOptionalMeshes()
{
   mExitAction.mExitAction = -1;
   mExitAction.mTweenToAnimation = -1;
   mExitAction.mTweenTime = 0.0f;
}

//==============================================================================
// BProtoVisualAnim::operator=
//==============================================================================
BProtoVisualAnim& BProtoVisualAnim::operator=(const BProtoVisualAnim& source)
{
   if(this==&source)
      return *this;
   mAnimType=source.mAnimType;
   mExitAction=source.mExitAction;
   mAssets=source.mAssets;
   mAttachmentLinks=source.mAttachmentLinks;
   mOptionalMeshes=source.mOptionalMeshes;
   return *this;
}

//==============================================================================
// BProtoVisualModel::BProtoVisualModel
//==============================================================================
BProtoVisualModel::BProtoVisualModel() : 
   mModelName(),
   mpLogicNode(NULL), 
   mAsset(),
   mAttachmentLinks(),
   mAnims(),
   mRefModelName(),
   mRefModelIndex(-1),
   mPoints(),
   mPointsInitialized(false)
{
}

//==============================================================================
// BProtoVisualModel::operator=
//==============================================================================
BProtoVisualModel& BProtoVisualModel::operator=(const BProtoVisualModel& source)
{
   if(this==&source)
      return *this;
   mModelName=source.mModelName;
   mpLogicNode=source.mpLogicNode;
   mAsset=source.mAsset;
   mAttachmentLinks=source.mAttachmentLinks;
   mAnims=source.mAnims;
   mRefModelName=source.mRefModelName;
   mRefModelIndex=source.mRefModelIndex;
   mPoints=source.mPoints;
   mPointsInitialized=source.mPointsInitialized;
   return *this;
}



char *gAttachmentTypeNames[] = { "ParticleFile",
                                 "ModelFile",
                                 "ModelRef",
                                 "LightFile",
                                 "TerrainEffect"};

char *gAssetTypeNames[] = {   "Model",
                              "Anim",
                              "Particle",
                              "Light" 
                          };

char *gAnimTypeNames[] =  {  "Idle",
                             "Walk",
                             "Jog",
                             "Run",
                             "RangedAttack",
                             "Attack",
                             "Limber",
                             "Unlimber",
                             "Death",
                             "Work",
                             "Research",
                             "Train",
                             "Bored",
                             "PelicanGarrison",
                             "PelicanGarrison2",
                             "PelicanGarrison3",
                             "PelicanGarrison4",
                             "PelicanGarrison5",
                             "PelicanGarrison6",
                             "PelicanGarrison7",
                             "PelicanGarrison8",
                             "PelicanGarrison9",
                             "PelicanGarrison10",
                             "PelicanUngarrison",
                             "PelicanUngarrison2",
                             "PelicanUngarrison3",
                             "PelicanUngarrison4",
                             "PelicanUngarrison5",
                             "PelicanUngarrison6",
                             "PelicanUngarrison7",
                             "PelicanUngarrison8",
                             "PelicanUngarrison9",
                             "PelicanUngarrison10",
                             "Incoming",
                             "Landing",
                             "Takeoff",
                             "Outgoing",
                             "HoverIdle",
                             "ShadePlasmaAttack",
                             "Clamshell",
                             "ClamshellRocket",
                             "Cinematic",
                             "EvadeLeft",
                             "EvadeRight",
                             "EvadeFront",
                             "EvadeBack",
                             "Block",
                             "Cheer",
                             "Retreat",
                             "HandSignalGo",
                             "HandSignalStop",
                             "Reload",
                             "CombatAction",
                             "Flail",
                             "Stop",
                             "TurnAround",
                             "TurnLeft",
                             "TurnRight",
                             "TurnRight45Forward",
                             "TurnRight45Back",
                             "TurnLeft45Back",
                             "TurnLeft45Forward",
                             "TurnWalk",
                             "TurnJog",
                             "TurnRun",
                             "Repair",
                             "FloodDeathJog",
                             "FloodDeath",
                             "IdleWalk",
                             "IdleJog",
                             "IdleRun",
                             "WalkIdle",
                             "JogIdle",
                             "RunIdle",
                             "Hitch",
                             "Unhitch", 
                             "Sprint",
                             "Recover",
                             "Heal",
                             "ArtilleryAttack",
                             "LockdownArtilleryAttack",
                             "Stasis",
                             "Roar",
                             "Cower",
                             "Kamikaze",
                             "Stunned",
                             "Detonated",
                             "ShatterDeath",
                             "RageLeap",
                             "HotDropDown",
                             "PowerIdle"
                             };

char *gPointTypeNames[] = {   "Impact",
                              "Launch",
                              "HitpointBar",
                              "Reflect",
                              "Cover",
                              "Carry",
                              "Pickup",
                              "Board",
                              "Bobble"
                              };

char *gTagTypeNames[] = {     "Attack",
                              "Sound",
                              "Particle",
                              "TerrainEffect",
                              "CameraShake",
                              "Loop",
                              "Light",
                              "GroundIK",
                              "AttachTarget",
                              "SweetSpot",
                              "TerrainAlpha",
                              "Rumble",
                              "BuildingDecal",
                              "UVOffset",
                              "KillAndThrow",
                              "PhysicsImpulse"};

char *gLogicTypeNames[] = {   "Variation",
                              "BuildingCompletion",
                              "Tech",
                              "SquadMode",
                              "ImpactSize",
                              "Destruction" };


//==============================================================================
// VIS loading helper functions
//==============================================================================
void loadAttachment(BProtoVisual *pProtoVisual, BXMLNode node, BProtoVisualModel *pModel, BProtoVisualAttachment *pAttachment)
{   
   long attachedType = -1;
   BSimString type;

   if(node.getAttribValue("type", &type))
   {
      if(type==gAttachmentTypeNames[0])
      {
         attachedType = cAttachmentParticleFile;
      }
      else if (type==gAttachmentTypeNames[1])
      {
         attachedType = cAttachmentModelFile;
      }
      else if (type==gAttachmentTypeNames[2])
      {
         attachedType = cAttachmentModelRef;
      }
      else if (type==gAttachmentTypeNames[3])
      {
         attachedType = cAttachmentLightFile;
      }
      else if (type==gAttachmentTypeNames[4])
      {
         attachedType = cAttachmentTerrainEffect;
      }
   } 
   
   node.getAttribValueAsString("name", pAttachment->mAttachmentName);
   node.getAttribValueAsString("frombone", pAttachment->mFromBoneName);
   node.getAttribValueAsString("tobone", pAttachment->mToBoneName);
   node.getAttribValueAsBool("syncanims", pAttachment->mSyncAnims);
   node.getAttribValueAsBool("disregardorient", pAttachment->mDisregardOrient);

   pAttachment->mAttachmentHandle=gVisualManager.getAttachmentType(pAttachment->mAttachmentName); // FIXME AJL 10/3/06 - We should be using a separate attachment type value instead of the name
   pAttachment->mAttachmentType = attachedType;

   // Create new asset if dealing with a ParticleFile, ModelFile, LightFile or TerrainEffect
   if((attachedType == cAttachmentParticleFile) || (attachedType == cAttachmentModelFile) || (attachedType == cAttachmentLightFile) || (attachedType == cAttachmentTerrainEffect))
   {
      bool foundAsset = false;
      long attachmentIndex = -1;
      for(long k=0; k<pProtoVisual->mModels.getNumber(); k++)
      {
         if(pProtoVisual->mModels[k]->mModelName==pAttachment->mAttachmentName)
         {
            foundAsset = true;
            attachmentIndex=k;
            break;
         }
      }

      if(!foundAsset)
      {
         attachmentIndex = pProtoVisual->mModels.getNumber();

         BProtoVisualModel *pNewModel = new BProtoVisualModel();
         pProtoVisual->mModels.add(pNewModel);

         switch(attachedType)
         {
            case cAttachmentParticleFile:
               pNewModel->mAsset.mAssetType = cVisualAssetParticleSystem;
               break;
            case cAttachmentModelFile:
               pNewModel->mAsset.mAssetType = cVisualAssetGrannyModel;
               break;
            case cAttachmentLightFile:
               pNewModel->mAsset.mAssetType = cVisualAssetLight;
               break;
            case cAttachmentTerrainEffect:
               pNewModel->mAsset.mAssetType = cVisualAssetTerrainEffect;
               break;
            default:
               BDEBUG_ASSERT(0);
         }
         pNewModel->mAsset.mAssetName = pAttachment->mAttachmentName;
         pNewModel->mModelName = pAttachment->mAttachmentName;
      }

      pAttachment->mAttachmentIndex = attachmentIndex;
   }

   pProtoVisual->setFlag(BProtoVisual::cFlagHasAttachment, true);
}

void loadPoint(BProtoVisual* pProtoVisual, BXMLNode node, BProtoVisualPoint* pPoint)
{
   const BXMLAttribute attr(node.getAttribute("pointType"));
   if (attr)
   {
     BSimString type;
     attr.getValue(type);
      if (type == gPointTypeNames[0])
         pPoint->mPointType = cVisualPointImpact;
      else if (type == gPointTypeNames[1])
         pPoint->mPointType = cVisualPointLaunch;
      else if (type == gPointTypeNames[2])
         pPoint->mPointType = cVisualPointHitpointBar;
      else if (type == gPointTypeNames[3])
         pPoint->mPointType = cVisualPointReflect;
      else if (type == gPointTypeNames[4])
         pPoint->mPointType = cVisualPointCover;
      else if (type == gPointTypeNames[5])
         pPoint->mPointType = cVisualPointCarry;
      else if (type == gPointTypeNames[6])
         pPoint->mPointType = cVisualPointPickup;
      else if (type == gPointTypeNames[7])
         pPoint->mPointType = cVisualPointBoard;
      else if (type == gPointTypeNames[8])
         pPoint->mPointType = cVisualPointBobble;
   }

   node.getAttribValueAsString("pointdata", pPoint->mPointDataName);
   node.getAttribValueAsString("bone", pPoint->mBoneName);
}

void loadTag(BProtoVisual *pProtoVisual, BXMLNode node, BProtoVisualTag *pTag)
{
   const BXMLAttribute attr(node.getAttribute("type"));
   if(attr)
   {
      BSimString type;
      attr.getValue(type);
      if(type == gTagTypeNames[0])
         pTag->mEventType = cAnimEventAttack;
      else if(type == gTagTypeNames[1])
         pTag->mEventType = cAnimEventSound;
      else if(type == gTagTypeNames[2])
         pTag->mEventType = cAnimEventParticles;
      else if(type == gTagTypeNames[3])
         pTag->mEventType = cAnimEventTerrainEffect;
      else if(type == gTagTypeNames[4])
         pTag->mEventType = cAnimEventCameraShake;
      else if(type == gTagTypeNames[5])
         pTag->mEventType = cAnimEventLoop;
      else if(type == gTagTypeNames[6])
         pTag->mEventType = cAnimEventLight;         
      else if(type == gTagTypeNames[7])
         pTag->mEventType = cAnimEventGroundIK;
      else if(type == gTagTypeNames[8])
         pTag->mEventType = cAnimEventAttachTarget;
      else if(type == gTagTypeNames[9])
         pTag->mEventType = cAnimEventSweetSpot;
      else if(type == gTagTypeNames[10])
         pTag->mEventType = cAnimEventAlphaTerrain;
      else if(type == gTagTypeNames[11])
         pTag->mEventType = cAnimEventRumble;
      else if(type == gTagTypeNames[12])
         pTag->mEventType = cAnimEventBuildingDecal;
      else if(type == gTagTypeNames[13])
         pTag->mEventType = cAnimEventUVOffset;
      else if(type == gTagTypeNames[14])
         pTag->mEventType = cAnimEventKillAndThrow;
      else if(type == gTagTypeNames[15])
         pTag->mEventType = cAnimEventPhysicsImpulse;
   }

   node.getAttribValueAsFloat("position", pTag->mPosition);

   switch(pTag->mEventType)
   {
   case cAnimEventAttack:
      node.getAttribValueAsString("tobone", pTag->mToBoneName);
      break;
   case cAnimEventSound:
      {

         node.getAttribValueAsString("name", pTag->mName);

#ifndef BUILD_FINAL
         //-- Validate the sound tag name
         if(gConfig.isDefined(cConfigDebugSound) && !pTag->mName.isEmpty())
         {
            BCueIndex cueIndex = gDatabase.getSoundCueIndex(pTag->mName.getPtr());
            if(cueIndex == cInvalidCueIndex)
            {
               BString msg;
               msg.format("Cue Name not found in soundtable.xml: %s", pTag->mName.getPtr());
               BASSERTM(cueIndex != cInvalidCueIndex, msg);
               gConsoleOutput.output(cMsgError, msg);
            }
         }
#endif
         node.getAttribValueAsString("tobone", pTag->mToBoneName);

         bool checkVisible = true;
         node.getAttribValueAsBool("checkvisible", checkVisible);

         if(checkVisible)
         {
            pTag->setFlag(BProtoVisualTag::cFlagCheckVisible, true);
         }

         bool checkFOW = true;
         node.getAttribValueAsBool("checkFOW", checkFOW);
         if(checkFOW)
         {
            pTag->setFlag(BProtoVisualTag::cFlagCheckFOW, true);
         }
      }
      break;
   case cAnimEventParticles:
      node.getAttribValueAsString("name", pTag->mName);
      node.getAttribValueAsString("tobone", pTag->mToBoneName);
      node.getAttribValueAsFloat("lifespan", pTag->mLifespan);
      break;
   case cAnimEventTerrainEffect:
      node.getAttribValueAsString("name", pTag->mName);
      node.getAttribValueAsString("tobone", pTag->mToBoneName);
      node.getAttribValueAsFloat("lifespan", pTag->mLifespan);
      break;
   case cAnimEventCameraShake:
   {
      node.getAttribValueAsFloat("force", pTag->mValue1);
      node.getAttribValueAsFloat("lifespan", pTag->mLifespan);

      bool checkSelected = false;
      node.getAttribValueAsBool("checkSelected", checkSelected);
      if (checkSelected)
         pTag->setFlag(BProtoVisualTag::cFlagCheckSelected, true);

      break;
   }
   case cAnimEventLoop:
      break;
   case cAnimEventLight:
      node.getAttribValueAsString("name", pTag->mName);
      node.getAttribValueAsString("tobone", pTag->mToBoneName);
      node.getAttribValueAsFloat("lifespan", pTag->mLifespan);
      break;
   case cAnimEventGroundIK:
      {
         node.getAttribValueAsString("tobone", pTag->mToBoneName);
         node.getAttribValueAsFloat("end", pTag->mValue0);

         bool lockToGround = false;
         node.getAttribValueAsBool("lockToGround", lockToGround);

         if (lockToGround)
         {
            pTag->setFlag(BProtoVisualTag::cFlagLockToGround, true);
         }

         pTag->mBoolValue0 = true;
         node.getAttribValueAsBool("checkVisible", pTag->mBoolValue0);
      }
      break;
   case cAnimEventAttachTarget:
      {
         node.getAttribValueAsString("tobone", pTag->mToBoneName);

         bool attach = false;
         node.getAttribValueAsBool("attach", attach);

         if (attach)
         {
            pTag->setFlag(BProtoVisualTag::cFlagAttach, true);
         }
      }
      break;
   case cAnimEventSweetSpot:
      {
         // Trigger the tag when we need to start sweet spotting
         // mValue1 is the position in the animation where we give full weight to the sweet spot
         // mValue2 is where the sweet spot has no weight again
         pTag->mValue1 = pTag->mPosition;

         node.getAttribValueAsString("tobone", pTag->mToBoneName);
         node.getAttribValueAsFloat("start", pTag->mPosition);
         node.getAttribValueAsFloat("end", pTag->mValue2);
      }
      break;
   case cAnimEventBuildingDecal:
      {

         //parse our input string from the .vis file
         node.getAttribValueAsString("name", pTag->mName);

         node.getAttribValueAsString("tobone", pTag->mToBoneName);

         BSimString userData;
         node.getAttribValueAsString("userData", userData);


         bool storingSizeX=false;
         bool storingSizeZ=false;
         bool storingOpaque =false;
         bool storingFadeout=false;
         bool storingRandom=false;
         char* str = (char*)(userData.asNative());
         //"size 2.3 opaque 22.2 fadeouttime 53.2 orientation random"
         char * pch;
         char* pStrTokContext = NULL;
         pch = strtok_s(str," ,=", &pStrTokContext);
         while (pch != NULL)
         {
            if(storingSizeX)
            {
               pTag->mValue0 = (float)atof(pch);
               storingSizeX=false;
            }
            else if(storingSizeZ)
            {
               pTag->mValue1 = (float)atof(pch);
               storingSizeZ=false;
            }
            else if(storingOpaque)
            {
               pTag->mLifespan = (float)atof(pch);
               storingOpaque=false;
            }
            else if(storingFadeout)
            {
               pTag->mValue2 = (float)atof(pch);
               storingFadeout=false;
            }
            else if(storingRandom)
            {
               if(!strcmp(pch,"random"))               pTag->mBoolValue0 = false;
               if(!strcmp(pch,"forward"))              pTag->mBoolValue0 = true;
               storingRandom=false;
            }


            if(!strcmp(pch,"sizeX"))
            {
               storingSizeX=true;
            }
            else if(!strcmp(pch,"sizeZ"))
            {
               storingSizeZ=true;
            }
            else if(!strcmp(pch,"opaque"))
            {
               storingOpaque =true;
            }
            else if(!strcmp(pch,"fadeouttime"))
            {
               storingFadeout=true;
            }
            else  if(!strcmp(pch,"orientation"))
            {
               storingRandom=true;
            }
            pch = strtok_s(NULL, " ,=", &pStrTokContext);
         }
         break;
      }
   case cAnimEventAlphaTerrain:
      {
         node.getAttribValueAsString("tobone", pTag->mToBoneName);

         BSimString userData;
         node.getAttribValueAsString("userData", userData);


         bool storingSizeX=false;
         bool storingSizeZ=false;
         bool storingType =false;
         bool storingValue=false;

         char* str = (char*)(userData.asNative());
         //"type Rect value On sizeX 53.2 sizeZ 53.2"
         char * pch;
         char* pStrTokContent = NULL;
         pch = strtok_s(str," ,=", &pStrTokContent);
         while (pch != NULL)
         {
            if(storingSizeX)
            {
               pTag->mValue0 = (float)atof(pch);
               storingSizeX=false;
            }
            else if(storingSizeZ)
            {
               pTag->mValue1 = (float)atof(pch);
               storingSizeZ=false;
            }
            else if(storingType)
            {
               pTag->mBoolValue0 = !strcmp(pch,"circle");
               storingType=false;
            }
            else if(storingValue)
            {
               pTag->mData0 = !strcmp(pch,"on");
               storingValue=false;
            }


            if(!strcmp(pch,"sizeX"))
            {
               storingSizeX=true;
            }
            else if(!strcmp(pch,"sizeZ"))
            {
               storingSizeZ=true;
            }
            else if(!strcmp(pch,"type"))
            {
               storingType =true;
            }
            else  if(!strcmp(pch,"value"))
            {
               storingValue=true;
            }
            pch = strtok_s(NULL, " ,=", &pStrTokContent);
         }
         break;
      }
   case cAnimEventRumble:
      {
         node.getAttribValueAsFloat("force", pTag->mValue0);
         node.getAttribValueAsFloat("force2", pTag->mValue1);
         node.getAttribValueAsFloat("lifespan", pTag->mLifespan);

         BSimString str;
         if (node.getAttribValueAsString("leftRumbleType", str))
            pTag->mData0 = BGamepad::getRumbleType(str);

         if (node.getAttribValueAsString("rightRumbleType", str))
            pTag->mValueInt2 = BGamepad::getRumbleType(str);

         node.getAttribValueAsBool("loop", pTag->mBoolValue0);

         bool checkSelected = false;
         node.getAttribValueAsBool("checkSelected", checkSelected);
         if (checkSelected)
            pTag->setFlag(BProtoVisualTag::cFlagCheckSelected, true);

         break;
      }
   case cAnimEventUVOffset:
      {

         BSimString userData;
         node.getAttribValueAsString("userData", userData);


         bool uOffset=false;
         bool vOffset=false;
       

         char* str = (char*)(userData.asNative());
         //"type Rect value On sizeX 53.2 sizeZ 53.2"
         char * pch;
         char* pStrTokContent = NULL;
         pch = strtok_s(str," ,=", &pStrTokContent);
         while (pch != NULL)
         {
            if(uOffset)
            {
               pTag->mValue0 = (float)atof(pch);
               uOffset=false;
            }
            else if(vOffset)
            {
               pTag->mValue1 = (float)atof(pch);
               vOffset=false;
            }
          

            if(!strcmp(pch,"u0Offset"))
            {
               uOffset=true;
            }
            else if(!strcmp(pch,"v0Offset"))
            {
               vOffset=true;
            }
          
            pch = strtok_s(NULL, " ,=", &pStrTokContent);
         }
         break;
      }
   case cAnimEventKillAndThrow:
      {
         node.getAttribValueAsString("tobone", pTag->mToBoneName);
         node.getAttribValueAsFloat("lifespan", pTag->mLifespan);
      }
      break;
   case cAnimEventPhysicsImpulse:
      {
         node.getAttribValueAsString("tobone", pTag->mToBoneName);
         // type (linear, angular, point)
         node.getAttribValueAsInt("start", pTag->mData0);
         pTag->mData0 = Math::Clamp(pTag->mData0, 0, 2);
         // xyz/ypr
         node.getAttribValueAsFloat("force", pTag->mValue0);
         node.getAttribValueAsFloat("force2", pTag->mValue1);
         node.getAttribValueAsFloat("lifespan", pTag->mValue2);
         // attached
         node.getAttribValueAsBool("checkSelected", pTag->mBoolValue0);
         break;
      }
   }
   

   bool disregardOrient = false;
   node.getAttribValueAsBool("disregardorient", disregardOrient);

   if(disregardOrient)
   {
      pTag->setFlag(BProtoVisualTag::cFlagDiregardOrient, true);
   }
}


static void loadAssetNode(BProtoVisual *pProtoVisual, BXMLNode node, BProtoVisualAsset *pAsset)
{
   BSimString type;
   if(node.getAttribValue("type", &type))
   {
      if(type==gAssetTypeNames[0])
      {
         pAsset->mAssetType=cVisualAssetGrannyModel;
      }
      else if (type==gAssetTypeNames[1])
      {
         pAsset->mAssetType=cVisualAssetGrannyAnim;
      }
      else if (type==gAssetTypeNames[2])
      {
         pAsset->mAssetType=cVisualAssetParticleSystem;
      }
      else if(type==gAssetTypeNames[3])
      {
         pAsset->mAssetType=cVisualAssetLight;
      }
   } 

   long numChildren = node.getNumberChildren();
   for(long i=0; i<numChildren; i++)
   {
      BXMLNode child(node.getChild(i));
      const BPackedString childName(child.getName());

      if(childName == "file")
      {
         child.getText(pAsset->mAssetName);
      }
      else if(childName == "uvOffset")
      {
         uint channelIndex = 0;
         float uOfs = 0.0f;
         float vOfs = 0.0f;
         
         child.getAttribValueAsUInt("channel", channelIndex);
         child.getAttribValueAsFloat("uOfs", uOfs);
         child.getAttribValueAsFloat("vOfs", vOfs);
         
         if (channelIndex >= BVisualModelUVOffsets::cMaxUVOffsets)
         {
            gConsoleOutput.error("Invalid uvOffset channel in protovisual: %s", pProtoVisual->getName().getPtr());
            channelIndex = 0;
         }
         
         pAsset->mModelUVOffsets.mUVOffsets[channelIndex].set(uOfs, vOfs);
      }
      else if(childName == "damagefile")
      {
         child.getText(pAsset->mDamageAssetName);
      }
      else if(childName == "weight")
      {
         child.getTextAsLong(pAsset->mWeight);
      }
      else if(childName == "tag")
      {
         long tagIndex=pAsset->mTags.getNumber();
         if(pAsset->mTags.setNumber(tagIndex+1))
         {
            BProtoVisualTag* pTag=&(pAsset->mTags[tagIndex]);
            loadTag(pProtoVisual, child, pTag);
         }
      }
      else if(childName == "progression")
      {
         pAsset->mpOpacityProgression = new BFloatProgression();
         pAsset->mpOpacityProgression->load(child, (BXMLReader*) 0x11111111);  // pass junk for reader param since it isn't use and can't be null
      }
   }
}


void loadLogicData(BProtoVisual *pProtoVisual, BXMLNode node, long logicType, BProtoVisualLogicValue *pLogicValue);

void loadLogic(BProtoVisual *pProtoVisual, BXMLNode node, BProtoVisualLogicNode *pLogic)
{
   const BXMLAttribute attr=node.getAttribute("type");
   if(attr)
   {
      BSimString type;
      attr.getValue(type);
      if(type == gLogicTypeNames[0])
         pLogic->mLogicType = cVisualLogicVariation;
      else if(type == gLogicTypeNames[1])
         pLogic->mLogicType = cVisualLogicBuildingCompletion;
      else if(type == gLogicTypeNames[2])
         pLogic->mLogicType = cVisualLogicTech;
      else if(type == gLogicTypeNames[3])
         pLogic->mLogicType = cVisualLogicSquadMode;
      else if(type == gLogicTypeNames[4])
         pLogic->mLogicType = cVisualLogicImpactSize;
      else if(type == gLogicTypeNames[5])
         pLogic->mLogicType = cVisualLogicDestruction;
   }

   long numChildren = node.getNumberChildren();
   for(long i=0; i<numChildren; i++)
   {
      BXMLNode child(node.getChild(i));
      const BPackedString childName(child.getName());

      if(childName == "logicdata")
      {
         long valueIndex=pLogic->mLogicValues.getNumber();
         if(pLogic->mLogicValues.setNumber(valueIndex+1))
         {
            BProtoVisualLogicValue* pLogicValue=&(pLogic->mLogicValues[valueIndex]);
            loadLogicData(pProtoVisual, child, pLogic->mLogicType, pLogicValue);
         }
      }
   }
}

void loadLogicData(BProtoVisual *pProtoVisual, BXMLNode node, long logicType, BProtoVisualLogicValue *pLogicValue)
{
   pLogicValue->mpModel = new BProtoVisualModel();
   if(pLogicValue->mpModel)
   {
      node.getAttribValueAsString("value", pLogicValue->mValueName);
      gVisualManager.getVisualLogicValue(logicType, pLogicValue->mValueName, pLogicValue->mValueDWORD, pLogicValue->mValueFloat);

      if (logicType==cVisualLogicTech && pLogicValue->mValueDWORD!=(DWORD)-1)
         pProtoVisual->setFlag(BProtoVisual::cFlagHasTechLogic, true);

      if(logicType==cVisualLogicDestruction)
         pProtoVisual->setFlag(BProtoVisual::cFlagHasDestructionLogic, true);

      if(logicType==cVisualLogicSquadMode)
         pProtoVisual->setFlag(BProtoVisual::cFlagHasSquadModeLogic, true);

      node.getAttribValueAsString("modelref", pLogicValue->mpModel->mRefModelName);

      long numChildren = node.getNumberChildren();
      for(long i=0; i<numChildren; i++)
      {
         BXMLNode nodeChild(node.getChild(i));
         const BPackedString nodeChildName(nodeChild.getName());

         if(nodeChildName=="asset")
         {
            loadAssetNode(pProtoVisual, nodeChild, &pLogicValue->mpModel->mAsset);
         }
         else if(nodeChildName=="logic")
         {
            pLogicValue->mpModel->mpLogicNode = new BProtoVisualLogicNode();
            loadLogic(pProtoVisual, nodeChild, pLogicValue->mpModel->mpLogicNode);
         }
      }
   }
}

void loadAnim(BProtoVisual *pProtoVisual, BXMLNode node, BProtoVisualModel *pModel, BProtoVisualAnim *pAnim)
{
   const BXMLAttribute attr=node.getAttribute("type");
   if(attr)
   {
      BSimString valueStr;
      pAnim->mAnimType=gVisualManager.getAnimType(attr.getValue(valueStr));
   }

   // Get exitAction
   pAnim->mExitAction.mExitAction = cAnimExitActionLoop;
   BSimString exitActionName;
   if (node.getAttribValueAsString(B("exitAction"), exitActionName))
   {
      if (exitActionName == "Loop")
         pAnim->mExitAction.mExitAction = cAnimExitActionLoop;
      else if (exitActionName == "Freeze")
         pAnim->mExitAction.mExitAction = cAnimExitActionFreeze;
      else if (exitActionName == "Transition")
         pAnim->mExitAction.mExitAction = cAnimExitActionTransition;
   }

   // Get tweenToAnimation
   BSimString transitionToAnimName;
   if (node.getAttribValueAsString(B("tweenToAnimation"), transitionToAnimName) && !transitionToAnimName.isEmpty())
      pAnim->mExitAction.mTweenToAnimation = gVisualManager.getAnimType(transitionToAnimName);
   else
      pAnim->mExitAction.mTweenToAnimation = -1;

   // Get tweenTime
   long tweenTimeInFrames = 0;
   if (node.getAttribValueAsLong(B("tweenTime"), tweenTimeInFrames))
      pAnim->mExitAction.mTweenTime = float(tweenTimeInFrames) * (1.0f / 30.0f); // Convert to seconds
   else
      pAnim->mExitAction.mTweenTime = 0.0f;

   // load assets and attachments
   long childCount=node.getNumberChildren();
   for(long i=0; i<childCount; i++)
   {
      BXMLNode nodeChild(node.getChild(i));
      const BPackedString nodeChildName(nodeChild.getName());

      if(nodeChildName=="attach")
      {
         long attachmentIndex=pAnim->mAttachmentLinks.getNumber();
         if(pAnim->mAttachmentLinks.setNumber(attachmentIndex+1))
         {
            BProtoVisualAttachment* pAttachment=&(pAnim->mAttachmentLinks[attachmentIndex]);
            loadAttachment(pProtoVisual, nodeChild, pModel, pAttachment);
         }
      }
      else if(nodeChildName=="asset")
      {
         long assetIndex=pAnim->mAssets.getNumber();
         if(pAnim->mAssets.setNumber(assetIndex+1))
         {
            BProtoVisualAsset* pAsset=&(pAnim->mAssets[assetIndex]);
            loadAssetNode(pProtoVisual, nodeChild, pAsset);
         }
      }
      else if(nodeChildName=="meshEnable")
      {
         long optionalMeshIndex=pAnim->mOptionalMeshes.getNumber();
         if(pAnim->mOptionalMeshes.setNumber(optionalMeshIndex+1))
         {
            BProtoVisualOptionalMesh* pOptionalMesh = &(pAnim->mOptionalMeshes[optionalMeshIndex]);
            nodeChild.getText(pOptionalMesh->mMeshName);
            pOptionalMesh->mMeshIndex = -1;
            pProtoVisual->setFlag(BProtoVisual::cFlagHasOptionalMesh, true);
         }
      }
   }

   pProtoVisual->setFlag(BProtoVisual::cFlagHasAnimation, true);
}

void loadComponent(BProtoVisual *pProtoVisual, BXMLNode node, BProtoVisualModel *pModel)
{
   long childCount=node.getNumberChildren();
   for(long i=0; i<childCount; i++)
   {
      BXMLNode nodeChild(node.getChild(i));
      const BPackedString nodeChildName(nodeChild.getName());

      if(nodeChildName=="asset")
      {
         loadAssetNode(pProtoVisual, nodeChild, &pModel->mAsset);
      }
      else if(nodeChildName=="attach")
      {
         long attachmentIndex=pModel->mAttachmentLinks.getNumber();
         if(pModel->mAttachmentLinks.setNumber(attachmentIndex+1))
         {
            BProtoVisualAttachment* pAttachment=&(pModel->mAttachmentLinks[attachmentIndex]);
            loadAttachment(pProtoVisual, nodeChild, pModel, pAttachment);
         }
      }
      else if(nodeChildName=="point")
      {
         long pointIndex=pModel->mPoints.getNumber();
         if(pModel->mPoints.setNumber(pointIndex+1))
         {
            BProtoVisualPoint* pPoint=&(pModel->mPoints[pointIndex]);
            loadPoint(pProtoVisual, nodeChild, pPoint);
         }
      }
      else if(nodeChildName=="logic")
      {
         pModel->mpLogicNode = new BProtoVisualLogicNode();
         loadLogic(pProtoVisual, nodeChild, pModel->mpLogicNode);
      }
   }
}

//==============================================================================
// BProtoVisual::loadVIS
//==============================================================================
bool BProtoVisual::loadVIS()
{
   BString visName = mName;
   visName.removeExtension();
   visName.append(".vis");
      
   gConsoleOutput.resource("Loading VIS file: %s", visName.getPtr());

   BXMLReader reader;
   if(!reader.load(gVisualManager.getDirID(), visName.getPtr(), XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;

   BXMLNode rootNode(reader.getRootNode());


   // Read default mode
   BSimString defModelName;
   rootNode.getAttribValueAsString("defaultmodel", defModelName);


   long nodeCount=rootNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      const BPackedString nodeName(node.getName());

      if(nodeName=="model")
      {
         BProtoVisualModel *pModel =new BProtoVisualModel();
         node.getAttribValueAsString("name", pModel->mModelName);

         mModels.add(pModel);

         long nodeChildCount=node.getNumberChildren();
         for(long j=0; j<nodeChildCount; j++)
         {
            BXMLNode nodeChild(node.getChild(j));
            const BPackedString nodeChildName(nodeChild.getName());

            if(nodeChildName=="component")
            {
               loadComponent(this, nodeChild, pModel);
            }
            else if(nodeChildName=="anim")
            {
               long animIndex=pModel->mAnims.getNumber();
               if(pModel->mAnims.setNumber(animIndex+1))
               {
                  BProtoVisualAnim* pAnim=&(pModel->mAnims[animIndex]);
                  loadAnim(this, nodeChild, pModel, pAnim);
               }
            }
         }
      }
      else if(nodeName=="logic")
      {
         mpLogicNode = new BProtoVisualLogicNode();
         loadLogic(this, node, mpLogicNode);
      }
   }

   mDefaultModelIndex = -1;
   if(!defModelName.isEmpty())
   {
      for(long k=0; k<mModels.getNumber(); k++)
      {
         if(mModels[k]->mModelName==defModelName)
         {
            mDefaultModelIndex = k;
            break;
         }
      }
   }

   if((mDefaultModelIndex == -1) && (mModels.getNumber() >= 0))
   {
      mDefaultModelIndex = 0;
   }

   return(true);
}


#ifdef SAVE_VIS

//==============================================================================
// VIS saving helper functions
//==============================================================================
void saveAttachment(BXMLWriter &writer, const BProtoVisualModel &model, const BProtoVisualAttachment &attachment)
{
   if(attachment.mAttachmentIndex != -1)
   {
      BProtoVisualModel* pAttachmentModel = model.mAttachmentModels[attachment.mAttachmentIndex];

      // Only value attaments types are 
      if((pAttachmentModel->mAsset.mAssetType == cVisualAssetGrannyModel) ||
         (pAttachmentModel->mAsset.mAssetType == cVisualAssetParticleSystem) ||
         (pAttachmentModel->mAsset.mAssetType == cVisualAssetLight))
      {   
         writer.startItem("attach");

         switch(pAttachmentModel->mAsset.mAssetType)
         {
            case cVisualAssetGrannyModel:
               writer.addAttribute("type", gAttachmentTypeNames[1]);
               break;
            case cVisualAssetParticleSystem:
               writer.addAttribute("type", gAttachmentTypeNames[0]);
               break;
         }

         //writer.addAttribute("a", attachment.mAttachmentName);
         writer.addAttribute("name", pAttachmentModel->mAsset.mAssetName);

         if(!attachment.mToBoneName.isEmpty())
            writer.addAttribute("tobone", attachment.mToBoneName);

         if(!attachment.mFromBoneName.isEmpty() && (pAttachmentModel->mAsset.mAssetType != cVisualAssetParticleSystem))
            writer.addAttribute("frombone", attachment.mFromBoneName);

         writer.addAttribute("syncanims", attachment.mSyncAnims);
         writer.endItem();
      }
   }
}

void savePoint(BXMLWriter &writer, const BProtoVisualPoint &point)
{
   writer.startItem("point");

   if((point.mPointType < 0) || (point.mPointType >= cVisualPointMaxCount))
      writer.addAttribute("pointType", "unknown");
   else
      writer.addAttribute("pointType", gPointTypeNames[point.mPointType]);

   writer.addAttribute("bone", point.mBoneName);
   writer.addAttribute("pointData", point.mPointDataName);
   writer.endItem();
}

void saveTag(BXMLWriter &writer, const BProtoVisualTag &tag)
{
   writer.startItem("tag");

   if((tag.mEventType < 0) || (tag.mEventType >= cAnimEventMaxCount))
      writer.addAttribute("type", "unknown");
   else
      writer.addAttribute("type", gTagTypeNames[tag.mEventType]);

   if (tag.mEventType == cAnimEventSweetSpot)
   {
      // swap start with position
      float temp = tag.mPosition;
      tag.mPosition = tag.mValue1;
      tag.mValue1 = temp;
   }

   writer.addAttribute("position", tag.mPosition, 2);

   switch(tag.mEventType)
   {
   case cAnimEventAttack:
      break;
   case cAnimEventSound:
      writer.addAttribute("name", tag.mName);   
      writer.addAttribute("checkvisible", tag.getFlag(BProtoVisualTag::cFlagCheckVisible));
      break;
   case cAnimEventParticles:
      writer.addAttribute("name", tag.mName);   
      break;
   case cAnimEventCameraShake:
      writer.addAttribute("force", tag.mValue1);   
      writer.addAttribute("lifespan", tag.mLifespan);   
      break;
   case cAnimEventLoop:
      break;
   case cAnimEventLight:
      writer.addAttribute("name", tag.mName);   
      break;
   case cAnimEventGroundIK:
      writer.addAttribute("lockToGround", tag.getFlag(BProtoVisualTag::cFlagLockToGround));
      break;
   case cAnimEventAttachTarget:
      writer.addAttribute("attach", tag.getFlag(BProtoVisualTag::cFlagAttach));
      break;
   case cAnimEventSweetSpot:
      writer.addAttribute("start", tag.mValue1);
      writer.addAttribute("end", tag.mValue2);
      break;
   }

   writer.endItem();
}

void saveAsset(BXMLWriter &writer, const BProtoVisualAsset &asset)
{
   writer.startItem("asset");

   if((asset.mAssetType < 0) || (asset.mAssetType >= cVisualAssetMaxCount))
      writer.addAttribute("type", "unknown");
   else
      writer.addAttribute("type", gAssetTypeNames[asset.mAssetType]);

   switch(asset.mAssetType)
   {
   case cVisualAssetGrannyModel:
   case cVisualAssetParticleSystem:
   case cVisualAssetLight:
      writer.addItem("file", asset.mAssetName);
      writer.addItem("damagefile", asset.mDamageAssetName);
      break;
   case cVisualAssetGrannyAnim:
      writer.addItem("file", asset.mAssetName);
      writer.addItem("weight", asset.mWeight);
      break;
   }


   // Tags
   for(long l=0; l<asset.mTags.getNumber(); l++)
   {
      saveTag(writer, asset.mTags[l]);
   }

   writer.endItem();
}


void saveLogicData(BXMLWriter &writer, const BProtoVisualLogicValue &logicValue);

void saveLogic(BXMLWriter &writer, const BProtoVisualLogicNode &logic)
{
   writer.startItem("logic");

   if((logic.mLogicType < 0) || (logic.mLogicType >= cVisualLogicMaxCount))
      writer.addAttribute("type", "unknown");
   else
      writer.addAttribute("type", gLogicTypeNames[logic.mLogicType]);

   for(long i=0; i<logic.mLogicValues.getNumber(); i++)
   {
      saveLogicData(writer, logic.mLogicValues[i]);
   }

   writer.endItem();
}

void saveLogicData(BXMLWriter &writer, const BProtoVisualLogicValue &logicValue)
{
   writer.startItem("logicdata");

   writer.addAttribute("value", logicValue.mValueName);
   if(!logicValue.mpModel->mRefModelName.isEmpty())
      writer.addAttribute("modelref", logicValue.mpModel->mRefModelName);

   if(!logicValue.mpModel->mAsset.mAssetName.isEmpty())
   {
      saveAsset(writer, logicValue.mpModel->mAsset);
   }
   if(logicValue.mpModel->mpLogicNode)
   {
      saveLogic(writer, *logicValue.mpModel->mpLogicNode);
   }

   writer.endItem();
}


void saveAnim(BXMLWriter &writer, const BProtoVisualModel &model, const BProtoVisualAnim &anim)
{
   // Early out if no assets and no attachments
   if((anim.mAssets.getNumber() == 0) && (anim.mAttachmentLinks.getNumber() == 0))
      return;

   writer.startItem("anim");

   if((anim.mAnimType < 0) || (anim.mAnimType >= cAnimTypeMaxCount))
      writer.addAttribute("type", "unknown");
   else
      writer.addAttribute("type", gAnimTypeNames[anim.mAnimType]);

   // Assets
   for(long k=0; k<anim.mAssets.getNumber(); k++)
   {
      saveAsset(writer, anim.mAssets[k]);
   }

   // Attachments
   for(long k=0; k<anim.mAttachmentLinks.getNumber(); k++)
   {
      saveAttachment(writer, model, anim.mAttachmentLinks[k]);
   }

   writer.endItem(); // end anim
}


//==============================================================================
// BProtoVisual::saveVIS
//==============================================================================
bool BProtoVisual::saveVIS()
{
   BString visName = mName;
   visName.removeExtension();
   visName.append(".vis");

   BXMLWriter writer;
   if(!writer.create(gVisualManager.getDirID(), visName.getPtr()))
      return false;

   // visual
   writer.startItem("visual");



   // Models
   bool wait = true;
   for(long i=0; i<mModels.getNumber(); i++)
   {
      BSimString name = mModels[i]->mModelName;
      wait = false;
   }

   // If last model in the file is not named then name it.
   BProtoVisualModel *pLastModel = mModels[mModels.getNumber()-1];
   if(pLastModel->mModelName.isEmpty())
   {
      pLastModel->mModelName = "Default";
   }

   // Default model.  Set the default model to be the last model in the file.
   writer.addAttribute("defaultmodel", mModels.getNumber() ? mModels[mModels.getNumber()-1]->mModelName : "");


   // Check if the first model just contains a logic node and no asset, and there
   // are more than just one model, in this case we are really dealing a visual logic.
   // (a top level logic node).
   bool bFirstNodeIsSeparateLogic = false;
   if(mModels.getNumber() > 1)
   {
      BProtoVisualModel *pFirstModel = mModels[0];
      if((pFirstModel->mpLogicNode != NULL) && (pFirstModel->mAsset.mAssetType == -1))
      {
         bFirstNodeIsSeparateLogic = true;
      }
   }

   // Models
   for(long i=bFirstNodeIsSeparateLogic ? 1 : 0; i<mModels.getNumber(); i++)
   {
      BProtoVisualModel *pModel = mModels[i];

      // Model
      writer.startItem("model");
      writer.addAttribute("name", pModel->mModelName);

      // Component
      writer.startItem("component");

      if(pModel->mAsset.mAssetType != -1)
      {
         saveAsset(writer, pModel->mAsset);
      }

      // Attachments
      for(long j=0; j<pModel->mAttachmentLinks.getNumber(); j++)
      {
         saveAttachment(writer, *pModel, pModel->mAttachmentLinks[j]);
      }

      // Points
      for(long j=0; j<pModel->mPoints.getNumber(); j++)
      {
         savePoint(writer, pModel->mPoints[j]);
      }

      // Logic
      if(pModel->mpLogicNode != NULL)
      {
         saveLogic(writer, *pModel->mpLogicNode);
      }

      writer.endItem(); // end component

      // Anims
      for(long j=0; j<pModel->mAnims.getNumber(); j++)
      {
         saveAnim(writer, *pModel, pModel->mAnims[j]);
      }

      writer.endItem(); // end model
   }

   // Separate logic
   if(bFirstNodeIsSeparateLogic)
   {
      saveLogic(writer, *mModels[0]->mpLogicNode);
   }

   writer.endItem(); // end visual


   return(true);
}

#endif

//==============================================================================
//==============================================================================
void BProtoVisual::getTechLogicIDs(BSmallDynamicSimArray<long>& techIDs)
{
   techIDs.clear();

   if (!getFlag(cFlagHasTechLogic))
      return;

   getTechLogicIDs(mpLogicNode, techIDs);

   for(long i=0; i<mModels.getNumber(); i++)
      getTechLogicIDs(mModels[i]->mpLogicNode, techIDs);
}

//==============================================================================
//==============================================================================
void BProtoVisual::getTechLogicIDs(BProtoVisualLogicNode* pLogicNode, BSmallDynamicSimArray<long>& techIDs)
{
   if(!pLogicNode)
      return;
   for(long i=0; i<pLogicNode->mLogicValues.getNumber(); i++)
   {
      BProtoVisualLogicValue& logicValue=pLogicNode->mLogicValues[i];
      if (pLogicNode->mLogicType==cVisualLogicTech)
      {
         if (logicValue.mValueDWORD!=(DWORD)-1)
            techIDs.uniqueAdd((long)logicValue.mValueDWORD);
      }
      if (logicValue.mpModel)
         getTechLogicIDs(logicValue.mpModel->mpLogicNode, techIDs);
   }
}


//==============================================================================
void BProtoVisual::getBuildingConstructionTags(BProtoVisualTagPtrArray& tags, int targetTag, long animType) const
{
   if (mpLogicNode)
   {
      getBuildingConstructionTags(tags, mpLogicNode,targetTag, animType);
   }
   else
   {
      if (mDefaultModelIndex != -1)
      {
         const BProtoVisualModel* pModel=mModels[mDefaultModelIndex];
         if (pModel)
            getBuildingConstructionTags(tags, pModel->mpLogicNode, targetTag, animType);
      }
   }
}

//==============================================================================
bool BProtoVisual::getBuildingConstructionTags(BProtoVisualTagPtrArray& tags, BProtoVisualLogicNode* pNode, int targetTag, long animType) const
{
   if(!pNode)
      return false;

   if (pNode->mLogicType==cVisualLogicBuildingCompletion)
   {
      for (int i=0; i<pNode->mLogicValues.getNumber(); i++)
      {
         const BProtoVisualLogicValue* pLogicValue=&(pNode->mLogicValues[i]);
         if (pLogicValue->mValueFloat < 1.0f)
         {
            const BProtoVisualModel* pModel=pLogicValue->mpModel;
            if (pModel->mRefModelIndex != -1)
               pModel=mModels[pModel->mRefModelIndex];
            if (pModel)
            {
               for (int j=0; j<pModel->mAnims.getNumber(); j++)
               {
                  const BProtoVisualAnim* pAnim=&(pModel->mAnims[j]);
                  if (pAnim->mAnimType==animType)
                  {
                     for (int k=0; k<pAnim->mAssets.getNumber(); k++)
                     {
                        const BProtoVisualAsset* pAsset=&(pAnim->mAssets[k]);
                        for (int m=0; m<pAsset->mTags.getNumber(); m++)
                        {
                           BProtoVisualTag* pTag=const_cast<BProtoVisualTag*>(&(pAsset->mTags[m]));
                           if (pTag->mEventType==targetTag)//cAnimEventAlphaTerrain
                              tags.add(pTag);
                        }
                     }
                  }
               }
            }
         }
      }
      return true;
   }
   else
   {
      for (int i=0; i<pNode->mLogicValues.getNumber(); i++)
      {
         const BProtoVisualModel* pModel=pNode->mLogicValues[i].mpModel;
         if (pModel && pModel->mpLogicNode)
         {
            if (getBuildingConstructionTags(tags, pNode,targetTag, animType))
               return true;
         }
      }
   }
   return false;
}

//==============================================================================
long BProtoVisual::getModelIndex(const BSimString& modelName) const
{
   // Look for the model and return the index
   for (long i = 0; i < mModels.getNumber(); i++)
   {
      if (mModels[i]->mModelName == modelName)
         return i;
   }

   return -1;
}

//==============================================================================
const BProtoVisualModel* BProtoVisual::getDefaultModel() const
{
   if (mDefaultModelIndex < 0 || mDefaultModelIndex >= mModels.getNumber())
      return NULL;
   return mModels[mDefaultModelIndex];
}

#ifndef BUILD_FINAL
   //==============================================================================
   // BProtoVisual::ensureUnloaded
   //==============================================================================
   void BProtoVisual::ensureUnloaded() const
   {
      for (uint i = 0; i < mModels.getSize(); i++)
      {
         BProtoVisualModel* pModel = mModels[i];
         if (pModel)
         {
            BASSERT(pModel->mAsset.mAssetIndex == -1);
            BASSERT(pModel->mAsset.mDamageAssetIndex == -1);

            for (uint j = 0; j < pModel->mAnims.getSize(); j++)
            {
               BProtoVisualAnim& anim = pModel->mAnims[j];

               for (uint k = 0; k < anim.mAssets.getSize(); k++)
               {
                  BProtoVisualAsset& animAsset = anim.mAssets[k];
                  animAsset;

                  BASSERT(animAsset.mAssetIndex == -1);
                  BASSERT(animAsset.mDamageAssetIndex == -1);
               }
            }
         }
      }

      if (mpLogicNode)
      {
         for (uint i = 0; i < mpLogicNode->mLogicValues.getSize(); i++)
         {
            BProtoVisualModel* pModel = mpLogicNode->mLogicValues[i].mpModel;
            pModel;

            BASSERT(pModel->mAsset.mAssetIndex == -1);
            BASSERT(pModel->mAsset.mDamageAssetIndex == -1);
         }
      }
   }

   //==============================================================================
   // BProtoVisual::getDamageTemplateCount
   //==============================================================================
   long BProtoVisual::getDamageTemplateCount() const
   {
      long count = 0;

      for (uint i = 0; i < mModels.getSize(); i++)
      {
         BProtoVisualModel* pModel = mModels[i];
         if (pModel)
         {
            if (pModel->mAsset.mDamageAssetIndex != -1)
               count++;

            for (uint j = 0; j < pModel->mAnims.getSize(); j++)
            {
               BProtoVisualAnim& anim = pModel->mAnims[j];

               for (uint k = 0; k < anim.mAssets.getSize(); k++)
               {
                  BProtoVisualAsset& animAsset = anim.mAssets[k];

                  if (animAsset.mDamageAssetIndex != -1)
                     count++;
               }
            }
         }
      }

      if (mpLogicNode)
      {
         for (uint i = 0; i < mpLogicNode->mLogicValues.getSize(); i++)
         {
            BProtoVisualModel* pModel = mpLogicNode->mLogicValues[i].mpModel;

            if (pModel->mAsset.mDamageAssetIndex != -1)
               count++;
         }
      }

      return count;
   }
#endif

//==============================================================================
//==============================================================================
bool BProtoVisual::findPoints(const BProtoVisualPointArray* pPoints, long &modelIndex) const
{
   modelIndex = -1;

   if (pPoints == NULL)
      return false;

   long numModels = mModels.getNumber();
   for (long i=0; i<numModels; i++)
   {
      const BProtoVisualModel* pModel = mModels[i];
      if (pModel->mPointsInitialized)
      {
         if (&(pModel->mPoints) == pPoints)
         {
            modelIndex = i;
            return true;
         }
      }
   }
   return false;
}

//==============================================================================
//==============================================================================
bool BProtoVisual::findTags(const BProtoVisualTagArray* pTags, long &modelIndex, long &animIndex, long& assetIndex) const
{
   modelIndex = -1;
   animIndex = -1;
   assetIndex = -1;

   if (pTags == NULL)
      return false;

   long numModels = mModels.getNumber();

   for (long i=0; i<numModels; i++)
   {
      const BProtoVisualModel* pModel = mModels[i];
      long numAnims = pModel->mAnims.getNumber();
      for (long j=0; j<numAnims; j++)
      {
         const BProtoVisualAnim& anim = pModel->mAnims[j];
         long numAssets = anim.mAssets.getNumber();
         for (long k=0; k<numAssets; k++)
         {
            const BProtoVisualAsset& asset = anim.mAssets[k];
            if (&(asset.mTags) == pTags)
            {
               modelIndex = i;
               animIndex = j;
               assetIndex = k;
               return true;
            }
         }
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BProtoVisual::findProgression(const BFloatProgression* pProgressions, long &modelIndex, long &animIndex, long& assetIndex) const
{
   modelIndex = -1;
   animIndex = -1;
   assetIndex = -1;

   if (pProgressions == NULL)
      return false;

   long numModels = mModels.getNumber();

   for (long i=0; i<numModels; i++)
   {
      const BProtoVisualModel* pModel = mModels[i];
      long numAnims = pModel->mAnims.getNumber();
      for (long j=0; j<numAnims; j++)
      {
         const BProtoVisualAnim& anim = pModel->mAnims[j];
         long numAssets = anim.mAssets.getNumber();
         for (long k=0; k<numAssets; k++)
         {
            const BProtoVisualAsset& asset = anim.mAssets[k];
            if (asset.mpOpacityProgression == pProgressions)
            {
               modelIndex = i;
               animIndex = j;
               assetIndex = k;
               return true;
            }
         }
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
BProtoVisualPointArray* BProtoVisual::getPoints(long modelIndex) const
{
   if (modelIndex >= 0 && modelIndex < mModels.getNumber())
   {
      BProtoVisualModel* pModel = mModels[modelIndex];
      return &(pModel->mPoints);
   }
   return NULL;
}

//==============================================================================
//==============================================================================
BProtoVisualTagArray* BProtoVisual::getTags(long modelIndex, long animIndex, long assetIndex) const
{
   if (modelIndex >= 0 && modelIndex < mModels.getNumber())
   {
      BProtoVisualModel* pModel = mModels[modelIndex];
      if (animIndex >= 0 && animIndex < pModel->mAnims.getNumber())
      {
         BProtoVisualAnim& anim = pModel->mAnims[animIndex];
         if (assetIndex >= 0 && assetIndex < anim.mAssets.getNumber())
            return &(anim.mAssets[assetIndex].mTags);
      }
   }
   return NULL;
}

//==============================================================================
//==============================================================================
BFloatProgression* BProtoVisual::getProgression(long modelIndex, long animIndex, long assetIndex) const
{
   if (modelIndex >= 0 && modelIndex < mModels.getNumber())
   {
      BProtoVisualModel* pModel = mModels[modelIndex];
      if (animIndex >= 0 && animIndex < pModel->mAnims.getNumber())
      {
         BProtoVisualAnim& anim = pModel->mAnims[animIndex];
         if (assetIndex >= 0 && assetIndex < anim.mAssets.getNumber())
            return anim.mAssets[assetIndex].mpOpacityProgression;
      }
   }
   return NULL;
}
