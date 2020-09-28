//==============================================================================
// BObjectAnimationState.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once

// Includes
#include "gamefilemacros.h"

// Forward declarations
struct BProtoVisualAnimExitAction;

//==============================================================================
// BObjectAnimationState
//==============================================================================
class BObjectAnimationState
{
public:
   enum
   {
      // Shared state
      cAnimationStateIdle,

      // Action states
      cAnimationStateRangedAttack,
      cAnimationStateHandAttack,
      cAnimationStatePostAnim,
      cAnimationStateDeath,
      cAnimationStateWork,
      cAnimationStateResearch,
      cAnimationStateTrain,
      cAnimationStateGarrison,
      cAnimationStateUngarrison,
      cAnimationStateMisc,
      cAnimationStateWarthogMove,

      cNumberAnimationStates
   };

   BObjectAnimationState() { clear(); mAnimType=-1; }

   long                    getState() const                         { return mState; }
   long                    getAnimType() const                      { return mAnimType; }
   long                    getForceAnim() const                     { return mForceAnimID; }
   float                   getMoveSpeed() const                     { return mMoveSpeed; }

   void                    setAnimType(long animType)               { if (animType!=mAnimType) { mAnimType=(int16)animType; mFlagDirty=true; } }
   void                    setState(long newState, long animType = -1, bool applyInstantly = false, bool reset = false, long forceAnimID = -1, bool lock = false);
   void                    setMoveSpeed(float v)                    { mMoveSpeed = v; }

   void                    overrideExitAction(const BProtoVisualAnimExitAction* pOverrideExitAction);

   void                    clear();

   bool                    isReset() const                          { return mFlagReset; }
   bool                    isApplyInstantly() const                 { return mFlagApplyInstantly; }
   bool                    isLocked() const                         { return mFlagLock; }
   bool                    isDirty() const                          { return mFlagDirty; }
   bool                    isMoving() const                         { return mFlagMoving; }
   bool                    isTurning() const                        { return mFlagTurning; }

   void                    clearReset()                             { if (mFlagReset) { mFlagReset=false; mFlagDirty=true; } }
   void                    setReset()                               { if (!mFlagReset) { mFlagReset=true; mFlagDirty=true; } }

   void                    clearApplyInstantly()                    { if (mFlagApplyInstantly) { mFlagApplyInstantly=false; mFlagDirty=true; } }
   void                    setApplyInstantly()                      { if (!mFlagApplyInstantly) { mFlagApplyInstantly=true; mFlagDirty=true; } }

   void                    clearLock()                              { if (mFlagLock) { mFlagLock=false; mFlagDirty=true; } }
   void                    setLock()                                { if (!mFlagLock) { mFlagLock=true; mFlagDirty=true; } }

   void                    clearDirty()                             { mFlagDirty=false; }
   void                    setDirty()                               { mFlagDirty=true; }

   void                    clearMoving()                            { mFlagMoving=false; mMoveSpeed=0.0f; }
   void                    setMoving()                              { mFlagMoving=true; }

   void                    clearTurning()                           { mFlagTurning=false; }
   void                    setTurning()                             { mFlagTurning=true; }

   bool                    isOverrideExitAction() const             { return mFlagOverrideExitAction; }
   void                    getOverrideExitAction(BProtoVisualAnimExitAction& exitAction);

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   int16       mAnimType;
   int16       mTweenToAnimation;
   float       mTweenTime;
   float       mMoveSpeed;
   int         mForceAnimID;
   int8        mState;
   int8        mExitAction;
   bool        mFlagDirty:1;
   bool        mFlagMoving:1;
   bool        mFlagTurning:1;
   bool        mFlagReset:1;
   bool        mFlagApplyInstantly:1;
   bool        mFlagLock:1;
   bool        mFlagOverrideExitAction:1;
};

// objectanimationstate.h