//==============================================================================
// platoonactionmove.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "platoon.h"

class BUnitOpp;

//#define DEBUGPROJECTIONCOLLISIONS

class BSquadActionEntry
{
public:
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);
   GFDECLAREVERSION();

   enum 
   {
      cActionEntryStateWorking,
      cActionEntryStateProceedingToPlot,
      cActionEntryStateDone
   };

   BEntityID   mSquad;
   BActionID   mActionID;
   bool        mRefreshed;
   int         mState;     // DLM 10/31/08 - (Happy Halloween!) Time to change these bools to a state variable.  
   DWORD       mLastCanPathUpdate; // DLM 11/3/08 - This is used to meter how often we check for canPaths..
};

//==============================================================================
//==============================================================================
class BPlatoonActionMove : public BAction
{
   public:
      BPlatoonActionMove() { }
      virtual ~BPlatoonActionMove() { }

      // Action Overrides
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      virtual bool               init();
      virtual bool               isInterruptible() const;
      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsedTime);      

      // Um, We need to actually implement target behaviour.. who knew?
      virtual const BSimTarget*  getTarget() const { return (const BSimTarget *)&mTarget; }
      virtual void               setTarget(BSimTarget target) { mTarget = target; if (mTarget.isPositionValid()) mOrigTargetPos = mTarget.getPosition(); }
      virtual void               setTargetID(BEntityID targetID) { mTarget.setID(targetID); if (mTarget.isPositionValid()) mOrigTargetPos = mTarget.getPosition(); }
      virtual void               setTargetPosition(BVector targetPosition) { mTarget.setPosition(targetPosition); if (mTarget.isPositionValid()) mOrigTargetPos = mTarget.getPosition(); }

      // DLM debug statement
      virtual void               debug(char *v, ...) const;

      // Pause Movement
      void                       pauseMovement(DWORD pauseTime);
      void                       getSquadActionEntry(const BEntityID squad, BSquadActionEntry &entry);
      void                       notifyThatSquadWillHandleItsOwnMovement(BEntityID squad);

      // ParentAction
      virtual BAction*           getParentAction() const { return (mpParentAction); }
      virtual void               setParentAction(BAction* v) { mpParentAction = v; }

#ifndef BUILD_FINAL
      virtual uint               getNumberDebugLines() const { return (3); }
      virtual void               getDebugLine(uint index, BSimString &string) const;
#endif
      DECLARE_FREELIST(BPlatoonActionMove, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      BActionState               doWorkUpdate(float elapsedTime);
      BActionState               advanceWP(bool bForce = false);
      void                       updateSquadActions();
      bool                       checkForChildrenComplete();
      float                      computeVelocityFactor() const;
      BActionState               incPathingAttempt();

      int32                      mLLPathingAttempts;
      BDynamicSimArray<BSquadActionEntry> mSquadActionList;
      BSimTarget                 mTarget;
      BActionState               mUnpauseState;
      DWORD                      mPauseTimeRemaining;
      BVector                    mOrigTargetPos;         // When we're attacking a target, this is where the target was originally located
      BAction*                   mpParentAction;

};
