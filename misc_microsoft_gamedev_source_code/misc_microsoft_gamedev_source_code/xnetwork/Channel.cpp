//==============================================================================
// Channel.cpp
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "Channel.h"
#include "Session.h"
#include "TimeSync.h"

//==============================================================================
// 
//==============================================================================
BChannel::BChannel() :
   mpSession(NULL),
   mChannelID(-1),
   mPacketID(0),
   mSyncOnDisconnect(false)
{
}

//==============================================================================
// 
//==============================================================================
BChannel::BChannel(long channelID, BSession* pSession, bool syncOnDisconnect) :
   mpSession(pSession),
   mChannelID(channelID),
   mPacketID(0),
   mSyncOnDisconnect(syncOnDisconnect)
{
   BDEBUG_ASSERT((channelID > 0)&&(channelID<255));
}

//==============================================================================
// 
//==============================================================================
BChannel::~BChannel()
{

}

//==============================================================================
// 
//==============================================================================
void BChannel::init(long channelID, BSession* pSession, bool syncOnDisconnect)
{
   mChannelID = channelID;
   mpSession = pSession;
   mPacketID = 0;
   mSyncOnDisconnect = syncOnDisconnect;
}

//==============================================================================
// 
//==============================================================================
uint BChannel::getNextPacketID()
{
   if (!mSyncOnDisconnect)
      return 0;

   if (mPacketID == XNetwork::cMaxPacketID)
      mPacketID = 1;
   else
      mPacketID++;

   return mPacketID;
}

//==============================================================================
// 
//==============================================================================     
long BChannel::_SendPacket(BChannelPacket& packet, long* sizeOut, DWORD flags, const char* file, long line)
{  
   //nlog(cSessionNL, "BChannel::sendPacket size %ld, channelID %ld", sizeOut?*sizeOut:0, getChannelID());

   packet.setChannel(getChannelID());
   packet.setTimeOffset(mpSession->getTimeSync()->getSendOffset());
   if (mSyncOnDisconnect)
      packet.setPacketID(getNextPacketID());

   return mpSession->_SendPacket(packet, sizeOut, flags, file, line);
}

//==============================================================================
// 
//==============================================================================
long BChannel::_SendPacketTo(BClient* pClient, BChannelPacket& packet, long* sizeOut, DWORD flags, const char* file, long line)
{
   BDEBUG_ASSERT(pClient != NULL);
   if (pClient == NULL)
      return -1;

   //nlog(cSessionNL, "BChannel::sendPacketTo getID %ld, size %ld, channelID %ld", client->getID(), sizeOut?*sizeOut:0, getChannelID());

   packet.setChannel(getChannelID());
   packet.setTimeOffset(mpSession->getTimeSync()->getSendOffset());
   // XXX when sending to a specifc client, if you wish to provide synchronization on disconnect, you must
   // set the packetID before calling this method
   BASSERT((mSyncOnDisconnect && packet.getPacketID() > 0) || !mSyncOnDisconnect);

   return mpSession->_SendPacketTo(pClient, packet, sizeOut, flags, file, line);
}

//==============================================================================
// 
//==============================================================================
long BChannel::_SendPacketTo(BMachine* pMachine, BChannelPacket& packet, long* sizeOut, DWORD flags, const char* file, long line)
{
   BDEBUG_ASSERT(pMachine != NULL);
   if (pMachine == NULL)
      return -1;

   packet.setChannel(getChannelID());
   packet.setTimeOffset(mpSession->getTimeSync()->getSendOffset());
   // XXX when sending to a specifc machine, if you wish to provide synchronization on disconnect, you must
   // set the packetID before calling this method
   BASSERT((mSyncOnDisconnect && packet.getPacketID() > 0) || !mSyncOnDisconnect);

   return mpSession->_SendPacketTo(pMachine, packet, sizeOut, flags, file, line);
}

//==============================================================================
// 
//==============================================================================
void BChannel::channelDataReceived(const long clientIndex, const void* data, const DWORD size)
{
   //nlog(cSessionNL, "BChannel::clientDataReceived fromClient %ld, size %ld", clientIndex, size);

   //nlog(cSessionNL, "  notifying observers.");

   mObserverList.channelDataReceived(clientIndex, data, size);
}
