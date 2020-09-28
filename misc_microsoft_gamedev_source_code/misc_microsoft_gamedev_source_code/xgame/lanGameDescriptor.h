//==============================================================================
// lanGameDescriptor.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Used to track connection/game data for local LAN sessions
#pragma once

//--------------------------------------------------------------------------------------
// Message structures used by lan discovery
//--------------------------------------------------------------------------------------
#define NONCE_SIZE 8

enum MessageIDsLANDiscovery
{
   MessageSeekRequest,       // broadcast: looking for a host
   MessageSeekResponse       // broadcast: response to a SEEKING message
};

struct MessageLANDiscoveryBase
{
   BYTE                   mNonce[NONCE_SIZE];      //Used to give a unique ID to each sender
   DWORD                  mTitleID;                //In case other games use this same port
   MessageIDsLANDiscovery mId;                     //Message ID
};

struct MessageLANDiscoverySeek : public MessageLANDiscoveryBase
{
   //Might be additional request data in the future
};

struct MessageLANDiscoveryResponse : public MessageLANDiscoveryBase
{
   BCHAR_T     mName[_MAX_PATH];
   XNADDR      mXnAddr;              // XNADDR of the host
   XNKEY       mXnKey;               // Key to use for secure communication
   XNKID       mXnKID;               // Key ID to use for secure communication
   DWORD       mChecksum;
   //game name, players, map?
};

//==============================================================================
class BLanGameDescriptor
{
   public:
      //Base constructor
      BLanGameDescriptor();
      //Constructor that builds itself off a buffer with serialized LAN data in it
      BLanGameDescriptor(MessageLANDiscoveryResponse* pMsgResponse);

      //Takes the current descriptor lan-related data and packs it into the message buffer you pass in
      DWORD                serializeLANData(MessageLANDiscoveryResponse* pMsgResponse);

      //Given another description, this compares the NONCEs to see if they are from the same sender
      bool                 isFromSameSenderAs(BLanGameDescriptor* pDesc);
      //Returns true if this descriptor has been around longer than the expireTime (in ms)
      bool                 hasExpired( DWORD expireTime );

      void                 setName(const BSimString& name);
      const BCHAR_T*       getName() const;

      void                 setLocal(bool local);
      bool                 getLocal() const;

      void                 setChecksum(DWORD checksum);
      DWORD                getChecksum() const;  

      void                 setGameType(long gameType) { mGameType = gameType; }
      long                 getGameType() { return mGameType; }

      void                 setXNKID(const XNKID& newKid);
      const XNKID&         getXNKID() const { return mXnKID; }
      void                 setXNKEY(const XNKEY& newKey);
      const XNKEY&         getXNKEY() const { return mXnKey; }

      void                 setXnAddr(const XNADDR& xnaddr);
      const XNADDR&        getXnAddr() const { return mXnAddr; }

   void                 setSlots(uint8 slots) {mSlots=slots;};
   uint8                getSlots() {return mSlots;};

   void                 setNonce(uint64 n);
   uint64               getNonce();

   //These query data out of the xnaddr - added to be nice
   const IN_ADDR&       getTranslatedAddress(void) const;
   const IN_ADDR&       getAddress(void) const;
   WORD                 getPort(void) const;

   protected:
      BCHAR_T              mName[_MAX_PATH];
      XNADDR               mXnAddr;
      XNKEY                mXnKey;
      XNKID                mXnKID;
      BYTE                 mNonce[NONCE_SIZE];
      DWORD                mChecksum;
      long                 mGameType;
      DWORD                mUpdateTime;               //When this record was last created/updated
      uint8                mSlots;           //Maxplayers for this session
      bool                 mLocal : 1;
};

typedef BDynamicSimArray<BLanGameDescriptor>    BLanGameDescriptorArray;
typedef BDynamicSimArray<BLanGameDescriptor*>   BLanGameDescriptorPtrArray;
