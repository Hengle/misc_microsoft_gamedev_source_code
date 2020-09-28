//==============================================================================
// unitactionrage.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unit.h"
#include "unitactionrage.h"
#include "game.h"
#include "world.h"
#include "simhelper.h"
#include "squadactionattack.h"
#include "tactic.h"
#include "fatalitymanager.h"
#include "visual.h"
#include "squadactionmove.h"
#include "powerhelper.h"
#include "pather.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionRage, 5, &gSimHeap);

const static float cInitialImpulseScalar = 1.75f;
const static float cHitImpulseScalar = 0.4f;
const static float cImpulseRadiusReciprocalMultiplier = 10.0f;

//==============================================================================
//==============================================================================
bool BUnitActionRage::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   if (!pUnit)
      return false;

   mMoveTarget = pUnit->getPosition();

   // find our proto action
   BTactic* pTactic = pUnit->getTactic();
   if (!pTactic)
      return false;

   long numProtoActions = pTactic->getNumberProtoActions();
   for (long i = 0; i < numProtoActions; ++i)
   {
      BProtoAction* pProtoAction = pTactic->getProtoAction(i);
      if (pProtoAction && pProtoAction->getActionType() == BAction::cActionTypeUnitRage)
      {
         setProtoAction(pProtoAction);
         break;
      }
   }
   if (!mpProtoAction)
      return false;

   setFlagConflictsWithIdle(true);
   pUnit->removeAllActionsOfType(BAction::cActionTypeEntityIdle);

   // wait to grab controllers after we change mode
   setState(cRageStateWaiting);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionRage::disconnect()
{
   //Release our controllers.
   releaseControllers();

   // set all our movement data
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (pUnit)
   {
      pUnit->setFlagMoving(false);

      BSquad* pOwnerSquad = pUnit->getParentSquad();
      if (pOwnerSquad)
      {
         pOwnerSquad->setLeashPosition(pUnit->getPosition());
         pOwnerSquad->setPosition(pUnit->getPosition());

         BPlatoon* pOwnerPlatoon = pOwnerSquad->getParentPlatoon();
         if (pOwnerPlatoon)
            pOwnerPlatoon->setPosition(pUnit->getPosition());
      }

      pUnit->setFlagTiesToGround(true);
      if (pUnit->isAnimationLocked())
         pUnit->unlockAnimation();

      // boarded attacking determines this on its own
      // or the none state means we're dying from getting thrown - hacky! :( 
      if (mState != cRageStateBoardedAttacking && mState != cRageStateNone)
         pUnit->tieToGround();
   }

   // clean up our current state
   setState(cRageStateNone);

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::init()
{
   if(!BAction::init())
      return false;

   mFlagPersistent = true;
   mUsePather = false;

   mMoveTarget = cOriginVector;
   mFutureState = cRageStateNone;
   mTargettedSquad = cInvalidObjectID;
   mpBoardedAttackProtoAction = NULL;
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::setState(BActionState state)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return false;

   BSquad* pSquad = pUnit->getParentSquad();
   if (!pSquad)
      return false;

   // old state
   ERageState oldState = (ERageState)mState;
   switch(oldState)   
   {
   case cRageStateBoardedAttacking:
      {
         // if our new state isn't jumping, then we should place on the ground, else don't check here
         detachFromBoarding(state != cRageStateJumping);
         mTargettedSquad = cInvalidObjectID;
         break;
      }
   case cRageStateAttacking:
      {
         mTargettedSquad = cInvalidObjectID;
         break;
      }
   }

   // new state
   switch(state)
   {
   case cRageStateActive:
      {
         stopMoving();
         break;
      }
   case cRageStateMoving:
      {
         // we need to make sure to turn this back on!
         pUnit->setFlagTiesToGround(true);

         long numTargetHardpoints = pUnit->getNumberHardpoints();
         for (long i = 0; i < numTargetHardpoints; i++)
            pUnit->clearHardpoint(i);
         pUnit->removeOpps();

         break;
      }
   case cRageStateAttacking:
      {
         // we need to make sure to turn this back on!
         pUnit->setFlagTiesToGround(true);

         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   //If we have a future state, go.
   if (mFutureState != cRageStateNone)
   {
      setState((BActionState)mFutureState);
      if ((mFutureState == cRageStateDone) || (mFutureState == cRageStateFailed))
         return (true);
      mFutureState = cRageStateNone;
   }

   switch (mState)
   {
   case cRageStateWaiting:
      {
         if (!pSquad->getFlagChangingMode() && grabControllers())
            setState(cRageStateActive);
         break;
      }
   case cRageStateAttacking:
   case cRageStateBoardedAttacking:
      {
         BSquad* pTargetSquad = gWorld->getSquad(mTargettedSquad);
         if (!pTargetSquad || !pTargetSquad->isAlive())
            setState(cRageStateActive);

         if (shouldStartMoving())
         {
            setState(cRageStateMoving);
            updateMoving(elapsed);
         }

         break;
      }

   case cRageStateActive:
      {
         if (shouldStartMoving())
         {
            setState(cRageStateMoving);
            updateMoving(elapsed);
         }
         break;
      }
   case cRageStateMoving:
      {
         updateMoving(elapsed);
         break;
      }
   case cRageStateJumping:
      {
         break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionRage::startJump(const BVector& targetLocation, BEntityID targetSquadId)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   stopMoving();

   // disable this while jumping or boarded 
   pUnit->setFlagTiesToGround(false);
   if (pUnit->isAnimationLocked())
      pUnit->unlockAnimation();

   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, cAnimTypeRageLeap, true, true, -1, true);
   pUnit->computeAnimation();
   pUnit->lockAnimation(10000000, false);
   BASSERT(pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateMisc, cAnimTypeRageLeap));

   setState(cRageStateJumping);
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::canBoard(BSquad& squad) const
{
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   const BUnit* pTargetUnit = squad.getLeaderUnit();
   if (!pUnit || !pTargetUnit || !mpProtoAction)
      return false;

   uint assetIndex = 0;
   BFatality* pFatality = mpProtoAction->getFatality(pUnit, pTargetUnit, assetIndex);
   if (!pFatality)
      return false;

   BFatalityAsset* pFatalityAsset = gFatalityManager.getFatalityAsset(pFatality->mFatalityAssetArray.get(assetIndex));
   if (!pFatalityAsset)
      return false;

   long animIndex = pFatality->mAttackerAnimType;
   return pUnit->hasAnimation(animIndex);
}

//==============================================================================
//==============================================================================
void BUnitActionRage::teleportToAndAttack(const BVector& targetLocation, BEntityID targetSquadId)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   BSquad* pTargetSquad = gWorld->getSquad(targetSquadId);
   BUnit* pTargetUnit = (pTargetSquad) ? pTargetSquad->getLeaderUnit() : NULL;

   mTargettedSquad = targetSquadId;

   if (pUnit->isAnimationLocked())
   {
      pUnit->setFlagTiesToGround(false);
      pUnit->unlockAnimation();
   }

   // see if we have a fatality entry for this
   uint assetIndex = 0;
   if (mpProtoAction && pTargetUnit)
   {
      BFatality* pFatality = mpProtoAction->getFatality(pUnit, pTargetUnit, assetIndex);
      if (pFatality)
      {
         BFatalityAsset* pFatalityAsset = gFatalityManager.getFatalityAsset(pFatality->mFatalityAssetArray.get(assetIndex));
         if (pFatalityAsset)
         {
            // we found a fatality anim! hooray!
            long animIndex = pFatality->mAttackerAnimType;
            if (pUnit->hasAnimation(animIndex))
            {
               // if the target unit has a physics object, give it an impulse
               if (pTargetUnit->isPhysicsAircraft())
               {
                  // yes, we want the reciprocal, because our awesome systems has the same physics for all air units
                  // but larger units should have less angular impulse than smaller units. whee. 
                  BVector impulseLocation = (pUnit->getPosition() - pTargetUnit->getPosition());
                  impulseLocation.normalize();
                  impulseLocation *= (cImpulseRadiusReciprocalMultiplier / pTargetUnit->getObstructionRadius());
                  BPowerHelper::impulsePhysicsAirUnit(*pTargetUnit, cInitialImpulseScalar, &impulseLocation);
               }

               long targetToBoneHandle = -1;
               long boardPointHandle = pTargetUnit->getVisual()->getPointHandle(cActionAnimationTrack, cVisualPointBoard);
               const BProtoVisualPoint* pPoint = pTargetUnit->getVisual()->getPointProto(cActionAnimationTrack, boardPointHandle);
               if (pPoint)
               {
                  targetToBoneHandle = pPoint->mBoneHandle;

                  // update the rotation of the attacker to the target bone
                  if (targetToBoneHandle != -1)
                  {
                     BVector tempPos;
                     BMatrix targetMatrix, tempMatrix;
                     pTargetUnit->getWorldMatrix(targetMatrix);
                     if (pTargetUnit->getVisual()->getBone(targetToBoneHandle, &tempPos, &tempMatrix, 0, &targetMatrix))
                     {
                        if (pFatality->mToBoneRelative)
                           targetMatrix = tempMatrix;

                        BQuaternion attackerOrientation;
                        attackerOrientation = pFatalityAsset->mTargetOrientationOffset.inverse() * BQuaternion(targetMatrix);
                        BMatrix attackerOrientationMatrix;
                        attackerOrientation.toMatrix(attackerOrientationMatrix);

                        // Calculate attacker offset between desired and current
                        BVector targetPos;
                        targetMatrix.getTranslation(targetPos);
                        BVector desiredAttackerPosition;
                        attackerOrientationMatrix.transformVectorAsPoint(-pFatalityAsset->mTargetPositionOffset, desiredAttackerPosition);
                        desiredAttackerPosition += targetPos;
                        BVector desiredAttackerPositionOffset = desiredAttackerPosition - pUnit->getPosition();

                        // Make the position transition vector relative to the target so that it can be transformed during the update
                        targetMatrix.invert();
                        targetMatrix.transformVector(desiredAttackerPositionOffset, desiredAttackerPositionOffset);

                        pUnit->setPosition(desiredAttackerPosition);
                        pUnit->setRotation(attackerOrientationMatrix);
                     }                 
                  }
               }

               pTargetUnit->attachObject(pUnit->getID(), targetToBoneHandle, -1, true);
               pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateMisc, animIndex, true, true, -1, true);
               pUnit->computeAnimation();
               pUnit->lockAnimation(10000000, false);
               bool animSet = pUnit->isAnimationSet(BObjectAnimationState::cAnimationStateMisc, animIndex);
               BASSERT(animSet);

               // find a valid protoaction, if the anim wasn't set - don't set a proto action
               // so we don't try and deal damage from an invalid animation
               if (animSet)
               {
                  BTactic* pTactic = pUnit->getTactic();
                  if (pTactic)
                  {
                     long numProtoActions = pTactic->getNumberProtoActions();
                     for (long i = 0; i < numProtoActions; ++i)
                     {
                        BProtoAction* pProtoAction = pTactic->getProtoAction(i);
                        if (pProtoAction && pProtoAction->getAnimType() == animIndex)
                        {
                           mpBoardedAttackProtoAction = pProtoAction;
                           break;
                        }
                     }
                  }
               }
               BASSERTM(mpBoardedAttackProtoAction, "Couldn't find a protoaction with a matching animation to deal damage for boarding this target!");

               setState(cRageStateBoardedAttacking);
               return;
            }
         }
      }
   }

   // generic slam to location - this removes the blocking anim order
   pSquad->doTeleport(targetLocation, 1, false);

   // if we're in a bad spot (like if we tried to target an air unit in an unpathable land area that died before we got there
   // find an exit location
   if (!BSimHelper::findPosForAirborneSquad(pSquad, mpProtoAction->getWorkRange(), false, false, false))
   {
      // hack :( to not get in here when we kill the unit
      mState = cRageStateNone;
      pUnit->setFlagTiesToGround(false);
      pUnit->createAndThrowPhysicsReplacement((IDamageInfo*)mpProtoAction, pUnit->getPosition(), 1.0f, false, 1.0f, pUnit->getID());
   }

   BSquadActionAttack *pAttackAction = reinterpret_cast<BSquadActionAttack *>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
   if (pAttackAction)
      pAttackAction->setTarget(BSimTarget(targetSquadId));
   else
      pSquad->queueAttack(BSimTarget(targetSquadId), true);
   setState(cRageStateAttacking);
}

//==============================================================================
//==============================================================================
void BUnitActionRage::detachFromBoarding(bool placeOnGround)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad(); 
   BASSERT(pSquad);

   BObject* pAttachTo = NULL;

   // Detach unit if attached
   if (pUnit && pUnit->isAttached())
   {
      pAttachTo = pUnit->getAttachedToObject();
      if (pAttachTo)
      {
         BUnit* pAttachToUnit = pAttachTo->getUnit();
         if (pAttachToUnit)
            pAttachToUnit->unattachObject(pUnit->getID());
      }
   }

   if (pUnit->isAnimationLocked())
   {
      pUnit->setFlagTiesToGround(false);
      pUnit->unlockAnimation();
      pUnit->setFlagTiesToGround(true);
   }
   pUnit->clearAnimationState();

   mpBoardedAttackProtoAction = NULL;

   // make sure the up vector is setup correctly 
   pUnit->setUp(cYAxisVector);
   pUnit->calcRight();
   pUnit->calcForward();

   // attach the teleport particle
   long boneHandle = pUnit->getVisual()->getBoneHandle("Bip01");
   pUnit->addAttachment(mTeleportAttachObject, boneHandle);

   // place the arbiter in a valid location
   if (placeOnGround)
   {
      if (!BSimHelper::findPosForAirborneSquad(pSquad, mpProtoAction->getWorkRange(), false, false, false))
      {
         // hack :( to not get in here when we kill the unit
         mState = cRageStateNone;
         pUnit->setFlagTiesToGround(false);
         pUnit->createAndThrowPhysicsReplacement((IDamageInfo*)mpProtoAction, (pAttachTo) ? pAttachTo->getPosition() : pUnit->getPosition(), 1.0f, false, 1.0f, pUnit->getID());
      }
      else
      {
         pUnit->tieToGround();
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRage::stopMoving()
{
   // set all our movement data
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (pUnit)
   {
      pUnit->setFlagMoving(false);
      pUnit->setVelocity(cOriginVector);

      BSquad* pOwnerSquad = pUnit->getParentSquad();
      if (pOwnerSquad)
      {
         pOwnerSquad->setLeashPosition(pUnit->getPosition());
         pOwnerSquad->setPosition(pUnit->getPosition());

         BPlatoon* pOwnerPlatoon = pOwnerSquad->getParentPlatoon();
         if (pOwnerPlatoon)
            pOwnerPlatoon->setPosition(pUnit->getPosition());
      }

      pOwnerSquad->removeAllOrders();
      pUnit->removeAllActionsOfType(BAction::cActionTypeUnitMove);

      mMoveTarget = pUnit->getPosition();
   }

   mMoveDirection = cOriginVector;
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::shouldStartMoving() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return false;
   BSquad* pSquad = pUnit->getParentSquad();
   if (!pSquad)
      return false;

   if (mUsePather)
   {
      BVector ownerToDestination = mMoveTarget - pSquad->getPosition();
      return (ownerToDestination.length() > pSquad->getObstructionRadius());
   }
   else
   {
      return (mMoveDirection.length() > cFloatCompareEpsilon);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionRage::updateMoving(float elapsedTime)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return;
   BSquad* pSquad = pUnit->getParentSquad();
   if (!pSquad)
      return;

   if (!pUnit->canMove())
      return;

   if (mUsePather)
   {
      BVector ownerToDestination = mMoveTarget - pSquad->getPosition();

      if (ownerToDestination.length() <= pSquad->getObstructionRadius())
      {
         setState(cRageStateActive);
         return;
      }

      // do move controls
      if (pSquad->getParentPlatoon())
         pSquad->getParentPlatoon()->setPosition(mMoveTarget);
      pSquad->setLeashPosition(mMoveTarget);
      pSquad->removeAllActionsOfType(BAction::cActionTypeSquadAttack);
      BSquadActionMove *pMoveAction = reinterpret_cast<BSquadActionMove *>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
      if (pMoveAction)
         pMoveAction->setTarget(BSimTarget(mMoveTarget));
      else
         pSquad->queueMove(BSimTarget(mMoveTarget));
   }
   else
   {
      float speedScalar = mMoveDirection.length();
      if (speedScalar < cFloatCompareEpsilon)
      {
         setState(cRageStateActive);
         return;
      }

      BVector direction = mMoveDirection;
      direction.y = 0.0f;
      direction.safeNormalize();

      speedScalar = Math::Clamp(speedScalar, 0.0f, 1.0f);

      float velocity = pUnit->getDesiredVelocity() * speedScalar;

      BVector newPosition = pUnit->getPosition() + (direction * velocity * elapsedTime);

      // super crazy aggresive - if this will move you into a bad place, bail!
      BVector intersection = pUnit->getPosition();
      if (checkCollisions(pUnit->getPosition(), newPosition, intersection))
      {
         stopMoving();
         return;
      }

      pUnit->setFlagMoving(true);

      pUnit->setForward(direction);
      pUnit->calcRight();
      pUnit->calcUp();

      BSimHelper::advanceToNewPosition(pUnit, elapsedTime, newPosition, direction);

      BSquad* pOwnerSquad = pUnit->getParentSquad();
      if (pOwnerSquad)
      {
         pOwnerSquad->setLeashPosition(pUnit->getPosition());
         pOwnerSquad->setPosition(pUnit->getPosition());

         BPlatoon* pOwnerPlatoon = pOwnerSquad->getParentPlatoon();
         if (pOwnerPlatoon)
            pOwnerPlatoon->setPosition(pUnit->getPosition());
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::checkCollisions(BVector start, BVector end, BVector &iPoint)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);

   BVector direction = end - start;
   direction.y = 0.0f;
   direction.normalize();

   // First, figure out what type of obstructions we care about.
   long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads | BObstructionManager::cIsNewTypeBlockLandUnits;

   // Set up ignore list.
   BEntityIDArray ignoreList;
   ignoreList.add(pSquad->getID());
   const BEntityIDArray &myUnitList = pSquad->getChildList();
   ignoreList.add(myUnitList.getPtr(), myUnitList.size());

   // Set up obstruction manager.
   gObsManager.begin(BObstructionManager::cBeginNone, pSquad->getPathingRadius(), quadtrees, BObstructionManager::cObsNodeTypeAll, pSquad->getPlayerID(), cDefaultRadiusSofteningFactor, &ignoreList, pSquad->canJump());

   // See if we intersect anything.
   iPoint = end;     // jce [4/17/2008] -- setting this to end is probably being overly cautious since it should always be initialized if there is an intersection
   BObstructionNodePtrArray obstructions;
   gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, start, end, true, iPoint, obstructions);

   bool retValue = false; 

   // Run through the obstructions
   for(long i=0; i<obstructions.getNumber(); i++)
   {
      // Get obstruction node.
      BOPObstructionNode *ob = obstructions[i];
      if(!ob)
         continue;

      if(ob->mType == BObstructionManager::cObsNodeTypeUnit || ob->mType == BObstructionManager::cObsNodeTypeSquad)
      {
         // if we're moving away from this obstruction in this frame, ignore it
         if (ob->mObject)
         {
            BVector dirToObject = ob->mObject->getPosition() - start; 
            dirToObject.y = 0.0f;
            dirToObject.normalize();

            if (dirToObject.angleBetweenVector(direction) > cThreePiOver4)
            {
               continue;
            }
         }
      }

      retValue = true;
      break;
   }

   // Done with obstruction manager.
   gObsManager.end();

   return retValue;
}

//==============================================================================
//==============================================================================
void BUnitActionRage::handleAttackTag(long attachmentHandle, long boneHandle)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return;
   BSquad* pSquad = pUnit->getParentSquad();
   if (!pSquad)
      return;

   if (mState != cRageStateBoardedAttacking)
      return;

   BSquad* pTargetSquad = gWorld->getSquad(mTargettedSquad);
   if (!pTargetSquad)
      return;

   BUnit* pTarget = pTargetSquad->getLeaderUnit();
   if (!pTarget)
      return;

   if (!mpBoardedAttackProtoAction)
      return;

   //Calculate the damage amount.         
   float damage=BDamageHelper::getDamageAmount(pUnit->getID(), mpBoardedAttackProtoAction->getDamagePerAttack(), pTarget->getID(), mpBoardedAttackProtoAction->usesHeightBonusDamage());

   // If a squad is attacking keep marking the squad as detected, so the undetect timer keeps getting reset.
   pSquad->notify(BEntity::cEventDetected, pSquad->getID(), NULL, NULL);

   // AOE
   float damageDealt = 0.0f;
   if (mpProtoAction->getAOERadius() > 0.0f)
   {
      BEntityIDArray unitsKilled;
      BVector targetPosition = pTarget->getPosition();
      BVector direction = targetPosition - pUnit->getPosition();

      damageDealt = BDamageHelper::doAreaEffectDamage(pUnit->getPlayerID(), pUnit->getTeamID(), pUnit->getID(), const_cast<BProtoAction*>(mpBoardedAttackProtoAction), damage, 
         targetPosition, cInvalidObjectID, direction, &unitsKilled, pTarget->getID());
   }
   else
   {
      //Deal some damage.
      BVector direction=pTarget->getPosition()-pUnit->getPosition();

      damageDealt = BDamageHelper::doDamageWithWeaponType(pUnit->getPlayerID(), pUnit->getTeamID(), pTarget->getID(), const_cast<BProtoAction*>(mpBoardedAttackProtoAction), damage,
         mpProtoAction->getWeaponType(), true, direction, 1.0f, pUnit->getPosition(), cInvalidObjectID, pUnit->getID());               

   }

   // if the target unit has a physics object, give it an impulse
   if (pTarget->isPhysicsAircraft())
   {
      // yes, we want the reciprocal, because our awesome systems has the same physics for all air units
      // but larger units should have less angular impulse than smaller units. whee. 
      BVector impulseLocation = (pUnit->getPosition() - pTarget->getPosition());
      impulseLocation.normalize();
      impulseLocation *= (cImpulseRadiusReciprocalMultiplier / pTarget->getObstructionRadius());
      BPowerHelper::impulsePhysicsAirUnit(*pTarget, cHitImpulseScalar, &impulseLocation);
   }

   //-- Play the melee sound
   pUnit->playMeleeAttackSound(pTarget);

   if (mpBoardedAttackProtoAction->getFlagDoShakeOnAttackTag())
      mpBoardedAttackProtoAction->doImpactRumbleAndCameraShake(BRumbleEvent::cTypeImpact, pUnit->getPosition(), false, pUnit->getID());
}

//==============================================================================
//==============================================================================
void BUnitActionRage::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   switch (eventType)
   {
   case BEntity::cEventSquadModeChanaged:
      {
         // check to see if we can take control
         if (grabControllers())
            setState(cRageStateActive);
         break;
      }
   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         break;
      }
   case BEntity::cEventAnimAttackTag:
      {
         handleAttackTag(data, data2);
         break;
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::validateControllers() const
{
   //Return true if we have the controllers we need, false if not.
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
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
bool BUnitActionRage::grabControllers()
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
void BUnitActionRage::releaseControllers()
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   if (!pUnit)
      return;

   //Release them.
   pUnit->releaseController(BActionController::cControllerAnimation, this);
   pUnit->releaseController(BActionController::cControllerOrient, this);   
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEPROTOACTIONPTR(pStream, mpBoardedAttackProtoAction);
   GFWRITEVAR(pStream, BEntityID, mTargettedSquad);
   GFWRITEVECTOR(pStream, mMoveTarget);
   GFWRITEVECTOR(pStream, mMoveDirection);
   GFWRITEVAR(pStream, long, mFutureState);
   GFWRITEBITBOOL(pStream, mUsePather);
   GFWRITEVAR(pStream, BProtoObjectID, mTeleportAttachObject);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionRage::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADPROTOACTIONPTR(pStream, mpBoardedAttackProtoAction);
   GFREADVAR(pStream, BEntityID, mTargettedSquad);
   GFREADVECTOR(pStream, mMoveTarget);
   GFREADVECTOR(pStream, mMoveDirection);
   GFREADVAR(pStream, long, mFutureState);
   GFREADBITBOOL(pStream, mUsePather);
   if (BAction::mGameFileVersion >= 30)
   {
      GFREADVAR(pStream, BProtoObjectID, mTeleportAttachObject);
   }

   return true;
}
