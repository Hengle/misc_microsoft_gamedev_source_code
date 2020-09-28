//==============================================================================
// unitactionDOT.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "unit.h"
#include "containers/hashmap.h"

class BObject;
//==============================================================================
//==============================================================================
class BUnitActionDOT : public BAction
{
   public:
      BUnitActionDOT() { }
      virtual ~BUnitActionDOT() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      bool                       hasSource(BEntityID id);
      void                       setDamageMultiplier(float mult) { mDamageMultiplier = mult; }
      float                      getDamageMultiplier() { return (mDamageMultiplier); }
      void                       setDOTrate(float rate) { mDOTrate = rate; }
      float                      getDOTrate() { return (mDOTrate); }
      void                       setDOTduration(float duration);
      float                      getDOTduration() { return (mDOTduration); }
      void                       setDOTeffect(long dotEffect) { mDOTEffect = dotEffect; }
      void                       setEffectLocation(const BVector& location) { mAttachLocation = location; }

      void                       addStack(BEntityID source, BTeamID sourceTeam, DWORD endTime);
      void                       refreshStack(BEntityID source, BTeamID sourceTeam, DWORD endTime);

      bool                       operator==(const BUnitActionDOT& rhs);
      bool                       operator==(const BDamage& rhs);

      DECLARE_FREELIST(BUnitActionDOT, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      struct DotInfo
      {
         BEntityID               mSource;
         BTeamID                 mSourceTeamID;
         DWORD                   mEndTime;
      };

      typedef BSmallDynamicSimArray<DotInfo> DotCtnr;

      DotCtnr                    mDotStacks;

      BVector                    mAttachLocation;
      long                       mDOTEffect;
      BEntityID                  mAttachID;
      float                      mDamageMultiplier;
      float                      mDOTrate;
      float                      mDOTduration;
};