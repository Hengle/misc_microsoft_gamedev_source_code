//==============================================================================
// squadactionplayblockinganimation.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "squadactionplayblockinganimation.h"
#include "squad.h"
#include "world.h"
#include "soundmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionPlayBlockingAnimation, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionPlayBlockingAnimation::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT( pOwner );
   BASSERT( pOwner->isClassType(BEntity::cClassTypeSquad) );

   if (!BAction::connect(pOwner, pOrder))
      return(false);

   // Block
   BSquad* pSquad = reinterpret_cast<BSquad*>(pOwner);
   pSquad->setFlagPlayingBlockingAnimation(true);
   pSquad->setFlagUseMaxHeight(mFlagUseMaxHeight);
   pSquad->setFlagIgnoreLeash(true);

   return(true);
}

//==============================================================================
//==============================================================================
void BSquadActionPlayBlockingAnimation::disconnect()
{
   if (!mFlagEndActionCalled)
      endAction();

   // Unblock
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   pSquad->setFlagPlayingBlockingAnimation(false);
   pSquad->setFlagDontPopCommand(true);
   pSquad->setFlagUseMaxHeight(false);
   pSquad->setFlagIgnoreLeash(false);

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionPlayBlockingAnimation::init()
{
   if (!BAction::init())
      return(false);

   mActiveChildActions = 0;
   mAnimationState = -1;
   mAnimationType = -1;
   mFlagConflictsWithIdle=true;
   mFlagApplyAnimationInstantly=false;
   mFlagUseSquadMatrix=false;
   mFlagUseMaxHeight=false;
   mFlagLoop=false;
   mFlagDisableMotionExtractionCollisions = false;
   mFlagUpdateSquadPhysicsPosAtEnd = false;
   mFlagEndActionCalled = false;
   mFlagClearUnitNoRenderFlag = false;
   mFlagBirthAnim = false;
   mSoundCue = cInvalidCueIndex;
   mTriggerModule.clearNotificationData();
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionPlayBlockingAnimation::setState(BActionState state)
{
   if (state == cStateDone)
      mTriggerModule.notifyActionStatus(BTriggerVarActionStatus::cDoneSuccess);
   else if (state == cStateFailed)
      mTriggerModule.notifyActionStatus(BTriggerVarActionStatus::cDoneFailure);
   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BSquadActionPlayBlockingAnimation::update(float elapsed)
{
   switch (mState)
   {
      case cStateNone:
      {
         createUnitActions();
         setState(cStateWorking);
      }
      break;

      case cStateWorking:
      {
         if (mActiveChildActions <= 0)
            setState(cStateDone);
      }
      break;
   }

   //-- now do some housekeeping
   if (!BAction::update(elapsed))
      return (false);

   return (true);

}

//==============================================================================
//==============================================================================
void BSquadActionPlayBlockingAnimation::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   switch (eventType)
   {
      case BEntity::cEventActionDone:
         if (data == BAction::cActionTypeSquadPlayBlockingAnimation)
         {
            mActiveChildActions--; // A child action that we were waiting on has finished

            // TRB 10/27/08 - Call endAction here if possible to update the squad's position as
            // soon after physics control is returned for the unit.  If this is delayed the unit
            // may try to move to the old squad position before its new position is set.
            if (!mFlagEndActionCalled && (mActiveChildActions <= 0))
               endAction();
         }
         break;
   }    
}

//==============================================================================
//==============================================================================
void BSquadActionPlayBlockingAnimation::endAction()
{
   mFlagEndActionCalled = true;

   BSquad* pSquad=mpOwner->getSquad();
   BVector averagePosition = XMVectorZero();
   long numUnits = pSquad->getNumberChildren();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         pUnit->endPlayBlockingAnimation();
         averagePosition += pUnit->getPosition();

         // Failsafe to make sure no render flag cleared in case unit action wasn't able to do it
         if (mFlagClearUnitNoRenderFlag && pUnit->getFlagNoRender())
            pUnit->setFlagNoRender(false);
      }
   }

   averagePosition /= float(numUnits);

   pSquad->setPosition(averagePosition); // Set new squad position based on average unit position
   pSquad->tieToGround();

   // TRB 10/27/08 - It's probably good to always do this but I wanted this change to be as least disruptive as possible.
   // For physics vehicles, the turn radius pos and forward must be set so the physics code doesn't jerk the vehicle to its
   // old position.  Same thing with the leash.  This change was made for birthing specifically.
   if (mFlagUpdateSquadPhysicsPosAtEnd)
   {
      pSquad->setTurnRadiusPos(pSquad->getPosition());
      pSquad->setTurnRadiusFwd(pSquad->getForward());
      pSquad->setLeashPosition(pSquad->getPosition());
   }

   // MPB 6/22/2007
   // Update the platoon to the squad position if this is the only
   // squad in the platoon.  This was to fix positioning after
   // vehicle birth anims.
   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if (pPlatoon && (pPlatoon->getNumberChildren() == 1))
   {
      pPlatoon->setPosition(pSquad->getPosition());
   }

   if(mSoundCue != cInvalidCueIndex)
      gSoundManager.playCue(mSoundCue);
}

//==============================================================================
//==============================================================================
void BSquadActionPlayBlockingAnimation::createUnitActions()
{
//-- FIXING PREFIX BUG ID 4913
   const BSquad* pSquad=mpOwner->getSquad();
//--

   BVector position, forward, right;

   if (mFlagUseSquadMatrix)
   {
      position = pSquad->getPosition();
      forward = pSquad->getForward();
      right = pSquad->getRight();
   }

   const char* pBaseAnimName = NULL;
   pBaseAnimName = gVisualManager.getAnimName(mAnimationType);

   long numUnits = pSquad->getNumberChildren();
   for (long i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if (pUnit)
      {
         if (mFlagUseSquadMatrix)
         {
            #ifdef SYNC_Unit
               syncUnitData("BSquadActionPlayBlockingAnimation::createUnitActions", position);
            #endif
            pUnit->setPosition(position);
            pUnit->setForward(forward);
            pUnit->setRight(right);
            pUnit->calcUp();
         }

         // If we have more than one squad member, give the subsequent members unique animTypes that have their place in the squad appended to the name
         BSimString animNameString = BSimString(pBaseAnimName);
         if (i>0)
            animNameString.format("%s%d", animNameString.getPtr(), i+1);

         long animType = gVisualManager.getAnimType(animNameString);

         pUnit->beginPlayBlockingAnimation(mAnimationState, animType, mFlagApplyAnimationInstantly, mFlagUseMaxHeight, false, -1, NULL, mFlagLoop, mFlagDisableMotionExtractionCollisions, mFlagClearUnitNoRenderFlag);
         mActiveChildActions++;
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionPlayBlockingAnimation::setAnimationState(long state, long type, bool applyInstantly, bool useSquadMatrix, bool useMaxHeight, bool loop)
{
   mAnimationState = state;
   mAnimationType = type;
   mFlagApplyAnimationInstantly=applyInstantly;
   mFlagUseSquadMatrix=useSquadMatrix;
   mFlagUseMaxHeight=useMaxHeight;
   mFlagLoop=loop;
}

//==============================================================================
//==============================================================================
bool BSquadActionPlayBlockingAnimation::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, BActionTriggerModule, mTriggerModule);
   GFWRITEVAR(pStream, long, mActiveChildActions);
   GFWRITEVAR(pStream, long, mAnimationState);
   GFWRITEVAR(pStream, long, mAnimationType);
   GFWRITEVAR(pStream, BCueIndex, mSoundCue);
   GFWRITEBITBOOL(pStream, mFlagApplyAnimationInstantly);
   GFWRITEBITBOOL(pStream, mFlagUseSquadMatrix);
   GFWRITEBITBOOL(pStream, mFlagUseMaxHeight);
   GFWRITEBITBOOL(pStream, mFlagLoop);
   GFWRITEBITBOOL(pStream, mFlagDisableMotionExtractionCollisions);
   GFWRITEBITBOOL(pStream, mFlagUpdateSquadPhysicsPosAtEnd);
   GFWRITEBITBOOL(pStream, mFlagEndActionCalled);
   GFWRITEBITBOOL(pStream, mFlagClearUnitNoRenderFlag);
   GFWRITEBITBOOL(pStream, mFlagBirthAnim);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionPlayBlockingAnimation::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   // This action wasn't saved before version 33
   if (BAction::mGameFileVersion < 33)
      return true;

   GFREADVAR(pStream, BActionTriggerModule, mTriggerModule);
   GFREADVAR(pStream, long, mActiveChildActions);
   GFREADVAR(pStream, long, mAnimationState);
   GFREADVAR(pStream, long, mAnimationType);
   GFREADVAR(pStream, BCueIndex, mSoundCue);
   GFREADBITBOOL(pStream, mFlagApplyAnimationInstantly);
   GFREADBITBOOL(pStream, mFlagUseSquadMatrix);
   GFREADBITBOOL(pStream, mFlagUseMaxHeight);
   GFREADBITBOOL(pStream, mFlagLoop);
   GFREADBITBOOL(pStream, mFlagDisableMotionExtractionCollisions);
   GFREADBITBOOL(pStream, mFlagUpdateSquadPhysicsPosAtEnd);
   GFREADBITBOOL(pStream, mFlagEndActionCalled);
   if (BAction::mGameFileVersion >= 38)
      GFREADBITBOOL(pStream, mFlagClearUnitNoRenderFlag);
   if (BAction::mGameFileVersion >= 51)
      GFREADBITBOOL(pStream, mFlagBirthAnim);

   gSaveGame.remapAnimType(mAnimationType);

   return true;
}
