//==============================================================================
// SocksUnreliableSocket.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
#include "Socket.h"
#include "SocksSocketBase.h"
#include "maxsendsize.h"
#include "containers\PointerList.h"

//==============================================================================
// Forward declarations
class BObserver;
class BSocksUnreliableSocket;


class BSocksUnreliableSocketUser
{
public:
   BSocksUnreliableSocketUser() : mUnreliableSocket(NULL) {}
   
   BSocksUnreliableSocket*    getSocket(void) const { return mUnreliableSocket; }
   void                       setSocket(BSocksUnreliableSocket* socket) { mUnreliableSocket = socket; }

   virtual bool recvd (
            IN BSocket * Socket,
            IN const void * Buffer,
            IN DWORD Length,
            IN DWORD voiceLength,
            IN CONST SOCKADDR_IN * RemoteAddress) = 0;

protected:
   BSocksUnreliableSocket        *mUnreliableSocket;
};

//
// Header structure for tracking send and receive buffers.
// Actual receive buffer immediately follows this structure (single alloc)
//

class BSocksUnreliableBuffer :
   public BSendBuffer
{
public:
   SOCKADDR_IN    RemoteAddress;
   INT            RemoteAddressLength;
   BSocksUnreliableSocket *   Socket;
};

//==============================================================================
// Const declarations

//==============================================================================
class BSocksUnreliableSocket : public BSocksSocketBase
{
public:
   enum { DefaultRecvAllocationLength = 0x1000 };
   enum { DefaultRecvActiveGoal = 4 };


public:
   // Constructors      
   BSocksUnreliableSocket (
      IN BObserver * Observer = 0);   

   // Functions      

   virtual HRESULT connect (
      IN CONST SOCKADDR_IN * RemoteAddress,
      IN CONST SOCKADDR_IN * LocalAddress OPTIONAL);

   virtual HRESULT close (void);

   virtual HRESULT sendAllocateBuffer (
      IN DWORD MaximumLength,
      OUT BSendBuffer ** ReturnSendBuffer);

   virtual HRESULT sendFreeBuffer (
      IN BSendBuffer * SendBuffer);

   virtual HRESULT send (
      IN BSendBuffer * SendBuffer);

   virtual HRESULT sendTo (
      IN BSendBuffer * SendBuffer,
      IN CONST SOCKADDR_IN * DestinationAddress);

   virtual HRESULT getLocalAddress (
      OUT SOCKADDR_IN * ReturnLocalAddress);

   virtual HRESULT getRemoteAddress (
      OUT SOCKADDR_IN * ReturnRemoteAddress);
   
   HRESULT createAndBind (
      IN CONST SOCKADDR_IN * LocalAddress OPTIONAL);

   void ILikeToProvokeVtableComplaints (void)
   {
      delete new BSocksUnreliableSocket();
   }   

   virtual HRESULT dispose(void);

   virtual DWORD getLastRecvTime(void) const { return 0; } // FIXME: this value is bogus, but not used

private:

   HRESULT sendNextBuffer (void);

   virtual void sendReady (void);

   virtual BOOL recvReady (void);

   void GUIDtoString(unsigned char *src, PSTR strDest, unsigned long destBufLen);

   HRESULT allocateBuffer (
      IN DWORD MaximumLength,
      OUT BSocksUnreliableBuffer ** ReturnBuffer);

   HRESULT freeBuffer (
      IN BSocksUnreliableBuffer * Buffer);

private:

   SOCKADDR_IN       mLocalAddress;
   SOCKADDR_IN       mRemoteAddress;

protected:
   // Destructors
   virtual ~BSocksUnreliableSocket(void);

private:

   //
   // Receive State Machine
   //

   struct BRecvState
   {
      LPBYTE Buffer;
      DWORD MaximumLength;

      BRecvState (void)
      {
         Buffer = NULL;
         MaximumLength = 0;
      }

      ~BRecvState (void)
      {
         if (Buffer != NULL)
         {
            delete [] Buffer;
            Buffer = NULL;
            MaximumLength = 0;
         }
      }

      HRESULT grow (
         IN DWORD RequiredLength)
      {
         if (RequiredLength <= MaximumLength)
         {
            return S_OK;
         }

         DWORD NewMaximumLength = RequiredLength * 3 / 2;
         LPBYTE NewBuffer = new BYTE [NewMaximumLength];

         if (NewBuffer == NULL)
         {
            return E_OUTOFMEMORY;
         }

         if (Buffer != NULL)
         {
            delete [] Buffer;
         }

         Buffer = NewBuffer;
         MaximumLength = RequiredLength;
         return S_OK;
      }

   } mRecv;

   //
   // Send State Machine
   //

   struct BSendState
   {
      // contains BSocksUnreliableBuffer, of buffers waiting to be transmitted
      BPointerList<BSocksUnreliableBuffer> mQueueList;

      BSendState (void)
      {         
      }

      ~BSendState (void)
      {
         BASSERT (mQueueList.getSize() == 0);
      }

   } mSend;

   char mSendBuffer[cMaxSendSize];

}; // BSocksUnreliableSocket



//==============================================================================
// eof: SocksUnreliableSocket.h
//==============================================================================
