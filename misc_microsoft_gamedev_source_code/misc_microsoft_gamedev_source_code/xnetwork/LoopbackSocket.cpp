//==============================================================================
// LoopbackSocket.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"

int BLoopbackSocket::mNumActiveSockets = 0;
BLoopbackSocket* BLoopbackSocket::mpActiveSockets[BLoopbackSocket::MaxActiveSockets];

//////////////////////////////////////////////////////////////////////////
// static
void BLoopbackSocket::tickActiveSockets(void)
{
   for (int i = 0; i < mNumActiveSockets; i++)
      mpActiveSockets[i]->checkForEvents();
}

//////////////////////////////////////////////////////////////////////////
BLoopbackSocket::BLoopbackSocket (BObserver * Observer)
: BSocket (Observer)
, mSendMessages(false)
{
   mIsEnabled = TRUE;
   ZeroMemory (&mRemoteAddress, sizeof SOCKADDR_IN);

   if (mNumActiveSockets < MaxActiveSockets)
      mpActiveSockets[mNumActiveSockets++] = this;
}

//////////////////////////////////////////////////////////////////////////
HRESULT BLoopbackSocket::dispose(void)
{
   mSendMessages = false;

   mIsEnabled = FALSE;

   int i;
   for (i = 0; i < mNumActiveSockets; i++)
   {  
      if (mpActiveSockets[i] == this)
      {
         mpActiveSockets[i] = mpActiveSockets[mNumActiveSockets - 1];
         break;
      }
   }

   BASSERT(i != mNumActiveSockets);
   if (i < mNumActiveSockets)
      mNumActiveSockets--;

   // clear out the send list
   BSendBuffer * SendBuffer = mSendList.removeHead();
   while (SendBuffer)
   {
      sendFreeBuffer(SendBuffer);

      SendBuffer = mSendList.removeHead();
   }

   return BSocket::dispose();
}

//////////////////////////////////////////////////////////////////////////
HRESULT BLoopbackSocket::sendQueueBuffer (
   IN BSendBuffer * SendBuffer)
{
   if (!mIsEnabled)
   {
      nlog (cTransportNL, "BLoopbackSocket: cannot accept buffer, object is being destroyed");
      sendFreeBuffer (SendBuffer);
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   BASSERT (SendBuffer != NULL);
   BASSERT (SendBuffer -> Length <= SendBuffer -> MaximumLength);

   mSendList.addToTail(SendBuffer);

   return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT BLoopbackSocket::sendAllocateBuffer (
   IN DWORD MaximumLength,
   OUT BSendBuffer ** ReturnSendBuffer)
{
   if (ReturnSendBuffer == NULL)
   {
      return E_INVALIDARG;
   }

   LPBYTE MemoryBlock;
   BSendBuffer * Buffer;
   DWORD AllocationLength;

   AllocationLength = MaximumLength + sizeof (BSendBuffer);
   MemoryBlock = (LPBYTE)gNetworkHeap.New(AllocationLength);
   if (MemoryBlock == NULL)
   {
      BFAIL("gNetworkHeap: Out of memory");
      return E_OUTOFMEMORY;
   }

   Buffer = reinterpret_cast <BSendBuffer *> (MemoryBlock);
   Buffer -> Buffer = MemoryBlock + sizeof (BSendBuffer);
   Buffer -> MaximumLength = MaximumLength;
   Buffer -> Length = 0;
   Buffer -> VoiceLength = 0;
   Buffer -> Flags = 0;

   *ReturnSendBuffer = Buffer;
   return S_OK;
}

//////////////////////////////////////////////////////////////////////////
HRESULT BLoopbackSocket::sendFreeBuffer (
   IN BSendBuffer * SendBuffer)
{
   sendDeleteBuffer (SendBuffer);
   return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// static
void BLoopbackSocket::sendDeleteBuffer (
   IN BSendBuffer * SendBuffer)
{
   BASSERT (SendBuffer != NULL);
   if (SendBuffer)
      gNetworkHeap.Delete(SendBuffer);
}

//////////////////////////////////////////////////////////////////////////
HRESULT BLoopbackSocket::send (
   IN BSendBuffer * SendBufferBase)
{
   if (mRemoteAddress.sin_family != AF_INET)
   {
      //
      // If you're going to use this version of send(), you need to call connect().
      //
      
      return HRESULT_FROM_WIN32 (ERROR_INVALID_STATE);
   }

   nlog (cTransportNL, "BLoopbackSocket: send %ld", SendBufferBase->Length);

   return sendQueueBuffer (SendBufferBase);
}

//////////////////////////////////////////////////////////////////////////
HRESULT BLoopbackSocket::sendTo (
   IN BSendBuffer * SendBufferBase,
   CONST SOCKADDR_IN * RemoteAddress)
{
   if (RemoteAddress == NULL)
   {
      return E_INVALIDARG;
   }

   nlog (cTransportNL, "BLoopbackSocket: sendTo %ld", SendBufferBase->Length);

   return sendQueueBuffer (SendBufferBase);
}

//////////////////////////////////////////////////////////////////////////
BOOL BLoopbackSocket::recvReady()
{
   BSendBuffer * SendBuffer = mSendList.removeHead();
   while (SendBuffer)
   {
      if (getObserver())
         getObserver()->recvd(this, SendBuffer->Buffer, SendBuffer->Length, SendBuffer->VoiceLength, &mRemoteAddress);

      sendFreeBuffer (SendBuffer);

      SendBuffer = mSendList.removeHead();
   }
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////
void BLoopbackSocket::checkForEvents(void)
{
   if (!mSendMessages)
      return;

   recvReady();
}
