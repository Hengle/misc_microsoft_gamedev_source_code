//==============================================================================
// squadactionplayblockinganimation.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"


//==============================================================================
//==============================================================================
class BSquadActionPlayBlockingAnimation : public BAction
{
   public:
      BSquadActionPlayBlockingAnimation() { }
      virtual ~BSquadActionPlayBlockingAnimation() { }

      //IPoolable Methods
      //virtual void               onAcquire() { }
      //virtual void               onRelease() { }
      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);
      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
      virtual void               setTriggerNotificationData(BTriggerScriptID triggerScriptID, BTriggerVarID triggerVarID) { mTriggerModule.setNotificationData(triggerScriptID, triggerVarID); }

      void                       setAnimationState(long state, long type, bool applyInstantly, bool useSquadMatrix, bool useMaxHeight, bool loop);
      void                       setSoundCue(BCueIndex cue) {mSoundCue = cue;}
      void                       setDisableMotionExtractionCollisions(bool value) { mFlagDisableMotionExtractionCollisions = value; }
      void                       setUpdateSquadPhysicsPosAtEnd(bool value) { mFlagUpdateSquadPhysicsPosAtEnd = value; }
      void                       setClearUnitNoRenderFlag(bool value) { mFlagClearUnitNoRenderFlag = value; }
      bool                       getFlagBirthAnim() const { return mFlagBirthAnim; }
      void                       setFlagBirthAnim(bool value) { mFlagBirthAnim = value; }

      long                       getAnimationState() const { return mAnimationState; }
      long                       getAnimationType() const { return mAnimationType; }

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

      DECLARE_FREELIST(BSquadActionPlayBlockingAnimation, 4);

   protected:

      void                       createUnitActions();
      void                       endAction();

      BActionTriggerModule       mTriggerModule;
      long                       mActiveChildActions;
      long                       mAnimationState;
      long                       mAnimationType;
      BCueIndex                  mSoundCue;
      bool                       mFlagApplyAnimationInstantly:1;
      bool                       mFlagUseSquadMatrix:1;
      bool                       mFlagUseMaxHeight:1;
      bool                       mFlagLoop:1;
      bool                       mFlagDisableMotionExtractionCollisions:1;
      bool                       mFlagUpdateSquadPhysicsPosAtEnd:1;
      bool                       mFlagEndActionCalled:1;
      bool                       mFlagClearUnitNoRenderFlag:1;
      bool                       mFlagBirthAnim:1;
};
