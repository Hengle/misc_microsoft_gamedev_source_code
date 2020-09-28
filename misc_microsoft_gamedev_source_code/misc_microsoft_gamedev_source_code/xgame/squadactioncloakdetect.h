//==============================================================================
// squadactiondetect.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"


//==============================================================================
//==============================================================================
class BSquadActionCloakDetect : public BAction
{
public:
   BSquadActionCloakDetect() { }
   virtual ~BSquadActionCloakDetect() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

   DECLARE_FREELIST(BSquadActionCloakDetect, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   
   void              detectionSearch();
   
   DWORD             mDetectionTimer;
};