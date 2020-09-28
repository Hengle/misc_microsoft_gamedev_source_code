//==============================================================================
// unitactioninfect.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BInfectee
{
   public:
      bool operator == (const BInfectee& b) const
      {
         return (squadId == b.squadId);
      }

      typedef std::pair<BEntityID, BEntityID> UnitFxPair;

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BSmallDynamicSimArray<UnitFxPair> units;
      BEntityID squadId;
      float time;
      float combatValue;
};

//==============================================================================
//==============================================================================
class BUnitActionInfect : public BAction
{
   public:
      BUnitActionInfect() { }
      virtual ~BUnitActionInfect() { }
      virtual bool connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void disconnect();
      virtual bool init();
      virtual bool setState(BActionState state);
      virtual bool update(float elapsed);

      DECLARE_FREELIST(BUnitActionInfect, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      int mInfectedCount;

      typedef BSmallDynamicSimArray<BInfectee> InfecteeCtr;
      InfecteeCtr mInfectees;
      float mTimeUntilNextScan;
};