//==============================================================================
// packets.h
//
// Copyright (c) 2000-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mpPackets.h"
#include "commandtypes.h"

////==============================================================================
//// BPacketType
////==============================================================================
//namespace BPacketType
//{
//   enum 
//   { 
//      cReplace = cNumberMPTypedPackets, // start where net leaves off
//
//      // "subtype" packets,
//      cCommGameTimingPacket,
//      cStreamEventPacket,
//      cStreamBeginPacket,
//      cStreamUpdatePacket,
//      cStreamFileMarkerPacket,
//      cMigrateSessionPacket,
//      cLoadingStatusPacket,
//      cNumberOfBangPackets,
//   };
//};

//==============================================================================
namespace BChannelPacketType
{
   enum
   {
      cCommandPacketsStart = cNumberOfMPChannelPackets,
      cCommandPacketsEnd = cCommandPacketsStart+cNumberCommands+1,
      // "data" packets
      //cVectorPacket=cCommandPacketsEnd, 
      //cConfigDataInitPacket,
      //cConfigDataPacket,
      // "subtype" packets
      //cChatPacket,
      //cFlarePacket,
      //cTauntPacket,
      //cWhisperPacket,
      //cGCEventPacket,
   };
};

////==============================================================================
//// "data" packet types
////==============================================================================
//class BVectorPacket : public BChannelPacket
//{
//   public:
//      BVectorPacket(long packetType = BChannelPacketType::cVectorPacket) : BChannelPacket(packetType), 
//         x(0.0f), 
//         y(0.0f), 
//         z(0.0f) 
//      {
//      }
//
//      BVectorPacket(float ax, float ay, float az, long packetType = BChannelPacketType::cVectorPacket) : BChannelPacket(packetType), 
//         x(ax),
//         y(ay),
//         z(az)
//      {
//      }
//
//      virtual void            serialize(BSerialBuffer &sb) 
//      { 
//         BChannelPacket::serialize(sb); 
//
//         sb.add(x); 
//         sb.add(y); 
//         sb.add(z); 
//      }
//      virtual void            deserialize(BSerialBuffer &sb) 
//      { 
//         BChannelPacket::deserialize(sb);
//
//         sb.get(&x); 
//         sb.get(&y); 
//         sb.get(&z); 
//      }
//
//   public:
//      float                   x;
//      float                   y;
//      float                   z;      
//};
//
//
////==============================================================================
//
////==============================================================================
//// "subtype" packet types
////==============================================================================
//class BChatPacket : public BChannelTextPacket
//{
//   public:
//      BChatPacket(void) : BChannelTextPacket(BChannelPacketType::cChatPacket) {}
//      BChatPacket(const BCHAR_T *text) : BChannelTextPacket(text, NULL, BChannelPacketType::cChatPacket) {}
//};
//
//class BWhisperPacket : public BChannelTextPacket
//{
//   public:
//      BWhisperPacket(void) : BChannelTextPacket(BChannelPacketType::cWhisperPacket) {}
//      BWhisperPacket(const BCHAR_T *text) : BChannelTextPacket(text, NULL, BChannelPacketType::cWhisperPacket) {}
//};
//
//class BTauntPacket : public BChannelTextPacket
//{
//   public:
//      BTauntPacket(void) : BChannelTextPacket(BChannelPacketType::cTauntPacket) {}
//      BTauntPacket(const BCHAR_T *text) : BChannelTextPacket(text, NULL, BChannelPacketType::cTauntPacket) {}
//};
//
//class BFlarePacket : public BVectorPacket
//{
//   public:
//      BFlarePacket(void) : BVectorPacket(BChannelPacketType::cFlarePacket) {}
//      BFlarePacket(float x, float y, float z) : BVectorPacket(x, y, z, BChannelPacketType::cFlarePacket) {}
//};
//
//class BCommGameTimingPacket : public BTypedMessageDataPacket
//{
//   public:
//      enum { cStopTimeMsg=0, cTimeStoppedMsg, cRestartTimeMsg };
//      BCommGameTimingPacket(void) : BTypedMessageDataPacket(BPacketType::cCommGameTimingPacket) {}
//      BCommGameTimingPacket(long message, long data1=0, long data2=0) : BTypedMessageDataPacket(message, data1, data2, BPacketType::cCommGameTimingPacket) {}
//};
//
//class BGCEventPacket : public BChannelPacket
//{
//public:
//   BGCEventPacket(long packetType = BChannelPacketType::cGCEventPacket) : BChannelPacket(packetType)
//   {
//      mEventType = mCharacterID = mParam1 = mParam2 = -1;
//   }
//
//   BGCEventPacket(long nEventType, long nCharacterID, long nParm1, long nParm2, const BSimString &string, long packetType = BChannelPacketType::cGCEventPacket) : BChannelPacket(packetType)
//   {
//      mEventType = nEventType;
//      mCharacterID = nCharacterID;
//      mParam1 = nParm1;
//      mParam2 = nParm2;
//      mString = string;
//   }
//
//   virtual void            serialize(BSerialBuffer &sb) 
//   { 
//      BChannelPacket::serialize(sb); 
//
//      sb.add(mEventType);
//      sb.add(mCharacterID);
//      sb.add(mParam1);
//      sb.add(mParam2);
//      sb.add(mString);
//   }
//   virtual void            deserialize(BSerialBuffer &sb) 
//   { 
//      BChannelPacket::deserialize(sb);
//
//      sb.get(&mEventType);
//      sb.get(&mCharacterID);
//      sb.get(&mParam1);
//      sb.get(&mParam2);
//      sb.get(&mString);
//   }
//
//public:
//   long  mEventType;
//   long  mCharacterID;
//   long  mParam1;
//   long  mParam2;
//   BSimString mString;
//};
//