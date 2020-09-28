//==============================================================================
// MPPackets.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _MPPACKETS_H_
#define _MPPACKETS_H_

#include "NetPackets.h"

//==============================================================================
namespace BPacketType
{
   enum
   {
      cGameFinalizePacket = cNumberOfTypedPackets,
      cAGSMPGamePacket,
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
//==============================================================================
class BCompleteSettingsPacket : public BChannelPacket
{
   public:
      BCompleteSettingsPacket(long packetType) : BChannelPacket(packetType), mData(NULL), mSize(0){}
      BCompleteSettingsPacket(long packetType, void* data, long size) : BChannelPacket(packetType), mData(data), mSize(size) {}

      virtual void serialize(BSerialBuffer &sb)   { BChannelPacket::serialize(sb); sb.add(mSize); sb.add(mData, mSize); }
      virtual void deserialize(BSerialBuffer &sb) { BChannelPacket::deserialize(sb); sb.get(&mSize); sb.getPointer(&mData, &mSize); }

      void        *mData;
      long        mSize;
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

      DWORD       mIndex;
      void        *mData;
      long        mSize;
      #ifndef BUILD_FINAL
      bool        mOverrideLock;
      #endif
};

//==============================================================================
class BSyncPacket : public BChannelPacket
{
   public:
      BSyncPacket(long packetType = BChannelPacketType::cSyncPacket) : BChannelPacket(packetType),
         mID(0), mChecksum(0), mChecksumSize(0)
      {
      }

      BSyncPacket(long ID, void *checksum, long checksumSize, long packetType = BChannelPacketType::cSyncPacket) : BChannelPacket(packetType),
         mID(ID), mChecksum(0), mChecksumSize(checksumSize)
      {         
         if (mChecksum) delete mChecksum;
         mChecksum = new char[mChecksumSize];
         memcpy(mChecksum, checksum, checksumSize);
      }

      virtual ~BSyncPacket(void) { if (mChecksum) delete mChecksum; }

      virtual void            serialize(BSerialBuffer &sb) 
      { 
         BChannelPacket::serialize(sb);

         sb.add(mID);          
         sb.add(mChecksumSize);
         sb.add(mChecksum, mChecksumSize);
      }
      virtual void            deserialize(BSerialBuffer &sb) 
      {  
         BChannelPacket::deserialize(sb);

         sb.get(&mID);
         sb.get(&mChecksumSize);        
         if (mChecksum) delete mChecksum;
         mChecksum = new char[mChecksumSize];
         sb.get(&mChecksum, mChecksumSize);
      }

   public:
      long                    mID;
      void                    *mChecksum;
      long                    mChecksumSize;
      
};

#endif // _MPPACKETS_H_