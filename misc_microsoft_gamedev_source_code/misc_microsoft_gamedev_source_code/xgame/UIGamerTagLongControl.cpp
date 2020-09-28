//============================================================================
// UIGamerTagLongControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIGamerTagLongControl.h"


//============================================================================
//============================================================================
BUIGamerTagLongControl::BUIGamerTagLongControl( void ):
mAIDifficulty(-1),
mStatus(-1),
mReady(false)
{
   mControlType.set("UIGamerTagLongControl");
   mLeaderPicURL.set("");
}

//============================================================================
//============================================================================
BUIGamerTagLongControl::~BUIGamerTagLongControl( void )
{
}

//============================================================================
//============================================================================
void BUIGamerTagLongControl::reset()
{
   mXuid=INVALID_XUID;
   mRank=-1;
   mHost=false;
   mPing=-1;
   mPort=-1;
   mPlayerID=-1;
   mUserType=-1;
  
   mText.empty();
   mTitle.empty();
   mCivPicURL.empty();
   mGamerPicURL.empty();

   mAIDifficulty=-1;
   mStatus=-1;
   mReady=false;
   mLeaderPicURL.set("");
}

//============================================================================
//============================================================================
void BUIGamerTagLongControl::clear()
{
   reset();
   invokeActionScript("clear");
}


//============================================================================
//============================================================================
bool BUIGamerTagLongControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent(command.getPtr());
   return true;
}

//============================================================================
//============================================================================
void BUIGamerTagLongControl::setLeaderPic(const BString& text)
{
   mLeaderPicURL = text;

   GFxValue value;
   value.SetString(mLeaderPicURL.getPtr());
   invokeActionScript( "setLeaderPic", &value, 1);
}



//============================================================================
//============================================================================
void BUIGamerTagLongControl::setAIDifficulty(int difficulty)
{
   if (mAIDifficulty==difficulty)
      return;

   mAIDifficulty =difficulty;

   GFxValue value;
   value.SetNumber(mAIDifficulty);
   invokeActionScript( "setAIDifficulty", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagLongControl::setStatus(int status)
{
   if (mStatus == status)
      return;

   mStatus = status;

   GFxValue value;
   value.SetNumber(mStatus);
   invokeActionScript( "setStatus", &value, 1);
}


//============================================================================
//============================================================================
void BUIGamerTagLongControl::setReady(bool ready)
{
   if (mReady == ready)
      return; 

   mReady = ready;

   GFxValue value;
   value.SetBoolean(mReady);
   invokeActionScript( "setReady", &value, 1);
}