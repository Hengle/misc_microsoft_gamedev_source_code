//============================================================================
// UIMenuItemControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIGamerTagControl.h"

#include "usermanager.h"
#include "user.h"
#include "userprofilemanager.h"

//============================================================================
//============================================================================
BUIGamerTagControl::BUIGamerTagControl( void ) : 
   mRank(-1),
   mHost(false),
   mPing(-1),
   mPort(-1),
   mPlayerID(-1),
   mXuid(INVALID_XUID),
   mSpeakerState(cSpeakerOff)
{
   mControlType.set("UIGamerTagControl");
   mText.set("");
}

//============================================================================
//============================================================================
BUIGamerTagControl::~BUIGamerTagControl( void )
{
   gUserManager.removeListener(this);
}

//============================================================================
//============================================================================
bool BUIGamerTagControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent(command.getPtr());
   return true;
}


//============================================================================
//============================================================================
void BUIGamerTagControl::setTitleText(const BUString& text)
{
   mTitle = text;

   GFxValue value;
   value.SetStringW(mTitle.getPtr());
   invokeActionScript( "setTitle", &value, 1);
}


//============================================================================
//============================================================================
void BUIGamerTagControl::setGamerTag(const BString& text)
{
   BUString temp;
   temp.locFormat(L"%S", text.getPtr());
   setGamerTag(temp);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setGamerTag(const BUString& text)
{
   mText = text;

   GFxValue value;
   value.SetStringW(mText.getPtr());
   invokeActionScript( "setText", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setPort(int port)
{
   mPort = port;

   GFxValue value;
   value.SetNumber(port);
   invokeActionScript( "setPort", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setCiv(const BString& civ)
{
   if( civ == "UNSC" )
   {
      mCivPicURL = "img://art\\ui\\flash\\shared\\textures\\civ\\CivPict_unsc.ddx";
   }
   else if( civ == "Covenant" )
   {
      mCivPicURL = "img://art\\ui\\flash\\shared\\textures\\civ\\CivPict_covenant.ddx";
   }

   

   GFxValue value;
   value.SetString(mCivPicURL.getPtr());
   invokeActionScript( "setCivPic", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setGamerPic(const BString& url)
{
   mGamerPicURL = url;

   GFxValue value;
   value.SetString(mGamerPicURL.getPtr());
   invokeActionScript( "setGamerPic", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::populateFromUser(BUser* user)
{
   if (!user)
      return;

   mPort = user->getPort();
   mGamerPicURL.format("img://gamerPic:%I64x", user->getXuid() );

   setGamerTag(user->getName());
   setPort(mPort);

   if (user->getProfile())
   {
      const BProfileRank& rank = user->getProfile()->getRank();
      setRank(rank.mRank);
   }

   if (user->isSignedIn())
      setGamerPic(mGamerPicURL);

}


//============================================================================
//============================================================================
void BUIGamerTagControl::listenForUserChanges(int userType)
{
   mUserType=userType;

   BUser* pUser = gUserManager.getUser(mUserType);
   if (pUser)
      populateFromUser(pUser);

   gUserManager.addListener(this);
}

// BUserNotificationListener methods.
//============================================================================
//============================================================================
void BUIGamerTagControl::userStatusChanged()
{
   // get my user type and update the control.
   BUser* pUser = gUserManager.getUser(mUserType);
   if (pUser)
      populateFromUser(pUser);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setRank(int rank)
{
   mRank = rank;

   GFxValue value;
   value.SetNumber(mRank);
   invokeActionScript( "setRank", &value, 1);
}


//============================================================================
//============================================================================
void BUIGamerTagControl::setHost(bool bHost)
{
   mHost = bHost;

   GFxValue value;
   value.SetNumber(mHost);
   invokeActionScript( "setHost", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setPing(int ping)
{
   mPing=ping;

   GFxValue value;
   value.SetNumber(mPing);
   invokeActionScript( "setPing", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setStatus(int status)
{
   GFxValue value;
   value.SetNumber(status);
   invokeActionScript( "setStatus", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setSpeakerState(int state)
{
   if (mSpeakerState == state)
      return;

   mSpeakerState = state;
   GFxValue value;
   value.SetNumber(mSpeakerState);
   invokeActionScript("setSpeakerState", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::setPlayerColor(DWORD color) 
{ 
   mPlayerColor = color;

   GFxValue value;
   value.SetNumber(mPlayerColor);
   invokeActionScript("setPlayerColor", &value, 1);
}

//============================================================================
//============================================================================
void BUIGamerTagControl::enablePlayerColor(bool enable)
{
   GFxValue value;
   value.SetBoolean(enable);
   invokeActionScript("enablePlayerColor", &value, 1);
}
