//==============================================================================
// unitactionmovewarthog.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once

#include "unitactionmove.h"


class BImpulseEntry
{
   public:
      BImpulseEntry() { reset(); }
      
      void                       setImpulse(BVector impulse, BVector impulseOffset) { mImpulse=impulse; mImpulseOffset=impulseOffset; }
      void                       setDuration(float duration) { mDuration=duration; mTimer=0.0f; }
      bool                       apply(BUnit* pUnit, float elapsedTime);
      void                       reset()
                                 {
                                    mImpulse=cOriginVector;
                                    mImpulseOffset=cOriginVector;
                                    mDuration=0.0f;
                                    mTimer=0.0f;
                                 }
      
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      BVector                    mImpulse;
      BVector                    mImpulseOffset;
      float                      mDuration;
      float                      mTimer;
};


//==============================================================================
// Unit move action for the physics warthog.  Instead of setting unit position
// and orientation, it sets a target position that is queried by the havok
// physics action to actually move the unit.
//==============================================================================
class BUnitActionMoveWarthog : public BUnitActionMove
{
   public:
      BUnitActionMoveWarthog () { }
      virtual ~BUnitActionMoveWarthog () { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsedTime);

      const BVector              getTargetPos() const { return mTargetPos; }
      bool                       isTargetPosTheGoal() const { return mTargetPosIsGoal; }
      void                       setInAir(bool inAir);
      bool                       isInAir() const { return mInAir; }

      const BPathMoveData*       getPathData() const { return mPathMoveData; }

      DECLARE_FREELIST(BUnitActionMoveWarthog, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      virtual BActionState       followPath(float elapsedTime);
      BActionState               calcMovement2(float elapsedTime, BVector& newPosition);
      void                       updateHitAndRun();
      void                       updateJumpPad();
      void                       updateImpulses(float elapsedTime);
      virtual void               updateSkidAnimState();

      BDynamicSimArray<BImpulseEntry> mImpulses;

      BVector                    mTargetPos;
      BVector                    mLastPosition;


      bool                       mLastPositionValid:1;
      bool                       mTargetPosIsGoal : 1;
      bool                       mHitAndRun:1;
      bool                       mSkidding:1;
      bool                       mInAir:1;
      bool                       mIgnoreTerrainCollisions:1;
      bool                       mPlayingInAirSound:1;
};
