//==============================================================================
// dopple.h
//
// The basic simulation participant.  These are essentially interactive objects.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once

// Includes
#include "object.h"
#include "obstructionmanager.h"
#include "gamefilemacros.h"

//==============================================================================
// BDopple
//==============================================================================
class BDopple : public BObject
{
   public:

      BDopple() {};
      virtual                 ~BDopple() { mActions.clearActions(); };

      virtual void            init(void);
      virtual void            destroy(void);
      virtual void            kill(bool bKillImmediately);
      bool                    initFromObject(const BObject*  pObject, BTeamID teamID, bool goIdle);

      void                    setForTeamID(long id) { mForTeamID=id; }
      long                    getForTeamID() const { return mForTeamID; }

      virtual bool            update(float elapsedTime);

      virtual long            computeObstructionType( void)                { return BObstructionManager::cObsTypeDoppleganger; }
//      virtual long            computeObstructionType( void)                { return BObstructionManager::cObsTypeCollidableNonMovableUnit; }

      GFDECLAREVERSION();
      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);
      virtual bool postLoad(int saveType);

   protected:

      virtual void            updateVisibleLists(void);

      // TOTALS:                          // 4 bytes
      long                    mForTeamID; // 4 bytes
};
