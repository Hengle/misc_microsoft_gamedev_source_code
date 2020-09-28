//==============================================================================
// Socket.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "Socket.h"

//==============================================================================
// Defines

//==============================================================================
//
HRESULT BSocket::sendPacket(BPacket &packet)
{
   BSerialBuffer sb;
   packet.serialize(sb);

   BSendBuffer *buf=0;
   sendAllocateBuffer(sb.getBufferSize(), &buf);
   buf->Length = sb.getBufferSize();
   memcpy(buf->Buffer, sb.getBuffer(), buf->Length);

   return send(buf);
}

//==============================================================================
//
HRESULT BSocket::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress)
{ 
   BSerialBuffer sb;
   packet.serialize(sb);

   BSendBuffer *buf=0;
   sendAllocateBuffer(sb.getBufferSize(), &buf);
   buf->Length = sb.getBufferSize();
   memcpy(buf->Buffer, sb.getBuffer(), buf->Length);

   return sendTo(buf, RemoteAddress);
}



//==============================================================================
// eof: Socket.cpp
//==============================================================================
