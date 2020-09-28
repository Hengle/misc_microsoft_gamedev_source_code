//==============================================================================
// customcommand.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

#include "gamefilemacros.h"
#include "cost.h"

//==============================================================================
// BCustomCommand
//==============================================================================
class BCustomCommand
{
   public:
      BCustomCommand();

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      int                        mID;
      BEntityID                  mUnitID;
      int                        mPosition;
      BCost                      mCost;
      int                        mLimit;
      float                      mTimer;
      int                        mPrimaryUserIconID;
      int                        mSecondaryUserIconID;
      int                        mNameStringIndex;
      int                        mInfoStringIndex;
      int                        mHelpStringIndex;
      int                        mQueuedCount;
      int                        mFinishedCount;
      bool                       mFlagQueue:1;
      bool                       mFlagAllowMultiple:1;
      bool                       mFlagShowLimit:1;
      bool                       mFlagCloseMenu:1;
      bool                       mFlagPersistent:1;
      bool                       mFlagUnavailable:1;
      bool                       mFlagAllowCancel:1;
};
