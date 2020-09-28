//==============================================================================
// unitactionrevealtoteam.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "maximumsupportedplayers.h"

//==============================================================================
//==============================================================================
class BUnitActionRevealToTeam : public BAction
{
public:
   BUnitActionRevealToTeam() {}
   virtual ~BUnitActionRevealToTeam() {}

   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   virtual bool               update(float elapsed);

   void                       addReveal(BTeamID teamID);
   void                       removeReveal(BTeamID teamID);

   DECLARE_FREELIST(BUnitActionRevealToTeam, 5);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void              syncWithUnit();

   float             mRevealRadius;
   long              mSimRevealRadius;
   long              mRevealX;
   long              mRevealZ;
   long              mTotalRevealCount;
   long              mRevealCount[cMaximumSupportedTeams];
};