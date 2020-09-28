//==============================================================================
// usermanager.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

#pragma once

#include "asynctaskmanager.h"

// Forward declarations
class BUser;
class BUserManager;

#include "timer.h"

// Global variable for the one BUserManager object
extern BUserManager gUserManager;

//==============================================================================
// BSelectDeviceAsyncTask - Select an XBox device
//==============================================================================
class BSelectDeviceAsyncTask : public BAsyncTask
{
   public:

      enum eDeviceSelectState
      {
         cStateIdle = 0,
         cStateSelectDefaultDevice,
         cStatePendingDeviceSelect,
         cStateVerifyDefaultDevice,
         cStatePendingDeviceVerification,
         cTotalStates
      };

      BSelectDeviceAsyncTask();
      //~BSelectDeviceAsyncTask();

      bool selectDevice(BUser* pUser, DWORD bytesRequested=0, bool forceShow=false);

      // overrides
      BOOL isComplete();
      bool update();
      void processResult();

      XCONTENTDEVICEID getDeviceID() const { return mDeviceID; }

   protected:

      void                 setDeviceSelectState(eDeviceSelectState state);

      BUser*               mpUser;
      DWORD                mBytesRequested;
      XCONTENTDEVICEID     mDeviceID;

      eDeviceSelectState   mDeviceSelectState;

      bool                 mForceShow;
};

class BUserNotificationListener
{
public:
   virtual void userStatusChanged() = 0;
};

//==============================================================================
// BUserManager
//==============================================================================
class BUserManager
{
   public:
      enum
      {
         cPrimaryUser=0,
         cSecondaryUser,
         cMaxUsers
      };

                              BUserManager();
                              ~BUserManager();

      bool                    setup();
      void                    reset();
      void                    update();
      bool                    handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      BUser*                  createUser(long type);
      void                    destroyUser(BUser* user);

      long                    getUserCount() const { return mUserList.getNumber(); }
      BUser*                  getUser(long type); // cPrimaryUser or cSecondaryUser
      BUser*                  getUserByPort(long port);
      BUser*                  getUserByPlayerID(long playerID);
      BUser*                  getPrimaryUser() { return getUser(BUserManager::cPrimaryUser); }
      BUser*                  getSecondaryUser() { return getUser(BUserManager::cSecondaryUser); }
      void                    dropSecondaryUser();
      // Secondary user available?
      BOOL                    isSecondaryUserAvailable(bool mustBeSignedIn);

      void                    setUserPort(long type, long port);

      bool                    isGuestXuid(XUID xuid);

      void                    notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      // Check users that are signed in
      DWORD                   getSignedInUserCount() { return mNumSignedInUsers; }
      DWORD                   getSignedInUserMask()  { return mSignedInUserMask; }
      BOOL                    isUserSignedIn( DWORD port ) { return ( mSignedInUserMask & ( 1 << port ) ) != 0; }
      bool                    userProfileChanged(BUser* pUser);

      BOOL                    areUsersSignedIn() { return ( mNumSignedInUsers >= 1 ); }

      // Get the first signed-in user
      //DWORD                   getSignedInUser() { return mFirstSignedInUser; }

      // Check users that are signed into live
      DWORD                   getOnlineUserMask() { return mOnlineUserMask; }
      BOOL                    isUserOnline( DWORD port ) {  return ( mOnlineUserMask & ( 1 << port ) ) != 0; }

      // Check the presence of system UI
      BOOL                    isSystemUIShowing() { return mSystemUIShowing || mNeedToShowSignInUI; }

      // Function to reinvoke signin UI
      void                    showSignInUI(DWORD flags=0) { mSignInFlags = flags; mNeedToShowSignInUI = TRUE; }

      bool                    showDeviceSelector(BUser* pUser, BAsyncNotify* pNotify=NULL, DWORD eventID=0, DWORD bytesRequested=0, bool forceShow=false);

      // Check privileges for a signed-in users
      BOOL                    checkPrivilege(DWORD port, XPRIVILEGE_TYPE priv);

      void                    addListener(BUserNotificationListener* listener);
      void                    removeListener(BUserNotificationListener* listener);

      void                    updateSigninByPort();
      void                    updateSigninByNotification(DWORD dwValid);

      void                    setShowingDeviceSelector(BOOL val) { mShowingDeviceSelector=val; }
      BOOL                    getShowingDeviceSelector() const { return mShowingDeviceSelector; }

   protected:
      bool                    signinStatusChanged(BUser* pUser);
      bool                    signinLiveStatusChanged(BUser* pUser);
      bool                    userSignedOut(BUser* pUser);


      void                    updateSigninStatus(BUser* pUser);
      void                    querySigninStatus();
      void                    queryControllers();
      void                    updateUserPorts();

      // let anybody that wants to know about a sign in change know.
      void                    notifyListeners();

      // Internal variables
      DWORD                   mSignedInUserMask;           // bitfields for signed-in users
      DWORD                   mFirstSignedInUser;          // first signed-in user
      DWORD                   mNumSignedInUsers;           // number of signed-in users
      DWORD                   mOnlineUserMask;             // users who are online

      DWORD                   mNumControllers;
      DWORD                   mControllerMask;
      DWORD                   mFirstController;

      DWORD                   mSignInFlags;

      BOOL                    mSystemUIShowing;            // system UI present
      BOOL                    mNeedToShowSignInUI;         // invoking signin UI necessary
      BOOL                    mSigninUIWasShown;           // was the signin ui shown at least once?
      BOOL                    mShowingDeviceSelector;

      BUser*                  mPrimaryUser;
      BUser*                  mSecondaryUser;
      BUser*                  mUsers[XUSER_MAX_COUNT];

      BUser*                  allocateUser();
      
      BTimer                  mUpdateTimer;

      BDynamicSimArray<BUser*>    mUserList;

      BDynamicSimArray<BUserNotificationListener*> mListeners;
};
