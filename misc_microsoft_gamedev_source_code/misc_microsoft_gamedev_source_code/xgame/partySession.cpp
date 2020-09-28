//==============================================================================
// BPartySession.cpp
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

#include "Common.h"
#include "PartySession.h"
#include "LiveSystem.h"
#include "liveSession.h"
#include "liveSessionSearch.h"
#include "liveVoice.h"
#include "matchMakingHopperList.h"
#include "XLastGenerated.h"          
#include "mpcommheaders.h"
#include "commlog.h"
#include "config.h"
#include "econfigenum.h"
#include "consoleOutput.h"
#include "notification.h"
#include "xmlreader.h"
#include "xmlwriter.h"
#include "config.h"
#include "econfigenum.h"

#include "usermanager.h"
#include "user.h"
#include "userprofilemanager.h"

//==============================================================================
// FIXME-COOP - BPartySessionPlayerSettings was not being initialized, what are
//                good defaults?
//==============================================================================
BPartySessionPlayerSettings::BPartySessionPlayerSettings() : 
   mCiv(-1),
   mLeader(-1),
   mTeam(-1),
   mSlot(-1),
   mPartyRoomMode(0),
   mConnectionState(0),
   mStatusFlags(0),
   mCampaignBits(0)
{
}

//==============================================================================
// FIXME-COOP - BPartySessionPartyMember was not being initialized, what are
//                good defaults?
//==============================================================================
BPartySessionPartyMember::BPartySessionPartyMember() :
   mXuid(INVALID_XUID),
   mClientID(cMPInvalidClientID),
   mLastTimeUpdated(0),
   mState(cBPartySessionMemberStateIdle),
   mMemberIndex(UINT8_MAX),
   mPort(-1),
   mQoSValue(0),
   mLiveSessionJoined(false),
   mLevelLoaded(false),
   mRestrictedNAT(false),
   mSlotChanged(false),
   mGamerPicLoaded(false)
{
}

//==============================================================================
// 
//==============================================================================
BPartySession::BPartySession(BLiveVoice* pVoice, const BPartySessionPlayerSettings& defaultMemberSettings) :
   mState(cBPartySessionStateNone),
   mpSession(NULL),
   mpLiveSession(NULL),
   mPort(0),
   mPublicSlots(0),
   mPartyLocked(false),
   mLocalChecksum(NULL),
   mSessionInterface(NULL),
   mSettingsComplete( false ),
   mSessionConnected( false ),
   mGameConnected( false ),
   mGameEnded( false ),
   mUpdateLocalSettings( true ),
   mLockedPlayers(0),
   mLaunchCountdown(0),
   mLaunchRequestTime(0),
   mLaunchLastUpdate(0),
   mLocalControllerID(XUSER_MAX_COUNT),
   mAbort(FALSE),
   mLocalXuid(0),
   mStartupMode(cBPartySessionStartupNone),
   mpVoice(pVoice),
   mQoSResponseDataSize(0),
   mQoSResponding(FALSE),
   mGameTypeIndex(0),
   mLocalMemberIndex(cBPartySessionInvalidMemberID),
   mLanSecurityKeyRegistered(false),
   mReactivatePartyLogic(false),
   mCurrentMatchMakingStatusCode(BLiveMatchMaking::cBLiveMatchMakingStatusCodeNone)
{
   BASSERT(mpVoice);

   Utils::FastMemSet(&mLocalXNKey, NULL, sizeof(mLocalXNKey));
   Utils::FastMemSet(&mLocalXNKID, NULL, sizeof(mLocalXNKID));
   Utils::FastMemSet(&mLiveJoinGameTarget, NULL, sizeof(mLiveJoinGameTarget));
   Utils::FastMemSet(&mQoSResponseData, NULL, sizeof(mQoSResponseData));
   Utils::FastMemCpy(&mDefaultPlayerSetting, &defaultMemberSettings, sizeof(BPartySessionPlayerSettings));
   Utils::FastMemCpy(&mLocalMemberInitialSettings, &defaultMemberSettings, sizeof(BPartySessionPlayerSettings));   
   Utils::FastMemSet(&mCurrentHostSettings, NULL, sizeof(BPartySessionHostSettings));
   mCurrentHostSettings.mLiveMode = cBLiveSessionHostingModeOpen;

   //Set up members array
   for (uint i=0; i < cPartySessionMaxUsers; i++)
   {
      //Utils::FastMemCpy(&mMembers[i].mSettings, &mDefaultPlayerSetting, sizeof(BPartySessionPlayerSettings));
      mMembers[i].mSettings = mDefaultPlayerSetting;
      mMembers[i].mClientID = cMPInvalidClientID;
      mMembers[i].mXuid = INVALID_XUID;
      mMembers[i].mPort = -1;
   }

   //Store the checksum for this current running game
   mLocalChecksum = gLiveSystem->getMPSession()->getCachedGameChecksum();
   BASSERT(mLocalChecksum);

   registerMPCommHeaders();
}

//==============================================================================
//
//==============================================================================
BPartySession::~BPartySession()
{
   BASSERT(mState==cBPartySessionStateFinished);
   shutdown();
}

//==============================================================================
//
//==============================================================================
void BPartySession::startUpHost(BPartySessionInterface* pInt, const BPartySessionPlayerSettings& playerSettings, const BPartySessionHostSettings& initialHostSettings, DWORD controllerID)
{
   if (mState != cBPartySessionStateNone)
   {
      return;
   }

   if (!pInt)
   {
      BFAIL( "BPartySession::startUp - **ERROR** There must be a GameInterface and SessionInterface to startup this object." );
      return;
   }

   BASSERT(mpVoice);

   //Re-initialize stuff
   Utils::FastMemSet(&mLocalXNKey, NULL, sizeof(mLocalXNKey));
   Utils::FastMemSet(&mLocalXNKID, NULL, sizeof(mLocalXNKID));
   Utils::FastMemSet(&mLiveJoinGameTarget, NULL, sizeof(mLiveJoinGameTarget));

   setState(cBPartySessionStateStartingWaitingForXNAddr);
   mSessionInterface = pInt;
   mLocalMemberIndex = 0;
   setControllerID(controllerID);
   mPublicSlots = initialHostSettings.mNumPlayers;
   mMembers[0].mLastTimeUpdated = timeGetTime();
   Utils::FastMemCpy(&mMembers[0].mSettings, &playerSettings, sizeof(BPartySessionPlayerSettings));
   Utils::FastMemCpy(&mLocalMemberInitialSettings, &playerSettings, sizeof(BPartySessionPlayerSettings));
   //mMembers[0].mSettings = playerSettings;
   mCurrentHostSettings = initialHostSettings;
   mStartupMode = cBPartySessionStartupHosting;

   nlog(cMPPartySystemCL, "BPartySession::startUpHost - startup complete");
   //Rest of startup code happens in update when we have an XNAddr
}

//==============================================================================
//
//==============================================================================
void BPartySession::startUpJoin(BPartySessionInterface* pInt, const BPartySessionPlayerSettings& playerSettings, DWORD controllerID, XINVITE_INFO* inviteInfo)
{
   BASSERT(!gLiveSystem->getMPSession()->isInLANMode());

   if (mState != cBPartySessionStateNone)
   {
      return;
   }

   if (!pInt)
   {
      BFAIL( "BPartySession::startUp - **ERROR** There must be a GameInterface and SessionInterface to startup this object." );
      return;
   }

   BASSERT(mpVoice);

   //Re-initialize stuff
   Utils::FastMemSet(&mLocalXNKey, NULL, sizeof(mLocalXNKey));
   Utils::FastMemSet(&mLocalXNKID, NULL, sizeof(mLocalXNKID));
   Utils::FastMemSet(&mLiveJoinGameTarget, NULL, sizeof(mLiveJoinGameTarget));
   Utils::FastMemCpy(&mHostTarget,inviteInfo,sizeof(mHostTarget));
   Utils::FastMemSet(&mCurrentHostSettings, NULL, sizeof(BPartySessionHostSettings));

   setState(cBPartySessionStateStartingWaitingForXNAddr);
   mSessionInterface = pInt;
   mLocalMemberIndex = 0;
   setControllerID(controllerID);
   mMembers[0].mLastTimeUpdated = timeGetTime();
   Utils::FastMemCpy(&mMembers[0].mSettings, &playerSettings, sizeof(BPartySessionPlayerSettings));
   Utils::FastMemCpy(&mLocalMemberInitialSettings, &playerSettings, sizeof(BPartySessionPlayerSettings));
   //mMembers[0].mSettings = playerSettings;
   mStartupMode = cBPartySessionStartupJoining;
   //gLiveSystem->setLiveRequired(TRUE);
   nlog(cMPPartySystemCL, "BPartySession::startUpJoin - startup complete");
   //Rest of startup code happens in update when we have an XNAddr
}

//==============================================================================
// LAN version
//==============================================================================
void BPartySession::startUpJoin(BPartySessionInterface* pInt, const BPartySessionPlayerSettings& playerSettings, DWORD controllerID, const BLanGameInfo& lanInfo)
{
   BASSERT(gLiveSystem->getMPSession()->isInLANMode());

   if (mState != cBPartySessionStateNone)
   {
      return;
   }

   if (!pInt)
   {
      BFAIL( "BPartySession::startUp - **ERROR** There must be a GameInterface and SessionInterface to startup this object." );
      return;
   }

   BASSERT(mpVoice);

   //Re-initialize stuff
   Utils::FastMemSet(&mLocalXNKey, NULL, sizeof(mLocalXNKey));
   Utils::FastMemSet(&mLocalXNKID, NULL, sizeof(mLocalXNKID));
   Utils::FastMemSet(&mLiveJoinGameTarget, NULL, sizeof(mLiveJoinGameTarget));
   Utils::FastMemSet(&mCurrentHostSettings, NULL, sizeof(BPartySessionHostSettings));

   setState(cBPartySessionStateStartingWaitingForXNAddr);
   mSessionInterface = pInt;
   mLocalMemberIndex = 0;
   setControllerID(controllerID);
   mMembers[0].mLastTimeUpdated = timeGetTime();
   Utils::FastMemCpy(&mMembers[0].mSettings, &playerSettings, sizeof(BPartySessionPlayerSettings));
   Utils::FastMemCpy(&mLocalMemberInitialSettings, &playerSettings, sizeof(BPartySessionPlayerSettings));
   //mMembers[0].mSettings = playerSettings;
   mLanInfo = lanInfo;
   mStartupMode = cBPartySessionStartupJoining;
   
   //gLiveSystem->setLiveRequired(FALSE);
   nlog(cMPPartySystemCL, "BPartySession::startUpJoin - startup complete");
   //Rest of startup code happens in update when we have an XNAddr
}

//==============================================================================
// Clean up everything
//==============================================================================
void BPartySession::shutdown()
{
   nlog(cMPPartySystemCL, "BPartySession::shutdown - shutdown requested");
   reset();
   mSessionInterface = NULL;
   if (mState==cBPartySessionStateShuttingDownToReset)
   {
      mState=cBPartySessionStateShuttingDown;
   }
}

//==============================================================================
// Disconnects from any current session and cleans up without destroying connection to its creator
//==============================================================================
void BPartySession::reset()
{
   if (mState>=cBPartySessionStateShuttingDown)
   {
      //Don't re-reset stuff
      return;
   }

   //Kill Qos response if it is up
   setQoSNotification(FALSE);

   if (mpSession)
   {
      if (isHosting())
      {
         mpSession->disconnect(BSession::cHostCancelledGame);
      } 
      else
      {
         mpSession->disconnect(BSession::cNormal);
      }
      mpSession->removeObserver(this);
      mpSession->dispose();
      HEAP_DELETE(mpSession, gNetworkHeap);
      mpSession = NULL;
   }

   if (mLanSecurityKeyRegistered)
   {
      XNetUnregisterKey( &mLocalXNKID );
      mLanSecurityKeyRegistered = FALSE;
   }

   mPartyLocked = false;
   mSettingsComplete = false;
   mSessionConnected = false;
   mGameConnected = false;
   mGameEnded = false;
   mUpdateLocalSettings = true;
   mLaunchCountdown = 0;
   mLaunchRequestTime = 0;
   mLaunchLastUpdate = 0;
   mAbort = FALSE;
   mLocalControllerID = XUSER_MAX_COUNT;
   mPort = 0;
   mLocalXuid = 0;
   mPublicSlots = 0;
   mLocalChecksum = NULL;
   mLocalGamertag = sEmptySimString;
   mReactivatePartyLogic = false;
   Utils::FastMemSet(&mCurrentHostSettings, NULL, sizeof(BPartySessionHostSettings));

   if (mpLiveSession)
   {
      mpLiveSession->deinit();
      //mState = cBPartySessionStateShuttingDownToReset;
   }
//    else
//    {
//       mState = cBPartySessionStateStartingWaitingForXNAddr;
//    }  
   mState = cBPartySessionStateShuttingDownToReset;
}

//==============================================================================
//Returns the number of connected people in the party
//==============================================================================
uint BPartySession::getPartyCount()
{
   uint count = 0;
   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      //if (mMembers[i].mClientID != cMPInvalidClientID)
      if (mMembers[i].mXuid != INVALID_XUID)
      {
         count++;
      }
   }
   return count;
}

//==============================================================================
//For a client, if they think they are currently matchmaking (via a msg from the party host) then this is valid
//==============================================================================
bool BPartySession::thisClientThinksItIsMatchmaking()
{
   if ((mCurrentMatchMakingStatusCode==BLiveMatchMaking::cBLiveMatchMakingStatusCodeNone) ||
       (mCurrentMatchMakingStatusCode==BLiveMatchMaking::cBLiveMatchMakingStatusCodeSearchStoppedCode))
   {
      return false;
   }
   return true;
}

//==============================================================================
//Returns the player settings for a the first valid record for that index
//  This lets the internal array have open slots yet still be indexed as if it was sequential
//==============================================================================
BPartySessionPlayerSettings* BPartySession::getPlayerSettings( uint index )
{
   BPartySessionPartyMember* member = getPartyMember( index );
   if (member)
   {
      return &member->mSettings;
   }
   else
   {
      return NULL;
   }
}

//==============================================================================
//The host calls this to broadcast out to everyone a particular member's settings
//==============================================================================
bool BPartySession::broadcastChangeSettings(const BPartySessionPlayerSettings& playerSettings, XUID changedXUID)
{
   if (!mpSession) 
   {
      //No Session? - Log error
      nlog(cMPPartySystemCL, "BPartySession::changeSettings - called but I have no party BSession");
      return false;
   }

   BASSERT(isHosting());

//-- FIXING PREFIX BUG ID 1449
   const BPartySessionPartyMember* dude = NULL;
//--
   dude = findPartyMemberByXUID(changedXUID);
   BASSERT(dude);

   nlog(cMPPartySystemCL, "BPartySession::broadcastChangeSettings - Sending out settings for player %s to everyone", dude->mGamerTag.getPtr() );
   BPartyMemberSettingsPacket packet(dude->mXuid, playerSettings.mCiv, playerSettings.mLeader, playerSettings.mTeam, playerSettings.mSlot, playerSettings.mConnectionState, playerSettings.mPartyRoomMode, playerSettings.mStatusFlags, playerSettings.mPlayerRank.mValue, playerSettings.mCampaignBits, BPacketType::cPartyMemberSettingsEchoPacket);
   mpSession->SendPacket(packet);

   return true;
}

//==============================================================================
//Call this to change settings for an AI to new values and send those out (host only call)
//==============================================================================
bool BPartySession::changeAIPlayerSettings(XUID aiXuid, const BPartySessionPlayerSettings& playerSettings)
{
   if (!mpSession) 
   {
      //No Session? - Log error
      nlog(cMPPartySystemCL, "BPartySession::changeAIPlayerSettings - called but I have no party BSession");
      return false;
   }

   if (!isHosting())
   {
      BASSERT(false);
      return false;
   }  

   nlog(cMPPartySystemCL, "BPartySession::changeAIPlayerSettings - Sending out settings for player Xuid:%I64u to the host", aiXuid);
   BPartyMemberSettingsPacket packet(aiXuid, playerSettings.mCiv, playerSettings.mLeader, playerSettings.mTeam, playerSettings.mSlot, playerSettings.mConnectionState, playerSettings.mPartyRoomMode, playerSettings.mStatusFlags, playerSettings.mPlayerRank.mValue, playerSettings.mCampaignBits);
   mpSession->SendPacketTo(getSession()->getHostMachine(), packet);

   return true;
}

//==============================================================================
//Call this to change your settings to new values and send those to the host
// FIXME-COOP - sending party member settings packet
//==============================================================================
bool BPartySession::changeSettings(BPartySessionPlayerSettings& playerSettings)
{
   //DEPRICATED
   //All calls to change settings must now specify the XUID for the settings they are change
   //We can't assume we can find 'the local dude' anymore
   BASSERT(false);
   return false;
}

//==============================================================================
//Call this to change your settings to new values and send those to the host
// FIXME-COOP - sending party member settings packet
//==============================================================================
bool BPartySession::changeSettings(XUID xuid, BPartySessionPlayerSettings& playerSettings)
{
   if (!mpSession) 
   {
      //No Session? - Log error
      nlog(cMPPartySystemCL, "BPartySession::changeSettings - called but I have no party BSession");
      return false;
   }

   //No need to continue if the host is gone
   BMachine* pHostMachine = mpSession->getHostMachine();
   if (!pHostMachine)
   {
      nlog(cMPPartySystemCL, "BPartySession::changeSettings - called but I have no party host");
      return false;
   }

//-- FIXING PREFIX BUG ID 1450
   const BMachine* pLocalMachine = mpSession->getLocalMachine();
//--
   if (!pLocalMachine)
      return false;

   //Get my local settings
//-- FIXING PREFIX BUG ID 1451
   const BPartySessionPartyMember* pMember = findPartyMemberByXUID(xuid);
//--
   BDEBUG_ASSERTM(pMember, "failed to find party member for the local client ID");
   if (pMember == NULL)
      return false;
 
   nlog(cMPPartySystemCL, "BPartySession::changeSettings - Sending out settings for player %s to the host", pMember->mGamerTag.getPtr());
   BPartyMemberSettingsPacket packet(pMember->mXuid, playerSettings.mCiv, playerSettings.mLeader, playerSettings.mTeam, playerSettings.mSlot, playerSettings.mConnectionState, playerSettings.mPartyRoomMode, playerSettings.mStatusFlags, playerSettings.mPlayerRank.mValue, playerSettings.mCampaignBits);
   mpSession->SendPacketTo(pHostMachine, packet);

   return true;
}

//==============================================================================
//
//==============================================================================
void BPartySession::changeHostSettings(const BPartySessionHostSettings& newSettings)
{
   BASSERT(isHosting());
   mCurrentHostSettings = newSettings; 
   BPartyHostSettingsPacket packet(mCurrentHostSettings.mRandomTeam, mCurrentHostSettings.mMapIndex, mCurrentHostSettings.mPartyRoomMode, mCurrentHostSettings.mNumPlayers, mCurrentHostSettings.mDifficulty, mCurrentHostSettings.mHopperIndex, mCurrentHostSettings.mGameMode, mCurrentHostSettings.mHostStatusInformation, mCurrentHostSettings.mLiveMode);
   mpSession->SendPacket(packet);

   //Submit a max player change - if it didn't really change, it won't do anything
   changeMaxMemberCount(newSettings.mNumPlayers);
}

//==============================================================================
//Returns the party memeber's data for a particular member index (used to iterate through the valid, in-use entries)
//==============================================================================
BPartySessionPartyMember* BPartySession::getPartyMember(uint index)
{
   if (index>=cPartySessionMaxUsers)
      return NULL;

   uint count = 0;
   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      //if (mMembers[i].mClientID == cMPInvalidClientID)
      if (mMembers[i].mXuid == INVALID_XUID)
      {
         continue;
      }

      if (index==count)
      {
         return (&mMembers[i]);
      }
      count++;


//       if (mMembers[i].mClientID != cMPInvalidClientID)
//       {
//          count++;       //Count indexes into the array 1-based, so we need to shift down 1 to compare versus index which is 0-based
//          if ((count-1)==index)
//          {
//             return (&mMembers[i]);
//          }
//       }
   }

   return NULL;
}

//==============================================================================
//Given the pointer to a primary member, find any secondary member on that same connection
//==============================================================================
BPartySessionPartyMember* BPartySession::findSecondaryMemberFromThisPrimaryMember(BPartySessionPartyMember* primaryMember)
{
   BASSERT(primaryMember->mSettings.getPartyMemberType()==cBPartySessionPartyMemberPrimaryPlayer);
   if (primaryMember->mSettings.getPartyMemberType()!=cBPartySessionPartyMemberPrimaryPlayer)
   {
      return NULL;
   }

//-- FIXING PREFIX BUG ID 1453
   const BClient* primaryClient = getSession()->getClient(primaryMember->mClientID);
//--
   if (!primaryClient)
   {
      return NULL;
   }

   BMachineID primaryMachineID = getSession()->findMachineIDFromClientID(primaryMember->mClientID);
   if (primaryMachineID==BMachine::cInvalidMachineID)
   {
      BASSERT(false);
      return NULL;
   }
   
   for (uint i=0; i < cPartySessionMaxUsers;i++)
   {
      if ((mMembers[i].mClientID != primaryMember->mClientID) &&
          (mMembers[i].mClientID != cMPInvalidClientID))
      {
//-- FIXING PREFIX BUG ID 1452
         const BClient* client = getSession()->getClient(mMembers[i].mClientID);
//--
         if (client)
         {
            BMachineID machineID = getSession()->findMachineIDFromClientID(mMembers[i].mClientID);
            if (machineID==primaryMachineID)
            {
               //Found it
               return (&mMembers[i]);
            }
         }
      }
   }
   return NULL;

}

//==============================================================================
//Find a member by his client ID
//==============================================================================
BPartySessionPartyMember* BPartySession::findPartyMemberByClientID(BClientID id)
{
   for (uint i=0; i < cPartySessionMaxUsers;i++)
   {
      if (mMembers[i].mClientID == id)
      {
         return &mMembers[i];
      }
   }
   return NULL;
}

//==============================================================================
//Find a member by his XUID
//==============================================================================
BPartySessionPartyMember* BPartySession::findPartyMemberByXUID(XUID xuid)
{
   BASSERT(xuid);
   if (xuid==0)
   {
      nlog(cMPPartySystemCL, "BPartySession::findPartyMemberByXUID - Error, requested to find the party member for XUID ZERO|null|0!!!");
      return NULL;
   }

   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      if (mMembers[i].mXuid == xuid)
      {
         return &mMembers[i];
      }
   }
   return NULL;
}

//==============================================================================
//Find a member by his controller port
//==============================================================================
BPartySessionPartyMember* BPartySession::findPartyMemberByPort(long port)
{
   if (port==-1)
   {
      nlog(cMPPartySystemCL, "BPartySession::findPartyMemberByPort - Error, requested to find the party member for port -1");
      return NULL;
   }

   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      if (mMembers[i].mPort == port)
      {
         return &mMembers[i];
      }
   }
   return NULL;
}


//==============================================================================
//Return all the party member's XUIDs
//==============================================================================
 void BPartySession::getPartyXUIDs(XUID *xuidArray)
 {
    uint count = 0;
    for (uint i=0;i<cPartySessionMaxUsers;i++)
    {
       if (mMembers[i].mXuid != 0)
       { 
          BASSERT(count<3);
          xuidArray[count] = mMembers[i].mXuid;
          count++;
       }
    }
 }

//==============================================================================
//Drop a member via his client id
//==============================================================================
void BPartySession::dropPartyMemberByClientID(BClientID id, bool notifyEveryone)
{
   nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- Request to drop cid:%i", id);

//-- FIXING PREFIX BUG ID 1454
   const BPartySessionPartyMember* pMember = findPartyMemberByClientID(id);
//--
   if (pMember == NULL)
   {
      nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- Could not find that client ID in the member list, must have not fully joined");
      BClient* client = getSession()->getClient(id);
      if (!client)
      {
         //No member and he is not in the network session layer either - he isn't here to kick
         nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- Could not find that client ID in the network session - ignoring reqest");
         return;
      }
      dropPartyMemberByIndex(0, id, client->getXuid(), getSession()->isHostClientID(id), client->isLocal(), notifyEveryone, true);
      return;
   }
   else
   {
      dropPartyMemberByIndex(pMember->mMemberIndex, notifyEveryone);
   }
}

//==============================================================================
// 
//==============================================================================
void BPartySession::handleClientDisconnect(const BSessionEvent& event)
{
   // BSession has disconnected a client
   BClientID clientID = event.mData1;
   BMachineID machineID = event.mData2 >> 16 & 0xFF;
   BMachineID localMachineID = event.mData2 >> 8 & 0xFF;
   BMachineID hostMachineID = event.mData2 & 0xFF;
   BOOL isHost = (machineID == hostMachineID);
   BOOL isLocal = (machineID == localMachineID);
   XUID xuid = event.mData3;

   nlog(cMPPartySystemCL, "BPartySession::handleClientDisconnect -- Request to drop cid:%i", clientID);

//-- FIXING PREFIX BUG ID 1455
   const BPartySessionPartyMember* pMember = findPartyMemberByClientID(clientID);
//--
   // this assert will always be hit when a client exits due to the current event structure
   //BDEBUG_ASSERTM(pMember, "Missing BPartySessionPartyMember");
   if (pMember == NULL)
   {
      nlog(cMPPartySystemCL, "BPartySession::handleClientDisconnect -- Error: Could not find that clientID[%d] in the list", clientID);
      return;
   }

   dropPartyMemberByIndex(pMember->mMemberIndex, clientID, xuid, isHost, isLocal, true);
}

//==============================================================================
//Drop a member via his direct index
//==============================================================================
void BPartySession::dropPartyMemberByIndex(uint index, bool notifyEveryone)
{
   BDEBUG_ASSERTM(index >= 0 && index < cPartySessionMaxUsers, "Invalid mMembers index");
   if (index >= cPartySessionMaxUsers)
      return;

   int32 id = mMembers[index].mClientID;
   XUID xuid = mMembers[index].mXuid;

   BDEBUG_ASSERTM(mpSession, "Missing BSession");
   if (mpSession == NULL)
      return;

   BOOL isHost = (mpSession->isHostClientID(id));
   BOOL isLocal = (mpSession->isLocalClientID(id));

   dropPartyMemberByIndex(index, id, xuid, isHost, isLocal, notifyEveryone);
}

//==============================================================================
// Kick all the AIs
//==============================================================================
void BPartySession::kickAllAIMembers()
{
   for (int memberIndex=cPartySessionMaxUsers-1;memberIndex>=0;memberIndex--)
   {
      if ((mMembers[memberIndex].mXuid != INVALID_XUID) &&
          (mMembers[memberIndex].mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI))
      {
         dropPartyMemberByIndex(memberIndex);
      }
   }
}

//==============================================================================
//Drop a member via his direct index
//==============================================================================
void BPartySession::dropPartyMemberByIndex(uint index, ClientID clientId, XUID xuid, BOOL isHost, BOOL isLocal, bool notifyEveryone, bool noMemberRecord)
{
   nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByIndex -- Request to drop index:%i", index);

   if (!gLiveSystem->getMPSession())
   {
      nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByIndex -- No mpsession - exiting");
      return;
   }

   if (!noMemberRecord)
   {
      if (index >= cPartySessionMaxUsers)
      {
         nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- ERROR, request out of range");
         return;
      }

      //Determine if we need to unlock the party or not
      if (isHosting() && !gLiveSystem->getMPSession()->isSessionInterfaceActive())
      {
         //Do not unlock the party
      }
      //If the party is locked and a game is not running, unlock the party
      else if (!gLiveSystem->getMPSession()->isGameRunning() || mReactivatePartyLogic)
      {
         //EVERYONE needs to do this,  not just the host
         gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);
      }

      //Kill the matchmaking or game session if it is running but the game not started.
      if (isHosting())
      {      
         //If the game is running - DONT unlock the party, we don't want folks joining us again (until game over)
         if (!gLiveSystem->getMPSession()->isGameRunning())
         {
            //Don't directly unlock here - this will get handled in the modePartyRoom2 as the matchmaking and game sessions end
            //gLiveSystem->getMPSession()->getPartySession()->lockPartyMembers(false);
            nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- I'm the host and the party is no longer complete, unlocking party, reseting matckmaking if it is running");
            if (gLiveSystem->getMPSession()->isInMatchmakingMode())
            {
               gLiveSystem->getMPSession()->abortMatchMaking();
            }
            gLiveSystem->getMPSession()->abortGameSession();
         }
         else
         {
             nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- We are in-game, so I am NOT unlocking the party");
         }
      }

      //Save this info to finish cleaning up
      //bool dropMembersNetworkConnection = true;
      BPartySessionPartyMemberTypes memberType = mMembers[index].mSettings.getPartyMemberType();

      //Mark the record as empty/reusable
      mMembers[index].mClientID = cMPInvalidClientID;
      mMembers[index].mXuid = INVALID_XUID;
      mMembers[index].mPort = -1;      

      if (xuid != INVALID_XUID)
      {
         //Tell live session to drop them
         if (memberType==cBPartySessionPartyMemberPrimaryPlayer)
         {
            if (mpLiveSession)
            {
               mpLiveSession->dropRemoteUserFromSession(xuid);
            }
         }
         else if (memberType==cBPartySessionPartyMemberAI)
         {
            //Dropped an AI
            if (isHosting())
            {
               nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- sending msg for everyone to kick AI player");
               BPartyMemberKickRequest packet(xuid);
               mpSession->SendPacket(packet);
            }
         }
         else if (memberType==cBPartySessionPartyMemberSecondaryPlayer)
         {
            BASSERT(false);
            /*
            dropMembersNetworkConnection = false;
            //Drop secondary player
            if (isHosting())
            {
               nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- sending msg for everyone to kick secondary player");
               BPartyMemberKickRequest packet(xuid);
               mpSession->SendPacket(packet);
            }
            */
         }
      }

      //Let the above layer know they have left
      mSessionInterface->BPartySessionEvent_playerLeft(xuid);
   }

   //Am I removing myself?
   if (isLocal)
   {
      nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- Dropping myself as a member(cid:%i) ", clientId);
      getSession()->disconnect(BSession::cFailedClientConnect);
   }
   //Am I removing the host?
   else if (isHost)
   {
      nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- Dropping host as a member(cid:%i) - so this will drop me from the session", clientId);
      getSession()->disconnect(BSession::cFailedClientConnect);
   }
   else
   {
      if (clientId!=cMPInvalidClientID) 
      {
         //Kick him from the session
         nlog(cMPPartySystemCL, "BPartySession::dropPartyMemberByClientID -- kicking client (cid:%i)", clientId);
         //Send message telling everyone else to kick them
         if (notifyEveryone)
         {
            BPartyMemberKickRequest packet(xuid);
            mpSession->SendPacket(packet);
         }
         getSession()->beginClientDisconnect(clientId, BSession::cHostDecision);
      }
   }

   //Refresh out broadcasted information (Live or LAN)
   updateBroadcastedHostData();
}

//==============================================================================
//
//==============================================================================
//BPartySession::BClientConnector interface 
//bool BPartySession::connectionAttempt( BClient* requestingClient )
//{
//   long open = mPublicSlots - getPartyCount();
//
//   if (open <= 0)
//   {
//      nlog(cMPPartySystemCL, "BPartySession::connectionAttempt -- Party Full");
//      return false;
//   }
//
//   if (mPartyLocked)
//   {
//      nlog(cMPPartySystemCL, "BPartySession::connectionAttempt -- Party members locked");
//      return false;
//   }
//
//   return true;
//}

//==============================================================================
//Callback going up when we have a client requesting to connect to the host of the session.
//  If approved, then they are assigned a client ID, and allowed to try and fully connect to the session
// FIXME-COOP - check space for two potential users
//==============================================================================
bool BPartySession::sessionConnectionRequest(const BSessionUser users[], BSession::BJoinReasonCode* reasonCode)
{
   if (mPartyLocked)
   {
      nlog(cMPPartySystemCL, "BPartySession::sessionConnectionRequest -- Party members locked");
      *reasonCode = BSession::cResultRejectFull;
      return false;
   }

   long open = mPublicSlots - getPartyCount();

   if (open <= 0)
   {
      nlog(cMPPartySystemCL, "BPartySession::sessionConnectionRequest -- Game Full");
      *reasonCode = BSession::cResultRejectFull;
      return false;
   }

   if (*reasonCode != BSession::cResultJoinOk)
   {
      return false;
   }

   return true;
}

//==============================================================================
// Temp measure to provide a unique, single handle for AI members
//==============================================================================
XUID BPartySession::getFakeXUID()
{
   XUID ID = 0;;
   uint tries = 100;
   while (ID == 0)
   {
      XNetRandom((BYTE*)&ID, sizeof(XUID));
      for (uint i=0;i<cPartySessionMaxUsers;i++)
      {
         if (mMembers[i].mXuid == ID)
         {
            ID = 0;
            break;
         }
      }
      if (ID != 0)
      {
         return ID;
      }
      tries--;
      if (tries<1)
      {
         return (0);
      }
   }
   //NOTE: no code path to this return
   return (0);   
}

//==============================================================================
// Add a non-primary player into the party session 
//==============================================================================
BOOL BPartySession::addNonNetworkedPlayer(BPartySessionPartyMemberTypes memberType, const BSimString& gamerTag, const BPartySessionPlayerSettings& settings)
{ 
   BASSERT(memberType == cBPartySessionPartyMemberAI);

   if (!isHosting())
   {
      BASSERT(false);
      return FALSE;
   }

   if (getPartyCount()>=cPartySessionMaxUsers)
   {
      //Too many in there already
      BASSERT(false);
      return FALSE;
   }

   if (mPartyLocked)
   {
      //We are locked currently
      return FALSE;
   }

   //Get a new tracking ID
   XUID newXuid = getFakeXUID();

   BOOL openIndexFound = FALSE;
   for (uint memberIndex=0;memberIndex<cPartySessionMaxUsers;memberIndex++)
   {
      //if (mMembers[memberIndex].mClientID == cMPInvalidClientID)
      if (mMembers[memberIndex].mXuid == INVALID_XUID)
      {
         openIndexFound=TRUE;
         //Utils::FastMemCpy(&(mMembers[memberIndex].mSettings), settings, sizeof(BPartySessionPlayerSettings));
         mMembers[memberIndex].mSettings = settings;
         mMembers[memberIndex].mXuid = newXuid;
         mMembers[memberIndex].mPort = -1;
         mMembers[memberIndex].mClientID = cMPInvalidClientID;
         mMembers[memberIndex].mMemberIndex = (uint8)memberIndex;
         mMembers[memberIndex].mLastTimeUpdated = timeGetTime();
         mMembers[memberIndex].mGamerTag.copy(gamerTag);
         mMembers[memberIndex].mQoSValue = 1;           
         mMembers[memberIndex].mRestrictedNAT = false;
         mMembers[memberIndex].mState = cBPartySessionMemberStateIdle;
         mMembers[memberIndex].mLevelLoaded = false;
         mMembers[memberIndex].mSlotChanged = false;
         mMembers[memberIndex].mLiveSessionJoined = true;
         mMembers[memberIndex].mSettings.clearFlags();
         mMembers[memberIndex].mSettings.setPartyMemberType(memberType);
         mMembers[memberIndex].mSettings.mPlayerRank.reset();
         mMembers[memberIndex].mSettings.mCampaignBits = 0;
         nlog(cMPPartySystemCL, "BPartySession::addNonNetworkedPlayer - Member added to list slot:%d Xuid:%I64u ClientID:%d Name:%s",
            memberIndex, mMembers[memberIndex].mXuid, mMembers[memberIndex].mClientID, mMembers[memberIndex].mGamerTag.getPtr() );
         break;
      }
   }

   if (!openIndexFound)
   {
      //There were no open slots - drop them
      nlog(cMPPartySystemCL, "BPartySession::addNonNetworkedPlayer - Client joined, but we have no slots for them - dropping them");
      return FALSE;
   }

   //Did adding this AI fill our game?
   if (getPartyCount() >= mPublicSlots)
   {
      lockPartyMembers(true);
   }
 
   //Update our broadcasted info
   updateBroadcastedHostData();

   //Send an event up
   mSessionInterface->BPartySessionEvent_playerJoined(newXuid);

   //Send out a packet telling everyone to add them
   BPartyAddAICommand addPacket(newXuid, gamerTag);
   mpSession->SendPacket(addPacket);

   //Send out a packet with info about that newly added dude
   const BPartySessionPlayerSettings& playerSettings = mMembers[memberIndex].mSettings;
   BPartyMemberSettingsPacket packet(newXuid, playerSettings.mCiv, playerSettings.mLeader, playerSettings.mTeam, playerSettings.mSlot, playerSettings.mConnectionState, playerSettings.mPartyRoomMode, playerSettings.mStatusFlags, playerSettings.mPlayerRank.mValue, playerSettings.mCampaignBits);
   getSession()->SendPacket(packet);

   return TRUE;
}

//==============================================================================
//
//==============================================================================
void BPartySession::newMemberJoined(ClientID newClientID)
{
   uint partyCount = getPartyCount();
   nlog(cMPPartySystemCL, "BPartySession::newMemberJoined - Client id [%d], current party size [%d/%d]", newClientID, partyCount, cPartySessionMaxUsers);

   //Check to make sure we have not exceeded our max count
   if (partyCount >= cPartySessionMaxUsers)
   {
      //Too many
      nlog(cMPPartySystemCL, "BPartySession::newMemberJoined - Client id %d dropped on connect event because the member list is full", newClientID);
      dropPartyMemberByClientID(newClientID, true);
      return;
   }

   //Check that we are not locked
   if (mPartyLocked)
   {
      //We are locked currently
      nlog(cMPPartySystemCL, "BPartySession::newMemberJoined - Client id %d dropped on connect event because the party is locked", newClientID);
      dropPartyMemberByClientID(newClientID, true);
      return;
   }

   BClient* pClient = getSession()->getClient(newClientID);
   if (!pClient)
   {
      BDEBUG_ASSERTM(false, "Received cEventClientConnect for a disconnected client, ignoring it");
      return;
   }

   //Find what port this user is from - IF THEY ARE LOCAL, otherwise -1 is just fine for the port assignment
   long port = -1;
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser && (pUser->getXuid()==pClient->getXuid()))
   {
      port = pUser->getPort();
   } 

   BOOL openIndexFound = FALSE;
   for (uint memberIndex=0;memberIndex<cPartySessionMaxUsers;memberIndex++)
   {
      //if (mMembers[memberIndex].mClientID == cMPInvalidClientID)
      if (mMembers[memberIndex].mXuid == INVALID_XUID)
      {
         openIndexFound=TRUE;
         //Utils::FastMemCpy(&mMembers[memberIndex].mSettings, &mDefaultPlayerSetting, sizeof(mDefaultPlayerSetting));
         mMembers[memberIndex].mSettings = mDefaultPlayerSetting;
         if (port != -1 && pUser != NULL)
         {
            const BUserProfile* pProfile = pUser->getProfile();
            if (pProfile != NULL)
            {
               Utils::FastMemCpy(&mMembers[memberIndex].mSettings, &mLocalMemberInitialSettings, sizeof(BPartySessionPlayerSettings));  
               mMembers[memberIndex].mSettings.mPlayerRank.mValue = pProfile->getRank().mValue;
            }
         }

         mMembers[memberIndex].mXuid = pClient->getXuid();
         mMembers[memberIndex].mPort = port;
         mMembers[memberIndex].mClientID = newClientID;
         mMembers[memberIndex].mMemberIndex = (uint8)memberIndex;
         mMembers[memberIndex].mLastTimeUpdated = timeGetTime();

         if (gLiveSystem->getMPSession()->isInLANMode() && (pClient->getXuid() & 0xFFFF) == 0xBEEF)
         {
            BUString tempName;
            tempName.locFormat(gDatabase.getLocStringFromID(25567).getPtr(), newClientID+1);

            BSimString gamertag;
            gamertag.set(tempName.getPtr());
            pClient->setGamertag(gamertag);

            if (pUser)
            {
               if (pUser->getXuid() == pClient->getXuid())
                  pUser->setName(gamertag);
               if (mLocalXuid == pClient->getXuid())
                  mLocalGamertag = gamertag;
            }
         }

         mMembers[memberIndex].mGamerTag.copy(pClient->getName());
         mMembers[memberIndex].mQoSValue = 1;           // setting to best possible value
         mMembers[memberIndex].mRestrictedNAT = false;
         mMembers[memberIndex].mState = cBPartySessionMemberStateIdle;
         mMembers[memberIndex].mLevelLoaded = false;
         mMembers[memberIndex].mSlotChanged = false;
         mMembers[memberIndex].mLiveSessionJoined = true;       //todo - drop this field it will ALWAYS be true of fully connected members
         mMembers[memberIndex].mSettings.clearFlags();
         //mMembers[memberIndex].mSettings.mPlayerRank.reset();
         //mMembers[memberIndex].mSettings.mCampaignBits = 0;
         if ((getSession()->getHostMachine()) &&
             (newClientID == (uint32)getSession()->getHostMachine()->mUsers[0].mClientID ))
         {
            mMembers[memberIndex].mSettings.setIsSessionHost( true );
         }
         nlog(cMPPartySystemCL, "BPartySession::newMemberJoined - Member added to list slot:%d Xuid:%I64u ClientID:%d Name:%s",
            memberIndex, mMembers[memberIndex].mXuid, mMembers[memberIndex].mClientID, mMembers[memberIndex].mGamerTag.getPtr() );
         break;
      }
   }

   if (!openIndexFound)
   {
      //There were no open slots - drop them
      nlog(cMPPartySystemCL, "BPartySession::newMemberJoined - Client joined, but we have no slots for them - dropping them");
      dropPartyMemberByClientID(newClientID, true);
      return;
   }
   else
   {
      //Add them to the Live session if they are remote
      if (mpLiveSession && (port==-1))
      {
         mpLiveSession->addRemoteUserToSession( pClient->getXuid(), false );
      }

      //If I'm the host - send the host game settings to this new member
      if (isHosting())
      {
         BPartyHostSettingsPacket packet(mCurrentHostSettings.mRandomTeam, mCurrentHostSettings.mMapIndex, mCurrentHostSettings.mPartyRoomMode, mCurrentHostSettings.mNumPlayers, mCurrentHostSettings.mDifficulty, mCurrentHostSettings.mHopperIndex, mCurrentHostSettings.mGameMode, mCurrentHostSettings.mHostStatusInformation, mCurrentHostSettings.mLiveMode);
         mpSession->SendPacketTo(pClient, packet);
      }
      
      //Am I the dude who just joined?
      if (mpSession->isLocalClientID(newClientID))
      {
         //Tell the above layer that I'm up and ready
         mSessionInterface->BPartySessionEvent_systemReady();
      }

      if (mpVoice)
         mpVoice->setChannel(BVoice::cPartySession, pClient->getXuid(), BVoice::cAll);
   }

   //Update our broadcasted info
   updateBroadcastedHostData();

   //Send an event up
   mSessionInterface->BPartySessionEvent_playerJoined(pClient->getXuid()); //newClientID, memberIndex);

}

//==============================================================================
// FIXME-COOP - could have two local clients in cEventConnected, need to change
//              the code to respond to all local clientIDs
//              OR, simply deal with the local BMachine instance
//              BMachine::mUsers -> BSessionUser::mClientID -> BClient
//==============================================================================
//BPartySession::BSessionEventObserver interface 
void BPartySession::processSessionEvent(const BSessionEvent* pEvent)
{
   switch (pEvent->mEventID)
   {
   case BSession::cEventJoinFailed:
      {
         if (mSessionInterface)
         {
            nlog(cMPPartySystemCL, "BPartySession::processSessionEvent - Session join fail event with BSession::BJoinReasonCode code %d", pEvent->mData1 );
            //mSessionInterface->BPartySessionEvent_systemStartupFailed( cBPartySessionPartyStatupCouldNotJoinSession );
            mSessionInterface->BPartySessionEvent_systemStartupFailed( (BSession::BJoinReasonCode)pEvent->mData1 );
            setState(cBPartySessionStateError);
         }         
         break;
      }

   case BSession::cEventConnected:
      {   
         nlog(cMPPartySystemCL, "BPartySession::processSessionEvent - We are now fully connected to everyone in the party session");
         //Send a settings packet to the host so he knows I'm ready - host will respond by sending me EVERYONE's current settings, including my own
//-- FIXING PREFIX BUG ID 1458
         const BPartySessionPartyMember* pMember = NULL;
//--
//-- FIXING PREFIX BUG ID 1459
         const BMachine* pMachine = mpSession->getLocalMachine();
//--
         BASSERTM(pMachine != NULL, "Missing local local machine, party will not form");
         if (!pMachine)
            return;
         BClientID clientID = -1;
         for (uint i=0; i < BMachine::cMaxUsers; ++i)
         {
            if (pMachine->mUsers[i].mClientID != -1)
            {
               clientID = pMachine->mUsers[i].mClientID;
               pMember = findPartyMemberByClientID(pMachine->mUsers[i].mClientID);
               break;
            }
         }
         if (!pMember)
         {
            //I can't find myself? - Must be in an odd disconnecting state
            nlog(cMPPartySystemCL, "BPartySession::processSessionEvent - Error, attempting to send out settings for my client but there is no party member my clientID!" );
            dropPartyMemberByClientID(clientID);
            return;
         }
         //Build a settings packet with the state set to "request initial settings" - the host (ONLY!) will see this and respond with settings for everyone currently in the session
         BPartySessionPlayerSettings requestSettings = pMember->mSettings;
         requestSettings.mConnectionState = cBPartySessionMemberConnectionStateRequestingInitial;
         changeSettings(pMember->mXuid, requestSettings);
         mGameConnected = true;
         break;
      }

   case BSession::cEventDisconnected:
      {
         nlog(cMPPartySystemCL, "BPartySession::processSessionEvent - Session disconnected event with code %d", pEvent->mData1 );
         mSessionInterface->BPartySessionEvent_systemDisconnected();
         break;
      }

   case BSession::cEventClientConnect:
      {
         BClientID clientID = pEvent->mData1;

         //Add this new player
         newMemberJoined(clientID);

         break;
      }

   case BSession::cEventClientDisconnect:
      {
         //Drop them from the member lists
         BDEBUG_ASSERTM(pEvent, "Missing BSessionEvent");
         if (pEvent == NULL)
            break;
         nlog(cMPPartySystemCL, "BPartySession::processSessionEvent - Dropping client %d because we received a client disconnect event", pEvent->mData1);
         handleClientDisconnect(*pEvent);
         //dropPartyMemberByClientID((ClientID)pEvent->mData1, true);
         break;
      }

   case BSession::cEventClientData:
      {
         handleClientData(pEvent);
         break;
      }

   //case BSession::cEventClientNotResponding:
   //   {
   //      //Not responding - drop them
   //      nlog(cMPPartySystemCL, "BPartySession::processSessionEvent - Dropping client %d because we received a client not responding event", pEvent->mData1);
   //      dropPartyMemberByClientID((ClientID)pEvent->mData1, true);

   //      //Send event up that they dropped?
   //      break;
   //   }

   //case BSession::cEventClientResponding:
   //   //Currently we do nothing with this
   //   //clientResponding(pEvent->mData1, pEvent->mData2);
   //   break;

   case BSession::cEventClientPing:
      {
         //Update their last interaction time
         BPartySessionPartyMember* pMember = findPartyMemberByClientID(pEvent->mData1);

         if (!pMember)
         {
            //No member for this ping - ignore it
            nlog(cMPPartySystemCL,"BPartySession::processSessionEvent - Dropping ping data for a client ID [%d] that I cannot find a member for", pEvent->mData1);         
            return;
         }
         pMember->mLastTimeUpdated = timeGetTime();
         // keep value in RTT
         //uint8 newQOSValue = (pEvent->mData2!=0)?uint8(pEvent->mData2 / 2):1;
         uint32 ping = Math::Clamp<uint32>(pEvent->mData2, 0, 800);
         uint8 newQoS;
         if (ping < 100)
            newQoS = 0;
         else if (ping < 150)
            newQoS = 20;
         else if (ping < 300)
            newQoS = 40;
         else if (ping < 450)
            newQoS = 60;
         else
            newQoS = 80;
         //if (newQOSValue>100)
         //{
         //   //Cap it at a max of 100
         //   newQOSValue = 100;
         //}
         //Only update this (and send out an event) if it changed by 10% or more
         if ((newQoS > pMember->mQoSValue && newQoS - pMember->mQoSValue >= 10) ||
             (newQoS < pMember->mQoSValue && pMember->mQoSValue - newQoS >= 10))
         {
            pMember->mQoSValue = newQoS;
#ifndef BUILD_FINAL
            if (!gConfig.isDefined(cConfigNoQOSUpdatesToUI))
#endif
            {
               mSessionInterface->BPartySessionEvent_memberSettingsChanged(pMember);
            }
         }

         break;
      }

   default:
      break;
   }   
}

//==============================================================================
// 
//==============================================================================
void BPartySession::handleClientData(const BSessionEvent* pEvent)
{
   const void *data = (((char*)pEvent)+sizeof(BSessionEvent));
   long size  = pEvent->mData2;
   long fromClientIndex = pEvent->mData1;
   
//-- FIXING PREFIX BUG ID 1464
   const BPartySessionPartyMember* partyMemberPtr = findPartyMemberByClientID(fromClientIndex);
//--
   if (!partyMemberPtr)
   {
//-- FIXING PREFIX BUG ID 1461
      const BClient* client = getSession()->getClient(pEvent->mData1);
//--
      if (client) 
      {
         nlog(cMPPartySystemCL, "BPartySession::handleClientData - Warning: I have client data from valid client ID [%d] but no member for them, dropping that client and losing the data", fromClientIndex);
         dropPartyMemberByClientID(pEvent->mData1);
      }
      else
      {
         nlog( cMPPartySystemCL, "BPartySession::handleClientData - Warning: I have client data from an INVALID client ID [%d], losing the data", fromClientIndex);
      }
      return;
   }

   switch (BTypedPacket::getType(data))
   {
      case BPacketType::cPartyMemberSettingsPacket:
      case BPacketType::cPartyMemberSettingsEchoPacket:
         {
            nlog(cMPPartySystemCL, "BPartySession::handleClientData - Got party member packet");
            if (!partyMemberPtr)
            {
               //Cannot find the dude that sent this data - very odd
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - received settings data from client %i, but no such party member", fromClientIndex);
               return;
            }
            BPartyMemberSettingsPacket packet;
            packet.deserializeFrom(data, size);
            BPartySessionPartyMember* partyMemberPtrTarget = findPartyMemberByXUID(packet.mXUID);
            if (!partyMemberPtrTarget)
            {
               //Cannot find the dude for the sender of this data - very odd
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - received settings data for client xuid %I64u, but no such party member", packet.mXUID);
               return;
            }

            //Change the packet into the settings structure
            BPartySessionPlayerSettings newSettings;
            newSettings.mCiv = packet.mCiv;
            newSettings.mLeader = packet.mLeader;
            newSettings.mTeam = packet.mTeam;
            newSettings.mSlot = packet.mSlot;
            newSettings.mConnectionState = packet.mConnectionState;
            newSettings.mPartyRoomMode = packet.mPartyRoomMode;
            newSettings.mStatusFlags = packet.mStatusFlags;
            newSettings.mPlayerRank.mValue = packet.mPlayerRank;
            newSettings.mCampaignBits = packet.mCampaignBits;

            //The Host processes all settings packets including echoing those out to the other client (where needed)
            if (isHosting())
            {
               //It is a settings packet
               if (BTypedPacket::getType(data) == BPacketType::cPartyMemberSettingsPacket)
               {
                  //Is this a request for settings?
                  if (newSettings.mConnectionState == cBPartySessionMemberConnectionStateRequestingInitial)
                  {
                     //Need the client for this joiner
                     BClient* targetJoiningClient = mpSession->getClient(partyMemberPtr->mClientID);
                     if (!targetJoiningClient)
                     {
                        //We have an initial request from an invalid client? Kick them
                        nlog(cMPPartySystemCL, "BPartySession::handleClientData - Host has received an initial settings request from %s but I cannot find their BClient, kicking them", partyMemberPtrTarget->mGamerTag.getPtr());
                        dropPartyMemberByClientID(partyMemberPtr->mClientID, false);
                        return;
                     }

                     //They are FULLY CONNECTED, assign them a slot/etc, send that change out to everyone, then send the requester EVERYONES settings
                     nlog(cMPPartySystemCL, "BPartySession::handleClientData - Host has received an initial settings request from %s", partyMemberPtrTarget->mGamerTag.getPtr() );

                     //Call the setting changed callback which will handle setting anything needed and sending it out to everyone
                     newSettings.mConnectionState=cBPartySessionMemberConnectionStateJoining;
                     partyMemberPtrTarget->mSettings = newSettings;

                     //Check if we have a name collision - LAN only! 
                     //if (gLiveSystem->getMPSession()->isInLANMode())
                     //{
                     //   BSimString newDudesName(targetJoiningClient->getName());
                     //   bool nameChanged = false;
                     //   for (uint i=0;i<cPartySessionMaxUsers;i++)
                     //   {
                     //      if ((mMembers[i].mClientID != cMPInvalidClientID) &&
                     //          (mMembers[i].mClientID != partyMemberPtr->mClientID) &&
                     //          (mMembers[i].mXuid != INVALID_XUID))
                     //      {
                     //         uint32 startIndex = 2;
                     //         while (newDudesName.compare(mMembers[i].mGamerTag, false)==0)
                     //         {
                     //            BUString temp;
                     //            temp.locFormat(gDatabase.getLocStringFromID(25567).getPtr(), startIndex);
                     //            newDudesName.set(temp.getPtr());
                     //            startIndex++;
                     //            BASSERT(startIndex<20);
                     //            nameChanged = true;
                     //         }
                     //      }
                     //   }
                     //   if (nameChanged)
                     //   {
                     //      BPartyAddAICommand packet(targetJoiningClient->getXuid(), newDudesName, BPacketType::cPartyChangeGamertagCommand);
                     //      mpSession->SendPacket(packet);
                     //   }
                     //}
                                   
                     //Notify the member settings changed
                     mSessionInterface->BPartySessionEvent_memberSettingsChanged(partyMemberPtrTarget);

                     //Now send the joiner everyones current settings (except his own which he just got)
                     for (uint i=0;i<cPartySessionMaxUsers;i++)
                     {
                        if ((mMembers[i].mClientID != cMPInvalidClientID) &&
                            (mMembers[i].mClientID != partyMemberPtr->mClientID) &&
                            (mMembers[i].mXuid != INVALID_XUID))
                        {
                           BPartyMemberSettingsPacket currentMemberPacket(mMembers[i].mXuid, mMembers[i].mSettings.mCiv, mMembers[i].mSettings.mLeader, mMembers[i].mSettings.mTeam, mMembers[i].mSettings.mSlot, mMembers[i].mSettings.mConnectionState, mMembers[i].mSettings.mPartyRoomMode, mMembers[i].mSettings.mStatusFlags, mMembers[i].mSettings.mPlayerRank.mValue, mMembers[i].mSettings.mCampaignBits, BPacketType::cPartyMemberSettingsEchoPacket);
                           mpSession->SendPacketTo(targetJoiningClient, currentMemberPacket); 
                        }
                     }

                     //Then send him Add/Updates for each AI currently in session
                     for (uint i=0;i<cPartySessionMaxUsers;i++)
                     {
                        if ((mMembers[i].mXuid != INVALID_XUID) &&
                            (mMembers[i].mSettings.getPartyMemberType()==cBPartySessionPartyMemberAI))
                        {
                           BPartyAddAICommand addPacket(mMembers[i].mXuid, mMembers[i].mGamerTag);
                           mpSession->SendPacketTo(targetJoiningClient, addPacket);
                           BPartyMemberSettingsPacket currentMemberPacket(mMembers[i].mXuid, mMembers[i].mSettings.mCiv, mMembers[i].mSettings.mLeader, mMembers[i].mSettings.mTeam, mMembers[i].mSettings.mSlot, mMembers[i].mSettings.mConnectionState, mMembers[i].mSettings.mPartyRoomMode, mMembers[i].mSettings.mStatusFlags, mMembers[i].mSettings.mPlayerRank.mValue, mMembers[i].mSettings.mCampaignBits, BPacketType::cPartyMemberSettingsEchoPacket);
                           mpSession->SendPacketTo(targetJoiningClient, currentMemberPacket); 
                        }
                     }

                     //We don't want any further, normal processing of this packet
                     return;
                  }
                  else if (newSettings.mConnectionState == cBPartySessionMemberConnectionStateJoining)
                  {
                     //Client is requesting to get settings for the current mode - the callback will handle that
                     if (partyMemberPtrTarget->mSettings.mSlot != newSettings.mSlot)
                     {
                        partyMemberPtrTarget->mSlotChanged = true;
                     }
                     partyMemberPtrTarget->mSettings = newSettings;
                     mSessionInterface->BPartySessionEvent_memberSettingsChanged(partyMemberPtrTarget);
                     //No further processing here
                     return;
                  }

                  //Should we echo it out to everyone else?
                  if (partyMemberPtrTarget->mSettings.mConnectionState!=cBPartySessionMemberConnectionStateJoining)
                  {
                     //NOTE: This means that host sends echo's changes to himself that he just sent himself - that second packet of changes will not be echoed out
                     nlog(cMPPartySystemCL, "BPartySession::handleClientData - Host is echoing settings change from %s", partyMemberPtrTarget->mGamerTag.getPtr());
                     broadcastChangeSettings(newSettings, partyMemberPtrTarget->mXuid);
                  }
                  else
                  {
                     nlog(cMPPartySystemCL, "BPartySession::handleClientData - Host is NOT echoing settings change from %s (no changes or in joining state)", partyMemberPtrTarget->mGamerTag.getPtr());
                  }
               }
               else
               {
                  //This is an echo'd packet - the host logic ignores these
               }
            }

            //Only echo'd packets should ever be processed by the client logic
            if (BTypedPacket::getType(data) == BPacketType::cPartyMemberSettingsEchoPacket)
            {
               //OMG HAX - eric
               //Ok - here's the deal
               //If the data is actually a state change that is a request to either switch teams or to cancel a team switch
               //  Then we DONT take any of the payload settings EXCEPT for that state change
               //...sigh
               if ((newSettings.mConnectionState == cBPartySessionMemberConnectionStateCancelSwitchTeams) ||
                   (newSettings.mConnectionState == cBPartySessionMemberConnectionStateWantToSwitchTeams))
               {
                  partyMemberPtrTarget->mSettings.mConnectionState = newSettings.mConnectionState;
               }
               else
               {
                  if (partyMemberPtrTarget->mSettings.mSlot != newSettings.mSlot)
                  {
                     partyMemberPtrTarget->mSlotChanged = true;
                  }
                  partyMemberPtrTarget->mSettings = newSettings;
               }
               mSessionInterface->BPartySessionEvent_memberSettingsChanged(partyMemberPtrTarget);
            }
            break;
         }

      case BPacketType::cPartyHostSettingsPacket:
         {
            BPartyHostSettingsPacket packet;
            packet.deserializeFrom(data, size);
            BPartySessionHostSettings newSettings;
            newSettings.mNumPlayers = packet.mNumPlayers;
            newSettings.mDifficulty = packet.mDifficulty;
            newSettings.mMapIndex = packet.mMapIndex;
            newSettings.mPartyRoomMode = packet.mPartyRoomMode;
            newSettings.mRandomTeam = packet.mRandomTeam;
            newSettings.mHopperIndex = packet.mHopperIndex;
            newSettings.mGameMode = packet.mGameMode;
            newSettings.mHostStatusInformation = packet.mHostStatusInformation;
            newSettings.mLiveMode = packet.mLiveMode;
            mCurrentHostSettings = newSettings;
            //Change the max member count
            changeMaxMemberCount(newSettings.mNumPlayers);

            //Change the live room mode
            if (!gLiveSystem->getMPSession()->isInLANMode() && mpLiveSession)
            {
               mpLiveSession->changeLiveMode((BLiveSessionHostingModes)newSettings.mLiveMode);
            }

            //HAX - If the client is still in a gameSession, but the host returns to the party - the party session is still locked and nobody can join
            // So we need to watch for a trigger that the host has returned to the game
            // That trigger is a mode setting - on that, unlock the party
            if ((mCurrentHostSettings.mHostStatusInformation == cBPartySessionHostStatusInformationCodePostGameResetUI) &&
                (gLiveSystem->getMPSession()->isGameRunning()))
            {
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - Host has left game session, but I have not - unlocking party session");
               mReactivatePartyLogic = true;
               lockPartyMembers(false);
            }

            //Changing the local values BEFORE the callback so that receivers can just get updated data from the party session.
            mSessionInterface->BPartySessionEvent_hostSettingsChanged(&newSettings);
            break;
         }

      //case BPacketType::cPartyChangeGamertagCommand:
      //   {
      //      BPartyAddAICommand packet;
      //      packet.deserializeFrom(data, size);
      //      nlog(cMPPartySystemCL, "BPartySession::handleClientData - received change gamertag command for xuid %I64u from client %i", packet.mXuid, fromClientIndex );
      //      if (mpSession)
      //      {
      //         BClient* client = mpSession->getClientByXuid(packet.mXuid);
      //         if (client)
      //         {
      //            client->setGamertag(packet.mGamertag);
      //            BPartySessionPartyMember* member = findPartyMemberByXUID(packet.mXuid);
      //            if (member)
      //            {
      //               member->mGamerTag.set(packet.mGamertag);
      //            }
      //            BUser* user = gUserManager.getPrimaryUser();
      //            BASSERT(user);
      //            if (user->getXuid()==packet.mXuid)
      //            {
      //               user->setName(packet.mGamertag);
      //            }
      //         }
      //      }            
      //      break;
      //   }

      case BPacketType::cPartyKickRequest:
         {
            BPartyMemberKickRequest packet;
            packet.deserializeFrom(data, size);
            nlog(cMPPartySystemCL, "BPartySession::handleClientData - received kick for target xuid %I64u from client %i", packet.mXuid, fromClientIndex );
            //Drop this dude if he is connected
            if (mpSession)
            {
               BClient* client = mpSession->getClientByXuid(packet.mXuid);
               if (client)
               {
                  dropPartyMemberByClientID(client->getID(), false);
               }
               else
               {
                  BPartySessionPartyMember* member = findPartyMemberByXUID(packet.mXuid);
                  if (member)
                  {
                     dropPartyMemberByIndex(member->mMemberIndex);
                  }
               }
            }            
            break;
         }

      case BPacketType::cPartyAddAICommand:
         {
            if (isHosting())
            {
               //Host will have already added them - ignore this
               return;
            }

            BPartyAddAICommand packet;
            packet.deserializeFrom(data, size);
            nlog(cMPPartySystemCL, "BPartySession::handleClientData - received add AI command for xuid %I64u from client %i", packet.mXuid, fromClientIndex );

            const BPartySessionPartyMember* pMember = findPartyMemberByXUID(packet.mXuid);
            if (pMember != NULL)
            {
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - received add AI command for xuid %I64u from client %i - already exists", packet.mXuid, fromClientIndex );
               return;
            }

            BOOL openIndexFound = FALSE;
            for (uint memberIndex=0;memberIndex<cPartySessionMaxUsers;memberIndex++)
            {
               if (mMembers[memberIndex].mXuid == INVALID_XUID)
               {
                  openIndexFound=TRUE;
                  //Utils::FastMemCpy(&mMembers[memberIndex].mSettings, &mDefaultPlayerSetting, sizeof(mDefaultPlayerSetting));
                  mMembers[memberIndex].mSettings = mDefaultPlayerSetting;
                  mMembers[memberIndex].mXuid = packet.mXuid;
                  mMembers[memberIndex].mClientID = cMPInvalidClientID; //fromClientIndex;
                  mMembers[memberIndex].mMemberIndex = (uint8)memberIndex;
                  mMembers[memberIndex].mLastTimeUpdated = timeGetTime();
                  mMembers[memberIndex].mGamerTag.copy(packet.mGamertag);
                  mMembers[memberIndex].mQoSValue = 1;           // setting to best possible value
                  mMembers[memberIndex].mRestrictedNAT = false;
                  mMembers[memberIndex].mState = cBPartySessionMemberStateIdle;
                  mMembers[memberIndex].mLevelLoaded = false;
                  mMembers[memberIndex].mSlotChanged = false;
                  mMembers[memberIndex].mLiveSessionJoined = true;       //todo - drop this field it will ALWAYS be true of fully connected members
                  mMembers[memberIndex].mSettings.clearFlags();
                  mMembers[memberIndex].mSettings.setPartyMemberType(cBPartySessionPartyMemberAI);
                  mMembers[memberIndex].mSettings.mPlayerRank.reset();
                  mMembers[memberIndex].mSettings.mCampaignBits = 0;
                  nlog(cMPPartySystemCL, "BPartySession::handleClientData - AI Member added to list slot:%d Xuid:%I64u ClientID:%d Name:%s",
                     memberIndex, mMembers[memberIndex].mXuid, mMembers[memberIndex].mClientID, mMembers[memberIndex].mGamerTag.getPtr() );
                  break;
               }
            }

            if (!openIndexFound)
            {
               //There were no open slots - drop them
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - AI Client join command, but we have no slots for them - dropping them");
               //BASSERT(false); - no assert here - this is possible if the AI joins as another human is joining - we DO want the AI to be dropped
               return;
            }

            //Did adding this AI fill our game?
            if (getPartyCount() >= mPublicSlots)
            {
               lockPartyMembers(true);
            }

            //Update our broadcasted info
            updateBroadcastedHostData();

            //Send an event up
            mSessionInterface->BPartySessionEvent_playerJoined(packet.mXuid);
       
            break;
         }

      case BPacketType::cPartyMatchMakingCommand:
         {
            BPartyMemberMatchMakingCommand mpacket;
            mpacket.deserializeFrom(data, size);
            BASSERT(gLiveSystem->getMPSession());
            handlePartyMatchMakingCommand(fromClientIndex, mpacket.mCommandCode, mpacket.mTargetNonce);
            break;
         }
      case BPacketType::cPartyMatchMakingJoinCommand:
         {
            //Party host ignores this - since he sent it
            if (isHosting())
            {
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - received cPartyMatchMakingJoinCommand but ignoring it since I'm the host"); 
               return;
            }
            BPartyMemberMatchMakingJoinCommand mpacket;
            mpacket.deserializeFrom(data, size);
            BASSERT(gLiveSystem->getMPSession());
            //Check if they are already in a game session - this should never happen
            if (gLiveSystem->getMPSession()->getSession())
            {
               nlog(cMPPartySystemCL, "BPartySession::handleClientData - ERROR:received cPartyMatchMakingJoinCommand but there is already a game session object active!"); 
               BASSERT(false);
               return;
            }
            nlog(cMPPartySystemCL, "BPartySession::handleClientData - received cPartyMatchMakingJoinCommand to nonce %I64u", mpacket.mTargetNonce); 
            gLiveSystem->getMPSession()->partyCommandJoinTarget(fromClientIndex, mpacket.mTargetHopper, mpacket.mTargetXNADDR, mpacket.mTargetXNKEY, mpacket.mTargetXNKID, mpacket.mTargetNonce);
            break;
         } 
      case (BPacketType::cPartyMatchMakingStatusInfo):
         {
            nlog(cMPPartySystemCL, "BPartySession::handleClientData - received new matchmaking info");
            if (!mSessionInterface)
            {
               //TODO LOG
               return;
            }
            BPartyMatchMakingStatusInfo mpacket;  
            mpacket.deserializeFrom(data, size);
            mCurrentMatchMakingStatusCode = (BLiveMatchMaking::BLiveMatchMakingStatusCode)mpacket.mStatusCode;
            mSessionInterface->BPartySessionEvent_matchMakingStatusChanged(mpacket.mStatusCode, mpacket.mData1, mpacket.mData2);
            break;
         }
   }

}

//==============================================================================
// 
//==============================================================================
void BPartySession::setState(BPartySessionState newState)
{
   if (mState>=cBPartySessionStateShuttingDown)
   {
      nlog(cMPPartySystemCL, "BPartySession::setState - Ignoring state change, Im shutting down");
   }
   mState = newState;
}

//==============================================================================
// 
//==============================================================================
HRESULT BPartySession::setControllerID(const uint32 controllerID)
{
   mLocalControllerID = XUSER_MAX_COUNT;
   mLocalGamertag.empty();

//-- FIXING PREFIX BUG ID 1465
   //const BUser* pUser = gUserManager.getUserByPort(controllerID);
//--
   BUser* pUser = gUserManager.getPrimaryUser();
   BASSERTM(pUser, "Failed to find user for requested controller ID");
   if (pUser == NULL)
      return E_FAIL;

   mLocalControllerID = controllerID;

   mLocalGamertag = pUser->getName();
   mLocalXuid = pUser->getXuid();

   return S_OK;
}

//==============================================================================
// 
//==============================================================================
void BPartySession::update()
{
   if (mAbort)
      return;

   if (mpLiveSession)
   {
      mpLiveSession->update();
      if (mState < cBPartySessionStateShuttingDown)
      {
         if (mpLiveSession->sessionHasError())
         {
            //Live Session has died - I'm screwed!
            setState(cBPartySessionStateError);
            if (gLiveSystem->getMPSession())
            {
               gLiveSystem->setPartySystemForReenter(false);
               gLiveSystem->getMPSession()->abortPartySession();
            }        
            return;
         }
         uint startingSearchIndex = 0;
         XUID xuidAddFailed = mpLiveSession->getNextUserAddFailure(startingSearchIndex);
         while (xuidAddFailed !=0 )
         {
            nlog(cMPPartySystemCL, "BPartySession::update - Found an XUID [%I64u] that failed in getting added to the Live session", xuidAddFailed);
            //Find the member for whom this failed
//-- FIXING PREFIX BUG ID 1466
            const BPartySessionPartyMember* failedMember = findPartyMemberByXUID(xuidAddFailed);
//--
            if (failedMember)
            {
               //Drop them
               nlog(cMPPartySystemCL, "BPartySession::update - Dropping member with clientID [%d] because they could not be added to the Live session", failedMember->mClientID );
               dropPartyMemberByClientID(failedMember->mClientID);
            }
            //See if there are any more
            startingSearchIndex++;
            xuidAddFailed = mpLiveSession->getNextUserAddFailure(startingSearchIndex);
         }
      }
      else
      {
         //Im in the shutting down state - check if the livesession has errored out instead of shutting down
         if (mpLiveSession->sessionHasError())
         {
            mpLiveSession->deinit();
         }
      }
   }

   //Give the BSession a chance to update
   if (mpSession)
   {
      mpSession->service();
      if (mpSession->isDisconnecting() && mpSession->getActiveClientAmount() == 0)
      {
         mpSession->removeObserver(this);
         mpSession->dispose();
         HEAP_DELETE(mpSession, gNetworkHeap);
         mpSession = NULL;
      }
   }

   //See if we need query for an updated config file
   if (((mState==cBPartySessionStateReady) ||
        (mState==cBPartySessionStateJoiningReady)) &&
       (!gLiveSystem->getMPSession()->isInLANMode()) &&
       (!gLiveSystem->getMPSession()->isGameRunning()) &&
       (!gLiveSystem->getMPSession()->isMatchmakingRunning()))
   {
      //Is there one there waiting for us?
      if (gLiveSystem->getHopperListUpdatedVersion()->isLoaded())
      {
         gLiveSystem->postUpdatedVersionToBaseHopperList();
      }
      else
      {
         //Request a new one - TTL will stop the request if we have not waited long enough
         gLSPManager.requestConfigData(gLiveSystem->getHopperListUpdatedVersion());
      }
   }

   //State dependent internal updates *************************************************
   switch (mState)
   {
   case (cBPartySessionStateReady) :
   case (cBPartySessionStateJoiningWaitingForInitialSetup):
   case (cBPartySessionStateJoiningReady):
      {
         //Update for each of the pending player connections if we are in pre-game
         //updatePendingPlayers(); - No more pending player list
         break;
      }

   case (cBPartySessionStateStartingWaitingForXNAddr) :
      {
         XNADDR localXnAddr;
         if (gLiveSystem->getLocalXnAddr(localXnAddr))
         {
            //I have an XNAddr - go to the next phase
            //Live mode - need to set Live login as required
            //gLiveSystem->setLiveRequired( !gLiveSystem->getMPSession()->isInLANMode() );
            setState(cBPartySessionStateStartingWaitingForLoggin);
            //Wait til we have a live login
         }
         break;
      }

   case (cBPartySessionStateStartingWaitingForLoggin) :
      {
         //Am I done starting up?
         //Yes - If i'm logged in if I need to be
//-- FIXING PREFIX BUG ID 1467
         const BUser* pUser = gUserManager.getPrimaryUser();
//--
         if (pUser && (pUser->isSignedIn() || gLiveSystem->getMPSession()->isInLANMode()))
         {
            if (FAILED(setControllerID(pUser->getPort())) || mLocalControllerID == XUSER_MAX_COUNT)
            {
               // failed to set our controller ID
               setState(cBPartySessionStateError);
               mSessionInterface->BPartySessionEvent_systemStartupFailed(BSession::cLiveSessionFailure);
            }
            else if (mStartupMode == cBPartySessionStartupHosting)
            {
               hostPartyStartup();
            }
            else if (mStartupMode == cBPartySessionStartupJoining)
            {
               joinPartyStartup();
            }
            else
            {
               BASSERT(false);
            }
         }
         else
         {
   //TODO TIMEOUT TRAP!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //Watch for errors in the LiveSession
            //Remove this crap
            /*
            if ((!mpLiveSession) ||
                (mpLiveSession->sessionHasError()))
            {
               //There was a problem
//               mSessionInterface->BPartySessionEvent_ESOConnectFailed(cBPartySessionESOConnectCouldNotRequestLogin);
               mState = cBPartySessionStateError;
               return;
            }
            */
         }
         break;
      }

   case (cBPartySessionStateIdle) :
      {
         //TODO remove
         break;
      }

   case (cBPartySessionStateLaunchHostWaitingForSessionCreation) :
      {
         //LIVE game hosting - waiting for the Live-side session to be created and joined
         if (mStartupMode == cBPartySessionStartupHosting)
         {
            hostStartupProcessing();              
         }
         else if (mStartupMode == cBPartySessionStartupJoining)
         {
            joinStartupProcessing();
         }
         else
         {
            BASSERT(false);
         }
         break;
      }
   case (cBPartySessionStateShuttingDownToReset):
   case (cBPartySessionStateShuttingDown):
      {
         //BASSERT(mpLiveSession);
         if (mpLiveSession && mpLiveSession->isShutdown())
         {            
            delete mpLiveSession;
            mpLiveSession = NULL;
         }
         if (!mpLiveSession)
         {
            if (mState==cBPartySessionStateShuttingDown)
            {
               nlog(cMPPartySystemCL, "BPartySession::update - live session is shutdown, deleting it. Reset is complete - safe to delete BPartySession");
               mState = cBPartySessionStateFinished;
            }
            else
            {
               nlog(cMPPartySystemCL, "BPartySession::update - live session is shutdown, deleting it. Reset is complete - returning to ready to startup state");
               mState = cBPartySessionStateStartingWaitingForXNAddr;
            }
         }
         break;
      }
   }     //end of switch (mState)
}

//==============================================================================
// 
//==============================================================================
void BPartySession::updateBroadcastedHostData()
{
   if (!isHosting())
   {
      //Currently - only the host has data that you can query without being in the session already
      return;
   }

   if (!gLiveSystem->getMPSession())
   {
      return;
   }
   if (!gLiveSystem->getMPSession()->isInLANMode())
   {
      //Update the information which is in the host's QoS response
      Utils::FastMemSet(&mQoSResponseData, 0 ,sizeof(mQoSResponseData));
      mQoSResponseDataSize = cBQoSResponseDataBaseSize;
      mQoSResponseData.mCheckSum = mLocalChecksum;
      mQoSResponseData.mNonce = mpLiveSession->getNonce();
      /*
      mQoSResponseData.mClientCount = 0;

      BDEBUG_ASSERT(getSession());
      if (getSession() == NULL)
         return;

      for (int i=0; i < getSession()->getMachineCount(); ++i)
      {
//-- FIXING PREFIX BUG ID 1468
         const BMachine* pMachine = getSession()->getMachine(i);
//--
         // FIXME-COOP, why are we excluding the local machine in our client out?
         // we obviously have at least one locally but now we could have two.
         if (pMachine != NULL && !pMachine->isLocal() && pMachine->isConnected())
         {
            mQoSResponseData.mClientXNAddrs[mQoSResponseData.mClientCount] = pMachine->getXnAddr();
            mQoSResponseData.mClientCount++;
            mQoSResponseDataSize += sizeof(XNADDR);
         }
      }
      */

      mQoSResponseData.mPublicSlotsOpen = (uint8)(mPublicSlots - getPartyCount());
      mQoSResponseData.mPublicSlots = (uint8)mPublicSlots;

      INT hr;         
      hr = XNetQosListen(&mLocalXNKID, reinterpret_cast<BYTE*>(&mQoSResponseData), mQoSResponseDataSize, 0, (XNET_QOS_LISTEN_SET_DATA));
      if (hr != 0)
      {
         //QoS start failed.
         nlog(cMPPartySystemCL, "BPartySession::updateBroadcastedHostData - host data update request failed, error code %d", hr);
      }
   }
   else
   {
      BXLanDiscovery* pLanDiscovery = gLiveSystem->getMPSession()->getLanDiscovery();
      if (pLanDiscovery == NULL)
         return;

      XNADDR localXnAddr;
      if (!gLiveSystem->getLocalXnAddr(localXnAddr))
         return;

      BLanGameInfo& lanInfo = pLanDiscovery->getLocalInfo();

#ifdef TITLEID_HALO_WARS_ALPHA
      uint titleID = TITLEID_HALO_WARS_ALPHA;
#else
      uint titleID = TITLEID_HALO_WARS;
#endif

      lanInfo.init(titleID, mLocalChecksum);

      BUString info;
      info.format(L"%S", mLocalGamertag.getPtr());
      lanInfo.setInfo(info);
      lanInfo.setLocked(mPartyLocked);
      lanInfo.setSlots(getPartyCount(), getPartyMaxSize());
      lanInfo.setMapIndex(mCurrentHostSettings.mMapIndex);
      lanInfo.setGameType(mCurrentHostSettings.mGameMode);
      lanInfo.setDifficulty(mCurrentHostSettings.mDifficulty);

      lanInfo.setXnAddr(localXnAddr);
      lanInfo.setXnKey(mLocalXNKey);
      lanInfo.setXnKID(mLocalXNKID);

      // this will only issue a broadcast if we've never sent one before
      // future updates/changes will broadcast on an internal timer
      pLanDiscovery->initialBroadcast();
   }
}

//==============================================================================
// 
//==============================================================================
void BPartySession::setQoSNotification(BOOL enabled)
{
   if (gLiveSystem->getMPSession()->isInLANMode())
   {
      //No QoS in LAN mode
      return;
   }

   nlog(cMPPartySystemCL, "BPartySession::setQoSNotification");
   if (mQoSResponding == enabled)
   {
      //I'm already doing what you want
      nlog(cMPPartySystemCL, "BPartySession::setQoSNotification - already in requested state");
      return;
   }

   if (mQoSResponding)
   {
      //Stop it
      mQoSResponding = false;
      INT hr = XNetQosListen(&mLocalXNKID, NULL, 0, 0, (XNET_QOS_LISTEN_RELEASE));
      if (hr != 0)
      {
         //QoS stop failed.
         nlog(cMPPartySystemCL, "BPartySession::setQoSNotification - stop request failed, error code %d", hr);
         return;
      }
      nlog(cMPPartySystemCL, "BPartySession::setQoSNotification - response stopped");
   }
   else
   {
      //Start Qos Response    
      if (isHosting())
      {
         //Call refresh to fill in the state data about this host
         updateBroadcastedHostData();   
         INT hr;         
         hr = XNetQosListen(&mLocalXNKID, reinterpret_cast<BYTE*>(&mQoSResponseData), mQoSResponseDataSize, 0, (XNET_QOS_LISTEN_ENABLE|XNET_QOS_LISTEN_SET_DATA));
         if (hr != 0)
         {
            //QoS start failed.
            nlog(cMPPartySystemCL, "BPartySession::setQoSNotification - host data start request failed, error code %d", hr);
            return;
         }
      }
      else
      {
         //Clients host out no additional Qos information
         INT hr;
         hr = XNetQosListen(&mLocalXNKID, NULL, 0, 0, (XNET_QOS_LISTEN_ENABLE));
         if (hr != 0)
         {
            //QoS start failed.
            nlog(cMPPartySystemCL, "BPartySession::setQoSNotification - client data start request failed, error code %d", hr);
            return;
         }
      }
      nlog(cMPPartySystemCL, "BPartySession::setQoSNotification - response running");
      mQoSResponding = true;
   }
}


//==============================================================================
// 
//==============================================================================
BOOL BPartySession::isHosting() const
{
   if ((!isRunning()) || (!getSession()))
      return FALSE;

   return (getSession()->isHosted());
}

//==============================================================================
// 
//==============================================================================
BOOL BPartySession::isRunning() const
{
   if ((mState >= cBPartySessionStateIdle) &&
      (mState < cBPartySessionStateDeleting) &&
      (mpSession))
   {
      return TRUE;
   }
   return FALSE;
}

//==============================================================================
//
//==============================================================================
void BPartySession::hostPartyStartup()
{
   //HRESULT hr = setControllerID(mLocalControllerID);
   //BASSERT(SUCCEEDED(hr));
   //hr;

   //Set some variables
   setState(cBPartySessionStateLaunchHostWaitingForSessionCreation);
   mGameTypeIndex = CONTEXT_GAME_MODE_PARTYHOPPER;
   mPublicSlots = 2;
   mPort = cPartySessionHostingPort;

   //Create a LIVE session and post it up
   BASSERT(mpLiveSession==NULL);
   //Constructor for starting a Live matchmaking session
   if (!gLiveSystem->getMPSession()->isInLANMode())
   {
      mpLiveSession = new BLiveSession( mLocalControllerID, mPublicSlots );
   }
   else
   {
//-- FIXING PREFIX BUG ID 1469
      const BUser* pUser = gUserManager.getPrimaryUser();
//--
      if (pUser == NULL)
         return;

      //Generate a local key pair
      if (XNetCreateKey( &mLocalXNKID, &mLocalXNKey)!=0)
      {
         nlog(cMPPartySystemCL, "BPartySession::hostStartupLocal -- Failed, could not create key - Last Error: %d", GetLastError());
         reset();
         return;
      }
      if (XNetRegisterKey( &mLocalXNKID, &mLocalXNKey )!=0)
      {
         nlog(cMPPartySystemCL, "BPartySession::hostStartupLocal -- Failed, could not register key - Last Error: %d", GetLastError());
         reset();
         return;
      }

      mLanSecurityKeyRegistered = TRUE;

      //Make sure we have a game interface
      if (!gLiveSystem->getMPSession()->getGameInterface())
      {
         nlog(cMPPartySystemCL, "BPartySession::hostStartupLocal -- Failed, mpSession has no game interface");
         reset();
         return;
      }

      //Get our local machine's XNADDR
      XNADDR localXnAddr;
      if (!gLiveSystem->getLocalXnAddr(localXnAddr))
      {
         nlog(cMPPartySystemCL, "BPartySession::hostStartupLocal -- Failed, machine had no XNADDR");
         reset();
         return;
      }

      BDEBUG_ASSERT(mpSession == NULL);
      if (mpSession != NULL)
      {
         mpSession->dispose();
         HEAP_DELETE(mpSession, gNetworkHeap);
      }

      //Start this session up
      mPort = cPartySessionHostingPort;
      mpSession = HEAP_NEW(BSession, gNetworkHeap);
      mpSession->init(mLocalChecksum, this, NULL, mpVoice, true);
      mpSession->addObserver(this);
      mpSession->setLocalXnAddr(localXnAddr);
      mpSession->setMaxClientCount(cPartySessionMaxUsers);
      mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

      //Start voice
      gLiveSystem->getLiveVoice()->initSession(BVoice::cPartySession, mpSession->getConnectionEventHandle(), true);

      //Start the session hosting
      HRESULT hr = mpSession->host(mLocalXNKID, mPort);
      BASSERT( hr==S_OK );
      if (hr!=S_OK)
      {
         nlog(cMPPartySystemCL, "BPartySession::hostStartupLocal -- Failed, BSession/socket error");
         reset();
         return;
      }

      //Wait for the session layer to say its connected - then we are ready
      setState(cBPartySessionStateReady);
   }

   //All done - next update loop will see if we are ready to continue host processing
}

//==============================================================================
//This processes the startup of a hosting session
//  It is state driven as at several points (LAN or Live) the system must wait for events before continuing on
//  Generally the UPDATE method is looking for those events and then calling this to continue once those conditions have been met
//==============================================================================
void BPartySession::hostStartupProcessing()
{
   if (mState == cBPartySessionStateLaunchHostWaitingForSessionCreation)
   {
      if (!mpLiveSession || mpLiveSession->sessionHasError() )
      {
         //No Live session - complete failure here - todo log/debug
         mSessionInterface->BPartySessionEvent_systemStartupFailed(BSession::cLiveSessionFailure);
         setState(cBPartySessionStateError);
         return;
      }
      if (mpLiveSession->isSessionValid())
      {
//-- FIXING PREFIX BUG ID 1470
         const BUser* pUser = gUserManager.getPrimaryUser();
//--
         if (!pUser)
            return;

         //Session has been created and is connected

         //Get our local machine's XNADDR
         XNADDR localXnAddr;
         if (!mpLiveSession->getSessionHostXNAddr(localXnAddr))
         {
            //No Live session - complete failure here - todo log/debug
            reset();
            return;
         }

         if (!mpLiveSession->getXNKID(mLocalXNKID))
         {
            //No Live session - complete failure here - todo log/debug
            reset();
            return;
         }

         BDEBUG_ASSERT(mpSession == NULL);
         if (mpSession != NULL)
         {
            mpSession->dispose();
            HEAP_DELETE(mpSession, gNetworkHeap);
         }

         //Start/end the party session so that Live is happy
         //Might have to do this for cert - eric
         //mpLiveSession->startEndPartySession();

         //Start this session up
         mpSession = HEAP_NEW(BSession, gNetworkHeap);
         BDEBUG_ASSERT(mpVoice);
         mpSession->init(mLocalChecksum, this, NULL, mpVoice, true);
         mpSession->addObserver(this);
         mpSession->setLocalXnAddr(localXnAddr);
         mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

         gLiveSystem->getLiveVoice()->initSession(BVoice::cPartySession, mpSession->getConnectionEventHandle(), true);

         //Start the session hosting
         HRESULT hr = getSession()->host(mLocalXNKID, mPort);   //Just name the session the same as the host's name for now
         BASSERT(hr == S_OK);
         //TODO handle errors on session create
         hr;

         //Start QoS hosting about this session
         setQoSNotification(TRUE);

//         mSessionInterface->BPartySessionEvent_systemReady();
         setState(cBPartySessionStateReady);

         //getSession()->joinUser(pUser->getPort(), pUser->getXuid(), pUser->getName());
      }
   }
}

//==============================================================================
//
//==============================================================================
void BPartySession::joinPartyStartup()
{
   //HRESULT hr = setControllerID(mLocalControllerID);
   //BASSERT(SUCCEEDED(hr));
   //hr;

//-- FIXING PREFIX BUG ID 1471
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return;

   //Set some variables
   setState(cBPartySessionStateLaunchHostWaitingForSessionCreation);
   mGameTypeIndex = CONTEXT_GAME_MODE_PARTYHOPPER;
   mPublicSlots = cPartySessionMaxUsers;
   mPort = cPartySessionHostingPort;

   //Create a LIVE session and post it up
   BDEBUG_ASSERT(mpLiveSession == NULL);
   //Constructor for starting a Live matchmaking session
   if (!gLiveSystem->getMPSession()->isInLANMode())
   {
      mpLiveSession = new BLiveSession(mLocalControllerID, mPublicSlots, mHostTarget.hostInfo.hostAddress, mHostTarget.hostInfo.sessionID, mHostTarget.hostInfo.keyExchangeKey);
   }
   else
   {
      nlog(cMPGameCL, "BMPGameSession::joinLanGame");

      XNADDR localXnAddr;
      if (!gLiveSystem->getLocalXnAddr(localXnAddr))
      {
         nlog(cMPPartySystemCL, "BPartySession::hostStartupLocal -- Failed, machine had no XNADDR");
         reset();
         return;
      }

      mPort = cPartySessionHostingPort;

      BDEBUG_ASSERT(mpSession == NULL);
      if (mpSession != NULL)
      {
         mpSession->dispose();
         HEAP_DELETE(mpSession, gNetworkHeap);
      }

      //Start this session up
      mpSession = HEAP_NEW(BSession, gNetworkHeap);
      mpSession->init(mLocalChecksum, this, NULL, mpVoice, true);
      mpSession->addObserver(this);
      mpSession->setLocalXnAddr(localXnAddr);
      mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

      gLiveSystem->getLiveVoice()->initSession(BVoice::cPartySession, mpSession->getConnectionEventHandle(), true);

      mLocalXNKID = mLanInfo.getXnKID();
      mLocalXNKey = mLanInfo.getXnKey();

      //LAN mode key management
      if (mLanSecurityKeyRegistered)
      {
         XNetUnregisterKey( &mLocalXNKID );
         mLanSecurityKeyRegistered = FALSE;
      }

      if (XNetRegisterKey(&mLocalXNKID, &mLocalXNKey) != 0)
      {
#ifndef BUILD_FINAL
         DWORD lastErr = GetLastError();
         nlog(cMPPartySystemCL, "BPartySession::join -- Failed, could not register key - Last Error: %d", lastErr);
#endif
         reset();
         return;
      }

      mLanSecurityKeyRegistered = TRUE;

      if (!mpSession->join(mLanInfo.getXnAddr(), mPort, mLocalXNKID))
      {
         XNetUnregisterKey(&mLocalXNKID);
         mLanSecurityKeyRegistered = TRUE;
         reset();
         return;
      }

      setState(cBPartySessionStateReady);
   }
   //All done - next update loop will see if we are ready to continue host processing
}

//==============================================================================
//
//==============================================================================
void BPartySession::joinStartupProcessing()
{
   if (mState == cBPartySessionStateLaunchHostWaitingForSessionCreation)
   {
      if (!mpLiveSession || mpLiveSession->sessionHasError() )
      {
         //No Live session - complete failure here - todo log/debug
         mSessionInterface->BPartySessionEvent_systemStartupFailed(BSession::cLiveSessionFailure);
         setState(cBPartySessionStateError);
         return;
      }
      if (mpLiveSession->isSessionValid())
      {
         //Session has been created and is connected
//-- FIXING PREFIX BUG ID 1472
         const BUser* pUser = gUserManager.getPrimaryUser();
//--
         if (!pUser)
            return;

         //Get our local machine's XNADDR
         XNADDR localXnAddr;
         if (!gLiveSystem->getLocalXnAddr(localXnAddr))
         {
            //No Live session - complete failure here - todo log/debug
            reset();
            return;
         }

         if (!mpLiveSession->getXNKID(mLocalXNKID))
         {
            //No Live session - complete failure here - todo log/debug
            reset();
            return;
         }

         BDEBUG_ASSERT(mpSession == NULL);
         if (mpSession != NULL)
         {
            mpSession->dispose();
            HEAP_DELETE(mpSession, gNetworkHeap);
         }

         //Start this session up
         mpSession = HEAP_NEW(BSession, gNetworkHeap);
         BDEBUG_ASSERT(mpVoice);
         mpSession->init(mLocalChecksum, this, NULL, mpVoice, true);
         mpSession->addObserver(this);
         mpSession->setLocalXnAddr(localXnAddr);
         mpSession->addUser(pUser->getPort(), pUser->getXuid(), pUser->getName());

         gLiveSystem->getLiveVoice()->initSession(BVoice::cPartySession, mpSession->getConnectionEventHandle(), true);

         //Start the session hosting
         bool retval = getSession()->join(mHostTarget.hostInfo.hostAddress, mPort, mHostTarget.hostInfo.sessionID);
         BDEBUG_ASSERTM(retval, "Failed to join host");
         retval;

         //Start QoS hosting about this session
         setQoSNotification(TRUE);

         //mSessionInterface->BPartySessionEvent_systemReady();
         setState(cBPartySessionStateReady);        
      }
   }
}

//==============================================================================
//
//==============================================================================
void BPartySession::handlePartyMatchMakingCommand(ClientID client, uint8 commandCode, uint64 nonce)
{
   //Processes match making commands sent around through the party system session
   //  Does not handle "JoinTarget"
   BASSERT(mSessionInterface);

   //Get the sender's party member record
   BPartySessionPartyMember* member = findPartyMemberByClientID(client);
   if (!member)
   {
      //Somehow I've a message from a client that is not in my party....
      nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - message command [%d] was from clientID [%d] that is not in my party member list", commandCode, client);
      dropPartyMemberByClientID(client);
      return;               
   }

   //If a machine has a secondary player, then whenever the primary player sends a matckmaking command
   //  treat it as if the secondary player has sent the SAME command
   //TODO CO-OP FIXME - this is a hack! will remove soon - eric
   BPartySessionPartyMember* secondaryMember = NULL;
   //secondaryMember = findSecondaryMemberFromThisPrimaryMember(member);

   //Process the command
   switch (commandCode)
   {
      case (cBPartySessionCommandLeaveTarget):
      case (cBPartySessionCommandDisconnected):
      case (cBPartySessionCommandJoinFailure):
         {
            BSimString commandName = "?";
            switch (commandCode)
            {
              case (cBPartySessionCommandLeaveTarget):{commandName="cBPartySessionCommandLeaveTarget";break;}
              case (cBPartySessionCommandDisconnected):{commandName="cBPartySessionCommandDisconnected";break;}
              case (cBPartySessionCommandJoinFailure):{commandName="cBPartySessionCommandJoinFailure";break;}
            }
            nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Processing %s", commandName.getPtr());
            member->mState = cBPartySessionMemberStateIdle;
            if (secondaryMember)
            {
               secondaryMember->mState = cBPartySessionMemberStateIdle;
            }

            //If I'm the host - then check and once every is back to idle - start looking again
            if (isHosting() && (commandCode != cBPartySessionCommandLeaveTarget))
            {
               if (commandCode == cBPartySessionCommandJoinFailure && gLiveSystem->getMPSession()->getSessionInterface() && !gLiveSystem->getMPSession()->isInMatchmakingMode())
                  gLiveSystem->getMPSession()->getSessionInterface()->mpSessionEvent_gameSessionJoinFailed(BSession::cResultHostConnectFailure);

               uint memberCount = 0;
               uint membersIdle = 0;
               uint membersGreenedUp = 0;
               for (uint i=0;i<cPartySessionMaxUsers;i++)
               {
                  //Note - leave this as a clientID check so that it skips AI (which have an invalid clientID)
                  if (mMembers[i].mClientID != cMPInvalidClientID)
                  {
                     memberCount++;
                     //Valid member in this slot
                     if (mMembers[i].mState==cBPartySessionMemberStateIdle)
                     {
                        membersIdle++;
                     } 
                     if (mMembers[i].mSettings.mConnectionState==cBPartySessionMemberConnectionStateReadyToStart)
                     {
                        membersGreenedUp++;
                     }
                  }
               }
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - There are %d members, %d are idle", memberCount, membersIdle );
               if (memberCount==membersIdle)
               {
                  //At this point everyone is out of the previous GAME host target
                  //  SO I need to continue matchmaking - or it is ok to go ahead and let them launch a game again
                  //NOTE - this code make get hit multiple time if there are 'disconnected!' messages still out on the wire even after the host
                  //    has seen that everyone is out of their game session (its chatty)
                  if ((gLiveSystem->getMPSession()->isInMatchmakingMode()) &&
                      (membersGreenedUp==memberCount))
                  {
                     //Added check here in case a client drops at the magic time in matchmaking
                     // Look that we still have the right number of folks for this hopper
                     if (membersGreenedUp != gLiveSystem->getMPSession()->getMaxMembersPerTeamForCurrentMatchMakingSearch())
                     {
                        //We are wrong!
                        nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand -Was going to restart matchmaking, but we have the wrong number of folks now");
                        gLiveSystem->getMPSession()->matchMakingFailed(BLiveMatchMaking::cBLiveMatchMakingErrorWrongNumberOfPartyMembers);
                        return;
                     }
                     nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Host resuming matchmaking" );
                     gLiveSystem->getMPSession()->resumeMatchmaking();
                  }
                  return;
               }
            }

            //Am I in a session?
            if ((!gLiveSystem->getMPSession()) ||
                (!gLiveSystem->getMPSession()->getSession()))
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Not in a session, ignoring command");
               return;
            }
            //See if the nonce he is telling me to leave is the one I am joined to
            if (gLiveSystem->getMPSession()->getGameSessionNonce() != nonce)
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Command nonce [%I64u] does not match current session [%I64u] , ignoring command", nonce, gLiveSystem->getMPSession()->getGameSessionNonce());
               return;
            }
            //Am I in a game that is running
            if (gLiveSystem->isMultiplayerGameActive())
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Ignoring command leave command, the game I am in is running");
               return;
            }
            //All matches up - tell BMPSession to drop everything but matchmaking and the party system
            gLiveSystem->getMPSession()->abortGameSession();

            break;
         }

      case (cBPartySessionCommandJoinSuccess):
         {
            //Only host cares about this to see if everyone is in
            if (!isHosting())
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Ignoring cBPartySessionCommandJoinSuccess because I am not the party host");
               return;
            }
            nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Processing cBPartySessionCommandJoinSuccess");
            //Am I in a session?
            if ((!gLiveSystem->getMPSession()) ||
                (!gLiveSystem->getMPSession()->getSession()))
            {
               //So I'm the host, not in a session, but that dude is reporting he is - tell him (and any else in it to get out)
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Client id [%d] reported he has joined nonce [%I64u] while host is not in session - telling everyone to leave that nonce", client, nonce);
               partySendLeaveTargetCommand(nonce);
               return;
            } 
            //Is it the same session?           
            if (gLiveSystem->getMPSession()->getGameSessionNonce() != nonce)
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Client id [%d] reported he has joined nonce [%I64u] while host is in session [%I64u] - telling everyone to leave that nonce", client, nonce, gLiveSystem->getMPSession()->getGameSessionNonce());
               partySendLeaveTargetCommand(nonce);
               return;
            }
            //Then we are in the same session - mark him as in
            BASSERT(member->mState==cBPartySessionMemberStateJoining);
            member->mState=cBPartySessionMemberStateJoined;
            if (secondaryMember)
            {
               secondaryMember->mState = cBPartySessionMemberStateJoined;
            }

            //Check to see if everyone is joined
            uint memberCount = 0;
            uint membersJoined = 0;
            for (uint i=0;i<cPartySessionMaxUsers;i++)
            {
               if (mMembers[i].mClientID != cMPInvalidClientID)
               {
                  memberCount++;
                  //Valid member in this slot
                  if (mMembers[i].mState==cBPartySessionMemberStateJoined)
                  {
                    membersJoined++;
                  } 
                  else if (mMembers[i].mState==cBPartySessionMemberStateIdle)
                  {
                     //Hmmm - we have someone who either:
                     //  Joined AFTER we sent out the command - bad because we should have never sent the command unless the party was full then locked
                     //  Got the command but then failed to join, but that failure didn't cause the host to get everyone to exit that target
                     nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - We have a member who is idle while everyone should be trying to join a target memindex/cid[%d/%d], aborting join for everyone", i, client );
                     BASSERT(false);
                     partySendLeaveTargetCommand(nonce);
                     return;
                  }
               }
            }
            nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - There are %d members, %d are joined", memberCount, membersJoined );
            if (memberCount==membersJoined)
            {
               //We are good to go - send the green light
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Host sees all party members in game session [%I64u]", nonce );
               gLiveSystem->getMPSession()->allPartyMembersJoinedTarget();
               //Party members now green themselves up when they see a full game session
               //partySendGreenUpCommand(nonce);
            }
            break;
         }

      case (cBPartySessionCommandGreenUp):
         {
            //Deprecated
            BASSERT(false);

            nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Processing cBPartySessionCommandGreenUp");
            //Am I in a session?
            if ((!gLiveSystem->getMPSession()) ||
               (!gLiveSystem->getMPSession()->getSession()))
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Not in a session, telling everyone to leave thier session");
               partySendLeaveTargetCommand(nonce);
               return;
            }
            //See if the nonce he is telling me to leave is the one I am joined to
            if (gLiveSystem->getMPSession()->getGameSessionNonce() != nonce)
            {
               nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - Command nonce [%I64u] does not match current session [%I64u] , telling everyone to leave either session", nonce, gLiveSystem->getMPSession()->getGameSessionNonce());
               partySendLeaveTargetCommand(gLiveSystem->getMPSession()->getGameSessionNonce());
               partySendLeaveTargetCommand(nonce);
               return;
            }
            //All matches up - tell BMPSession to drop everything but matchmaking and the party system
            gLiveSystem->getMPSession()->sendLaunchReady(true);
            break;
         }

      default:
      {
         nlog(cMPPartySystemCL, "BPartySession::handlePartyMatchMakingCommand - ERROR: Unknown command code %d", commandCode);
         BASSERT(false);
      }
   }
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendJoinTargetCommand(uint8 targetHopper, const XNADDR& targetXNADDR, const XNKEY& targetXNKEY, const XNKID& targetXNKID, uint64 nonce)
{
   //Sent from the host out to everyone (host will ignore)
   BPartyMemberMatchMakingJoinCommand packet(cBPartySessionCommandJoinTarget, targetHopper, nonce, targetXNADDR, targetXNKID, targetXNKEY);
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   }
   
   BASSERT(getSession()->isHosted());
   BASSERT(mPartyLocked == true);
   nlog(cMPPartySystemCL, "BPartySession::partySendJoinTargetCommand - Sending command for everyone to join target nonce %I64u", nonce);

   //Set everyone's state to joining
   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      if (mMembers[i].mClientID != cMPInvalidClientID)
      {
         BASSERT(mMembers[i].mState==cBPartySessionMemberStateIdle);    //Check to make sure host aint sending out another join until everyone is idle and reset
         mMembers[i].mState=cBPartySessionMemberStateJoining;
      }
   }

   //Set everyone in host list to join pending
   getSession()->SendPacket(packet);
}

//==============================================================================
//Has everyone returned to an idle state after a game launch
//==============================================================================
bool BPartySession::isPartyAtIdleState()
{
   if (!getSession())
   {
      //Party session is dead - abort
      return false;
   }

   uint memberCount = 0;
   uint membersIdle = 0;
   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      //Note - leave this as a clientID check so that it skips AI (which have an invalid clientID)
      if (mMembers[i].mClientID != cMPInvalidClientID)
      {
         memberCount++;
         //Valid member in this slot
         if (mMembers[i].mState==cBPartySessionMemberStateIdle)
         {
            membersIdle++;
         } 
      }
   }

   return (memberCount==membersIdle);
}

//==============================================================================
//
//==============================================================================
void BPartySession::partyResetJoinState()
{
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   }

   //Set everyone's state to idle
   for (uint i=0;i<cPartySessionMaxUsers;i++)
   {
      if (mMembers[i].mClientID != cMPInvalidClientID)
      {
         mMembers[i].mState=cBPartySessionMemberStateIdle;
      }
   }
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendLeaveTargetCommand(uint64 nonce)
{
   //Sent from the host out to everyone (host will ignore)
   BPartyMemberMatchMakingCommand packet(cBPartySessionCommandLeaveTarget, nonce);
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   }
   nlog(cMPPartySystemCL, "BPartySession::partySendLeaveTargetCommand - Sending command for everyone to leave target nonce[%I64u]", nonce);
   BASSERT(getSession()->isHosted());
   getSession()->SendPacket(packet); 
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendGreenUpCommand(uint64 nonce)
{
   //Deprecated
   BASSERT(false);

   //Sent from the host out to everyone (host will ignore)
   BPartyMemberMatchMakingCommand packet(cBPartySessionCommandGreenUp, nonce);
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   }
   BASSERT(getSession()->isHosted());
   nlog(cMPPartySystemCL, "BPartySession::partySendGreenUpCommand - Sending command for everyone to green up (target nonce %I64u)", nonce);
   getSession()->SendPacket(packet); 
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendMatchmakingStatusInfo(uint8 status, uint data1, uint data2)
{
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   }
   BASSERT(getSession()->isHosted());
   BPartyMatchMakingStatusInfo packet((uint8)status, data1, data2);
   nlog(cMPPartySystemCL, "BPartySession::partySendMatchmakingStatusInfo - Sending new status info out");
   getSession()->SendPacket(packet); 
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendJoinSuccessCommand(uint64 nonce)
{
   //Send from clients to the host (including from the host himself)
   BPartyMemberMatchMakingCommand packet(cBPartySessionCommandJoinSuccess, nonce);
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   } 
   BMachine* pHost = getSession()->getHostMachine();
   if (!pHost)
   {
      return;
   }
   nlog(cMPPartySystemCL, "BPartySession::partySendJoinSuccessCommand - Sending command to host that I joined target nonce %I64u", nonce);
   getSession()->SendPacketTo(pHost, packet);
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendJoinFailureCommand(uint64 nonce)
{
   //Send from clients to the host (including from the host himself)
   BPartyMemberMatchMakingCommand packet(cBPartySessionCommandJoinFailure, nonce);
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   } 
   BMachine* pHost = getSession()->getHostMachine();
   if (!pHost)
   {
      return;
   }
   nlog(cMPPartySystemCL, "BPartySession::partySendJoinFailureCommand - Sending command to host that I FAILED to joined target nonce %I64u", nonce);
   getSession()->SendPacketTo(pHost, packet);
}

//==============================================================================
//
//==============================================================================
void BPartySession::partySendDisconnectedFromTargetCommand(uint64 nonce)
{
   //Send from clients to the host (including from the host himself)
   BPartyMemberMatchMakingCommand packet(cBPartySessionCommandDisconnected, nonce);
   if (!getSession())
   {
      //Party session is dead - abort
      return;
   }
   BMachine* pHost = getSession()->getHostMachine();
   if (!pHost)
   {
      return;
   }
   nlog(cMPPartySystemCL, "BPartySession::partySendDisconnectedFromTargetCommand - Sending command to host that I DISCONNECTED from target nonce %I64u", nonce);
   getSession()->SendPacketTo(pHost, packet);

   //If I am the party host - then I should also send out a msg telling them to leave this session
   if (isHosting())
   {
      partySendLeaveTargetCommand(nonce);
   }
}  

//==============================================================================
//
//==============================================================================
void BPartySession::lockPartyMembers(bool lock)
{
   if (mPartyLocked == lock)
   {
      return;
   }

   nlog(cMPPartySystemCL, "BPartySession::lockPartyMembers - Setting party lock to [%d]", lock);

   //Update live session
   if (mpLiveSession)
   {
      mpLiveSession->modify(!lock, mPublicSlots);
   }

   mPartyLocked = lock;

   updateBroadcastedHostData();
}

//==============================================================================
//
//==============================================================================
void BPartySession::changeMaxMemberCount(DWORD newMaxMemberCount)
{
   if (mPublicSlots == newMaxMemberCount)
   {
      //no change
      return;
   }

   mPublicSlots = newMaxMemberCount;

   //Do I need to lock/unlock the party because we full up on AI?
   if (getPartyCount() >= mPublicSlots)
   {
      mPartyLocked = true;
   }
   else
   {
      mPartyLocked = false;
   }


   if (mpLiveSession)
   {
      mpLiveSession->modify(!mPartyLocked, mPublicSlots);
   }

   mSessionInterface->BPartySessionEvent_partySizeChanged(mPublicSlots);
   updateBroadcastedHostData();
}

//==============================================================================
// BPartySessionPlayerSettings method
//==============================================================================
void BPartySessionPlayerSettings::setPartyMemberType(BPartySessionPartyMemberTypes value)
{
   mStatusFlags = mStatusFlags & ~cBPartySessionPlayerStatusFlagSecondaryPlayer;
   mStatusFlags = mStatusFlags & ~cBPartySessionPlayerStatusFlagAIPlayer;
   if (value==cBPartySessionPartyMemberSecondaryPlayer)
   {
      mStatusFlags |= cBPartySessionPlayerStatusFlagSecondaryPlayer;     
   }
   else if (value==cBPartySessionPartyMemberAI)
   {
      mStatusFlags |= cBPartySessionPlayerStatusFlagAIPlayer;
   }
}

//==============================================================================
// BPartySessionPlayerSettings method
//==============================================================================
BPartySessionPartyMemberTypes BPartySessionPlayerSettings::getPartyMemberType() const
{
   if (mStatusFlags & cBPartySessionPlayerStatusFlagSecondaryPlayer)
   {
      return (cBPartySessionPartyMemberSecondaryPlayer);
   }
   else if (mStatusFlags & cBPartySessionPlayerStatusFlagAIPlayer)
   {
      return (cBPartySessionPartyMemberAI);
   }
   return (cBPartySessionPartyMemberPrimaryPlayer);
}
