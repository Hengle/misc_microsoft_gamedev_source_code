//==============================================================================
// Socket.h
//
// Copyright (c) Ensemble Studios, 2001-2007
//==============================================================================

#pragma once

#ifndef __SOCKET_H__
#define __SOCKET_H__

//==============================================================================
// Includes

#include "winsockinc.h"
#include "Packet.h"
#include "GarbageCollected.h"

//==============================================================================
// Forward declarations

class BSocket;

//==============================================================================
// Const declarations


//
// This structure describes a send buffer.
// You get these from BSocket::sendAllocateBuffer.
// You should NEVER allocate these directly.
//
class BSendBuffer
{
   public:
      uint32      Flags;               // going to replace NoResends with a flag so we can also piggy back things like unencrypted traffic
      uint32      MaximumLength;       // Length of memory allocated for Buffer; do not exceed this, and do not alter MaximumLength
      uint32      Length;              // Length of the data actually stored in Buffer.  Update after storing data in *Buffer.
      uint32      VoiceLength;         // Length of the voice data stored in the Buffer.
      PVOID       Buffer;              // Buffer for message storage
};

//==============================================================================
class BSocket : public BGarbageCollected
{
   public:
      enum { cUnresponsiveTimeoutValue = 10000 };
      enum { cVDPHeader=2 }; // reserve two bytes at the beginning of the send buffer for VDP support

      // Internal classes
      class BObserver
      {
         public:
            virtual void recvd (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN DWORD voiceLength,
               IN CONST SOCKADDR_IN * RemoteAddress) { Socket;Buffer;Length;RemoteAddress; }

            virtual void connected ( IN BSocket *Socket ) { Socket; }
            virtual void interfaceAddressChanged () {}

            // return true from this method to drop the socket connection
            virtual bool notifyUnresponsiveSocket ( IN BSocket *Socket, DWORD lastRecvTime ) { Socket; lastRecvTime; return true; }
            virtual bool notifyResponsiveSocket ( IN BSocket *Socket, DWORD lastRecvTime ) { Socket; lastRecvTime; return true; }

            virtual void disconnected (
               IN BSocket * Socket,
               IN DWORD Status) { Socket; Status; }
      };

      enum { cBroadcast = 0, cAnyPort = 0, cAnyAddress = 0, cLoopbackAddress = cAnyAddress };

      // Constructors
      BSocket (
         IN BObserver * Observer)
         : mObserver (Observer), mConnected(false)
      {
      }      

      // Functions      

      // All data is packetized - this is an automatic feature of the sockets

      //virtual HRESULT connect(IN CONST SOCKADDR_IN* RemoteAddress, IN CONST SOCKADDR_IN* LocalAddress OPTIONAL) = 0;

      //
      // Requests a buffer descriptor.
      //
      virtual HRESULT sendAllocateBuffer(IN DWORD MaximumLength, OUT BSendBuffer** ReturnSendBuffer) = 0;

      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      // The implementation takes ownership of the buffer, regardless of what the return value is.
      //
      virtual HRESULT send(IN BSendBuffer* SendBuffer) = 0;

      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      // The implementation takes ownership of the buffer, regardless of what the return value is.
      //
      virtual HRESULT sendTo(IN BSendBuffer* SendBuffer, IN CONST SOCKADDR_IN* DestinationAddress) = 0;

      //
      // Call this to free a buffer allocated using sendAllocateBuffer.
      // This is only useful to free a buffer that you have NOT submitted to send().
      //
      virtual HRESULT sendFreeBuffer(IN BSendBuffer* SendBuffer) = 0;

      HRESULT sendPacket(BPacket &packet);
      HRESULT sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress);

      void setObserver(BObserver *observer) { mObserver = observer; }

      virtual HRESULT close (void) = 0;

      virtual DWORD getLastRecvTime(void) const = 0; // last time we received any data on this socket

      //virtual HRESULT getLocalAddress(OUT SOCKADDR_IN* ReturnLocalAddress) = 0;

      //virtual HRESULT getRemoteAddress(OUT SOCKADDR_IN* ReturnRemoteAddress) = 0;
      
      virtual bool isConnected(void) { return mConnected; }

      //New 360 interfaces
      virtual HRESULT sendTo (
         IN BSendBuffer * SendBuffer,
         IN CONST IN_ADDR * targetTranslatedAddress) {SendBuffer;targetTranslatedAddress;return S_FALSE;}




   protected:
      // Functions
      BObserver                     *getObserver(void) const { return mObserver; }
      bool                          mConnected;

      // Destructors
      virtual ~BSocket(void)
      {
      }

      // Variables    

   private:
      // Functions

      // Variables    
      BObserver                     *mObserver;      
}; // BSocket

#include "LoopbackSocket.h"

//==============================================================================

#endif // __SOCKET_H__
