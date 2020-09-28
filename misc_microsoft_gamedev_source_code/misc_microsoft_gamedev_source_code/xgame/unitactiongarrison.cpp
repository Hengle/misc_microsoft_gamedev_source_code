//==============================================================================
// unitactiongarrison.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactiongarrison.h"
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
#include "physics.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionGarrison, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionGarrison::connect(BEntity* pOwner, BSimOrder* pOrder)
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
void BUnitActionGarrison::disconnect()
{
   //Release our controllers.
   BASSERT(mpOwner);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // We don't need physics while we're garrisoned
   if (pUnit->getPhysicsObject())
      pUnit->getPhysicsObject()->forceDeactivate();

   if (!mFlagNotified)
   {
      BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
      if (pTargetUnit)
      {
         pTargetUnit->notify(BEntity::cEventGarrisonFail, pUnit->getID(), 0, 0);

         // Idle the anim.
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
      }
   }

   releaseControllers();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionGarrison::init(void)
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;

   mTarget.reset();
   mOppID = BUnitOpp::cInvalidID;
   mNoAnimTimer = 0.0f;
   mIgnoreRange = false;
   mFlagNotified = false;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionGarrison::setState(BActionState state)
{
   syncUnitActionData("BUnitActionGarrison::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionGarrison::setState state", state);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (state)
   {
      case cStateNone:
         //Idle the anim.
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         break;

      case cStateWorking:
         {
            if (!validateTarget())
            {
               setState(cStateFailed);
               return (true);
            }

            // Have enough room?
            BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
            if (!pTargetUnit || !pTargetUnit->canContain(pUnit))
            {
               setState(cStateFailed);
               return (true);
            }                        

            // Play cover animation
            bool useCover = false;
            if (pTargetUnit->hasAvailableCover())
            {
               useCover = playCoverAnimation();
            }

            // Play garrison animation
            if (!useCover)
            {
               playGarrisonAnimation();
            }

            pTargetUnit->notify(BEntity::cEventGarrisonStart, pUnit->getID(), 0, 0);
         }
         break;

      case cStateDone:
      case cStateFailed:
         if (state == cStateDone)
         {
            BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
            if (pTargetUnit)
            {
               pTargetUnit->notify(BEntity::cEventGarrisonEnd, pUnit->getID(), 0, 0);
               mFlagNotified = true;
            }
            pUnit->completeOpp(mOppID, true);
         }
         else if (state == cStateFailed)
         {
            BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
            if (pTargetUnit)
            {
               pTargetUnit->notify(BEntity::cEventGarrisonFail, pUnit->getID(), 0, 0);
               mFlagNotified = true;
            }
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
bool BUnitActionGarrison::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (mState)
   {
      case cStateNone:
         {
            if (!validateTarget())
            {
               setState(cStateFailed);
               break;
            }

            // Check range.
            if (!validateRange())
            {
               setState(cStateFailed);
               break;
            }

            // At this point, we need to have the controllers to do anything.
            if (!grabControllers())
            {
               break;
            }

            // Can't go on if the animation is locked
            if (pUnit->isAnimationLocked())
               break;

            // Go.
            syncUnitActionCode("BUnitActionGarrison::update stateWorking");
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
               syncUnitActionCode("BUnitActionGarrison::update stateDone");
               setState(cStateFailed);
               break;
            }

            // Enough room?
//-- FIXING PREFIX BUG ID 5004
            const BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
//--
            if (!pTargetUnit || !pTargetUnit->canContain(pUnit))
            {
               setState(cStateFailed);
               break;
            }            

            //Check range.
            if (!validateRange())
            {
               setState(cStateFailed);
               break;
            }

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
void BUnitActionGarrison::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BASSERT(mpOwner);
//-- FIXING PREFIX BUG ID 4994
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);
   BUnit* pTarget = gWorld->getUnit(mTarget.getID());

   switch (eventType)
   {
      case BEntity::cEventAnimChain:
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimLoop:
         if (data2 == cActionAnimationTrack)
         {
            if (validateTarget())
            {
               if (pUnit->getFlagAttached())
               {
                  // We're done here
                  syncUnitActionCode("BUnitActionGarrison::update stateDone");
                  setState(cStateDone);
               }
               else if (pTarget->canContain(pUnit))
               {
                  // Stop unit and garrison it
                  pTarget->containUnit(pUnit->getID());

                  // We're done here
                  syncUnitActionCode("BUnitActionGarrison::update stateDone");
                  setState(cStateDone);
               }
               else
               {
                  syncUnitActionCode("BUnitActionGarrison::update stateFailed");
                  setState(cStateFailed);            
               }
            }
            else
            {
               // We're done here
               syncUnitActionCode("BUnitActionGarrison::update stateFailed");
               setState(cStateFailed);            
               break;
            }
         }
         break;      
   }
}

//==============================================================================
//==============================================================================
void BUnitActionGarrison::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));

   mTarget = target;
}

//==============================================================================
//==============================================================================
void BUnitActionGarrison::setOppID(BUnitOppID oppID)
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
uint BUnitActionGarrison::getPriority() const
{
//-- FIXING PREFIX BUG ID 4995
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
bool BUnitActionGarrison::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 4996
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
bool BUnitActionGarrison::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.  
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   // Try to grab the attack controller so that units won't attack when they are garrisoning
   grabbed = pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());

   return (grabbed);
}

//==============================================================================
//==============================================================================
void BUnitActionGarrison::releaseControllers()
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
bool BUnitActionGarrison::validateRange() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

//-- FIXING PREFIX BUG ID 4999
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   if (mIgnoreRange)
      return true;

   //Get our range as defined by our PA.
   float range = 0.0f;
   if (mTarget.isRangeValid())
   {
      range = mTarget.getRange();
   }
   else if (mpProtoAction)
   {
      range = mpProtoAction->getMaxRange(pUnit);
   }

   //See if our squad is in range of our target.
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 4998
      const BEntity* pEntity = gWorld->getEntity(mTarget.getID());
//--
      if (!pEntity)
      {
         return (false);
      }

      return (pSquad->calculateXZDistance(pEntity) <= range);      
   }

   if (!mTarget.isPositionValid())
   {
      return (false);
   }

   return (pSquad->calculateXZDistance(mTarget.getPosition()) <= range);
}

//==============================================================================
//==============================================================================
bool BUnitActionGarrison::validateTarget() const
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 5000
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
void BUnitActionGarrison::playGarrisonAnimation()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   //Set new animation.
   int animType = mpProtoAction ? mpProtoAction->getAnimType() : -1;
   if ((animType != -1) && (animType != pUnit->getAnimationType(cActionAnimationTrack)))
   {
      if (animType == cAnimTypePelicanGarrison)
      {
         const BProtoObject* pUnitProtoObject = pUnit->getProtoObject();
         if (pUnitProtoObject && pUnitProtoObject->isType(gDatabase.getOTIDGroundVehicle()) && mTarget.getID().isValid())
         {
            // Only attach one vehicle for transporting
            BUnit* pTarget = gWorld->getUnit(mTarget.getID());
            if (pTarget && !pTarget->getFlagHasAttached() && pTarget->isType(gDatabase.getOTIDUnsc()))
            {  
//-- FIXING PREFIX BUG ID 5001
               const BVisual* pTargetVisual = pTarget->getVisual();
//--
               if (pTargetVisual)
               {
                  //XXXHalwes - 6/27/2007 - Need to add code to attach to a from bone
                  pUnit->setForward(pTarget->getForward());
                  pUnit->setRight(pTarget->getRight());
                  pUnit->calcUp();

                  long boneHandle = -1;
                  const float* unitBBExt = pUnit->getSimBoundingBox() ? pUnit->getSimBoundingBox()->getExtents() : NULL;
                  static const float cSmallAttachThreshold = 3.5f;
                  if(unitBBExt && unitBBExt[1] < cSmallAttachThreshold)
                     boneHandle = pTargetVisual->getBoneHandle("bone_attachpoint_sm_veh");
                  else
                     boneHandle = pTargetVisual->getBoneHandle("bone_attachpoint");                  
                  if (boneHandle != -1)
                  {
                     BVector bonePos;
                     BMatrix worldMatrix;
                     pTarget->getWorldMatrix(worldMatrix);
                     pTargetVisual->getBone(boneHandle, &bonePos, NULL, NULL, &worldMatrix, false);                        
                     #ifdef SYNC_Unit
                        syncUnitData("BUnitActionGarrison::playGarrisonAnimation", bonePos);
                     #endif
                     pUnit->setPosition(bonePos, true);
                     pTarget->attachObject(pUnit->getID(), boneHandle);                  
                  }
               }                  
            }            
         }
         else
         {
            animType += findAnimOffset();
         }
      }
      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateGarrison, animType);
      pUnit->computeAnimation();
      BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateGarrison, animType));

      // Get the attack rate from the animation if it exists
      float animLen = pUnit->getAnimationDuration(cActionAnimationTrack);
      if (animLen == 0.0f)
      {
         mFlagMissingAnim = true;
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
// Play the appropriate cover animation
//==============================================================================
bool BUnitActionGarrison::playCoverAnimation()
{
   bool result = false;
//-- FIXING PREFIX BUG ID 5002
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   // Only infantry can take cover
   const BProtoObject* pUnitProtoObject = pUnit->getProtoObject();
   if (pUnitProtoObject && pUnitProtoObject->isType(gDatabase.getOTIDInfantry()))
   {
      //Set new animation.
      float animLen = 0.0f;
      //int animType = mpProtoAction ? mpProtoAction->getAnimType() : -1;
      //XXXHalwes - 2/5/2008 - Cover animation solution
      if (1 /*(animType != -1) && (animType != pUnit->getAnimationType(cActionAnimationTrack))*/)
      {
         //XXXHalwes - 2/5/2008 - Cover animation solution
         if (0 /*animType == cAnimTypeTakeCover*/)
         {
            //pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateGarrison, animType);
            //pUnit->computeAnimation();
            //BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateGarrison, animType));

            //// Get the attack rate from the animation if it exists
            //animLen = pUnit->getAnimationDuration(cActionAnimationTrack);            
         }

         if (animLen == 0.0f)
         {
            mFlagMissingAnim = true;
         }

         result = true;
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

   return (result);
}

//==============================================================================
//==============================================================================
void BUnitActionGarrison::updateNoAnimTimer(float elapsedTime)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   mNoAnimTimer += elapsedTime;
   //XXXHalwes - 6/15/2007 - What should the default rate be?
   //float workRate = mpProtoAction ? mpProtoAction->getWorkRate() : 0.0f;
   float workRate = 0.0f;
   if (mNoAnimTimer >= workRate)
   {
      //Fake end of garrison animation.
      pUnit->notify(BEntity::cEventAnimEnd, pUnit->getID(), pUnit->getAnimationType(cActionAnimationTrack), cActionAnimationTrack);
      mNoAnimTimer = 0.0f;
   }
}

//==============================================================================
//==============================================================================
int BUnitActionGarrison::findAnimOffset()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
   {
      return (0);
   }

//-- FIXING PREFIX BUG ID 5003
   const BSquad* pParentSquad = pUnit->getParentSquad();
//--
   if (!pParentSquad)
   {
      return (0);
   }

   uint numChildren = pParentSquad->getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
      if (pUnit->getID() == pParentSquad->getChild(i))
      {
         return (Math::Min(i, (uint)10));
      }
   }

   return (0);
}


//==============================================================================
//==============================================================================
bool BUnitActionGarrison::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, float, mNoAnimTimer);
   GFWRITEBITBOOL(pStream, mIgnoreRange);
   GFWRITEBITBOOL(pStream, mFlagNotified);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionGarrison::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, float, mNoAnimTimer);
   GFREADBITBOOL(pStream, mIgnoreRange);
   GFREADBITBOOL(pStream, mFlagNotified);
   return true;
}
