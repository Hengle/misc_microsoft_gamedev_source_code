//==============================================================================
// squadactionattack.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"
#include "SimTypes.h"
#include "UnitOpportunity.h"

class BUnit;

//==============================================================================
//==============================================================================
class BSquadActionAttack : public BAction
{
   public:
      BSquadActionAttack() { }
      virtual ~BSquadActionAttack() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               pause();
      virtual bool               unpause();
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);
      virtual void               setHitZoneIndex(long index) { mHitZoneIndex=index; }
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      //ParentAction.
      virtual BAction*           getParentAction() const { return (mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction=v; }

      void                       incStrafing() { if (mNumChildrenStrafing < 255) mNumChildrenStrafing++; }
      void                       decStrafing() { if (mNumChildrenStrafing > 0) mNumChildrenStrafing--; }
      bool                       isStrafing() const { return (mNumChildrenStrafing > 0); }

      virtual bool               isInterruptible() const;
      bool                       getFlagAbilityExecuted() const { return mFlagAbilityExecuted; }
      bool                       getFlagAbilityCommand() const { return mFlagAbilityCommand; }

      bool                       validateRange(DWORD& lastValidationTime) const;

#if DPS_TRACKER
      void                       updateTargetDPS();      
      float                      getRealDPS() const { return mRealDPS; }
#endif

      DECLARE_FREELIST(BSquadActionAttack, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       calculateRange(const BSquad& squad, BSimTarget& target, float& minRange);
      bool                       validateTarget(bool& targetIsMobile);
      bool                       addOpps(BUnitOpp opp, bool trackOpps=true, BEntityIDArray* children=NULL);
      void                       removeOpps(bool removeActions);

      void                       setupChildOrder(BSimOrder* pOrder);

      bool                       checkTargetAlive(BEntity* pTarget);
      bool                       isTargetCrashingAirUnit(BEntity* pTarget);
      bool                       haveActiveChildAttacks();
      void                       restartAttackingTarget();


      BDynamicSimArray<uint>     mUnitOppIDs;

      BSimTarget                 mTarget;
      float                      mMinRange;
      long                       mHitZoneIndex;
      BAction*                   mpParentAction;      
      BSimOrder*                 mpChildOrder;
      BActionID                  mChildActionID;
      int                        mAutoMode;
      int                        mAbilityID;
      float                      mPreMovePositionX;
      float                      mPreMovePositionZ;
      BActionState               mUnpauseState;
      uint8                      mUnitOppIDCount;
      BActionState               mFutureState;       
      uint8                      mNumChildrenStrafing;
      uint8                      mNumMoveAttempts;

      DWORD                      mLastRangeValidationTime;

#if DPS_TRACKER
      //-- Squad DPS Tracking
      float                   mRealDPS;
#endif

      bool                       mFlagAnyFailed:1;
      bool                       mFlagAbilityCommand:1;
      bool                       mFlagAutoLock:1;
      bool                       mFlagAutoMode:1;
      bool                       mFlagAutoAttackMode:1;
      bool                       mFlagAutoExitMode:1;
      bool                       mFlagMoveAutoSquadMode:1;
      bool                       mFlagHitAndRun:1;
      bool                       mFlagDontInterruptAttack:1;
      bool                       mFlagAbilityExecuted:1;
      bool                       mFlagAnySucceeded:1;
};
