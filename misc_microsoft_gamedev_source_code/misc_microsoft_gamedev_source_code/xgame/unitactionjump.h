//==============================================================================
// unitactionjump.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "parametricsplinecurve.h"

class BVisualItem;

//==============================================================================
//==============================================================================
class BUnitActionJump : public BAction
{
   public:
      BUnitActionJump() { }
      virtual ~BUnitActionJump() { }

      // Init / Connect / Disconnect.
      virtual bool               init();
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      // Update
      virtual bool               update(float elapsed);

      // Accessors
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v) { mOppID = v; }

      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target) { mTarget = target; }

      virtual BAction*           getParentAction() const { return(mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction = v; }

      void                       setThrowerProtoActionID(uint8 protoID) { mThrowerProtoActionID = protoID; }
      void                       setNoOrient() { mFlagOrient = false; }

      void                       setJumpType(BJumpOrderType v) { mJumpType = v; }
      BJumpOrderType             getJumpType()                 { return mJumpType; }

      // Pool of actions
      DECLARE_FREELIST(BUnitActionJump, 10);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();
      void                       removeParticles(BVisualItem* pAttach);

      BUnitOppID                 mOppID;
      BSimTarget                 mTarget;
      BAction*                   mpParentAction;
      float                      mXYDist;
      float                      mParam;
      long                       mThrowerProtoActionID;

      BParametricSplineCurve     mCurve;
      float                      mTargetHeight;

      BJumpOrderType             mJumpType;

      bool                       mFlagOrient:1;
};