//==============================================================================
// unitactionroar.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;

//==============================================================================
//==============================================================================
class BUnitActionRoar : public BAction
{
   public:
      BUnitActionRoar() { }
      virtual ~BUnitActionRoar() { }

      //Init.
      virtual bool               init();
      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      virtual bool               update(float elapsed);
      void                       roar();

      DECLARE_FREELIST(BUnitActionRoar, 5);      

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:      
      void                       rollTimer();
      void                       scurryNearbyUnits();
      void                       scurrySquad(BSquad* pSquad);
      void                       cowerUnit(BUnit* pUnit);
      float                      mRoarTimer;
      BUnitOppID                 mRoarOppID;
};