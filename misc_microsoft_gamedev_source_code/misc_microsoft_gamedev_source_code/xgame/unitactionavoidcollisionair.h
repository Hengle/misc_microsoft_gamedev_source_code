//==============================================================================
// unitactionavoidcollisionair.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"


//==============================================================================
//==============================================================================
class BUnitActionAvoidCollisionAir : public BAction
{
   public:

      BUnitActionAvoidCollisionAir() { }
      virtual ~BUnitActionAvoidCollisionAir() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      bool                       Avoiding() { return (mbAvoid); }
      bool                       Crashing() { return (mbCrashing); }
      BVector                    getAvoidanceVec() { return (mAvoidanceVec); }
      BVector                    getCrashPos() { return (mCrashPos); }
      BUnit*                     getKamikazeTarget();
      float                      getNearestObstacleAlt() { return (mNearestObstacleAlt); }
      bool                       findKamikazeTarget(BUnit* killer);
      void                       crashDetonate(BEntityID unitHit);
      void                       createImpactEffect(void);
      bool                       speedLimited() { return mbLimitSpeed; }
      void                       setCanKamikaze(bool newVal) { mbCanKamikaze = newVal; }

      DECLARE_FREELIST(BUnitActionAvoidCollisionAir, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:


      BVector     mAvoidanceVec;
      BVector     mPrevPos;
      BVector     mCrashPos;
      BEntityID   mKamikazeTarget;
      BEntityID   mKillerUnit;
      BPlayerID   mKillerPlayer;
      BTeamID     mKillerTeam;
      float       mNearestObstacleAlt;
      float       mMaxTgtDepressionAngle;
      float       mBirthTimer;
      uint        mClaimedAirSpotIndex;
      uint        mAnchorSearchPass;
      DWORD       mDetonateTime;
      bool        mbAvoid:1;
      bool        mbOffsetAnchorSearch:1;
      bool        mbCrashing:1;
      bool        mbLimitSpeed:1;
      bool        mbCanKamikaze:1;
};
