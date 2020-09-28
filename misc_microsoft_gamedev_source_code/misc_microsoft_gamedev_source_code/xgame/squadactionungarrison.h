//==============================================================================
// squadactionungarrison.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnitOpp;

//==============================================================================
//==============================================================================
class BSquadActionUngarrison : public BAction
{
   public:

      BSquadActionUngarrison() {}
      virtual ~BSquadActionUngarrison() {}

      // Connect/Disconnect
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      // Init.
      virtual bool               init();

      // State
      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
      virtual void               setTarget(BSimTarget target);
      void                       setSource(const BSimTarget &target) { mSource = target; }
      const BSimTarget&          getSource() const { return mSource; }
      void                       useMaxHeight(bool useMaxHeight) { mFlagUseMaxHeight = useMaxHeight; }
      void                       setRallyPoint(BVector rallyPoint) { mRallyPoint = rallyPoint; }
      void                       setExitDirection(uint8 exitDirection) { mExitDirection = exitDirection; }
      void                       setFacing(BVector facing) { mFacing = facing; }
      void                       setSpawnPoint(BVector spawnPoint) { mSpawnPoint = spawnPoint; }
      void                       setIgnoreSpawnPoint(bool v) { mIgnoreSpawnPoint = v; }
      void                       setAllowMovingSquadsFromUngarrisonPoint(bool allow) { mFlagAllowMovingSquadsFromUngarrisonPoint = allow; }

      // ParentAction
      virtual BAction*           getParentAction() const { return (mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction = v; }

      virtual bool               isInterruptible() const { return (mInterruptable); }

      // Flags
      void                       setFlagAlertWhenComplete(bool v) { mFlagAlertWhenComplete = v; }

      DECLARE_FREELIST(BSquadActionUngarrison, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateTarget();
      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();
      void                       moveToRallyPoint();      
      BVector                    calculateUngarrisonPosition();
      BUnit*                     getTargetUnit();

      BSimTarget                 mTarget;
      BVector                    mRallyPoint;
      BVector                    mFacing;
      BVector                    mSpawnPoint;
      BUnitOppID                 mUnitOppID;      
      BAction*                   mpParentAction;
      DWORD                      mBlockedTimeStamp;
      uint                       mNumBlockedAttempts;
      uint8                      mUnitOppIDCount;
      uint8                      mExitDirection;
      BActionState               mFutureState;
      BSimTarget                 mSource;
      bool                       mFlagUseMaxHeight:1;
      bool                       mFlagAnyFailed:1;
      bool                       mFlagOwnTarget:1;
      bool                       mFlagAllowMovingSquadsFromUngarrisonPoint:1;
      bool                       mFlagBlocked:1;
      bool                       mFlagAlertWhenComplete:1;
      bool                       mIgnoreSpawnPoint:1;
      bool                       mInterruptable:1;
};
