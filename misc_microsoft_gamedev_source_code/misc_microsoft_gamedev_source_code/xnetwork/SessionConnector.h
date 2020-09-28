////==============================================================================
//// SessionConnector.h
////
//// Copyright (c) 2002, Ensemble Studios
////==============================================================================
//
//#pragma once 
//
//#ifndef _SessionConnector_H_
//#define _SessionConnector_H_
//
////==============================================================================
//// Includes
//
//#include "Socket.h"
//#include "NetPackets.h"
//
////==============================================================================
//// Forward declarations
//
//class BSocksUnreliableSocket;
//
////==============================================================================
//// Const declarations
//
////==============================================================================
//class BSessionDescriptor
//{
//   public:
//      BSessionDescriptor(const SOCKADDR_IN &advertiseAddress, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress,
//                           void *advertiseInfo, DWORD advertiseInfoSize, DWORD foundTime)
//      {
//         mAdvertiseAddress = advertiseAddress;
//         mRemoteAddress = remoteAddress;
//         mTranslatedRemoteAddress = translatedRemoteAddress;
//         mAdvertiseInfo = NULL;
//         if (advertiseInfoSize)
//         {
//            mAdvertiseInfo = new char[advertiseInfoSize];
//            memcpy(mAdvertiseInfo, advertiseInfo, advertiseInfoSize);
//         }
//         mAdvertiseInfoSize = advertiseInfoSize;         
//         mFoundTime = foundTime;
//         mJoinTime = 0;
//         mState = cFound;
//      }
//      ~BSessionDescriptor(void)
//      {
//         if (mAdvertiseInfo)
//         {
//            delete [] mAdvertiseInfo;
//            mAdvertiseInfo=NULL;
//         }
//         mAdvertiseInfoSize=0;
//      }
//      SOCKADDR_IN mRemoteAddress;
//      SOCKADDR_IN mAdvertiseAddress;
//      SOCKADDR_IN mTranslatedRemoteAddress;
//      void *mAdvertiseInfo;
//      DWORD mAdvertiseInfoSize;
//      enum { cFound=0, cJoining, cJoiningIP, cJoined };
//      long mState;
//      DWORD mFoundTime;      
//      DWORD mJoinTime;
//};
//
//// This class is used to find and enumerate sessions on the LAN, but it is also to 
//// find and enumerate ports for the connectivity code on startup. FIXME: These two tasks
//// should either be split into seperate classes, or this class should be made more generic.
//// you control which task this class does with the ctor "type" param
////==============================================================================
//class BSessionConnector : public BSocket::BObserver
//{
//   public:
//      class BSCObserver
//      {
//         public:
//            enum eJoinResult
//            {
//               cJoinOK,
//               cJoinPending,
//               cJoinFull,
//               cJoinRejected,
//               cJoinCRCMismatch,
//               cJoinUserExists,
//               cJoinMax
//            };
//
//            virtual void findListUpdated(BSessionConnector *connector) = 0;
//            virtual void joinRequest(BSessionConnector *connector, XUID xuid, const BSimString &gamertag, DWORD crc, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, eJoinResult &result) = 0;
//            virtual void joinReply(BSessionConnector *connector, long index, eJoinResult result) = 0;
//            virtual void interfaceAddressChanged() {}
//      };
//
//      // Constructors
//      enum { cSessionConnector, cPortFinder };
//      BSessionConnector( BSCObserver *observer );
//
//      // Destructors
//      ~BSessionConnector( void );
//
//      // Functions
//
//      void service(void);
//
//      // use this class to either find sessions or discover the "use of ports", but not both at the same time
//      // 
//      
//      // default advertise function, advertises a session's existence on the local subnet
//      HRESULT advertise(const SOCKADDR_IN &localAddress, const SOCKADDR_IN &translatedLocalAddress, 
//                           void *advertiseInfo, const DWORD advertiseInfoSize, DWORD localCrc, unsigned short advertisePort=0);
//      // default find function, finds sessions on the local subnet
//      HRESULT find(const SOCKADDR_IN &remoteAddress, DWORD findInterval = cFindInterval);
//      // send a join request to a session
//      HRESULT join(const long index, const SOCKADDR_IN &localAddress, 
//                    const SOCKADDR_IN &translatedLocalAddress, DWORD localCRC, XUID xuid);
//
//      HRESULT findIP(const SOCKADDR_IN &remoteAddress, DWORD findInterval = cFindInterval);
//      HRESULT joinIP(const long index, const SOCKADDR_IN &localAddress, 
//                     const SOCKADDR_IN &translatedLocalAddress, DWORD localCRC, XUID xuid);
//      long getSessionDescriptorAmount(void) { return mFoundSessions.getNumber(); }
//      BSessionDescriptor *getSessionDescriptor(long index) { return mFoundSessions[index]; }
//
//      HRESULT advertisePort(unsigned short bindPort, unsigned short advertisePort);
//      HRESULT findPorts(const SOCKADDR_IN &remoteAddress)
//      {
//         return find(remoteAddress, cPortFindInterval);
//      }   
//
//      //Horrible hack by Eric
//      BSessionDescriptor* sessionListHack(const SOCKADDR_IN &advertiseAddress, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress, void *advertiseInfo, DWORD advertiseInfoSize);
//
//      // BSocket::BObserver
//      virtual void recvd (
//         IN BSocket * Socket,
//         IN const void * Buffer,
//         IN DWORD Length,
//         IN CONST SOCKADDR_IN * RemoteAddress);
//
//      virtual void interfaceAddressChanged();
//
//#ifdef XBOX
//      enum { cAdvertisePort = 2289, cPortAdvertisePort = 2286, cPortAdvertiseLockPort = 2285 };
//#else
//      enum { cAdvertisePort = 2279, cPortAdvertisePort = 2276, cPortAdvertiseLockPort = 2275 };
//#endif
//
//   private:            
//
//      enum { cFindInterval = 333, cJoinInterval = 1000 };
//      enum { cPortFindInterval = 333 };
//      enum { cFoundTimeout = cFindInterval+2000 };
//
//      DWORD                               mFindTimer;
//      DWORD                               mFindInterval;
//      DWORD                               mFoundTimeout;
//      DWORD                               mLocalCRC;
//      DWORD                               mAdvertiseInfoSize;
//
//      void                                *mAdvertiseInfo;
//      BSocksUnreliableSocket              *mAdvertiseSocket;
//      BSocksUnreliableSocket              *mFindSocket;
//      BSCObserver                         *mObserver;
//
//      XUID                                mXuid;
//
//      SOCKADDR_IN                         mLocalAddress;
//      SOCKADDR_IN                         mTranslatedLocalAddress;
//      SOCKADDR_IN                         mRemoteAddress;
//      SOCKADDR_IN                         mTranslatedRemoteAddress;
//      SOCKADDR_IN                         mFindAddress;
//
//      BSimString                          mGamertag;
//
//      BDynamicSimArray <BSessionDescriptor*>  mFoundSessions;
//      BDynamicSimArray <SOCKADDR_IN>          mJoinedAddresses;
//
//}; // BSessionConnector
//
//
////==============================================================================
//#endif // _SessionConnector_H_
//
////==============================================================================
//// eof: SessionConnector.h
////==============================================================================
