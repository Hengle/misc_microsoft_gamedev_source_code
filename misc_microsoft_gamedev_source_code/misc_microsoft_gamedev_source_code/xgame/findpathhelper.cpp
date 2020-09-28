//==============================================================================
// findpathhelper.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "ConfigsGame.h"
#include "Platoon.h"
#include "database.h"
#include "EntityScheduler.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "pather.h"
#include "pathingLimiter.h"
#include "obstructionmanager.h"
#include "findpathhelper.h"


const uint cProjIndexBackMiddle  = 5;
const uint cProjIndexBackLeft    = 0;
const uint cProjIndexBackRight   = 4;
const uint cProjIndexFrontLeft   = 1;
const uint cProjIndexFrontRight  = 3;
const uint cProjIndexFrontMiddle = 2;

const float cEmergencyStopBufferFactor=1.1f;
const float cProjectionTime=2.0f;
const DWORD cPathingRetryDelayTime = 500;
const uchar cMaxNumPathingRetries = 1;
const DWORD cProjectionSiblingUpdateTime = 500;
const DWORD cDefaultMovementPauseTime = 500;

// Minimum number of path segments that should be generated at any time
const uint cMinimumPathSegmentsNeeded = 3;

// Maximum distance between pathing hint waypoints
const float cMaximumReducedWaypointDistance = 60.0f;


//==============================================================================
//==============================================================================
BFindPathHelper::BFindPathHelper()
{
   mTarget.reset();
   mpEntity = NULL;
   mPathingRadius = 0.0f;
   mFlagPathableAsFlyingUnit = false;
   mFlagPathableAsFloodUnit = false;
   mFlagPathableAsScarabUnit = false;
   mFlagReducePath = false;
   mFlagPathAroundSquads = false;
   mFlagSkipLRP = false;
}

//==============================================================================
//==============================================================================
void BFindPathHelper::setTarget(const BSimTarget* target)
{
   if (target != NULL)
      mTarget = *target;
   else
      mTarget.reset();
}

//==============================================================================
//==============================================================================
void BFindPathHelper::setEntity(BEntity* pEntity, bool ignorePlatoonMates)
{
   mpEntity = pEntity;

   mIgnoreUnits.clear();
   if (mpEntity != NULL)
   {
      // Get the squad's platoon so all the platoon's units can be ignored
      BEntity* pRealEntity = mpEntity;
      if ((mpEntity->getClassType() == BEntity::cClassTypeSquad) && ignorePlatoonMates)
      {
         BSquad* pSquad = reinterpret_cast<BSquad*>(mpEntity);
         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon != NULL)
            pRealEntity = pPlatoon;

         // Add hitched unit and squad
         if (pSquad->hasHitchedSquad())
         {
            mIgnoreUnits.add(pSquad->getHitchedUnit());
            mIgnoreUnits.add(pSquad->getHitchedSquad());
         }
      }

      if (pRealEntity->getClassType() == BEntity::cClassTypeSquad)
      {
         BSquad* pSquad = reinterpret_cast<BSquad*>(pRealEntity);         
         uint numChildren = pSquad->getNumberChildren();
         for (uint index = 0; index < numChildren; index++)
         {
            mIgnoreUnits.add(pSquad->getChild(index));
         }
         mIgnoreUnits.add(pSquad->getID());         
      }
      else if (pRealEntity->getClassType() == BEntity::cClassTypePlatoon)
      {
         BPlatoon* pPlatoon = reinterpret_cast<BPlatoon*>(pRealEntity);
         uint squadIndex;
         for (squadIndex = 0; squadIndex < pPlatoon->getNumberChildren(); squadIndex++)
         {
            BSquad* pSquad = gWorld->getSquad(pPlatoon->getChild(squadIndex));
            if (pSquad != NULL)
            {               
               uint numChildren = pSquad->getNumberChildren();
               for (uint unitIndex = 0; unitIndex < numChildren; unitIndex++)
               {
                  mIgnoreUnits.add(pSquad->getChild(unitIndex));
               }
               mIgnoreUnits.add(pSquad->getID());
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
uint BFindPathHelper::findPath(BPath& path, const BDynamicSimVectorArray &waypoints, const BVector* overrideStartPosition, long targetID, bool useTargetRange)
{
   BASSERT(mpEntity);

   path.reset();

   // Get starting position
   BVector startWaypoint = mpEntity->getPosition();
   if (overrideStartPosition != NULL)
      startWaypoint = *overrideStartPosition;

   for (uint wpIndex = 0; wpIndex < waypoints.getSize(); wpIndex++)
   {
      // Get a path
      static BPath tempPath;
      tempPath.reset();
      uint result = findPath(tempPath, startWaypoint, waypoints[wpIndex], false, targetID, useTargetRange);
      if (result == BPath::cFailed)
         return (BPath::cFailed);

      // Append the path to the end of the current path
      if (tempPath.getNumberWaypoints() > 0)
      {
         if (!appendPath(tempPath, path))
            return (BPath::cFailed);
      }

      // Set starting point for next path
      startWaypoint = waypoints[wpIndex];
   }
   
   path.setCreationTime(gWorld->getGametime());

   return (BPath::cFull);
}

//==============================================================================
//==============================================================================
uint BFindPathHelper::findPath(BPath& path, BVector pos1, BVector pos2, bool failOnPathing, long targetID, bool useTargetRange)
{
   //SCOPEDSAMPLE(BFindPathHelper_findPath)
   // Range
   float range = 0.0f;
   // Use the target range if specified.  In general squad paths that want to get to a specific point won't
   // want to set the targetID or the range.  Platoon paths that are just trying to get to an attack target
   // will pass in a targetID and range
   if (useTargetRange && mTarget.isRangeValid())
      range += mTarget.getRange();

   // Determine whether this should path as land or flying unit
   int pathClass = BPather::cLandPath;
   if (mFlagPathAroundSquads)
      pathClass = BPather::cSquadLandPath;
   if (mFlagPathableAsFloodUnit)
      pathClass = BPather::cFloodPath;
   else if (mFlagPathableAsScarabUnit)
      pathClass = BPather::cScarabPath;
   else if (mFlagPathableAsFlyingUnit)
      pathClass = BPather::cAirPath;

   // Find a path.
   // Ideally this would use the largest unit's radius as the radius here, but just use
   // a constant that ensures the first bucket is used.  This used to use the largest
   // squad radius, but that was causing the findPath to fail because the squad was large
   // and it was using the largest bucket.
   updatePathingLimits(true);

   // Long range path
   long result = BPath::cFull;
   if (mFlagSkipLRP)
   {
      // This skipping of LRP is a hack for ramming moves
      path.addWaypointAtEnd(pos1);
      path.addWaypointAtEnd(pos2);
   }
   else
   {
      bool canJump = mpEntity ? mpEntity->canJump() : false;

      result = gPather.findPath(&gObsManager, pos1, pos2, mPathingRadius, range,
         NULL, &path, false, true, canJump, targetID, BPather::cLongRange, pathClass/*BPather::cAirPath*/);
   }

   path.setCreationTime(gWorld->getGametime());

   //If we're in range at the state, we're done.
   if (result == BPath::cInRangeAtStart)
   {
      // If within distance of end position then discard the path it found
      path.reset();
      return (BPath::cFull);
   }

   // If there are obstructions between the high level waypoints run a short range path and insert
   // it in the high level path to ensure the path doesn't run through obstructions.
//   if ((pathClass != BPather::cAirPath) && (result != BPath::cFailed) && (mpEntity->getClassType() != BEntity::cClassTypePlatoon))
   if ((result != BPath::cFailed) && (mpEntity->getClassType() != BEntity::cClassTypePlatoon))
   {
      uint fixupPathResult = fixupPathAroundObstructions(path, mPathingRadius, range, pathClass);

      if (fixupPathResult == BPath::cInRangeAtStart)
      {
         // If within distance of end position then discard the path it found
         path.reset();
         return (BPath::cFull);
      }

      // If fail conditions being detected, report back if path can't be found or partial path was found.
      // Otherwise return success (full path was found).
      if (failOnPathing)
      {
         if (fixupPathResult == BPath::cFailed)
         {
            path.reset();
            return (BPath::cFailed);
         }
         else if (fixupPathResult == BPath::cPartial)
         {
            return (BPath::cPartial);
         }
      }
   }


   //If that failed, bail now.
   if ((result == BPath::cFailed) || (path.getNumberWaypoints() <= 0))
   {
      path.reset();
      return (BPath::cFailed);
   }

   return (BPath::cFull);
}

//==============================================================================
//==============================================================================
bool BFindPathHelper::appendPath(BPath& sourcePath, BPath& destinationPath)
{
   // Remove duplicate waypoint at the beginning of new path
   if ((destinationPath.getNumberWaypoints() > 0) && (sourcePath.getNumberWaypoints() > 0))
   {
      if (destinationPath.getWaypoint(destinationPath.getNumberWaypoints() - 1).almostEqualXZ(sourcePath.getWaypoint(0)))
         sourcePath.removeWaypoint(0);
   }

   // Append the new path at the end of the old path
   if (!destinationPath.addWaypointsAtEnd(sourcePath.getWaypointList()))
      return false;

   destinationPath.setCreationTime(Math::Max(destinationPath.getCreationTime(), sourcePath.getCreationTime()));

   return true;
}

//==============================================================================
//==============================================================================
bool BFindPathHelper::reducePath(BPath& path)
{
   const BDynamicVectorArray& oldWaypoints = path.getWaypointList();
   if (oldWaypoints.getSize() <= 2)
      return true;

   // Add first waypoint to list
   long currentWaypoint = 1;
   BDynamicSimVectorArray newWaypoints;
   newWaypoints.add(oldWaypoints[0]);
   BVector currentPos(oldWaypoints[0]);

   // Traverse original path and produce spaced out waypoints
   float totalDistance = path.calculatePathLengthConst(true);
   while ((totalDistance > cFloatCompareEpsilon) && (currentWaypoint < oldWaypoints.getNumber()))
   {
      BVector nextPos;
      float realDistance;
      long newNextWaypoint;

      // End of path
      if (totalDistance < cMaximumReducedWaypointDistance)
      {
         break;
      }
      // Less than two waypoints' distance remaining
      else if (totalDistance < (2.0f * cMaximumReducedWaypointDistance))
      {
         // Find waypoint half way between current position and end
         path.calculatePointAlongPath(totalDistance * 0.5f, currentPos, currentWaypoint, nextPos, realDistance, newNextWaypoint);

         newWaypoints.add(nextPos);
         break;
      }

      // Find next position a fixed distance along path
      if (!path.calculatePointAlongPath(cMaximumReducedWaypointDistance, currentPos, currentWaypoint, nextPos, realDistance, newNextWaypoint) || (realDistance < cFloatCompareEpsilon))
         break;
      newWaypoints.add(nextPos);

      currentPos = nextPos;
      currentWaypoint = newNextWaypoint;
      totalDistance -= realDistance;
   }

   // Add final waypoint
   newWaypoints.add(oldWaypoints[oldWaypoints.getSize() - 1]);

   // Copy waypoints to the path
   path.setWaypoints(newWaypoints.getData(), newWaypoints.getSize());

   return true;
}

//==============================================================================
//==============================================================================
uint BFindPathHelper::fixupPathAroundObstructions(BPath& path, float radius, float range, int pathClass)
{
   SCOPEDSAMPLE(BFindPathHelper_fixupPathAroundObstructions)
   BASSERT(mpEntity);

   bool needBackPath = false;
   bool pathingFailed = false;
   static BDynamicSimVectorArray newWaypoints;
   newWaypoints.clear();

   bool breakAfterProcessing = false;
   int hlWaypoint;
   bool inRangeAtStart = false;
   for (hlWaypoint = 0; hlWaypoint < (path.getNumberWaypoints() - 1); hlWaypoint++)
   {
      BVector wp1 = path.getWaypoint(hlWaypoint);
      if (newWaypoints.getSize() > 0)
         wp1 = newWaypoints[newWaypoints.getSize() - 1];
      BVector wp2 = path.getWaypoint(hlWaypoint + 1);

      BVector lineCollision;
      if( testForOneWayBarrierLineCross(wp1, wp2, lineCollision) )
      {
         wp2 = lineCollision;
         breakAfterProcessing = true;
      }

      BSimCollisionResult collisionResult = checkForCollisions(wp1, wp2, radius, 0);
      if (collisionResult != cSimCollisionNone)
      {
         // Find a short range path
         updatePathingLimits(false);

         static BPath shortRangePath;
         shortRangePath.reset();
         int shortRangeResult = gPather.findPath(mpEntity->getID().asLong(), mpEntity->getClassType(), &gObsManager, wp1, wp2,
            radius, range, mIgnoreUnits, &shortRangePath, false, false, mpEntity->canJump(), -1L, BPather::cShortRange, pathClass);
         if (shortRangeResult == BPath::cFailed)
         {
            pathingFailed = true;
            break;
         }
         // DLM If we're already in range, then return this to my caller.
         if (shortRangeResult == BPath::cInRangeAtStart)
         {
            inRangeAtStart = true;
            continue;
         }
         inRangeAtStart = false;
         if (shortRangePath.getNumberWaypoints() > 0)
         {
            needBackPath = true;

            // Remove original waypoints
            if (newWaypoints.getSize() > 0)
               newWaypoints.removeIndex(newWaypoints.getSize() - 1);

            // Add waypoints from new path
            for (int srWaypoint = 0; srWaypoint < shortRangePath.getNumberWaypoints(); srWaypoint++)
            {
               newWaypoints.add(shortRangePath.getWaypoint(srWaypoint));
            }
         }
         else
         {
            // Add waypoints
            if (newWaypoints.getSize() == 0)
               newWaypoints.add(wp1);
            newWaypoints.add(wp2);
         }
      }
      else
      {
         // Add waypoints
         if (newWaypoints.getSize() == 0)
            newWaypoints.add(wp1);
         newWaypoints.add(wp2);
      }

      if (breakAfterProcessing)
         break;
   }

   path.zeroWaypoints();

   // If we returned inRangeAtStart, then we should just be done.
   if (inRangeAtStart)
      return BPath::cInRangeAtStart;

   // Copy new waypoints into path
   for (uint i = 0; i < newWaypoints.getSize(); i++)
   {
      path.addWaypointAtEnd(newWaypoints[i]);
   }

   // Reduce the path to the minimal path needed for pathing hints
   if (mFlagReducePath)
   {
      reducePath(path);
   }
   // Remove unnecessary waypoints
   else if (needBackPath)
   {
      backPath(path, radius, true);
   }

   // If pathing failed, then return partial if some pathing succeeded.  Otherwise return failure.
   if (pathingFailed)
   {
      if (path.getNumberWaypoints() > 0)
         return (BPath::cPartial);
      else
         return (BPath::cFailed);
   }

   return (BPath::cFull);
}

//==============================================================================
//==============================================================================
void BFindPathHelper::backPath(BPath& path, float radius, bool generateWaypoints)
{
   // This is basically copied from BLrpTree::backPath()

   BVector vAnchor;
   BVector vTesting;
   BVector vRemoval;
   BVector vMarker(-1.0, -1.0, -1.0);
   int lAnchor = 0;
   int lNextAnchor = 1;
   int lTesting = 2;
   int lNumberWaypoints = path.getNumberWaypoints();

   bool bDone = false;
   while (!bDone)
   {
      vAnchor = path.getWaypoint(lAnchor);

      // Duplicate waypoint check
      if (((lAnchor + 1) < lNumberWaypoints) && (vAnchor.almostEqualXZ(path.getWaypoint(lAnchor + 1))))
      {
         path.setWaypoint(lAnchor + 1, vMarker);
      }

      for (lTesting = lAnchor + 2; lTesting < lNumberWaypoints; lTesting++)
      {
         if (vAnchor.x < 0.0f || vAnchor.z < 0.0f)
            break;
         vTesting = path.getWaypoint(lTesting);
         if (vTesting.x < 0.0f || vTesting.z < 0.0f)
            continue;
         BSimCollisionResult collisionResult = checkForCollisions(vAnchor, vTesting, radius, 0);
         if (collisionResult == cSimCollisionNone)
         {
            path.setWaypoint(lTesting-1, vMarker);
            lNextAnchor = lTesting;
         }
         else
         {
            // Don't worry about trying to skip other waypoints once one is found that must be kept
            break;
         }
      }
      lAnchor = lNextAnchor;
      lNextAnchor++;
      if (lNextAnchor >= lNumberWaypoints)
         bDone = true;
   }
   for (int n = 0; n < path.getNumberWaypoints(); n++)
   {
      vTesting = path.getWaypoint(n);
      if (vTesting.x < 0.0f || vTesting.z < 0.0f)
      {
         path.removeWaypoint(n);
         n--;
      }
   }

   if (generateWaypoints)
   {
      // Icky code to make sure that the distance between waypoints is small enough
      // to make everything copasetic with the low-level pather..
      BVector vCurrent = path.getWaypoint(0);
      for (int i = 1; i < path.getNumberWaypoints(); i++)
      {
         BVector vNext = path.getWaypoint(i);
         if (vCurrent.xzDistanceSqr(vNext) > cMaximumLowLevelDistSqr - cLowLevelRelaxFactorSqr)
         {
            // Project a point along this vector and insert a waypoint here..
            BVector vDir = vNext - vCurrent;
            vDir.y = 0.0f;
            vDir.normalize();
            // Use a fudge factor just to make sure we stay under the limit.. dlm 2/21/01
            BVector vNew = vCurrent + (vDir * (cMaximumLowLevelDist - (cLowLevelRelaxFactor + 1.0f)));
            path.addWaypointBefore(i, vNew);
            vCurrent = vNew;            
         }
         else
            vCurrent = vNext;
      }
   }
}

//==============================================================================
//==============================================================================
BSimCollisionResult BFindPathHelper::checkForCollisions(BVector pos1, BVector pos2, float radius, DWORD lastPathTime)
{
   static BObstructionNodePtrArray collisionObs;
   collisionObs.setNumber(0);
   BVector intersectionPoint(0.0f);

   //Actually do the collision check.
   long lObOptions=
      BObstructionManager::cIsNewTypeBlockLandUnits |
      BObstructionManager::cIsNewTypeAllCollidableUnits |
      BObstructionManager::cIsNewTypeAllCollidableSquads;
   long lObNodeType = BObstructionManager::cObsNodeTypeAll;

   bool canJump = mpEntity ? mpEntity->canJump() : false;

   // Do the obstruction check
   gObsManager.begin(BObstructionManager::cBeginEntity, radius, lObOptions, lObNodeType, 0, cDefaultRadiusSofteningFactor, &mIgnoreUnits, canJump);
   gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, pos1, pos2, 
      true, intersectionPoint, collisionObs);
   gObsManager.end();

   //Get the target entity.
//   BEntity* pTarget=gWorld->getEntity(mTarget.getID());

   BSimCollisionResult result = cSimCollisionNone;

   // Check the type of collisions returned
   uint obIndex;
   uint obCount = collisionObs.getNumber();
   for (obIndex = 0; obIndex < obCount; obIndex++)
   {
      BOPObstructionNode* pObstructionNode = collisionObs[obIndex];
      if ((pObstructionNode == NULL) || (pObstructionNode->mObject == mpEntity))
         continue;
      BEntity* pObject = pObstructionNode->mObject;
      if (pObject == NULL)
      {
         if (result == cSimCollisionNone)
            result = cSimCollisionTerrain;
         continue;
      }

      if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeEdgeofMap)
      {
         if (result == cSimCollisionNone)
            result = cSimCollisionEdgeOfMap;
         continue;
      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeTerrain)
      {
         if (result == cSimCollisionNone)
            result = cSimCollisionTerrain;
         continue;
      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeUnit)
      {
//-- FIXING PREFIX BUG ID 1483
         const BUnit* pCollisionUnit = pObject->getUnit();
//--
         if (pCollisionUnit == NULL)
            continue;

         // If unit hasn't moved since path was made, ignore this unit collision
         if (pCollisionUnit->getLastMoveTime() < lastPathTime)
            continue;

         //Ignore collisions with the target specifically handling squad and unit
         //target cases.
         // TRB (1/29/2007): Try disabling this for now.  Units would sometimes path inside other units
         // because this wouldn't detect that the long range path intersected with the target.  Also
         // backPath would also remove short range path waypoints that pathed around the target.
         /* if (pTarget)
         {
            if (pTarget->getClassType() == BEntity::cClassTypeSquad)
            {
               BSquad* pSquad=reinterpret_cast<BSquad*>(pTarget);
               if (pSquad->containsChild(pCollisionUnit->getID()))
                  continue;
            }
            else if ((pTarget->getClassType() == BEntity::cClassTypeUnit) && (pTarget->getID() == pCollisionUnit->getID()))
               continue;
            //else
            //   continue;
         } */

         // Only check buildings and non-moving units.
         if (isCollisionEnabledWithEntity(pObject))
         {
            if (result == cSimCollisionNone)
               result = cSimCollisionUnit;
            continue;
         }
         else
         {
            // Add unit to ignore list if we don't want to collide with it
            mIgnoreUnits.add(pObject->getID());
         }
      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeSquad)
      {
         // Return squad collisions if pathing around squads
         if (mFlagPathAroundSquads)
         {
            // If unit hasn't moved since path was made, ignore this unit collision
//-- FIXING PREFIX BUG ID 1484
            const BSquad* pCollisionSquad = pObject->getSquad();
//--
            if ((pCollisionSquad != NULL) && (pCollisionSquad->getLastMoveTime() < lastPathTime))
               continue;

            if (isCollisionEnabledWithEntity(pObject))
            {
               if (result == cSimCollisionNone)
                  result = cSimCollisionSquad;
               continue;
            }
            else
            {
               // Add unit to ignore list if we don't want to collide with it
               mIgnoreUnits.add(pObject->getID());
            }
         }
      }
   }

   return result;
}

//==============================================================================
//==============================================================================
bool BFindPathHelper::isCollisionEnabledWithEntity(BEntity* pEntity) const
{
   if (mpEntity)
      return (mpEntity->isCollisionEnabledWithEntity(pEntity));
   else
      return (false);
}

//==============================================================================
//==============================================================================
void BFindPathHelper::updatePathingLimits(bool longRange)
{
   // DLM 11/10/08 this is totally obsolete code.
   /*
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (pathLimiter)
   {
      // Long range pathing limits
      if (longRange)
      {
         if (mpEntity->getClassType() == BEntity::cClassTypePlatoon)
         {
            pathLimiter->incNumPlatoonLRPs();
            pathLimiter->incFramePlatoonLRPCount();
            if (pathLimiter->getFramePlatoonLRPCount() > pathLimiter->getMaxPlatoonLRPperFrame())
               pathLimiter->setMaxPlatoonLRPperFrame(pathLimiter->getFramePlatoonLRPCount());
         }
         else if (mpEntity->getClassType() == BEntity::cClassTypeSquad)
         {
            pathLimiter->incNumSquadLRPs();
            pathLimiter->incFrameSquadLRPCount();
            if (pathLimiter->getFrameSquadLRPCount() > pathLimiter->getMaxSquadLRPperFrame())
               pathLimiter->setMaxSquadLRPperFrame(pathLimiter->getFrameSquadLRPCount());
         }
      }
      // Short range pathing limits
      else
      {
         if (mpEntity->getClassType() == BEntity::cClassTypePlatoon)
         {
            pathLimiter->incNumPlatoonSRPs();
            pathLimiter->incFramePlatoonSRPCount();
            if (pathLimiter->getFramePlatoonSRPCount() > pathLimiter->getMaxPlatoonSRPperFrame())
               pathLimiter->setMaxPlatoonSRPperFrame(pathLimiter->getFramePlatoonSRPCount());
         }
         else if (mpEntity->getClassType() == BEntity::cClassTypeSquad)
         {
            pathLimiter->incNumSquadSRPs();
            pathLimiter->incFrameSquadSRPCount();
            if (pathLimiter->getFrameSquadSRPCount() > pathLimiter->getMaxSquadSRPperFrame())
               pathLimiter->setMaxSquadSRPperFrame(pathLimiter->getFrameSquadSRPCount());
         }
      }

      if (gWorld->getUpdateNumber() != pathLimiter->getReferenceFrame())
      {
         pathLimiter->setReferenceFrame(gWorld->getUpdateNumber());
         pathLimiter->incNumPathingFrames();
      }
      pathLimiter->incNumPathingCallsThisFrame();
   }
   */
}


//==============================================================================
//==============================================================================
bool BFindPathHelper::testForOneWayBarrierLineCross(BVector &p1, BVector &p2, BVector &suggestedStop)
{
   BASSERT(mpEntity);

   if( !mpEntity->canJump() )
      return false;

   //Get the DOM.
   BDesignObjectManager* pDOM=gWorld->getDesignObjectManager();
   if (pDOM && (pDOM->getDesignLineCount() > 0))
   {
      //gConsoleOutput.debug("Platoon position (%f, %f, %f)", getPosition().x, getPosition().y, getPosition().z );
      for (uint i=0; i < pDOM->getDesignLineCount(); i++)
      {
         //Get the line.  Ignore non-physics lines.
         BDesignLine& line=pDOM->getDesignLine(i);
         if (line.mDesignData.isOneWayBarrierLine())
         {
            BVector forward = p2 - p1;
            forward.safeNormalize();
            //gConsoleOutput.debug("waypoint position %d (%f, %f, %f)   %d (%f, %f, %f)", p1.x, p1.y, p1.z, p2.x, p2.y, p2.z );

            //extend the line so that the front of the entity does not pass the line.
            BVector extendedp2 = forward;
            float obstRad = mpEntity->getObstructionRadius();
            extendedp2.scale( (obstRad > 10.0f) ? obstRad : 10.0f );  //make sure the radius is at least 10 m
            extendedp2.assignSum( p2, extendedp2 );

            //See if we intersect.
            BVector intersectionPoint;
            if (line.imbeddedIncidenceIntersects(forward, p1, extendedp2, intersectionPoint))
            {
               //gConsoleOutput.debug("Truncating");
               //move the newpoint 1 meter off the line (so repeated clicks don't put us past it)
               suggestedStop = forward;
               suggestedStop.scale(-1.0f * (mpEntity->getObstructionRadius() + 5.0f));  //little extra to keep it away from the line
               suggestedStop.assignSum( suggestedStop, intersectionPoint );
               return true;
            }
         }
      }
   }
   return false;
}
