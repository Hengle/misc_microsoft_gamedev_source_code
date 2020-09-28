//==============================================================================
// squadactionunhitch.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;
class BUnitOpp;

//==============================================================================
//==============================================================================
class BSquadActionUnhitch : public BAction
{
public:
   BSquadActionUnhitch() {}
   virtual ~BSquadActionUnhitch() {}

   // Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   // Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);
   virtual void               setTarget(BSimTarget target);

   virtual bool               isInterruptible() const { return (false); }

   DECLARE_FREELIST(BSquadActionUnhitch, 5);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   bool                       validateTarget();
   bool                       addOpp(BUnitOpp opp);
   void                       removeOpp();
   void                       setupChildOrder(BSimOrder* pOrder);
   void                       enableUserAbility();

   BSimTarget                 mTarget;
   BUnitOppID                 mUnitOppID;  
   uint8                      mUnitOppIDCount;
   BActionState               mFutureState;
   bool                       mFlagAnyFailed:1;      
};
