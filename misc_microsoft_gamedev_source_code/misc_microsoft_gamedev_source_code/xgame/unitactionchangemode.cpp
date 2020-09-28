//==============================================================================
// unitactionchangemode.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionchangemode.h"
#include "protoobject.h"
#include "unit.h"
#include "visualitem.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionChangeMode, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   //Take our controllers.  If we can't get them, fail.
   if (!grabControllers())
   {
      BAction::disconnect();
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionChangeMode::disconnect()
{
   //Release our controllers.
   releaseControllers();

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::init()
{
   if(!BAction::init())
      return false;

   mSquadMode = -1;
   mAnimType = -1;
   mOppID = BUnitOpp::cInvalidID;
   mFutureState = cStateNone;
   mFlagConflictsWithIdle = true;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch(state)
   {
      case cStateWorking:
      {
         mAnimType = pUnit->getProtoObject()->getSquadModeAnimType(mSquadMode);
         if (mAnimType == -1)
            return (setState(cStateDone));
         else
         {
            pUnit->setAnimationRate(1.0f);
            pUnit->setAnimation(mID, state, mAnimType, true, false, -1, true);
            pUnit->computeAnimation();
            BASSERT(pUnit->isAnimationSet(state, mAnimType));
            if (pUnit->getAnimationDuration(cActionAnimationTrack) == 0.0f)
               return (setState(cStateDone));
         }
         break;
      }

      case cStateDone:
      case cStateFailed:
      case cStateNone:
      {
         if (state == cStateDone)
            pUnit->completeOpp(mOppID, true);
         else if (state == cStateFailed)
            pUnit->completeOpp(mOppID, false);

         releaseControllers();

         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //If we have a future state, go.
   if (mFutureState != cStateNone)
   {
      setState(mFutureState);
      if ((mFutureState == cStateDone) || (mFutureState == cStateFailed))
         return (true);
      mFutureState=cStateNone;
   }

   switch (mState)
   {
      case cStateNone:
      {
         //At this point, we need to have the controllers to do anything.
         if (!grabControllers())
            break;

         // Can't go on if the animation is locked
         if (pUnit->isAnimationLocked())
            break;

         setState(cStateWorking);
         break;
      }

      case cStateWorking:
      {
         //If we've lost the controllers, go back to None.
         if (!validateControllers())
         {
            setState(cStateNone);
            break;
         }

         // We're done!
         if (pUnit->getAnimationType(cActionAnimationTrack) != mAnimType)
         {
            setState(cStateDone);
            break;
         }

         break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionChangeMode::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   switch (eventType)
   {
      case BEntity::cEventAnimChain:
      case BEntity::cEventAnimLoop:
      case BEntity::cEventAnimEnd:
      {
         if (mState == cStateWorking)
         {
            //-- If our anim just looped, then we're done
            if(senderID == mpOwner->getID() && data2 == cActionAnimationTrack && data == (DWORD)mAnimType && mState==cStateWorking)
               mFutureState = cStateDone;
               //setState(cStateDone);
         }
         break;
      }

      default:
         break;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionChangeMode::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);
      pUnit->updateControllerOppIDs(mOppID, oppID);
   }
   mOppID=oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionChangeMode::getPriority() const
{
//-- FIXING PREFIX BUG ID 5118
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   const BUnitOpp* pOpp=pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 5119
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   //We need them all.
   if (pUnit->getController(BActionController::cControllerOrient)->getActionID() != mID)
      return (false);
   if (pUnit->getController(BActionController::cControllerAnimation)->getActionID() != mID)
      return (false);
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::grabControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Take them all.
   if (!pUnit->grabController(BActionController::cControllerOrient, this, getOppID()))
      return (false);
   if (!pUnit->grabController(BActionController::cControllerAnimation, this, getOppID()))
   {
      pUnit->releaseController(BActionController::cControllerOrient, this);
      return (false);
   }
   
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionChangeMode::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, long, mSquadMode);
   GFWRITEVAR(pStream, long, mAnimType);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, BActionState, mFutureState);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionChangeMode::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, long, mSquadMode);
   GFREADVAR(pStream, long, mAnimType);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, BActionState, mFutureState);
   gSaveGame.remapAnimType(mAnimType);
   return true;
}
