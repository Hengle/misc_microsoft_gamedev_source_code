//#pragma once

#ifndef _LoopbackSocket_H_
#define _LoopbackSocket_H_

class BLoopbackSocket;

//==============================================================================
class BLoopbackSocket : public BSocket
{
private:

   BOOL mIsEnabled;

   public:
      static void tickActiveSockets(void);

      // Constructors
      BLoopbackSocket (
         IN BObserver * Observer = NULL);

      // Functions      
      
      virtual HRESULT connect (
         IN CONST SOCKADDR_IN * RemoteAddress)
      {
         if (RemoteAddress == NULL)
         {
            return E_INVALIDARG;
         }

         mRemoteAddress = *RemoteAddress;

         mSendMessages = true;

         getObserver()->connected(this);

         return S_OK;
      }

      virtual HRESULT dispose(void);

      virtual HRESULT sendAllocateBuffer (
         IN DWORD MaximumLength,
         OUT BSendBuffer ** ReturnSendBuffer);

      virtual HRESULT sendFreeBuffer (
         IN BSendBuffer * SendBuffer);

      virtual HRESULT send (
         IN BSendBuffer * SendBufferBase);

      virtual HRESULT sendTo (
         IN BSendBuffer * SendBufferBase,
         CONST SOCKADDR_IN * RemoteAddress);

   private:

      static void sendDeleteBuffer (
         IN BSendBuffer * SendBuffer);

      HRESULT sendQueueBuffer (
         IN BSendBuffer * SendBuffer);

      static void CALLBACK apcRoutine (
         IN DWORD ApcArgument);

   public:

      virtual HRESULT getLocalAddress (
         OUT SOCKADDR_IN * ReturnLocalAddress)
      {
         ReturnLocalAddress -> sin_family = AF_INET;
         ReturnLocalAddress -> sin_addr.s_addr = htonl (INADDR_LOOPBACK);
         ReturnLocalAddress -> sin_port = htons (0);
         return S_OK;
      }

      virtual HRESULT getRemoteAddress (
         OUT SOCKADDR_IN * ReturnRemoteAddress)
      {
         *ReturnRemoteAddress = mRemoteAddress;
         return S_OK;
      }

   
      void ILikeToProvokeVtableErrors (void)
      {
         delete new BLoopbackSocket ();
      }

      HRESULT connect (
         IN CONST SOCKADDR_IN * RemoteAddress,
         IN CONST SOCKADDR_IN * LocalAddress OPTIONAL)
      {
         UNREFERENCED_PARAMETER (LocalAddress);

         if (RemoteAddress == NULL)
         {
            return E_INVALIDARG;
         }

         mRemoteAddress = *RemoteAddress;

         mSendMessages = true;

         return S_OK;
      }

      HRESULT close (void)
      {
         return S_OK;
      }

      virtual DWORD getLastRecvTime(void) const { return 0; }

   private:

      bool mSendMessages;
      SOCKADDR_IN mRemoteAddress;

      static int mNumActiveSockets;
      enum { MaxActiveSockets = 32 };
      static BLoopbackSocket* mpActiveSockets[MaxActiveSockets];

      BPointerList<BSendBuffer> mSendList;

      virtual BOOL recvReady (void);

      virtual void checkForEvents(void);

}; // BSocket

#endif // _LoopbackSocket_H_
