//==============================================================================
// BObjectAnimationState.cpp
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once

#include "common.h"
#include "objectanimationstate.h"
#include "protovisual.h"
#include "savegame.h"

GFIMPLEMENTVERSION(BObjectAnimationState, 1);

//==============================================================================
//==============================================================================
void BObjectAnimationState::clear()
{ 
   mAnimType=cAnimationStateIdle;
   mForceAnimID=-1;
   mTweenToAnimation=-1;
   mTweenTime=0;
   mMoveSpeed=0.0f;
   mState=-1;
   mExitAction=-1;
   mFlagDirty=true;
   mFlagMoving=false;
   mFlagTurning=false;
   mFlagReset=false;
   mFlagApplyInstantly=false;
   mFlagLock=false;
   mFlagOverrideExitAction=false;
}

//==============================================================================
//==============================================================================
void BObjectAnimationState::setState(long newState, long animType, bool applyInstantly, bool reset, long forceAnimID, bool lock)
{
   if (newState != mState || animType != mAnimType || reset || forceAnimID != mForceAnimID || lock != mFlagLock || mFlagOverrideExitAction)
   {
      mState = (int8)newState;
      mAnimType = (int16)animType;
      mFlagApplyInstantly = applyInstantly;
      mFlagReset = reset;
      mForceAnimID = forceAnimID;
      mFlagLock = lock;
      mFlagOverrideExitAction = false;
      mFlagDirty = true;
   }
}

//==============================================================================
//==============================================================================
void BObjectAnimationState::overrideExitAction(const BProtoVisualAnimExitAction* pOverrideExitAction)
{
   if (pOverrideExitAction)
   {
      int8 exitAction = (int8)pOverrideExitAction->mExitAction;
      int16 tweenToAnimation = (int16)pOverrideExitAction->mTweenToAnimation;
      if (exitAction != mExitAction || pOverrideExitAction->mTweenTime != mTweenTime || tweenToAnimation != mTweenToAnimation)
      {
         mExitAction = exitAction;
         mTweenTime = pOverrideExitAction->mTweenTime;
         mTweenToAnimation = tweenToAnimation;
         mFlagOverrideExitAction = true;
         mFlagDirty = true;
      }
   }
   else
   {
      if (mFlagOverrideExitAction)
      {
         mFlagOverrideExitAction = false;
         mFlagDirty = true;
      }
   }
}

//==============================================================================
//==============================================================================
void BObjectAnimationState::getOverrideExitAction(BProtoVisualAnimExitAction& exitAction)
{ 
   exitAction.mExitAction=mExitAction; 
   exitAction.mTweenTime=mTweenTime; 
   exitAction.mTweenToAnimation=mTweenToAnimation; 
}

//==============================================================================
//==============================================================================
bool BObjectAnimationState::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int16, mAnimType);
   GFWRITEVAR(pStream, int16, mTweenToAnimation);
   GFWRITEVAR(pStream, float, mTweenTime);
   GFWRITEVAR(pStream, float, mMoveSpeed);
   GFWRITEVAR(pStream, int, mForceAnimID);
   GFWRITEVAR(pStream, int8, mState);
   GFWRITEVAR(pStream, int8, mExitAction);
   GFWRITEBITBOOL(pStream, mFlagDirty);
   GFWRITEBITBOOL(pStream, mFlagMoving);
   GFWRITEBITBOOL(pStream, mFlagTurning);
   GFWRITEBITBOOL(pStream, mFlagReset);
   GFWRITEBITBOOL(pStream, mFlagApplyInstantly);
   GFWRITEBITBOOL(pStream, mFlagLock);
   GFWRITEBITBOOL(pStream, mFlagOverrideExitAction);
   return true;
}

//==============================================================================
//==============================================================================
bool BObjectAnimationState::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int16, mAnimType);
   GFREADVAR(pStream, int16, mTweenToAnimation);
   GFREADVAR(pStream, float, mTweenTime);
   GFREADVAR(pStream, float, mMoveSpeed);
   GFREADVAR(pStream, int, mForceAnimID);
   GFREADVAR(pStream, int8, mState);
   GFREADVAR(pStream, int8, mExitAction);
   GFREADBITBOOL(pStream, mFlagDirty);
   GFREADBITBOOL(pStream, mFlagMoving);
   GFREADBITBOOL(pStream, mFlagTurning);
   GFREADBITBOOL(pStream, mFlagReset);
   GFREADBITBOOL(pStream, mFlagApplyInstantly);
   GFREADBITBOOL(pStream, mFlagLock);
   GFREADBITBOOL(pStream, mFlagOverrideExitAction);

   gSaveGame.remapAnimType(mAnimType);

   return true;
}
