//==============================================================================
// liveVoice.h
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

#pragma once

#include <xaudio.h>
#include <xhv.h>

// xnetwork
#include "session.h"
#include "xnetwork.h"

// This handles all voice communications for the game, connected through Live or through LAN.
//  It maintains a list of voice info per player
//  (thus this system is only usable once you have a session established - not before, or after
// Note: that it needs to use the XAudio engine which it initializes on object creation

// Issue: Currently this system discards chat data that the audio system cannot currently process
//   So if the XAudio system is overloaded and cannot currently play the chat - that chat is lost
//   Since we are the only system using XAudio, the only time this should happen is if there is a ton
//   of chat going on at the same time or if the frame rate has tanked, in either case we probably don't
//   need to enhance this system so that next frame it can continue to put more chat in

//==============================================================================
// 
//==============================================================================
namespace BVoice
{
   enum Channel
   {
      cNone,
      cAll,
      cTeam1,
      cTeam2,
      cMax
   };

   enum Session
   {
      cPartySession = 0,
      cGameSession,
      cMaxSessions
   };

   enum
   {
      cThreadIndexVoice = cThreadIndexSimHelper,
   };
}

//==============================================================================
// 
//==============================================================================
class BVoiceSessionMuteRequestPayload : public BEventPayload
{
   public:
      BVoiceSessionMuteRequestPayload() {}
      ~BVoiceSessionMuteRequestPayload() {}

      void init(XUID xuid, BOOL mute)
      {
         mXuid = xuid;
         mMute = mute;
      }

      // BEventPayload
      void deleteThis(bool delivered)
      {
         HEAP_DELETE(this, gNetworkHeap);
      }

      XUID mXuid;
      BOOL mMute;
};

//==============================================================================
// 
//==============================================================================
class BReadProfileRequest
{
   public:
      BReadProfileRequest();
      ~BReadProfileRequest();

      void init(uint controllerID);
      void cancel();

      BOOL isComplete();

      BOOL getVoiceThruSpeakers();
      BOOL getVoiceMuted();

      XOVERLAPPED                         mOverlapped;
      DWORD                               mdwSettingSizeMax;
      XUSER_READ_PROFILE_SETTING_RESULT*  mpSettingResults;

      DWORD                               mSettingID[2];

      uint                                mControllerID;
};

//==============================================================================
// 
//==============================================================================
class BVoicePlayer
{
   public:
      BVoicePlayer();

      BVoicePlayer(const BVoicePlayer& source);
      BVoicePlayer& operator=(const BVoicePlayer& source);

      void init(uint clientID, uint controllerID, XUID xuid, const SOCKADDR_IN& addr, uint sessionId);

      uint getSessionID() const { return mSessionID; }
      bool isHeadsetPresent() const { return (mHeadset == TRUE); }
      uint getClientID() const { return mClientID; }
      XUID getXuid() const { return mXuid; }

      void setHeadset(BOOL headset) { mHeadset = headset; }

      bool isLocal() const { return mIsLocal; }

      // each player maintains their own talker list
      XUID                    mTalkerList[XNetwork::cMaxClients];
      BOOL                    mSessionMute[XUSER_MAX_COUNT];
      SOCKADDR_IN             mAddr;
      XUID                    mXuid;
      uint                    mControllerID;
      uint                    mClientID;
      uint                    mChannelCode;
      uint                    mSessionID;
      BOOL                    mHeadset;
      bool                    mIsLocal : 1;
      bool                    mIsMuted : 1;
};

class BLiveVoice;

//==============================================================================
// 
//==============================================================================
class BVoiceSession
{
   public:

      BVoiceSession() : mEventHandle(cInvalidEventReceiverHandle), mpVoice(NULL), mSessionID(BVoice::cMaxSessions) {}

      void init(BLiveVoice* pVoice, BEventReceiverHandle eventHandle, uint sessionId);
      void deinit();

      void addClient(uint clientID, uint controllerID, XUID xuid, const SOCKADDR_IN& addr);
      void removeClient(XUID xuid);
      void updateTalkers(const BVoiceTalkerListPayload& request);
      void setChannel(XUID xuid, uint channel);
      void setChannel(uint channel);
      void updateTalkerList();
      void updateHeadset(uint clientID, BOOL headset);
      void mutePlayer(uint controllerID, XUID xuid, BOOL mute);

      bool isValidXUID(XUID xuid) const;
      bool isHeadsetPresent(XUID xuid) const;

      BVoicePlayer* getPlayer(XUID xuid);

      BEventReceiverHandle getEventHandle() const { return mEventHandle; }

      BDynamicNetArray<BVoicePlayer>               mPlayers;
      BDynamicNetArray<BVoiceTalkerListPayload*>   mTalkerListQueue;
      BEventReceiverHandle                         mEventHandle;
      BLiveVoice*                                  mpVoice;
      uint                                         mSessionID;
      uint                                         mHeadsetCache;
};

//==============================================================================
// 
//==============================================================================
class BLiveVoice : public BVoiceInterface, public BEventReceiverInterface
{
   friend BVoiceSession;

   public:

      BLiveVoice();
      ~BLiveVoice();

      //Initialize voice communications with whoever is in its list currently (has a registered XUID)

      void                       init();
      void                       shutdown();

      void                       initSession(BVoice::Session session, BEventReceiverHandle eventHandle, bool set=false);

      void                       setSession(BVoice::Session session);
      void                       setChannel(BVoice::Session session, XUID xuid, BVoice::Channel channel);
      void                       setAllChannels(BVoice::Session session, BVoice::Channel channel);

      void                       migrateSessionIfAble(BVoice::Session fromSession, BVoice::Session toSession, BVoice::Channel channel);

      //Use this to set a player's XUID which will activate its voice comm (if the system has been started)
      //  It is safe to use this before the startup has been called.
      //  If the playerID drops - use this to set the XUID to zero so they are cleared out of the list

      // Use this to query if a particular player has talked recently (for the UI mainly)
      BOOL                       isTalking(BVoice::Session session, uint clientID);

      // Returns true if a headset is currently present for the client,
      //    returns true whether you've flipped the mute switch or not
      BOOL                       isHeadsetPresent(BVoice::Session session, uint clientID);

      // Query for the mute status of a given XUID, needs your controllerID
      //
      // By default, the value returned is the value you've set in your blade for
      //    a given player
      //
      // Setting the session and clientID will return the mute status after all the privilege states and
      //    console environment are taken into account for the given session
      BOOL                       isMuted(BVoice::Session session, uint controllerID, uint clientID, XUID xuid, BOOL& tcrMute, BOOL& sessionMute);

      // (Un)Mute a specific player in the specified session for the given controller
      void                       mutePlayer(BVoice::Session session, uint controllerID, XUID xuid, BOOL mute);

      //Puts into the buffer a string describing its current send/transmit/playback byte counts
      //void                       getDebugString(char* buffer, DWORD bufferLength);

      // BVoiceInterface
      static  BLiveVoice*              getInstance();
      virtual void                     addRef();
      virtual void                     release();
      virtual BVoiceBufferAllocator*   getAllocator() { return &mAllocator; }
      virtual BEventReceiverHandle     getEventHandle() const { return mVoiceEventHandle; }

   protected:
      void                       setTalking(uint session, XUID xuid, BOOL talking);
      void                       setHeadset(uint controllerID, BOOL headset);
      void                       setHeadset(XUID xuid, BOOL headset);
      void                       addClient(const BVoicePlayer& player);
      void                       removeClient(uint session, XUID xuid, bool shutdown=false);
      void                       getHeadset(XUID xuid, BOOL& headset);

   private:

      enum
      {
         cVoiceEventDeinit = cNetEventFirstUser,

         cVoiceEventInitSession,

         cVoiceEventSetSession,
         cVoiceEventSetChannel,

         cVoiceEventMigrateSession,

         cVoiceEventMutePlayer,

         cVoiceEventTimer,

         cVoiceEventXNotify,

         cVoiceEventTotal
      };

      enum
      {
         cMaxLocalTalkers = 2,
         cMaxRemoteTalkers = XNetwork::cMaxClients-1,
         cDefaultVoiceInterval = 100,
         cChatBufferSize = 200
      };

      enum
      {
         cMaxVoiceSessions = 2,
         cMaxClients = XNetwork::cMaxClients,
      };

      // used for tracking additions/subtractions from the XHV
      class BVoicePlayerInternal
      {
         public:
            BVoicePlayerInternal() :
               mXuid(0),
               //mClientID(cMaxClients),
               mControllerID(XUSER_MAX_COUNT),
               mRefCount(1),
               mHeadset(FALSE),
               mIsLocal(false),
               mInXHV(false)
            {
            }

            bool operator==(const BVoicePlayer& player)
            {
               return (mXuid == player.mXuid);
            }

            BVoicePlayerInternal& operator=(const BVoicePlayer& source)
            {
               mXuid = source.mXuid;
               //mClientID = source.mClientID;
               mControllerID = source.mControllerID;
               mRefCount = 1;
               mHeadset = source.mHeadset;
               mIsLocal = source.mIsLocal;
               return *this;
            }

            void reset()
            {
               mXuid = 0;
               //mClientID = cMaxClients;
               mControllerID = XUSER_MAX_COUNT;
               mRefCount = 1;
               mHeadset = FALSE;
               mIsLocal = false;
               mInXHV = false;
            }

            XUID getXuid() const { return mXuid; }

            uint getControllerID() const { return mControllerID; }

            bool inXHV() const { return mInXHV; }
            void setXHV(bool value) { mInXHV = value; }

            XUID mXuid;
            //uint mClientID;
            uint mControllerID;
            int  mRefCount;
            BOOL mHeadset;
            bool mIsLocal : 1;
            bool mInXHV : 1;
      };

      //Called by the network/mp layer when it has a voice data packet ready to process
      //void                       processIncomingVoice(XUID senderXuid, DWORD dataLength, const BYTE* pVoiceData);
      bool                       submitVoice(BVoiceBuffer& data);
      void                       processIncomingVoice(BVoiceBuffer& data);
      void                       processVoiceQueue();

      void                       update();

      void                       checkHeadsets();

      void                       addLocal(BVoicePlayerInternal& player);
      void                       removeLocal(BVoicePlayerInternal& player);

      void                       addRemote(BVoicePlayerInternal& player);
      void                       removeRemote(BVoicePlayerInternal& player);

      void                       mutePlayer(uint localControllerID, XUID xuid, bool mute);

      void                       setHeadset(BVoicePlayerInternal& player, BOOL headset);

      bool                       isFriend(uint localControllerID, XUID xuid) const;
      bool                       checkPrivilege(uint localControllerID, XPRIVILEGE_TYPE priv) const;
      bool                       isMuted(uint localControllerID, XUID xuid, BOOL& tcr90Mute) const;
      void                       updateTalkerList();
      void                       updateTalkerList(BVoiceSession& session, BVoicePlayer& player);

      void                       setSessionInternal(uint session);
      void                       setSessionNext(uint current);

      void                       processDeinit(const BEvent& event);

      void                       processInitSession(const BEvent& event);
      void                       processDeinitSession(const BEvent& event);
      void                       processDeinitSession(BEventReceiverHandle eventHandle);

      void                       processInitClient(const BEvent& event, const BVoiceRequestPayload& request);
      void                       processDeinitClient(const BEvent& event, const BVoiceRequestPayload& request);

      void                       processSetSession(const BEvent& event);

      void                       processVoice(const BEvent& event, BVoiceBuffer& voice);

      void                       processTalkersList(const BEvent& event, const BVoiceTalkerListPayload& request);

      void                       processHeadset(const BEvent& event);

      void                       processSetChannel(const BEvent& event, const BVoiceRequestPayload& request);
      void                       processSetChannel(const BEvent& event);

      void                       processMigrateSession(const BEvent& event);

      void                       processMute(const BEvent& event, const BVoiceSessionMuteRequestPayload& request);

      void                       queryProfiles();
      void                       queryProfiles(DWORD profileMask);
      void                       queryDirtyProfiles();

      void                       processXNotify();

      // BEventReceiverInterface
      bool                       receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      // ** WARNING **
      // many of these variables are accessed only from the voice thread
      // and should not be used from the sim thread

      // lock free allocator used to transfer data through the network layer
      BVoiceBufferAllocator      mAllocator;

      // stores data read from XHV
      BYTE                       mChatBuffer[XUSER_MAX_COUNT][cChatBufferSize];

      // mute list by session (game/party) per controller for all connected clients (by clientID)
      volatile LONG              mMuteList[BVoice::cMaxSessions][XUSER_MAX_COUNT][cMaxClients];

      // party and game get their own sessions to operate independently
      BVoiceSession              mSessions[BVoice::cMaxSessions]; // only supporting two sessions atm: party and game

      // fast lookups for a port's current XUID
      volatile XUID              mPortXuidMap[XUSER_MAX_COUNT];

      // sim thread and voice thread variants to determine whether a particular client is speaking
      volatile LONG              mIsTalking[BVoice::cMaxSessions*cMaxClients];
      BOOL                       mIsTalkingInternal[BVoice::cMaxSessions*cMaxClients];

      // collection of players for the active session
      //BDynamicNetArray<BVoicePlayerInternal> mPlayers;
      BVoicePlayerInternal       mPlayers[cMaxClients];

      BPointerList<BVoiceBuffer> mQueuedData;

      // sim thread and voice thread variants to check the status of a connected headset
      volatile LONG              mHeadsetPresent[BVoice::cMaxSessions*cMaxClients];
      BOOL                       mHeadsetPresentInternal[XUSER_MAX_COUNT];

      // read from the user profile to determine if speakers are enabled for voice traffic
      BOOL                       mSpeakersEnabled[XUSER_MAX_COUNT];
      // read from the user profile to determine if they muted their own microphone
      volatile LONG              mProfileMute[XUSER_MAX_COUNT];

      // Voice permissions array
      BOOL                       mHasVoice[XUSER_MAX_COUNT];

      // used as a catch in case the profile is modified while I'm already in the middle/completing one
      BOOL                       mProfileDirty[XUSER_MAX_COUNT];

      // tracks current requests to read from the user profile
      BReadProfileRequest*       mReadProfileRequest[XUSER_MAX_COUNT];

      // pairs up with mChatBuffer to show how much data we've read from XHV
      WORD                       mLocalDataSize[XUSER_MAX_COUNT];

      // used to communicate with the voice thread
      BEventReceiverHandle       mVoiceEventHandle;

      // used to communicate with the thread that's servicing the network connections
      // for the active session
      BEventReceiverHandle       mSessionEventHandle;
      // and it's index into the mSessions array
      uint                       mSessionIndex;

      // XHV voice support
      PIXHVENGINE                mpXHVEngine;

      // voice is polled per this timer, currently defaulting to 8Hz
      BWin32WaitableTimer        mVoiceTimer;

      // watch for mute list and user profile changes
      HANDLE                     mXNotifyHandle;

      // polling interval, defaults to 100 (10Hz)
      // set via the config VoiceSampleInterval
      uint                       mVoiceSampleInterval;

      // simply incrementer to throttle our polling of:
      // * presence of headsets
      // * pending user profile reads
      // * check if a client is "talking"
      //
      // we run those checks every X times through our update interval
      uint                       mTalkerThrottle;

      uint                       mQueuedThrottle;

      static BLiveVoice*         mpInstance;
      static uint                mRefCount;

      // check so we can properly shutdown our voice timer if it was enabled
      bool                       mVoiceTimerSet : 1;
};
