//==============================================================================
// unitactionplayblockinganimation.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"
#include "protovisual.h"


//==============================================================================
//==============================================================================
class BUnitActionPlayBlockingAnimation : public BAction
{
   public:
      BUnitActionPlayBlockingAnimation() { }
      virtual ~BUnitActionPlayBlockingAnimation() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               setTriggerNotificationData(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID) { mTriggerModule.setNotificationData(triggerScriptID, triggerVarID); }
      void                       setAnimationState(long state, long type, bool applyInstantly, bool forceReset = false, long forceAnimID = -1);
      void                       setFlagTargetInvert(bool v) { mFlagTargetInvert=v; }
      void                       setFlagLoop(bool v) { mFlagLoop=v; }
      void                       setFlagAllowMove(bool v) { mFlagAllowMove=v; }
      void                       setFlagPreserveDPS(bool v) { mFlagPreserveDPS=v; }
      void                       setDisableMotionExtractionCollisions(bool value) { mFlagDisableMotionExtractionCollisions = value; }
      void                       setClearUnitNoRenderFlag(bool value) { mFlagClearUnitNoRenderFlag = value; }
      void                       overrideExitAction(BProtoVisualAnimExitAction* pOverrideExitAction);

      //Target.
      virtual const BSimTarget*  getTarget() const { return (&mTarget); }
      virtual void               setTarget(BSimTarget target);
      //OppID (and resulting things).
      virtual BUnitOppID         getOppID() const { return (mOppID); }
      virtual void               setOppID(BUnitOppID v);
      virtual uint               getPriority() const;

      bool                       getFlagHasPlayedAnimation() const { return (mFlagHasPlayedAnimation); }

      DECLARE_FREELIST(BUnitActionPlayBlockingAnimation, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       playAnimation();
      bool                       validateControllers() const;
      bool                       grabControllers();
      void                       releaseControllers();

      BProtoVisualAnimExitAction mOverrideExitAction;
      BActionTriggerModule       mTriggerModule;
      BSimTarget                 mTarget;
      BUnitOppID                 mOppID;
      DWORD                      mAnimationEndTime;
      long                       mAnimationState;
      long                       mAnimationType;
      long                       mForceAnimationID;      // Forcing an specific animation that is not added to the vis file.  this is used
                                                         // for cinematics since these animations are not added to the file.
      bool                       mFlagForceReset:1;
      bool                       mFlagApplyAnimationInstantly:1;
      bool                       mFlagTargetInvert:1;
      bool                       mFlagSaveIKDisabled:1;
      bool                       mFlagLoop:1;
      bool                       mFlagAllowMove:1;
      bool                       mFlagPreserveDPS:1;
      bool                       mFlagOverrideExitAction:1;
      bool                       mFlagHasPlayedAnimation:1;
      bool                       mFlagDisableMotionExtractionCollisions:1;
      bool                       mFlagClearUnitNoRenderFlag:1;
};
