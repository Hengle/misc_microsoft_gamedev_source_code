//==============================================================================
// unitactionhotdrop.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionHotDrop : public BAction
{
   public:
      BUnitActionHotDrop() { }
      virtual ~BUnitActionHotDrop() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   
      virtual bool               update(float elapsed);

      void                       setHotdropTarget(BEntityID target) { mHotdropTarget = target; }
      BUnit*                     getHotdropTarget();

      DECLARE_FREELIST(BUnitActionHotDrop, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       transportGarrisonedSquads();
      void                       calculateExitLocation(BVector& loc, BSquad* pSquad, bool& rValidExit);
      void                       completeTeleport(BSquad* pSquad);
      int                        findSquad(BEntityID squadID);
      void                       applyAIMissionBookkeeping(BSquad& rHotDroppedSquad);
      void                       removeSquad(int index);
      int                        addGarrisonedSquad(BSquad& squad);


      struct SquadInfo
      {
         BEntityID   mSquadID;
         BVector     mExitLoc;
         BEntityID   mHotdropBeam;
         BActionID   mUngarrisonID;
         float       mGroundLevel;
         float       mHeight;
         bool        mFlagGarrison:1;
         bool        mGoingUp:1;
         bool        mValid:1;
         bool        mReenableAvoidAir:1;
         bool        mValidExit:1;
      };

      typedef std::pair<BEntityID, BActionID> SquadUngarrisonPair;
      typedef BSmallDynamicSimArray<SquadInfo> SquadList;
      typedef BSmallDynamicSimArray<SquadUngarrisonPair> SquadToUngarrisonList;

      BEntityID                  mHotdropTarget;
      BEntityID                  mHotdropBeam;
      SquadList                  mCurrentSquads;
      SquadToUngarrisonList      mSquadToUngarrison;
};