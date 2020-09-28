//==============================================================================
// unitactiontowerwall.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionTowerWall : public BAction
{
   public:
      BUnitActionTowerWall() { }
      virtual ~BUnitActionTowerWall() { }
      virtual bool connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void disconnect();
      virtual bool init();
      virtual bool setState(BActionState state);
      virtual bool update(float elapsed);
      virtual void   notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void setTarget(BSimTarget target);

      DECLARE_FREELIST(BUnitActionTowerWall, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:      
      void updateGate();
      void closeGate();
      void openGate();
      void pushOffUnits();
      void createObstruction(BUnit* pSource, BUnit*pDest);
      void createBeam();
      void updateBeam(float elapsed);
      void killUnmovableSquads();
      void moveSquadsToPosition(const BSmallDynamicSimArray<BPlayerID>& movePlayers, const BEntityIDArray& squads, const BVector& position);

      BSimTarget mTarget;
      bool mGateOpen;
      BEntityID mObstructionUnitID;
      BEntityID mBeamProjectileID;
      double mWaitTimer;
      BVector mBeamStartPos;
      BVector mBeamEndPos;
};