//==============================================================================
// unitactionungarrison.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BUnitActionUngarrison : public BAction
{
   public:
      BUnitActionUngarrison() {}
      virtual ~BUnitActionUngarrison() {}
 
      // Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      // Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);

      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;

      // Source ID
      void                       setSourceID(BEntityID id) { mSourceID = id; }
      BEntityID                  getSourceID() const { return mSourceID; }

      // Exit Direction
      void                       setExitDirection(uint8 exitDirection) { mExitDirection = exitDirection; }
      // Spawn point override
      void                       setSpawnPoint(BVector spawnPoint) { mSpawnPoint = spawnPoint; }
      void                       setIgnoreSpawnPoint(bool v) { mIgnoreSpawn = v; }

      virtual bool               isInterruptible() const { return (false); }

      DECLARE_FREELIST(BUnitActionUngarrison, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();
      bool                       validateTarget() const;
      void                       playUngarrisonAnimation();
      bool                       playUncoverAnimation();
      void                       updateNoAnimTimer(float elapsedTime);
      int                        findAnimOffset();
      BVector                    calculateUngarrisonPosition();
      BVector                    calculateUngarrisonPositionFromSquadSpawnPoint();

      BSimTarget                 mTarget;
      BVector                    mSpawnPoint;
      BUnitOppID                 mOppID;
      float                      mNoAnimTimer;
      uint8                      mExitDirection;
      BSimTarget                 mSource;
      BEntityID                  mSourceID;
      BVector                    mOffset;
      bool                       mIgnoreSpawn:1;
};
