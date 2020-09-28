//==============================================================================
// AddressGrabber.cpp
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "AddressGrabber.h"
#include "serialbuffer.h"
#include "sockshelper.h"
#include "SocksMapper.h"

//==============================================================================
// Defines

//==============================================================================
// BAddressGrabber::BAddressGrabber
//==============================================================================
BAddressGrabber::BAddressGrabber(BAddressGrabberObserver *observer) :
   mObserver(observer),
   mSendTimer(0),
   mServerPing(0),
   mState(cIdle),
   mStartGrabbingTime(0),
   mTimeout(cDefaultTimeout),
   mExternalPort(0)
{
   memset(&mServer1Address, 0, sizeof(mServer1Address));
   memset(&mServer2Address, 0, sizeof(mServer2Address));
   memset(&mTranslatedLocalAddress, 0, sizeof(mTranslatedLocalAddress));
} // BAddressGrabber::BAddressGrabber

//==============================================================================
// BAddressGrabber::~BAddressGrabber
//==============================================================================
BAddressGrabber::~BAddressGrabber(void)
{   
} // BAddressGrabber::~BAddressGrabber

//==============================================================================
// 
bool BAddressGrabber::recvd (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN CONST SOCKADDR_IN * RemoteAddress) 
{ 
   Socket;Buffer;Length; 

   if (RemoteAddress && 
      (RemoteAddress->sin_addr.S_un.S_addr != mServer1Address.sin_addr.S_un.S_addr) &&
       (RemoteAddress->sin_addr.S_un.S_addr != mServer2Address.sin_addr.S_un.S_addr))
      return false;

   if (getSocket() != Socket)
      return false;

   if (!isGrabbing())
      return false;

   BYTE header = *((BYTE *)Buffer);

   if (header == BSocksGameHeaderType::cAddressRequest1Packet)
   {
      nlog(cTransportNL, "BAddressGrabber recvd cAddressRequest1Packet");
      SOCKADDR_IN *translatedLocalAddress = (SOCKADDR_IN *) (((char*)Buffer)+1);
      mTranslatedLocalAddress = *translatedLocalAddress; // FIXME: Check for buffer underrun?

      nlog(cTransportNL, "  external address %s:%ld", inet_ntoa(mTranslatedLocalAddress.sin_addr), mTranslatedLocalAddress.sin_port);
      if (mExternalPort != 0)
      {
         mTranslatedLocalAddress.sin_port = mExternalPort; // override the inbound port if we're talking over a UPnP device      
         nlog(cTransportNL, "    overriding to port %ld", mTranslatedLocalAddress.sin_port);
      }

      BSerialBuffer sb( ((char *)Buffer)+sizeof(SOCKADDR_IN)+1, Length );
   
      uint32 sentTime=0;
      sb.get(&sentTime);

      mServerPing = timeGetTime()-sentTime;    

      // do we have a second server to talk to?
      if (mServer2Address.sin_family != AF_INET)
      {
         // if not, we're done
         mState = cGrabbed;
         if (mObserver && BSocksHelper::isValidExternalIP(mTranslatedLocalAddress))
            mObserver->addressGrabbed(S_OK, &mTranslatedLocalAddress, mServerPing);
         else
         {
            mState = cFailed;
            if (mObserver)
               mObserver->addressGrabbed(E_FAIL, 0, 0);
         }

         //stopGrabbingLocalAddress();
      }
      else
      {
         // otherwise, transition to the second state
         mState = cGrabbing2;
      }

      return true;
   }
   else if (header == BSocksGameHeaderType::cAddressRequest2Packet)
   {      
      nlog(cTransportNL, "BAddressGrabber recvd cAddressRequest2Packet");
      mState = cGrabbed;
      if (mObserver && BSocksHelper::isValidExternalIP(mTranslatedLocalAddress))
         mObserver->addressGrabbed(S_OK, &mTranslatedLocalAddress, mServerPing);
      else
      {
         nlog(cTransportNL, "  no observer or invalid external IP");
         stopGrabbingLocalAddress();
         mState = cFailed;
         if (mObserver)
            mObserver->addressGrabbed(E_FAIL, 0, 0);
      }

      return true;
   }

   return false;
}


//==============================================================================
// 
HRESULT BAddressGrabber::grabLocalAddress( const SOCKADDR_IN &localAddress, SOCKADDR_IN *server1Address, SOCKADDR_IN *server2Address, unsigned short externalPort, const DWORD timeout )
{   
   nlog(cTransportNL, "BAddressGrabber::grabLocalAddress externalPort %ld", externalPort);

   mState = cFailed; // in case this func errors out - otherwise it gets set to cGrabbing at the bottom
   
   mStartGrabbingTime = 0;

   if (!server1Address)
      return E_FAIL;   

   BSocksMapper *mapper = BSocksMapper::getInstance();
   HRESULT hr = mapper->connectSocket(this, server1Address, &localAddress);
   if (hr != S_OK)
      return hr;
   
   nlog(cTransportNL, "BAddressGrabber::grabLocalAddress --  SocksMapper connectSocket [%p]", getSocket());

   if (!getSocket())
      return E_FAIL;

   // stuff the latest server address   
   mServer1Address = *server1Address;

   // do we have a second server address? If so, stuff that one too
   if (server2Address)
      mServer2Address = *server2Address;
   
   mSendTimer = 0;   
   memset(&mTranslatedLocalAddress, 0, sizeof(mTranslatedLocalAddress));

   mTimeout = timeout;
   mStartGrabbingTime = timeGetTime();
   mState = cGrabbing1;

   if (externalPort != (unsigned short)-1)
      mExternalPort = htons(externalPort);

   return S_OK;
}

//==============================================================================
// 
void BAddressGrabber::stopGrabbingLocalAddress( void )
{
   BSocksMapper::getInstance()->disconnectSocket(this);

   mState = cIdle;
   mStartGrabbingTime = 0;
   mSendTimer = 0;   
   mTimeout = cDefaultTimeout;
}

//==============================================================================
// 
void BAddressGrabber::service(void)
{
   if ((mState == cIdle) || (mState == cGrabbed) || (mState == cFailed) || !getSocket())
      return;

   if (mStartGrabbingTime && (timeGetTime() - mStartGrabbingTime > mTimeout))
   {
      stopGrabbingLocalAddress();
      mState = cFailed;
      mObserver->addressGrabbed(HRESULT_FROM_WIN32(ERROR_TIMEOUT), 0, 0); // timed out
      return;
   }

   if ((timeGetTime() - mSendTimer) > cSendInterval)
   {
      if (mState == cGrabbing1)
      {
         // send first address request packet
         BSerialBuffer sb;      
         uint32 now = timeGetTime();      
         sb.add(now);

         BSendBuffer *buf=0;
         if (FAILED(getSocket()->sendAllocateBuffer(sb.getBufferSize()+1, &buf)))
         {
            stopGrabbingLocalAddress();
            mState = cFailed;
            mObserver->addressGrabbed(E_FAIL, 0, 0);
            return;
         }
      
         BYTE header = BSocksGameHeaderType::cAddressRequest1Packet;
         memcpy(buf->Buffer, &header, 1);
      
         memcpy(((char *)buf->Buffer)+1, sb.getBuffer(), sb.getBufferSize());    
         buf->Length = sb.getBufferSize()+1;

         mSendTimer = timeGetTime();
      
         if (FAILED(getSocket()->sendTo(buf, &mServer1Address)))
         {
            stopGrabbingLocalAddress();
            mState = cFailed;
            mObserver->addressGrabbed(E_FAIL, 0, 0);
            return;
         }     
      }
      else if (mState == cGrabbing2)
      {
         // send second address request packet         
         BSendBuffer *buf=0;
         if (FAILED(getSocket()->sendAllocateBuffer(1+sizeof(SOCKADDR_IN), &buf)))
         {
            stopGrabbingLocalAddress();
            mState = cFailed;
            mObserver->addressGrabbed(E_FAIL, 0, 0);
            return;
         }
      
         BYTE header = BSocksGameHeaderType::cAddressRequest2Packet;        

         memcpy(buf->Buffer, &header, 1);
         memcpy(((char *)buf->Buffer)+1, &mTranslatedLocalAddress, sizeof(SOCKADDR_IN));
               
         buf->Length = 1+sizeof(SOCKADDR_IN);

         mSendTimer = timeGetTime();
      
         if (FAILED(getSocket()->sendTo(buf, &mServer1Address)))
         {
            stopGrabbingLocalAddress();
            mState = cFailed;
            mObserver->addressGrabbed(E_FAIL, 0, 0);
            return;
         }      
      }
   }
}

//==============================================================================
// BAddressGrabber::
//==============================================================================



//==============================================================================
// eof: AddressGrabber.cpp
//==============================================================================
