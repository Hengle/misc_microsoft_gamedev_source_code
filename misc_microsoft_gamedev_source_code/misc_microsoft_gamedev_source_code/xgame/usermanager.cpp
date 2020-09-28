//==============================================================================
// usermanager.cpp
//
// Copyright (c) Ensemble Studios, 2005-2007
//==============================================================================

// Includes
#include "common.h"
#include "usermanager.h"
#include "XeCR.h"
#include "game.h"
#include "modemanager.h"
#include "player.h"
#include "protoobject.h"
#include "prototech.h"
#include "user.h"
#include "modemenu.h"
#include "configsgame.h"
#include "LiveSystem.h"
#include "ModeCampaign2.h"
#include "GamerPicManager.h"
#include "UITicker.h"

// xrender
#include "renderThread.h"

// xsystem
#include "notification.h"

// xinput
#include "inputsystem.h"
#include "keyboard.h"

// Globals
BUserManager gUserManager;

//==============================================================================
// 
//==============================================================================
BSelectDeviceAsyncTask::BSelectDeviceAsyncTask() :
   BAsyncTask(),
   mpUser(NULL),
   mBytesRequested(0),
   mDeviceID(XCONTENTDEVICE_ANY),
   mDeviceSelectState(cStateIdle),
   mForceShow(false)
{
}

//==============================================================================
// BSelectDeviceAsyncTask - Select an XBox device
//==============================================================================
bool BSelectDeviceAsyncTask::selectDevice(BUser* pUser, DWORD bytesRequested /*=0*/, bool forceShow /*=false*/)
{
   if (pUser == NULL)
      return false;

   if (!pUser->isSignedIn())
      return false;

   mpUser = pUser;
   mBytesRequested = bytesRequested;
   mForceShow = forceShow;
   gUserManager.setShowingDeviceSelector(TRUE);

   setDeviceSelectState(cStateSelectDefaultDevice);

   return true;
};

//==============================================================================
// 
//==============================================================================
BOOL BSelectDeviceAsyncTask::isComplete()
{
   return (mState == cStateError || mState == cStateDone);
}

//==============================================================================
// 
//==============================================================================
bool BSelectDeviceAsyncTask::update()
{
   if (BAsyncTask::update())
      return true;

   switch (mDeviceSelectState)
   {
      case cStateSelectDefaultDevice:
         {
            // show the device selector UI

            // check the user's profile for a default device ID
            mDeviceID = mpUser->getDefaultDevice();

            if (mDeviceID != XCONTENTDEVICE_ANY && !mForceShow)
            {
               setDeviceSelectState(cStateVerifyDefaultDevice);
               break;
            }

            ULARGE_INTEGER iBytesRequested;// = {mBytesRequested};
            iBytesRequested.QuadPart = mBytesRequested;

            DWORD dwRet = XShowDeviceSelectorUI(mpUser->getPort(),         // User to receive input from
                                                XCONTENTTYPE_SAVEDGAME,    // List only save game devices
                                                (mForceShow ? XCONTENTFLAG_FORCE_SHOW_UI : XCONTENTFLAG_NONE),         // No special flags
                                                iBytesRequested,           // Size of the device data struct
                                                &mDeviceID,                // Return selected device information
                                                mpOverlapped);

            if (dwRet != ERROR_IO_PENDING)
            {
               setState(cStateError);
            }
            else
            {
               setDeviceSelectState(cStatePendingDeviceSelect);
            }
            break;
         }

      case cStatePendingDeviceSelect:
         {
            if (!XHasOverlappedIoCompleted(mpOverlapped))
               break;

            DWORD dwResult;
            DWORD dwRet = XGetOverlappedResult(mpOverlapped, &dwResult, FALSE);
            if (dwRet == ERROR_SUCCESS)
            {
               setDeviceSelectState(cStateVerifyDefaultDevice);
            }
            else if (dwRet == ERROR_FUNCTION_FAILED)
            {
               dwRet = XGetOverlappedExtendedError(mpOverlapped);
               if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
               {
                  mpUser->setDefaultDevice(0);

                  // pull up the device selector again
                  mDeviceSelectState = cStateSelectDefaultDevice;
               }
               else if (dwRet == ERROR_CANCELLED)
               {
                  mpUser->setDefaultDevice(0);

                  setState(cStateDone, cErrorCancelled);
               }
               else
               {
                  setState(cStateError, cErrorAPIFailure);
               }
            }
            else
            {
               setState(cStateError, cErrorAPIFailure);
            }
            break;
         }

      case cStateVerifyDefaultDevice:
         {
            // now we need to verify mDeviceID
            DWORD dwRet = XContentGetDeviceState(mDeviceID, mpOverlapped);
            if (dwRet != ERROR_IO_PENDING)
               setState(cStateError, cErrorAPIFailure);
            else
               setDeviceSelectState(cStatePendingDeviceVerification);
            break;
         }

      case cStatePendingDeviceVerification:
         {
            // waiting on the overlapped result from XContentGetDeviceState
            if (!XHasOverlappedIoCompleted(mpOverlapped))
               break;

            DWORD dwResult;
            DWORD dwRet = XGetOverlappedResult(mpOverlapped, &dwResult, FALSE);
            if (dwRet == ERROR_IO_INCOMPLETE)
            {
               setState(cStateError, cErrorAPIFailure);
            }
            else if (dwRet == ERROR_SUCCESS)
            {
               setState(cStateDone);
            }
            else if (dwRet == ERROR_FUNCTION_FAILED)
            {
               HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
               if (HRESULT_CODE(hr) == ERROR_DEVICE_NOT_CONNECTED)
               {
                  mpUser->setDefaultDevice(0);

                  // pull up the device selector again
                  mDeviceSelectState = cStateSelectDefaultDevice;
               }
               else
               {
                  setState(cStateError, cErrorAPIFailure);
               }
            }
            else
            {
               setState(cStateError, cErrorAPIFailure);
            }
            break;
         }
   }

   return true;
}

//==============================================================================
// BSelectDeviceAsyncTask - Select an XBox device
//==============================================================================
void BSelectDeviceAsyncTask::processResult()
{
   gUserManager.setShowingDeviceSelector(FALSE);

   if (mDeviceID!=XCONTENTDEVICE_ANY)
   {
      // set the device first before completing the async notifications
      mpUser->setDefaultDevice(mDeviceID);
   }

   BAsyncTask::processResult();
}

//==============================================================================
// 
//==============================================================================
void BSelectDeviceAsyncTask::setDeviceSelectState(eDeviceSelectState state)
{
   mDeviceSelectState = state;
   setRetries(10);
}

//==============================================================================
// BUserManager::BUserManager
//==============================================================================
BUserManager::BUserManager()
: mSignedInUserMask(0)
, mFirstSignedInUser((DWORD)-1)
, mNumSignedInUsers(0)
, mOnlineUserMask(0)
, mNumControllers(0)
, mControllerMask(0)
, mFirstController((DWORD)-1)
, mSignInFlags(0)
, mSystemUIShowing(FALSE)
, mNeedToShowSignInUI(FALSE)
, mSigninUIWasShown(FALSE)
, mShowingDeviceSelector(FALSE)
, mPrimaryUser(NULL)
, mSecondaryUser(NULL)
{
   XMemSet(mUsers, 0, sizeof(BUser*)*XUSER_MAX_COUNT);
}

//==============================================================================
// BUserManager::~BUserManager
//==============================================================================
BUserManager::~BUserManager()
{
   reset();
}

//==============================================================================
// BUserManager::setup
//==============================================================================
bool BUserManager::setup()
{
   BUser::initOptionMaps();

   // create the primary user
   if (createUser(cPrimaryUser) == NULL)
      return false;

   // create the secondary user
   if (createUser(cSecondaryUser) == NULL)
      return false;

   // there should exist primary and secondary users at this point
   BASSERT(mPrimaryUser);
   BASSERT(mSecondaryUser);

   // query the state of the connected controllers
   queryControllers();

   // query the state of the connected controllers and their associated profiles
   querySigninStatus();

   // The primary user is always active 
   //  No they are not, at game startup we don't have an active user - eric
   if( gConfig.isDefined( cConfigAutoScenarioLoad ) )
   {
      mPrimaryUser->setFlagUserActive(true);
   }

   return true;
}

//==============================================================================
// BUserManager::reset
//==============================================================================
void BUserManager::reset()
{
   mPrimaryUser = NULL;
   mSecondaryUser = NULL;

   XMemSet(mUsers, 0, sizeof(BUser*)*XUSER_MAX_COUNT);

   for(long i=0; i<mUserList.getNumber(); i++)
      delete mUserList[i];
   mUserList.clear();

   // hopefully this list is empty by now.
   mListeners.clear();     // we don't want to delete any listeners, that is done elsewhere
}

//==============================================================================
// BUserManager::signinStatusChanged
//==============================================================================
bool BUserManager::signinStatusChanged(BUser* pUser)
{
   if (!pUser)
      return false;

   // get the port of this user
   long port = pUser->getPort();

   // query the system for the status
   XUSER_SIGNIN_STATE signInState = XUserGetSigninState(port);

   // assume the user is signed in.
   BOOL signedIn = true;
   if (eXUserSigninState_NotSignedIn==signInState)
      signedIn = false;

   // return the comparison of user state
   return ( signedIn != pUser->isSignedIn() );
}

//==============================================================================
// BUserManager::userSignedOut
//==============================================================================
bool BUserManager::userSignedOut(BUser* pUser)
{
   if (!pUser)
      return false;

   // get the port of this user
   long port = pUser->getPort();

   // query the system for the status
   XUSER_SIGNIN_STATE signInState = XUserGetSigninState(port);

   // if you're going from a non-profile user (eXUserSigninState_NotSignedIn)
   // to a signed-in user, then we should consider that a sign-out as well

   // assume the user is signed in.
   BOOL signedIn = true;
   if (eXUserSigninState_NotSignedIn==signInState)
      signedIn = false;

   // they went from signed in to signed out.
   if ( (eXUserSigninState_NotSignedIn==signInState) && 
        ( signedIn != pUser->isSignedIn() ) )
   {
      return true;
   }
   // or they were playing without a profile and just now signed-in
   else if (signedIn && pUser->getSigninState() == eXUserSigninState_NotSignedIn)
      return true;

   return false;
}

//==============================================================================
// 
//==============================================================================
bool BUserManager::userProfileChanged(BUser* pUser)
{
   // if the user was signed into live and now isn't, 
   if (!pUser)
      return false;

   // get the port of this user
   long port = pUser->getPort();

   // get the xuid the user thinks it is
   XUID xuidOld = pUser->getXuid();

   // [9/26/2008 xemu] ok, we can't just get the XUID off the port, since that doesn't disambiguate from Live status changes
   // [9/26/2008 xemu] so check via the sign-in info instead 
   XUID onlineXUID;
   XUID offlineXUID;
   XUSER_SIGNIN_INFO onlineSigninInfo;
   XUSER_SIGNIN_INFO offlineSigninInfo;
   uint rc1, rc2;

   rc1 = XUserGetSigninInfo(port, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &onlineSigninInfo);
   rc2 = XUserGetSigninInfo(port, XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY, &offlineSigninInfo);
   if ((rc1 == ERROR_SUCCESS) && (rc2 == ERROR_SUCCESS))
   {
      onlineXUID = onlineSigninInfo.xuid;
      offlineXUID = offlineSigninInfo.xuid;
   }
   else
   {
      XUID xuid;
      rc1 = XUserGetXUID(port, &xuid);
      onlineXUID = xuid;
      offlineXUID = xuid;
   }

   // Handle the case where we have a no-profile user currently playing and someone signs-in on another controller
   //
   // The XUser* calls return either ERROR_NO_SUCH_USER or ERROR_NOT_LOGGED_IN depending on which call you make
   //
   // WARNING If a valid gamer profile just logged out then the previous calls to check for a change in their sign-in
   // status should handle things so this is a safe check for now.  The concern would be that the user profile DID
   // change from valid to invalid
   if ((rc1 == ERROR_NO_SUCH_USER) || (rc2 == ERROR_NO_SUCH_USER))
      return false;
   else if ((rc1 != ERROR_SUCCESS) && (rc2 != ERROR_SUCCESS))
      return true;   // if we fail to get the xuid, assume it has changed.

   // return the comparison
   // [9/26/2008 xemu] if we match to either the online xuid OR offline xuid, then our profile hasn't changed, only our Xbox Live(tm) status 
   if ((xuidOld == onlineXUID) || (xuidOld == offlineXUID))
      return(false);

   return(true);
}

//==============================================================================
// 
//==============================================================================
bool BUserManager::signinLiveStatusChanged(BUser* pUser)
{
   // if the user was signed into live and now isn't, 
   if (!pUser)
      return false;

   // get the port of this user
   long port = pUser->getPort();

   // query the system for the status
   XUSER_SIGNIN_STATE signInState = XUserGetSigninState(port);

   // assume the user is signed in.
   BOOL signedIntoLive = true;
   if (eXUserSigninState_SignedInToLive!=signInState)
      signedIntoLive=false;

   // return the comparison of user state
   return ( signedIntoLive != pUser->isSignedIntoLive() );
}

//==============================================================================
// BUserManager::update
//==============================================================================
void BUserManager::update()
{
   SCOPEDSAMPLE(UserManagerUpdate);

   mSystemUIShowing = gNotification.isSystemUIShowing();

   // determine if we need to show the signin ui
   switch (gNotification.getNotification())
   {
      case XN_SYS_PROFILESETTINGCHANGED:
         {
            BUser* pUser = getPrimaryUser();
            if (!pUser || !pUser->getFlagUserActive())
               break;

            DWORD port = 1<<pUser->getPort();      // change to a bitfield.

            if (gNotification.getParam() & port)
            {
               // the profile for the primary user has changed, read the gamer pic
               gGamerPicManager.readGamerPic(pUser->getXuid(), pUser->getPort(), TRUE);
            }
         }
         break;
      case XN_SYS_STORAGEDEVICESCHANGED:
         {
            BUser* pUser = getPrimaryUser();
            if (!pUser || !pUser->getFlagUserActive())
               break;

            XCONTENTDEVICEID deviceID = pUser->getDefaultDevice();
            if (deviceID != XCONTENTDEVICE_ANY)
            {
               XDEVICE_DATA deviceData;
               ZeroMemory(&deviceData, sizeof(XDEVICE_DATA));

               // Check to see that the device is currently usable.
               if (XContentGetDeviceData(deviceID, &deviceData) != ERROR_SUCCESS)
               {
                  BModeCampaign2* pCampaignMode = (BModeCampaign2*) gModeManager.getMode(BModeManager::cModeCampaign2);
                  if (pCampaignMode)
                     pCampaignMode->updateSaveGameData();

                  gUserManager.getPrimaryUser()->setDefaultDevice(XCONTENTDEVICE_ANY);
                  gUserManager.getPrimaryUser()->setDeviceRemoved(true);

                  if (gModeManager.inModeGame())
                  {
                     if (gUIManager)
                        gUIManager->refreshUI();
                  }

                  BUIGlobals* pUIGlobals = gGame.getUIGlobals();
                  BASSERT(pUIGlobals);
                  if (pUIGlobals)
                  {
                     bool alreadyShowingDeviceRemovedMessage = (pUIGlobals->isYorNBoxVisible() && pUIGlobals->getDeviceRemovedMessage());
                     if (!alreadyShowingDeviceRemovedMessage)
                        pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25988), BUIGlobals::cDialogButtonsOK, (DWORD)-1, sEmptyUString, sEmptyUString, false, true);
                  }
               }
            }
         }

         break;
      case XN_SYS_SIGNINCHANGED:
         if (!gConfig.isDefined(cConfigEnableAttractMode))
         {
            querySigninStatus();
            notifyListeners();
         }
         else
         {
            // Check the primary user
            BUser* pUser = getPrimaryUser();
            if (!pUser || !pUser->getFlagUserActive())
               break;

            updateSigninByPort();
            updateSigninByNotification(gNotification.getParam());

            // if the status changed and the user changed
            // or if the user profile changed (During loading screens this update loop is not ticked.  It is possible
            // that the player could sign out and quickly sign back in again with a different profile)
            if ( (signinStatusChanged(pUser) && userSignedOut(pUser)) || userProfileChanged(pUser) )
            {
               // get rid of all gamer pics from our cache
               gGamerPicManager.reset();

               // if sign in status has changed on the primary, remove the primary user, then return to the main menu.
               setUserPort(BUserManager::cPrimaryUser, -1);

               // remove the primary user
               pUser->setFlagUserActive(false);
               pUser->setDefaultDevice(XCONTENTDEVICE_ANY);

               BUITicker::clear();

               // Mark them as signed out - because they are
               // NOTE: Many systems check the isSignedIn() method, but not the getFlagUserActive to see if a user is good to go
               // Before this change those system may have been using a player who they should not have
               // Love eric
               pUser->updateSigninStatus();

               // go to the attract mode state of modeMenu.
               gModeManager.getModeMenu()->setNextState(BModeMenu::cStateAttractMode);
               if (gModeManager.getModeType() != BModeManager::cModeMenu)
                  gModeManager.setMode(BModeManager::cModeMenu);

               // notify the user that the active gamer profile status has changed
               BUIGlobals* pUIGlobals = gGame.getUIGlobals();
	            BASSERT(pUIGlobals);
               if (pUIGlobals)
                  pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25934), BUIGlobals::cDialogButtonsOK);

               notifyListeners();
               break;;
            }
            else if (signinLiveStatusChanged(pUser))
            {
               pUser->updateSigninStatus();
               // if signed into live status has changed - call listeners (notifyListeners)
               notifyListeners();
            }

            // if the user profile changed (based on the xuid) then update the user.
            if (userProfileChanged(pUser))
            {
               updateSigninStatus(pUser);
            }

            // Check the secondary user
            pUser = getSecondaryUser();
            if (!pUser || !pUser->getFlagUserActive())
               break;

            if (signinStatusChanged(pUser) )
            {
               // if the user has signed into the profile, then update the BUser object
               if (!pUser->isSignedIn())
               {
                  // The user wasn't signed in, now is, update the user status
                  updateSigninStatus(pUser);
                  notifyListeners();
                  break;;
               }
               else
               {
                  // The user was signed in, and now isn't, unhook the user from the system.
                  setUserPort(BUserManager::cSecondaryUser, -1);
                  pUser->setFlagUserActive(false);
                  notifyListeners();
                  break;
               }
            }
            else if (signinLiveStatusChanged(pUser))
            {
               pUser->updateSigninStatus();
               notifyListeners();
            }
         }
         break;

      case XN_SYS_INPUTDEVICESCHANGED:
         queryControllers();
         notifyListeners();
         break;
   }

   if (mNeedToShowSignInUI && !mSystemUIShowing)
   {
      mNeedToShowSignInUI = FALSE;
      if (XShowSigninUI(1, mSignInFlags) == ERROR_SUCCESS)
      {
         mSystemUIShowing = TRUE;
         mSigninUIWasShown = TRUE;
      }
   }

   float elapsedTime;
   if(mUpdateTimer.isStarted())
      elapsedTime=(float)mUpdateTimer.getElapsedSeconds();
   else
      elapsedTime = 0.0f;
   mUpdateTimer.start();
   
   for(long i=0; i<mUserList.getNumber(); i++)
   {
      if (mUserList[i]->getFlagUserActive())
         mUserList[i]->update(elapsedTime);
   }
}

//==============================================================================
// BUserManager::handleInput
//==============================================================================
bool BUserManager::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   BASSERT(port >= 0 && port < XUSER_MAX_COUNT);
   if (port < 0 || port >= XUSER_MAX_COUNT)
      return false;

   BUser * const user = mUsers[port];
   if (user && user->getFlagUserActive())
   {
      if (user->handleInput(port, event, controlType, detail))
         return true;
   }

   //for(long i=0; i<mUserList.getNumber(); i++)
   //{
   //   if(mUserList[i]->handleInput(port, event, controlType, detail))
   //      return true;
   //}
   return false;
}

//==============================================================================
// BUserManager::createUser
//==============================================================================
BUser* BUserManager::createUser(long type)
{
   BASSERT(type == cPrimaryUser || type == cSecondaryUser);
   if (type < 0 || type >= cMaxUsers)
      return NULL;

   BUser* user=allocateUser();
   if(!user)
      return NULL;

   // pass in -1 for the port, we'll fix up the ports later
   if(!user->setup(-1))
   {
      delete user;
      return NULL;
   }

   if(mUserList.add(user)==-1)
   {
      delete user;
      return NULL;
   }

   if (type == cPrimaryUser)
      mPrimaryUser = user;
   else
      mSecondaryUser = user;

   return user;
}

//==============================================================================
// BUserManager::allocateUser
//==============================================================================
BUser* BUserManager::allocateUser()
{
   return new BUser();
}

//==============================================================================
// BUserManager::getUser
//==============================================================================
BUser* BUserManager::getUser(long type)
{
   // XXX this is temporary
   if (type == BUserManager::cPrimaryUser)
      return mPrimaryUser;
   else if (type == BUserManager::cSecondaryUser)
      return mSecondaryUser;
   // place logic for primary/secondary user determination here
   // support secondary user
   return NULL;
}

//==============================================================================
void BUserManager::dropSecondaryUser()
{
   BUser* pUser = getSecondaryUser();
   if (!pUser)
   {
      BASSERT(0);
      return;
   }

   setUserPort(BUserManager::cSecondaryUser, -1);
   pUser->setFlagUserActive(false);

}

//==============================================================================
// BUserManager::getUserByPort
//==============================================================================
BUser* BUserManager::getUserByPort(long port)
{
   BASSERT(port >= 0 && port < XUSER_MAX_COUNT);
   if (port < 0 || port >= XUSER_MAX_COUNT)
      return NULL;

   return mUsers[port];

   //for(long i=0; i<mUserList.getNumber(); i++)
   //{
   //   BUser* user=mUserList[i];
   //   if(user->getPort()==port)
   //      return user;
   //}
   //return NULL;
}

//==============================================================================
// BUserManager::getUserByPlayerID
//==============================================================================
BUser* BUserManager::getUserByPlayerID(long playerID)
{
   for(long i=0; i<mUserList.getNumber(); i++)
   {
      BUser* user=mUserList[i];
      if(user->getFlagUserActive() && user->getPlayerID()==playerID)
         return user;
   }
   return NULL;
}

//==============================================================================
// BUserManager::setUserPort
//==============================================================================
void BUserManager::setUserPort(long type, long port)
{
   BASSERT(mPrimaryUser);
   if (type == BUserManager::cPrimaryUser)
      mPrimaryUser->setPort(port);
   else if (type == BUserManager::cSecondaryUser)
      mSecondaryUser->setPort(port);

   // then map the primary and secondary users to their ports
   updateUserPorts();
}

//==============================================================================
// 
//==============================================================================
bool BUserManager::isGuestXuid(XUID xuid)
{
   DWORD HighPart;
   HighPart = (DWORD)(xuid >> 48);
   return ((HighPart & 0x000F) == 0x9) && ((HighPart & 0x00C0) > 0);
}

//==============================================================================
// BUserManager::destroyUser
//==============================================================================
void BUserManager::destroyUser(BUser* user)
{
   if(!user)
      return;

   for (long i=0; i < XUSER_MAX_COUNT; ++i)
   {
      if (mUsers[i] == user)
      {
         mUsers[i] = NULL;
         break;
      }
   }

   if (mPrimaryUser == user)
      mPrimaryUser = NULL;
   else if (mSecondaryUser == user)
      mSecondaryUser = NULL;

   mUserList.remove(user, true);

   delete user;
}

//==============================================================================
// BUserManager::notify
//==============================================================================
void BUserManager::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   // need to see who's calling the notify() method and if we are able to distinguish
   // which user the data was destined for (primary or secondary?).
   if(mPrimaryUser)
      mPrimaryUser->notify(eventType, senderID, data, data2);
   if(mSecondaryUser && mSecondaryUser->getFlagUserActive())
      mSecondaryUser->notify(eventType, senderID, data, data2);
}

//==============================================================================
// BUserManager::checkPrivilege
//==============================================================================
BOOL BUserManager::checkPrivilege( DWORD port, XPRIVILEGE_TYPE priv )
{
   BOOL result;

   return ( XUserCheckPrivilege( port, priv, &result) == ERROR_SUCCESS ) && result;
}

//==============================================================================
// 
//==============================================================================
void BUserManager::updateSigninStatus(BUser* pUser)
{
   if (pUser == NULL)
      return;

   //Update that primary user's sign-in state
   pUser->updateSigninStatus();

   // if we have a primary user, then we need to see if that user either
   // a) signed-out
   // b) signed-in using a different controller
   // I'm not sure if we're guaranteed to receive a sign-out notification
   // before receiving a sign-in notification on a different controller
   // scenario:
   // Profile A sign-out controller 0
   // Profile A sign-in controller 1
   //
   // XXX may need to check against the secondary player and reassign if necessary
   if (pUser->isSignedIn())
   {
      XUID xuidOld = pUser->getXuid();

      // update our xuid in case our sign-in state has changed
      pUser->updateXuid();

      // initialize the achievements system here too. (also, we need to load up the achievement file at some point)

      // verify that the primary user is still signed-in
      // verify that the port in question still belongs to mPrimaryUser
      XUID xuid;
      if (!isUserSignedIn(pUser->getPort()) ||
         XUserGetXUID(pUser->getPort(), &xuid) != ERROR_SUCCESS ||
         xuid != xuidOld)
      {
         // we failed to retrieve the Xuid for the primary user (how to handle that failure?)
         // or the xuid on the primary user's port no longer matches the primary user
         //
         // need to remove the primary user
         // right now this involves quitting the game they're currently in
         // do we need to respond if they're watching a record game or something?
         //
         // XXX not sure if this will cleanup everything properly in a multiplayer game
         // multiplayer modes should be prepared to handle a mode change and cleanup their stuff!

         // [9/26/2008 xemu] note we only want to do this in a Live game, as losing your Live signin should not boot you from 
         //   single player or system link games.
         if (pUser->getPlayer() && (gLiveSystem->getMPSession() != NULL) && !gLiveSystem->getMPSession()->isInLANMode())
         {
            pUser->getPlayer()->sendResign();
         }

         // This is redundant because ModeManager checks, but I want to be clear about what we are doing here.
         //
         // XXX the secondary user may still be participating in a game
         //if (gModeManager.getModeType() != BModeManager::cModeMenu)
         //{
         //   gModeManager.setMode(BModeManager::cModeMenu);
         //}

         // reset the port so it will flush the profile settings
         pUser->setPort(pUser->getPort());
      }
   }
   else if (mNumSignedInUsers > 0 && ((pUser == mPrimaryUser) || (pUser == mSecondaryUser && mNumSignedInUsers > 1)))
   {
      // the primary user is not signed in, let's attempt to do that now
      if (pUser == mPrimaryUser)
         pUser->setPort(mFirstSignedInUser);
      else
      {
         for (uint i=mFirstSignedInUser+1; i < XUSER_MAX_COUNT; ++i)
         {
            // find second signed-in user
            if (XUserGetSigninState(i) != eXUserSigninState_NotSignedIn)
            {
               pUser->setPort(i);
               break;
            }
         }
      }
   }
   else
   {
      // make sure we update the cache stuff (privilegs, xuid, etc)
      if (pUser == mPrimaryUser)
         pUser->setPort(mFirstController);
      else
      {
         XINPUT_STATE state;
         for (uint i=mFirstController+1; i < XUSER_MAX_COUNT; ++i)
         {
            if (XInputGetState(i, &state) == ERROR_SUCCESS)
            {
               pUser->setPort(i);
            }
         }
      }
   }
}


//==============================================================================
// BUserManager::querySigninStatus
//==============================================================================
void BUserManager::updateSigninByPort()
{
   // do not reset the primary user here, need to validate all the users first
   mSignedInUserMask = 0;
   mOnlineUserMask   = 0;

   // Count the signed-in users
   mNumSignedInUsers  = 0;
   mFirstSignedInUser = (DWORD)-1;

   // find out the status of the users at each port.
   for( UINT nUser = 0; nUser < XUSER_MAX_COUNT; nUser++ )
   {
      XUSER_SIGNIN_STATE State = XUserGetSigninState( nUser );

      if( State != eXUserSigninState_NotSignedIn )
      {
         // Check whether the user is online
         BOOL userOnline = (State == eXUserSigninState_SignedInToLive);

         mOnlineUserMask |= userOnline << nUser;

         mSignedInUserMask |= ( 1 << nUser );

         if( mFirstSignedInUser == (DWORD)-1 )
         {
            mFirstSignedInUser = nUser;
         }

         ++mNumSignedInUsers;
      }
   }

}

//==============================================================================
// 
//==============================================================================
void BUserManager::updateSigninByNotification(DWORD dwValid)
{
   // dwValid is a bitmask specifying which players are valid. The least significant bit represents user zero.
   for (uint i=0; i < XUSER_MAX_COUNT; ++i)
   {
      BUser* pUser = getUserByPort(i);
      if (pUser && (dwValid & (1 << i)))
         pUser->setLiveStateChanged();
   }
}

//==============================================================================
// BUserManager::querySigninStatus
//==============================================================================
void BUserManager::querySigninStatus()
{
   updateSigninByPort();

   // check to see if we need to invoke the signin UI
   mNeedToShowSignInUI = FALSE;

   updateSigninStatus(mPrimaryUser);
   updateSigninStatus(mSecondaryUser);

   // then map the primary and secondary users to their ports
   updateUserPorts();
}

//==============================================================================
// BUserManager::queryControllers
//==============================================================================
void BUserManager::queryControllers()
{
   mControllerMask = 0;
   mNumControllers = 0;
   mFirstController = (DWORD)-1;

   XINPUT_STATE state;
   for( UINT nUser = 0; nUser < XUSER_MAX_COUNT; nUser++ )
   {
      if (XInputGetState(nUser, &state) == ERROR_SUCCESS)
      {
         mControllerMask |= ( 1 << nUser );

         if (mFirstController == (DWORD)-1)
            mFirstController = nUser;

         ++mNumControllers;
      }
   }

   // if the user is not signed-in, then make sure the primary user still has access to a valid controller
   // if the user was signed in and they disconnected their controller, the xbox will prompt them to
   // re-connect it.  The other game systems will handle pausing, etc...
   //if (mPrimaryUser && !mPrimaryUser->isSignedIn())
   //{
   //   mPrimaryUser->setPort(mFirstController);

   //   updateUserPorts();
   //}
}

//==============================================================================
// BUserManager::updateUserPorts
//==============================================================================
void BUserManager::updateUserPorts()
{
   Utils::FastMemSet(mUsers, 0, sizeof(BUser*)*XUSER_MAX_COUNT);

   if (mPrimaryUser != NULL)
   {
      // if we have a primary user, then map the BUser object to the port in the usermanager list.
      long port = mPrimaryUser->getPort();
      if (port >= 0 && port < XUSER_MAX_COUNT)
      {
         mUsers[port] = mPrimaryUser;

         // also map the keyboard to the specific controller if it's connected.
         BKeyboard* pKeyboard = gInputSystem.getKeyboard();
         if (pKeyboard)
            pKeyboard->setPort(port);
      }
   }

   if (mSecondaryUser && mSecondaryUser->getFlagUserActive())
   {
      // if we have a secondary user, then map the BUser object to the port in the usermanager list.
      long port = mSecondaryUser->getPort();
      if (port >= 0 && port < XUSER_MAX_COUNT)
         mUsers[port] = mSecondaryUser;
   }
}

//==============================================================================
// BUserManager::showDeviceSelector
//==============================================================================
bool BUserManager::showDeviceSelector(BUser* pUser, BAsyncNotify* pNotify, DWORD eventID, DWORD bytesRequested, bool forceShow)
{
   if (pUser == NULL)
      return false;

   // XXX need a way to cancel this task
   // perhaps move to BUser?
   // the task will have access to the BUser pointer
   // which could be deleted before the task completes
   BSelectDeviceAsyncTask* pTask = new BSelectDeviceAsyncTask();
   bool result = pTask->selectDevice(pUser, bytesRequested, forceShow);
   if (!result)
   {
      delete pTask;
      return false;
   }

   pTask->setNotify(pNotify, eventID);

   gAsyncTaskManager.addTask(pTask);

   return true;
}

//==============================================================================
//==============================================================================
BOOL BUserManager::isSecondaryUserAvailable(bool mustBeSignedIn)
{
   if (mSecondaryUser == NULL)
      return FALSE;

   if (mSecondaryUser->getFlagUserActive())
   {
      if (mustBeSignedIn)
         return mSecondaryUser->isSignedIn();
      else
         return TRUE;
   }
   else
   {
      if (mustBeSignedIn)
      {
         if (mNumSignedInUsers > 1)
            return TRUE;
      }
      else
      {
         for (DWORD i=0; i<mNumControllers; i++)
         {
            if ((mControllerMask & ( 1 << i )) != 0 && (DWORD)mPrimaryUser->getPort() != i)
               return TRUE;
         }
      }
   }
   return FALSE;
}

//==============================================================================
//==============================================================================
void BUserManager::addListener(BUserNotificationListener* listener)
{
   // we don't want the listener added twice.
   mListeners.uniqueAdd(listener);
}

//==============================================================================
//==============================================================================
void BUserManager::removeListener(BUserNotificationListener* listener)
{
   mListeners.remove(listener);
}

//==============================================================================
//==============================================================================
void BUserManager::notifyListeners()
{
   for(long i=0; i<mListeners.getNumber(); i++)
   {
      BUserNotificationListener* pListener = mListeners[i];
      if (pListener)
         pListener->userStatusChanged();
   }
}
