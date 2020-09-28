//==============================================================================
// unitactiondeflect.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionDeflect : public BAction
{
   public:
      BUnitActionDeflect();
      virtual ~BUnitActionDeflect() { }

      //Init.
      virtual bool               init();

      bool                       connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual bool               update(float elapsed);

      bool                       canDeflect() const;
      bool                       tryDeflect(BEntityID deflecteeID, BVector trajectory, float damage);

      void                       setDeflectAnimOppID(BUnitOppID v) { mDeflectAnimOppID = v; }
      void                       setDeflecteeID(BEntityID id) { mDeflecteeID = id; }
      BEntityID                  getDeflecteeID() const;
      DWORD                      getDeflectCooldownDoneTime() { return mDeflectCooldownDoneTime; }

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      bool                       deflectSmallArms() const;

      DECLARE_FREELIST(BUnitActionDeflect, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       raiseShield();
      void                       lowerShield();

      BUnitOppID                 mDeflectAnimOppID;
      BEntityID                  mDeflecteeID; // entity the unit is deflecting, NULL if not currently deflecting
      DWORD                      mDeflectCooldownDoneTime;

      float                      mDamageDone;
      DWORD                      mLastDamageTime;
      bool                       mDeflecting:1;
};