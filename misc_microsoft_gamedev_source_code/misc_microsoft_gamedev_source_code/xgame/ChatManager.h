//==============================================================================
// chatmanager.h
//
// chatmanager manages all chats
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "simtypes.h"
#include "containers\freelist.h"
#include "gamefilemacros.h"
#include "binkvideo.h"

#pragma once

class BGeneralEventSubscriber;

struct BSoundEventParams;

//==============================================================================
// BChatMessage
//==============================================================================
class BChatMessage
{
public:
   enum
   {
      cChatSpeakerSerena=0,
      cChatSpeakerForge,
      cChatSpeakerCutter,
      cChatSpeakerGod,        // voice of god
      cChatSpeakerSoldiers,   // generic soldiers
      cChatSpeakerPolice,     // arcadian police
      cChatSpeakerCivilians,  
      cChatSpeakerAnders,
      cChatSpeakerRhinoCommander,
      cChatSpeakerSpartan1,
      cChatSpeakerSpartan2,
      cChatSpeakerSpartanSniper,
      cChatSpeakerSpartanRocketLauncher,
      cChatSpeakerCovenant,
      cChatSpeakerArbiter,
      cChatSpeakerODST,
   };

   BChatMessage(long stringID, const BSimString& sound, bool bQueueSound, bool allPlayers, int speakerID, const BPlayerIDArray& recipientIDs, float duration);
   BChatMessage();
   ~BChatMessage() {}

   void updateTime(float elapsedTime);
   bool hasExpired() { if(mbNeverExpire) return false; return (mTimeToDisplay <= 0.0f); }
   void expire() { mTimeToDisplay = 0.0f; }

   void setNeverExpire(bool v) { mbNeverExpire=v; }
   bool getNeverExpire() const { return mbNeverExpire; }
   
   void setAutoExpire(bool v) { mbAutoExpire=v; }
   bool getAutoExpire() const { return mbAutoExpire; }

   void setForceSubtitles(bool v) { mbForceSubtitles=v; }
   bool getForceSubtitles() const { return mbForceSubtitles; }

   bool getIsNew() const { return (mbIsNew); }
   bool getQueueSound() const { return (mbQueueSound); }
   bool getAllPlayers() const { return (mbAllPlayers); }

   void setIsNew(bool v) { mbIsNew = v; }

   // chat string
   bool                 hasChatString() { return (mStringID >= 0); }
   const BUString&      getChatString();
   int                  getChatStringID() { return mStringID; }

   // chat audio
   bool                 hasChatSound() { return (mSound.length() > 0); }
   const BSimString&    getSoundString() { return mSound; }

   BCueIndex            getCueIndex() { return mCueIndex; }
   void                 setCueIndex(BCueIndex cueIndex) { mCueIndex=cueIndex; }

   // chat speaker
   int                  getSpeakerID() const { return (mSpeakerID); }
   const BPlayerIDArray&   getRecipientIDs() const { return (mRecipientIDs); }

   BBinkVideoHandle     getVideoHandle() const { return mVideoHandle; }
   void                 setVideoHandle(BBinkVideoHandle handle) { mVideoHandle=handle; }

   bool save(BStream* pStream, int saveType);
   bool load(BStream* pStream, int saveType);

protected:
   BSimString           mSound;
   BPlayerIDArray       mRecipientIDs;
   int                  mSpeakerID;
   long                 mStringID;
   float                mTimeToDisplay;
   BCueIndex            mCueIndex;
   long                 mStringIDIndex;
   BBinkVideoHandle     mVideoHandle;

   bool                 mbIsNew        : 1;
   bool                 mbQueueSound   : 1;
   bool                 mbAllPlayers   : 1;
   bool                 mbNeverExpire  : 1;
   bool                 mbAutoExpire   : 1;
   bool                 mbForceSubtitles : 1;
};


//==========================================
// BChatManager
//
// Management and access to game chats
//==========================================
class BChatManager : public BBinkVideoStatus
{
   public:

      BChatManager( void );
      virtual ~BChatManager( void );      

      // Management functions 
      bool init( void );
      void reset( void );

      // chat queue manipulation
      BChatMessage* addChat(long stringID, const BSimString& sound, bool bQueueSound, bool allPlayers, int speakerID, const BPlayerIDArray& recipientIDs, float duration);
      void removeChat( BChatMessage* chatMessage);
      BChatMessage* getChat();
      void processSoundStoppedEvent(BSoundEventParams& event);

      bool getChatsEnabled() const { return mChatsEnabled; }
      void setChatsEnabled(bool enabled) { mChatsEnabled = enabled; }

      bool getForceSubtitlesOn() const { return mForceSubtitlesOn; }
      void setForceSubtitlesOn(bool enabled) { mForceSubtitlesOn = enabled; }
      
      BGeneralEventSubscriber* getChatCompletedEventSubscriber() { return (mpChatCompleted); }
      void setChatCompletedEventSubscriber(BGeneralEventSubscriber* pChatCompleted) { mpChatCompleted = pChatCompleted; }

      BBinkVideoHandle        getVideoHandle() const;

      virtual void            onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType);
      bool load(BStream* pStream, int saveType);

      int getNumChats() const { return mChatMessageList.size(); }

protected:
      BDynamicSimArray<BChatMessage*>   mChatMessageList;
      BGeneralEventSubscriber*          mpChatCompleted;
      bool                              mChatsEnabled : 1;
      bool                              mForceSubtitlesOn : 1;
};