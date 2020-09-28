//==============================================================================
// squadactionmove.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "ConfigsGame.h"
#include "Platoon.h"
#include "database.h"
#include "EntityScheduler.h"
#include "squadactionmove.h"
#include "squad.h"
#include "unit.h"
#include "world.h"
#include "pather.h"
#include "pathingLimiter.h"
#include "obstructionmanager.h"
#include "commands.h"
#include "squadPlotter.h"
#include "triggervar.h"
#include "UnitOpportunity.h"
#include "protoobject.h"
#include "Formation2.h"
#include "protosquad.h"
#include "physics.h"
#include "physicsobject.h"
#include "tactic.h"
#include "weapontype.h"
#include "worldsoundmanager.h"
#include "physicsinfomanager.h"
#include "physicsinfo.h"
#include "physicswarthogaction.h"
#include "ability.h"
#include "unitactioncollisionattack.h"
#include "squadactionattack.h"
#include "SimOrderManager.h"
#include "squadlosvalidator.h"
#include "user.h"
#include "UserManager.h"
#include "platoonactionmove.h"
#include "unitactionphysics.h"
#include "actionmanager.h"

#include "physicsinfo.h"
#include "physicsinfomanager.h"
#include "wwise_ids.h"

// Defines
#ifndef BUILD_FINAL
   //#define DEBUG_UNIT_POSITION
   //#define DEBUGCALCULATEVELOCITYMODIFIER
   // SAM = SquadActionMove, of course! 
   //#define DEBUG_SAM_UPDATE
   //#define DEBUG_SAM_MARKCOMPLETEDUSERLEVELWAYPOINTS
   //#define DEBUG_SAM_FINDPATH
   //#define DEBUG_SAM_FINDMOREPATHSEGMENTS
   //#define DEBUG_SAM_PATHRESULTS
   
   //#define DEBUG_MOVE4
#endif

#ifdef DEBUG_MOVE4
   #define debugMove4 sDebugSquadTempID=(reinterpret_cast<BSquad*>(mpOwner))->getID(), dbgSquadInternalTempID
#else
   #define debugMove4 __noop
#endif

#ifdef _MOVE4
#define IGNORE_PLATOONMATES_4

const float cActionCompleteEpsilon = 0.1f;
const int32 cMaxSpecificPathingAttemptsAllowed = 3;
const float cCatchUpScalar = 2.0f;
const float cCatchUpFastestDistance = 50.0f;
const unsigned long cDefaultPauseTimeout = 250L;
const unsigned long cLongPauseTimeout = 1500L;
#endif

#define _NEWWARTHOG_MOVEMENT

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


#ifndef BUILD_FINAL
const char *getPauseReasonString(long id)
{
   switch(id)
   {
      case BSquadActionMove::cPausedReasonPathFailed:
         return("PathFailed");
      
      case BSquadActionMove::cPausedReasonPathInRangeAtStart:
         return("PathInRangeAtStart");

      case BSquadActionMove::cPausedReasonOtherIsFaster:
         return("OtherIsFaster");
         
      case BSquadActionMove::cPausedReasonOtherIsInFront:
         return("OtherIsInFront");
         
      case BSquadActionMove::cPausedReasonOtherWakesUpSoon:
         return("OtherWakesUpSoon");
         
      case BSquadActionMove::cPausedReasonOtherIsHigherPriority:
         return("OtherIsHigherPriority");

      case BSquadActionMove::cPausedReasonNoClearSpot:
         return("NoClearSpot");

      case BSquadActionMove::cPausedReasonPathSucks:
         return("PathSucks");

      case BSquadActionMove::cPausedReasonPlatoonIsPaused:
         return("PlatoonIsPaused");
   }
         
   return("Unknown");
}
#endif


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionMove, 5, &gSimHeap);


//==============================================================================
//==============================================================================
bool BSquadActionMove::connect(BEntity* pEntity, BSimOrder* pOrder)
{
   if (!BAction::connect(pEntity, pOrder))
      return(false);

   //Figure our range (if we're moving for ourself).
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!mTarget.isRangeValid() && pOrder && (pOrder->getOwnerID() == pSquad->getID()))
      pSquad->calculateRange(&mTarget, NULL);

   // Hacktastic assert to try to catch why move actions are getting associated with the cleansing power
   #ifndef BUILD_FINAL
   static BProtoObjectID cleansingProtoID1 = gDatabase.getProtoObject("pow_gp_cleansing_01");
   static BProtoObjectID cleansingProtoID2 = gDatabase.getProtoObject("pow_gp_cleansing_02");
   static BProtoObjectID cleansingProtoID3 = gDatabase.getProtoObject("pow_gp_cleansing_03");

   BProtoObjectID protoID = pSquad->getProtoID();
   BUnit *pLeaderUnit = pSquad->getLeaderUnit();
   if (pLeaderUnit)
      protoID = pLeaderUnit->getProtoID();
   if (protoID == cleansingProtoID1 || protoID == cleansingProtoID2 || protoID == cleansingProtoID3)
   {
      // This assert is interfering with automated testing.  We have the information we need from the assert, but 
      // we still need to track this down with a record game. 
      //BASSERTM(0, "We're attempting to connect a move action to the cleansing power.  Should not be trying to do this.");
      return false;
   }
   #endif

   // If this is a single unit squads with turn radius, update the turn radius pos/fwd at
   // the beginning of the move.  Currently only physics vehicles use this.
   #ifdef NEW_TURNRADIUS
      if (pSquad->getFlagUpdateTurnRadius())
      {
         // Initialize turn radius pos/fwd
         const BUnit* pLeaderUnit = pSquad->getLeaderUnit();
         if (pLeaderUnit)
         {
            // Do some distance checking before resetting the turnRadius Pos/Fwd, otherwise
            // we might get some popping because of small distances between the current unit position
            // and the turn radius pos
            if (pLeaderUnit->getPosition().xzDistance(pSquad->getTurnRadiusPos()) > pLeaderUnit->getObstructionRadius())
            {
               pSquad->setTurnRadiusPos(pLeaderUnit->getPosition());
               pSquad->setTurnRadiusFwd(pLeaderUnit->getForward());
            }
         }
      }
   #endif

   // SLB: To fix infantry turning bugs
   mVelocity = pSquad->getDesiredVelocity();
   #ifdef _MOVE4
   // in Move4, by default, our velocity is the velocity the platoon wants us to move.
   // DLM 7/2/08 - changing this to use squad's desired velocity.  So different squads
   // will travel at different speeds.
   // mVelocity = getPlatoonVelocity_4();
   mVelocity = pSquad->getDesiredVelocity();
   #endif

   // Reactivate physics object (and disallow deactivation) during a move
   if (pSquad->isSquadAPhysicsVehicle())
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
      if(pUnit)
      {
         BPhysicsObject* pPO = pUnit->getPhysicsObject();
         BASSERT(pPO);
         pPO->enableDeactivation(false);
         pPO->forceActivate();
      }
   }
   /* DLM 11/10/08 - No longer supported   
   if (gWorld->getPathingLimiter())
      gWorld->getPathingLimiter()->incNumSquadMoves();
   */

   // Handle ability command
   if (mTarget.isAbilityIDValid() && mTarget.getAbilityID() != -1)
   {
      int abilityID = gDatabase.getSquadAbilityID(pSquad, mTarget.getAbilityID());
      const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);

      // Check the proto object's ability disabled flag
      bool abilityEnabled = true;
      BUnit *pLeaderUnit = pSquad->getLeaderUnit();
      if (pLeaderUnit && pLeaderUnit->getProtoObject() && pLeaderUnit->getProtoObject()->getFlagAbilityDisabled())
         abilityEnabled = false;

      if (pAbility && abilityEnabled && (!pSquad->getFlagRecovering() || (pSquad->getRecoverType() != cRecoverMove)))
      {
         mFlagAbilityCommand = true;
         mAbilityID = abilityID;

         // Auto squad mode changing.  Only do something for hit and run or change mode abilities.  Boost shouldn't execute this code.
         int squadMode = pAbility->getSquadMode();
         bool isMoveAbility = ((cAbilityChangeMode == pAbility->getType()) || (squadMode == BSquadAI::cModeHitAndRun));
         if ( mFlagAutoSquadMode && squadMode != -1 && isMoveAbility)
         {
            if (squadMode == BSquadAI::cModeLockdown && pSquad->getSquadMode() == BSquadAI::cModeLockdown)
            {
               // Get the target squad.
               BSquad* pTargetSquad = NULL;
               if (mTarget.isIDValid())
               {
                  BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());
                  if (pTargetEntity)
                  {
                     BUnit* pTargetUnit = pTargetEntity->getUnit();
                     if (pTargetUnit)
                        pTargetSquad = pTargetUnit->getParentSquad();
                     else
                        pTargetSquad = pTargetEntity->getSquad();
                  }
               }

               // Auto unlock if no enemy target.
               // TRB 10/9/08 - Jira 14791 - Cobra's move and attack actions were getting out of sync because
               // it was attacking an attackable gaia unit while trying to unlock and lockdown.
               // So do a special check for attackable gaia.  The better thing to do is call getOrderType to
               // see if an attack order can be carried out.  But that call is slow so do this unless it becomes a problem.
               bool attackable = false;
               if (pTargetSquad)
               {
                  // Enemy
                  const BPlayer *pTargetPlayer = pTargetSquad->getPlayer();
                  attackable = pSquad->getPlayer()->isEnemy(pTargetPlayer);

                  // Attackable gaia
                  if (!attackable && pTargetPlayer && (pTargetPlayer->getID() == cGaiaPlayer))
                  {
                     BUnit *pTargetLeaderUnit = pTargetSquad->getLeaderUnit();
                     if (pTargetLeaderUnit && pTargetLeaderUnit->getTactic() && pTargetLeaderUnit->getTactic()->canAttack())
                        attackable = true;
                  }
               }
               
               if (!attackable)
               {
                  mFlagAutoUnlock=true;
                  mFlagAutoLock=true;
               }
               else
                  mFlagAutoLock=true;

               if (!mFlagAutoUnlock && (pTargetSquad || mTarget.isPositionValid()))
               {
                  // Auto unlock if target is out of range
                  float range = 0.0f;
                  if (mTarget.isRangeValid())
                     range = mTarget.getRange();
                  else
                  {
                     BSimTarget tempTarget = mTarget;
                     if (pSquad->calculateRange(&tempTarget, NULL))
                        range = tempTarget.getRange();
                  }
                  float distance=0.0f;
                  if (pTargetSquad)
                     distance = pSquad->calculateXZDistance(pTargetSquad);
                  else if (mTarget.isPositionValid())
                     distance = pSquad->calculateXZDistance(mTarget.getPosition());
                  if (distance > mTarget.getRange())
                     mFlagAutoUnlock=true;
                  else
                  {
                     // Check LOS
                     if (pTargetSquad && !gSquadLOSValidator.validateLOS(pSquad, pTargetSquad))
                        mFlagAutoUnlock = true;
                  }
               }
            }
            else if (squadMode == BSquadAI::cModeLockdown)
            {
               // Auto lockdown.
               mFlagAutoLock=true;
            }
            else if (squadMode == BSquadAI::cModeHitAndRun)
            {
               // Change squad mode, it will be unset when the squad and all its units are done moving
               // We can't just turn it off on disconnect because the physicsified units may still
               // be moving
               debugMove4("SquadActionMove::connect - Squad being put in HitAndRun mode..");
               pSquad->getSquadAI()->setMode(squadMode);

               // Hand off target to collision attack action
               for (uint i = 0; i < pSquad->getNumberChildren(); i++)
               {
                  BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
                  if (pUnit)
                  {
                     BUnitActionCollisionAttack* pUACA = reinterpret_cast<BUnitActionCollisionAttack*>(pUnit->getActionByType(BAction::cActionTypeUnitCollisionAttack));
                     if (pUACA)
                     {
                        pUACA->setTarget(mTarget);
                        pUACA->setMoveOrder(pOrder);
                        pUACA->setAbilityID(mAbilityID);
                     }
                  }
               }
            }
            else if (squadMode != pSquad->getSquadMode())
            {
               mAutoMode = squadMode;
               mFlagAutoMode = true;
               mFlagAutoAttackMode = pAbility->getAttackSquadMode();
               if (squadMode != BSquadAI::cModeNormal)
                  mFlagAutoExitMode = !pAbility->getKeepSquadMode();
            }
         }

         // Movement speed modifier
         if (pAbility->getMovementModifierType()==BAbility::cMovementModifierAbility)
            pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierAbility, pAbility->getMovementSpeedModifier(), false);

         // Sprinting
         if (pAbility->getSprinting())
         {
            pSquad->setFlagSprinting(true);
            if (pAbility->getRecoverType() != -1 && pSquad->getRecoverType() == pAbility->getRecoverType())
               pSquad->setRecover(-1, 0.0f, -1);
         }
      }
   }
   // ajl 4/20/08 - Jira 5650 X/Y button change
   // TRB 7/30/08 - Changing this so it only unlocks if pressing the Y button.  X button does nothing.
   /*
   else if (pOrder && pOrder->getPriority() == BSimOrder::cPriorityUser)
   {
      if (pSquad->getSquadMode() == BSquadAI::cModeLockdown || pSquad->getSquadMode() == BSquadAI::cModeAbility)
      {
         // Auto go back to normal mode.
         mAutoMode = BSquadAI::cModeNormal;
         mFlagAutoMode = true;
      }
   }
   */

   // TRB 7/17/08 - As mentioned above, hit and run mode isn't disabled in disconnect because the physics unit may still be
   // moving and bowling things.  So hit and run is usually disabled when the unit comes to rest.  However, if the user keeps
   // the unit in motion it won't ever leave hit and run mode.  So this will disable hit and run if the squad successfully
   // bowled and a new move action is started.
   if ((pSquad->getSquadMode() == BSquadAI::cModeHitAndRun) && !mFlagAbilityCommand)
   {
      debugMove4("SquadActionMove::connect - Squad being taken out of HitAndRun mode..");
      pSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);
   }

   if (!mOverridePosition.almostEqual(cInvalidVector) && (pSquad->getSquadMode() == BSquadAI::cModeLockdown))
   {
      mFlagOverrideLockDown = true;
   }

   // DLM 6/16/08 - By default, MonitorOpps is on for platoon moves.  However, we don't
   // want to monitor opps and terminate on them if our formation type is gaggle, as gaggle 
   // unitActions never terminate.  So turn that off for formation type gaggle. 
   if (pSquad->getFormationType() == BFormation2::eTypeGaggle)
      mFlagMonitorOpps = false;

   return(true);
}

//==============================================================================
//==============================================================================
void BSquadActionMove::disconnect()
{
   // Allow physics object to deactivate once a move is complete
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   if (pSquad && pSquad->isSquadAPhysicsVehicle())
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
      if(pUnit)
      {
         BPhysicsObject* pPO = pUnit->getPhysicsObject();
         BASSERT(pPO);
         pPO->enableDeactivation(true);
      }
   }
   /* No longer supported
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (pathLimiter)
   {
      if (mNumPathingDelays > 0)
         pathLimiter->addToTotalNumPathingDelays(mNumPathingDelays);
   }
   */

   if (pSquad)
   {      
      // Remove the child action.
      pSquad->removeActionByID(mChildActionID);
      pSquad->playMovementSound(false);
   }

   if (mpOwner)
   {
      mpOwner->stopMove();
      removeOpp();      
   }   

   if (pSquad && mFlagAutoExitMode)
      pSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);

   if (mFlagAbilityCommand)
   {
      const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
      if (pAbility)
      {
         if (pSquad && pAbility->getMovementModifierType()==BAbility::cMovementModifierAbility)
            pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierAbility, pAbility->getMovementSpeedModifier(), true);

         if (pSquad && pAbility->getSprinting())
            pSquad->setFlagSprinting(false);

         // Handle recovery mode
         if (pSquad && (pAbility->getRecoverStart() == cRecoverMove) && pSquad->isAlive())
            pSquad->setRecover(pAbility->getRecoverType(), pSquad->getPlayer()->getAbilityRecoverTime(pAbility->getID()), pAbility->getID());
      }
   }

   if (mpChildOrder)
   {
      mpChildOrder->decrementRefCount();
      if (mpChildOrder->getRefCount() == 0)
         gSimOrderManager.markForDelete(mpChildOrder);
      mpChildOrder = NULL;
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::init()
{
   if (!BAction::init())
      return(false);

   #ifdef _MOVE4
      mInterimTarget_4 = cOriginVector;
      mFormationPositionWhenSpecificPathWasCreated_4 = cOriginVector;
      #ifdef PRECALC_TURNRADIUS
         mTurnRadiusSourcePath_4.reset();
      #endif
   #endif

   mUserLevelWaypoints.clear();
   mUserLevelWaypointsPathIndex.clear();
   mUserLevelWaypointUpdateTime = 0;
   mPath.reset();
   mCurrentWaypoint=0;
   mPathingRetryTime = 0;
   mNumPathingDelays = 0;
   mMaxPathingDelays = 0;
   
   mProjectionPath.clear();
   mProjections.clear();
   mProjectionSiblings.clear();
   mProjectionDistance=0.0f;
   mProjectionTime=0.0f;
   mVelocity=0.0f;
   mLastProjectionSiblingUpdate = 0;
   mBlockingEntityID = cInvalidObjectID;
   mPercentComplete = 0.0f;

   #ifdef PRECALC_TURNRADIUS
      mTurnRadiusArcCenters.clear();
      mTurnRadiusArcData.clear();
      mTurnRadiusTimes.clear();
      mStartingDir = cOriginVector;
      mTurnSegmentTime = 0.0f;
      mTurnRadiusOverlapAngle.clear();
   #endif

   mFlagConflictsWithIdle = true;
   mTarget.reset();
   mpParentAction = NULL;
   mpChildOrder = NULL;
   mChildActionID = cInvalidActionID;
   mUnpauseState=cStateNone;
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
   mNumPathingRetriesRemaining = cMaxNumPathingRetries;

   mAutoMode=-1;
   mChangeModeState=-1;
   mAbilityID=-1;

   mFlagPlatoonMove = false;
   mFlagMonitorOpps = false;
   mFlagAutoSquadMode = false;
   mFlagFailed = false;   
   mFlagMustFinishPath=false;
   mFlagLastOutsidePlayableBounds = false;
   mFlagPathingFailedDueToBlocking = false;
   mFlagInAir = false;
   mFlagSkidding = false;
   mFlagAbilityCommand = false;
   mFlagAutoExitMode = false;
   mFlagAutoLock = false;
   mFlagAutoUnlock = false;
   mFlagForceLeashUpdate = false;
   mFlagAutoMode = false;
   mFlagAutoAttackMode = false;

   mFlagFollowingSpecificPath_4 = false;
   mFlagSelfImposedPause_4 = false;

   #ifdef _MOVE4
   mSpecificPathingAttempts_4 = 0;
   mLastPathResult_4 = BPath::cNone;
   mPauseTimeRemaining_4 = 0;
   #endif

   // Override squad plotter
   mOverridePosition = cInvalidVector;
   mOverrideRange = -1.0f;      
   mOverrideRadius = 0.0f;
   mFlagOverrideLockDown = false;

   // Ignore Unit
   mIgnoreUnits.clear();

   #ifdef DEBUGPROJECTIONCOLLISIONS
   mDebugProjections.clear();
   mDebugCollisions.clear();
   mDebugEarlyCollisionPoint=cInvalidVector;
   mDebugEarlyCollisionPointValid=false;
   #endif
   
   #ifndef BUILD_FINAL
   mLastCollisionCheck[0] = cOriginVector;
   mLastCollisionCheck[1] = cOriginVector;
   mPausedReason = -1;
   #endif

   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::isInterruptible() const
{
   // Don't allow this to be interrupted if it has a child change mode action
   if ((mState == cStateLockDown) || (mState == cStateUnlock))
      return false;
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::tackOnWaypoints(const BDynamicSimVectorArray &waypoints)
{
   // Append the waypoints onto the user waypoints list
   uint wpIndex;
   for (wpIndex = 0; wpIndex < waypoints.getSize(); wpIndex++)
   {
      mUserLevelWaypoints.add(waypoints[wpIndex]);
      mUserLevelWaypointsPathIndex.add(cUserWpNeedPath);
   }

   mUserLevelWaypointUpdateTime = gWorld->getGametime();

   // Generate path segments
   findMorePathSegments();

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::getFuturePosition(float desiredTimeIntoFuture, bool ignoreBlocked, BVector &futurePosition, float &realTimeNeeded)
{
   syncSquadData("desiredTimeIntoFuture", desiredTimeIntoFuture);
   syncSquadData("ignoreBlocked", ignoreBlocked);

   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BDEBUG_ASSERT(pSquad);
   
   // Get the proper path for prediction, which might not be mPath for _MOVE4
   BPath path;
   int currentWaypoint = getPathAndWaypointForPredictions(path);
   
   // Sync the path's waypoints.
   #ifdef SYNC_Squad    // to make sure the for loop inside here disappears entirely
   syncSquadData("predicted path num waypoints", path.getNumberWaypoints());
   syncSquadData("predicted path, current waypoint", currentWaypoint);
   for(long i=0; i<path.getNumberWaypoints(); i++)
      syncSquadData("   waypoint", path.getWaypoint(i));
   #endif
   
   if ((currentWaypoint >= path.getNumberWaypoints()) || (currentWaypoint < 0) || (path.getNumberWaypoints() <= 0))
   {
      futurePosition = mpOwner->getPosition();
      realTimeNeeded = 0.0f;
      syncSquadData("BSquadActionMove::getFuturePosition -- currentWaypoint", currentWaypoint);
      syncSquadData("BSquadActionMove::getFuturePosition -- getNumberWaypoints", path.getNumberWaypoints());
      return (true);
   }
   if (getState() == cStateFailed)
   {
      syncSquadCode("BSquadActionMove::getFuturePosition -- cStateFailed");
      return false;
   }
   if (getState() == cStateWaitOnChildren)
   {
      syncSquadCode("BSquadActionMove::getFuturePosition -- cStateWaitOnChildren");
      futurePosition = mpOwner->getPosition();
      realTimeNeeded = 0.0f;
      return (true);
   }
   if (!ignoreBlocked && (getState() == cStateBlocked))
   {
      syncSquadCode("BSquadActionMove::getFuturePosition -- cStateBlocked");
      futurePosition = mpOwner->getPosition();
      realTimeNeeded = 0.0f;
      return (true);
   }

   //Decide what velocity to use. If we have 0 velocity, use the desired velocity (as we'll
   //assume we're accelerating).
   float velocityToUse;
   if (mVelocity > 0.0f)
      velocityToUse=mVelocity;
   else
      velocityToUse=pSquad->getDesiredVelocity();

   // Target waypoint to use in case position along path can't be calculated
   int targetWaypoint = currentWaypoint;

   // Determine future position certain time into future based on velocity.
   // desiredTimeIntoFuture == 0 means to use next waypoint
   if ((getState() == cStateWorking) && (desiredTimeIntoFuture > cFloatCompareEpsilon))
   {
      // New turn radius version gets future positions around turns
      #ifndef _MOVE4       // jce [4/22/2008] -- move4 version doesn't have turn radius as 1:1 with the path waypoints
      //BSquad* pSquad = mpOwner->getSquad();
      if (pSquad->getProtoSquad()->hasTurnRadius())
      {
         // Desired time
         float endSegmentTime = mTurnSegmentTime + desiredTimeIntoFuture;
         if (endSegmentTime > mTurnRadiusTimes[path.getNumberWaypoints() - 2].z)
            endSegmentTime = mTurnRadiusTimes[path.getNumberWaypoints() - 2].z;

         BVector pos, dir;
         uint tempWaypoint;
         turnRadiusInterpolate(endSegmentTime, futurePosition, dir, tempWaypoint);

         // Set time and distance
         realTimeNeeded = endSegmentTime - mTurnSegmentTime;
         syncSquadData("BSquadActionMove::getFuturePosition -- futurePosition 1", futurePosition);
         syncSquadData("BSquadActionMove::getFuturePosition -- realTimeNeeded 1", realTimeNeeded);

         return true;
      }
      // Standard, non-turn radius version
      else
      #endif
      {
         float distance = velocityToUse * desiredTimeIntoFuture;
         float realDistance = 0.0f;
         long newNextWaypoint;
         if (path.calculatePointAlongPath(distance, mpOwner->getPosition(), currentWaypoint, futurePosition, realDistance, newNextWaypoint))
         {
            if (velocityToUse > cFloatCompareEpsilon)
               realTimeNeeded = realDistance / velocityToUse;
            else
               realTimeNeeded = desiredTimeIntoFuture;
            syncSquadData("BSquadActionMove::getFuturePosition -- futurePosition 2", futurePosition);
            syncSquadData("BSquadActionMove::getFuturePosition -- realTimeNeeded 2", realTimeNeeded);
            return true;
         }
      }

      // Failed to find point so return last waypoint as future position
      targetWaypoint = path.getNumberWaypoints() - 1;
   }

   // Entity is stoppped or failed to find point along path.  Position is our "next" waypoint (the one we're walking to right now).
   futurePosition=path.getWaypoint(targetWaypoint);
   syncSquadData("BSquadActionMove::getFuturePosition stopped-- futurePosition", futurePosition);

   // Calculate time it will take to reach the waypoint
   if (velocityToUse > cFloatCompareEpsilon)
      realTimeNeeded = futurePosition.xzDistance(mpOwner->getPosition()) / velocityToUse;
   else
      realTimeNeeded = 0.0f;

   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::getFuturePosition2(float desiredTimeIntoFuture, bool ignoreBlocked, bool useCurrentVelocity,
   BDynamicVectorArray& futurePositions, float &realTimeNeeded, float& realDistanceNeeded)
{
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BDEBUG_ASSERT(pSquad);

   // Get the proper path for prediction, which might not be mPath for _MOVE4
   BPath path;
   int currentWaypoint = getPathAndWaypointForPredictions(path);   

   futurePositions.setNumber(0);
   if ((currentWaypoint >= path.getNumberWaypoints()) || (currentWaypoint < 0) || (path.getNumberWaypoints() <= 0))
   {
      return false;
      //futurePositions.add(mpOwner->getPosition());
      //realTimeNeeded = 0.0f;
      //realDistanceNeeded=0.0f;
      //return (true);
   }
   if (getState() == cStateFailed)
      return false;
   if (getState() == cStateWaitOnChildren)
   {
      futurePositions.add(mpOwner->getPosition());
      realTimeNeeded = 0.0f;
      realDistanceNeeded=0.0f;
      return (true);
   }
   if (!ignoreBlocked && (getState() == cStateBlocked))
   {
      futurePositions.add(mpOwner->getPosition());
      realTimeNeeded = 0.0f;
      realDistanceNeeded=0.0f;
      return (true);
   }

   //Decide what velocity to use. If we have 0 velocity, use the desired velocity (as we'll
   //assume we're accelerating).
   float velocityToUse = pSquad->getDesiredVelocity();
   if (useCurrentVelocity && (mVelocity > 0.0f))
      velocityToUse = mVelocity;

   // Target waypoint to use in case position along path can't be calculated
   int targetWaypoint = currentWaypoint;

   // Determine future position certain time into future based on velocity.
   // desiredTimeIntoFuture == 0 means to use next waypoint
   if ( ((getState() == cStateWorking) || (getState() == cStateBlocked)) &&
      (desiredTimeIntoFuture > cFloatCompareEpsilon))
   {
      // New turn radius version gets future positions around turns
      // TODO - update turn radius path for single waypoint mPaths
      #ifndef _MOVE4       // jce [4/22/2008] -- move4 version doesn't have turn radius as 1:1 with the path waypoints
      if (pSquad->getProtoSquad()->hasTurnRadius() && (path.getNumberWaypoints() > 1))
      {
         // Add current point
         futurePositions.add(mpOwner->getPosition());

         // Add every intermediate source arc end point, line segment end point, and destination waypoint
         // between current turn segment time and endSegmentTime
         float endSegmentTime = mTurnSegmentTime + desiredTimeIntoFuture;
         if (endSegmentTime > mTurnRadiusTimes[path.getNumberWaypoints() - 2].z)
            endSegmentTime = mTurnRadiusTimes[path.getNumberWaypoints() - 2].z;

         BVector pos, dir;
         uint tempWaypoint;
         int srcWaypoint = Math::Max(0, currentWaypoint - 1); // start looking ahead from current waypoint
         while ((srcWaypoint < path.getNumberWaypoints() - 1) && (endSegmentTime > mTurnRadiusTimes[srcWaypoint].x))
         {
            // End of src arc (== start of line segmnent) - only add this for "large enough" turns
            if ((mTurnSegmentTime < mTurnRadiusTimes[srcWaypoint].x) && (endSegmentTime > mTurnRadiusTimes[srcWaypoint].x) &&
                (mTurnRadiusArcData[srcWaypoint].x > cPiOver8 || mTurnRadiusArcData[srcWaypoint].x < -cPiOver8))
            {
               turnRadiusInterpolate(mTurnRadiusTimes[srcWaypoint].x, pos, dir, tempWaypoint);
               futurePositions.add(pos);
            }
            // End of line segment (== start of dest arc) - only add this for "large enough" turns
            if ((mTurnSegmentTime < mTurnRadiusTimes[srcWaypoint].y) && (endSegmentTime > mTurnRadiusTimes[srcWaypoint].y) &&
                (mTurnRadiusArcData[srcWaypoint].y > cPiOver8 || mTurnRadiusArcData[srcWaypoint].y < -cPiOver8))
            {
               turnRadiusInterpolate(mTurnRadiusTimes[srcWaypoint].y, pos, dir, tempWaypoint);
               futurePositions.add(pos);
            }
            // End of dest arc (== destination waypoint)
            if ((mTurnSegmentTime < mTurnRadiusTimes[srcWaypoint].z) && (endSegmentTime > mTurnRadiusTimes[srcWaypoint].z))
               futurePositions.add(path.getWaypoint(srcWaypoint + 1));

            srcWaypoint++;
         }

         // Add final point
         turnRadiusInterpolate(endSegmentTime, pos, dir, tempWaypoint);
         futurePositions.add(pos);

         // Set time and distance
         realTimeNeeded = endSegmentTime - mTurnSegmentTime;
         realDistanceNeeded = velocityToUse * realTimeNeeded;
         syncSquadData("BSquadActionMove::getFuturePosition2 -- realDistanceNeeded", realDistanceNeeded);

         return true;
      }
      // Standard, non-turn radius version
      else
      #endif
      {
         float distance = velocityToUse * desiredTimeIntoFuture;
         if (path.calculatePointAlongPath(distance, mpOwner->getPosition(), currentWaypoint, futurePositions, realDistanceNeeded))
         {
            if (velocityToUse > cFloatCompareEpsilon)
               realTimeNeeded = realDistanceNeeded / velocityToUse;
            else
               realTimeNeeded = desiredTimeIntoFuture;
            return true;
         }
      }

      // Failed to find point so return last waypoint as future position
      targetWaypoint = path.getNumberWaypoints() - 1;
   }

   // Entity is stoppped or failed to find point along path.  Position is our "next" waypoint (the one we're walking to right now).
   futurePositions.add(path.getWaypoint(targetWaypoint));
   // Calculate time it will take to reach the waypoint
   if (velocityToUse > cFloatCompareEpsilon)
   {
      realDistanceNeeded=futurePositions[0].xzDistance(mpOwner->getPosition());
      realTimeNeeded = realDistanceNeeded / velocityToUse;
   }
   else
   {
      realDistanceNeeded=0.0f;
      realTimeNeeded = 0.0f;
   }

   syncSquadData("BSquadActionMove::getFuturePosition2 -- final realDistanceNeeded", realDistanceNeeded);

   return(true);
}

//==============================================================================
//==============================================================================
void BSquadActionMove::calcPercentComplete()
{
   SCOPEDSAMPLE(BSquadActionMove_calcPercentComplete);

   //Assume we're on the path.
   float distance;
   if (mPath.getFlag(BPath::cLengthValid))
      distance = mPath.getPathLength();
   else
      distance = mPath.calculatePathLength(true);

   if (distance < cFloatCompareEpsilon)
   {
      mPercentComplete = 1.0f;
      return;
   }
   float remainingDistance=mPath.calculateRemainingDistance(mpOwner->getPosition(), mCurrentWaypoint, true);

   // Add the distance to remaining user level waypoints that haven't been added to the found path yet.  This
   // fixes problems where sibling squads have found a different number of segments.
   BVector prevWaypoint = mpOwner->getPosition();
   for (uint wpIndex = 0; wpIndex < mUserLevelWaypointsPathIndex.getSize(); wpIndex++)
   {
      if (mUserLevelWaypointsPathIndex[wpIndex] == cUserWpNeedPath)
      {
         float tempDistance = prevWaypoint.xzDistance(mUserLevelWaypoints[wpIndex]);
         distance += tempDistance;
         remainingDistance += tempDistance;
      }
      prevWaypoint = mUserLevelWaypoints[wpIndex];
   }

   mPercentComplete = (1.0f-remainingDistance/distance);
}

//==============================================================================
//==============================================================================
uint BSquadActionMove::getCurrentUserLevelWaypointIndex() const
{
   uint currentWaypoint = 0;
   for (currentWaypoint = 0; currentWaypoint < mUserLevelWaypointsPathIndex.getSize(); currentWaypoint++)
   {
      if ((mUserLevelWaypointsPathIndex[currentWaypoint] != cUserWpComplete) && (mCurrentWaypoint <= mUserLevelWaypointsPathIndex[currentWaypoint]))
      {
         return currentWaypoint;
      }
   }
   if (mUserLevelWaypointsPathIndex.getSize() > 0)
      return (mUserLevelWaypointsPathIndex.getSize() - 1);
   return 0;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::setState(BActionState state)
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   #ifdef DEBUG_SAM_UPDATE
   debug("setState: New State: %s", getStateName(state));
   #endif

   // If somehow the squad is crapped out, then just return false.
   if (pSquad == NULL)
   {
      debugMove4("BSquadActionMove::setState -- ERROR!!  pSquad is NULL in setState.");
      return false;
   }

   switch (state)
   {
      case cStateWorking:
      {
         #ifdef _MOVE4
         // By default, whenever we enter working state, we return/set to the platoon's velocity..
         // mVelocity = getPlatoonVelocity_4();
         mVelocity = pSquad->getDesiredVelocity();
         #else
         //Setup the opp.
         if (!setupOpp())
            return (true);
         #endif
         //Start the move.
         pSquad->startMove();

         // Reactivate physics object (and disallow deactivation) during a move
         if (pSquad->isSquadAPhysicsVehicle())
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
            if(pUnit)
            {
               BPhysicsObject* pPO = pUnit->getPhysicsObject();
               BASSERT(pPO);
               pPO->enableDeactivation(false);
               pPO->forceActivate();

               BUnitActionPhysics* pPhysAction = reinterpret_cast<BUnitActionPhysics*>(pUnit->getActionByType(BAction::cActionTypeUnitPhysics));
               if (pPhysAction)
                  pPhysAction->setFlagEndMoveOnInactivePhysics(false);
            }
         }
   
         //Play sound
         pSquad->playMovementSound(true);
         break;
      }
      #ifdef _MOVE4
      case cStateWaitOnChildren:
      {
         // TRB 9/30/08 - Gaggling unit move actions are persistent so never go to a state where
         // we're waiting on them to end.  Just go to done state.
         if (pSquad->getFormationType() == BFormation2::eTypeGaggle)
         {
            return (setState(cStateDone));
         }

         pSquad->stopMove();
         break;
      }
         
      #endif
      case cStateBlocked:
      case cStateWait:
      {
         #ifndef _MOVE4
         //Stop the squad from moving.
         mpOwner->stopMove();
         //Pull the opp from our units.
         removeOpp();
         //Insta-stop.
         mVelocity=0.0f;
         #else
         
         // jce [5/8/2008] -- Nuke our specific path if we have one since the obstructions
         // are probably going to look a lot different when we come out of pause.
         if(mFlagFollowingSpecificPath_4)
         {
            mFlagFollowingSpecificPath_4 = false;
            mPath.reset();
            mCurrentWaypoint = 0;
         }
         
         
         mpOwner->stopMove();

         // Allow physics to deactivate if it wants to.
         if (pSquad->isSquadAPhysicsVehicle())
         {
            BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
            if(pUnit)
            {
               BPhysicsObject* pPO = pUnit->getPhysicsObject();
               BASSERT(pPO);
               pPO->enableDeactivation(true);

               BUnitActionPhysics* pPhysAction = reinterpret_cast<BUnitActionPhysics*>(pUnit->getActionByType(BAction::cActionTypeUnitPhysics));
               if (pPhysAction)
                  pPhysAction->setFlagEndMoveOnInactivePhysics(true);
            }
         }

         pSquad->playMovementSound(false);
         //removeOpp();
         #endif
         break;
      }

      case cStatePaused:
      {
         //Save the state.
         mUnpauseState=mState;
         //Stop the squad from moving.
         // 5/21/08: BSR - Aircraft at least, do not like this aspect of attack-move
         // Do not remove the following cMovementTypeAir test without checking consequences on attack-move behavior.
         bool isAir = (pSquad->getProtoObject() && pSquad->getProtoObject()->getMovementType() == cMovementTypeAir);
         if (!isAir)
         {
            mpOwner->stopMove();
            pSquad->playMovementSound(false);
         }
         //Pull the opp from our units.
         removeOpp();
         //Insta-stop.
         if (!isAir)
            mVelocity=0.0f;

         pausePlatoon();
         break;
      }

      case cStateUnpaused:
         //Restore the actual state.  This will need to restart the opps, etc.
         setState(mUnpauseState);
         //Reset the unpause state.
         mUnpauseState=cStateNone;
         return (true);

      case cStateDone:
      case cStateFailed:
      {
         // Don't allow the action to finish if the squad is jumping. Wait for it to land.
         if (mFlagInAir)
            return true;

         // If we have a parent action and all opps haven't reported then spin
         // TRB 9/30/08 - WaitOnChildren isn't a valid state for gaggle squads.
         if (mFlagMonitorOpps && (mUnitOppIDCount > 0) && (pSquad->getFormationType() != BFormation2::eTypeGaggle))
         {
            if (state == cStateFailed)
            {
               mFlagFailed = true;
            }
            else
            {
               debugMove4("Was headed for cStateDone, but had a unitOpp, so setting state to WaitOnChildren...");
               setState(cStateWaitOnChildren);
               return (true);
            }
         }

         #ifdef NEW_TURNRADIUS
            // If we have a unit turn radius to update and it is not to the squad
            // position yet, set us in waitOnChildren state so that it updates until it has caught up with the squad
            // TRB 9/30/08 - WaitOnChildren isn't a valid state for gaggle squads.
            if (pSquad->getFlagUpdateTurnRadius() && (pSquad->getFormationType() != BFormation2::eTypeGaggle))
            {
               const float cActionCompleteEpsilon = 0.1f;
               float distanceRemaining = pSquad->getPosition().xzDistance(pSquad->getTurnRadiusPos());
               if (distanceRemaining >= cActionCompleteEpsilon)
               {
                  if (state == cStateFailed)
                  {
                     mFlagFailed = true;
                  }

                  debugMove4("Was headed for cStateDone, but had we're not done turning, so setting state to WaitOnChildren...");
                  setState(cStateWaitOnChildren);
                  return (true);
               }
            }
         #endif

         // Halwes - 7/7/2008 - If we've already failed then don't bother checking lock down.  Fix for #8261.
         if (mChangeModeState != cStateFailed)
         {
            if (mFlagAutoLock && !pSquad->isLockedDown())
            {
               mChangeModeState = state;
               setState(cStateLockDown);
               return true;
            }
            else if (mFlagAutoMode && mFlagAutoAttackMode && pSquad->getSquadMode()!=mAutoMode)
            {
               mChangeModeState = state;
               setState(cStateLockDown);
               return true;
            }

            if (mFlagAutoExitMode && pSquad->getSquadMode()==mAutoMode)
            {
               mChangeModeState = state;
               setState(cStateUnlock);
               return true;
            }

            if (mFlagAutoUnlock && pSquad->isLockedDown())
            {
               mChangeModeState = state;
               setState(cStateUnlock);
               return true;
            }
         }

         // Invalidate the pathing hint for this squad so it doesn't continue to use them
         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon != NULL)
            pPlatoon->setPathingHintsComplete(pSquad->getID(), mpOrder);

         // Remove the child action.
         pSquad->removeActionByID(mChildActionID);

         //Take our opp away.
         removeOpp();

         //Stop the move.
         pSquad->stopMove();
         //Insta-stop.
         mVelocity=0.0f;
         //Notify our parent, if any.
         if (mpParentAction)
         {
            if (state == cStateDone)
               mpParentAction->notify(BEntity::cEventActionDone, mpOwner->getID(), getID(), 0);
            else
               mpParentAction->notify(BEntity::cEventActionFailed, mpOwner->getID(), getID(), 0);
         }
         break;
      }

      case cStateUnlock:
      {
         mFlagAutoUnlock=false;
         mFlagAutoExitMode=false;
         setupChildOrder(mpOrder);
         if (!mpChildOrder)
         {
            syncSquadCode("BSquadActionMove::setState cStateFailed 1");
            setState(cStateFailed);
            return (true);
         }
         mpChildOrder->setMode((int8)BSquadAI::cModeNormal);
         mChildActionID = pSquad->doChangeMode(mpChildOrder, this);
         if (mChildActionID == cInvalidActionID)
         {
            syncSquadCode("BSquadActionMove::setState cStateFailed 2");
            setState(cStateFailed);
            return (true);
         }
         break;
      }

      case cStateLockDown:
      {
         setupChildOrder(mpOrder);
         if (!mpChildOrder)
         {
            syncSquadCode("BSquadActionMove::setState cStateFailed 3");
            setState(cStateFailed);
            return (true);
         }
         if (mFlagAutoMode && !mFlagOverrideLockDown)
            mpChildOrder->setMode((int8)mAutoMode);
         else
            mpChildOrder->setMode((int8)BSquadAI::cModeLockdown);
         mChildActionID = pSquad->doChangeMode(mpChildOrder, this);
         if (mChildActionID == cInvalidActionID)
         {
            syncSquadCode("BSquadActionMove::setState cStateFailed 4");
            setState(cStateFailed);
            return (true);
         }
         break;
      }
   }
 
   return(BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::pause()
{
   return (setState(cStatePaused));
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::unpause()
{
   return (setState(cStateUnpaused));
}

//==============================================================================
// Pause platoon move
//==============================================================================
void BSquadActionMove::pausePlatoon()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (pSquad)
   {
      BPlatoon* pPlatoon = pSquad->getParentPlatoon();
      if (pPlatoon)
      {
         BPlatoonActionMove* pPlatoonActionMove = reinterpret_cast<BPlatoonActionMove*>(pPlatoon->getActionByType(BAction::cActionTypePlatoonMove));
         if (pPlatoonActionMove && (pPlatoonActionMove->getState() != cStatePaused))
         {
            pPlatoonActionMove->setState(cStatePaused);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::update(float elapsedTime)
{
   SCOPEDSAMPLE(BSquadActionMove_update);
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   syncSquadData("BSquadActionMove::update -- Action mID", (int)getID());
   syncSquadData("BSquadActionMove::update -- mpOwner mID", mpOwner->getID());
   syncSquadData("BSquadActionMove::update -- mState", mState);

   if (pSquad->getFlagPlayingBlockingAnimation())
      return (true);

   // Don't update squad move if last child is in fatality
   if (pSquad->getNumberChildren() == 1)
   {
      const BUnit* pUnit = pSquad->getLeaderUnit();
      if (pUnit && pUnit->getFlagDoingFatality())
         return true;
   }

   // DLM 6/16/08 - By default, MonitorOpps is on for platoon moves.  However, we don't
   // want to monitor opps and terminate on them if our formation type is gaggle, as gaggle 
   // unitActions never terminate.  So turn that off for formation type gaggle. 
   if (pSquad->getFormationType() == BFormation2::eTypeGaggle && mFlagMonitorOpps)
      mFlagMonitorOpps = false;

#ifdef _MOVE4
   return update_4(elapsedTime);
#endif

   calcPercentComplete();

   //No update if paused. 
   if (mState == cStatePaused)
      return (true);
 
   //Main logic is here.  Don't put too much linear execution stuff in the actual
   //case statements themselves; write straightforward methods for that.
   switch (mState)
   {
      case cStateNone:
      {
         // Update outside playable bounds flag
         const BPlayer* pPlayer = pSquad->getPlayer();
         if (pPlayer && !pPlayer->isHuman())
         {
            mFlagLastOutsidePlayableBounds = pSquad->isOutsidePlayableBounds();
         }

         if (mFlagAutoMode && pSquad->getSquadMode()!=mAutoMode && !mFlagAutoAttackMode)
         {
            mChangeModeState = cStatePathing;
            if (mAutoMode == BSquadAI::cModeNormal)
               setState(cStateUnlock);
            else
               setState(cStateLockDown);
            return true;
         }

         if (mFlagAutoUnlock)
         {
            if (pSquad->isLockedDown())
            {
               mChangeModeState = cStatePathing;
               setState(cStateUnlock);
               return true;
            }
            mFlagAutoUnlock = false;
         }

         // Halwes - 10/5/2007 - Intentional fall through here.
      }
      case cStatePathing:
      {
         SCOPEDSAMPLE(BSquadActionMove_update_cStatePathing);
         BActionState newState = cStateWorking;
         #ifdef DEBUG_SAM_UPDATE
         debug("BSquadActionMove::update -- in cStatePathing..");
         #endif

         // Squad in charge of movement so run a path
         if (!mFlagPlatoonMove)
         {
            BASSERT(mTarget.getID().isValid() || mTarget.isPositionValid());
            newState = findPath();
            #ifdef DEBUG_SAM_UPDATE
            debug("BSquadActionMove::update -- mFlagPlatoonMove != true, called findPath, newState is now: %s", getStateName(newState));
            #endif
         }
         // Get initial path from the platoon
         else
         {
            /* DLM 11/10/08 - This is deprecated code to be removed anyway.  Commenting out
            BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
            if (pathLimiter)
            {
               pathLimiter->resetTmpPathCalls();
               pathLimiter->setInGenPathFromPltn(true);
            }
            */
            newState = generatePathFromPlatoon(false);
            #ifdef DEBUG_SAM_UPDATE
            debug("BSquadActionMove::update -- called generatePathFromPlatoon, newState is now: %s", getStateName(newState));
            #endif
            /*
            if (pathLimiter)
            {
               pathLimiter->setInGenPathFromPltn(false);
               if (pathLimiter->getTmpPathCalls() > pathLimiter->getMaxPathCallsInGenPathFromPltn())
                  pathLimiter->setMaxPathCallsInGenPathFromPltn(pathLimiter->getTmpPathCalls());
            }
            */
         }

         if (mFlagAutoLock && (newState == cStateFailed || newState == cStateDone) && !pSquad->isLockedDown())
         {
            mChangeModeState = newState;
            setState(cStateLockDown);
            return true;
         }

         if (mFlagAutoMode && mFlagAutoAttackMode && (newState == cStateFailed || newState == cStateDone) && pSquad->getSquadMode()!=mAutoMode)
         {
            mChangeModeState = newState;
            setState(cStateLockDown);
            return true;
         }

         setState(newState);

         //NOTE: This intentionally falls through to the next case (as long as it's cStateWorking:).
         if (mState != cStateWorking)
            break;
      }

      case cStateWorking:
      case cStateBlocked:
      {
         SCOPEDSAMPLE(BSquadActionMove_update_cStateBlocked);
         // Temporary immobility
         if (pSquad->getFlagNonMobile())
            break;

         //If we cannot move, then fail.
         if (!pSquad->canMove())
         {
            syncSquadCode("BSquadActionMove::update cStateFailed 1");
            setState(cStateFailed);
            break;
         }

         // Movement paused due to higher priority squad blocking it
         if (mState == cStateBlocked)
         {
            if (isStillBlocked())
               break;

            // No longer blocked.  Clear ID.
            mBlockingEntityID = cInvalidObjectID;
         }

         const BUnit* pLeader = pSquad->getLeaderUnit();
         if (pLeader && pLeader->isLockedDown() && pLeader->canAutoUnlock())
         {
            mChangeModeState = cStateWorking;
            setState(cStateUnlock);
            break;
         }

         BActionState newState = cStateWorking;

         // Follow platoon's path if flag set.  Otherwise follow squad-generated path to target.
         //If we're moving for our platoon, update our leash position to that spot.
         if (mFlagPlatoonMove)
         {
            // See if squad path needs to be updated
            if (isPlatoonPathUpdateNeeded())
            {
               newState = generatePathFromPlatoon(true);
            }

            if (newState == cStateWorking)
            {
               newState = followPath(elapsedTime);
               pSquad->setLeashPosition(pSquad->getPosition());
            }
         }
         else
         {
            newState = followPath(elapsedTime);
            if (mFlagForceLeashUpdate)
               pSquad->setLeashPosition(pSquad->getPosition());
         }

         // If movement done, make sure there still aren't user waypoints.  This will happen if there was
         // a pathing failure while moving.  Give it one last try to find paths.
         if (newState == cStateDone && mFlagMustFinishPath)
         {
            for (uint i = 0; i < mUserLevelWaypointsPathIndex.getSize(); i++)
            {
               if (mUserLevelWaypointsPathIndex[i] == cUserWpNeedPath)
               {
                  newState = cStateWorking;
                  mPathingRetryTime = 0;
                  break;
               }
            }
         }

         if (gConfig.isDefined(cConfigSlaveUnitPosition) ||
            (mpOwner->getSquad()->getProtoSquad()->hasTurnRadius() && (mpOwner->getSquad()->getFormationType() != BFormation2::eTypeGaggle)) )
         {
            // Don't whack unit position if it's a physics vehicle
            if (!pSquad->isSquadAPhysicsVehicle())
               pSquad->whackUnitPositions(elapsedTime);
         }

         //Handle state changes.
         if (newState != mState)
         {
            if (mFlagOverrideLockDown && ((newState == cStateDone) || (newState == cStateFailed)) && !pSquad->isLockedDown())
            {
               mChangeModeState = newState;
               newState = cStateLockDown;
            }
            else if (mFlagAutoLock && (newState == cStateDone || newState == cStateFailed) && !pSquad->isLockedDown())
            {
               mChangeModeState = newState;
               newState = cStateLockDown;
            }
            else if (mFlagAutoMode && mFlagAutoAttackMode && (newState == cStateDone || newState == cStateFailed) && pSquad->getSquadMode()!=mAutoMode)
            {
               mChangeModeState = newState;
               newState = cStateLockDown;
            }
            setState(newState);
         }

         //Set sound flags for skidding/jumping
         updateMovementSound();

         break;
      }

      // Wait until all opps have reported
      case cStateWaitOnChildren:
      {
         if (mUnitOppIDCount <= 0)
         {
            if (mFlagFailed)
            {
               syncSquadCode("BSquadActionMove::update cStateFailed 2");
               setState(cStateFailed);
            }
            else
            {
               syncSquadCode("BSquadActionMove::update cStateDone 1");
               setState(cStateDone);
            }
         }
         break;
      }

      // Waiting on timer.  Attempt pathing again once complete.
      case cStateWait:
      {
         if (gWorld->getGametime() >= mPathingRetryTime)
         {
            setState(cStateWorking);
         }
         break;
      }
   }

   // MPB TODO - adjust squad / unit velocity to match physics so that it doesn't get too far ahead
   // Keep velocity up-to-date
   /*
   if ((pSquad->getNumberChildren() == 1) && (gWorld->getUnit(pSquad->getChild(0))->getFlagPhysicsControl()))
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(0));
      BPhysicsObject* pPhysicsObject=pUnit->getPhysicsObject();
      BVector velocity;
      pPhysicsObject->getLinearVelocity(velocity);
      mVelocity = pUnit->getForward().dot(velocity);//velocity.length();
      //velocity.y = 0.0f;
      //pUnit->setVelocity(velocity);
      //pSquad->setVelocity(velocity);
   }
   */

   //Done.
   return(true);
}

//==============================================================================
//==============================================================================
void BSquadActionMove::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   if (!pSquad)
      return;

   switch (eventType)
   {
      // Check if this squad's units have completed their move opportunities.  We don't care if they failed or not just that they
      // have completed them.
      case BEntity::cEventOppComplete:
         {
            //Data1:  OppID.
            //Data2:  Success.
            if (data1 == mUnitOppID)
            {                  
               mUnitOppIDCount--;
            }            
         }
         break;

      case BEntity::cEventActionFailed:
         if (data1 == mChildActionID)
         {
            if (mState == cStateUnlock || mState == cStateLockDown)
            {
               syncSquadCode("BSquadActionMove::notify cStateFailed 1");
               setState(cStateFailed);
            }
         }
         break;

      case BEntity::cEventActionDone:
         if (data1 == mChildActionID)
         {
            if (mState == cStateUnlock)
            {
               setState(mChangeModeState);
               mChangeModeState = cStateNone;
            }
            else if (mState == cStateLockDown)
            {
               if (mFlagAutoMode)
               {
                  // Once the auto mode is entered, apply the movement modifier if specified in the ability
                  if (mFlagAbilityCommand)
                  {
                     const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
                     if (pAbility && pAbility->getMovementModifierType()==BAbility::cMovementModifierMode)
                        pSquad->setAbilityMovementSpeedModifier(BAbility::cMovementModifierMode, pAbility->getMovementSpeedModifier(), false);
                  }
               }
               setState(mChangeModeState);
               mChangeModeState = cStateNone;
            }
         }
         break;

      // A unit was added to the squad (probably through repair / reinforce).  Add any unit opps associated with this action to
      // the new unit.
      case BEntity::cEventSquadUnitAdded:
         {
            if (pSquad->containsChild(static_cast<BEntityID>(data1)))
            {
               BUnit* pUnit = gWorld->getUnit(data1);
               if (pUnit)
               {
                  if (mUnitOppID != BUnitOpp::cInvalidID)
                  {
                     // Find a unit opp amongst the squad mate units that is in the
                     // list of unit opps associated with this squad action
                     const BUnitOpp* pSquadmateOpp = NULL;
                     for (uint i = 0; i < pSquad->getNumberChildren(); i++)
                     {
                        BUnit* pSquadmate = gWorld->getUnit(pSquad->getChild(i));
                        if (pSquadmate && pSquadmate->getID() != pUnit->getID())
                        {
                           pSquadmateOpp = const_cast<BUnitOpp*>(pSquadmate->getOppByID(mUnitOppID));
                           if (pSquadmateOpp)
                              break;
                        }
                     }

                     // If found the correct unit opp associated with this squad action,
                     // copy and add it to the new unit.
                     if (pSquadmateOpp)
                     {
                        // Copy new opp
                        BUnitOpp* pNewOpp = BUnitOpp::getInstance();
                        *pNewOpp = *pSquadmateOpp;

                        //Add it.
                        if (!pUnit->addOpp(pNewOpp))
                           BUnitOpp::releaseInstance(pNewOpp);
                        else
                           mUnitOppIDCount++;
                     }
                  }
               }
            }
         }
         break;
   }    
}

//==============================================================================
//==============================================================================
void BSquadActionMove::debugRender()
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);


   #ifdef _MOVE4
   // For now, only draw the path, if we have one.. 
   if (mFlagFollowingSpecificPath_4)
   {
      BVector wp1 = cOriginVector;
      BVector wp2 = cOriginVector;
      if (mPath.getNumberWaypoints() >= 2)
      {
         wp1 = mPath.getWaypoint(0);
         gTerrainSimRep.addDebugPointOverTerrain(wp1, 2.0f, cDWORDPurple, 1.0f, BDebugPrimitives::cCategoryPathing);
         for (long l = 1; l < mPath.getNumberWaypoints(); l++)
         {
            wp2 = mPath.getWaypoint(l);
            gTerrainSimRep.addDebugLineOverTerrain(wp1, wp2, cDWORDPurple, cDWORDPurple, 1.0f, BDebugPrimitives::cCategoryPathing);
            gTerrainSimRep.addDebugPointOverTerrain(wp2, 2.0f, cDWORDPurple, 1.0f, BDebugPrimitives::cCategoryPathing);
            wp1 = wp2;
         }
      }
   }
   
   // jce [5/9/2008] -- Not sure why the heck this whole function isn't taken out of final builds
   #ifndef BUILD_FINAL

   // Draw the segment we last used for our collision check.
   gTerrainSimRep.addDebugLineOverTerrain(mLastCollisionCheck[0], mLastCollisionCheck[1], cDWORDWhite, cDWORDWhite, 1.5f, BDebugPrimitives::cCategoryPathing);
   
   // State based string.
   if(gConfig.isDefined(cConfigRenderSquadState))
   {
      float velocityFactor = mVelocity/pSquad->getDesiredVelocity();
      BVector pos = pSquad->getPosition();
      pos.y += 1.0f;
      BSimString str;
      DWORD color = cDWORDBlack;
      switch(mState)
      {
         case cStateWait:
               str.format("paused %d (id=%d %s)", mPauseTimeRemaining_4, mPausedByID.asLong(), getPauseReasonString(mPausedReason));
               color = cDWORDBlue;
               break;
            
         case cStateWorking:
            str.format("%0.1f(%0.1fx)", mVelocity, velocityFactor);
            break;
            
         case cStateWaitOnChildren:
            str.format("wait on children");
            break;
            
         case cStateNone:
            str.format("none");
            break;
            
         case cStatePathing:
            str.format("pathing");
            break;
            
         case cStatePaused:
            str.format("action is paused");
            break;
            
         default:
            str.format("unknown state");
            break;
      }
      BSimString str2;
      str2.format("%d: %s", pSquad->getID(), str.asNative());
      BSimString buffer;
      gpDebugPrimitives->addDebugText(str2.asANSI(buffer), pos, 0.5f, color);
   }
   
   // Predicted path.
   if(gConfig.isDefined(cConfigRenderPredictedPaths))
   {
      BPath path;
      getPathAndWaypointForPredictions(path);
      path.render(cDWORDCyan, cDWORDCyan, true);
   }
   
   
   #endif
   return;
   #endif
   
   
   DWORD pathColor=cDWORDOrange;
   if (mFlagMustFinishPath)
      pathColor=cDWORDWhite;
   mPath.render(cDWORDDarkOrange, pathColor, true);
   
   DWORD projectionColor=cDWORDWhite;
   if (getState() == cStateBlocked)
      projectionColor=cDWORDRed;
   else if (mVelocity < pSquad->getDesiredVelocity())
      projectionColor=cDWORDDarkGrey;
   else if (mVelocity > pSquad->getDesiredVelocity())
      projectionColor=cDWORDGreen;
   // Draw projection path
   if (mProjectionPath.getSize() > 1)
   {
      for (uint i = 0; i < mProjectionPath.getSize() - 1; i++)
      {
         BVector p1 = mProjectionPath[i];
         BVector p2 = mProjectionPath[i + 1];
         gTerrainSimRep.addDebugLineOverTerrain(p1, p2, projectionColor, projectionColor, 0.5f, BDebugPrimitives::cCategoryPathing);
      }
   }
   // Draw projections
   for (uint i=0; i < mProjections.getSize(); i++)
   {
      if (mProjections[i].getPointCount() <= 0)
         continue;
         
      for (long j=0; j < mProjections[i].getPointCount(); j++)
      {
         gTerrainSimRep.addDebugThickCircleOverTerrain(mProjections[i].getPoint(j), 0.5f, 0.1f, projectionColor, 0.5f, BDebugPrimitives::cCategoryPathing);
         if (j >= mProjections[i].getPointCount()-1)
            gTerrainSimRep.addDebugLineOverTerrain(mProjections[i].getPoint(j), mProjections[i].getPoint(0), projectionColor, projectionColor, 0.5f, BDebugPrimitives::cCategoryPathing);
         else
            gTerrainSimRep.addDebugLineOverTerrain(mProjections[i].getPoint(j), mProjections[i].getPoint(j+1), projectionColor, projectionColor, 0.5f, BDebugPrimitives::cCategoryPathing);
      }
   }

   #ifdef DEBUGPROJECTIONCOLLISIONS
   //Debug collision projections.
   for (uint i=0; i < mDebugProjections.getSize(); i++)
   {
      if (mDebugProjections[i].getPointCount() <= 0)
         continue;
      for (long j=0; j < mDebugProjections[i].getPointCount(); j++)
      {
         gTerrainSimRep.addDebugThickCircleOverTerrain(mDebugProjections[i].getPoint(j), 0.5f, 0.1f, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryPathing);
         if (j >= mDebugProjections[i].getPointCount()-1)
            gTerrainSimRep.addDebugLineOverTerrain(mDebugProjections[i].getPoint(j), mDebugProjections[i].getPoint(0), cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryPathing);
         else
            gTerrainSimRep.addDebugLineOverTerrain(mDebugProjections[i].getPoint(j), mDebugProjections[i].getPoint(j+1), cDWORDRed, cDWORDRed, 0.5f, BDebugPrimitives::cCategoryPathing);
      }
   }
   //Debug collisions.
   for (uint i=0; i < mDebugCollisions.getSize(); i++)
      gTerrainSimRep.addDebugPointOverTerrain(mDebugCollisions[i], 2.0f, cDWORDRed, 1.0f, BDebugPrimitives::cCategoryPathing);
   //Early collision.
   if (mDebugEarlyCollisionPointValid)
      gTerrainSimRep.addDebugPointOverTerrain(mDebugEarlyCollisionPoint, 2.0f, cDWORDWhite, 2.0f, BDebugPrimitives::cCategoryPathing);
   #endif

   // Draw any user level waypoints that don't have paths yet
   BASSERT(mUserLevelWaypoints.getSize() == mUserLevelWaypointsPathIndex.getSize());
   for (uint wpIndex = 0; wpIndex < mUserLevelWaypointsPathIndex.getSize(); wpIndex++)
   {
      if (mUserLevelWaypointsPathIndex[wpIndex] != cUserWpNeedPath)
         continue;

      gTerrainSimRep.addDebugThickCircleOverTerrain(mUserLevelWaypoints[wpIndex], 0.5f, 0.1f, pathColor, 0.75f, BDebugPrimitives::cCategoryPathing);
   }

   // Go through arc data and draw turn radius path
   #ifndef BUILD_FINAL
      #ifdef PRECALC_TURNRADIUS
         long simDebugValue=0;
         if (gConfig.get(cConfigRenderSimDebug, &simDebugValue) && (simDebugValue == 5))
         {
            const BProtoSquad* pPS = mpOwner->getSquad()->getProtoSquad();
            if (pPS->hasTurnRadius() && (mPath.getNumberWaypoints() > 1))
            {
               // Line and circle per intermediate waypoint
               BVector prevEndPt = mPath.getWaypoint(0);
               for (int k = 0; k < (mPath.getNumberWaypoints() - 1); k++)
               {
                  BVector srcWaypoint = mPath.getWaypoint(k);
                  BVector destWaypoint = mPath.getWaypoint(k + 1);

                  // Line to turn circle
                  BVector srcTurnCenter = mTurnRadiusArcCenters[k];
                  float srcTurnRadius = mTurnRadiusArcData[k].z;
                  float srcTurnRadians = mTurnRadiusArcData[k].x;
                  float destTurnRadians = mTurnRadiusArcData[k].y;
                  bool srcTurnCW = mTurnRadiusArcData[k].w > 0.0f;
                  bool destTurnCW = destTurnRadians >= 0.0f;
                  BVector nextSrcTurnCenter;
                  bool nextSrcTurnCW;
                  float destTurnRadius;
                  if (k == (mPath.getNumberWaypoints() - 2))
                  {
                     nextSrcTurnCenter = mPath.getWaypoint(mPath.getNumberWaypoints() - 1);
                     nextSrcTurnCW = true;
                     destTurnRadius = 0.0f;
                  }
                  else
                  {
                     nextSrcTurnCenter = mTurnRadiusArcCenters[k + 1];
                     nextSrcTurnCW = mTurnRadiusArcData[k + 1].w > 0.0f;
                     destTurnRadius = mTurnRadiusArcData[k + 1].z;
                  }

                  // Src circle
                  gTerrainSimRep.addDebugThickCircleOverTerrain(srcTurnCenter, srcTurnRadius, 0.1f, srcTurnCW ? cDWORDRed : cDWORDGreen, 1.2f, BDebugPrimitives::cCategoryPathing, -1.0f);

                  // Dest circle
                  BVector destTurnCenter;
                  if (destTurnCW == nextSrcTurnCW)
                     destTurnCenter = nextSrcTurnCenter;
                  if (destTurnCW != nextSrcTurnCW)
                  {
                     destTurnCenter = destWaypoint + destWaypoint - nextSrcTurnCenter;
                     gTerrainSimRep.addDebugThickCircleOverTerrain(destTurnCenter, destTurnRadius, 0.1f, destTurnCW ? cDWORDRed : cDWORDGreen, 1.2f, BDebugPrimitives::cCategoryPathing, -1.0f);
                  }

                  // Start and end points
                  float startPtAngle = (srcWaypoint - srcTurnCenter).getAngleAroundY() + srcTurnRadians;
                  BVector startPtVec(sinf(startPtAngle), 0.0f, cosf(startPtAngle));
                  BVector startPt = srcTurnCenter + startPtVec * srcTurnRadius;
                  float endPtAngle = (destWaypoint - destTurnCenter).getAngleAroundY() - destTurnRadians;
                  BVector endPtVec(sinf(endPtAngle), 0.0f, cosf(endPtAngle));
                  BVector endPt = destTurnCenter + endPtVec * destTurnRadius;

                  // Line
                  gTerrainSimRep.addDebugThickLineOverTerrain(startPt, endPt, 0.1f, srcTurnCW ? cDWORDRed : cDWORDGreen, destTurnCW ? cDWORDRed : cDWORDGreen, 1.2f, BDebugPrimitives::cCategoryPathing);
               }
            }
         }
      #endif
   #endif
}

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BSquadActionMove::getDebugLine(uint index, BSimString &string) const
{
   if (index < 2)
      return (BAction::getDebugLine(index, string));
   switch (index)
   {
      case 2:
      {
         uint defaultMovementPriority=BFormationPosition2::cInvalidPriority;
         uint movementPriority=BFormationPosition2::cInvalidPriority;
         BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
         const BPlatoon* pPlatoon=pSquad->getParentPlatoon();
         if (pPlatoon)
         {
            defaultMovementPriority=pPlatoon->getDefaultMovementPriority(pSquad->getID());
            movementPriority=pPlatoon->getMovementPriority(pSquad->getID());
         }
         string.format("Vel=%5.2f, DesVel=%5.2f, MovePri=%d (Def=%d), PercentComplete=%5.2f.", mVelocity, pSquad->getDesiredVelocity(), movementPriority, defaultMovementPriority, getPercentComplete());
         break;
      }
   }
}
#endif

//==============================================================================
//==============================================================================
float BSquadActionMove::getPathingRadius() const
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   return (pSquad->getPathingRadius());
}

//==============================================================================
//==============================================================================
float BSquadActionMove::getLargestUnitRadius() const
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   return (pSquad->getLargestUnitRadius());
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::isPathableAsFlyingUnit() const
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   // Oh snap!  We're out of the playable bounds we better path like we have wings.
   if (pSquad->getPlayer() && !pSquad->getPlayer()->isHuman() && pSquad->isOutsidePlayableBounds())
      return (true);   

   if (pSquad->getProtoObject() != NULL)
      return (pSquad->getProtoObject()->getMovementType() == cMovementTypeAir);
   return false;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::isPathableAsFloodUnit() const
{
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->getProtoObject() != NULL)
      return (pSquad->getProtoObject()->getMovementType() == cMovementTypeFlood);
   return false;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::isPathableAsScarabUnit() const
{
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->getProtoObject() != NULL)
      return (pSquad->getProtoObject()->getMovementType() == cMovementTypeScarab);
   return false;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::isPlatoonPathUpdateNeeded()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   const BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if (mFlagPlatoonMove && (pPlatoon != NULL) && (pPlatoon->getPathingHintsTime() > mUserLevelWaypointUpdateTime))
   {
      return true;
   }
   return false;
}

//==============================================================================
//==============================================================================
BActionState BSquadActionMove::generatePathFromPlatoon(bool tackOnNewWaypoints)
{
   SCOPEDSAMPLE(BSquadActionMove_generatePathFromPlatoon);
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if (pPlatoon == NULL)
      return cStateFailed;

   if (!updatePathingLimits())
      return cStatePathing;

   // Make sure the platoon's pathing hints are for the same order
   if ((mpOrder != NULL) && (mpOrder->getID() != pPlatoon->getPathingHintsSimOrderID()))
      return cStateFailed;

   // Replace all the waypoints with the ones from the platoon
   if (!tackOnNewWaypoints)
   {
      // Get the waypoints from the platoon path
      mUserLevelWaypoints.clear();
      mUserLevelWaypointsPathIndex.clear();
      mUserLevelWaypointUpdateTime = 0;

      uint maxSiblingHintIndex = 0;
      if (!pPlatoon->getInitialPathingHintsForChild(pSquad->getID(), 0, mUserLevelWaypoints, maxSiblingHintIndex))
         return cStateFailed;
      if (maxSiblingHintIndex >= mUserLevelWaypoints.getSize())
         maxSiblingHintIndex = (mUserLevelWaypoints.getSize() - 1);

      // Fill out the index array
      mUserLevelWaypointsPathIndex.setNumber(mUserLevelWaypoints.getSize());
      uint wpIndex;
      for (wpIndex = 0; wpIndex < mUserLevelWaypoints.getSize(); wpIndex++)
      {
         // Mark waypoint completed
         if (wpIndex < maxSiblingHintIndex)
            mUserLevelWaypointsPathIndex.setAt(wpIndex, cUserWpComplete);
         else
            mUserLevelWaypointsPathIndex.setAt(wpIndex, cUserWpNeedPath);
      }
      mUserLevelWaypointUpdateTime = gWorld->getGametime();

      // Generate a path for the squad
      mCurrentWaypoint = 0;
      return (findPath());
   }
   // Determine new platoon waypoints and tack them on the current path
   else
   {
      // Get the waypoints from the platoon path
      BDynamicSimVectorArray newWaypoints;
      uint maxSiblingHintIndex = 0;
      if (!pPlatoon->getInitialPathingHintsForChild(pSquad->getID(), mUserLevelWaypoints.getSize(), newWaypoints, maxSiblingHintIndex))
         return cStateFailed;

      // Add the waypoints to the end of the path
      if (!tackOnWaypoints(newWaypoints))
         return cStateFailed;
   }
   return cStateWorking;
}

//==============================================================================
//==============================================================================
void BSquadActionMove::markCompletedUserLevelWaypoints()
{
   // Mark a user level waypoint as complete if the current waypoint passed it.
   // Don't set the last one as complete.  The action should have detected that
   // the target was reached at this point, but to be on the safe side always
   // leave the last waypoint as a valid target.
   if (mUserLevelWaypointsPathIndex.getSize() > 1)
   {
      uint wpIndex;
      for (wpIndex = 0; wpIndex < (mUserLevelWaypointsPathIndex.getSize() - 1); wpIndex++)
      {
         if ((mUserLevelWaypointsPathIndex[wpIndex] < mCurrentWaypoint) && (mUserLevelWaypointsPathIndex[wpIndex] != cUserWpNeedPath))
            mUserLevelWaypointsPathIndex[wpIndex] = cUserWpComplete;
      }
   }
   #ifdef DEBUG_SAM_MARKCOMPLETEDUSERLEVELWAYPOINTS
      debug("   after markCompletedUserLevelWaypoints...");
      debug("   mUserLevelWaypointsPathIndex:");
      debug("   cUserWpComplete = %d cUserWpNeedPath = %d", cUserWpComplete, cUserWpNeedPath);
      int wpIndex;
      char szBuff[512];
      szBuff[0] = '\0';
      for (wpIndex = 0; wpIndex < (signed)(mUserLevelWaypointsPathIndex.getSize() - 1); wpIndex++)
      {
         char szIndex[15];
         sprintf_s(szIndex, 15, "[%d] ", mUserLevelWaypointsPathIndex[wpIndex]);
         strcat_s(szBuff, 512, szIndex);
      }
      debug(szBuff);
   #endif

}

//==============================================================================
//==============================================================================
BActionState BSquadActionMove::findPath(bool ignorePlatoonMates, bool repathing)
{
   // Mark completed user level waypoints before the current waypoint is reset
   #ifdef DEBUG_SAM_FINDPATH
   debug("   ---> BSquadActionMove::findPath");
   #endif
   markCompletedUserLevelWaypoints();

   // If repathing, clear out the user level waypoint indices so it can distinguish between
   // segments that we've moved beyond and ones that need to be repathed
   if (repathing)
   {
      for (uint wpIndex = 0; wpIndex < mUserLevelWaypointsPathIndex.getSize(); wpIndex++)
      {
         if (mUserLevelWaypointsPathIndex[wpIndex] != cUserWpComplete)
            mUserLevelWaypointsPathIndex[wpIndex] = cUserWpNeedPath;
      }
   }

   //Reset the data.
   mPath.reset();
   mCurrentWaypoint=-1;
   #ifdef PRECALC_TURNRADIUS
      mTurnSegmentTime = 0.0f;
   #endif

   const BSimTarget* pTarget = getTarget();
   BASSERT(pTarget);

   // Always refresh the waypoints if there is an entity target
   // We only want to do this for opportunity moves, not regular platoon moves
   // as this will skip intermediate waypoints and path straign to the final waypoint
   if (pTarget->getID().isValid() && (gWorld->getEntity(pTarget->getID()) != NULL) && !mFlagPlatoonMove)
   {
      #ifdef DEBUG_SAM_FINDPATH
      debug("   Have a valid target and this is not a PlatoonMove, so we'll clear the UserLevelWaypoints and reset the pathing index..");
      #endif
      mUserLevelWaypoints.clear();
      mUserLevelWaypointsPathIndex.clear();
      mUserLevelWaypointUpdateTime = 0;
   }

   // Add the target location to the user level waypoints if it's not already there
   if (mUserLevelWaypoints.getSize() == 0)
   {
      #ifdef DEBUG_SAM_FINDPATH
      debug("   mUserLevelWaypoints is empty..");
      #endif
      // See if the order has multiple waypoints
      if ((mpOrder != NULL) && (mpOrder->getWaypointNumber() > 1))
      {
         mUserLevelWaypoints.setNumber(mpOrder->getWaypointNumber());
         mUserLevelWaypointsPathIndex.setNumber(mpOrder->getWaypointNumber());
         mUserLevelWaypointUpdateTime = gWorld->getGametime();

         uint wpIndex;
         #ifdef DEBUG_SAM_FINDPATH
         debug("   and we have an order, so copying following waypoints over:");
         #endif
         for (wpIndex = 0; wpIndex < mpOrder->getWaypointNumber(); wpIndex++)
         {
            mUserLevelWaypoints.setAt(wpIndex, mpOrder->getWaypoint(wpIndex));
            #ifdef DEBUG_SAM_FINDPATH
            debug("   [%d]: (%6.2f, %6.2f)", wpIndex, mUserLevelWaypoints[wpIndex].x, mUserLevelWaypoints[wpIndex].z);
            #endif
            mUserLevelWaypointsPathIndex.setAt(wpIndex, cUserWpNeedPath);
         }
      }

      // Else use target position
      else
      {
         BVector position;
         if (pTarget->getID().isValid())
         {
            const BEntity* pEntity=gWorld->getEntity(pTarget->getID());
            if (!pEntity)
               return (cStateFailed);
            position=pEntity->getPosition();

            // If there is an entity target call the squad plotter to get a valid position
            static BDynamicSimVectorArray foo;
            foo.setNumber(1);
            //foo[0] = position;
            foo[0] = mpOwner->getPosition(); // fix the wrong squad plotter thing.
            static BEntityIDArray squads;
            squads.setNumber(1);
            squads[0] = mpOwner->getID();
            bool haveValidPlots = gSquadPlotter.plotSquads(squads, mpOwner->getPlayerID(), pTarget->getID(), foo, position, pTarget->getAbilityID(), BSquadPlotter::cSPFlagIgnorePlayerRestriction);
            if (haveValidPlots && (gSquadPlotter.getResults().getNumber() == squads.getNumber()))
               position = gSquadPlotter.getResults()[0].getDesiredPosition();
         }
         else
            position=pTarget->getPosition();

         #ifdef DEBUG_SAM_FINDPATH
         debug("   So we stuffed in our current target position: (%6.2f, %6.2f)", position.x, position.z);
         #endif
         mUserLevelWaypoints.add(position);
         mUserLevelWaypointsPathIndex.add(cUserWpNeedPath);
         mUserLevelWaypointUpdateTime = gWorld->getGametime();
      }
   }

   // Generate as many path segments as necessary
   #ifdef DEBUG_SAM_FINDPATH
   debug("   Checking for more PathSegments..");
   #endif
   BActionState actionState = findMorePathSegments(ignorePlatoonMates);

   mCurrentWaypoint = 0;

   // Advance to next waypoint if already at first one
   if (mPath.getNumberWaypoints() >= 2)
   {
      BVector tempPosition = mPath.getWaypoint(0);
      if (tempPosition.almostEqualXZ(mpOwner->getPosition()))
      {
         #ifdef DEBUG_SAM_FINDPATH
         debug("   We determined we were practically at the first waypoint already, so we skipped to the second one..");
         #endif
         mCurrentWaypoint = 1;
      }
   }

   #ifdef DEBUG_SAM_FINDPATH
   debug("   findPath returned with actionState: %s", getStateName(actionState));
   #endif

   return actionState;
}

//==============================================================================
//==============================================================================
BActionState BSquadActionMove::findMorePathSegments(bool ignorePlatoonMates)
{
   SCOPEDSAMPLE(BSquadActionMove_findMorePathSegments)

   BASSERT(mUserLevelWaypoints.getSize() == mUserLevelWaypointsPathIndex.getSize());

   #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
   debug("   ---> BSquadActionMove::findMorePathSegments");
   #endif

   // Check timer for next pathing attempt
   if ((mPathingRetryTime > gWorld->getGametime()) && (getState() != cStateWait))
   {
      // If at the end of the path, go to the wait state.
      if ((mPath.getNumberWaypoints() <= 0) || (mCurrentWaypoint >= mPath.getNumberWaypoints()))
      {
         mNumPathingRetriesRemaining = cMaxNumPathingRetries;
         mPathingRetryTime = gWorld->getGametime() + cPathingRetryDelayTime;
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("     At end of path, returning with state of cStateWait");
         #endif
         return cStateWait;
      }

      #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
      debug("      mPathingRetryTime > current time, returning immediately with state working.");
      #endif

      return cStateWorking;
   }

   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   if (!updatePathingLimits())
   {
      #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
      debug("      pathing limits reached, returning immediately with cStatePathing...");
      #endif
      return cStatePathing;
   }

   #ifdef PRECALC_TURNRADIUS
      uint oldNumWaypoints = mPath.getNumberWaypoints();
   #endif

   // Setup path finder
   BFindPathHelper pathFinder;
   pathFinder.setTarget(getTarget());
   pathFinder.setEntity(mpOwner, ignorePlatoonMates);
   //pathFinder.setPathingRadius(getPathingRadius());
   pathFinder.setPathingRadius(getLargestUnitRadius());
   pathFinder.setPathableAsFlyingUnit(isPathableAsFlyingUnit());
   pathFinder.setPathableAsFloodUnit(isPathableAsFloodUnit());
   pathFinder.setPathableAsScarabUnit(isPathableAsScarabUnit());
   pathFinder.enableReducePath(false);
   pathFinder.enablePathAroundSquads(true);
   // Don't run the high level path, cause the segments we're pathing to were generated
   // from the high level paths.
   pathFinder.enableSkipLRP(true);
   // Disable LRP for a ramming move
   // Dlm - no need for all this fuss now.  Unless we decided to go back to using LRP's here,
   // in which case we would want to restore ut
   /*
   if (pSquad->getSquadMode() == BSquadAI::cModeHitAndRun)
   {
      BSquadActionAttack* pSQA = reinterpret_cast<BSquadActionAttack*>(pSquad->getActionByType(BAction::cActionTypeSquadAttack));
      if (pSQA && pSQA->getState() == BAction::cStateRamming)
         pathFinder.enableSkipLRP(true);
   }
   */

   bool generatingPaths = false;
   uint numPathSegmentsGenerated = 0;
   bool allPathsDone = true;
   bool foundPaths = false;
   bool foundPartialPath = false;
   for (uint wpIndex = 0; wpIndex < mUserLevelWaypoints.getSize(); wpIndex++)
   {
      #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
      debug("      findMorePathSegments, examining mUserLevelWaypoint: %d.", wpIndex);
      #endif
      // Ignore waypoints that have already been passed.
      if ((mUserLevelWaypointsPathIndex[wpIndex] == cUserWpComplete) ||
          ((mUserLevelWaypointsPathIndex[wpIndex] != cUserWpNeedPath) && (mUserLevelWaypointsPathIndex[wpIndex] < mCurrentWaypoint)))
      {
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      Waypoint complete, or paths Index at this waypoint < current waypoint.  Continueing.");
         #endif
         continue;
      }

      //Decide whether or not we want to hard fail based on a fail of the pathing below.
      bool failOnPathing=false;
      if ((wpIndex == 0) || (mUserLevelWaypointsPathIndex[wpIndex-1] == cUserWpComplete))
      {
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      at first waypoint, or next non-complete waypoint, failOnPathing == true.");
         #endif
         failOnPathing=true;
      }

      // Check whether path has been generated up to this user level waypoint
      if (mUserLevelWaypointsPathIndex[wpIndex] == cUserWpNeedPath)
      {
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      mPathingRetryTime > current time, returning immediately with state working.");
         #endif
         generatingPaths = true;
      }

      // Path has been generated for this waypoint so just count amount of path found to see whether that is enough
      if (!generatingPaths)
      {
         numPathSegmentsGenerated++;
      }

      // Generate more path segments until there are enough
      else
      {
         // Enough segments found so quit
         if (numPathSegmentsGenerated >= cMinimumPathSegmentsNeeded)
            break;

         // Update the pathing hint with the latest from the platoon
         bool isLastPathingHint = (wpIndex == (mUserLevelWaypoints.getSize() - 1));
         if (mFlagPlatoonMove)
         {
            // If the pathing hints have been marked complete then use the target position
            bool hintsComplete = false;
            BPlatoon* pPlatoon = pSquad->getParentPlatoon();
            if (pPlatoon != NULL)
               hintsComplete = pPlatoon->getPathingHintsComplete(pSquad->getID(), mpOrder);

            // If repathing because of a previous failure due to blocking, just try to repath to the target directly.
            // Should this change the final pathing hint in the case where the target isn't an entity too?
            if ((mFlagPathingFailedDueToBlocking || hintsComplete) && mTarget.isIDValid() && isLastPathingHint)
            {
               #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
               debug("      This is a repath attempt or at the end of pathing hints - pathing directly to target..");
               #endif
               const BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());
               if (pTargetEntity != NULL)
                  mUserLevelWaypoints.setAt(wpIndex, pTargetEntity->getPosition());
            }
            // Ask the platoon for an updated hint computed by the squad plotter
            else
            {
               BVector updatedPathingHint;
               BEntityID squadPlotterTargetID = cInvalidObjectID;
               // Make sure target ID and range are valid.  If range invalid and plotter called, it might not give the desired results because
               // it won't be doing the ranged arc placement.
               if (isLastPathingHint && mTarget.isIDValid() && mTarget.isRangeValid())
                  squadPlotterTargetID = mTarget.getID();
               bool useSquadPlotter = (isLastPathingHint && ((squadPlotterTargetID != cInvalidObjectID) || !mOverridePosition.almostEqual(cInvalidVector)));
               if ((pPlatoon != NULL) && (pPlatoon->getFinalPathingHintForChild(pSquad->getID(), wpIndex, useSquadPlotter, squadPlotterTargetID, mTarget.getAbilityID(), updatedPathingHint, mOverrideRange, mOverridePosition, mOverrideRadius)))
               {
                  #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
                  debug("      we have a target and we're in range of target, so we're getting finalpathing hint using that target and pathing to that..");
                  #endif
                  mUserLevelWaypoints.setAt(wpIndex, updatedPathingHint);
                  // MPB [11/6/2007] - Taking out the "must finish path" condition for now.  This allows squads to stop
                  // moving as soon as they get in range of their target.
                  //if (isLastPathingHint && pPlatoon->getFinalPathingHintValid(wpIndex))
                  //   mFlagMustFinishPath=true;
                  // Halwes - 5/8/2008 - If override position is valid then we must finish path
                  if (!mOverridePosition.almostEqual(cInvalidVector))
                  {
                     mFlagMustFinishPath = true;
                  }
               }
            }
         }

         // Get previous waypoint
         BVector startWaypoint = mpOwner->getPosition();
         if (mPath.getNumberWaypoints() > 0)
            startWaypoint = mPath.getWaypoint(mPath.getNumberWaypoints() - 1);
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      startWaypoint set to: (%6.2f, %6.2f)", startWaypoint.x, startWaypoint.z);
         debug("      endWaypoint (mUserLevelWaypoint[wpIndex]) set to: (%6.2f, %6.2f)", mUserLevelWaypoints[wpIndex].x, mUserLevelWaypoints[wpIndex].z);
         #endif

         // Get a path
         BActionState actionState = cStateWorking;
         if (mPath.getNumberWaypoints() == 0)
         {
            #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
            debug("      running a path and putting it directly int mPath..");
            #endif
            uint pathingResult = pathFinder.findPath(mPath, startWaypoint, mUserLevelWaypoints[wpIndex], failOnPathing, -1, false);
            if (pathingResult == BPath::cFailed)
               actionState = cStateFailed;
            else if (pathingResult == BPath::cPartial)
               foundPartialPath = true;
            else if (mPath.getNumberWaypoints() == 0)
               actionState = cStateDone;
            #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
            debug("      pathFinder.findPath returned: %s", BPath::getPathResultName(pathingResult));
            #endif
            #ifdef DEBUG_SAM_PATHRESULTS
            if (pathingResult == BPath::cFailed || pathingResult == BPath::cOutsideHulledAreaFailed)
               debug("Attempting to find path segments.. path failed with: %s", BPath::getPathResultName(pathingResult));
            #endif
         }
         else
         {
            static BPath path;
            path.reset();
            #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
            debug("      running a temp path to append to mPath..");
            #endif
            uint pathingResult = pathFinder.findPath(path, startWaypoint, mUserLevelWaypoints[wpIndex], failOnPathing, -1, false);
            if (pathingResult == BPath::cFailed)
               actionState = cStateFailed;
            else if (pathingResult == BPath::cPartial)
               foundPartialPath = true;
            else if (path.getNumberWaypoints() == 0)
               actionState = cStateDone;

            #ifdef DEBUG_SAM_PATHRESULTS
            if (pathingResult == BPath::cFailed || pathingResult == BPath::cOutsideHulledAreaFailed)
               debug("Attempting to find path segments.. path failed with: %s", BPath::getPathResultName(pathingResult));
            #endif
            if (actionState == cStateWorking)
            {
               // Append the path to the end of the current path
               if (!pathFinder.appendPath(path, mPath))
                  actionState = cStateFailed;
               #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
               if (actionState != cStateFailed)
                  debug("      temp path appended to mPath.");
               #endif
            }
         }

         // Check whether path valid
         if (actionState != cStateDone)
            allPathsDone = false;

         if (foundPartialPath || (actionState == cStateFailed))
         {
            // Keep moving while there is some path left.  Set a timer to wait until next repath attempt.
            if ((mPath.getNumberWaypoints() > 0) && (mCurrentWaypoint < mPath.getNumberWaypoints()))
            {
               mPathingRetryTime = gWorld->getGametime() + cPathingRetryDelayTime;
               if (foundPartialPath)
                  foundPaths = true;
               #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
               debug("      I still have waypoints left to path to, and I found at least a partial path, so setting foundPaths to true and breaking..");
               #endif
               break;
            }
            // Pathing failed so attempt a repath after waiting some amount of time
            else if (mNumPathingRetriesRemaining != 0)
            {
               mNumPathingRetriesRemaining--;
               mPathingRetryTime = gWorld->getGametime() + cPathingRetryDelayTime;
               #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
               debug("      No waypoints left, didn't succeed with path, so attempting retry.  nNumPathingRetriesRemaining: %d", mNumPathingRetriesRemaining);
               #endif
               #ifdef DEBUG_SAM_PATHRESULTS
               debug("      No waypoints left, didn't succeed with path, so attempting retry.  nNumPathingRetriesRemaining: %d", mNumPathingRetriesRemaining);
               #endif
               return cStateWait;
            }
            // Repath already attempted so fail
            else
            {
               #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
               debug("      We didn't succeed, we can't retry, so we're just returning with failed.");
               #endif
               #ifdef DEBUG_SAM_PATHRESULTS
               debug("      We didn't succeed, we can't retry, so we're just returning with failed.");
               #endif
               return cStateFailed;
            }
         }


         // Update index of user level waypoint in the path
         mUserLevelWaypointsPathIndex[wpIndex] = static_cast<int16>(mPath.getNumberWaypoints()) - 1;         
         numPathSegmentsGenerated++;
         foundPaths = true;
#ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      mUserLevelWaypointsPathIndex[wpIndex] set to: %d", mPath.getNumberWaypoints() -1);
         debug("      numPathSegmentsGenerated: %d", numPathSegmentsGenerated);
         debug("      foundPaths = true");

#endif

      }
   }
   
   if (foundPaths)
   {
      // Successfully found paths so reset number of pathing retries remaining in case there's a future failure
      mNumPathingRetriesRemaining = cMaxNumPathingRetries;

      #ifdef PRECALC_TURNRADIUS
         // Update all new turn radius path segments (from index (oldNumWaypoints - 2) to end)
         if (oldNumWaypoints == 0)
            mStartingDir = mpOwner->getForward();
         if (mPath.getNumberWaypoints() > 1)
            updateTurnRadiusPath(Math::Max(0L, long(oldNumWaypoints - 2)));
      #endif

      mPath.setCreationTime(gWorld->getGametime());

      // Further error reporting
      if (allPathsDone)
      {
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      All path's done, returning cStateDone");
         #endif
         return cStateDone;
      }
      if (mPath.getNumberWaypoints() <= 0)
      {
         #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
         debug("      mPath has no waypoints, returning cStateFailed");
         #endif
         return cStateFailed;
      }
   }

   // Assert for turn radius paths updated to match regular path
   //BASSERT(!pSquad->getProtoSquad() || !pSquad->getProtoSquad()->hasTurnRadius() || ((mPath.getNumberWaypoints() - 1) == mTurnRadiusTimes.getNumber()));
   #ifdef DEBUG_SAM_FINDMOREPATHSEGMENTS
   debug("      returning from findMorePathSegments with cStateWorking..");
   #endif

   return cStateWorking;
}

//==============================================================================
//==============================================================================
BActionState BSquadActionMove::calcMovement(float elapsedTime, float velocity,
   BVector& newPosition, BVector& newDirection)
{
   // Apply any stasis effect speed multiplier
   BSquad *pSquad = reinterpret_cast<BSquad*>(mpOwner);
   if(pSquad)
   {
      velocity *= pSquad->getStasisSpeedMult();
      if (pSquad->getStasisSpeedMult() == 0.0f)
         return (cStateFailed);
   }

   //NOTE: If this ever returns false, the movement data is trashed/malformed.
   newPosition=mpOwner->getPosition();
   newDirection=mpOwner->getForward();

   const BSimTarget* pTarget = getTarget();
   BASSERT(pTarget);

   //See if we're done.  If we must finish our path, we skip this check.
   if (!mFlagMustFinishPath && pTarget->isRangeValid())
   {
      float distanceToTarget;
      BEntity* pTargetEntity = NULL;
      if (pTarget->getID().isValid())
         pTargetEntity = gWorld->getEntity(pTarget->getID());

      if (pTargetEntity)
         distanceToTarget=mpOwner->calculateXZDistance(pTargetEntity);
      else if (pTarget->isPositionValid())
         distanceToTarget=mpOwner->calculateXZDistance(pTarget->getPosition());
      else
         return (cStateFailed);

      if (!gConfig.isDefined(cConfigTrueLOS))
      {
         //See if we're done.      
         if (distanceToTarget <= pTarget->getRange())
            return (cStateDone);
      }
      else
      {
         //See if we're done.      
         if (distanceToTarget <= pTarget->getRange())
         {
            if(pTargetEntity)
            {
               const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
               BASSERT(pSquad);

               const BSquad* pTargetSquad = NULL;
               if(pTargetEntity->getUnit())
               {
                  pTargetSquad = pTargetEntity->getUnit()->getParentSquad();
               }
               else if(pTargetEntity->getSquad())
               {
                  pTargetSquad = pTargetEntity->getSquad();
               }

               if(pTargetSquad)
               {
                  // Since we are inrange, now also check for line of sight
                  if(gSquadLOSValidator.validateLOS(pSquad, pTargetSquad))
                     return (cStateDone);
               }
            }
            else
            {
               return (cStateDone);
            }
         }
      }
   }

   syncSquadData("BSquadActionMove::calcMovement -- mCurrentWaypoint", mCurrentWaypoint);
   //Validate that we're okay.
   if (mCurrentWaypoint >= mPath.getNumberWaypoints())
      return(cStateFailed);
   BVector targetPosition=mPath.getWaypoint(mCurrentWaypoint);

   #ifdef PRECALC_TURNRADIUS
      // New turn radius movement
      BActionState tempResultState = calcTurnRadiusMovement(elapsedTime, velocity, newPosition, newDirection);
      if (tempResultState == cStateDone || tempResultState == cStateWorking)
         return tempResultState;
   #endif

   // Figure the remaining distance.
   float distanceRemaining=mpOwner->getPosition().xzDistance(targetPosition);
   if (distanceRemaining < 0.0f)
      distanceRemaining=0.0f;
   
   //Handle the case where we're at the next waypoint.  If we're out of waypoints, we're done.
   // DLM 2/28/07 - Relax the distance check a little bit.  If we're within a half meter or so, advance our waypoint.
   // I'm sure this breaks the world.
   if (distanceRemaining <= 0.5f)
   {
      mCurrentWaypoint++;
      if (mCurrentWaypoint >= mPath.getNumberWaypoints())
         return(cStateDone);
      targetPosition=mPath.getWaypoint(mCurrentWaypoint);
      distanceRemaining = mpOwner->getPosition().xzDistance(targetPosition);
   }
   // Not ready for prime time
   /*
   //DLM 2/28/07 - Even if we aren't yet at our current waypoint, see if we're within two tiles of our next high-level
   //waypoint.  If so, then let's just move on to the next waypoint.  
   float fNextHLWPDistance = getNextUserLevelWaypointDistance();
   if (fNextHLWPDistance > cFloatCompareEpsilon && fNextHLWPDistance < 8.0f) // Approx. 2 tiles worth
   {
      mCurrentWaypoint++;
      if (mCurrentWaypoint >= mPath.getNumberWaypoints())
         return(cStateDone);
      targetPosition=mPath.getWaypoint(mCurrentWaypoint);
      distanceRemaining = mpOwner->getPosition().xzDistance(targetPosition);
   }
   */

   //Calculate our movement direction.  If the resulting vector is invalid (which
   //shouldn't happen unless the path is malformed), just bail.
   newDirection=targetPosition-mpOwner->getPosition();
   newDirection.y=0;
   if (!newDirection.safeNormalize())
      return(cStateFailed);

   //Calculate our position change.  If we wrap past the current waypoint, go to the next one
   //if we have one (with the funky position stuff that entails).  If we don't have one, still
   //do the move, but we're done after that.
   newPosition=mpOwner->getPosition();
   BVector positionChange=newDirection*velocity*elapsedTime;
   float distanceChange=positionChange.length();
   BActionState returnState=cStateWorking;
   if (distanceChange > distanceRemaining)
   {
      mCurrentWaypoint++;
      if (mCurrentWaypoint >= mPath.getNumberWaypoints())
      {
         returnState=cStateDone;
         positionChange=newDirection;
         positionChange.scale(distanceRemaining);
      }
      else
      {
         //Track the distance we need to wrap.
         float wrapDistance=distanceChange-distanceRemaining;
         //Redo the newDirection calc using the waypoint we're wrapping past as the current position.
         newPosition=targetPosition;
         targetPosition=mPath.getWaypoint(mCurrentWaypoint);
         newDirection=targetPosition-newPosition;
         newDirection.y=0;
         if (!newDirection.safeNormalize())
            return(cStateFailed);
         positionChange=newDirection;
         positionChange.scale(wrapDistance);
      }
   }

   //Calculate the new position.
   newPosition+=positionChange;

   syncSquadData("BSquadActionMove::calcMovement -- returnState", returnState);

   //Done here.
   return(returnState);
}

//==============================================================================
// getNextUserLevelWaypointDistance
// This routine is supposed to examine the userWaypointLevelIndex thingamajiggy
// and figure out how far we are from the next user level waypoint.  If it's
// not doing that then someone should let me know.  DLM.
//==============================================================================
float BSquadActionMove::getNextUserLevelWaypointDistance()
{
   // Get the current index
   int currentUserWaypointIndex = getCurrentUserLevelWaypointIndex();
   if (currentUserWaypointIndex < 0 || currentUserWaypointIndex > mUserLevelWaypoints.getNumber()-1)
      return -1.0f;
   return mpOwner->getPosition().xyDistance(mUserLevelWaypoints[currentUserWaypointIndex]);
}


//==============================================================================
//==============================================================================
bool BSquadActionMove::needToRepathToMovingTarget()
{
   const BSimTarget* pTarget = getTarget();
   BASSERT(pTarget);
   BASSERT(mpOwner);

   // Only repath if targeting an entity and it's out of range of this entity
   bool needToRepath = false;
   if (pTarget->isRangeValid())
   {
      float distanceToTarget;
      if (pTarget->getID().isValid())
      {
         const BEntity* pTargetEntity=gWorld->getEntity(pTarget->getID());
         if (!pTargetEntity)
            return false;

         // Make sure entity is visible to this player
         const BPlayer* pPlayer = mpOwner->getPlayer();
         BASSERT(pPlayer);
         if (!pTargetEntity->isVisible(pPlayer->getTeamID()))
            return false;

         distanceToTarget=mpOwner->calculateXZDistance(pTargetEntity);

         //See if we're done.      
         if (distanceToTarget > pTarget->getRange())
            needToRepath = true;
      }
   }

   return needToRepath;
}

//==============================================================================
//==============================================================================
BActionState BSquadActionMove::followPath(float elapsedTime)
{
   SCOPEDSAMPLE(BSquadActionMove_followPath);
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BDEBUG_ASSERT(pSquad);

   // See if anymore path segments need to be generated.  If retrying pathing because a platoon
   // mate was blocking, then don't ignore platoon mates on next pathing attempt.
   // DLM 11/10/08 - this entire function is no longer called.  Removing the pathlimiter aspects of it.
   /*
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (pathLimiter)
   {
      pathLimiter->resetTmpPathCallsInFindMorePathSeg();
      pathLimiter->setInFindMorePathSeg(true);
   }
   */
   BActionState tempActionState = findMorePathSegments(!mFlagPathingFailedDueToBlocking);
   /*
   if (pathLimiter)
   {
      pathLimiter->setInFindMorePathSeg(false);
      if (pathLimiter->getTmpPathCallsInFindMorePathSeg() > pathLimiter->getMaxPathCallsInFindMorePathSeg())
         pathLimiter->setMaxPathCallsInFindMorePathSeg(pathLimiter->getTmpPathCallsInFindMorePathSeg());
   }
   */
   if ((tempActionState == cStateFailed) || (tempActionState == cStateWait))
      return tempActionState;

   // Make sure there is valid path data
   if (mCurrentWaypoint >= mPath.getNumberWaypoints())
      return cStateFailed;

   // Pathing succeeded so reset this flag
   mFlagPathingFailedDueToBlocking = false;

   //Figure out if we need to slow down to avoid a collision.
   float velocityModifier;
   bool inside = false;
   bool blockedOnStoppedSquads = false;
   bool finalDest = false;
   BEntityID blockingEntityID(cInvalidObjectID);
   if (mpOwner->getClassType() == BEntity::cClassTypeSquad)
   {
      SCOPEDSAMPLE(BSquadActionMove_followPath_calcVeloMod);
      calculateProjection();
      velocityModifier=calculateVelocityModifier(inside, blockedOnStoppedSquads, finalDest, blockingEntityID);
   }
   else
   {
      SCOPEDSAMPLE(BSquadActionMove_followPath_calcVeloModSimp);
      velocityModifier=calculateVelocityModifierSimple();
   }

   //Calc our "desired" velocity.
   float desiredVelocity = calculateDesiredVelocityBySquadType() * velocityModifier;
   //Limit it.
   limitVelocity(elapsedTime, desiredVelocity, mVelocity);

   //If we've got a 0 velocity, we're blocked.
   bool blocked = false;
   // DLM - this is too small of a compare -- leaving units in an endless loop of ever diminishing returns.
   // If our velocity is below .01, we're blocked. 
   //if ((mVelocity < cFloatCompareEpsilon) && !finalDest)
   if ((mVelocity < 0.01f) && !finalDest)
      blocked = true;

   // Calculate new position
   BVector newPosition;
   BVector newDirection;
   BActionState actionState=cStateWorking;
   bool needToRepath=false;
   bool npaRepath = false;
   // Force repath if we have just crossed the playable bounds boundary
   const BPlayer* pPlayer = pSquad->getPlayer();
   if (pPlayer && !pPlayer->isHuman() && mFlagLastOutsidePlayableBounds && !pSquad->isOutsidePlayableBounds())
   {
      npaRepath = true;
      mFlagLastOutsidePlayableBounds = false;
   }
   else if (!blocked)
   {
      SCOPEDSAMPLE(BSquadActionMove_followPath_calcMovement);
      actionState=calcMovement(elapsedTime, mVelocity, newPosition, newDirection);
   }
   else
   {
      if (!blockedOnStoppedSquads)
      {
         // Save blocking entity so we can keep blocking until it moves out of the way
         syncSquadData("BSquadActionMove::followPath -- blockingEntityID", blockingEntityID);
         if (gConfig.isDefined(cConfigMoreNewMovement3))
            mBlockingEntityID = blockingEntityID;
         return (cStateBlocked);
      }
      needToRepath=true;
   }

   //If we're "done", see if we need to repath because our target moved.
   if (actionState == cStateDone)
   {
      // Make sure final position got set
      syncSquadCode("BSquadActionMove::followPath -- setPosition");
      mpOwner->setPosition(newPosition);

      // If done see if need to repath because target moved
      needToRepath = needToRepathToMovingTarget();
      if (needToRepath)
      {
         SCOPEDSAMPLE(BSquadActionMove_followPath_needToRepath);
         actionState = cStateWorking;

         // Invalidate the pathing hint for this squad so it doesn't continue to use them
         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon != NULL)
            pPlatoon->setPathingHintsComplete(pSquad->getID(), mpOrder);
      }
   }

   // Done or couldn't calculate movement
   if (actionState != cStateWorking)
   {
      return actionState;
   }

   // Check for obstructions
   BSimCollisionResult collisionResult = cSimCollisionNone;
   if (!needToRepath && !blocked)
   {
      SCOPEDSAMPLE(BSquadActionMove_followPath_checkForCollisions);
      BFindPathHelper pathFinder;
      pathFinder.setTarget(getTarget());
      pathFinder.setEntity(mpOwner, true);
      pathFinder.enablePathAroundSquads(true);
      collisionResult = pathFinder.checkForCollisions(mpOwner->getPosition(), newPosition, getPathingRadius(), mPath.getCreationTime());
   }

   // Repath around obstructions.  Ignore terrain obstructions since terrain is static and the path should get around
   // these ok (although minor collisions due to floating point accuracy are possible).
   if (npaRepath || needToRepath || ((collisionResult != cSimCollisionNone) && (collisionResult != cSimCollisionTerrain)))
   {      
      SCOPEDSAMPLE(BSquadActionMove_followPath_repathObst);
      // Find new path      
      bool ignorePlatoonMates = !blocked;
      mFlagPathingFailedDueToBlocking = blocked;
      if (npaRepath)
      {
         BPlatoon* pPlatoon = pSquad->getParentPlatoon();
         if (pPlatoon && pPlatoon->isOutsidePlayableBounds())
         {
            ignorePlatoonMates = true;
         }
      }
      actionState = findPath(ignorePlatoonMates, true);
      if (actionState != cStateWorking)
         return actionState;

      // Calculate new position based on new pathing method
      actionState = calcMovement(elapsedTime, mVelocity, newPosition, newDirection);
      if (actionState == cStateFailed)
         return actionState;

      mFlagPathingFailedDueToBlocking = false;
   }

   //Set the new position.
   syncSquadCode("BSquadActionMove::followPath -- setPosition");
   mpOwner->setPosition(newPosition);
   //Set the new direction.
   mpOwner->setForward(newDirection);
   mpOwner->calcRight();
   mpOwner->calcUp();
   
   syncSquadData("BSquadActionMove::followPath -- new mpOwner position", mpOwner->getPosition());

   return cStateWorking;
}

//==============================================================================
//==============================================================================
void BSquadActionMove::updateProjectionSiblings()
{
   SCOPEDSAMPLE(BSquadActionMove_updateProjSiblings);

   mProjectionSiblings.clear();

   // Create a hull based on the projection hulls
   static BDynamicSimVectorArray hullPoints;
   hullPoints.clear();
   for (int i = 0; i < mProjections.getNumber(); i++)
   {
      // The projection hulls can have nearly identical points, which causes problems for the convex
      // hull builder for some reason.  So remove the interior points and the potentially identical
      // points from consecutive hulls to help the hull builder out.

      // If not 6 points then add them all and hope the convex hull builder can deal with it
      if (mProjections[i].getPointCount() != 6)
      {
         hullPoints.add(mProjections[i].getPoints().getData(), mProjections[i].getPointCount());
         continue;
      }

      // First hull so add back middle
      if (i == 0)
         hullPoints.add(mProjections[i].getPoint(cProjIndexBackMiddle));

      // Always add the back left and right points
      hullPoints.add(mProjections[i].getPoint(cProjIndexBackLeft));
      hullPoints.add(mProjections[i].getPoint(cProjIndexBackRight));

      // Last hull so add all the front points
      if (i == (mProjections.getNumber() - 1))
      {
         hullPoints.add(mProjections[i].getPoint(cProjIndexFrontLeft));
         hullPoints.add(mProjections[i].getPoint(cProjIndexFrontMiddle));
         hullPoints.add(mProjections[i].getPoint(cProjIndexFrontRight));
      }
   }

   // If no projection hull points then just use the squad bounding box
   if (hullPoints.getNumber() == 0)
   {
      BVector forward = mpOwner->getForward();
      forward *= mpOwner->getObstructionRadiusX();
      BVector right = mpOwner->getRight();
      right *= mpOwner->getObstructionRadiusZ();
      BVector position = mpOwner->getPosition();

      hullPoints.add(BVector(position.x + forward.x + right.x, 0.0f, position.z + forward.z + right.z));
      hullPoints.add(BVector(position.x + forward.x - right.x, 0.0f, position.z + forward.z - right.z));
      hullPoints.add(BVector(position.x - forward.x - right.x, 0.0f, position.z - forward.z - right.z));
      hullPoints.add(BVector(position.x - forward.x + right.x, 0.0f, position.z - forward.z + right.z));
   }

   // Build the hull
   static BConvexHull projectionHull;
   projectionHull.clear();
   projectionHull.addPoints(hullPoints.getData(), hullPoints.getNumber(), false);

   // Expand the hull by the amount the squad can move between sibling updates
   float projectionTime = 1.0f;
   if (!gConfig.get(cConfigProjectionTime, &projectionTime))
      projectionTime = cProjectionTime;
   projectionHull.expand(mpOwner->getMaxVelocity() * projectionTime);

   // Do the obstruction check against the hull
   static BObstructionNodePtrArray collisionObs;
   collisionObs.resize(0);
   BVector intersectionPoint(0.0f);
   long lObOptions=
      BObstructionManager::cIsNewTypeCollidableMovingUnit |
      BObstructionManager::cIsNewTypeCollidableStationaryUnit |
      BObstructionManager::cIsNewTypeAllCollidableSquads;
   long lObNodeType = BObstructionManager::cObsNodeTypeAll;

   bool canJump = mpOwner ? mpOwner->canJump() : false;

   gObsManager.begin(BObstructionManager::cBeginEntity, 1.0f, lObOptions, lObNodeType, 0, cDefaultRadiusSofteningFactor, NULL, canJump);
	gObsManager.findObstructions(projectionHull, false, false, collisionObs);
   gObsManager.end();

   // Add to sibling list
   uint obIndex;
   uint obCount = collisionObs.getNumber();
   for (obIndex = 0; obIndex < obCount; obIndex++)
   {
      BOPObstructionNode* pObstructionNode = collisionObs[obIndex];
      if ((pObstructionNode == NULL) || (pObstructionNode->mObject == mpOwner))
         continue;
      BEntity* pObject = pObstructionNode->mObject;
      if (pObject != NULL)
      {
         BSquad* pPathingSquad = reinterpret_cast<BSquad*>(mpOwner);
         BUnit* pPathingUnit = gWorld->getUnit(pPathingSquad->getChild(0));
         const BEntity* pParentSquad = gWorld->getEntity(pObstructionNode->mObject->getParentID());
         BSquad* pObsSquad = NULL;
         BUnit* pObsUnit = NULL;

         bool  bAvoidHitting = true;
         if (pObject->isClassType(BEntity::cClassTypeSquad))
         {
            pObsSquad = pObject->getSquad();
            pObsUnit = gWorld->getUnit(pObsSquad->getLeader());
         }
         else if (pObject->isClassType(BEntity::cClassTypeUnit))
         {
            pObsUnit = pObject->getUnit();
            pObsSquad = pObsUnit->getParentSquad();
         }

         if ((pPathingUnit && pPathingSquad && (pPathingUnit->getProtoObject()->getMovementType() == cMovementTypeAir)) &&
            (pObsUnit && !pObsUnit->getProtoObject()->getFlagObstructsAir()))
            bAvoidHitting = false;

         if ((pPathingUnit && pPathingSquad && (pPathingUnit->getProtoObject()->getMovementType() != cMovementTypeAir)) &&
            (pObsUnit && pObsSquad && (pObsUnit->getProtoObject()->getMovementType() == cMovementTypeAir)))
            bAvoidHitting = false;

         // If this is something we are going to bowl through, don't avoid them.
         if (pPathingUnit && !pPathingUnit->isCollisionEnabledWithEntity(pObsUnit, true))
            bAvoidHitting = false;
         if (pPathingSquad && !pPathingSquad->isCollisionEnabledWithEntity(pObsSquad, true))
            bAvoidHitting = false;         

         // DLM & TB - Some pathing magic to try to keep infantry from projecting against each other.  
         if (pPathingSquad)
         {
            const BProtoObject *pSquadProtoObject = pPathingSquad->getProtoObject();
            bool isInfantry = (pSquadProtoObject)?pSquadProtoObject->isType(gDatabase.getOTIDInfantry()):false;
            const BProtoObject *pObjectProtoObject= pObject->getProtoObject();
            bool isObstructionInfantry = (pObjectProtoObject)?pObjectProtoObject->isType(gDatabase.getOTIDInfantry()):false;            
            if (isInfantry && isObstructionInfantry)
            {
               // DLM 3/16/08 - Only disallow projection against infantry in our own platoon.  Other platoons should still be avoided. 
               if (pObsSquad)
               {
                  if (pPathingSquad->getParentID() == pObsSquad->getParentID())
                     bAvoidHitting = false;
               }
               //bAvoidHitting = false;
            }
         }

         if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeUnit)
         {
            if ((pParentSquad != NULL) && (pParentSquad != mpOwner) && bAvoidHitting)
               mProjectionSiblings.uniqueAdd(pParentSquad->getID());
         }
         else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeSquad)
         {
            if (bAvoidHitting)
               mProjectionSiblings.uniqueAdd(pObstructionNode->mObject->getID());
         }
      }
   }
}

//==============================================================================
//==============================================================================
float BSquadActionMove::calculateVelocityModifier(bool& inside, bool& blockedOnStoppedSquads, bool& finalDest, BEntityID& blockingEntityID)
{
   SCOPEDSAMPLE(BSquadActionMove_calcVelocityModifier);

   if ((mpOwner->getClassType() != BEntity::cClassTypeSquad) ||
      (mProjections.getSize() == 0) ||
      (mProjectionPath.getSize() == 0) ||
      (mProjectionDistance < cFloatCompareEpsilon))
      return (1.0f);

   //float rVal = 1.0f;
   
   inside = false;
   blockedOnStoppedSquads = false;
   finalDest = false;
   blockingEntityID = cInvalidObjectID;
      
   // Update the list of squads to check projection against
   if ((mLastProjectionSiblingUpdate == 0) || (gWorld->getGametime() > (mLastProjectionSiblingUpdate + cProjectionSiblingUpdateTime)))
   {
      updateProjectionSiblings();
      mLastProjectionSiblingUpdate = gWorld->getGametime();
   }

   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   const BPlatoon* pPlatoon=pSquad->getParentPlatoon();
   // No squads to test projection against.
   if (!pPlatoon)
   {
      return (testFinalDestination(finalDest));
   }

   #ifdef DEBUGCALCULATEVELOCITYMODIFIER
   pSquad->debug("SAM::calculateVelocityModifier:");
   #endif

   //Figure the obstruction radius.
   float obstructionRadiusToUse=pSquad->getObstructionRadius();

   //Look at the projected paths for everything we need to work around in our parent.
   BEntityID earliestCollisionID=cInvalidObjectID;
   float earliestCollisionDistance=0.0f;
   bool earliestCollisionMoving=false;
   #ifdef DEBUGPROJECTIONCOLLISIONS
   mDebugEarlyCollisionPointValid=false;
   mDebugProjections.clear();
   mDebugCollisions.clear();
   #endif

   const BEntityIDArray& siblings = mProjectionSiblings;
   for (uint i=0; i < siblings.getSize(); i++)
   {
      BSquad* pSiblingSquad=gWorld->getSquad(siblings[i]);
      if (!pSiblingSquad || (pSiblingSquad == pSquad))
         continue;
      //Get the sibling move action.  Okay if NULL.
      const BSquadActionMove *pSiblingAction = pSiblingSquad->getNonPausedMoveAction_4();
      
      bool moving=false;
      #ifdef _MOVE4
         moving = pSiblingSquad->isMoving();
         if(!moving)
            continue;
      #endif

      //Skip collisions with things that have lower movement priority than we
      //do; they'll move around us (unless they have no move action, in which 
      //case we treat them as something we need to move around).
      #ifdef _MOVE4
         bool haveHigherPriority = (doForwardCompare_4(pSiblingSquad) != cCollisionPause);
      #else
         bool haveHigherPriority = (pPlatoon->getHigherMovementPriority(pSquad->getID(), pSiblingSquad->getID()) == pSquad->getID());
      #endif
      if (pSiblingAction && haveHigherPriority)
         continue;

      //See if we collide.
      BConvexHull expandedSiblingHull;
      bool collision=false;
      float collisionDistance=0.0f;
      BVector collisionPoint(0.0f);

      //If the sibling has a move action, use the projections from it.
      if (pSiblingAction && (pSiblingAction->getProjections().getSize() > 0))
      {
         #ifndef _MOVE4
            moving=true;
         #endif

//         if (gConfig.isDefined(cConfigMoreNewMovement3) && haveHigherPriority && (pSiblingAction->getState() == cStateBlocked))
//            moving = false;

         //See if we collide.
         for (uint j=0; (j < pSiblingAction->getProjections().getSize()) && !collision; j++)
         {
            //Expand his hull by our radius.
            expandedSiblingHull.expandFrom(obstructionRadiusToUse, pSiblingAction->getProjections()[j]);
            #ifdef DEBUGPROJECTIONCOLLISIONS
            uint debugProjectionIndex=mDebugProjections.getSize();
            mDebugProjections.setNumber(debugProjectionIndex+1);
            mDebugProjections[debugProjectionIndex]=expandedSiblingHull;
            #endif

            //Check the collision.            
            collision=checkProjectedCollision(expandedSiblingHull, inside, collisionDistance, collisionPoint);
            #ifdef DEBUGPROJECTIONCOLLISIONS
            if (collision)
               mDebugCollisions.add(collisionPoint);
            #endif
         }
      }
      //Else, use his current spot.
      else
      {
         //If he hasn't moved since after we've pathed, skip him.
         DWORD siblingMoveTime=pSiblingSquad->getLastMoveTime();
         if (mPath.getCreationTime() > siblingMoveTime)
            continue;

         //Else, use his current spot.      
         BOPQuadHull quadHull;
         pSiblingSquad->getObstructionHull(quadHull);
         BVector ugh[4];
         for (uint i=0; i < 4; i++)
            ugh[i].set(quadHull.mPoint[i].mX, 0.0f, quadHull.mPoint[i].mZ);
         expandedSiblingHull.addPoints(ugh, 4, true);
         expandedSiblingHull.expand(obstructionRadiusToUse);
         #ifdef DEBUGPROJECTIONCOLLISIONS
         uint debugProjectionIndex=mDebugProjections.getSize();
         mDebugProjections.setNumber(debugProjectionIndex+1);
         mDebugProjections[debugProjectionIndex]=expandedSiblingHull;
         #endif

         //Check the collision.            
         collision=checkProjectedCollision(expandedSiblingHull, inside, collisionDistance, collisionPoint);
         #ifdef DEBUGPROJECTIONCOLLISIONS
         if (collision)
            mDebugCollisions.add(collisionPoint);
         #endif
      }

      //If we collided, then we need to see if this is the closest one.
      if (collision)
      {
         if ((earliestCollisionID == cInvalidObjectID) ||
            (earliestCollisionDistance > collisionDistance))
         {
            earliestCollisionDistance=collisionDistance;
            earliestCollisionID=pSiblingSquad->getID();
            earliestCollisionMoving=moving;
            #ifdef DEBUGPROJECTIONCOLLISIONS
            mDebugEarlyCollisionPoint=collisionPoint;
            mDebugEarlyCollisionPointValid=true;
            #endif
         }
      } 
   }
   #ifdef DEBUGCALCULATEVELOCITYMODIFIER
   pSquad->debug("  EarliestCollisionID=%d (Moving=%d, Dist=%6.2f), Inside=%d.",
      earliestCollisionID.asLong(), earliestCollisionMoving, earliestCollisionDistance, inside);
   #endif

   //If we're inside, we're at 0.
   if (inside)
   {
      #ifdef DEBUGCALCULATEVELOCITYMODIFIER
      pSquad->debug("    Returning 0.0f.");
      #endif
      blockedOnStoppedSquads=!earliestCollisionMoving;
      blockingEntityID = earliestCollisionID;
      return (0.0f);
   }

   bool noCollision = ((earliestCollisionID == cInvalidObjectID) || (earliestCollisionDistance > mProjectionDistance));

   #ifdef _MOVE4
      if(noCollision)
         return(1.0f);
   #else
      // If we have no collision, then test projection against final destination point
      if (noCollision)
      {      
         float val = testFinalDestination(finalDest);
         if (finalDest)
         {
            syncSquadData("BSquadActionMove::calculateVelocityModifier -- finalDest", val);
            return (val);
         }
      }

      // If we still have no collision, see if we can speed up.  If not, just go normal.
      if (noCollision)
      {
         //Get the platoon's desired movement progress.
         float platoonProgress=pPlatoon->getDesiredMovementProgress();
         if (platoonProgress < cFloatCompareEpsilon)
         {
            #ifdef DEBUGCALCULATEVELOCITYMODIFIER
            pSquad->debug("    Returning 1.0f because platoon progress is 0.0.");
            #endif
            syncSquadCode("BSquadActionMove::calculateVelocityModifier -- noPlatoonProgress");
            return (1.0f);
         }
         //Get our progress.
         float progress=getPercentComplete();
         #ifdef DEBUGCALCULATEVELOCITYMODIFIER
         pSquad->debug("  PlatoonProgress=%6.2f, MyProgress=%6.2f.", platoonProgress, progress);
         #endif
         
         //Speed Up/Slow down.
         float ratio=progress/platoonProgress * 2.0f;
         #ifdef DEBUGCALCULATEVELOCITYMODIFIER
         pSquad->debug("  InitialRatio=%6.2f", ratio);
         #endif
         if (ratio > 2.5f)
            ratio=2.5f;
         else if (ratio < 0.5f)
            ratio=0.5f;
         float rVal=3.0f-ratio;
         #ifdef DEBUGCALCULATEVELOCITYMODIFIER
         pSquad->debug("  FinalRatio=%6.2f.", ratio);
         pSquad->debug("    RVal=%6.2f", rVal);
         #endif
         syncSquadData("BSquadActionMove::calculateVelocityModifier -- noCollision", rVal);
         return (rVal);
      }
   #endif
      
   //Else, do the math to see how much we should slow down.
   blockingEntityID = earliestCollisionID;
   // DLM/TRB - We need to also set the blockedonstoppedsquads flag here in the case that we adjusted velocity
   // based on another squad.
   blockedOnStoppedSquads=!earliestCollisionMoving;
   float rVal=earliestCollisionDistance/mProjectionDistance;
   #ifdef DEBUGCALCULATEVELOCITYMODIFIER
   pSquad->debug("  ProjectionDistance=%6.2f.", mProjectionDistance);
   pSquad->debug("  RVal=%6.2f.", rVal);
   #endif
   syncSquadData("BSquadActionMove::calculateVelocityModifier -- collision", rVal);
   
   return (rVal);
}

//==============================================================================
//==============================================================================
float BSquadActionMove::calculateVelocityModifierSimple()
{
   if (mpOwner->getClassType() != BEntity::cClassTypeSquad)
      return (1.0f);
      
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   const BPlatoon* pPlatoon=pSquad->getParentPlatoon();

   //No platoon or only us means default.
   if (!pPlatoon || (pPlatoon->getNumberChildren() == 1))
      return (1.0f);

   //Get the platoon's desired movement progress.
   float platoonProgress=pPlatoon->getDesiredMovementProgress();
   if (platoonProgress < cFloatCompareEpsilon)
      return (1.0f);

   //Get our progress.
   float progress=getPercentComplete();
   
   //Speed Up/Slow down.
   const float cMaxRatio=1.3f;
   const float cMinRatio=0.7f;
   const float cMinVelocityModifier=0.2f;
   const float cMaxVelocityModifier=1.5f;
   float ratio=progress/platoonProgress;
   //Slow down if we're too far ahead.
   if (ratio > 1.0f)
   {
      if (ratio > cMaxRatio)
         return (cMinVelocityModifier);
      float rVal=1.0f-(ratio-1.0f)/(cMaxRatio-1.0f);
      if (rVal < cMinVelocityModifier)
         return (cMinVelocityModifier);
      return (rVal);
   }
   //Else, speed up if we're behind.
   else if (ratio < 1.0f)
   {
      if (ratio < cMinRatio)
         return (cMaxVelocityModifier);
      float rVal=(1.0f-ratio)/(1.0f-cMinRatio)+1.0f;
      if (rVal > cMaxVelocityModifier)
         return (cMaxVelocityModifier);
      return (rVal);
   }
   return (1.0f);
}

//==============================================================================
//==============================================================================
float BSquadActionMove::calculateDesiredVelocityBySquadType()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   const BProtoObject* pSquadProtoObject = pSquad->getProtoObject();

   if (gConfig.isDefined(cConfigDisableVelocityMatchingBySquadType))
      return (pSquad->getDesiredVelocity());

   // If no parent platoon, use the squad's desired velocity
   const BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if ((pPlatoon == NULL) || !mFlagPlatoonMove)
      return (pSquad->getDesiredVelocity());

   // TRB 2/12/07:  For new movement, all squads in platoon will move at the speed of the slowest squad.
   if (gConfig.isDefined(cConfigMoreNewMovement3))
   {
      return (pPlatoon->getDesiredVelocity());
   }

   if (!pSquadProtoObject)
      return (pSquad->getDesiredVelocity());

   // Check other squads of the same type (infantry, vehicle, or air for now) and use same desired velocity

   // TRB:  Yes, I know this is a goofy way to separate out the different squad types but there isn't a unified
   // set of types for this.  One solution is to add safe-for-code-to-use air and non-air vehicle types to the list
   // of standard types.  Another solution would be to split out the land movement type into two types.  This is probably
   // the best solution for this case, but it would've required a bunch of changes to support the new type and I didn't
   // think it was necessary for this small movement change.
   bool isAir = pSquadProtoObject->getMovementType() == cMovementTypeAir;
   bool isInfantry = pSquadProtoObject->isType(gDatabase.getOTIDInfantry());

   float desiredVelocity = pSquad->getDesiredVelocity();
   uint squadIndex;
   for (squadIndex = 0; squadIndex < pPlatoon->getNumberChildren(); squadIndex++)
   {
      const BSquad* pSiblingSquad = gWorld->getSquad(pPlatoon->getChild(squadIndex));
      if (pSiblingSquad && pSiblingSquad->getProtoObject() && ((pSiblingSquad->getID() != pSquad->getID())) && (desiredVelocity > pSiblingSquad->getDesiredVelocity()))
      {
         // Make sure the squad types match
         const BProtoObject* pSiblingProtoObject = pSiblingSquad->getProtoObject();
         bool isSiblingAir = pSiblingProtoObject->getMovementType() == cMovementTypeAir;
         bool isSiblingInfantry = pSiblingProtoObject->isType(gDatabase.getOTIDInfantry());
         if ((isAir == isSiblingAir) && (isInfantry == isSiblingInfantry))
         {
            desiredVelocity = pSiblingSquad->getDesiredVelocity();
         }
      }
   }

   return desiredVelocity;
}

//==============================================================================
//==============================================================================
void BSquadActionMove::calculateProjection()
{
   SCOPEDSAMPLE(BSquadActionMove_calculateProjection);

   mProjections.clear();
   mProjectionPath.clear();
   mProjectionDistance=0.0f;
   mProjectionTime=0.0f;

   if (mpOwner->getClassType() != BEntity::cClassTypeSquad)
      return;
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   
   //Figure out our projected position.
   BVector position=pSquad->getPosition();

   float acceleration = pSquad->getAcceleration();
   float minStopTime = (acceleration > cFloatCompareEpsilon) ? mVelocity / acceleration : 0.0f;   
   float projectionTime = 0.0f;
   if (!gConfig.get(cConfigProjectionTime, &projectionTime))
      projectionTime = cProjectionTime;
   if (projectionTime < minStopTime)
      projectionTime = minStopTime;

   // Get the future position path
   bool haveFuturePosition = getFuturePosition2(projectionTime, true, false, mProjectionPath, mProjectionTime, mProjectionDistance);
   bool currentPositionAtFront = false;
   if ((mProjectionPath.getSize() > 0) && mProjectionPath[0].almostEqualXZ(position))
      currentPositionAtFront = true;

   //If we can't get a future position or the future position is only
   //our current position, just bail with default for our current location.
   if (!haveFuturePosition ||
      (mProjectionPath.getSize() == 0) ||
      (mProjectionTime < cFloatCompareEpsilon) ||
      ((mProjectionPath.getSize() == 1) && currentPositionAtFront))
   {
      //Setup the path.
      mProjectionPath.setNumber(1);
      mProjectionPath[0]=position;

      //Setup the projection.
      mProjections.setNumber(1);
      BOPQuadHull quadHull;
      mpOwner->getObstructionHull(quadHull);
      BVector ugh[4];
      for (uint i=0; i < 4; i++)
         ugh[i].set(quadHull.mPoint[i].mX, 0.0f, quadHull.mPoint[i].mZ);
      mProjections[0].addPoints(ugh, 4, true);

      //No distance or time.
      mProjectionDistance=0.0f;
      mProjectionTime=0.0f;
      return;
   }

   //Push our current position on the front if it's not already there.
   if (!currentPositionAtFront)
      mProjectionPath.insertAtIndex(position, 0);
   BASSERT(mProjectionPath.getSize() >= 2);
   
   //Figure the obstruction radius.
   float radiusX=pSquad->getObstructionRadiusX();
   float radiusZ=pSquad->getObstructionRadiusZ();
      
   //Create the hull(s).
   BVector foo[6];
   mProjections.setNumber(mProjectionPath.getSize()-1);
   for (uint i=0; i < mProjectionPath.getSize()-1; i++)
   {
      BVector direction=mProjectionPath[i+1]-mProjectionPath[i];
      direction.y=0.0f;
      float distance=direction.length();
      direction.safeNormalize();

      //6-sided version.
      //Back Right.
      foo[cProjIndexBackRight].set(direction.z, 0.0f, -direction.x);
      foo[cProjIndexBackRight]*=radiusX;
      foo[cProjIndexBackRight]+=mProjectionPath[i];
      //Back Left.
      foo[cProjIndexBackLeft].set(-direction.z, 0.0f, +direction.x);
      foo[cProjIndexBackLeft]*=radiusX;
      foo[cProjIndexBackLeft]+=mProjectionPath[i];
      //Front Left.
      foo[cProjIndexFrontLeft]=foo[cProjIndexBackLeft]+direction*distance;
      //Front Middle.
      foo[cProjIndexFrontMiddle]=direction;
      foo[cProjIndexFrontMiddle]*=distance+radiusZ;
      foo[cProjIndexFrontMiddle]+=mProjectionPath[i];
      //Front Right.
      foo[cProjIndexFrontRight]=foo[cProjIndexBackRight]+direction*distance;
      //Back Middle.
      foo[cProjIndexBackMiddle]=-direction;
      foo[cProjIndexBackMiddle]*=radiusZ;
      foo[cProjIndexBackMiddle]+=mProjectionPath[i];
      //Add the points.
      mProjections[i].addPoints(foo, 6);

      //4-sided version.
      //Back Right.
      /*foo[3].set(direction.z, 0.0f, -direction.x);
      foo[3]*=radiusX;
      foo[3]+=mProjectionPath[i];
      //Back Left.
      foo[0].set(-direction.z, 0.0f, +direction.x);
      foo[0]*=radiusX;
      foo[0]+=mProjectionPath[i];
      //Front Left.
      foo[1]=foo[0]+direction*distance;
      //Front Right.
      foo[2]=foo[3]+direction*distance;
      //Add the points.
      mProjections[i].addPoints(foo, 4);*/
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::checkProjectedCollision(BConvexHull& hull, bool &inside, float &collisionDistance, BVector& collisionPoint)
{
   //See if we're already inside.
   if (hull.inside(mpOwner->getPosition()))
   {
      inside=true;
      return (true);
   }
   
   //Check our segments (not checking inside since we've already done that).
   float collisionDistanceSqr=0.0f;
   long numberCollisions=0;
   long collisionSegment=-1;
   collisionDistance=0.0f;
   for (uint k=0; k < mProjectionPath.getSize()-1; k++)
   {
      if (hull.segmentIntersects(mProjectionPath[k], mProjectionPath[k+1], -1,
         collisionPoint, collisionSegment, collisionDistanceSqr, numberCollisions, true, false))
      {
         collisionDistance+=mProjectionPath[k].xzDistance(collisionPoint);
         return (true);
      }
      collisionDistance+=mProjectionPath[k].xzDistance(mProjectionPath[k+1]);
   }
   
   return (false);
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::checkProjectedCollision(BVector& pos, float& collisionDistance)
{
   // Check point on projection hull
   collisionDistance = 0.0f;
   uint numProjections = mProjections.getSize();
   uint numProjPath = mProjectionPath.getSize();
   for (uint i = 0; i < numProjections; i++)
   {      
      if (mProjections[i].inside(pos))
      {
         collisionDistance += mProjectionPath[i].xzDistance(pos);
         return (true);
      }
      else if ((i + 1) < numProjPath)
      {
         collisionDistance += mProjectionPath[i].xzDistance(mProjectionPath[i + 1]);
      }
   }

   return (false);
}

//==============================================================================
// Test to see if our projection contains our final destination point
//==============================================================================
float BSquadActionMove::testFinalDestination(bool& finalDest)
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   finalDest = false;
   BVector finalPoint = mUserLevelWaypoints[mUserLevelWaypoints.getSize() - 1];      
   float testDistance = 0.0f;
   finalDest = checkProjectedCollision(finalPoint, testDistance);
   if (finalDest)
   {
      // Calculate correct velocity to slow down in time to hit final destination point
      float newVelocity = Math::fSqrt(2.0f * pSquad->getAcceleration() * testDistance);
      return ((newVelocity < mVelocity) ? newVelocity / mVelocity : 1.0f);
   }

   return (1.0f);
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::setupOpp()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   #ifdef _MOVE4
   // In the new movement position, create opportunities for physics and turn radius vehicles as well.
   if (gConfig.isDefined(cConfigSlaveUnitPosition) ||
      ((/* pSquad->getProtoSquad()->hasTurnRadius() || */ pSquad->isSquadAPhysicsVehicle()) &&
      (pSquad->getFormationType() != BFormation2::eTypeGaggle)) )
   #else
   if (gConfig.isDefined(cConfigSlaveUnitPosition) ||
      ((pSquad->getProtoSquad()->hasTurnRadius() || pSquad->isSquadAPhysicsVehicle()) &&
      (pSquad->getFormationType() != BFormation2::eTypeGaggle)) )
   #endif
   {
      long numChildren = pSquad->getNumberChildren();
      for (long i = 0; i < numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
            pUnit->setFlagMoving(true);
      }
      return (true);
   }
   
   //Create the unit opp.
   BUnitOpp opp;
   opp.init();
   opp.setTarget(mTarget);
   opp.setType(BUnitOpp::cTypeMove);
   opp.setSource(pSquad->getID());
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   // if we've never set an opportunity, then create a new one
   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      opp.generateID();
      mUnitOppID = opp.getID();            
   }
   // otherwise we want to use the existing id and refresh the same opportunity on any units that have lost the unit move action.
   else
      opp.setID(mUnitOppID);

   if (!addOpp(opp))
   {
      mUnitOppID = BUnitOpp::cInvalidID;
      syncSquadCode("BSquadActionMove::setupOpp cStateFailed 1");
      setState(cStateFailed);
      return (false);
   }

   
   return (true);
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::addOpp(BUnitOpp &opp)
{
   //Give our opp to our units.
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (pSquad->addOppToChildren(opp, mUnitOppIDCount))
   {      
      return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BSquadActionMove::removeOpp()
{
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BDEBUG_ASSERT(pSquad);

   if (gConfig.isDefined(cConfigSlaveUnitPosition) ||
      ((pSquad->getProtoSquad()->hasTurnRadius() || pSquad->isSquadAPhysicsVehicle())
      && (pSquad->getFormationType() != BFormation2::eTypeGaggle)) )
   {
      long numChildren = pSquad->getNumberChildren();
      for (long i = 0; i < numChildren; i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            // Mark physics action to set moving flag when it goes inactive.  If no
            // physics action, just set flag
            BUnitActionPhysics* pPhysAction = reinterpret_cast<BUnitActionPhysics*>(pUnit->getActionByType(BAction::cActionTypeUnitPhysics));
            if (pPhysAction)
               pPhysAction->setFlagEndMoveOnInactivePhysics(true);
            else
               pUnit->setFlagMoving(false);
         }
      }
   }

   if (mUnitOppID == BUnitOpp::cInvalidID)
   {
      return;
   }   

   //Remove the opportunity that we've given the units.  That's all we do here.
   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
   mUnitOppIDCount = 0;
}

#ifdef PRECALC_TURNRADIUS
   //==============================================================================
   //==============================================================================
   BActionState BSquadActionMove::calcTurnRadiusMovement(float elapsedTime, float velocity, BVector& newPosition, BVector& newDirection)
   {
      BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
      BDEBUG_ASSERT(pSquad);

      const BProtoSquad* pPS = mpOwner->getSquad()->getProtoSquad();
      #ifdef _MOVE4
      // In Move4, don't fail here.. just early out.  
      if (!pPS->hasTurnRadius())
         return cStateWorking;
      #else
      if (!pPS->hasTurnRadius())
         return cStateFailed;
      #endif

      // Various velocity stuff unused for now
      // Get allowed distance and turn angle based on velocity and turn rate
      //const BProtoObject* pPO = mpOwner->getProtoObject();
      //float velocityToUse=mpOwner->getDesiredVelocity();
      //velocityToUse*=velocityModifier;
      //float distanceToMove = velocityToUse * elapsedTime;
      //float turnAmountLeft = pPO->getTurnRate() * cRadiansPerDegree * elapsedTime;

      // Update turn segment time scaled by velocity modifier
      float desiredVelocity=pSquad->getDesiredVelocity();
      if (desiredVelocity < cFloatCompareEpsilon)
         return (cStateFailed);
      float velocityRatio=mVelocity/desiredVelocity;
      mTurnSegmentTime += (elapsedTime * velocityRatio);

      // Interpolate position, direction, waypoint using updated time
      uint sourceWaypoint = 0;
      turnRadiusInterpolate(mTurnSegmentTime, newPosition, newDirection, sourceWaypoint);

      // If at the end, set done
      if ((mTurnRadiusTimes.getNumber() < 1) || (mTurnSegmentTime >= mTurnRadiusTimes[mTurnRadiusTimes.getSize() - 1].z))
      {
         mCurrentWaypoint = mPath.getNumberWaypoints();
         return cStateDone;
      }
      // Otherwise, update the current waypoint and set working
      else
      {
         mCurrentWaypoint = sourceWaypoint + 1;
         return cStateWorking;
      }
   }

   //==============================================================================
   //==============================================================================
   void BSquadActionMove::turnRadiusInterpolate(float time, BVector& position, BVector& direction, uint& srcWaypoint)
   {
      // Sanity checks for 0 length or 1 waypoint paths
      // TODO - Rework turn radius data to support 1 waypoint paths
      uint numPathWaypoints = static_cast<uint>(mPath.getNumberWaypoints());
      if (numPathWaypoints == 1)
      {
         position = mPath.getWaypoint(0);
         direction = mpOwner->getForward();
         srcWaypoint = 0;
         return;
      }
      if ((mTurnRadiusTimes.getNumber() < 1) || (numPathWaypoints == 0))
      {
         position = mpOwner->getPosition();
         direction = mpOwner->getForward();
         srcWaypoint = 0;
         return;
      }

      // Clamp time to valid range
      if (time < 0.0f)
         time = 0.0f;
      else if (time >= mTurnRadiusTimes[mTurnRadiusTimes.getSize() - 1].z)
         time = mTurnRadiusTimes[mTurnRadiusTimes.getSize() - 1].z;

      // Determine which waypoints the given time is between
      srcWaypoint = 0;
      while ((srcWaypoint < numPathWaypoints - 2) && (time >= mTurnRadiusTimes[srcWaypoint].z) )
         srcWaypoint++;

      // Start arc
      if (time < mTurnRadiusTimes[srcWaypoint].x)
      {
         // Calculate segment percentage, handle the case where this segment is 0 length
         float totalTime;
         if (srcWaypoint > 0)
            totalTime = mTurnRadiusTimes[srcWaypoint].x - mTurnRadiusTimes[srcWaypoint - 1].z;
         else
            totalTime = mTurnRadiusTimes[srcWaypoint].x;

         float segmentPct;
         if (totalTime <= 0.0f)
            segmentPct = 1.0f;
         else
         {
            if (srcWaypoint > 0)
               segmentPct = (time - mTurnRadiusTimes[srcWaypoint - 1].z) / totalTime;
            else
               segmentPct = time / totalTime;
         }

         // Calculate position from segment percent
         BVector srcTurnCenter = mTurnRadiusArcCenters[srcWaypoint];
         float srcTurnRadius = mTurnRadiusArcData[srcWaypoint].z;
         float srcTurnRadians = mTurnRadiusArcData[srcWaypoint].x;

         // Normally the source point is the waypoint
         BVector sourcePoint = mPath.getWaypoint(srcWaypoint);
         sourcePoint.y = 0.0f;
         // But in cases where the source and dest arcs overlap, we will usually skip past the waypoint to
         // avoid spinning over the same section of turn.  In this case the source point is offset
         // by the overlap angle
         if ((srcWaypoint > 0) && (mTurnRadiusOverlapAngle[srcWaypoint - 1] != 0.0f))
         {
            float remainderAngle = mTurnRadiusOverlapAngle[srcWaypoint - 1];
            float tempAngle = (sourcePoint - srcTurnCenter).getAngleAroundY() + remainderAngle;
            BVector tempVec(sinf(tempAngle), 0.0f, cosf(tempAngle));
            sourcePoint = srcTurnCenter + tempVec * srcTurnRadius;
         }

         float startPtAngle = (sourcePoint - srcTurnCenter).getAngleAroundY() + (srcTurnRadians * segmentPct);
         BVector startPtVec(sinf(startPtAngle), 0.0f, cosf(startPtAngle));
         position = srcTurnCenter + startPtVec * srcTurnRadius;

         // Direction - tangent to position on circle (multiply in backwards factor
         // which is stored in srcTurnCenter.w
         direction = startPtVec;
         if (srcTurnRadians >= 0.0f)
            direction.rotateXZ(cPiOver2 * srcTurnCenter.w);
         else
            direction.rotateXZ(-cPiOver2 * srcTurnCenter.w);
      }
      // Line
      else if (time < mTurnRadiusTimes[srcWaypoint].y)
      {
         // Calculate segment percentage, handle the case where this segment is 0 length
         float totalTime = mTurnRadiusTimes[srcWaypoint].y - mTurnRadiusTimes[srcWaypoint].x;
         float segmentPct;
         if (totalTime <= 0.0f)
            segmentPct = 1.0f;
         else
            segmentPct = (time - mTurnRadiusTimes[srcWaypoint].x) / totalTime;

         // Start point
         BVector srcTurnCenter = mTurnRadiusArcCenters[srcWaypoint];
         float srcTurnRadians = mTurnRadiusArcData[srcWaypoint].x;
         float srcTurnRadius = mTurnRadiusArcData[srcWaypoint].z;
         BVector sourcePoint = mPath.getWaypoint(srcWaypoint);
         sourcePoint.y = 0.0f;
         // Calculate offset source point if there is overlap (see comment in start arc section above)
         if ((srcWaypoint > 0) && (mTurnRadiusOverlapAngle[srcWaypoint - 1] != 0.0f))
         {
            float remainderAngle = mTurnRadiusOverlapAngle[srcWaypoint - 1];
            float tempAngle = (sourcePoint - srcTurnCenter).getAngleAroundY() + remainderAngle;
            BVector tempVec(sinf(tempAngle), 0.0f, cosf(tempAngle));
            sourcePoint = srcTurnCenter + tempVec * srcTurnRadius;
         }
         float startPtAngle = (sourcePoint - srcTurnCenter).getAngleAroundY() + srcTurnRadians;
         BVector startPtVec(sinf(startPtAngle), 0.0f, cosf(startPtAngle));
         BVector startPt = srcTurnCenter + startPtVec * srcTurnRadius;

         // End point
         BVector destWaypoint = mPath.getWaypoint(srcWaypoint + 1);
         destWaypoint.y = 0.0f;
         BVector destTurnCenter;
         float destTurnRadius;
         float destTurnRadians = mTurnRadiusArcData[srcWaypoint].y;
         if (srcWaypoint < numPathWaypoints - 2)
         {
            destTurnCenter = mTurnRadiusArcCenters[srcWaypoint + 1];
            bool nextSrcTurnCW = mTurnRadiusArcData[srcWaypoint + 1].w > 0.0f;
            bool destTurnCW = destTurnRadians >= 0.0f;
            if (nextSrcTurnCW != destTurnCW)
            {
               destTurnCenter = destWaypoint + destWaypoint - destTurnCenter;
            }

            destTurnRadius = mTurnRadiusArcData[srcWaypoint + 1].z;
         }
         else
         {
            destTurnCenter = mPath.getWaypoint(mPath.getNumberWaypoints() - 1);
            destTurnCenter.y = 0.0f;
            destTurnRadius = 0.0f;
         }
         float endPtAngle = (destWaypoint - destTurnCenter).getAngleAroundY() - destTurnRadians;
         BVector endPtVec(sinf(endPtAngle), 0.0f, cosf(endPtAngle));
         BVector endPt = destTurnCenter + endPtVec * destTurnRadius;

         BVector dir = endPt - startPt;
         position = startPt + dir * segmentPct;
         dir.normalize();
         direction = dir;
      }
      // Dest arc
      else
      {
         // Calculate segment percentage, handle the case where this segment is 0 length
         float totalTime = mTurnRadiusTimes[srcWaypoint].z - mTurnRadiusTimes[srcWaypoint].y;
         float segmentPct;
         if (totalTime <= 0.0f)
            segmentPct = 1.0f;
         else
            segmentPct = (time - mTurnRadiusTimes[srcWaypoint].y) / totalTime;

         BVector destWaypoint = mPath.getWaypoint(srcWaypoint + 1);
         destWaypoint.y = 0.0f;
         BVector destTurnCenter;
         float destTurnRadius;
         float destTurnRadians = mTurnRadiusArcData[srcWaypoint].y;
         if (srcWaypoint < numPathWaypoints - 2)
         {
            destTurnCenter = mTurnRadiusArcCenters[srcWaypoint + 1];
            bool nextSrcTurnCW = mTurnRadiusArcData[srcWaypoint + 1].w > 0.0f;
            bool destTurnCW = destTurnRadians >= 0.0f;
            if (nextSrcTurnCW != destTurnCW)
            {
               destTurnCenter = destWaypoint + destWaypoint - destTurnCenter;
            }

            destTurnRadius = mTurnRadiusArcData[srcWaypoint + 1].z;
         }
         else
         {
            destTurnCenter = mPath.getWaypoint(mPath.getNumberWaypoints() - 1);
            destTurnCenter.y = 0.0f;
            destTurnRadius = 0.0f;
         }

         float endPtAngle = (destWaypoint - destTurnCenter).getAngleAroundY() - (destTurnRadians * (1.0f - segmentPct));
         BVector endPtVec(sinf(endPtAngle), 0.0f, cosf(endPtAngle));
         position = destTurnCenter + endPtVec * destTurnRadius;

         // Direction - tangent to position on circle
         direction = endPtVec;
         if (destTurnRadians >= 0.0f)
            direction.rotateXZ(cPiOver2);
         else
            direction.rotateXZ(-cPiOver2);
      }
   }

   //==============================================================================
   //==============================================================================
   void BSquadActionMove::updateTurnRadiusPath(int startWaypointIndex)
   {
      BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
      BDEBUG_ASSERT(pSquad);

      const BProtoSquad* pPS = pSquad->getProtoSquad();
      if (!pPS->hasTurnRadius())
         return;
      float minTurnRadius = pPS->getTurnRadiusMin();
      float maxTurnRadius = pPS->getTurnRadiusMax();
      float desiredVelocity=pSquad->getDesiredVelocity();
      BASSERT(desiredVelocity > 0.0f);
      if (desiredVelocity <= 0.0f)
         return;

      BASSERT(startWaypointIndex >= 0);
      if (startWaypointIndex < 0)
         startWaypointIndex = 0;

      //==========================================================================
      // This method builds the sequence of turn arc + line segments for a 
      // path with turn radius
      //==========================================================================

      int numWaypoints = mPath.getNumberWaypoints();
      BASSERT(numWaypoints > 1);
      if (numWaypoints <= 1)
         return;
      mTurnRadiusArcCenters.resize(numWaypoints - 1); // vector center (x,y,z), backwards flag multiplier (-1 or 1)
      mTurnRadiusArcData.resize(numWaypoints - 1); // src turn angle delta, dest turn angle delta, turn radius, src turn direction (-1 for ccw, 1 for cw - can't just use srcRadians >= 0.0f b/c of 0 length ccw turns)
      mTurnRadiusTimes.resize(numWaypoints - 1);
      mTurnRadiusOverlapAngle.resize(numWaypoints - 1);



      //==========================================================================
      // Get direction vectors at each waypoint, essentially the average between
      // incoming and outgoing vectors.
      // As we're doing this, also determine turn radius at each waypoint
      BDynamicSimVectorArray waypointDirs;
      waypointDirs.resize(numWaypoints);
      waypointDirs[0] = mStartingDir;
      mTurnRadiusArcData[0].z = minTurnRadius;

      // Don't need to update direction at first waypoint - that's stored in mStartingDir
      int startWaypointDirectionIndex = Math::Max(1, startWaypointIndex);
      for (int i = startWaypointDirectionIndex; i < numWaypoints - 1; i++)
      {
         // Direction at waypoint.  Get the average direction of p1p2 and p2p3 vectors, then
         // do a couple cross products to make sure the average vector is between the incoming (p1p2)
         // and outgoing (p2p3) vectors in the optimal direction.
         BVector p1 = mPath.getWaypoint(i - 1);
         p1.y = 0.0f;
         BVector p2 = mPath.getWaypoint(i);
         p2.y = 0.0f;
         BVector p3 = mPath.getWaypoint(i + 1);
         p3.y = 0.0f;
         BVector p1p2 = p2 - p1;;
         BVector p2p3 = p3 - p2;
         float len1 = p1p2.length();
         float len2 = p2p3.length();

         BVector waypointDir;
         float angleBetween;
         if (len1 > 0.0f && len2 > 0.0f)
         {
            // Get angle between.  Do a manual calculation because the XMVector version
            // is giving some bad results.
            float val = Math::Clamp((p1p2.dot(p2p3) / (len1 * len2)), -1.0f, 1.0f);
            angleBetween = acosf(val);

            // Amount to angle waypoint direction towards outgoing vector (p2p3)
            // The waypoint direction should be weighted towards the shorter
            // of the incoming and outgoing vectors, so the outgoing contribution
            // is higher if the incoming vector is larger.
            float outgoingVectorContrib = 0.5f;
            outgoingVectorContrib = len1 / (len1 + len2);

            waypointDir = p1p2;
            if (p1p2.cross(p2p3).y >= 0)
               waypointDir.rotateXZ(angleBetween * outgoingVectorContrib);
            else
               waypointDir.rotateXZ(-angleBetween * outgoingVectorContrib);
         }
         else
         {
            angleBetween = cPi; // set angle to max so we get smallest turn radius below
            // Failure cases
            if (len1 <= 0.0f && len2 > 0.0f)
               waypointDir = p2p3;
            else if (len1 > 0.0f && len2 <= 0.0f)
               waypointDir = p1p2;
            else
               waypointDir = mpOwner->getForward();
         }
         waypointDir.normalize();
         waypointDirs[i] = waypointDir;

         //==========================================================================
         // Angle based turn radius - large angles between should give small turn radius
         float turnRadiusLerpFactor = 1.0f - Math::Clamp(angleBetween / cPi, 0.0f, 1.0f);
         float turnRadius = minTurnRadius + turnRadiusLerpFactor * (maxTurnRadius - minTurnRadius);
         // Don't allow turn radius to be bigger than the distance between waypoints, otherwise
         // we'll get big turns that go in bad directinos just to hit waypoints.
         // TODO - these close waypoints may want to be skipped altogether instead of trying to
         // force the turn path to hit them all
         turnRadius = Math::Min(turnRadius, len1 * 0.5f);
         turnRadius = Math::Min(turnRadius, len2 * 0.5f);
         mTurnRadiusArcData[i].z = turnRadius;
      }
      waypointDirs[numWaypoints - 1] = waypointDirs[numWaypoints - 2]; // last direction is same as next to last (no turn - straight shot)

      //==========================================================================
      // Given waypoint vectors and turn radii, calculate the optimal turn
      // directions and line segments between turns
      for (int i = startWaypointIndex; i < numWaypoints - 1; i++)
      {
         // Start and end point data
         BVector p1 = mPath.getWaypoint(i);
         p1.y = 0.0f;
         BVector p2 = mPath.getWaypoint(i + 1);
         p2.y = 0.0f;
         BVector dir1 = waypointDirs[i];
         BVector dir2 = waypointDirs[i + 1];
         float turnRadius1 = mTurnRadiusArcData[i].z;
         float turnRadius2;
         if (i < numWaypoints - 2)
            turnRadius2 = mTurnRadiusArcData[i + 1].z;
         else
            turnRadius2 = 0.0f;

         BVector right1 = dir1;
         right1.rotateXZ(cPiOver2);
         BVector right2 = dir2;
         right2.rotateXZ(cPiOver2);
         BVector turnCenter1CW = p1 + right1 * turnRadius1;
         BVector turnCenter1CCW = p1 - right1 * turnRadius1;
         BVector turnCenter2CW = p2 + right2 * turnRadius2;
         BVector turnCenter2CCW = p2 - right2 * turnRadius2;

         // Given starting / ending points with turn radius, there are 4 possible paths
         // around the to sets of turn circles.  This involves finding the common tangents
         // between circles.  Between 2 circles there are 2 internal and 2 external tangents
         // depending on the direction of movement around the circles.
         // External (CW / CW, CCW / CCW), Internal (CW / CCW, CCW / CW)
         float startAngleCCW = right1.getAngleAroundY();
         float startAngleCW = (-right1).getAngleAroundY();
         float endAngleCCW = right2.getAngleAroundY();
         float endAngleCW = (-right2).getAngleAroundY();
         float pathLengths[8];
         float srcArcRadians[4];
         float destArcRadians[4];
         float lineLengths[4];

         //==========================================================================
         // CW / CW - External 1 - This exists as long as one circle is not fully contained
         // in the other circle.
         BVector p = turnCenter2CW - turnCenter1CW;
         float pLength = p.length();
         if ((pLength > cFloatCompareEpsilon) && (pLength >= fabs(turnRadius1 - turnRadius2))) // check for existence
         {
            float theta;
            if (turnRadius1 > turnRadius2) // First circle bigger
            {
               //p = turnCenter2CW - turnCenter1CW; already set
               theta = acosf((turnRadius1 - turnRadius2) / pLength);
               p.normalize();
               p.rotateXZ(-theta); // CW
            }
            else
            {
               p = turnCenter1CW - turnCenter2CW;
               theta = acosf((turnRadius2 - turnRadius1) / pLength);
               p.normalize();
               p.rotateXZ(theta); // CW
            }
            BVector startPt1 = turnCenter1CW + p * turnRadius1;
            BVector endPt1 = turnCenter2CW + p * turnRadius2;

            //==========================================================================
            // Calculate path segment lengths

            // Src arc
            float srcAngle1 = (startPt1 - turnCenter1CW).getAngleAroundY(); // end of source turn
            if (srcAngle1 < startAngleCW)
               srcAngle1 += cTwoPi;
            srcArcRadians[0] = srcAngle1 - startAngleCW;
            if (Math::EqualTol(srcArcRadians[0], cTwoPi, 0.001f))
               srcArcRadians[0] = 0.0f;

            // Dest arc - handle 0 dest turn radius for last waypoint
            if (turnRadius2 > 0.0f)
            {
               float destAngle1 = (endPt1 - turnCenter2CW).getAngleAroundY(); // start of dest turn
               if (destAngle1 > endAngleCW)
                  destAngle1 -= cTwoPi;
               destArcRadians[0] = endAngleCW - destAngle1;
               if (Math::EqualTol(destArcRadians[0], cTwoPi, 0.001f))
                  destArcRadians[0] = 0.0f;
            }
            else
               destArcRadians[0] = 0.0f;

            // Line, whole path
            lineLengths[0] = (endPt1 - startPt1).length();
            pathLengths[0] = srcArcRadians[0] * turnRadius1 + destArcRadians[0] * turnRadius2 + lineLengths[0];
            // Calculate path starting backwards.  Reverse the starting turn by adding new turn and subtracting old turn from the path length.
            pathLengths[4] = pathLengths[0] + (cTwoPi - srcArcRadians[0] - srcArcRadians[0]) * turnRadius1;
         }
         else
         {
            pathLengths[0] = FLT_MAX;
            pathLengths[4] = FLT_MAX;
         }

         //==========================================================================
         // CCW / CCW - External 2 - This exists as long as one circle is not fully contained
         // in the other circle.
         p = turnCenter2CCW - turnCenter1CCW;
         pLength = p.length();
         if ((pLength > cFloatCompareEpsilon) && (pLength >= fabs(turnRadius1 - turnRadius2))) // check for existence
         {
            float theta;
            if (turnRadius1 > turnRadius2) // First circle bigger
            {
               //p = turnCenter2CCW - turnCenter1CCW; already set
               theta = acosf((turnRadius1 - turnRadius2) / pLength);
               p.normalize();
               p.rotateXZ(theta); // CCW
            }
            else
            {
               p = turnCenter1CCW - turnCenter2CCW;
               theta = acosf((turnRadius2 - turnRadius1) / pLength);
               p.normalize();
               p.rotateXZ(-theta); // CCW
            }
            BVector startPt2 = turnCenter1CCW + p * turnRadius1;
            BVector endPt2 = turnCenter2CCW + p * turnRadius2;

            //==========================================================================
            // Calculate path segment lengths

            // Src arc
            float srcAngle2 = (startPt2 - turnCenter1CCW).getAngleAroundY(); // end of source turn
            if (srcAngle2 > startAngleCCW)
               srcAngle2 -= cTwoPi;
            srcArcRadians[1] = startAngleCCW - srcAngle2;
            if (Math::EqualTol(srcArcRadians[1], cTwoPi, 0.001f))
               srcArcRadians[1] = 0.0f;
            
            // Dest arc - handle 0 dest turn radius for last waypoint
            if (turnRadius2 > 0.0f)
            {
               float destAngle2 = (endPt2 - turnCenter2CCW).getAngleAroundY(); // start of dest turn
               if (destAngle2 < endAngleCCW)
                  destAngle2 += cTwoPi;
               destArcRadians[1] = destAngle2 - endAngleCCW;
               if (Math::EqualTol(destArcRadians[1], cTwoPi, 0.001f))
                  destArcRadians[1] = 0.0f;
            }
            else
               destArcRadians[1] = 0.0f;

            // Line, whole path
            lineLengths[1] = (endPt2 - startPt2).length();
            pathLengths[1] = srcArcRadians[1] * turnRadius1 + destArcRadians[1] * turnRadius2 + lineLengths[1];
            // Calculate path starting backwards.  Reverse the starting turn by adding new turn and subtracting old turn from the path length.
            pathLengths[5] = pathLengths[1] + (cTwoPi - srcArcRadians[1] - srcArcRadians[1]) * turnRadius1;
         }
         else
         {
            pathLengths[1] = FLT_MAX;
            pathLengths[5] = FLT_MAX;
         }

         //==========================================================================
         // CW / CCW - Internal 1 - Internal tangents exist if the circles intersect
         // in no more than one point, and that point must be on the outside of the circles.
         p = turnCenter2CCW - turnCenter1CW;
         pLength = p.length();
         if ((pLength > cFloatCompareEpsilon) && (pLength >= (turnRadius1 + turnRadius2)))
         {
            //p = turnCenter2CCW - turnCenter1CW; already set
            float theta = acosf((turnRadius1 + turnRadius2) / pLength);
            p.normalize();
            p.rotateXZ(-theta); // CW

            BVector startPt3 = turnCenter1CW + p * turnRadius1;
            BVector endPt3 = turnCenter2CCW - p * turnRadius2;

            //==========================================================================
            // Calculate path segment lengths

            // Src arc
            float srcAngle3 = (startPt3 - turnCenter1CW).getAngleAroundY(); // end of source turn
            if (srcAngle3 < startAngleCW)
               srcAngle3 += cTwoPi;
            srcArcRadians[2] = srcAngle3 - startAngleCW;
            if (Math::EqualTol(srcArcRadians[2], cTwoPi, 0.001f))
               srcArcRadians[2] = 0.0f;

            // Dest arc - handle 0 dest turn radius for last waypoint
            if (turnRadius2 > 0.0f)
            {
               float destAngle3 = (endPt3 - turnCenter2CCW).getAngleAroundY(); // start of dest turn
               if (destAngle3 < endAngleCCW)
                  destAngle3 += cTwoPi;
               destArcRadians[2] = destAngle3 - endAngleCCW;
               if (Math::EqualTol(destArcRadians[2], cTwoPi, 0.001f))
                  destArcRadians[2] = 0.0f;
            }
            else
               destArcRadians[2] = 0.0f;

            // Line, whole path
            lineLengths[2] = (endPt3 - startPt3).length();
            pathLengths[2] = srcArcRadians[2] * turnRadius1 + destArcRadians[2] * turnRadius2 + lineLengths[2];
            // Calculate path starting backwards.  Reverse the starting turn by adding new turn and subtracting old turn from the path length.
            pathLengths[6] = pathLengths[2] + (cTwoPi - srcArcRadians[2] - srcArcRadians[2]) * turnRadius1;
         }
         else
         {
            pathLengths[2] = FLT_MAX;
            pathLengths[6] = FLT_MAX;
         }

         //==========================================================================
         // CCW / CW - Internal 2 - Internal tangents exist if the circles intersect
         // in no more than one point, and that point must be on the outside of the circles.
         p = turnCenter2CW - turnCenter1CCW;
         pLength = p.length();
         if ((pLength > cFloatCompareEpsilon) && (pLength >= (turnRadius1 + turnRadius2)))
         {
            //p = turnCenter2CW - turnCenter1CCW; already set
            float theta = acosf((turnRadius1 + turnRadius2) / pLength);
            p.normalize();
            p.rotateXZ(theta); // CCW

            BVector startPt4 = turnCenter1CCW + p * turnRadius1;
            BVector endPt4 = turnCenter2CW - p * turnRadius2;

            //==========================================================================
            // Calculate path segment lengths

            // Src arc
            float srcAngle4 = (startPt4 - turnCenter1CCW).getAngleAroundY(); // end of source turn
            if (srcAngle4 > startAngleCCW)
               srcAngle4 -= cTwoPi;
            srcArcRadians[3] = startAngleCCW - srcAngle4;
            if (Math::EqualTol(srcArcRadians[3], cTwoPi, 0.001f))
               srcArcRadians[3] = 0.0f;

            // Dest arc - handle 0 dest turn radius for last waypoint
            if (turnRadius2 > 0.0f)
            {
               float destAngle4 = (endPt4 - turnCenter2CW).getAngleAroundY(); // start of dest turn
               if (destAngle4 > endAngleCW)
                  destAngle4 -= cTwoPi;
               destArcRadians[3] = endAngleCW - destAngle4;
               if (Math::EqualTol(destArcRadians[3], cTwoPi, 0.001f))
                  destArcRadians[3] = 0.0f;
            }
            else
               destArcRadians[3] = 0.0f;

            // Line, whole path
            lineLengths[3] = (endPt4 - startPt4).length();
            pathLengths[3] = srcArcRadians[3] * turnRadius1 + destArcRadians[3] * turnRadius2 + lineLengths[3];
            // Calculate path starting backwards.  Reverse the starting turn by adding new turn and subtracting old turn from the path length.
            pathLengths[7] = pathLengths[3] + (cTwoPi - srcArcRadians[3] - srcArcRadians[3]) * turnRadius1;
         }
         else
         {
            pathLengths[3] = FLT_MAX;
            pathLengths[7] = FLT_MAX;
         }

         //==========================================================================
         // Calculate the shortest of the valid paths
         int shortestPath = 0;
         if (pathLengths[1] < pathLengths[shortestPath])
            shortestPath = 1;
         if (pathLengths[2] < pathLengths[shortestPath])
            shortestPath = 2;
         if (pathLengths[3] < pathLengths[shortestPath])
            shortestPath = 3;

         bool backwardsSrcTurn = false;
         bool checkBackPaths = false;
         //check backwards paths at start waypoint and we're not gaggling.
         if ((i == 0) && (pSquad->getFormationType() != BFormation2::eTypeGaggle))
            checkBackPaths = true;
         if (checkBackPaths)
         {
            if (pathLengths[4] < pathLengths[shortestPath])
               shortestPath = 4;
            if (pathLengths[5] < pathLengths[shortestPath])
               shortestPath = 5;
            if (pathLengths[6] < pathLengths[shortestPath])
               shortestPath = 6;
            if (pathLengths[7] < pathLengths[shortestPath])
               shortestPath = 7;
            if (shortestPath > 3)
            {
               backwardsSrcTurn = true;
               shortestPath = shortestPath % 4;
            }
         }

         BASSERT(shortestPath >= 0 && shortestPath < 4);

         // If no valid paths found, the current and future waypoints are probably on top of
         // each other.  In this case we just snap to the next waypoint (0 radian src arc, 0 length line, 0 radian dest arc)
         if (pathLengths[shortestPath] == FLT_MAX)
         {
            mTurnRadiusArcCenters[i] = turnCenter1CW;
            mTurnRadiusArcCenters[i].w = 1.0f; // fwd movement
            mTurnRadiusArcData[i].x = 0.0f; // src arc radians
            mTurnRadiusArcData[i].y = 0.0f; // dest arc radians
            mTurnRadiusArcData[i].w = 1.0f; // source turn cw
            lineLengths[0] = 0.0f;
            shortestPath = 0;
         }
         // Otherwise setup for shortest path
         else
         {
            /*
            // Debug stuff
            if (srcArcRadians[shortestPath] > cThreePiOver2)
               trace("blah");
            if (destArcRadians[shortestPath] > cThreePiOver2)
               trace("blah");
            */

            //==========================================================================
            // Now we have the path to use, set the turn circle center, start / end radians, line dist etc

            // Turn center and cw/ccw flag in w
            if (shortestPath == 0 || shortestPath == 2)
            {
               mTurnRadiusArcCenters[i] = turnCenter1CW;
               mTurnRadiusArcData[i].w = 1.0f;
            }
            else
            {
               mTurnRadiusArcCenters[i] = turnCenter1CCW;
               mTurnRadiusArcData[i].w = -1.0f;
            }

            // Source turn arc radians
            if (shortestPath == 0 || shortestPath == 2)
            {
               if (backwardsSrcTurn)
               {
                  mTurnRadiusArcData[i].x = srcArcRadians[shortestPath] - cTwoPi;
                  mTurnRadiusArcCenters[i].w = -1.0f;
               }
               else
               {
                  mTurnRadiusArcData[i].x = srcArcRadians[shortestPath];
                  mTurnRadiusArcCenters[i].w = 1.0f;
               }
            }
            else
            {
               if (backwardsSrcTurn)
               {
                  mTurnRadiusArcData[i].x = cTwoPi - srcArcRadians[shortestPath];
                  mTurnRadiusArcCenters[i].w = -1.0f;
               }
               else
               {
                  mTurnRadiusArcData[i].x = -srcArcRadians[shortestPath];
                  mTurnRadiusArcCenters[i].w = 1.0f;
               }
            }

            // Dest turn arc radians
            if (shortestPath == 0 || shortestPath == 3)
               mTurnRadiusArcData[i].y = destArcRadians[shortestPath];
            else
               mTurnRadiusArcData[i].y = -destArcRadians[shortestPath];

            // Calculate any src and previous waypoint dest arc overlap.  This is used to skip the
            // overlapping section in the interpolate function
            mTurnRadiusOverlapAngle[i] = 0.0f;
            if (i > 0)
            {
               // Summed turn of previous destination and current source arcs.  Should be within +/- 2pi
               float totalTurnAngle = mTurnRadiusArcData[i].x * mTurnRadiusArcCenters[i].w + mTurnRadiusArcData[i - 1].y;
               if (totalTurnAngle > cTwoPi)
               {
                  float remainderAngle = cTwoPi - mTurnRadiusArcData[i - 1].y;
                  mTurnRadiusArcData[i].x -= remainderAngle;
                  mTurnRadiusTimes[i - 1].z = mTurnRadiusTimes[i - 1].y; // zero out the prev dest arc as it is getting added to the src arc
                  mTurnRadiusOverlapAngle[i - 1] = remainderAngle;
               }
               else if (totalTurnAngle < -cTwoPi)
               {
                  float remainderAngle = -cTwoPi - mTurnRadiusArcData[i - 1].y;
                  mTurnRadiusArcData[i].x -= remainderAngle;
                  mTurnRadiusTimes[i - 1].z = mTurnRadiusTimes[i - 1].y; // zero out the prev dest arc as it is getting added to the src arc
                  mTurnRadiusOverlapAngle[i - 1] = remainderAngle;
               }
            }
         }
           

         // Calculate and store "time" values at each segment end.  These times are the maximum amount
         // of time to travel the distance or turn the number or radians along the segment at maximum
         // velocity and maximum turn rate.  These time values are used to interpolate along the turn
         // radius path.
         float startTime = 0.0f;
         if (i > 0)
            startTime = mTurnRadiusTimes[i - 1].z;
         float maxTurnSpeed = 1.0f;
         const BProtoObject* pPO = mpOwner->getProtoObject();
         BASSERT(pPO);
         if (pPO)
            maxTurnSpeed = pPO->getTurnRate() * cRadiansPerDegree;
         float absSrcRadians = fabs(mTurnRadiusArcData[i].x);
         float absDestRadians = fabs(mTurnRadiusArcData[i].y);
         mTurnRadiusTimes[i].x = startTime + Math::Max((absSrcRadians * turnRadius1) / desiredVelocity, absSrcRadians / maxTurnSpeed);
         mTurnRadiusTimes[i].y = mTurnRadiusTimes[i].x + (lineLengths[shortestPath] / desiredVelocity);
         mTurnRadiusTimes[i].z = mTurnRadiusTimes[i].y + Math::Max((absDestRadians * turnRadius2) / desiredVelocity, absDestRadians / maxTurnSpeed);
      }
   }
#endif

//==============================================================================
//==============================================================================
void  BSquadActionMove::updateMovementSound()
{
   if (!mpOwner)
      return;

   //-- See if the unit has a physics object that we need to check
   const BSquad* pSquad= mpOwner->getSquad();
   if(pSquad)
   {
      bool visible = true;
      const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
      if( pUser && !pSquad->isVisible(pUser->getTeamID()) )
         visible = false;

      for(uint i=0; i < pSquad->getNumberChildren(); i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if(!pUnit)
            continue;

         const BProtoObject* pProto = pUnit->getProtoObject();
         BPhysicsObject* pPhysObject = pUnit->getPhysicsObject();
         if(!pPhysObject || !pProto)
            continue;
         
         const BPhysicsInfo* pPhysInfo = gPhysicsInfoManager.get(pProto->getPhysicsInfoID(), true);
         if(!pPhysInfo)
            continue;

         if(pPhysInfo->getVehicleType() == BPhysicsInfo::cWarthog)
         {
            if(pPhysObject->getNumActions() <= 0)
               continue;

            BPhysicsWarthogAction* pPhysWarthog = reinterpret_cast<BPhysicsWarthogAction*>(pPhysObject->getAction(0));
            if(pPhysWarthog)
            {
               //-- See if it's jumping
               if(pPhysWarthog->getFlagInAir() && !mFlagInAir)
               {
                  if( visible )  //only start the in air sound if we are visible.
                  {
                     //gConsoleOutput.debug("warthog in air");
                     //-- Play the in air sound
                     BCueIndex index = pUnit->getProtoObject()->getSound(cObjectSoundJump);
                     BRTPCInitArray rtpc;
                     rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
                     gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, true, &rtpc); 
                  }

                  mFlagInAir = true;
               }
               else if(!pPhysWarthog->getFlagInAir() && mFlagInAir)
               {
                  BVector linVel;
                  pPhysWarthog->getPrevLinearVelocity(linVel);
                  //gConsoleOutput.debug("warthog lin vel at land: (%f, %f, %f)", linVel.x, linVel.y, linVel.z);

                  BRTPCInitArray rtpc;
                  rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
                  BCueIndex index = pUnit->getProtoObject()->getSound(cObjectSoundLand);
                  gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, false, &rtpc);

                  if( visible )
                  {
                     BRTPCInitArray rtpc2;
                     rtpc2.add( BRTPCInit(AK::GAME_PARAMETERS::RAM_VELOCITY, Math::fAbs(linVel.y)) );
                     index = pUnit->getProtoObject()->getSound(cObjectSoundLandHard);
                     gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, false, &rtpc2);
                  }

                  mFlagInAir = false;
               }

               //-- See if it's skidding
               if(pPhysWarthog->getFlagSkidding()&& !mFlagSkidding)
               {
                  BCueIndex index = pUnit->getProtoObject()->getSound(cObjectSoundSkidOn);
                  BCueIndex stopIndex = pUnit->getProtoObject()->getSound(cObjectSoundSkidOff);
                  gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, stopIndex, false, true);
                  mFlagSkidding = true;
               }
               else if(!pPhysWarthog->getFlagSkidding()&& mFlagSkidding)
               {
                  BCueIndex index = pUnit->getProtoObject()->getSound(cObjectSoundSkidOff);
                  gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, false, cInvalidCueIndex, false, false);
                  mFlagSkidding = false;
               }
            }
         }
      }
   }

}

//==============================================================================
//==============================================================================
bool BSquadActionMove::updatePathingLimits()
{
   /* DLM 11/10/08 - this entire function obsolute.

   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BPathingLimiter *pathLimiter = gWorld->getPathingLimiter();
   if (pathLimiter)
   {
      if (pathLimiter->getNumPathingCallsThisFrame() >= gDatabase.getMaxSquadPathingCallsPerFrame())
      {
         // If this is the first delay in this pathing attempt, increment global counter
         if (mNumPathingDelays == 0)
            pathLimiter->incNumDelayedPathingAttempts();

         mNumPathingDelays++;
         if (mNumPathingDelays > mMaxPathingDelays)
            mMaxPathingDelays = mNumPathingDelays;
         if (mNumPathingDelays > pathLimiter->getMaxNumPathingDelays())
            pathLimiter->setMaxNumPathingDelays(mNumPathingDelays);
         if (mNumPathingDelays > pathLimiter->getMaxRecentPathingDelays())
         {
            pathLimiter->setMaxRecentPathingDelays(mNumPathingDelays);
            pathLimiter->setTimeOfLastMaxPathDelay(timeGetTime());
         }
         long playerID = pSquad->getPlayerID();
         pathLimiter->incSquadFramesDenied(playerID);
         if (pathLimiter->getSquadFramesDenied(playerID) > pathLimiter->getMaxSquadFramesDenied(playerID))
            pathLimiter->setMaxSquadFramesDenied(playerID, pathLimiter->getSquadFramesDenied(playerID));

         return false;
      }
      else
      {
         if (mNumPathingDelays > 0)
            pathLimiter->addToTotalNumPathingDelays(mNumPathingDelays);

         mNumPathingDelays = 0;
      }
   }
   */
   return true;
}

//==============================================================================
// Setup the child order based on the parent order
//==============================================================================
void BSquadActionMove::setupChildOrder(BSimOrder* pOrder)
{
   // TRB 3/11/08  This was crashing because the child change mode order was still executing
   // when the move action was disconnected.  The order should always be non-NULL so keep the
   // assert but check the pointer just in case to prevent crashing.
   BASSERT(pOrder);
   if (!mpChildOrder && pOrder)
   {
      mpChildOrder = gSimOrderManager.createOrder();
      BASSERT(mpChildOrder);
      mpChildOrder->incrementRefCount();
      mpChildOrder->setOwnerID(pOrder->getOwnerID());
      mpChildOrder->setPriority(pOrder->getPriority());
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::isStillBlocked()
{
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);

   if (mBlockingEntityID != cInvalidObjectID)
   {
      const BEntity* pBlocker = gWorld->getEntity(mBlockingEntityID);
      if (pBlocker != NULL)
      {
         // Make sure blocking entity is still moving
         const BSquadActionMove *pBlockerMoveAction = reinterpret_cast<const BSquad *>(pBlocker)->getNonPausedMoveAction_4();
         if ((pBlockerMoveAction == NULL) || (pBlockerMoveAction->getState() == cStateBlocked) || (pBlockerMoveAction->getProjections().getSize() == 0))
            return false;

         // Distance check.  See if the blocker is still close enough to care about.
         float dist = pSquad->calculateXZDistance(pBlocker);
         if (dist < Math::Min(pSquad->getObstructionRadiusX(), pSquad->getObstructionRadiusZ()))
         {
            // Direction check.  See if blocker is still in front of this squad.
            BVector dir = pBlocker->getPosition() - pSquad->getPosition();
            dir.y = 0.0f;
            dir.normalize();
            float angle = (float) fabs(pSquad->getForward().angleBetweenVector(dir));
            if (angle < cPi)
               return true;
         }
      }
   }
   return false;
}

#ifdef _MOVE4
//==============================================================================
// update_4 - temporary new version of update
//==============================================================================
bool BSquadActionMove::update_4(float elapsedTime)
{
   SCOPEDSAMPLE(BSquadActionMove_update);
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   BActionState startingState = mState;

   switch(mState)
   {
      case cStateNone:
      {
         // Lock/Unlock Hackery from old Unit Update.  If we're already ready to lockdown (or unlock) deal with it.
         // Update outside playable bounds flag
         const BPlayer* pPlayer = pSquad->getPlayer();
         if (pPlayer && !pPlayer->isHuman())
         {
            mFlagLastOutsidePlayableBounds = pSquad->isOutsidePlayableBounds();
         }

         if (mFlagAutoMode && pSquad->getSquadMode()!=mAutoMode && !mFlagAutoAttackMode)
         {
            mChangeModeState = cStatePathing;
            if (mAutoMode == BSquadAI::cModeNormal)
               setState(cStateUnlock);
            else
               setState(cStateLockDown);
            return true;
         }

         if (mFlagAutoUnlock)
         {
            if (pSquad->isLockedDown())
            {
               mChangeModeState = cStatePathing;
               setState(cStateUnlock);
               return true;
            }
            mFlagAutoUnlock = false;
         }
         
         setState(cStateWorking);
         // DLM 7/3/08 - Intentionally fall through to the working state,
         // to allow the squad to get a single movement update on the *first* update, and to generate a unit
         // opportunity on the first update.  This will prevent units from "hitching" when they are recommanded.
         // break;
      }
      case cStateWorking:
      {
         //If we cannot move, then fail.
         if (!pSquad->canMove())
         {
            syncSquadCode("BSquadActionMove::update_4 cStateFailed 1");
            setState(cStateFailed);
            break;
         }

         // If we should unlock, do so.         
         const BUnit* pLeader = pSquad->getLeaderUnit();
         if (pLeader && pLeader->isLockedDown() && pLeader->canAutoUnlock())
         {
            mChangeModeState = cStateWorking;
            setState(cStateUnlock);
            break;
         }

         BActionState newState = doWorkUpdate_4(elapsedTime);
         bool squadMoved = pSquad->getFlagMoved();
         if (newState != mState)
         {
            setState(newState);
         }
         
         // jce [5/7/2008] -- repeat this check here since sometimes state can have changed
         // It can't be in the else clause because sometimes we've SET wait state already then
         // returned newState as wait anyway.  Convoluted, yes.
         if(mState == cStateWorking)
         {
            // If we havne't yet created opportunities for our units to move us,
            // do so.  I'ts important we do this after doWorkUpdate, so that the
            // opportunites to move towards the squad location are done *after*
            // the squad has been updated.             
            if (mUnitOppIDCount < pSquad->getNumberChildren())
               setupOpp();            
         }

         // If this went from state none to working to done in one update then make sure
         // tie to ground got called.  Otherwise it's possible the squad's y value is invalid.
         if (squadMoved && (startingState == cStateNone) && ((mState == cStateDone) || (mState == cStateFailed)) && !pSquad->getFlagUseMaxHeight())
            pSquad->tieToGround();

         break;
      }
      case cStatePathing:
      {
         setState(cStateWorking);
         break;
      }

      // Wait until all opps have reported and turn radius is done updating
      case cStateWaitOnChildren:
      {
         bool turnRadiusDone = true;
         #ifdef NEW_TURNRADIUS
            BActionState turnState = pSquad->getTurnRadiusState();
            turnRadiusDone = ((turnState == cStateDone) || (turnState == cStateFailed));
         #endif

         if (turnRadiusDone && (mUnitOppIDCount <= 0))
         {
            if (mFlagFailed)
            {
               syncSquadCode("BSquadActionMove::update_4 cStateFailed 2");
               setState(cStateFailed);
            }
            else
            {
               debugMove4("InStateWaitOnChildren, no opp counts and turn Radius is done, so setting state to Done.");
               syncSquadCode("BSquadActionMove::update_4 cStateDone 1");
               setState(cStateDone);
            }
         }
         break;
      }
      case cStateWait:
      {
         // Convert to milliseconds elapsed.
         DWORD elapsedMS = 1000.0f*elapsedTime;
         
         // Check time remaining.
         if (mPauseTimeRemaining_4 <= (DWORD)elapsedMS)
         {
            // Time's up, back to work.
            // Reset timer to 0 here for good measure (we don't always just subtract because negative = giant unsigned number)
            debugMove4("Coming out of wait state.");
            mPauseTimeRemaining_4 = 0;
            
            #ifndef BUILD_FINAL
            mPausedReason = -1;
            mPausedByID.invalidate();
            #endif
            
            setState(cStateWorking);
         }
         else
         {
            // Update the timer
            mPauseTimeRemaining_4 -= elapsedMS;
            debugMove4("In wait state, time remaining=%d.", mPauseTimeRemaining_4);
         }

         break;
      }
   }
   /*
   // Hacktabulous slaveUnitPosition crap from other update.. 
   if (gConfig.isDefined(cConfigSlaveUnitPosition) ||
      (mpOwner->getSquad()->getProtoSquad()->hasTurnRadius() && (mpOwner->getSquad()->getFormationType() != BFormation2::eTypeGaggle)) )
   {
      // Don't whack unit position if it's a physics vehicle
      if (!pSquad->isSquadAPhysicsVehicle())
         pSquad->whackUnitPositions(elapsedTime);
   }
   */

   updateMovementSound();

   return true;
}

//==============================================================================
//==============================================================================
BActionState BSquadActionMove::doWorkUpdate_4(float elapsedTime)
{
   // This wraps the internal function to make sure all exit points end() the obstruction manager if needed without
   // having 200 cases check this or some kind of goto cleanup type of thing.
   
   // First do the real work update.
   BActionState result = doWorkUpdateInternal_4(elapsedTime);
   
   // Now end the obstruction manager if needed.
   if(gObsManager.inUse())
      gObsManager.end();
   
   // Hand back the result.
   return(result);
}


//==============================================================================
// doWorkUpdate - handles the case of cStateWork in update..
//==============================================================================
BActionState BSquadActionMove::doWorkUpdateInternal_4(float elapsedTime)
{
   // Temporary Fill in for doWorkUpdate.. currently this only moves us to our target. 
   SCOPEDSAMPLE(BSquadActionMove_update);
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("doWorkUpdate_4 -- no squad, failing.");
      syncSquadCode("doWorkUpdate_4 -- no squad, failing.");
      return cStateFailed;
   }

   // For NonPlatoon moves, so that we can use the remaining logic,
   // set our interimTarget just to be the same as our actual target.
   if (!mFlagPlatoonMove)
   {
      // If we don't have a platoon move, we *should* have a target.  An entity.. position.. something. 
      if (mTarget.isIDValid())
      {
         BEntity *pEntity = gWorld->getEntity(mTarget.getID());
         if (!pEntity)
         {
            debugMove4("doWorkUpdate_4 -- Invalid Target EntityID in action.  Failing.");
            syncSquadCode("doWorkUpdate_4 -- Invalid Target EntityID in action.  Failing.");
            return cStateFailed;
         }
         mInterimTarget_4 = pEntity->getPosition();
      }
      else if (mTarget.isPositionValid())
         mInterimTarget_4 = mTarget.getPosition();
      else
      {
         debugMove4("doWorkUpdate_4 - Action failed, because we have a non platoon move that does not have a target.");
         syncSquadCode("doWorkUpdate_4 - Action failed, because we have a non platoon move that does not have a target.");
         return cStateFailed;
      }
      debugMove4("doWorkUpdate_4 -- This is not a platoon move -- proceeding to location (%f, %f, %f)", mInterimTarget_4.x, mInterimTarget_4.y, mInterimTarget_4.z);
      syncSquadData("doWorkUpdate_4 -- This is not a platoon move -- proceeding to location ", mInterimTarget_4);
   }
   /*
   #ifndef BUILD_FINAL
   static BProtoObjectID cleansingProtoID1 = gDatabase.getProtoObject("pow_gp_cleansing_01");
   static BProtoObjectID cleansingProtoID2 = gDatabase.getProtoObject("pow_gp_cleansing_02");
   static BProtoObjectID cleansingProtoID3 = gDatabase.getProtoObject("pow_gp_cleansing_03");

   BProtoObjectID protoID = pSquad->getProtoID();
   BUnit *pLeaderUnit = pSquad->getLeaderUnit();
   if (pLeaderUnit)
      protoID = pLeaderUnit->getProtoID();
   if (protoID == cleansingProtoID1 || protoID == cleansingProtoID2 || protoID == cleansingProtoID3)
   {
      BASSERTM(0, "We're attempting to connect a move action to the cleansing power.  Should not be trying to do this.");
      return cStateFailed;
   }
   #endif
   */

   // Get platoon.
   BPlatoon *pPlatoon = pSquad->getParentPlatoon();
   if (!pPlatoon && mFlagPlatoonMove)
   {
      debugMove4("doWorkUpdate_4 -- no platoon, failing.");
      syncSquadCode("doWorkUpdate_4 -- no platoon, failing.");
      return cStateFailed;
   }
   
   // Get our current status.
   BVector position = pSquad->getPosition();

   // Get destination.
   BVector destination;

   // Assume we'll stay working unless we figure out otherwise.
   BActionState returnState = cStateWorking;
   
   // This is a flag used for a little optimization.  It will be set true if we are on a specific path
   // and find we have a clear check to our interim target.  In that case, we can skip the more relaxed
   // checking to destination that occurs later since this is such a conservative check.
   bool segCheckedOkToInterimTarget = false;
   bool forceRepath = false;

   // Figure out what type of obstructions we care about.
   // DLM - Okay jce has convinced me the current logic may be "less than optimal"  Revert back to (forward to?) Age4/3 logic of first determining if I'm
   // following a path, and only doing the long nasty obstruction check to our Interim Target location if we're following the path. 
   if (mFlagFollowingSpecificPath_4)
   {

      syncSquadCode("doWorkUpdate_4 -- Following Specific path...");

      // We're following a path, lets see if we can get off of it.. 
        
      // Assume we'll keep this specific path.
      bool keepSpecificPath = true;
            
      // See how far it is from our path's endpoint to the current formation position.
      float distSqr = mFormationPositionWhenSpecificPathWasCreated_4.xzDistanceSqr(mInterimTarget_4);
      
      // If the formation position has moved a good amount from where our path is headed, time to forget it.
      const float cDistThresholdSqr = 256.0f;
      if(distSqr > cDistThresholdSqr)
      {
         // Drop this path.
         keepSpecificPath = false;
         
         // Force a repath later when we get to the collision check.
         forceRepath = true;
         
         debugMove4("doWorkUpdate_4 -- goal point of our specific path is pretty far from formation position, so dropping it.");
         syncSquadCode("doWorkUpdate_4 -- goal point of our specific path is pretty far from formation position, so dropping it.");
      }
      else
      {
         // Destination is pretty close to formation position, but see if we can get directly to it.  If so, we'll drop our path.
         
         // Set up obstruction manager.
         doObMgrBegin_4(pSquad);

         // See if we intersect anything.
         BVector iPoint;
         bool intersects = gObsManager.segmentIntersects(position, mInterimTarget_4, true, iPoint);

         // Clear shot to the goal, so drop this path.
         if(!intersects)
         {
            debugMove4("doWorkUpdate_4 -- we have a clear shot to the formation position, so dropping specific path.");
            syncSquadCode("doWorkUpdate_4 -- we have a clear shot to the formation position, so dropping specific path.");
            keepSpecificPath = false;
         }
      }
      
      if(keepSpecificPath)
      {
         // We intersected something, so don't leave the current path, just continue to follow it. 
         if(mCurrentWaypoint<1 || mCurrentWaypoint>=mPath.getNumberWaypoints())
         {
            BFAIL("Waypoint is invalid");
            return(cStateFailed);
         }

         // Destination is the waypoint.
         destination = mPath.getWaypoint(mCurrentWaypoint);
         debugMove4("doWorkUpdate_4 -- destination is specific path waypoint %d.", mCurrentWaypoint);
         syncSquadData("doWorkUpdate_4 -- destination ", destination);
         syncSquadData("doWorkUpdate_4 -- destination is specific path waypoint ", mCurrentWaypoint);
      }
      else
      {
         // No intersection means we can drop this specific path and just go back to normal movement.
         destination = mInterimTarget_4;
         syncSquadData("doWorkUpdate_4 -- no intersection, dropping path, destination:", destination);
         mPath.reset();
         mCurrentWaypoint = 0;
         mFlagFollowingSpecificPath_4 = false;
         mLastPathResult_4 = BPath::cNone;

         // If we didn't collide this update, reset pathing attempts for the next time we did collide
         mSpecificPathingAttempts_4 = 0;

         // Remember we did this seg check and it was clear.
         segCheckedOkToInterimTarget = true;
         debugMove4("doWorkUpdate_4 -- we can get directly to our formation position now, so stop following specific path.");
         syncSquadCode("doWorkUpdate_4 -- we can get directly to our formation position now, so stop following specific path.");
      }

      // DLM 5/23/08 - New Logic.  Check to see if we can early terminate.  
      /*
      if (canEarlyTerminate_4())
         return cStateDone;
      */
   }
   else
   {
      // Destination is target position
      destination = mInterimTarget_4;
      syncSquadData("doWorkUpdate_4 -- not following path, destination:", destination);
   }
   
   // Now we need to figure our next position, and do collision check.  
   // Here is where we determine if we need an alternate path or not.. 

   // Compute the direction to our destination.
   BVector direction = destination - position;
   direction.y = 0;
   BVector iPoint;
   
   float distance = direction.length();
   syncSquadData("distance", distance);
   
   if(distance < cFloatCompareEpsilon)
   {
      // jce [5/5/2008] -- We're right at our destination already, so just make sure we aren't on a path and 
      // see what happens (hopefully the platoon moves or we're done).
      mFlagFollowingSpecificPath_4 = false;
      mCurrentWaypoint = 0;
      mPauseTimeRemaining_4 = cDefaultPauseTimeout;
      
      // jce [9/26/2008] -- update velocity to be zero since we're not moving at all this update but we don't
      // get down to the velocity adjustment code below.
      mVelocity = 0.0f;
      
      // DLM 5/12/08 - Use this check for platoonmoves..
      if (mFlagPlatoonMove)
      {
         // DLM 5/6/08 - If the platoon's done, we're done. Otherwise, wait and try again later.
         bool platoonPaused;
         if (pPlatoon->isPlatoonDoneMoving_4(&platoonPaused))
            return cStateDone;
         // TRB 10/2/08 - If squad reaches its position before the platoon and/or platoon mates, the platoon will mark the move
         // as done.  This releases the squad to finish its move action and to start attacking.
         else if (pPlatoon->hasValidPlots_4() && pPlatoon->isSquadDoneMoving_4(pSquad->getID()))
            return cStateDone;
         else
         {
            // If the platoon is paused and we're at our position, then we should just pause too.
            if(platoonPaused)
            {
               #ifndef BUILD_FINAL
               mPausedByID.invalidate();
               mPausedReason = cPausedReasonPlatoonIsPaused;
               #endif
               return cStateWait;
            }
            else
               return cStateWorking;
         }
      }
      else
      {
         // If we're at the destination in a non platoon move, we're done.
         debugMove4("doWorkUpdate_4 -- safeNormalize returned false in non-platoon move move - return cStateDone.");
         syncSquadCode("doWorkUpdate_4 -- safeNormalize returned false in non-platoon move move - return cStateDone.");
         return cStateDone;
      }
   }
   // jce [9/26/2008] -- taking this out for now since it was having some interaction with transporting... it's not doing
   // anything really useful yet anyway      
   // Halwes - 9/29/2008 - Added this back in after fix to transporting movement.
   // TRB 9/29/08 - Taking this back out since it was causing moves for air vehicles to end prematurely, which
   // was causing attack actions to fail.  The move action should have had an accompanying platoon move action
   // so I'm not sure why distance was too big.
/*
   else if(distance > cMaximumLowLevelDist)
   {
      // Well we're more than the max low level path distance from where we need to go, and squads
      // don't know how to long-range path (yet?) so just fail out.
      debugMove4("doWorkUpdate_4 -- got more than max LLP distance from destination, failing.");
      syncSquadCode("doWorkUpdate_4 -- got more than max LLP distance from destination, failing.");
      return(cStateFailed);
   }    
*/
   // Normalize.
   direction /= distance;

   // Update our velocity based on how far out of position we are.
   float desiredVelocity = pSquad->getDesiredVelocity();
   syncSquadData("Desired velocity", desiredVelocity);
   
   // Temporarily, disable the catch up code.
   // jce [8/27/2008] -- turned this back on ... Dusty couldn't remember why he turned it off :)
   // jce [10/2/2008] -- Turned off catch-up code once we have plotted positions since they are placed way ahead of the units
   // and can cause a freaky unwarranted speed boost as they approach the target.
   syncSquadData("platoon move", mFlagPlatoonMove);
   if(pPlatoon && mFlagPlatoonMove && !pPlatoon->hasValidPlots_4())
   {
      const BFormation2 *formation = pPlatoon->getFormation();
      syncSquadData("formation null", formation?false:true);

      if(formation)
      {
         const BFormationPosition2 *formationPosition = formation->getFormationPosition(pSquad->getID());
         syncSquadData("formation position null", formationPosition?false:true);

         if(formationPosition)
         {
            // Figure out how far out of position we are.
            float distFromPos = formationPosition->getPosition().xzDistance(position);
            syncSquadData("distFromPos", distFromPos);
            
            // Our normal velocity if we're in position.
            // float normalVelocity = getPlatoonVelocity_4();
            // DLM 7/2/8 - changed to use squad velocity.
            float normalVelocity = pSquad->getDesiredVelocity();
            syncSquadData("normalVelocity", normalVelocity);
            
            // Fastest we're allowed to cheat our velocity if we're out of position.
            float fastestVelocity = cCatchUpScalar * normalVelocity;
            syncSquadData("fastestVelocity", fastestVelocity);

            // Scale between normal and fastest based on how far out we are.
            desiredVelocity = Math::Lerp(normalVelocity, fastestVelocity, Math::fSmoothStep(0.0f, 1.0f, distFromPos/cCatchUpFastestDistance));
            syncSquadData("speedup's desiredVelocity", desiredVelocity);

         }
      }
   }

   //static bool slowdown=true;
   bool slowdown = !gConfig.isDefined(cConfigNoSquadSlowdown);
   /*
   if(slowdown)
   {
      //Figure out if we need to slow down to avoid a collision.
      float velocityModifier;
      bool inside = false;
      bool blockedOnStoppedSquads = false;
      bool finalDest = false;
      BEntityID blockingEntityID(cInvalidObjectID);
      if (mpOwner->getClassType() == BEntity::cClassTypeSquad)
      {
         SCOPEDSAMPLE(BSquadActionMove_followPath_calcVeloMod);
         calculateProjection();
         velocityModifier=calculateVelocityModifier(inside, blockedOnStoppedSquads, finalDest, blockingEntityID);
      }
      else
      {
         SCOPEDSAMPLE(BSquadActionMove_followPath_calcVeloModSimp);
         velocityModifier=calculateVelocityModifierSimple();
      }
      
      if(!blockedOnStoppedSquads && velocityModifier < 0.99f)
      {
         desiredVelocity = getPlatoonVelocity_4()*velocityModifier;
      }
   }   
   */

   // jce [5/23/2008] -- hack slowdown stuff... really needs cleanup/speedup (like ignore list getting rebuilt), etc. if we keep it.
   if(slowdown)
   {   
      // Get the target squad
      BEntityID targetSquadID = cInvalidObjectID;
      if (mTarget.isIDValid())
      {
         if (mTarget.getID().getType() == BEntity::cClassTypeSquad)
            targetSquadID = mTarget.getID();
         else if (mTarget.getID().getType() == BEntity::cClassTypeUnit)
         {
            BUnit* pTargetUnit = gWorld->getUnit(mTarget.getID());
            if (pTargetUnit)
               targetSquadID = pTargetUnit->getParentID();
         }
      }

      // Set up obstruction manager.
      doObMgrBegin_4(pSquad);

      // See if we intersect anything.
      BVector iPoint;
      static float cMaxPredictionDistance = 10.0f;
      float predictionDistance = min(cMaxPredictionDistance, distance);
      BObstructionNodePtrArray obstructions;
      bool intersects = gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, position, position+direction*predictionDistance, true, iPoint, obstructions);

      syncSquadData("intersects", intersects);
      syncSquadData("num obstructions", obstructions.getNumber());
      if(intersects)
         syncSquadData("iPoint", iPoint);

      if(intersects)
      {
         for(long i=0; i<obstructions.getNumber(); i++)
         {
            BOPObstructionNode *ob = obstructions[i];
            
            syncSquadData("ob type", ob->mType);

            if(ob->mType == BObstructionManager::cObsNodeTypeSquad)
            {
               BSquad *collidedSquad = reinterpret_cast<BSquad*>(ob->mObject);

               // Ignore the target squad
               if (collidedSquad->getID() == targetSquadID)
                  continue;
               
               // Don't slow down for stopped things since we just are going to have to
               // go around them.
               if(!collidedSquad->isMoving())
               {
                  syncSquadCode("Not moving, skipping");
                  continue;
               }

               // Look at how fast the other guy is going.
               float collidedActionVelocity = collidedSquad->getActionVelocity();
               syncSquadData("collidedActionVelocity", collidedActionVelocity);
               
               // jce [9/22/2008] -- Treat things moving extremely slowly as stopped.
               if(collidedActionVelocity < 0.1f)
                  continue;

               // Only slow down for things that are in front of us, moving in roughly our direction.                  
               int predictedCollisionResult = doForwardCompare_4(collidedSquad, true);
               if(predictedCollisionResult != cCollisionPause)
               {
                  syncSquadData("Collision result not pause, was actually", predictedCollisionResult);
                  continue;
                  
               }
               
               // Scale the slowdown based on how far away the collision is predicted to occur.
               float distToCollision = position.xzDistance(iPoint);
               float distFactor = distToCollision/cMaxPredictionDistance;
               if(distFactor>1.0f)
                  distFactor = 1.0f;
               //else if(distFactor<0.1f)
                 // distFactor = 0.1f;
               //desiredVelocity = desiredVelocity*distFactor;

               
               // Slowdown relative to that speed.
               float slowdownFactor = Math::Lerp(0.7f, 1.0f, distFactor);
               float slowdownSpeed = slowdownFactor * collidedActionVelocity;
               syncSquadData("slowdownFactor", slowdownFactor);

               // Our new velocity is a fraction of the other guy's current speed, unless our
               // speed is slower than that already, in which case we can just keep our speed.               
               desiredVelocity = min(desiredVelocity, slowdownSpeed);
               syncSquadData("slowdown's desiredVelocity", desiredVelocity);
               
               // Keep searching since the first thing we check might not be the thing that requires us to 
               // slow down the most.
            }
         }
      }
   }
   // DLM 10/15/08 - Further limit velocity by distance from physics unit..
   #ifdef _NEWWARTHOG_MOVEMENT
      if (pSquad)
      {
         // Just get the first child.  We should never have a squad with more than a single physics object child in it.  
         const BEntity* pChild = gWorld->getEntity(pSquad->getChild(0));
         if (pChild && pChild->getFlagPhysicsControl() && pChild->getPhysicsObject() && pChild->getPhysicsObject()->isActive()  && 
            (pChild->getProtoObject() && pChild->getProtoObject()->getMovementType() != cMovementTypeAir))
         {
            BPhysicsObject *pPhysicsObect = pChild->getPhysicsObject();
            BVector physicsPos;
            pPhysicsObect->getPosition(physicsPos);
            float fPhysicsDist = pSquad->getPosition().xzDistance(physicsPos);
            // If we're anywhere between radius to 2x radius distance from our physics unit, scale our velocity down.
            float fRadius = pChild->getObstructionRadius() * 2.0f;
            float fMaxDist = fRadius * 2.0f;
            if (fPhysicsDist > fMaxDist)
               fPhysicsDist = fMaxDist;
            if (fPhysicsDist > fRadius && fabs(fRadius) > cFloatCompareEpsilon)
            {
               float fSlowFactor = 1.0f - ((fPhysicsDist - fRadius) / fRadius);
               float fSlowVelocity = Math::Lerp((desiredVelocity * 0.3f), desiredVelocity, fSlowFactor);
               desiredVelocity = fSlowVelocity;
            }
         }
      }
   #endif
   // Limit it.
   static bool useLimit = true;
   if(useLimit)
   {
      //limitVelocity(elapsedTime, desiredVelocity, mVelocity, false);
      
      if(desiredVelocity < mVelocity)
         mVelocity = desiredVelocity;
      else
      {
         float acceleration=mpOwner->getAcceleration();
         mVelocity += acceleration*elapsedTime;
         if(mVelocity > desiredVelocity)
            mVelocity = desiredVelocity;
      }
   }
   else
      mVelocity = desiredVelocity;



   // Compute tentative new position
   float distCovered = mVelocity * elapsedTime;
   BVector newPosition = position + (direction * distCovered);

   // Check to see if we overshot the destination.
   BVector newDirection = destination - newPosition;
   newDirection.y = 0;
   newDirection.safeNormalize();
   if (direction.dot(newDirection) < 0)
   {
      // We'll overshoot, so just set our destination as our position
      debugMove4("doWorkUpdate_4 -- overshot destination, snapping goal to destination! dot product was: %f", direction.dot(newDirection));
      syncSquadData("doWorkUpdate_4 -- overshot destination, snapping goal to destination! dot product was: ", direction.dot(newDirection));
      newPosition = destination;

      // jce [4/23/2008] -- this used to set the done state here, but a collision might prevent you from getting
      // to this final (now clamped) destination.  Also you might just be going towards a waypoint.  So, the check
      // for doneness is performed at the end.
      float distReallyCovered = newPosition.xzDistance(position);
      mVelocity = distReallyCovered / elapsedTime;
   }


   debugMove4("doWorkUpdate_4 -- using velocity of %0.2f, from (%0.2f, %0.2f, %0.2f) towards (%0.2f, %0.2f, %0.2f)", mVelocity, position.x, position.y, position.z, 
      destination.x, destination.y, destination.z);
   syncSquadData("doWorkUpdate_4 -- using velocity of ", mVelocity);
   syncSquadData("from ", position);
   syncSquadData("towards ", destination);

   // Check for collisions.
   int collisionResult;
   if(forceRepath)
   {
      // Just pretend we got a collision at our current point so that we are forced to repath.
      collisionResult = cCollisionRepath;
      iPoint = position;
   }
   else
   {
      // If we already did the conservative interim target check and that passed, no need to do the checkCollisions check.
      // If this is a squad for a flying unit, the just ignore collision checking altogether. 
   //   if(segCheckedOkToInterimTarget || pSquad->isFlyingSquad_4())
   //      collisionResult = cCollisionClear;
   //   else
         collisionResult = checkCollisions_4(position, newPosition, iPoint);
   }
      
   if ((collisionResult == cCollisionRepath) || (collisionResult == cCollisionPauseOtherAndRepath))
   {
      // If we need to repath, then do so now.  
      // The following look ahead code is only in the case of platoon moves, which are trying to get to their formation position...
      BVector predictedPosition;
      bool foundSpot = false;
      if (mFlagPlatoonMove)
      {
         // If we are on a specific path and are asked to repath, instead go back to normal movement and see if that starts working.
         // If not, we'll be right back here next update but not on a specific path and will do our repath.
         if(mFlagFollowingSpecificPath_4)
         {
            debugMove4("doWorkUpdate_4 -- collided on specific path, bailing to see what happens next update");
            syncSquadCode("doWorkUpdate_4 -- collided on specific path, bailing to see what happens next update");
            
            destination = mInterimTarget_4;
            mPath.reset();
            mCurrentWaypoint = 0;
            mFlagFollowingSpecificPath_4 = false;
            mLastPathResult_4 = BPath::cNone;
            return(cStateWorking);
         }

         // Snap to collision point.
         newPosition = iPoint;
         debugMove4("doWorkUpdate_4 -- snapping to collision point!");
         syncSquadCode("doWorkUpdate_4 -- snapping to collision point!");

      
         // DLM 6/9/08 - I think I only want to do the predictive look ahead logic if the platoon
         // is still moving.  Otherwise, just get as close as we can to InterimTarget. 
         // DLM 7/31/08 - AND, if the platoon doesn't have valid plot positions.
         // DLM 10/31/08 - So now that we keep moving even if we have valid plots, remove that from this condition.
         //if(!pPlatoon->isPlatoonDoneMoving_4() && !pPlatoon->hasValidPlots_4())
         if(!pPlatoon->isPlatoonDoneMoving_4() && !pPlatoon->isSquadProceedingToPlot_4(pSquad->getID()))
         {
            // Get predicted path our formation position is going to travel through.
            BPath predictedPath;
            long waypoint = getPathAndWaypointForPredictions(predictedPath);

            // jce [4/25/2008] -- Blah, this can be repeated work from above in certain cases... figure out how to clean up

            // Set up obstruction manager.
            doObMgrBegin_4(pSquad);

            // Predict forward a certain amount along our path, trying to find an unobstructed future position.
            float distanceLeft = predictedPath.calculateRemainingDistance(newPosition, waypoint, true);

            // Cap the distance we'll go to some arbitrary amount so we don't go too crazy here.
            distanceLeft = min(40.0f, distanceLeft);

            // March along the path at some arbitrary interval looking for a clear future spot.
            BObstructionNodePtrArray obstructions;
            float realDistance;
            long newWaypoint;
            foundSpot = false;
            for(float t=0.0f; t<distanceLeft; t += 4.0f)
            {
               // Get the new predicted point.
               predictedPath.calculatePointAlongPath(t, predictedPath.getWaypoint(0), waypoint, predictedPosition, realDistance, newWaypoint);
               
               // Save old quadtrees.
               long oldQuadtrees = gObsManager.getTreesToScan();
               long quadtrees = oldQuadtrees;

               // jce [9/15/2008] -- Pulling out moving things like the pather does.
               quadtrees &= ~BObstructionManager::cIsNewTypeCollidableMovingUnit;
               quadtrees &= ~BObstructionManager::cIsNewTypeCollidableMovingSquad;
            
               // jce [9/15/2008] -- also, no need for units that can move even if they are stopped since we really care about their squad obstruction
               quadtrees &= ~BObstructionManager::cIsNewTypeCollidableStationaryUnit;
               
               // Set updated quadtree flags.
               gObsManager.setTreesToScan(quadtrees);

               // See if it's clear.
               gObsManager.findObstructions(predictedPosition, false,  false, obstructions);

               // Restore old settings.
               gObsManager.setTreesToScan(oldQuadtrees);

               if(obstructions.getNumber() == 0)
               {
                  debugMove4("doWorkUpdate_4 -- found unobstructed future position (%0.2f, %0.2f, %0.2f)", predictedPosition.x, predictedPosition.y, predictedPosition.z);
                  syncSquadData("doWorkUpdate_4 -- found unobstructed future position ", predictedPosition);
                  foundSpot = true;
                  break;
               }
            }

            // If we found a good spot to path to, we're going to use that.  Otherwise, we want to do some 
            // go to the platoon position, which is basically following the spline. 
            if(!foundSpot)
            {
               debugMove4("doWorkUpdate_4 -- could not find unobstructed future position, pathing to platoon position for now");
               syncSquadCode("doWorkUpdate_4 -- could not find unobstructed future position, pathing to platoon position for now");
               
               // jce [5/6/2008] -- ported this improvement from age3 code that did basically the same thing.
               /*
               // Don't path to the unit group's position.  Rather, path to the point on the on the segment
               // defined by the group's position and orientation that's closest to our current position.
               predictedPosition = pPlatoon->getPosition();
               predictedPosition.y = 0.0f;
               BVector vFPPosition = mInterimTarget_4;
               vFPPosition.y = 0.0f;
               BVector vTemp = vFPPosition - predictedPosition;
               float fLen = vTemp.length();
               vTemp.normalize();
               BVector projectDirection = pSquad->getForward();
               float fDot = (projectDirection.x * vTemp.x) + (projectDirection.z * vTemp.z);
               if (_fabs(fDot) > cFloatCompareEpsilon)
                  predictedPosition = predictedPosition + (projectDirection * (fDot * fLen));
               */
               
               // jce [9/4/2008] -- new attempt: find closest point on current LL path.
               
               /*
               const BPath &path = pPlatoon->getCurrentLLPath_4();
               if(path.getNumberWaypoints() < 2)
               {
                  // Not sure why the platoon doesn't have a valid path, but use it's position in this
                  // case.
                  predictedPosition = pPlatoon->getPosition();
               }
               else
               {
                  // Get the closest point on the platoon's current path.
                  predictedPosition = path.findClosestPoint(pSquad->getPosition());
               }
               */
               
               // jce [9/25/2008] -- hmm, let's try current formation position?
               predictedPosition = mInterimTarget_4;
               
               /*
               gObsManager.findObstructions(predictedPosition, false,  false, obstructions);
               if(obstructions.getNumber() == 0)
               {
                  debugMove4("doWorkUpdate_4 -- found unobstructed prediction along path (%0.2f, %0.2f, %0.2f)", predictedPosition.x, predictedPosition.y, predictedPosition.z);
                  syncSquadData("doWorkUpdate_4 -- found unobstructed future position on path", predictedPosition);
                  foundSpot = true;
               }
               */
               
               // jce [9/25/2008] -- checking for fully clear spot is too restrictive.  It's helpful in some cases but maybe it needs some
               // more to the heuristic.
               foundSpot = true;
            }
         }
         else
         {
            debugMove4("doWorkUpdate_4 -- platoon is stopped, so we're not boing to look for a predictive position -- just try to get to formation location.");
            syncSquadCode("doWorkUpdate_4 -- platoon is stopped, so we're not boing to look for a predictive position -- just try to get to formation location.");
            predictedPosition = mInterimTarget_4;
            
            // jce [9/12/2008] -- Spot isn't clear, but formation has stopped so just path as close as possible at this point.
            foundSpot = true;
         }

      } // end of if mFlagPlatoonMove
      else
      {
         // Otherwise, just path to target.
         debugMove4("doWorkUpdate_4 -- collision response said to repath, but we're not on a platoonMove, so just repathing to destination.");
         syncSquadCode("doWorkUpdate_4 -- collision response said to repath, but we're not on a platoonMove, so just repathing to destination.");
         newPosition = iPoint;
         predictedPosition = mInterimTarget_4;
         foundSpot = true;
      }
      
      // If no clear spot found, pause.
      if(!foundSpot)
      {
         debugMove4("doWorkUpdate_4 -- no clear spot found to path to, so pausing.");

         #ifndef BUILD_FINAL
         mPausedByID.invalidate();
         mPausedReason = cPausedReasonNoClearSpot;
         #endif
         
         pauseMovement_4(cDefaultPauseTimeout);
         
         return(cStateWait);
      }

      // Get a path to our interim destination.
      long pathResult = BPath::cFailed;
      int findResult = findLowLevelPath_4(newPosition, predictedPosition, pathResult);
      if(findResult == cFindPathFailedPathLimiter)
      {
         // We had no pathing attempts left, so just retry on the next update.
         debugMove4("doWorkUpdate_4 -- we weren't allowed to path due to pathing limits, so we'll try again later");
         syncSquadCode("doWorkUpdate_4 -- we weren't allowed to path due to pathing limits, so we'll try again later");
         return(cStateWorking);
      }
      else if(findResult == cFindPathFailedTooManyRetries)
      {
         // We've exceeded our allowed retries so now it's time to give up instead of sucking down CPU not getting anywhere.
         debugMove4("doWorkUpdate_4 -- we exceeded our pathing retries, so time to fail");
         syncSquadCode("doWorkUpdate_4 -- we exceeded our pathing retries, so time to fail");
         
         // Make a note that we've failed.
         mFlagFailed = true;
         
         // Wait on unit opps to complete.  When that is done we'll go into state failed and be killed off.
         return(cStateWaitOnChildren);
      }
      else
      {
         debugMove4("doWorkUpdate_4 -- pathed and got result of '%s'", BPath::getPathResultName(pathResult));
         syncSquadData("doWorkUpdate_4 -- pathed and got result of ", BPath::getPathResultName(pathResult));

         // See what our path got us.
         if(pathResult == BPath::cFull || pathResult == BPath::cPartial || pathResult == BPath::cOutsideHulledAreaFailed)
         {
            // jce [9/26/2008] -- Trying out a metric to see if our path is sucky.  Basically, check to see if the distance we
            // have to travel is really long compared to how much closer it gets us to the goal.
            
            if(pPlatoon && !pPlatoon->isPlatoonDoneMoving_4() && !pPlatoon->hasValidPlots_4())
            {
               // Get the final waypoint's distance to the goal.
               float endOfPathDistance = mPath.getWaypoint(mPath.getNumberWaypoints()-1).xzDistance(predictedPosition);
               
               // Now get the current distance.
               float currentDistance = newPosition.xzDistance(predictedPosition);
               
               // How much better is that than where we are now?
               float distanceImprovement = currentDistance-endOfPathDistance;
               
               // Get the length of the path.
               float pathLength = mPath.calculatePathLength(true);
               
               // Check how much work we're doing compared to the improvment
               static float cLengthRatioCutoff = 1.0f/3.0f;          // 1/x means path can't be more than x times longer than the distance improvement
               if(distanceImprovement < cLengthRatioCutoff*pathLength)
               {
                  // Path sucks, so just pause.
                  debugMove4("The path we just got sucks, so pausing for now.");
                  
                  mFlagFollowingSpecificPath_4 = false;
                  mCurrentWaypoint = 0;
                  mPauseTimeRemaining_4 = cDefaultPauseTimeout;
              
                  #ifndef BUILD_FINAL
                  mPausedByID.invalidate();
                  mPausedReason = cPausedReasonPathSucks;
                  #endif
               
                  return cStateWait;
               }
            }
            
         
            // Remember that we're now following our own specific path.
            mFlagFollowingSpecificPath_4 = true;
            
            // Remember where formation position was when we did this
            mFormationPositionWhenSpecificPathWasCreated_4 = mInterimTarget_4;

            // We're moving towards the first waypoint
            mCurrentWaypoint = 1;
            
            // We got a good path, so reset our pathing attempts
            // jce [5/5/2008] -- Only reset our count on full paths to a clear spot, so we don't endlessly ping-pong 
            // when we're not really getting to our goal.
            // DLM - Okay we shouldn't really be pingponging with the changes in the logic above.  So let retry attempts reset as
            // long as we didnt' get fail. 
            //if(pathResult == BPath::cFull && foundSpot)
            mSpecificPathingAttempts_4 = 0;

            #ifdef PRECALC_TURNRADIUS
               // DLM - I'm not sure where *else* I'm going to update the turn radius path, but I know when I get a valid
               // localized path, I'm definitely updating it here.
               // DLM 5/12/08 - Legacy from the old code, updateTurnRadiusPath wants 'mStartingDir' to be set to something reasonable.
               mStartingDir = mPath.getWaypoint(0) - pSquad->getPosition();
               mStartingDir.y = 0;
               mStartingDir.safeNormalize();
               updateTurnRadiusPath(0);            
            #endif

            debugMove4("doWorkUpdate_4 -- following specific path");
            syncSquadCode("doWorkUpdate_4 -- following specific path");
         }
         else if(pathResult == BPath::cInRangeAtStart)
         {
            debugMove4("doWorkUpdate_4 -- path came back as InRangeAtStart, trying again later");
            syncSquadCode("doWorkUpdate_4 -- path came back as InRangeAtStart, trying again later");

            // This really shouldn't happen since we just ran the path and it's not going to help us
            // get anywhere...
            // jce [5/5/2008] -- It IS happening though, so drop off this useless path for now and hope things
            // move next update.
            mFlagFollowingSpecificPath_4 = false;
            mCurrentWaypoint = 0;
            // DLM 7/7/08 - If the platoon is actually done, then just call us done too.
            // jce [10/7/2008] -- also, being in-range means we're done if we have a non-platoon move or a plotted position.
            if (!pPlatoon || pPlatoon->isPlatoonDoneMoving_4() || pPlatoon->hasValidPlots_4())
               return cStateDone;
            // Otherwise, we'll try again a few updates from now.
            mPauseTimeRemaining_4 = cDefaultPauseTimeout;
            
            #ifndef BUILD_FINAL
            mPausedByID.invalidate();
            mPausedReason = cPausedReasonPathInRangeAtStart;
            #endif
            
            return cStateWait;
         }
         else
         {
            // Path failed... we're left with waiting around to try again later.
            // If we were following a specific path, and we had to repath, and the repath failed,
            // we should just pause.. 
            if (mFlagFollowingSpecificPath_4)
            {
               mFlagFollowingSpecificPath_4 = false;
               mCurrentWaypoint = 0;

               debugMove4("doWorkUpdate_4 -- our path failed while following a specific path, so pausing.");
               syncSquadCode("doWorkUpdate_4 -- our path failed while following a specific path, so pausing.");
               
               // Pause for a reasonably long time here since we completely failed our path.
               mPauseTimeRemaining_4 = cLongPauseTimeout;
               return cStateWait;
            }
            
            // Otherwise, proceed to try again.
            debugMove4("doWorkUpdate_4 -- our path failed, so we'll try again later");
            syncSquadCode("doWorkUpdate_4 -- our path failed, so we'll try again later");

            // Pause for a reasonably long time here since we completely failed our path.
            mPauseTimeRemaining_4 = cLongPauseTimeout;

            #ifndef BUILD_FINAL
            mPausedByID.invalidate();
            mPausedReason = cPausedReasonPathFailed;
            #endif

            return(cStateWait);
         }
      } // end of path OK
   } // end of if collisionResponse was 'repath'. 
   else if (collisionResult == cCollisionPause)
   {
      // Pause ourselves.
      pauseMovement_4(cDefaultPauseTimeout);
      
      // Don't advance this update.. immediately return and try again next update.
      return cStateWait;
   }

   // Okay, if we're not still working, we can exit now..
   if (returnState != cStateWorking)
      return returnState;

   // Adjust forward and right appropriately as well..
   direction = newPosition - pSquad->getPosition();
   direction.y = 0;
   if(direction.safeNormalize())
   {
      pSquad->setForward(direction);
      pSquad->setUp(cYAxisVector);
      pSquad->calcRight();
   }
   // Update position   
   syncSquadCode("BSquadActionMove::doWorkUpdate_4 -- setPosition");
   pSquad->setPosition(newPosition);

   // Update the squad's leash too.. 
   bool userCommandAndVisibleTarget = false;
   if(mpOrder && mpOrder->getPriority() == BSimOrder::cPriorityUser)   
   {      
      //-- Can we see it our target?
      BEntity* pTarget = gWorld->getEntity(mTarget.getID());
      if(pTarget)
         userCommandAndVisibleTarget = pTarget->isVisible(pSquad->getTeamID());      
   }

   if (mFlagPlatoonMove || mFlagForceLeashUpdate || (userCommandAndVisibleTarget))
      pSquad->setLeashPosition(newPosition);

   // Check the new position
   if(mFlagFollowingSpecificPath_4)
   {
      // If we're really close to our next waypoint, we're done.
      // DLM - don't use calculateXZDistance -- it uses the squad's radius, which gives us false positives
      // for complete, resulting in an endless cycle of work/complete/work/complete ad naseum. 
      //float distToWaypoint = pSquad->calculateXZDistance(mPath.getWaypoint(mCurrentWaypoint));
      BVector direction = mPath.getWaypoint(mCurrentWaypoint) - pSquad->getPosition();
      float distToWaypoint = direction.length();
      if(distToWaypoint < cFloatCompareEpsilon)
      {
         // Move on to the next waypoint.
         mCurrentWaypoint++;
         
         // If that's the end of our path, go back to no-specific-path mode.
         if(mCurrentWaypoint >= mPath.getNumberWaypoints())
         {
            debugMove4("doWorkUpdate_4 -- reached the end of our specific path, so returning to normal movement.");
            syncSquadCode("doWorkUpdate_4 -- reached the end of our specific path, so returning to normal movement.");
            mFlagFollowingSpecificPath_4 = false;
            mPath.reset();
            mCurrentWaypoint = 0;
            if (!mFlagPlatoonMove)
               returnState = cStateDone;
         }
      }
   }

   // For hit and run the move needs to stop on top of the target so skip the early out below based on distance to target.
   if (pSquad->getSquadMode() == BSquadAI::cModeHitAndRun)
   {
      debugMove4("doWorkUpdate_4 -- Hit and run skipping in range of target check.");
      syncSquadCode("doWorkUpdate_4 -- Hit and run skipping in range of target check.");
      return returnState;
   }

   // Ask the platoon if we're close enough to our target location.  
   // I don't really want to be done unless the platoon is stopped *and* I'm close enough to my formation position.
   bool checkPlatoon = mFlagPlatoonMove;

   // TRB 9/23/08 - If a squad couldn't get a plot (so it defaulted to the target's position) then allow it to
   // stop once it's in range so it doesn't move all the way to the edge of the target's obstruction.
   if (mFlagPlatoonMove && pPlatoon->hasValidPlots_4() && pPlatoon->hasDefaultSquadPlot_4(pSquad->getID()))
      checkPlatoon = false;

   if (checkPlatoon)
   {
      // DLM 10/23/08 - Fixes a problem where units walk in place if they reach their destination point,
      // but the platoon is paused, waiting on another squad to finish attacking.
      bool bDone = false;
      bool bPaused = false;
      bDone = pPlatoon->isPlatoonDoneMoving_4(&bPaused);
      if (pPlatoon->isSquadAtTarget_4(pSquad->getID()) && (bDone || bPaused))
      {
         debugMove4("doWorkUpdate_4 -- reached target, so we're done.");
         syncSquadCode("doWorkUpdate_4 -- reached target, so we're done.");
         returnState = cStateDone;
      }        
   }
   else
   {
      // If we're not on a platoon move, then just calculate dist to target..       
      float distToTarget;
      if (mTarget.isIDValid() || mTarget.isPositionValid())
      {
         distToTarget = pSquad->calculateXZDistance(mTarget);
      }
      else
      {
         debugMove4("doWorkUpdate4 -- Non Platoon move has no valid target or position.  EPIC FAIL.");
         syncSquadCode("doWorkUpdate4 -- Non Platoon move has no valid target or position.  EPIC FAIL.");
         return cStateFailed;
      }
      float fRange = cActionCompleteEpsilon;
      if (mTarget.isRangeValid())
         fRange = mTarget.getRange();

      // TRB 9/2/08 - Following moving target change.
      // If the target is moving, move further inside of range before ending the move so the attack action
      // has some extra frames to fire up.
      float rangeMultiplier = 1.0f;
      BEntity* pTarget = gWorld->getEntity(mTarget.getID());
      if (pTarget && pTarget->isMoving())
         rangeMultiplier = gDatabase.getMovingTargetRangeMultiplier();

      if (distToTarget < (fRange * rangeMultiplier))
      {
         debugMove4("doWorkUpdate_4 -- on non-platoon move, we got within range of destination.  We are done.");
         syncSquadCode("doWorkUpdate_4 -- on non-platoon move, we got within range of destination.  We are done.");

         // TRB 9/2/08 - Following moving target change.
         // Go to the wait on children state so the squad doesn't get too far ahead of its units.
         // Without this, it was possible for the squad to enter the move state, move, and re-enter the attack state
         // all in one frame while the unit would not have time to process the move so it wouldn't move.
         returnState = cStateWaitOnChildren;
      }
   }

   return returnState;
}


//==============================================================================
//==============================================================================
void BSquadActionMove::doObMgrBegin_4(BSquad *pSquad)
{
   // We're relying on doWorkUpdate and the things it calls to not use this improperly.  The assumption is that if the
   // obmgr is already in use, it's from some other portion of the update doing the same exact begin call.
   // As such, if obmgr is already in use, don't do any additional work.
   if(gObsManager.inUse())
      return;
   
   // First, figure out what type of obstructions we care about.
   long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads;
   if (isPathableAsFlyingUnit())
      quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;
   else if (isPathableAsFloodUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
   else if (isPathableAsScarabUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
   else
      quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits;

   // Set up ignore list.
   static BEntityIDArray ignoreList;
   ignoreList.resize(0);
   buildIgnoreList_4(ignoreList);

   for(long i=0; i<ignoreList.getNumber(); i++)
      syncSquadData("ignore list", ignoreList[i]);

   // Set up obstruction manager.
   gObsManager.begin(BObstructionManager::cBeginNone, getPathingRadius(), quadtrees, BObstructionManager::cObsNodeTypeAll, pSquad->getPlayerID(), cDefaultRadiusSofteningFactor, &ignoreList, pSquad->canJump());
}


//==============================================================================
// isBehindPlatoon_4
// We use an incredibly scientific and exact method of determining if we're
// lagging behind the platoon.  We get our platoon's formation position, and
// if the straighline distance between us and that location is more than half
// the short-range pathing limit, then we're "behind".  
//==============================================================================
bool BSquadActionMove::isBehindPlatoon_4()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   if (!pSquad)
   {
      debugMove4("isBehindPlatoon -- no squad, failing.");
      return false;
   }

   if (!mFlagPlatoonMove)
      return false;

   // Get platoon.
   BPlatoon *pPlatoon = pSquad->getParentPlatoon();
   if (!pPlatoon)
   {
      debugMove4("isBehindPlatoon -- no platoon, failing.");
      return false;
   }
   const BFormation2 *formation = pPlatoon->getFormation();
   if (!formation)
      return false;
   const BFormationPosition2 *pos = formation->getFormationPosition(pSquad->getID());
   if (!pos)
      return false;
   BVector desiredPosition = pos->getPosition();
   float fDistance = (desiredPosition - pSquad->getPosition()).length();
   if (fDistance > (cMaximumLowLevelDist * 0.5f))
      return true;

   return false;
}


//==============================================================================
//==============================================================================
float BSquadActionMove::getPlatoonVelocity_4()
{
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BDEBUG_ASSERT(pSquad);

   BPlatoon *pPlatoon = pSquad->getParentPlatoon();
   if (!pPlatoon)
      return pSquad->getDesiredVelocity();

   // If we're not in a platoon move, OR, if we're not using classic platoon grouping,
   // then "platoon" velocity is really just this squad's velocity.
   if (!mFlagPlatoonMove)
      return pSquad->getDesiredVelocity();

   return pPlatoon->getPlatoonVelocity_4();
}

//==============================================================================
//==============================================================================
int BSquadActionMove::checkCollisions_4(BVector start, BVector end, BVector &iPoint)
{  

   #ifndef BUILD_FINAL
   // jce [5/9/2008] -- Save off collision segment for debug rendering.
   mLastCollisionCheck[0] = start;
   mLastCollisionCheck[1] = end;
   #endif


   // Get squad.
   // jce [4/23/2008] -- can this ever really fail?
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("checkCollisions_4 -- no squad, so ... um... I guess you didn't collide with anything.");
      return(cCollisionClear);
   }

   BPlatoon *pPlatoon = pSquad->getParentPlatoon();
   if (!pPlatoon && mFlagPlatoonMove)
   {
      debugMove4("checkCollisions_4 -- no platoon.  Wierd.");
      return(cCollisionClear);
   }
   // Return value
   int collisionCheck = cCollisionClear;

   debugMove4("In checkCollisions_4 -- mFlagFollowingSpecificPath_4: %d", mFlagFollowingSpecificPath_4);

   // Set up obstruction manager.
   doObMgrBegin_4(pSquad);

   // See if we intersect anything.
   iPoint = end;     // jce [4/17/2008] -- setting this to end is probably being overly cautious since it should always be initialized if there is an intersection
   BObstructionNodePtrArray obstructions;
   bool intersects = gObsManager.getObjectsIntersections(BObstructionManager::cGetAllIntersections, start, end, true, iPoint, obstructions);
   
   // This will hold a list of squads we want to pause.  We're not going to pause them though if we figure out that WE need
   // to be paused, so we just collect the list and see what happens.
   BDynamicSimArray<BSquad*> squadsToPause;

   // If we did intersect, we need to check whether it's a collision we care about.
   long collisionResult = cCollisionClear;
   if(intersects)
   {
      debugMove4("   Found %d intersections --", obstructions.getNumber());

      // Run through the obstructions
      for(long i=0; i<obstructions.getNumber(); i++)
      {
         // Get obstruction node.
         BOPObstructionNode *ob = obstructions[i];
         if(!ob)
            continue;

         // jce [4/18/2008] -- as far as I know in HW, only units/squads can move after the game begins (so things like
         // terrain are never valid collisions here).
         if(ob->mType == BObstructionManager::cObsNodeTypeUnit || ob->mType == BObstructionManager::cObsNodeTypeSquad)
         {
            // jce [4/25/2008] -- If the collision is a unit, but the unit has a squad, ignore the unit collision.  This
            // might not really be the right solution, but if unit's aren't going to have a third layer of fancy moving
            // and pausing logic, then it probably is the right thing.
            // DLM - there really aren't good solutions to this until we do something real with units.
            if(ob->mType == BObstructionManager::cObsNodeTypeUnit && ob->mObject->getParent())
            {
               // Get parent.
               BEntity *parent = ob->mObject->getParent();
               
               // If it's a squad parent and the squad actually has an obstruction, we ignore this collision (see caveats above).
               // jce [5/5/2008] -- We check the existence of the squad obstruction because according to MBean, single units in squads
               // don't actually have squad obstructions.
               if(parent->getClassType() == BEntity::cClassTypeSquad && parent->getObstructionNode() != NULL)
               {
                  debugMove4("   this was a unit collision and the unit has a squad, so skipping it.");
                  continue;
               }
            }

            BSquad *pCollidedSquad;
            if (ob->mType == BObstructionManager::cObsNodeTypeUnit)
            {
               // Get the squad, if available.
               pCollidedSquad = reinterpret_cast<BUnit*>(ob->mObject)->getParentSquad();

               if(!pCollidedSquad)
               {
                  // DLM If we collide with a unit that doesn't have a squad, but we're already following a specific path,
                  // then ignore the unit.  We assume it's been handled by the pathing.  (We might already be inside the unit,
                  // trying to path out.  If we're not following a specific path, then try to path around it.
                  // jce/dlm [8/29/2008] -- changed this check to consider if the move time is newer than the path time, in which
                  // case we should repath even when on a specific path
                  DWORD lastMoveTime = ob->mObject->getLastMoveTime();
                  if (!mFlagFollowingSpecificPath_4 || (mPath.getCreationTime() <= lastMoveTime))
                  {
                     // If this is a unit, and doesn't have a squad, then we're just going to assume it's not a moving thing,
                     // and so we'll repath around it. 
                     debugMove4("      we've collided with a unit that doesn't have a squad.  We're going to just move around it.");
                     collisionResult = cCollisionRepath;
                     continue;
                  }
                  else
                  {
                     debugMove4("      we've collided with a unit that doesn't have a squad, but we're already on a path, so ignoring this collision.");
                     continue;
                  }
               }
            }
            else  
            {
               // Otherwise must be a squad type from outer if-check.
               pCollidedSquad = (BSquad *)gWorld->getEntity(ob->mEntityID);
            }
            
            // Sanity check that we got a valid squad.
            if (!pCollidedSquad)
            {
               debugMove4("checkCollisions_4 - Unable to retrieve squad for entity: %ld", ob->mEntityID);
               continue;
            }
            // DLM 6/5/08 - Ignore flying units
            if (pCollidedSquad->isFlyingSquad_4())
            {
               debugMove4("checkCollisions_4 - collided with a flying unit.  Ignoring that collisions.");
               continue;
            }
            
            // Debug out squad if the actual collision is with a unit.
            if(ob->mObject != pCollidedSquad)
            {
               debugMove4("      (his squad=%d)", pCollidedSquad->getID());
            }

            // Look at the last time the entity we collided with has moved.
            DWORD lastMoveTime = pCollidedSquad->getLastMoveTime();

            // If we moved since the path was made, then this is a valid collision.
            // (Though if we have no specific path, then any collision is valid since we aren't following a real path).
            if(!mFlagFollowingSpecificPath_4 || (mPath.getCreationTime() <= lastMoveTime))
            {
               debugMove4("   we collided with %d", ob->mEntityID);

               
               // See if the other guy is already stopped.
               // jce [5/7/2008] -- moved this to use the collided squad because the individual unit could be
               // moving even if the squad is paused.
               bool otherGuyStopped = !pCollidedSquad->isMoving();
               if(otherGuyStopped)
               {
                  // jce [5/14/2008] -- If the other guy is paused, but he isn't paused for long, and normally
                  // we would have paused ourselves (i.e. he's in front of us), then go ahead and ignore the fact
                  // that he's stopped and pause ourselves instead.
                  // jce [5/14/2008] -- The way this is structured is goofy, confusing, and duplicates work.  So we should
                  // fix this up if we decide to keep it.
                  bool ignoreStop = false;
                  
                  // jce [9/23/2008] -- trying this off for now... not sure it doesn't cause as many problems as it solves
                  /*
                  DWORD remainingTime;
                  // Is other guy just paused (i.e. not fully stopped?)
                  if(pCollidedSquad->isMovementPaused_4(&remainingTime))
                  {
                     // Is he coming off pause fairly soon?
                     if(remainingTime<cDefaultPauseTimeout)
                     {
                        // Would we normally have paused instead of pausing him?
                        collisionCheck = doForwardCompare_4(pCollidedSquad);
                        if(collisionCheck == cCollisionPause)
                        {
                           // Ok, just ignore the fact that he's paused and wait for him to come off pause
                           // and (hopefully) get out of our way.
                           ignoreStop = true;
                           debugMove4("      he is stopped, but we're going to pause anyway since he's higher priority and not paused for long");
                        }
                     }
                  }
                  */
               
                  if(!ignoreStop)
                  {
                     // If he is stopped, we need to path around him but no need to pause since he already is stopped.
                     debugMove4("      he is stopped, so we can re-path and keep moving");
                     
                     // jce [5/6/2008] -- re-pause him too in case he's paused and the pause timer is almost up
                     squadsToPause.add(pCollidedSquad);
                     collisionResult = cCollisionRepath;
                     continue;
                  }
               }

               // Are we in the same platoon?
               if (pPlatoon && pCollidedSquad->getParentPlatoon() && pPlatoon->getID() == pCollidedSquad->getParentPlatoon()->getID())
               {
                  collisionCheck = doPlatoonCompare_4(pCollidedSquad);
               }
               else
               {
                  // Are we allies?
                  BPlayer *player=gWorld->getPlayer(pSquad->getPlayerID());
                  if (!player)
                     continue;
                  bool allies = player->isAlly(pCollidedSquad->getPlayerID());
                  if (allies)
                  {
                     collisionCheck = doAllyCompare_4(pCollidedSquad);
                  }
                  else
                  {
                     // Everything else.. 
                     collisionCheck = doGenericCompare_4(pCollidedSquad);
                  }
               }
               if (collisionCheck == cCollisionPauseOtherAndRepath)            
               {
                  // Just go ahead and pause him.
                  squadsToPause.add(pCollidedSquad);
                  
                  // Save result, but keep searching.
                  collisionResult = cCollisionRepath;
                  continue;
               }
               else if(collisionCheck == cCollisionPause)
               {
                  return(cCollisionPause);
               }
               else if(collisionCheck == cCollisionRepath)
               {
                  // Save result, but keep searching.
                  collisionResult = cCollisionRepath;
                  continue;
               }
               else
                  continue;
            }
            else
            {
               // Since this hasn't moved since we pathed, we trust that our path was going around this properly and we have merely "grazed" it.
               debugMove4("   we collided with %d but it hasn't moved since our path was created, so skipping", ob->mEntityID);
               
               // jce [5/9/2008] -- schedule a pause for him since we don't want him to suddenly wake up at this point
               squadsToPause.add(pCollidedSquad);
               
               continue;
            }
         }
         else
         {
            if(mFlagFollowingSpecificPath_4)
            {
               // If we're on a specific path, then we can ignore this since it's not an obstruction type that can move
               // and we trust that our path was going around it properly and we have merely "grazed" it.
               debugMove4("   ignoring obstruction for entity %d", ob->mEntityID);
               continue;
            }
            else
            {
               // We hit something non-movable, so no need to pause, but we do need to re-path.
               debugMove4("   hit non-movable entity %d, need to re-path", ob->mEntityID);
               //return(cCollisionRepath);
               collisionResult = cCollisionRepath;
            }
         }
      }
   }
   else
   {
      // No intersections, so that was easy.
      debugMove4("   No intersections");
      return(cCollisionClear);
   }

   // If we got here, but marked that we need to repath, we need pause all the guys we saved off
   // to pause.
   if(collisionResult == cCollisionRepath)
   {
      for(long i=0; i<squadsToPause.getNumber(); i++)
      {
         squadsToPause[i]->pauseMovement_4(cDefaultPauseTimeout);
         debugMove4("Pausing %d", squadsToPause[i]->getID());
      }
   }
   return(collisionResult);
}



//==============================================================================
//==============================================================================
int BSquadActionMove::doPlatoonCompare_4(BSquad *pCollidedSquad)
{
   // Get our protobject first.. 
   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("doPlatoonCompare -- no squad, so ... um... I guess you didn't collide with anything.");
      return(cCollisionClear);
   }


   // jce [9/30/2008] -- Config-ifying the preemption by speed
   float preemptSpeedFactor = 0.0f;
   bool preemptBySpeed = gConfig.get(cConfigPreemptSpeedFactor, &preemptSpeedFactor);
   
   // If the factor is less than one it just means turn this off.
   if(preemptBySpeed && preemptSpeedFactor < 1.0f)
      preemptBySpeed = false;
   
   if(preemptBySpeed)
   {
      // Give priority to the faster unit.
      if (pSquad->getDesiredVelocity() > preemptSpeedFactor*pCollidedSquad->getDesiredVelocity())
      {
         #ifndef BUILD_FINAL
         BSquadActionMove *moveAction = pCollidedSquad->getMoveAction_4();
         if(moveAction)
         {
            moveAction->mPausedByID = pSquad->getID();
            moveAction->mPausedReason = cPausedReasonOtherIsFaster;
         }
         #endif
         
         debugMove4("doPlatoonCompare -- I'm faster than squad %d, so pausing him and repathing myself..", pCollidedSquad->getID().asLong());
         return(cCollisionPauseOtherAndRepath);
      }
      if (pCollidedSquad->getDesiredVelocity() > preemptSpeedFactor*pSquad->getDesiredVelocity())
      {
         #ifndef BUILD_FINAL
         mPausedByID = pCollidedSquad->getID();
         mPausedReason = cPausedReasonOtherIsFaster;
         #endif

         debugMove4("doPlatoonCompare -- I'm slower than squad %d, so I'll pause myself..", pCollidedSquad->getID().asLong());
         return(cCollisionPause);
      }
   }
   
   // jce [5/14/2008] -- For now, we're skipping the funky infantry stuff.  If we keep this, clean up all these layers of calling.
   return doForwardCompare_4(pCollidedSquad);

   /*
   const BProtoObject *pSquadProtoObject = pSquad->getProtoObject();   
   bool isInfantry = (pSquadProtoObject)?pSquadProtoObject->isType(gDatabase.getOTIDInfantry()):false;
   // Get the type of the collided squad.. 
   const BProtoObject *pCollidedProtoObject= pCollidedSquad->getProtoObject();
   bool isCollidedInfantry = (pCollidedProtoObject)?pCollidedProtoObject->isType(gDatabase.getOTIDInfantry()):false;            
   if (isInfantry)
   {
      if (isCollidedInfantry)
         return doForwardCompare_4(pCollidedSquad);
      else
         return cCollisionPause;
   }
   else
   {
      if (isCollidedInfantry)
      {
         return cCollisionPauseOtherAndRepath;
      }
      else
      {
         // Here we should do the in "who's in front" check.. 
         return doForwardCompare_4(pCollidedSquad);
      }
   }
   */
}

//==============================================================================
//==============================================================================
int BSquadActionMove::doAllyCompare_4(BSquad *pCollidedSquad)
{
   /*
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("doAllyCompare -- no squad, so ... um... I guess you didn't collide with anything.");
      return(cCollisionClear);
   }

   // Give priority to the faster unit.
   if (pSquad->getDesiredVelocity() - pCollidedSquad->getDesiredVelocity() > cFloatCompareEpsilon)
   {
      debugMove4("doAllyCompare -- I'm faster than squad %d, so pausing him and repathing myself..", pCollidedSquad->getID().asLong());
      return(cCollisionPauseOtherAndRepath);
   }
   if (pCollidedSquad->getDesiredVelocity() - pSquad->getDesiredVelocity() > cFloatCompareEpsilon)
   {
      debugMove4("doAllyCompare -- I'm slower than squad %d, so I'll pause myself..", pCollidedSquad->getID().asLong());
      return(cCollisionPause);
   }
   */

   // otherwise, we're the same speed, so see how's ahead. 
   return(doPlatoonCompare_4(pCollidedSquad));
}

//==============================================================================
//==============================================================================
int BSquadActionMove::doGenericCompare_4(BSquad *pCollidedSquad)
{
   return(doPlatoonCompare_4(pCollidedSquad));
}

//==============================================================================
//==============================================================================
int BSquadActionMove::doForwardCompare_4(BSquad *pCollidedSquad, bool skipPriorities)
{

   // Get our squad first.. 
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("doForwardCompare -- no squad, so ... um... I guess you didn't collide with anything.");
      return(cCollisionClear);
   }

   // Check to see if we are moving in roughly the same direction as the guy we collided with.
   // If we are, then pause ourselves if he's in front.
   // jce [4/24/2008] -- need real movement directions here.
   BVector ourDir = pSquad->getForward();
   BVector hisDir = pCollidedSquad->getForward();
   
   // jce [5/9/2008] -- normalize the XZ projection of this in case the up vector is not straight up the Y axis.
   ourDir.y = 0.0f;
   ourDir.normalize();
   hisDir.y = 0.0f;
   hisDir.normalize();

   // Get 2D dot product.
   float dot=ourDir.x*hisDir.x + ourDir.z*hisDir.z;
   debugMove4("      cos angle between us=%0.2f (angle=%0.2f degrees)   _fabs(dot-1.0f)=%0.2f.", dot, acosf(dot)*cDegreesPerRadian, _fabs(dot-1.0f));
   
   // If directions are "close enough", check who is in front.
   //if(_fabs(dot-1.0f)<0.3f)
   // jce [5/12/2008] -- trying wider range here.
   if(_fabs(dot-1.0f)<0.7f)
   {
      // Find direction to other unit.
      BVector delta = pCollidedSquad->getPosition() - pSquad->getPosition();
      delta.y=0.0f;
      delta.normalize();
      
      // Average our directions.
      BVector combinedDir = ourDir+hisDir;
      combinedDir.y = 0.0f;
      combinedDir.normalize();

      // Compare to movement direction.
      dot=combinedDir.x*delta.x + combinedDir.z*delta.z;
      if(dot>0.0f)
      {
         #ifndef BUILD_FINAL
         if(!skipPriorities)
         {
            mPausedByID = pCollidedSquad->getID();
            mPausedReason = cPausedReasonOtherIsInFront;
         }
         #endif

         // He's in front, so we should pause and let him get out of our way.
         debugMove4("      he is in front of us so we'll pause.");
         return(cCollisionPause);
      }
      else
      {
         #ifndef BUILD_FINAL
         if(!skipPriorities)
         {
            BSquadActionMove *moveAction = pCollidedSquad->getMoveAction_4();
            if(moveAction)
            {
               moveAction->mPausedByID = pSquad->getID();
               moveAction->mPausedReason = cPausedReasonOtherIsInFront;
            }
         }
         #endif

         // We're in front, so we'll tell him to pause and let us move out of the way.
         debugMove4("      he is in behind us so we'll make him pause.");
         return cCollisionPauseOtherAndRepath;
      }
   }
   else if(!skipPriorities)
   {
      // We're not really going the same direction, so use priority to decide who to pause
      bool weAreHigherPriority = hasHigherPriority_4(pSquad, pCollidedSquad);
      if(weAreHigherPriority)
      {
         #ifndef BUILD_FINAL
         if(!skipPriorities)
         {
            BSquadActionMove *moveAction = pCollidedSquad->getMoveAction_4();
            if(moveAction)
            {
               moveAction->mPausedByID = pSquad->getID();
               moveAction->mPausedReason = cPausedReasonOtherIsHigherPriority;
            }
         }
         #endif

         // He's lower priority, so pause him, but we need to re-path
         debugMove4("      we're higher priority, so pause him and re-path.");
         return(cCollisionPauseOtherAndRepath);
      }
      else
      {
         #ifndef BUILD_FINAL
         if(!skipPriorities)
         {
            mPausedByID = pCollidedSquad->getID();
            mPausedReason = cPausedReasonOtherIsHigherPriority;
         }
         #endif

         // He's higher priority, so we'll pause
         debugMove4("      he is higher priority, so we'll pause.");
         return(cCollisionPause);
      }
   }

   // We only get here if skipPriorities is on and the squads weren't going in roughly the same direction.
   // The is a semi-misused return code indicating that the units are not aligned.
   return(cCollisionRepath);
}


//==============================================================================
//==============================================================================
bool BSquadActionMove::hasHigherPriority_4(const BEntity *entity, const BEntity *compareTo)
{
   // jce [4/25/2008] -- This really needs a ton of cleanup, just bolting as much into existing
   // functionality as I can for now, though.

   // NULL == lower priority I guess
   if(!entity)
      return(false);
   // Other is NULL == we're higher
   if(!compareTo)
      return(true);
      
   // If they are both squads, use the existing platoon function that compares squad priorities.
   if(entity->isClassType(BEntity::cClassTypeSquad) && compareTo->isClassType(BEntity::cClassTypeSquad))
   {
      if(entity->getParent())
      {
         // Get parent as platoon (getPlatoon as really asPlatoon)
         BPlatoon *platoon = entity->getParent()->getPlatoon();
         if(platoon)
         {
            // Ask platoon to compare.  This even works if the two squads are not in the same platoon, comparing arbitrarily by ID.
            BEntityID higherID = platoon->getHigherMovementPriority(entity->getID(), compareTo->getID());
            
            // If the higher one was the entity, then return that.
            if(higherID == entity->getID())
               return(true);
               
            // compareTo was higher priority.
            return(false);
         }
      }
   }
   
   // Fallback is arbitrary (bogus?) ID comparison for now
   bool higher = entity->getID() > compareTo->getID();
   return(higher);
}


//==============================================================================
//==============================================================================
int BSquadActionMove::findLowLevelPath_4(BVector start, BVector goal, long &pathResult, bool bCountPath, BPath *pOutPath)
{
   // Get squad.
   // jce [4/23/2008] -- can this ever really fail?
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("findLowLevelPath -- no squad, so ... um... I guess you fail.");
      
      // Giving back too many retries here since it's pretty catastrophic and we should just stop trying
      // to do stuff with this.
      return(cFindPathFailedTooManyRetries);
   }
   
   // Get pathing limiter
   BPathingLimiter* pathLimiter = gWorld->getPathingLimiter();
   if (!pathLimiter)
   {
      // Giving back too many retries here since it's pretty catastrophic and we should just stop trying
      // to do stuff with this.
      return(cFindPathFailedTooManyRetries);
   }

   // See if we're allowed to path.
   bool bAllowed = pathLimiter->requestSquadSRP(pSquad->getPlayerID());
   if (!bAllowed)
   {
      debugMove4("findLowLevelPath: pathing limiter prevented us from pathing");
      
      // Let the caller know we just didn't try to path at all due to path limiter.
      return(cFindPathFailedPathLimiter);
   }
   
   // Increment retry counter
   // If we're counting this path, then increment the path counter, and put the path in our member variable. 
   // if we're not counting it, just put the path in a throw away place and don't increment the counter.
   static BPath tempPath;
   tempPath.reset();
   BPath *pPath = NULL;
   if (bCountPath)
   {
      mSpecificPathingAttempts_4++;
      pPath = &mPath;
   }
   else
   {
      if (pOutPath != NULL)
         pPath = pOutPath;
      else
         pPath = &tempPath;
   }

   
   // Have we done too many already?
   if(mSpecificPathingAttempts_4 > cMaxSpecificPathingAttemptsAllowed)
   {
      // Too many retries, so don't path and let the caller know it's time to give up.
      debugMove4("findLowLevelPath: ran out of retries");
      return(cFindPathFailedTooManyRetries);
   }
   

   // Set up the path class.
   int pathClass = BPather::cSquadLandPath;
   if (isPathableAsFloodUnit())
      pathClass = BPather::cFloodPath;
   else if (isPathableAsScarabUnit())
      pathClass = BPather::cScarabPath;
   else if (isPathableAsFlyingUnit())
      pathClass = BPather::cAirPath;

   // Establish the range. Platoon moves should never use target range.  This will
   // be handled by the platoon's low level pather.  We should always try to get to
   // our formation position. 
   // DLM 10/29/08 - Exempting HitAndRun mode from this range calculation
   float fRange;
   if (!mFlagPlatoonMove && mTarget.getID().asLong() != -1 && (pSquad->getSquadMode() != BSquadAI::cModeHitAndRun))
   {
      // Get range if we have a target.
      pSquad->calculateRange(&mTarget, fRange, NULL, NULL);

      debugMove4("findLowLevelPath: pathing with target ID: %d, range: %0.2f", mTarget.getID().asLong(), fRange);
   }
   else
   {
      // No range with target.
      fRange = 0.0f;
      debugMove4("findLowLevelPath: pathing without target ID");
   }
   
   // Get current distance to target.
   float distToTarget = pSquad->calculateXZDistance(mTarget);
   
   // See if we're in range.  If so, just return inRangeAtStart.  The pather doesn't account for your
   // obstruction size, etc. in it's version of this because it was assuming age3 action logic.
   if(distToTarget <= fRange)
   {
      pathResult = BPath::cInRangeAtStart;
      return(cFindPathOk);
   }


   // And the radius..
   float fRadius = getPathingRadius();
   debugMove4("findLowLevelPath: pathing with radius of: %f", fRadius);

   // Figure out canJump
   bool jumping = pSquad->canJump();

   // Build an ignore list.
   //BEntityIDArray ignoreList;
   //buildIgnoreList_4(ignoreList);
   doObMgrBegin_4(pSquad);

   // We need to adjust out some moving stuff for pathing.
   // Save old quadtrees.
   long oldQuadtrees = gObsManager.getTreesToScan();
   long quadtrees = oldQuadtrees;
   quadtrees &= ~BObstructionManager::cIsNewTypeCollidableMovingUnit;
   quadtrees &= ~BObstructionManager::cIsNewTypeCollidableMovingSquad;
   quadtrees &= ~BObstructionManager::cIsNewTypeCollidableStationaryUnit;
   
   // Set updated quadtree flags.
   gObsManager.setTreesToScan(quadtrees);

   // and Path   
   pathResult = gPather.findPath(pSquad->getID(), // EntityID
                                 BEntity::cClassTypeSquad,           // EntityType
                                 &gObsManager,                       // Pointer to obstruction mgr
                                 start,                              // Start
                                 goal,                               // Goal
                                 fRadius,                            // Radius
                                 fRange,                             // Range
                                 gObsManager.getEntityIgnoreList(),  // Ignore List
                                 pPath,                             // Path to be filled in
                                 true,                               // Skip BeginPathing
                                 false,                              // Full Path Only
                                 jumping,                            // Can Unit Jump?
                                 mTarget.getID().asLong(),           // Target ID if we have one
                                 BPather::cShortRange,               // Path Type
                                 pathClass);                         // Pathing Class

   // Restore old settings.
   gObsManager.setTreesToScan(oldQuadtrees);

   debugMove4("findLowLevelPath: LLP from (%0.2f, %0.2f, %0.2f) to (%0.2f, %0.2f, %0.2f)", start.x, start.y, start.z, goal.x, goal.y, goal.z);
   debugMove4("   pathResult=%s, %d waypoints", BPath::getPathResultName(pathResult), pPath->getNumberWaypoints());

   // Save the creation time.
   pPath->setCreationTime(gWorld->getGametime());

   // Save the result.. 
   if (bCountPath)
      mLastPathResult_4 = pathResult;
    
   // Successfully ran a path.  Result of the path itself might still be "failed", however.
   return(cFindPathOk);
}


#endif


//==============================================================================
//==============================================================================
void BSquadActionMove::debug(char* v, ... ) const
{
   #ifndef BUILD_FINAL
   static char out[BLogManager::cMaxLogTextLength];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   static char out2[BLogManager::cMaxLogTextLength*2];

   const BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   bsnprintf(out2, sizeof(out2), "SQUAD ID#%5d: %s", pSquad->getID(), out);
   gConsole.output(cChannelSim, out2);
   #endif
   return;
}


//==============================================================================
//==============================================================================
int BSquadActionMove::getPathAndWaypointForPredictions(BPath &path)
{
   #ifdef _MOVE4
      // jce [4/22/2008] -- completely bogus version for _MOVE4
      path.reset();
      
      // Get squad
      BSquad *squad = reinterpret_cast<BSquad*>(mpOwner);
      if(!squad)
         return(0);
      
      // Get platoon.
      BPlatoon *platoon = squad->getParentPlatoon();
      if(!platoon)
         return(0);
      
      // Get the low-level path & current waypoint.
      const BPath &llPath = platoon->getCurrentLLPath_4();
      if(llPath.getNumberWaypoints() < 2)
         return(0);
      int llWaypoint = platoon->getCurrentLLWP_4();
      if(llWaypoint < 1)
         return(0);
         
      // We'll offset by our formation position, unless for some reason we fail to get an offset.
      const BFormation2 *formation = platoon->getFormation();
      if(!formation)
         return(0);
      
      // First waypoint is based on the platoon's current position.
      BVector destination = formation->getTransformedFormationPositionFast(squad->getID(), platoon->getPosition(), platoon->getForward(), platoon->getRight());
      path.addWaypointAtEnd(destination);
      
      for (long i = llWaypoint; i < llPath.getNumberWaypoints(); i++)
      {
         // We'll do more than just offset.. we'll transform the formation at each waypoint, using a
         // forward vector of the current waypoint minus the previous waypoint..
         BVector forward = llPath.getWaypoint(i) - llPath.getWaypoint(i-1);
         forward.y = 0;
         forward.safeNormalize();
         BVector right;
         right.assignCrossProduct(cYAxisVector, forward);
         destination = formation->getTransformedFormationPositionFast(squad->getID(), llPath.getWaypoint(i), forward, right);

         path.addWaypointAtEnd(destination);

      }
            
      // Hand back current waypoint.
      //return(llWaypoint);
      
      // jce [9/12/2008] -- New version gives the current position as the 0th waypoint, so we're always
      // moving towards waypoint 1.
      return(1);
   #else
      // For old movement, just use mPath as was originally done.
      path = mPath;
      return(mCurrentWaypoint);
   #endif
}


#ifdef _MOVE4
//==============================================================================
//==============================================================================
void BSquadActionMove::buildIgnoreList_4(BEntityIDArray &ignoreList)
{
   // Clear out any existing items.
   ignoreList.resize(0);
   
   // If we have a target, put him on the list.
   // DLM 7/24/08 - Hi.  So having squad put their target on the ignore list is allowing them to walk directly
   // through target obstructions.  This is bad.  We'll leave the target on the ignore list at 
   // the platoon level, but leave it off hte list at the squad level.  I'm sure this breaks
   // something else horribly.

   // DLM - 7/25/08 Something new.  We DON'T automagically ignore targets anymore, but we CAN have a
   // special ignore unit.  SO, hijacking the ignoreTarget code to allow us to ignore a special ignoreUnit.  Weeee!
   uint numIgnore = mIgnoreUnits.getSize();
   if (numIgnore > 0)
   {
      ignoreList.append(mIgnoreUnits);
      for (uint i = 0; i < numIgnore; i++)
      {
         // If the thing is a squad, we need to ignore it's children, really.  (as well as the squad)
         const BEntity* pEntity = gWorld->getEntity(mIgnoreUnits[i]);
         if (pEntity && (pEntity->getClassType() == BEntity::cClassTypeSquad))
         {
            const BSquad* pTargetSquad = reinterpret_cast<const BSquad*>(pEntity);
            const BEntityIDArray& myUnitList = pTargetSquad->getChildList();
            ignoreList.add(myUnitList.getPtr(), myUnitList.size());
         }
         // Conversely, if this is a unit, please add it's squad ID to the list as well..
         if (pEntity && (pEntity->getClassType() == BEntity::cClassTypeUnit) && pEntity->getParent())
         {
            const BSquad* pTargetSquad = reinterpret_cast<BSquad*>(pEntity->getParent());
            ignoreList.add(pTargetSquad->getID());
         }
      }
   }
   
   // Get squad.
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;

   // Always add my own squad and it's children..
   ignoreList.add(pSquad->getID());
   const BEntityIDArray &myUnitList = pSquad->getChildList();
   ignoreList.add(myUnitList.getPtr(), myUnitList.size());


   // Get platoon.
   BPlatoon *pPlatoon = pSquad->getParentPlatoon();
   // DLM 5/19/08 - We should not RETURN if there is not a platoon.  We just need to be cognizant that we might
   // be on a non-platoon move, and therefore should not depend on having a platoon.
   /*
   if(!pPlatoon)
      return;
   */
   
   // jce [10/15/2008] -- If a non-platoon move, we don't ignore any platoonmates.
   if(!mFlagPlatoonMove)
      return;
   
   // jce [10/7/2008] -- If we have a platoon and it has plotted positions, do not ignore our platoonmates at all.
   if(pPlatoon && pPlatoon->hasValidPlots_4())
      return;

   // Special Halo Wars Ignore Rules
   // If I'm an infantry squad, ignore other infantry in my
   // platoon.
#ifdef IGNORE_PLATOONMATES_4
   if(gConfig.isDefined(cConfigIgnoreAllPlatoonmates))
   {
      if (pPlatoon)
      {
         const BEntityIDArray &squadList = pPlatoon->getChildList();
         for (long s = 0; s < squadList.getNumber(); s++)
         {
            // Don't add myself -- I'll do that below.
            if (squadList[s] == mpOwner->getID())
               continue;

            // Add the squad
            BSquad *pChildSquad = reinterpret_cast<BSquad*>(gWorld->getEntity(squadList[s]));
            if (!pChildSquad)
               continue;
            // DLM - 7/23/08 - More ignore rules.  Don't ingore anything that isn't done moving
            if (pPlatoon->isSquadDoneMoving_4(squadList[s]))
               continue;
           
            // to the ignore list. 
            ignoreList.add(squadList[s]);
            // Add it's children
            const BEntityIDArray &unitList = pChildSquad->getChildList();
            ignoreList.add(unitList.getPtr(), unitList.size());
         }
      }
   }
   else
   {
      const BProtoObject *pSquadProtoObject = pSquad->getProtoObject();
      bool isInfantry = (pSquadProtoObject)?pSquadProtoObject->isType(gDatabase.getOTIDInfantry()):false;
      if (isInfantry && pPlatoon)
      {
         const BEntityIDArray &squadList = pPlatoon->getChildList();
         for (long s = 0; s < squadList.getNumber(); s++)
         {
            // Don't add myself -- I'll do that below.
            if (squadList[s] == mpOwner->getID())
               continue;
            BEntity *pOther = gWorld->getEntity(squadList[s]);
            if (pOther)
            {
               const BProtoObject *pOtherProtoObject = pOther->getProtoObject();
               if (!pOtherProtoObject)
                  continue;
               if (pOtherProtoObject->isType(gDatabase.getOTIDInfantry()))
               {
                  // Add the squad
                  BSquad *pChildSquad = reinterpret_cast<BSquad*>(gWorld->getEntity(squadList[s]));
                  if (!pChildSquad)
                     continue;
                  // DLM - 7/23/08 - More ignore rules.  Don't ingore anything that isn't done moving, 
                  // or anything that's in the wait state.  
                  bool bIgnore = true;
                  if (pPlatoon->isSquadDoneMoving_4(squadList[s]))
                     bIgnore = false;
                  /*
                  // Check the state of the squad's move action, if it has one.  If it's in a wait state, don't
                  // ignore it.  
                  BSquadActionMove* pMoveAction=reinterpret_cast <BSquadActionMove*> (pChildSquad->getActionByType(BAction::cActionTypeSquadMove));
                  if (pMoveAction)
                  {
                     if (pMoveAction->getState() == cStateWait)
                        bIgnore = false;
                  }
                  else
                     bIgnore = false;
                  if (!pChildSquad->isMoving())
                     bIgnore = false;
                  */

                  // to the ignore list. 
                  if (bIgnore)
                  {
                     ignoreList.add(squadList[s]);
                     // Add it's children
                     const BEntityIDArray &unitList = pChildSquad->getChildList();
                     ignoreList.add(unitList.getPtr(), unitList.size());
                  }
               }
            }
         }
      }
   }
#endif

}


//==============================================================================
//==============================================================================
void BSquadActionMove::pauseMovement_4(DWORD pauseTime)
{
   // Remember the timeout.  However, if we're already paused, don't reduce the timeout that
   // someone else has already asked for (only increase it)
   if(mState != cStateWait || pauseTime>mPauseTimeRemaining_4)
      mPauseTimeRemaining_4 = pauseTime;
   
   // Go into wait state.
   setState(cStateWait);
   
   // Get squad.
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
      return;

   // Notify parent platoon, if any.
   BPlatoon *pPlatoon = pSquad->getParentPlatoon();
   if(pPlatoon)
      pPlatoon->notifyThatSquadHasPaused_4(pSquad->getID());
}

//==============================================================================
//==============================================================================
void BSquadActionMove::setMoveActionPath_4(BPath &newPath)
{
   if (newPath.getNumberWaypoints() <= 0)
      return;
   mPath = newPath;
   mCurrentWaypoint = 1;
   mFlagFollowingSpecificPath_4 = true;
}


//==============================================================================
// canEarlyTerminate
// DLM 5/23/08 - This is a hack to attempt to allow squads to settle down more 
// quickly at the end of a movement.  The premise is actual formation positions
// are not important.  If the platoon is done moving, and we're "close enough"
// to the platoon's position, then if we're following a specific path, see
// if we're on an obstructed location.  If so, then just stop where we are.  
//==============================================================================
bool BSquadActionMove::canEarlyTerminate_4()
{
   BFAIL("Calling is is going to break the doObMgrBegin optimization and that will need to get unraveled if we start using this again");
 
   // Get our squad first.. 
   BSquad* pSquad=reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pSquad);
   if (!pSquad)
   {
      debugMove4("canEarlyTerminate -- no squad, so no, you can't terminate.");
      return false;
   }
   if (!mFlagFollowingSpecificPath_4)
      return false;
   if (!mFlagPlatoonMove)
      return false;

   // Get the platoon.
   BPlatoon* pPlatoon = pSquad->getParentPlatoon();
   if (!pPlatoon)
      return false;

   BPlatoonActionMove* pPlatoonActionMove = reinterpret_cast<BPlatoonActionMove*>(pPlatoon->getActionByType(BAction::cActionTypePlatoonMove));
   if (pPlatoonActionMove && (pPlatoonActionMove->getState() != cStateWaitOnChildren))
   {
      return false;
   }

   // If the platoon has valid plot points, then don't early terminate.  It's not important we get to formation positions,
   // per se, but it is important that we got to plotted positions. 
   if (pPlatoon->hasValidPlots_4())
      return false;

   float fDistToPlatoon = pPlatoon->getPosition().xzDistanceSqr(pSquad->getPosition());
   if (fDistToPlatoon > (float)BPather::cShortRangeLimitDist)
      return false;

   // All the easy checks are done.  We need to see if we're standing on anything. 
   long quadtrees=BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeAllCollidableSquads;
   if (isPathableAsFlyingUnit())
      quadtrees = BObstructionManager::cIsNewTypeBlockAirMovement;
   else if (isPathableAsFloodUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
   else if (isPathableAsScarabUnit())
      quadtrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
   else
      quadtrees |= BObstructionManager::cIsNewTypeBlockLandUnits;

   // Set up ignore list.
   BEntityIDArray ignoreList;
   // Don't use the standard ignoreList for movement.  In this case, we want to check against EVERY ONE, 
   // including our platoon mates.  So only add myself and my children.
   ignoreList.add(pSquad->getID());
   const BEntityIDArray &myUnitList = pSquad->getChildList();
   ignoreList.add(myUnitList.getPtr(), myUnitList.size());

   // Set up obstruction manager.
   gObsManager.begin(BObstructionManager::cBeginNone, getPathingRadius(), quadtrees, BObstructionManager::cObsNodeTypeAll, pSquad->getPlayerID(), cDefaultRadiusSofteningFactor, &ignoreList, pSquad->canJump());

   // See if we intersect anything.
   BVector iPoint;
   BObstructionNodePtrArray obstructionList;
   gObsManager.findObstructions(pSquad->getPosition(), false, false, obstructionList);

   // Done with obstruction manager.
   gObsManager.end();

   if (obstructionList.getNumber() != 0)
      return false;

   return true;
}


#endif

//==============================================================================
//==============================================================================
bool BSquadActionMove::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   if (mpOwner)
   {
      BASSERT(mpOwner->getSquad());
   }

   #ifdef _MOVE4
      GFWRITEVAL(pStream, bool, true);
   #else
      GFWRITEVAL(pStream, bool, false);
   #endif

   #ifdef PRECALC_TURNRADIUS
      GFWRITEVAL(pStream, bool, true);
   #else
      GFWRITEVAL(pStream, bool, false);
   #endif

   #ifdef _MOVE4
      GFWRITEVECTOR(pStream, mInterimTarget_4);
      GFWRITEVECTOR(pStream, mFormationPositionWhenSpecificPathWasCreated_4);
      GFWRITEVAR(pStream, int32, mSpecificPathingAttempts_4);
      GFWRITEVAR(pStream, DWORD, mPauseTimeRemaining_4);
      #ifdef PRECALC_TURNRADIUS
         GFWRITECLASS(pStream, saveType, mTurnRadiusSourcePath_4);
      #endif
   #endif

   GFWRITEVECTORARRAY(pStream, mUserLevelWaypoints, uint8, 200);
   GFWRITEARRAY(pStream, int16, mUserLevelWaypointsPathIndex, uint8, 200);
   GFWRITECLASS(pStream, saveType, mPath);
   GFWRITEVAR(pStream, DWORD, mUserLevelWaypointUpdateTime);
   GFWRITEVAR(pStream, int, mCurrentWaypoint);
   GFWRITEVAR(pStream, DWORD, mPathingRetryTime);
   GFWRITEVAR(pStream, float, mPercentComplete);

   #ifdef _MOVE4
      GFWRITEVAR(pStream, long, mLastPathResult_4);
   #endif

   GFWRITEVECTORARRAY(pStream, mProjectionPath, uint8, 200);
   GFWRITECLASSARRAY(pStream, saveType, mProjections, uint8, 200);
   GFWRITEARRAY(pStream, BEntityID, mProjectionSiblings, uint8, 200);
   GFWRITEVAR(pStream, float, mProjectionDistance);
   GFWRITEVAR(pStream, float, mProjectionTime);
   GFWRITEVAR(pStream, float, mVelocity);
   GFWRITEVAR(pStream, DWORD, mLastProjectionSiblingUpdate);
   GFWRITEVAR(pStream, BEntityID, mBlockingEntityID);

   #ifdef PRECALC_TURNRADIUS
      GFWRITEVECTORARRAY(pStream, mTurnRadiusArcCenters, uint8, 200);
      GFWRITEVECTORARRAY(pStream, mTurnRadiusArcData, uint8, 200);
      GFWRITEVECTORARRAY(pStream, mTurnRadiusTimes, uint8, 200);
      GFWRITEVECTOR(pStream, mStartingDir);
      GFWRITEVAR(pStream, float, mTurnSegmentTime);
      GFWRITEARRAY(pStream, float, mTurnRadiusOverlapAngle, uint8, 200);
   #endif

   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);
   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFWRITEVAR(pStream, BActionID, mChildActionID);
   GFWRITEARRAY(pStream, BEntityID, mIgnoreUnits, uint8, 200);
   GFWRITEVAR(pStream, uint32, mNumPathingDelays);
   GFWRITEVAR(pStream, uint32, mMaxPathingDelays);
   GFWRITEVAR(pStream, int, mAutoMode);
   GFWRITEVAR(pStream, int, mChangeModeState);
   GFWRITEVAR(pStream, int, mAbilityID);
   GFWRITEVECTOR(pStream, mOverridePosition);
   GFWRITEVAR(pStream, float, mOverrideRange);      
   GFWRITEVAR(pStream, float, mOverrideRadius);      
   GFWRITEVAR(pStream, uint8, mUnitOppIDCount);
   GFWRITEVAR(pStream, BActionState, mUnpauseState);
   GFWRITEVAR(pStream, uchar, mNumPathingRetriesRemaining);
   GFWRITEBITBOOL(pStream, mFlagPlatoonMove);
   GFWRITEBITBOOL(pStream, mFlagMonitorOpps);
   GFWRITEBITBOOL(pStream, mFlagAutoSquadMode);
   GFWRITEBITBOOL(pStream, mFlagFailed);   
   GFWRITEBITBOOL(pStream, mFlagMustFinishPath);   
   GFWRITEBITBOOL(pStream, mFlagLastOutsidePlayableBounds);
   GFWRITEBITBOOL(pStream, mFlagPathingFailedDueToBlocking);
   GFWRITEBITBOOL(pStream, mFlagInAir);
   GFWRITEBITBOOL(pStream, mFlagSkidding);
   GFWRITEBITBOOL(pStream, mFlagAbilityCommand);
   GFWRITEBITBOOL(pStream, mFlagAutoExitMode);
   GFWRITEBITBOOL(pStream, mFlagAutoLock);
   GFWRITEBITBOOL(pStream, mFlagAutoUnlock);
   GFWRITEBITBOOL(pStream, mFlagForceLeashUpdate);
   GFWRITEBITBOOL(pStream, mFlagAutoMode);
   GFWRITEBITBOOL(pStream, mFlagAutoAttackMode);
   GFWRITEBITBOOL(pStream, mFlagFollowingSpecificPath_4);
   GFWRITEBITBOOL(pStream, mFlagOverrideLockDown);
   GFWRITEBITBOOL(pStream, mFlagSelfImposedPause_4);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionMove::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   if (mpOwner)
   {
      BASSERT(mpOwner->getSquad());
   }

   // Don't bother supporting loading save games made with different move4/precalc defines
   bool oldMove4, oldPrecalc;
   GFREADVAR(pStream, bool, oldMove4);
   GFREADVAR(pStream, bool, oldPrecalc);
   bool curMove4, curPrecalc;
   #ifdef _MOVE4
      curMove4=true;
   #else
      curMove4=false;
   #endif
   #ifdef PRECALC_TURNRADIUS
      curPrecalc=true;
   #else
      curPrecalc=false;
   #endif
   if (curMove4 != oldMove4 || curPrecalc != oldPrecalc)
   {
      GFERROR("GameFile Error: squadactionmove defines different oldMove4=%d oldPrecalc=%d curMove4=%d curPrecalc=%d", (int)oldMove4, (int)oldPrecalc, (int)curMove4, (int)curPrecalc);
      return false;
   }

   #ifdef _MOVE4
      GFREADVECTOR(pStream, mInterimTarget_4);
      if (BAction::mGameFileVersion >= 4)
         GFREADVECTOR(pStream, mFormationPositionWhenSpecificPathWasCreated_4)
      else
         mFormationPositionWhenSpecificPathWasCreated_4 = mInterimTarget_4;
      GFREADVAR(pStream, int32, mSpecificPathingAttempts_4);
      GFREADVAR(pStream, DWORD, mPauseTimeRemaining_4);
      #ifdef PRECALC_TURNRADIUS
         GFREADCLASS(pStream, saveType, mTurnRadiusSourcePath_4);
      #endif
   #endif

   GFREADVECTORARRAY(pStream, mUserLevelWaypoints, uint8, 200);
   GFREADARRAY(pStream, int16, mUserLevelWaypointsPathIndex, uint8, 200);
   GFREADCLASS(pStream, saveType, mPath);
   GFREADVAR(pStream, DWORD, mUserLevelWaypointUpdateTime);
   GFREADVAR(pStream, int, mCurrentWaypoint);
   GFREADVAR(pStream, DWORD, mPathingRetryTime);
   GFREADVAR(pStream, float, mPercentComplete);

   #ifdef _MOVE4
      GFREADVAR(pStream, long, mLastPathResult_4);
   #endif

   GFREADVECTORARRAY(pStream, mProjectionPath, uint8, 200);
   GFREADCLASSARRAY(pStream, saveType, mProjections, uint8, 200);
   GFREADARRAY(pStream, BEntityID, mProjectionSiblings, uint8, 200);
   GFREADVAR(pStream, float, mProjectionDistance);
   GFREADVAR(pStream, float, mProjectionTime);
   GFREADVAR(pStream, float, mVelocity);
   GFREADVAR(pStream, DWORD, mLastProjectionSiblingUpdate);
   GFREADVAR(pStream, BEntityID, mBlockingEntityID);

   #ifdef PRECALC_TURNRADIUS
      GFREADVECTORARRAY(pStream, mTurnRadiusArcCenters, uint8, 200);
      GFREADVECTORARRAY(pStream, mTurnRadiusArcData, uint8, 200);
      GFREADVECTORARRAY(pStream, mTurnRadiusTimes, uint8, 200);
      GFREADVECTOR(pStream, mStartingDir);
      GFREADVAR(pStream, float, mTurnSegmentTime);
      GFREADARRAY(pStream, float, mTurnRadiusOverlapAngle, uint8, 200);
   #endif

   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);
   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADFREELISTITEMPTR(pStream, BSimOrder, mpChildOrder);
   GFREADVAR(pStream, BActionID, mChildActionID);
   GFREADARRAY(pStream, BEntityID, mIgnoreUnits, uint8, 200);
   GFREADVAR(pStream, uint32, mNumPathingDelays);
   GFREADVAR(pStream, uint32, mMaxPathingDelays);
   GFREADVAR(pStream, int, mAutoMode);
   GFREADVAR(pStream, int, mChangeModeState);
   GFREADVAR(pStream, int, mAbilityID);
   GFREADVECTOR(pStream, mOverridePosition);
   GFREADVAR(pStream, float, mOverrideRange);      
   GFREADVAR(pStream, float, mOverrideRadius);      
   GFREADVAR(pStream, uint8, mUnitOppIDCount);
   GFREADVAR(pStream, BActionState, mUnpauseState);
   GFREADVAR(pStream, uchar, mNumPathingRetriesRemaining);
   GFREADBITBOOL(pStream, mFlagPlatoonMove);
   GFREADBITBOOL(pStream, mFlagMonitorOpps);
   GFREADBITBOOL(pStream, mFlagAutoSquadMode);
   GFREADBITBOOL(pStream, mFlagFailed);   
   GFREADBITBOOL(pStream, mFlagMustFinishPath);   
   GFREADBITBOOL(pStream, mFlagLastOutsidePlayableBounds);
   GFREADBITBOOL(pStream, mFlagPathingFailedDueToBlocking);
   GFREADBITBOOL(pStream, mFlagInAir);
   GFREADBITBOOL(pStream, mFlagSkidding);
   GFREADBITBOOL(pStream, mFlagAbilityCommand);
   GFREADBITBOOL(pStream, mFlagAutoExitMode);
   GFREADBITBOOL(pStream, mFlagAutoLock);
   GFREADBITBOOL(pStream, mFlagAutoUnlock);
   GFREADBITBOOL(pStream, mFlagForceLeashUpdate);
   GFREADBITBOOL(pStream, mFlagAutoMode);
   GFREADBITBOOL(pStream, mFlagAutoAttackMode);
   GFREADBITBOOL(pStream, mFlagFollowingSpecificPath_4);
   GFREADBITBOOL(pStream, mFlagOverrideLockDown);
   GFREADBITBOOL(pStream, mFlagSelfImposedPause_4);

   return true;
}

//==============================================================================
//==============================================================================
