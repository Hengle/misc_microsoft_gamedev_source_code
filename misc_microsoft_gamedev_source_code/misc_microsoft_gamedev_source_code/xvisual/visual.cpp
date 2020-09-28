//==============================================================================
// visual.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "visual.h"
#include "render.h"
#include "protovisual.h"
#include "visualmanager.h"

GFIMPLEMENTVERSION(BVisual, 1);

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BVisual, 10, &gSimHeap);

//==============================================================================
// BVisual::BVisual
//==============================================================================
BVisual::BVisual()
{
}

//==============================================================================
// BVisual::~BVisual
   //==============================================================================
BVisual::~BVisual()
{
}

//==============================================================================
//==============================================================================
void BVisual::onAcquire()
{ 
   BVisualItem::onAcquire();
   mpProtoVisual=NULL;
   mPrevProtoVisualGeneration=1;
   mUserData=0;
}

//==============================================================================
//==============================================================================
void BVisual::onRelease()
{ 
   BVisualItem::onRelease();
}

//==============================================================================
// BVisual::init
//==============================================================================
bool BVisual::init(BProtoVisual* pProtoVisual, int64 userData, bool synced, DWORD tintColor, const BMatrix& worldMatrix, int displayPriority)
{
   mpProtoVisual=pProtoVisual;
   mUserData=userData;
   mPrevProtoVisualGeneration=(mpProtoVisual ? mpProtoVisual->getGeneration() : 0);
   return BVisualItem::init(pProtoVisual, userData, synced, tintColor, worldMatrix, displayPriority);
}

//==============================================================================
// BVisual::clone
//==============================================================================
bool BVisual::clone(const BVisual* pSource, bool synced, int64 userData, bool bDisregardAttachments, DWORD tintColor, const BMatrix& worldMatrix)
{
   mUserData=userData;
   // [11-11-08 CJS] Added NULL check to prevent crash
   if (pSource && pSource->mpProtoVisual)
   {
      mpProtoVisual=pSource->mpProtoVisual;
      mPrevProtoVisualGeneration=mpProtoVisual->getGeneration();
   }
   return BVisualItem::clone(mpProtoVisual, mUserData, pSource, synced, bDisregardAttachments, tintColor, worldMatrix);
}

//==============================================================================
// BVisual::clone
//==============================================================================
bool BVisual::clone(const BVisualItem* pSource, bool synced, int64 userData, bool bDisregardAttachments, DWORD tintColor, const BMatrix& worldMatrix)
{
   mUserData=userData;
   return BVisualItem::clone(mpProtoVisual, mUserData, pSource, synced, bDisregardAttachments, tintColor, worldMatrix);
}

//==============================================================================
// BVisual::deinit
//==============================================================================
void BVisual::deinit()
{
   BVisualItem::deinit();
}
//==============================================================================
// BVisual::update
//==============================================================================
void BVisual::updatePreAsync(float elapsedTime, DWORD tintColor, const BMatrix& matrix, DWORD subUpdate, bool animationEnabled)
{
   SCOPEDSAMPLE(BVisual_updatePreAsync);
   if (animationEnabled && mpProtoVisual) 
   {
      if (mPrevProtoVisualGeneration != mpProtoVisual->getGeneration())
      {
         // SLB: Old code didn't handle clones. This ended up causing both tracks to fire the same animation tags.
//          for (long track = 0; track < cNumAnimationTracks; track++)
//             updateState(track, mpProtoVisual, mUserData, mAnimationTrack[track].mAnimType, false, 0.0f, -1, false, NULL, matrix, NULL);

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

         // Yes. Set animation of master than copy it to the clone.
         if (clone != -1)
         {
            long master = 1 - clone;
            updateState(master, mpProtoVisual, mUserData, mAnimationTrack[master].mAnimType, false, 0.0f, -1, false, NULL, tintColor, matrix, NULL, false, false);
            copyAnimationTrack(master, clone, false, tintColor, matrix);
         }
         // No. Just set animations.
         else
         {
            for (long track = 0; track < cNumAnimationTracks; track++)
            {
               updateState(track, mpProtoVisual, mUserData, mAnimationTrack[track].mAnimType, false, 0.0f, -1, false, NULL, tintColor, matrix, NULL, false, false);
            }
         }

         validateAnimationTracks();

         mPrevProtoVisualGeneration = mpProtoVisual->getGeneration();
      }
   }

   // Update the base item
   BVisualItem::updatePreAsync(elapsedTime, subUpdate, animationEnabled);
}

//==============================================================================
// BVisual::updateAsync
//==============================================================================
void BVisual::updateAsync(float elapsedTime, DWORD subUpdate)
{
   // Update the base item
   BVisualItem::updateAsync(elapsedTime, true, subUpdate);
}

//==============================================================================
// BVisual::update
//==============================================================================
void BVisual::update(float elapsedTime, DWORD tintColor, const BMatrix& matrix, DWORD subUpdate)
{
   // Update the base item
   BVisualItem::update(elapsedTime, this, -1, true, tintColor, matrix, subUpdate);
}

//==============================================================================
// BVisual::setAnimation
//==============================================================================
void BVisual::setAnimation(long animationTrack, long animType, bool applyInstantly, DWORD tintColor, const BMatrix& worldMatrix, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction)
{
   if (mpProtoVisual) 
   {
      if (getFlag(cFlagSynced))
         gVisualManager.handleSetAnimSync(this, animationTrack, animType, applyInstantly, timeIntoAnimation, forceAnimID, reset, startOnThisAttachment, pOverrideExitAction);
      BVisualItem::setAnimation(animationTrack, mpProtoVisual, mUserData, animType, applyInstantly, timeIntoAnimation, tintColor, worldMatrix, forceAnimID, reset, startOnThisAttachment, pOverrideExitAction);
   }
}

//==============================================================================
// BVisual::getAnimation
//==============================================================================
BVisualAnimationData BVisual::getAnimationData(long animationTrack, long animType)
{
   return BVisualItem::getAnimationData(animationTrack, mpProtoVisual, mUserData, animType);
}

//==============================================================================
// BVisual::copyAnimationTrack
//==============================================================================
void BVisual::copyAnimationTrack(long fromTrack, long toTrack, bool applyInstantly, DWORD tintColor, const BMatrix& worldMatrix, BVisualItem* startOnThisAttachment)
{
   BVisualItem::copyAnimationTrack(fromTrack, toTrack, mpProtoVisual, mUserData, applyInstantly, tintColor, worldMatrix, startOnThisAttachment);
}

//==============================================================================
// BVisual::hasAnimation
//==============================================================================
bool BVisual::hasAnimation(long animType) const
{
   if (mpProtoVisual)
   {
      // Iterate through models
      long numModels = mpProtoVisual->mModels.getNumber();
      for (long model = 0; model < numModels; model++)
      {
         const BProtoVisualModel* pProtoModel = mpProtoVisual->mModels.get(model);
         if (pProtoModel)
         {
            // Iterate through animations
            long numAnimations = pProtoModel->mAnims.getNumber();
            for (long animation = 0; animation < numAnimations; animation++)
            {
               // Look for animType
               const BProtoVisualAnim* pProtoAnim = &pProtoModel->mAnims.get(animation);
               if (pProtoAnim && (pProtoAnim->mAnimType == animType))
               {
                  return true; // Found it
               }
            }
         }
      }
   }

   return false; // Didn't find it
}

//==============================================================================
//==============================================================================
long BVisual::getProtoVisualID() const
{
   if (mpProtoVisual)
      return mpProtoVisual->getID();
   else
      return -1;
}

//==============================================================================
//==============================================================================
bool BVisual::save(BStream* pStream, int saveType) const
{
   //BProtoVisual* mpProtoVisual;

   GFWRITEVAR(pStream, int64, mUserData);

   //DWORD mPrevProtoVisualGeneration;

   if (!BVisualItem::save(pStream, saveType, mpProtoVisual))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BVisual::load(BStream* pStream, int saveType, BProtoVisual* pProtoVisual, const BMatrix& worldMatrix, DWORD tintColor)
{
   mpProtoVisual = pProtoVisual;

   GFREADVAR(pStream, int64, mUserData);

   mPrevProtoVisualGeneration=(mpProtoVisual ? mpProtoVisual->getGeneration() : 0);

   if (!BVisualItem::load(pStream, saveType, pProtoVisual, worldMatrix, tintColor))
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BVisual::postLoad(int saveType, const BMatrix& worldMatrix, DWORD tintColor)
{
   return BVisualItem::postLoad(saveType, mpProtoVisual, worldMatrix, tintColor);
}
