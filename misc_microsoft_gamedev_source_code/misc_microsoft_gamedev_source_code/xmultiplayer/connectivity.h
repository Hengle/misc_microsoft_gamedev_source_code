//==============================================================================
// connectivity.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once

#ifndef __CONNECTIVITY_H__
#define __CONNECTIVITY_H__

//==============================================================================
// Includes
#include "Socket.h"
#include "bitvector.h"
#include "SessionConnector.h"
#include "AddressGrabber.h"
#include "SocksHelper.h"
#include "ObserverList.h"

//==============================================================================
// Forward declarations
class BSessionConnector;

//==============================================================================
// Const declarations

class BConnectivityObserver
{
public:
   enum
   {
      cStatePortFinding,
      cStateAllocatingPort,
      cStateGettingLocalAddress,
      cStateContactAddressServer,
      cStateComplete,
      cStateFailed,
      cStateFailedAddressGrabber
   };

   virtual void connectivityState(long state, HRESULT hr=S_OK) {state; hr;}
   virtual void addressResult(long state) {state;}
   virtual void networkDisabled() {}


};

//==============================================================================
class BConnectivity : public BSessionConnector::BSCObserver,
                      public BAddressGrabber::BAddressGrabberObserver,
                      public BSocksHelper::BAddressNotify,
                      public BSocket::BObserver
{
public:

   virtual ~BConnectivity();

   enum
   {
      cFlagDirectIP,
      cFlagDisableUPNP,
      cFlagDisablePortFinding,
      cFlagDisableNetworking,
      cFlagInterfaceQuery,

      cFlagTotal
   };

   enum
   {
      cErrNone,
      cErrFailedLocalGrab,
      cErrAddressServerGrabFailed,

      cErrTotal
   };

   bool                    isNetworkingEnabled(void) const { return(!mFlags.isSet(cFlagDisableNetworking)); }
   DWORD                   service(void);
   
   void                    setFlag(long index);
   bool                    isFlagSet(long index);
   void                    clearFlag(long index);

   const SOCKADDR_IN&      getLocalAddress(void)            { return mLocalAddress; }
   const SOCKADDR_IN&      getTranslatedLocalAddress(void)  { return mTranslatedLocalAddress; }
   long                    getAddressServerPing(void) const { return mAddressServerPing; }

   void                    addObserver(BConnectivityObserver* observer);
   void                    removeObserver(BConnectivityObserver* observer);
   HRESULT                 setupConnectivity(BSimString* addr1 = NULL, BSimString* addr2 = NULL);

   // BSessionConnector::BSCObserver
   virtual void            findListUpdated(BSessionConnector *connector);
   virtual void            joinRequest(BSessionConnector *, const BSimString&, DWORD, const SOCKADDR_IN &, const SOCKADDR_IN &, eJoinResult&) {}
   virtual void            joinReply(BSessionConnector *, long, eJoinResult) {}
   virtual void            interfaceAddressChanged();

   // BAddressGrabber::BAddressGrabberObserver
   virtual void            addressGrabbed(HRESULT errCode, SOCKADDR_IN *translatedLocalAddress, long serverPing);

   virtual void            addressResult(long addressOut);

   static BConnectivity *getInstance(void);
   static void destroyInstance(void);

protected:
   class BConnectivityObserverList :
      public BObserverList <BConnectivityObserver>
   {
      DECLARE_OBSERVER_METHOD (connectivityState, (long state, HRESULT hr=S_OK), (state, hr))
      DECLARE_OBSERVER_METHOD (addressResult, (long state), (state))
      DECLARE_OBSERVER_METHOD (networkDisabled, (),())
   };

   BConnectivity();

   void                    setupPortLocking(void);
   void                    buildPortList(void);
   void                    allocatePorts(void);
   void                    setupAddresses(void);
   void                    getAddress(void);
   void                    setCurrentState(long state, HRESULT hr=S_OK);
   DWORD                   testAddressChange();

   enum
   {
      cStateInvalid,
      cStateStartConnectivity,
      cStateSetupPortFinding,
      cStatePFStartFindingLocks,
      cStatePFWaitForLock,
      cStatePFFoundLock,
      cStatePFNoLock,
      cStateAllocatePorts,
      cStateSetupAddresses,
      cStateWaitingOnResolve,
      cStateGetLocalAddress,
      cStateContactAddressServer1,
      cStateResolveAddressServer2,
      cStateContactAddressServer2,
      cStateStartAddressGrabber,
      cStateGrabbingAddress,
      cStateComplete,
      cStateFailed
   };

   enum { cAddressGrabberTimeout = 20000 };
   BSocksUnreliableSocket  *mAddressSocket;
   BConnectivityObserverList  mObserverList;
   BAddressGrabber         *mAddressGrabber;
   long                    mAddressServerPing;

   long                    mCurrentState;
   long                    mInternalPort;
   long                    mExternalPort;
   long                    mAddressGrabFailures;
   
   enum { cNone, cSetupLAN };
   long                    mSetupConnectivity;       
   BSessionConnector       *mPortFinder;     
   BSessionConnector       *mPortLocker;
   DWORD                   mStartTime;
   DWORD                   mWaitTime;
   DWORD                   mAddressGrabberTimer;
   DWORD                   mdwAddressChangeTime;

   HRESULT                 mLastError;

   BSimString                 mNetAddressServer1;
   BSimString                 mNetAddressServer2;

   SOCKADDR_IN             mFirstAddress;
   SOCKADDR_IN             mRoutingAddress;
   SOCKADDR_IN             mLocalAddress;
   SOCKADDR_IN             mTranslatedLocalAddress;
   SOCKADDR_IN             mAddressServer1Address;
   SOCKADDR_IN             mAddressServer2Address;
   
   BBitVector              mFlags;

   unsigned short          mLocalLock;
   bool                    mFoundLock;
};


#endif // __CONNECTIVITY_H__