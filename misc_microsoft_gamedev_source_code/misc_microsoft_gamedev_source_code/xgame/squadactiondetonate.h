//==============================================================================
// squadactiondetonate.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BSquadActionDetonate : public BAction
{
   public:
      BSquadActionDetonate() { }
      virtual ~BSquadActionDetonate() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      virtual void               setTarget(BSimTarget target);
      virtual const BSimTarget*  getTarget() const { return &mAttackTarget; }

      DECLARE_FREELIST(BSquadActionDetonate, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       removeChildActions();
      bool                       inGlowyRange();
      void                       createDetonateOpp(bool onDeath, BUnitOpp& opp);

      void                       startGlowy();
      void                       stopGlowy();

      typedef BSmallDynamicSimArray<BEntityID> GrenadeCtr;

      BSimTarget                 mAttackTarget;      
      BUnitOppID                 mDetonateOppID;
      BActionID                  mMoveID;
      GrenadeCtr                 mGrenades;
      bool                       mbGlowy:1;
};