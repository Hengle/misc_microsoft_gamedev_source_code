//==============================================================================
// SessionConnector.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "SessionConnector.h"
#include "SocksUnreliableSocket.h"
#include "sockshelper.h"

#include "config.h"
#include "econfigenum.h"

uint gSessionConnectorDummy;

//
////==============================================================================
//// Defines
//
////==============================================================================
//// BSessionConnector::BSessionConnector
////==============================================================================
//BSessionConnector::BSessionConnector(BSCObserver *observer) :
//   mFindSocket(0),
//   mAdvertiseSocket(0),
//   mObserver(observer),
//   mFindInterval(cFindInterval),
//   mFoundTimeout(cFoundTimeout),
//   mLocalCRC(0),
//   mXuid(0)
//{
//   memset(&mLocalAddress, 0, sizeof(mLocalAddress));
//   memset(&mTranslatedLocalAddress, 0, sizeof(mTranslatedLocalAddress));
//   memset(&mRemoteAddress, 0, sizeof(mRemoteAddress));
//   memset(&mTranslatedRemoteAddress, 0, sizeof(mTranslatedRemoteAddress));
//   memset(&mFindAddress, 0, sizeof(mFindAddress));
//   mAdvertiseInfo = 0;
//   mAdvertiseInfoSize = 0;
//} // BSessionConnector::BSessionConnector
//
////==============================================================================
//// BSessionConnector::~BSessionConnector
////==============================================================================
//BSessionConnector::~BSessionConnector(void)
//{
//   for(long i=0; i<mFoundSessions.getNumber(); i++)
//      delete mFoundSessions[i];
//   mFoundSessions.clear();
//   if (mAdvertiseInfo)
//      delete mAdvertiseInfo;
//   if (mFindSocket)
//      mFindSocket->dispose();
//   if (mAdvertiseSocket)
//      mAdvertiseSocket->dispose();
//} // BSessionConnector::~BSessionConnector
//
////==============================================================================
//// BSessionConnector::
////==============================================================================
//
////==============================================================================
//// 
//HRESULT BSessionConnector::advertise(const SOCKADDR_IN &localAddress, const SOCKADDR_IN &translatedLocalAddress, 
//                           void *advertiseInfo, const DWORD advertiseInfoSize, DWORD localCrc, unsigned short advertisePort)
//{
//   //nlog(cSessionConnAddrNL, "BSessionConnector[%p]::advertise at %ld", this, advertisePort);
//
//   mLocalCRC = localCrc;
//   mLocalAddress = localAddress;   
//   mTranslatedLocalAddress = translatedLocalAddress;
//   
//   SOCKADDR_IN advertiseAddress;
//   memset(&advertiseAddress, 0, sizeof(advertiseAddress));
//   advertiseAddress.sin_family = AF_INET; 
//   if (advertisePort == 0)
//      advertisePort = cAdvertisePort;
//      
////#ifdef XBOX
//// rg [7/9/05] From XDK Docs: The sin_addr member of the SOCKADDR_IN structure (whose type is an in_addr structure) must have its s_addr member set to INADDR_ANY.
//// bind() RIP's otherwise.
//// I fixed this by locally forcing s_addr to INADDR_ANY before the bind call
////   advertiseAddress.sin_addr.s_addr = INADDR_ANY;
////#else      
//   advertiseAddress.sin_addr = localAddress.sin_addr;
////#endif
//   
//   advertiseAddress.sin_port = htons(advertisePort);
//
//   if (mAdvertiseInfo)
//      delete mAdvertiseInfo;
//   mAdvertiseInfo = 0;
//
//   if (advertiseInfoSize)
//   {
//      mAdvertiseInfo = new char[advertiseInfoSize];
//      memcpy(mAdvertiseInfo, advertiseInfo, advertiseInfoSize);      
//   }
//
//   mAdvertiseInfoSize = advertiseInfoSize;
//
//   mAdvertiseSocket = new BSocksUnreliableSocket(this);
//   HRESULT hr = mAdvertiseSocket->createAndBind(&advertiseAddress);
//   if (FAILED(hr))
//      return hr;
//
//   return S_OK;
//}
//
////==============================================================================
////    
//HRESULT BSessionConnector::advertisePort(unsigned short bindPort, unsigned short advertisePort)
//{          
//   SOCKADDR_IN addr = BSocksHelper::getLocalIP();
//   return advertise(addr, addr, &advertisePort, sizeof(advertisePort), 0, bindPort);
//}
//   
////==============================================================================
////    
//HRESULT BSessionConnector::find(const SOCKADDR_IN &remoteAddress, DWORD findInterval)
//{  
//   mFindInterval = findInterval;
//   mFoundTimeout = mFindInterval+2000;
//
//   //nlog(cSessionConnAddrNL, "BSessionConnector[%p]::find at %s:%ld", this, inet_ntoa(remoteAddress.sin_addr), ntohs(remoteAddress.sin_port));
//
//   mFindAddress = remoteAddress;   
//   if (mFindAddress.sin_port == 0)
//      mFindAddress.sin_port = ntohs(cAdvertisePort);
//   
//   if (mFindSocket)
//      mFindSocket->dispose();
//
//   mFindSocket = new BSocksUnreliableSocket(this);
//   HRESULT hr = mFindSocket->connect(&mFindAddress, 0);
//   if (FAILED(hr))
//      return hr;
//
//   mFindTimer = 0;
//
//   return S_OK;
//}
//
////==============================================================================
//// 
//HRESULT BSessionConnector::join(const long index, const SOCKADDR_IN &localAddress, 
//                                const SOCKADDR_IN &translatedLocalAddress, DWORD localCrc, XUID xuid)
//{
//   BSessionDescriptor *descriptor = getSessionDescriptor(index);
//   if (!descriptor)
//      return E_FAIL;
//
//  
//   if (!mFindSocket)
//      return E_FAIL; // Call find first
//
//   mXuid = xuid;
//   mLocalCRC = localCrc;
//   mLocalAddress = localAddress;
//   mTranslatedLocalAddress = translatedLocalAddress;
//   descriptor->mState = BSessionDescriptor::cJoining;   
//   descriptor->mJoinTime = timeGetTime();
//
//   return S_OK;
//}
//
////==============================================================================
//// 
//HRESULT BSessionConnector::findIP(const SOCKADDR_IN &remoteAddress, DWORD findInterval)
//{
//   for(long i=0; i<mFoundSessions.getNumber(); i++)
//      delete mFoundSessions[i];
//   mFoundSessions.clear();
//   return find(remoteAddress, findInterval);
//}
//
////==============================================================================
//// 
//HRESULT BSessionConnector::joinIP(const long index, const SOCKADDR_IN &localAddress, 
//                                const SOCKADDR_IN &translatedLocalAddress, DWORD localCrc, XUID xuid)
//{
//   BSessionDescriptor *descriptor = getSessionDescriptor(index);
//   if (!descriptor)
//      return E_FAIL;
//
//  
//   if (!mFindSocket)
//      return E_FAIL; // Call find first
//
//   mXuid = xuid;;
//   mLocalCRC = localCrc;
//   mLocalAddress = localAddress;
//   mTranslatedLocalAddress = translatedLocalAddress;
//   descriptor->mState = BSessionDescriptor::cJoiningIP;   
//   descriptor->mJoinTime = timeGetTime();
//
//   return S_OK;
//}
//
////==============================================================================
//// 
//// Ok - this is a horrible hack needed by the MPLiveConnector
////  What it does is dump the session descriptor list, add in just ONE entry, and pass back a handle to that entry
////  Then the caller can stuff into that structure the connection data from Live
//BSessionDescriptor* BSessionConnector::sessionListHack(const SOCKADDR_IN &advertiseAddress, const SOCKADDR_IN &remoteAddress, const SOCKADDR_IN &translatedRemoteAddress,
//                                                       void *advertiseInfo, DWORD advertiseInfoSize)
//{
//    for(long i=0; i<mFoundSessions.getNumber(); i++)
//        delete mFoundSessions[i];
//    mFoundSessions.clear();
//    BSessionDescriptor* pDesc=new BSessionDescriptor(advertiseAddress, remoteAddress, translatedRemoteAddress, advertiseInfo, advertiseInfoSize, timeGetTime());
//    mFoundSessions.add(pDesc);
//    return pDesc;
//}
//
//
//
////==============================================================================
//// 
//void BSessionConnector::recvd (
//         IN BSocket * Socket,
//         IN const void * Buffer,
//         IN DWORD Length,
//         IN CONST SOCKADDR_IN * RemoteAddress)
//{
//   Socket;Buffer;Length;RemoteAddress;
//
//   if (Socket == mAdvertiseSocket)
//   {
//      // nlog(cSessionConnAddrNL, "BSessionConnector[%p]::recvd %ld on mAdvertiseSocket", this, BTypedPacket::getType(Buffer));
//      if (BTypedPacket::getType(Buffer) == BPacketType::cFindPacket)
//      {         
//         if (mLocalAddress.sin_family == AF_INET) // this means we're advertising
//         {  
//            nlog(cSessionConnAddrNL, "  sending a find reply to %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));
//            // send out a find reply
//            BSessionConnectorPacket spacket(BPacketType::cAdvertisePacket, &mLocalAddress, &mTranslatedLocalAddress, 1, mLocalCRC, 0, sEmptySimString, 0, mAdvertiseInfo, mAdvertiseInfoSize);
//            HRESULT hr = Socket->sendPacketTo(spacket, RemoteAddress);
//            if (FAILED(hr))
//               nlog(cSessionConnAddrNL, "SessionConnector::recvd -- failed to send find packet to %s", inet_ntoa(RemoteAddress->sin_addr));
//         }
//      }
//      else if (BTypedPacket::getType(Buffer) == BPacketType::cJoinPacket)
//      {
//         BSessionConnectorPacket packet(BPacketType::cJoinPacket);
//         packet.deserializeFrom(Buffer, Length);      
//         nlog(cSessionConnJoinNL, "join packet from %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));
//
//         if (mLocalAddress.sin_family == AF_INET) // this means we're advertising
//         {
//            for (long i=0;i<mJoinedAddresses.getNumber();i++)
//            {
//               if (memcmp(&mJoinedAddresses[i], RemoteAddress, sizeof(SOCKADDR_IN)) == 0)
//               {
//                  nlog(cSessionConnJoinNL, " already have join packet from him. ");
//                  break; // we already got a join request from this guy
//               }
//            }
//
//            BSCObserver::eJoinResult result = BSCObserver::cJoinOK;
//
//            if (mObserver /*&& (i >= mJoinedAddresses.getNumber())*/)
//            {
//               if ((packet.mCRC != mLocalCRC)
//#ifndef BUILD_FINAL
//                  && (!gConfig.isDefined(cConfigIgnoreMPChecksum))
//#endif
//                  )
//               {
//                  nlog(cSessionConnJoinNL, " his CRC[%x] doesn't match local CRC[%x]. ", packet.mCRC, mLocalCRC);
//                  result = BSCObserver::cJoinCRCMismatch;
//               }
//               else
//               {
//                  mObserver->joinRequest(this, packet.mXuid, packet.mGamertag, packet.mCRC, packet.mAddresses[0], packet.mTranslatedAddresses[0], result);
//                  nlog(cSessionConnJoinNL, " joinRequest observer called. Result: %d", result);
//               }
//               mJoinedAddresses.add(*RemoteAddress);
//            }
//
//            // send out a join reply
//            BSessionConnectorPacket spacket(BPacketType::cJoinResponsePacket, &mLocalAddress, &mTranslatedLocalAddress, 1, mLocalCRC, packet.mXuid, packet.mGamertag, (long)result);
//            HRESULT hr = Socket->sendPacketTo(spacket, RemoteAddress);
//            if (FAILED(hr))
//               nlog(cSessionConnJoinNL, "Failed to send packet to address %s", inet_ntoa(RemoteAddress->sin_addr));
//         }
//      }   
//      else if (BTypedPacket::getType(Buffer) == BPacketType::cJoinIPPacket)
//      {
//         BSessionConnectorPacket packet(BPacketType::cJoinIPPacket);
//         packet.deserializeFrom(Buffer, Length);
//         nlog(cSessionConnJoinNL, "join IP packet from %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));
//
//         if (mLocalAddress.sin_family == AF_INET) // this means we're advertising
//         {    
//            BSCObserver::eJoinResult result = BSCObserver::cJoinOK;
//            if (mObserver)
//            {
//               if ((packet.mCRC != mLocalCRC)
//#ifndef BUILD_FINAL
//                  && (!gConfig.isDefined(cConfigIgnoreMPChecksum))
//#endif
//                  )
//               {
//                  nlog(cSessionConnJoinNL, " his CRC[%x] doesn't match local CRC[%x]. ", packet.mCRC, mLocalCRC);
//                  result = BSCObserver::cJoinCRCMismatch;
//               }
//               else
//               {
//                  mObserver->joinRequest(this, packet.mXuid, packet.mGamertag, packet.mCRC, packet.mAddresses[0], packet.mTranslatedAddresses[0], result);
//                  nlog(cSessionConnJoinNL, " joinRequest observer called. Result: %d", result);
//               }
//            }
//
//            // send out a join reply
//            BSessionConnectorPacket spacket(BPacketType::cJoinIPResponsePacket, &mLocalAddress, &mTranslatedLocalAddress, 1, mLocalCRC, packet.mXuid, packet.mGamertag, (long)result);
//            HRESULT hr = Socket->sendPacketTo(spacket, RemoteAddress);
//            if (FAILED(hr))
//               nlog(cSessionConnJoinNL, "Failed to send packet to address %s",inet_ntoa(RemoteAddress->sin_addr));
//         }
//      }
//   }
//   else if (Socket == mFindSocket)
//   {
//      // nlog(cSessionConnAddrNL, "BSessionConnector[%p]::recvd %ld on mFindSocket", this, BTypedPacket::getType(Buffer));
//      if (BTypedPacket::getType(Buffer) == BPacketType::cAdvertisePacket)
//      {
//         BSessionConnectorPacket packet(BPacketType::cAdvertisePacket);
//         packet.deserializeFrom(Buffer, Length);      
//
//         // nlog(cSessionConnAddrNL, "  got a find reply from %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));
//
//         // have we already found this session?
//         for (long i=0;i<mFoundSessions.getNumber();i++)
//         {
//            if (memcmp(&mFoundSessions[i]->mAdvertiseAddress, RemoteAddress, sizeof(SOCKADDR_IN)) == 0)
//            {
//               mFoundSessions[i]->mFoundTime = timeGetTime();
//               // nlog(cSessionConnAddrNL, "    already have it");
//               return; // we've already found this session
//            }
//         }
//
//         // if not, add it and notify
//         BSessionDescriptor* pDesc=new BSessionDescriptor(*RemoteAddress, packet.mAddresses[0], packet.mTranslatedAddresses[0], packet.mData, packet.mSize, timeGetTime());
//         mFoundSessions.add(pDesc);
//         // nlog(cSessionConnAddrNL, "    added it");
//
//         if (mObserver)
//            mObserver->findListUpdated(this);
//      }
//      else if (BTypedPacket::getType(Buffer) == BPacketType::cJoinResponsePacket)
//      {
//         BSessionConnectorPacket packet(BPacketType::cJoinResponsePacket);
//         packet.deserializeFrom(Buffer, Length);
//         nlog(cSessionConnJoinNL, " join Response from: %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));
//
//         for (long i=0;i<mFoundSessions.getNumber();i++)
//         {
//            if (
//                  (memcmp(&mFoundSessions[i]->mRemoteAddress, &packet.mAddresses[0], sizeof(SOCKADDR_IN)) == 0) &&
//                  (memcmp(&mFoundSessions[i]->mTranslatedRemoteAddress, &packet.mTranslatedAddresses[0], sizeof(SOCKADDR_IN)) == 0) &&
//                  (mFoundSessions[i]->mState == BSessionDescriptor::cJoining)
//               )
//            {
//               nlog(cSessionConnJoinNL, " found join request");
//               mFoundSessions[i]->mState = BSessionDescriptor::cJoined;
//               // we found a matching session               
//               if (mObserver)
//               {
//                  nlog(cSessionConnJoinNL, " notifying observer. Result: %d", packet.mResult);
//                  mObserver->joinReply(this, i, (BSCObserver::eJoinResult)packet.mResult);
//               }
//               return;
//            }
//         }
//         nlog(cSessionConnJoinNL, " join request not found");
//      }
//      else if (BTypedPacket::getType(Buffer) == BPacketType::cJoinIPResponsePacket)
//      {
//         BSessionConnectorPacket packet(BPacketType::cJoinIPResponsePacket);
//         packet.deserializeFrom(Buffer, Length);
//         nlog(cSessionConnJoinNL, " join IP Response from: %s:%ld", inet_ntoa(RemoteAddress->sin_addr), ntohs(RemoteAddress->sin_port));
//
//         for (long i=0;i<mFoundSessions.getNumber();i++)
//         {
//            if (
//                  (memcmp(&mFoundSessions[i]->mRemoteAddress, &packet.mAddresses[0], sizeof(SOCKADDR_IN)) == 0) &&
//                  (memcmp(&mFoundSessions[i]->mTranslatedRemoteAddress, &packet.mTranslatedAddresses[0], sizeof(SOCKADDR_IN)) == 0) &&
//                  (mFoundSessions[i]->mState == BSessionDescriptor::cJoiningIP)
//               )
//            {
//               nlog(cSessionConnJoinNL, " found join request");
//               mFoundSessions[i]->mState = BSessionDescriptor::cJoined;
//               // we found a matching session               
//               if (mObserver)
//               {
//                  nlog(cSessionConnJoinNL, " notifying observer.");
//                  mObserver->joinReply(this, i, (BSCObserver::eJoinResult)packet.mResult);
//               }
//               return;
//            }
//         }
//         nlog(cSessionConnJoinNL, " join request not found");
//      }
//   }      
//}
//
////==============================================================================
//// 
//void BSessionConnector::interfaceAddressChanged()
//{
//   if (mObserver)
//      mObserver->interfaceAddressChanged();
//}
//
////==============================================================================
//// 
//void BSessionConnector::service(void)
//{
//   // time to send a find packet?
//   if (mFindAddress.sin_family == AF_INET)
//   {
//      if (timeGetTime() - mFindTimer > mFindInterval)
//      {
//         if (mFindSocket)
//         {
//            BSessionConnectorPacket packet(BPacketType::cFindPacket);
//            HRESULT hr = mFindSocket->sendPacket(packet);
//            if (FAILED(hr))
//               nlog(cSessionConnJoinNL, "BSessionConnector::service -- failed to send packet to broadcast address hr: %d", hr);
//         }
//         mFindTimer = timeGetTime();
//      }
//   }
//   
//   for (long i=0;i<mFoundSessions.getNumber();i++)
//   {
//      // any joining sessions we need to resend to?
//      if (  (mFoundSessions[i]->mState == BSessionDescriptor::cJoining) &&
//            (timeGetTime() - mFoundSessions[i]->mJoinTime > cJoinInterval) )
//      {
//         nlog(cSessionConnJoinNL, "Sending join request to %s:%ld", inet_ntoa(mFoundSessions[i]->mAdvertiseAddress.sin_addr), ntohs(mFoundSessions[i]->mAdvertiseAddress.sin_port));
//         BSessionConnectorPacket packet(BPacketType::cJoinPacket, &mLocalAddress, &mTranslatedLocalAddress, 1, mLocalCRC, mXuid, mGamertag);
//         HRESULT hr = mFindSocket->sendPacketTo(packet, &mFoundSessions[i]->mAdvertiseAddress);
//         if (FAILED(hr))
//         {
//            nlog(cSessionConnJoinNL, "Send join request failed.");
//            BSessionDescriptor* pDesc=mFoundSessions[i];
//            mFoundSessions.remove(pDesc);
//            delete pDesc;
//            return;
//         }
//         mFoundSessions[i]->mJoinTime = timeGetTime();
//      }      
//      else if (  (mFoundSessions[i]->mState == BSessionDescriptor::cJoiningIP) &&
//            (timeGetTime() - mFoundSessions[i]->mJoinTime > cJoinInterval) )
//      {
//         nlog(cSessionConnJoinNL, "Sending join IP request to %s:%ld", inet_ntoa(mFoundSessions[i]->mAdvertiseAddress.sin_addr), ntohs(mFoundSessions[i]->mAdvertiseAddress.sin_port));
//         BSessionConnectorPacket packet(BPacketType::cJoinIPPacket, &mLocalAddress, &mTranslatedLocalAddress, 1, mLocalCRC, mXuid, mGamertag);
//         HRESULT hr = mFindSocket->sendPacketTo(packet, &mFoundSessions[i]->mAdvertiseAddress);
//         if (FAILED(hr))
//         {
//            nlog(cSessionConnJoinNL, "Send join IP request failed.");
//            BSessionDescriptor* pDesc=mFoundSessions[i];
//            mFoundSessions.remove(pDesc);
//            delete pDesc;
//            return;
//         }
//         mFoundSessions[i]->mJoinTime = timeGetTime();
//      }
//      // check for sessions timing out
//      else if (
//               (mFoundSessions[i]->mState == BSessionDescriptor::cFound) &&
//               (timeGetTime() - mFoundSessions[i]->mFoundTime > mFoundTimeout)
//              )
//      {
//         BSessionDescriptor* pDesc=mFoundSessions[i];
//         mFoundSessions.remove(pDesc);
//         delete pDesc;
//
//         if (mObserver)
//            mObserver->findListUpdated(this);
//
//         break;
//      }
//   }
//}
//
////==============================================================================
//// eof: SessionConnector.cpp
////==============================================================================
