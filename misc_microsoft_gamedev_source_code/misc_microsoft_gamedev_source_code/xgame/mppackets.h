//==============================================================================
// MPPackets.h
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

#pragma once

#include "NetPackets.h"

//==============================================================================
namespace BPacketType
{
   enum
   {
      cGameFinalizePacket = cNumberOfTypedPackets,
      cPartyHostSettingsPacket,           //Used by the host settings structure
      cPartyMemberSettingsPacket,         //Used by the member settings structure
      cPartyMemberSettingsEchoPacket,     //Same as cPartyMemberSettingsPacket, but indicates it was sent echoed from the host
      cPartyMatchMakingJoinCommand,       //Used by host to tell the clients to join/disconnect from a particular target
      cPartyMatchMakingCommand,           //Used party system to pass around various command requests
      cPartyKickRequest,                  //XUID
      cPartyJoinTarget,                   //XNADDR, WORD gamePortNumber, XNKID* sessionkey 
      cPartyJoinResults,                  //BYTE code
      cPartyMatchMakingStatusInfo,        //uint, uint
      cPartyAddAICommand,                 //XUID, gamertag
      cPartyChangeGamertagCommand,        //Same as cPartyAddAICommand
      cNumberMPTypedPackets
   };
};

namespace BChannelPacketType
{
   enum
   {
      cSyncPacket = cNumberOfChannelPackets,
      cRequestSettingsPacket,
      cLockSettingsPacket,
      cSettingsLockedPacket,
      cPreLaunchDataHostDataPacket,
      cPreLaunchDataClientDataPacket,
      cStartGamePacket,
      cLaunchUpdatePacket,
      cLaunchAbortRequestPacket,
      cLaunchAbortPacket,
      cLaunchReadyPacket,
      cStartVote,
      cAbortVote,
      cVoteResult,
      cInitialSettingsPacket,
      cSettingsPacket,
      cFinalSettingsPacket,
      cNumberOfMPChannelPackets
   };
}

//==============================================================================
class BPartyHostSettingsPacket : public BTypedPacket
{
public:
   BPartyHostSettingsPacket(long packetType = BPacketType::cPartyHostSettingsPacket) : BTypedPacket(packetType), mRandomTeam(false), mMapIndex(0), mPartyRoomMode(0), mNumPlayers(6), mDifficulty(0), mHopperIndex(0), mGameMode(0), mHostStatusInformation(0), mLiveMode(0) {}
   BPartyHostSettingsPacket(bool randomTeam, uint8 mapIndex, uint8 partyRoomMode, uint8 numPlayers, uint8 difficulty, uint8 hopperIndex, uint8 gameMode, uint8 hostStatusInformation, uint8 liveMode, long packetType = BPacketType::cPartyHostSettingsPacket) : BTypedPacket(packetType), mRandomTeam(randomTeam), mMapIndex(mapIndex), mPartyRoomMode(partyRoomMode), mNumPlayers(numPlayers), mDifficulty(difficulty), mHopperIndex(hopperIndex), mGameMode(gameMode), mHostStatusInformation(hostStatusInformation), mLiveMode(liveMode) {}

   virtual ~BPartyHostSettingsPacket(void) {}

   virtual void            serialize(BSerialBuffer &sb)   { BTypedPacket::serialize(sb); sb.add(mRandomTeam); sb.add(mMapIndex); sb.add(mPartyRoomMode); sb.add(mNumPlayers);sb.add(mDifficulty); sb.add(mHopperIndex); sb.add(mGameMode); sb.add(mHostStatusInformation); sb.add(mLiveMode);}
   virtual void            deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mRandomTeam); sb.get(&mMapIndex); sb.get(&mPartyRoomMode); sb.get(&mNumPlayers); sb.get(&mDifficulty); sb.get(&mHopperIndex); sb.get(&mGameMode); sb.get(&mHostStatusInformation); sb.get(&mLiveMode); }

public:   
   bool     mRandomTeam;
   uint8    mMapIndex;
   uint8    mPartyRoomMode;
   uint8    mNumPlayers;         // 2, 4, 6
   uint8    mDifficulty;
   uint8    mHopperIndex; 
   uint8    mGameMode;           // game mode victory condition
   uint8    mHostStatusInformation;
   uint8    mLiveMode;           //Which Live mode the party is in (open/friends/closed)
};

//==============================================================================
class BPartyMemberSettingsPacket : public BTypedPacket
{
public:
   BPartyMemberSettingsPacket(long packetType = BPacketType::cPartyMemberSettingsPacket) : BTypedPacket(packetType), mXUID(0), mCiv(0), mLeader(0), mTeam(0), mSlot(0), mConnectionState(0), mPartyRoomMode(0), mStatusFlags(0), mPlayerRank(0), mCampaignBits(0) {}
   BPartyMemberSettingsPacket(XUID xuid, uint8 civ, uint8 leader, uint8 team, uint8 slot, uint8 connectionState, uint8 partyRoomMode, uint8 statusFlags, uint16 playerRank, uint32 campaignBits, long packetType = BPacketType::cPartyMemberSettingsPacket) : BTypedPacket(packetType), mXUID(xuid), mCiv(civ), mLeader(leader), mTeam(team), mSlot(slot), mConnectionState(connectionState), mPartyRoomMode(partyRoomMode), mStatusFlags(statusFlags), mPlayerRank(playerRank), mCampaignBits(campaignBits) {}

   virtual ~BPartyMemberSettingsPacket(void) {}

   virtual void            serialize(BSerialBuffer &sb)   { BTypedPacket::serialize(sb); sb.add(mXUID); sb.add(mCiv); sb.add(mLeader); sb.add(mTeam); sb.add(mSlot); sb.add(mConnectionState); sb.add(mPartyRoomMode); sb.add(mStatusFlags); sb.add(mPlayerRank); sb.add(mCampaignBits); }
   virtual void            deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mXUID); sb.get(&mCiv); sb.get(&mLeader); sb.get(&mTeam); sb.get(&mSlot); sb.get(&mConnectionState); sb.get(&mPartyRoomMode); sb.get(&mStatusFlags); sb.get(&mPlayerRank); sb.get(&mCampaignBits); }

public:   
   XUID     mXUID;
   uint8    mCiv;
   uint8    mLeader;
   uint8    mTeam;
   uint8    mSlot;
   uint8    mConnectionState;
   uint8    mPartyRoomMode;
   uint8    mStatusFlags;
   uint16   mPlayerRank;
   uint32   mCampaignBits;
};

//==============================================================================
class BPartyMemberMatchMakingCommand : public BTypedPacket
{
public:
   BPartyMemberMatchMakingCommand(long packetType = BPacketType::cPartyMatchMakingCommand) : BTypedPacket(packetType), mCommandCode(0), mTargetNonce(0) {}      
   BPartyMemberMatchMakingCommand(uint8 commandCode, uint64 targetNonce, long packetType = BPacketType::cPartyMatchMakingCommand) : BTypedPacket(packetType), mCommandCode(commandCode), mTargetNonce(targetNonce) {}

   virtual ~BPartyMemberMatchMakingCommand(void) {}

   virtual void            serialize(BSerialBuffer &sb)   { BTypedPacket::serialize(sb); sb.add(mCommandCode); sb.add(mTargetNonce);}
   virtual void            deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mCommandCode); sb.get(&mTargetNonce);}

public:   
   uint8    mCommandCode;
   uint64   mTargetNonce;
};

//==============================================================================
class BPartyMatchMakingStatusInfo : public BTypedPacket
{
public:
   BPartyMatchMakingStatusInfo(long packetType = BPacketType::cPartyMatchMakingStatusInfo) : BTypedPacket(packetType), mStatusCode(0), mData1(0),  mData2(0) {}      
   BPartyMatchMakingStatusInfo(uint8 statusCode, uint data1, uint data2, long packetType = BPacketType::cPartyMatchMakingStatusInfo) : BTypedPacket(packetType), mStatusCode(statusCode), mData1(data1), mData2(data2) {}

   virtual ~BPartyMatchMakingStatusInfo(void) {}

   virtual void            serialize(BSerialBuffer &sb)   { BTypedPacket::serialize(sb); sb.add(mStatusCode); sb.add(mData1);sb.add(mData2);}
   virtual void            deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mStatusCode); sb.get(&mData1); sb.get(&mData2);}

public:   
   uint8    mStatusCode;
   uint     mData1;
   uint     mData2;
};

//==============================================================================
class BPartyAddAICommand : public BTypedPacket
{
public:
   BPartyAddAICommand(long packetType = BPacketType::cPartyAddAICommand) : BTypedPacket(packetType), mXuid(0), mGamertag() {}      
   BPartyAddAICommand(XUID xuid, BSimString gamertag, long packetType = BPacketType::cPartyAddAICommand) : BTypedPacket(packetType), mXuid(xuid), mGamertag(gamertag) {}

   virtual ~BPartyAddAICommand(void) {}

   virtual void            serialize(BSerialBuffer &sb)   { BTypedPacket::serialize(sb); sb.add(mXuid); sb.add(mGamertag);}
   virtual void            deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mXuid); sb.get(&mGamertag);}

public:   
   XUID        mXuid;
   BSimString  mGamertag;
};


//==============================================================================
class BPartyMemberMatchMakingJoinCommand : public BTypedPacket
{
public:
   BPartyMemberMatchMakingJoinCommand(long packetType = BPacketType::cPartyMatchMakingJoinCommand) : BTypedPacket(packetType), mCommandCode(0), mTargetNonce(0), mTargetHopper(0)
   {
      Utils::FastMemSet(&mTargetXNADDR,0,sizeof(XNADDR));       
      Utils::FastMemSet(&mTargetXNKID,0,sizeof(XNKID));
      Utils::FastMemSet(&mTargetXNKEY,0,sizeof(XNKEY));
   }      
   BPartyMemberMatchMakingJoinCommand(uint8 commandCode, uint8 targetHopper, uint64 targetNonce, const XNADDR& targetXNADDR, const XNKID& targetXNKID, const XNKEY& targetXNKEY, long packetType = BPacketType::cPartyMatchMakingJoinCommand)
      : BTypedPacket(packetType), mCommandCode(commandCode), mTargetHopper(targetHopper), mTargetNonce(targetNonce)
   {
      mTargetXNADDR = targetXNADDR;
      mTargetXNKID = targetXNKID;
      mTargetXNKEY = targetXNKEY;
   }

   virtual ~BPartyMemberMatchMakingJoinCommand() {}

   virtual void serialize(BSerialBuffer &sb) 
   { 
      BTypedPacket::serialize(sb); 
      sb.add(mCommandCode); 
      sb.add(mTargetHopper);
      sb.add((void*)&mTargetXNADDR,sizeof(XNADDR));
      sb.add((void*)&mTargetXNKID,sizeof(XNKID));
      sb.add((void*)&mTargetXNKEY,sizeof(XNKEY));
      sb.add(mTargetNonce); 
   }

   virtual void deserialize(BSerialBuffer &sb) 
   { 
      BTypedPacket::deserialize(sb); 
      sb.get(&mCommandCode); 
      sb.get(&mTargetHopper);
      void *p1 = (void *)&mTargetXNADDR;
      sb.get(&p1, sizeof(XNADDR));
      void *p2 = (void *)&mTargetXNKID;
      sb.get(&p2, sizeof(XNKID));
      void *p3 = (void *)&mTargetXNKEY;
      sb.get(&p3, sizeof(XNKEY));
      sb.get(&mTargetNonce); 
   }

public:   
   uint8    mCommandCode;
   uint8    mTargetHopper;
   XNADDR   mTargetXNADDR;
   XNKID    mTargetXNKID;
   XNKEY    mTargetXNKEY;
   uint64   mTargetNonce;
};

//==============================================================================
class BPartyMemberKickRequest : public BTypedPacket
{
public:
   BPartyMemberKickRequest(long packetType = BPacketType::cPartyKickRequest) : BTypedPacket(packetType), mXuid(0) {}      
   BPartyMemberKickRequest(XUID xuid, long packetType = BPacketType::cPartyKickRequest) : BTypedPacket(packetType), mXuid(xuid){}

   virtual ~BPartyMemberKickRequest(void) {}

   virtual void            serialize(BSerialBuffer &sb)   { BTypedPacket::serialize(sb); sb.add(mXuid); }
   virtual void            deserialize(BSerialBuffer &sb) { BTypedPacket::deserialize(sb); sb.get(&mXuid); }

public:               
   XUID     mXuid;
};

//==============================================================================
//==============================================================================
class BCompleteSettingsPacket : public BChannelPacket
{
   public:
      BCompleteSettingsPacket(long packetType) : BChannelPacket(packetType), mData(NULL), mSize(0){}
      BCompleteSettingsPacket(long packetType, void* data, long size) : BChannelPacket(packetType), mData(data), mSize(size) {}

      virtual void serialize(BSerialBuffer &sb)   { BChannelPacket::serialize(sb); sb.add(mSize); sb.add(mData, mSize); }
      virtual void deserialize(BSerialBuffer &sb) { BChannelPacket::deserialize(sb); sb.get(&mSize); sb.getPointer(&mData, &mSize); }

      const void* mData;
      int32       mSize;
};

//==============================================================================
class BSettingsPacket : public BChannelPacket
{
   public:
      BSettingsPacket() : BChannelPacket(BChannelPacketType::cSettingsPacket), mIndex(0), mData(NULL), mSize(0)
      #ifndef BUILD_FINAL
      , mOverrideLock(false)
      #endif
      {}

      BSettingsPacket(DWORD index, void* data, long size) : BChannelPacket(BChannelPacketType::cSettingsPacket), mIndex(index), mData(data), mSize(size)
      #ifndef BUILD_FINAL
      , mOverrideLock(false)
      #endif
      {}

      virtual void serialize(BSerialBuffer &sb)   { BChannelPacket::serialize(sb); sb.add(mIndex); sb.add(mSize); sb.add(mData, mSize); 
      #ifndef BUILD_FINAL
      sb.add(mOverrideLock);
      #endif
      }
      virtual void deserialize(BSerialBuffer &sb) { BChannelPacket::deserialize(sb); sb.get(&mIndex); sb.get(&mSize); sb.getPointer(&mData, &mSize); 
      #ifndef BUILD_FINAL
      sb.get(&mOverrideLock);
      #endif
      }

      uint32      mIndex;
      const void* mData;
      int32       mSize;
      #ifndef BUILD_FINAL
      bool        mOverrideLock;
      #endif
};

//==============================================================================
class BSyncPacket : public BChannelPacket
{
   public:
      BSyncPacket(long packetType = BChannelPacketType::cSyncPacket) :
         BChannelPacket(packetType),
         mID(0),
         mChecksum(0)
      {
      }

      BSyncPacket(long ID, uint checksum, long packetType = BChannelPacketType::cSyncPacket) :
         BChannelPacket(packetType),
         mID(ID),
         mChecksum(checksum)
      {
      }

      virtual void serialize(BSerialBuffer &sb)
      { 
         BChannelPacket::serialize(sb);

         sb.add(mID);
         sb.add(mChecksum);
      }
      virtual void deserialize(BSerialBuffer &sb)
      {
         BChannelPacket::deserialize(sb);

         sb.get(&mID);
         sb.get(&mChecksum);
      }

   public:
      int32 mID;
      uint  mChecksum;
};


//==============================================================================
// Used for the host to send pre-launch final tuning data, as well as clients to echo to host their pre-launch profile
//   data (which the host then sends out to everyone else)
// cPreLaunchDataHostDataPacket and cPreLaunchDataClientDataPacket
//==============================================================================
class BPreLaunchDataPacket : public BChannelPacket
{
public:
   BPreLaunchDataPacket(long packetType) : BChannelPacket(packetType), mData(NULL), mSize(0){}
   BPreLaunchDataPacket( long packetType, long playerID, void* data, long size) : BChannelPacket(packetType), mPlayerID(playerID), mData(data), mSize(size) {}

   virtual void serialize(BSerialBuffer &sb)   { BChannelPacket::serialize(sb); sb.add(mPlayerID); sb.add(mSize); sb.add(mData, mSize); }
   virtual void deserialize(BSerialBuffer &sb) { BChannelPacket::deserialize(sb); sb.get(&mPlayerID); sb.get(&mSize); sb.getPointer(&mData, &mSize); }

   const void* mData;
   int32       mSize;
   int32       mPlayerID;
};
