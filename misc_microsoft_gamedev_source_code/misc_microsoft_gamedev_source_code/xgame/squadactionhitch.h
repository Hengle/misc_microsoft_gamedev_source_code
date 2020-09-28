//==============================================================================
// squadactionhitch.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;
class BUnitOpp;

//==============================================================================
//==============================================================================
class BSquadActionHitch : public BAction
{
   public:
      BSquadActionHitch() {}
      virtual ~BSquadActionHitch() {}

      // Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      // Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);
      virtual void               setTarget(BSimTarget target);

      virtual bool               isInterruptible() const { return ((mState == cStateMoving) ? true : false); }

      DECLARE_FREELIST(BSquadActionHitch, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateTarget();
      bool                       validateRange();
      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();
      void                       setupChildOrder(BSimOrder* pOrder);
      void                       disableUserAbility();
      void                       enableUserAbility();

      BSimTarget                 mTarget;
      BVector                    mHitchPos;
      BVector                    mHitchDir;
      BUnitOppID                 mUnitOppID;  
      BSimOrder*                 mpChildOrder;
      BActionID                  mChildActionID;
      uint8                      mUnitOppIDCount;
      BActionState               mFutureState;
      bool                       mFlagAnyFailed:1;      
      bool                       mFlagPlatoonMove:1;
      bool                       mFlagPlatoonHasMoved:1;
};
