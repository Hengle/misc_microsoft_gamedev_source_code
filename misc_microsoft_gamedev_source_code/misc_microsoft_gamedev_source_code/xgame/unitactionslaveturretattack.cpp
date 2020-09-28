//==============================================================================
// unitactionslaveturretattack.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionslaveturretattack.h"
#include "world.h"
#include "tactic.h"
#include "unitquery.h"
#include "protoobject.h"
#include "visual.h"
#include "squadactioncarpetbomb.h"
#include "unitactionavoidcollisionair.h"
#include "actionmanager.h"
#include "unitactionrangedattack.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionSlaveTurretAttack, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   // If crashing, don't connect
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pUnit->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
   if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
   {
      BAction::disconnect();
      return (false);
   }

   // Need a hardpoint
   if (!grabControllers())
      return false;

   mIsAttacking = false;

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::disconnect(void)
{
   if (mIsAttacking)
      stopAttacking();

   releaseControllers();

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::init()
{
   if (!BAction::init())
      return false;

   mTargetingLead.zero();
   mIsAttacking = false;
   mpParentAction = NULL;
   mLastLOSValidationTime=(DWORD)0;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::setState(BActionState state)
{
   #ifdef SYNC_UnitAction
      long ownerID = cInvalidObjectID;
      if (mpOwner)
         ownerID = mpOwner->getID().asLong();
      syncUnitActionData("BUnitActionSlaveTurretAttack::setState owner ID", ownerID);
      syncUnitActionData("BUnitActionSlaveTurretAttack::setState state", state);
   #endif

   switch (state)
   {
   case cStateBlocked:
   case cStateNone:
   case cStateDone:
      {
         if (mIsAttacking)
            stopAttacking();

         mIsAttacking = false;
         break;
      }

   case cStateWorking:
      {
         startAttacking();
         break;
      }
   }

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::update(float elapsed)
{
   syncUnitActionData("BUnitActionSlaveTurretAttack::update mState", mState);

   switch (mState)
   {
   case cStateNone:
   case cStateWorking:
      trackTarget(elapsed);
      break;

   case cStateBlocked:
      if (!isBlocked())
         setState(cStateNone);
      break;

   case cStateDone:
      return (false);
   }

   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::isBlocked() const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5177
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   if (!pSquad)
      return false;

   return (pUnit->getFlagAttackBlocked() || pSquad->getFlagAttackBlocked());
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   switch (eventType)
   {
   case BEntity::cEventAnimAttackTag:
      {
         // Validate target
         if (!validateTarget())
         {
            setState(cStateNone);
            break;
         }

         doProjectileAttack(data, data2);

         //Poke our attack rate in as the unit's attack wait timer.
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
         BVisual* pVisual = pUnit->getVisual();
         if (pHP && pVisual)
         {
//-- FIXING PREFIX BUG ID 5178
            const BVisualItem* pAttachment = pVisual->getAttachment(pHP->getPitchAttachmentHandle());
//--
            if (pAttachment)
            {
               float animLen=pAttachment->getAnimationDuration(cActionAnimationTrack);
               pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), 0.0f);
               if (mpProtoAction->isAttackWaitTimerEnabled())
               {
                  if (animLen != 0.0f)
                     pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), animLen);                 
               }
            }
         }
         break;
      }

   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         if ((data2 == cActionAnimationTrack) && (mState == cStateWorking))
         {
            if (mpProtoAction->getDontLoopAttackAnim() && (senderID == pUnit->getID()))
            {
               setState(cStateNone);
               break;
            }

            if (eventType == BEntity::cEventAnimEnd)
            {
               setState(cStateNone);
               break;
            }
         }
         break;
      }
   case BEntity::cEventRecomputeVisualStarting:
      {
         // unlock the attachment, in case the recompute needs to replace it
         setAttachmentAnimationLock(false, false);
         break;
      }
   case BEntity::cEventRecomputeVisualCompleted:
      {
         // if we were attacking, we need to make sure we're in a valid state here for the visuals
         if (mpProtoAction && mIsAttacking)
         {
            // check to see if the attachments have the animations set
            bool attachmentAnimsSet = checkAttachmentAnimationSet(mpProtoAction->getAnimType());
            if (!attachmentAnimsSet)
            {
               // not set, restart the attack to make sure it's all valid
               // setting the state to working will call start attack
               mIsAttacking = false; 
               setState(cStateWorking);
            }
         }
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::trackTarget(float elapsed)
{
   // If we're blocked, stop tracking
   if (isBlocked())
   {
      setState(cStateBlocked);
      return;
   }

   // Make sure we still have our controllers
   if (!validateControllers())
   {
      setState(cStateNone);
      return;
   }

   // Validate target
   if (!validateTarget())
   {
      setState(cStateNone);
      return;
   }

   // Validate range
   const BSimTarget* tempSimTarget = getTarget();
   if(tempSimTarget)
   {
      if (!validateUnitAttackRange(*tempSimTarget, mLastLOSValidationTime))
      {
         setState(cStateNone);
         return;
      }
   }

   // Update targeting lead
   updateTargetingLead();

   // Rotate turrets
   if (!updateTurretRotation(elapsed))
   {
      setState(cStateNone);
      return;
   }

   // Can we hit the target?
   if (!canHitTarget())
   {
      setState(cStateNone);
      return;
   }

   // Start attacking
   //If we're still waiting to attack, then keep waiting.
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   bool bWaitToAttack = pUnit->attackWaitTimerOn(mpProtoAction->getWeaponID());

   if (!mIsAttacking && !bWaitToAttack)
      setState(cStateWorking);
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::validateControllers() const
{
   //If we have to have a hardpoint, check that.
//-- FIXING PREFIX BUG ID 5179
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   if (mpProtoAction->getHardpointID() >= 0)
   {
      long hardpointController=pUnit->getHardpointController(mpProtoAction->getHardpointID());
      if (hardpointController != (long)mID)
         return (false);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //If we have to have a hardpoint, grab that.
   if (mpProtoAction->getHardpointID() != -1)
   {
      if (!pUnit->grabHardpoint(mID, mpProtoAction->getHardpointID(), BUnitOpp::cInvalidID))
         return (false);
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   //If we have a hardpoint, release it.
   if (mpProtoAction->getHardpointID() != -1)
      pUnit->releaseHardpoint(mID, mpProtoAction->getHardpointID());
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::getTurretPositionForward(BVector &position, BVector &forward) const
{
//-- FIXING PREFIX BUG ID 5180
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);

   long hardpointID = mpProtoAction->getHardpointID();

   // Get hardpoint orientation
   BMatrix matrix;
   if (!pUnit->getHardpointYawTransform(hardpointID, matrix))
   {
      //position = pUnit->getVisualCenter();
      // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
      position = pUnit->getSimCenter();
      forward = pUnit->getForward();
      return;
   }

   forward = matrix.getRow(2);
   forward.normalize();
   matrix.getTranslation(position);

   // Transform to world space
   BMatrix worldMatrix;
   pUnit->getWorldMatrix(worldMatrix);
   forward = XMVector3TransformNormal(forward, worldMatrix);
   position = XMVector3Transform(position, worldMatrix);
}

//==============================================================================
//==============================================================================
/*bool BUnitActionSlaveTurretAttack::validateRange() const
{
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   const BSimTarget* pSimTarget = getTarget();
   if (!pSimTarget)
      return false;
   const BUnit* pTarget = gWorld->getUnit(pSimTarget->getID());
   if (!pTarget)
      return false;

   float maxRange = mpProtoAction->getMaxRange(pUnit);
   float distance = pUnit->calculateXZDistance(pTarget);

   return (distance <= maxRange);
}*/

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::validateTarget() const
{
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   const BSimTarget* pSimTarget = getTarget();
   if(pSimTarget == NULL)
      return false;

   BEntityID targetID = pSimTarget->getID();

   if (!targetID.isValid())
      return false;

   BUnit* pTarget = gWorld->getUnit(targetID);
   return (pTarget && pTarget->isAlive() && !pTarget->getFlagDestroy() && pTarget->getParentSquad() && (!pTarget->isGarrisoned() || pTarget->isInCover()) && 
      !pTarget->getFlagNotAttackable() && (mpProtoAction->getTargetPriority(pTarget->getProtoObject()) > 0.0f) && !pUnit->isObjectAttached(pTarget));
}

//==============================================================================
//==============================================================================
BVector BUnitActionSlaveTurretAttack::applyGravityOffset(const BUnit* pTarget, BVector targetPosition) const
{
   // SLB: Pitch up to match the ballistic arc
   if (mpProtoAction->isWeaponAffectedByGravity(mpOwner->getPlayer()))
   {
      const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
      BDEBUG_ASSERT(pUnit);

      BVector hardpointPosition;
      BMatrix hardpointMatrix;
 
      // Get hardpoint matrix in local space
      if (pUnit->getHardpointPitchLocation(mpProtoAction->getHardpointID(), hardpointPosition, hardpointMatrix))
      {
         // Transform hardpoint matrix to world space
         BMatrix unitWorldMatrix;
         pUnit->getWorldMatrix(unitWorldMatrix);
         hardpointMatrix.mult(hardpointMatrix, unitWorldMatrix);

         // Get world space hardpoint position
         hardpointMatrix.getTranslation(hardpointPosition);

         // Get projectile proto object
         const BPlayer *pPlayer = pUnit->getPlayer();
         const BProtoObject *pProtoObject = pPlayer->getProtoObject(mpProtoAction->getProjectileID());

         // Calculate projectile world space launch direction
         BVector launchDirection;
         bool useGravity;
         float gravity;
         const float maxRange = mpProtoAction->getMaxRange(pUnit);
         gWorld->calculateProjectileLaunchDirection(maxRange, pUnit, pTarget, pProtoObject, hardpointPosition, targetPosition, launchDirection, useGravity, gravity);

         // Project target location onto launchDirection vector
         if (useGravity)
         {
            launchDirection.normalize();
            targetPosition = hardpointPosition + (launchDirection * 100.0f);
         }
      }
   }

   return targetPosition;
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::updateTurretRotation(float elapsed)
{
   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnitActionSlaveTurretAttack::updateTurretRotation owner ID", mpOwner->getID().asLong());
   #endif

   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
//-- FIXING PREFIX BUG ID 5182
   const BUnit* pTarget = gWorld->getUnit(getTarget()->getID());
//--
   if (!pTarget)
      return false;

   BVector targetPosition = applyGravityOffset(pTarget, pTarget->getPosition() + mTargetingLead);

   #ifdef SYNC_UnitDetail
      syncUnitDetailData("BUnitActionSlaveTurretAttack::updateTurretRotation targetPosition", targetPosition);
      syncUnitDetailData("BUnitActionSlaveTurretAttack::updateTurretRotation mTargetingLead", mTargetingLead);
   #endif

   long hardpointID = mpProtoAction->getHardpointID();

   return pUnit->pitchHardpointToWorldPos(hardpointID, targetPosition, elapsed);
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::canHitTarget()
{
   //-- Get our parent attack action and return whether it can attack or not
   if(!mpParentAction || mpParentAction->getType() != cActionTypeUnitRangedAttack)
   {
      BASSERT(0);
      return false;
   }

   BUnitActionRangedAttack* pAction = static_cast<BUnitActionRangedAttack*>(mpParentAction);
   if(pAction)
      return pAction->canHitTarget();
   else
   {
      BASSERT(0);
      return false;
   }
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::startAttacking()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   BVisual* pVisual = pUnit->getVisual();

   if (pHP && pVisual)
   {
      BVisualItem* pAttachment = pVisual->getAttachment(pHP->getPitchAttachmentHandle());
      if (pAttachment)
      {
         mIsAttacking = true;

         pAttachment->setAnimationLock(cActionAnimationTrack, false);
         pAttachment->setAnimationLock(cMovementAnimationTrack, false);

         BMatrix worldMatrix; pUnit->getWorldMatrix(worldMatrix);
         DWORD playerColor = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);
         pVisual->setAnimation(cActionAnimationTrack, mpProtoAction->getAnimType(), false, playerColor, worldMatrix, 0.0f, -1, false, pAttachment);
         pVisual->copyAnimationTrack(cActionAnimationTrack, cMovementAnimationTrack, false, playerColor, worldMatrix, pAttachment);

         pAttachment->setAnimationLock(cActionAnimationTrack, true);
         pAttachment->setAnimationLock(cMovementAnimationTrack, true);

         pAttachment->validateAnimationTracks();
         pAttachment->updateVisibility(pUnit->getVisualIsVisible());

         //Poke our attack rate in as the unit's attack wait timer.   
         float animLen=pAttachment->getAnimationDuration(cActionAnimationTrack);
         pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), 0.0f);
         if (mpProtoAction->isAttackWaitTimerEnabled())
         {
            if (animLen != 0.0f)
               pUnit->setAttackWaitTimer(mpProtoAction->getWeaponID(), animLen);            
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::stopAttacking()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());

   if (pHP)
   {
      pUnit->resetAttachmentAnim(pHP->getPitchAttachmentHandle());
   }

   mIsAttacking = false;
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::doProjectileAttack(long attachmentHandle, long boneHandle)
{
   BDEBUG_ASSERT(boneHandle != -1);
   BDEBUG_ASSERT(attachmentHandle != -1);
//-- FIXING PREFIX BUG ID 5188
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BDEBUG_ASSERT(pUnit);
   BUnit* pTarget = gWorld->getUnit(getTarget()->getID());
   if (!pTarget)
      return;

   //Get the range.
   float maxRange = mpProtoAction->getMaxRange(pUnit);

   //Projectile launch parms.
   BObjectCreateParms parms;
   parms.mPlayerID = pUnit->getPlayerID();
   parms.mProtoObjectID = mpProtoAction->getProjectileID();
   if (parms.mProtoObjectID == -1)
      return;

   //Calculate the point to launch the projectile from.
   getLaunchPosition(pUnit, attachmentHandle, boneHandle, parms.mPosition, parms.mForward, parms.mRight);

   //Does this protoaction and this target mean we need to shoot at the foot of the target?   
   bool targetGround = false;
   bool collideWithUnits = true;
   if (mpProtoAction->getTargetsFootOfUnit() && pTarget->getProtoObject()->getFlagTargetsFootOfUnit())
   {
      targetGround = true;
      collideWithUnits = false;
   }

   //Figure out the target position and offset.
   BVector targetPos;
   BVector targetOffset;
   getTargetPosition(pTarget, targetPos, targetOffset, targetGround);

   //Calculate the damage amount.         
   float damage = getDamageAmount(pUnit, pTarget);

   syncProjectileData("BUnitActionSlaveTurretAttack::doProjectileAttack Unit ID", pUnit->getID().asLong());
   if (pTarget)
      syncProjectileData("BUnitActionSlaveTurretAttack::doProjectileAttack Target ID", pTarget->getID().asLong());

   syncProjectileData("BUnitActionSlaveTurretAttack::doProjectileAttack launchPosition", parms.mPosition);

   //Projectile orientation.
   BVector turretPosition;
   BVector projectileOrientation;
   getTurretPositionForward(turretPosition, projectileOrientation);

   //Take unit accuracy scalar into account.
   bool useMovingAccuracy = (pUnit->isMoving() && (pUnit->getVelocity().length() >= (pUnit->getDesiredVelocity() * 0.9f))) ? true : false;
   float accuracy = mpProtoAction->getAccuracy() * pUnit->getAccuracyScalar();
   float movingAccuracy = mpProtoAction->getMovingAccuracy() * pUnit->getAccuracyScalar();
   float maxDeviation = mpProtoAction->getMaxDeviation();
   float movingMaxDeviation = mpProtoAction->getMovingMaxDeviation();
   if (pUnit->getAccuracyScalar())
   {
      float recAccuracyScalar = 1.0f / pUnit->getAccuracyScalar();
      maxDeviation *= recAccuracyScalar;
      movingMaxDeviation *= recAccuracyScalar;
   }

   //Actually launch the projectile.
   targetOffset += gWorld->getProjectileDeviation(parms.mPosition, pTarget ? ( pTarget->getPosition() + targetOffset ) : targetOffset,
      mTargetingLead, useMovingAccuracy ? movingAccuracy : accuracy, maxRange,
      useMovingAccuracy ? movingMaxDeviation : maxDeviation,
      mpProtoAction->getAccuracyDistanceFactor(), mpProtoAction->getAccuracyDeviationFactor());
   gWorld->launchProjectile(parms, targetOffset, mTargetingLead, projectileOrientation, damage,
      mpProtoAction, (IDamageInfo*)mpProtoAction, mpOwner->getID(), pTarget, -1, false, collideWithUnits );
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::getLaunchPosition(const BUnit* pUnit, long attachmentHandle, long boneHandle, BVector & position, BVector & forward, BVector & right) const
{
   #ifdef SYNC_Projectile
      syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition owner ID", mpOwner->getID().asLong());
   #endif

   BVisual* pVisual = pUnit->getVisual();
   if (pVisual)
   {
      BMatrix attachmentMat;
//-- FIXING PREFIX BUG ID 5189
      const BVisualItem* pVisualItem = pVisual->getAttachment(attachmentHandle, &attachmentMat);
//--
      if (pVisualItem)
      {
         BMatrix unitMat;
         pUnit->getWorldMatrix(unitMat);

         BMatrix mat;
         mat.mult(attachmentMat, unitMat);

         #ifdef SYNC_Projectile
            syncProjectileCode("BUnitActionSlaveTurretAttack::getLaunchPosition hasVisual");
         #endif

         if (pVisualItem->getBone(boneHandle, &position, NULL, NULL, &mat))
         {
            mat.getForward(forward);
            mat.getRight(right);
            #ifdef SYNC_Projectile
               syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition position", position);
               syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition forward", forward);
               syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition right", right);
            #endif
            return;
         }
      }
   }

   //*position = pUnit->getVisualCenter();
   // SLB: We shouldn't use visual data, so I changed this to use the sim center instead.
   position = pUnit->getSimCenter();
   forward = pUnit->getForward();
   right = pUnit->getRight();

   #ifdef SYNC_Projectile
      syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition position", position);
      syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition forward", forward);
      syncProjectileData("BUnitActionSlaveTurretAttack::getLaunchPosition right", right);
   #endif
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::getTargetPosition(const BUnit* pTarget, BVector& targetPosition, BVector& targetOffset, bool targetGround) const
{
   if (targetGround)
   {
      BVector tempPos = pTarget->getPosition();
      float retHeight;
      gTerrainSimRep.getHeightRaycast(tempPos, retHeight, true);
      targetPosition = BVector(tempPos.x, retHeight, tempPos.z);
      targetOffset = cOriginVector;

      return;
   }

   //Target Position.
   targetPosition = pTarget->getPosition();
   targetOffset = BVector(0.0f, pTarget->getProtoObject()->getObstructionRadiusY(), 0.0f);
}

//==============================================================================
//==============================================================================
float BUnitActionSlaveTurretAttack::getDamageAmount(const BUnit* pUnit, const BUnit* pTarget) const
{
   //Get the damage modifier.
   float damageModifier = pUnit->getDamageModifier();
   if (damageModifier <= 0.0f)
      return 0.0f;

   //Get the base damage amount.
   float damageAmount = mpProtoAction->getDamagePerAttack();

   //Add in our modifier.
   damageAmount *= damageModifier;

   //Add Height Bonus Damage.
   if (mpProtoAction->usesHeightBonusDamage() && pTarget)
   {
      float heightDiff = pUnit->getPosition().y - pTarget->getPosition().y;
      if (heightDiff > 0)
         damageAmount += damageAmount * heightDiff * gDatabase.getHeightBonusDamage();
   }

   return damageAmount;
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::validateTag(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2) const
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   if (senderID != pUnit->getID())
      return false;

   switch (eventType)
   {
   case BEntity::cEventAnimAttackTag:
      {
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
         return (pHP && (pHP->getPitchAttachmentHandle() != -1) && (pHP->getPitchAttachmentHandle() == (long)data));
      }

   case BEntity::cEventAnimChain:
   case BEntity::cEventAnimLoop:
   case BEntity::cEventAnimEnd:
      {
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
         BVisual* pVisual = pUnit->getVisual();

         if (pHP && (pHP->getPitchAttachmentHandle() != -1) && pVisual)
         {
//-- FIXING PREFIX BUG ID 5190
            const BVisualItem* pVisualItem = pVisual->getAttachment(pHP->getPitchAttachmentHandle());
//--
            return (pVisualItem && (pVisualItem->getAnimationType(data2) == (long)data));
         }

         return false;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionSlaveTurretAttack::updateTargetingLead()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);

   BPlayer* pPlayer = pUnit->getPlayer();
//-- FIXING PREFIX BUG ID 5183
   const BProtoObject* pProto = pPlayer ? pPlayer->getProtoObject(mpProtoAction->getProjectileID()) : NULL;
//--
   float projectileSpeed = 0.0f;
   if (pProto)
      projectileSpeed = pProto->getDesiredVelocity();

   BUnit* pTarget = gWorld->getUnit(getTarget()->getID());
   if (!pTarget || !pTarget->isMoving())
   {
      mTargetingLead.zero();
      return;
   }
   BVector targetVelocity = pTarget->getVelocity();
   float targetSpeed = targetVelocity.length();
   if ((projectileSpeed < cFloatCompareEpsilon) || (targetSpeed < cFloatCompareEpsilon))
   {
      mTargetingLead.zero();
      return;
   }

   BVector attackerPos = pUnit->getPosition();
   BVector targetPos = pTarget->getPosition();
   float d = attackerPos.distance(targetPos);
   if (d < cFloatCompareEpsilon)
   {
      mTargetingLead.zero();
      return;
   }

   float t = d / projectileSpeed;
   targetVelocity *= Math::Min(targetSpeed, mpProtoAction->getMaxVelocityLead())  / targetSpeed;
   BVector newTargetPos = targetPos + targetVelocity * t;
   mTargetingLead = newTargetPos - targetPos;
   float d2 = attackerPos.distance(newTargetPos);
   mTargetingLead *= d2 / d;
}


//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVECTOR(pStream, mTargetingLead);
   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEBITBOOL(pStream, mIsAttacking);
   GFWRITEVAR(pStream, DWORD, mLastLOSValidationTime);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionSlaveTurretAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVECTOR(pStream, mTargetingLead);
   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADBITBOOL(pStream, mIsAttacking);
   if (BAction::mGameFileVersion >= 42)
      GFREADVAR(pStream, DWORD, mLastLOSValidationTime);
   return true;
}
