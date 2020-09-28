//==============================================================================
// squadactiondaze.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"


//==============================================================================
//==============================================================================
class BSquadActionDaze : public BAction
{
public:
   BSquadActionDaze() { }
   virtual ~BSquadActionDaze() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

   void                       setupDazeAttributes(float duration, float movementModifier, bool smartTarget);
   void                       resetDaze(float dazeDuration);

   DECLARE_FREELIST(BSquadActionDaze, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void                       daze();
   void                       undaze();
   
   DWORD                      mDazedDuration;
   BSmallDynamicSimArray<BEntityID> mFXAttachment;

   float                      mMovementModifier;

   bool                       mSmartTarget:1;
};