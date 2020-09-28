//==============================================================================
// squadactionjump.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BSquadActionJump : public BAction
{
   public:
      BSquadActionJump() { }
      virtual ~BSquadActionJump() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      virtual void               setTarget(BSimTarget target) { mTarget = target; }
      virtual const BSimTarget*  getTarget() const { return &mTarget; }

      virtual bool               isInterruptible() const;

      void                       setJumpType(BJumpOrderType v) { mJumpType = v; }
      BJumpOrderType             getJumpType()                 { return mJumpType; }

      void                       setThrowerProtoActionID(uint8 protoID) { mThrowerProtoActionID = protoID; }

      bool                       PlayJumpSound(bool startJump);

      DECLARE_FREELIST(BSquadActionJump, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BSimTarget                 mTarget;
      BUnitOppID                 mOppID;

      BEntityIDArray             mJumping;
      BJumpOrderType             mJumpType;
      long                       mThrowerProtoActionID;
};
