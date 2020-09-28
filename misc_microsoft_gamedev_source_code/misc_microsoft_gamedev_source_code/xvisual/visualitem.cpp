//==============================================================================
// visualitem.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "visualitem.h"
#include "boundingBox.h"
#include "grannymanager.h"
#include "grannyinstance.h"
#include "lightEffectManager.h"
#include "particlegateway.h"
#include "protovisual.h"
#include "visual.h"
#include "visualinstance.h"
#include "visualmanager.h"
#include "grannymodel.h"
#include "econfigenum.h"
#include "rendertime.h"

// xcore
#include "math\fastFloat.h"

//xgame
#include "..\xgame\terrainsimrep.h"
#include "..\xgame\terraineffect.h"
#include "..\xgame\terraineffectmanager.h"
#include "..\xgame\database.h"
#include "..\xgame\object.h"
#include "..\xgame\syncmacros.h"
#include "..\xgame\savegame.h"


#define FAST_UPDATE_TRANSFORMS

// Defines
//#define DEBUG_RENDER_BOUNDING_BOX
#define DEBUG_INIT_FROM_SOURCE
#define DEBUG_COPY_ANIMATION_TRACK
#define DEBUG_INTRUSIVE_SYNCING

const uint cMaxAttachmentCount = 96;

// jce [10/30/2008] -- yes, this is evil but probably not as evil as adding another upward header dependency
extern bool gEnableSubUpdating;


const float cAirborneHeightAboveGround = 0.1f;

BVisualItem_updateSyncCallback BVisualItem::sUpdateSyncCallback = BVisualItem::defaultUpdateSyncCallback;

GFIMPLEMENTVERSION(BVisualItem, 3);
GFIMPLEMENTVERSION(BVisualAnimationData, 2);

//==============================================================================
//==============================================================================
IMPLEMENT_THREADSAFE_FREELIST(BVisualItem, 10, &gSimHeap);

//==============================================================================
// BVisualItem::BVisualItem
//==============================================================================
BVisualItem::BVisualItem()
{ 
}

//==============================================================================
// BVisualItem::~BVisualItem
//==============================================================================
BVisualItem::~BVisualItem()
{   
}

//==============================================================================
//==============================================================================
BVisualItem* BVisualItem::getNewInstance(bool isUserAttachment) const
{
   BVisualItem* pNewInstance = BVisualItem::getInstance();

   if (pNewInstance)
   {
      pNewInstance->setFlag(cFlagUser, isUserAttachment);
      pNewInstance->mFlagObjectVisible = mFlagObjectVisible;

      if (!isUserAttachment)
      {
         pNewInstance->setFlag(cFlagSynced, getFlag(cFlagSynced));
         pNewInstance->setDisplayPriority(mDisplayPriority);
      }
   }

   return pNewInstance;
}

//==============================================================================
//==============================================================================
void BVisualItem::onAcquire()
{ 
   mpInstance = NULL;
   mMatrix.makeIdentity();
   mTransform.makeIdentity();
   mOldTransform.makeIdentity();
   mCombinedMinCorner = cOriginVector;  
   mCombinedMaxCorner = cOriginVector;
   mMinCorner = cOriginVector;
   mMaxCorner = cOriginVector;
   mModelAsset.mIndex = -1;
   mModelAsset.mType = -1;
   mModelUVOffsets.clear();
   mFlags.setAll(0);
   mAttachments.clear();
   mFromBoneHandle = -1;
   mToBoneHandle = -1;
   mAttachmentHandle = -1;
   mAttachmentType = -1;
   mIndex = -1;
   mpName = NULL;
   mLifespan = 0.0f;
   mModelVariationIndex = -1;
   mDamageTemplateIndex = -1;
   for (long track = 0; track < cNumAnimationTracks; track++)
      mAnimationTrack[track].init();
   setFlag(cFlagActive, true);
   setFlag(cFlagVisible, true);
   mFlagObjectVisible=false;
   mDisplayPriority=cVisualDisplayPriorityNormal;
   mSubUpdateNumber = 0;   
   mGrannySubUpdateNumber = 0;
   mData = -1;
}

//==============================================================================
//==============================================================================
void BVisualItem::onRelease()
{ 
   deinit();
}

//==============================================================================
// BVisualItem::init
//==============================================================================
bool BVisualItem::init(BProtoVisual* pProtoVisual, int64 userData, bool synced, DWORD tintColor, const BMatrix& worldMatrix, int displayPriority)
{
   deinit();

   setFlag(cFlagSynced, synced);
   mDisplayPriority=displayPriority;

   setAnimation(0, pProtoVisual, userData, cAnimTypeIdle, true, 0.0f, tintColor, worldMatrix);
   for (long track = 1; track < cNumAnimationTracks; track++)
      copyAnimationTrack(0, track, pProtoVisual, userData, true, tintColor, worldMatrix);

   validateAnimationTracks();

   return true;
}

//==============================================================================
// BVisualItem::clone
//==============================================================================
bool BVisualItem::clone(BProtoVisual* pProtoVisual, int64 userData, const BVisualItem* pSource, bool synced, bool bDisregardAttachments, DWORD tintColor, const BMatrix& worldMatrix)
{
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::clone userData", (DWORD)userData);
         syncAnimData("BVisualItem::clone bDisregardAttachments", bDisregardAttachments);
      }
   #endif

   deinit();

   mFlags=pSource->mFlags;

   setFlag(cFlagSynced, synced);

   // Copy some data from the source
   mCombinedMinCorner=pSource->mCombinedMinCorner;
   mCombinedMaxCorner=pSource->mCombinedMaxCorner;
   mMinCorner=pSource->mMinCorner;
   mMaxCorner=pSource->mMaxCorner;
   mModelAsset.mIndex = -1;
   mModelAsset.mType = -1;
   mModelUVOffsets.clear();
   mFromBoneHandle=pSource->mFromBoneHandle;
   mToBoneHandle=pSource->mToBoneHandle;
   mpName=pSource->mpName;         
   mAttachmentHandle=pSource->mAttachmentHandle;
   mAttachmentType=pSource->mAttachmentType;
   mIndex=pSource->mIndex;
   mMatrix=pSource->mMatrix;
   mTransform=pSource->mTransform;
   mOldTransform=pSource->mOldTransform;
   mDisplayPriority=pSource->mDisplayPriority;
   mModelVariationIndex=pSource->mModelVariationIndex;
   mDamageTemplateIndex=pSource->mDamageTemplateIndex;

   #ifdef SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::clone mMinCorner", mMinCorner);
         syncAnimData("BVisualItem::clone mMaxCorner", mMaxCorner);
      }
   #endif

   // Any clones?
   long clone = -1;
   for (long track = 0; track < cNumAnimationTracks; track++)
   {
      if (pSource->mAnimationTrack[track].mIsClone)
      {
         clone = track;
         break;
      }
   }

   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::clone clone", clone);
      }
   #endif

   // Yes. Set animation of master than copy it to the clone.
   if (clone != -1)
   {
      long master = 1 - clone;

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::clone master", master);
         }
      #endif

      BVisualAnimationData &animation = mAnimationTrack[master];
      const BVisualAnimationData &sourceAnimation = pSource->mAnimationTrack[master];

      // Resume from the source's position
      float timeIntoAnimation = (sourceAnimation.mDuration > cFloatCompareEpsilon) ? (sourceAnimation.mPosition / sourceAnimation.mDuration) : 0.0f;

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::clone timeIntoAnimation", timeIntoAnimation);
         }
      #endif

      // set animation on the master track
      animation.init();
      animation.mExitAction = sourceAnimation.mExitAction;
      animation.mTweenToAnimation = sourceAnimation.mTweenToAnimation;
      animation.mTweenTime = sourceAnimation.mTweenTime;

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::clone animation.mExitAction", animation.mExitAction);
            syncAnimData("BVisualItem::clone animation.mTweenTime", animation.mTweenTime);
         }
      #endif

      updateState(master, sourceAnimation.mAnimType, synced, pSource->mModelAsset.mType, pSource->mModelAsset.mIndex, pSource->mModelUVOffsets, 
         sourceAnimation.mAnimAsset.mType, sourceAnimation.mAnimAsset.mIndex, sourceAnimation.mpTags, sourceAnimation.mpPoints, sourceAnimation.mpOpacityProgression,
         pSource->mMinCorner, pSource->mMaxCorner, true, timeIntoAnimation, 0.0f, pSource->mDamageTemplateIndex, false, NULL, tintColor, worldMatrix);

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::clone mAnimationTrack[master].mIsDirty", mAnimationTrack[master].mIsDirty);
            syncAnimData("BVisualItem::clone userData", (DWORD)userData);
         }
      #endif

      // Copy flag
      mAnimationTrack[master].mIsDirty = pSource->mAnimationTrack[master].mIsDirty;

      // Copy from master to clone. No recursion.
      copyAnimationTrack(master, clone, pProtoVisual, userData, true, tintColor, worldMatrix, NULL, true);

      // Copy flag
      mAnimationTrack[clone].mIsDirty = pSource->mAnimationTrack[clone].mIsDirty;
   }
   // No. Just set animations.
   else
   {
      for (long track = 0; track < cNumAnimationTracks; track++)
      {
         BVisualAnimationData &animation = mAnimationTrack[track];
         const BVisualAnimationData &sourceAnimation = pSource->mAnimationTrack[track];

         // Resume from the source's position
         float timeIntoAnimation = (sourceAnimation.mDuration > cFloatCompareEpsilon) ? (sourceAnimation.mPosition / sourceAnimation.mDuration) : 0.0f;

         // Set animation
         animation.init();
         animation.mExitAction = sourceAnimation.mExitAction;
         animation.mTweenToAnimation = sourceAnimation.mTweenToAnimation;
         animation.mTweenTime = sourceAnimation.mTweenTime;
         updateState(track, sourceAnimation.mAnimType, synced, pSource->mModelAsset.mType, pSource->mModelAsset.mIndex, pSource->mModelUVOffsets, 
            sourceAnimation.mAnimAsset.mType, sourceAnimation.mAnimAsset.mIndex, sourceAnimation.mpTags, sourceAnimation.mpPoints, sourceAnimation.mpOpacityProgression,
            pSource->mMinCorner, pSource->mMaxCorner, true, timeIntoAnimation, 0.0f, pSource->mDamageTemplateIndex, false, NULL, tintColor, worldMatrix);

         // Copy flag
         mAnimationTrack[track].mIsDirty = pSource->mAnimationTrack[track].mIsDirty;
      }
   }

   validateAnimationTracks();

   // Do we care about attachments?
   if(!bDisregardAttachments)
   {
      // Copy over source attachments
      long count=pSource->mAttachments.getNumber();
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=BVisualItem::getInstance();
         if(!pAttachment)
            break;
         long index=mAttachments.add(pAttachment);
         if(index==-1)
         {
            BASSERTM(0, "BVisualItem::clone - Failed to add attachment.");
            BVisualItem::releaseInstance(pAttachment);
            break;
         }

         // Init new attachment from the source attachment (recursive)
         pAttachment->clone(pProtoVisual, userData, pSource->mAttachments[i], synced, bDisregardAttachments, tintColor, worldMatrix);
      }
   }

//    // Set proper sync flag
//    if (!pSource->getFlag(cFlagSynced))
//       synced = false;
//    setFlag(cFlagSynced, synced);

   // if this is a granny asset, copy over the mask 
   if (pSource->mModelAsset.mType == cVisualAssetGrannyModel)
   {
      BGrannyInstance* pGrannyInstance = (BGrannyInstance*)mpInstance;
//-- FIXING PREFIX BUG ID 7361
      const BGrannyInstance* pSourceGrannyInstnace = (BGrannyInstance*)pSource->mpInstance;
//--
      if (pGrannyInstance && pSourceGrannyInstnace)
         pGrannyInstance->setMeshRenderMask(pSourceGrannyInstnace->getMeshRenderMask());
   }

   #ifdef DEBUG_INIT_FROM_SOURCE
      BASSERT(mMatrix.getRow(0).almostEqual(pSource->mMatrix.getRow(0)));
      BASSERT(mMatrix.getRow(1).almostEqual(pSource->mMatrix.getRow(1)));
      BASSERT(mMatrix.getRow(2).almostEqual(pSource->mMatrix.getRow(2)));
      BASSERT(mMatrix.getRow(3).almostEqual(pSource->mMatrix.getRow(3)));
      BASSERT(mTransform.getRow(0).almostEqual(pSource->mTransform.getRow(0)));
      BASSERT(mTransform.getRow(1).almostEqual(pSource->mTransform.getRow(1)));
      BASSERT(mTransform.getRow(2).almostEqual(pSource->mTransform.getRow(2)));
      BASSERT(mTransform.getRow(3).almostEqual(pSource->mTransform.getRow(3)));
      if (synced)
      {
         BASSERT(mCombinedMinCorner.almostEqual(pSource->mCombinedMinCorner));
         BASSERT(mCombinedMaxCorner.almostEqual(pSource->mCombinedMaxCorner));
      }
      BASSERT(mMinCorner.almostEqual(pSource->mMinCorner));
      BASSERT(mMaxCorner.almostEqual(pSource->mMaxCorner));
      BASSERT(mModelAsset.mIndex == pSource->mModelAsset.mIndex);
      BASSERT(mModelAsset.mType == pSource->mModelAsset.mType);
      if (synced)
      {
         BASSERT(bDisregardAttachments || (mAttachments.getNumber() == pSource->mAttachments.getNumber()));
      }
      BASSERT(mFromBoneHandle == pSource->mFromBoneHandle);
      BASSERT(mToBoneHandle == pSource->mToBoneHandle);
      BASSERT(mAttachmentType == pSource->mAttachmentType);
      BASSERT(mModelVariationIndex == pSource->mModelVariationIndex);
      BASSERT(mDamageTemplateIndex == pSource->mDamageTemplateIndex);
      BASSERT(mDisplayPriority == pSource->mDisplayPriority);
   #endif

   return true;
}

//==============================================================================
// BVisualItem::deinit
//==============================================================================
void BVisualItem::deinit()
{
   releaseInstance(false);

   long count=mAttachments.getNumber();
   for(long i=0; i<count; i++)
      BVisualItem::releaseInstance(mAttachments[i]);
   mAttachments.clear();

   for (long track = 0; track < cNumAnimationTracks; track++)
      mAnimationTrack[track].deinit();
}

//==============================================================================
// BVisualItem::createInstance
//==============================================================================
void BVisualItem::createInstance(const BMatrix& worldMatrix, DWORD tintColor)
{
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimCode("BVisualItem::createInstance");
         syncAnimData("BVisualItem::createInstance mModelAsset.mType", mModelAsset.mType);
         syncAnimData("BVisualItem::createInstance mModelAsset.mIndex == -1", (mModelAsset.mIndex == -1));
      }
   #endif

   if(mModelAsset.mIndex==-1)
      return;


   switch(mModelAsset.mType)
   {
      case cVisualAssetGrannyModel:
      {
         BGrannyInstance* pGrannyInstance = gGrannyManager.createInstance();
         #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
            if (getFlag(cFlagSynced))
            {
               syncAnimData("BVisualItem::createInstance pGrannyInstance != NULL", (pGrannyInstance != NULL));
            }
         #endif

         if(!pGrannyInstance)
            return;
         if(pGrannyInstance->init(mModelAsset.mIndex, &mModelUVOffsets))
         {
            #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
               if (getFlag(cFlagSynced))
               {
                  syncAnimCode("BVisualItem::createInstance granny instance inited");
               }
            #endif

            mpInstance=pGrannyInstance;
         }
         else
            gGrannyManager.releaseInstance(pGrannyInstance);
         break;
      }
      case cVisualAssetParticleSystem:
      {
         if (!gConfig.isDefined(cConfigNoParticles))
         {
            BParticleCreateParams params;
            params.mDataHandle        = (BParticleEffectDataHandle)mModelAsset.mIndex;
            params.mMatrix            = worldMatrix;
            params.mNearLayerEffect   = false;
            params.mPriority          = mDisplayPriority;
            params.mTintColor         = tintColor;

            BParticleInstance* pParticleInstance = gParticleGateway.createInstance(params);
            mpInstance = pParticleInstance;
         }
         break;
      }
      case cVisualAssetLight:
      {
         BLightEffectVisualInstance* pLightEffectInstance = gLightEffectManager.createInstance(mModelAsset.mIndex, worldMatrix);
         mpInstance = pLightEffectInstance;
         break;
      }
      case cVisualAssetTerrainEffect:
      {
         BParticleEffectDataHandle pfxHandle = -1;

         BVector particlePos;
         worldMatrix.getTranslation(particlePos);


         // TODO: This needs to be optimized for units that will always be on the ground (tanks) so that the airbone
         // state isn't tested at all.  (SAT)

         // Check if the unit is airborne
         bool bIsAirborne = false;

         // Disable this for now, since warthogs dont' jump anyway
         //
         /*
         float heightAboveGround;
         gTerrainSimRep.getHeightRaycast(particlePos, heightAboveGround, true);
         heightAboveGround = particlePos.y - heightAboveGround;

         if(heightAboveGround > cAirborneHeightAboveGround)
            bIsAirborne = true;
         */


         if(!bIsAirborne)
         {
            // If not airborne check the terrain type that we are in

            // Get terrain type
            BYTE surfaceType = gTerrainSimRep.getTileType(particlePos);

            BTerrainEffect *pTerrainEffect = gTerrainEffectManager.getTerrainEffect(mModelAsset.mIndex, true);

            if(pTerrainEffect)
            {
               pfxHandle = pTerrainEffect->getParticleEffectHandleForType(surfaceType, -1);
            }
         }
         else
         {
            // If airborne then no pfx
            pfxHandle = -1;
         }

         if(pfxHandle != -1)
         {
            BParticleCreateParams params;
            params.mDataHandle        = pfxHandle;
            params.mMatrix            = worldMatrix;
            params.mNearLayerEffect   = false;
            params.mPriority          = mDisplayPriority;
            params.mTintColor         = tintColor;

            BParticleInstance* pParticleInstance = gParticleGateway.createInstance(params);
            mpInstance = pParticleInstance;
         }
         break;
      }
   }

   if (mpInstance)
      mpInstance->setVisibility(mFlagObjectVisible && getFlag(cFlagVisible));
}

//==============================================================================
// BVisualItem::releaseInstance
//==============================================================================
void BVisualItem::releaseInstance(bool fullRelease)
{
   if(mpInstance)
   {
      switch(mModelAsset.mType)
      {
         case cVisualAssetGrannyModel:
         {
            BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
            gGrannyManager.releaseInstance(pGrannyInstance);
            mpInstance=NULL;
            break;
         }
         case cVisualAssetParticleSystem:
         {
            BParticleInstance* pParticleInstance = (BParticleInstance*)mpInstance;
            if (getFlag(cFlagImmediateRemove))
               gParticleGateway.releaseInstance(pParticleInstance, true, false);
            else
               gParticleGateway.releaseInstance(pParticleInstance, false);
            mpInstance=NULL;
            break;
         }
         case cVisualAssetLight:
         {
            BLightEffectVisualInstance* pLightEffectInstance = (BLightEffectVisualInstance*)mpInstance;
            gLightEffectManager.releaseInstance(pLightEffectInstance);
            mpInstance=NULL;
            break;
         }
         case cVisualAssetTerrainEffect:
         {
            BParticleInstance* pParticleInstance = (BParticleInstance*)mpInstance;
            gParticleGateway.releaseInstance(pParticleInstance, false);
            mpInstance=NULL;
            break;
         }
      }
   }
   if(fullRelease)
   {
      mModelAsset.mType=-1;
      mModelAsset.mIndex=-1;
      mModelUVOffsets.clear();
// SLB: This breaks stuff so I'm commenting it out
//      for (long track = 0; track < cNumAnimationTracks; track++)
//         mAnimationTrack[track].deinit();
   }
}

//==============================================================================
void BVisualItem::updatePreAsync(float elapsedTime, DWORD subUpdate, bool animationEnabled)
{   
   SCOPEDSAMPLE(BVisualItem_updatePreAsync);
   
   if(animationEnabled && mpInstance)
   {
      if (mModelAsset.mType != cVisualAssetLight) // this is now handled every frame in ::render
      {
         if (!getFlag(cFlagAnimationDisabled) || (mModelAsset.mType != cVisualAssetGrannyModel))
         {
            mpInstance->update(elapsedTime, getFlag(cFlagSynced));
         }
      }
   }

   // Update the attachments
   long count=mAttachments.getNumber();
   if(count>0)
   {
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];

         if(!pAttachment->getFlag(cFlagUsed))
         {
            BVisualItem::releaseInstance(pAttachment);
            mAttachments.removeIndex(i, false);
            i--;
            count--;
            continue;
         }

         //-- is this a timed attachment
         if(pAttachment->getFlag(cFlagUseLifespan))
         {
            if(pAttachment->getFlag(cFlagJustCreated))
               pAttachment->setFlag(cFlagJustCreated, false);
            else
            {
               pAttachment->mLifespan -= elapsedTime;

               //-- did we die?  then mark the attachment for deletion
               if (pAttachment->mLifespan <= 0.0f)
               {
                  BVisualItem::releaseInstance(pAttachment);
                  mAttachments.removeIndex(i, false);
                  i--;
                  count--;
                  continue;
               }
            }
         }

         pAttachment->updatePreAsync(elapsedTime, subUpdate, animationEnabled);
      }
   }   

   mGrannySubUpdateNumber = subUpdate;       
}

//==============================================================================
// BVisualItem::updateAsync
//==============================================================================
void BVisualItem::updateAsync(float elapsedTime, bool root, DWORD subUpdate)
{   
   SCOPEDSAMPLE(BVisualItem_updateAsync);

#ifndef FAST_UPDATE_TRANSFORMS

   // jce [10/22/2008] -- Moving this block before the attachment update so that we're set up right
   // before attachments do their thing.
   if (subUpdate > 0) // means we're sub-updating
   {
      if (mpInstance && !getFlag(cFlagAnimationDisabled) && mModelAsset.mType==cVisualAssetGrannyModel)
      {
         BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
         pGrannyInstance->updateInterpolatedClockValues();
      }
   }

   // Update the attachments
   long count=mAttachments.getNumber();
   if(count>0)
   {
      //updatePreAsync ensures that this list should have only VALID references

      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         pAttachment->updateAsync(elapsedTime, false, subUpdate);
      }

      updateAttachmentTransforms();
   }

   // Recompute the bounding box
   if (root)
      computeCombinedBoundingBox();   

#else

   updateAsyncRecurse(elapsedTime, subUpdate);

   BSparsePoseHelper parentSP;
   updateTransformsRecurse(parentSP);

   // Recompute the bounding box
   computeCombinedBoundingBox();

#endif
}

//==============================================================================
// BVisualItem::updateAsyncRecurse
//==============================================================================
void BVisualItem::updateAsyncRecurse(float elapsedTime, DWORD subUpdate)
{
   SCOPEDSAMPLE(BVisualItem_updateAsync);

   // jce [10/22/2008] -- Moving this block before the attachment update so that we're set up right
   // before attachments do their thing.
   if (subUpdate > 0) // means we're sub-updating
   {
      if (mpInstance && !getFlag(cFlagAnimationDisabled) && mModelAsset.mType==cVisualAssetGrannyModel)
      {
         BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
         pGrannyInstance->updateInterpolatedClockValues();
      }
   }

   // Update the attachments
   long count=mAttachments.getNumber();
   if(count>0)
   {
      //updatePreAsync ensures that this list should have only VALID references

      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         pAttachment->updateAsyncRecurse(elapsedTime, subUpdate);
      }
   }
}
 

//==============================================================================
// BVisualItem::resetInterpolatedClockValues
//==============================================================================
void BVisualItem::resetInterpolatedClockValues()
{
   long numAttachments = mAttachments.getNumber();
   for (long i = 0; i < numAttachments; i++)
      mAttachments[i]->resetInterpolatedClockValues();

   if (mpInstance && mModelAsset.mType==cVisualAssetGrannyModel)
   {
      BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
      pGrannyInstance->resetInterpolatedClockValues();
   }
}

//==============================================================================
// BVisualItem::validateAnimationTracks
//==============================================================================
void BVisualItem::validateAnimationTracks() const
{
   // SLB: Lets make sure the clone flag is set if we have an animation mimicking another
   BASSERT((mAnimationTrack[0].mAnimType == -1) || (mAnimationTrack[0].mAnimType != mAnimationTrack[1].mAnimType) || mAnimationTrack[0].mIsClone || mAnimationTrack[1].mIsClone);
}

//==============================================================================
// BVisualItem::update
//==============================================================================
void BVisualItem::update(float elapsedTime, BVisual* pVisual, long attachmentHandle, bool sendLoopEvent, DWORD tintColor, const BMatrix& worldMatrix, DWORD subUpdate)
{
   //SCOPEDSAMPLE(BVisualItem_update);      
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(BVisualItem::cFlagSynced))
      {
      	
	      if (!BProtoVisual::mGenerationChanged)
	      {
            const char* pName = gVisualManager.getAttachmentName(attachmentHandle);

            if (pName)
               syncAnimData("BVisualItem::update attachmentName", pName);
         }
         syncAnimData("BVisualItem::update elapsedTime", elapsedTime);
         syncAnimData("BVisualItem::update sendLoopEvent", sendLoopEvent);
         syncAnimData("BVisualItem::update subUpdate", subUpdate);
         syncAnimData("BVisualItem::update getFlag(cFlagAnimationDisabled)", getFlag(cFlagAnimationDisabled));
      }
   #endif

   mSubUpdateNumber = subUpdate;

   if (getFlag(cFlagAnimationDisabled))
      return;

#ifdef SYNC_Anim
   if (getFlag(BVisualItem::cFlagSynced))
   {
      // SLB: Don't dereference mpName on the same frame that it gets reloaded.
      if (!BProtoVisual::mGenerationChanged && mpName)
      {
         syncAnimData("BVisualItem::update visName", *(mpName));
      }
      else if (pVisual->getProtoVisual())
      {
         syncAnimData("BVisualItem::update visName", pVisual->getProtoVisual()->getName());
      }
   }
#endif

   // SLB: Lets make sure we don't fire off the same tag multiple times
   BASSERT(!mAnimationTrack[0].mpTags || (mAnimationTrack[0].mpTags != mAnimationTrack[1].mpTags));
   validateAnimationTracks();

   float timeIntoChainedAnimation[cNumAnimationTracks];
   bool chain[cNumAnimationTracks];
   //if (mModelAsset.mType == cVisualAssetGrannyModel)
   {
     //SCOPEDSAMPLE(BVisualItem_update_tags);

      // Update position and fire tags
      for (long track = 0; track < cNumAnimationTracks; track++)
         mAnimationTrack[track].update(track, this, elapsedTime, pVisual, attachmentHandle, sendLoopEvent, chain[track], timeIntoChainedAnimation[track], tintColor, worldMatrix);
   }

   updateTerrainEffects(worldMatrix, tintColor);
   
   long count=mAttachments.getNumber();
   if(count>0)
   {
#ifdef SYNC_Anim
      if (getFlag(BVisualItem::cFlagSynced))
      {
         syncAnimData("BVisualItem::update num attachments", count);
      }
#endif

      //SCOPEDSAMPLE(BVisualItem_update_Attachments);
      //updatePreAsync ensures that this list should have only VALID refrences

      BMatrix finalMatrix;
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         if (pAttachment->getFlag(cFlagUsed))
         {
            finalMatrix.mult(pAttachment->mMatrix, worldMatrix);
            pAttachment->update(elapsedTime, pVisual, pAttachment->mAttachmentHandle, false, tintColor, finalMatrix, subUpdate);
         }
      }
   }


   //if (mModelAsset.mType == cVisualAssetGrannyModel)
   {
      //SCOPEDSAMPLE(BVisualItem_update_nextAnim);
      // Play next animation in the chain
      if ((chain[0] || chain[1]) && (attachmentHandle == -1))
      {
         // Any clones?
         long clone = -1;
         for (long track = 0; track < cNumAnimationTracks; track++)
         {
            if (mAnimationTrack[track].mIsClone)
            {
               clone = track;
               break;
            }
         }

         // Are we going to become a clone?
         if (clone == -1)
         {
            if (chain[0] && (mAnimationTrack[0].mTweenToAnimation == mAnimationTrack[1].mAnimType))
            {
               clone = 0;
            }
            else if (chain[1] && (mAnimationTrack[1].mTweenToAnimation == mAnimationTrack[0].mAnimType))
            {
               clone = 1;
            }
         }

         // Yes. Set animation of master than copy it to the clone.
         if (clone != -1)
         {
            long master = 1 - clone;
            if (chain[master] && !mAnimationTrack[master].mIsDirty)
            {
               setAnimation(master, pVisual->getProtoVisual(), pVisual->getUserData(), mAnimationTrack[master].mTweenToAnimation, false, timeIntoChainedAnimation[master], tintColor, worldMatrix, -1, true);
               copyAnimationTrack(master, clone, pVisual->getProtoVisual(), pVisual->getUserData(), false, tintColor, worldMatrix);
            }
         }
         // No. Just chain what needs to be chained.
         else
         {
            for (long track = 0; track < cNumAnimationTracks; track++)
            {
               if (chain[track] && (attachmentHandle == -1) && !mAnimationTrack[track].mIsDirty)
                  setAnimation(track, pVisual->getProtoVisual(), pVisual->getUserData(), mAnimationTrack[track].mTweenToAnimation, false, timeIntoChainedAnimation[track], tintColor, worldMatrix, -1, true);
            }
         }

         validateAnimationTracks();
      }
   }
}


//==============================================================================
//==============================================================================
void BVisualItem::updateDone()
{
   // Update interpolation history.
   if(!getFlag(cFlagTransformUpdated))
      mOldTransform = mTransform;
   else
      setFlag(cFlagTransformUpdated, false);
   
   // Children
   for(long i=mAttachments.getNumber()-1; i>=0; i--)
      mAttachments[i]->updateDone();
}



//==============================================================================
// BVisualItem::updateGrannySync
//
// ajl 7/25/07 - Attempt to fix an OOS that seems to be caused by the way Granny keeps track
// of time internally. It's affected by the rendering of a unit... If a unit isn't rendered,
// then it is in a different state than if it was rendered, and that can cause a slight
// OOS when a bone position is looked up.
//==============================================================================
void BVisualItem::updateGrannySync(bool doSyncChecks)
{
   // SLB: Only do this for synced visual items
   if (getFlag(cFlagSynced))
   {
      sUpdateSyncCallback(this, true, doSyncChecks);

      if (mpInstance && mModelAsset.mType==cVisualAssetGrannyModel)
      {
         BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
         pGrannyInstance->updateGrannySync(doSyncChecks);
      }

      int count=mAttachments.getNumber();
      for (int i=0; i<count; i++)
      {
         BVisualItem& attachment=*(mAttachments[i]);
         if (attachment.getFlag(cFlagUsed))
            attachment.updateGrannySync(doSyncChecks);
      }
   }
}

//==============================================================================
// BVisualItem::updateAttachmentTransforms
//==============================================================================
void BVisualItem::updateAttachmentTransforms(bool init)
{
   //SCOPEDSAMPLE(BVisualItem_updateAttachmentTransforms)
   long count=mAttachments.getNumber();
   if(count>0)
   {
      for(long i=0; i<count; i++)
      {
         BVisualItem& attachment=*(mAttachments[i]);
         if (attachment.getFlag(cFlagUsed))
         {
            attachment.updateTransform(mpInstance);
         }
      }
   }
}

//==============================================================================
// BVisualItem::updateTransformsRecurse
//==============================================================================
void BVisualItem::updateTransformsRecurse(BSparsePoseHelper parentSP, long attachmentLevel)
{
   BSparsePoseHelper curSP;

   long count = mAttachments.getNumber();

   // Create a new sparse pose for all bones that we are going to query
   //
   if (((count>0) || (mFromBoneHandle != -1)) && 
      (mpInstance && mModelAsset.mType == cVisualAssetGrannyModel))
   {
      BGrannyInstance* pGrannyInstance = (BGrannyInstance*)mpInstance;
      granny_model_instance* pModelInstance = pGrannyInstance->getModelInstance();
      const granny_skeleton* pSkeleton = GrannyGetSourceSkeleton(pModelInstance);

      // Build a unique list of bone handles for our attachments
      const long cMaxAttachmentBones = 64;
      long boneHandles[cMaxAttachmentBones];
      long boneCount = 0;


      if(mFromBoneHandle != -1)
      {
         long boneHandle = BONEFROMGRANNYBONEHANDLE(mFromBoneHandle);
         if(boneHandle < pSkeleton->BoneCount)
         {
            boneHandles[boneCount] = boneHandle;
            boneCount++;
         }
      }
      
      for(long i=0; i<count; i++)
      {
         const BVisualItem* pAttachment=mAttachments[i];
         if (pAttachment->getFlag(cFlagUsed))
         {
            if (pAttachment->mToBoneHandle != -1)
            {
               long boneHandle = BONEFROMGRANNYBONEHANDLE(pAttachment->mToBoneHandle);

               if(boneHandle >= pSkeleton->BoneCount)
                  continue;

               for (long j=0; j<boneCount; j++)
               {
                  if (boneHandles[j] == boneHandle)   
                     break;
               }
               if (j == boneCount)
               {
                  if (boneCount == cMaxAttachmentBones)
                  {
                     BASSERT(0);
                     return;
                  }
                  boneHandles[boneCount] = boneHandle;
                  boneCount++;
               }
            }
         }
      }

      // Sample the animation and get the local pose
      if (boneCount > 0)
      {
         bool bHaveIK = false;
         long numIKNodes = pGrannyInstance->getNumIKNodes();
         for (long j = numIKNodes - 1; j >= 0; j--)
         {
            const BIKNode &IKNode = pGrannyInstance->getIKNode(j);
            if (IKNode.mIsActive && (IKNode.mNodeType == BIKNode::cIKNodeTypeSingleBone))
            {
               bHaveIK = true;
               break;
            }
         }

         if(!bHaveIK)
         {
            // Sample only needed bones
            BSparsePoseCache* pSparsePose = gGrannyManager.getLocalSparsePoseCache(attachmentLevel);
            if(pSparsePose)
            {
               granny_local_pose* pLocalPose = pSparsePose->mLocalPose; 
               granny_int32x *sparseBoneArray = pSparsePose->mSparseBoneArray;
               granny_int32x *sparseBoneArrayReverse = pSparsePose->mSparseBoneArrayReverse;

               granny_int32x sparseBoneCount = GrannySparseBoneArrayCreateSingleBone(pSkeleton, boneHandles[0], sparseBoneArray, sparseBoneArrayReverse);
               for (long j=1; j<boneCount; j++)
                  sparseBoneCount = GrannySparseBoneArrayAddBone(pSkeleton, boneHandles[j], sparseBoneCount, sparseBoneArray, sparseBoneArrayReverse);
               GrannySampleModelAnimationsLODSparse(pModelInstance, 0, sparseBoneCount, pLocalPose, 0.0f, sparseBoneArray);

               curSP.set(pSkeleton, pLocalPose, sparseBoneArray, sparseBoneArrayReverse);
            }
         }
         else
         {
            // Sample all bones since we have IK
            BSparsePoseCache* pSparsePose = gGrannyManager.getLocalSparsePoseCache(attachmentLevel);
            if(pSparsePose)
            {
               granny_local_pose* pLocalPose = pSparsePose->mLocalPose; 
               granny_int32x *sparseBoneArray = NULL;
               granny_int32x *sparseBoneArrayReverse = NULL;

               GrannySampleModelAnimations(pModelInstance, 0, pSkeleton->BoneCount, pLocalPose);

               // Update any IK nodes.  Essentially cut&paste from elsewhere.
               for (int k = 0; k < numIKNodes; k++)
               {
                  const BIKNode *pIKNode = &(pGrannyInstance->getIKNode(k));
                  if (pIKNode->mIsActive)
                  {
                     int boneIndex = BONEFROMGRANNYBONEHANDLE(pIKNode->mBoneHandle);
                     BDEBUG_ASSERT((boneIndex >= 0) && (boneIndex < (long)pSkeleton->BoneCount));

                     switch (pIKNode->mNodeType)
                     {
                        case BIKNode::cIKNodeTypeSingleBone:
                        {
                           granny_transform* boneTransform = GrannyGetLocalPoseTransform(pLocalPose, boneIndex);
                           granny_transform  IKTransform;
                           GrannyMakeIdentity(&IKTransform);
                           IKTransform.Flags = GrannyHasOrientation;
                           IKTransform.Orientation[0] = pIKNode->getAnchorPos().x;
                           IKTransform.Orientation[1] = pIKNode->getAnchorPos().y;
                           IKTransform.Orientation[2] = pIKNode->getAnchorPos().z;
                           IKTransform.Orientation[3] = pIKNode->getAnchorPos().w;
                           granny_triple savedPosition;
                           savedPosition[0] = boneTransform->Position[0];
                           savedPosition[1] = boneTransform->Position[1];
                           savedPosition[2] = boneTransform->Position[2];
                           GrannyPreMultiplyBy(boneTransform, &IKTransform);
                           boneTransform->Position[0] = savedPosition[0];
                           boneTransform->Position[1] = savedPosition[1];
                           boneTransform->Position[2] = savedPosition[2];
                           break;
                        }
                        default:
                        {
                           // jce [11/14/2008] -- the world pose is actually unneeded here because we have our target in local space and don't pass any world matrix in since we're
                           // just trying to cache the local pose.  
                           GrannyBuildWorldPose(pSkeleton, 0, pSkeleton->BoneCount, pLocalPose, (const granny_real32*)NULL, gGrannyManager.getWorldPose());
                           BVector targetPos = pIKNode->getTargetPos();
                           GrannyIKUpdate(pIKNode->mLinkCount, boneIndex, (const granny_real32*)&targetPos, 20, pSkeleton, (const granny_real32*)NULL, pLocalPose, gGrannyManager.getWorldPose());
                           break;
                        }
                     }
                  }
               }

               curSP.set(pSkeleton, pLocalPose, sparseBoneArray, sparseBoneArrayReverse);
            }
         }
      }
   }


   // Update this visual item's transform
   //
   getTransform(parentSP, curSP, mMatrix);



   // Recurse for all attachments
   //
   for(long i=0; i<count; i++)
   {
      BVisualItem& attachment=*(mAttachments[i]);
      if (attachment.getFlag(cFlagUsed))
      {
         attachment.updateTransformsRecurse(curSP, attachmentLevel + 1);
      }
   }
}

//==============================================================================
// BVisualItem::updateTransform
//==============================================================================
void BVisualItem::updateTransform(IVisualInstance* parentInstance)
{
   //SCOPEDSAMPLE(BVisualItem_updateTransform);
   getTransform(parentInstance, mMatrix);
}


//==============================================================================
//==============================================================================
void BVisualItem::getTransform(IVisualInstance* parentInstance, BMatrix &result) const
{
   BMatrix toMat, fromMat, transformMat;

   if (parentInstance && parentInstance->getBone(mToBoneHandle, NULL, &toMat, NULL, NULL))
   {
      if (mpInstance && mpInstance->getBone(mFromBoneHandle, NULL, &fromMat, NULL, NULL))
      {
         fromMat.invert();
         if(getFlag(cFlagUseTransform))
         {
            transformMat.mult(fromMat, mTransform);
            result.mult(transformMat, toMat);
         }
         else
            result.mult(fromMat, toMat);
      }
      else
      {
         if (getFlag(cFlagUseTransform))
            result.mult(mTransform, toMat);                      
         else
            result=toMat;
      }
   }
   else
   {
      if (mpInstance && mpInstance->getBone(mFromBoneHandle, NULL, &fromMat, NULL, NULL))
      {
         fromMat.invert();
         if(getFlag(cFlagUseTransform))
            result.mult(fromMat, mTransform);
         else
            result = fromMat;
      }
      else
      {
         if (getFlag(cFlagUseTransform))
            result=mTransform;
         else
            result.makeIdentity();
      }
   }

   if (getFlag(cFlagDisregardOrient))
      result.clearOrientation();

   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(0)));
   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(1)));
   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(2)));
   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(3)));
}


//==============================================================================
//==============================================================================
void BVisualItem::getTransform(BSparsePoseHelper parentSP, BSparsePoseHelper curSP, BMatrix &result) const
{
   BMatrix toMat, fromMat, transformMat;

   if (parentSP.getBone(mToBoneHandle, &toMat))
   {
      if (curSP.getBone(mFromBoneHandle, &fromMat))
      {
         fromMat.invert();
         if(getFlag(cFlagUseTransform))
         {
            transformMat.mult(fromMat, mTransform);
            result.mult(transformMat, toMat);
         }
         else
            result.mult(fromMat, toMat);
      }
      else
      {
         if (getFlag(cFlagUseTransform))
            result.mult(mTransform, toMat);                      
         else
            result=toMat;
      }
   }
   else
   {
      if (curSP.getBone(mFromBoneHandle, &fromMat))
      {
         fromMat.invert();
         if(getFlag(cFlagUseTransform))
            result.mult(fromMat, mTransform);
         else
            result = fromMat;
      }
      else
      {
         if (getFlag(cFlagUseTransform))
            result=mTransform;
         else
            result.makeIdentity();
      }
   }

   if (getFlag(cFlagDisregardOrient))
      result.clearOrientation();

   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(0)));
   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(1)));
   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(2)));
   //BASSERT(!XMVector4IsNaN(mMatrix.getRow(3)));
}


//==============================================================================
//==============================================================================
void BVisualItem::getRenderTransform(IVisualInstance* parentInstance, BMatrix &result) const
{
   BMatrix toMat, fromMat, transformMat;

   if (parentInstance && parentInstance->getBoneForRender(mToBoneHandle, toMat))
   {
      if (mpInstance && mpInstance->getBoneForRender(mFromBoneHandle, fromMat))
      {
         fromMat.invert();
         if(getFlag(cFlagUseTransform))
         {
            // Get the interpolated transform.
            BMatrix interpolatedTransform;
            BObject::getInterpolatedMatrix(mOldTransform, mTransform, interpolatedTransform, mSubUpdateNumber, false);
            
            transformMat.mult(fromMat, interpolatedTransform);
            result.mult(transformMat, toMat);
         }
         else
            result.mult(fromMat, toMat);
      }
      else
      {
         if (getFlag(cFlagUseTransform))
         {
            // Get the interpolated transform.
            BMatrix interpolatedTransform;
            BObject::getInterpolatedMatrix(mOldTransform, mTransform, interpolatedTransform, mSubUpdateNumber, false);
            
            result.mult(interpolatedTransform, toMat);                      
         }
         else
            result=toMat;
      }
   }
   else
   {
      if (mpInstance && mpInstance->getBoneForRender(mFromBoneHandle, fromMat))
      {
         fromMat.invert();
         if(getFlag(cFlagUseTransform))
         {
            // Get the interpolated transform.
            BMatrix interpolatedTransform;
            BObject::getInterpolatedMatrix(mOldTransform, mTransform, interpolatedTransform, mSubUpdateNumber, false);         
            result.mult(fromMat, interpolatedTransform);
         }
         else
            result = fromMat;
      }
      else
      {
         if (getFlag(cFlagUseTransform))
         {
            // Get the interpolated transform.
            BMatrix interpolatedTransform;
            BObject::getInterpolatedMatrix(mOldTransform, mTransform, interpolatedTransform, mSubUpdateNumber, false);
            
            result=interpolatedTransform;
         }
         else
            result.makeIdentity();
      }
   }

   if (getFlag(cFlagDisregardOrient))
      result.clearOrientation();
}



//==============================================================================
// BVisualItem::updateTerrainEffects
//==============================================================================
void BVisualItem::updateTerrainEffects(const BMatrix& worldMatrix, DWORD tintColor)
{
   //SCOPEDSAMPLE(BVisualItem_updateTerrainEffects)

   if (mModelAsset.mType != cVisualAssetTerrainEffect)
      return;

   // Check what pfx we need to be showing for the type of terrain that we
   // are currently in.
   //
   BParticleEffectDataHandle currentPfxHandle = -1;
   BParticleEffectDataHandle desiredPfxHandle = -1;


   // Get desired handle
   //

   BVector particlePos;
   worldMatrix.getTranslation(particlePos);


   // TODO: This needs to be optimized for units that will always be on the ground (tanks) so that the airbone
   // state isn't tested at all.  (SAT)

   // Check if the unit is airborne
   bool bIsAirborne = false;

   // Disable this for now, since warthogs dont' jump anyway
   //
   /*
   float heightAboveGround;
   gTerrainSimRep.getHeightRaycast(particlePos, heightAboveGround, true);
   heightAboveGround = particlePos.y - heightAboveGround;

   if(heightAboveGround > cAirborneHeightAboveGround)
      bIsAirborne = true;
   */


   if(!bIsAirborne)
   {
      // If not airborne check the terrain type that we are in

      // Get terrain type
      BYTE surfaceType = gTerrainSimRep.getTileType(particlePos);

      BTerrainEffect *pTerrainEffect = gTerrainEffectManager.getTerrainEffect(mModelAsset.mIndex, true);

      if(pTerrainEffect)
      {
         desiredPfxHandle = pTerrainEffect->getParticleEffectHandleForType(surfaceType, -1);
      }
   }
   else
   {
      // If airborne then no pfx
      desiredPfxHandle = -1;
   }


   // Get current handle
   //

   //currentPfxHandle = mModelAsset.mIndex;
   if(mpInstance != NULL)
   {
      currentPfxHandle = gParticleGateway.getDataHandleForInstance(((BParticleInstance*)mpInstance));
   }
   else
   {
      currentPfxHandle = -1;
   }



   if(desiredPfxHandle != currentPfxHandle)
   {
      // Release previews instace
      releaseInstance(false);

      if(desiredPfxHandle != -1)
      {
         BParticleCreateParams params;
         params.mDataHandle        = desiredPfxHandle;
         params.mMatrix            = worldMatrix;
         params.mNearLayerEffect   = false;
         params.mPriority          = mDisplayPriority;

         BParticleInstance* pParticleInstance = gParticleGateway.createInstance(params);
         mpInstance = pParticleInstance;
      }
   }
}

//==============================================================================
// BVisualItem::isWorldMatrixUpdateNeeded
//==============================================================================
bool BVisualItem::isWorldMatrixUpdateNeeded()
{
   if (mpInstance && mModelAsset.mType != cVisualAssetGrannyModel && mModelAsset.mType != -1)
      return true;

   long count=mAttachments.getNumber();   
   for (long i=0; i<count; i++)
   {
      BVisualItem* pAttachment=mAttachments[i];
      if (pAttachment->getFlag(cFlagUsed) && pAttachment->isWorldMatrixUpdateNeeded())
         return true;
   }

   return false;
}

//==============================================================================
// BVisualItem::updateWorldMatrix
//==============================================================================
void BVisualItem::updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix)
{
   if (mpInstance)
      mpInstance->updateWorldMatrix(worldMatrix, pLocalMatrix);

   long count=mAttachments.getNumber();   
   if(count>0)
   {
      const BMatrix* pWorldMatrix;
      BMatrix matrix;
      if (pLocalMatrix)
      {
         matrix.mult(*pLocalMatrix, worldMatrix);
         pWorldMatrix=&matrix;
      }
      else 
         pWorldMatrix=&worldMatrix;

      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         if(pAttachment->getFlag(cFlagUsed))
         {
            pAttachment->updateWorldMatrix(*pWorldMatrix, &mMatrix);
         }
      }
   }
}

//==============================================================================
// BVisualItem::updateSecondaryWorldMatrix
//==============================================================================
void BVisualItem::updateSecondaryWorldMatrix(BMatrix secondaryMatrix)
{
   if (mpInstance)
      mpInstance->setSecondaryMatrix(secondaryMatrix);

   long count=mAttachments.getNumber();   
   if(count>0)
   {      
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         if(pAttachment->getFlag(cFlagUsed))
            pAttachment->updateSecondaryWorldMatrix(secondaryMatrix);
      }
   }
}

//==============================================================================
// void BVisualItem::updateVisibility(bool bVisible)
//==============================================================================
void BVisualItem::updateVisibility(bool bVisible)
{
   mFlagObjectVisible = bVisible;

   if (mpInstance)
      mpInstance->setVisibility(bVisible && getFlag(cFlagVisible));

   long count=mAttachments.getNumber();   
   if(count>0)
   {
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         if(pAttachment->getFlag(cFlagUsed))
            pAttachment->updateVisibility(bVisible);
      }
   }
}

//==============================================================================
// BVisualItem::updateNearLayer
//==============================================================================
void BVisualItem::updateNearLayer(bool bNearLayer)
{
   if (mpInstance)
      mpInstance->setNearLayer(bNearLayer);

   long count=mAttachments.getNumber();   
   if(count>0)
   {
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         if(pAttachment->getFlag(cFlagUsed))
            pAttachment->updateNearLayer(bNearLayer);
      }
   }
}


//==============================================================================
//==============================================================================
void BVisualItem::renderPrepare()
{
   // Bail if we're not visible.
   // jce [11/5/2008] -- I'm just mimicking the old code by commenting this out here since
   // elsewhere only attachments check this flag. Otherwise it's inconsistent with render and fails.
   //if(!getFlag(cFlagVisible))
     // return;

   // This function sets up all the granny models by posing them for rendering.
   if(mpInstance && mModelAsset.mType == cVisualAssetGrannyModel)
   {
      // Cast to the right type.
      BGrannyInstance *grannyInstance = (BGrannyInstance*)mpInstance;
      
      if (grannyInstance && gEnableSubUpdating)
         grannyInstance->preRender(BObject::getInterpolation(mGrannySubUpdateNumber));

      // Prepare.
      grannyInstance->renderPrepare();
      
      // Queue this instance with the visual manager.
      gVisualManager.queueGrannyInstanceForSampling(grannyInstance);
   }
   
   // Now our attachments.
   long count = mAttachments.getNumber();
   for(long i=0; i<count; i++)
   {
      // Get attachment.
      BVisualItem *attachment = mAttachments[i];
      if(!attachment)
         continue;

      // jce [11/5/2008] -- Skip if not visible (moved here so it does not occur on root)      
      if(!getFlag(cFlagVisible))
         continue;

      // Skip if unused.
      if(!attachment->getFlag(cFlagUsed))
         continue;
      
      // Prepare to render.
      attachment->renderPrepare();
   }
}


//==============================================================================
//==============================================================================
void BVisualItem::render(BVisualRenderAttributes* pRenderAttributes)
{
   // Queue ourselves.  Children will be handled when this list of top-level items is processed.
   gVisualManager.queueVisualItemForRender(this, pRenderAttributes);
}



//==============================================================================
//==============================================================================
void BVisualItem::internalRender(BVisualRenderAttributes* pRenderAttributes)
{
   if(mpInstance)
   {
      if (mModelAsset.mType == cVisualAssetLight) // this is now handled every frame in ::render
         mpInstance->update((float)gRenderTime.getLastUpdateLength());
   }

   BASSERT(pRenderAttributes);
   if (!pRenderAttributes)
      return;

   // Set up and render the instance
   if (mpInstance)
   {
      // jce [11/3/2008] -- Update world matrix for things like particles that aren't really rendered from render but need
      // an interpolated matrix set for them.
      BMatrix worldMatrix=gRender.getWorldBMatrix();
      mpInstance->updateWorldMatrix(worldMatrix, NULL);


      BGrannyInstance* pGrannyInstance = NULL;
      
      if (mModelAsset.mType == cVisualAssetGrannyModel)         
         pGrannyInstance = static_cast<BGrannyInstance*>(mpInstance);

      // Set up reflect bone index if this is a granny instance with local
      // reflection turned on
      if (pGrannyInstance)
      {
         long modelIndex = pGrannyInstance->getModelIndex();
//-- FIXING PREFIX BUG ID 7367
         const BGrannyModel* pModel = gGrannyManager.getModel(modelIndex);
//--
         if (pModel && (pModel->getUGXGeomRenderInfo().mRenderFlags & cRFLocalReflection))
         {
            long reflectPointHandle = getPointHandle(cActionAnimationTrack, cVisualPointReflect);
            const BProtoVisualPoint* pPoint = getPointProto(cActionAnimationTrack, reflectPointHandle);
            if (pPoint)
            {
               uint16 reflectBoneIndex = pPoint->mBoneHandle >= 0 ? static_cast<uint16>(BONEFROMGRANNYBONEHANDLE(pPoint->mBoneHandle)) : 0;
               pRenderAttributes->mReflectBoneIndex = reflectBoneIndex;
            }
         }
      }
      
      // jce [11/3/2008] -- this happens in renderPrepare now  
      //if (pGrannyInstance && gEnableSubUpdating)
        // pGrannyInstance->preRender(BObject::getInterpolation(mGrannySubUpdateNumber));
        
      mpInstance->render(pRenderAttributes);
   }

#ifdef DEBUG_RENDER_BOUNDING_BOX
   // ajl 3/7/06 - Was using bounding box draw code, but it appears to be broken
   BVector e=(mCombinedMaxCorner-mCombinedMinCorner)*0.5f;
   BVector c=mCombinedMinCorner+e;

   BMatrix matrix=gRender.getWorldBMatrix();
   matrix.multTranslate(c.x, c.y, c.z);
   gpDebugPrimitives->addDebugAxis(matrix, 1.0f);

   matrix=gRender.getWorldBMatrix();

   BVector p[8];
   matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y+e.y,c.z+e.z), p[0]);
   matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y+e.y,c.z-e.z), p[1]);
   matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y+e.y,c.z-e.z), p[2]);
   matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y+e.y,c.z+e.z), p[3]);

   matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y-e.y,c.z+e.z), p[4]);
   matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y-e.y,c.z-e.z), p[5]);
   matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y-e.y,c.z-e.z), p[6]);
   matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y-e.y,c.z+e.z), p[7]);

   gpDebugPrimitives->addDebugLine(p[0], p[1], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[1], p[2], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[2], p[3], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[3], p[0], cDWORDOrange, cDWORDOrange);

   gpDebugPrimitives->addDebugLine(p[4], p[5], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[5], p[6], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[6], p[7], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[7], p[4], cDWORDOrange, cDWORDOrange);

   gpDebugPrimitives->addDebugLine(p[0], p[4], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[1], p[5], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[2], p[6], cDWORDOrange, cDWORDOrange);
   gpDebugPrimitives->addDebugLine(p[3], p[7], cDWORDOrange, cDWORDOrange);
#endif

   float unit_opacity = D3DCOLOR_GETALPHA(pRenderAttributes->mTintColor) / 255.0f;

   // Render the attachments
   long count=mAttachments.getNumber();
   if(count>0)
   {
      BMatrix worldMatrix=gRender.getWorldBMatrix();
      BMatrix matrix;
      for(long i=0; i<count; i++)
      {
         BVisualItem& attachment=*(mAttachments[i]);
         if(!attachment.getFlag(cFlagUsed) || !attachment.mpInstance || !attachment.getFlag(cFlagVisible))
            continue;

         // get opacity
         float attachment_opacity = attachment.getAnimationOpacity();
         attachment_opacity *= unit_opacity;
   
         // do not render fully alpha visual
         if(attachment_opacity == 0.0f)
            continue;

         //pRenderAttributes->mTintColor = D3DCOLOR_ARGB(Math::iClampToByte((int)(255 * attachment_opacity)), 0, 0, 0);
         pRenderAttributes->mTintColor = D3DCOLOR_ARGB(Math::iClampToByte((int)(255 * attachment_opacity)), D3DCOLOR_GETRED(pRenderAttributes->mTintColor), D3DCOLOR_GETGREEN(pRenderAttributes->mTintColor), D3DCOLOR_GETBLUE(pRenderAttributes->mTintColor));

         // Get attachment's transform.
         BMatrix attachmentTransform;
         attachment.getRenderTransform(mpInstance, attachmentTransform);
         
         // Factor in accumulated parent matrices + world matrix.
         matrix.mult(attachmentTransform, worldMatrix);
         
         // Set the concatenated matrix as the new world matrix.
         gRender.setWorldMatrix(matrix);

         // Render the attachment.
         attachment.internalRender(pRenderAttributes);
         

#ifdef DEBUG_RENDER_BOUNDING_BOX
         // ajl 3/7/06 - Was using bounding box draw code, but it appears to be broken
         e=(attachment.mCombinedMaxCorner-attachment.mCombinedMinCorner)*0.5f;
         c=attachment.mCombinedMinCorner+e;

         gpDebugPrimitives->addDebugAxis(matrix, 1.0f);

         BVector p[8];
         matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y+e.y,c.z+e.z), p[0]);
         matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y+e.y,c.z-e.z), p[1]);
         matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y+e.y,c.z-e.z), p[2]);
         matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y+e.y,c.z+e.z), p[3]);

         matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y-e.y,c.z+e.z), p[4]);
         matrix.transformVectorAsPoint(BVector(c.x+e.x,c.y-e.y,c.z-e.z), p[5]);
         matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y-e.y,c.z-e.z), p[6]);
         matrix.transformVectorAsPoint(BVector(c.x-e.x,c.y-e.y,c.z+e.z), p[7]);

         gpDebugPrimitives->addDebugLine(p[0], p[1], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[1], p[2], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[2], p[3], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[3], p[0], cDWORDPurple, cDWORDPurple);

         gpDebugPrimitives->addDebugLine(p[4], p[5], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[5], p[6], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[6], p[7], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[7], p[4], cDWORDPurple, cDWORDPurple);

         gpDebugPrimitives->addDebugLine(p[0], p[4], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[1], p[5], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[2], p[6], cDWORDPurple, cDWORDPurple);
         gpDebugPrimitives->addDebugLine(p[3], p[7], cDWORDPurple, cDWORDPurple);
#endif
      }
      gRender.setWorldMatrix(worldMatrix);
   }
}

//==============================================================================
// BVisualItem::applyAnimation
//==============================================================================
void BVisualItem::applyAnimation(long animationTrack, long animType, bool synced, bool applyInstantly, float timeIntoAnimation, float tweenTime, BVisualItem* startOnThisAttachment)
{
   BVisualAnimationData &animation = mAnimationTrack[animationTrack];
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::applyAnimation animationTrack", animationTrack);
         syncAnimData("BVisualItem::applyAnimation anim", gVisualManager.getAnimName(animType));
         //syncAnimData("BVisualItem::applyAnimation animType", animType);
         syncAnimData("BVisualItem::applyAnimation applyInstantly", applyInstantly);
         syncAnimData("BVisualItem::applyAnimation timeIntoAnimation", timeIntoAnimation);
         syncAnimData("BVisualItem::applyAnimation tweenTime", tweenTime);
      }
   #endif

   animation.mpTweenOutTags = animation.mpTags;
   animation.mTweenOutPosition = animation.mPosition;
   animation.mTweenOutDuration = animation.mDuration;
   animation.mTweenOutAnimAssetType = animation.mAnimAsset.mType;
   animation.mTweenOutTimeLeft = 0.0f;

   animation.mPosition=0.0f;
   animation.mDuration=0.0f;

   if(mpInstance==NULL)
      return;

   if (mpInstance && (mModelAsset.mType == cVisualAssetGrannyModel))
   {
//-- FIXING PREFIX BUG ID 7368
      const BGrannyAnimation* pGrannyAnim = gGrannyManager.getAnimation(animation.mAnimAsset.mIndex);
//--
      if (pGrannyAnim)
      {
         animation.mDuration = pGrannyAnim->getDuration();
         #ifdef SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::applyAnimation mDuration", animation.mDuration);
            }
         #endif
      }
   }
   animation.mPosition = timeIntoAnimation * animation.mDuration;

   #ifdef SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::applyAnimation mPosition", animation.mPosition);
      }
   #endif

   if(mModelAsset.mIndex==-1 || animation.mAnimAsset.mIndex==-1)
   {
      if(mpInstance && mModelAsset.mType==cVisualAssetGrannyModel)
      {
         BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
         animation.mTweenOutTimeLeft = applyInstantly ? 0.0f : tweenTime;
         if (!startOnThisAttachment)
            pGrannyInstance->stopAnimations(animationTrack, animation.mTweenOutTimeLeft);
      }
      setFlag(cFlagSendNoAnimEvent, true);
      return;
   }
   else
      setFlag(cFlagSendNoAnimEvent, false);

   switch(animation.mAnimAsset.mType)
   {
      case cVisualAssetGrannyAnim:
         if(mModelAsset.mType==cVisualAssetGrannyModel && mpInstance)
         {
            BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;
            animation.mTweenOutTimeLeft = applyInstantly ? 0.0f : tweenTime;
            animation.mAnimationRate = 1.0f;
            if (!startOnThisAttachment)
               pGrannyInstance->playAnimation(animationTrack, animation.mAnimAsset.mIndex, 1.0f, 0.0f, (animation.mExitAction==cAnimExitActionLoop?0:1), animation.mTweenOutTimeLeft, animation.mPosition);
         }
         break;
   }
}

//==============================================================================
// BVisualItem::getBoneHandle
//==============================================================================
long BVisualItem::getBoneHandle(const char* pBoneName) const
{
   if(mpInstance)
      return (mpInstance->getBoneHandle(pBoneName));
   else
      return (-1);
}

//==============================================================================
// BVisualItem::getBone
//==============================================================================
bool BVisualItem::getBone(long boneHandle, BVector* pPos, BMatrix* pMatrix, BBoundingBox* pBox, const BMatrix* pOffsetMatrix, bool applyIK, const BVisualItem* pHasThisChild) const
{
   if(mpInstance)
   {
      bool success;

      // Only get the bone from a visualItem that has the specified child
      if (pHasThisChild)
      {
         long numAttachments = mAttachments.getNumber();
         for (long i = 0; i < numAttachments; i++)
         {
//-- FIXING PREFIX BUG ID 7369
            const BVisualItem* pAttachment = mAttachments[i];
//--
            if (pAttachment == pHasThisChild)
            {
               if (!pAttachment->getFlag(cFlagUsed))
                  return false;

               return mpInstance->getBone(boneHandle, pPos, pMatrix, pBox, pOffsetMatrix, applyIK);
            }
         }

         success = false;
      }
      else
         success = mpInstance->getBone(boneHandle, pPos, pMatrix, pBox, pOffsetMatrix, applyIK);

      if (!success)
      {
         long numAttachments = mAttachments.getNumber();
         for (long i = 0; i < numAttachments; i++)
         {
//-- FIXING PREFIX BUG ID 7370
            const BVisualItem* pAttachment = mAttachments[i];
//--
            if (pAttachment->getFlag(cFlagUsed))
            {
               success = mAttachments[i]->getBone(boneHandle, pPos, pMatrix, pBox, pOffsetMatrix, applyIK, pHasThisChild);
               if (success)
                  break;
            }
         }
      }

      return success;
   }
   else
      return false;
}

//==============================================================================
// BVisualItem::getNumBones
//==============================================================================
long BVisualItem::getNumBones() const
{
   if(mModelAsset.mType == cVisualAssetGrannyModel)
   {
//-- FIXING PREFIX BUG ID 7372
      const BGrannyInstance* pGrannyInstance=(BGrannyInstance*)(mpInstance);
//--
      if(pGrannyInstance)
      {
         long modelIndex = pGrannyInstance->getModelIndex();

//-- FIXING PREFIX BUG ID 7371
         const BGrannyModel* pModel = gGrannyManager.getModel(modelIndex);
//--
         if(pModel)
            return(pModel->getNumBones());
      }
   }

   return 0;
}

//==============================================================================
// BVisualItem::getNumTagsOfType
//==============================================================================
long BVisualItem::getNumTagsOfType(long animationTrack, long eventType) const
{
   const BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   if(animation.mpTags == NULL)
      return 0;

   long numOfType = 0;
   long numTags = animation.mpTags->getNumber();
   for(long i = 0; i < numTags; i++)
   {
      if((*(animation.mpTags))[i].mEventType == eventType)
         numOfType++;
   }

   return numOfType;
}

//==============================================================================
// BVisualItem::getNumTags
//==============================================================================
long BVisualItem::getNumTags(long animationTrack) const
{
   const BVisualAnimationData &animation = mAnimationTrack[animationTrack];
   if (animation.mpTags == NULL)
      return 0;
   else
      return animation.mpTags->getNumber();
}

//==============================================================================
// BVisualItem::setAnimationEnabled
//==============================================================================
void BVisualItem::setAnimationEnabled(bool flag)
{
   setFlag(cFlagAnimationDisabled, !flag);
   
   long numAttachments = mAttachments.getNumber();
   for (long i = 0; i < numAttachments; i++)
      mAttachments[i]->setAnimationEnabled(flag);
}

//==============================================================================
// BVisualItem::setAnimation
//==============================================================================
void BVisualItem::setAnimation(long animationTrack, BProtoVisual* pProtoVisual, int64 userData, long animType, bool applyInstantly, float timeIntoAnimation, DWORD tintColor, const BMatrix& worldMatrix, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction)
{
   //SCOPEDSAMPLE(BVisualItem_setAnimation)
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::setAnimation animationTrack", animationTrack);
         syncAnimData("BVisualItem::setAnimation userData", (DWORD)userData);
         syncAnimData("BVisualItem::applyAnimation anim", gVisualManager.getAnimName(animType));
         //syncAnimData("BVisualItem::setAnimation animType", animType);
         syncAnimData("BVisualItem::setAnimation applyInstantly", applyInstantly);
         syncAnimData("BVisualItem::setAnimation timeIntoAnimation", timeIntoAnimation);
         syncAnimData("BVisualItem::setAnimation reset", reset);
         syncAnimData("BVisualItem::setAnimation getFlag(cFlagAnimationDisabled)", getFlag(cFlagAnimationDisabled));
      }
   #endif

   if (!pProtoVisual || getFlag(cFlagAnimationDisabled))
      return;
   if (!mAnimationTrack[animationTrack].mIsLocked)
   {
      // SLB: This breaks stuff
      //mAnimationTrack[animationTrack].mIsClone = false;

      updateState(animationTrack, pProtoVisual, userData, animType, applyInstantly, timeIntoAnimation, forceAnimID, reset, startOnThisAttachment, tintColor, worldMatrix, pOverrideExitAction, false, false);
      if (getFlag(cFlagSynced))
         sUpdateSyncCallback(this, false, true);
   }
}

//==============================================================================
// BVisualItem::getAnimationDuration
//==============================================================================
float BVisualItem::getAnimationDuration(long animationTrack) const
{
   return mAnimationTrack[animationTrack].mDuration;
}

//==============================================================================
// BVisualItem::getAnimationPosition
//==============================================================================
float BVisualItem::getAnimationPosition(long animationTrack) const
{
   return mAnimationTrack[animationTrack].mPosition;
}

//==============================================================================
// BVisualItem::getAnimationOpacity
//==============================================================================
float BVisualItem::getAnimationOpacity() const
{
   if(mAnimationTrack[0].mpOpacityProgression)
   {
      float animPos = getAnimationPosition(0);
      float animDur = getAnimationDuration(0);

      float animPosNorm = (animDur <= cFloatCompareEpsilon) ? 0.0f : (animPos / animDur);

      animPosNorm = Math::Clamp(animPosNorm, 0.0f, 1.0f);
      XMVECTOR value;
      mAnimationTrack[0].mpOpacityProgression->getValueWithCycles(animPosNorm, 0, &value);

      value.x = Math::Clamp(value.x, 0.0f, 1.0f);
      return(value.x);
   }

   return(1.0f);
}


//==============================================================================
// BVisualItem::getAnimationClock
//==============================================================================
float BVisualItem::getAnimationClock(long animationTrack) const
{
   if(mpInstance && mModelAsset.mType==cVisualAssetGrannyModel && mAnimationTrack[animationTrack].mAnimAsset.mType==cVisualAssetGrannyAnim)
   {
//-- FIXING PREFIX BUG ID 7373
      const BGrannyInstance* pGrannyInstance=(BGrannyInstance*)(mpInstance);
//--
      return pGrannyInstance->getAnimClock();
   }
   return 0.0f;
}

//==============================================================================
// BVisualItem::copyAnimationTrack
//==============================================================================
void BVisualItem::copyAnimationTrack(long fromTrack, long toTrack, BProtoVisual* pProtoVisual, int64 userData, bool applyInstantly, DWORD tintColor, const BMatrix& worldMatrix, BVisualItem* startOnThisAttachment, bool noRecursion)
{
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::copyAnimationTrack fromTrack", fromTrack);
         syncAnimData("BVisualItem::copyAnimationTrack toTrack", toTrack);
         syncAnimData("BVisualItem::copyAnimationTrack userData", (DWORD)userData);
         syncAnimData("BVisualItem::copyAnimationTrack applyInstantly", applyInstantly);
         syncAnimData("BVisualItem::copyAnimationTrack noRecursion", noRecursion);
         syncAnimData("BVisualItem::copyAnimationTrack getFlag(cFlagAnimationDisabled)", getFlag(cFlagAnimationDisabled));
      }
   #endif

   //SCOPEDSAMPLE(BVisualItem_copyAnimationTrack)
   if (!pProtoVisual || getFlag(cFlagAnimationDisabled))
      return;

   BVisualAnimationData &toAnimation = mAnimationTrack[toTrack];

   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::copyAnimationTrack toAnimation.mIsLocked", toAnimation.mIsLocked);
      }
   #endif

   if (!toAnimation.mIsLocked)
   {
      BVisualAnimationData &fromAnimation = startOnThisAttachment ? startOnThisAttachment->mAnimationTrack[fromTrack] : mAnimationTrack[fromTrack];

      // Match the master track's animation position and animation ID
      float timeIntoAnimation = (fromAnimation.mDuration >= cFloatCompareEpsilon) ? (fromAnimation.mPosition / fromAnimation.mDuration) : 0.0f;
      long forceAnimID = (fromAnimation.mAnimAsset.mIndex == -1) ? -1 : ((~fromAnimation.mAnimAsset.mIndex) << 16);

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::copyAnimationTrack timeIntoAnimation", timeIntoAnimation);
         }
      #endif

      updateState(toTrack, pProtoVisual, userData, fromAnimation.mAnimType, applyInstantly, timeIntoAnimation, forceAnimID, true, startOnThisAttachment, tintColor, worldMatrix, NULL, true, noRecursion);

      #ifdef DEBUG_COPY_ANIMATION_TRACK
         if (startOnThisAttachment)
         {
            BASSERT(Math::fAbs(startOnThisAttachment->mAnimationTrack[cActionAnimationTrack].mPosition - startOnThisAttachment->mAnimationTrack[cMovementAnimationTrack].mPosition) <= cFloatCompareEpsilon);
            BASSERT(Math::fAbs(startOnThisAttachment->mAnimationTrack[cActionAnimationTrack].mDuration - startOnThisAttachment->mAnimationTrack[cMovementAnimationTrack].mDuration) <= cFloatCompareEpsilon);
         }
         else
         {
            BASSERT(Math::fAbs(mAnimationTrack[cActionAnimationTrack].mPosition - mAnimationTrack[cMovementAnimationTrack].mPosition) <= cFloatCompareEpsilon);
            BASSERT(Math::fAbs(mAnimationTrack[cActionAnimationTrack].mDuration - mAnimationTrack[cMovementAnimationTrack].mDuration) <= cFloatCompareEpsilon);
         }
      #endif
   }
}

//==============================================================================
// BVisualItem::updateState
//==============================================================================
void BVisualItem::updateState(long animationTrack, BProtoVisual* pProtoVisual, int64 userData, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, DWORD tintColor, const BMatrix& worldMatrix, const BProtoVisualAnimExitAction* pOverrideExitAction, bool clone, bool noRecursion)
{
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::updateState animationTrack", animationTrack);
         syncAnimData("BVisualItem::updateState userData", (DWORD)userData);
         syncAnimData("BVisualItem::applyAnimation anim", gVisualManager.getAnimName(animType));
         //syncAnimData("BVisualItem::updateState animType", animType);
         syncAnimData("BVisualItem::updateState applyInstantly", applyInstantly);
         syncAnimData("BVisualItem::updateState timeIntoAnimation", timeIntoAnimation);
         syncAnimData("BVisualItem::updateState reset", reset);
         syncAnimData("BVisualItem::updateState clone", clone);
         syncAnimData("BVisualItem::updateState noRecursion", noRecursion);
         syncAnimData("BVisualItem::updateState getFlag(cFlagAnimationDisabled)", getFlag(cFlagAnimationDisabled));
      }
   #endif

   if (!pProtoVisual)
      return;

   if (startOnThisAttachment == this)
   {
      startOnThisAttachment = NULL;

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::updateState startOnThisAttachment = NULL", animationTrack);
         }
      #endif
   }

   BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::updateState animation.mIsLocked", animation.mIsLocked);
      }
   #endif

   if (!animation.mIsLocked)
   {
      // Calculate the current visual state from the protoVisual
      BVisualItem* pState = (clone) ? this : getNewInstance(false);
      if(!pState)
         return;
      BVisualItem& state=*pState;
      BProtoVisualAnim* pAnim = NULL;

      if (!clone)
      {
         pProtoVisual->calcState(animationTrack, animType, (getFlag(cFlagSynced)?cSimRand:cUnsyncedRand), userData, &state, &pAnim, this, forceAnimID);
      }

      float tweenTime = animation.mTweenTime;
      long newExitAction;
      long newTweenToAnimation;
      float newTweenTime;
      if (pOverrideExitAction)
      {
         newExitAction = pOverrideExitAction->mExitAction;
         newTweenTime = pOverrideExitAction->mTweenTime;
         newTweenToAnimation = pOverrideExitAction->mTweenToAnimation;
      }
      else
      {
         if (clone)
         {
            BVisualAnimationData &masterAnimation = mAnimationTrack[1 - animationTrack];
            newExitAction = masterAnimation.mExitAction;
            newTweenTime = masterAnimation.mTweenTime;
            newTweenToAnimation = masterAnimation.mTweenToAnimation;
         }
         else if (pAnim)
         {
            newExitAction = pAnim->mExitAction.mExitAction;
            newTweenTime = pAnim->mExitAction.mTweenTime;
            newTweenToAnimation = pAnim->mExitAction.mTweenToAnimation;
         }
         else
         {
            newExitAction = cAnimExitActionLoop;
            newTweenTime = 0.0f;
            newTweenToAnimation = -1;
         }
      }

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::updateState newExitAction", newExitAction);
            syncAnimData("BVisualItem::updateState newTweenTime", newTweenTime);
         }
      #endif

      updateState(animationTrack, pState, getFlag(cFlagSynced), animType, applyInstantly, timeIntoAnimation, tweenTime, newExitAction, newTweenToAnimation, newTweenTime, reset, startOnThisAttachment, clone, tintColor, worldMatrix, noRecursion);

      // Reset mesh render mask for "optional" meshes when animation changes (these are static meshes toggled per anim).  This
      // reuses the damage mesh render mask so it should not be used on objects with destructible pieces as this resets
      // the current "damage" render state.
      if (pAnim && pProtoVisual->getFlag(BProtoVisual::cFlagHasOptionalMesh))
      {
         BGrannyInstance* pGrannyInstance = getGrannyInstance();
         if (pGrannyInstance)
         {
            // First reset to default state
            pGrannyInstance->setMeshRenderMaskToUndamageState();
            // Enable all optional meshes specified by the new anim
            for (uint i = 0; i < pAnim->mOptionalMeshes.getSize(); i++)
            {
               pGrannyInstance->setMeshVisible(pAnim->mOptionalMeshes[i].mMeshIndex, true);
            }
         }
      }

      if (!clone)
         BVisualItem::releaseInstance(pState);
   }
}

//==============================================================================
// BVisualItem::getAnimation
//==============================================================================
BVisualAnimationData BVisualItem::getAnimationData(long animationTrack, BProtoVisual* pProtoVisual, int64 userData, long animType)
{
   BVisualAnimationData retData;
   retData.init();

   if (!pProtoVisual)
      return retData;

   BVisualItem* pState = getNewInstance(false);
   if (!pState)
      return retData;
   BVisualItem& state = *pState;
   BProtoVisualAnim* pAnim = NULL;

   pProtoVisual->calcState(animationTrack, animType, (getFlag(cFlagSynced)?cSimRand:cUnsyncedRand), userData, &state, &pAnim, this);
   retData = pState->mAnimationTrack[animationTrack];
//-- FIXING PREFIX BUG ID 7378
   const BGrannyAnimation* pGrannyAnim = gGrannyManager.getAnimation(retData.mAnimAsset.mIndex);
//--
   if (pGrannyAnim)
      retData.mDuration = pGrannyAnim->getDuration();

   BVisualItem::releaseInstance(pState);

   return retData;
}

//==============================================================================
// BVisualItem::updateState
//==============================================================================
void BVisualItem::updateState(long animationTrack, BVisualItem* pState, bool synced, long animType, bool applyInstantly, float timeIntoAnimation, float tweenTime, long newExitAction, long newTweenToAnimation, float newTweenTime, bool reset, BVisualItem* startOnThisAttachment, bool clone, DWORD tintColor, const BMatrix& worldMatrix, bool noRecursion)
{
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::updateState animationTrack", animationTrack);
         syncAnimData("BVisualItem::applyAnimation anim", gVisualManager.getAnimName(animType));
         //syncAnimData("BVisualItem::updateState animType", animType);
         syncAnimData("BVisualItem::updateState applyInstantly", applyInstantly);
         syncAnimData("BVisualItem::updateState timeIntoAnimation", timeIntoAnimation);
         syncAnimData("BVisualItem::updateState tweenTime", tweenTime);
         syncAnimData("BVisualItem::updateState newExitAction", newExitAction);
         syncAnimData("BVisualItem::updateState newTweenTime", newTweenTime);
         syncAnimData("BVisualItem::updateState reset", reset);
         syncAnimData("BVisualItem::updateState clone", clone);
         syncAnimData("BVisualItem::updateState noRecursion", noRecursion);
         syncAnimData("BVisualItem::updateState getFlag(cFlagAnimationDisabled)", getFlag(cFlagAnimationDisabled));
      }
   #endif

   if (startOnThisAttachment == this)
   {
      startOnThisAttachment = NULL;

      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (synced)
         {
            syncAnimData("BVisualItem::updateState startOnThisAttachment = NULL", animationTrack);
         }
      #endif
   }

   bool updateThisState = (!startOnThisAttachment);

   BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::updateState updateThisState", updateThisState);
         syncAnimData("BVisualItem::updateState animation.mIsLocked", animation.mIsLocked);
      }
   #endif

   if (!animation.mIsLocked)
   {
      BVisualAnimationData &stateAnimation = (clone) ? mAnimationTrack[1 - animationTrack] : pState->mAnimationTrack[animationTrack];

      if (updateThisState)
      {
         animation.mIsClone = clone;

         if (animation.mExitAction != cAnimExitActionLoop)
            tweenTime = Math::Clamp(animation.mDuration - animation.mPosition, 0.0f, tweenTime);

         animation.mExitAction = newExitAction;
         animation.mTweenToAnimation = newTweenToAnimation;
         animation.mTweenTime = newTweenTime;
         animation.mIsDirty = true;

         #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::updateState tweenTime", tweenTime);
               syncAnimData("BVisualItem::updateState animation.mIsClone", animation.mIsClone);
               syncAnimData("BVisualItem::updateState animation.mExitAction", animation.mExitAction);
               syncAnimData("BVisualItem::updateState animation.mTweenTime", animation.mTweenTime);
               syncAnimData("BVisualItem::updateState animation.mIsDirty", animation.mIsDirty);

               if (pState)
               {
                  syncAnimData("BVisualItem::updateState pState->mModelAsset.mType", pState->mModelAsset.mType);
                  syncAnimData("BVisualItem::updateState pState->mMinCorner", pState->mMinCorner);
                  syncAnimData("BVisualItem::updateState pState->mMaxCorner", pState->mMaxCorner);
               }
            }
         #endif


         // Update the current item's state
         updateState(animationTrack, animType, synced,
            pState->mModelAsset.mType, pState->mModelAsset.mIndex, pState->mModelUVOffsets,
            stateAnimation.mAnimAsset.mType, stateAnimation.mAnimAsset.mIndex, stateAnimation.mpTags, stateAnimation.mpPoints, stateAnimation.mpOpacityProgression,
            pState->mMinCorner, pState->mMaxCorner, applyInstantly, timeIntoAnimation, tweenTime, pState->mDamageTemplateIndex, reset, startOnThisAttachment, tintColor, worldMatrix);

         // Don't recurse through the attachments
         if (noRecursion)
         {
            BASSERT(clone);
            return;
         }
      }

      // Recurse through the attachments
      if (clone)
      {
         long numAttachments = mAttachments.getNumber();

         #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::updateState numAttachments", numAttachments);
            }
         #endif

         for (long i = 0; i < numAttachments; i++)
         {
            BVisualItem& attachment = *(mAttachments[i]);

            #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
               if (synced)
               {
                  syncAnimData("BVisualItem::updateState attachment.getFlag(cFlagUser)", attachment.getFlag(cFlagUser));
               }
            #endif

            // Skip user attachments
            if(attachment.getFlag(cFlagUser))
               continue;
            
            #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
               if (synced)
               {
                  syncAnimData("BVisualItem::updateState attachment.getFlag(cFlagUsed)", attachment.getFlag(cFlagUsed));
                  syncAnimData("BVisualItem::updateState attachment.mAnimationTrack[1 - animationTrack].mUsedByThisTrack", attachment.mAnimationTrack[1 - animationTrack].mUsedByThisTrack);
               }
            #endif

            // Clear unused attachments or attachments unused by the master track that we're copying
            if(!attachment.getFlag(cFlagUsed) || !attachment.mAnimationTrack[1 - animationTrack].mUsedByThisTrack)
            {
               BMatrix finalMatrix;
               finalMatrix.makeIdentity();
               attachment.updateState(animationTrack, -1, getFlag(cFlagSynced), -1, -1, pState->mModelUVOffsets, -1, -1, NULL, NULL, NULL, cOriginVector, cOriginVector, applyInstantly, timeIntoAnimation, 0.0f, -1, reset, startOnThisAttachment, tintColor, finalMatrix);
               attachment.clearAttachmentData();
            }
            else
            {
               BMatrix finalMatrix;
               finalMatrix.mult(attachment.mMatrix, worldMatrix);
               attachment.updateState(animationTrack, &attachment, synced, animType, applyInstantly, timeIntoAnimation, tweenTime, newExitAction, newTweenToAnimation, newTweenTime, reset, startOnThisAttachment, clone, tintColor, finalMatrix, noRecursion);
            }
         }
      }
      else
      {
         // Update the attachment states. First attempt to find a matching attachment in the list. If no match
         // is found, use the first unused entry in the list. If no unused entries exist, add a new one.
         UTBitVector<cMaxAttachmentCount> attachmentUpdated;
         long updatedCount=0;
         long oldCount=mAttachments.getNumber();
         long newCount=pState->mAttachments.getNumber();
         BASSERT(newCount <= cMaxAttachmentCount);
         BASSERT(oldCount <= cMaxAttachmentCount);

         #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::updateState oldCount", oldCount);
               syncAnimData("BVisualItem::updateState newCount", newCount);
            }
         #endif

         for(long i=0; i<newCount; i++)
         {
            long oldIndex=-1;
            long inactiveIndex=-1;
            bool updating=false;
            BVisualItem& newAttachment=*(pState->mAttachments[i]);
            for(long j=0; j<oldCount; j++)
            {
               #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                  if (synced)
                  {
                     syncAnimData("BVisualItem::updateState j", j);
                     syncAnimData("BVisualItem::updateState attachmentUpdated.isSet(j)", attachmentUpdated.isSet(j));
                  }
               #endif

               if(attachmentUpdated.isSet(j))
                  continue;
//-- FIXING PREFIX BUG ID 7379
               const BVisualItem& oldAttachment=*(mAttachments[j]);
//--

               #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                  if (synced)
                  {
                     syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUser)", oldAttachment.getFlag(cFlagUser));
                  }
               #endif

               // Skip user attachments
               if(oldAttachment.getFlag(cFlagUser))
                  continue;

               #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                  if (synced)
                  {
                     syncAnimData("BVisualItem::updateState oldAttachment.mAnimationTrack[0].mUsedByThisTrack", oldAttachment.mAnimationTrack[0].mUsedByThisTrack);
                     syncAnimData("BVisualItem::updateState oldAttachment.mAnimationTrack[1].mUsedByThisTrack", oldAttachment.mAnimationTrack[1].mUsedByThisTrack);
                     syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUsed)", oldAttachment.getFlag(cFlagUsed));
                     syncAnimData("BVisualItem::updateState newAttachment.mModelAsset.mType", newAttachment.mModelAsset.mType);
                     syncAnimData("BVisualItem::updateState oldAttachment.mModelAsset.mType", oldAttachment.mModelAsset.mType);
                  }
               #endif

               // Skip attachments not used by either track
               if(!oldAttachment.mAnimationTrack[0].mUsedByThisTrack && !oldAttachment.mAnimationTrack[1].mUsedByThisTrack)
                  continue;
               if(!oldAttachment.getFlag(cFlagUsed))
               {
                  if(inactiveIndex==-1)
                     inactiveIndex=j;
               }
               if(newAttachment.mModelAsset.mType==oldAttachment.mModelAsset.mType && newAttachment.mModelAsset.mIndex==oldAttachment.mModelAsset.mIndex)
               {
                  // Use this matching attachment entry.
                  oldIndex=j;
                  attachmentUpdated.set(j);
                  updatedCount++;
                  if(newAttachment.mIndex==oldAttachment.mIndex)
                     updating=true;
                  break;
               }
            }

            #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
               if (synced)
               {
                  syncAnimData("BVisualItem::updateState oldIndex1", oldIndex);
                  syncAnimData("BVisualItem::updateState updating", updating);
                  syncAnimData("BVisualItem::updateState updatedCount", updatedCount);
                  syncAnimData("BVisualItem::updateState updateThisState", updateThisState);
               }
            #endif

            bool isNewAttachment = false;
            if(oldIndex==-1)
            {
               isNewAttachment = true;
               if(inactiveIndex!=-1)
               {
                  // Use the first unused attachment entry.
                  oldIndex=inactiveIndex;
                  attachmentUpdated.set(inactiveIndex);
                  updatedCount++;       
                  mAttachments[inactiveIndex]->releaseInstance(true);
               }
               else
               {
                  // Create a new attachment entry.
                  BVisualItem* pAttachment=getNewInstance(false);
                  if(pAttachment)
                  {
                     oldIndex=mAttachments.add(pAttachment);
                     if(oldIndex==-1)
                     {
                        BASSERTM(0, "BVisualItem::updateState - Failed to add attachment.");
                        BVisualItem::releaseInstance(pAttachment);
                     }
                  }
               }
            }

            #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
               if (synced)
               {
                  syncAnimData("BVisualItem::updateState oldIndex2", oldIndex);
                  syncAnimData("BVisualItem::updateState isNewAttachment", isNewAttachment);
               }
            #endif

            if(oldIndex!=-1)
            {
               BVisualItem& oldAttachment=*(mAttachments[oldIndex]);

               if(oldAttachment.mAnimationTrack[animationTrack].mIsLocked)
                  continue;

               if (updateThisState || isNewAttachment)
               {
                  oldAttachment.mFromBoneHandle=newAttachment.mFromBoneHandle;
                  oldAttachment.mToBoneHandle=newAttachment.mToBoneHandle;
                  oldAttachment.mAttachmentHandle=newAttachment.mAttachmentHandle;
                  oldAttachment.mAttachmentType=newAttachment.mAttachmentType;
                  oldAttachment.setFlag(cFlagUsed, true);
                  oldAttachment.setFlag(cFlagDisregardOrient, newAttachment.getFlag(cFlagDisregardOrient));

                  #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                     if (synced)
                     {
                        if (!BProtoVisual::mGenerationChanged)
                        {
                           const char* pName = gVisualManager.getAttachmentName(oldAttachment.mAttachmentHandle);

                           if (pName)
                              syncAnimData("BVisualItem::update oldAttachment.Name", pName);
                        }

                        syncAnimData("BVisualItem::updateState oldAttachment.mAttachmentType", oldAttachment.mAttachmentType);
                        syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUsed)", oldAttachment.getFlag(cFlagUsed));
                        syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagDisregardOrient)", oldAttachment.getFlag(cFlagDisregardOrient));
                        syncAnimData("BVisualItem::updateState newAttachment.getFlag(cFlagSyncAnims)", newAttachment.getFlag(cFlagSyncAnims));
                     }
                  #endif

                  
                  // Leave the existing attachment intact if updating and we're not syncing anims
                  if(!(updating && !newAttachment.getFlag(cFlagSyncAnims)))
                  {
                     oldAttachment.updateTransform(mpInstance);

                     BMatrix finalMatrix;
                     finalMatrix.mult(oldAttachment.mMatrix, worldMatrix);
                     oldAttachment.updateState(animationTrack, &newAttachment, synced, animType, applyInstantly, timeIntoAnimation, tweenTime, newExitAction, newTweenToAnimation, newTweenTime, reset, startOnThisAttachment, clone, tintColor, finalMatrix, noRecursion);
                  }

                  if(!updating)
                  {
                     // Only update these values if the attachment is not a matching entry that's being updated.
                     // Leaving this data alone for a matching entry allows dynamic data such as a user set
                     // transform to stay active.
                     oldAttachment.mpName=newAttachment.mpName;         
                     oldAttachment.mIndex=newAttachment.mIndex;
                     oldAttachment.mMatrix.makeIdentity();
                     oldAttachment.mTransform.makeIdentity();
                     oldAttachment.mOldTransform.makeIdentity();
                     oldAttachment.setFlag(cFlagUseTransform, false);
                     oldAttachment.setFlag(cFlagUser, false);

                     #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                        if (synced)
                        {
                           syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUsed)", oldAttachment.getFlag(cFlagUsed));
                           syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUser)", oldAttachment.getFlag(cFlagUser));
                        }
                     #endif
                  }
               }
               else
               {
                  BMatrix finalMatrix;
                  finalMatrix.mult(oldAttachment.mMatrix, worldMatrix);
                  oldAttachment.updateState(animationTrack, &newAttachment, synced, animType, applyInstantly, timeIntoAnimation, tweenTime, newExitAction, newTweenToAnimation, newTweenTime, reset, startOnThisAttachment, clone, tintColor, finalMatrix, noRecursion);
               }
            }
         }

         #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::updateState oldCount", oldCount);
            }
         #endif

         if (updateThisState && (updatedCount<oldCount))
         {
            // Release any old attachments that are no longer being used
            for(long i=0; i<oldCount; i++)
            {
               #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                  if (synced)
                  {
                     syncAnimData("BVisualItem::updateState i", i);
                     syncAnimData("BVisualItem::updateState attachmentUpdated.isSet(i)", attachmentUpdated.isSet(i));
                  }
               #endif


               if(attachmentUpdated.isSet(i))
                  continue;
               BVisualItem& oldAttachment=*(mAttachments[i]);

               #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
                  if (synced)
                  {
                     syncAnimData("BVisualItem::updateState oldAttachment.mAnimationTrack[animationTrack].mIsLocked", oldAttachment.mAnimationTrack[animationTrack].mIsLocked);
                     syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUser)", oldAttachment.getFlag(cFlagUser));
                     syncAnimData("BVisualItem::updateState oldAttachment.getFlag(cFlagUsed)", oldAttachment.getFlag(cFlagUsed));
                     syncAnimData("BVisualItem::updateState oldAttachment.mAnimationTrack[0].mUsedByThisTrack", oldAttachment.mAnimationTrack[0].mUsedByThisTrack);
                     syncAnimData("BVisualItem::updateState oldAttachment.mAnimationTrack[1].mUsedByThisTrack", oldAttachment.mAnimationTrack[1].mUsedByThisTrack);
                  }
               #endif

               // Skip locked attachments
               if(oldAttachment.mAnimationTrack[animationTrack].mIsLocked)
                  continue;
               // Skip user attachments
               if(oldAttachment.getFlag(cFlagUser))
                  continue;
               // Skip unused attachments
               if(!oldAttachment.getFlag(cFlagUsed))
                  continue;
               // Skip attachments not used by either track
               if(!oldAttachment.mAnimationTrack[0].mUsedByThisTrack && !oldAttachment.mAnimationTrack[1].mUsedByThisTrack)
                  continue;
               BMatrix finalMatrix;
               finalMatrix.makeIdentity();
               oldAttachment.updateState(animationTrack, -1, getFlag(cFlagSynced), -1, -1, pState->mModelUVOffsets, -1, -1, NULL, NULL, NULL, cOriginVector, cOriginVector, applyInstantly, timeIntoAnimation, 0.0f, -1, reset, startOnThisAttachment, tintColor, finalMatrix);
               oldAttachment.clearAttachmentData();
            }
         }

         #ifndef FAST_UPDATE_TRANSFORMS
            updateAttachmentTransforms(true);
         #else
            BSparsePoseHelper parentSP;
            updateTransformsRecurse(parentSP);
         #endif

         computeCombinedBoundingBox();
      }
   }
}

//==============================================================================
// BVisualItem::updateState
//==============================================================================
void BVisualItem::updateState(
   long animationTrack, long animType, bool synced, 
   long modelAssetType, long modelAssetIndex, const BVisualModelUVOffsets& modelUVOffsets, 
   long animAssetType, long animAssetIndex, 
   BProtoVisualTagArray* pTags, 
   BProtoVisualPointArray* pPoints, 
   BFloatProgression* pOpacityProgression, 
   BVector minCorner, BVector maxCorner, 
   bool applyInstantly, 
   float timeIntoAnimation, 
   float tweenTime, 
   long damageIndex, 
   bool reset, 
   BVisualItem* startOnThisAttachment, 
   DWORD tintColor,
   const BMatrix& worldMatrix)
{
   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (synced)
      {
         syncAnimData("BVisualItem::updateState animationTrack", animationTrack);
         syncAnimData("BVisualItem::applyAnimation anim", gVisualManager.getAnimName(animType));
         //syncAnimData("BVisualItem::updateState animType", animType);
         syncAnimData("BVisualItem::updateState synced", synced);
         syncAnimData("BVisualItem::updateState modelAssetType", modelAssetType);
         syncAnimData("BVisualItem::updateState animAssetType", animAssetType);
         syncAnimData("BVisualItem::updateState minCorner", minCorner);
         syncAnimData("BVisualItem::updateState maxCorner", maxCorner);
         syncAnimData("BVisualItem::updateState applyInstantly", applyInstantly);
         syncAnimData("BVisualItem::updateState timeIntoAnimation", timeIntoAnimation);
         syncAnimData("BVisualItem::updateState tweenTime", tweenTime);
         syncAnimData("BVisualItem::updateState reset", reset);
      }
   #endif

   BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   if (animType == -1)
   {
      animation.mIsClone = false;
      animation.mExitAction = cAnimExitActionLoop;
      animation.mTweenToAnimation = -1;
      animation.mTweenTime = 0.0f;
      animation.mIsDirty = true;
   }

   if (startOnThisAttachment == this)
   {
      startOnThisAttachment = NULL;
      #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
         if (getFlag(cFlagSynced))
         {
            syncAnimData("BVisualItem::updateState startOnThisAttachment = NULL", animationTrack);
         }
      #endif
   }

   #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
      if (getFlag(cFlagSynced))
      {
         syncAnimData("BVisualItem::updateState animation.mIsLocked", animation.mIsLocked);
      }
   #endif

   if (!animation.mIsLocked && !startOnThisAttachment)
   {
      if (animAssetIndex == -1)
         animation.mDuration = 0.0f;

      // Do not compare the uvoffsets to be different since if we do this we recreate the 
      // instance and loose the uvoffsets set by the damage file.  (SAT)

      //if(modelAssetType!=mModelAsset.mType || modelAssetIndex!=mModelAsset.mIndex) || mModelUVOffsets!=modelUVOffsets)
      //{
      //   releaseInstance(true);
      //   mModelAsset.mType=modelAssetType;
      //   mModelAsset.mIndex=modelAssetIndex;
      //   mModelUVOffsets=modelUVOffsets;
      //   createInstance(worldMatrix);
      //}
      if(modelAssetType!=mModelAsset.mType || modelAssetIndex!=mModelAsset.mIndex)
      {
         BASSERT(!animation.mIsClone);
         releaseInstance(true);
         mModelAsset.mType=modelAssetType;
         mModelAsset.mIndex=modelAssetIndex;
         createInstance(worldMatrix, tintColor);
      }

      if(animAssetType!=animation.mAnimAsset.mType || animAssetIndex!=animation.mAnimAsset.mIndex || reset)
      {
         animation.mAnimAsset.mType=animAssetType;
         animation.mAnimAsset.mIndex=animAssetIndex;

         #if defined DEBUG_INTRUSIVE_SYNCING && defined SYNC_Anim
            if (getFlag(cFlagSynced))
            {
               syncAnimData("BVisualItem::updateState animation.mAnimAsset.mType", animation.mAnimAsset.mType);
            }
         #endif

         applyAnimation(animationTrack, animType, synced, applyInstantly, timeIntoAnimation, tweenTime, startOnThisAttachment);
      }
      else if (animation.mAnimType != animType)
      {
         animation.mPosition = timeIntoAnimation * animation.mDuration;

         #ifdef SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::updateState mPosition", animation.mPosition);
            }
         #endif
      }

      animation.mpTags = ((!animation.mIsClone && (animation.mDuration > 0.0f) && pTags) ? pTags : NULL);
      animation.mpPoints = pPoints;
      animation.mpOpacityProgression = pOpacityProgression;

      if (!animation.mIsClone)
      {
         mMinCorner=minCorner;
         mMaxCorner=maxCorner;
         mDamageTemplateIndex = damageIndex;

         #ifdef SYNC_Anim
            if (synced)
            {
               syncAnimData("BVisualItem::updateState mMinCorner", mMinCorner);
               syncAnimData("BVisualItem::updateState mMaxCorner", mMaxCorner);
            }
         #endif
      }

      animation.mAnimType=animType;

      // This track is now using this visual item
      animation.mUsedByThisTrack=true;
   }
}

//==============================================================================
// BVisualItem::resetCombinedBoundingBox
//==============================================================================
void BVisualItem::resetCombinedBoundingBox()
{
   if (mpInstance && (mModelAsset.mType == cVisualAssetGrannyModel))
      mpInstance->computeBoundingBox(&mMinCorner, &mMaxCorner, true);
   mCombinedMinCorner = mMinCorner;
   mCombinedMaxCorner = mMaxCorner;

   long count = mAttachments.getNumber();
   for (long i = 0; i < count; i++)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if (pAttachment->getFlag(cFlagUsed))
         pAttachment->resetCombinedBoundingBox();
   }
}

//==============================================================================
// BVisualItem::computeCombinedBoundingBox
//==============================================================================
void BVisualItem::computeCombinedBoundingBox()
{
   mCombinedMinCorner=mMinCorner;
   mCombinedMaxCorner=mMaxCorner;
   computeCombinedBoundingBox(&mCombinedMinCorner, &mCombinedMaxCorner, false);
}

//==============================================================================
// BVisualItem::computeCombinedBoundingBox
//==============================================================================
void BVisualItem::computeCombinedBoundingBox(BVector* pMinCorner, BVector* pMaxCorner, bool attachment)
{
   //SCOPEDSAMPLE(BVisualItem_computeCombinedBoundingBox);
   if(attachment)
   {
      BBoundingBox box;
      box.initializeTransformed(mMinCorner, mMaxCorner, mMatrix);

      BVector minPoint, maxPoint;
      box.computeWorldCorners(minPoint, maxPoint);

      pMinCorner->x=min(pMinCorner->x, minPoint.x);
      pMinCorner->y=min(pMinCorner->y, minPoint.y);
      pMinCorner->z=min(pMinCorner->z, minPoint.z);
      pMaxCorner->x=max(pMaxCorner->x, maxPoint.x);
      pMaxCorner->y=max(pMaxCorner->y, maxPoint.y);
      pMaxCorner->z=max(pMaxCorner->z, maxPoint.z);
   }

   long count=mAttachments.getNumber();
   if(count>0)
   {
      for(long i=0; i<count; i++)
      {
         BVisualItem* pAttachment=mAttachments[i];
         if(!pAttachment->getFlag(cFlagUsed))
            continue;
         pAttachment->computeCombinedBoundingBox(pMinCorner, pMaxCorner, true);
      }
   }
}

//==============================================================================
// BVisualItem::getNumberPoints
//==============================================================================
long BVisualItem::getNumberPoints(long animationTrack, long pointType) const
{
   const BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   long total=0;
   if(animation.mpPoints)
   {
      long pointCount=animation.mpPoints->getNumber();
      for(long i=0; i<pointCount; i++)
      {
         const BProtoVisualPoint& point=(*(animation.mpPoints))[i];
         if(point.mPointType==pointType)
            total++;
      }
   }
   long attachmentCount=mAttachments.getNumber();
   for(long i=0; i<attachmentCount; i++)
   {
      const BVisualItem& attachment=*(mAttachments[i]);
      if(attachment.getFlag(cFlagUsed))
         total+=attachment.getNumberPoints(animationTrack, pointType);
   }
   return total;
}

//==============================================================================
// BVisualItem::getPointHandle
//==============================================================================
long BVisualItem::getPointHandle(long animationTrack, long pointType) const
{
   return getNextPointHandle(animationTrack, -1, pointType);
}

//==============================================================================
// BVisualItem::getNextPointHandle
//==============================================================================
long BVisualItem::getNextPointHandle(long animationTrack, long pointHandle, long pointType) const
{
   long item=0;
   long point=-1;

   if(pointHandle!=-1)
   {
      item=ITEMFROMPOINTHANDLE(pointHandle);
      point=POINTFROMPOINTHANDLE(pointHandle);
   }

   long itemCounter=0;
   return getNextPointHandle(animationTrack, item, point, pointType, itemCounter);
}

//==============================================================================
// BVisualItem::getNextPointHandle
//==============================================================================
long BVisualItem::getNextPointHandle(long animationTrack, long& item, long point, long pointType, long& itemCounter) const
{
   const BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   if(item==itemCounter)
   {
      if(animation.mpPoints)
      {
         long pointCount=animation.mpPoints->getNumber();
         for(long i=point+1; i<pointCount; i++)
         {
//-- FIXING PREFIX BUG ID 7381
            const BProtoVisualPoint& point=(*(animation.mpPoints))[i];
//--
            if(point.mPointType==pointType)
            {
               long pointHandle=POINTHANDLE(item, i);
               return pointHandle;
            }
         }
      }
      item++;
      point=-1;
   }

   long attachmentCount=mAttachments.getNumber();
   for(long i=0; i<attachmentCount; i++)
   {
      itemCounter++;
//-- FIXING PREFIX BUG ID 7382
      const BVisualItem* pAttachment=mAttachments[i];
//--
      if(pAttachment->getFlag(cFlagUsed))
      {
         long pointHandle=pAttachment->getNextPointHandle(animationTrack, item, point, pointType, itemCounter);
         if(pointHandle!=-1)
            return pointHandle;
      }
   }

   return -1;
}

//==============================================================================
// BVisualItem::getRandomPoint
//==============================================================================
long BVisualItem::getRandomPoint(long animationTrack, long pointType) const
{
   const long cMaxPoints=64;
   long pointHandles[cMaxPoints];
   long numPoints = 0;
   long pointHandle = -1;

   // Create a list of point handles
   for (;;)
   {
      pointHandle = getNextPointHandle(animationTrack, pointHandle, pointType);
      if (pointHandle == -1)
         break;

      pointHandles[numPoints++] = pointHandle;
      if(numPoints==cMaxPoints)
         break;
   }

   // Select one at random
   if(numPoints > 0)
   {
      long index;
      //if(getFlag(cFlagSynced))
         index = gVisualManager.getRandomValue((getFlag(cFlagSynced)?cSimRand:cUnsyncedRand), 0, numPoints - 1);
      //else
      //   index = gVisualManager.getRandomValue(cUnsyncedRand, 0, numPoints - 1);
      return pointHandles[index];
   }

   return -1;
}

//==============================================================================
// BVisualItem::getPointPosition
//==============================================================================
bool BVisualItem::getPointPosition(long animationTrack, long pointHandle, BVector& position, BMatrix *pMatrixOut) const
{
   BMatrix mat;
   const BVisualItem* pItem=getPointItem(pointHandle, &mat);
   if(!pItem)
      return false;

   const BVisualAnimationData &itemAnimation = pItem->mAnimationTrack[animationTrack];
   if (!itemAnimation.mpPoints)
      return false;

   long index=POINTFROMPOINTHANDLE(pointHandle);
   if(index<0 || index>=itemAnimation.mpPoints->getNumber())
      return false;

//-- FIXING PREFIX BUG ID 7384
   const BProtoVisualPoint& point=itemAnimation.mpPoints->get(index);
//--
   return pItem->getBone(point.mBoneHandle, &position, pMatrixOut, NULL, (pItem==this ? NULL : &mat));
}

//==============================================================================
// BVisualItem::getPointData
//==============================================================================
long BVisualItem::getPointData(long animationTrack, long pointHandle) const
{
   const BVisualItem* pItem=getPointItem(pointHandle);
   if(!pItem)
      return -1;

   const BVisualAnimationData &itemAnimation = pItem->mAnimationTrack[animationTrack];
   if (!itemAnimation.mpPoints)
      return -1;

   long index=POINTFROMPOINTHANDLE(pointHandle);
   if(index<0 || index>=itemAnimation.mpPoints->getNumber())
      return -1;

   BVector pos;
//-- FIXING PREFIX BUG ID 7385
   const BProtoVisualPoint& point=itemAnimation.mpPoints->get(index);
//--
   return point.mPointDataValue;
}

//==============================================================================
// BVisualItem::getPointProto
//==============================================================================
const BProtoVisualPoint* BVisualItem::getPointProto(long animationTrack, long pointHandle) const
{
   const BVisualItem* pItem=getPointItem(pointHandle);
   if(!pItem)
      return NULL;

   const BVisualAnimationData &itemAnimation = pItem->mAnimationTrack[animationTrack];
   if (!itemAnimation.mpPoints)
      return NULL;

   long index=POINTFROMPOINTHANDLE(pointHandle);
   if(index<0 || index>=itemAnimation.mpPoints->getNumber())
      return NULL;

   BVector pos;
//-- FIXING PREFIX BUG ID 7386
   const BProtoVisualPoint& point=itemAnimation.mpPoints->get(index);
//--
   return &point;
}

//==============================================================================
// BVisualItem::getPointItem
//==============================================================================
const BVisualItem* BVisualItem::getPointItem(long pointHandle, BMatrix* pMatrixOut) const
{
   if(pointHandle==-1)
      return NULL;

   long item=0;
   long point=-1;

   if(pointHandle!=-1)
   {
      item=ITEMFROMPOINTHANDLE(pointHandle);
      point=POINTFROMPOINTHANDLE(pointHandle);
   }

   long itemCounter=0;
   return getPointItem(item, point, itemCounter, pMatrixOut);
}

//==============================================================================
// BVisualItem::getPointItem
//==============================================================================
const BVisualItem* BVisualItem::getPointItem(long& item, long point, long& itemCounter, BMatrix* pMatrixOut) const
{
   if(item==itemCounter)
   {
      if(pMatrixOut)
         *pMatrixOut=mMatrix;
      return this;
   }

   long attachmentCount=mAttachments.getNumber();
   for(long i=0; i<attachmentCount; i++)
   {
      itemCounter++;
//-- FIXING PREFIX BUG ID 7387
      const BVisualItem* pAttachment=mAttachments[i];
//--
      if(pAttachment->getFlag(cFlagUsed))
      {
         const BVisualItem* pItem=pAttachment->getPointItem(item, point, itemCounter, pMatrixOut);
         if(pItem!=NULL)
         {
            if(pMatrixOut)
               pMatrixOut->mult(*pMatrixOut, mMatrix);
            return pItem;
         }
      }
   }

   return NULL;
}

//==============================================================================
// BVisualItem::getAttachmentHandle
//==============================================================================
long BVisualItem::getAttachmentHandle(const char* pAttachmentName)
{
   long count=mAttachments.getNumber();
   for(long i=0; i<count; i++)
   {
      BVisualItem* pAttachment=mAttachments[i];
      if(pAttachment->getFlag(cFlagUsed))
      {
         long handle=pAttachment->getAttachmentHandle(pAttachmentName);
         if(handle!=-1)
            return handle;
         if(pAttachment->mpName && *(pAttachment->mpName)==pAttachmentName)
            return pAttachment->mAttachmentHandle;
      }
   }
   return -1;
}

//==============================================================================
// BVisualItem::getAttachmentByToBoneName
//==============================================================================
BVisualItem* BVisualItem::getAttachmentByToBoneName(const char* pToBoneName) const
{
   long toBoneHandle = getBoneHandle(pToBoneName);
   if (toBoneHandle == -1)
      return NULL;

   long count=mAttachments.getNumber();
   for(long i=0; i<count; i++)
   {
      BVisualItem* pAttachment=mAttachments[i];
      if (pAttachment->getFlag(cFlagUsed))
      {
         if (pAttachment->mToBoneHandle == toBoneHandle)
            return pAttachment;
      }
   }
   return NULL;
}

//==============================================================================
BVisualItem* BVisualItem::getAttachmentByData(long visualAssetType, long data0) const
{
   long count=mAttachments.getNumber();
   for(long i=0; i<count; i++)
   {
      BVisualItem* pAttachment=mAttachments[i];
      if (pAttachment->mAttachmentType == visualAssetType && pAttachment->mData == data0)
         return pAttachment;
   }
   return NULL;
}

//==============================================================================
// BVisualItem::getAttachment
//==============================================================================
BVisualItem* BVisualItem::getAttachment(long attachmentHandle, BMatrix* pTransformedMatrixOut, BMatrix* pMatrixOut)
{
   const BVisualItem* thisPtr=this;
   return const_cast<BVisualItem*>(thisPtr->getAttachment(attachmentHandle, pTransformedMatrixOut, pMatrixOut));
}

//==============================================================================
// BVisualItem::getAttachment
//==============================================================================
const BVisualItem* BVisualItem::getAttachment(long attachmentHandle, BMatrix* pTransformedMatrixOut, BMatrix* pMatrixOut) const
{
   if(attachmentHandle==-1)
      return NULL;
   else
   {
      long count=mAttachments.getNumber();
      for(long i=0; i<count; i++)
      {
         const BVisualItem* pAttachment=mAttachments[i];
         if(!pAttachment->getFlag(cFlagUsed))
            continue;
         //if(reinterpret_cast<long>(pAttachment)==attachmentHandle)
         if(pAttachment->mAttachmentHandle==attachmentHandle)
         {
            if(pTransformedMatrixOut)
               pTransformedMatrixOut->mult(pAttachment->mMatrix, mMatrix);
            if(pMatrixOut)
               *pMatrixOut = mMatrix;
            return pAttachment;
         }
         const BVisualItem* pSubAttachment=pAttachment->getAttachment(attachmentHandle, pTransformedMatrixOut, pMatrixOut);
         if(pSubAttachment!=NULL)
         {
            if(pTransformedMatrixOut)
               pTransformedMatrixOut->mult(*pTransformedMatrixOut, mMatrix);
            if(pMatrixOut)
               pMatrixOut->mult(*pMatrixOut, mMatrix);
            return pSubAttachment;
         }
      }
      return NULL;
   }
}

//==============================================================================
// BVisualItem::getAttachmentFromBoneHandle
//==============================================================================
long BVisualItem::getAttachmentFromBoneHandle(long attachmentHandle)
{
//-- FIXING PREFIX BUG ID 7388
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
//--
   if(pAttachment)
      return pAttachment->mFromBoneHandle;
   else
      return -1;
}


//==============================================================================
// BVisualItem::getAttachmentToBoneHandle
//==============================================================================
long BVisualItem::getAttachmentToBoneHandle(long attachmentHandle)
{
//-- FIXING PREFIX BUG ID 7389
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
//--
   if(pAttachment)
      return pAttachment->mToBoneHandle;
   else
      return -1;
}


//==============================================================================
// BVisualItem::getAttachmentTransform
//==============================================================================
bool BVisualItem::getAttachmentTransform(long attachmentHandle, BMatrix& matrix)
{
//-- FIXING PREFIX BUG ID 7390
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
//--
   if(pAttachment)
   {
      matrix=pAttachment->mTransform;
      return true;
   }
   else
      return false;
}

//==============================================================================
//==============================================================================
void BVisualItem::setTransform(BMatrix matrix)
{
   if(!getFlag(cFlagTransformUpdated))
   {
      mOldTransform = mTransform;
      setFlag(cFlagTransformUpdated, true);
   }
   mTransform = matrix;
   setFlag(cFlagUseTransform, true);
}


//==============================================================================
//==============================================================================
void BVisualItem::clearTransform()
{
   // jce [11/6/2008] -- currently clearing transform isn't going to interpolate out properly.
   // It would need to render frames for another update and then make useTransform false.
   mTransform.makeIdentity();
   mOldTransform.makeIdentity();
   setFlag(cFlagUseTransform, false);
}


//==============================================================================
// BVisualItem::setAttachmentTransform
//==============================================================================
void BVisualItem::setAttachmentTransform(long attachmentHandle, BMatrix matrix)
{
   BVisualItem* pAttachment=getAttachment(attachmentHandle);
   if(pAttachment)
      pAttachment->setTransform(matrix);
}

//==============================================================================
// BVisualItem::clearAttachmentTransform
//==============================================================================
void BVisualItem::clearAttachmentTransform(long attachmentHandle)
{
   BVisualItem* pAttachment=getAttachment(attachmentHandle);
   if(pAttachment)
      pAttachment->clearTransform();
}

//==============================================================================
// BVisualItem::getAttachmentAnimationType
//==============================================================================
long BVisualItem::getAttachmentAnimationType(long animationTrack, long attachmentHandle) const
{
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
   if(pAttachment)
      return pAttachment->mAnimationTrack[animationTrack].mAnimType;
   else
      return -1;
}

//==============================================================================
// BVisualItem::getAttachmentAnimationDuration
//==============================================================================
float BVisualItem::getAttachmentAnimationDuration(long animationTrack, long attachmentHandle) const
{
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
   if(pAttachment)
      return pAttachment->mAnimationTrack[animationTrack].mDuration;
   else
      return 0.0f;
}

//==============================================================================
// BVisualItem::getAttachmentAnimationPosition
//==============================================================================
float BVisualItem::getAttachmentAnimationPosition(long animationTrack, long attachmentHandle) const
{
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
   if(pAttachment)
      return pAttachment->mAnimationTrack[animationTrack].mPosition;
   else
      return 0.0f;
}

//==============================================================================
// BVisualItem::getAttachmentAnimationClock
//==============================================================================
float BVisualItem::getAttachmentAnimationClock(long animationTrack, long attachmentHandle) const
{
   const BVisualItem* pAttachment=getAttachment(attachmentHandle);
   if(!pAttachment)
      return 0.0f;
   if(pAttachment->mpInstance && pAttachment->mModelAsset.mType==cVisualAssetGrannyModel && pAttachment->mAnimationTrack[animationTrack].mAnimAsset.mType==cVisualAssetGrannyAnim)
   {
//-- FIXING PREFIX BUG ID 7392
      const BGrannyInstance* pGrannyInstance=(BGrannyInstance*)(pAttachment->mpInstance);
//--
      return pGrannyInstance->getAnimClock();
   }
   return 0.0f;
}

//==============================================================================
// BVisualItem::processAttachmentEvent
//==============================================================================
bool BVisualItem::processAttachmentEvent(long animationTrack, long animType, long eventType, BProtoVisualTag* pTag, float tweenTime, DWORD tintColor, const BMatrix& worldMatrix)
{
   BDEBUG_ASSERT(pTag!=NULL);

   if ((eventType == cAnimEventParticles) || (eventType == cAnimEventLight))
   {
      long visualAssetType = -1;
      switch(eventType)
      {
         case cAnimEventParticles:
            if (gConfig.isDefined(cConfigNoParticles))
               return false;
            visualAssetType = cVisualAssetParticleSystem; 
            break;

         case cAnimEventLight:
            visualAssetType = cVisualAssetLight; 
            break;
      }

      return addAttachment(visualAssetType, animationTrack, animType, tweenTime, pTag->mData0, pTag->mToBoneHandle, pTag->mLifespan, pTag->getFlag(BProtoVisualTag::cFlagDiregardOrient), tintColor, worldMatrix) >= 0;
   }
   return false;

}

//==============================================================================
// BVisualItem::addAttachment
//==============================================================================
int BVisualItem::addAttachment(long visualAssetType, long animationTrack, long animType, float tweenTime, int data0, long toBoneHandle, float lifespan, bool bDisregardOrient, DWORD tintColor, const BMatrix& worldMatrix, const BMatrix *transformMatrix, bool bIsUserAttachment)
{
   uint count = mAttachments.getNumber();
   BDEBUG_ASSERT(count <= cMaxAttachmentCount);
   int attachmentIndex = -1;

   //-- all existing slots are being used.  Add another slot if we have room left
   //-- current max is 64 right now      
   if ((count < cMaxAttachmentCount) && (attachmentIndex <= -1))
   {
      BVisualItem* pAttachment=getNewInstance(bIsUserAttachment);
      if(pAttachment)
      {
         attachmentIndex=mAttachments.add(pAttachment);
         if(attachmentIndex==-1)
         {
            BASSERTM(0, "BVisualItem::addAttachment - Failed to add attachment.");
            BVisualItem::releaseInstance(pAttachment);
         }
      }
   }

   //-- we could not find a slot or create a new one -- don't launch the effect
   if (attachmentIndex <= -1)
      return -1;

   BVisualItem& newAttachment=*(mAttachments[attachmentIndex]);

   BVector minCorner(-0.1f,-0.1f,-0.1f);
   BVector maxCorner(0.1f,0.1f,0.1f);

   newAttachment.mAttachmentType = visualAssetType;
   newAttachment.mData = data0;
   newAttachment.mFromBoneHandle=-1;
   newAttachment.mToBoneHandle=toBoneHandle;
   newAttachment.setFlag(cFlagUsed, true);
   newAttachment.setFlag(cFlagDisregardOrient, bDisregardOrient);

   if(lifespan != -1)
   {
      newAttachment.setFlag(cFlagUseLifespan, true);
      newAttachment.setFlag(cFlagJustCreated, true);
      newAttachment.mLifespan = lifespan;
   }
   else
   {
      newAttachment.setFlag(cFlagUseLifespan, false);
   }

   if(transformMatrix != NULL)
   {
      newAttachment.mTransform = *transformMatrix;
      newAttachment.mOldTransform = *transformMatrix;
      newAttachment.setFlag(cFlagUseTransform, true);
   }

   newAttachment.updateTransform(mpInstance);
               
   BMatrix finalMatrix;
   finalMatrix.mult(newAttachment.mMatrix, worldMatrix);

   // SLB: This is only called when particles and lights are added so it's OK if we don't check for clones here.
   BASSERT((visualAssetType == cVisualAssetParticleSystem) || (visualAssetType == cVisualAssetLight));
   // Update the attachment item state.
   newAttachment.updateState(animationTrack, animType, getFlag(cFlagSynced), 
      visualAssetType, data0, BVisualModelUVOffsets(), -1, -1, NULL, NULL, NULL, minCorner, maxCorner, false, 0.0f, tweenTime, -1, false, false, tintColor, finalMatrix);

   return attachmentIndex;
}


//==============================================================================
// BVisualItem::getDamageTemplateID
//==============================================================================
long BVisualItem::getDamageTemplateID() const
{
   return(mDamageTemplateIndex);
}


//==============================================================================
// BVisualItem::setNumIKNodes
//==============================================================================
bool BVisualItem::setNumIKNodes(long numNodes)
{
   if (mpInstance)
   {
      return mpInstance->setNumIKNodes(numNodes);
   }
   else
   {
      return false;
   }
}


//==============================================================================
// BVisualItem::setIKNode
//==============================================================================
void BVisualItem::setIKNode(long node, BVector targetPos)
{
   if (mpInstance)
   {
      return mpInstance->setIKNode(node, targetPos);
   }
}


//==============================================================================
// BVisualItem::setIKNode
//==============================================================================
void BVisualItem::setIKNode(long node, long boneHandle, uint8 linkCount, BVector targetPos, uint8 nodeType)
{
   if (mpInstance)
   {
      mpInstance->setIKNode(node, boneHandle, linkCount, targetPos, nodeType);
   }
}


//==============================================================================
//==============================================================================
BIKNode* BVisualItem::getIKNodeFromTypeBoneHandle(long type, long boneHandle)
{
   if (mpInstance)
      return mpInstance->getIKNodeFromTypeBoneHandle(type, boneHandle);
   else
      return NULL;
}

//==============================================================================
//==============================================================================
BIKNode* BVisualItem::getIKNodeFromIndex(long node)
{
   if (mpInstance)
      return mpInstance->getIKNodeFromIndex(node);
   else
      return NULL;
}

//==============================================================================
// BVisualItem::lockIKNodeToGround
//==============================================================================
void BVisualItem::lockIKNodeToGround(long boneHandle, bool lock, float start, float end)
{
   if (mpInstance)
   {
      mpInstance->lockIKNodeToGround(boneHandle, lock, start, end);
   }
}


//==============================================================================
// BVisualItem::getIKNodeBoneHandle
//==============================================================================
long BVisualItem::getIKNodeBoneHandle(long node) const
{
   if (mpInstance)
   {
      return mpInstance->getIKNodeBoneHandle(node);
   }

   return -1;
}


//==============================================================================
// BVisualItem::getIKNodeAnchor
//==============================================================================
bool BVisualItem::getIKNodeAnchor(long node, BVector &anchorPos, float& lockStartTime, float& lockEndTime, bool& lockComplete) const
{
   if (mpInstance)
   {
      return mpInstance->getIKNodeAnchor(node, anchorPos, lockStartTime, lockEndTime, lockComplete);
   }

   return false;
}


//==============================================================================
//==============================================================================
void BVisualItem::setIKNodeLockComplete(long node, bool lockComplete)
{
   if (mpInstance)
   {
      return mpInstance->setIKNodeLockComplete(node, lockComplete);
   }
}

//==============================================================================
// BVisualItem::setIKNodeSweetSpot
//==============================================================================
void BVisualItem::setIKNodeSweetSpot(long boneHandle, BVector sweetSpotPos, float start, float sweetSpot, float end)
{
   if (mpInstance)
   {
      mpInstance->setIKNodeSweetSpot(boneHandle, sweetSpotPos, start, sweetSpot, end);
   }
}


//==============================================================================
// BVisualItem::getIKNodeSweetSpot
//==============================================================================
bool BVisualItem::getIKNodeSweetSpot(long node, BVector &sweetSpotPos, float &start, float &sweetSpot, float &end)
{
   if (mpInstance)
   {
      return mpInstance->getIKNodeSweetSpot(node, sweetSpotPos, start, sweetSpot, end);
   }

   return false;
}


//==============================================================================
// BVisualItem::isIKNodeActive
//==============================================================================
bool BVisualItem::isIKNodeActive(long node) const
{
   if (mpInstance)
   {
      return mpInstance->isIKNodeActive(node);
   }

   return false;
}


//==============================================================================
// BVisualItem::setIKNodeActive
//==============================================================================
void BVisualItem::setIKNodeActive(long node, bool active)
{
   if (mpInstance)
   {
      mpInstance->setIKNodeActive(node, active);
   }
}


//==============================================================================
// BVisualItem::setIKNodeSingleBone
//==============================================================================
void BVisualItem::setIKNodeSingleBone(long node, BVector position, BQuaternion orientation)
{
   if (mpInstance)
   {
      mpInstance->setIKNodeSingleBone(node, position, orientation);
   }
}


//==============================================================================
// BVisualItem::getIKNodeSingleBone
//==============================================================================
bool BVisualItem::getIKNodeSingleBone(long node, BVector &position, BQuaternion &orientation)
{
   if (mpInstance)
   {
      return mpInstance->getIKNodeSingleBone(node, position, orientation);
   }

   return false;
}


//==============================================================================
// BVisualItem::setAnimationLock
//==============================================================================
void BVisualItem::setAnimationLock(long animationTrack, bool lock)
{
   mAnimationTrack[animationTrack].mIsLocked = lock;

   // Do the same thing for the attachments
   uint i;
   for (i = 0; i < mAttachments.getSize(); i++)
   {
      BVisualItem* pAttachment = mAttachments[i];
      pAttachment->setAnimationLock(animationTrack, lock);
   }
}


//==============================================================================
// BVisualItem::setAnimationRate
//==============================================================================
void BVisualItem::setAnimationRate(long animationTrack, float animationRate)
{
   BVisualAnimationData &animation = mAnimationTrack[animationTrack];

   if (!mAnimationTrack[animationTrack].mIsLocked)
   {
      //CLM [04.30.08] removed fcmp
      if(!FastFloat::compareEqual(animation.mAnimationRate,animationRate))
      // origional
      //if (animation.mAnimationRate != animationRate)
      {
         mAnimationTrack[animationTrack].mAnimationRate = animationRate;

         if (animation.mAnimAsset.mType == cVisualAssetGrannyAnim)
         {
            if ((mModelAsset.mType == cVisualAssetGrannyModel) && mpInstance)
            {
               BGrannyInstance* pGrannyInstance = (BGrannyInstance*) mpInstance;
               pGrannyInstance->setAnimationRate(animationTrack, animationRate);
            }
         }
      }
   }

   //setAttachmentAnimationRate(animationTrack, animationRate);
   // Do the same thing for the attachments
   for (uint i = 0; i < mAttachments.getSize(); i++)
   {
      BVisualItem* pAttachment = mAttachments[i];
      // ajl 7/23/08 - cFlagSyncAnims isn't being set for some reason so ignore for now
      //if (pAttachment->getFlag(cFlagUsed) && pAttachment->getFlag(cFlagSyncAnims))
      if (pAttachment->getFlag(cFlagUsed))
         pAttachment->setAnimationRate(animationTrack, animationRate);
   }
}


//==============================================================================
// BVisualItem::setAttachmentAnimationRate
//==============================================================================
void BVisualItem::setAttachmentAnimationRate(long animationTrack, float animationRate)
{
   if (!mAnimationTrack[animationTrack].mIsLocked)
   {
      mAnimationTrack[animationTrack].mAnimationRate = animationRate;

      // Do the same thing for the attachments
      uint i;
      for (i = 0; i < mAttachments.getSize(); i++)
      {
         BVisualItem* pAttachment = mAttachments[i];
         pAttachment->setAttachmentAnimationRate(animationTrack, animationRate);
      }
   }
}


//==============================================================================
// BVisualAnimationData::init
//==============================================================================
void BVisualAnimationData::init()
{
   mAnimType = -1;
   mAnimAsset.mIndex = -1;
   mAnimAsset.mType = -1;
   mpTags = NULL;
   mpPoints = NULL;
   mpOpacityProgression = NULL;
   mPosition = 0.0f;
   mDuration = 0.0f;
   mExitAction = cAnimExitActionLoop;
   mTweenToAnimation = -1;
   mTweenTime = 0.0f;
   mpTweenOutTags = NULL;
   mTweenOutPosition = 0.0f;
   mTweenOutTimeLeft = 0.0f;
   mTweenOutDuration = 0.0f;
   mTweenOutAnimAssetType = -1;
   mAnimationRate = 1.0f;
   mIsClone = false;
   mIsLocked = false;
   mIsDirty = true;
   mUsedByThisTrack = false;
}

//==============================================================================
// BVisualAnimationData::init
//==============================================================================
void BVisualAnimationData::init(const BVisualAnimationData &pSource)
{
/*
   mAnimType = -1;
   mAnimAsset.mIndex = -1;
   mAnimAsset.mType = -1;
   mExitAction = pSource.mExitAction;
   mTweenTime = pSource.mTweenTime;
   mTweenToAnimation = pSource.mTweenToAnimation;
   mTweenOutAnimAssetType = -1;
   mAnimationRate = pSource.mAnimationRate;
   mIsClone = pSource.mIsClone;
*/   
   mAnimType = -1;
   mAnimAsset.mIndex = -1;
   mAnimAsset.mType = -1;
   mpTags = NULL;
   mpPoints = NULL;
   mpOpacityProgression = NULL;
   mPosition = 0.0f;
   mDuration = 0.0f;
   mExitAction = pSource.mExitAction;
   mTweenToAnimation = pSource.mTweenToAnimation;
   mTweenTime = pSource.mTweenTime;
   mpTweenOutTags = NULL;
   mTweenOutPosition = 0.0f;
   mTweenOutTimeLeft = 0.0f;
   mTweenOutDuration = 0.0f;
   mTweenOutAnimAssetType = -1;
   mAnimationRate = pSource.mAnimationRate;
   mIsClone = pSource.mIsClone;
   mIsLocked = pSource.mIsLocked;
   mIsDirty = true;
   mUsedByThisTrack = pSource.mUsedByThisTrack;
}

//==============================================================================
// BVisualAnimationData::deinit
//==============================================================================
void BVisualAnimationData::deinit()
{
   mAnimType = -1;
   mAnimAsset.mIndex = -1;
   mAnimAsset.mType = -1;
   mpTags = NULL;
   mpPoints = NULL;
   mpOpacityProgression = NULL;
   mPosition = 0.0f;
   mDuration = 0.0f;
   mExitAction = cAnimExitActionLoop;
   mTweenToAnimation = -1;
   mTweenTime = 0.0f;
   mpTweenOutTags = NULL;
   mTweenOutPosition = 0.0f;
   mTweenOutTimeLeft = 0.0f;
   mTweenOutDuration = 0.0f;
   mTweenOutAnimAssetType = -1;
   mAnimationRate = 1.0f;
   mIsClone = false;
   mIsLocked = false;
   mIsDirty = true;
   mUsedByThisTrack = false;
}

//==============================================================================
// BVisualAnimationData::update
//==============================================================================
void BVisualAnimationData::update(long animationTrack, BVisualItem* pVisualItem, float elapsedTime, BVisual* pVisual, long attachmentHandle, 
                                  bool sendLoopEvent, bool &chain, float &timeIntoChainedAnimation, DWORD tintColor, const BMatrix& worldMatrix)
{
   //SCOPEDSAMPLE(BVisualAnimationData_update)
   mIsDirty = false;
   timeIntoChainedAnimation = 0.0f;
   chain = false;

   BDEBUG_ASSERT(Math::IsValidFloat(elapsedTime));
   BDEBUG_ASSERT(Math::IsValidFloat(mAnimationRate));
   BDEBUG_ASSERT(Math::IsValidFloat(mPosition));

   if (pVisualItem->mModelAsset.mType == cVisualAssetGrannyModel)
   {
      BDEBUG_ASSERT((mExitAction == cAnimExitActionFreeze) || (mAnimationRate >= 0.0f)); // Support negative animation rates for freeze exit action only

      // Scale by animation rate
      elapsedTime *= mAnimationRate;
   }

   float oldPosition = mPosition;
   mPosition+=elapsedTime;
   
   BDEBUG_ASSERT(Math::IsValidFloat(mPosition));

   if (mExitAction == cAnimExitActionLoop)
   {
      //CLM [04.30.08] Removal of FCMP
      const float test0 =  mPosition - mDuration;
      const float test1 =  Math::fAbs(mDuration) - cFloatCompareEpsilon;
      const float test1Res = static_cast<float>(Math::fSelect(test1,fmod(mPosition, mDuration), 0.0f));
      mPosition = static_cast<float>(Math::fSelect(test0,test1Res,mPosition));
    

      //origional
    /*  if (mPosition >= mDuration)
         mPosition = (Math::fAbs(mDuration) > cFloatCompareEpsilon) ? fmod(mPosition, mDuration) : 0.0f;*/
   }
   else //if (mExitAction == cAnimExitActionFreeze)
      mPosition = Math::Clamp(mPosition, 0.0f, mDuration);

   BDEBUG_ASSERT(Math::IsValidFloat(mPosition));

   #ifdef SYNC_Anim
      if (pVisualItem->getFlag(BVisualItem::cFlagSynced))
      {
         syncAnimData("BVisualAnimationData::update mPosition", mPosition);
      }
   #endif
   
   // Generate animation tag events
   if((mExitAction == cAnimExitActionTransition) || mpTags || (sendLoopEvent && (!FastFloat::compareZero(mDuration)) && (mAnimAsset.mType != -1)))
   {
      if (mExitAction == cAnimExitActionTransition)
      {
         // SLB: This fixes missed tags at the start of a chained animation
         float tweenMarker = mDuration;// - mTweenTime;
         //timeIntoChainedAnimation = (mPosition - tweenMarker) / mDuration;
         timeIntoChainedAnimation = 0.0f;
         chain = (mPosition >= tweenMarker) ? true : false;
      }

      float newPosition = mPosition;
      bool wrapped = (oldPosition > newPosition);
      bool ended = (newPosition >= mDuration);

      if (!mIsClone)
      {
         if (mpTags && (mAnimationRate > 0.0f))
         {
            //SCOPEDSAMPLE(BVisualAnimationData_update_Tags)
            float recDuration = 1.0f / mDuration;
            float p1 = oldPosition * recDuration;
            float p2 = newPosition * recDuration;

            long tagCount=mpTags->getNumber();
            for(long i=0; i<tagCount; i++)
            {
               BProtoVisualTag& tag=(*mpTags)[i];
               float tagPosition=tag.mPosition;
               if( (tagPosition<cFloatCompareEpsilon && p1<cFloatCompareEpsilon && p2>=cFloatCompareEpsilon) ||
                  (tagPosition>p1 && tagPosition<=p2) ||
                  (wrapped && (tagPosition<=p2 || tagPosition>p1)) ||
                  (chain && (tagPosition>=p1)) )
               {
                  if(!pVisualItem->processAttachmentEvent(animationTrack, mAnimAsset.mType, tag.mEventType, &tag, mTweenTime, tintColor, worldMatrix))
                     gVisualManager.handleAnimEvent(attachmentHandle, mAnimType, pVisual, &tag);

                  // Stop iterating over tags if the animation got changed in handleAnimEvent
                  if (mIsDirty)
                     break;
               }
            }
         }

         if (sendLoopEvent)
         {
            if (chain)
            {
               BProtoVisualTag tag;
               tag.mEventType=cAnimEventChain;
               tag.mData0=animationTrack;
               tag.mValueInt1=mTweenToAnimation;
               gVisualManager.handleAnimEvent(attachmentHandle, mAnimType, pVisual, &tag);
            }
            else if (ended)
            {
               BProtoVisualTag tag;
               tag.mEventType=cAnimEventEnd;
               tag.mData0=animationTrack;
               gVisualManager.handleAnimEvent(attachmentHandle, mAnimType, pVisual, &tag);
            }
            else if(wrapped)
            {
               BProtoVisualTag tag;
               tag.mEventType=cAnimEventLoop;
               tag.mData0=animationTrack;
               gVisualManager.handleAnimEvent(attachmentHandle, mAnimType, pVisual, &tag);
            }
         }
      }
   }
   /* ajl 10/9/06 - disabling for now because this appears to be causing an OOS
   else if(getFlag(cFlagSendNoAnimEvent))
   {
   BProtoVisualTag tag;
   tag.mEventType=cAnimEventLoop;
   gVisualManager.handleAnimEvent(attachmentHandle, mAnimAsset.mType, pVisual, &tag);
   setFlag(cFlagSendNoAnimEvent, false);
   }
   */

   // Generate animation tag events for animation being tweened out
   // SLB: Don't do this anymore
   //if (!mIsClone && mpTweenOutTags && (mTweenOutTimeLeft > 0.0f) && (mAnimationRate > 0.0f))
   //{
   //   float recDuration = 1.0f / mTweenOutDuration;
   //   float p1 = mTweenOutPosition * recDuration;

   //   mTweenOutPosition += elapsedTime;
   //   mTweenOutPosition = Math::Min(mTweenOutPosition, mTweenOutDuration);

   //   mTweenOutTimeLeft -= elapsedTime;
   //   float p2 = mTweenOutPosition * recDuration;

   //   long tagCount = mpTweenOutTags->getNumber();
   //   for (long i = 0; i < tagCount; i++)
   //   {
   //      BProtoVisualTag& tag = (*mpTweenOutTags)[i];
   //      float tagPosition = tag.mPosition;
   //      if( (tagPosition<cFloatCompareEpsilon && p1<cFloatCompareEpsilon && p2>=cFloatCompareEpsilon) ||
   //         (tagPosition>p1 && tagPosition<=p2))
   //      {
   //         if(!pVisualItem->processAttachmentEvent(animationTrack, mTweenOutAnimAssetType, tag.mEventType, &tag, 0.0f))
   //            gVisualManager.handleAnimEvent(attachmentHandle, mTweenOutAnimAssetType, pVisual, &tag);
   //      }
   //   }

   //   if (mTweenOutTimeLeft <= 0.0f)
   //   {
   //      mpTweenOutTags = NULL;
   //      mTweenOutTimeLeft = 0.0f;
   //   }
   //}
   
   BDEBUG_ASSERT(Math::IsValidFloat(mPosition));
   BDEBUG_ASSERT(Math::IsValidFloat(mAnimationRate));
   BDEBUG_ASSERT(Math::IsValidFloat(mTweenOutTimeLeft));
   BDEBUG_ASSERT(Math::IsValidFloat(mTweenOutPosition));
}

//==============================================================================
//==============================================================================
const BGrannyAnimation* BVisualItem::getAnimation(long animationTrack) const
{
   return gGrannyManager.getAnimation(mAnimationTrack[animationTrack].mAnimAsset.mIndex);
}

//==============================================================================
//==============================================================================
bool BVisualItem::raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, const BMatrix &worldMatrix, float *distanceSqr, float &intersectDistanceSqr, long &intersectAttachmentHandle, long &intersectBoneHandle, BVector &intersectBoneSpacePos, BVector &intersectBoneSpaceDir)
{
   // Note: this function only goes one level deep when checking for collisions.  It will look at the item's attachments but it doesn't 
   // recurse into them.  So if these attachments have other attachments, these will be disregarded.  I originally made this recursive,
   // but returning the attachment index tree is problematic, I am changing it to just be one level deep.  (SAT)

   bool foundHit = false;

   // Check attachments first since they are usually on the outside
   //
   long numAttachments = mAttachments.getNumber();
   for(long i=0; i<numAttachments; i++)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if(!pAttachment->getFlag(cFlagUsed))
         continue;
      if(pAttachment->mModelAsset.mType == cVisualAssetGrannyModel && pAttachment->mpInstance)
      {
         BMatrix attachmentWorldSpaceMat;
         attachmentWorldSpaceMat.mult(pAttachment->mMatrix, worldMatrix);

         // Collide with bounding box first, and eary out
         BBoundingBox attachmentBoundingBox;
         attachmentBoundingBox.initializeTransformed(pAttachment->mMinCorner, pAttachment->mMaxCorner, attachmentWorldSpaceMat);
         float temp;

         if(!attachmentBoundingBox.raySegmentIntersects(origin, vector, segment, NULL, temp))
         {
            continue;
         }

         foundHit = ((BGrannyInstance *)pAttachment->mpInstance)->raySegmentIntersects(origin, vector, segment, attachmentWorldSpaceMat, distanceSqr, intersectDistanceSqr, intersectBoneHandle, intersectBoneSpacePos, intersectBoneSpaceDir);

         if(foundHit)
         {
            intersectAttachmentHandle = pAttachment->mAttachmentHandle;
            return true;
         }
      }
   }

   // Now check itself
   //
   if(mModelAsset.mType == cVisualAssetGrannyModel && mpInstance)
   {
      foundHit = ((BGrannyInstance *)mpInstance)->raySegmentIntersects(origin, vector, segment, worldMatrix, distanceSqr, intersectDistanceSqr, intersectBoneHandle, intersectBoneSpacePos, intersectBoneSpaceDir);

      if(foundHit)
      {
         intersectAttachmentHandle = -1;
         return true;
      }
   }


   return false;
}

//==============================================================================
//==============================================================================
void BVisualItem::setUVOffsets(const BVisualModelUVOffsets &uvOffsets)
{
   mModelUVOffsets = uvOffsets;

   if(mModelAsset.mType != cVisualAssetGrannyModel)
      return;

   if(!mpInstance)
      return;

   BGrannyInstance *pGrannyInstance = (BGrannyInstance*)(mpInstance);
   BASSERT(pGrannyInstance);

   pGrannyInstance->setUVOffsets(uvOffsets);
}

//==============================================================================
//==============================================================================
BGrannyInstance* BVisualItem::getGrannyInstance()
{
   if (mModelAsset.mType == cVisualAssetGrannyModel)
      return (BGrannyInstance*)mpInstance;
   else
      return NULL;
}

//==============================================================================
//==============================================================================
void BVisualItem::setGrannyMeshMask(bool visible)
{
   // only works on granny instances
   BGrannyInstance* pGrannyInstance = getGrannyInstance();
   if (pGrannyInstance)
   {
      // for granny instance, all visible should be undamaged? 
      if (visible)
         pGrannyInstance->setMeshRenderMaskToUndamageState();
      else
         pGrannyInstance->clearMeshRenderMask();
   }

   // do the same for all attachments
   long count = mAttachments.getNumber();
   for (long i = 0; i < count; ++i)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if (pAttachment && pAttachment->getFlag(BVisualItem::cFlagUsed))
         pAttachment->setGrannyMeshMask(visible);
   }
}

//==============================================================================
// BVisualItem::defaultUpdateSyncCallback
//==============================================================================
void BVisualItem::defaultUpdateSyncCallback(BVisualItem* pVisualItem, bool fullSync, bool doSyncChecks)
{
}

//==============================================================================
//==============================================================================
void BVisualItem::clearAttachmentData()
{
   mFromBoneHandle=-1;
   mToBoneHandle=-1;
   setFlag(cFlagUsed, false);
   mpName=NULL;
   mAttachmentHandle=-1;
   mAttachmentType=-1;
   mIndex=-1;                
   setFlag(cFlagDisregardOrient, false);
   mAnimationTrack[cActionAnimationTrack].deinit();
   mAnimationTrack[cMovementAnimationTrack].deinit();
}

//==============================================================================
//==============================================================================
void BVisualItem::removeAttachmentsOfAssetType(long assetType)
{
   // recurse through all attachments
   long count = mAttachments.getNumber();
   for (long i = 0; i < count; ++i)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if (pAttachment)
      {
         if (pAttachment->mModelAsset.mType == assetType)
            pAttachment->setFlag(cFlagUsed, false);
         else 
            pAttachment->removeAttachmentsOfAssetType(assetType);
      }
   }
}

//==============================================================================
//==============================================================================
void BVisualItem::setTintColor(DWORD color)
{
   if (mpInstance)
      mpInstance->setTintColor(color);

   // recurse through all attachments
   long count = mAttachments.getNumber();
   for (long i = 0; i < count; ++i)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if (pAttachment)
         pAttachment->setTintColor(color);
   }
}

//==============================================================================
//==============================================================================
bool BVisualItem::save(BStream* pStream, int saveType, BProtoVisual* pProtoVisual) const
{
   BMatrix identityMatrix;
   identityMatrix.makeIdentity();

   // mMatrix
   bool saveMatrix = (mMatrix == identityMatrix ? false : true);
   GFWRITEVAR(pStream, bool, saveMatrix);
   if (saveMatrix)
   {
      BVector vec;
      mMatrix.getForward(vec);
      GFWRITEVECTOR(pStream, vec);
      mMatrix.getRight(vec);
      GFWRITEVECTOR(pStream, vec);
      mMatrix.getUp(vec);
      GFWRITEVECTOR(pStream, vec);
      mMatrix.getTranslation(vec);
      GFWRITEVECTOR(pStream, vec);
   }

   GFWRITEVAR(pStream, DWORD, mSubUpdateNumber);
   GFWRITEVAR(pStream, DWORD, mGrannySubUpdateNumber);

   // mTransform
   saveMatrix = (mTransform == identityMatrix ? false : true);
   GFWRITEVAR(pStream, bool, saveMatrix);
   if (saveMatrix)
   {
      BVector vec;
      mTransform.getForward(vec);
      GFWRITEVECTOR(pStream, vec);
      mTransform.getRight(vec);
      GFWRITEVECTOR(pStream, vec);
      mTransform.getUp(vec);
      GFWRITEVECTOR(pStream, vec);
      mTransform.getTranslation(vec);
      GFWRITEVECTOR(pStream, vec);
   }

   // mOldTransform
   saveMatrix = (mOldTransform == identityMatrix ? false : true);
   GFWRITEVAR(pStream, bool, saveMatrix);
   if (saveMatrix)
   {
      BVector vec;
      mOldTransform.getForward(vec);
      GFWRITEVECTOR(pStream, vec);
      mOldTransform.getRight(vec);
      GFWRITEVECTOR(pStream, vec);
      mOldTransform.getUp(vec);
      GFWRITEVECTOR(pStream, vec);
      mOldTransform.getTranslation(vec);
      GFWRITEVECTOR(pStream, vec);
   }

   GFWRITEVECTOR(pStream, mCombinedMinCorner);
   GFWRITEVECTOR(pStream, mCombinedMaxCorner);
   GFWRITEVECTOR(pStream, mMinCorner);
   GFWRITEVECTOR(pStream, mMaxCorner);
   GFWRITEVAR(pStream, BVisualAsset, mModelAsset);

   // mModelUVOffsets
   BVisualModelUVOffsets blankOffsets;
   blankOffsets.clear();
   bool saveOffests = (mModelUVOffsets != blankOffsets);
   GFWRITEVAR(pStream, bool, saveOffests);
   if (saveOffests)
      GFWRITEVAR(pStream, BVisualModelUVOffsets, mModelUVOffsets);

   GFWRITEUTBITVECTOR(pStream, mFlags, uint8, 32);

   // mAttachments;
   uint numAttachments = mAttachments.size();
   GFWRITEVAL(pStream, uint8, numAttachments);
   GFVERIFYCOUNT(numAttachments, 100);
   for (uint i=0; i<numAttachments; i++)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if (pAttachment && pAttachment->getFlag(cFlagUsed))
      {
         GFWRITEVAL(pStream, bool, true);
         if (!pAttachment->save(pStream, saveType, pProtoVisual))
         {
            {GFERROR("GameFile Error: pAttachment->save failed");}
            return false;
         }
      }
      else
         GFWRITEVAL(pStream, bool, false);
   }

   GFWRITEVAR(pStream, long, mFromBoneHandle);
   GFWRITEVAR(pStream, long, mToBoneHandle);        
   GFWRITEVAR(pStream, long, mAttachmentHandle);
   GFWRITEVAR(pStream, long, mAttachmentType);
   GFWRITEVAR(pStream, long, mIndex);
   GFWRITEVAR(pStream, long, mData);

   //const BSimString* mpName;

   GFWRITEVAR(pStream, float, mLifespan);

   // mAnimationTrack
   for (int i=0; i<2; i++)
   {
      if (!mAnimationTrack[i].save(pStream, saveType, pProtoVisual))
      {
         {GFERROR("GameFile Error: mAnimationTrack[%d].save failed", i);}
         return false;
      }
   }

   GFWRITEVAL(pStream, int8, mModelVariationIndex);
   GFWRITEVAL(pStream, int16, mDamageTemplateIndex);
   GFWRITEVAL(pStream, int8, mDisplayPriority);

   // mpInstance;
   bool haveInstance = (mpInstance != NULL);
   GFWRITEVAR(pStream, bool, haveInstance);
   if (haveInstance)
   {
      switch (mModelAsset.mType)
      {
         case cVisualAssetGrannyModel:
         {
            BGrannyInstance* pGrannyInstance = (BGrannyInstance*)mpInstance;
            GFWRITECLASSPTR(pStream, saveType, pGrannyInstance);
            break;
         }
         case cVisualAssetGrannyAnim:
            break;
         case cVisualAssetParticleSystem:
            break;
         case cVisualAssetLight:
            break;
         case cVisualAssetTerrainEffect:
            break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BVisualItem::load(BStream* pStream, int saveType, BProtoVisual* pProtoVisual, const BMatrix& worldMatrix, DWORD tintColor)
{
   // mMatrix
   bool loadMatrix;
   GFREADVAR(pStream, bool, loadMatrix);
   if (loadMatrix)
   {
      BVector forward, up, right, pos;
      GFREADVECTOR(pStream, forward);
      GFREADVECTOR(pStream, right);
      GFREADVECTOR(pStream, up);
      GFREADVECTOR(pStream, pos);
      mMatrix.makeOrient(forward, up, right);
      mMatrix.setTranslation(pos);
   }

   // jce [11/5/2008] -- this used to be the old and new matrix for the broken attachment interpolation.
   if(mGameFileVersion < 3)
   {
      // Just read and throw away these matrices if the are present
      GFREADVAR(pStream, bool, loadMatrix);
      if (loadMatrix)
      {
         BVector forward, up, right, pos;
         GFREADVECTOR(pStream, forward);
         GFREADVECTOR(pStream, right);
         GFREADVECTOR(pStream, up);
         GFREADVECTOR(pStream, pos);
      }

      // mNewMatrix
      GFREADVAR(pStream, bool, loadMatrix);
      if (loadMatrix)
      {
         BVector forward, up, right, pos;
         GFREADVECTOR(pStream, forward);
         GFREADVECTOR(pStream, right);
         GFREADVECTOR(pStream, up);
         GFREADVECTOR(pStream, pos);
      }
   }

   GFREADVAR(pStream, DWORD, mSubUpdateNumber);
   GFREADVAR(pStream, DWORD, mGrannySubUpdateNumber);

   // mTransform
   GFREADVAR(pStream, bool, loadMatrix);
   if (loadMatrix)
   {
      BVector forward, up, right, pos;
      GFREADVECTOR(pStream, forward);
      GFREADVECTOR(pStream, right);
      GFREADVECTOR(pStream, up);
      GFREADVECTOR(pStream, pos);
      mTransform.makeOrient(forward, up, right);
      mTransform.setTranslation(pos);
   }

   // mOldTransform
   if(mGameFileVersion >= 3)
   {
      GFREADVAR(pStream, bool, loadMatrix);
      if (loadMatrix)
      {
         BVector forward, up, right, pos;
         GFREADVECTOR(pStream, forward);
         GFREADVECTOR(pStream, right);
         GFREADVECTOR(pStream, up);
         GFREADVECTOR(pStream, pos);
         mOldTransform.makeOrient(forward, up, right);
         mOldTransform.setTranslation(pos);
      }
   }
   else
   {
      // In an old savegame, just make old = new transform since we weren't interpolating.
      mOldTransform = mTransform;
   }

   GFREADVECTOR(pStream, mCombinedMinCorner);
   GFREADVECTOR(pStream, mCombinedMaxCorner);
   GFREADVECTOR(pStream, mMinCorner);
   GFREADVECTOR(pStream, mMaxCorner);

   GFREADVAR(pStream, BVisualAsset, mModelAsset);
   GFVERIFYCOUNT(mModelAsset.mType, cVisualAssetMaxCount);
   if (mModelAsset.mType == cVisualAssetGrannyModel)
      gSaveGame.remapGrannyModelID(mModelAsset.mIndex);

   // mModelUVOffsets
   bool loadOffests;
   GFREADVAR(pStream, bool, loadOffests);
   if (loadOffests)
      GFREADVAR(pStream, BVisualModelUVOffsets, mModelUVOffsets);

   GFREADUTBITVECTOR(pStream, mFlags, uint8, 32);

   // mAttachments;
   uint numAttachments;
   GFREADVAL(pStream, uint8, uint, numAttachments);
   GFVERIFYCOUNT(numAttachments, 100);
   mAttachments.setNumber(numAttachments);
   if (mAttachments.size() != numAttachments)
   {
      {GFERROR("GameFile Error: mAttachments.setNumber(%u) failed", numAttachments);}
      return false;
   }
   for (uint i=0; i<numAttachments; i++)
   {
      BVisualItem* pAttachment=BVisualItem::getInstance();
      if (!pAttachment)
      {
         {GFERROR("GameFile Error: BVisualItem::getInstance for attachment %u failed", i);}
         for (uint j=0; j<i; j++)
            BVisualItem::releaseInstance(mAttachments[j]);
         mAttachments.clear();
         return false;
      }
      mAttachments[i] = pAttachment;
   }
   for (uint i=0; i<numAttachments; i++)
   {
      bool loadAttachment;
      GFREADVAR(pStream, bool, loadAttachment);
      if (loadAttachment)
      {
         BVisualItem* pAttachment = mAttachments[i];
         if (!pAttachment->load(pStream, saveType, pProtoVisual, worldMatrix, tintColor))
         {
            {GFERROR("GameFile Error: pAttachment->load failed");}
            return false;
         }
      }
   }

   GFREADVAR(pStream, long, mFromBoneHandle);
   GFREADVAR(pStream, long, mToBoneHandle);        

   GFREADVAR(pStream, long, mAttachmentHandle);
   gSaveGame.remapAttachmentType(mAttachmentHandle);

   // mAttachmentType is one of the cAttachment as opposed to an attachment handle which we get from gVisualManager.getAttachmentType
   GFREADVAR(pStream, long, mAttachmentType);

   GFREADVAR(pStream, long, mIndex);
   GFREADVAR(pStream, long, mData);

   //const BSimString* mpName;

   GFREADVAR(pStream, float, mLifespan);

   // mAnimationTrack
   for (int i=0; i<2; i++)
   {
      if (!mAnimationTrack[i].load(pStream, saveType, pProtoVisual))
      {
         {GFERROR("GameFile Error: mAnimationTrack[%d].load failed", i);}
         return false;
      }
   }

   GFREADVAL(pStream, int8, long, mModelVariationIndex);

   GFREADVAL(pStream, int16, long, mDamageTemplateIndex);
   if (mGameFileVersion < 2)
      mDamageTemplateIndex = -1;

   GFREADVAL(pStream, int8, int, mDisplayPriority);

   // mpInstance;
   /*
   cVisualAssetGrannyModel,
   cVisualAssetGrannyAnim,
   cVisualAssetParticleSystem,
   cVisualAssetLight,
   cVisualAssetTerrainEffect,
   */
   bool haveInstance;
   GFREADVAR(pStream, bool, haveInstance);
   if (haveInstance)
   {
      if (mModelAsset.mType == cVisualAssetGrannyModel)
      {
         createInstance(worldMatrix, tintColor);
         BGrannyInstance* pGrannyInstance=(BGrannyInstance*)mpInstance;

         if (!pGrannyInstance)
            GFREADTEMPCLASS(pStream, saveType, BGrannyInstance)
         else
            GFREADCLASSPTR(pStream, saveType, pGrannyInstance)

         if (pGrannyInstance)
         {
            for (int animationTrack=0; animationTrack<2; animationTrack++)
            {
               BVisualAnimationData &animation = mAnimationTrack[animationTrack];
               pGrannyInstance->playAnimation(animationTrack, animation.mAnimAsset.mIndex, 1.0f, 0.0f, (animation.mExitAction==cAnimExitActionLoop?0:1), animation.mTweenOutTimeLeft, animation.mPosition);
            }
         }
      }
      else
         setFlag(cFlagPostLoadCreateInstance, true);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BVisualItem::postLoad(int saveType, BProtoVisual* pProtoVisual, const BMatrix& worldMatrix, DWORD tintColor)
{
   if (mDamageTemplateIndex != -1)
      gSaveGame.remapDamageTemplate(mDamageTemplateIndex);

   switch (mModelAsset.mType)
   {
      case cVisualAssetGrannyModel:
         break;
      case cVisualAssetParticleSystem:
         gSaveGame.remapParticleEffectID(mModelAsset.mIndex);
         break;
      case cVisualAssetLight:
         gSaveGame.remapLightEffectID(mModelAsset.mIndex);
         break;
      case cVisualAssetTerrainEffect:
         // ajl 10/25/08 - currently not handling loading of terrain effects
         mModelAsset.mIndex = -1;
         mModelAsset.mType = -1;
         break;
   }

   if (getFlag(cFlagPostLoadCreateInstance))
   {
      createInstance(worldMatrix, tintColor);
      setFlag(cFlagPostLoadCreateInstance, false);
   }

   uint numAttachments = mAttachments.size();
   for (uint i=0; i<numAttachments; i++)
   {
      BVisualItem* pAttachment = mAttachments[i];
      if (pAttachment->getFlag(cFlagUsed))
      {
         BMatrix attachmentWorldMatrix;
         attachmentWorldMatrix.mult(pAttachment->mMatrix, worldMatrix);
         if (!pAttachment->postLoad(saveType, pProtoVisual, attachmentWorldMatrix, tintColor))
            return false;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BVisualAnimationData::save(BStream* pStream, int saveType, BProtoVisual* pProtoVisual) const
{
   GFWRITEVAR(pStream, long, mAnimType);
   GFWRITEVAR(pStream, BVisualAsset, mAnimAsset);

   // mpTags
   long modelIndex=-1, animIndex=-1, assetIndex=-1;
   if (pProtoVisual)
      pProtoVisual->findTags(mpTags, modelIndex, animIndex, assetIndex);
   GFWRITEVAL(pStream, int8, modelIndex);
   GFWRITEVAL(pStream, int8, animIndex);
   GFWRITEVAL(pStream, int8, assetIndex);

   // mpPoints
   if (pProtoVisual)
      pProtoVisual->findPoints(mpPoints, modelIndex);
   GFWRITEVAL(pStream, int8, modelIndex);

   // mpOpacityProgression
   if (pProtoVisual)
      pProtoVisual->findProgression(mpOpacityProgression, modelIndex, animIndex, assetIndex);
   GFWRITEVAL(pStream, int8, modelIndex);
   GFWRITEVAL(pStream, int8, animIndex);
   GFWRITEVAL(pStream, int8, assetIndex);

   GFWRITEVAR(pStream, float, mPosition);
   GFWRITEVAR(pStream, float, mDuration);
   GFWRITEVAR(pStream, long, mExitAction);
   GFWRITEVAR(pStream, long, mTweenToAnimation);
   GFWRITEVAR(pStream, float, mTweenTime);
   GFWRITEVAR(pStream, float, mAnimationRate);

   //BProtoVisualTagArray* mpTweenOutTags;
   if (pProtoVisual)
      pProtoVisual->findTags(mpTweenOutTags, modelIndex, animIndex, assetIndex);
   GFWRITEVAL(pStream, int8, modelIndex);
   GFWRITEVAL(pStream, int8, animIndex);
   GFWRITEVAL(pStream, int8, assetIndex);

   GFWRITEVAR(pStream, float, mTweenOutPosition);
   GFWRITEVAR(pStream, float, mTweenOutTimeLeft);
   GFWRITEVAR(pStream, float, mTweenOutDuration);
   GFWRITEVAR(pStream, long, mTweenOutAnimAssetType);

   GFWRITEBITBOOL(pStream, mIsClone);
   GFWRITEBITBOOL(pStream, mIsLocked);
   GFWRITEBITBOOL(pStream, mIsDirty);

   GFWRITEBITBOOL(pStream, mUsedByThisTrack);

   return true;
}

//==============================================================================
//==============================================================================
bool BVisualAnimationData::load(BStream* pStream, int saveType, BProtoVisual* pProtoVisual)
{
   GFREADVAR(pStream, long, mAnimType);
   gSaveGame.remapAnimType(mAnimType);

   GFREADVAR(pStream, BVisualAsset, mAnimAsset);
   GFVERIFYCOUNT(mAnimAsset.mType, cVisualAssetMaxCount);
   if (mAnimAsset.mType == cVisualAssetGrannyAnim)
      gSaveGame.remapGrannyAnimID(mAnimAsset.mIndex);

   // mpTags
   long modelIndex, animIndex, assetIndex;
   GFREADVAL(pStream, int8, long, modelIndex);
   GFREADVAL(pStream, int8, long, animIndex);
   GFREADVAL(pStream, int8, long, assetIndex);
   if (pProtoVisual)
      mpTags = pProtoVisual->getTags(modelIndex, animIndex, assetIndex);

   // mpPoints
   GFREADVAL(pStream, int8, long, modelIndex);
   if (pProtoVisual)
      mpPoints = pProtoVisual->getPoints(modelIndex);

   // mpOpacityProgression
   GFREADVAL(pStream, int8, long, modelIndex);
   GFREADVAL(pStream, int8, long, animIndex);
   GFREADVAL(pStream, int8, long, assetIndex);
   if (pProtoVisual)
      mpOpacityProgression = pProtoVisual->getProgression(modelIndex, animIndex, assetIndex);

   GFREADVAR(pStream, float, mPosition);
   GFREADVAR(pStream, float, mDuration);
   GFREADVAR(pStream, long, mExitAction);
   GFREADVAR(pStream, long, mTweenToAnimation);
   GFREADVAR(pStream, float, mTweenTime);
   GFREADVAR(pStream, float, mAnimationRate);

   //BProtoVisualTagArray* mpTweenOutTags;
   GFREADVAL(pStream, int8, long, modelIndex);
   GFREADVAL(pStream, int8, long, animIndex);
   GFREADVAL(pStream, int8, long, assetIndex);
   if (pProtoVisual)
      mpTweenOutTags = pProtoVisual->getTags(modelIndex, animIndex, assetIndex);

   GFREADVAR(pStream, float, mTweenOutPosition);
   GFREADVAR(pStream, float, mTweenOutTimeLeft);
   GFREADVAR(pStream, float, mTweenOutDuration);
   GFREADVAR(pStream, long, mTweenOutAnimAssetType);

   GFREADBITBOOL(pStream, mIsClone);
   GFREADBITBOOL(pStream, mIsLocked);
   GFREADBITBOOL(pStream, mIsDirty);

   if (mGameFileVersion >= 2)
   {
      GFREADBITBOOL(pStream, mUsedByThisTrack);
   }
   else
   {
      mUsedByThisTrack = false;
   }

   return true;
}


//==============================================================================
//==============================================================================
void BSparsePoseHelper::set(const granny_skeleton* pSkeleton, granny_local_pose* pLocalPose, void* pSparseBoneArray, void *pSparseBoneArrayReverse) 
{
   mpSkeleton = pSkeleton;
   mpLocalPose = pLocalPose;
   mpSparseBoneArray = pSparseBoneArray;
   mpSparseBoneArrayReverse = pSparseBoneArrayReverse;
   bIsValid = true;
};

//==============================================================================
//==============================================================================
bool BSparsePoseHelper::getBone(long boneHandle, BMatrix *result) const
{
   if(!isValid() || boneHandle==-1)
      return false;

   long boneIndex=BONEFROMGRANNYBONEHANDLE(boneHandle);

   if(boneIndex >= mpSkeleton->BoneCount)
      return false;

   GrannyGetWorldMatrixFromLocalPose(mpSkeleton, boneIndex, mpLocalPose, NULL, (granny_real32*)result, (granny_int32x*)mpSparseBoneArray, (granny_int32x*)mpSparseBoneArrayReverse);
   return true;
};
