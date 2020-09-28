//==============================================================================
// unitactioninfantryenergyshield.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionInfantryEnergyShield : public BAction
{
   public:
      BUnitActionInfantryEnergyShield() { }
      virtual ~BUnitActionInfantryEnergyShield() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      DECLARE_FREELIST(BUnitActionInfantryEnergyShield, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       setupShieldAttachment();
      void                       shieldsHit();

      long                       mShieldAttachment;
      float                      mShieldTimer;
      bool                       mHit:1;
};