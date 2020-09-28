//==============================================================================
// customcommand.cpp
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "customcommand.h"
#include "gamefile.h"

GFIMPLEMENTVERSION(BCustomCommand, 1);

//==============================================================================
//==============================================================================
BCustomCommand::BCustomCommand() :
   mID(-1), 
   mUnitID(cInvalidObjectID), 
   mPosition(-1), 
   mCost(), 
   mLimit(0),
   mTimer(0.0f), 
   mPrimaryUserIconID(-1),
   mSecondaryUserIconID(-1),
   mNameStringIndex(-1),
   mInfoStringIndex(-1),
   mHelpStringIndex(-1),
   mQueuedCount(0),
   mFinishedCount(0),
   mFlagQueue(false),
   mFlagAllowMultiple(false),
   mFlagShowLimit(false),
   mFlagCloseMenu(false), 
   mFlagPersistent(false),
   mFlagUnavailable(false),
   mFlagAllowCancel(false)
{
}

//==============================================================================
//==============================================================================
bool BCustomCommand::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int, mID);
   GFWRITEVAR(pStream, BEntityID, mUnitID);
   GFWRITEVAR(pStream, int, mPosition);
   GFWRITECLASS(pStream, saveType, mCost);
   GFWRITEVAR(pStream, int, mLimit);
   GFWRITEVAR(pStream, float, mTimer);
   GFWRITEVAR(pStream, int, mPrimaryUserIconID);
   GFWRITEVAR(pStream, int, mSecondaryUserIconID);
   GFWRITEVAR(pStream, int, mNameStringIndex);
   GFWRITEVAR(pStream, int, mInfoStringIndex);
   GFWRITEVAR(pStream, int, mHelpStringIndex);
   GFWRITEVAR(pStream, int, mQueuedCount);
   GFWRITEVAR(pStream, int, mFinishedCount);
   GFWRITEBITBOOL(pStream, mFlagQueue);
   GFWRITEBITBOOL(pStream, mFlagAllowMultiple);
   GFWRITEBITBOOL(pStream, mFlagShowLimit);
   GFWRITEBITBOOL(pStream, mFlagCloseMenu);
   GFWRITEBITBOOL(pStream, mFlagPersistent);
   GFWRITEBITBOOL(pStream, mFlagUnavailable);
   GFWRITEBITBOOL(pStream, mFlagAllowCancel);
   return true;
}

//==============================================================================
//==============================================================================
bool BCustomCommand::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int, mID);
   GFREADVAR(pStream, BEntityID, mUnitID);
   GFREADVAR(pStream, int, mPosition);
   GFREADCLASS(pStream, saveType, mCost);
   GFREADVAR(pStream, int, mLimit);
   GFREADVAR(pStream, float, mTimer);
   GFREADVAR(pStream, int, mPrimaryUserIconID);
   GFREADVAR(pStream, int, mSecondaryUserIconID);
   GFREADVAR(pStream, int, mNameStringIndex);
   GFREADVAR(pStream, int, mInfoStringIndex);
   GFREADVAR(pStream, int, mHelpStringIndex);
   GFREADVAR(pStream, int, mQueuedCount);
   GFREADVAR(pStream, int, mFinishedCount);
   GFREADBITBOOL(pStream, mFlagQueue);
   GFREADBITBOOL(pStream, mFlagAllowMultiple);
   GFREADBITBOOL(pStream, mFlagShowLimit);
   GFREADBITBOOL(pStream, mFlagCloseMenu);
   GFREADBITBOOL(pStream, mFlagPersistent);
   GFREADBITBOOL(pStream, mFlagUnavailable);
   GFREADBITBOOL(pStream, mFlagAllowCancel);
   return true;
}
