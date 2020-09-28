//==============================================================================
// unitactionspawnsquad.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionSpawnSquad : public BAction
{
   public:
      BUnitActionSpawnSquad() { }
      virtual ~BUnitActionSpawnSquad() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      DECLARE_FREELIST(BUnitActionSpawnSquad, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BSquad*                    completeSpawn();
      long                       getSquadTrainLimit(long id, uint8* pTrainLimitBucketOut) const;

      long                       mSquadType;
      float                      mCurrentPoints;
      float                      mTotalPoints;
      float                      mRandomWorkRateVariance;
      BEntityID                  mSpawnedUnit;
      long                       mCount;
};
