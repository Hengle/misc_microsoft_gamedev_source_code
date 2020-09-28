//============================================================================
//
//  renderTime.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "renderTime.h"
#include "renderThread.h"

//============================================================================
// Globals
//============================================================================
BRenderTime gRenderTime;

//============================================================================
// BRenderTime::BRenderTime
//============================================================================
BRenderTime::BRenderTime()
{
   mState.clear();
   mPrevState.clear();
}

//============================================================================
// BRenderTime::~BRenderTime
//============================================================================
BRenderTime::~BRenderTime()
{
}

//============================================================================
// BRenderTime::init
//============================================================================
void BRenderTime::init(void)
{
   mState.clear();
   mPrevState.clear();
}

//============================================================================
// BRenderTime::deinit
//============================================================================
void BRenderTime::deinit(void)
{
}

//============================================================================
// BRenderTime::simUpdate
//============================================================================
void BRenderTime::simUpdate(uint updateNumber, bool paused, double updateRealtime, double renderRealtime, double deltaTRealtime, double lastUpdateLength, double totalRealtime, double totalGametime)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   gRenderThread.submitDeferredDataCopy(&mPrevState, &mState, sizeof(mPrevState));
   
   BStateSnapshot* pSnapshot = static_cast<BStateSnapshot*>(gRenderThread.submitDataCopyBegin(&mState, sizeof(BStateSnapshot)));
   
   pSnapshot->mUpdateNumber            = updateNumber;
   pSnapshot->mPaused                  = paused;
   pSnapshot->mUpdateRealtime          = updateRealtime;
   pSnapshot->mRenderRealtime          = renderRealtime;
   pSnapshot->mDeltaTRealtime          = deltaTRealtime;
   pSnapshot->mLastUpdateLength        = lastUpdateLength;
   pSnapshot->mTotalRealtime           = totalRealtime;
   pSnapshot->mTotalGametime           = totalGametime;
   
   gRenderThread.submitDataCopyEnd(sizeof(BStateSnapshot));
}
