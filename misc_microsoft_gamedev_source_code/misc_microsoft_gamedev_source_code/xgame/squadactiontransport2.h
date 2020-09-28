//==============================================================================
// squadactiontransport2.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BSquad;
class BArmy;
class BUnitOpp;

//==============================================================================
//==============================================================================
class BSquadActionTransport2 : public BAction
{
   public:
      BSquadActionTransport2() {}
      virtual ~BSquadActionTransport2() {}
 
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      virtual bool               init();
      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);
      virtual bool               isInterruptible() const { return false; }

      void                       setSquadList(BEntityIDArray squadList) { mSquadList = squadList; }
      void                       setPickupLocation(BVector location) { mPickupLocation = location; }
      void                       setDropOffLocation(BVector location) { mDropOffLocation = location; }
      void                       setFlyOffLocation(BVector location) { mFlyOffLocation = location; }

      bool                       getHoverOffset(float& offset) const;

      static bool                transportSquads(BSimOrder* pSimOrder, BEntityIDArray squadList, BVector dropOffLocation, BPlayerID playerID, BProtoObjectID transportProtoObjectID);

      DECLARE_FREELIST(BSquadActionTransport2, 5);

   protected:
      static uint                calculateNumTransports(BEntityIDArray squadList, BPlayerID playerID, BProtoObjectID transportProtoObjectID);

      bool                       positionSquads();
      bool                       moveTransport(BVector loc);
      bool                       updateMovementStuck(float elapsed);
      bool                       loadSquads();
      bool                       unloadSquads();
      void                       selfDestruct();

      void                       setupChildOrder(BSimOrder* pOrder);
      void                       removeChildActions();
      bool                       actionsComplete(BActionID id);

      bool                       addOpp(BUnitOpp opp);
      void                       removeOpp();

      BEntityIDArray              mSquadList;
      BSmallDynamicSimArray<BVector>   mSquadOffsets;

      BActionIDArray              mChildActionIDs;

      BVector                     mPickupLocation;
      BVector                     mDropOffLocation;
      BVector                     mFlyOffLocation;
      BVector                     mMovementPos;

      BSimOrder*                  mpChildOrder;
      BUnitOppID                  mUnitOppID;
      BActionState                mFutureState;
      float                       mMovementTimer;

      uint8                       mUnitOppIDCount;

      bool                        mFlagActionFailed;
      bool                        mFlagAnyFailed;
      bool                        mFlagUseSquadPosition;

};
