//==============================================================================
// ReliableConnection.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#ifndef _ReliableConnection_H_
#define _ReliableConnection_H_

//==============================================================================
// Includes

#include "Socket.h"
#include "Listener.h"

//==============================================================================
// Forward declarations

class BSocksGameSocket;
class BSocksReliableSocket;
class BSocksReliableListener;

//==============================================================================
// Const declarations


//==============================================================================
class BReliableConnection : public BSocket::BObserver
{
   public:
      // Async connection observer (threads generate events asynchronously)
      class BConnObserver
      {
         public:
            // Your notifications                        
            virtual void dataReceived (
               IN BReliableConnection * Connection,
               IN const void * Buffer,
               IN LONG Length,
               IN CONST SOCKADDR_IN * RemoteAddress) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
               UNREFERENCED_PARAMETER (Buffer);
               UNREFERENCED_PARAMETER (Length);
               UNREFERENCED_PARAMETER (RemoteAddress);
            }

            virtual void disconnected (
               IN BReliableConnection * Connection) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
            }
            
            virtual bool notifyUnresponsiveConnection (
               IN BReliableConnection * Connection, DWORD lastRecvTime)
            {
               UNREFERENCED_PARAMETER (Connection);
               UNREFERENCED_PARAMETER (lastRecvTime);
               return true;
            }

            virtual bool notifyResponsiveConnection (
               IN BReliableConnection * Connection, DWORD lastRecvTime)
            {
               UNREFERENCED_PARAMETER (Connection);
               UNREFERENCED_PARAMETER (lastRecvTime);
               return true;
            }

            virtual void connected (
               IN BReliableConnection * Connection) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
            }

            virtual void connectTimeout (
               IN BReliableConnection * Connection) = 0
            {
               UNREFERENCED_PARAMETER (Connection);
            }
      };

      // Constructors
      BReliableConnection( BSocket *socket = 0 );

      // Destructors
      ~BReliableConnection( void );

      // Functions

      enum { cRandomConnectionFailFlag = 0x1 };
      HRESULT connect(bool primary, const SOCKADDR_IN &localAddress, const SOCKADDR_IN &translatedLocalAddress, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress);
      void disconnect(void);

      enum { cRandomConnectionFail = 0x1 };      

      BConnObserver* setObserver( BConnObserver* pObserver = 0 )  { BConnObserver* pPrevObs = mpObserver; mpObserver = pObserver; return pPrevObs; }

      enum { cErrorNoBuffers=1 };

      HRESULT send( const void* pData, const long size, const DWORD flags=0 );

      HRESULT sendTo (
         IN CONST VOID * Buffer,
         IN ULONG Length,
         IN CONST SOCKADDR_IN * RemoteAddress);

      virtual HRESULT sendPacket(BPacket &packet);    
      virtual HRESULT sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress);
      
      bool isConnected(void) { return mState==cConnected?true:false; }

      SOCKADDR_IN *getRemoteAddress(void) { return &mRemoteAddress; }
      SOCKADDR_IN *getTranslatedRemoteAddress(void) { return &mTranslatedRemoteAddress; }      

      BSocket                 *mConnectedSocket;   

#ifdef XBOX    
      static void tickActiveConnections(void);
#endif

   protected:
   
#ifndef XBOX
      friend class BReliableConnectionWindowClass;

      static LRESULT WINAPI WindowProcedure (
         IN HWND Window,
         IN UINT Message,
         IN WPARAM Parameter1,
         IN LPARAM Parameter2);

      
       LRESULT onWindowMessage (
         IN HWND Window,
         IN UINT Message,
         IN WPARAM Parameter1,
         IN LPARAM Parameter2);
#endif     

   private:      

      // From BSocket::BObserver
      //
      virtual void recvd (
         IN BSocket * Socket,
         IN const void * Buffer,
         IN DWORD Length,
         IN DWORD voiceLength,
         IN CONST SOCKADDR_IN * RemoteAddress);

      virtual void disconnected (
         IN BSocket * Socket,
         IN DWORD Status);

      virtual bool notifyUnresponsiveSocket (
         IN BSocket * Socket,
         DWORD lastRecvTime);
          
      virtual bool notifyResponsiveSocket (
         IN BSocket * Socket,
         DWORD lastRecvTime);

      virtual void connected ( IN BSocket *Socket );

      void destroyUnconnectedSockets(void);

      BSocket *getBestConnectedSocket(void);
      const char *getSocketName(BSocket *socket);
      void tic (DWORD timerID);

      BConnObserver* mpObserver;
      
      // Hidden to deny access
      //
      BReliableConnection( const BReliableConnection& other );
      BReliableConnection& operator=( const BReliableConnection& rhs );

      enum { cConnectTimer = 0 };
      enum { cConnectTimerInterval = 250 };

      enum { cDisconnected=0, cConnecting, cConnected };
      long  mState;

      enum { cDirectGameSocketConnectTimeout = 5000 };
      BSocksGameSocket        *mDirectGameSocket;
      DWORD                   mDirectGameSocketConnectTimer;

      BSocksGameSocket        *mTranslatedGameSocket;
      enum { cTranslatedGameSocketConnectTimeout = 5000 };
      DWORD                   mTranslatedGameSocketConnectTimer;
     
      BSocket                 *mSuppliedSocket;

#ifndef XBOX      
      HWND                    mWindow;      
#endif
      
      bool                    mInServiceCB;
      bool                    mPrimary;
      bool                    mDisableNotifications;
      SOCKADDR_IN             mRemoteAddress;
      SOCKADDR_IN             mTranslatedRemoteAddress;

      DWORD                   mConnectTimer;

#ifdef XBOX
      static int              mNumActiveConnections;
      enum { MaxActiveConnections = 32 };
      static BReliableConnection* mpActiveConnections[MaxActiveConnections];
      virtual void checkForEvents(void);
      
      DWORD mTimerInterval;
      DWORD mTimerLastTick;   
#endif      
            
      // Variables

}; // BReliableConnection


//==============================================================================
#endif // _ReliableConnection_H_

//==============================================================================
// eof: ReliableConnection.h
//==============================================================================
