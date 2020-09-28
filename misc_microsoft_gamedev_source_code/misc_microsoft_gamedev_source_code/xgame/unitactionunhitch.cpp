//==============================================================================
// unitactionhitch.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionunhitch.h"
#include "entity.h"
#include "player.h"
#include "protoobject.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"
#include "UnitOpportunity.h"
#include "syncmacros.h"
#include "visualitem.h"
#include "visual.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionUnhitch, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   // Connect.
   if (!BAction::connect(pOwner, pOrder))
   {
      return (false);
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::disconnect()
{
   //Release our controllers.
   releaseControllers();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::init(void)
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;

   mTarget.reset();
   mOppID = BUnitOpp::cInvalidID;
   mNoAnimTimer = 0.0f;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::setState(BActionState state)
{
   syncUnitActionData("BUnitActionUnhitch::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionUnhitch::setState state", state);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (state)
   {
      case cStateWorking:         
         // Play unhitch animation
         playUnhitchAnimation();
         break;

      case cStateDone:
      case cStateFailed:
         if (state == cStateDone)
         {
            pUnit->completeOpp(mOppID, true);
         }
         else if (state == cStateFailed)
         {
            pUnit->completeOpp(mOppID, false);
         }

         // Release our controllers
         releaseControllers();

         // Idle the anim.
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
         {
            if (!validateTarget())
            {
               setState(cStateDone);
               break;
            }

            // Check range.
            //if (!validateRange())
            //{
            //   setState(cStateFailed);
            //   break;
            //}

            // At this point, we need to have the controllers to do anything.
            if (!grabControllers())
            {
               break;
            }

            // Can't go on if the animation is locked
            if (pUnit->isAnimationLocked())
               break;

            // Idle the unit
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
            // Idle the target
            BEntityID targetID = mTarget.getID();
            BUnit* pTarget = gWorld->getUnit(targetID);
            if (pTarget)
            {
               pTarget->setAnimation(targetID, BObjectAnimationState::cAnimationStateIdle);
            }

            // Go.
            syncUnitActionCode("BUnitActionUnhitch::update stateWorking");
            setState(cStateWorking);
         }
         break;

      case cStateWorking:
         {
            // If we've lost the controllers, go back to None.
            if (!validateControllers())
            {
               setState(cStateNone);
               break;
            }

            //Check target.
            if (!validateTarget())
            {
               syncUnitActionCode("BUnitActionHitch::update stateDone");
               setState(cStateDone);
               break;
            }

            //Check range.
            //if (!validateRange())
            //{
            //   setState(cStateFailed);
            //   break;
            //}

            // If we have no animation, call the fugly fake-it-out function.
            if (mFlagMissingAnim)
            {
               updateNoAnimTimer(elapsed);
            }
         }
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BASSERT(mpOwner);
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5051
   const BUnit* pTarget = gWorld->getUnit(mTarget.getID());
//--
   BASSERT(pTarget);

   switch (eventType)
   {
      case BEntity::cEventAnimChain:
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimLoop:
         if (data2 == cActionAnimationTrack)
         {
            if (validateTarget())
            {
               // unhitch unit
               pUnit->unhitchUnit(pTarget->getID());
            }
            else
            {
               // We're done here
               syncUnitActionCode("BUnitActionUnhitch::update stateFailed");
               setState(cStateFailed);            
               break;
            }

            // We're done here
            syncUnitActionCode("BUnitActionUnhitch::update stateDone");
            setState(cStateDone);            
         }
         break;      
   }
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));

   mTarget = target;
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::setOppID(BUnitOppID oppID)
{
   if (mpOwner)
   {
      BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BASSERT(pUnit);

      pUnit->updateControllerOppIDs(mOppID, oppID);
   }

   mOppID = oppID;
}

//==============================================================================
//==============================================================================
uint BUnitActionUnhitch::getPriority() const
{
//-- FIXING PREFIX BUG ID 5052
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   const BUnitOpp* pOpp = pUnit->getOppByID(mOppID);
   if (!pOpp)
      return (BUnitOpp::cPriorityNone);
   return (pOpp->getPriority());
}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 5053
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   bool valid = (pUnit->getController(BActionController::cControllerOrient)->getActionID() == mID);
   if (!valid)
      return (false);

   valid = (pUnit->getController(BActionController::cControllerAnimation)->getActionID() == mID);

   return (valid);
}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.  
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   // Try to grab the attack controller so that units won't attack when they are unhitching
   grabbed = pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());

   return (grabbed);
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release the orientation controller
   pUnit->releaseController(BActionController::cControllerOrient, this);   

   // Release the animation controller
   pUnit->releaseController(BActionController::cControllerAnimation, this);
}

//==============================================================================
//==============================================================================
//bool BUnitActionUnhitch::validateRange() const
//{
//   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//   BASSERT(pUnit);
//
//   BSquad* pSquad = pUnit->getParentSquad();
//   BASSERT(pSquad);
//
//   //Get our range as defined by our PA.
//   float range = 0.0f;
//   if (mTarget.isRangeValid())
//   {
//      range = mTarget.getRange();
//   }
//   else if (mpProtoAction)
//   {
//      range = mpProtoAction->getMaxRange(pUnit);
//   }
//
//   //See if our squad is in range of our target.
//   if (mTarget.getID().isValid())
//   {
//      BEntity* pEntity = gWorld->getEntity(mTarget.getID());
//      if (!pEntity)
//      {
//         return (false);
//      }
//
//      return (pSquad->calculateXZDistance(pEntity) <= range);      
//   }
//
//   if (!mTarget.isPositionValid())
//   {
//      return (false);
//   }
//
//   return (pSquad->calculateXZDistance(mTarget.getPosition()) <= range);
//}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::validateTarget() const
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 5054
      const BEntity* pEnt = gWorld->getEntity(mTarget.getID());
//--
      if (!pEnt)
      {
         return (false);
      }

      return (pEnt->isAlive());
   }

   return (mTarget.isPositionValid());
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::playUnhitchAnimation()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Set new animation.
   int animType = mpProtoAction ? mpProtoAction->getAnimType() : -1;
   if ((animType != -1) && (animType != pUnit->getAnimationType(cActionAnimationTrack)))
   {
      BEntityID targetID = mTarget.getID();
      if ((animType == cAnimTypeUnhitch) && targetID.isValid())
      {
         BUnit* pTarget = gWorld->getUnit(targetID);
         if (pTarget)
         {              
            pTarget->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, animType);
            pTarget->computeAnimation();
            BASSERT(pTarget->isAnimationSet(BObjectAnimationState::cAnimationStateMisc, animType));

            // Get the length from the animation if it exists
            float animLen = pUnit->getAnimationDuration(cActionAnimationTrack);
            if (animLen == 0.0f)
            {
               mFlagMissingAnim = true;
            }
         }
      }
   }
   else
   {
      mFlagMissingAnim = true;
   }

   if (mFlagMissingAnim)
   {
      mNoAnimTimer = 0.0f;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionUnhitch::updateNoAnimTimer(float elapsedTime)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   mNoAnimTimer += elapsedTime;
   //XXXHalwes - 6/15/2007 - What should the default rate be?
   //float workRate = mpProtoAction ? mpProtoAction->getWorkRate() : 0.0f;
   float workRate = 0.0f;
   if (mNoAnimTimer >= workRate)
   {
      //Fake end of hitch animation.
      pUnit->notify(BEntity::cEventAnimEnd, pUnit->getID(), pUnit->getAnimationType(cActionAnimationTrack), cActionAnimationTrack);
      mNoAnimTimer = 0.0f;
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, float, mNoAnimTimer);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionUnhitch::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, float, mNoAnimTimer);
   return true;
}
