//==============================================================================
// unitactionjoin.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "cost.h"

class BUnit;
class BGrannyInstance;
class BBitArray;

//==============================================================================
//==============================================================================
class BUnitActionJoin : public BAction
{
   public:      
      BUnitActionJoin() { }
      virtual ~BUnitActionJoin() { }

      enum JOIN_TYPE
      {
         cJoinTypeFollow,
         cJoinTypeMerge,
         cJoinTypeBoard,
         cJoinTypeFollowAttack
      };

      enum MERGE_TYPE
      {
         cMergeTypeGround = 0x00000001,
         cMergeTypeAir    = 0x00000002,
      };

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);

      //Target.
      virtual const BSimTarget*  getTarget() const          { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      const BSimTarget*          getAttackTarget() const    { return &mAttackTarget; }
      
      virtual bool               isInterruptible() const    { return false; }

      void                       setAllowMultiple(bool v)   { mAllowMultiple = v; }
      bool                       getAllowMultiple()         { return mAllowMultiple; }

      static bool                swapDriverMeshRendering(BGrannyInstance* pGrannyInstance, const BBitArray& previousMask);

      DECLARE_FREELIST(BUnitActionJoin, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      void                       applyPostJoinEffects();
      void                       applyAIMissionBookkeeping(BSquad& rTargetSquad, BSquad& rHijackerSquad);
      void                       applyBuffs(const BProtoAction* pPA, bool unapply);
      void                       board();

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      void                       changeTarget(const BSimTarget& newTarget);

      BSimTarget                 mTarget;
      BSimTarget                 mAttackTarget;
      BSimTarget                 mOriginalTarget;
      BProtoSquadID              mOriginalProtoSquadID;
      BUnitOppID                 mOppID;
      BUnitOppID                 mChildOppID;
      BPlayerID                  mBoardTargetFormerOwnerID;
      float                      mAttackTargetUpdate;
      long                       mBoardStartAnimType;
      long                       mBoardEndAnimType;
      DWORD                      mBoardTimerEnd;
      bool                       mJoinComplete:1;
      bool                       mAllowMultiple:1;
      bool                       mTargetJoined:1;
};
