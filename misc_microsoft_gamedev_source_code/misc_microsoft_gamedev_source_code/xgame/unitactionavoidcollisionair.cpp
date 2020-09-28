//==============================================================================
// unitactionavoidcollisionair.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "ActionManager.h"
#include "unitactionavoidcollisionair.h"
#include "unitactionrangedattack.h"
#include "unitactionstasis.h"
#include "unit.h"
#include "world.h"
#include "unitquery.h"
#include "squadactionattack.h"
#include "squadactionmove.h"
#include "Physics.h"
#include "tactic.h"
#include "user.h"
#include "usermanager.h"
#include "terraineffect.h"
#include "terraineffectmanager.h"
#include "protoimpacteffect.h"
#include "worldsoundmanager.h"
#include "selectionmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionAvoidCollisionAir, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionAvoidCollisionAir::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   if (mpProtoAction && !mpProtoAction->getFlagAvoidOnly())
   {
      uint index = cMaxAirSpotIndex;
      BAirSpot* pNewClaimedAirSpot = gWorld->getObjectManager()->createClaimedAirSpot(index);
      if (pNewClaimedAirSpot)
      {
         pNewClaimedAirSpot->init();
         mClaimedAirSpotIndex = index;
         pNewClaimedAirSpot->mAircraftID = mpOwner->getID();
         pNewClaimedAirSpot->mClaimedPos = mpOwner->getPosition();
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionAvoidCollisionAir::disconnect()
{
//-- FIXING PREFIX BUG ID 2188
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   BAction::disconnect();

   if (mClaimedAirSpotIndex != cMaxAirSpotIndex)
      gWorld->getObjectManager()->releaseClaimedAirSpot(mClaimedAirSpotIndex);

   if (mKamikazeTarget != cInvalidObjectID)
   {
      BUnit* pTarget = gWorld->getUnit(mKamikazeTarget);
      if (pTarget)
         pTarget->removeAttackingUnit(pUnit->getID(), getID());
      mKamikazeTarget = cInvalidObjectID;
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionAvoidCollisionAir::init()
{
   if (!BAction::init())
      return(false);
      
   setFlagPersistent(true);
   mFlagConflictsWithIdle=false;
   mbOffsetAnchorSearch=false;
   mbCrashing=false;
   mbLimitSpeed=true;
   mbCanKamikaze=true;
   mAnchorSearchPass=0;
   mBirthTimer=0.0f;
   mKamikazeTarget=cInvalidObjectID;
   mCrashPos=cInvalidVector;
   mKillerUnit=cInvalidObjectID;
   mKillerPlayer=-1;
   mKillerTeam=-1;
   mDetonateTime=0;
   mClaimedAirSpotIndex = cMaxAirSpotIndex;

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAvoidCollisionAir::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (pUnit->isGarrisoned())
      return (true);

   // Early out if squad is under cinematic control, that is the squad is controlled by the wow moment
   if (pUnit->getFlagIsUnderCinematicControl())
      return (true);

   BObjectManager* pObjMan = gWorld->getObjectManager();
   BASSERT(pObjMan);

   // If we're a ground unit, just update our air obstruction
   if (!pUnit->getFlagFlying() && !pUnit->isPhysicsAircraft())
   {
      if (pUnit->getPhysicsObject() && !pUnit->getPhysicsObject()->isActive() && mClaimedAirSpotIndex != cMaxAirSpotIndex)
      {
         BAirSpot* myNewSpot = pObjMan->getClaimedAirSpot(mClaimedAirSpotIndex);
         if (myNewSpot)
            myNewSpot->mClaimedPos = pUnit->getPhysicsPosition();
      }

      return true;
   }

   mbAvoid = false;
   bool bAvoidFriendly = false;

   if (mbLimitSpeed)
   {
      if (mBirthTimer > 1.5f)
         mbLimitSpeed = false;
      mBirthTimer += elapsed;
   }

   BSquad *pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return (false);

   if (mpProtoAction->getFlagDisabled())
      return (true);

   // If crashing - check for impact
   if (mbCrashing)
   {
      // Set up detonate time if we haven't already done so
      if (mDetonateTime == 0)
      {
         DWORD detonateDelay = getRandRange(cSimRand, 2500, 5000);
         if ((pSquad->getNumStasisEffects() > 0) || (detonateDelay < 3000))
            detonateDelay = 0;
         mDetonateTime = gWorld->getGametime() + detonateDelay;
         pUnit->setFlagNotAttackable(true);
      }

      // Kill any attack actions
//-- FIXING PREFIX BUG ID 2191
      const BSquadActionAttack* pSquadAttackAction = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
//--
      if (pSquadAttackAction)
         pSquad->removeActionByID(pSquadAttackAction->getID());

      BVector currPos = pUnit->getPosition();
      BVector trajectory = currPos - mPrevPos;
      BEntityIDArray results(0, 100);
      BUnitQuery query(mPrevPos, currPos);
      query.setRelation(pUnit->getPlayerID(), cRelationTypeEnemy);
      gWorld->getUnitsInArea(&query, &results);

      // if our kamikaze target has died, do the detonate time
      if (!getKamikazeTarget())
         mKamikazeTarget = cInvalidObjectID;

      // Sometimes we detonate in the air at random
      if (mKamikazeTarget==cInvalidObjectID && (gWorld->getGametime() >= mDetonateTime))
      {
         crashDetonate(cInvalidObjectID);
         return (true);
      }

      long numResults = results.getNumber();
      for (long i = 0; i < numResults; ++i)
      {
         BUnit* pTestUnit = gWorld->getUnit(results[i]);
         if (!pTestUnit)
            continue;

         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         float intersectDistSqr = 0.0f;

         // Box collision check
         bool collided = pTestUnit->getSimBoundingBox()->raySegmentIntersects(mPrevPos, trajectory, true, NULL, intersectDistSqr);

         if (collided)
         {
            // Use the more accurate physics shape collision for buildings that have it.
            //
//-- FIXING PREFIX BUG ID 2190
            const BPhysicsObject* pPhysicsObject = pTestUnit->getPhysicsObject();
//--
            if(pPhysicsObject && pProtoObject->isType(gDatabase.getOTIDBuilding()))
               collided = pPhysicsObject->raySegmentIntersects(mPrevPos, trajectory, true, intersectDistSqr);
         }

         if (collided)
         {
            // Collision! - Time to Detonate
            crashDetonate(pTestUnit->getID());
            return (true);
         }
      }

      // If we haven't hit any objects, check for ground contact (project ahead since physics object should prevent overlap)
      {
         BVector terrainPos = currPos;
         gTerrainSimRep.getHeight(terrainPos, true);
         if ((currPos.y - (1.2f * pUnit->getProtoObject()->getObstructionRadiusY())) <= terrainPos.y)
         {
            // Collision! - Time to Detonate
            crashDetonate(cInvalidObjectID);
            return (true);
         }
      }
   }
   else // Not crashing
   {
      if (pUnit->getFlagDiesAtZeroHP())
         pUnit->setFlagDiesAtZeroHP(false);
   }

   mPrevPos = pUnit->getPosition();

   if (pSquad->getAnchorPosition() == cOriginVector)
   {
      pSquad->setAnchorPosition(pSquad->getLeashPosition());
   }

   #ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionAvoidCollisionAir::update - pSquad->getPosition()", pSquad->getPosition());
      syncUnitActionData("BUnitActionAvoidCollisionAir::update - pSquad->getAnchorPosition()", pSquad->getAnchorPosition());
      syncUnitActionData("BUnitActionAvoidCollisionAir::update - pSquad->getLeashPosition()", pSquad->getLeashPosition());
      syncUnitActionData("BUnitActionAvoidCollisionAir::update - pUnit->getPhysicsPosition()", pUnit->getPhysicsPosition());
   #endif

   // Manage movement animations (since physics objects do not have a normal unitMoveAction
//-- FIXING PREFIX BUG ID 2193
   const BSquadActionMove* pSquadMoveAction = reinterpret_cast<BSquadActionMove*>(pSquad->getActionByType(BAction::cActionTypeSquadMove));
//--

   // Rather than using the obstruction manager for this - we use a custom list of BAirSpots maintained by the ObjectManager
   // This allows us to quickly traverse only the aircraft and do simple range tests.
   mAvoidanceVec.zero();
   mNearestObstacleAlt = 0.0f;

   BVector ownPos = pUnit->getPhysicsPosition();
   BVector squadPos = pSquad->getPosition();

#ifdef SYNC_UnitAction
   syncUnitActionData("BUnitActionAvoidCollisionAir::update - ownPos", ownPos);
#endif

   //Get edge of map positions
   float tileSize = gTerrainSimRep.getDataTileScale();
   float fMinX = 0.0f, fMaxX = 0.0f, fMinZ = 0.0f, fMaxZ = 0.0f;
   if (gWorld->getFlagPlayableBounds())
   {
      fMinX = gWorld->getSimBoundsMinX();
      fMaxX = gWorld->getSimBoundsMaxX();
      fMinZ = gWorld->getSimBoundsMinZ();
      fMaxZ = gWorld->getSimBoundsMaxZ();
   }
   else
   {
      fMaxX = gTerrainSimRep.getNumXDataTiles() * tileSize;
      fMaxZ = gTerrainSimRep.getNumXDataTiles() * tileSize;
   }

   // Contain the unit further according to its radius to keep it within bounds
   float ownRadius = pUnit->getObstructionRadius();
   fMinX += ownRadius;
   fMaxX -= ownRadius;
   fMinZ += ownRadius;
   fMaxZ -= ownRadius;

   bool bMoveSquad = false;
   if (squadPos.x < fMinX)
   {
      squadPos.x = fMinX;
      bMoveSquad = true;
   }
   if (squadPos.x > fMaxX)
   {
      squadPos.x = fMaxX;
      bMoveSquad = true;
   }
   if (squadPos.z < fMinZ)
   {
      squadPos.z = fMinZ;
      bMoveSquad = true;
   }
   if (squadPos.z > fMaxZ)
   {
      squadPos.z = fMaxZ;
      bMoveSquad = true;
   }

   if (bMoveSquad)
   {
      pSquad->setPosition(squadPos);
      // Removing the squad from the platoon stops any oscillation of crowded aircraft commanded to the map edge.
      // BSR 9/30/08: Commenting this out because it makes squads on the edge of the map unresponsive and causes 
      // problems for pelican drop ships coming from off the map edge.
//      BPlatoon* pParentPlatoon = reinterpret_cast<BPlatoon*>(pSquad->getParent());
//      if (pParentPlatoon)
//         pParentPlatoon->removeChild(pSquad);
   }

   if (bMoveSquad || (!pSquadMoveAction && (ownPos.distance(squadPos) > pSquad->getLeashDistance())))
   {
      if (pUnit->getPhysicsObject())
         pUnit->getPhysicsObject()->forceActivate();
   }

   float closestRange = 100000.0f;
   uint airspotIndex=0;

   uint numSpots = pObjMan->getNumClaimedAirSpots();
   for (uint i=0; i < numSpots; i++)
   {
      BAirSpot* spot = pObjMan->getNextClaimedAirSpot(airspotIndex);
      airspotIndex++;
      if (!spot)
         continue;

      BASSERT(spot->mAircraftID);
      if (spot->mAircraftID == cInvalidObjectID)
         continue;

//-- FIXING PREFIX BUG ID 2192
      const BUnit* nearbyUnit = gWorld->getUnit(spot->mAircraftID);
//--
      if (!nearbyUnit)
         continue;
      if (nearbyUnit->getID() == pUnit->getID())
         continue;
      if (mpProtoAction->getStationary() && nearbyUnit->getProtoID() != pUnit->getProtoID())
      {
         // Everything dodges ground units.  Period.
         if (nearbyUnit->getFlagFlying() || nearbyUnit->isPhysicsAircraft())
            continue;
      }

      BVector nearbyUnitPos = nearbyUnit->getPhysicsPosition();
      nearbyUnitPos.y = 0.0f;
      ownPos.y = 0.0f;
#ifdef SYNC_UnitAction
      syncUnitActionData("BUnitActionAvoidCollisionAir::update - ownPos", ownPos);
#endif
      float range = ownPos.distance(nearbyUnitPos);
      float radiusSum = ownRadius + nearbyUnit->getObstructionRadius();
      if ((range - radiusSum) <= 0.0f)
      {
         mbAvoid = true;
         if (nearbyUnit->getTeamID() == pUnit->getTeamID())
            bAvoidFriendly = true;

         BVector repelVec = ownPos - nearbyUnitPos;
         repelVec.y = 0.0f;
         if (repelVec.lengthSquared() == 0.0f)
         {
            repelVec.x = getRandRangeFloat(cSimRand, -1.0f, 1.0f);
            repelVec.z = getRandRangeFloat(cSimRand, -1.0f, 1.0f);
         }
         repelVec.normalize();
         bool bUseRepelVec = true;

         if (nearbyUnit->getVelocity().lengthEstimate() > 1.0f)
         {
            BVector tgtVel = nearbyUnit->getVelocity();
            tgtVel.y = 0.0f;

            tgtVel.normalize();
            float closureFactor = -1.0f * tgtVel.dot(repelVec);

            if (closureFactor > 0.0f )
            {
               BVector upVector(0.0f, 1.0f, 0.0f);
               BVector motionNormal = tgtVel.cross(upVector);
               motionNormal.normalize();
               float detourMag = motionNormal.dot(repelVec);
               if (detourMag < 0.0f)
                  motionNormal *= -1.0f;
               mAvoidanceVec += motionNormal;
#ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - motionNormal", motionNormal);
#endif
               bUseRepelVec = false;
            }
         }
         if (bUseRepelVec)
         {
            mAvoidanceVec += 2.0f * repelVec * (radiusSum - range);
#ifdef SYNC_UnitAction
            syncUnitActionData("BUnitActionAvoidCollisionAir::update - repelVec", repelVec);
            syncUnitActionData("BUnitActionAvoidCollisionAir::update - radiusSum", radiusSum);
            syncUnitActionData("BUnitActionAvoidCollisionAir::update - range", range);
#endif
         }

         if (range < closestRange)
         {
            mNearestObstacleAlt = nearbyUnit->getPhysicsPosition().y;
            closestRange = range;
         }
      }
   }

   // Check whether target is too close
   bool bTooCloseToTarget = false;
   bool bCheckReturnToAnchor = true;
//-- FIXING PREFIX BUG ID 2194
   const BUnitActionRangedAttack* pUnitAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitRangedAttack));
//--
   if (pUnitAttackAction && pUnitAttackAction->getTarget()->isIDValid())
   {
      BEntity* pEntity=gWorld->getEntity(pUnitAttackAction->getTarget()->getID());
      if (pEntity)
      {
         BVector targetPosition=pEntity->getPosition();

         ownPos = pUnit->getPhysicsPosition();
#ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionAvoidCollisionAir::update - ownPos", ownPos);
#endif
         float xzDist = ownPos.xzDistance(targetPosition);
         if (xzDist < cFloatCompareEpsilon) // Don't risk divide by zero
            xzDist = cFloatCompareEpsilon;
         float yDist = fabs(ownPos.y - targetPosition.y);
         float tgtDepressionAngle = cDegreesPerRadian * atanf(yDist/xzDist);
         float maxTgtDepressionAngle = getProtoAction()->getMaxTgtDepressionAngle();

         if ((maxTgtDepressionAngle < 89.0f) && (tgtDepressionAngle > maxTgtDepressionAngle))
         {
            bTooCloseToTarget = true;
            BVector vecToTgt = targetPosition - ownPos;
            vecToTgt.y = 0.0f;
            vecToTgt.safeNormalize();
            float desiredXZdist = 1.2f * yDist / tan(maxTgtDepressionAngle * cRadiansPerDegree);
            if ((pUnit->getReverseSpeed() > 0.1f) || (pUnit->getReverseSpeed() == -1.0f))  // -1.0 means it is not set and therefore defaults to maxSpeed
            {
               mAvoidanceVec = -desiredXZdist * vecToTgt;
#ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - desiredXZdist", desiredXZdist);
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - vecToTgt", vecToTgt);
#endif
            }
            else
            {
               BVector upVector(0.0f, 1.0f, 0.0f);
               mAvoidanceVec = desiredXZdist * vecToTgt.cross(upVector);
#ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - desiredXZdist", desiredXZdist);
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - vecToTgt", vecToTgt);
#endif
            }
         }

         // If we have drifted away from our anchor point, see if the target is too close to the anchor for us to go back now
         float anchorToLeashDist = pSquad->getLeashPosition().distanceEstimate(pSquad->getAnchorPosition());
         if (anchorToLeashDist > (3.0f * ownRadius))
         {
            BVector anchorPos = pSquad->getAnchorPosition();
            float xzDist = anchorPos.xzDistance(targetPosition);
            if (xzDist < cFloatCompareEpsilon) // Don't risk divide by zero
               xzDist = cFloatCompareEpsilon;
            float yDist = fabs(ownPos.y - targetPosition.y);
            float tgtDepressionAngle = cDegreesPerRadian * atanf(yDist/xzDist);
            float maxTgtDepressionAngle = getProtoAction()->getMaxTgtDepressionAngle();

            if ((maxTgtDepressionAngle < 89.0f) && (tgtDepressionAngle > maxTgtDepressionAngle))
               bCheckReturnToAnchor = false;
         }

         // If not too close to the target, eliminate any component of avoidance vector that is away from the target
         if (!bTooCloseToTarget && mbAvoid)
         {
            BVector vecToTgt = targetPosition - ownPos;
            vecToTgt.y = 0.0f;
            vecToTgt.safeNormalize();
            BVector normAvoidVec = mAvoidanceVec;
            normAvoidVec.safeNormalize();

            float vecDotProd = vecToTgt.dot(normAvoidVec);
            if (vecDotProd < 0.0f) // There is a component away from the target
            {
               BVector vecComponentAway = vecToTgt * vecDotProd * mAvoidanceVec.length();
               mAvoidanceVec -= vecComponentAway;
#ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - vecComponentAway", vecComponentAway);
#endif
            }
         }
      }
   }

//-- FIXING PREFIX BUG ID 2195
//   const BSquadActionAttack* pSquadActionAttack = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
//--
   // If there is currently a squad attack action, leave the squad's position alone.
//   if ((mbCrashing || mbAvoid || bTooCloseToTarget) && !pSquadMoveAction && !pSquadActionAttack)
   if ((mbCrashing || mbAvoid || bTooCloseToTarget) && !pSquadMoveAction)
   {
      BVector desiredLocation = ownPos + mAvoidanceVec;

      if (pUnit->getPhysicsObject())
      {
         pUnit->getPhysicsObject()->forceActivate();

         if (bTooCloseToTarget || (mbAvoid && bAvoidFriendly))
         {
            #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - desiredLocation", desiredLocation);
            #endif

            // Make sure there are no "ObstructsAir" obstacles in the way and that the position is within map boundaries before setting the squad position
            long obstructionQuadTrees = BObstructionManager::cIsNewTypeBlockAirMovement;
            bool bObstructions = gObsManager.testObstructions(desiredLocation, ownRadius, 0.0f, obstructionQuadTrees, BObstructionManager::cObsNodeTypeAll, pUnit->getPlayerID());

            bool bOutOfBounds = false;
            if ((desiredLocation.x < fMinX) || (desiredLocation.x > fMaxX) || (desiredLocation.z < fMinZ) || (desiredLocation.z > fMaxZ))
               bOutOfBounds = true;

            if (!bObstructions && !bOutOfBounds)
            {
               pSquad->setPosition(desiredLocation);
               pSquad->setLeashPosition(desiredLocation, false); // Don't change the anchor position
            }
         }
      }
   }
   else if (bCheckReturnToAnchor)
   {
      // Check to see if we have dragged our leash away from the anchor position. If so, see if there is an open spot large enough for us to get back.
      float anchorToLeashDist = pSquad->getLeashPosition().distanceEstimate(pSquad->getAnchorPosition());
      float leashRange = pSquad->getLeashDistance();
      float leashDeadZone = pSquad->getLeashDeadzone();
      if (anchorToLeashDist > (leashRange + leashDeadZone))
      {
         // The following is compensating for the fact that we have to drag our leash with us in collision avoidance to prevent continual awkward oscillation of units.
         // Look at the anchor spot for an open spot big enough for our vehicle. Disregard enemy air vehicles, they will not keep us from obeying our leash.

         // [brad] I was planning to make a widening search, but it seems at this point to be unnecessary

         // DISREGARD THE FOLLOWING ABOUT AN ITERATIVE SEARCH - IT SEEMS TO WORK WELL WITHOUT IT FOR NOW
         // Iterate on successive frames, shifting the search center or widening the search on each new attempt until we find an open spot or we have searched to our current distance.
         // The search pattern forms a hex-grid search centered around the anchor position.
         // By toggling a base offset on and off between successive searches, we can detect spaces between adjacent "barely blocked" spaces that failed on the baseline test.
         
         bool bBlocked = false;
         BVector testPos;
         BVector baseOffset;
         testPos = pSquad->getAnchorPosition();
         baseOffset.zero();
/*
         if (mbOffsetAnchorSearch)
            baseOffset.x = 1.1547f * ownRadius; // offset the hex by 2r/sqrt(3)
         mbOffsetAnchorSearch = !mbOffsetAnchorSearch; // toggle in preparation for the next pass

         testPos += baseOffset;
*/
         uint airspotIndex=0;
         for (uint i=0; i < numSpots; i++) // Check for obstacles in this test position
         {
            BAirSpot* spot = pObjMan->getNextClaimedAirSpot(airspotIndex);
            airspotIndex++;
            if (!spot)
               continue;

            BUnit* nearbyUnit = gWorld->getUnit(spot->mAircraftID);
            if (!nearbyUnit)
               continue;
            if (nearbyUnit->getID() == pUnit->getID())
               continue;

            if (nearbyUnit->getTeamID() != pUnit->getTeamID()) // This is for XZ positioning only - do not let enemies block us
               continue;

            if (spot->mClaimedPos.distance(testPos) < (ownRadius + nearbyUnit->getObstructionRadius()))
            {
               bBlocked = true;
               break; // This space is blocked, try another
            }
         }

         if (!bBlocked)
         {
            #ifdef SYNC_UnitAction
               syncUnitActionData("BUnitActionAvoidCollisionAir::update - testPos", testPos);
            #endif

            pUnit->getPhysicsObject()->forceActivate();
            pSquad->setPosition(testPos);
            pSquad->setLeashPosition(testPos, false); // Don't change the anchor position

            if (mClaimedAirSpotIndex != cMaxAirSpotIndex)
            {
               // Update my claimed spot as the one I am taking now
               BAirSpot* myNewSpot = pObjMan->getClaimedAirSpot(mClaimedAirSpotIndex);
               if (myNewSpot)
                  myNewSpot->mClaimedPos = testPos;
            }
         }
      }
   }

   if (mpProtoAction && !mpProtoAction->getFlagAvoidOnly())
   {
      // Note: if this changes, update the code at the top of the method for ground vehicles
      if (!pUnit->getPhysicsObject()->isActive())
      {
         if (mClaimedAirSpotIndex != cMaxAirSpotIndex)
         {
            // Update my claimed spot as the one I am in now
            BAirSpot* myNewSpot = pObjMan->getClaimedAirSpot(mClaimedAirSpotIndex);
            if (myNewSpot)
               myNewSpot->mClaimedPos = ownPos;
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAvoidCollisionAir::findKamikazeTarget(BUnit* killer)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if (!mbCanKamikaze)
      return (false);

   if (mbCrashing)
      return true;

   mbCrashing = true;

   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   BSelectionManager* pSelectionManager = (pUser) ? pUser->getSelectionManager() : NULL;
   if(pSelectionManager && pSelectionManager->isUnitSelected(pUnit->getID()))
      pSelectionManager->unselectUnit(pUnit->getID());
   pUnit->setFlagSelectable(false);

   if (killer) // Record who should get credit for killing us when we die
   {
      mKillerUnit = killer->getID();
      mKillerPlayer = killer->getPlayerID();
      mKillerTeam = killer->getTeamID();
   }

   //Start the death sound (plays crashing noise)
   long cueIndex=pUnit->getProtoObject()->getSound(cObjectSoundDeath);
   if(cueIndex!=cInvalidCueIndex)
      gWorld->getWorldSoundManager()->addSound(pUnit, -1, cueIndex, true, cInvalidCueIndex, true, true);

//-- FIXING PREFIX BUG ID 2200
   const BTactic* pTactic = pUnit->getTactic();
//--
   if (!pTactic)
      return (false);

   BVector fwdVec = pUnit->getForward();
   BVector ownPos = pUnit->getPosition();

   BUnitActionStasis* pUnitStasisAction = reinterpret_cast<BUnitActionStasis*>(pUnit->getActionByType(BAction::cActionTypeUnitStasis));

   const BWeapon* pWeapon = pTactic->getWeapon(mpProtoAction->getWeaponID());
   if (pWeapon && !pUnitStasisAction)
   {

      BUnit* pTargetUnit = NULL;
      BVector vecToTgt;
      BVector tgtPos;
      BUnit* bestTgt = NULL;
      float tgtDot = 0.0f;
      float bestTgtDot = 0.0f;

      BUnitQuery query(ownPos, pWeapon->mMaxRange, false);            
      query.setFlagIgnoreDead(true);
      query.setUnitVisibility(pUnit->getPlayerID());
      query.setRelation(pUnit->getPlayerID(), cRelationTypeEnemy);
      BEntityIDArray results(0, 100);
      long numResults = gWorld->getUnitsInArea(&query, &results);
      if (numResults > 0)
      {
         for (long i = 0; i < numResults; ++i)
         {
            pTargetUnit = gWorld->getUnit(results[i]);
            if (pTargetUnit->isType(gDatabase.getObjectType("Flying")))
               continue;

            // don't try and kamikaze things attached to us - that's just silly
            if (pTargetUnit->getAttachedToObject() == pUnit)
               continue;

            tgtPos = pTargetUnit->getPosition();
            vecToTgt = tgtPos - ownPos;
            vecToTgt.safeNormalize();
            tgtDot = fwdVec.dot(vecToTgt);

            if (tgtDot > bestTgtDot) // If this target is closer to the nose than the previous best choice, choose it instead.
            {
               bestTgtDot = tgtDot;
               bestTgt = pTargetUnit;
            }
         }
      }

      mKamikazeTarget = (bestTgt ? bestTgt->getID() : cInvalidObjectID);
      if (bestTgt)
      {
         bestTgt->addAttackingUnit(pUnit->getID(), getID());
         mCrashPos = bestTgt->getPosition();

         if (pUnit->hasAnimation(cAnimTypeKamikaze))
         {
            pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());
            pUnit->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeKamikaze, true);
         }
         return (true);
      }
   }

   BVector rightVec = pUnit->getRight();

   float crashDistFwd = getRandRangeFloat(cSimRand, 0.0f, 40.0f);
   float crashDistRight = getRandRangeFloat(cSimRand, -40.0f, 40.0f);
   mCrashPos = ownPos + (fwdVec * crashDistFwd) + (rightVec * crashDistRight);
   gTerrainSimRep.getHeight(mCrashPos, true);
   mCrashPos.y -= 20.0f;

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionAvoidCollisionAir::crashDetonate(BEntityID unitHit)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   if(mpProtoAction)
   {
      float damage = mpProtoAction->getDamagePerSecond();
      if(mpProtoAction->getAOERadius() > cFloatCompareEpsilon)
      {      
         BEntityIDArray unitsKilled;
         BVector damagePosition = pUnit->getPosition();
         BDamageHelper::doAreaEffectDamage(pUnit->getPlayerID(), pUnit->getTeamID(), pUnit->getID(), (IDamageInfo*)mpProtoAction, damage, damagePosition, pUnit->getID(), pUnit->getVelocity(), &unitsKilled, unitHit);         
      }

      // Give credit to the player who did final damage to owner and throw final debris in the explosion
      pUnit->setFlagDiesAtZeroHP(true);

      BDamageHelper::doDamageWithWeaponType(mKillerPlayer, mKillerTeam, pUnit->getID(), (IDamageInfo*)mpProtoAction, 10.0f, ((IDamageInfo*)mpProtoAction)->getWeaponType(), true, pUnit->getVelocity(), 1.0f, mPrevPos, mKillerUnit, mKillerUnit);
      if (pUnit->getKilledByID() == cInvalidObjectID)
         pUnit->setKilledByID(mKillerUnit);

   }

   createImpactEffect();
   pUnit->kill(true);
}

//==============================================================================
// BUnitActionAvoidCollisionAir::createImpactEffect
//==============================================================================
void BUnitActionAvoidCollisionAir::createImpactEffect(void)
{
//-- FIXING PREFIX BUG ID 2201
   const BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
//--
   if (!pUnit)
      return;

   BObjectCreateParms parms;
   parms.mPlayerID = pUnit->getPlayerID();
   parms.mPosition = pUnit->getPosition();
   parms.mRight = cXAxisVector;
   parms.mForward = cZAxisVector;

   bool specialDamageControllingEffect = false;
   BDamageAreaOverTimeInstance* pDAOTInst = NULL;

   byte surfaceImpactType = gTerrainSimRep.getTileType(pUnit->getPosition());

   //-- Create the visual impact effects
   if (mpProtoAction)
   {
      int impactProtoID = mpProtoAction->getImpactEffectProtoID();
      if (impactProtoID != -1)
      {
         // Create shockwave damage associated with impact effect
         if (pUnit->getProtoObject() && pUnit->getProtoObject()->getTactic())
         {
            BProtoAction* pShockwaveAction = pUnit->getProtoObject()->getTactic()->getShockwaveAction();
            if (mpProtoAction->getDoShockwaveAction() && pShockwaveAction)
            {
               // Get new instance
               pDAOTInst = BDamageAreaOverTimeInstance::getInstance();
               if (pDAOTInst)
               {
                  // Init
                  bool result = pDAOTInst->init(gWorld->getGametime(), pShockwaveAction , pUnit->getPlayerID(), pUnit->getTeamID(), cInvalidObjectID);

                  // If init'd successfully, setup callback data for impact effect
                  if (result)
                  {
                     specialDamageControllingEffect = true;
                  }
                  // otherwise, release the bad instance
                  else
                  {
                     BDamageAreaOverTimeInstance::releaseInstance(pDAOTInst);
                  }
               }
            }
         }

         if( !specialDamageControllingEffect )
         {
            //If we aren't a special impact effect that deals damage, return if we shouldn't be seen.
            BTeamID teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();
            if( pUnit->isVisible(teamID) == false )
               return;
         }

         // Create impact effect
         const BProtoImpactEffect* pImpactData = gDatabase.getProtoImpactEffectFromIndex(impactProtoID);
         BASSERT(pImpactData);

         BTerrainEffect* pTerrainEffect = gTerrainEffectManager.getTerrainEffect(pImpactData->mTerrainEffectIndex, true);
         if (pTerrainEffect)
         {
            BObject* pImpactObject = NULL;
            pTerrainEffect->instantiateEffect(surfaceImpactType, mpProtoAction->getImpactEffectSize(), parms.mPosition, parms.mForward, true, parms.mPlayerID, pImpactData->mLifespan, pUnit->getFlagVisibleToAll(), cVisualDisplayPriorityCombat, &pImpactObject);
            if (specialDamageControllingEffect && pImpactObject && pDAOTInst)
            {
               pDAOTInst->setObjectID(pImpactObject->getID());
            }
         }


         // Create surface impact effect
         if (surfaceImpactType != -1 )
         {
            const BProtoImpactEffect* pImpactData = gDatabase.getProtoImpactEffectFromIndex(impactProtoID);
            BTerrainEffect* pTerrainEffect = gTerrainEffectManager.getTerrainEffect(gDatabase.getSurfaceTypeImpactEffect(), true);
            if (pTerrainEffect && pImpactData)
            {
               pTerrainEffect->instantiateEffect(surfaceImpactType, mpProtoAction->getImpactEffectSize(), parms.mPosition, parms.mForward, true, parms.mPlayerID, pImpactData->mLifespan, pUnit->getFlagVisibleToAll(), cVisualDisplayPriorityNormal, NULL);
            }
         }
      }
   }

   //-- Play our impact sound 
   BCueIndex cueIndex = cInvalidCueIndex;
   const BProtoObject* pProto = pUnit->getProtoObject();
   if(pProto)
      cueIndex = pProto->getSound(cObjectSoundImpactDeath);
   if(cueIndex != cInvalidCueIndex)
      gWorld->getWorldSoundManager()->addSound(pUnit->getPosition(), cueIndex, true, cInvalidCueIndex, true, true);

   //-- Play our impact sound
   //if (surfaceImpactType != -1 )
   //{
   //   BImpactSoundInfo soundInfo;
   //   bool result = pUnit->getProtoObject()->getImpactSoundCue(surfaceImpactType, soundInfo);
   //   if (result && (soundInfo.mSoundCue != cInvalidCueIndex))
   //      gWorld->getWorldSoundManager()->addSound(pUnit->getPosition(), soundInfo.mSoundCue, false, cInvalidCueIndex, soundInfo.mCheckSoundRadius, true);            
   //}

   //-- Rumble controller and shake camera
   if (mpProtoAction)
      mpProtoAction->doImpactRumbleAndCameraShake(BRumbleEvent::cTypeImpact, parms.mPosition, false, cInvalidObjectID);
}

//==============================================================================
//==============================================================================
BUnit* BUnitActionAvoidCollisionAir::getKamikazeTarget()
{ 
   return gWorld->getUnit(mKamikazeTarget);
}

//==============================================================================
//==============================================================================
bool BUnitActionAvoidCollisionAir::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVECTOR(pStream, mAvoidanceVec);
   GFWRITEVECTOR(pStream, mPrevPos);
   GFWRITEVECTOR(pStream, mCrashPos);
   GFWRITEVAL(pStream, BEntityID, mKamikazeTarget);
   GFWRITEVAR(pStream, BEntityID, mKillerUnit);
   GFWRITEVAR(pStream, BPlayerID, mKillerPlayer);
   GFWRITEVAR(pStream, BTeamID, mKillerTeam);
   GFWRITEVAR(pStream, float, mNearestObstacleAlt);
   GFWRITEVAR(pStream, float, mMaxTgtDepressionAngle);
   GFWRITEVAR(pStream, float, mBirthTimer);
   GFWRITEVAR(pStream, uint, mClaimedAirSpotIndex);
   GFWRITEVAR(pStream, uint, mAnchorSearchPass);
   GFWRITEVAR(pStream, DWORD, mDetonateTime);
   GFWRITEBITBOOL(pStream, mbAvoid);
   GFWRITEBITBOOL(pStream, mbOffsetAnchorSearch);
   GFWRITEBITBOOL(pStream, mbCrashing);
   GFWRITEBITBOOL(pStream, mbLimitSpeed);
   GFWRITEBITBOOL(pStream, mbCanKamikaze);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionAvoidCollisionAir::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVECTOR(pStream, mAvoidanceVec);
   GFREADVECTOR(pStream, mPrevPos);
   GFREADVECTOR(pStream, mCrashPos);
   GFREADVAR(pStream, BEntityID, mKamikazeTarget);
   GFREADVAR(pStream, BEntityID, mKillerUnit);
   GFREADVAR(pStream, BPlayerID, mKillerPlayer);
   GFREADVAR(pStream, BTeamID, mKillerTeam);
   GFREADVAR(pStream, float, mNearestObstacleAlt);
   GFREADVAR(pStream, float, mMaxTgtDepressionAngle);
   GFREADVAR(pStream, float, mBirthTimer);
   GFREADVAR(pStream, uint, mClaimedAirSpotIndex);
   GFREADVAR(pStream, uint, mAnchorSearchPass);
   GFREADVAR(pStream, DWORD, mDetonateTime);
   GFREADBITBOOL(pStream, mbAvoid);
   GFREADBITBOOL(pStream, mbOffsetAnchorSearch);
   GFREADBITBOOL(pStream, mbCrashing);
   GFREADBITBOOL(pStream, mbLimitSpeed);
   GFREADBITBOOL(pStream, mbCanKamikaze);

   return true;
}
