//==============================================================================
// squadactioncarpetbomb.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

// Constants
const float cDefaultAttackRunDistance = 20.0f; 
const uint cDefaultAttackNumber = 1;

//==============================================================================
//==============================================================================
class BSquadActionCarpetBomb : public BAction
{
   public:
      BSquadActionCarpetBomb() { }
      virtual ~BSquadActionCarpetBomb() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      void                       setCarpetBombLocation(BVector location) { mCarpetBombLocation = location; }
      BVector                    getCarpetBombLocation() { return(mCarpetBombLocation); }
      void                       setStartingLocation(BVector location) { mStartingLocation = location; }
      void                       setAttackRunDistance(float dist) { mAttackRunDistance = dist; }
      void                       setNumberAttacks(uint numAttacks) { mNumAttacks = numAttacks; }

      static bool                carpetBomb(BSimOrder* pSimOrder, BEntityID bomberSquadID, BVector targetLocation, BVector launchLocation, float attackRunDistance = cDefaultAttackRunDistance, uint numAttacks = cDefaultAttackNumber);

      DECLARE_FREELIST(BSquadActionCarpetBomb, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       setupAttacks();
      bool                       flyInFrom();
      bool                       bombsAway();
      bool                       flyOffTo();

      void                       removeChildActions();
      bool                       actionsComplete(BActionID id);      
      void                       setupChildOrder(BSimOrder* pOrder);

      BSmallDynamicSimArray<BVector> mAttackLocations;      
      BVector                        mStartingLocation;
      BVector                        mCarpetBombLocation;
      BEntityID                      mBomberSquad;
      BSimOrder*                     mpChildOrder;
      float                          mAttackRunDistance;      
      uint                           mNumAttacks;
      BActionID                      mChildActionID;
      BActionState                   mFutureState;
};