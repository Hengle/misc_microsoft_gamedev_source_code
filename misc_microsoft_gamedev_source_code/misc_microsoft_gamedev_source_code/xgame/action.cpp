//==============================================================================
// action.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "action.h"
#include "actionmanager.h"
#include "database.h"
#include "SimOrder.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "unit.h"
#include "visual.h"
#include "world.h"
#include "tactic.h"
#include "protoobject.h"
#include "squad.h"
#include "unitactionjoin.h"
#include "tactic.h"
#include "configsgame.h"
#include "squadlosvalidator.h"

//#define DEBUGACTION 

GFIMPLEMENTVERSION(BAction, 52);

//==============================================================================
//==============================================================================
void BActionTriggerModule::notifyActionStatus(long status)
{
   // Early bail.
   if (mTriggerScriptID < 0 || mTriggerVarID == BTriggerVar::cVarIDInvalid)
      return;

   BTriggerScript*  pScript  = gTriggerManager.getTriggerScript(mTriggerScriptID);
   if (!pScript)
      return;

   BTriggerVar *pVar = pScript->getTriggerVar(mTriggerVarID);
   if (!pVar)
      return;

   pVar->asActionStatus()->writeVar(status); 
}



//==============================================================================
//==============================================================================
BAction::BAction()
{
}

//==============================================================================
//==============================================================================
BAction::~BAction()
{
}

//==============================================================================
//==============================================================================
bool BAction::init()
{
   //This is a simple "Init my memory to default values" method.
   //DO NOT DO ANY LOGICAL OPERATIONS in any init() methods.  Those things
   //should go in connect().

   mpOwner=NULL;
   mpOrder=NULL;
   mpProtoAction=NULL;
   mID=cInvalidActionID;
   mState=cStateNone;

   mFlagActive=true;
   mFlagDestroy=false;
   mFlagPersistent=false;
   mFlagFromTactic=false;
   mFlagConflictsWithIdle=false;
   mFlagMissingAnim=false;
   mFlagAIAction=false;
   mFlagCanHaveMultipleOfType=false;
   
   return(true);
}

//==============================================================================
//==============================================================================
bool BAction::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(pOwner);
   BDEBUG_ASSERT(!mpOwner);

   mpOwner=pOwner;
   mpOrder=pOrder;
   if (mpOrder)
      mpOrder->incrementRefCount();

   #ifdef DEBUGACTION
   mpOwner->debug("%s connect, ID=%d.", getTypeName(), mID);
   #endif

   return(true);
}

//==============================================================================
//==============================================================================
void BAction::disconnect()
{
   #ifdef DEBUGACTION
   mpOwner->debug("%s disconnect, ID=%d.", getTypeName(), mID);
   #endif
   
   mFlagDestroy=true;
   mpOwner=NULL;
   if (mpOrder)
   {
      mpOrder->decrementRefCount();
      mpOrder=NULL;
   }
}

//==============================================================================
//==============================================================================
/*bool BAction::update(float elapsed)
{
   BDEBUG_ASSERT(mpOwner);
   BDEBUG_ASSERT(!mpOrder || mpOrder->getValid());

   if (mFlagDestroy)
      return(false);
   if (!mFlagActive)
      return(true);

   //Remove the action if the unit is dead and this action doesn't stick around through death.
   if (!mFlagPersistWhenDead && mpOwner && !mpOwner->isAlive())
      return(false);

   return(true);
}*/

//==============================================================================
//==============================================================================
BActionType BAction::getType() const
{
   return(static_cast <BActionType> (ACTIONTYPEFROMID(mID) ));
}

//==============================================================================
//==============================================================================
const char* BAction::getTypeName() const
{
   return gActionManager.getActionName(getType());
}

//==============================================================================
//==============================================================================
const char* BAction::getStateName(int state) const
{
   int nState = state;
   if (state < 0)
      nState = mState;
   switch (nState)
   {
      case cStateNone: return("None");
      case cStateDone: return("Done");
      case cStateFailed: return("Failed");
      case cStateMoving: return("Moving");
      case cStateWorking: return("Working");
      case cStateWait: return("Wait");
      case cStateAttacking: return("Attacking");
      case cStateLockDown: return("LockDown");
      case cStateUnlock: return("Unlock");
      case cStateFading: return("Fading");
      case cStateLoading: return("Loading");
      case cStateReloading: return("Reloading");
      case cStateBlocked: return("BLOCKED");
      case cStateUnloadingWait: return("UnloadingWait");
      case cStateUnloading: return("Unloading");
      case cStatePathing: return("Pathing");
      case cStateFlyIn: return("FlyIn");
      case cStateFlyInWait: return("FlyInWait");
      case cStateFlyOff: return("FlyOff");
      case cStateFlyOffWait: return("FlyOffWait");
      case cStateIncoming: return("Incoming");
      case cStateReturning: return("Returning");
      case cStateSearch: return("Search");
      case cStateHealingUnits: return("HealingUnits");
      case cStateReinforcingUnits: return("ReinforcingUnits");
      case cStateUndamaged: return("Undamaged");
      case cStateWaitOnOpps: return("WaitOnOpps");
      case cStatePaused: return("Paused");
      case cStateUnpaused: return("Unpaused");
      case cStateInvalid: return("Invalid");
      case cStateTurning: return("Turning");
      case cStateStartMove: return("StartMove");
      case cStateStopMove: return("StopMove");
      case cStateRamming: return("Ramming");
      case cStateAdvanceWP: return("AdvanceWP");
      case cStateWaitOnChildren: return ("WaitOnChildren");
	}
	return("Unknown");
}

//==============================================================================
//==============================================================================
bool BAction::setState(BActionState state) 
{
   BDEBUG_ASSERT(mpOwner || state == cStateDone || state == cStateFailed);
   #ifdef DEBUGACTION
   mpOwner->debug("%s ID=%d, setState=%d.", getTypeName(), mID, state);
   #endif

   //TODO: This should return true, IMO.
   if (mState == state)
      return(false);

   switch (state)
   {
      case cStateFailed:
      case cStateDone:
         mFlagDestroy=true;
         break;
      default:
         break;
   }

   mState=state;

   //sendStateChangeEvent();
   return(true);
}

//==============================================================================
//==============================================================================
bool BAction::conflicts(const BAction* pAction) const
{
   //Persistent actions don't ever conflict.
   if (mFlagPersistent || pAction->getFlagPersistent())
      return (false);
   //Paused actions don't ever conflict.
   if ((mState == cStatePaused) || (pAction->mState == cStatePaused))
      return (false);

   //All actions of the same type conflict right now.
   if (!mFlagCanHaveMultipleOfType && pAction->getType() == this->getType())
      return (true);

   //If this action conflicts with idle, see if pAction is an idle action.
   if (mFlagConflictsWithIdle)
   {
      if (pAction->getType() == BAction::cActionTypeEntityIdle)
         return (true);
   }
   //If this action IS an idle, see if pAction conflicts with idles.
   if (getType() == BAction::cActionTypeEntityIdle)
   {
      if (pAction->getFlagConflictsWithIdle())
         return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
bool BAction::conflicts(BActionType type) const
{
   //Persistent actions don't ever conflict.
   if (mFlagPersistent)
      return (false);
   //Paused actions don't ever conflict.
   if (mState == cStatePaused)
      return (false);

   //All actions of the same type conflict right now.
   if (!mFlagCanHaveMultipleOfType && type == this->getType())
      return(true);

   //If this action conflicts with idle, see if pAction is an idle action.
   if (mFlagConflictsWithIdle)
   {
      if (type == BAction::cActionTypeEntityIdle)
         return(true);
   }

   //DCP 05/18/07: We can't really do this check w/o the instance.  Does that make
   //this too busted to use?
   //If this action IS an idle, see if pAction conflicts with idles.
   /*if (getType() == BAction::cActionTypeEntityIdle)
   {
      if (pAction->getFlagConflictsWithIdle())
         return(true);
   }*/

   return(false);
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BAction::getDebugLine(uint index, BSimString &string) const
{
   switch (index)
   {
      case 0:
         string.format("%s (ID=%d, %s), State=%s, Act=%d, OrdID=%d, ParID=%d, Per=%d.",
            getTypeName(), getID(), mpProtoAction ? mpProtoAction->getName().getPtr() : "Unknown", getStateName(), mFlagActive, mpOrder ? mpOrder->getID(): -1,
            getParentAction() ? getParentAction()->getID(): -1, getFlagPersistent());
         break;
      case 1:
      {
         const BSimTarget* pTarget=getTarget();
         if (!pTarget)
            string.format("Target: NULL.");
         else
            string.format("Target: ID=%s, Pos=(%.2f, %.2f, %.2f)%s, Range=%.2f%s, AbilityID=%d%s.", pTarget->getID().getDebugString().getPtr(),
               pTarget->getPosition().x, pTarget->getPosition().y, pTarget->getPosition().z, pTarget->isPositionValid() ? "(V)" : "",
               pTarget->getRange(), pTarget->isRangeValid() ? "(V)" : "", pTarget->getAbilityID(), pTarget->isAbilityIDValid() ? "(V)" : "");
      }
   }
}
#endif

//==============================================================================
//==============================================================================
bool BAction::sendStateChangeEvent(void)
{
   bool eventSent = false;

   switch(mState)
   {
      case cStateNone:
      case cStateDone:
      case cStateFailed:
         mpOwner->sendEvent(mpOwner->getID(), mpOwner->getID(), BEntity::cEventActionStateChange, mID);
         eventSent = true;
         break;

      default:
         break;
   }
   
   return eventSent;
}

//==============================================================================
//==============================================================================
bool BAction::validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const
{
//-- FIXING PREFIX BUG ID 3422
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   if (senderID != pUnit->getID())
      return false;

   switch (eventType)
   {
   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         const BVisual* pVisual = pUnit->getVisual();
         return (pVisual && (pVisual->getAnimationType(data2) == (long)data));
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BAction::limitVelocity(float elapsedTime, float desiredVelocity, float& velocity, bool useUpperBound)
{
   //Velocity is our current velocity.  It will be limited/modified coming
   //out of this method.
   BDEBUG_ASSERT(mpOwner);

   //Get our acceleration potential.
   float velocityDiff=(float)fabs(velocity-desiredVelocity);
   float acceleration=mpOwner->getAcceleration();
   acceleration*=elapsedTime;

   //Limit velocity changes as needed.
   if (velocityDiff > acceleration)
      velocityDiff=acceleration;
   //Slow down.
   if (velocity > desiredVelocity)
   { 
      velocity-=velocityDiff;
      if (velocity < desiredVelocity)
         velocity=desiredVelocity;
   }
   //Speed up.
   else
   {
      velocity+=velocityDiff;
      if (velocity > desiredVelocity)
         velocity=desiredVelocity;
   }

   //Bound velocity.
   if (velocity < 0.0f)
      velocity=0.0f;
   else if (useUpperBound && (velocity > mpOwner->getMaxVelocity()))
      velocity=mpOwner->getMaxVelocity();
   
   return (true);
}


//==============================================================================
//==============================================================================
bool BAction::validateUnitAttackRange(const BSimTarget& target, DWORD& lastValidationTime) const
{
   // Not to be called by squad actions!

   //Get the unit and squad things upfront.
   BUnit* pUnit=mpOwner->getUnit();
   if (!pUnit)
      return (false);
   BSquad* pSquad=pUnit->getParentSquad();
   if (!pSquad)
      return false;
   if (!mpProtoAction)
      return (false);
   //Get the target and target squad.  If we have a target, we must have a target squad.
   const BUnit* pTargetUnit=gWorld->getUnit(target.getID());
   BSquad *pTargetSquad=NULL;
   if (pTargetUnit)
   {
      pTargetSquad=pTargetUnit->getParentSquad();
      if (!pTargetSquad)
         return (false);
   }
   
   //If we have a non-zero validation time, see if we're okay to skip this whole thing.
   if (pTargetUnit)
   {
      if ((lastValidationTime > (DWORD)0) &&
         (lastValidationTime >= pUnit->getLastMoveTime()) &&
         (lastValidationTime >= pSquad->getLastMoveTime()) &&
         (lastValidationTime >= pTargetUnit->getLastMoveTime()) &&
         (lastValidationTime >= pTargetSquad->getLastMoveTime()) )
         return (true);
   }
   
   // If we're attacking our target's target, we're always in range if the squad
   // we're attached to is attacking something
   // Used for protectors to ensure they're always in range of the target of the
   // squad they're attached to.
   if (mpProtoAction->getFlagTargetOfTarget())
   {
      if (!pTargetUnit || !pTargetSquad)
         return (false);
   
      const BUnitActionJoin* pJoin = static_cast<BUnitActionJoin*>(pUnit->getActionByType(BAction::cActionTypeUnitJoin));
      if (pJoin && pJoin->getAttackTarget() != NULL && pJoin->getAttackTarget()->isIDValid())
      {
         pTargetSquad = gWorld->getSquad(pJoin->getAttackTarget()->getID());
         return (true);
      }
   }

   BTactic* pTactic = pUnit->getTactic();
   // Do weapon group range calculation if necessary
   BWeapon* pWeapon = (BWeapon*)pTactic->getWeapon(mpProtoAction->getWeaponID());
   if (pWeapon && pWeapon->getFlagUseGroupRange() && (pUnit->getGroupDiagSpan() > 0.0f))
   {
      if(!isTargetInRangeOfWeaponGroup(pWeapon, target))
         return(false);
   }
   else
   {

      //Min range is always defined by PA.
      float minRange=mpProtoAction->getMinRange();
      //Get our max range as defined by our sim target vs. PA.
      float maxRange;
      if (target.isRangeValid())
         maxRange=target.getRange();
      else
         maxRange=mpProtoAction->getMaxRange(pUnit);

      //Determine our range.
      float range;
      if (target.isRangeValid())
         range=target.getRange();
      else
         range=mpProtoAction->getMaxRange(pUnit);
      
      //If we're doing the melee range calc, we don't want a fudge factor at all.
      float fudgeFactor=1.0f;
      if (mpProtoAction->getMeleeRange())
         fudgeFactor=0.0f;

      //See if we're GTG.
      if(!pSquad->isChildInRange(mpProtoAction->getMeleeRange(), pUnit->getID(), target, minRange, maxRange, fudgeFactor))
         return(false);
   }


   if (gConfig.isDefined(cConfigTrueLOS) && pTargetSquad)
   {
      // Only do true LOS check if this is a secondary attack.  Otherwise the squad attack will handle this check.
      const BUnitOpp* pCurrentOpp = pUnit->getOppByID(getOppID());
      if (!pCurrentOpp || (pCurrentOpp->getType() == BUnitOpp::cTypeSecondaryAttack))
      {
         // Since we are inrange, now also check for line of sight
         if(!pSquad->validateLOS(mpProtoAction, pTargetSquad))
            return(false);
      }
   }

   //If we made it all the way here, timestamp our validation time to save us some
   //iterations in the future.
   lastValidationTime=gWorld->getGametime();

   return(true);
}


//==============================================================================
//==============================================================================
bool BAction::isTargetInRangeOfWeaponGroup(BWeapon* pWeapon, const BSimTarget& target) const
{
   // This function currently presumes a group of exactly four weapon locations that form a rectangle when projected onto the XZ plane.
   // "In range" of the weapon group means that any and all weapons in the group (on turrets around a base) are allowed to fire -
   // this is subject to a possible further refinement that occludes weapons which do not have direct line-of-sight to the target.
   // The "in range" region is a rectangle with rounded corners and is composed of radial ranges from each of the corner positions,
   // and a volume inscribed by the tangents connecting the outer extents of the corner circles. Inside - true. Outside - false.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 3430
   const BUnit* pTarget = NULL;
//--

   BVector tgtPos = cInvalidVector;
   if (target.getID().isValid())
   {
      if (target.getID().getType() == BEntity::cClassTypeUnit)
      {
         pTarget = gWorld->getUnit(target.getID());
         if (!pTarget || !pTarget->getParentSquad())
            return (false);

         tgtPos = pTarget->getPosition();
      }
   }

//-- FIXING PREFIX BUG ID 3431
   const BOPQuadHull* hull = pUnit->getGroupHull();
//--
   if (hull && pTarget)
   {
      float fDist = hull->distance(tgtPos);

      fDist -= pTarget->getObstructionRadius();
      if (fDist <= mpProtoAction->getMaxRange(pUnit, false))
         return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BAction::setAttachmentAnimationLock(bool newVal, bool yaw)
{
   if (!mpOwner || !mpProtoAction)
      return;

   BUnit* pUnit = mpOwner->getUnit();
   if (!pUnit)
      return;

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   BVisual* pVisual = pUnit->getVisual();

   if (pHP && pVisual)
   {
      BVisualItem* pAttachment = NULL;
      if (yaw)
         pAttachment = pVisual->getAttachment(pHP->getYawAttachmentHandle());
      else
         pAttachment = pVisual->getAttachment(pHP->getPitchAttachmentHandle());

      if (pAttachment)
      {
         pAttachment->setAnimationLock(cActionAnimationTrack, newVal);
         pAttachment->setAnimationLock(cMovementAnimationTrack, newVal);
      }
   }
}

//==============================================================================
//==============================================================================
bool BAction::checkAttachmentAnimationSet(long animType)
{
   // can't have an anim set if you don't have one... 
   if (!mpProtoAction || !mpOwner)
      return false;

   BUnit* pUnit = mpOwner->getUnit(); 
   if (!pUnit)
      return false;

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   BVisual* pVisual = pUnit->getVisual();
   if (!pHP || !pVisual)
      return false;

   // check to see if the yaw attachment is playing the anim (if we have one) 
   BVisualItem* pYawAttachment = pVisual->getAttachment(pHP->getYawAttachmentHandle());
   if (pYawAttachment && pYawAttachment->getAnimationType(cActionAnimationTrack) != animType)
      return false;

   // check to see if the yaw attachment is playing the anim (if we have one) 
   BVisualItem* pPitchAttachment = pVisual->getAttachment(pHP->getPitchAttachmentHandle());
   if (pPitchAttachment && pPitchAttachment->getAnimationType(cActionAnimationTrack) != animType)
      return false;

   return true;
}

//==============================================================================
//==============================================================================
bool BAction::save(BStream* pStream, int saveType) const
{
   GFWRITEVAL(pStream, BEntityID, (mpOwner ? mpOwner->getID() : cInvalidObjectID));
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpOrder);
   GFWRITEPROTOACTIONPTR(pStream, mpProtoAction);
   GFWRITEVAR(pStream, BActionID, mID);
   GFWRITEVAR(pStream, BActionState, mState);
   GFWRITEBITBOOL(pStream, mFlagActive);
   GFWRITEBITBOOL(pStream, mFlagDestroy);
   GFWRITEBITBOOL(pStream, mFlagPersistent);
   GFWRITEBITBOOL(pStream, mFlagFromTactic);
   GFWRITEBITBOOL(pStream, mFlagConflictsWithIdle);
   GFWRITEBITBOOL(pStream, mFlagMissingAnim);
   GFWRITEBITBOOL(pStream, mFlagAIAction);
   GFWRITEBITBOOL(pStream, mFlagCanHaveMultipleOfType);
   return true;
}

//==============================================================================
//==============================================================================
int tempDebug = 0;
bool BAction::load(BStream* pStream, int saveType)
{
   if (!init())
      return false;

   // Load the owner ID into the mpOwner variable and convert to entity pointer in postLoad.
   BEntityID ownerID;
   GFREADVAR(pStream, BEntityID, ownerID);
   mpOwner = gWorld->getEntity(ownerID);

   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpOrder);

   // mpProtoAction
   BProtoAction* pProtoAction = (BProtoAction*)mpProtoAction;
   GFREADPROTOACTIONPTR(pStream, pProtoAction);
   mpProtoAction = (const BProtoAction*)pProtoAction;

   GFREADVAR(pStream, BActionID, mID);
   GFREADVAR(pStream, BActionState, mState);
   GFREADBITBOOL(pStream, mFlagActive);
   GFREADBITBOOL(pStream, mFlagDestroy);
   GFREADBITBOOL(pStream, mFlagPersistent);
   GFREADBITBOOL(pStream, mFlagFromTactic);
   GFREADBITBOOL(pStream, mFlagConflictsWithIdle);
   GFREADBITBOOL(pStream, mFlagMissingAnim);
   GFREADBITBOOL(pStream, mFlagAIAction);
   GFREADBITBOOL(pStream, mFlagCanHaveMultipleOfType);
   return true;
}

//==============================================================================
//==============================================================================
bool BAction::postLoad(int saveType)
{
   return true;
}
