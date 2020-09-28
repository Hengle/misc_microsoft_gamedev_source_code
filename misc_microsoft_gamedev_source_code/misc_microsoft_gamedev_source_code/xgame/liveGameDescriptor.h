//==============================================================================
// liveGameDescriptor.h
//
// Copyright (c) 2006-2008, Ensemble Studios
//==============================================================================

#pragma once
// Used to track connection/game data for local Live sessions 

#define cBLiveGameDescriptorMaxClients             4    //6 peeps max, but one is the host and if there are 5 - then its full, don't query againist it

//==============================================================================
class BLiveGameDescriptor
{
   public:
      enum BDescriptorState
      {
         cStateNone,
         cStateHostQoSRunning,
         cStateClientQoSRunning,
         cStateComplete,
         cStateProcessed
      };
      enum BResultsState
      {
         cResultsNone,
         cResultsFull,
         cResultsCouldNotContactHost,
         cResultsCouldNotContactPeer,
         cResultsBufferError,
         cResultsClientQoSError,
         cResultsVersionCodeMismatch,
         cResultsLanguageCodeMismatch,
         cResultsPingAboveMaximum,
         cResultsJoinable,
         cResultsBadMatchQuality,
         cResultsBadTargetEntry
      };

      //Base constructor
      BLiveGameDescriptor();
      //Destructor
      ~BLiveGameDescriptor();

      //Returns true if this descriptor has been around longer than the expireTime (in ms)
      bool                 hasExpired( DWORD expireTime );
      //Given a list of client XNADDRs - run QoS against all of them
      void                 launchClientQoS(uint clientCount, XNADDR* clientXNADDRArray);
      //Per frame update hook so that it can check for QoS results
      void                 processQoS();
      //Kill QoS for this record if it is running
      void                 cancelQoS();

      void                 setName(const BSimString& name);
      const BSimString&    getName() const;
      void                 setHostGamertag(const BSimString& gamertag);
      const BSimString&    getHostGamertag() const;

      void                 setHostRating(float score) { mHostRating=score; }
      float                getHostRating() { return mHostRating; }
      void                 setHostSigma(double score) { mHostSigma=score; }
      double               getHostSigma() { return mHostSigma; }
      void                 setHostMu(double score) { mHostMu=score; }
      double               getHostMu() { return mHostMu; }
      double               getMatchQuality() const { return mMatchQuality; }
      void                 calcuateMatchQuality(double targetSigma, double targetMu );

      void                 setLocal(bool local);
      bool                 getLocal() const;

      void                 setChecksum(DWORD checksum);
      DWORD                getChecksum() const;

      void                 setGameType(long gameType) { mGameType = gameType; mUpdateTime = timeGetTime();}
      long                 getGameType() { return mGameType; }

      void                 setOpenSlots(uint8 openSlots) { mOpenSlots=openSlots; }
      uint8                getOpenSlots() const { return mOpenSlots; }
      void                 setSlots(uint8 slots) { mSlots=slots; }
      uint8                getSlots() const { return mSlots; }
      void                 setNonce(uint64 n) { mNonce=n; }
      uint64               getNonce() const { return mNonce; }

      void                 setXNKID(const XNKID& newKid);
      const XNKID&         getXNKID() const { return mXnKID; }
      void                 setXNKEY(const XNKEY& newKey);
      const XNKEY&         getXNKEY() const { return mXnKey; }
      void                 setXnAddr(const XNADDR& xnaddr); 
      const XNADDR&        getXnAddr() const { return mXnAddr; }

      void                 setAvgPing(WORD pingAvgInMS) { mAveragePing=pingAvgInMS; }
      WORD                 getAvgPing() const { return mAveragePing; }

      BDescriptorState     getState() const { return mState; }
      void                 setState(BDescriptorState newState) { mState = newState;mUpdateTime = timeGetTime(); }
      BResultsState        getResultState() const {return mResultState;}
      void                 setResultState(BResultsState newState) { mResultState = newState;mUpdateTime = timeGetTime(); }

      bool                 getRanked() const { return mRanked; }
      void                 setRanked(bool ranked) { mRanked=ranked; }

      uint                 getGameModeIndex() { return mGameModeIndex; }
      void                 setGameModeIndex(uint index) { mGameModeIndex=index; }
      DWORD                getQoSSearchIndex() { return mQOSSearchIndex; }
      void                 setQoSSearchIndex(DWORD index) { mQOSSearchIndex=index; }
      
      uint8                getLanguageCode() {return mLanguageCode;};
      void                 setLanguageCode(uint8 lcode) {mLanguageCode=lcode;};
      uint8                getVersionCode() {return mVersionCode;};
      void                 setVersionCode(uint8 vcode) {mVersionCode=vcode;};

      DWORD                getQOSHostTime() {return mQOSHostTime;};
      void                 setQOSHostTime(DWORD totalTime) {mQOSHostTime=totalTime;};
      DWORD                getQOSClientTime() {return mQOSClientTime;};

      //These query data out of the xnaddr - added to be nice
      const IN_ADDR&       getTranslatedAddress() const;
      const IN_ADDR&       getAddress() const;
      WORD                 getPort() const;

   protected:
      XNADDR               mClientXnAddr[cBLiveGameDescriptorMaxClients];  // 36 * 4 == 144
      XNKEY                mClientXnKey[cBLiveGameDescriptorMaxClients];   // 16 * 4 == 64

      XNADDR               mXnAddr;             // 36

      XNKID                mClientXnKID[cBLiveGameDescriptorMaxClients];   // 8 * 4 == 32

      XNKEY                mXnKey;              // 16

      BSimString           mName;               // 8
      BSimString           mHostGamertag;       // 8

      XNKID                mXnKID;              // 8

      uint64               mNonce;              // 8

      BDescriptorState     mState;              // 4
      BResultsState        mResultState;        // 4
      DWORD                mChecksum;           // 4
      long                 mGameType;           // 4
      uint                 mGameModeIndex;      // 4
      float                mHostRating;         // 4
      double               mHostSigma;    
      double               mHostMu;
      double               mMatchQuality;
      DWORD                mQOSSearchIndex;     
      DWORD                mQOSHostTime;        // 4 STAT How long QOS against the host took
      DWORD                mQOSClientTime;      // 4 STAT How long QOS against all the clients took (internally calced by this class)
      DWORD                mTimeTrack;          // 4
      XNQOS*               mpQoSResults;        // 4
      DWORD                mUpdateTime;         // 4 When this record was last created/updated
      WORD                 mAveragePing;        // 2
      uint8                mClientCount;        // 1
      uint8                mLanguageCode;       // 1
      uint8                mVersionCode;        // 1
      uint8                mOpenSlots;          // 1
      uint8                mSlots;              // 1
      bool                 mRanked : 1;         // 1 (1/8)
      bool                 mLocal : 1;          //   (2/8)
};

typedef BDynamicSimArray<BLiveGameDescriptor>  BLiveGameDescriptorArray;
typedef BDynamicSimArray<BLiveGameDescriptor*> BLiveGameDescriptorPtrArray;
