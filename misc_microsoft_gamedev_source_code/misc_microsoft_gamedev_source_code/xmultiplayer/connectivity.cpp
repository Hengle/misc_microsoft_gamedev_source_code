//==============================================================================
// connectivity.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "connectivity.h"

#ifndef XBOX
#include "UPnPHelper.h"
#endif

#include "session.h"

#include "SessionConnector.h"
#include "SocksHelper.h"
#include "AddressGrabber.h"
#include "commlog.h"
#include "commheaders.h"

//==============================================================================
// Defines
const DWORD cMaxLockSearchTime = 60000;
const DWORD cMaxPortWaitTime = 2000;
const DWORD cLockRetryTime = 500;
const DWORD cMaxAddressFailures = 20;
const DWORD cAddressFailureRetry = 5000;

const int cPortsToTry = 10;
static BDynamicSimArray<long> sPorts;

static BConnectivity *sConnectivity = NULL;

//==============================================================================
// BConnectivity::
//==============================================================================
BConnectivity* BConnectivity::getInstance(void)
{
   if (sConnectivity != NULL)
      return sConnectivity;
   else
   {
      sConnectivity = new BConnectivity();      
      return sConnectivity;
   }
}

//==============================================================================
// BConnectivity::
//==============================================================================
void BConnectivity::destroyInstance(void)
{
   if (sConnectivity != NULL)
   {
      delete sConnectivity;
   }
   sConnectivity = NULL;
}

//==============================================================================
// BConnectivity::BConnectivity
//==============================================================================
BConnectivity::BConnectivity() : 
   mAddressGrabber(NULL),
   mAddressServerPing(0),
   mInternalPort(-1),
   mExternalPort(-1),
   mSetupConnectivity(cNone),
   mPortFinder(NULL),
   mPortLocker(NULL),
   mAddressGrabberTimer(0),
   mdwAddressChangeTime(0),
   mCurrentState(cStateInvalid),
   mStartTime(0),
   mWaitTime(0),
   mLastError(S_OK),
   mAddressGrabFailures(0),
   mAddressSocket(NULL)
{
   memset(&mFirstAddress, 0, sizeof(mFirstAddress));
   memset(&mRoutingAddress, 0, sizeof(mRoutingAddress));
   memset(&mLocalAddress, 0, sizeof(mLocalAddress));
   memset(&mTranslatedLocalAddress, 0, sizeof(mTranslatedLocalAddress));
   memset(&mAddressServer1Address, 0, sizeof(mAddressServer1Address));
   memset(&mAddressServer2Address, 0, sizeof(mAddressServer2Address));

   //The connectivity layer needs the network to be running, so lets start/hold a ref to it
   if(!networkStartup())
   {
       nlog(cConnectivityNL, " BConnectivity::getInstance Failed to initialize WinSock");
   }

}

//==============================================================================
// BConnectivity::~BConnectivity
//==============================================================================
BConnectivity::~BConnectivity()
{
#ifndef XBOX
   BUPnPHelper::destroyInstance(); // in case we have one active
#endif   

   if (mAddressGrabber)
   {
      delete mAddressGrabber;
      mAddressGrabber = NULL;
   }

   if (mPortFinder)
   {
      delete mPortFinder;
      mPortFinder = NULL;
   }

   if (mPortLocker)
   {
      delete mPortLocker;
      mPortLocker = NULL;
   }

   if (mAddressSocket)
   {
      mAddressSocket->dispose();
      mAddressSocket = NULL;
   }

   networkCleanup();
}

//==============================================================================
// BConnectivity::service
//==============================================================================
DWORD BConnectivity::service(void)
{
   // are all these state changes re-entrant or do we need to perform
   // cleanup if one of them fails and the process is restarted?
   switch (mCurrentState)
   {
      case cStateStartConnectivity:
      {
         // remove the disable flag so we can attempt again
         mFlags.unset(cFlagDisableNetworking);

         setCurrentState(cStateSetupPortFinding);
      }
      break;

      case cStateSetupPortFinding:
      {
         setupPortLocking();
      }
      break;

      case cStatePFStartFindingLocks:
      {
         if ((timeGetTime() - mStartTime) < cMaxLockSearchTime)
         {
            mWaitTime = timeGetTime();
            setCurrentState(cStatePFWaitForLock);
         }
         else
            setCurrentState(cStateFailed);
      }
      break;

      case cStatePFWaitForLock:
      {
         if ((timeGetTime() - mWaitTime) < cMaxPortWaitTime)
         {
            if (mPortFinder)
               mPortFinder->service();
            if (mPortLocker)
               mPortLocker->service();
         }
         else
         {
            setCurrentState(cStatePFNoLock);
         }
      }
      break;

      case cStatePFFoundLock:
      {
         if ((timeGetTime() - mWaitTime) > cLockRetryTime)
         {
            nlog(cConnectivityNL, "  found a lock, starting over");
            setCurrentState(cStatePFStartFindingLocks);
         }
      }
      break;

      case cStatePFNoLock:
      {
         buildPortList();
      }
      break;


      case cStateAllocatePorts:
      {
         allocatePorts();
      }
      break;

      case cStateSetupAddresses:
      {
         setupAddresses();
      }
      break;

      case cStateGetLocalAddress:
      {
         getAddress();
      }
      break;

      case cStateFailed:
      {
         // we need to remove this flag before attempting
         // to setup connectivity again
         mFlags.set(cFlagDisableNetworking);
         mObserverList.networkDisabled();
         setCurrentState(cStateInvalid);
      }
      break;

      case cStateResolveAddressServer2:
      {
         // start resolving the second address server
         HRESULT hr = BSocksHelper::socksGetAddress(mNetAddressServer2.getPtr(), this);
         if (FAILED(hr))
            nlog(cConnectivityNL, "Failed to get address server 2 IP address");
         mCurrentState = cStateContactAddressServer2;
      }
      break;

      case cStateStartAddressGrabber:
      {
         nlog(cConnectivityNL, "Grabbing address from %s", inet_ntoa(mAddressServer1Address.sin_addr));
         if (mAddressServer2Address.sin_family == AF_INET)
            nlog(cConnectivityNL, "Grabbing address 2 from %s", inet_ntoa(mAddressServer2Address.sin_addr));            

         SOCKADDR_IN localAddress;
         localAddress = mLocalAddress;
         localAddress.sin_port = ntohs(BSession::cDefaultHostPort);

         if (mInternalPort>0) // set by the UPnP stuff
            localAddress.sin_port = ntohs((unsigned short)mInternalPort);
         if (mAddressGrabber == NULL)
            mAddressGrabber = new BAddressGrabber(this);      

         SOCKADDR_IN *addressServer2Address = 0;
         if (mAddressServer2Address.sin_family == AF_INET)
            addressServer2Address = &mAddressServer2Address;

         if (FAILED(mAddressGrabber->grabLocalAddress(localAddress, &mAddressServer1Address, addressServer2Address, (unsigned short)mExternalPort, 30000)))
         {
            mObserverList.addressResult(BConnectivityObserver::cStateFailed);
            setCurrentState(cStateComplete);
            nlog(cConnectivityNL, "Failed to start grabbing address"); 
            break;
         }
         setCurrentState(cStateGrabbingAddress);
      }
      break;

      case cStateGrabbingAddress:
      {
         if (mAddressGrabber)
         {
            // are we grabbing the address?
            if (mAddressGrabber->isGrabbing())
            {
               mAddressGrabber->service();
            }
            else if (mAddressGrabber->hasFailed())
            {
               nlog(cConnectivityNL, "Timed out attempting to grab address."); 
               nlog(cConnectivityNL, "failed to grab address - I am:");
               nlog(cConnectivityNL, "  %s:%ld", inet_ntoa(mLocalAddress.sin_addr), htons(mLocalAddress.sin_port));
               nlog(cConnectivityNL, "  %s:%ld", inet_ntoa(mTranslatedLocalAddress.sin_addr), htons(mTranslatedLocalAddress.sin_port));

               mAddressGrabber->stopGrabbingLocalAddress();
               delete mAddressGrabber;
               mAddressGrabber = NULL;
               mObserverList.addressResult(BConnectivityObserver::cStateFailed);
               setCurrentState(cStateComplete);

               // return(cErrAddressServerGrabFailed); // we don't want an address server grab to disable our network.               
            }
            else if (mAddressGrabber->hasGrabbed())
            {
               mAddressGrabber->stopGrabbingLocalAddress();
               delete mAddressGrabber;
               mAddressGrabber = NULL;
               setCurrentState(cStateComplete);
            }
         }
      }
      break;
   }

   if (mPortFinder)
      mPortFinder->service();

   // if the connectivity is still being established or we're contacting the address server, don't do anything else.
   if (mCurrentState < cStateComplete)
      return (cErrNone);

   if ((timeGetTime() - mdwAddressChangeTime) > 30000)
   {
      // in the polling method we allow the address to fail for N number of times, then we bail. (not sure why we wait so long)
      DWORD failures = testAddressChange();
      if (failures == 0)
         mdwAddressChangeTime = timeGetTime();
      else if (failures < cMaxAddressFailures)
         mdwAddressChangeTime += cAddressFailureRetry;
      else
         return (cErrAddressServerGrabFailed);
   }

   return(cErrNone);
}

//==============================================================================
// BConnectivity::setFlag
//==============================================================================
void BConnectivity::setFlag(long index)
{
   if (index < 0 || index >= cFlagTotal)
      return;

   mFlags.set(index);
}

//==============================================================================
// BConnectivity::isFlagSet
//==============================================================================
bool BConnectivity::isFlagSet(long index)
{
   if (index < 0 || index >= cFlagTotal)
      return(false);

   return(mFlags.isSet(index));
}

//==============================================================================
// BConnectivity::clearFlag
//==============================================================================
void BConnectivity::clearFlag(long index)
{
   if (index < 0 || index >= cFlagTotal)
      return;

   mFlags.unset(index);
}

//==============================================================================
// BConnectivity::addObserver
//==============================================================================
void BConnectivity::addObserver(BConnectivityObserver* observer)
{
   if (observer != NULL)
      mObserverList.Add(observer);
}

//==============================================================================
// BConnectivity::removeObserver
//==============================================================================
void BConnectivity::removeObserver(BConnectivityObserver* observer)
{
   if (observer != NULL)
      mObserverList.Remove(observer);
}

//==============================================================================
// BConnectivity::setupConnectivity
//==============================================================================
HRESULT BConnectivity::setupConnectivity(BSimString* addr1 /*= NULL*/, BSimString* addr2 /*= NULL*/)
{
   HRESULT hr = S_OK;

   nlog(cConnectivityNL, "BConnectivity::setupConnectivity");

   if ((mSetupConnectivity == cSetupLAN))
   {

      nlog(cConnectivityNL, "  already setup");
      mObserverList.connectivityState(BConnectivityObserver::cStateComplete);
      return hr; // we only do this once per game
   }

   if (addr1 != NULL)
      mNetAddressServer1.set(*addr1);
   else
      mNetAddressServer1.empty();

   if (addr2 != NULL)
      mNetAddressServer2.set(*addr2);
   else
      mNetAddressServer2.empty();

      mSetupConnectivity = cSetupLAN;

      clearFlag(cFlagDirectIP);

   if (mPortFinder)
   {
      delete mPortFinder;
      mPortFinder = NULL;
   }
      
   // here's how this connectivity stuff works:

   // First the game uses the SessionConnector object to see if there are other games running on the local
   // subnet that are using the same port - if so, it picks the first available port and continues.

   // Next the game calls setupConnectivity, which tries a range of internal sPorts (other than the ones being used
   // as reported by the SessionConnector object), attempting to map one of them
   // to an external port. 
   setCurrentState(cStateStartConnectivity);
   
   return hr;   
}

//==============================================================================
// BConnectivity::findListUpdated
//==============================================================================
void BConnectivity::findListUpdated(BSessionConnector *connector)
{
   for (long i=0;i<connector->getSessionDescriptorAmount();i++)
   {
      if (*((unsigned short *)connector->getSessionDescriptor(i)->mAdvertiseInfo) > mLocalLock)
      {
         setCurrentState(cStatePFFoundLock);
         mWaitTime = timeGetTime();
         break;
      }
   }
}

//==============================================================================
// BConnectivity::interfaceAddressChanged
//==============================================================================
void BConnectivity::interfaceAddressChanged()
{
   // if we got a notification that the interface address changed, verify it and fail if it did indeed change
   if (testAddressChange() > 0)
   {
      {setBlogError(4180); blogerror("BConnectivity::interfaceAddressChanged -- Interface Address Changed.");}
      setFlag(cFlagDisableNetworking);
      mObserverList.connectivityState(BConnectivityObserver::cStateFailed, E_FAIL);
      mObserverList.networkDisabled();
   }
}

//==============================================================================
// BConnectivity::addressGrabbed
//==============================================================================
void BConnectivity::addressGrabbed(HRESULT errCode, SOCKADDR_IN *translatedLocalAddress, long serverPing)
{
   translatedLocalAddress;

   if (errCode == S_OK)
   {
      mTranslatedLocalAddress = *translatedLocalAddress;
      if (inet_ntoa(mTranslatedLocalAddress.sin_addr))
      {          
         nlog(cConnectivityNL, "Address grabbed %s:%ld", inet_ntoa(mTranslatedLocalAddress.sin_addr), htons(mTranslatedLocalAddress.sin_port));
      }
      else
      {            
         nlog(cConnectivityNL, "Address grabbed - BAD LOCAL ADDRESS!");
         BASSERT(0);         
      }
   }
   else
   {      
      nlog(cConnectivityNL, "Failed addressGrabbed");
   }

   nlog(cConnectivityNL, "AMPCommunications::addressGrabbed - I am:");
   nlog(cConnectivityNL, "  %s:%ld", inet_ntoa(mLocalAddress.sin_addr), htons(mLocalAddress.sin_port));
   nlog(cConnectivityNL, "  %s:%ld", inet_ntoa(mTranslatedLocalAddress.sin_addr), htons(mTranslatedLocalAddress.sin_port));

   mAddressServerPing = serverPing;
}

//==============================================================================
// BConnectivity::addressResult
//==============================================================================
void BConnectivity::addressResult(long addressOut)
{
   switch(mCurrentState)
   {
      // resolving ESO.COM
      case cStateWaitingOnResolve:
      {
         if (addressOut == -1)
         {      
            setCurrentState(cStateFailed, HRESULT_FROM_WIN32(ERROR_SERVER_DISABLED));
            nlog(cConnectivityNL, "BConnectivity::setupAddresses -- Failed to look up eso.com");
            return;
         }

         memset(&mRoutingAddress, 0, sizeof(mRoutingAddress));
         mRoutingAddress.sin_family = AF_INET;
         mRoutingAddress.sin_addr.S_un.S_addr = addressOut;
         mRoutingAddress.sin_port = ntohs(2300);
         setCurrentState(cStateGetLocalAddress);
      }
      break;

      // resolving first address server
      case cStateContactAddressServer1:
      {
         // if first server didn't fail
         if (addressOut != -1)
         {
            mAddressServer1Address.sin_family = AF_INET;
            mAddressServer1Address.sin_addr.S_un.S_addr = addressOut;
            mAddressServer1Address.sin_port = ntohs(BAddressGrabber::cForwardingServerPort);
         }

         // if there is a second server to resolve, do that.
         if (mNetAddressServer2.length() > 0)
            setCurrentState(cStateResolveAddressServer2);
         else
            setCurrentState(cStateStartAddressGrabber);
      }
      break;

      // resolving second address server
      case cStateContactAddressServer2:
      {
         // if the second server failed
         if (addressOut == -1)
         {
            // first failed as well
            if (mAddressServer1Address.sin_family == 0)
            {
               mObserverList.addressResult(BConnectivityObserver::cStateFailed);
               // we didn't fail setting up connectivity, just finding the address servers
               setCurrentState(cStateComplete);
               return;
            }
            // It is ok to fail on the second address for some reason... see grabLocalAddress below
            // mAddressServer2Address = mAddressServer1Address;
         }
         else
         {
            mAddressServer2Address.sin_family = AF_INET;
            mAddressServer2Address.sin_addr.S_un.S_addr = addressOut;
            mAddressServer2Address.sin_port = ntohs(BAddressGrabber::cForwardingServerPort);

            // if first server failed to resolve, use second as first
            if (mAddressServer1Address.sin_family == 0)
            {
               mAddressServer1Address = mAddressServer2Address;
               memset(&mAddressServer2Address, 0, sizeof(mAddressServer2Address));
            }
         }

         setCurrentState(cStateStartAddressGrabber);
      }
      break;
   }
}

//==============================================================================
// BConnectivity::setupPortLocking
//==============================================================================
void BConnectivity::setupPortLocking(void)
{
   // delete any old port finder 
   if (mPortFinder)
      delete mPortFinder;      
   mPortFinder = NULL;
   if (mPortLocker)
      delete mPortLocker;
   mPortLocker = NULL;

   sPorts.clear();

   nlog(cConnectivityNL, "  1: %ld", timeGetTime());

   // first thing we do is create our own lock
   int saveseed = rand();
   srand(timeGetTime());
   mLocalLock = (unsigned short)(rand()%64000);
   srand(saveseed); // reseed

   mPortLocker = new BSessionConnector(this);   // we want to be notified if locks are found

   if (isFlagSet(cFlagDirectIP) && !isFlagSet(cFlagDisablePortFinding))
   {  
      // advertise our lock
      HRESULT hr = mPortLocker->advertisePort(BSessionConnector::cPortAdvertiseLockPort, mLocalLock);
      if (FAILED(hr))
         nlog(cConnectivityNL, "Failed to get advertise our port lock");

      mStartTime = timeGetTime();
      nlog(cConnectivityNL, "  finding sPorts/locks, we are %ld", mLocalLock);

      mPortFinder = new BSessionConnector(NULL);   // we don't want to be notified on this finder
      setCurrentState(cStatePFStartFindingLocks);

      // now we find other ports and other locks
      SOCKADDR_IN addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.S_un.S_addr = INADDR_BROADCAST;   

      addr.sin_port = htons(BSessionConnector::cPortAdvertisePort);
      nlog(cConnectivityNL, "  finding sPorts");
      mPortFinder->findPorts(addr);

      addr.sin_port = htons(BSessionConnector::cPortAdvertiseLockPort);
      nlog(cConnectivityNL, "  finding locks");
      mPortLocker->findPorts(addr);
   }
   else
   {
      // try default sPorts
      for (long i=0;i<cPortsToTry;i++)
         sPorts.add(BSession::cDefaultHostPort+i);

      setCurrentState(cStateAllocatePorts);
   }
}

//==============================================================================
// BConnectivity::buildPortList
//==============================================================================
void BConnectivity::buildPortList(void)
{
   nlog(cConnectivityNL, "  found %ld ports", mPortFinder->getSessionDescriptorAmount());
   for (long i=0;i<mPortFinder->getSessionDescriptorAmount();i++)
      nlog(cConnectivityNL, "    %ld", *((unsigned short *)mPortFinder->getSessionDescriptor(i)->mAdvertiseInfo));

   // now build a list of 10 available sPorts      
   long tryPort=BSession::cDefaultHostPort;
   nlog(cConnectivityNL, "  starting at port %ld", tryPort);
   while (sPorts.getNumber() < cPortsToTry)
   {
      long i;
      for (i=0;i<mPortFinder->getSessionDescriptorAmount();i++)
      {
         // is someone already using this port?
         if (mPortFinder->getSessionDescriptor(i) && 
            (*((unsigned short *)mPortFinder->getSessionDescriptor(i)->mAdvertiseInfo) == tryPort))
         {
            nlog(cConnectivityNL, "    someone already using port %ld", tryPort);
            // if so, move on
            break;
         }
      }

      if (i >= mPortFinder->getSessionDescriptorAmount())
      {
         // found a port that works, add it
         sPorts.add(tryPort);
         nlog(cConnectivityNL, "    %ld is a good one", tryPort);
      }

      // try the next port
      tryPort++;
   }

   setCurrentState(cStateAllocatePorts);
}

//==============================================================================
// BConnectivity::allocatePorts
//==============================================================================
void BConnectivity::allocatePorts(void)
{
   long internalPort = 0;
   long externalPort = 0;

   HRESULT hr = E_FAIL;
   if ((isFlagSet(cFlagDirectIP)) && !isFlagSet(cFlagDisableUPNP))
   {
#ifndef XBOX   
      // setup UPnP
      BUPnPHelper::destroyInstance(); // in case we already had one active
      BUPnPHelper *helper = BUPnPHelper::getInstance();      

    
      BDynamicSimArray <long> aports;
      aports.add(BSessionConnector::cAdvertisePort);
      // register a static port for LAN game-finding broadcasts - we don't check return value, coz we silently fail anyway
      hr = helper->registerStaticPort(aports);
      if (FAILED(hr))
         hr = S_OK;

      hr = helper->registerStaticPort(sPorts, &internalPort, &externalPort);

      if (!FAILED(hr))
      {
         if (internalPort)
            nlog(cConnectivityNL, "  got a UPnP internal port of %ld", internalPort);
      }
#endif      
   }

   if ((internalPort == 0) && (sPorts.getNumber() > 0)) // didn't get a UPnP port? just use the first available one then
      internalPort = sPorts[0];
   else if (internalPort == 0)
      internalPort = BSession::cDefaultHostPort;

   if (internalPort)
      mInternalPort = internalPort;
   if (externalPort)
      mExternalPort = externalPort;

   if (internalPort)   
      nlog(cConnectivityNL, "  setup to use internalPort %ld, externalPort %ld", internalPort, externalPort);   
   else
      nlog(cConnectivityNL, "  setup to use default sPorts");

   if (!isFlagSet(cFlagDisablePortFinding) && (isFlagSet(cFlagDirectIP)) && internalPort)
   {
      // start advertising the port we got            
      // advertise on a specific port
      nlog(cConnectivityNL, "  advertising port %ld", internalPort);

      if (!mPortFinder)
         mPortFinder = new BSessionConnector(NULL);
      hr = mPortFinder->advertisePort(BSessionConnector::cPortAdvertisePort, (unsigned short)internalPort); // we'll use the UPnP assigned one
      if (FAILED(hr))
         nlog(cConnectivityNL, "Failed to advertise our port");
   }

   // done with port locking
   if (mPortLocker)
   {
      delete mPortLocker;
      mPortLocker = NULL;
   }

   setCurrentState(cStateSetupAddresses);
}

//==============================================================================
// BConnectivity::setupAddresses
//==============================================================================
void BConnectivity::setupAddresses(void)
{
   nlog(cConnectivityNL, "BConnectivity::setupAddresses");

   if (mSetupConnectivity == cSetupLAN && !isFlagSet(cFlagDirectIP))
   {
      memset(&mRoutingAddress, 0, sizeof(mRoutingAddress));
      memset(&mLocalAddress, 0, sizeof(mLocalAddress));
      memset(&mFirstAddress, 0, sizeof(mFirstAddress));

      mLocalAddress = BSocksHelper::getLocalIP();

      // if we don't have a local ip, fail.
      if ( mLocalAddress.sin_addr.S_un.S_un_b.s_b1 == 0 && 
           mLocalAddress.sin_addr.S_un.S_un_b.s_b2 == 0 && 
           mLocalAddress.sin_addr.S_un.S_un_b.s_b3 == 0 && 
           mLocalAddress.sin_addr.S_un.S_un_b.s_b4 == 0
         )
      {
         setFlag(cFlagDisableNetworking);
         mObserverList.connectivityState(BConnectivityObserver::cStateFailed, E_FAIL);
         mObserverList.networkDisabled();
         setCurrentState(cStateFailed);
         return;
      }

      mFirstAddress = mLocalAddress;

      mLocalAddress.sin_port = ntohs(BSession::cDefaultHostPort);
      if (mInternalPort>0) // set by the connectivity stuff
      {
         nlog(cConnectivityNL, "  configInternalPort=%ld", mInternalPort);
         mLocalAddress.sin_port = ntohs((unsigned short)mInternalPort);  
      }

      mTranslatedLocalAddress = mLocalAddress;

      setCurrentState(cStateComplete);
      return;
   }
}


//==============================================================================
// BConnectivity::getAddress
//==============================================================================
void BConnectivity::getAddress(void)
{
   mdwAddressChangeTime = timeGetTime(); // begin time

   // FIXME: If we just fail to figure out a local address altogether, multiplayer should lock you out entirely      
   // grab an intial forwarding server address for the LAN/DirectIP case - ESO will replace this if needed
   memset(&mAddressServer1Address, 0, sizeof(mAddressServer1Address));
   memset(&mAddressServer2Address, 0, sizeof(mAddressServer2Address));

   nlog(cConnectivityNL, "mAddressServer1Address %s", inet_ntoa(mAddressServer1Address.sin_addr));      

   memset(&mLocalAddress, 0, sizeof(mLocalAddress));
   memset(&mFirstAddress, 0, sizeof(mFirstAddress));

   mFlags.set(cFlagInterfaceQuery);
   HRESULT hr = BSocksHelper::getLocalIP(mRoutingAddress, &mLocalAddress, true);
   if ( hr != NOERROR)
   {
      setCurrentState(cStateFailed, hr);
      return;
   }

   mFirstAddress.sin_addr.S_un.S_addr = mLocalAddress.sin_addr.S_un.S_addr;

   mLocalAddress.sin_port = ntohs(BSession::cDefaultHostPort);
   if (mInternalPort>0) // set by the connectivity stuff
   {
      nlog(cConnectivityNL, "  configInternalPort=%ld", mInternalPort);
      mLocalAddress.sin_port = ntohs((unsigned short)mInternalPort);  
   }

   nlog(cConnectivityNL, "  mLocalAddress %s:%ld", inet_ntoa(mLocalAddress.sin_addr), ntohs(mLocalAddress.sin_port));

   mTranslatedLocalAddress = mLocalAddress;

   if (inet_ntoa(mTranslatedLocalAddress.sin_addr))
      nlog(cConnectivityNL, "Translated Local address %s:%ld", inet_ntoa(mTranslatedLocalAddress.sin_addr), htons(mTranslatedLocalAddress.sin_port));
   else
      nlog(cConnectivityNL, "Bad local address");

   // should we contact the address servers?
   if (mNetAddressServer1.length() > 0)
   {
      // start resolving the first address server
      hr = BSocksHelper::socksGetAddress(mNetAddressServer1.getPtr(), this);
      if (FAILED(hr))
         nlog(cConnectivityNL, "Failed to get address server 1 IP address");

      setCurrentState(cStateContactAddressServer1);
   }
   // nope, done
   else
      setCurrentState(cStateComplete);
}

//==============================================================================
// BConnectivity::getLocalAddress
//==============================================================================
void BConnectivity::setCurrentState(long state, HRESULT hr)
{
   mCurrentState = state;
   mLastError    = hr;
   switch (mCurrentState)
   {
      case cStateSetupPortFinding:
         mObserverList.connectivityState(BConnectivityObserver::cStatePortFinding);
      break;

      case cStateAllocatePorts:
         mObserverList.connectivityState(BConnectivityObserver::cStateAllocatingPort);
         break;

      case cStateGetLocalAddress:
         mObserverList.connectivityState(BConnectivityObserver::cStateGettingLocalAddress);
         break;

      case cStateContactAddressServer1:
         mObserverList.connectivityState(BConnectivityObserver::cStateContactAddressServer);
         break;

      case cStateComplete:
      {
         if (mAddressSocket != NULL)
            mAddressSocket->dispose();

         mAddressSocket = new BSocksUnreliableSocket(this);
         SOCKADDR_IN address;
         memset(&address, 0, sizeof(address));
         address.sin_family = AF_INET; 
         address.sin_addr = mLocalAddress.sin_addr;
         address.sin_port = htons(2297);
         mAddressSocket->createAndBind(&address);

         mObserverList.connectivityState(BConnectivityObserver::cStateComplete);
      }
      break;            

      case cStateFailed:
         mSetupConnectivity = cNone;
         if (SUCCEEDED(mLastError))
            mLastError = E_FAIL;
         mObserverList.connectivityState(BConnectivityObserver::cStateFailed, mLastError);
         break;            
   }
}

//==============================================================================
// BConnectivity::testAddressChange
//==============================================================================
DWORD BConnectivity::testAddressChange()
{
   // check to see if we have a new address
   SOCKADDR_IN testAddress;
   DWORD res;

   memset(&testAddress, 0, sizeof(testAddress));

   if (mSetupConnectivity == cSetupLAN && !isFlagSet(cFlagDirectIP))
   {
      testAddress = BSocksHelper::getLocalIP();
   }
   else
   {
      res = BSocksHelper::getLocalIP(mRoutingAddress, &testAddress, isFlagSet(cFlagInterfaceQuery));
      if (res != NOERROR)
      {
         {setBlogError(4180); blogerror("BConnectivity::service -- failed to grab local ip. result: %d", res);}
         setFlag(cFlagDisableNetworking);
         mObserverList.connectivityState(BConnectivityObserver::cStateFailed, E_FAIL);
         mObserverList.networkDisabled();
         return cMaxAddressFailures;
      }
   }

   if (testAddress.sin_addr.S_un.S_addr != mFirstAddress.sin_addr.S_un.S_addr)
   {
      blog("BConnectivity::service -- %d.%d.%d.%d != %d.%d.%d.%d", testAddress.sin_addr.S_un.S_un_b.s_b1,
         testAddress.sin_addr.S_un.S_un_b.s_b2,
         testAddress.sin_addr.S_un.S_un_b.s_b3,
         testAddress.sin_addr.S_un.S_un_b.s_b4,
         mFirstAddress.sin_addr.S_un.S_un_b.s_b1,
         mFirstAddress.sin_addr.S_un.S_un_b.s_b2,
         mFirstAddress.sin_addr.S_un.S_un_b.s_b3,
         mFirstAddress.sin_addr.S_un.S_un_b.s_b4);

      // if we failed to get the local IP address, keep track of the number of failures.
      // we'll keep retrying for some number of failures and then return an error.
      mAddressGrabFailures++;
      if (mAddressGrabFailures >= cMaxAddressFailures)
      {
         mAddressGrabFailures=0;
         setFlag(cFlagDisableNetworking);
         mObserverList.connectivityState(BConnectivityObserver::cStateFailed, E_FAIL);
         mObserverList.networkDisabled();
         //return (DWORD)mAddressGrabFailures;
      }
   }

   return (DWORD)mAddressGrabFailures;
}