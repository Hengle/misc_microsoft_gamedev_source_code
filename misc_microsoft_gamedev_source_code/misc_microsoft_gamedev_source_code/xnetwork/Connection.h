////==============================================================================
//// Connection.h
////
//// Copyright (c) 2001, Ensemble Studios
////==============================================================================
//
//#ifndef Connection_h
//#define Connection_h
//
////==============================================================================
//// Includes
////
//#include "Socket.h"
//#include "Packet.h"
//
////==============================================================================
////
//class BConnection : private BSocket::BObserver
//{
//   public:
//      
//      enum { cBufferPoolSize = 1024 };
//
//      // Async connection observer (threads generate events asynchronously)
//      class BConnObserver
//      {
//         public:
//            // Your notifications                        
//            virtual void dataReceived (
//               IN BConnection * Connection,
//               IN const void * Buffer,
//               IN LONG Length,
//               IN CONST SOCKADDR_IN * RemoteAddress) = 0
//            {
//               UNREFERENCED_PARAMETER (Connection);
//               UNREFERENCED_PARAMETER (Buffer);
//               UNREFERENCED_PARAMETER (Length);
//               UNREFERENCED_PARAMETER (RemoteAddress);
//            }
//
//            virtual void disconnected (
//               IN BConnection * Connection) = 0
//            {
//               UNREFERENCED_PARAMETER (Connection);
//            }
//
//            /*virtual void connected (
//               IN BConnection * Connection) = 0
//            {
//               UNREFERENCED_PARAMETER (Connection);
//            }*/
//      };
//
//      BConnection( BSocket* pSocket );
//      virtual ~BConnection()  { mpSocket->setObserver( 0 ); mpSocket->dispose(); }
//
//      BConnObserver* setObserver( BConnObserver* pObserver = 0 )  { BConnObserver* pPrevObs = mpObserver; mpObserver = pObserver; return pPrevObs; }
//
//      enum { cErrorNoBuffers=1 };
//
//      HRESULT send( const void* pData, const long size );
//
//      HRESULT sendTo (
//         IN CONST VOID * Buffer,
//         IN ULONG Length,
//         IN CONST SOCKADDR_IN * RemoteAddress);
//
//      virtual HRESULT sendPacket(BPacket &packet);    
//      virtual HRESULT sendPacketTo(BPacket &packet, IN CONST SOCKADDR_IN * RemoteAddress);    
//      
//      BSocket * getSocket (void) { return mpSocket; }
//
//   private:
//
//      // From BSocket::BObserver
//      //
//      virtual void recvd (
//         IN BSocket * Socket,
//         IN const void * Buffer,
//         IN DWORD Length,
//         IN CONST SOCKADDR_IN * RemoteAddress);
//
//      virtual void disconnected (
//         IN BSocket * Socket,
//         IN DWORD Status)
//      {
//         UNREFERENCED_PARAMETER (Socket);
//         UNREFERENCED_PARAMETER (Status);
//
//         mbDisconnected = true;
//
//         if (mpObserver)
//         {
//            mpObserver -> disconnected (this);
//         }
//      }
//
//      /*virtual void connected ( IN BSocket *Socket ) 
//      {  
//         UNREFERENCED_PARAMETER(Socket);
//
//         if (mpObserver)
//         {
//            mpObserver -> connected (this);
//         }
//      }*/
//
//      BSocket* mpSocket;
//      bool     mbDisconnected;
//
//      BConnObserver* mpObserver;
//      //BBufferPool    mBufferPool;      
//
//      // Hidden to deny access
//      //
//      BConnection( const BConnection& other );
//      BConnection& operator=( const BConnection& rhs );
//};
//
////==============================================================================
////
//#endif // Connection_h
//
////==============================================================================
//// EOF: Connection.h
