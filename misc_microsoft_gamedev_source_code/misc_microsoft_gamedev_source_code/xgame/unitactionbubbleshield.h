//==============================================================================
// unitactionbubbleshield.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "cost.h"

class BUnit;

//==============================================================================
//==============================================================================
class BUnitActionBubbleShield : public BAction, IEventListener
{
   public:      
      BUnitActionBubbleShield() { }
      virtual ~BUnitActionBubbleShield() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
      
      virtual int                getEventListenerType() const { return cEventListenerTypeAction; }
      virtual bool               savePtr(BStream* pStream) const;

      DECLARE_FREELIST(BUnitActionBubbleShield, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       createShield();
      void                       createBeam();

      BSimTarget                 mTarget;
      BEntityID                  mShieldID;
      BEntityID                  mBeamID;
      BEntityID                  mBeamHeadID;
      BEntityID                  mBeamTailID;
      DWORD                      mLastDamageTime;
      bool                       mRender:1;
};
