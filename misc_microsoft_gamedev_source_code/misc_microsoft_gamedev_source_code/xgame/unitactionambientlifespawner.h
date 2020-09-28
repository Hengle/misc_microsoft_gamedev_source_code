//==============================================================================
// unitactionambientlifespawner.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionAmbientLifeSpawner : public BAction
{
public:
   BUnitActionAmbientLifeSpawner() { }
   virtual ~BUnitActionAmbientLifeSpawner() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               update(float elapsed);

   virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

   DECLARE_FREELIST(BUnitActionAmbientLifeSpawner, 5);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   //BSquad*                    completeSpawn();

   void                       updateOpps();

   long                       mSquadType;
   BEntityID                  mSpawnedUnit;
   DWORD                      mOppTimer;
};
