//==============================================================================
// unitactionmines.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "cost.h"

class BUnit;


//==============================================================================
//==============================================================================
class BUnitActionMines : public BAction
{
   public:      
      BUnitActionMines() { }
      virtual ~BUnitActionMines() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;
      

      DECLARE_FREELIST(BUnitActionMines, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();
      bool                       validateRange() const;

      bool                       placeMine();
      bool                       checkMineSpot(BSmallDynamicSimArray<BVector>& minePositions, float x, float z);

      BSimTarget                 mTarget;
      BUnitOppID                 mOppID;
      bool                       mPlacedAny:1;
};
