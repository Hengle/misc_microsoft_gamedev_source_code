//==============================================================================
// notification.cpp
//
// Copyright (c) Ensemble Studios, 2007
//==============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "notification.h"

// Globals
BNotification gNotification;

//==============================================================================
// 
//==============================================================================
BNotification::BNotification() :
   mID(0),
   mParam(0),
   mNotification(NULL),
   mSystemUIShowing(FALSE),
   mInputDeviceChangedNotification(FALSE)
{
}

//==============================================================================
// 
//==============================================================================
bool BNotification::setup()
{
   // do not call setup multiple times without first calling shutdown
   BASSERT(mNotification == NULL);
   if (mNotification != NULL)
      return false;

   // Register our notification listener
   // TODO - eventually add notification traps for XNOTIFY_XMP  ?
#ifdef XBOX               
   mNotification = XNotifyCreateListener( XNOTIFY_SYSTEM | XNOTIFY_LIVE | XN_LIVE_INVITE_ACCEPTED | XNOTIFY_CUSTOM );
#endif // XBOX
   if( mNotification == NULL || mNotification == INVALID_HANDLE_VALUE )
      return false;

   // Set the position for notification popups
   //XNotifyPositionUI( XNOTIFYUI_POS_TOPRIGHT );

   return true;
}

//==============================================================================
// 
//==============================================================================
void BNotification::shutdown()
{
   if (mNotification != NULL)
      CloseHandle(mNotification);

   mNotification = NULL;
}

//==============================================================================
// 
//==============================================================================
void BNotification::update()
{
   // if the other modes need to know if the blade is open or not, then they need 
   // to check isSystemUIShowing()
   mID = 0;
   mParam = 0;

   BASSERT(mNotification != NULL);

   if (mNotification == NULL)
      return;

#ifdef XBOX               
	if (XNotifyGetNext( mNotification, 0, &mID, &mParam ))
   {
      if (mID == XN_SYS_UI)
         mSystemUIShowing = static_cast<BOOL>(mParam);
	  
	  // If we have an input device changed notification (occurs when a controller is plugged in or unplugged)
	  // we set mInputDeviceChangedNotification to true so that the object that needs to respond to it
	  // can do so at the appropriate time.  (mID will be reset the next time XNotifyGetNext is called,
	  // so without this, the notification will be lost unless it is responded to within the next tick).
	  if (mID == XN_SYS_INPUTDEVICESCHANGED)
		 mInputDeviceChangedNotification = TRUE;
   }
#endif // XBOX

   //// Check for notifications from the system/live
   //DWORD dwNotificationID;
   //ULONG_PTR ulParam;

   //if( XNotifyGetNext( mNotification, 0, &dwNotificationID, &ulParam ) )
   //{
   //   switch( dwNotificationID )
   //   {

   //      //Sent when the system displays or hides UI over the top of the current title. 
   //      // Can be used to pause or resume the game when appropriate.
   //   case XN_SYS_UI:
   //      {
   //         mSystemScreenActive = false;
   //         if (ulParam)
   //            mSystemScreenActive = true;
   //         break;
   //      }

   //      //Sent upon any change of the user signin state. The accompanying parameter indicates which player 
   //      // indices are now valid and only those profiles that were selected during the current sign-in process.
   //   case XN_SYS_SIGNINCHANGED:
   //      {
   //         //DWORD previousUserCount = mNumSignedInUsers;

   //         updateSignedInUserCount();

   //         if (mNumSignedInUsers>0)
   //         {
   //            mConnectionState = cLiveConnectionStateReadyLoggedIn;
   //         }

   //         //TODO: Detect if they they just signed out and they are in a game state that needs to terminate (such as playing an MP game)

   //         //Do we need to show the login UI?
   //         if (( mStateLiveRequired ) &&
   //            ( mNumSignedInUsers < 1 )) 
   //         {		
   //            mConnectionState = cLiveConnectionStateReady;
   //            if (mInitialNotificationReceived)
   //            {
   //               //They have already been through the signin part and it failed/they declines
   //               //TODO find the command to kick them to the main menu no matter what they are doing...
   //               mStateLiveRequired = false;
   //               gModeManager.setMode(BModeManager::cModeMenu);
   //            }
   //            else
   //            {
   //               showSignInScreen();
   //            }
   //         }

   //         mInitialNotificationReceived = TRUE;
   //         break;
   //      }

   //      //Broadcast when the connection to the Xbox Live server changes state.
   //   case XN_LIVE_CONNECTIONCHANGED:
   //      {
   //         if ((mStateLiveRequired) && 
   //            (ulParam != XONLINE_S_LOGON_CONNECTION_ESTABLISHED ))
   //         {
   //            //We have lost our connection to live, kick out to main menu
   //            //TODO - nicer handling + message to the user + log actual error code 
   //            mConnectionState = cLiveConnectionStateReady;
   //            mStateLiveRequired = false;
   //            gModeManager.setMode(BModeManager::cModeMenu);
   //         }
   //         break;
   //      }

   //      //the Ethernet cable has been plugged in or unplugged
   //   case XN_LIVE_LINK_STATE_CHANGED:
   //      {
   //         if (ulParam)
   //         {
   //            //They unhooked the Ethernet cable
   //            mConnectionState = cLiveConnectionStateCableUnplugged;
   //            if (mStateLiveRequired)
   //            {
   //               //Kick em
   //               //TODO - nicer handling + punish them? if they are in a game + log
   //               mStateLiveRequired = false;
   //               gModeManager.setMode(BModeManager::cModeMenu);
   //            }
   //         }
   //         else
   //         {
   //            //They reconnected the cable
   //            mConnectionState = cLiveConnectionStateStartupWaiting;
   //         }
   //         break;
   //      }

   //      //Not trapped yet: 
   //      //case XN_SYS_STORAGEDEVICESCHANGED
   //      //case XN_SYS_PROFILESETTINGCHANGED
   //      //case XN_SYS_MUTELISTCHANGED
   //      //case XN_SYS_INPUTDEVICESCHANGED
   //      //case XN_LIVE_INVITE_ACCEPTED
   //      //case XN_LIVE_CONTENT_INSTALLED
   //      //case XN_LIVE_MEMBERSHIP_PURCHASED
   //      //case XN_LIVE_VOICECHAT_AWAY
   //      //case XN_LIVE_PRESENCE_CHANGED

   //   }
   //}
}

//==============================================================================
// 
//==============================================================================
BOOL BNotification::getInputDeviceChangedNotification()
{ 
	BOOL ret = mInputDeviceChangedNotification;
	mInputDeviceChangedNotification = FALSE;
	return ret; 
}