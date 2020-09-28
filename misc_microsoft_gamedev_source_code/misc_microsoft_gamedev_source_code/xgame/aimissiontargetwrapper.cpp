//==============================================================================
// aimissiontargetwrapper.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aimissiontargetwrapper.h"
#include "player.h"
#include "world.h"

GFIMPLEMENTVERSION(BAIMissionTargetWrapper, 2);

//==============================================================================
//==============================================================================
BAIMissionTargetWrapper::BAIMissionTargetWrapper()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
BAIMissionTargetWrapper::~BAIMissionTargetWrapper()
{

}


//==============================================================================
//==============================================================================
void BAIMissionTargetWrapper::resetNonIDData()
{
   mTargetID = cInvalidAIMissionTargetID;
   mMissionID = cInvalidAIMissionID;
   mLeashDist = 0.0f;
   mSecureDist = 0.0f;
   mRallyDist = 0.0f;
   mSearchWorkDist = 0.0f;
   mHotZoneDist = 0.0f;
   mMinRetreatRatio = 0.0f;
   mMinRalliedPercent = 0.0f;
   mMinSecureTime = 0;
   mTimestampTargetSecured = 0;
   mTimestampStopGathering = 0;
   mFlagAllowRetreat = false;
   mFlagAllowSkip = false;
   mFlagRequireSecure = false;
   mFlagGatherCrates = false;
   mFlagCurrentlySecured = false;
   mFlagCurrentlyGathering = false;
   //mFlagAutoGenTargets = false;
}


//==============================================================================
//==============================================================================
void BAIMissionTargetWrapper::setTargetID(BAIMissionTargetID v)
{
   // Nothing is changing.
   BAIMissionTargetID oldTargetID = mTargetID;
   BAIMissionTargetID newTargetID = v;
   if (oldTargetID == newTargetID)
      return;

   // Decrement the ref on the old target
   BAIMissionTarget* pOldTarget = gWorld->getAIMissionTarget(oldTargetID);
   if (pOldTarget)
   {
      pOldTarget->removeWrapperRef(mID);
      if (pOldTarget->getDestroyOnNoRefs() && pOldTarget->getNumWrapperRefs() == 0)
         gWorld->deleteAIMissionTarget(oldTargetID);
   }

   // Increment the ref on the new target
   BAIMissionTarget* pNewTarget = gWorld->getAIMissionTarget(newTargetID);
   if (pNewTarget)
   {
      pNewTarget->addWrapperRef(mID);
   }

   // Set to the new target.
   mTargetID = v;
}

//==============================================================================
// BAIMissionTargetWrapper::save
//==============================================================================
bool BAIMissionTargetWrapper::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BAIMissionTargetWrapperID, mID);
   GFWRITEVAR(pStream, BAIMissionTargetID, mTargetID);
   GFWRITEVAR(pStream, BAIMissionID, mMissionID);

   GFWRITEVAR(pStream, float, mLeashDist);
   GFWRITEVAR(pStream, float, mSecureDist);
   GFWRITEVAR(pStream, float, mRallyDist);
   GFWRITEVAR(pStream, float, mSearchWorkDist);
   GFWRITEVAR(pStream, float, mHotZoneDist);

   GFWRITEVAR(pStream, float, mMinRetreatRatio);
   GFWRITEVAR(pStream, float, mMinRalliedPercent);
   GFWRITEVAR(pStream, DWORD, mMinSecureTime);
   GFWRITEVAR(pStream, DWORD, mTimestampTargetSecured);
   GFWRITEVAR(pStream, DWORD, mTimestampStopGathering);

   GFWRITEBITBOOL(pStream, mFlagAllowRetreat);
   GFWRITEBITBOOL(pStream, mFlagAllowSkip);
   GFWRITEBITBOOL(pStream, mFlagRequireSecure);
   GFWRITEBITBOOL(pStream, mFlagGatherCrates);
   GFWRITEBITBOOL(pStream, mFlagCurrentlySecured);
   GFWRITEBITBOOL(pStream, mFlagCurrentlyGathering);
   return true;
}

//==============================================================================
// BAIMissionTargetWrapper::load
//==============================================================================
bool BAIMissionTargetWrapper::load(BStream* pStream, int saveType)
{  
   GFREADVAR(pStream, BAIMissionTargetWrapperID, mID);
   GFREADVAR(pStream, BAIMissionTargetID, mTargetID);
   GFREADVAR(pStream, BAIMissionID, mMissionID);

   GFREADVAR(pStream, float, mLeashDist);
   GFREADVAR(pStream, float, mSecureDist);
   GFREADVAR(pStream, float, mRallyDist);
   GFREADVAR(pStream, float, mSearchWorkDist);
   GFREADVAR(pStream, float, mHotZoneDist);

   GFREADVAR(pStream, float, mMinRetreatRatio);
   GFREADVAR(pStream, float, mMinRalliedPercent);
   GFREADVAR(pStream, DWORD, mMinSecureTime);
   GFREADVAR(pStream, DWORD, mTimestampTargetSecured);
   if (BAIMissionTargetWrapper::mGameFileVersion >= 2)
      GFREADVAR(pStream, DWORD, mTimestampStopGathering);

   GFREADBITBOOL(pStream, mFlagAllowRetreat);
   GFREADBITBOOL(pStream, mFlagAllowSkip);
   GFREADBITBOOL(pStream, mFlagRequireSecure);
   GFREADBITBOOL(pStream, mFlagGatherCrates);
   GFREADBITBOOL(pStream, mFlagCurrentlySecured);
   if (BAIMissionTargetWrapper::mGameFileVersion >= 2)
      GFREADBITBOOL(pStream, mFlagCurrentlyGathering);

   return true;
}

