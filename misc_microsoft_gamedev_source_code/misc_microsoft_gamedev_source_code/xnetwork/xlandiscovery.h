//==============================================================================
// xlandiscovery.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

#include "NetEvents.h"
#include "xudpsocket.h"
#include "xnetwork.h"

class BXLanDiscovery;

//============================================================================
class BLanGameInfo : public BTypedPacket
{
   friend BXLanDiscovery;

   public:
      BLanGameInfo();

      void                 init(uint titleID, uint checksum);

      void                 reset();

      uint                 getTitleID() const { return mTitleID; }
      uint                 getChecksum() const { return mChecksum; }
      uint64               getNonce() const { return mNonce; }

      const BUString&      getInfo() const { return mInfo; }

      uint                 getGameType() const { return mGameType; }
      uint                 getMapIndex() const { return mMapIndex; }
      uint                 getDifficulty() const { return mDifficulty; }
      uint                 getFilledSlots() const { return mFilledSlots; }
      uint                 getMaxSlots() const { return mMaxSlots; }
      bool                 getLocked() const { return mLocked; }
      bool                 getBadCRC() const { return mBadCRC; }

      const XNADDR&        getXnAddr() const { return mXnAddr; }
      const XNKEY&         getXnKey() const { return mXnKey; }
      const XNKID&         getXnKID() const { return mXnKID; }

      uint                 getUpdateTime() const { return mUpdateTime; }

      void                 setInfo(const BUString& info);
      void                 setMapIndex(uint mapIndex);
      void                 setGameType(uint gameType);
      void                 setDifficulty(uint difficulty);
      void                 setSlots(uint availableSlots, uint maxSlots);
      void                 setLocked(bool locked);
      void                 setXnAddr(const XNADDR& xnAddr);
      void                 setXnKey(const XNKEY& xnKey);
      void                 setXnKID(const XNKID& xnKID);
      void                 setNonce(uint64 nonce);

      bool                 update(const BLanGameInfo& game);

      // check to see if the information for this game has been updated
      bool                 isUpdated();

      BLanGameInfo&        operator=(const BLanGameInfo&);
      bool                 operator==(const BLanGameInfo&) const;

      virtual void         serialize(BSerialBuffer& sb);
      virtual void         deserialize(BSerialBuffer& sb);

   protected:
      void                 setBadCRC();

   private:

      BUString             mInfo;

      XNADDR               mXnAddr;
      XNKEY                mXnKey;
      XNKID                mXnKID;

      uint64               mNonce;

      uint32               mChecksum;

      uint                 mGameType;
      uint                 mMapIndex;
      uint                 mDifficulty;

      uint                 mTitleID;

      uint                 mUpdateTime;  // When this record was last created/updated

      uint                 mFilledSlots;
      uint                 mMaxSlots;

      bool                 mLocked;

      bool                 mUpdated : 1;
      bool                 mBadCRC : 1;
};

//============================================================================
class BXLanDiscovery : public BEventReceiver
{
   public:

      BXLanDiscovery();
      ~BXLanDiscovery();

      bool init(uint titleID, uint checksum);
      void deinit();

      void initialBroadcast();

      BLanGameInfo& getLocalInfo() { return mInfo; }

      bool isListUpdated() const { return mListUpdated; }
      void resetListUpdated() { mListUpdated = false; }

      BDynamicSimArray<BLanGameInfo>& getList() { return mGamesList; }

   private:

      void initBroadcast();
      void deinitBroadcast();

      bool broadcastInfo();
      void checkList();
      void addGame(const BLanGameInfo& info);

      // BEventReceiver
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      enum
      {
         cLanEventTimer = cNetEventFirstUser,

         cLanEventTotal
      };

      BXNetBufferAllocator             mBufferAllocator;   // 332808

      BXUDPSocket                      mUDPThread;             // 4280

      BXNetBuffer                      mTempNetBuffer;         // 1296

      BLanGameInfo                     mInfo;

      BDynamicSimArray<BLanGameInfo>   mGamesList;             // 16

      BWin32WaitableTimer              mBroadcastTimer;        // 4

      uint                             mTitleID;
      uint                             mChecksum;

      uint16                           mPort;                  // 2

      bool                             mBroadcastTimerSet : 1; // 1 (1/8)
      bool                             mListUpdated : 1;       //   (2/8)
      bool                             mFirstBroadcastSent : 1;//   (3/8)
};
