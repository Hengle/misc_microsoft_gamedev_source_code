//==============================================================================
// actionmove.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "movementHelper.h"
#include "obstructionmanager.h"

//#define RENDERGAGGLE

#ifndef _MOVE4
#define _MOVE4
#endif

//==============================================================================
//==============================================================================
class BPathMoveData : public IPoolable
{
   public:
      BPathMoveData();
      ~BPathMoveData();

      // IPoolable Methods
      virtual void onAcquire();
      virtual void onRelease();

      typedef enum
      {
         cPathLevelLow,
         cPathLevelMid,
         cPathLevelUser
      };

      void clear();
      bool                    isJumpPath() const;

      BPath                   mPath;
      int                     mCurrentWaypoint;
      DWORD                   mPathTime;
      BPathMoveData*          mLinkedPath;
      BPathLevel              mPathLevel;

      DECLARE_FREELIST(BPathMoveData, 4)

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

};


//==============================================================================
//==============================================================================
class BUnitActionMove : public BAction
{
   public:
      BUnitActionMove() { }
      virtual ~BUnitActionMove() { }

      // Find path results
      typedef enum
      {
         cFindPathSuccess,
         cFindPathFailure,
         cFindPathInRangeAtStart
      } BFindPathResult;

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();
      void                       clearPathData();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsedTime);
      
      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;
      //FlagSquadMove.
      bool                       getFlagSquadMove() const { return (mFlagSquadMove); }
      void                       setFlagSquadMove(bool v) { mFlagSquadMove=v; }

      float                      getVelocity() const { return mVelocity; }

      virtual void               debugRender();
      #ifndef BUILD_FINAL
      virtual uint               getNumberDebugLines() const { return (3); }
      virtual void               getDebugLine(uint index, BSimString &string) const;
      #endif

      DECLARE_FREELIST(BUnitActionMove, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       working(float elapsedTime, bool moving);
      bool                       playTurnAnimation(long animType);
      bool                       playStopMoveAnimation();
      bool                       playStartMoveAnimation();
      void                       drawDebugArrow(BVector arrowDirection, float height, DWORD color);
      BActionState               createInitialPathData();
      BActionState               advancePathData();
      bool                       attemptVectorPathToNextUserWaypoint(BEntityIDArray& ignoreUnits);
      inline BFindPathResult     findPath(BVector targetPosition, BPathLevel pathLevel, const BEntityIDArray& ignoreUnits);
      BActionState               findPath();
      inline float               calcDistanceRemaining(BUnit* pUnit, BEntity* pTarget, BVector targetPosition);
      bool                       calcTurning(float elapsedTime, float distanceChange, float distanceRemainingToTarget, BVector& newPosition, BVector& newForward);
      BActionState               calcMovement(float elapsedTime, BVector& newPosition, BVector& newForward, BVector& unlimitedNewForward);
      BActionState               calcMovementGaggle(float elapsedTime, BVector& newPosition, BVector& newForward, BVector& unlimitedNewForward);
      virtual BActionState       followPath(float elapsedTime, bool moving);
      virtual BActionState       followPathGaggle(float elapsedTime);
      void                       buildIgnoreList(BEntityIDArray& ignoreUnits, bool addTarget);
      void                       advanceToNewPosition(float elapsedTime, BVector newPosition, BVector newForward);
      bool                       determineTargetPosition(BVector& position, float& timeNeeded);
      bool                       determineTargetPath(BDynamicVectorArray& positions, float& timeNeeded, float& distanceNeeded, int& projectedTargetWaypoint);

      bool                       determineTargetPosition2(float projectionTime, BVector& position, float &distance, float& timeNeeded);
      bool                       calcGaggle(float elapsedTime, BVector& targetPosition, float& velocity);
      bool                       limitTurn(float elapsedTime, BVector forward, BVector up, BVector& newForward, float correctionFactor);
      void                       getGaggleObstructions(BVector pos1, BVector pos2, float radius, BObstructionNodePtrArray& obstructions);

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      BSimTarget                 mTarget;
      BPathMoveData*             mPathMoveData;
      BUnitOppID                 mOppID;

      float                      mVelocity;
      float                      mVelocityRandomFactor;
      float                      mVelocityRandomFactorTimer;
      float                      mWaitTimer;

      #ifdef _MOVE4
      long                       mRetryAttempts_4;
      #endif   

      bool                       mFlagSquadMove:1;
      bool                       mRefreshPathData:1;
      bool                       mHasTurnAnimation:1;

      #ifdef RENDERGAGGLE      
      BDynamicSimVectorArray     mGaggleObstructions;
      BVector                    mGaggleDirections[4];
      #endif
};
