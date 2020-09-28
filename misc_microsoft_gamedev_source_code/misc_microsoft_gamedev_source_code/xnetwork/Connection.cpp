//==============================================================================
// Connection.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//
#include "Precompiled.h"

#include "Connection.h"
#include "SerialBuffer.h"

uint gConnectionDummy;

////==============================================================================
////
//BConnection::BConnection( BSocket* pSocket ) :
//   mpSocket( pSocket ),
//   mbDisconnected( false ),
//   mpObserver( 0 )
//{
//   BASSERT( mpSocket != 0 );
//
//   mpSocket->setObserver( this );
//}
//// BConnection::BConnection
//
////==============================================================================
////
//HRESULT BConnection::send (
//   const void* Buffer,
//   const long Length)
//{
//   if ( mbDisconnected )
//      return 0;
//
//   BSendBuffer * SendBuffer=0;
//   HRESULT Result;
//
//   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
//   if (FAILED (Result))
//   {
//      nlog(cTransportNL,"BConnection::send: failed to allocate buffer");
//      return Result;
//   }
//
//   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
//   SendBuffer -> Length = Length;
//
//   Result = mpSocket -> send (SendBuffer);
//
//   return Result;
//}
//
//// BConnection::send
//
//
//HRESULT BConnection::sendTo (
//   IN CONST VOID * Buffer,
//   IN ULONG Length,
//   IN CONST SOCKADDR_IN * RemoteAddress)
//{
//   if (mbDisconnected)
//   {
//      nlog(cTransportNL, "BConnection::sendTo: can't send, not connected");
//      return E_FAIL;
//   }
//
//
//   BSendBuffer * SendBuffer;
//   HRESULT Result;
//
//   Result = mpSocket -> sendAllocateBuffer (Length, &SendBuffer);
//   if (FAILED (Result))
//   {
//      nlog(cTransportNL, "BConnection::send: failed to allocate buffer");
//      return Result;
//   }
//
//   CopyMemory (SendBuffer -> Buffer, Buffer, Length); // suck it
//   SendBuffer -> Length = Length;
//
//   Result = mpSocket -> sendTo (SendBuffer, RemoteAddress);
//
//   return Result;
//}
//
//
////==============================================================================
////
//HRESULT BConnection::sendPacket(BPacket &packet)
//{
//   if ( mbDisconnected )
//      return 0;  
//   
//   BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
//   packet.serialize(sb);
//
//   return send(sb.getBuffer(), sb.getBufferSize());
//}
//// BConnection::sendPacket
//
////==============================================================================
////
//HRESULT BConnection::sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress)
//{
//   if ( mbDisconnected )
//      return 0;  
//   
//   BSerialBuffer sb; //(data, mBufferPool.getBufferSize(), false);
//   packet.serialize(sb);
//
//   return sendTo(sb.getBuffer(), sb.getBufferSize(), RemoteAddress);
//}
//// BConnection::sendPacket
//
////==============================================================================
////
//
////==============================================================================
////
//void BConnection::recvd (
//   IN BSocket * Socket,
//   IN const void *  Buffer,
//   IN DWORD Length,
//   IN CONST SOCKADDR_IN * RemoteAddress)
//{ 
//   UNREFERENCED_PARAMETER (Socket);
//
//   if (mpObserver)
//   {
//      mpObserver -> dataReceived (this, Buffer, Length, RemoteAddress);
//   }
//}
//
////==============================================================================
////
//
////==============================================================================
//// EOF: Connection.cpp
