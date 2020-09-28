//==============================================================================
// AddressGrabber.h
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _AddressGrabber_H_
#define _AddressGrabber_H_

//==============================================================================
// Includes

#include "SocksUnreliableSocket.h"
#include "SocksGameSocket.h"

//==============================================================================
// Forward declarations
class BSocket;

//==============================================================================
// Const declarations


//==============================================================================
class BAddressGrabber : public BSocksUnreliableSocketUser
{
   public:
      
      class BAddressGrabberObserver
      {
         public:
            virtual void addressGrabbed(HRESULT errCode, SOCKADDR_IN *translatedLocalAddress, long serverPing) = 0;
      };

      enum { cForwardingServerPort = 2300 };

      // Constructors
      BAddressGrabber( BAddressGrabberObserver *observer );

      // Destructors
      ~BAddressGrabber( void );

      // Functions

      enum { cDefaultTimeout = 10000 };
      HRESULT grabLocalAddress( const SOCKADDR_IN &localAddress, SOCKADDR_IN *server1Address, SOCKADDR_IN *server2Address=0, const unsigned short ExternalPort = 0, const DWORD timeout=cDefaultTimeout);
      void stopGrabbingLocalAddress( void );
      bool hasGrabbed(void) { return (mState==cGrabbed?true:false); }
      bool hasFailed(void) { return (mState==cFailed?true:false); }
      bool isGrabbing(void) { return ((mState==cGrabbing1||mState==cGrabbing2)?true:false); } // this class sounds so perverted
      long getServerPing(void) { return mServerPing; }

      void service(void);

      // BSocksUnreliableSocketUser
      virtual bool recvd (
               IN BSocket * Socket,
               IN const void * Buffer,
               IN DWORD Length,
               IN CONST SOCKADDR_IN * RemoteAddress);

      // Variables

   private:      

      // Variables

      BAddressGrabberObserver    *mObserver;
      enum { cGrabbing1, cGrabbing2, cGrabbed, cFailed, cIdle };
      long                       mState;
      enum { cSendInterval = 1000 };
      DWORD                      mSendTimer;
      long                       mServerPing;
      SOCKADDR_IN                mServer1Address;
      SOCKADDR_IN                mServer2Address;
      SOCKADDR_IN                mTranslatedLocalAddress;
      DWORD                      mStartGrabbingTime;
      DWORD                      mTimeout;      
      unsigned short             mExternalPort;
}; // BAddressGrabber


//==============================================================================
#endif // _AddressGrabber_H_

//==============================================================================
// eof: AddressGrabber.h
//==============================================================================
