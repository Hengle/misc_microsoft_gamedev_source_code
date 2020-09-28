//==============================================================================
// unitactiongather.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "cost.h"

class BUnit;

//==============================================================================
//==============================================================================
class BUnitActionGather : public BAction
{
   public:      
      BUnitActionGather() { }
      virtual ~BUnitActionGather() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;
      
      DECLARE_FREELIST(BUnitActionGather, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      enum
      {
         cMaxGather=2,
      };

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();
      bool                       validateRange() const;

      void                       addResource(long resourceID, float amount, uint flags=0);
      bool                       isResourceActive(long resourceID) const;
      void                       handleFloatyText(BPlayerID playerID, long resourceID, float amount);

      BSimTarget                 mTarget;
      BUnitOppID                 mOppID;
      BUnitOppID                 mMoveOppID;
      float                      mElapsed[cMaxGather];
      float                      mResourceTotal;
      long                       mCurrResourceID;
      BActionState               mFutureState;
      int                        mNewMode;

      bool                       mFlagAuto:1;
      bool                       mFlagSetGathering:1;
      bool                       mbSwitchAnims:1;

};
