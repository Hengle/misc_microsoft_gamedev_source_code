//==============================================================================
// squadplotter.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "common.h"
#include "squadplotter.h"
#include "command.h"
#include "game.h"
#include "plane.h"
#include "protoobject.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "terrain.h"
#include "selectionmanager.h"
#include "syncmacros.h"
#include "database.h"
#include "mathutil.h"
#include "debugprimitives.h"
#include "commands.h"
#include "usermanager.h"
#include "user.h"
#include "pather.h"
#include "squadactionmove.h"
#include "squadactionattack.h"
#include "config.h"
#include "configsgame.h"
#include "tactic.h"
#include "squadlosvalidator.h"
#include "tactic.h"

//#define DEBUG_SQUAD_OBSTRUCT 1
#ifdef DEBUG_SQUAD_OBSTRUCT
   BDynamicSimVectorArray debugSquadPoints;
#endif

// More debug stuff
#ifndef BUILD_FINAL
   class BDebugSurroundPosition
   {
      public:
         enum BObstructionState
         {
            ObstructionUnchecked,
            ObstructionPassed,
            ObstructionFailed
         };
         
         BDebugSurroundPosition() {}
         BDebugSurroundPosition(BVector pos, float innerRadius, float outerRadius, BObstructionState obstructed)
         {
            mPos = pos;
            mInnerRadius = innerRadius;
            mOuterRadius = outerRadius;
            mObstructed = obstructed;
         }
         BVector mPos;
         float mInnerRadius;
         float mOuterRadius;
         BObstructionState mObstructed;
   };
   BDynamicSimArray<BDebugSurroundPosition> debugSurroundPositions;

   BEntity* pDebugImmobileTargetEntity = NULL;
   BDynamicSimFloatArray debugImmobileTargetRanges;
#endif

//#define DEBUG_DRAW_WEAPON_LOS

BSquadPlotter gSquadPlotter;

BDynamicSimUIntArray BSquadPlotter::mTempSquadOrder;
BDynamicSimArray<BOPObstructionNode*> BSquadPlotter::mTempObstructions;

GFIMPLEMENTVERSION(BSquadPlotterResult,1);

//==============================================================================
// BSquadPlotterResult::BSquadPlotterResult
//==============================================================================
BSquadPlotterResult::BSquadPlotterResult(void)
{
   reset();
}

//==============================================================================
// BSquadPlotterResult::~BSquadPlotterResult
//==============================================================================
BSquadPlotterResult::~BSquadPlotterResult(void)
{
   cleanUp();
}

//==============================================================================
// BSquadPlotterResult::setWaypoints
//==============================================================================
bool BSquadPlotterResult::setWaypoints(const BDynamicSimVectorArray &v)
{
   if (mWaypoints.setNumber(v.getNumber()) == false)
      return(false);
   for (long i=0; i < mWaypoints.getNumber(); i++)
      mWaypoints[i]=v[i];
   return(true);
}

//==============================================================================
// BSquadPlotterResult::getSquadCorners
//==============================================================================
bool BSquadPlotterResult::getSquadCorners(BVector *pts) const
{  
   BVector forward;
   if(mWaypoints.getNumber() == 0)
   {
      BEntity *entity = gWorld->getEntity(mSquadID);
      if (!entity)
         return false;
      forward = entity->getPosition() - mDesiredPosition;
   }
   else
      forward = mWaypoints[mWaypoints.getNumber() - 1] - mDesiredPosition;
   if (!forward.safeNormalize())
   {
      forward = cZAxisVector;
   }
   BVector right(cYAxisVector.cross(forward));
   pts[0] = (mDesiredPosition + (forward * (mDepth * 0.5f)) + (right * (mWidth * 0.5f)));
   pts[1] = (pts[0] - (forward * mDepth));
   pts[2] = (pts[1] - (right * mWidth));
   pts[3] = (pts[2] + (forward * mDepth));    

   return true;
}

//==============================================================================
// BSquadPlotterResult::render
//==============================================================================
void BSquadPlotterResult::render(void) const
{
}

//==============================================================================
// BSquadPlotterResult::reset
//==============================================================================
void BSquadPlotterResult::reset(void)
{
   mSquadID=cInvalidObjectID;
   mWidth=1.0f;
   mDepth=1.0f;
   mRange=0.0f;
   mTrueActionRange=0.0f;
   mWaypoints.setNumber(0);
   mDesiredPosition=cInvalidVector;
   mMovementType=-1;
   mObstructed = false;
   mInRange=false;
   mpProtoAction=NULL;
   mUsingOverrideRange = false;
   mDefaultDesiredPos = false;
}

//==============================================================================
// BSquadPlotterResult::cleanUp
//==============================================================================
void BSquadPlotterResult::cleanUp(void)
{
   reset();
}



//==============================================================================
// BSquadPlotterProjectileGroup::BSquadPlotterProjectileGroup
//==============================================================================
BSquadPlotterProjectileGroup::BSquadPlotterProjectileGroup( const BProtoAction* pProtoAction )
{
   reset();

   mpProtoAction = pProtoAction;
}

//==============================================================================
// BSquadPlotterProjectileGroup::BSquadPlotterProjectileGroup
//==============================================================================
BSquadPlotterProjectileGroup::BSquadPlotterProjectileGroup()
{
   reset();
}

//==============================================================================
// BSquadPlotterProjectileGroup::~BSquadPlotterProjectileGroup
//==============================================================================
BSquadPlotterProjectileGroup::~BSquadPlotterProjectileGroup(void)
{
   cleanUp();
}

//==============================================================================
// BSquadPlotterProjectileGroup::reset
//==============================================================================
void BSquadPlotterProjectileGroup::reset(void)
{
   mSquadsIndices.setNumber(0);
   mpProtoAction=NULL;
}

//==============================================================================
// BSquadPlotterProjectileGroup::cleanUp
//==============================================================================
void BSquadPlotterProjectileGroup::cleanUp(void)
{
   reset();
}


//==============================================================================
// BSquadPlotter::BSquadPlotter
//==============================================================================
BSquadPlotter::BSquadPlotter(void)
{
   reset();
}

//==============================================================================
// BSquadPlotter::~BSquadPlotter
//==============================================================================
BSquadPlotter::~BSquadPlotter(void)
{
   cleanUp();
}


//==============================================================================
//==============================================================================
bool BSquadPlotter::plotSquads(const BEntityIDArray &squads, const BCommand *command, DWORD flags)
{
   return (plotSquads(squads, command->getPlayerID(), command->getTargetID(),
      command->getWaypointList(), command->getTargetPosition(), command->getAbilityID(), flags));
}

//==============================================================================
//==============================================================================
bool BSquadPlotter::plotSquads(const BEntityIDArray& squads, long playerID, BEntityID targetID, 
                               const BDynamicSimVectorArray& waypoints, BVector targetPosition, long abilityID, DWORD flags, 
                               float overrideRange /*= -1.0f*/, BVector overrideTargetPos /*= cInvalidVector*/, float overrideRadius /*= 0.0f*/)
{
   bool sync = !(flags & cSPFlagNoSync);

   //Reset.
   if (!(flags & cSPFlagSkipReset))
   {
      reset();
   }
   
   //Validate squads.
   if (!(flags & cSPFlagSkipReset))
   {
      for (long i=0; i < squads.getNumber(); i++)
      {
         BSquad *sq = gWorld->getSquad(squads[i]);
         if ((sq != NULL) && ((flags & cSPFlagIgnorePlayerRestriction) || (sq->getPlayerID() == playerID)) )
         {
            if (sync)
            {
               syncSquadData("BSquadPlotter::plotSquads -- squadID", sq->getID().asLong());
               syncSquadData("                                 position", sq->getPosition());
            }
            mSquads.add(sq->getID());
         }
      }
   }
   if (mSquads.getNumber() <= 0)
      return(false);

   // Get the target object/entity.
   BEntity* pTargetEntity = gWorld->getEntity(targetID);

   // ajl 2/26/07 - If the target object has a parent squad, use the parent instead...
   //  this is to fix a discrepency between the squad plotter position calculation and 
   //  squad attack action range checking.
   if (pTargetEntity && (pTargetEntity->getParentID() != cInvalidObjectID))
   {
      BSquad* pTargetSquad = gWorld->getSquad(pTargetEntity->getParentID());
      if (pTargetSquad)
         pTargetEntity = pTargetSquad;
   }

   bool validTarget = false;   // only used when determining form of plot   

   bool useOverrideTargetPos = !overrideTargetPos.almostEqual(cInvalidVector);
   
   // Fail if no waypoints, target entity, and override target position is not used
   if ((waypoints.getNumber() <= 0) && !pTargetEntity && !useOverrideTargetPos)
      return (false);

   bool mixedMovementTypes = false;
   bool forceLandMovementType = ((flags & cSPFlagForceLandMovementType) != 0);
   long movementType = -1;
   if (forceLandMovementType)
   {
      movementType = gDatabase.getMovementType("Land");
   }
   //Allocate enough results.
   if (mResults.setNumber(mSquads.getNumber()) == false)
      return (false);
   
   // Resolve target position and radius
   float targetRadius = 0.0f;   
   if (useOverrideTargetPos)
   {
      targetPosition = overrideTargetPos;
      targetRadius = overrideRadius;
   }
   else if (pTargetEntity)
   {
      targetPosition = pTargetEntity->getPosition();
      targetRadius = pTargetEntity->getObstructionRadius();
   }
   else if (targetPosition.almostEqual(cInvalidVector))
   {
      BASSERTM(false, "No valid target data can be resolved!");
      return (false);
   }
   if (sync)
   {
      syncSquadData("BSquadPlotter::plotSquads -- targetRadius", targetRadius);
   }

   //Init all of the squad data.
   BSimTarget target(targetID, targetPosition, abilityID);
   if (pTargetEntity)
   {
      target.setID(pTargetEntity->getID());
      target.setPosition(pTargetEntity->getPosition());
   }
   // DLM - only set the target to valid if it's an invalid target for ALL units in the squad -- not for only one target.
   validTarget = false;
   for (uint i = 0; i < mSquads.getSize(); i++)
   {
      //Set the ID.
      mResults[i].setSquadID(mSquads[i]);

      //Width/Depth.
//-- FIXING PREFIX BUG ID 2855
      const BSquad* sq = gWorld->getSquad(mSquads[i]);
//--
      float width = 1.0f;
      float depth = 1.0f;
      float range = 0.0f;
      BProtoAction* pProtoAction = NULL;

      // Override range
      bool useOverrideRange = (overrideRange >= 0.0f);
      if (useOverrideRange)
      {
         range = overrideRange;
         validTarget = true;
      }
      //Get Squad Range.
      else if (sq && sq->calculateRange(&target, range, &pProtoAction, NULL))
         validTarget = true;

      // Save the true range before reducing it
      float trueActionRange = range;
      if (trueActionRange <= 0.0f)
         trueActionRange = 0.1f;
    
      range -= 1.0f;
      if (range <= 0.0f)
         range = 0.1f;

      if (sq)
         sq->getWidthAndDepth(width, depth);

      //Save it.
      mResults[i].setWidth(width);
      mResults[i].setDepth(depth);
      mResults[i].setRange(range);
      mResults[i].setTrueActionRange(trueActionRange);
      mResults[i].setProtoAction(pProtoAction);
      mResults[i].setUsingOverrideRange(useOverrideRange);
      mResults[i].setDefaultDesiredPos(false);

      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- squad data", sq ? sq->getID().asLong() : -1);
         syncSquadData("                                 width", width);
         syncSquadData("                                 depth", depth);
         if(pTargetEntity)
            syncSquadData("                                 range", range);
      }
      
      if (!forceLandMovementType)
      {
//-- FIXING PREFIX BUG ID 2856
         const BUnit* pUnit = sq ? sq->getLeaderUnit() : NULL;
//--
         if (pUnit)
            movementType = pUnit->getProtoObject()->getMovementType();
      }

      mResults[i].setMovementType(movementType);
      if (!mixedMovementTypes)
      {
         if ((movementType == -1) || ((mBaseMovementType != -1) && (movementType != mBaseMovementType)))
         {
            mBaseMovementType = -1;
            mixedMovementTypes = true;
         }
         else if (movementType != mBaseMovementType)
            mBaseMovementType = movementType;
      }
   }

   // Get target object - don't use if its in the squads we're plotting   
   if (pTargetEntity)
   {
      for (uint i = 0; i < mSquads.getSize(); i++)
      {
//-- FIXING PREFIX BUG ID 2857
         const BSquad* sq = gWorld->getSquad(mSquads[i]);
//--
         if (sq && sq->containsChild(pTargetEntity->getID()))
         {
            validTarget = false;
            break;
         }
      }
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- targetObjectID", pTargetEntity->getID().asLong());
         if (pTargetEntity->getProtoObject())
            syncSquadData("BSquadPlotter::plotSquads -- targetName", pTargetEntity->getProtoObject()->getName());
      }
   }

   //HACK HACK HACK: Evil temp code ensues.
   //Go through the squads to get widths and average current position.
   static BDynamicSimFloatArray widths;
   if (widths.setNumber(mSquads.getNumber()) == false)
      return(false);
   memset(widths.getPtr(), 0, sizeof(float)*widths.getNumber());
   float totalWidth=0.0f;
   float totalDepth=0.0f;
   BVector averageCurrentPosition(0.0f);
   if (sync)
   {
      syncSquadData("BSquadPlotter::plotSquads -- mSquads.getNumber", mSquads.getNumber());
   }
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      BSquad *sq=gWorld->getSquad(mSquads[i]);
      widths[i]=mResults[i].getWidth();
      totalWidth+=widths[i];
      totalDepth+=mResults[i].getDepth();
      averageCurrentPosition+=sq->getPosition();
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- sq->getPosition", sq->getPosition());
      }
   }
   if (sync)
   {
      syncSquadData("BSquadPlotter::plotSquads -- totalWidth", totalWidth);
      syncSquadData("BSquadPlotter::plotSquads -- totalDepth", totalDepth);
   }
   if (totalWidth < cFloatCompareEpsilon)
      return(false);
   if (totalDepth < cFloatCompareEpsilon)
      return(false);
   averageCurrentPosition/=(float)mSquads.getNumber();
   averageCurrentPosition.y = 0.0f;
   if (sync)
   {
      syncSquadData("BSquadPlotter::plotSquads -- averageCurrentPosition", averageCurrentPosition);
   }

   //Figure out where I want to orient "around".
   BVector orientForward;
   if (sync)
   {
      syncSquadData("BSquadPlotter::plotSquads -- commandWaypoints.getNumber", waypoints.getNumber());
   }
   
   //DCP 06/05/07: No facing right now.
   //If we have a facing, use it.
   //if (command->getFacing()!=cInvalidVector)
   //{
   //   orientForward=command->getFacing();
   //   if (sync)
   //   {
   //      syncSquadData("BSquadPlotter::plotSquads -- orientForward", orientForward);
   //   }
   //}
   //If we have no waypoints, use our target position (if we have one).
   //else
   if (waypoints.getNumber() <= 0)
   {
      orientForward = targetPosition - averageCurrentPosition;
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientForward", orientForward);
      }
   }
   //Else, if we have two waypoints to use, use them.
   else if (waypoints.getNumber() > 1)
   {
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- last command waypoint", waypoints[waypoints.getNumber()-1]);
      }
      orientForward=waypoints[waypoints.getNumber()-1]-waypoints[waypoints.getNumber()-2];
      // Fallback if last 2 waypoints are equal
      orientForward.y = 0.0f;
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientForward", orientForward);
      }
      if (!orientForward.safeNormalize())
      {
         orientForward=waypoints[0]-averageCurrentPosition;
         if (sync)
         {
            syncSquadData("BSquadPlotter::plotSquads -- orientForward", orientForward);
         }
      }
   }
   //Else, if we have one waypoint, use that and the squad's position.
   else
   {      
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- commandWaypoints[0]", waypoints[0]);
      }
      orientForward=waypoints[0]-averageCurrentPosition;
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientForward", orientForward);
      }
   }
   orientForward.y=0.0f;
   if (!orientForward.safeNormalize())
   {
      // Fallback
      orientForward = cXAxisVector;
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientForward", orientForward);
      }
   }

   BVector orientRight(cYAxisVector.cross(orientForward));
   if (sync)
   {
      syncSquadData("BSquadPlotter::plotSquads -- orientRight", orientRight);
   }
   float orientDistance=20.0f;
   BVector orientPosition=orientForward;
   orientPosition*=orientDistance;
   if (waypoints.getNumber() > 0)
   {
      orientPosition+=waypoints[waypoints.getNumber()-1];
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientPosition", orientPosition);
      }
   }
   else if (pTargetEntity)
   {
      orientPosition+=pTargetEntity->getPosition();
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientPosition", orientPosition);
      }
   }
   else
   {
      orientPosition+=targetPosition;
      if (sync)
      {
         syncSquadData("BSquadPlotter::plotSquads -- orientPosition", orientPosition);
      }
   }

   //Create a copy of the waypoint list with 1 extra spot for the orientation waypoint,
   //with some special casing for the 0 command waypoints case.
   static BDynamicSimVectorArray resultWaypoints;
   if (waypoints.getNumber() > 0)
   {
      if (resultWaypoints.setNumber(waypoints.getNumber()+1) == false)
         return(false);
      long j;
      for (j=0; j < waypoints.getNumber(); j++)
         resultWaypoints[j]=waypoints[j];
      resultWaypoints[j]=cInvalidVector;
   }
   else
   {
      if (resultWaypoints.setNumber(2) == false)
         return(false);
      resultWaypoints[0]=averageCurrentPosition;
      resultWaypoints[1]=cInvalidVector;
   }
   
   // Debug stuff
   #ifdef DEBUG_SQUAD_OBSTRUCT
      debugSquadPoints.clear();
   #endif
   #ifndef BUILD_FINAL
      debugSurroundPositions.clear();
      pDebugImmobileTargetEntity = NULL;
      debugImmobileTargetRanges.clear();
   #endif
  
   // Check desired point against obstructions if we're not targeting a unit and no override position
   if ((!pTargetEntity && !useOverrideTargetPos) || !validTarget)
   {
      BVector newPos(cOriginVector);
      BVector origPos(orientPosition - (orientForward * orientDistance));
      if (findClosestUnobstructedPosition(origPos, orientForward, 1.0f, newPos, cForwardAndBackward, mBaseMovementType))
      {
         if (sync)
         {
            syncSquadData("BSquadPlotter::plotSquads -- newPos", newPos);
         }
         orientPosition = newPos + (orientForward * orientDistance);
         if (sync)
         {
            syncSquadData("  BSquadPlotter orientPosition", orientPosition);
         }
      }
   }

   if (sync)
   {
      // Sync values sent to plotting functions
      syncSquadData("  BSquadPlotter orientDistance", orientDistance);
      syncSquadData("  BSquadPlotter orientForward", orientForward);
      syncSquadData("  BSquadPlotter orientRight", orientRight);
      syncSquadData("  BSquadPlotter orientPosition", orientPosition);
   }

   // Make the actual squad positions / orientations
   // TODO
   // -Need better range positioning

   // Do squad placement for targeting a unit or an override position
   if ((pTargetEntity || useOverrideTargetPos) && validTarget)
   {
      // Do surround positioning - use the final waypoint as the position to place squads near
      if ((waypoints.getNumber() > 0) && (waypoints[0].distance(targetPosition) > targetRadius))
      {
         orientPosition = waypoints[0];
      }
      else
      {
         orientPosition = averageCurrentPosition;
      }
      makeSurroundPositionConfiguration(orientForward, orientRight, orientPosition, resultWaypoints, targetRadius, pTargetEntity, flags, overrideTargetPos, overrideRadius);

      if (sync)
      {
         // Sync after ideal positioning
         #ifdef SYNC_Squad
            syncResults();
         #endif
      }
   }
   // Do movement squad placement
   else
   {
      // Check obstructions and fallback as necessary
      //makeRowConfiguration(totalWidth, orientDistance, orientForward, orientRight, orientPosition, resultWaypoints, flags);
      make2ColumnConfiguration(orientDistance, orientForward, orientRight, orientPosition, resultWaypoints, flags);
      //makeColumnConfiguration(totalDepth, orientDistance, orientForward, orientRight, orientPosition, resultWaypoints, flags);

      if (sync)
      {
         // Sync after ideal positioning
         #ifdef SYNC_Squad
            syncResults();
         #endif
      }

      /*
      // Check and move off obstructions
      checkAgainstObstructions(cBackwardOnly, NULL);

      if (sync)
      {
         // Sync post-obstruction handling
         #ifdef SYNC_Squad
            syncResults();
         #endif
      }
      */
   }

   pushSquadsOffEdgeOfMap();

   //DCPTODO 06/05/07: Turning this off for now to make things simpler.
   //==============================================================================================================================
   // SLB: TEMP HACK
   //==============================================================================================================================
   //if (!(flags & cSPFlagIgnoreWeaponLOS) && pTargetEntity && validTarget)// && (command->getType() == cCommandWork))
   //{
   //   positionSquadsForWeaponLOSToTarget(pTargetEntity, playerID, targetID, waypoints, targetPosition, abilityID, flags);
   //}
   //==============================================================================================================================

   //Done.
   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadPlotter::debugPlotSquads(BVector &point)
{
   BUser* pUser = gUserManager.getPrimaryUser();
   if ((pUser == NULL) || (pUser->getSelectionManager() == NULL))
      return false;
   BSelectionManager *pSelMgr = pUser->getSelectionManager();

   // Do a plot for the selected squads
   BPlatoon* pPlatoon = NULL;
   BEntityIDArray squads;
   for (int i = 0; i < pSelMgr->getNumberSelectedSquads(); i++)
   {
      squads.add(pSelMgr->getSelectedSquad(i));
      BSquad* pSquad = gWorld->getSquad(squads[i]);
      if (!pPlatoon && pSquad && pSquad->getParentPlatoon())
         pPlatoon = pSquad->getParentPlatoon();
   }
   if (squads.getSize() == 0)
      return false;

   // Use the unit being hovered over as the target
   BEntityID targetID;
   for (int i = 0; i < pSelMgr->getPossibleSelectionCount(); i++)
   {
      BEntityID entityID = pSelMgr->getPossibleSelection(i);
//-- FIXING PREFIX BUG ID 2859
      const BEntity* pTargetEntity = gWorld->getEntity(entityID);
//--
      if (pTargetEntity != NULL)
      {
         targetID = entityID;
         if (entityID.getType() == BEntity::cClassTypeUnit)
         {
            pTargetEntity = gWorld->getEntity(pTargetEntity->getParentID());
            if (pTargetEntity)
               point = pTargetEntity->getPosition();
         }
         break;
      }
   }

   // If there are any parent platoons, use the first one to run a long range
   // path to get a general direction toward target
   static long abilityID = -1;
   static BDynamicSimVectorArray waypoints;
   if (pPlatoon)
   {
      BFindPathHelper pathFinder;
      BSimTarget target(targetID, point, abilityID);
      pathFinder.setTarget(&target);
      pathFinder.setEntity(pPlatoon);
      pathFinder.setPathingRadius(pPlatoon->getPathingRadius());
      pathFinder.setPathableAsFlyingUnit(false);
      pathFinder.enableReducePath(false);
      pathFinder.enablePathAroundSquads(false);
      static BPath path;
      path.reset();
      pPlatoon->calculateRange(&(pathFinder.getTarget()));
      uint result = pathFinder.findPath(path, pPlatoon->getPosition(), point, false, targetID, true);
      if ((result != BPath::cFailed) && (path.getNumberWaypoints() > 1))
      {
         path.render(cDWORDPurple);

         // Get index of first in range waypoint
         long wpIndex = path.getNumberWaypoints() - 1;
//-- FIXING PREFIX BUG ID 2860
         const BEntity* pTargetEntity = gWorld->getEntity(targetID);
//--
         if (pTargetEntity)
         {
            while ((wpIndex > 0) && (pTargetEntity->calculateXZDistance(path.getWaypoint(wpIndex)) < pathFinder.getTarget().getRange()))
            {
               wpIndex--;
            }
         }
         
         waypoints.setNumber(2);
         waypoints[0] = path.getWaypoint(wpIndex);
         waypoints[1] = point;
      }
      else
      {
         waypoints.setNumber(2);
         waypoints[0] = pPlatoon->getPosition();
         waypoints[1] = point;
      }
   }
   else
   {
      waypoints.setNumber(1);
      waypoints[0] = point;
   }

   return (plotSquads(squads, -1, targetID, waypoints, point, abilityID, cSPFlagIgnorePlayerRestriction));
}



struct BSquadPlotterLOSTest
{
   float mPercentage;
   float mDistanceSqr;
   BVector mPosition;
   BDynamicSimArray<BSquadPlotterResult> mResults;
};


//==============================================================================================================================
// SLB: TEMP HACK
//==============================================================================================================================
void BSquadPlotter::positionSquadsForWeaponLOSToTarget(const BEntity *pTargetEntity, long playerID, BEntityID targetID, const BDynamicSimVectorArray& waypoints, BVector targetPosition, long abilityID, DWORD flags)
{
   BVector originalWaypoint = waypoints[waypoints.getNumber()-1];
   BSquadPlotterLOSTest bestLOSTestResult;


   float middlePercentage;
   float leftPercentage;
   float rightPercentage;
   BVector leftIntersectionPoint;
   BVector rightIntersectionPoint;
   BVector platoonPosition;

   int32 numResults = mResults.getNumber();
   float recNumResults = 1.0f / (float) numResults;
   platoonPosition.zero();
   for (int32 i = 0; i < numResults; i++)
   {
      const BSquadPlotterResult &result = mResults.get(i);
      platoonPosition += result.getDesiredPosition();
   }
   platoonPosition *= recNumResults;
   gTerrainSimRep.getHeightRaycast(platoonPosition, platoonPosition.y, true);

   middlePercentage = percentageSquadsHaveWeaponLOSToTarget(pTargetEntity, platoonPosition, leftIntersectionPoint, rightIntersectionPoint);

   if (middlePercentage < (1.0f - cFloatCompareEpsilon))
   {
      bestLOSTestResult.mPercentage = middlePercentage;
      bestLOSTestResult.mDistanceSqr = 0.0f;
      bestLOSTestResult.mPosition = originalWaypoint;
      bestLOSTestResult.mResults = mResults;


      static BWorkCommand workCommand;
      BVector targetPosition = pTargetEntity->getPosition();
      BVector middleLeftSegment;
      BVector middleRightSegment;
      middleLeftSegment.assignDifference(platoonPosition, targetPosition);
      middleRightSegment = middleLeftSegment;
      float distance = middleLeftSegment.length();
      flags |= cSPFlagIgnoreWeaponLOS | cSPFlagIgnoreForwardOnly | cSPFlagSkipReset | cSPFlagForceMove;

      BDynamicSimVectorArray newWaypoints;


      for (uint32 loop = 0; loop < 10; loop++)
      {
         BVector tempLeftL;
         BVector tempRightL;
         BVector tempLeftR;
         BVector tempRightR;
         BVector leftPlatoonPosition;
         BVector rightPlatoonPosition;
         BVector leftSegment;
         BVector rightSegment;
         BVector left;
         BVector right;
         const float step = 10.0f;

         left.assignCrossProduct(cYAxisVector, middleLeftSegment);
         left *= step / left.length();

         right.assignCrossProduct(-cYAxisVector, middleRightSegment);
         right *= step / right.length();

         leftIntersectionPoint += left;
         rightIntersectionPoint += right;

         leftSegment.assignDifference(leftIntersectionPoint, targetPosition);
         rightSegment.assignDifference(rightIntersectionPoint, targetPosition);

         float leftDistance = leftSegment.length();
         leftPlatoonPosition.assignSum(targetPosition, leftSegment * (-distance / leftDistance));
         gTerrainSimRep.getHeightRaycast(leftPlatoonPosition, leftPlatoonPosition.y, true);

         /*
         workCommand = *(reinterpret_cast<const BWorkCommand *>(pCommand));
         workCommand.addWaypoint(leftPlatoonPosition);
         plotSquads(mSquads, &workCommand, flags);
         */
         newWaypoints.clear();
         newWaypoints.add(leftPlatoonPosition);
         plotSquads(mSquads, playerID, targetID, newWaypoints, targetPosition, abilityID, flags);

         leftPercentage = percentageSquadsHaveWeaponLOSToTarget(pTargetEntity, leftPlatoonPosition, tempLeftL, tempRightL);

         if (leftPercentage >= (1.0f - cFloatCompareEpsilon))
         {
            return;
         }

         if(leftPercentage <= bestLOSTestResult.mPercentage)
         {
            float distanceSqr = originalWaypoint.distanceSqr(leftPlatoonPosition);
            if(distanceSqr < bestLOSTestResult.mDistanceSqr)
            {
               bestLOSTestResult.mPercentage = leftPercentage;
               bestLOSTestResult.mDistanceSqr = distanceSqr;
               bestLOSTestResult.mPosition = leftPlatoonPosition;
               bestLOSTestResult.mResults = mResults;
            }
         }

         float rightDistance = rightSegment.length();
         rightPlatoonPosition.assignSum(targetPosition, rightSegment * (-distance / rightDistance));
         gTerrainSimRep.getHeightRaycast(rightPlatoonPosition, rightPlatoonPosition.y, true);

         /*
         workCommand = *(reinterpret_cast<const BWorkCommand *>(pCommand));
         workCommand.addWaypoint(rightPlatoonPosition);
         plotSquads(mSquads, &workCommand, flags);
         */
         newWaypoints.clear();
         newWaypoints.add(rightPlatoonPosition);
         plotSquads(mSquads, playerID, targetID, newWaypoints, targetPosition, abilityID, flags);

         rightPercentage = percentageSquadsHaveWeaponLOSToTarget(pTargetEntity, rightPlatoonPosition, tempLeftR, tempRightR);

         if (rightPercentage >= (1.0f - cFloatCompareEpsilon))
         {
            return;
         }

         if(rightPercentage <= bestLOSTestResult.mPercentage)
         {
            float distanceSqr = originalWaypoint.distanceSqr(rightPlatoonPosition);
            if(distanceSqr < bestLOSTestResult.mDistanceSqr)
            {
               bestLOSTestResult.mPercentage = rightPercentage;
               bestLOSTestResult.mDistanceSqr = distanceSqr;
               bestLOSTestResult.mPosition = rightPlatoonPosition;
               bestLOSTestResult.mResults = mResults;
            }
         }

         leftIntersectionPoint = tempLeftL;
         rightIntersectionPoint = tempRightR;
         middleLeftSegment.assignDifference(leftPlatoonPosition, targetPosition);
         middleRightSegment.assignDifference(rightPlatoonPosition, targetPosition);
      }


      // No position is perfect, pick the best one from all the ones tested.  The best one will be
      // the one with the best percentage that is the closest.
      //
      mResults = bestLOSTestResult.mResults;
      /*
      waypoints.clear();
      waypoints.add(bestLOSTestResult.mPosition);
      plotSquads(mSquads, playerID, targetID, waypoints, targetPosition, abilityID, flags);
      */
   }
   else
   {
      return;
   }
}

float BSquadPlotter::percentageSquadsHaveWeaponLOSToTarget(const BEntity *pTargetEntity, BVector platoonPosition, BVector &maxLeftIntersectionPoint, BVector &maxRightIntersectionPoint) const
{
   float numValid = 0.0f;
   int32 numSquads = mSquads.getNumber();
   float recNumSquads = 1.0f / (float) numSquads;
   BVector targetPosition = pTargetEntity->getPosition();
   BVector segment;
   segment.assignDifference(targetPosition, platoonPosition);
   BVector right;
   right.assignCrossProduct(cYAxisVector, segment);
   maxLeftIntersectionPoint = platoonPosition;
   maxRightIntersectionPoint = platoonPosition;
   bool hasIntersection = false;
   for (int32 i = 0; i < numSquads; i++)
   {
      bool leftIntersection;
      bool rightIntersection;
      BVector leftIntersectionPoint;
      BVector rightIntersectionPoint;
      if (squadHasWeaponLOSToTarget(i, pTargetEntity, leftIntersection, leftIntersectionPoint, rightIntersection, rightIntersectionPoint))
      {
         numValid++;
      }
      else
      {
         if (!hasIntersection)
         {
            maxLeftIntersectionPoint = (leftIntersection) ? leftIntersectionPoint : rightIntersectionPoint;
            maxRightIntersectionPoint = (rightIntersection) ? rightIntersectionPoint : leftIntersectionPoint;
         }
         else
         {
            if (leftIntersection)
            {
               BVector offsetFromMax;
               offsetFromMax.assignDifference(leftIntersectionPoint, maxLeftIntersectionPoint);
               maxLeftIntersectionPoint = (offsetFromMax.dot(right) < 0.0f) ? leftIntersectionPoint : maxLeftIntersectionPoint;
            }
            if (rightIntersection)
            {
               BVector offsetFromMax;
               offsetFromMax.assignDifference(rightIntersectionPoint, maxRightIntersectionPoint);
               maxRightIntersectionPoint = (offsetFromMax.dot(right) > 0.0f) ? rightIntersectionPoint : maxRightIntersectionPoint;
            }
         }
         hasIntersection = true;
      }
   }

   return (numValid * recNumSquads);
}

bool BSquadPlotter::squadHasWeaponLOSToTarget(int32 squadIndex, const BEntity *pTargetEntity, bool &leftIntersection, BVector &leftIntersectionPoint, bool &rightIntersection, BVector &rightIntersectionPoint) const
{
   leftIntersection = false;
   rightIntersection = false;
   const BSquad *pSquad = gWorld->getSquad(mSquads.get(squadIndex));
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   BVector targetPosition = pTargetEntity->getPosition();
   BSimTarget target(pTargetEntity->getID(), targetPosition);
   BSimOrderType orderType=pSquad->getOrderType(target, NULL, NULL);
   if (orderType == BSimOrder::cTypeAttack)
   {
      const BSquadPlotterResult &result = mResults.get(squadIndex);
      BVector squadPosition = result.getDesiredPosition();
      gTerrainSimRep.getHeightRaycast(squadPosition, squadPosition.y, true);
      if (pSquad->getNumberChildren() == 1)
      {
         // Check for obstructions between squad position and target
         leftIntersection = !positionHasWeaponLOSToTarget(squadPosition, targetPosition, leftIntersectionPoint);
         rightIntersection = leftIntersection;
         rightIntersectionPoint = leftIntersectionPoint;
      }
      else
      {
         // Check for obstructions between the front corners of the squad and the target.
         float width;
         float depth;
         pSquad->getWidthAndDepth(width, depth);
         float radius = Math::Max(width, depth) * 0.5f;
         BVector segment;
         segment.assignDifference(targetPosition, squadPosition);
         BVector right;
         right.assignCrossProduct(cYAxisVector, segment);
         BVector front;
         front.assignCrossProduct(-cYAxisVector, right);
         right *= radius / right.length();
         front *= radius / front.length();
         BVector inFrontOfSquad;
         inFrontOfSquad.assignSum(front, squadPosition);

         BVector p1;
         p1.assignDifference(inFrontOfSquad, right);
         leftIntersection = !positionHasWeaponLOSToTarget(p1, targetPosition, leftIntersectionPoint);

         BVector p2;
         p2.assignSum(inFrontOfSquad, right);
         rightIntersection = !positionHasWeaponLOSToTarget(p2, targetPosition, rightIntersectionPoint);
      }
   }

   return !(leftIntersection | rightIntersection);
}

bool BSquadPlotter::positionHasWeaponLOSToTarget(BVector sourcePosition, BVector targetPosition, BVector &intersectionPoint) const
{
   sourcePosition.y += 5.0f;
   targetPosition.y += 5.0f;
   bool intersection = gTerrainSimRep.segmentIntersects(targetPosition, sourcePosition, intersectionPoint);

   #if defined(DEBUG_DRAW_WEAPON_LOS)
      gpDebugPrimitives->addDebugLine(sourcePosition, targetPosition, cDWORDRed, cDWORDRed, BDebugPrimitives::cCategoryTest, 5.0f);
      if (intersection)
      {
         gpDebugPrimitives->addDebugSphere(intersectionPoint, 1.0f, cDWORDBlue, BDebugPrimitives::cCategoryTest, 5.0f);
      }
   #endif

   intersectionPoint.y -= 5.0f;

   return !intersection;
}
//==============================================================================================================================

//==============================================================================
// BSquadPlotter::makeCircularConfiguration
//==============================================================================
void BSquadPlotter::makeCircularConfiguration(float totalWidth, float orientDistance,
                                              BVector &orientForward, BVector &orientRight,
                                              BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, DWORD flags)
{
   // Given desired pos / orientation, figure out order in which to place squads
   // to minimize criss-cross.  We just do some simple plane sorting here.
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      BSquad *sqToPlace = gWorld->getSquad(mSquads[i]);
      BASSERT(sqToPlace);

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool inFront = true;
      while ((insertIndex < i) && inFront)
      {
         // Make plane
         BSquad *sq=gWorld->getSquad(mSquads[mTempSquadOrder[insertIndex]]);
         BASSERT(sq);
         BPlane plane(orientRight, sq->getPosition());

         // Check location of this squad wrt plane
         long result = plane.checkPoint(sqToPlace->getPosition());
         if (result == BPlane::cFrontOfPlane)
            insertIndex++;
         else
            inFront = false;
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // Get min range of squads
   float minRange = FLT_MAX;
   float maxDepth = -FLT_MAX;
   for (long i = 0; i < mResults.getNumber(); i++)
   {
      minRange = min(minRange, mResults[i].getRange());
      maxDepth = max(maxDepth, mResults[i].getDepth());
   }
   minRange = (minRange * 0.9f) - (maxDepth * 0.5f);

   // Make spaced positions / orientations about circle
   // HACK - temp hardcoded spacing stuff
   float squadSpacing = totalWidth * 0.125f;
   float singleSquadSpace = squadSpacing / (mSquads.getNumber() - 1);
   float totalArcLength = totalWidth + squadSpacing;
   float arcLengthFromCenter = totalArcLength * 0.5f;
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      long squadIndex = mTempSquadOrder[i];
      float halfSquadWidth=mResults[squadIndex].getWidth()*0.5f;

      /////////////////////////////////////////////////////////////////////////
      // Determine desired position

      // Update arcLengthFromCenter and determine angle
      arcLengthFromCenter -= halfSquadWidth;
      float theta = arcLengthFromCenter / minRange;//orientDistance;

      // Rotate -orientForward by theta and add scaled result to orientPosition
      BVector offsetVector = -orientForward;
      offsetVector.rotateXZ(theta);
      float offsetDistance=orientDistance;
      offsetDistance+=mResults[squadIndex].getRange()*0.9f;
      BVector desiredPosition = orientPosition + (offsetVector * offsetDistance);
      
      // Set desired position
      mResults[squadIndex].setDesiredPosition(desiredPosition);
      mResults[squadIndex].setDefaultDesiredPos(false);

      // Set waypoints
      // The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
      // The last waypoint is in the orientation of the formation.
      BVector orientDiff= orientPosition - mResults[squadIndex].getDesiredPosition();
      orientDiff.normalize();
      resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
      mResults[squadIndex].setWaypoints(resultWaypoints);

      // Remove space
      arcLengthFromCenter -= (halfSquadWidth + singleSquadSpace);
   }
}

//==============================================================================
// BSquadPlotter::makeRowConfiguration
//==============================================================================
void BSquadPlotter::makeRowConfiguration(float totalWidth, float orientDistance,
                                         BVector &orientForward, BVector &orientRight,
                                         BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, DWORD flags)
{
   // Given desired pos / orientation, figure out order in which to place squads
   // to minimize criss-cross.  We just do some simple plane sorting here.
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      BSquad *sqToPlace = gWorld->getSquad(mSquads[i]);
      BASSERT(sqToPlace);

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool inFront = true;
      while ((insertIndex < i) && inFront)
      {
         // Make plane
         BSquad *sq=gWorld->getSquad(mSquads[mTempSquadOrder[insertIndex]]);
         BASSERT(sq);
         BPlane plane(orientRight, sq->getPosition());

         // Check location of this squad wrt plane
         long result = plane.checkPoint(sqToPlace->getPosition());
         if (result == BPlane::cFrontOfPlane)
            insertIndex++;
         else
            inFront = false;
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // Make spaced positions / orientations about line / row
   // HACK - temp hardcoded spacing stuff
   float singleSquadSpace = 2.0f;//squadSpacing / (mSquads.getNumber() - 1);
   BVector desiredPosition = orientPosition - (orientForward * orientDistance);
   desiredPosition -= (orientRight * ((totalWidth * 0.5f) + (singleSquadSpace * (mSquads.getNumber() - 1) * 0.5f)));
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      long squadIndex = mTempSquadOrder[i];
      float halfSquadWidth=mResults[squadIndex].getWidth()*0.5f;

      /////////////////////////////////////////////////////////////////////////
      // Determine desired position
      desiredPosition += (orientRight * halfSquadWidth);

      // Set desired position
      mResults[squadIndex].setDesiredPosition(desiredPosition);
      mResults[squadIndex].setDefaultDesiredPos(false);

      // Set waypoints
      // The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
      // The last waypoint is in the orientation of the formation.
      BVector orientDiff= orientForward;//orientPosition - mResults[squadIndex].getDesiredPosition();
      if (!orientDiff.safeNormalize())
         orientDiff = cXAxisVector;
      resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
      mResults[squadIndex].setWaypoints(resultWaypoints);

      // Remove space
      desiredPosition += (orientRight * (halfSquadWidth + singleSquadSpace));
   }
}

//==============================================================================
// BSquadPlotter::make2ColumnConfiguration
//==============================================================================
void BSquadPlotter::make2ColumnConfiguration(float orientDistance, BVector &orientForward, BVector &orientRight,
                                             BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, DWORD flags)
{
   // Sort squads by range, equally ranged squads will get sorted by position to 
   // minimize forward criss-cross
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      float rangeToPlace = mResults[i].getRange();
      BSquad *sqToPlace = gWorld->getSquad(mSquads[i]);
      BASSERT(sqToPlace);

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool outRanged = true;
      while ((insertIndex < i) && outRanged)
      {
         float rangeToCompare = mResults[mTempSquadOrder[insertIndex]].getRange();
         BSquad *sqToCompare = gWorld->getSquad(mSquads[mTempSquadOrder[insertIndex]]);
         BASSERT(sqToCompare);

         // Compare ranges
         if (rangeToCompare < rangeToPlace)
            insertIndex++;
         else if (rangeToCompare > rangeToPlace)
            outRanged = false;
         else
         {
            // Ranges equal, so sort by position.  If it is behind, keep incrementing
            BPlane plane(orientForward, sqToCompare->getPosition());
            long result = plane.checkPoint(sqToPlace->getPosition());
            if (result == BPlane::cBehindPlane)
               insertIndex++;
            else
               outRanged = false;
         }
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // Now sort each row (2 squads) by position to minimize lateral criss-cross
   float totalDepth = 0.0f;
   long numRows = (mSquads.getNumber() + 1) / 2;
   for (long i = 0; i < numRows; i++)
   {
      BSquad *sq0 = gWorld->getSquad(mSquads[mTempSquadOrder[i * 2]]);
      BASSERT(sq0);
      float rowDepth = mResults[mTempSquadOrder[i * 2]].getDepth();

      // Make sure this isn't the last row with only 1 squad
      if ((i * 2) + 1 < mSquads.getNumber())
      {
         BSquad *sq1 = gWorld->getSquad(mSquads[mTempSquadOrder[(i * 2) + 1]]);
         BASSERT(sq1);
         rowDepth = max(rowDepth, mResults[mTempSquadOrder[i * 2 + 1]].getDepth());

         BPlane plane(orientRight, sq0->getPosition());
         long result = plane.checkPoint(sq1->getPosition());
         if (result == BPlane::cBehindPlane)
         {
            mTempSquadOrder.swapIndices(i * 2, i * 2 + 1);
         }
      }

      totalDepth += rowDepth;
   }

   BASSERT(mTempSquadOrder.getNumber() == mSquads.getNumber());
   BASSERT(mTempSquadOrder.getNumber() == mResults.getNumber());

   // Now determine positions / orientations
   // Make spaced positions / orientations about in box of 2 columns, n rows
   // HACK - temp hardcoded spacing stuff
   float singleSquadSpace = 3.0f;
   totalDepth += (numRows - 1) * singleSquadSpace;
   //-- Changed so that a the squad plotter plots from your cursor back instead of the cursor being the middle point of your group of squads
   //BVector desiredPosition = orientPosition + ((orientDistance - totalDepth * 0.5f) * -orientForward);
   BVector desiredPosition = orientPosition + (orientDistance * -orientForward);
   float currentRowDepth = 0.0f;
   float sideBit = 1.0f;
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      // -Update starting position
      // -Get depth of this row (only needs to be updated every
      //  2 squads (since we have 2 squads per row)
      if ((i % 2) == 0)
      {
         // Push back by row spacing for each row
         if (i != 0)
            desiredPosition += ((currentRowDepth + singleSquadSpace) * -orientForward);

         long squadIndex = mTempSquadOrder[i];
         currentRowDepth = mResults[squadIndex].getDepth();
         if (i + 1 < mSquads.getNumber())
         {
            squadIndex = mTempSquadOrder[i + 1];
            currentRowDepth = max(currentRowDepth, mResults[squadIndex].getDepth());
            sideBit = -1.0f;
         }
         else
            sideBit = 0.0f;
      }
      else
         sideBit = 1.0f;

      // Get width
      long squadIndex = mTempSquadOrder[i];
      float squadWidth = mResults[squadIndex].getWidth() + (2.0f * singleSquadSpace);

      /////////////////////////////////////////////////////////////////////////
      // Determine desired position
      mResults[squadIndex].setDesiredPosition(desiredPosition + (currentRowDepth * 0.5f * -orientForward) + 
                                                                (sideBit * 0.5f * squadWidth * orientRight));
      mResults[squadIndex].setDefaultDesiredPos(false);

      // Set waypoints
      // The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
      // The last waypoint is in the orientation of the formation.
      BVector orientDiff= orientForward;
      if (!orientDiff.safeNormalize())
         orientDiff = cXAxisVector;
      resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
      mResults[squadIndex].setWaypoints(resultWaypoints);
   }
}

//==============================================================================
// BSquadPlotter::makeColumnConfiguration
//==============================================================================
void BSquadPlotter::makeColumnConfiguration(float totalDepth, float orientDistance, BVector &orientForward,
                                            BVector &orientRight, BVector &orientPosition,
                                            BDynamicSimVectorArray &resultWaypoints, DWORD flags)
{
   totalDepth;
   orientRight;
   // Figure out order in columns
   // TODO - we may want some range-based checks here as opposed to distance

   // Given desired pos / orientation, figure out order in which to place squads
   // to minimize criss-cross.  We just do some simple plane sorting here.
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      float rangeToPlace = mResults[i].getRange();
      BSquad *sqToPlace = gWorld->getSquad(mSquads[i]);
      BASSERT(sqToPlace);

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool outRanged = true;
      while ((insertIndex < i) && outRanged)
      {
         float rangeToCompare = mResults[mTempSquadOrder[insertIndex]].getRange();
         BSquad *sqToCompare = gWorld->getSquad(mSquads[mTempSquadOrder[insertIndex]]);
         BASSERT(sqToCompare);

         // Compare ranges
         if (rangeToCompare < rangeToPlace)
            insertIndex++;
         else if (rangeToCompare > rangeToPlace)
            outRanged = false;
         else
         {
            // Ranges equal, so sort by position.  If it is behind, keep incrementing
            BPlane plane(orientForward, sqToCompare->getPosition());
            long result = plane.checkPoint(sqToPlace->getPosition());
            if (result == BPlane::cBehindPlane)
               insertIndex++;
            else
               outRanged = false;
         }
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // Make spaced positions / orientations about circle
   // HACK - temp hardcoded spacing stuff
   float rowSpacing = 1.0f;
   //float totalSpacedDepth = totalDepth + rowSpacing * (mSquads.getNumber() - 1);
   //BVector desiredPosition = orientPosition + ((orientDistance - (totalSpacedDepth * 0.5f)) * -orientForward);
   BVector desiredPosition = orientPosition + (orientDistance * -orientForward);
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      long squadIndex = mTempSquadOrder[i];
      float halfSquadDepth=mResults[squadIndex].getDepth()*0.5f;

      /////////////////////////////////////////////////////////////////////////
      // Determine desired position
      desiredPosition += (halfSquadDepth * -orientForward);
      
      // Set desired position
      mResults[squadIndex].setDesiredPosition(desiredPosition);
      mResults[squadIndex].setDefaultDesiredPos(false);

      // Set waypoints
      // The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
      // The last waypoint is in the orientation of the formation.
      BVector orientDiff= orientPosition - mResults[squadIndex].getDesiredPosition();
      orientDiff.normalize();
      resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
      mResults[squadIndex].setWaypoints(resultWaypoints);

      // Add space
      desiredPosition += ((halfSquadDepth + rowSpacing ) * -orientForward);
   }
}

//==============================================================================
// BSquadPlotter::makeSurroundPositionConfiguration
//==============================================================================
void BSquadPlotter::makeSurroundPositionConfiguration(BVector& orientForward, BVector& orientRight, BVector& orientPosition, 
                                                      BDynamicSimVectorArray& resultWaypoints, float minRange, BEntity* pTargetEntity, DWORD flags, 
                                                      BVector overrideTargetPos /*= cInvalidVector*/, float overrideRadius /*= 0.0f*/)
{
   // MPB TODOs
   // - Check out logical arc calcs b/c we may want to go ahead and let each specific
   //   range have it's own arc.  Also may want each squad size to have own arc
   //   to get better spacing.
   // - Split up arcs by movement type so obstruction checking can consider correct
   //   nodes

   // Resolve target position
   bool useOverrideTargetPos = !overrideTargetPos.almostEqual(cInvalidVector);
   BVector targetPos = cInvalidVector;
   if (useOverrideTargetPos)
   {
      targetPos = overrideTargetPos;
   }
   else if (pTargetEntity)
   {
      targetPos = pTargetEntity->getPosition();
   }
   else
   {
      BASSERTM(false, "No target position can be resolved!");
      return;
   }

   // Initialize the desired positions to the target position.  There are a few paths that
   // can return without setting each of the desired positions, which can cause problems later.   
   for (uint i = 0; i < mResults.getSize(); i++)
   {
      mResults[i].setDesiredPosition(targetPos);
      mResults[i].setDefaultDesiredPos(true);
   }

   //==============================================================================
   // Sort squads into logical arcs based on range / depth
   // Everything on the same arc doesn't have the same range, it just
   // means we can't fit a squad of lower range between the target and
   // the highest range squad on the same arc (because of squad formation depth)
   //==============================================================================
   // Sort squads from smallest to largest range
   mTempSquadOrder.clear();
   for (uint i = 0; i < static_cast<uint>(mSquads.getNumber()); i++)
   {
      BSquad* sq = gWorld->getSquad(mSquads[i]);
      
      if(!sq)
         continue;

      // If protoaction is NULL then we can't attack, set it to the target position and skip.
      // If an override range is set then still find a position.
      if((mResults[i].getProtoAction() == NULL) && !mResults[i].getUsingOverrideRange())
      {
         BVector newPos = targetPos;
         BVector currentPos = sq->getPosition();
         BVector forward = newPos - currentPos;
         forward.safeNormalize();
         
         mResults[i].setDesiredPosition(newPos);
         mResults[i].setDefaultDesiredPos(true);
         resultWaypoints[resultWaypoints.getNumber() - 2] = newPos;
         resultWaypoints[resultWaypoints.getNumber() - 1] = newPos + forward;
         mResults[i].setWaypoints(resultWaypoints);
         mResults[i].setObstructed(false);
         mResults[i].setInRange(false);
         continue;
      }

      // If squad is in hitAndRun mode (i.e. bowling), set it to the target position and skip
      if(sq->getSquadMode() == BSquadAI::cModeHitAndRun)
      {
         BSquadActionAttack* pSQA = reinterpret_cast<BSquadActionAttack*>(sq->getActionByType(BAction::cActionTypeSquadAttack));
         // TRB 6/24/08 - This used to make sure the attack action was in ramming state, but for short distances between
         // the attacker and target it was possible that the platoon would call the plotter before the attack action had gone
         // through its first update.  If some validation is needed maybe this could verify it has a ram proto action.
         if (pSQA)
         {
            BVector newPos = targetPos;
            BVector forward = newPos - sq->getPosition();
            forward.safeNormalize();

            // [4/21/2008 xemu] if we are overrunning, then overshoot the target to have some good speed 
            if (pSQA->getProtoAction() != NULL)
            {
               bool overrunHack = pSQA->getProtoAction()->getOverrun();
               if (overrunHack)
               {
                  BVector hackPos = forward;
                  hackPos.scale(gDatabase.getOverrunDistance());
                  newPos = newPos + hackPos;
               }
            }
            mResults[i].setDesiredPosition(newPos);
            mResults[i].setDefaultDesiredPos(true);
            resultWaypoints[resultWaypoints.getNumber() - 2] = newPos;
            resultWaypoints[resultWaypoints.getNumber() - 1] = newPos + forward;
            mResults[i].setWaypoints(resultWaypoints);
            mResults[i].setObstructed(false);
            mResults[i].setInRange(false);
            continue;
         }
      }

      // Test to see if squad is already in range and with LOS of target, if so set it's data and skip the rest
      if (!(flags & cSPFlagForceMove))
      {
         // Resolve distance
         float xzDistance = 0.0f;
         if (useOverrideTargetPos)
         {
            xzDistance = sq->calculateXZDistance(targetPos);
         }
         else
         {
            xzDistance = pTargetEntity->calculateXZDistance(targetPos, sq);
         }

         // first check for in range test.  Use the true range, not the reduced range
         if (xzDistance <= mResults[i].getTrueActionRange())
         {
            const BProtoAction* pProtoAction = mResults[i].getProtoAction();
            if (pTargetEntity && gConfig.isDefined(cConfigTrueLOS) && pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeUnitRangedAttack))
            {      
               // now check line of sight
//-- FIXING PREFIX BUG ID 2878
               const BSquad* pTargetSquad = NULL;
//--
               if (pTargetEntity->getUnit())
               {
                  pTargetSquad = pTargetEntity->getUnit()->getParentSquad();
               }
               else if (pTargetEntity->getSquad())
               {
                  pTargetSquad = pTargetEntity->getSquad();
               }

               if (pTargetSquad)
               {
                  bool hasLOS = gSquadLOSValidator.validateLOS(sq, mResults[i].getProtoAction(), pTargetSquad);

                  if (hasLOS)
                  {
                     mResults[i].setDesiredPosition(sq->getPosition());
                     mResults[i].setDefaultDesiredPos(false);
                     mResults[i].setInRange(true);
                     mResults[i].zeroWaypoints();
                     continue;
                  }
               }
            }
            else
            {
               mResults[i].setDesiredPosition(sq->getPosition());
               mResults[i].setDefaultDesiredPos(false);
               mResults[i].setInRange(true);
               mResults[i].zeroWaypoints();
               continue;
            }
         }
      }

      // Otherwise insert into ordered position
      uint insertIndex = 0;
      while ((insertIndex < static_cast<uint>(mTempSquadOrder.getNumber())) &&
             (mResults[i].getRange() > mResults[mTempSquadOrder[insertIndex]].getRange()))
      {
         insertIndex++;
      }
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // Early out if everything in range
   if (mTempSquadOrder.getNumber() < 1)
      return;

   //==============================================================================
   // Now that the squads are sorted by range, put them
   // into logical arcs.  You only add a new arc if the 
   // inner radius is greater than the outer radius (+ some spacing)
   // of the arc you're testing against.
   static BDynamicSimUIntArray arcs;                        // number of squads in each arc
   static BDynamicSimFloatArray arcOuterRadii;              // the outer radii of each arc
   static BDynamicSimFloatArray arcInnerRadii;              // the inner radii of each arc
   static BDynamicSimFloatArray maxSqrSquadSizePerArc;      // max squared diagonal length of a squad on each arc
   static BDynamicSimUIntArray largestSquadPerArc;          // index of largest squad per arc
   arcs.resize(0);
   arcOuterRadii.resize(0);
   arcInnerRadii.resize(0);
   maxSqrSquadSizePerArc.resize(0);
   largestSquadPerArc.resize(0);

   // Initialize arc data with 1st squad
   arcs.add(1);
   BSquadPlotterResult *pTempResult = &mResults[mTempSquadOrder[0]];
   float tempInnerRadii = pTempResult->getRange();
   float radiiToTest = tempInnerRadii + pTempResult->getDepth();
   arcOuterRadii.add(radiiToTest);
   arcInnerRadii.add(tempInnerRadii);
   largestSquadPerArc.add(mTempSquadOrder[0]);
   maxSqrSquadSizePerArc.add(pTempResult->getWidth() * pTempResult->getWidth() + pTempResult->getDepth() * pTempResult->getDepth());

   for (uint i = 1; i < static_cast<uint>(mTempSquadOrder.getNumber()); i++)
   {
      pTempResult = &mResults[mTempSquadOrder[i]];
      float radiiToSort = pTempResult->getRange();

      // Go through all arcs testing to see which one this gets added to
      bool squadPlaced = false;
      long arcIndex = 0;
      while (!squadPlaced && (arcIndex < arcs.getNumber()))
      {
         radiiToTest = arcOuterRadii[arcIndex];

         // If this squad's inner radius is greater than the outer radius of the arc we're testing
         // against, promote it to a further arc
         if (radiiToSort > radiiToTest)
         {
            arcIndex++;
         }
         // Otherwise, it gets added to the tested arc, and the outer radii potentially increases
         else
         {
            arcs[arcIndex] = arcs[arcIndex] + 1;

            float tempOuterRadii = pTempResult->getRange() + pTempResult->getDepth();
            arcOuterRadii[arcIndex] = max(arcOuterRadii[arcIndex], tempOuterRadii);

            tempInnerRadii = pTempResult->getRange();
            arcInnerRadii[arcIndex] = min(arcInnerRadii[arcIndex], tempInnerRadii);

            // Update largest squad index and size
            float squadSqrSize = pTempResult->getWidth() * pTempResult->getWidth() + pTempResult->getDepth() * pTempResult->getDepth();
            if (squadSqrSize > maxSqrSquadSizePerArc[arcIndex])
            {
               largestSquadPerArc[arcIndex] = mTempSquadOrder[i];
               maxSqrSquadSizePerArc[arcIndex] = squadSqrSize;
            }

            squadPlaced = true;
         }
      }

      // If existing arc not found, make a new one
      if (!squadPlaced)
      {
         arcs.add(1);

         float tempOuterRadii = pTempResult->getRange() + pTempResult->getDepth();
         arcOuterRadii.add(tempOuterRadii);

         tempInnerRadii = pTempResult->getRange();;
         arcInnerRadii.add(tempInnerRadii);

         // Add largest squad index and size
         largestSquadPerArc.add(mTempSquadOrder[i]);
         maxSqrSquadSizePerArc.add(pTempResult->getWidth() * pTempResult->getWidth() + pTempResult->getDepth() * pTempResult->getDepth());
      }
   }

   //==============================================================================
   // Now for each logical arc, place the squads on unobstructed surround positions
   uint arcOffset = 0;
   for (uint i = 0; i < static_cast<uint>(arcs.getNumber()); i++)
   {
      //==============================================================================
      // Calculate the surround positions for this target.  Use the
      // largest squad as reference so everything is spaced better.
      static BDynamicArray<BSurroundPosition> surroundPositions;
      float minRange = mResults[mTempSquadOrder[arcOffset]].getRange();
      long largestSquadIndex = largestSquadPerArc[i];

      calculateSurroundPositions(pTargetEntity, minRange, largestSquadIndex, surroundPositions, arcs[i], orientPosition, overrideTargetPos, overrideRadius);
      uint numTotalSurroundPositions = static_cast<uint>(surroundPositions.getNumber());
      uint numUsedSurroundPositions = Math::Min(numTotalSurroundPositions, arcs[i]);


      //==============================================================================
      // Sort all surround positions now from closest to farthest (this could be optimized by 
      // using a better sorting algorithm)
      static BDynamicSimLongArray positionSortOrderAll;
      static BDynamicSimFloatArray positionDist;
      positionSortOrderAll.clear();
      positionDist.clear();

      for (uint j = 0; j < numTotalSurroundPositions; j++)
      {
         uint insertIndex = 0;
         float posDist = orientPosition.distanceSqr(surroundPositions[j].mPosition);
         while ((insertIndex < static_cast<uint>(positionDist.getNumber())) && (posDist > positionDist[insertIndex]))
         {
            insertIndex++;
         }

         positionDist.insertAtIndex(posDist, insertIndex);
         positionSortOrderAll.insertAtIndex(j, insertIndex);
      }

      //==============================================================================
      // Set up ignore list with squads on this arc as they shouldn't obstruct potential squad plots
      static BEntityIDArray ignoreList;
      ignoreList.clear();
      for (uint j = 0; j < arcs[i]; j++)
      {
         BEntityID entityID = mSquads[mTempSquadOrder[arcOffset + j]];
         ignoreList.add(entityID);
//-- FIXING PREFIX BUG ID 2879
         const BSquad* pTempSq = gWorld->getSquad(entityID);
//--
         if (pTempSq)
            ignoreList.append(pTempSq->getChildList());
      }
      // DLM - for determining obstruction locations, ignore the target itself, so as to not allow us
      // to be forced to move off the target if we slightly overlap.  
      if (pTargetEntity)
      {
         ignoreList.add(pTargetEntity->getID());
         if (pTargetEntity->getClassType() == BEntity::cClassTypeSquad)
         {
            BSquad* pTargetSquad = reinterpret_cast<BSquad*>(pTargetEntity);
            const BEntityIDArray& myUnitList = pTargetSquad->getChildList();
            ignoreList.add(myUnitList.getPtr(), myUnitList.size());
         }
         // Conversely, if this is a unit, please add it's squad ID to the list as well..
         if ((pTargetEntity->getClassType() == BEntity::cClassTypeUnit) && pTargetEntity->getParent())
         {
            BSquad* pTargetSquad = reinterpret_cast<BSquad*>(pTargetEntity->getParent());
            ignoreList.add(pTargetSquad->getID());
         }
      }


      //==============================================================================
      // Create projectileGroups.  These are groups of units that are on the same
      // arc that share the same BProtoAction.  There are many parameters in the 
      // BProtoAction that control how the unit will fire (isAffectedByGravity,
      // maxProjectileHeight, maxRange), that need to be taken into account when
      // testing for true line of sight.
      static BDynamicArray<BSquadPlotterProjectileGroup> s_projectileGroups;
      s_projectileGroups.resize(0);
      
      for (uint j = 0; j < numUsedSurroundPositions; j++)
      {
         uint squadIndex = mTempSquadOrder[arcOffset + j];
         const BProtoAction* pProtoAction = mResults[squadIndex].getProtoAction();
         BSquadPlotterProjectileGroup* pFoundProjectileGroup = NULL;

         // Try to find a group with the same proto action to add the index to
         for(long z = s_projectileGroups.getNumber() - 1; z >= 0; z--)
         {
            if(pProtoAction == s_projectileGroups[z].getProtoAction())
            {
               pFoundProjectileGroup = &s_projectileGroups[z];
               break;
            }
         }

         // If we couldn't find an existing group create a new one
         if(!pFoundProjectileGroup)
         {
            BSquadPlotterProjectileGroup pNewProjectileGroup(pProtoAction);
            s_projectileGroups.add(pNewProjectileGroup);

            pFoundProjectileGroup = &s_projectileGroups[s_projectileGroups.getNumber()-1];
         }

         // Add the index to the corresponding group
         pFoundProjectileGroup->addSquadIndex(squadIndex);
      }


      //==============================================================================
      // Loop throw all projectile type groups
      for (long groupIdx = s_projectileGroups.getNumber() - 1; groupIdx >= 0; groupIdx--)
      {
         BSquadPlotterProjectileGroup* pProjectileGroup = &s_projectileGroups[groupIdx];
         uint numPositionsToOccupy = static_cast<uint>(pProjectileGroup->getSquadIndices().getNumber());


         //==============================================================================
         // Now go through the sorted list and create a new list of unobstructed positions.  A position can be
         // "obstructed" if there is something overlapping the position or if the position does not have line
         // of sight to the target.
         static BDynamicSimLongArray positionSortOrder;
         positionSortOrder.clear();

         calculateUnobstructedSurroundPositions(pTargetEntity, largestSquadIndex, ignoreList, surroundPositions, positionSortOrderAll, pProjectileGroup, positionSortOrder, flags);
         uint numTotalUnobstructedSurroundPositions = static_cast<uint>(positionSortOrder.getNumber());

         //==============================================================================
         // If there are more squads on this arc than valid surround positions, then
         // pick the squads closest to "orientPosition" as the ones to get the surround
         // positions.  The remaining squads will just be plotted at the target position.
         static BDynamicSimUIntArray validSurroundSquadIndices;
         static BDynamicSimUIntArray remainingSquadIndices;
         validSurroundSquadIndices.clear();
         remainingSquadIndices.clear();
         if (numTotalUnobstructedSurroundPositions < numPositionsToOccupy)
         {
            // Add squad distances and indices into the squadDistances and validSurroundIndices arrays
            // in order of increasing distance
            static BDynamicSimFloatArray squadDistances;
            squadDistances.clear();
            float squadDist = orientPosition.distanceSqr(gWorld->getSquad(mSquads[pProjectileGroup->getSquadIndices()[0]])->getPosition());
            squadDistances.add(squadDist);
            validSurroundSquadIndices.add(pProjectileGroup->getSquadIndices()[0]);
            for (uint j = 1; j < numPositionsToOccupy; j++)
            {
               uint insertIndex = 0;
               //squadDist = orientPosition.distanceSqr(gWorld->getSquad(mSquads[mTempSquadOrder[arcOffset + j]])->getPosition());;
               squadDist = orientPosition.distanceSqr(gWorld->getSquad(mSquads[pProjectileGroup->getSquadIndices()[j]])->getPosition());;
               while ((insertIndex < static_cast<uint>(squadDistances.getNumber())) && (squadDist > squadDistances[insertIndex]))
               {
                  insertIndex++;
               }
               squadDistances.insertAtIndex(squadDist, insertIndex);
               validSurroundSquadIndices.insertAtIndex(pProjectileGroup->getSquadIndices()[j], insertIndex);
            }

            // Crop valid array to number of surroundPositions and put cropped ones in remaining array
            remainingSquadIndices.insert(0, numPositionsToOccupy - numTotalUnobstructedSurroundPositions, &(validSurroundSquadIndices[numTotalUnobstructedSurroundPositions]));
            validSurroundSquadIndices.setNumber(numTotalUnobstructedSurroundPositions);
         }
         else
         {
            // If there are enough valid positions, just copy the array
            validSurroundSquadIndices.insert(0, numPositionsToOccupy, &(pProjectileGroup->getSquadIndices()[0]));
         }


         //==============================================================================
         // Sort arcs by positioning now to minimize criss-cross
         // Given desired pos / orientation, figure out order in which to place squads
         // to minimize criss-cross.  We just do some simple plane sorting here.
         // Sort both the surround positions and the current squads by position

         // Sort surround positions using planes
         for (uint j = 1; j < static_cast<uint>(positionSortOrder.getNumber()); j++)
         {
            BPlane comparePlane(orientRight, surroundPositions[positionSortOrder[j]].mPosition);
            uint tempIndex = positionSortOrder[j];
            uint compareIndex = j;
            while ((compareIndex > 0) && (comparePlane.checkPoint(surroundPositions[positionSortOrder[compareIndex - 1]].mPosition) == BPlane::cFrontOfPlane))
            {
               positionSortOrder[compareIndex] = positionSortOrder[compareIndex - 1];
               compareIndex--;
            }
            positionSortOrder[compareIndex] = tempIndex;
         }

         // Sort squads that get surround positions using planes
         for (uint j = 1; j < static_cast<uint>(validSurroundSquadIndices.getNumber()); j++)
         {
            BVector compareSquadPos = gWorld->getSquad(mSquads[validSurroundSquadIndices[j]])->getPosition();
            BPlane comparePlane(orientRight, compareSquadPos);
            uint tempIndex = validSurroundSquadIndices[j];
            uint compareIndex = j;
            while ((compareIndex > 0) && (comparePlane.checkPoint(gWorld->getSquad(mSquads[validSurroundSquadIndices[compareIndex - 1]])->getPosition()) == BPlane::cFrontOfPlane))
            {
               validSurroundSquadIndices[compareIndex] = validSurroundSquadIndices[compareIndex - 1];
               compareIndex--;
            }
            validSurroundSquadIndices[compareIndex] = tempIndex;
         }

         //==============================================================================
         // Do the actual positioning
         for (uint j = 0; j < static_cast<uint>(validSurroundSquadIndices.getNumber()); j++)
         {
            // Get new position and forward
            BVector newPos = surroundPositions[positionSortOrder[j]].mPosition;            
            BVector forward = targetPos - newPos;
            forward.safeNormalize();

            // Potentially adjust position forward as different sized units got spaced out
            // based on the largest squad and we need each at their own range + radius
            // TODO - moving this along the fwd vector is technically correct because
            // for rounded box immobile targets, the fwd isn't straight into the target.
            // We need to do something better for non-homogenous arcs anyway, because
            // smaller squads end up with too much spacing
            long squadIndex = validSurroundSquadIndices[j];
//-- FIXING PREFIX BUG ID 2880
            const BSquad* pSquad = gWorld->getSquad(mResults[squadIndex].getSquadID());
//--
            float range = mResults[squadIndex].getRange();
            float distToTarget = 0.0f;
            // Resolve distance
            if (useOverrideTargetPos)
            {
               distToTarget = pSquad->calculateXZDistance(newPos, targetPos);
            }
            else
            {
               distToTarget = pSquad->calculateXZDistance(newPos, pTargetEntity);
            }
            if (distToTarget > range)
               newPos += (forward * (distToTarget - range));

            // Set squad results with new position
            mResults[squadIndex].setDesiredPosition(newPos);
            mResults[squadIndex].setDefaultDesiredPos(false);
            resultWaypoints[resultWaypoints.getNumber() - 2] = newPos;
            resultWaypoints[resultWaypoints.getNumber() - 1] = newPos + forward;
            mResults[squadIndex].setWaypoints(resultWaypoints);
            mResults[squadIndex].setObstructed(false);
            mResults[squadIndex].setInRange(false);
         }
         for (uint j = 0; j < static_cast<uint>(remainingSquadIndices.getNumber()); j++)
         {
            // Get new position and forward            
            BVector newPos = targetPos;
            BVector currentPos = gWorld->getSquad(mSquads[remainingSquadIndices[j]])->getPosition();
            BVector forward = newPos - currentPos;
            forward.safeNormalize();

            // Set squad results with new position
            long squadIndex = remainingSquadIndices[j];
            mResults[squadIndex].setDesiredPosition(newPos);
            mResults[squadIndex].setDefaultDesiredPos(true);
            resultWaypoints[resultWaypoints.getNumber() - 2] = newPos;
            resultWaypoints[resultWaypoints.getNumber() - 1] = newPos + forward;
            mResults[squadIndex].setWaypoints(resultWaypoints);
            mResults[squadIndex].setObstructed(false);
            mResults[squadIndex].setInRange(false);
         }
      }

      // Advance arcOffset
      arcOffset += arcs[i];
   }
}

//==============================================================================
// BSquadPlotter::makeRangedArcConfiguration
//==============================================================================
void BSquadPlotter::makeRangedArcConfiguration(float orientDistance, BVector &orientForward, BVector &orientRight,
                                               BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, float minRange, BEntity *pTargetEntity, DWORD flags)
{
   orientDistance;

   ////////////////////////////////////////////////////////////////////////////
   // Sort squads into logical arcs based on range / depth
   // Everything on the same arc doesn't have the same range, it just
   // means we can't fit a squad of lower range between the target and
   // the highest range squad on the same arc (because of squad formation depth)

   ////////////////////////////////////////////////////////////////////////////
   // Sort squads from lowest inner radius to highest inner radius
   long numSquads = mSquads.getNumber();
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < numSquads; i++)
   {
      float innerRadiusToSort = mResults[i].getRange() - (mResults[i].getDepth() * 0.5f);

      // Increment insert index while this squad's inner radius larger than
      // those already inserted
      bool outDistanced = false;
      long insertIndex = 0;
      while ((insertIndex < i) && !outDistanced)
      {
         float innerRadiusToTest = mResults[mTempSquadOrder[insertIndex]].getRange() -
                                   (mResults[mTempSquadOrder[insertIndex]].getDepth() * 0.5f);
         if (innerRadiusToSort > innerRadiusToTest)
            insertIndex++;
         else
            outDistanced = true;
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   ////////////////////////////////////////////////////////////////////////////
   // Now that the squads are sorted by inner radius, put them
   // into logical arcs.  You only add a new arc if the 
   // inner radius is greater than the outer radius (+ some apacing)
   // of the arc you're testing against.
   static BDynamicSimLongArray arcs;       // number of squads in each arc
   static BDynamicSimFloatArray arcOuterRadii;  // the outer radii of each arc
   static BDynamicSimFloatArray arcInnerRadii;  // the inner radii of each arc
   arcs.clear();
   arcOuterRadii.clear();
   arcInnerRadii.clear();

   // Initialize arc data with 1st squad
   arcs.add(1);
   BSquadPlotterResult *pTempResult = &mResults[mTempSquadOrder[0]];
   float tempInnerRadii = pTempResult->getRange();
   float radiiToTest = tempInnerRadii + pTempResult->getDepth();
   arcOuterRadii.add(radiiToTest);
   arcInnerRadii.add(tempInnerRadii);

   for (long i = 1; i < numSquads; i++)
   {
      pTempResult = &mResults[mTempSquadOrder[i]];
      float radiiToSort = pTempResult->getRange();

      // Go through all arcs testing to see which one this gets added to
      bool squadPlaced = false;
      long arcIndex = 0;
      while (!squadPlaced && (arcIndex < arcs.getNumber()))
      {
         radiiToTest = arcOuterRadii[arcIndex];

         // If this squad's inner radius is greater than the outer radius of the arc we're testing
         // against, promote it to a further arc
         if (radiiToSort > radiiToTest)
         {
            arcIndex++;
         }
         // Otherwise, it gets added to the tested arc, and the outer radii potentially increases
         else
         {
            arcs[arcIndex] = arcs[arcIndex] + 1;

            float tempOuterRadii = pTempResult->getRange() + pTempResult->getDepth();
            arcOuterRadii[arcIndex] = max(arcOuterRadii[arcIndex], tempOuterRadii);

            tempInnerRadii = pTempResult->getRange();
            arcInnerRadii[arcIndex] = min(arcInnerRadii[arcIndex], tempInnerRadii);

            squadPlaced = true;
         }
      }

      // If existing arc not found, make a new one
      if (!squadPlaced)
      {
         arcs.add(1);

         float tempOuterRadii = pTempResult->getRange() + pTempResult->getDepth();
         arcOuterRadii.add(tempOuterRadii);

         tempInnerRadii = pTempResult->getRange();;
         arcInnerRadii.add(tempInnerRadii);
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // Sort arcs by positioning now to minimize criss-cross
   // Given desired pos / orientation, figure out order in which to place squads
   // to minimize criss-cross.  We just do some simple plane sorting here.
   // Do it in place (in mTempSquadOrder) so as not to mess up the arc sorting above
   long squadOrderIndex = 0;
   for (long i = 0; i < arcs.getNumber(); i++)
   {
      // Only do it if we have more than 1 squad in the arc
      if (arcs[i] > 1)
      {
         for (long j = 1; j < arcs[i]; j++)
         {
            long toPlaceIndex = j;
            long toTestIndex = j - 1;
            BSquad *sqToPlace = gWorld->getSquad(mSquads[mTempSquadOrder[squadOrderIndex + toPlaceIndex]]);
            BASSERT(sqToPlace);
            bool inFront = true;
            while (inFront && (toTestIndex >= 0))
            {
               // Squad to potentially swap with
               BSquad *sqToCompare = gWorld->getSquad(mSquads[mTempSquadOrder[squadOrderIndex + toTestIndex]]);
               BASSERT(sqToCompare);

               // Make plane
               BPlane plane(orientRight, sqToCompare->getPosition());

               // Check location of this squad wrt plane
               long result = plane.checkPoint(sqToPlace->getPosition());
               if (result == BPlane::cBehindPlane)
               {
                  // Swap
                  mTempSquadOrder.swapIndices(squadOrderIndex + toPlaceIndex, squadOrderIndex + toTestIndex);
                  toPlaceIndex = toTestIndex;
                  toTestIndex--;
               }
               else
                  inFront = false;
            }
         }
      }

      // Increment squad order index
      squadOrderIndex += arcs[i];
   }

   BASSERT(mTempSquadOrder.getNumber() == mSquads.getNumber());
   BASSERT(mTempSquadOrder.getNumber() == mResults.getNumber());

   ////////////////////////////////////////////////////////////////////////////
   // Now we're sorted into arcs, so do the actual placement now
   // Each squad will get placed at it's range along it's arc
   squadOrderIndex = 0;
   for (long i = 0; i < arcs.getNumber(); i++)
   {
      // Calculate starting angle by adding up angles for the
      // first half of squads + spacing.  Angles are calculated
      // based on atan of radius and squad widths.
      float singleSquadSpacing = 1.0f;
      float nextTheta = 0.0f;
      float arcRad = arcInnerRadii[i] + minRange;
      long midIndex = static_cast<long>(ceilf(arcs[i] * 0.5f));
      for (long j = 0; j < arcs[i]; j++)
      {
         //float squadWidth = mResults[mTempSquadOrder[squadOrderIndex + j]].getWidth();
         //float obsWidth = Math::Max(mResults[mTempSquadOrder[squadOrderIndex + j]].getWidth(), mResults[mTempSquadOrder[squadOrderIndex + j]].getDepth());
         float obsWidth = sqrtf(mResults[mTempSquadOrder[squadOrderIndex + j]].getWidth() * mResults[mTempSquadOrder[squadOrderIndex + j]].getWidth() + mResults[mTempSquadOrder[squadOrderIndex + j]].getDepth() * mResults[mTempSquadOrder[squadOrderIndex + j]].getDepth());
         if (arcs[i] > 1)
         {
            if ((j == 0) || ((j == midIndex - 1) && ((arcs[i] % 2) == 1)))
               nextTheta += atanf(((obsWidth + singleSquadSpacing) * 0.5f) / arcRad);
            else if (j < midIndex)
               nextTheta += atanf((obsWidth + singleSquadSpacing) / arcRad);
         }
      }

      // Place along arc at proper range
      for (long j = 0; j < arcs[i]; j++)
      {
         long squadIndex = mTempSquadOrder[squadOrderIndex + j];
         float halfSquadWidth=mResults[squadIndex].getWidth()*0.5f;
         float halfSquadDepth=mResults[squadIndex].getDepth()*0.5f;
         //float obsRadius = Math::Max(halfSquadWidth, halfSquadDepth);
         float obsRadius = sqrtf(halfSquadWidth * halfSquadWidth + halfSquadDepth * halfSquadDepth);
         BSquad *sq = gWorld->getSquad(mSquads[squadIndex]);
         BASSERT(sq);

         /////////////////////////////////////////////////////////////////////////
         // Determine desired position

         // Update next theta, subtracting off half squad width + spacing
         if (j != 0)
            nextTheta -= atanf((obsRadius + singleSquadSpacing * 0.5f) / arcRad);

         // First, check to see if we're in range.  If so, don't move
         BVector desiredPosition;
         bool inRange=false;
         //BVector sqPosition = sq->getFormation()->getAveragePosition();
         BVector sqPosition = sq->getPosition();
         if (!(flags & cSPFlagForceMove) && pTargetEntity && (pTargetEntity->calculateXZDistance(pTargetEntity->getPosition(), sq) <= mResults[squadIndex].getRange()))
         {
            desiredPosition = sqPosition;
            inRange=true;
         }
         /*if (sq->inRange(pTargetEntity, mResults[squadIndex].getRange()) == true)
         {
            desiredPosition = sq->getPosition();
            inRange=true;
         }*/
         // Otherwise find proper position on arc
         else
         {
            // Rotate -orientForward by theta and add scaled result to orientPosition
            BVector offsetVector = -orientForward;
            offsetVector.rotateXZ(nextTheta);
            float offsetDistance = mResults[squadIndex].getRange() + halfSquadDepth + minRange;
            desiredPosition = orientPosition + (offsetVector * offsetDistance);

            // Do extra distance calc to make sure we get within range of rectangular obstructions
            if (pTargetEntity)
            {
               float extraDist = (mResults[squadIndex].getRange() + halfSquadDepth) - pTargetEntity->calculateXZDistance(desiredPosition);
               // Only move closer if needed.  Without the sign check, this will also push units back to max range
               if (extraDist < 0.0f)
                  desiredPosition = desiredPosition + (offsetVector * extraDist);
            }
         }
         
         // Set desired position
         mResults[squadIndex].setDesiredPosition(desiredPosition);
         mResults[squadIndex].setDefaultDesiredPos(false);
         //Set in range.
         mResults[squadIndex].setInRange(inRange);

         // Set waypoints, if we're not already in range.  If we're already in range, 0 the waypoints out.
         if (inRange == false)
         {
            // The next to last result waypoint is the desired position.
            resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
            // The last waypoint is in the orientation of the formation.
            BVector orientDiff= orientPosition - mResults[squadIndex].getDesiredPosition();
            if (!orientDiff.safeNormalize())
               orientDiff = cXAxisVector;
            resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
            //resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition();
            mResults[squadIndex].setWaypoints(resultWaypoints);
         }
         else
            mResults[squadIndex].zeroWaypoints();
         
         // Advance next theta past this squad plus spacing
         nextTheta -= atanf((obsRadius + singleSquadSpacing * 0.5f) / arcRad);
      }

      // Increment squad order index
      squadOrderIndex += arcs[i];
   }
}

//==============================================================================
// BSquadPlotter::make2ColumnConfigurationTarget
//==============================================================================
void BSquadPlotter::make2ColumnConfigurationTarget(float orientDistance, BVector &orientForward, BVector &orientRight,
                                                   BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints,
                                                   float minRange, DWORD flags)
{
   // Sort by squad range
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      float rangeToSort = mResults[i].getRange();
      //BSquad *sqToPlace = gWorld->getSquad(mSquads[i]);

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool outRanged = true;
      while ((insertIndex < i) && outRanged)
      {
         float currentRange = mResults[mTempSquadOrder[insertIndex]].getRange();
         if (rangeToSort >= currentRange)
            insertIndex++;
         else
            outRanged = false;
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // Rows sorted, now sort columns
   // While we're here, get total depth (sum of max depth per row)
   // HACK - temp hardcoded spacing stuff
   float rowSpacing = 2.0f;
   float totalDepth = 0.0f;
   float firstRowDepth = 0.0f;
   float lastRowDepth = 0.0f;
   long numRows = (mSquads.getNumber() + 1) / 2;
   for (long i = 0; i < numRows; i++)
   {
      float rowDepth = mResults[mTempSquadOrder[i * 2]].getDepth();

      // Make sure this isn't the last row with only 1 squad
      if ((i * 2) + 1 < mSquads.getNumber())
      {
         rowDepth = max(rowDepth, mResults[mTempSquadOrder[i * 2 + 1]].getDepth());
      }

      // Determine first/last row depths
      if (i == 0)
         firstRowDepth = rowDepth;
      if (i == numRows - 1)
         lastRowDepth = rowDepth;

      // Add to total depth
      totalDepth += rowDepth;
   }

   // Now determine positions / orientations
   // Make spaced positions / orientations about in box of 2 columns, n rows
   float currentRowDepth = 0.0f;
   float sideBit = 1.0f;

   // Determine amount to back first row off.  This is at least minRange so it's not on top
   // of the unit.  We want to back off as much as we can while maximizing the number of squads
   // in range.  Once we figure that out, set the first desiredPosition
   float firstToLastRowDist = totalDepth - (firstRowDepth * 0.5f) - (lastRowDepth * 0.5f) + (rowSpacing * (numRows - 1));
   float minSquadRange = mResults[mTempSquadOrder[0]].getRange();
   float maxSquadRange = mResults[mTempSquadOrder[mSquads.getNumber() - 1]].getRange();
   float backOffAmount = minRange + max(0, min((maxSquadRange - firstToLastRowDist), minSquadRange));
   BVector desiredPosition = orientPosition + ((orientDistance + backOffAmount) * -orientForward);

   for (long i=0; i < mSquads.getNumber(); i++)
   {
      // -Update starting position
      // -Get depth of this row (only needs to be updated every
      //  2 squads (since we have 2 squads per row)
      if ((i % 2) == 0)
      {
         if (i != 0)
            desiredPosition += ((currentRowDepth + rowSpacing) * -orientForward);

         long squadIndex = mTempSquadOrder[i];
         currentRowDepth = mResults[squadIndex].getDepth();
         if (i + 1 < mSquads.getNumber())
         {
            squadIndex = mTempSquadOrder[i + 1];
            currentRowDepth = max(currentRowDepth, mResults[squadIndex].getDepth());
            sideBit = -1.0f;
         }
         else
            sideBit = 0.0f;
      }
      else
         sideBit = 1.0f;

      // Get width
      long squadIndex = mTempSquadOrder[i];
      float squadWidth = mResults[squadIndex].getWidth() + 2.0f;;

      /////////////////////////////////////////////////////////////////////////
      // Determine desired position
      mResults[squadIndex].setDesiredPosition(desiredPosition + (currentRowDepth * 0.5f * -orientForward) + 
                                                                (sideBit * 0.5f * squadWidth * orientRight));
      mResults[squadIndex].setDefaultDesiredPos(false);

      // Set waypoints
      // The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
      // The last waypoint is in the orientation of the formation.
      BVector orientDiff= orientPosition - mResults[squadIndex].getDesiredPosition();
      orientDiff.normalize();
      resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
      mResults[squadIndex].setWaypoints(resultWaypoints);

   }
}

//==============================================================================
// BSquadPlotter::makeColumnConfigurationTarget
//==============================================================================
void BSquadPlotter::makeColumnConfigurationTarget(float totalDepth, float orientDistance, BVector &orientForward,
                                                  BVector &orientRight, BVector &orientPosition,
                                                  BDynamicSimVectorArray &resultWaypoints, float minRange, DWORD flags)
{
   orientRight;

   // Sort by range
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      float rangeToSort = mResults[i].getRange();

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool outRanged = true;
      while ((insertIndex < i) && outRanged)
      {
         float currentRange = mResults[mTempSquadOrder[insertIndex]].getRange();
         if (rangeToSort >= currentRange)
            insertIndex++;
         else
            outRanged = false;
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   // HACK - temp hardcoded spacing stuff
   // Determine back off amount based on range
   float rowSpacing = 1.0f;
   float firstRowDepth = mResults[mTempSquadOrder[0]].getDepth();
   float lastRowDepth = mResults[mTempSquadOrder[mSquads.getNumber() - 1]].getDepth();
   float firstToLastRowDist = totalDepth - (firstRowDepth * 0.5f) - (lastRowDepth * 0.5f) + (rowSpacing * (mSquads.getNumber() - 1));
   float minSquadRange = mResults[mTempSquadOrder[0]].getRange();
   float maxSquadRange = mResults[mTempSquadOrder[mSquads.getNumber() - 1]].getRange();
   float backOffAmount = minRange + max(0, min(minSquadRange, (maxSquadRange - firstToLastRowDist)));

   // Make spaced positions / orientations in column
   //float squadSpacing = totalDepth * 0.25f;
   //float singleSquadSpace = 1.0f;//squadSpacing / (mSquads.getNumber() - 1);
   BVector desiredPosition = orientPosition + ((orientDistance + backOffAmount) * -orientForward);
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      long squadIndex = mTempSquadOrder[i];
      float halfSquadDepth=mResults[squadIndex].getDepth()*0.5f;

      /////////////////////////////////////////////////////////////////////////
      // Determine desired position
      desiredPosition += (halfSquadDepth * -orientForward);
      
      // Set desired position
      mResults[squadIndex].setDesiredPosition(desiredPosition);
      mResults[squadIndex].setDefaultDesiredPos(false);

      // Set waypoints
      // The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2] = mResults[squadIndex].getDesiredPosition();
      // The last waypoint is in the orientation of the formation.
      BVector orientDiff= orientPosition - mResults[squadIndex].getDesiredPosition();
      orientDiff.normalize();
      resultWaypoints[resultWaypoints.getNumber()-1] = mResults[squadIndex].getDesiredPosition() + orientDiff;
      mResults[squadIndex].setWaypoints(resultWaypoints);

      // Add space
      desiredPosition += ((halfSquadDepth + rowSpacing ) * -orientForward);
   }
}

//==============================================================================
// BSquadPlotter::makeRowConfiguration
//==============================================================================
void BSquadPlotter::makeOldRowConfiguration(float totalWidth, float orientDistance,
                                         BVector &orientForward, BVector &orientRight,
                                         BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints,
                                         BCommand *pCommand, DWORD flags)
{
   orientForward;
   orientDistance;

   // Given desired pos / orientation, figure out order in which to place squads
   // to minimize criss-cross.  We just do some simple plane sorting here.
   mTempSquadOrder.clear();
   mTempSquadOrder.add(0);
   for (long i = 1; i < mSquads.getNumber(); i++)
   {
      BSquad *sqToPlace = gWorld->getSquad(mSquads[i]);
      BASSERT(sqToPlace);

      // Increment insert index while this squad position is in front of
      // those already inserted
      long insertIndex = 0;
      bool inFront = true;
      while ((insertIndex < i) && inFront)
      {
         // Make plane
         BSquad *sq=gWorld->getSquad(mSquads[mTempSquadOrder[insertIndex]]);
         BASSERT(sq);
         BPlane plane(orientRight, sq->getPosition());

         // Check location of this squad wrt plane
         long result = plane.checkPoint(sqToPlace->getPosition());
         if (result == BPlane::cFrontOfPlane)
            insertIndex++;
         else
            inFront = false;
      }
      
      // Insert
      mTempSquadOrder.insertAtIndex(i, insertIndex);
   }

   //Init our iteration stuff.
   const BDynamicSimVectorArray &commandWaypoints = pCommand->getWaypointList();
   BVector iteratePosition=-orientRight;
   iteratePosition*=totalWidth/2.0f;
   iteratePosition+=commandWaypoints[commandWaypoints.getNumber()-1];
   //Go through the squads to figure out where they want to be placed.
   BVector averageDesiredPosition(0.0f);
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      //BSquad *sq=gWorld->getSquad(mSquads[mTempSquadOrder[i]]);
      //Do the position calc.
      BVector desiredPosition=orientRight;
      desiredPosition*=mResults[mTempSquadOrder[i]].getWidth()/2.0f;
      desiredPosition+=iteratePosition;
      //Save this.
      mResults[mTempSquadOrder[i]].setDesiredPosition(desiredPosition);
      mResults[mTempSquadOrder[i]].setDefaultDesiredPos(false);

      //Add this to the averaging total.
      averageDesiredPosition+=desiredPosition;

      //Increment the current position.
      BVector currentPositionIncrement=orientRight;
      currentPositionIncrement*=mResults[mTempSquadOrder[i]].getWidth();
      iteratePosition+=currentPositionIncrement;
   }
   averageDesiredPosition/=(float)mSquads.getNumber();

   //Now that we know where we want things to go, work some f'ed up mojo to fix that up.
   //Save the results, too.
   float desiredPositionScale=1.25f;
   for (long i=0; i < mSquads.getNumber(); i++)
   {
      //Figure out the desired position.
      BVector scaleDiff=mResults[i].getDesiredPosition()-averageDesiredPosition;
      scaleDiff*=desiredPositionScale;
      BVector newDesiredPosition=averageDesiredPosition+scaleDiff;
      mResults[i].setDesiredPosition(newDesiredPosition);
      mResults[i].setDefaultDesiredPos(false);

      //The next to last result waypoint is the desired position.
      resultWaypoints[resultWaypoints.getNumber()-2]=mResults[i].getDesiredPosition();
      //The last waypoint is in the orientation of the formation.
      BVector orientDiff=orientPosition-mResults[i].getDesiredPosition();
      orientDiff.normalize();
      resultWaypoints[resultWaypoints.getNumber()-1]=mResults[i].getDesiredPosition()+orientDiff;
      mResults[i].setWaypoints(resultWaypoints);
   }
}

//==============================================================================
// BSquadPlotter::checkAgainstObstructions
//==============================================================================
bool BSquadPlotter::checkAgainstObstructions(SQUAD_OBS_CHECK type)
{
   // Clear out temp obstructions
   clearTempObstructions();

   // Check obstructions against each squad and try to move off
   long numSquadsObstructed = 0;
   for (long i = 0; i < mTempSquadOrder.getNumber(); i++)
   {
      long sqIndex = mTempSquadOrder[i];
      //Skip this if we're in range already.
      if (mResults[sqIndex].getInRange() == true)
         continue;

      //-- If your range is tiny, and your only letting the squad move forward then you're going to end up on the wrong side of your target.
      SQUAD_OBS_CHECK tempType = type;
      if(mResults[sqIndex].getRange() <= cPushOffThreshold && type == cForwardOnly)
         tempType = cForwardAndBackward;

      if (!moveOffObstructions(sqIndex, tempType))
         numSquadsObstructed++;
   }

   // Clear out temp obstructions
   clearTempObstructions();

   if (numSquadsObstructed == 0)
      return true;
   else
      return false;
}

//==============================================================================
// BSquadPlotter::checkAgainstObstructions
//==============================================================================
bool BSquadPlotter::checkAgainstObstructions(BDynamicSimVectorArray &hullPoints, BObstructionNodePtrArray &obstructions, long movementType)
{
   // Check each squad placement against obstructions
   BSquad *sq = gWorld->getSquad(mSquads[0]);
   BASSERT(sq);
   long playerID = sq->getPlayerID();
   

   static BConvexHull hull;
   hull.clear();
   hull.addPoints(hullPoints.getData(), hullPoints.getNumber());

   long quadTreesToScan;
   switch(movementType)
   {
      case cMovementTypeLand: 
      case cMovementTypeHover: 
         quadTreesToScan=BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads |
            BObstructionManager::cIsNewTypeBlockLandUnits |
            BObstructionManager::cIsNewTypeDoppleganger;
         break;
      case cMovementTypeFlood: 
         quadTreesToScan=BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads |
            BObstructionManager::cIsNewTypeBlockFloodUnits |
            BObstructionManager::cIsNewTypeDoppleganger;
         break;
      case cMovementTypeScarab: 
         quadTreesToScan=BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads |
            BObstructionManager::cIsNewTypeBlockScarabUnits |
            BObstructionManager::cIsNewTypeDoppleganger;
         break;
      default:
         quadTreesToScan=0;
         break;
   }

   long nodeTypes = BObstructionManager::cObsNodeTypeAll;
   gObsManager.findObstructions(hull, 0.0f, quadTreesToScan, nodeTypes, playerID, false, obstructions);

   if (obstructions.getNumber() > 0)
      return false;
   else
      return true;
}

//==============================================================================
// BSquadPlotter::moveOffObstructions
//==============================================================================
bool BSquadPlotter::moveOffObstructions(long squadIndex, SQUAD_OBS_CHECK type)
{
   BASSERT((squadIndex >= 0) && (squadIndex < mResults.getNumber()));
   float width = mResults[squadIndex].getWidth();
   float depth = mResults[squadIndex].getDepth();
   BVector pos = mResults[squadIndex].getDesiredPosition();
   BDynamicSimVectorArray &waypoints = (BDynamicSimVectorArray&) mResults[squadIndex].getWaypoints();
   BVector forward = waypoints[waypoints.getNumber() - 1] - waypoints[waypoints.getNumber() - 2];
   forward.y = 0;
   if (!forward.safeNormalize())
      forward = cXAxisVector;

   long movementType=mResults[squadIndex].getMovementType();

   static BDynamicSimVectorArray points;
   makeQuadPoints(pos, forward, width, depth, points);

   static BObstructionNodePtrArray obstructions;
   bool clearOfObstructions = checkAgainstObstructions(points, obstructions, movementType);
   filterObstructions(obstructions);

   // If obstructed, go through wall-style obstruction checks to look
   // for valid location
   if (obstructions.getNumber() != 0)
   {
      BVector newPos;
      bool found = findClosestUnobstructedPosition(squadIndex, newPos, type);
      if (found)
      {
         BVector posDiff = newPos - pos;

         // Make new placement
         // Test new position and set obstructed state
         pos += posDiff;
         makeQuadPoints(pos, forward, width, depth, points);
         checkAgainstObstructions(points, obstructions, movementType);
         filterObstructions(obstructions);
         clearOfObstructions = obstructions.getNumber() == 0;

         // Set squad results with new position
         mResults[squadIndex].setDesiredPosition(pos);
         mResults[squadIndex].setDefaultDesiredPos(false);
         waypoints[waypoints.getNumber() - 2] += posDiff;
         waypoints[waypoints.getNumber() - 1] += posDiff;
         mResults[squadIndex].setWaypoints(waypoints);
      }
   }

   // Set obstructed state
   mResults[squadIndex].setObstructed(!clearOfObstructions);

   // Add temporary obstruction
   addTempObstruction(pos, width, depth, forward);

   return true;
}

//==============================================================================
// BSquadPlotter::findClosestUnobstructedPosition
//==============================================================================
bool BSquadPlotter::findClosestUnobstructedPosition(long squadIndex, BVector &newPos, SQUAD_OBS_CHECK type)
{
   // Get squad info
   float width = mResults[squadIndex].getWidth();
   BVector pos = mResults[squadIndex].getDesiredPosition();
   BDynamicSimVectorArray &waypoints = (BDynamicSimVectorArray&) mResults[squadIndex].getWaypoints();
   BVector forward = waypoints[waypoints.getNumber() - 1] - waypoints[waypoints.getNumber() - 2];
   forward.y = 0;
   if (!forward.safeNormalize())
      forward = cXAxisVector;

   long movementType=mResults[squadIndex].getMovementType();

   // Call find function
   return findClosestUnobstructedPosition(pos, forward, width, newPos, type, movementType);
}

//==============================================================================
// BSquadPlotter::findClosestUnobstructedPosition
//==============================================================================
bool BSquadPlotter::findClosestUnobstructedPosition(BVector &pos, BVector &forward,
                                                    float width, BVector &newPos, SQUAD_OBS_CHECK type, long movementType)
{
   float currentDist = FLT_MAX;
   bool foundNewPos = false;
   long quadTreesToScan;

   switch(movementType)
   {
      case cMovementTypeLand: 
      case cMovementTypeHover: 
         quadTreesToScan=BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads |
            BObstructionManager::cIsNewTypeBlockLandUnits |
            BObstructionManager::cIsNewTypeDoppleganger;
         break;
      case cMovementTypeFlood: 
         quadTreesToScan=BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads |
            BObstructionManager::cIsNewTypeBlockFloodUnits |
            BObstructionManager::cIsNewTypeDoppleganger;
         break;
      case cMovementTypeScarab: 
         quadTreesToScan=BObstructionManager::cIsNewTypeAllCollidableUnits |
            BObstructionManager::cIsNewTypeAllCollidableSquads |
            BObstructionManager::cIsNewTypeBlockScarabUnits |
            BObstructionManager::cIsNewTypeDoppleganger;
         break;
      default:
         quadTreesToScan=0;
         break;
   }

   // Get playerID
   BSquad *sq = gWorld->getSquad(mSquads[0]);
   BASSERT(sq);
   long playerID = sq->getPlayerID();

   // Open up the ObMgr..
   bool canJump = false;  //We don't want to ignore jump obstructions when finding the unobstructed position in the plotter.
   gObsManager.begin(BObstructionManager::cBeginEntity, width * 0.5f,
      quadTreesToScan, BObstructionManager::cObsNodeTypeAll, 0, 
      0.8f, NULL, canJump);

   // Do wall check
   BVector testPos = pos;
   testPos.y       = 0.0f;
   static BWallCheckResultArray wallCheckResults;
   wallCheckResults.clear();
   BVector startPt, endPt;
   switch (type)
   {
      case cForwardOnly:
         startPt = testPos;
         endPt   = testPos + ( forward * 20.0f );
         break;
      case cBackwardOnly:
         startPt = testPos - ( forward * 20.0f );
         endPt   = testPos;
         break;
      case cForwardAndBackward:
         startPt = testPos - ( forward * 20.0f );
         endPt   = testPos + ( forward * 20.0f );
         break;
   }
   long inside = gObsManager.performNewWallCheck(startPt, endPt, playerID, wallCheckResults, false);

   // No obstructions, just keep where it is
   if (wallCheckResults.getNumber() <= 0)
   {
      newPos = pos;

      // Shut down the obstruction manager..
      gObsManager.end();
      return true;
   }

   // Get closest point to place
   for (long i = 0; i < wallCheckResults.getNumber(); i++)
   {
      BWallCheckResult &wallObstruction = wallCheckResults[i];
      wallObstruction.mPoint.y = 0.0f;

      // See if this point is where we enter or leave an obstruction.
      if (wallObstruction.mIn)
      {
         // If we're entering and we were outside all obstructions, add segment.
         // Also, only do this if the segment isn't going backwards.
         if (inside == 0 && ((wallObstruction.mPoint - startPt).dot(forward)) > 0.0f)
         {
            // Get closest point to ideal location and update newPos
            BVector tempPos = closestPointOnLine(startPt, wallObstruction.mPoint, pos);
            float tempDist = pos.xzDistanceSqr(tempPos);
            if (tempDist < currentDist)
            {
               newPos = tempPos;
               currentDist = tempDist;
               foundNewPos = true;
            }

            // Debug stuff
            #ifdef DEBUG_SQUAD_OBSTRUCT
               debugSquadPoints.add(startPt);
               debugSquadPoints.add(wallObstruction.mPoint);
            #endif

            // Update new start point for next unobstructed segment
            startPt = wallObstruction.mPoint;
         }

         // Increase inside count.
         inside++;
      }
      else
      {
         // Decrease inside count.
         inside--;

         // If we're exiting all obstructions, set startpoint.
         if (inside == 0)
            startPt = wallObstruction.mPoint;
      }
   }

   // Add last segment if needed and the segment is not pointing backwards.
   if (!inside && ((endPt - startPt).dot(forward)) > 0.0f /*&& !foundForward*/)
   {
      // Set new position if closer
      BVector tempPos = closestPointOnLine(startPt, endPt, pos);
      float tempDist = pos.xzDistanceSqr(tempPos);
      if (tempDist < currentDist)
      {
         newPos = tempPos;
         foundNewPos = true;
      }

      // Debug stuff
      #ifdef DEBUG_SQUAD_OBSTRUCT
         debugSquadPoints.add(startPt);
         debugSquadPoints.add(endPt);
      #endif
   }

   // Shut down the obstruction manager..
   gObsManager.end();

   if( foundNewPos )
   {
      newPos.y = pos.y;
   }

   return foundNewPos;
}

//==============================================================================
//==============================================================================
void BSquadPlotter::calculateSurroundPositions(BEntity* pTargetEntity, float range, long largestSquadIndex, BDynamicArray<BSurroundPosition>& surroundPositions, uint numSquads, BVector orientPosition, BVector overrideTargetPos /*= cInvalidVector*/, float overrideRadius /*= 0.0f*/)
{
//-- FIXING PREFIX BUG ID 2891
   const BSquad* pRefSquad = gWorld->getSquad(mResults[largestSquadIndex].getSquadID());
//--
   if (!pRefSquad)
      return;   
      
   bool useOverrideTargetPos = !overrideTargetPos.almostEqual(cInvalidVector);
   if (!pTargetEntity && !useOverrideTargetPos)
      return;

   // Resolve target position, and target radius
   BVector testTargetPos = cInvalidVector;
   float testTargetRadius = 0.0f;   
   if (useOverrideTargetPos)
   {
      testTargetPos = overrideTargetPos;
      testTargetRadius = overrideRadius;
   }
   else if (pTargetEntity)
   {
      testTargetPos = pTargetEntity->getPosition();
      testTargetRadius = pTargetEntity->getObstructionRadius();
   }
   else
   {
      BASSERTM(false, "No valid target data can be resolved!");
      return;
   }

   surroundPositions.resize(0);

   //==============================================================================
   // Circular range
   if ((pTargetEntity && pTargetEntity->isEverMobile()) || useOverrideTargetPos)
   {
      // Debug rendering for target radius and range
      #ifndef BUILD_FINAL
         debugSurroundPositions.add(BDebugSurroundPosition(testTargetPos, testTargetRadius, testTargetRadius + range, BDebugSurroundPosition::ObstructionUnchecked));
      #endif

      //==============================================================================
      // Calculate number of squads that can fit at range
      float targetRadius = testTargetRadius;
      float innerRadius = pRefSquad->getObstructionRadius();
      float outerRadius = sqrtf(pRefSquad->getObstructionRadiusX() * pRefSquad->getObstructionRadiusX() + pRefSquad->getObstructionRadiusZ() * pRefSquad->getObstructionRadiusZ());
      float totalCircleLength = (targetRadius + range + innerRadius) * cTwoPi;
      uint numAvailableSpots = static_cast<uint>(floor(totalCircleLength / (outerRadius * 2.0f)));
      float spacing = (totalCircleLength - (numAvailableSpots * outerRadius * 2.0f)) / numAvailableSpots;

      //==============================================================================
      // Calc all positions at range
      BVector dir = orientPosition - testTargetPos;
      dir.safeNormalize();
      dir *= (targetRadius + range + innerRadius);
      float rotAmt = (outerRadius * 2.0f + spacing) / (targetRadius + range + innerRadius); // amount to rotate for next squad position
      for (uint i = 0; i < numAvailableSpots; i++)
      {
         // Calc position
         BVector pos = testTargetPos + dir;

         surroundPositions.add(BSurroundPosition(pos));

         // Debug rendering of surround positions
         #ifndef BUILD_FINAL
            debugSurroundPositions.add(BDebugSurroundPosition(pos, innerRadius, outerRadius, BDebugSurroundPosition::ObstructionUnchecked));
         #endif

         // Advance rotation for next position
         dir.rotateXZ(rotAmt);
      }
   }
   //==============================================================================
   // Rounded box range
   else if (pTargetEntity)
   {
      // Debug rendering of rounded range boxes
      #ifndef BUILD_FINAL
         pDebugImmobileTargetEntity = pTargetEntity;
         debugImmobileTargetRanges.add(range);
      #endif

      //==============================================================================
      // Calc total rounded box length
      float innerRadius = pRefSquad->getObstructionRadius();
      float outerRadius = sqrtf(pRefSquad->getObstructionRadiusX() * pRefSquad->getObstructionRadiusX() + pRefSquad->getObstructionRadiusZ() * pRefSquad->getObstructionRadiusZ());
      float totalRangedBoxLength = (pTargetEntity->getObstructionRadiusX() * 4.0f) + (pTargetEntity->getObstructionRadiusZ() * 4.0f) + (cTwoPi * (range + innerRadius));
      uint numAvailableSpots = static_cast<uint>(floor(totalRangedBoxLength / (outerRadius * 2.0f)));
      float spacing = (totalRangedBoxLength - (numAvailableSpots * outerRadius * 2.0f)) / numAvailableSpots;

      BVector frCorner = testTargetPos + (pTargetEntity->getForward() * pTargetEntity->getObstructionRadiusZ()) + (pTargetEntity->getRight() * pTargetEntity->getObstructionRadiusX());
      BVector brCorner = testTargetPos + (-pTargetEntity->getForward() * pTargetEntity->getObstructionRadiusZ()) + (pTargetEntity->getRight() * pTargetEntity->getObstructionRadiusX());
      BVector blCorner = testTargetPos + (-pTargetEntity->getForward() * pTargetEntity->getObstructionRadiusZ()) + (-pTargetEntity->getRight() * pTargetEntity->getObstructionRadiusX());
      BVector flCorner = testTargetPos + (pTargetEntity->getForward() * pTargetEntity->getObstructionRadiusZ()) + (-pTargetEntity->getRight() * pTargetEntity->getObstructionRadiusX());

      // fb side, corner, lr side, corner, fb side, corner, lr side, corner
      float sideLengths[8];
      sideLengths[0] = sideLengths[4] = pTargetEntity->getObstructionRadiusX() * 2.0f;
      sideLengths[2] = sideLengths[6] = pTargetEntity->getObstructionRadiusZ() * 2.0f;
      sideLengths[1] = sideLengths[3] = sideLengths[5] = sideLengths[7] = cPiOver2 * (range + innerRadius);

      //==============================================================================
      // Determine start offset (orient position's clockwise distance from front left corner).  Additional offset
      // for even number of squads
      float startOffset = calculateDistanceAroundRoundedBox(pTargetEntity, flCorner, frCorner, brCorner, blCorner, range + innerRadius, orientPosition);
      if ((numSquads % 2) == 0)
         startOffset -= outerRadius;
      if (startOffset < 0.0)
         startOffset += totalRangedBoxLength;

      //==============================================================================
      // Calc all positions at range
      for (uint i = 0; i < numAvailableSpots; i++)
      {
         // Determine which side or corner to place
         float offset = i * (outerRadius * 2.0f + spacing) + startOffset;
         offset = fmod(offset, totalRangedBoxLength);
         uint sideIndex = 0;
         while (offset > sideLengths[sideIndex])
         {
            offset -= sideLengths[sideIndex];
            sideIndex++;
         }
         sideIndex = sideIndex % 8;

         BVector pos;
         BVector dir;
         switch (sideIndex)
         {
            case 0:
               pos = flCorner + (pTargetEntity->getRight() * offset) + (pTargetEntity->getForward() * (range + innerRadius));
               break;
            case 1:
               dir = pTargetEntity->getForward();
               dir.rotateXZ(offset / (range + innerRadius));
               pos = frCorner + (dir * (range + innerRadius));
               break;
            case 2:
               pos = frCorner - (pTargetEntity->getForward() * offset) + (pTargetEntity->getRight() * (range + innerRadius));
               break;
            case 3:
               dir = pTargetEntity->getRight();
               dir.rotateXZ(offset / (range + innerRadius));
               pos = brCorner + (dir * (range + innerRadius));
               break;
            case 4:
               pos = brCorner - (pTargetEntity->getRight() * offset) - (pTargetEntity->getForward() * (range + innerRadius));
               break;
            case 5:
               dir = -pTargetEntity->getForward();
               dir.rotateXZ(offset / (range + innerRadius));
               pos = blCorner + (dir * (range + innerRadius));
               break;
            case 6:
               pos = blCorner + (pTargetEntity->getForward() * offset) - (pTargetEntity->getRight() * (range + innerRadius));
               break;
            case 7:
               dir = -pTargetEntity->getRight();
               dir.rotateXZ(offset / (range + innerRadius));
               pos = flCorner + (dir * (range + innerRadius));
               break;
         }

         surroundPositions.add(pos);

         // Debug rendering of surround positions
         #ifndef BUILD_FINAL
            debugSurroundPositions.add(BDebugSurroundPosition(pos, innerRadius, outerRadius, BDebugSurroundPosition::ObstructionUnchecked));
         #endif

      }
   }
}


//==============================================================================
//==============================================================================
void BSquadPlotter::calculateUnobstructedSurroundPositions(BEntity* pTargetEntity, long largestSquadIndex, const BEntityIDArray& ignoreList, BDynamicArray<BSurroundPosition>& surroundPositions, const BDynamicSimLongArray& positionSortOrderAll, const BSquadPlotterProjectileGroup* pProjectileGroup, BDynamicSimLongArray& positionSortOrder, DWORD flags)
{
   // Look through the sorted of positions and fill a new list of unobstructed positions.  A position can be
   // "obstructed" if there is something overlapping the position or if the position does not have line
   // of sight to the target.  Exit once we have the number of positions that we need (numUsedSurroundPositions).

//-- FIXING PREFIX BUG ID 2893
   const BSquad* pRefSquad = gWorld->getSquad(mResults[largestSquadIndex].getSquadID());
//--
   if (!pRefSquad)
      return;

   BPlayerID playerID = pRefSquad->getPlayerID();
   long quadTreesToScan = /*BObstructionManager::cIsNewTypeAllCollidableUnits |
                          BObstructionManager::cIsNewTypeAllCollidableSquads |
                          */
                          //BObstructionManager::cIsNewTypeCollidableStationaryUnit | 
                          // DLM - we need to check against moving squads and nonmoving squads.  But only
                          // nonmoveable units.  
                          BObstructionManager::cIsNewTypeAllCollidableSquads | 
                          BObstructionManager::cIsNewTypeCollidableNonMovableUnit | BObstructionManager::cIsNewTypeBlockAirMovement |
                          BObstructionManager::cIsNewTypeBlockLandUnits |
                          BObstructionManager::cIsNewTypeDoppleganger;
   long nodeTypes = BObstructionManager::cObsNodeTypeAll;

   float innerRadius = pRefSquad->getObstructionRadius();
   bool canJump = false; //We don't want any end positions inside a jump obstruction
   gObsManager.begin(BObstructionManager::cBeginEntity, 0.0f, quadTreesToScan, nodeTypes, playerID, 0.0f, &ignoreList, canJump);

   uint numPositionsToOccupy = static_cast<uint>(pProjectileGroup->getSquadIndices().getNumber());
   for (uint i = 0; i < numPositionsToOccupy; i++)
   {
      const BSquad* pSourceSquad = gWorld->getSquad(mResults[pProjectileGroup->getSquadIndices()[i]].getSquadID());
      const BProtoAction* pProtoAction = mResults[pProjectileGroup->getSquadIndices()[i]].getProtoAction();
      bool isAir = (pSourceSquad->getProtoObject() && !(flags & BSquadPlotter::cSPFlagForceLandMovementType)) ? (pSourceSquad->getProtoObject()->getMovementType() == cMovementTypeAir) : false;

      uint numTotalSurroundPositions = static_cast<uint>(surroundPositions.getNumber());
      for (uint j = 0; j < numTotalSurroundPositions; j++)
      {
         // Skip occupy positions
         if(surroundPositions[positionSortOrderAll[j]].mbIsOccupied)
            continue;

         // Skip ObstructionManager obstructed position
         if(surroundPositions[positionSortOrderAll[j]].mbIsObstructed)
            continue;

         BVector pos = surroundPositions[positionSortOrderAll[j]].mPosition;
         bool obstructed = false;
         // Check for ObjstructionManager obstruction
         if(!isAir)
         {
            obstructed = gObsManager.testObstructions(pos, innerRadius, 0.0f, quadTreesToScan, nodeTypes, playerID);
            if(obstructed)
            {
               // Debug rendering of surround positions
               #ifndef BUILD_FINAL
                  debugSurroundPositions.add(BDebugSurroundPosition(pos, innerRadius - 0.3f, innerRadius, BDebugSurroundPosition::ObstructionFailed));
               #endif

               surroundPositions[positionSortOrderAll[j]].mbIsObstructed = true;
               continue;
            }
         }

         // Check for LOS obstruction
         if (!(flags & BSquadPlotter::cSPFlagIgnoreWeaponLOS) && pTargetEntity && gConfig.isDefined(cConfigTrueLOS) && pProtoAction && (pProtoAction->getActionType() == BAction::cActionTypeUnitRangedAttack))
         {
//-- FIXING PREFIX BUG ID 2892
            const BSquad* pTargetSquad = NULL;
//--
            if (pTargetEntity->getUnit())
            {
               pTargetSquad = pTargetEntity->getUnit()->getParentSquad();
            }
            else if (pTargetEntity->getSquad())
            {
               pTargetSquad = pTargetEntity->getSquad();
            }

            if (pTargetSquad)
            {
               // Need to plant the position to the ground height or air height depending on unit type, else the position's Y value will be invalid
               if (isAir)
               {
                  // Hard code this for now since this is changing next week.
                  gTerrainSimRep.getHeightRaycast(pos, pos.y, true);
                  pos.y += 16.0f;
               }
               else
               {
                  gTerrainSimRep.getHeightRaycast(pos, pos.y, true);
               }

               obstructed = !gSquadLOSValidator.validateLOS(pos, pTargetSquad->getPosition(), pSourceSquad, pProtoAction, pTargetSquad, &ignoreList);
               if (obstructed)
               {
                  // Debug rendering of surround positions
                  #ifndef BUILD_FINAL
                     debugSurroundPositions.add(BDebugSurroundPosition(pos, innerRadius - 0.3f, innerRadius, BDebugSurroundPosition::ObstructionFailed));
                  #endif
                  continue;
               }
            }
         }

         positionSortOrder.add(positionSortOrderAll[j]);

         // Occupy this position
         surroundPositions[positionSortOrderAll[j]].mbIsOccupied = true;

         // Debug rendering of surround positions
         #ifndef BUILD_FINAL
            debugSurroundPositions.add(BDebugSurroundPosition(pos, innerRadius - 0.3f, innerRadius, BDebugSurroundPosition::ObstructionPassed));
         #endif

         break;
      }
   }

   gObsManager.end();
}

//==============================================================================
//==============================================================================
float BSquadPlotter::calculateDistanceAroundRoundedBox(BEntity* pTargetEntity, BVector flCorner, BVector frCorner, BVector brCorner, BVector blCorner, float range, BVector orientPosition)
{
   // This function finds the closest point on the ranged rounded box to orientPosition.
   // It then calculates the distance along the rounded box from the fl corner to that
   // closest point.

   //   fl   fm    fr
   //      ______            fwd
   //     /      \            _
   //     |      |           /|\
   //  cl |  cm  | cr         |
   //     |      |            |
   //     \______/
   //
   //   bl   bm    br

   //==============================================================================
   // Determine rounded box section
   enum LMR
   {
      cLeft,
      cMiddle,
      cRight
   };
   enum FCB
   {
      cFront,
      cCenter,
      cBack
   };

   LMR lmrPos = cMiddle;
   FCB fcbPos = cCenter;
   if (pointOnPlusSideOfLine(orientPosition.x, orientPosition.z, frCorner, pTargetEntity->getRight()))
      lmrPos = cRight;
   else if (pointOnPlusSideOfLine(orientPosition.x, orientPosition.z, flCorner, -pTargetEntity->getRight()))
      lmrPos = cLeft;

   if (pointOnPlusSideOfLine(orientPosition.x, orientPosition.z, frCorner, pTargetEntity->getForward()))
      fcbPos = cFront;
   else if (pointOnPlusSideOfLine(orientPosition.x, orientPosition.z, blCorner, -pTargetEntity->getForward()))
      fcbPos = cBack;

   // Given section, now calc distance along rounded box
   float dist = 0.0f;
   if (lmrPos == cLeft)
   {
      // Front left
      if (fcbPos == cFront)
      {
         BVector dir = orientPosition - flCorner;
         float angle = dir.angleBetweenVector(-pTargetEntity->getRight());
         float flOffset = pTargetEntity->getObstructionRadiusX() * 4.0f + pTargetEntity->getObstructionRadiusZ() * 4.0f + range * cThreePiOver2;
         dist = flOffset + angle * range;
      }
      // Back left
      else if (fcbPos == cBack)
      {
         BVector dir = orientPosition - blCorner;
         float angle = dir.angleBetweenVector(-pTargetEntity->getForward());
         float blOffset = pTargetEntity->getObstructionRadiusX() * 4.0f + pTargetEntity->getObstructionRadiusZ() * 2.0f + range * cPi;
         dist = blOffset + angle * range;
      }
      // Center left
      else
      {
         BVector closestPt = closestPointOnLine(blCorner, flCorner, orientPosition);
         float distAlongLine = blCorner.distance(closestPt);
         float clOffset = pTargetEntity->getObstructionRadiusX() * 4.0f + pTargetEntity->getObstructionRadiusZ() * 2.0f + range * cThreePiOver2;
         dist = clOffset + distAlongLine;
      }
   }
   else if (lmrPos == cRight)
   {
      // Front right
      if (fcbPos == cFront)
      {
         BVector dir = orientPosition - frCorner;
         float angle = dir.angleBetweenVector(pTargetEntity->getForward());
         float frOffset = pTargetEntity->getObstructionRadiusX() * 2.0f;
         dist = frOffset + angle * range;
      }
      // Back right
      else if (fcbPos == cBack)
      {
         BVector dir = orientPosition - brCorner;
         float angle = dir.angleBetweenVector(pTargetEntity->getRight());
         float brOffset = pTargetEntity->getObstructionRadiusX() * 2.0f + pTargetEntity->getObstructionRadiusZ() * 2.0f + range * cPiOver2;
         dist = brOffset + angle * range;
      }
      // Center right
      else
      {
         BVector closestPt = closestPointOnLine(frCorner, brCorner, orientPosition);
         float distAlongLine = frCorner.distance(closestPt);
         float crOffset = pTargetEntity->getObstructionRadiusX() * 2.0f + range * cPiOver2;
         dist = crOffset + distAlongLine;
      }
   }
   else
   {
      // Front middle
      if (fcbPos == cFront)
      {
         BVector closestPt = closestPointOnLine(flCorner, frCorner, orientPosition);
         float distAlongLine = flCorner.distance(closestPt);
         dist = distAlongLine;
      }
      // Back middle
      else if (fcbPos == cBack)
      {
         BVector closestPt = closestPointOnLine(brCorner, blCorner, orientPosition);
         float distAlongLine = brCorner.distance(closestPt);
         float bmOffset = pTargetEntity->getObstructionRadiusX() * 2.0f + pTargetEntity->getObstructionRadiusZ() * 2.0f + range * cPi;
         dist = bmOffset + distAlongLine;
      }
      // Otherwise thi is center middle - inside box.  Just use 0.0f for now
      else
         dist = 0.0f;
   }

   return dist;
}

//==============================================================================
// BSquadPlotter::makeQuadPoints
//==============================================================================
void BSquadPlotter::makeQuadPoints(BVector &pos, BVector& forward, float width, float depth, BDynamicSimVectorArray &points)
{
   BVector right = cYAxisVector.cross(forward);
   points.setNumber(4);
   points[0] = pos + (forward * (depth * 0.5f)) + (right * (width * 0.5f));
   points[1] = points[0] - (forward * depth);
   points[2] = points[1] - (right * width);
   points[3] = points[2] + (forward * depth);
}

//==============================================================================
// BSquadPlotter::filterObstructions
//==============================================================================
void BSquadPlotter::filterObstructions(BObstructionNodePtrArray &obstructions)
{
   // Remove invalid obstructions or obstructions that are units
   // in the selected squads
   for (long n = obstructions.getNumber() - 1; n >= 0; n--)
   {
      BOPObstructionNode *pNode = obstructions[n];
      if (!pNode)
      {
         obstructions.removeIndex(n);
         continue;
      }

      if (pNode->mProperties & BObstructionManager::cObsPropertyDeletedNode)
      {
         obstructions.removeIndex(n);
         continue;
      }

      if (pNode->mType == BObstructionManager::cObsNodeTypeUnit)
      {
         BUnit *pUnit = gWorld->getUnit(pNode->mEntityID);
         if (pUnit)
         {
            for (long i = 0; i < mSquads.getNumber(); i++)
            {
               BSquad *sq = gWorld->getSquad(mSquads[i]);
               if ((sq != NULL) && (sq->containsChild(pUnit->getID())))
               {
                  obstructions.removeIndex(n);
                  continue;
               }
            }
         }
      }
   }
}

//==============================================================================
// BSquadPlotter::clearTempObstructions
//==============================================================================
void BSquadPlotter::addTempObstruction(BVector &pos, float width, float depth, BVector &forward)
{
   BOPObstructionNode *pObstructionData = gObsManager.getNewObstructionNode();
   mTempObstructions.add(pObstructionData);

   gObsManager.resetObstructionNode(pObstructionData);
   gObsManager.fillOutRotatedPosition(pObstructionData, pos.x, pos.z, width * 0.5f,
                                  depth * 0.5f, forward.x, forward.z);

   pObstructionData->mType = BObstructionManager::cObsNodeTypeUnit;
   pObstructionData->mPlayerID = 0;   
   pObstructionData->mEntityID = -1;

   // Put it into the quadtree
   long obsType = BObstructionManager::cObsTypeCollidableNonMovableUnit;
   gObsManager.installObjectObstruction(pObstructionData, obsType);

   /*
   // Debug rendering
   gTerrainSimRep.addDebugBoxOverTerrain(
      BVector(pObstructionData->mX1, 0.0f, pObstructionData->mZ1),
      BVector(pObstructionData->mX2, 0.0f, pObstructionData->mZ2),
      BVector(pObstructionData->mX3, 0.0f, pObstructionData->mZ3),
      BVector(pObstructionData->mX4, 0.0f, pObstructionData->mZ4),
      cDWORDCyan, 0.75f);
   */
}

//==============================================================================
// BSquadPlotter::clearTempObstructions
//==============================================================================
void BSquadPlotter::clearTempObstructions()
{   
   for (long i = mTempObstructions.getNumber() - 1; i >= 0; i--)
   {
      if (mTempObstructions[i])
      {
         gObsManager.deleteObstruction(mTempObstructions[i]);
         mTempObstructions[i] = NULL;
      }
   }
   mTempObstructions.clear();
}

//==============================================================================
// BSquadPlotter::render
//==============================================================================
void BSquadPlotter::render(void) const
{
   if (mResults.getNumber() <= 0)
      return;


   for (long i = 0; i < mResults.getNumber(); i++)
   {
      BDynamicSimVectorArray &waypoints = (BDynamicSimVectorArray&) mResults[i].getWaypoints();
      for (long j = 0; j < waypoints.getNumber() - 1; j++)
      {
         gTerrainSimRep.addDebugLineOverTerrain(waypoints[j], waypoints[j + 1], cDWORDBlue, cDWORDBlue, 0.5f);
      }

      if (waypoints.getNumber() > 0)
      {
         BVector pts[4];
         if(mResults[i].getSquadCorners(pts))
         {
            DWORD color = cDWORDBlue;
            if (mResults[i].isObstructed())
               color = cDWORDRed;
            gTerrainSimRep.addDebugSquareOverTerrain(pts[0], pts[1], pts[2], pts[3], color, 0.5f);
         }

         // Add line to show who's going where
//-- FIXING PREFIX BUG ID 2898
         const BEntity* pSquad = gWorld->getEntity(mResults[i].getSquadID());
//--
         if (pSquad)
         {
            gTerrainSimRep.addDebugLineOverTerrain(pSquad->getPosition(), mResults[i].getDesiredPosition(), cDWORDWhite, cDWORDWhite, 0.5f);
         }
      }
    
   }

   // Surround position stuff
   #ifndef BUILD_FINAL
      // Target obstruction and range
      if (pDebugImmobileTargetEntity)
      {
         BOPQuadHull obsHull;
         pDebugImmobileTargetEntity->getObstructionHull(obsHull);
         gTerrainSimRep.addDebugSquareOverTerrain(
            BVector(obsHull.mX1, 0.0f, obsHull.mZ1), 
            BVector(obsHull.mX2, 0.0f, obsHull.mZ2), 
            BVector(obsHull.mX3, 0.0f, obsHull.mZ3), 
            BVector(obsHull.mX4, 0.0f, obsHull.mZ4), 
            cDWORDYellow, 0.5f);

         for (uint i = 0; i < static_cast<uint>(debugImmobileTargetRanges.getNumber()); i++)
            gTerrainSimRep.addDebugRoundedBoxOverTerrain(&obsHull, debugImmobileTargetRanges[i], cDWORDYellow, 0.5f);
      }
      // Surround positions
      for (uint i = 0; i < static_cast<uint>(debugSurroundPositions.getNumber()); i++)
      {
//-- FIXING PREFIX BUG ID 2899
         const BDebugSurroundPosition& pos = debugSurroundPositions[i];
//--

         DWORD color = cDWORDYellow;
         switch(pos.mObstructed)
         {
            case BDebugSurroundPosition::ObstructionUnchecked:
               color = cDWORDYellow;
               break;
            case BDebugSurroundPosition::ObstructionPassed:
               color = cDWORDGreen;
               break;
            case BDebugSurroundPosition::ObstructionFailed:
               color = cDWORDRed;
               break;
         }
         gTerrainSimRep.addDebugCircleOverTerrain(pos.mPos, pos.mInnerRadius, color, 0.5f);
         gTerrainSimRep.addDebugCircleOverTerrain(pos.mPos, pos.mOuterRadius, color, 0.5f);
      }
   #endif

   // Debug valid segments
   #ifdef DEBUG_SQUAD_OBSTRUCT
      for (long i = 0; i < debugSquadPoints.getNumber(); i+=2)
      {
         gTerrainSimRep.addDebugLineOverTerrain(debugSquadPoints[i],
                                          debugSquadPoints[i + 1],
                                          cDWORDGreen, cDWORDGreen, 0.75f);
      }
   #endif
}

//==============================================================================
// BSquadPlotter::reset
//==============================================================================
void BSquadPlotter::reset(void)
{
   mSquads.setNumber(0);   
   mResults.setNumber(0);
   mBaseMovementType=-1;
}

//==============================================================================
// BSquadPlotter::cleanUp
//==============================================================================
void BSquadPlotter::cleanUp(void)
{
   reset();
}

//==============================================================================
// BSquadPlotter::getEdgeOfMapOffset
//==============================================================================
BVector BSquadPlotter::getEdgeOfMapOffset(BVector point)
{
   BVector offset;
   offset.zero();

   //Get edge of map positions
   float tileSize = gTerrainSimRep.getDataTileScale();
   float fMinX = 0.0f, fMaxX = 0.0f, fMinZ = 0.0f, fMaxZ = 0.0f;
   fMaxX = gTerrainSimRep.getNumXDataTiles() * tileSize;
   fMaxZ = gTerrainSimRep.getNumXDataTiles() * tileSize;

   if(point.x  >= fMaxX)
      offset.x = fMaxX - point.x;

   if(point.z >= fMaxZ)
      offset.z = fMaxZ - point.z;

   if(point.x <= fMinX)
      offset.x = fMinX - point.x;

   if(point.z <= fMinZ)
      offset.z = fMinZ - point.z;

   return offset;
}

//==============================================================================
// BSquadPlotter::pushSquadOffEdgeOfMap
//==============================================================================
void BSquadPlotter::pushSquadsOffEdgeOfMap()
{
   BVector offset;
   offset.zero();

   // Check each squad and make sure they're not off the edge of the map   
   for (long i = 0; i < mResults.getNumber(); i++)
   {      
      //--Check the 4 corners of the squad to see if it is off the map
      BVector squadPoints[4];
      bool result = mResults[i].getSquadCorners(squadPoints);
      if(result)
      {
         for(long point = 0; point < 4; point++)
         {
            BVector tempOffset = getEdgeOfMapOffset(squadPoints[point]);
            if(abs(tempOffset.x) > abs(offset.x))
               offset.x = tempOffset.x;
            if(abs(tempOffset.z) > abs(offset.z))
               offset.z = tempOffset.z;
         }
      }      
   }

   //-- Apply our offset if we need to
   if(abs(offset.x) > 0.0f || abs(offset.z) > 0.0f)
   {      
      for (long i = 0; i < mResults.getNumber(); i++)
      {        
         BVector pos = mResults[i].getDesiredPosition();
         mResults[i].setDesiredPosition(pos + offset);
         mResults[i].setDefaultDesiredPos(false);

         BDynamicSimVectorArray &waypoints = (BDynamicSimVectorArray&) mResults[i].getWaypoints();
         for (long j = 0; j < waypoints.getNumber() - 1; j++)
         {
            waypoints[j] += offset;
            waypoints[j + 1] += offset;
         }
      }
   }   
}


// Syncing stuff
#ifdef SYNC_Squad
//==============================================================================
// BSquadPlotter::syncResults
//==============================================================================
void BSquadPlotter::syncResults()
{
   for (long i = 0; i < mResults.getNumber(); i++)
   {
      const BDynamicSimVectorArray &waypoints = mResults[i].getWaypoints();
      for (long j = 0; j < waypoints.getNumber(); j++)
      {
         syncSquadData(" BSquadPlotter Waypoint x", waypoints[j].x);
         syncSquadData(" BSquadPlotter Waypoint y", waypoints[j].y);
         syncSquadData(" BSquadPlotter Waypoint z", waypoints[j].z);
         #if 0
            BASSERT(Utils::IsValidFloat(waypoints[j].x));
            BASSERT(Utils::IsValidFloat(waypoints[j].y));
            BASSERT(Utils::IsValidFloat(waypoints[j].z));
         #endif
      }
   }
}

#endif

//==============================================================================
//==============================================================================
bool BSquadPlotterResult::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BEntityID, mSquadID);
   GFWRITEVAR(pStream, float, mWidth);
   GFWRITEVAR(pStream, float, mDepth);
   GFWRITEVAR(pStream, float, mRange);
   GFWRITEVAR(pStream, float, mTrueActionRange);
   GFWRITEPROTOACTIONPTR(pStream, mpProtoAction);
   GFWRITEVECTORARRAY(pStream, mWaypoints, uint16, 2000);
   GFWRITEVECTOR(pStream, mDesiredPosition);
   GFWRITEVAR(pStream, long, mMovementType);
   GFWRITEBITBOOL(pStream, mObstructed);
   GFWRITEBITBOOL(pStream, mInRange);
   GFWRITEBITBOOL(pStream, mUsingOverrideRange);
   GFWRITEBITBOOL(pStream, mDefaultDesiredPos);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadPlotterResult::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BEntityID, mSquadID);
   GFREADVAR(pStream, float, mWidth);
   GFREADVAR(pStream, float, mDepth);
   GFREADVAR(pStream, float, mRange);
   if (BSaveGame::mGameFileVersion >= 5)
      GFREADVAR(pStream, float, mTrueActionRange);
   GFREADPROTOACTIONPTR(pStream, mpProtoAction);
   GFREADVECTORARRAY(pStream, mWaypoints, uint16, 2000);
   GFREADVECTOR(pStream, mDesiredPosition);
   GFREADVAR(pStream, long, mMovementType);
   GFREADBITBOOL(pStream, mObstructed);
   GFREADBITBOOL(pStream, mInRange);
   GFREADBITBOOL(pStream, mUsingOverrideRange);
   if (BSaveGame::mGameFileVersion >= 4)
      GFREADBITBOOL(pStream, mDefaultDesiredPos);
   return true;
}
