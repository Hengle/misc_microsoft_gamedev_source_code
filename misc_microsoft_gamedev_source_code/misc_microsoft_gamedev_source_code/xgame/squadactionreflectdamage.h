//==============================================================================
// squadactionreflectdamage.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"


//==============================================================================
//==============================================================================
class BSquadActionReflectDamage : public BAction
{
public:
   BSquadActionReflectDamage() { }
   virtual ~BSquadActionReflectDamage() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

   DECLARE_FREELIST(BSquadActionReflectDamage, 4);

protected:
};