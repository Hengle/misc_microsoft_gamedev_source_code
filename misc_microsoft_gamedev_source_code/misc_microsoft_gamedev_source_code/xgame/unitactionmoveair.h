//==============================================================================
// unitactionmoveair.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "unitactionmove.h"



//==============================================================================
//==============================================================================
class BUnitActionMoveAir : public BUnitActionMove
{
   public:

      typedef enum
      {
         cTacticNav=0,
         cTacticNavBackToSquad,
         cTacticStrafe,
         cTacticExtend,
         cTacticLaunchHover,
         cTacticDip,
         cTacticClimb,
         cTacticKamikazeDive
      } BTacticState;

      typedef enum
      {
         cManeuverDefault=0,
         cManeuverRoll,
         cManeuverImmelman
      } BManeuverState;

      typedef enum
      {
         cDirLeft=0,
         cDirRight
      } BDirection;

      BUnitActionMoveAir() { }
      virtual ~BUnitActionMoveAir() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsedTime);

      BVector                    getBaseLocation() { return (mBasePosition); }
      void                       landingSiteDestroyed() { mbCanLand = false; }
      void                       launch(bool val) { mbLaunch = val; }
      bool                       isReturningToBase() { return (mbReturnToBase); }
      void                       returnToBase() { mbReturnToBase = true; }
      
      DECLARE_FREELIST(BUnitActionMoveAir, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      virtual BActionState       followPath(float elapsedTime);
      BActionState               calcFlightMovement(float elapsedTime, BVector& newPosition, BVector& newForward);
      void                       advanceToNewPosition2(float elapsedTime, BVector newPosition, BVector newForward);
      void                       updateManeuvers(float elapsedTime, BVector& newPosition, BVector& newForward);
      void                       updateTactics();
      BEntityID                  initEffectsFromProtoObject(long protoObjectID, BVector pos);
      bool                       validateRange() const;


      BVector                    mGoalPosition;
      BVector                    mBasePosition;
      BVector                    mPadSpot;
      BVector                    mSpotForward;
      BEntityID                  mAirBase;
      float                      mTargetDist;
      float                      mTurnRate;
      float                      mRollRate;
      float                      mRollAngle;
      float                      mGoalAltitudeInc;
      float                      mCurrAltitudeInc;
      float                      mPrevAltChange;
      float                      mAltitudeSelectTimer;
      float                      mSpeedSelectTimer;
      float                      mGoalSpeed;
      float                      mRollDelayTimer;
      float                      mAttackRunDelayTimer;
      float                      mHoverTimer;

      BTacticState               mTacticState;
      BManeuverState             mManeuverState;
      BDirection                 mRollDir;
      bool                       mbInverted:1;
      bool                       mbUpright:1;
      bool                       mbDiveAttack:1;
      bool                       mbFirstAttackPass:1;
      bool                       mbCanLand:1;
      bool                       mbLaunch:1;
      bool                       mbReturnToBase:1;
};
