//============================================================================
// fatalitymanager.cpp
//  
// Copyright (c) 2008 Ensemble Studios
//============================================================================

#include "common.h"
#include "fatalitymanager.h"
#include "unit.h"
#include "world.h"
#include "visualitem.h"
#include "visual.h"
#include "tactic.h"
#include "gamedirectories.h"
#include "string\convertToken.h"

BFatalityManager gFatalityManager;

GFIMPLEMENTVERSION(BFatalityManager, 2);
enum
{
   cSaveMarkerFatalityManager=10000,
};

//==============================================================================
// BFatalityManager::registerFatalityAsset
//==============================================================================
long BFatalityManager::registerFatalityAsset(BVector targetPositionOffset, BQuaternion targetOrientationOffset, long weight, long attackerAnimID, long targetAnimID)
{
   BFatalityAsset fatalityAsset;
   fatalityAsset.mTargetPositionOffset = targetPositionOffset;
   fatalityAsset.mTargetOrientationOffset = targetOrientationOffset;
   fatalityAsset.mWeight = weight;
   fatalityAsset.mAttackerAnimID = (~attackerAnimID) << 16;
   fatalityAsset.mTargetAnimID = (~targetAnimID) << 16;
   fatalityAsset.mFlagCooldown = false;
   fatalityAsset.mIndex = (uint8)mFatalityAssets.size();

   long index = mFatalityAssets.add(fatalityAsset);
   BDEBUG_ASSERT(index >= 0);
   return index;
}

//==============================================================================
// BFatalityManager::startFatality
//==============================================================================
bool BFatalityManager::startFatality(BActionID callingActionID, BUnit* pAttacker, BUnit* pTarget, BFatality* pFatality, uint assetIndex, DWORD attackerIdleTime)
{
   // Get fatality / asset data
   if (!pFatality || !pAttacker || !pTarget)
      return false;
   BFatalityAsset* pFatalityAsset = getFatalityAsset(pFatality->mFatalityAssetArray.get(assetIndex));
   if (!pFatalityAsset)
      return false;

   //==============================================================================
   // MPB [11/18/2008] - Make sure we don't already have any active fatalities involving the attacker or target.
   // This is supposed to be checked upstream, but these are strange days indeed.
   const BActiveFatality* pCurrentFatality = getFatality(pAttacker->getID());
   if (pCurrentFatality)
      return false;
   pCurrentFatality = getFatality(pTarget->getID());
   if (pCurrentFatality)
      return false;

   //==============================================================================
   // Setup unit flags
   if (!pFatality->mBoarding)
   {
      pAttacker->setFlagDoingFatality(true);
      pTarget->setFlagDoingFatality(true);
      pTarget->setFlagFatalityVictim(true);
   }

   //==============================================================================
   // Calculate current offset

   // Get attacker matrix
   BMatrix attackerMatrix;
   pAttacker->getWorldMatrix(attackerMatrix);

   // Get target matrix - use the boarding bone matrix for boarding fatalities
   BMatrix targetMatrix;
   pTarget->getWorldMatrix(targetMatrix);

   long targetToBoneHandle = -1;
   if (pFatality->mBoarding)
   {
      long boardPointHandle = pTarget->getVisual()->getPointHandle(cActionAnimationTrack, cVisualPointBoard);
      const BProtoVisualPoint* pPoint = pTarget->getVisual()->getPointProto(cActionAnimationTrack, boardPointHandle);
      if (pPoint)
         targetToBoneHandle = pPoint->mBoneHandle;

      if (targetToBoneHandle != -1)
      {
         BVector tempPos;
         BMatrix tempMtx;
         pTarget->getVisual()->getBone(targetToBoneHandle, &tempPos, &tempMtx, 0, &targetMatrix);
         if (pFatality->mToBoneRelative)
            targetMatrix = tempMtx;
      }
   }

   BMatrix attackerMatrixInvert(attackerMatrix);
   attackerMatrixInvert.invert();

   BMatrix offsetMatrix;
   offsetMatrix.mult(targetMatrix, attackerMatrixInvert);

   BVector positionOffset = offsetMatrix.getRow(3);
   BQuaternion orientationOffset(offsetMatrix);

   //==============================================================================
   // MPB [4/11/2008] - Removing this obstruction for now as the attacker / target obstructions are sufficient.
   // Create an obstruction over the fatality's area
   BOPObstructionNode* pObstruction = NULL;
   /*
   BVector attackerPosition = attackerMatrix.getRow(3);
   BVector attackerForward = attackerMatrix.getRow(2);
   BBoundingBox obstructionBB;
   obstructionBB.initializeTransformed(pFatalityAsset->mMinCorner, pFatalityAsset->mMaxCorner, attackerMatrix);
   BOPObstructionNode* pObstruction = gObsManager.getNewObstructionNode();
   gObsManager.resetObstructionNode(pObstruction);
   gObsManager.fillOutRotatedPosition(pObstruction, obstructionBB.getCenter().x, obstructionBB.getCenter().z, obstructionBB.getExtents()[0], 
      obstructionBB.getExtents()[2], attackerForward.x, attackerForward.z);
   pObstruction->mType = BObstructionManager::cObsNodeTypeTerrain;
   gObsManager.installObjectObstruction(pObstruction, BObstructionManager::cObsTypeBlockAllMovement);
   */

   //==============================================================================
   // Create active fatality node
   BActiveFatality* pActiveFatality = mActiveFatalities.acquire(true);
   pActiveFatality->mPositionTransition.assignDifference(positionOffset, pFatalityAsset->mTargetPositionOffset);
   attackerMatrix.transformVector(pActiveFatality->mPositionTransition, pActiveFatality->mPositionTransition);
   pActiveFatality->mOrientationTransition = pFatalityAsset->mTargetOrientationOffset * orientationOffset.inverse();
   pActiveFatality->mPosition.assignSum(pAttacker->getPosition(), pTarget->getPosition());
   pActiveFatality->mPosition *= 0.5f;
   pActiveFatality->mpFatality = pFatality;
   pActiveFatality->mFatalityAsset = pFatalityAsset;
   pActiveFatality->mObstruction = pObstruction;
   pActiveFatality->mAttackerID = pAttacker->getID();
   pActiveFatality->mTargetID = pTarget->getID();
   pActiveFatality->mAttackerIdleTimeDWORD = attackerIdleTime;
   pActiveFatality->mTransitionTimeDWORD = pFatality->mTransitionTimeDWORD;
   pActiveFatality->mTransitionDelayDWORD = pFatality->mTransitionDelayDWORD;
   const DWORD timeDWORD = gWorld->getGametime();
   if (pFatality->mTransitionBeforeAnimating)
   {
      float transitionTime;
      DWORD transitionTimeDWORD;
      if (pActiveFatality->mTransitionTimeDWORD > 0)
      {
         transitionTimeDWORD = pActiveFatality->mTransitionTimeDWORD;
         transitionTime = transitionTimeDWORD * 0.001f;
      }
      else
      {
         transitionTime = gDatabase.getFatalityMaxTransitionTime();
         transitionTimeDWORD = ((DWORD)(transitionTime * 1000.0f));
      }

      pActiveFatality->mTransitionStartTimeDWORD = timeDWORD;
      pActiveFatality->mTransitionEndTimeDWORD = pActiveFatality->mTransitionStartTimeDWORD + transitionTimeDWORD;
      pActiveFatality->mRecTransitionDuration = 1.0f / transitionTime;
      pActiveFatality->mLastTransitionUpdateTimeDWORD = pActiveFatality->mTransitionStartTimeDWORD;
   }
   else
   {
      pActiveFatality->mTransitionStartTimeDWORD = UINT_MAX;
      pActiveFatality->mTransitionEndTimeDWORD = UINT_MAX;
      pActiveFatality->mRecTransitionDuration = UINT_MAX;
      pActiveFatality->mLastTransitionUpdateTimeDWORD = UINT_MAX;
   }
   pActiveFatality->mFatalityEndTimeDWORD = UINT_MAX; // Don't set until anims have started
   pActiveFatality->mCooldownEndTimeDWORD = UINT_MAX;
   pActiveFatality->mAttackerEndTimeDWORD = UINT_MAX;
   pActiveFatality->mFlagFatalityOver = false;
   pActiveFatality->mMoveType = pFatality->mMoveType;
   pActiveFatality->mKillTarget = pFatality->mKillTarget;
   pActiveFatality->mCallerActionID = callingActionID;
   pActiveFatality->mToBoneRelative = pFatality->mToBoneRelative;
   pActiveFatality->mAircraftTransition = pFatality->mAircraftTransition;

   // For boarding fatalities with an attacker/target end animation, the fatality end and cooldown times will be reset
   // once the attacker end anim has started (which is at the end of the start anim + idle time).
   pActiveFatality->mAttackerEndAnimType = pFatality->mAttackerEndAnimType;
   pActiveFatality->mTargetEndAnimType = pFatality->mTargetEndAnimType;
   if (pFatality->mBoarding && pFatality->mAttackerEndAnimType >= 0)
      pActiveFatality->mAttackerEndAnimStarted = false;
   else
      pActiveFatality->mAttackerEndAnimStarted = true;

   // Flag fatality asset as being in cooldown
   pFatalityAsset->mFlagCooldown = true;

   // Orient towards target if we're not moving   
   if (pActiveFatality->mMoveType == cFatalityNoMove)
   {
      BVector ofs = pTarget->getPosition() - pAttacker->getPosition();
      ofs.y = 0;
      ofs.normalize();
      float angle = atan2f(ofs.x, ofs.z);
      BMatrix out;
      out.makeRotateY(angle);
      pAttacker->setRotation(out);
   }


   // Attach attacker to target for boarding fatality
   if (pFatality->mBoarding)
   {
      // Calculate attacker orientation (relative to target offset * target mtx)
      BQuaternion attackerOrientation;
      attackerOrientation = pFatalityAsset->mTargetOrientationOffset.inverse() * BQuaternion(targetMatrix);
      BMatrix attackerOrientationMatrix;
      attackerOrientation.toMatrix(attackerOrientationMatrix);

      // Snap rotation to desired
      pAttacker->setRotation(attackerOrientationMatrix);
      pActiveFatality->mOrientationTransition.makeIdentity();

      // Calculate attacker offset between desired and current
      BVector targetPos;
      targetMatrix.getTranslation(targetPos);
      BVector desiredAttackerPosition;
      attackerOrientationMatrix.transformVectorAsPoint(-pFatalityAsset->mTargetPositionOffset, desiredAttackerPosition);
      desiredAttackerPosition += targetPos;
      pActiveFatality->mPositionTransition = desiredAttackerPosition - pAttacker->getPosition();

      // Make the position transition vector relative to the target so that it can be transformed during the update
      targetMatrix.invert();
      targetMatrix.transformVector(pActiveFatality->mPositionTransition, pActiveFatality->mPositionTransition);

      // Attach
      pTarget->attachObject(pAttacker->getID(), targetToBoneHandle, -1, true);
   }

   // Start animating if we're not waiting for transitions to complete first
   if (!pFatality->mTransitionBeforeAnimating)
   {
      bool result = startAnims(pActiveFatality);
      if (!result)
         stopFatality(pAttacker->getID(), true);
   }
   else
   {
      // If not starting the anims immediately, go ahead and clear out the attacker hardpoints
      // so it lines up correctly
      long numAttackerHardpoints = pAttacker->getNumberHardpoints();
      for (long i = 0; i < numAttackerHardpoints; i++)
         pAttacker->clearHardpoint(i);
   }

   //SLB: Debug
    //const BOPQuadHull* pHull = pObstruction->getHull();
    //const BVector p1(pHull->mPoint[0].mX, 0.0f, pHull->mPoint[0].mZ);
    //const BVector p2(pHull->mPoint[1].mX, 0.0f, pHull->mPoint[1].mZ);
    //const BVector p3(pHull->mPoint[2].mX, 0.0f, pHull->mPoint[2].mZ);
    //const BVector p4(pHull->mPoint[3].mX, 0.0f, pHull->mPoint[3].mZ);
    //gTerrainSimRep.addDebugLineOverTerrain(p1, p2, cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //gTerrainSimRep.addDebugLineOverTerrain(p2, p3, cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //gTerrainSimRep.addDebugLineOverTerrain(p3, p4, cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //gTerrainSimRep.addDebugLineOverTerrain(p4, p1, cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //gTerrainSimRep.addDebugPointOverTerrain(pAttacker->getPosition(), 1.0f, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //gTerrainSimRep.addDebugLineOverTerrain(pTarget->getPosition(), pTarget->getForward() + pTarget->getPosition(), cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //gTerrainSimRep.addDebugPointOverTerrain(pAttacker->getPosition() + pActiveFatality->mPositionTransition, 1.0f, cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryNone, duration);
    //BQuaternion targetOrientation(pTarget->getForward(), pTarget->getUp(), pTarget->getRight());
    //targetOrientation = pActiveFatality->mOrientationTransition * targetOrientation;
    //BMatrix targetOrientationMatrix;
    //targetOrientation.toMatrix(targetOrientationMatrix);
    //gTerrainSimRep.addDebugLineOverTerrain(pTarget->getPosition(), targetOrientationMatrix.getRow(2) + pTarget->getPosition(), cDWORDGreen, cDWORDGreen, 0.5f, BDebugPrimitives::cCategoryNone, duration);

   return true;
}

//==============================================================================
// BFatalityManager::isPositionValid
//==============================================================================
bool BFatalityManager::isPositionValid(BVector position) const
{
   // Scan through the list and test active fatalities
   long highWaterMark = mActiveFatalities.getHighWaterMark();
   for (long i = 0; i < highWaterMark; i++)
   {
      if (mActiveFatalities.isInUse(i))
      {
         const BActiveFatality* pActiveFatality = mActiveFatalities.getConst(i);
         if (pActiveFatality->mPosition.distance(position) <= gDatabase.getFatalityExclusionRange())
            return false;
      }
   }

   return true;
}

//==============================================================================
// BFatalityManager::getFatality
//==============================================================================
const BActiveFatality* BFatalityManager::getFatality(BEntityID unit, long* pIndex) const
{
   // Scan through the list and look for the unit
   long highWaterMark = mActiveFatalities.getHighWaterMark();
   for (long i = 0; i < highWaterMark; i++)
   {
      if (mActiveFatalities.isInUse(i))
      {
         const BActiveFatality* pActiveFatality = mActiveFatalities.getConst(i);
         if ((pActiveFatality->mAttackerID == unit) || (pActiveFatality->mTargetID == unit))
         {
            if (pIndex)
               *pIndex = i;
            return pActiveFatality;
         }
      }
   }

   return NULL;
}

//==============================================================================
// BFatalityManager::update
//==============================================================================
void BFatalityManager::update()
{
   const DWORD timeDWORD = gWorld->getGametime();

   // Scan through the list and update active fatalities
   long highWaterMark = mActiveFatalities.getHighWaterMark();
   for (long i = 0; i < highWaterMark; i++)
   {
      if (mActiveFatalities.isInUse(i))
      {
         BActiveFatality* pActiveFatality = mActiveFatalities.get(i);
         BUnit* pAttacker = gWorld->getUnit(pActiveFatality->mAttackerID);
         BUnit* pTarget = gWorld->getUnit(pActiveFatality->mTargetID);

         // Start attacker end anim if it's time
         if (!pActiveFatality->mAttackerEndAnimStarted && (timeDWORD >= pActiveFatality->mAttackerEndTimeDWORD))
         {
            // Set attacker end animation
            float duration = 0.0f;
            if (pAttacker)
            {
               if (pAttacker->isAnimationLocked())
                  pAttacker->unlockAnimation();
               pAttacker->setAnimation(pActiveFatality->mCallerActionID, BObjectAnimationState::cAnimationStateHandAttack, pActiveFatality->mAttackerEndAnimType, false, false, -1, true);
               pAttacker->computeAnimation();
               bool animSet = pAttacker->isAnimationSet(BObjectAnimationState::cAnimationStateHandAttack, pActiveFatality->mAttackerEndAnimType);
               BASSERT(animSet);
               duration = pAttacker->getAnimationDuration(cActionAnimationTrack);
            }

            // Set target end animation
            if (pTarget && (pActiveFatality->mTargetEndAnimType >= 0))
            {
               // Clear out to prepare for anim
               long numTargetHardpoints = pTarget->getNumberHardpoints();
               for (long i = 0; i < numTargetHardpoints; i++)
                  pTarget->clearHardpoint(i);
               pTarget->removeOpps();
               for (long i = pTarget->getNumberActions() - 1; i >= 0; i--)
               {
                  const BAction* pAction = pTarget->getActionByIndexConst(i);
                  if (!pAction->getFlagPersistent() && (pAction->getType() != BAction::cActionTypeUnitUnderAttack) && (pAction->getType() != BAction::cActionTypeUnitRevealToTeam))
                     pTarget->removeActionByID(i);
               }
               pTarget->clearController(BActionController::cControllerOrient);
               pTarget->clearController(BActionController::cControllerAnimation);

               // Set anim
               if (pTarget->isAnimationLocked())
                  pTarget->unlockAnimation();
               pTarget->setAnimation(cInvalidActionID, BObjectAnimationState::cAnimationStateDeath, pActiveFatality->mTargetEndAnimType, false, false, -1, true);
               pTarget->computeAnimation();
               bool animSet = pTarget->isAnimationSet(BObjectAnimationState::cAnimationStateDeath, pActiveFatality->mTargetEndAnimType);
               BASSERT(animSet);
            }

            // Set timers
            pActiveFatality->mAttackerEndAnimStarted = true;
            const DWORD durationDWORD = ((DWORD)(duration * 1000.0f));
            DWORD cooldownTime = pActiveFatality->mCooldownEndTimeDWORD - pActiveFatality->mFatalityEndTimeDWORD;
            pActiveFatality->mFatalityEndTimeDWORD = timeDWORD + durationDWORD;
            pActiveFatality->mCooldownEndTimeDWORD = pActiveFatality->mFatalityEndTimeDWORD + cooldownTime;
         }

         // If we're done with the fatality, clear the flag and continue.  Don't check for completion until attacker end anim is started
         if (pActiveFatality->mAttackerEndAnimStarted && !pActiveFatality->mFlagFatalityOver && (timeDWORD >= pActiveFatality->mFatalityEndTimeDWORD))
         {
            if (pAttacker)
               pAttacker->setFlagDoingFatality(false);

            if (pTarget)
            {
               if (pActiveFatality->mKillTarget)
               {
                  if (pAttacker)
                     pTarget->setKilledByID(pAttacker->getID());

                  if (pAttacker && pAttacker->getProtoObject() && pAttacker->getProtoObject()->getFlagKillOnDetach())
                     pTarget->kill(true);
                  else
                     pTarget->kill(false);
               }
               pTarget->setFlagDoingFatality(false);
            }

            pActiveFatality->mFlagFatalityOver = true;
            if (pActiveFatality->mObstruction)
               gObsManager.deleteObstruction(pActiveFatality->mObstruction);
         }

         // If we hit the end of the cooldown, kill the entry and continue.  Don't check cooldown until attacker end anim is started
         if (pActiveFatality->mAttackerEndAnimStarted && (timeDWORD >= pActiveFatality->mCooldownEndTimeDWORD))
         {
            pActiveFatality->mFatalityAsset->mFlagCooldown = false;
            mActiveFatalities.release(i);
            continue;
         }

         // If we're done transitioning then continue
         if (pActiveFatality->mLastTransitionUpdateTimeDWORD >= pActiveFatality->mTransitionEndTimeDWORD)
            continue;

         // If there is no transition movement, continue
         if (pActiveFatality->mMoveType == cFatalityNoMove)
            continue;

         // Transition
         if (timeDWORD > pActiveFatality->mTransitionStartTimeDWORD)
         {
            float transitionScale;
            BVector transitionPositionOffset;

            // Calc transition scale and position offset

            // Aircraft transition is non-linear
            float airHeightOffset = 0.0f;
            if (pActiveFatality->mAircraftTransition)
            {
               // Calc transition percent/scale
               float transitionPercent = Math::Clamp((timeDWORD - pActiveFatality->mTransitionStartTimeDWORD) * 0.001f * pActiveFatality->mRecTransitionDuration, 0.0f, 1.0f);
               float lastPercent = Math::Clamp((pActiveFatality->mLastTransitionUpdateTimeDWORD - pActiveFatality->mTransitionStartTimeDWORD) * 0.001f * pActiveFatality->mRecTransitionDuration, 0.0f, 1.0f);
               transitionScale = sinf(transitionPercent * cPiOver2) - sinf(lastPercent * cPiOver2);

               // Calc quadratic coeffs
               float heightOffset = pActiveFatality->mPositionTransition.y;
               float apexPercent = pActiveFatality->mpFatality->mAirTransitionApexPercent;
               float apexHeight = heightOffset + pActiveFatality->mpFatality->mAirTransitionHeight;
               float b = (apexHeight - (apexPercent * apexPercent * heightOffset)) / (apexPercent - apexPercent * apexPercent);
               float a = heightOffset - b;

               // Calc heights at last/current times (ax^2 + bx).  Height offset is the difference
               float lastHeight = a * lastPercent * lastPercent + b * lastPercent;
               float currentHeight = a * transitionPercent * transitionPercent + b * transitionPercent;
               airHeightOffset = currentHeight - lastHeight;

               transitionPositionOffset = pActiveFatality->mPositionTransition * transitionScale;
               transitionPositionOffset.y = airHeightOffset;
            }
            // All others are linear
            else
            {
               float transitionTime = (Math::Min(timeDWORD, pActiveFatality->mTransitionEndTimeDWORD) - pActiveFatality->mLastTransitionUpdateTimeDWORD) * 0.001f;
               transitionScale = transitionTime * pActiveFatality->mRecTransitionDuration;
               transitionPositionOffset = pActiveFatality->mPositionTransition * transitionScale;
            }

            // Orientation offset
            BQuaternion identQuat;
            identQuat.makeIdentity();
            BQuaternion transitionOrientationOffset = identQuat.slerp(pActiveFatality->mOrientationTransition, transitionScale);

            // Update lastUpdate time
            pActiveFatality->mLastTransitionUpdateTimeDWORD = timeDWORD;

            if (pAttacker && pTarget)
            {
               if (pActiveFatality->mMoveType == cFatalityAttackerPosTargetOrient)
               {
                  BVector attackerPosition = pAttacker->getPosition();
                  attackerPosition += transitionPositionOffset;
                  BQuaternion targetOrientation(pTarget->getForward(), pTarget->getUp(), pTarget->getRight());
                  targetOrientation = transitionOrientationOffset * targetOrientation;
                  BMatrix targetOrientationMatrix;
                  targetOrientation.toMatrix(targetOrientationMatrix);

                  pTarget->setRotation(targetOrientationMatrix);
                  #ifdef SYNC_Unit
                     syncUnitData("BFatalityManager::update 1", attackerPosition);
                  #endif
                  pAttacker->setPosition(attackerPosition);
               }
               else if (pActiveFatality->mMoveType == cFatalityAttackerPosOrient)
               {
                  // Get the target orientation.  Use the to bone orientation if specified
                  BMatrix targetRot;
                  pTarget->getWorldMatrix(targetRot);
                  if (pActiveFatality->mToBoneRelative)
                  {
                     long targetToBoneHandle = -1;
                     long boardPointHandle = pTarget->getVisual()->getPointHandle(cActionAnimationTrack, cVisualPointBoard);
                     const BProtoVisualPoint* pPoint = pTarget->getVisual()->getPointProto(cActionAnimationTrack, boardPointHandle);
                     if (pPoint)
                        targetToBoneHandle = pPoint->mBoneHandle;

                     if (targetToBoneHandle != -1)
                     {
                        BMatrix targetMatrix;
                        pTarget->getWorldMatrix(targetMatrix);
                        BVector tempPos;
                        BMatrix tempMtx;
                        if (pTarget->getVisual()->getBone(targetToBoneHandle, &tempPos, &tempMtx, 0, &targetMatrix))
                           targetRot = tempMtx;
                     }
                  }

                  targetRot.transformVector(transitionPositionOffset, transitionPositionOffset);

                  // Calc updated attacker pos
                  BVector attackerPosition = pAttacker->getPosition();
                  attackerPosition += transitionPositionOffset;

                  BObjectAttachment* pObjectAttachment = pAttacker->getAttachedToObjectAttachment();
                  if (pObjectAttachment && pObjectAttachment->mUseOffset)
                  {
                     // Calculate matrix for positional movement relative to the current
                     // transform.  TODO - There is a smarter way to calculate this.
                     BMatrix oldWorldMatrix, newWorldMatrix;
                     pAttacker->getWorldMatrix(oldWorldMatrix);
                     #ifdef SYNC_Unit
                        syncUnitData("BFatalityManager::update 2", attackerPosition);
                     #endif
                     pAttacker->setPosition(attackerPosition);
                     pAttacker->getWorldMatrix(newWorldMatrix);

                     BMatrix fromPosMatrix;
                     oldWorldMatrix.invert();
                     fromPosMatrix.mult(newWorldMatrix, oldWorldMatrix);

                     // Calculate new offset matrix
                     BMatrix newOffsetMatrix;
                     newOffsetMatrix.mult(fromPosMatrix, pObjectAttachment->mOffset);
                     pObjectAttachment->mOffset = newOffsetMatrix;
                  }
               }

               //SLB: Debug
               //gTerrainSimRep.addDebugLineOverTerrain(pTarget->getPosition(), (pTarget->getForward() * 3.0f) + pTarget->getPosition(), cDWORDPurple, cDWORDPurple, 0.5f, BDebugPrimitives::cCategoryNone, 3.0f);
            }
         }

         // If animations not started and transitions complete, start the anims
         if ((pActiveFatality->mFatalityEndTimeDWORD == UINT_MAX) && (pActiveFatality->mLastTransitionUpdateTimeDWORD >= pActiveFatality->mTransitionEndTimeDWORD))
            startAnims(pActiveFatality);
      }
   }
}

//==============================================================================
// BFatalityManager::reset
//==============================================================================
void BFatalityManager::reset(bool resetAssets)
{
   if (resetAssets)
   {
      mFatalityAssetMap.clear();
      mFatalityAssets.resize(0);
      mFatalityAssetsLoaded = false;
   }
   mActiveFatalities.releaseAll();
   mActiveFatalities.clear();
}

//==============================================================================
//==============================================================================
void BFatalityManager::loadFatalityAssets()
{
   mFatalityAssets.resize(0);

   BXMLReader reader;
   BXMLNode rootNode;

   if (!reader.load(cDirTactics, "fatalityData.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return;

   rootNode = reader.getRootNode();
   for (int i = 0; i < rootNode.getNumberChildren(); i++)
   {
      BXMLNode& child = rootNode.getChild(i);
      if (child.getName().compare("Tactic") == 0)
      {
         // Get the attacker proto name / ID
         BSimString attackerName;
         if (!child.getAttribValue("attacker", &attackerName))
            continue;
         BProtoObjectID attackerPID = gDatabase.getProtoObject(attackerName.asNative());
         BProtoObject* pAttackerPO = gDatabase.getGenericProtoObject(attackerPID);
         if (!pAttackerPO)
            continue;
         BTactic* pTactic = pAttackerPO->getTactic();
         if (!pTactic)
            continue;

         for (int j = 0; j < child.getNumberChildren(); j++)
         {
            BXMLNode& actionChild = child.getChild(j);
            if (actionChild.getName().compare("Action") == 0)
            {
               BSimString actionName;
               actionChild.getText(actionName);
               // Get protoaction
               BProtoAction* pPA = NULL;
               int actionIndex = 0;
               while (!pPA && (actionIndex < pTactic->getNumberProtoActions()))
               {
                  pPA = pTactic->getProtoAction(actionIndex);
                  if (pPA->getName().compare(actionName) != 0)
                     pPA = NULL;
                  actionIndex++;
               }
               if (!pPA)
                  continue;
               int actionID = pPA->getIndex();

               for (int k = 0; k < actionChild.getNumberChildren(); k++)
               {
                  BXMLNode& targetChild = actionChild.getChild(k);
                  if (targetChild.getName().compare("Target") == 0)
                  {
                     BSimString targetName;
                     targetChild.getText(targetName);
                     BProtoObjectID targetPID = gDatabase.getProtoObject(targetName.asNative());
                     if (targetPID == cInvalidProtoObjectID)
                        continue;

                     int numAssets = 0;
                     int firstAssetIndex = -1;
                     bool firstAsset = true;
                     for (int m = 0; m < targetChild.getNumberChildren(); m++)
                     {
                        BXMLNode& assetChild = targetChild.getChild(m);
                        if (assetChild.getName().compare("Asset") == 0)
                        {
                           int attackerAssetIndex;
                           int targetAssetIndex;
                           BVector posOffset;
                           BVector4 orientOffset;
                           int weight = 1;
                           if (!assetChild.getAttribValueAsInt("attackerAssetIndex", attackerAssetIndex))
                              continue;
                           if (!assetChild.getAttribValueAsInt("targetAssetIndex", targetAssetIndex))
                              continue;
                           BSimString offsetString;
                           if (!assetChild.getAttribValue("orientOffset", &offsetString))
                              continue;
                           if (!convertSimTokenToVector4(offsetString, orientOffset))
                              continue;
                           if (!assetChild.getAttribValue("posOffset", &offsetString))
                              continue;
                           if (!convertSimTokenToVector(offsetString, posOffset))
                              continue;
                           assetChild.getAttribValueAsInt("weight", weight);

                           long fatalityAssetIndex = registerFatalityAsset(posOffset, orientOffset, weight, attackerAssetIndex, targetAssetIndex);

                           // If this is the first asset, add an entry into the hashmap
                           if (firstAsset)
                           {
                              firstAssetIndex = fatalityAssetIndex;
                              firstAsset = false;
                           }
                           numAssets++;
                        }
                     }

                     // If assets were added, add entry in hashmap
                     if (numAssets > 0)
                     {
                        BFatalityAssetKey key(attackerPID, targetPID, actionID);
                        BFatalityAssetValue value(firstAssetIndex, numAssets);
                        mFatalityAssetMap.insert(key, value);
                     }
                  }
               }
            }
         }
      }
   }

   mFatalityAssetsLoaded = true;
}


//==============================================================================
//==============================================================================
void BFatalityManager::addFatalityAssets(BFatalityAssetIndexArray& indexArray, BProtoObjectID attackerID, BProtoObjectID targetID, long actionID)
{
   // Clear array
   indexArray.resize(0);

   // Look up all assets and add indices to array
   BFatalityAssetKey key(attackerID, targetID, actionID);

   BFatalityAssetMap::const_iterator it = mFatalityAssetMap.find(key);
   BFatalityAssetValue* pValue = NULL;
   if (it != mFatalityAssetMap.end())
   {
      pValue = (BFatalityAssetValue*) &it->second;
      if (pValue)
      {
         for (int i = 0; i < pValue->mNumAssets; i++)
         {
            indexArray.add(pValue->mFirstAssetIndex + i);
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BFatalityManager::stopFatality(BEntityID unit, bool allowTargetKill)
{
   long index;
   BActiveFatality* pActiveFatality = const_cast<BActiveFatality*>(getFatality(unit, &index));
   if (!pActiveFatality)
      return;

   BUnit* pAttacker = gWorld->getUnit(pActiveFatality->mAttackerID);
   BUnit* pTarget = gWorld->getUnit(pActiveFatality->mTargetID);

   if (pAttacker)
      pAttacker->setFlagDoingFatality(false);

   if (pTarget)
   {
      if (pActiveFatality->mKillTarget && allowTargetKill)
      {
         pTarget->setKilledByID(pAttacker->getID());
         pTarget->kill(false);
      }
      pTarget->setFlagDoingFatality(false);
   }

   pActiveFatality->mFlagFatalityOver = true;
   if (pActiveFatality->mObstruction)
      gObsManager.deleteObstruction(pActiveFatality->mObstruction);

   pActiveFatality->mFatalityAsset->mFlagCooldown = false;
   mActiveFatalities.release(index);
}

//==============================================================================
//==============================================================================
bool BFatalityManager::startAnims(BActiveFatality* pActiveFatality)
{
   if (!pActiveFatality || !pActiveFatality->mpFatality || !pActiveFatality->mFatalityAsset)
      return false;

   BUnit* pAttacker = gWorld->getUnit(pActiveFatality->mAttackerID);
   BUnit* pTarget = gWorld->getUnit(pActiveFatality->mTargetID);

   long attackerAnimType = pActiveFatality->mpFatality->mAttackerAnimType;
   long targetAnimType = pActiveFatality->mpFatality->mTargetAnimType;

   //==============================================================================
   // Set attacker animation, free hardpoints
   if (pAttacker)
   {
      long numAttackerHardpoints = pAttacker->getNumberHardpoints();
      for (long i = 0; i < numAttackerHardpoints; i++)
         pAttacker->clearHardpoint(i);
      if (pActiveFatality->mpFatality->mBoarding && pAttacker->isAnimationLocked())
         pAttacker->unlockAnimation();
      pAttacker->setAnimation(pActiveFatality->mCallerActionID, BObjectAnimationState::cAnimationStateHandAttack, attackerAnimType, true, true, pActiveFatality->mFatalityAsset->mAttackerAnimID, true);
      pAttacker->computeAnimation();
      bool animSet = pAttacker->isAnimationSet(BObjectAnimationState::cAnimationStateHandAttack, attackerAnimType);
      BASSERT(animSet);
      if (!animSet)
         return false;
      if (!pActiveFatality->mpFatality->mBoarding) // Go back to idle once fatality done, unless its a boarding fatality anim
         pAttacker->setAnimation(pActiveFatality->mCallerActionID, BObjectAnimationState::cAnimationStateIdle, pAttacker->getIdleAnim(), false, false);
   }

   //==============================================================================
   // Set target animation, free hardpoints, controllers, opps and actions
   if (pTarget && pActiveFatality->mpFatality->mAnimateTarget)
   {
      long numTargetHardpoints = pTarget->getNumberHardpoints();
      for (long i = 0; i < numTargetHardpoints; i++)
         pTarget->clearHardpoint(i);
      pTarget->removeOpps();
      for (long i = pTarget->getNumberActions() - 1; i >= 0; i--)
      {
         const BAction* pAction = pTarget->getActionByIndexConst(i);
         if (!pAction->getFlagPersistent() && (pAction->getType() != BAction::cActionTypeUnitUnderAttack) && (pAction->getType() != BAction::cActionTypeUnitRevealToTeam))
            pTarget->removeActionByID(i);
      }
      pTarget->clearController(BActionController::cControllerOrient);
      pTarget->clearController(BActionController::cControllerAnimation);
      if (pTarget->isAnimationLocked())
         pTarget->unlockAnimation(); // Death overrides everything
      pTarget->setAnimation(cInvalidObjectID, BObjectAnimationState::cAnimationStateDeath, targetAnimType, false, false, pActiveFatality->mFatalityAsset->mTargetAnimID);
      pTarget->computeAnimation();
      pTarget->lockAnimation(10000000, false);
      bool animSet = pTarget->isAnimationSet(BObjectAnimationState::cAnimationStateDeath, targetAnimType);
      BASSERT(animSet);
      if (!animSet)
         return false;
   }

   // Set timers now that the anims are started
   const DWORD timeDWORD = gWorld->getGametime();
   const float duration = (pAttacker) ? pAttacker->getAnimationDuration(cActionAnimationTrack) : 0.0f;
   const DWORD durationDWORD = ((DWORD)(duration * 1000.0f));

   // Set transition time if not already set (boarding anims use a default time)
   if (pActiveFatality->mTransitionEndTimeDWORD == UINT_MAX)
   {
      // Get transition time
      float transitionTime;
      DWORD transitionTimeDWORD;
      if (pActiveFatality->mTransitionTimeDWORD > 0)
      {
         transitionTimeDWORD = Math::Min(pActiveFatality->mTransitionTimeDWORD, durationDWORD);
         transitionTime = transitionTimeDWORD * 0.001f;
      }
      else
      {
         transitionTime = Math::Min(duration * gDatabase.getFatalityTransitionScale(), gDatabase.getFatalityMaxTransitionTime());
         transitionTimeDWORD = ((DWORD)(transitionTime * 1000.0f));
      }
      DWORD transitionDelayDWORD = Math::Min(pActiveFatality->mTransitionDelayDWORD, durationDWORD - transitionTimeDWORD);

      // Set transition times
      pActiveFatality->mTransitionStartTimeDWORD = timeDWORD + transitionDelayDWORD;
      pActiveFatality->mTransitionEndTimeDWORD = pActiveFatality->mTransitionStartTimeDWORD + transitionTimeDWORD;
      pActiveFatality->mRecTransitionDuration = 1.0f / transitionTime;
      pActiveFatality->mLastTransitionUpdateTimeDWORD = pActiveFatality->mTransitionStartTimeDWORD;
   }

   pActiveFatality->mFatalityEndTimeDWORD = timeDWORD + durationDWORD;
   pActiveFatality->mCooldownEndTimeDWORD = pActiveFatality->mFatalityEndTimeDWORD + pActiveFatality->mpFatality->mCooldownTime;
   pActiveFatality->mAttackerEndTimeDWORD = pActiveFatality->mFatalityEndTimeDWORD + pActiveFatality->mAttackerIdleTimeDWORD;

   //-- Create audio reaction to the fatality or boarding
   if (pAttacker && pTarget && !pActiveFatality->mpFatality->mBoarding)
   {
      BSquadSoundType soundType = cSquadSoundNone;
      const BPlayer* pAttackingPlayer = pAttacker->getPlayer();
      BASSERT(pAttackingPlayer);
      if (pAttackingPlayer->getCivID() == gDatabase.getCivID("UNSC"))
      {
         soundType = cSquadSoundChatterReactFatalityUNSC;
      }
      else if (pAttackingPlayer->getCivID() == gDatabase.getCivID("Covenant"))
      {
         soundType = cSquadSoundChatterReactFatalityCOV;
      }

      // Play reaction
      if (soundType != cSquadSoundNone)
      {
         gWorld->createPowerAudioReactions(pAttacker->getPlayerID(), soundType, pAttacker->getPosition(), pAttacker->getLOS(), pAttacker->getParentID());
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BFatalityManager::save(BStream* pStream, int saveType) const
{
   GFWRITEFREELIST(pStream, saveType, BActiveFatality, mActiveFatalities, uint8, 200);
   GFWRITEMARKER(pStream, cSaveMarkerFatalityManager);
   return true;
}

//==============================================================================
//==============================================================================
bool BFatalityManager::load(BStream* pStream, int saveType)
{
   GFREADFREELIST(pStream, saveType, BActiveFatality, mActiveFatalities, uint8, 200);
   GFREADMARKER(pStream, cSaveMarkerFatalityManager);
   return true;
}

//==============================================================================
//==============================================================================
bool BActiveFatality::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPositionTransition);

   GFWRITEVAL(pStream, float, mOrientationTransition.x);
   GFWRITEVAL(pStream, float, mOrientationTransition.y);
   GFWRITEVAL(pStream, float, mOrientationTransition.z);
   GFWRITEVAL(pStream, float, mOrientationTransition.w);

   GFWRITEVECTOR(pStream, mPosition);

   // mpFatality
   BProtoAction* pProtoAction = (mpFatality ? mpFatality->mpProtoAction : NULL);
   GFWRITEPROTOACTIONPTR(pStream, pProtoAction);
   GFWRITEVAL(pStream, uint8, (mpFatality ? mpFatality->mIndex : UINT8_MAX));

   // mFatalityAsset
   GFWRITEVAL(pStream, uint16, (mFatalityAsset ? mFatalityAsset->mIndex : UINT16_MAX));

   //BOPObstructionNode*  mObstruction;

   GFWRITEVAR(pStream, BEntityID, mAttackerID);
   GFWRITEVAR(pStream, BEntityID, mTargetID);
   GFWRITEVAR(pStream, long, mAttackerEndAnimType);
   GFWRITEVAR(pStream, long, mTargetEndAnimType);
   GFWRITEVAR(pStream, BActionID, mCallerActionID);
   GFWRITEVAR(pStream, float, mRecTransitionDuration);
   GFWRITEVAR(pStream, DWORD, mTransitionStartTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mTransitionEndTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mTransitionTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mTransitionDelayDWORD);
   GFWRITEVAR(pStream, DWORD, mFatalityEndTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mCooldownEndTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mLastTransitionUpdateTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mAttackerEndTimeDWORD);
   GFWRITEVAR(pStream, DWORD, mAttackerIdleTimeDWORD);
   GFWRITEVAL(pStream, int8, mMoveType);
   GFWRITEBITBOOL(pStream, mFlagFatalityOver);
   GFWRITEBITBOOL(pStream, mKillTarget);
   GFWRITEBITBOOL(pStream, mAttackerEndAnimStarted);
   GFWRITEBITBOOL(pStream, mToBoneRelative);
   GFWRITEBITBOOL(pStream, mAircraftTransition);
   return true;
}

//==============================================================================
//==============================================================================
bool BActiveFatality::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPositionTransition);

   float x, y, z, w;
   GFREADVAR(pStream, float, x);
   GFREADVAR(pStream, float, y);
   GFREADVAR(pStream, float, z);
   GFREADVAR(pStream, float, w);
   mOrientationTransition.x = x;
   mOrientationTransition.y = y;
   mOrientationTransition.z = z;
   mOrientationTransition.w = w;

   GFREADVECTOR(pStream, mPosition);
   
   // mpFatality
   BProtoAction* pProtoAction = NULL;
   GFREADPROTOACTIONPTR(pStream, pProtoAction);
   uint8 fatilityIndex = UINT8_MAX;
   GFREADVAR(pStream, uint8, fatilityIndex);
   if (pProtoAction && fatilityIndex != UINT8_MAX)
      mpFatality = pProtoAction->getFatalityByIndex(fatilityIndex);

   // mFatalityAsset
   uint16 fatalityAssetIndex = UINT16_MAX;
   GFREADVAR(pStream, uint16, fatalityAssetIndex);
   if (fatalityAssetIndex != UINT16_MAX)
      mFatalityAsset = gFatalityManager.getFatalityAsset((long)fatalityAssetIndex);

   //BOPObstructionNode*  mObstruction;

   GFREADVAR(pStream, BEntityID, mAttackerID);
   GFREADVAR(pStream, BEntityID, mTargetID);
   GFREADVAR(pStream, long, mAttackerEndAnimType);
   GFREADVAR(pStream, long, mTargetEndAnimType);
   GFREADVAR(pStream, BActionID, mCallerActionID);
   GFREADVAR(pStream, float, mRecTransitionDuration);
   GFREADVAR(pStream, DWORD, mTransitionStartTimeDWORD);
   GFREADVAR(pStream, DWORD, mTransitionEndTimeDWORD);
   GFREADVAR(pStream, DWORD, mTransitionTimeDWORD);
   GFREADVAR(pStream, DWORD, mTransitionDelayDWORD);
   GFREADVAR(pStream, DWORD, mFatalityEndTimeDWORD);
   GFREADVAR(pStream, DWORD, mCooldownEndTimeDWORD);
   GFREADVAR(pStream, DWORD, mLastTransitionUpdateTimeDWORD);
   GFREADVAR(pStream, DWORD, mAttackerEndTimeDWORD);
   GFREADVAR(pStream, DWORD, mAttackerIdleTimeDWORD);
   GFREADVAL(pStream, int8, eFatalityMoveType, mMoveType);
   GFREADBITBOOL(pStream, mFlagFatalityOver);
   GFREADBITBOOL(pStream, mKillTarget);
   GFREADBITBOOL(pStream, mAttackerEndAnimStarted);
   if (BFatalityManager::cGameFileVersion > 1)
   {
      GFREADBITBOOL(pStream, mToBoneRelative);
      GFREADBITBOOL(pStream, mAircraftTransition);
   }

   gSaveGame.remapAnimType(mAttackerEndAnimType);
   gSaveGame.remapAnimType(mTargetEndAnimType);

   return true;
}
