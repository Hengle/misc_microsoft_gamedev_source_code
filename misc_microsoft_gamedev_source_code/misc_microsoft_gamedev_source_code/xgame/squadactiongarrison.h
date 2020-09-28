//==============================================================================
// squadactiongarrison.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;
class BUnitOpp;

//==============================================================================
//==============================================================================
class BSquadActionGarrison : public BAction
{
   public:
      BSquadActionGarrison() {}
      virtual ~BSquadActionGarrison() {}

      // Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      // Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);
      virtual void               setTarget(BSimTarget target);

      // ParentAction
      virtual BAction*           getParentAction() const { return (mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction = v; }

      // [8-21-2008 CJS] Removed this because it was causing squads that were garrisoning to not respond to commands if the user
      // tried to move them away
      //virtual bool               isInterruptible() const { return ((mState == cStateMoving) ? true : false); }

      void                       setIgnoreRange(bool v) { mIgnoreRange = v; }
      void                       setReverseHotDropGarrison(bool v) { mReverseHotDropGarrison = v; }

      virtual void               setProtoAction(const BProtoAction* pAction) { mpProtoAction=pAction; }

      DECLARE_FREELIST(BSquadActionGarrison, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateTarget();
      bool                       validateRange();
      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();
      BUnit*                     getTargetUnit();
      float                      distanceToTarget();

      BSimTarget                 mTarget;
      BUnitOppID                 mUnitOppID;  
      BAction*                   mpParentAction;
      BActionID                  mChildActionID;
      uint8                      mUnitOppIDCount;
      BActionState               mFutureState;
      uint8                      mPathCount;
      bool                       mFlagAnyFailed:1;   
      bool                       mFlagAnySucceed:1;
      bool                       mFlagPlatoonMove:1;
      bool                       mFlagPlatoonHasMoved:1;
      bool                       mFlagOwnTarget:1;
      bool                       mIgnoreRange:1;
      bool                       mReverseHotDropGarrison:1;
};
