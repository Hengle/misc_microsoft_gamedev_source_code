//==============================================================================
// unitactionungarrison.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionungarrison.h"
#include "entity.h"
#include "player.h"
#include "protoobject.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"
#include "usermanager.h"
#include "user.h"
#include "selectionmanager.h"
#include "visualitem.h"
#include "syncmacros.h"
#include "visual.h"
#include "physics.h"
#include "physicsinfomanager.h"
#include "physicsinfo.h"
#include "protoobject.h"
#include "squadactionungarrison.h"
#include "pather.h"
#include "simhelper.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionUngarrison, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   // Connect.
   if (!BAction::connect(pOwner, pOrder))
   {
      return (false);
   }

   BASSERT(mpOwner);
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // We don't need physics running
   if (pUnit->getPhysicsObject())
      pUnit->getPhysicsObject()->forceDeactivate();

   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());

   if (!pTargetSquad)
   {
      BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
      if (!pTargetUnit)
         return false;

      pTargetSquad = pTargetUnit->getParentSquad();

      if (!pTargetSquad)
         return false;
   }

   BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();

   // Get source from squad action
   BSquad* pActionSquad = gWorld->getSquad(mSourceID);

   if (!pActionSquad)
      return false;

   BSquadActionUngarrison* pAction = (BSquadActionUngarrison*)pActionSquad->getActionByType(BAction::cActionTypeSquadUngarrison);
   if (!pAction)
      return false;

   mSource = pAction->getSource();

   BSquad* pSourceSquad = gWorld->getSquad(mSource.getID());

   if (!pSourceSquad)
   {
      BUnit* pSourceUnit = gWorld->getUnit(mSource.getID());
      if (!pSourceUnit)
         return false;

      pSourceSquad = pSourceUnit->getParentSquad();

      if (!pSourceSquad)
         return false;
   }

   BUnit* pSourceUnit = pSourceSquad->getLeaderUnit();

   // Doesn't contain squad, bail
   // Halwes - 10/14/2008 - Do not do this check for transporters since it will fail with attached squads
   if (pTargetUnit && pSourceUnit && !pTargetUnit->isType(gDatabase.getOTIDTransporter()) && !pSourceUnit->doesContain(pUnit))
      return false;

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionUngarrison::connect 1", pUnit->getID());
   #endif

   BVector formationPos = pActionSquad->getFormationPosition(pUnit->getID());

   mOffset = (pActionSquad->getPosition() - formationPos);

   pUnit->setAnimationEnabled(true, true);

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionUngarrison::connect 2", pUnit->getID());
   #endif

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionUngarrison::disconnect()
{   
   BASSERT(mpOwner);

   BUnit* pUnit = mpOwner->getUnit();
   if (pUnit)
   {
      pUnit->setFlagIKDisabled(false);
   }

   //Release our controllers.
   releaseControllers();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::init()
{
   if (!BAction::init())
   {
      return (false);
   }

   mFlagConflictsWithIdle = true;

   mTarget.reset();
   mSource.reset();
   mSourceID = cInvalidObjectID;
   mOppID = BUnitOpp::cInvalidID;
   mNoAnimTimer = 0.0f;
   mExitDirection = BProtoObjectStatic::cExitFromFront;
   mSpawnPoint = cInvalidVector;
   mOffset.zero();
   mIgnoreSpawn = false;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::setState(BActionState state)
{
   syncUnitActionData("BUnitActionUngarrison::setState owner ID", mpOwner->getID().asLong());
   syncUnitActionData("BUnitActionUngarrison::setState state", state);

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   switch (state)
   {
      case cStateNone:
         // Idle the anim.
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         break;

      case cStateWorking:
         {
            BSquad* pSourceSquad = gWorld->getSquad(mSource.getID());

            if (!pSourceSquad)
            {
               BUnit* pSourceUnit = gWorld->getUnit(mSource.getID());
               if (!pSourceUnit)
               {
                  setState(cStateFailed);
                  return (false);
               }

               pSourceSquad = pSourceUnit->getParentSquad();

               if (!pSourceSquad)
               {
                  setState(cStateFailed);
                  return (false);
               }
            }

            BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());

            if (!pTargetSquad)
            {
               BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
               if (!pTargetUnit)
               {
                  setState(cStateFailed);
                  return (false);
               }

               pTargetSquad = pTargetUnit->getParentSquad();

               if (!pTargetSquad)
               {
                  setState(cStateFailed);
                  return (false);
               }
            }

            BUnit* pSourceUnit = pSourceSquad->getLeaderUnit();
            BUnit* pTargetUnit = pTargetSquad->getLeaderUnit();
            if (!pTargetUnit || !pSourceUnit)
            {
               setState(cStateFailed);
               return (false);
            }

            pTargetUnit->notify(BEntity::cEventUngarrisonStart, pUnit->getID(), 0, 0);
            
            bool inCover = pUnit->getFlagInCover();
            if (pUnit->getFlagGarrisoned() && pSourceUnit->getFlagHasGarrisoned())
            {
               // ungarrison
               #ifdef SYNC_Unit
                  syncUnitData("BUnitActionUngarrison::setState", pTargetUnit->getPosition());
               #endif
               BVector target = pTargetUnit->getPosition();

               if(mTarget.isPositionValid())
                  target = mTarget.getPosition();

               if (pUnit->getFlagFlying())
                  target.y = pUnit->getPosition().y;
               else
                  gTerrainSimRep.getHeight(target, true);
               pUnit->setPosition(target, false);

               if (pUnit->getPhysicsObject())
                  pUnit->getPhysicsObject()->forceDeactivate();
   
               pUnit->getParentSquad()->setPosition(target, false);
               pUnit->getParentSquad()->setLeashPosition(target);
               pUnit->getParentSquad()->setTurnRadiusPos(target);
               pUnit->getParentSquad()->setTurnRadiusFwd(pUnit->getForward());


               // [10-23-08 CJS] Removing, this causes major physics problems when teleporting
               /*BVector forward = pTargetUnit->getForward();
               forward.y = 0.0f;
               if (!forward.safeNormalize())
               {
                  forward = cZAxisVector;
               }
               if (!pUnit->getFlagFlying())
               {
                  pUnit->setForward(forward);
                  pUnit->setUp(cYAxisVector);
                  pUnit->calcRight();
                  pUnit->clearGoalVector();
               }*/
               pSourceUnit->unloadUnit(pUnit->getID(), false); // Destroys the entity ref!
            }

            #ifdef SYNC_Unit
               syncUnitData("BUnitActionUngarrison::setState before unload 1", inCover);
            #endif

            if (!inCover || !playUncoverAnimation())
            {
               playUngarrisonAnimation();
            }               
         }
         break;

      case cStateDone:
      case cStateFailed:
         {
            BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());

            if (state == cStateDone)
            {
               if (pTargetUnit)
                  pTargetUnit->notify(BEntity::cEventUngarrisonEnd, pUnit->getID(), 0, 0);

               pUnit->completeOpp(mOppID, true);
            }
            else if (state == cStateFailed)
            {
               if (pTargetUnit)
                  pTargetUnit->notify(BEntity::cEventUngarrisonFail, pUnit->getID(), 0, 0);

               pUnit->completeOpp(mOppID, false);
            }

            // Release our controllers
            releaseControllers();

            // Idle the anim.
            pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
         }
         break;
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::update(float elapsed)
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

            // At this point, we need to have the controllers to do anything.
            if (!grabControllers())
            {
               break;
            }

            // Can't go on if the animation is locked
            if (pUnit->isAnimationLocked())
               break;

            // Go.
            syncUnitActionCode("BUnitActionUngarrison::update stateWorking");
            setState(cStateWorking);
         }
         break;

      case cStateWorking:
         {
            // If we've lost the controllers, go back to None.
            if (!validateControllers())
            {
               //setState(cStateNone);
               syncUnitActionCode("BUnitActionUngarrison::update stateDone");
               setState(cStateDone);
               break;
            }

            //Check target.
            if (!validateTarget())
            {
               syncUnitActionCode("BUnitActionUngarrison::update stateDone");
               setState(cStateDone);
               break;
            }

            if (!pUnit->getFlagUseMaxHeight())
            {
               pUnit->tieToGround();
            }

            // If we have no animation, call the fugly fake-it-out function.
            if (mFlagMissingAnim)
            {
               updateNoAnimTimer(elapsed);
            }
            // If the animation is done playing (lock expired) then we're done
            else if (!pUnit->isAnimationLocked())
            {
               setState(cStateDone);            
            }
         }
         break;
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionUngarrison::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   switch (eventType)
   {
      case BEntity::cEventAnimChain:
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimLoop:
         if (data2 == cActionAnimationTrack)
         {
            // We're done here
            syncUnitActionCode("BUnitActionUngarrison::update stateDone");
            setState(cStateDone);            
         }
         break;      
   }
}
void BUnitActionUngarrison::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeUnit));

   mTarget = target;
}

//==============================================================================
//==============================================================================
void BUnitActionUngarrison::setOppID(BUnitOppID oppID)
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
uint BUnitActionUngarrison::getPriority() const
{
//-- FIXING PREFIX BUG ID 4965
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
bool BUnitActionUngarrison::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 4966
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
bool BUnitActionUngarrison::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.  
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   // Try to grab the attack controller so that units won't attack when they are ungarrisoning
   grabbed = pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());

   return (grabbed);
}

//==============================================================================
//==============================================================================
void BUnitActionUngarrison::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release the orientation controller
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::validateTarget() const
{
   if (mTarget.getID().isValid())
   {
//-- FIXING PREFIX BUG ID 4967
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
void BUnitActionUngarrison::playUngarrisonAnimation()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BUnit* pTarget = gWorld->getUnit(mTarget.getID());
   BASSERT(pTarget);

   // Set new animation.
   bool attachedUnit = false;
   bool transportTarget = false;
   const BProtoObject* pTargetProtoObject = pTarget->getProtoObject();
   if (pTargetProtoObject && pTargetProtoObject->isType(gDatabase.getOTIDTransporter()))
   {
      transportTarget = true;
   }

   int animType = mpProtoAction ? mpProtoAction->getAnimType() : -1;

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionUngarrison::playUngarrisonAnimation 0", animType);
   #endif

   if ((animType != -1) && (animType != pUnit->getAnimationType(cActionAnimationTrack)))
   {
      if ((animType == cAnimTypePelicanUngarrison) || (animType == cAnimTypeHotDropDown))
      {                                    
         if (pUnit->getFlagAttached())
         {            
            if (pTarget && pTarget->getFlagHasAttached())
            {  
               pUnit->clearGoalVector();
               pTarget->unattachObject(pUnit->getID());
               attachedUnit = true;

               if (!pUnit->getPhysicsObject() && pUnit->getProtoObject()->getFlagOrientUnitWithGround())
               {
                  pUnit->orientWithGround();
               }
            }            
         }
         else
         {  
            if (pTarget)
            {
               switch (mExitDirection)
               {
                  case BProtoObjectStatic::cExitFromFront:
                     {
                        BVector forward = pTarget->getForward();
                        forward.y = 0.0f;
                        if (!forward.safeNormalize())
                        {
                           forward = cZAxisVector;
                        }
                        pUnit->setForward(forward);
                        pUnit->setUp(cYAxisVector);
                        pUnit->calcRight();
                     }
                     break;

                  case BProtoObjectStatic::cExitFromRight:
                     {
                        BVector forward = pTarget->getRight();
                        forward.y = 0.0f;
                        if (!forward.safeNormalize())
                        {
                           forward = cZAxisVector;
                        }
                        pUnit->setForward(forward);
                        pUnit->setUp(cYAxisVector);
                        pUnit->calcRight();
                     }
                     break;

                  case BProtoObjectStatic::cExitFromLeft:
                     {
                        BVector forward = -pTarget->getRight();
                        forward.y = 0.0f;
                        if (!forward.safeNormalize())
                        {
                           forward = cZAxisVector;
                        }
                        pUnit->setForward(forward);
                        pUnit->setUp(cYAxisVector);
                        pUnit->calcRight();
                     }
                     break;

                  case BProtoObjectStatic::cExitFromFrontRight:
                     {
                        BVector forward = pTarget->getForward();
                        forward.rotateXZ(DEGREES_TO_RADIANS(45.0f));
                        forward.y = 0.0f;
                        if (!forward.safeNormalize())
                        {
                           forward = cZAxisVector;
                        }
                        pUnit->setForward(forward);
                        pUnit->setUp(cYAxisVector);
                        pUnit->calcRight();
                     }
                     break;

                  case BProtoObjectStatic::cExitFromFrontLeft:
                     {
                        BVector forward = pTarget->getForward();
                        forward.rotateXZ(DEGREES_TO_RADIANS(-45.0f));
                        forward.y = 0.0f;
                        if (!forward.safeNormalize())
                        {
                           forward = cZAxisVector;
                        }
                        pUnit->setForward(forward);
                        pUnit->setUp(cYAxisVector);
                        pUnit->calcRight();
                     }
                     break;

                  case BProtoObjectStatic::cExitFromBack:
                     {
                        BVector forward = -pTarget->getForward();
                        forward.y = 0.0f;
                        if (!forward.safeNormalize())
                        {
                           forward = cZAxisVector;
                        }
                        pUnit->setForward(forward);
                        pUnit->setUp(cYAxisVector);
                        pUnit->calcRight();
                     }
                     break;
               }

               BSquad* pParentSquad = pUnit->getParentSquad();
               if (pParentSquad)
               {
                  float numChildren = (float)pParentSquad->getNumberChildren();
                  BVector offset = pUnit->getForward();
                  offset *= (pParentSquad->getObstructionRadius() * 2.0f);
                  offset += pUnit->getPosition();
                  #ifdef SYNC_Unit
                     syncUnitData("BUnitActionUngarrison::playUngarrisonAnimation 1", offset);
                  #endif

                  BVector suggestedPos(0.0f);
                  static BDynamicSimVectorArray instantiatePositions;
                  instantiatePositions.setNumber(0);
                  long closestDesiredPositionIndex;

                  bool validExit = BSimHelper::findInstantiatePositions(1, instantiatePositions, 0.5f, offset,
                     pUnit->getForward(), pUnit->getRight(), pUnit->getObstructionRadius(), offset, closestDesiredPositionIndex, 4, true);

                  // MS 11/14/2008: PHX-17871, if we got a valid exit, we can get our adjusted offset... otherwise don't mess with the unit's position
                  if(validExit && instantiatePositions.getNumber() > 0)
                  {
                     if(closestDesiredPositionIndex >= 0 && closestDesiredPositionIndex < instantiatePositions.getNumber())
                        offset = instantiatePositions[closestDesiredPositionIndex];
                     else
                        offset = instantiatePositions[0];

                     pUnit->setPosition(offset);
                     pParentSquad->setPosition(offset);
                     pParentSquad->setLeashPosition(offset);
                     pParentSquad->setTurnRadiusPos(offset);
                     pParentSquad->setTurnRadiusFwd(pUnit->getForward());
                  }
               }
            }

            if (animType == cAnimTypePelicanUngarrison)
            {
               animType += findAnimOffset();
            }
         }
      }

      // If spawn point specified then move unit to that position before starting anim
      if (!mSpawnPoint.almostEqual(cInvalidVector))
      {
         BVector pos = calculateUngarrisonPosition();
         #ifdef SYNC_Unit
            syncUnitData("BUnitActionUngarrison::playUngarrisonAnimation 2", pos);
         #endif
         // [10-16-08 CJS] Fix units not ungarrisoning from objects that aren't cover correctly
         pUnit->setPosition(pos);
         if (pUnit->getParentSquad())
            pUnit->getParentSquad()->setPosition(pos);
      
         if (pUnit->getPhysicsObject())
            pUnit->getPhysicsObject()->forceDeactivate();
      }

      //XXXHalwes - 7/24/2208 - Debug
      //gpDebugPrimitives->addDebugSphere(pUnit->getPosition(), 5.0f, cDWORDGreen, BDebugPrimitives::cCategoryNone, 10.0f);   

      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateUngarrison, animType, true, false, -1, true);
      pUnit->computeAnimation();
      BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateUngarrison, animType));

      DWORD duration = pUnit->getAnimationDurationDWORD(cActionAnimationTrack);
      mFlagMissingAnim = (duration == 0);

      pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
   }
   else
   {
      mFlagMissingAnim = true;
   }

   if (mFlagMissingAnim)
   {
      mNoAnimTimer = 0.0f;

      // The unit has no ungarrison animation and was not an attached unit or being transported so we need to offset it
      if (pTarget && !attachedUnit && !transportTarget)
      {
         BVector pos = calculateUngarrisonPosition();

         #ifdef SYNC_Unit
            syncUnitData("BUnitActionUngarrison::playUngarrisonAnimation 3", pos);
         #endif
      
         // [10-16-08 CJS] Fix units not ungarrisoning from objects that aren't cover correctly
         pUnit->setPosition(pos);
         if (pUnit->getParentSquad())
            pUnit->getParentSquad()->setPosition(pos);
      
         if (pUnit->getPhysicsObject())
            pUnit->getPhysicsObject()->forceDeactivate();
      }
      pUnit->setFlagUseMaxHeight(false);
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::playUncoverAnimation()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 4971
   const BUnit* pTarget = gWorld->getUnit(mTarget.getID());
//--
   BASSERT(pTarget);

   mFlagMissingAnim = true;

   BVector pos = calculateUngarrisonPosition();
   #ifdef SYNC_Unit
      syncUnitData("BUnitActionUngarrison::playUncoverAnimation", pos);
   #endif
   pUnit->setPosition(pos);
   pUnit->setFlagUseMaxHeight(false);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionUngarrison::updateNoAnimTimer(float elapsedTime)
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
      setState(cStateDone);
      mNoAnimTimer = 0.0f;
   }
}

//==============================================================================
//==============================================================================
int BUnitActionUngarrison::findAnimOffset()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
   {
      return (0);
   }

//-- FIXING PREFIX BUG ID 4972
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
// Calculate the ungarrison position
//==============================================================================
BVector BUnitActionUngarrison::calculateUngarrisonPosition()
{
   // Use spawn point override if specified
   if (!mIgnoreSpawn && !mSpawnPoint.almostEqual(cInvalidVector))
      return (calculateUngarrisonPositionFromSquadSpawnPoint());

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   BVector result = cInvalidVector;
   BUnit* pTarget = gWorld->getUnit(mTarget.getID());
   BASSERT(pTarget);

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionUngarrison::calculateUngarrisonPosition 1", pUnit->getID());
   #endif

    if (mTarget.isPositionValid())
   {
      BVector ofs(mTarget.getPosition());
      //BSquad* pParent = pUnit->getParentSquad();

      // If we don't have an anim, we don't need to offset
      if (!mFlagMissingAnim)
         ofs += mOffset;

      return ofs;
   }


   if (pTarget && pUnit)
   {
      uint numInSquad = 1;
//-- FIXING PREFIX BUG ID 4974
      const BSquad* pSquad = pUnit->getParentSquad();
//--
      if (pSquad)
      {
         numInSquad = pSquad->getNumberChildren();
      }
      BVector pos = pTarget->getPosition();
      BVector dir = pTarget->getForward();
      dir.normalize();
      dir *= (pTarget->getObstructionRadius() + (pUnit->getObstructionRadius() * (float)numInSquad * 2.0f) + gTerrainSimRep.getDataTileScale());

      if (gPather.isObstructedTile(pos))
         result = pos + dir;
      else
         result = pos;

      // If we don't have an anim, we don't need to offset
      if (!mFlagMissingAnim)
         result += mOffset;

      #ifdef SYNC_Unit
         syncUnitData("BUnitActionUngarrison::calculateUngarrisonPosition 2", pUnit->getID());
      #endif

      // Make sure we've found a pathable spot (there has to be one, since we had to get in from somewhere...)
      BVector suggestedPos(0.0f);
      static BDynamicSimVectorArray instantiatePositions;
      instantiatePositions.setNumber(0);
      long closestDesiredPositionIndex;

      bool validExit = BSimHelper::findInstantiatePositions(1, instantiatePositions, pTarget->getObstructionRadius(), pTarget->getPosition(),
                           pTarget->getForward(), pTarget->getRight(), pSquad->getObstructionRadius(), pTarget->getPosition(), closestDesiredPositionIndex, 4);

      if (validExit && instantiatePositions.size() > 0)
         result = instantiatePositions[0];

      if (pUnit->getFlagFlying() && pUnit->getPhysicsObject())
      {
         BVector temp;
         pUnit->getPhysicsObject()->getPosition(temp);
         result.y = temp.y;
      }
      else
      {
         gTerrainSimRep.getHeight(result, true);
      }
   }

      #ifdef SYNC_Unit
         syncUnitData("BUnitActionUngarrison::calculateUngarrisonPosition 3", pUnit->getID());
      #endif

   //gpDebugPrimitives->addDebugSphere(result, 10.0f, cDWORDBlue, BDebugPrimitives::cCategoryNone, 10.0f);
   return (result);
}

//==============================================================================
//==============================================================================
BVector BUnitActionUngarrison::calculateUngarrisonPositionFromSquadSpawnPoint()
{
   // The spawn point is actually the squad's spawn point (since all units got the same opp and thus
   // the same spawn point).  So get the squad's offset for the unit and calculate the unit spawn point.
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionUngarrison::calculateUngarrisonPositionFromSquadSpawnPoint", pUnit->getID());
   #endif

   BVector result = pSquad->getFormationPosition(pUnit->getID());
   BVector offset = mSpawnPoint - pSquad->getPosition();
   result += offset;

   if (pUnit->getFlagFlying() && pUnit->getPhysicsObject())
   {
      BVector temp;
      pUnit->getPhysicsObject()->getPosition(temp);
      result.y = temp.y;
   }
   else
   {
      gTerrainSimRep.getHeight(result, true);
   }

   return result;
}


//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVECTOR(pStream, mSpawnPoint);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEVAR(pStream, float, mNoAnimTimer);
   GFWRITEVAR(pStream, uint8, mExitDirection);
   GFWRITEBITBOOL(pStream, mIgnoreSpawn);

   GFWRITECLASS(pStream, saveType, mSource);
   GFWRITEVAR(pStream, BEntityID, mSourceID);

   GFWRITEVECTOR(pStream, mOffset);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionUngarrison::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVECTOR(pStream, mSpawnPoint);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADVAR(pStream, float, mNoAnimTimer);
   GFREADVAR(pStream, uint8, mExitDirection);
   GFREADBITBOOL(pStream, mIgnoreSpawn);

   if (BAction::mGameFileVersion >= 26)
   {
      GFREADCLASS(pStream, saveType, mSource);
      GFREADVAR(pStream, BEntityID, mSourceID);
   }

   if (BAction::mGameFileVersion >= 35)
   {
      GFREADVECTOR(pStream, mOffset);
   }

   return true;
}
