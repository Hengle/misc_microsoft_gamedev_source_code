//============================================================================
// UIMenuItemControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "usermanager.h"

class BUser; 

class BUIGamerTagControl : public BUIControl, public BUserNotificationListener
{
public:
   enum Events
   {
      eSomeGamerTagEvent = UIGamerTagControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIGamerTagControl );

   enum
   {
      cSpeakerOff=0,
      cSpeakerOn,
      cSpeakerSpeaking,
      cSpeakerMuted,
   };

public:

   BUIGamerTagControl( void );
   virtual ~BUIGamerTagControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual const BUString& getGamerTag() const { return mText; }
   virtual const int getPort() const { return mPort; }
   virtual const BString& getGamerPic() const { return mGamerPicURL; }

   virtual void populateFromUser(BUser* user);

   virtual void setTitleText(const BUString& text);
   virtual void setGamerTag(const BString& text);
   virtual void setGamerTag(const BUString& text);
   virtual void setPort(int port);
   virtual void setGamerPic(const BString& text);
   virtual void setCiv(const BString& civ);

   virtual void setRank(int rank);
   virtual void setHost(bool bHost);
   virtual void setPing(int ping);

   virtual void setStatus(int status);

   virtual long getPlayerID() const { return mPlayerID; }
   virtual void setPlayerID(long value) { mPlayerID=value; }

   virtual void setSpeakerState(int state);
   virtual int  getSpeakerState() { return mSpeakerState; }

   virtual void setPlayerColor(DWORD color);
   virtual const DWORD getPlayerColor() const { return mPlayerColor; }

   virtual void enablePlayerColor(bool enable);

   void listenForUserChanges(int userType);

   // BUserNotificationListener methods.
   virtual void userStatusChanged();

   XUID getXuid() const { return mXuid; }
   void setXuid(XUID xuid) { mXuid = xuid; };


protected:
   XUID     mXuid;

   long     mPlayerID;
   DWORD    mPlayerColor;

   BUString  mText;
   BUString mTitle;
   int      mPort;
   int      mUserType;
   int      mRank;
   bool     mHost;
   int      mPing;
   int      mSpeakerState;

   BString  mCivPicURL;
   BString  mGamerPicURL;
};