//==============================================================================
// liveSession.cpp
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

#include "Common.h"
#include "XLastGenerated.h"
#include "liveSession.h"
#include "commlog.h"
#include "commheaders.h"
#include "LiveSystem.h"
#include "user.h"
#include "usermanager.h"
#include "econfigenum.h"

#include "database.h"
#include "game.h"
#include "UIGlobals.h"

//==============================================================================
// Constants
//==============================================================================
const DWORD MODIFY_FLAGS_ALLOWED = XSESSION_CREATE_USES_ARBITRATION |
                                   XSESSION_CREATE_INVITES_DISABLED |
                                   XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED |
                                   XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;

//==============================================================================
//Constructor for starting a party session
//==============================================================================
BLiveSession::BLiveSession(int nOwnerController, UINT slots) :
   mShuttingDownStartTime(0),
   mDeinit(false),
   mSessionEnded(false),
   mStatsGameClass(cLiveSessionGameClassificationUnknown),
   mStatsCampaignMapIndex(0),
   mStatsGameTypeIndex(0),
   mStatsDifficulty(0),
   mStatsPartyTeam(false),
   mStatsOnlySession(false),
   mStatsSaveGameRule(false),
   mStopAcceptingAsyncTasks(false),
   mLocalUserJoined(false),
   mShutdownDialog(false)
{
   //Set properties and context for the session
   nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- Constructor for starting a party session, %d slots", slots);

   BLiveSessionTaskSessionSetContext* newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_MODE, CONTEXT_GAME_MODE_PARTYHOPPER);
   addAsyncTaskToQueue(newContextTask);
   newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_STANDARD);
   addAsyncTaskToQueue(newContextTask);

   mSession                  = INVALID_HANDLE_VALUE;
   mIsHost                   = TRUE;
   mSessionNonce             = 0;         
   mOwnerController          = nOwnerController;
   mSessionState             = cSessionStateNone;
   mCommandIssuedTime        = timeGetTime();
   mRanked                   = FALSE;
   mArbitrationRegisteredCount= 0;
   mpArbitrationResults      = NULL;
   mGameModeIndex            = CONTEXT_GAME_MODE_PARTYHOPPER;
   mMaxPublicSlots           = slots;
   mMaxPrivateSlots          = 0;
   mJoinable                 = true;
   mHostingMode              = cBLiveSessionHostingModeOpen;
   mResendSessionModifyOnNextDrop = false;
   Utils::FastMemSet( &mSessionKID, 0, sizeof( mSessionKID ) );
   Utils::FastMemSet( &mSessionKey, 0, sizeof( mSessionKey ) );
   Utils::FastMemSet( &mHostXNAddr, 0, sizeof( mHostXNAddr ) );

   mSessionFlags = XSESSION_CREATE_USES_PEER_NETWORK | XSESSION_CREATE_USES_PRESENCE | XSESSION_CREATE_HOST;

   BLiveSessionTaskSessionCreate* newCreateTask = new BLiveSessionTaskSessionCreate(mSessionFlags, mOwnerController, slots, 0);
   addAsyncTaskToQueue(newCreateTask);

   /*
   Utils::FastMemSet(&mOverlapped, 0, sizeof(XOVERLAPPED));

   DWORD ret = XSessionCreate(
      mSessionFlags,
      mOwnerController,
      mSlots[ cSessionSlotsTotalPublic ],
      mSlots[ cSessionSlotsTotalPrivate ],
      &mSessionNonce,
      &mSessionInfo,
      &mOverlapped,
      &mSession );

   if ( ret != ERROR_IO_PENDING )
   {
      //Create failed
      nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- XSessionCreate call failed with error 0x%08x", ret);
      mSessionState = cSessionStateError;
      return;
   }
   */

   mSessionState = cSessionStateWaitingForRegistration;
}

//==============================================================================
//Constructor for joining a party session
//==============================================================================
BLiveSession::BLiveSession(int nOwnerController, UINT slots, const XNADDR& hostTargetXNADDR, const XNKID& hostKID, const XNKEY& hostKEY) :
   mShuttingDownStartTime(0),
   mDeinit(false),
   mSessionEnded(false),
   mStatsGameClass(cLiveSessionGameClassificationUnknown),
   mStatsCampaignMapIndex(0),
   mStatsGameTypeIndex(0),
   mStatsDifficulty(0),
   mStatsPartyTeam(false),
   mStatsOnlySession(false),
   mStatsSaveGameRule(false),
   mStopAcceptingAsyncTasks(false),
   mLocalUserJoined(false),
   mShutdownDialog(false)
{
   //Set properties and context for the session
   nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- Constructor for joining a party session, %d slots", slots);

   BLiveSessionTaskSessionSetContext* newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController,  X_CONTEXT_GAME_MODE, CONTEXT_GAME_MODE_PARTYHOPPER);
   addAsyncTaskToQueue(newContextTask);
   newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_STANDARD);
   addAsyncTaskToQueue(newContextTask);

   mSession                  = INVALID_HANDLE_VALUE;
   //mSessionError             = NULL;
   mIsHost                   = FALSE;
   mSessionNonce             = 0;         
   mOwnerController          = nOwnerController;
   mSessionState             = cSessionStateNone;
   mCommandIssuedTime        = timeGetTime();
   mRanked                   = FALSE;
   mArbitrationRegisteredCount= 0;
   mpArbitrationResults      = NULL;
   mGameModeIndex            = CONTEXT_GAME_MODE_PARTYHOPPER;
   mMaxPublicSlots           = slots;
   mMaxPrivateSlots          = 0;
   mJoinable                 = true;
   mHostingMode              = cBLiveSessionHostingModeOpen;
   mResendSessionModifyOnNextDrop = false;
   Utils::FastMemSet( &mSessionKID, 0, sizeof( mSessionKID ) );
   Utils::FastMemSet( &mSessionKey, 0, sizeof( mSessionKey ) );
   Utils::FastMemSet( &mHostXNAddr, 0, sizeof( mHostXNAddr ) );

   /*
   mSessionInfo.hostAddress = hostTargetXNADDR;
   mSessionInfo.keyExchangeKey = hostKEY;
   mSessionInfo.sessionID = hostKID;
   */

   //Utils::FastMemCpy( &mSessionInfo.hostAddress, hostTargetXNADDR, sizeof( XNADDR ));
   //Utils::FastMemCpy( &mSessionInfo.keyExchangeKey, hostKEY, sizeof( XNKEY ));
   //Utils::FastMemCpy( &mSessionInfo.sessionID, hostKID, sizeof( XNKID ));

   mSessionFlags = XSESSION_CREATE_USES_PEER_NETWORK | XSESSION_CREATE_USES_PRESENCE;

   BLiveSessionTaskSessionCreate* newCreateTask = new BLiveSessionTaskSessionCreate(mSessionFlags, mOwnerController, slots, 0, mSessionNonce, hostTargetXNADDR, hostKID, hostKEY);
   addAsyncTaskToQueue(newCreateTask);

   /*
   Utils::FastMemSet( &mOverlapped, 0, sizeof( mOverlapped ) );

   DWORD ret = XSessionCreate(
      mSessionFlags,
      mOwnerController,
      mSlots[ cSessionSlotsTotalPublic ],
      mSlots[ cSessionSlotsTotalPrivate ],
      &mSessionNonce,
      &mSessionInfo,
      &mOverlapped,
      &mSession );

   if ( ret != ERROR_IO_PENDING )
   {
      //Create failed
      nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- XSessionCreate call failed with error 0x%08x", ret);
      mSessionState = cSessionStateError;
      return;
   }
   */

   mSessionState = cSessionStateWaitingForRegistration;
}

//==============================================================================
//Constructor for a leaderboard only session
//==============================================================================
BLiveSession::BLiveSession(int nOwnerController, UINT slots, UINT gameModeIndex) :
mShuttingDownStartTime(0),
mDeinit(false),
mSessionEnded(false),
mStatsGameClass(cLiveSessionGameClassificationUnknown),
mStatsCampaignMapIndex(0),
mStatsGameTypeIndex(0),
mStatsDifficulty(0),
mStatsPartyTeam(false),
mStatsOnlySession(true),
mStatsSaveGameRule(false),
mStopAcceptingAsyncTasks(false),
mLocalUserJoined(false),
mShutdownDialog(false)
{
   BASSERT(slots==1);
   nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- Constructor for starting a Live games list session");

   BLiveSessionTaskSessionSetContext* newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController,  X_CONTEXT_GAME_MODE, gameModeIndex);
   addAsyncTaskToQueue(newContextTask);
   newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_STANDARD);
   addAsyncTaskToQueue(newContextTask);

   mSession                  = INVALID_HANDLE_VALUE;
   //mSessionError             = NULL;
   mIsHost                   = TRUE;
   mSessionNonce             = 0;         
   mOwnerController          = nOwnerController;
   mSessionState             = cSessionStateNone;
   mCommandIssuedTime        = timeGetTime();
   mRanked                   = FALSE;
   mArbitrationRegisteredCount= 0;
   mpArbitrationResults      = NULL;
   mGameModeIndex            = gameModeIndex;
   mMaxPublicSlots           = slots;
   mMaxPrivateSlots          = 0;
   mJoinable                 = true;
   mHostingMode              = cBLiveSessionHostingModeOpen;
   mResendSessionModifyOnNextDrop = false;
   Utils::FastMemSet( &mSessionKID, 0, sizeof( mSessionKID ) );
   Utils::FastMemSet( &mSessionKey, 0, sizeof( mSessionKey ) );
   Utils::FastMemSet( &mHostXNAddr, 0, sizeof( mHostXNAddr ) );

   mSessionFlags = XSESSION_CREATE_SINGLEPLAYER_WITH_STATS;

   BLiveSessionTaskSessionCreate* newCreateTask = new BLiveSessionTaskSessionCreate(mSessionFlags, mOwnerController, slots, 0);
   addAsyncTaskToQueue(newCreateTask);

   /*
   Utils::FastMemSet( &mOverlapped, 0, sizeof( mOverlapped ) );

   DWORD ret = XSessionCreate(
      mSessionFlags,
      mOwnerController,
      mSlots[ cSessionSlotsTotalPublic ],
      mSlots[ cSessionSlotsTotalPrivate ],
      &mSessionNonce,
      &mSessionInfo,
      &mOverlapped,
      &mSession );

   if ( ret != ERROR_IO_PENDING )
   {
      //Create failed
      nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- XSessionCreate call failed with error 0x%08x", ret);
      mSessionState = cSessionStateError;
      return;
   }
   */

   mSessionState = cSessionStateWaitingForRegistration;
}

//==============================================================================
//Constructor for starting a Live game session
//==============================================================================
BLiveSession::BLiveSession(int nOwnerController, UINT slots, UINT gameModeIndex, BOOL ranked) :
   mShuttingDownStartTime(0),
   mDeinit(false),
   mSessionEnded(false),
   mStatsGameClass(cLiveSessionGameClassificationUnknown),
   mStatsCampaignMapIndex(0),
   mStatsGameTypeIndex(0),
   mStatsDifficulty(0),
   mStatsPartyTeam(false),
   mStatsOnlySession(false),
   mStatsSaveGameRule(false),
   mStopAcceptingAsyncTasks(false),
   mLocalUserJoined(false),
   mShutdownDialog(false)
{
   nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- Constructor for starting a Live game session, %d slots", slots);
   mSession                  = INVALID_HANDLE_VALUE;
   mIsHost                   = TRUE;
   mSessionNonce             = 0;         
   mOwnerController          = nOwnerController;
   mSessionState             = cSessionStateNone;
   mCommandIssuedTime        = timeGetTime();
   mRanked                   = ranked;
   mArbitrationRegisteredCount= 0;
   mpArbitrationResults      = NULL;
   mGameModeIndex            = gameModeIndex;
   mMaxPublicSlots           = slots;
   mMaxPrivateSlots          = 0;
   mJoinable                 = true;
   mHostingMode              = cBLiveSessionHostingModeOpen;
   mResendSessionModifyOnNextDrop = false;
   Utils::FastMemSet( &mSessionKID, 0, sizeof( mSessionKID ) );
   Utils::FastMemSet( &mSessionKey, 0, sizeof( mSessionKey ) );
   Utils::FastMemSet( &mHostXNAddr, 0, sizeof( mHostXNAddr ) );

   //Set properties and context for the session
   BLiveSessionTaskSessionSetContext* newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController,  X_CONTEXT_GAME_MODE, gameModeIndex);
   addAsyncTaskToQueue(newContextTask);

   if (mRanked==TRUE)
   {
      newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_RANKED);
   }
   else
   {
      newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_STANDARD);
   } 
   addAsyncTaskToQueue(newContextTask);

   mSessionFlags = XSESSION_CREATE_USES_MATCHMAKING | XSESSION_CREATE_USES_PEER_NETWORK | XSESSION_CREATE_USES_STATS | XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED |
      XSESSION_CREATE_HOST;

   if (mRanked==TRUE)
   {
      mSessionFlags |= XSESSION_CREATE_USES_ARBITRATION;
   }

   BLiveSessionTaskSessionCreate* newCreateTask = new BLiveSessionTaskSessionCreate(mSessionFlags, mOwnerController, slots, 0);
   addAsyncTaskToQueue(newCreateTask);

   /*
   Utils::FastMemSet( &mOverlapped, 0, sizeof( mOverlapped));

   DWORD ret = XSessionCreate(
      mSessionFlags,
      mOwnerController,
      mSlots[ cSessionSlotsTotalPublic ],
      mSlots[ cSessionSlotsTotalPrivate ],
      &mSessionNonce,
      &mSessionInfo,
      &mOverlapped,
      &mSession );

   if ( ret != ERROR_IO_PENDING )
   {
      //Create failed
      nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- XSessionCreate call failed with error 0x%08x", ret);
      mSessionState = cSessionStateError;
      return;
   }
   */

   mSessionState = cSessionStateWaitingForRegistration;
}

//==============================================================================
//Constructor for joining a Live game session
//==============================================================================
BLiveSession::BLiveSession(int nOwnerController, UINT slots, UINT gameModeIndex, BOOL ranked, uint64 nonce, const XNADDR& hostTargetXNADDR, const XNKID& hostKID, const XNKEY& hostKEY) :
   mShuttingDownStartTime(0),
   mDeinit(false),
   mSessionEnded(false),
   mStatsGameClass(cLiveSessionGameClassificationUnknown),
   mStatsCampaignMapIndex(0),
   mStatsGameTypeIndex(0),
   mStatsDifficulty(0),
   mStatsPartyTeam(false),
   mStatsOnlySession(false),
   mStatsSaveGameRule(false),
   mStopAcceptingAsyncTasks(false),
   mLocalUserJoined(false),
   mShutdownDialog(false)
{

   //Set properties and context for the session
   nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- Constructor for joining a Live game session, %d slots", slots);
   mRanked = ranked;

   BLiveSessionTaskSessionSetContext* newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController,  X_CONTEXT_GAME_MODE, gameModeIndex);
   addAsyncTaskToQueue(newContextTask);

   if (mRanked==TRUE)
   {
      newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_RANKED);
   }
   else
   {
      newContextTask = new BLiveSessionTaskSessionSetContext(nOwnerController, X_CONTEXT_GAME_TYPE, X_CONTEXT_GAME_TYPE_STANDARD);
   } 
   addAsyncTaskToQueue(newContextTask);

   mSession                  = INVALID_HANDLE_VALUE;
   mIsHost                   = FALSE;
   mSessionNonce             = nonce;         
   mOwnerController          = nOwnerController;
   mSessionState             = cSessionStateNone;
   mCommandIssuedTime        = timeGetTime();   
   mArbitrationRegisteredCount= 0;
   mpArbitrationResults      = NULL;
   mGameModeIndex            = gameModeIndex;
   mMaxPublicSlots           = slots;
   mMaxPrivateSlots          = 0;
   mJoinable                 = true;
   mHostingMode              = cBLiveSessionHostingModeOpen;
   mResendSessionModifyOnNextDrop = false;
   Utils::FastMemSet( &mSessionKID, 0, sizeof( mSessionKID ) );
   Utils::FastMemSet( &mSessionKey, 0, sizeof( mSessionKey ) );
   Utils::FastMemSet( &mHostXNAddr, 0, sizeof( mHostXNAddr ) );

   /*
   mSessionInfo.hostAddress = hostTargetXNADDR;
   mSessionInfo.keyExchangeKey = hostKEY;
   mSessionInfo.sessionID = hostKID;
   */
   //Utils::FastMemCpy( &mSessionInfo.hostAddress, hostTargetXNADDR, sizeof( XNADDR ));
   //Utils::FastMemCpy( &mSessionInfo.keyExchangeKey, hostKEY, sizeof( XNKEY ));
   //Utils::FastMemCpy( &mSessionInfo.sessionID, hostKID, sizeof( XNKID ));

   mSessionFlags = XSESSION_CREATE_USES_MATCHMAKING | XSESSION_CREATE_USES_PEER_NETWORK | XSESSION_CREATE_USES_STATS | XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;

   if (mRanked==TRUE)
   {
      mSessionFlags |= XSESSION_CREATE_USES_ARBITRATION;
   }

   BLiveSessionTaskSessionCreate* newCreateTask = new BLiveSessionTaskSessionCreate(mSessionFlags, mOwnerController, slots, 0, mSessionNonce, hostTargetXNADDR, hostKID, hostKEY);
   addAsyncTaskToQueue(newCreateTask);

   /*

   Utils::FastMemSet( &mOverlapped, 0, sizeof( mOverlapped ) );

   DWORD ret = XSessionCreate(
      mSessionFlags,
      mOwnerController,
      mSlots[ cSessionSlotsTotalPublic ],
      mSlots[ cSessionSlotsTotalPrivate ],
      &mSessionNonce,
      &mSessionInfo,
      &mOverlapped,
      &mSession );

   if ( ret != ERROR_IO_PENDING )
   {
      //Create failed
      nlog(cLiveSessionNL, "BLiveSession::BLiveSession -- XSessionCreate call failed with error 0x%08x", ret);
      mSessionState = cSessionStateError;
      return;
   }
   */

   mSessionState = cSessionStateWaitingForRegistration;
}

//==============================================================================
//
//==============================================================================
void BLiveSession::sessionCreated(ULONGLONG nonce, HANDLE sessionHandle, PXSESSION_INFO pSessionInfo)
{
   BASSERT(mSessionState==cSessionStateWaitingForRegistration);
   mSessionNonce = nonce;
   mSession = sessionHandle;
   Utils::FastMemCpy( &mSessionKID, &pSessionInfo->sessionID, sizeof(mSessionKID));
   Utils::FastMemCpy( &mSessionKey, &pSessionInfo->keyExchangeKey, sizeof(mSessionKey));
   Utils::FastMemCpy( &mHostXNAddr, &pSessionInfo->hostAddress, sizeof(mHostXNAddr));

   //Session has been created!
   BSimString key;
   key.format("0x%08X%08X%08X%08X", *(DWORD*)&mSessionKey.ab[0],*(DWORD*)&mSessionKey.ab[4],*(DWORD*)&mSessionKey.ab[8],*(DWORD*)&mSessionKey.ab[12]);                  
   nlog(cLiveSessionNL, "BLiveSession::BLiveSession[0x%08X] - Session created, bound to key:%s", mSession, key.getPtr() );

   //Join it local
   DWORD userIndices [LIVESESSION_MAXLOCALUSERS];
   BOOL privateFlags[LIVESESSION_MAXLOCALUSERS];
   userIndices [ 0 ] = mOwnerController;
   privateFlags[ 0 ] = FALSE;         

   BLiveSessionTaskSessionJoinLocal* newTask = new BLiveSessionTaskSessionJoinLocal(1, userIndices, privateFlags);
   addAsyncTaskToQueue(newTask);

   mSessionState = cSessionStateJoiningLocal;               
}

//==============================================================================
//
//==============================================================================
BLiveSession::~BLiveSession()
{
   nlog(cLiveSessionNL, "BLiveSession::~BLiveSession[0x%08X] - AsyncTaskCount (should be zero) [%d]", mSession, mAsyncTaskList.getNumber());
   BASSERTM(mDeinit == true, "Failed to deinit BLiveSession before destruction");

   BSimString key;
   key.format("0x%08X%08X%08X%08X", *(DWORD*)&mSessionKey.ab[0],*(DWORD*)&mSessionKey.ab[4],*(DWORD*)&mSessionKey.ab[8],*(DWORD*)&mSessionKey.ab[12]);                  
   nlog(cLiveSessionNL, "BLiveSession::~BLiveSession[0x%08X] - Deleted session bound to key:%s", mSession, key.getPtr() );

   //Dump the task list
   for(int i=0; i<mAsyncTaskList.getNumber(); i++)
   {
      delete mAsyncTaskList[i];
   }
   mAsyncTaskList.setNumber(0);

   //Get rid of the memory used for arbitration
   if (mpArbitrationResults)
   {
      //Dump this memory
      delete[] mpArbitrationResults;
      mpArbitrationResults = NULL;
   }

   if (mSession!=INVALID_HANDLE_VALUE)
   {
      BOOL ret = CloseHandle(mSession);
      if (!ret)
      {
         nlog(cLiveSessionNL, "BLiveSession::~BLiveSession[0x%08X] - CloseHandle failed", mSession);
      }
   }
}

//==============================================================================
//
//==============================================================================
void BLiveSession::deinit()
{
   if (mDeinit)
      return;
   mDeinit = true;

   mShuttingDownStartTime = timeGetTime();

   // place us in the shutdown state
   nlog(cLiveSessionNL, "BLiveSession::deinit[0x%08X] - started", mSession);

   /*
   //The only overlap that *could* be running here is the one from the constructor
   if (mpOverlapped != NULL && XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
   {
      BASSERTM(false, "BLiveSession::deinit- Overlapped was NOT complete yet"); 
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         Sleep(1);
         if (!mShutdownDialog && timeGetTime() - mShuttingDownStartTime > 5000)
         {
            mShutdownDialog = true;

            BUIGlobals* puiGlobals = gGame.getUIGlobals();
            if (puiGlobals)
            {
               puiGlobals->showWaitDialog(gDatabase.getLocStringFromID(24117));
            }
         }
      }
   }
   */

   if (mSession != INVALID_HANDLE_VALUE)
   {

      if (mSessionState == cSessionStateInGame)
      {
         //If we had started the game - end it
         BLiveSessionTaskSessionEnd* newTask = new BLiveSessionTaskSessionEnd();
         addAsyncTaskToQueue(newTask);
      }

      if (mLocalUserJoined)
      {
         //Remove the local user
         DWORD userIndices [LIVESESSION_MAXLOCALUSERS];
         userIndices [ 0 ] = mOwnerController;       

         BLiveSessionTaskSessionLeaveLocal* newTask = new BLiveSessionTaskSessionLeaveLocal(1, userIndices);
         addAsyncTaskToQueue(newTask);
      }

      //Then delete it
      BLiveSessionTaskSessionDelete* newTask = new BLiveSessionTaskSessionDelete();
      addAsyncTaskToQueue(newTask);

      //Safe to delete this object when its state is cSessionStateSessionDeleted
   }
   else
   {
      //No handle then the session was never created - we can say we are done now
      setState(BLiveSession::cSessionStateSessionDeleted);     
   }
   mStopAcceptingAsyncTasks = true;

   nlog(cLiveSessionNL, "BLiveSession::deinit[0x%08X] - complete", mSession);
 
}

//==============================================================================
//
//==============================================================================
bool BLiveSession::isShutdown()
{
   return (mSessionState==cSessionStateSessionDeleted);
}

//==============================================================================
// Async Task Notifications
//==============================================================================
void BLiveSession::notify(DWORD eventID, void * task)
{
   //Task eventID just completed, call next one in the queue
   updateAsyncTaskQueue();
}

//==============================================================================
//
//==============================================================================
void BLiveSession::addAsyncTaskToQueue(BLiveSessionAsyncTask* newTask)
{
   if (mStopAcceptingAsyncTasks)
   {
      //No more tasks!
      nlog(cLiveSessionNL, "BLiveSession::addAsyncTaskToQueue[0x%08X] - no longer accepting tasks, deleting new request", mSession);
      delete newTask;
      return;
   }

   //Assign the mSession handle to this instance
   newTask->mOwningLiveSession = this;

   //Add it to the list
   mAsyncTaskList.add(newTask);

}

//==============================================================================
//
//==============================================================================
void BLiveSession::updateAsyncTaskQueue()
{
   if (mAsyncTaskList.getNumber()==0)
   {
      //Its empty - we are done
      return;
   }

   static bool doNotReenter=false;
   BASSERT(doNotReenter==false);
   doNotReenter=true;

   BLiveSessionAsyncTask *task = mAsyncTaskList.get(0);
   BASSERT(task);

   //Is it running?
   if (!task->mStarted)
   {
      nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- updateAsyncTaskQueue - Launching new task from queue" , mSession);
      task->doAsyncFunction();
   }

   //Give it an update
   task->update();

   //Is it complete?
   if (task->isComplete())
   {
      //Did it complete - but it has an error?
      DWORD hr = task->getExtendedError();
      bool apiFailure = false;
      if (hr != ERROR_SUCCESS)
      {
         apiFailure= true;
      }

      //It is done - move to the next one
      if ((task->getState()== BAsyncTask::cStateError) ||
          (apiFailure))
      {
         nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- updateAsyncTaskQueue has a failure" , mSession);
         //Set the state here so that the onFailure code has a chance to change it to something else if it needs to 
         mSessionState = cSessionStateError;
         task->onFailure();
         mAsyncTaskList.removeIndex(0);
         delete task;
         doNotReenter=false;
         //On failure - we don't immediately check for the next task - the next update() will catch that
         //  This allows the onFailure to fire off a shutdown/deinit if it needs to without having another cmd in the queue
         return;        
      }

      nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- updateAsyncTaskQueue - task complete, moving to next task", mSession);

      //It either completed successfully
      task->onSuccess();

      //Remove it from the list
      mAsyncTaskList.removeIndex(0);
      delete task;

      //Check immediately for the next task
      if (mAsyncTaskList.getNumber()>0)
      {
         task = mAsyncTaskList.get(0);
         BASSERT(task);
         nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- updateAsyncTaskQueue - Launching new task from queue", mSession);
         task->doAsyncFunction();
      }    
   }  
   doNotReenter=false;
}


//==============================================================================
//
//==============================================================================
void BLiveSession::update()
{
   updateAsyncTaskQueue();

   /*
   if (mSessionState == cSessionStateWaitingForRegistration)
   {
      if (mpOverlapped != NULL && XHasOverlappedIoCompleted(mpOverlapped))
      {
         HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
         if (FAILED(hr))
         {
             nlog(cLiveSessionNL, "BLiveSession::update -- XSessionCreate overlapped task failed with error 0x%08x",  hr );
             mSessionState = cSessionStateError;
             return;
         }

         //I have a valid session created on Live
         Utils::FastMemCpy( &mSessionKID, &mSessionInfo.sessionID, sizeof(mSessionKID));
         Utils::FastMemCpy( &mSessionKey, &mSessionInfo.keyExchangeKey, sizeof(mSessionKey));
         Utils::FastMemCpy( &mHostXNAddr, &mSessionInfo.hostAddress, sizeof(mHostXNAddr));

         //Join it local
         DWORD userIndices [LIVESESSION_MAXLOCALUSERS];
         BOOL privateFlags[LIVESESSION_MAXLOCALUSERS];
         userIndices [ 0 ] = mOwnerController;
         privateFlags[ 0 ] = FALSE;         
        
         BLiveSessionTaskSessionJoinLocal* newTask = new BLiveSessionTaskSessionJoinLocal(1, userIndices, privateFlags);
         addAsyncTaskToQueue(newTask);

         mSessionState = cSessionStateJoiningLocal;              
      }
   }
   */

   //See if I need to check on any clients
   if ((mSessionState >= cSessionStateSessionRunning) &&
       (mSessionState < cSessionStateDeleteing))
   {
      //HRESULT hr;
      for (int i=0; i < LIVESESSION_USERSLOTS; i++)
      {
         if ((mRemoteUsers[i].status == cLiveSessionUserStatusAddFailed)    ||
             (mRemoteUsers[i].status == cLiveSessionUserStatusDropFailed)   ||
             (mRemoteUsers[i].status == cLiveSessionUserStatusDropCompleted))
         {
            if ((timeGetTime() - mRemoteUsers[i].commandIssuedTime) > LIVESESSION_STATUSHOLDDELAY)
            {
               nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- clearing unused client record %i", mSession, i);
               mRemoteUsers[i].xuid = 0;
               mRemoteUsers[i].status = cLiveSessionUserStatusRecordUnused;
            }
         }
      } 
   }
}

//==============================================================================
//
//==============================================================================
bool BLiveSession::getSessionHostXNAddr(XNADDR& hostXNAddr)
{
   //Make sure they passed me in a memory space
   //BASSERT( hostXNAddr );

   if (!isSessionValid())
   {
      return false;
   }
   
   //Utils::FastMemCpy( hostXNAddr, &mHostXNAddr, sizeof(mHostXNAddr));
   hostXNAddr = mHostXNAddr;

   return true;
}

//==============================================================================
//
//==============================================================================
bool BLiveSession::getXNKID(XNKID& xnkid)
{
   if (!isSessionValid())
   {
      return false;
   }

   //Utils::FastMemCpy( xnkid, &mSessionKID, sizeof(XNKID));
   xnkid = mSessionKID;

   return true;
}

//==============================================================================
//
//==============================================================================
bool BLiveSession::getXNKEY(XNKEY& xnkey)
{
   if (!isSessionValid())
   {
      return false;
   }

   //Utils::FastMemCpy( xnkey, &mSessionKey, sizeof(XNKEY));
   xnkey = mSessionKey;

   return true;
}

//==============================================================================
// 
//==============================================================================
void BLiveSession::setUserState(XUID xuid, BLiveSessionUserStatus newState)
{
   //Find the user's record
   for (int i=0; i < LIVESESSION_USERSLOTS; i++)
   {
      if (mRemoteUsers[i].xuid == xuid)
      {
         //Found it
         mRemoteUsers[i].status = newState;
         return;
      }
   }
}

//==============================================================================
// 
//==============================================================================
void BLiveSession::arbitrationCompleted(bool succesful)
{  
   if (!succesful)
   {
      //It failed
      //Something else that failed is my spelling when I wrote this
      nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- Arbitration failed, we cannot continue with this registration/session.", mSession);
      mSessionState = cSessionStateError;
      return;
   }


   if (mpArbitrationResults->wNumRegistrants != (mMaxPublicSlots + mMaxPrivateSlots))//(mSlots[cSessionSlotsTotalPublic]+mSlots[cSessionSlotsTotalPrivate]))
   {
      //BASSERTM(false, "liveSession registration has WRONG NUMBER of peeps");
      nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- XSessionArbitrationRegister reported %d members, XSessionCreate was started with %d members, Incorrect count but host final registration should correct this",  mSession, mpArbitrationResults->wNumRegistrants, (mMaxPublicSlots + mMaxPrivateSlots));
      generateUserStatusSpam();
      //This only needs to fail if we are the host and this is the FINAL arbitration registration
      if (mIsHost && (mArbitrationRegisteredCount==1))
      {
         nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- FAILED!: Im the host and this is the FINAL arbitration, we cannot continue with this registration/session.", mSession);
         mSessionState = cSessionStateError;
         return;
      }
   }

   nlog(cLiveSessionNL, "BLiveSession::update[0x%08X] -- XSessionArbitrationRegister results: %d members", mSession, mpArbitrationResults->wNumRegistrants);
   for (DWORD i=0;i<mpArbitrationResults->wNumRegistrants;i++)
   {
      nlog(cLiveSessionNL, "  Player %d - XUID(0):%I64u - Connected Users:%d - Trustworthiness:%d ", i, mpArbitrationResults->rgRegistrants[i].rgUsers[0], mpArbitrationResults->rgRegistrants[i].bNumUsers, mpArbitrationResults->rgRegistrants[i].bTrustworthiness );
   }

   mArbitrationRegisteredCount ++;
   mSessionState = cSessionStateArbitrationRegistrationComplete;
}

//==============================================================================
//
//==============================================================================
bool BLiveSession::addRemoteUserToSession(XUID userXUID, bool privateSlot)
{
   //See if the user is already in the list and find an empty slot
   int firstEmpty=-1;
   for (int i=0; i < LIVESESSION_USERSLOTS; i++)
   {
      if (mRemoteUsers[i].xuid == userXUID)
      {
         //He is already in here - use this index and go ahead an re-call the API to add him remote again
         nlog(cLiveSessionNL, "BLiveSession::addRemoteUserToSession[0x%08X] -- user already in list, re-adding him remote to live session", mSession);
         firstEmpty = i;
         break;
      }
      else if (mRemoteUsers[i].status == cLiveSessionUserStatusRecordUnused)
      {
         firstEmpty = i;
         break;
      }
   }

   if (firstEmpty == -1)
   {
      //No slots
      nlog(cLiveSessionNL, "BLiveSession::addRemoteUserToSession[0x%08X] -- warning: No empty user slots in the live session", mSession);
      return false;
   }

   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete successfully");
      }
   }
   */

   mRemoteUsers[firstEmpty].xuid = userXUID;
   mRemoteUsers[firstEmpty].status = cLiveSessionUserStatusAddPending;
   mRemoteUsers[firstEmpty].privateSlots = (privateSlot?1:0);
   mRemoteUsers[firstEmpty].commandIssuedTime = timeGetTime();
   BLiveSessionTaskSessionJoinRemote* newTask = new BLiveSessionTaskSessionJoinRemote(userXUID, FALSE);
   addAsyncTaskToQueue(newTask);
   /*
   Utils::FastMemSet(&mRemoteUsers[firstEmpty].mOverlapped, 0, sizeof(XOVERLAPPED));

   mpOverlapped = &mRemoteUsers[firstEmpty].mOverlapped;

   DWORD dwRet = XSessionJoinRemote(mSession, 1, &mRemoteUsers[firstEmpty].xuid, &mRemoteUsers[firstEmpty].privateSlots, mpOverlapped);

   if (dwRet != ERROR_IO_PENDING)
   {
      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      nlog(cLiveSessionNL, "BLiveSession::addRemoteUserToSession -- warning:Failed to add remote users to session, error 0x%08x\n", hr);
      hr;
      mRemoteUsers[firstEmpty].status = cLiveSessionUserStatusAddFailed;
      return false;
   }

   nlog(cLiveSessionNL, "BLiveSession::addRemoteUserToSession -- user add called xuid[%I64u]", userXUID);
   */
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSession::dropRemoteUserFromSession(XUID userXUID)
{
   //Find the user or find an empty slot to store for the space so I can call the API to drop him
   int userIndex=-1;
   for (int i=0; i < LIVESESSION_USERSLOTS; i++)
   {
      if (mRemoteUsers[i].xuid == userXUID)
      {
         //Found him
         nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession[0x%08X] - found tracking record for [%I64u]", mSession, userXUID );
         userIndex = i;
         break;
      }
   }

   if (userIndex == -1)
   {
      return;
   }

   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete successfully");
      }
   }
   */

   //Found his record - lets make sure he was properly added to the session and that there is no outstanding overlapped call
   //if (userIndex != -1)
   //{
   //   //if (!XHasOverlappedIoCompleted(&mRemoteUsers[userIndex].mOverlapped))
   //   //{
   //   //   nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession - overlapped IO was still pending, canceling that request");
   //   //   HRESULT hr = XCancelOverlapped(&mRemoteUsers[userIndex].mOverlapped);
   //   //   if (FAILED( hr ))
   //   //   {
   //   //      nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession - XCancelOverlapped FAILED");
   //   //      BASSERT(false);
   //   //   }
   //   //}
   //}
   //else
   //{
   //   //Just find an empty slot to store the request overlap in 
   //   for (int i=0;i<LIVESESSION_USERSLOTS;i++)
   //   {
   //      if (mRemoteUsers[i].status == cLiveSessionUserStatusRecordUnused)
   //      {
   //         //Found him
   //         nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession - Could not find tracking record, assigning him into an empty slot for the drop call [%I64u]", userXUID );
   //         userIndex = i;
   //         break;
   //      }
   //   }
   //}

   //if (userIndex == -1)
   //{
   //   //No slots
   //   nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession -- warning: User not found, and no space to track him so I can drop him [%I64u]", userXUID );
   //   return;
   //}

   if ((mSessionState==cSessionStateInGame) ||
       (mSessionState==cSessionStateEndPending))
   {
      //This person is dropping early - record what we know about them
      nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession[0x%08X] -- Dropping while game in progress - writing their stats now", mSession);
      //TODO - when setPlayerWon is moved to ASYNC, this call will have to make the actual drop pending until after the write is complete
      //TODO - we need a back query into the game object to find out what team the person is on
      //       or (alternately) we need the game to register the teams with the live session once it knows the team (better solution!)
      //TODO - determine the % of win points we want to give to folks who's team won but they are early droppers
      //int teamID = 0;
      //TODO - temporarily disabled - eric
      //setPlayerWon(userXUID, false, teamID, 0, 0, 0);  //No need to give additional detail parameters here because they lost
   }

   //mRemoteUsers[userIndex].xuid = userXUID;
   mRemoteUsers[userIndex].status = cLiveSessionUserStatusDropPending;
   mRemoteUsers[userIndex].commandIssuedTime = timeGetTime();
   BLiveSessionTaskSessionLeaveRemote* newTask = new BLiveSessionTaskSessionLeaveRemote(userXUID);
   addAsyncTaskToQueue(newTask);
   /*
   Utils::FastMemSet(&mRemoteUsers[userIndex].mOverlapped, 0, sizeof(XOVERLAPPED));

   mpOverlapped = &mRemoteUsers[userIndex].mOverlapped;

   DWORD dwRet = XSessionLeaveRemote(mSession, 1, &mRemoteUsers[userIndex].xuid, mpOverlapped);

   if (dwRet != ERROR_IO_PENDING)
   {
      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession -- warning:Failed to drop remote user from session, error 0x%08x\n", hr);
      hr;
      mRemoteUsers[userIndex].status = cLiveSessionUserStatusDropFailed;
      return;
   }

   nlog(cLiveSessionNL, "BLiveSession::dropRemoteUserFromSession -- user drop called");
   */
}

//==============================================================================
// Odd little helper function that lets other systems scan through my member list
//   looking for session adds that have failed.
//==============================================================================
XUID BLiveSession::getNextUserAddFailure(uint startingIndex)
{
   if (startingIndex >= LIVESESSION_USERSLOTS)
   {
      return 0;
   }

   for (uint i=startingIndex; i < LIVESESSION_USERSLOTS; i++)
   {
      if (mRemoteUsers[i].status == cLiveSessionUserStatusAddFailed)
      {
         return mRemoteUsers[i].xuid;
      }
   }

   return 0;
}

//==============================================================================
//
//==============================================================================
BLiveSessionUserStatus BLiveSession::getUserStatus(XUID userXUID)
{

   //Are the asking about the local user?
   XUID localXuid;
   if ((XUserGetXUID(mOwnerController, &localXuid) == ERROR_SUCCESS) && 
      (userXUID == localXuid))
   {
      //They want status for our local user, so map this object's status into a user status
      if (mSessionState >= cSessionStateSessionRunning )
      {
         return cLiveSessionUserStatusInSession;
      }
      else
      {
         return cLiveSessionUserStatusAddPending;
      }
   }

   //Find the user
   for (int i=0;i<LIVESESSION_USERSLOTS;i++)
   {
      if (mRemoteUsers[i].xuid == userXUID)
      {
         //Found him
         return mRemoteUsers[i].status;
      }
   }

   return cLiveSessionUserStatusUserUnknown;
}

//==============================================================================
// Returns false if this live session did not need to register
//  Thus the caller can know if it needs to wait on that to complete or not 
//==============================================================================
bool BLiveSession::registerForArbitration()
{  
   //Don't need to do this if it is not a ranked game
   if (mRanked==FALSE)
   {
      return false;
   }

   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete successfully");
      }
   }
   */

   //Check if this has already been done
   if (mIsHost)
   {
      BASSERT( (mSessionState == cSessionStateSessionRunning) || (mSessionState == cSessionStateArbitrationRegistrationComplete));
      BASSERT( mArbitrationRegisteredCount<2);
   }
   else
   {
      BASSERT( mSessionState == cSessionStateSessionRunning );
      BASSERT( mArbitrationRegisteredCount==0 );
   }
 
   /*
   Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));
   mpOverlapped = &mOverlapped;
   */
   mCommandIssuedTime = timeGetTime();

   nlog(cLiveSessionNL, "BLiveSession::registerForArbitration[0x%08X] - Calling XSessionArbitrationRegister, as %s", mSession, mIsHost?"HOST":"CLIENT");
   //Lil debug spam here
   generateUserStatusSpam();

   if (mpArbitrationResults)
   {
      //Dump this memory
      delete[] mpArbitrationResults;
      mpArbitrationResults = NULL;
   }

   DWORD registrationResults=0;

   //Call it twice, first is SYNC just to get the size of the buffer to alloc
   DWORD ret = XSessionArbitrationRegister(mSession, 0, mSessionNonce, &registrationResults, mpArbitrationResults, NULL);

   if (ret == ERROR_SUCCESS)
   {
      nlog(cLiveSessionNL, "BLiveSession::registerForArbitration[0x%08X] - XSessionArbitrationRegister worked on the first call - very odd", mSession);
      BASSERT(false);
      return false;
   }

   if ((ret == ERROR_INSUFFICIENT_BUFFER) && 
       (registrationResults > 0))
   {
      mpArbitrationResults  = (PXSESSION_REGISTRATION_RESULTS) new BYTE[registrationResults];
      BASSERTM(mpArbitrationResults, "BLiveSession::registerForArbitration - ALLOC for buffer space failed");
      Utils::FastMemSet(mpArbitrationResults, 0, registrationResults);

      //Call it a second time ASYNC to actually use the API
      BLiveSessionTaskSessionArbitrationRegister* newTask = new BLiveSessionTaskSessionArbitrationRegister( mSessionNonce, registrationResults, mpArbitrationResults);
      addAsyncTaskToQueue(newTask);
      /*
      DWORD ret = XSessionArbitrationRegister(mSession, 0, mSessionNonce, &mRegistrationResults, mpArbitrationResults, mpOverlapped);
      if (ret != ERROR_IO_PENDING)
      {
         nlog(cLiveSessionNL, "BLiveSession::registerForArbitration -- warning:XSessionArbitrationRegister failed IMMEDATELY, error 0x%08x\n", ret);
         mSessionState = cSessionStateError;
         return false;
      }
      */
      mSessionState = cSessionStateArbitrationRegistrationPending;
      return true; 
   }
   else
   {
      BASSERTM(false, "BLiveSession::registerForArbitration - Call failed, unexpected result");
      nlog(cLiveSessionNL, "BLiveSession::registerForArbitration[0x%08X] - XSessionArbitrationRegister wCall failed, unexpected result %d", mSession, ret);
      mSessionState = cSessionStateError;
      return false;
   }  
}

//==============================================================================
// Returns false if this live session did not need to register
//  Thus the caller can know if it needs to wait on that to complete or not 
// This is to support when the host has to call registration a SECOND time after everyone else has
//==============================================================================
bool BLiveSession::hostOnlySecondaryRegisterForArbitration()
{
   return (registerForArbitration());
}


//==============================================================================
//
//==============================================================================
void BLiveSession::startEndPartySession()
{
   //Called on the party session to just start/end it right off so that Live is happy
   BLiveSessionTaskSessionStart* newTask = new BLiveSessionTaskSessionStart();
   addAsyncTaskToQueue(newTask);
   BLiveSessionTaskSessionEnd* newTask2 = new BLiveSessionTaskSessionEnd();
   addAsyncTaskToQueue(newTask2);
}

//==============================================================================
//
//==============================================================================
void BLiveSession::startGame()
{
   if (mStatsOnlySession && (mSessionState == cSessionStateSessionRunning))
   {
      //State hack because single player stats only sessions do not register for arbitration
      mSessionState = cSessionStateArbitrationRegistrationComplete;
   }

   BASSERT( (mSessionState == cSessionStateSessionRunning) || (mSessionState == cSessionStateArbitrationRegistrationComplete));

   BLiveSessionTaskSessionStart* newTask = new BLiveSessionTaskSessionStart();
   addAsyncTaskToQueue(newTask);

   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete");
      }
   }

   //BASSERT(XHasOverlappedIoCompleted(&mOverlapped));

   Utils::FastMemSet(&mOverlapped, 0, sizeof(XOVERLAPPED));
   mpOverlapped = &mOverlapped;
   mCommandIssuedTime = timeGetTime();

   nlog(cLiveSessionNL, "BLiveSession::startGame -- Calling XSessionStart" );

   DWORD ret = XSessionStart(mSession, 0, mpOverlapped);
   if ( ret != ERROR_IO_PENDING )
   {
      //Create failed
      nlog(cLiveSessionNL, "BLiveSession::startGame -- warning:XSessionStart failed IMMEDATELY, error 0x%08x\n", ret);
      mSessionState = cSessionStateError;
      return;
   }
   */

   mSessionState = cSessionStateStartPending;
}

//==============================================================================
//
//==============================================================================
void BLiveSession::endGame()
{   
   //Safe to call this multiple times - or at the wrong time
   if (mSessionState == cSessionStateInGame)
   {
      nlog(cLiveSessionNL, "BLiveSession::endGame[0x%08X] -- Calling XSessionEnd", mSession);
      BLiveSessionTaskSessionEnd* newTask = new BLiveSessionTaskSessionEnd();
      addAsyncTaskToQueue(newTask);
      mSessionState = cSessionStateEndPending;
   }
      /*
      //BASSERT(XHasOverlappedIoCompleted(&mOverlapped));
      //Utils::FastMemSet( &mOverlapped, 0, sizeof( mOverlapped ) );

      if (mpOverlapped != NULL)
      {
         while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
         {
            // insure that we have no outstanding XSession calls
            // FIXME include a timeout
            Sleep(1);
         }

         HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
         if (hr != ERROR_SUCCESS)
         {
            BASSERTM(FALSE, "A previous XSession call failed to complete");
         }
      }

      Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));
      mpOverlapped = &mOverlapped;

      mCommandIssuedTime = timeGetTime();

      if (!mSessionEnded)
      {
         mSessionEnded = true;

         DWORD ret = XSessionEnd(mSession, mpOverlapped);
         if (ret != ERROR_IO_PENDING)
         {
            //End failed
            mSessionState = cSessionStateError;
            return;
         }
         mSessionState = cSessionStateEndPending;
      }
   }
   */
}

//==============================================================================
// 
//==============================================================================
void BLiveSession::setPlayerWon(XUID playerXUID, BOOL playerWonGame, long teamID, uint leaderIndex, uint score, uint gamePlayTime)
{
   if (playerXUID == 0)
      return;

   //Placeholder BLOCKING call to post stats to Live
   //TODO - change to an async call, this will add a problem where ENDGAME is called before each stats write is possibly complete
   //  End game will have to que the endgame request until all writes are complete
   //  Rest of game system will have to wait on this entire thing being complete and posted before stopping updates to this object
   //TONOTDO - don't change this one, it just writes to memory in the OS side, it doesn't actually post anything

   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete");
      }
   }

   Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));
   mpOverlapped = &mOverlapped;
   */

   BLiveSessionTaskSessionWriteStats* newTask = new BLiveSessionTaskSessionWriteStats(playerXUID);
   //uint viewCount = 0;
   //XSESSION_VIEW_PROPERTIES Views[6];

   //Add in other views based on the other leaderboards we need to write to
   //if (mStatsGameClass==cLiveSessionGameClassificationCustom)
   if (mStatsGameClass==cLiveSessionGameClassificationMatchmade)
   {
      //XUSER_PROPERTY* Skill[2];
      newTask->mPropertySkill[ 0 ].dwPropertyId = X_PROPERTY_RELATIVE_SCORE;
      newTask->mPropertySkill[ 0 ].value.nData  = (playerWonGame?10:0);
      newTask->mPropertySkill[ 0 ].value.type   = XUSER_DATA_TYPE_INT32;
      newTask->mPropertySkill[ 1 ].dwPropertyId = X_PROPERTY_SESSION_TEAM;
      newTask->mPropertySkill[ 1 ].value.nData  = teamID;
      newTask->mPropertySkill[ 1 ].value.type   = XUSER_DATA_TYPE_INT32;
      newTask->mViews[ 0 ].dwNumProperties = 2;
      newTask->mViews[ 0 ].dwViewId        = X_STATS_VIEW_SKILL;
      newTask->mViews[ 0 ].pProperties     = newTask->mPropertySkill;
      newTask->mViewCount++;

      //Add one for the global wins leaderboard
      //XUSER_PROPERTY propertiesWins[3];
      newTask->mPropertyWins[0].dwPropertyId = PROPERTY_GAMEWINS;
      newTask->mPropertyWins[0].value.type = XUSER_DATA_TYPE_INT64;
      newTask->mPropertyWins[0].value.i64Data = (playerWonGame?1:0);
      newTask->mPropertyWins[1].dwPropertyId = PROPERTY_GAMESPLAYED;
      newTask->mPropertyWins[1].value.type = XUSER_DATA_TYPE_INT64;
      newTask->mPropertyWins[1].value.i64Data = 1;
      newTask->mPropertyWins[2].dwPropertyId = PROPERTY_GAMETOTALSCORE;
      newTask->mPropertyWins[2].value.type = XUSER_DATA_TYPE_INT64;
      newTask->mPropertyWins[2].value.i64Data = score;
      newTask->mViews[newTask->mViewCount].dwNumProperties = 3;
      newTask->mViews[newTask->mViewCount].dwViewId = gLiveSystem->findLeaderBoardIndex(cLeaderBoardTypeMPGametypeWinsLifetime,(mMaxPublicSlots + mMaxPrivateSlots),mStatsPartyTeam,0,0, mStatsGameTypeIndex, 0);
      newTask->mViews[newTask->mViewCount].pProperties = newTask->mPropertyWins;
      newTask->mViewCount++;

      //Add one for the monthly wins leaderboard (same data as above)
      newTask->mViews[newTask->mViewCount].dwNumProperties = 3;
      newTask->mViews[newTask->mViewCount].dwViewId = gLiveSystem->findLeaderBoardIndex(cLeaderBoardTypeMPGametypeWinsMonthly,(mMaxPublicSlots + mMaxPrivateSlots),mStatsPartyTeam,0,0, mStatsGameTypeIndex, 0);
      newTask->mViews[newTask->mViewCount].pProperties = newTask->mPropertyWins;
      newTask->mViewCount++;

      //Add one for the monthly wins leaderboard by leader (same data as above)
      newTask->mViews[newTask->mViewCount].dwNumProperties = 3;
      newTask->mViews[newTask->mViewCount].dwViewId = gLiveSystem->findLeaderBoardIndex(cLeaderBoardTypeMPLeaderWinsMonthly,0,false,0,0, mStatsGameTypeIndex, leaderIndex);
      newTask->mViews[newTask->mViewCount].pProperties = newTask->mPropertyWins;
      newTask->mViewCount++;

   }
   else if (mStatsGameClass==cLiveSessionGameClassificationCampaign)
   {
      //For campaign games - the leaderboards are not arbitrated - which means you only post data for your local members
      bool isLocal = false;
      BUser* user = gUserManager.getPrimaryUser();
      if (user && user->getXuid()==playerXUID)
      {
         isLocal = true;
      }
      else 
      {
         user = gUserManager.getSecondaryUser();
         if (user && user->getXuid()==playerXUID)
         {
            isLocal = true;
         }
      }
      if (!isLocal)
      {
         nlog( cLiveSessionNL, "BLiveSession::setPlayerWon[0x%08X] - not arbitrated session and XUID is not local, not posting", mSession);
         return;
      }
      if (mStatsCampaignMapIndex==0)
      {
         //No map index means no stats!
         //Note: this is a valid value because the tutorial levels map to 0
         nlog( cLiveSessionNL, "BLiveSession::setPlayerWon[0x%08X] - Mapindex is 0 (tutorial?) - not posting stats", mSession);
         delete newTask;
         return;
      }

      //Add one for the global campaign score board - solo or co-op (mStatsPartyTeam tells the difference)
      //XUSER_PROPERTY propertiesScore[2];
      newTask->mPropertyScore[0].dwPropertyId = PROPERTY_GAMESPLAYED;
      newTask->mPropertyScore[0].value.type = XUSER_DATA_TYPE_INT64;      
      newTask->mPropertyScore[1].dwPropertyId = PROPERTY_GAMETOTALSCORE;
      newTask->mPropertyScore[1].value.type = XUSER_DATA_TYPE_INT64;
      if (!mStatsSaveGameRule)
      {
         newTask->mPropertyScore[0].value.i64Data = 1;
         newTask->mPropertyScore[1].value.i64Data = score;
      }
      else
      {
         newTask->mPropertyScore[0].value.i64Data = 0;
         newTask->mPropertyScore[1].value.i64Data = 0;
      }
      newTask->mViews[newTask->mViewCount].dwNumProperties = 2;
      newTask->mViews[newTask->mViewCount].dwViewId = gLiveSystem->findLeaderBoardIndex(cLeaderBoardTypeCampaignLifetime,0,mStatsPartyTeam,0,0,0,0);
      newTask->mViews[newTask->mViewCount].pProperties = newTask->mPropertyScore;
      newTask->mViewCount++;

      //Add one for best campaign score per map/difficulty
      BASSERT(mStatsCampaignMapIndex!=0);
      //Check in case the mapname->index mapping fails
      if ((mStatsCampaignMapIndex!=0) &&
          (playerWonGame))
      {
         //XUSER_PROPERTY propertiesScoreByMap[2];
         newTask->mPropertyScoreByMap[0].dwPropertyId = PROPERTY_GAMEWASCOOP;
         newTask->mPropertyScoreByMap[0].value.type = XUSER_DATA_TYPE_INT32;
         newTask->mPropertyScoreByMap[0].value.nData = mStatsPartyTeam;
         newTask->mPropertyScoreByMap[1].dwPropertyId = PROPERTY_GAMESCORE;
         newTask->mPropertyScoreByMap[1].value.type = XUSER_DATA_TYPE_INT64;
         newTask->mPropertyScoreByMap[1].value.i64Data = score;
         newTask->mViews[newTask->mViewCount].dwNumProperties = 2;
         newTask->mViews[newTask->mViewCount].dwViewId = gLiveSystem->findLeaderBoardIndex(cLeaderBoardTypeCampaignBestScore,0,false,mStatsDifficulty,mStatsCampaignMapIndex, 0, 0);
         newTask->mViews[newTask->mViewCount].pProperties = newTask->mPropertyScoreByMap;
         newTask->mViewCount++;
      }
   }

   //TODO! Versus AI boards

   //If there is nothing to update - don't post
   if (newTask->mViewCount==0)
   {
      nlog( cLiveSessionNL, "BLiveSession::setPlayerWon[0x%08X] - no leaderboards to update, not posting.", mSession);
      delete newTask;
      return;
   }
  
   nlog(cLiveSessionNL, "BLiveSession::endGame[0x%08X] -- Calling XSessionWriteStats for %I64u", mSession, playerXUID);
   addAsyncTaskToQueue(newTask);
   /*
   DWORD ret = XSessionWriteStats(mSession, playerXUID, viewCount, Views, NULL);

   if (ret != ERROR_SUCCESS)
   {
      HRESULT hr = GetLastError();

      nlog( cLiveSessionNL, "BLiveSession::setPlayerWon - Failed to write stats, error 0x%08x\n", hr );
      hr;
   }
   */
}

//==============================================================================
// 
//==============================================================================
bool BLiveSession::isMatchmadeGame()
{
   return (mStatsGameClass==cLiveSessionGameClassificationMatchmade);
}

//==============================================================================
// 
//==============================================================================
void BLiveSession::storeStatsGameSettings( BLiveSessionGameClassification gameClass, bool partyTeam, uint campaignMapIndex, uint gameTypeIndex, uint difficulty, bool statsSaveGameRule )
{
   mStatsGameClass=gameClass;
   mStatsCampaignMapIndex=campaignMapIndex;
   mStatsGameTypeIndex=gameTypeIndex;  
   mStatsDifficulty=difficulty;
   mStatsPartyTeam=partyTeam;
   mStatsSaveGameRule=statsSaveGameRule;
}

//==============================================================================
// 
//==============================================================================
void BLiveSession::modify(bool joinable, uint totalSlots)
{
   //Note - currently this is only ever called by the party session and the handling here of the flags only works for party session
   nlog(cLiveSessionNL, "BLiveSession::modify[%d]", mSession);
   if (mSessionState != cSessionStateSessionRunning)
   {
      nlog(cLiveSessionNL, "BLiveSession::modify[0x%08X] - ERROR - XSessionModify requested but I'm not running - slots[%d]", mSession, totalSlots);
      BASSERT(false);
      return;
   }

   //Only thing that *should* be able to hit here is session creation
   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete");
      }
   }

   Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));
   mpOverlapped = &mOverlapped;
   */

   mJoinable = joinable;
   mMaxPublicSlots = totalSlots;

   //Just take out the flags we care about to begin with - then add them in if we need them
   mSessionFlags &= ~XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
   mSessionFlags &= ~XSESSION_CREATE_JOIN_VIA_PRESENCE_FRIENDS_ONLY;
   mSessionFlags &= ~XSESSION_CREATE_INVITES_DISABLED;

   if (!mJoinable)
   {
      mSessionFlags |= XSESSION_CREATE_INVITES_DISABLED;
      mSessionFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
   }
   else
   {
      if (mHostingMode==cBLiveSessionHostingModeOpen)
      {
         //nada
      }
      else if (mHostingMode==cBLiveSessionHostingModeFriendsOnly)
      {      
         mSessionFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_FRIENDS_ONLY;  
      }
      else if (mHostingMode==cBLiveSessionHostingModeInviteOnly)
      {
         mSessionFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
      }  
   }


   BLiveSessionTaskSessionModify* newTask = new BLiveSessionTaskSessionModify(mSessionFlags, totalSlots, 0);
   addAsyncTaskToQueue(newTask);

   /*
   DWORD retval = XSessionModify(mSession, mSessionFlags, totalSlots, 0, NULL);
   if (retval != ERROR_SUCCESS)
   {
      if (retval == ERROR_FUNCTION_FAILED)
      {
         //This method fails if we are reducing the number of slots to less that the number of peeps in the session currently
         //So this sets a flag so that after we process the next drop - we try again
         nlog(cLiveSessionNL, "BLiveSession::modify - XSessionModify failed with ERROR_FUNCTION_FAILED, marking this call to be tried again next time a player drops - session %d, %d slots", mSession, totalSlots);
         mResendSessionModifyOnNextDrop = true;
         return;
      }
      nlog(cLiveSessionNL, "BLiveSession::modify - XSessionModify failed IMMEDIATELY with code 0x%08X on session %d, %d slots", retval, mSession, totalSlots);
      BASSERT(false);
   }
   mResendSessionModifyOnNextDrop = false;
   */
}

//==============================================================================
//
//==============================================================================
void BLiveSession::changeLiveMode(BLiveSessionHostingModes newMode)
{
   if (newMode == mHostingMode)
   {
      //No change - exit
      return;
   }

   nlog(cLiveSessionNL, "BLiveSession::changeLiveMode[0x%08X]", mSession);

   if (mSessionState != cSessionStateSessionRunning)
   {
      nlog(cLiveSessionNL, "BLiveSession::changeLiveMode[0x%08X] - ERROR - XSessionModify requested but I'm not running", mSession);
      BASSERT(false);
      return;
   }

   /*
   if (mpOverlapped != NULL)
   {
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }

      HRESULT hr = XGetOverlappedExtendedError(mpOverlapped);
      if (hr != ERROR_SUCCESS)
      {
         BASSERTM(FALSE, "A previous XSession call failed to complete");
      }
   }

   Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));
   mpOverlapped = &mOverlapped;
   */

   mHostingMode = newMode;

   //Just take out the flags we care about to begin with - then add them in if we need them
   mSessionFlags &= ~XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
   mSessionFlags &= ~XSESSION_CREATE_JOIN_VIA_PRESENCE_FRIENDS_ONLY;
   mSessionFlags &= ~XSESSION_CREATE_INVITES_DISABLED;

   if (!mJoinable)
   {
      mSessionFlags |= XSESSION_CREATE_INVITES_DISABLED;
      mSessionFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
   }
   else
   {
      if (mHostingMode==cBLiveSessionHostingModeOpen)
      {
         //nada
      }
      else if (mHostingMode==cBLiveSessionHostingModeFriendsOnly)
      {      
         mSessionFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_FRIENDS_ONLY;  
      }
      else if (mHostingMode==cBLiveSessionHostingModeInviteOnly)
      {
         mSessionFlags |= XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED;
      }  
   }
  
   BLiveSessionTaskSessionModify* newTask = new BLiveSessionTaskSessionModify(mSessionFlags, mMaxPublicSlots, 0);
   addAsyncTaskToQueue(newTask);
   /*
   DWORD retval = XSessionModify(mSession, mSessionFlags, mTotalSlots, 0, NULL);
   if (retval != ERROR_SUCCESS)
   {
      if (retval == ERROR_FUNCTION_FAILED)
      {
         //This method fails if we are reducing the number of slots to less that the number of peeps in the session currently
         //So this sets a flag so that after we process the next drop - we try again
         nlog(cLiveSessionNL, "BLiveSession::changeLiveMode - XSessionModify failed with ERROR_FUNCTION_FAILED, marking this call to be tried again next time a player drops - session %d", mSession);
         mResendSessionModifyOnNextDrop = true;
         return;
      }
      nlog(cLiveSessionNL, "BLiveSession::changeLiveMode - XSessionModify failed IMMEDIATELY with code 0x%08X on session %d", retval, mSession);
      BASSERT(false);
   }
   mResendSessionModifyOnNextDrop = false;
   */
}

//==============================================================================
//
//==============================================================================
void BLiveSession::modifySkill(DWORD count, XUID* xuidArray)
{
   BLiveSessionTaskSessionModifySkill* newTask = new BLiveSessionTaskSessionModifySkill(count, xuidArray);
   addAsyncTaskToQueue(newTask);
}

//==============================================================================
//
//==============================================================================
void BLiveSession::generateUserStatusSpam()
{
   int countUsers = 0;
   int countAddPending = 0;
   int countDropPending = 0;
   int countUnknown = 0;
   for (int i=0;i<LIVESESSION_USERSLOTS;i++)
   {
      if (mRemoteUsers[i].status != cLiveSessionUserStatusRecordUnused)
      {
         if (mRemoteUsers[i].status == cLiveSessionUserStatusInSession)
         {
            countUsers++;
         }
         else if (mRemoteUsers[i].status == cLiveSessionUserStatusAddPending)
         {
            countAddPending++;
         }
         else if (mRemoteUsers[i].status == cLiveSessionUserStatusDropPending)
         {
            countDropPending++;
         }
         else
         {
            countUnknown++;
         }
      }
   }

   nlog(cLiveSessionNL, "BLiveSession[0x%08X] - Remote connected user Live session status:", mSession);
   nlog(cLiveSessionNL, " -    Connections in session:%d", countUsers);
   nlog(cLiveSessionNL, " -    Adds pending:%d", countAddPending);
   nlog(cLiveSessionNL, " -    Drops pending:%d", countDropPending);
   nlog(cLiveSessionNL, " -    Status unknowns!:%d", countUnknown);
}

//==============================================================================
// Constructor for the user list entries
//==============================================================================
BLiveSessionUserEntry::BLiveSessionUserEntry() :
   xuid(0),
   privateSlots(1),
   status(cLiveSessionUserStatusRecordUnused)
{
   Utils::FastMemSet(&mOverlapped, 0, sizeof(XOVERLAPPED));
}








//==============================================================================
//
//==============================================================================
BLiveSessionAsyncTask::~BLiveSessionAsyncTask() 
{
   if (mStarted &&
      (getState() == BAsyncTask::cStateIdle) &&
      (!isComplete()))
   {
      //Async call was started but does not appear to have finished - kill it
      XCancelOverlapped(mpOverlapped);
      //Spin til its dead
      while (XHasOverlappedIoCompleted(mpOverlapped) == FALSE)
      {
         // insure that we have no outstanding XSession calls
         // FIXME include a timeout
         Sleep(1);
      }
   }
}



//==============================================================================
// BLiveSessionTaskSessionJoinLocal
//==============================================================================
BLiveSessionTaskSessionJoinLocal::BLiveSessionTaskSessionJoinLocal(const DWORD userCount, const DWORD *userIndexes, const BOOL *privateSlots)
{
   mUserCount = userCount;
   setRetries(0,0);
   Utils::FastMemCpy(&mUserIndices, userIndexes, sizeof(DWORD)*mUserCount);
   Utils::FastMemCpy(&mPrivateFlags, privateSlots, sizeof(BOOL)*mUserCount);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionJoinLocal::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionJoinLocal(mOwningLiveSession->getSessionHandle(), 1, mUserIndices, mPrivateFlags, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinLocal::doAsyncFunction[0x%08X] -- XSessionJoinLocal call failed with error 0x%08x",  mOwningLiveSession->getSessionHandle(), ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinLocal::doAsyncFunction[0x%08X] -- XSessionJoinLocal call launched", mOwningLiveSession->getSessionHandle());
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionJoinLocal::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinLocal::onSuccess[0x%08X] -- XSessionJoinLocal call completed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->localUserJoined();
   mOwningLiveSession->setState(BLiveSession::cSessionStateSessionRunning);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionJoinLocal::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinLocal::onSuccess[0x%08X] -- XSessionJoinLocal call failed, killing liveSession", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateError);
}



//==============================================================================
// BLiveSessionTaskSessionLeaveLocal - Drop the local user from the local session
//==============================================================================
BLiveSessionTaskSessionLeaveLocal::BLiveSessionTaskSessionLeaveLocal(const DWORD userCount, const DWORD *userIndexes)
{
   mUserCount = userCount;
   setRetries(0,0);
   Utils::FastMemCpy(&mUserIndices, userIndexes, sizeof(DWORD)*mUserCount);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionLeaveLocal::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionLeaveLocal(mOwningLiveSession->getSessionHandle(), 1, mUserIndices, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::doAsyncFunction[0x%08X] -- XSessionLeaveLocal call failed with error 0x%08x",  mOwningLiveSession->getSessionHandle(), ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::doAsyncFunction[0x%08X] -- XSessionLeaveLocal call launched", mOwningLiveSession->getSessionHandle());
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionLeaveLocal::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::onSuccess[0x%08X] -- XSessionLeaveLocal call completed", mOwningLiveSession->getSessionHandle());
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionLeaveLocal::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::onSuccess[0x%08X] -- XSessionLeaveLocal call failed xuid[%I64u]", mOwningLiveSession->getSessionHandle());
   //Lets try to leave local again in a blocking manner
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::onFailure[0x%08X] -- Trying XSessionLeaveLocal again in a blocking call", mOwningLiveSession->getSessionHandle()); 
   DWORD ret = XSessionLeaveLocal(mOwningLiveSession->getSessionHandle(), 1, mUserIndices, NULL);
   if (ret != ERROR_SUCCESS)
   {
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::onFailure[0x%08X] -- XSessionLeaveLocal blocking also failed, with code 0x%08x", mOwningLiveSession->getSessionHandle(), ret);
   }
   else
   {
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveLocal::onFailure[0x%08X] -- XSessionLeaveLocal blocking version worked.", mOwningLiveSession->getSessionHandle() );
   }
}



//==============================================================================
// BLiveSessionTaskSessionJoinRemote
//==============================================================================
BLiveSessionTaskSessionJoinRemote::BLiveSessionTaskSessionJoinRemote(const XUID xuid, const BOOL isPrivate) :
mXuid(xuid),
mIsPrivate(isPrivate)
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionJoinRemote::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionJoinRemote(mOwningLiveSession->getSessionHandle(), 1, &mXuid, &mIsPrivate, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinRemote::doAsyncFunction[0x%08X] -- XSessionJoinRemote call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinRemote::doAsyncFunction[0x%08X] -- XSessionJoinRemote call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionJoinRemote::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinRemote::onSuccess[0x%08X] -- XSessionJoinRemote call completed xuid[%I64u]", mOwningLiveSession->getSessionHandle(), mXuid);
   mOwningLiveSession->setUserState(mXuid, cLiveSessionUserStatusInSession);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionJoinRemote::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionJoinRemote::onSuccess[0x%08X] -- XSessionJoinRemote call failed xuid[%I64u]", mOwningLiveSession->getSessionHandle(), mXuid);
   mOwningLiveSession->setUserState(mXuid, cLiveSessionUserStatusAddFailed);
}



//==============================================================================
// BLiveSessionTaskSessionLeaveRemote
//==============================================================================
BLiveSessionTaskSessionLeaveRemote::BLiveSessionTaskSessionLeaveRemote(const XUID xuid) :
mXuid(xuid)
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionLeaveRemote::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionLeaveRemote(mOwningLiveSession->getSessionHandle(), 1, &mXuid, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveRemote::doAsyncFunction[0x%08X] -- XSessionLeaveRemote call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveRemote::doAsyncFunction[0x%08X] -- XSessionLeaveRemote call launched, mOwningLiveSession->getSessionHandle()" );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionLeaveRemote::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveRemote::onSuccess[0x%08X] -- XSessionLeaveRemote call completed xuid[%I64u]", mOwningLiveSession->getSessionHandle(), mXuid);
   mOwningLiveSession->setUserState(mXuid, cLiveSessionUserStatusDropCompleted);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionLeaveRemote::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionLeaveRemotes::onSuccess[0x%08X] -- XSessionLeaveRemote call failed xuid[%I64u]", mOwningLiveSession->getSessionHandle(), mXuid);
   mOwningLiveSession->setUserState(mXuid, cLiveSessionUserStatusDropFailed);
}




//==============================================================================
// BLiveSessionTaskSessionEnd
//==============================================================================
BLiveSessionTaskSessionEnd::BLiveSessionTaskSessionEnd()
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionEnd::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionEnd(mOwningLiveSession->getSessionHandle(), &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionEnd::doAsyncFunction[0x%08X] -- XSessionEnd call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionEnd::doAsyncFunction[0x%08X] -- XSessionEnd call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionEnd::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionEnd::onSuccess[0x%08X] -- XSessionEnd call completed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateEnd);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionEnd::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionEnd::onSuccess[0x%08X] -- XSessionEnd call failed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateEnd);
}



//==============================================================================
// BLiveSessionTaskSessionDelete
//==============================================================================
BLiveSessionTaskSessionDelete::BLiveSessionTaskSessionDelete()
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionDelete::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionDelete(mOwningLiveSession->getSessionHandle(), &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::doAsyncFunction[0x%08X] -- XSessionDelete call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::doAsyncFunction[0x%08X] -- XSessionDelete call launched", mOwningLiveSession->getSessionHandle());
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionDelete::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::onSuccess[0x%08X] -- XSessionDelete call completed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateSessionDeleted);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionDelete::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::onFailure[0x%08X] -- XSessionDelete call failed", mOwningLiveSession->getSessionHandle());   
   HRESULT hr = XGetOverlappedExtendedError(&mOverlapped);
   if (FAILED(hr))
   {
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::onFailure[0x%08X] -- XSessionDelete overlapped task failed with error 0x%08x", mOwningLiveSession->getSessionHandle(), hr);
   }
   //OMG HAX
   //Lets try to delete it again in a blocking manner
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::onFailure[0x%08X] -- Trying XSessionDelete again in a blocking call", mOwningLiveSession->getSessionHandle()); 
   DWORD ret = XSessionDelete(mOwningLiveSession->getSessionHandle(), NULL);
   if (ret != ERROR_SUCCESS)
   {
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::onFailure[0x%08X] -- XSessionDelete blocking also failed, with code 0x%08x", mOwningLiveSession->getSessionHandle(), ret);
   }
   else
   {
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionDelete::onFailure[0x%08X] -- XSessionDelete blocking version worked.", mOwningLiveSession->getSessionHandle() );
   }
   mOwningLiveSession->setState(BLiveSession::cSessionStateSessionDeleted);
}



//==============================================================================
// BLiveSessionTaskSessionStart
//==============================================================================
BLiveSessionTaskSessionStart::BLiveSessionTaskSessionStart()
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionStart::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionStart(mOwningLiveSession->getSessionHandle(), 0, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionStart::doAsyncFunction[0x%08X] -- XSessionStart call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionStart::doAsyncFunction[0x%08X] -- XSessionStart call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionStart::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionStart::onSuccess[0x%08X] -- XSessionStart call completed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateInGame);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionStart::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionStart::onSuccess[0x%08X] -- XSessionStart call failed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateError);
}




//==============================================================================
// BLiveSessionTaskSessionModify
//==============================================================================
BLiveSessionTaskSessionModify::BLiveSessionTaskSessionModify(const DWORD flags, const DWORD maxPublicSlots, const DWORD maxPrivateSlots)
{
   mFlags = flags;
   mMaxPublicSlots = maxPublicSlots;
   mMaxPrivateSlots = maxPrivateSlots;
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionModify::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionModify(mOwningLiveSession->getSessionHandle(), mFlags, mMaxPublicSlots, mMaxPrivateSlots, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionModify::doAsyncFunction[0x%08X] -- XSessionModify call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionModify::doAsyncFunction[0x%08X] -- XSessionModify call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionModify::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionModify::onSuccess[0x%08X] -- XSessionModify call completed", mOwningLiveSession->getSessionHandle());
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionModify::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionModify::onSuccess[0x%08X] -- XSessionModify call failed, killing liveSession", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->setState(BLiveSession::cSessionStateError);
}



//==============================================================================
// BLiveSessionTaskSessionArbitrationRegister
//==============================================================================
BLiveSessionTaskSessionArbitrationRegister::BLiveSessionTaskSessionArbitrationRegister(const ULONGLONG sessionNonce, const DWORD results, XSESSION_REGISTRATION_RESULTS* pArbitrationResults )
{
   mSessionNonce = sessionNonce;
   mResults = results;
   mpArbitrationResults = pArbitrationResults;
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionArbitrationRegister::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionArbitrationRegister(mOwningLiveSession->getSessionHandle(), 0, mSessionNonce, &mResults, mpArbitrationResults, mpOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionArbitrationRegister::doAsyncFunction[0x%08X] -- XSessionArbitrationRegister call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionArbitrationRegister::doAsyncFunction[0x%08X] -- XSessionArbitrationRegister call launched, nonce [%I64u]", mOwningLiveSession->getSessionHandle(), mSessionNonce );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionArbitrationRegister::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionArbitrationRegister::onSuccess[0x%08X] -- XSessionArbitrationRegister call completed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->arbitrationCompleted(true);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionArbitrationRegister::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionArbitrationRegister::onSuccess[0x%08X] -- XSessionArbitrationRegister call failed, killing liveSession", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->arbitrationCompleted(false);
}



//==============================================================================
// BLiveSessionTaskSessionWriteStats
//==============================================================================
BLiveSessionTaskSessionWriteStats::BLiveSessionTaskSessionWriteStats(const XUID xuid) :
mXuid(xuid),
mViewCount(0)
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionWriteStats::doAsyncFunction()
{
   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   nlog(cLiveSessionNL, "STATSWRITER SPAM       XUID [%I64u]", mXuid );
   nlog(cLiveSessionNL, "   Viewcount:%i", mViewCount);
   for (uint i=0;i<mViewCount;i++)
   {
      nlog(cLiveSessionNL, "      View:%i ID:%x", i, mViews[i].dwViewId );
      for (uint j=0;j<mViews[i].dwNumProperties;j++)
      {
         BSimString temp;
         temp = "(not mapped)";
         if (mViews[i].pProperties[j].value.type == XUSER_DATA_TYPE_INT32)
         {
            temp.format("(int32)%d", mViews[i].pProperties[j].value.nData );
         }
         else if (mViews[i].pProperties[j].value.type == XUSER_DATA_TYPE_INT64)
         {
            temp.format("(int64)%d", mViews[i].pProperties[j].value.i64Data );
         }
         else if (mViews[i].pProperties[j].value.type == XUSER_DATA_TYPE_DOUBLE)
         {
            temp.format("(double)%f", mViews[i].pProperties[j].value.dblData );
         }
         nlog(cLiveSessionNL, "        Property:%i ID:%x Value:%s", j, mViews[i].pProperties[j].dwPropertyId, temp.getPtr());
      }
   }

   DWORD ret = XSessionWriteStats(mOwningLiveSession->getSessionHandle(), mXuid, mViewCount, mViews, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionWriteStats::doAsyncFunction[0x%08X] -- XSessionWriteStats call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionWriteStats::doAsyncFunction[0x%08X] -- XSessionWriteStats call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionWriteStats::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionWriteStats::onSuccess[0x%08X] -- XSessionWriteStats call completed xuid[%I64u]", mOwningLiveSession->getSessionHandle(), mXuid);
   //mOwningLiveSession->setUserState(mXuid, cLiveSessionUserStatusInSession);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionWriteStats::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionWriteStats::onSuccess[0x%08X] -- XSessionWriteStats call failed xuid[%I64u]", mOwningLiveSession->getSessionHandle(), mXuid);
}



//==============================================================================
// BLiveSessionTaskSessionSetContext
//==============================================================================
BLiveSessionTaskSessionSetContext::BLiveSessionTaskSessionSetContext(const DWORD controllerID, const DWORD contextID, const DWORD contextValue) :
   mControllerID(controllerID),
   mContextID(contextID),
   mContextValue(contextValue)
{
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionSetContext::doAsyncFunction()
{
   //This should only be called before the session is created
   BASSERT(mOwningLiveSession->getSessionHandle());

   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XUserSetContextEx(mControllerID, mContextID, mContextValue, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionSetContext::doAsyncFunction[0x%08X] -- XUserSetContextEx call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionSetContext::doAsyncFunction[0x%08X] -- XUserSetContextEx call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionSetContext::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionSetContext::onSuccess[0x%08X] -- XUserSetContextEx call completed", mOwningLiveSession->getSessionHandle());
   //Nothing here - queue processing just goes on
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionSetContext::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionSetContext::onSuccess[0x%08X] -- XUserSetContextEx call failed", mOwningLiveSession->getSessionHandle());
   //This is ONLY used when we are starting up session, so its failure is fatal
   mOwningLiveSession->setState(BLiveSession::cSessionStateError);
}



//==============================================================================
// BLiveSessionTaskSessionCreate
//==============================================================================
BLiveSessionTaskSessionCreate::BLiveSessionTaskSessionCreate(const DWORD flags, const DWORD userIndex, const DWORD maxPublicSlots, const DWORD maxPrivateSlots):
 mFlags(flags),
 mUserIndex(userIndex),
 mMaxPublicSlots(maxPublicSlots),
 mMaxPrivateSlots(maxPrivateSlots),
 mSessionNonce(0),
 mSessionHandle(NULL)
{   
   Utils::FastMemSet(&mSessionInfo, 0, sizeof(XSESSION_INFO));
   mSlots[ cSessionSlotsTotalPublic    ] = mMaxPublicSlots;
   mSlots[ cSessionSlotsTotalPrivate   ] = mMaxPrivateSlots;
   mSlots[ cSessionSlotsFilledPublic   ] = 0;
   mSlots[ cSessionSlotsFilledPrivate  ] = 0;
   setRetries(0,0);
}

 //==============================================================================
 // BLiveSessionTaskSessionCreate
 //==============================================================================
 BLiveSessionTaskSessionCreate::BLiveSessionTaskSessionCreate(const DWORD flags, const DWORD userIndex, const DWORD maxPublicSlots, const DWORD maxPrivateSlots, ULONGLONG nonce, const XNADDR& hostTargetXNADDR, const XNKID& hostKID, const XNKEY& hostKEY):
    mFlags(flags),
    mUserIndex(userIndex),
    mMaxPublicSlots(maxPublicSlots),
    mMaxPrivateSlots(maxPrivateSlots),
    mSessionNonce(nonce),
    mSessionHandle(NULL)
{   
    Utils::FastMemSet(&mSessionInfo, 0, sizeof(XSESSION_INFO));
    mSessionInfo.hostAddress = hostTargetXNADDR;
    mSessionInfo.keyExchangeKey = hostKEY;
    mSessionInfo.sessionID = hostKID;

    mSlots[ cSessionSlotsTotalPublic    ] = mMaxPublicSlots;
    mSlots[ cSessionSlotsTotalPrivate   ] = mMaxPrivateSlots;
    mSlots[ cSessionSlotsFilledPublic   ] = 0;
    mSlots[ cSessionSlotsFilledPrivate  ] = 0;
    setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionCreate::doAsyncFunction()
{
   //This should only be called before the session is created
   BASSERT(mOwningLiveSession->getSessionHandle());

   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionCreate(mFlags, mUserIndex, mSlots[ cSessionSlotsTotalPublic ], mSlots[ cSessionSlotsTotalPrivate ], &mSessionNonce, &mSessionInfo, &mOverlapped, &mSessionHandle);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionCreate::doAsyncFunction[0x%08X] -- XSessionCreate call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionCreate::doAsyncFunction[0x%08X] -- XSessionCreate call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionCreate::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionCreate::onSuccess[0x%08X] -- XSessionCreate call completed", mOwningLiveSession->getSessionHandle());
   mOwningLiveSession->sessionCreated(mSessionNonce, mSessionHandle, &mSessionInfo);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionCreate::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionCreate::onSuccess[0x%08X] -- XSessionCreate call failed", mOwningLiveSession->getSessionHandle());
   //This is ONLY used when we are starting up session, so its failure is fatal
   mOwningLiveSession->setState(BLiveSession::cSessionStateError);
}



//==============================================================================
// BLiveSessionTaskSessionModifySkill
//==============================================================================
BLiveSessionTaskSessionModifySkill::BLiveSessionTaskSessionModifySkill(const DWORD xuidCount, const XUID* pXuidArray):
mXuidCount(xuidCount)
{   
   BASSERT(xuidCount<=cBLiveSessionTaskSessionModifySkillMaxCount);

   Utils::FastMemCpy(&mXuidArray, pXuidArray, sizeof(XUID)*xuidCount);
   setRetries(0,0);
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionTaskSessionModifySkill::doAsyncFunction()
{
   //This should only be called before the session is created
   BASSERT(mOwningLiveSession->getSessionHandle());

   if (!BLiveSessionAsyncTask::doAsyncFunction())
   {
      return false;
   }

   DWORD ret = XSessionModifySkill(mOwningLiveSession->getSessionHandle(), mXuidCount, (XUID*)&mXuidArray, &mOverlapped);
   if( ret != ERROR_IO_PENDING )
   {
      //Call failed
      nlog(cLiveSessionNL, "BLiveSessionTaskSessionModifySkill::doAsyncFunction[0x%08X] -- XSessionModifySkill call failed with error 0x%08x", mOwningLiveSession->getSessionHandle(),  ret );
      setState(cStateError, cErrorAPIFailure);
      return false;
   }
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionModifySkill::doAsyncFunction[0x%08X] -- XSessionModifySkill call launched", mOwningLiveSession->getSessionHandle() );
   return true;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionModifySkill::onSuccess()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionModifySkill::onSuccess[0x%08X] -- XSessionModifySkill call completed", mOwningLiveSession->getSessionHandle());
}

//==============================================================================
//
//==============================================================================
void BLiveSessionTaskSessionModifySkill::onFailure()
{
   nlog(cLiveSessionNL, "BLiveSessionTaskSessionModifySkill::onSuccess[0x%08X] -- XSessionModifySkill call failed", mOwningLiveSession->getSessionHandle());
   //Not good, but not fatal
}







//==============================================================================
// BLiveSessionLeaderboardOnly stuff *******************************************
//==============================================================================

//==============================================================================
//
//==============================================================================
BLiveSessionLeaderboardOnly::BLiveSessionLeaderboardOnly(int nOwnerController, XUID owningXUID, UINT slots, UINT gameModeIndex) :
mpLiveSession(NULL),
mPostersControllerID(nOwnerController),
mPostersXUID(owningXUID),
mPlayerCount(slots),
mGameModeIndex(gameModeIndex),
mPlayersReportingEndOfGame(0),
mState(cLiveSessionLeaderboardOnlyStateNoSession),
mData_playerXUID(0),
mData_playerWonGame(0),
mData_teamID(0),
mData_leaderIndex(0),
mData_score(0),
mData_gamePlayTime(0)
{   
   nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::BLiveSessionLeaderboardOnly - Creating the leaderboard only session");
#ifdef BUILD_DEBUG
   if (gConfig.isDefined(cConfigMoreLiveOutputSpam))
   {      
      XDebugSetSystemOutputLevel(HXAMAPP_XGI, 5);
      XDebugSetSystemOutputLevel(HXAMAPP_XLIVEBASE, 5);
      XDebugSetSystemOutputLevel(HXAMAPP_XAM, 3);
   }   
#endif
}

//==============================================================================
//
//==============================================================================
BLiveSessionLeaderboardOnly::~BLiveSessionLeaderboardOnly()
{
   nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::~BLiveSessionLeaderboardOnly - Deleting the leaderboard only session");
   if (mpLiveSession)
   {      
      delete mpLiveSession;
      mpLiveSession = NULL;
   }
}
//==============================================================================
//
//==============================================================================
void BLiveSessionLeaderboardOnly::shutDown()
{
   if (mState!=cLiveSessionLeaderboardOnlyStateComplete)
   {
      nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::shutDown - killing myself");
      mpLiveSession->deinit();
      mState = cLiveSessionLeaderboardOnlyStateDeleteing;
   }
}

//==============================================================================
//
//==============================================================================
void BLiveSessionLeaderboardOnly::update()
{
   if (!mpLiveSession)
   {
      return;
   }
   mpLiveSession->update();

   switch (mState) 
   {
      case (cLiveSessionLeaderboardOnlyStateCreatingSession) :
         {
            if (mpLiveSession->isSessionValid())
            {
               nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::update - Starting the session");
               mpLiveSession->startGame();
               mState = cLiveSessionLeaderboardOnlyStateStarting;
            }
            break;
         }
      case (cLiveSessionLeaderboardOnlyStateStarting) :
         {
            if (mpLiveSession->sessionIsInGame())
            {
               mState = cLiveSessionLeaderboardOnlyStateIdle;
               if (mData_playerXUID!=0)
               {
                  nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::update - Posting the players data");
                  //I already have the user data to post - lets do it
                  mpLiveSession->setPlayerWon(mData_playerXUID, mData_playerWonGame, mData_teamID, mData_leaderIndex, mData_score, mData_gamePlayTime );                  
                  mpLiveSession->endGame();
                  mState = cLiveSessionLeaderboardOnlyStateEndPending;
               }
            }
            break;
         }
      case (cLiveSessionLeaderboardOnlyStateEndPending) :
         {
            if (mpLiveSession->sessionIsGameOver())
            {
               nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::update - Cleaning up");
               mpLiveSession->deinit();
               mState = cLiveSessionLeaderboardOnlyStateDeleteing;
            }
            break;
         }
      case (cLiveSessionLeaderboardOnlyStateDeleteing) :
         {
            if (mpLiveSession->isShutdown())
            {
               delete mpLiveSession;
               mpLiveSession = NULL;
               mState = cLiveSessionLeaderboardOnlyStateComplete;
               nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::update - Cleanup complete, ready for delete");
            }
         }
   }
}

//==============================================================================
//
//==============================================================================
bool BLiveSessionLeaderboardOnly::canBeDeleted()
{
   return (mState==cLiveSessionLeaderboardOnlyStateComplete);
}

//==============================================================================
//
//==============================================================================
void BLiveSessionLeaderboardOnly::storeStatsGameSettings( BLiveSessionGameClassification gameClass, bool partyTeam, uint campaignMapIndex, uint gameTypeIndex, uint difficulty, bool statsSaveGameRule )
{
   //At this point - do we NEED a stand along leaderboard?
   //TODO - don't filter out custom compstomps  
   if (gameClass!=cLiveSessionGameClassificationCampaign)
   {
      nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::storeStatsGameSettings - Ignoring settings, game is not of type Campaign");
      mState = cLiveSessionLeaderboardOnlyStateComplete;
      return;
   }

   //Make one and set its settings
   nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::storeStatsGameSettings - Saving game meta data for the leaderboard");
   mpLiveSession = new BLiveSession(mPostersControllerID, mPlayerCount, mGameModeIndex);
   BASSERT(mpLiveSession);
   if (mpLiveSession)
   {
      mpLiveSession->storeStatsGameSettings(gameClass, partyTeam, campaignMapIndex, gameTypeIndex, difficulty, statsSaveGameRule);
   }
   mState = cLiveSessionLeaderboardOnlyStateCreatingSession;
}

//==============================================================================
//
//==============================================================================
void BLiveSessionLeaderboardOnly::setPlayerWon(XUID playerXUID, BOOL playerWonGame, long teamID, uint leaderIndex, uint score, uint gamePlayTime)
{
   if (playerXUID!= mPostersXUID)
   {
      //This only works for non-arbitrated leaderboards, where each person reports their own scores only
      return;
   }

   //Store it
   nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::setPlayerWon - Storing local player win data");
   mData_playerXUID = playerXUID;
   mData_playerWonGame = playerWonGame;
   mData_teamID = teamID;
   mData_leaderIndex = leaderIndex;
   mData_score = score;
   mData_gamePlayTime = gamePlayTime;

   //See if the live session is in a state where I can send this
   if (mState==cLiveSessionLeaderboardOnlyStateIdle)
   {
      nlog(cLiveSessionNL, "BLiveSessionLeaderboardOnly::setPlayerWon - LB was in state Idle, going ahead and writing data to the Live session");
      mPlayersReportingEndOfGame++;
      mpLiveSession->setPlayerWon(mData_playerXUID, mData_playerWonGame, mData_teamID, mData_leaderIndex, mData_score, mData_gamePlayTime );      
      mpLiveSession->endGame();
      mState = cLiveSessionLeaderboardOnlyStateEndPending;
   }
}
