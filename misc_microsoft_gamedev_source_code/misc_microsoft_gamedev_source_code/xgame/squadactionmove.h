//==============================================================================
// squadactionmove.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "movementHelper.h"
#include "findpathhelper.h"

class BUnitOpp;

//#define DEBUGPROJECTIONCOLLISIONS

#ifndef _MOVE4
#define _MOVE4
#endif

#ifdef _MOVE4
   #define NEW_TURNRADIUS
#else
   #define PRECALC_TURNRADIUS
#endif


//==============================================================================
//==============================================================================
class BSquadActionMove : public BAction
{
   public:
      BSquadActionMove() { }
      virtual ~BSquadActionMove() { }

      typedef enum
      {
         cUserWpComplete = -1,            // Moved beyond this user level waypoint
         cUserWpNeedPath = -2             // Need to generate path to this user level waypoint
      };

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               isInterruptible() const;

      const BPath&               getPath() const                        { return mPath; }
      int                        getCurrentWaypoint() const             { return mCurrentWaypoint; }
      bool                       tackOnWaypoints(const BDynamicSimVectorArray &waypoints);
      bool                       getFuturePosition(float desiredTimeIntoFuture, bool ignoreBlocked, BVector &futurePosition, float &realTimeNeeded);
      bool                       getFuturePosition2(float desiredTimeIntoFuture, bool ignoreBlocked, bool useCurrentVelocity, BDynamicVectorArray& futurePositions, float &realTimeNeeded, float& realDistanceNeeded);

      const BDynamicVectorArray& getProjectionPath() const { return (mProjectionPath); }
      const BSimpleConvexHullArray& getProjections() const { return (mProjections); }
      float                      getProjectionTime() const { return (mProjectionTime); }
      float                      getProjectionDistance() const { return (mProjectionDistance); }

      float                      getPercentComplete() const                   { return mPercentComplete; }

      uint                       getCurrentUserLevelWaypointIndex() const;
      uint                       getNumUserLevelWaypoints() const             { return (mUserLevelWaypoints.getSize()); }
      DWORD                      getUserLevelWaypointUpdateTime() const       { return mUserLevelWaypointUpdateTime; }

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target) { mTarget=target; }
      //ParentAction.
      virtual BAction*           getParentAction() const { return (mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction=v; }
      
      bool                       getFlagPlatoonMove() const { return mFlagPlatoonMove; }
      void                       setFlagPlatoonMove(bool v) { mFlagPlatoonMove=v; }
      bool                       getFlagMonitorOpps() const { return (mFlagMonitorOpps); }
      void                       setFlagMonitorOpps(bool v) { mFlagMonitorOpps = v; }
      bool                       getFlagAutoSquadMode() const { return (mFlagAutoSquadMode); }
      void                       setFlagAutoSquadMode(bool v) { mFlagAutoSquadMode = v; }
      void                       setFlagForceLeashUpdate(bool v) { mFlagForceLeashUpdate = v; }
      
      // The ignore unit allows the move action to exempt a particular unit from pathing and collision.
      // This is currently used for warthog ramming.
      void                       addIgnoreUnit(BEntityID ignoreUnit) { mIgnoreUnits.uniqueAdd(ignoreUnit); }
      void                       addIgnoreUnits(BEntityIDArray ignoreUnits) { mIgnoreUnits.append(ignoreUnits); }

      float                      getVelocity() const { return (mVelocity); }

      virtual bool               setState(BActionState state);
      virtual bool               pause();
      virtual bool               unpause();
      void                       pausePlatoon();
      virtual bool               update(float elapsedTime);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      virtual void               debugRender();
      #ifndef BUILD_FINAL
      virtual uint               getNumberDebugLines() const { return (3); }
      virtual void               getDebugLine(uint index, BSimString &string) const;
      #endif
      // DLM 2/28/08 - added debug statement
      virtual void               debug(char *v, ...) const;

      #ifdef PRECALC_TURNRADIUS
         float                   getFinalTurnSegmentTime() const { return (mTurnRadiusTimes.getNumber() > 0) ? mTurnRadiusTimes[mTurnRadiusTimes.getSize() - 1].z : 0.0f; }
         float                   getTurnSegmentTime() const { return mTurnSegmentTime; }
         void                    turnRadiusInterpolate(float time, BVector& position, BVector& direction, uint& srcWaypoint);
      #endif

      uint32                     getNumPathingDelays() const { return (mNumPathingDelays); }
      uint32                     getMaxPathingDelays() const { return (mMaxPathingDelays); }

      // Override squad plotter
      void setOverridePosition(BVector overridePosition) { mOverridePosition = overridePosition; }
      void setOverrideRange(float overrideRange) { mOverrideRange = overrideRange;}      
      void setOverrideRadius(float overrideRadius) { mOverrideRadius = overrideRadius; }

      DECLARE_FREELIST(BSquadActionMove, 5);

      #ifdef _MOVE4
      float                      getPlatoonVelocity_4();
      // Interim Target - This is updated on a per update basis.  Each update, this is the platoon's formation position
      // for the squad, and is the thing the squad is trying to move towards.  We are using this so the squad can also
      // have the platoon's REAL target, so it can tell if it's really done or not.
      void                       setInterimTarget_4(const BVector &target) { mInterimTarget_4 = target;}
      BVector                    &getInterimTarget_4() { return mInterimTarget_4; }
      void                       pauseMovement_4(DWORD pauseTime);
      DWORD                      getPauseTimeRemaining_4() const {return(mPauseTimeRemaining_4);}
      void                       setMoveActionPath_4(BPath &newPath);

      // DLM 10/30/08 - Exposing things that should never be exposed.
      enum
      {
         cFindPathOk,
         cFindPathFailedPathLimiter,
         cFindPathFailedTooManyRetries,
         cFindPathFailedNoAction
      };
      int                        findLowLevelPath_4(BVector start, BVector goal, long &pathResult, bool bCountPath = true, BPath *pOutPath = NULL);


      #endif

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      float                      getPathingRadius() const;
      float                      getLargestUnitRadius() const;
      bool                       isPathableAsFlyingUnit() const;
      bool                       isPathableAsFloodUnit() const;
      bool                       isPathableAsScarabUnit() const;
      bool                       isPlatoonPathUpdateNeeded();
      BActionState               generatePathFromPlatoon(bool tackOnNewWaypoints);

      inline void                markCompletedUserLevelWaypoints();
      BActionState               findPath(bool ignorePlatoonMates=true, bool repathing=false);
      BActionState               findMorePathSegments(bool ignorePlatoonMates=true);
      inline BActionState        calcMovement(float elapsedTime, float velocity, BVector& newPosition, BVector& newDirection);
      bool                       needToRepathToMovingTarget();
      BActionState               followPath(float elapsedTime);
      float                      getNextUserLevelWaypointDistance();

      void                       updateProjectionSiblings();
      float                      calculateVelocityModifier(bool& inside, bool& blockedOnStoppedSquads, bool& finalDest, BEntityID& blockingEntity);
      float                      calculateVelocityModifierSimple();
      float                      calculateDesiredVelocityBySquadType();
      void                       calculateProjection();
      bool                       checkProjectedCollision(BConvexHull& hull, bool &inside, float &collisionDistance, BVector& collisionPoint);
      bool                       checkProjectedCollision(BVector& pos, float& collisionDistance);
      float                      testFinalDestination(bool& finalDest);
      void                       calcPercentComplete();

      void                       render();
      bool                       setupOpp();
      bool                       addOpp(BUnitOpp &opp);
      void                       removeOpp();
      
      #ifdef PRECALC_TURNRADIUS
         inline BActionState     calcTurnRadiusMovement(float elapsedTime, float velocity, BVector& newPosition, BVector& newDirection);
         void                    updateTurnRadiusPath(int startWaypointIndex);
      #endif
      
      void                       updateMovementSound();

      bool                       updatePathingLimits();
      bool                       isStillBlocked();

      void                       setupChildOrder(BSimOrder* pOrder);
      
      int                        getPathAndWaypointForPredictions(BPath &path);

#ifdef _MOVE4

      bool                       update_4(float elapsedTime);
      BActionState               doWorkUpdate_4(float elapsedTime);
      BActionState               doWorkUpdateInternal_4(float elapsedTime);
      void                       doObMgrBegin_4(BSquad *pSquad);
      void                       buildIgnoreList_4(BEntityIDArray &ignoreList);
      bool                       isBehindPlatoon_4();
      bool                       canEarlyTerminate_4();

      BVector                    mInterimTarget_4;
      BVector                    mFormationPositionWhenSpecificPathWasCreated_4;
      int32                      mSpecificPathingAttempts_4;
      DWORD                      mPauseTimeRemaining_4;
      #ifdef PRECALC_TURNRADIUS
         BPath                   mTurnRadiusSourcePath_4;         // This is the path that is to be used as the source path for updateTurnRadius calculations.
      #endif

      // Collision checking.
      enum
      {
         cCollisionInvalid = -1,
         cCollisionClear = 0,
         cCollisionPause,
         cCollisionPauseOther,
         cCollisionRepath,
         cCollisionPauseOtherAndRepath,
         cCollisionHitTarget,
         cCollisionReturn
      };
      int                        checkCollisions_4(BVector start, BVector end, BVector &iPoint);
      bool                       hasHigherPriority_4(const BEntity *entity, const BEntity *compareTo);
      int                        doPlatoonCompare_4(BSquad *pCollidedSquad);
      int                        doAllyCompare_4(BSquad *pCollidedSquad);
      int                        doGenericCompare_4(BSquad *pCollidedSquad);
      int                        doForwardCompare_4(BSquad *pCollidedSquad, bool skipPriorities=false);
      
#endif


      BDynamicSimVectorArray     mUserLevelWaypoints;
      BDynamicSimArray<int16>    mUserLevelWaypointsPathIndex;
      BPath                      mPath;
      DWORD                      mUserLevelWaypointUpdateTime;
      int                        mCurrentWaypoint;
      DWORD                      mPathingRetryTime;
      float                      mPercentComplete;

      #ifdef _MOVE4
      long                       mLastPathResult_4;
      #endif

      BDynamicVectorArray        mProjectionPath;
      BSimpleConvexHullArray     mProjections;
      BEntityIDArray             mProjectionSiblings;
      float                      mProjectionDistance;
      float                      mProjectionTime;
      float                      mVelocity;
      DWORD                      mLastProjectionSiblingUpdate;
      BEntityID                  mBlockingEntityID;

      // Turn radius stuff
      #ifdef PRECALC_TURNRADIUS
         BDynamicSimVectorArray     mTurnRadiusArcCenters;
         BDynamicSimArray<BVector4> mTurnRadiusArcData;
         BDynamicSimVectorArray     mTurnRadiusTimes;
         BVector                    mStartingDir;
         float                      mTurnSegmentTime;
         BDynamicSimFloatArray      mTurnRadiusOverlapAngle;
      #endif

      BSimTarget                 mTarget;
      BUnitOppID                 mUnitOppID;
      BAction*                   mpParentAction;
      BSimOrder*                 mpChildOrder;
      BActionID                  mChildActionID;
      BEntityIDArray             mIgnoreUnits;
      uint32                     mNumPathingDelays;
      uint32                     mMaxPathingDelays;
      int                        mAutoMode;
      int                        mChangeModeState;
      int                        mAbilityID;      

      // Override squad plotter
      BVector                    mOverridePosition;
      float                      mOverrideRange;      
      float                      mOverrideRadius;      

      uint8                      mUnitOppIDCount;
      BActionState               mUnpauseState;
      uchar                      mNumPathingRetriesRemaining;
      bool                       mFlagPlatoonMove:1;
      bool                       mFlagMonitorOpps:1;
      bool                       mFlagAutoSquadMode:1;
      bool                       mFlagFailed:1;   
      bool                       mFlagMustFinishPath:1;   
      bool                       mFlagLastOutsidePlayableBounds:1;
      bool                       mFlagPathingFailedDueToBlocking:1;
      bool                       mFlagInAir:1;
      bool                       mFlagSkidding:1;
      bool                       mFlagAbilityCommand:1;
      bool                       mFlagAutoExitMode:1;
      bool                       mFlagAutoLock:1;
      bool                       mFlagAutoUnlock:1;
      bool                       mFlagForceLeashUpdate:1;
      bool                       mFlagAutoMode:1;
      bool                       mFlagAutoAttackMode:1;
      bool                       mFlagFollowingSpecificPath_4:1;
      bool                       mFlagOverrideLockDown:1;
      bool                       mFlagSelfImposedPause_4:1;

      #ifdef DEBUGPROJECTIONCOLLISIONS
      BSimpleConvexHullArray     mDebugProjections;
      BDynamicSimVectorArray     mDebugCollisions;
      BVector                    mDebugEarlyCollisionPoint;
      bool                       mDebugEarlyCollisionPointValid:1;
      #endif
      
      #ifndef BUILD_FINAL
      public:
      // jce [9/11/2008] -- these are just for debugging display and don't need to be saved or anything
      BVector                    mLastCollisionCheck[2];
      
      enum
      {
         cPausedReasonPathFailed,
         cPausedReasonPathInRangeAtStart,
         cPausedReasonOtherIsFaster,
         cPausedReasonOtherIsInFront,
         cPausedReasonOtherWakesUpSoon,
         cPausedReasonOtherIsHigherPriority,
         cPausedReasonNoClearSpot,
         cPausedReasonPathSucks,
         cPausedReasonPlatoonIsPaused
      };
      long                       mPausedReason;
      BEntityID                  mPausedByID;
      
      protected:
      #endif
};
