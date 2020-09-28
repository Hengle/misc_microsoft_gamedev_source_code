//==============================================================================
// unitactionpointblankattack.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionPointBlankAttack : public BAction
{
   public:
      BUnitActionPointBlankAttack() { }
      virtual ~BUnitActionPointBlankAttack() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      virtual bool               isInterruptible() const { return false; }

      virtual bool               load(BStream* pStream, int saveType);
      virtual bool               save(BStream* pStream, int saveType) const;

      // Flare methods
      void                       attemptLaunchFlares(BProjectile* pInstigator);
      int                        getNumFlares() const { return mFlares.size(); }
      BEntityID                  getFlare();

      DECLARE_FREELIST(BUnitActionPointBlankAttack, 4);

   protected:
      void                       getLaunchPosition(const BUnit* pUnit, long attachmentHandle, long boneHandle, BVector* position, BVector* forward, BVector* right) const;

      BEntityIDArray             mFlares;
      DWORD                      mFlareCooldownDoneTime;
      BUnitOppID                 mFlareAnimOppID;
      BEntityID                  mProjectileInstigator;
};
