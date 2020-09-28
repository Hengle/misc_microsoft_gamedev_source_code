//==============================================================================
// SocksReliableSocket.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
#include "Socket.h"
#include "SocksSocketBase.h"
#include <etl/List.hpp>

//==============================================================================
// Forward declarations
class BObserver;
class BSocksReliableSocket;

const DWORD MaximumReliableMessageLength = 0x10000;

struct BSocksReliableMessageHeader
{
   DWORD    MessageLength;       // host order   
};

class BSocksReliableSendBuffer :
   public BSendBuffer
{
public:

   DWORD          Flags;
   BSocksReliableSocket * Socket;
   BSocksReliableMessageHeader * Header;
};

struct BSocksReliableRecvBuffer
{
   DWORD          Flags;
   BSocksReliableSocket *           Socket;
};



//==============================================================================
// Const declarations

//==============================================================================
class BSocksReliableSocket : public BSocksSocketBase
{
protected:

   public:
      // Constructor

      BSocksReliableSocket (
         IN BObserver * Observer);      

      HRESULT createFromAccept (
         IN SOCKET ClientSocket);
      
      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      //

      virtual HRESULT send (
         IN BSendBuffer * SendBuffer);

      //
      // This method takes ownership of SendBuffer.  Do not touch SendBuffer after calling this method.
      //

      virtual HRESULT sendTo (
         IN BSendBuffer * SendBuffer,
         IN CONST SOCKADDR_IN * DestinationAddress) { SendBuffer; DestinationAddress; return E_FAIL; }

      //
      // Call this to free a buffer allocated using sendAllocateBuffer.
      // This is only useful to free a buffer that you have NOT submitted to send().
      //

      virtual HRESULT sendAllocateBuffer (
         IN DWORD MaximumLength,
         OUT BSendBuffer ** ReturnSendBuffer);

      virtual HRESULT sendFreeBuffer (
         IN BSendBuffer * SendBuffer);

      virtual HRESULT close (void);

      virtual HRESULT getLocalAddress (
         OUT SOCKADDR_IN * ReturnLocalAddress);

      virtual HRESULT getRemoteAddress (
         OUT SOCKADDR_IN * ReturnRemoteAddress);

      virtual HRESULT connect (
         IN CONST SOCKADDR_IN * RemoteAddress,
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL);

      virtual HRESULT dispose(void);

      virtual DWORD getLastRecvTime(void) const { return 0; } // FIXME: this value is bogus, but not used

   protected:
      // Destructors
      virtual ~BSocksReliableSocket(void);

   private:

      HRESULT sendNextBuffer (void);

      virtual void sendReady (void);

      HRESULT startSocket (void);

      enum { RecvMinimumLength = 0x100 };

      HRESULT recvIssue (void);

      virtual void networkClose (
         IN HRESULT Status);

      void notifySocketClose (
         IN HRESULT Status);

      virtual BOOL recvReady (void);

      HRESULT recvParse (void);

      HRESULT recvGrow (
         IN DWORD Length);

      enum { cMaxRecvRecords=128 };      
      enum { cMaxRecvSize=2048 };

      SOCKADDR_IN       mLocalAddress;
      SOCKADDR_IN       mRemoteAddress;

      struct BSendState
      {
         //
         // List of buffers waiting to be transmitted
         //
         BPointerList<BSocksReliableSendBuffer> mQueueList;
         

         BSocksReliableSendBuffer * ActiveBuffer;
         DWORD                ActiveIndex;
         
         BSendState (void)
         {
            ActiveBuffer = 0;
            ActiveIndex = 0;            
         }

         ~BSendState (void)
         {
            BASSERT (ActiveBuffer == NULL);
            BASSERT (mQueueList.getSize() == 0);
         }

      }  mSend;



      //
      // Receive State
      //
      // We only ever have a single receive pending.
      //

      struct BRecvState
      {
         LPBYTE               Buffer;        // allocated using new BYTE []
         DWORD                MaximumLength;
         DWORD                Length;         
         BOOL                 IsBusy;                    // checks for reentrancy
         BOOL                 ReentrantCallOccurred;
         BOOL *               TerrifyingLocalVariable;

         BRecvState (void)
         {
            IsBusy = FALSE;
            ReentrantCallOccurred = FALSE;
            Buffer = NULL;
            Length = 0;
            MaximumLength = 0;            
            TerrifyingLocalVariable = NULL;
         }

         ~BRecvState (void)
         {
            if (IsBusy)
            {
               BASSERT (TerrifyingLocalVariable != NULL);
               *TerrifyingLocalVariable = TRUE;
            }
            else
            {
               BASSERT (TerrifyingLocalVariable == NULL);
            }

            BASSERT (Buffer == NULL);
            BASSERT (MaximumLength == 0);
            BASSERT (Length == 0);
         }

      } mRecv;

private:
      BSocksReliableSocket(const BSocksReliableSocket &other);
      BSocksReliableSocket &operator=(const BSocksReliableSocket &rhs);

}; // BSocksReliableSocket

//==============================================================================
// eof: SocksReliableSocket.h
//==============================================================================