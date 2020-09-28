//============================================================================
// UIMPSetupScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UIPlayer.h"

//==============================================================================
// BUIPlayer::BUIPlayer
//==============================================================================
BUIPlayer::BUIPlayer():
mActive(true),
mSlot(-1),
mSlotType(cEmpty),
mLeaderStringIDIndex(-1),
mLeader(-1),
mCiv(-1),
mVoice(0),
mPing(0),
mTeam(-1),
mRank(-1),
mControllerPort(-1),
mID(-1),
mInCenter(false),
mHost(false),
mReady(false),
mArrowLeft(false),
mArrowRight(false)
{

}
