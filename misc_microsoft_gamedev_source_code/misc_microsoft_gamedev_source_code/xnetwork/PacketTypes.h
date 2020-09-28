//==============================================================================
// PacketTypes.h
//
// Copyright (c) 2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
namespace BPacketType 
{ 
   enum
   { 
      cTypedPacket = 0, cChannelPacket, cTypedMessageDataPacket, cKickPacket,
      cPingPacket, cPongPacket, cHostNamePacket, cSessionNamePacket,
      cDataBlockPacket, cDataBlockFlowControlPacket, cDataStreamPacket, cFileStreamPacket,
      cWrapperPacket, cClientListPacket, cInitClientPacket, cInitPeersPacket,
      cClientConnectedPacket, cClientDisconnectedPacket, cTypedTextPacket,
      cConnectPeerPacket, cClientNotRespondingPacket,
      cFindPacket, cAdvertisePacket, cJoinRequestPacket, cJoinResponsePacket, 
      cJoinIPPacket, cJoinIPResponsePacket, cConnectionRejectedPacket,
      cFindPortPacket, cAdvertisePortPacket, cTwoLongsPacket, cPlayerIndexPacket, 
      cLSPCommandPacket, cStreamPacket, cChunkPacket, cAuthPacket, cConfigDataRequestPacket, cConfigDataResponsePacket,// LSP packets
      cPerfLogPacket,      //More LSP packets
      cRequestFileUploadPacket,
      cFileUploadBlockPacket, 
      cFileUploadCompletePacket,
      cInitPeerPacket,
      cInitPeersRequestPacket,
      cInitPeersResponsePacket,

      cTalkersPacket,
      cVoiceHeadsetPacket,

      cClientJoinRequestPacket, // similar to cJoinRequestPacket without the connection baggage
      cClientJoinResponsePacket, // similar to cJoinResponsePacket without the connection baggage
      // cInitClientPacket // already defined but old use has been long gone, now used for initializing a new client to an existing session

      cMediaCommandPacket,
      cPerfLogPacketV2,

      cTickerRequestPacket, 
      cTickerResponsePacket, 

      cProxyRequestPacket, // broadcast by a client looking for someone to route traffic to another client
      cProxyResponsePacket,
      cProxyPingPacket,
      cProxyPongPacket,

      cDisconnectRequest,  // sent by everyone when a client is disconnecting, contains last known state of channel information
      cDisconnectSync,     // contains channel data that is missing
      cDisconnectResponse, // sent after sending missing channel data in response to a disconnect request

      cLanInfoPacket,      // broadcast when a client is hosting a lan game

      cServiceRecordPacket,

      cNumberOfTypedPackets 
   };
};

//==============================================================================
namespace BChannelPacketType
{
   enum
   {
      //cPeerClientDisconnectPacket=0,
      //cChannelTextPacket,
      cTimeIntervalPacket = 0,
      //cTimeIntervalGamePacket,
      cTimeSyncPacket,
      //cTimeSyncGamePacket,
      cTimeSyncInitPacket,
      cPausePacket,
      cSingleStepPacket,
      //cChannelDataBlockPacket,

      cNumberOfChannelPackets
   };
};
