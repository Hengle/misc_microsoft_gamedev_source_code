//==============================================================================
// UPnPHelper.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "UPnPHelper.h"
#include "sockshelper.h"

BUPnPHelper *gUPnPHelper = 0;

const GUID FAR CLSID_DirectPlayNATHelpUPnP = { 0xb9c2e9c4, 0x68c1, 0x4d42, { 0xa7, 0xa1, 0xe7, 0x6a, 0x26, 0x98, 0x2a, 0xd6 } };
const GUID FAR IID_IDirectPlayNATHelp = { 0x154940b6, 0x2278, 0x4a2f, { 0x91, 0x1, 0x9b, 0xa9, 0xf4, 0x31, 0xf6, 0x3 } };

//==============================================================================
// Defines

#define IF_FAILED_RETURN(test) hr = test; if (FAILED(hr)) return hr;

//==============================================================================
// 
BUPnPHelper *BUPnPHelper::getInstance(void)
{
   if (!gUPnPHelper)
      gUPnPHelper = new BUPnPHelper();

   return gUPnPHelper;   
}

//==============================================================================
// 
void BUPnPHelper::destroyInstance(void)
{
   if (!gUPnPHelper)
      return;

   delete gUPnPHelper;
   gUPnPHelper = 0;
}

//==============================================================================
// BUPnPHelper::BUPnPHelper
//==============================================================================
BUPnPHelper::BUPnPHelper(void) : 
   mNatHelp(0),
   mWorkerThread(0),
   mStatus(S_OK),
   mMutex(false),
   mInThread(false),
   mShutdownThread(false),
   mLeaseTime(cMinLeaseTime)
{         
} // BUPnPHelper::BUPnPHelper

//==============================================================================
// BUPnPHelper::~BUPnPHelper
//==============================================================================
BUPnPHelper::~BUPnPHelper(void)
{
   releasePorts();
   
   gUPnPHelper = 0;
} // BUPnPHelper::~BUPnPHelper

//==============================================================================
// 
HRESULT BUPnPHelper::registerPort(bool dynamic, BSimpleArray <long> &portsToTry, long *internalPort, long *externalPort)
{
   long i=0;
   SOCKADDR *ptr1 = 0;
   SOCKADDR_IN requestedAddress;
   DPNHCAPS caps;
   DWORD size;
   SOCKADDR_IN address;
   memset(&address, 0, sizeof(address));

   while (mMutex) SleepEx(0, true);

   nlog(cTransportNL, "BUPnPHelper::registerPort dynamic %ld, numberOfPortsToTry %ld", (long)dynamic, portsToTry.getNumber());

   mMutex = true;

   HRESULT hr = S_OK;

   if (!mNatHelp)
   {      
      hr = initNAT();
      if (FAILED(hr))
         goto end;
   }

   DPNHANDLE registeredPortHandle;
   memset(&registeredPortHandle, 0, sizeof(registeredPortHandle));
   
   memset(&requestedAddress, 0, sizeof(requestedAddress));

   requestedAddress = mLocalAddress;
   requestedAddress.sin_family = AF_INET;

   ptr1 = (SOCKADDR *)&requestedAddress;

   // try registering the port(s)
   for (i=0;i<portsToTry.getNumber();i++)
   {      
      requestedAddress.sin_port = htons((unsigned short)portsToTry[i]);   

      DWORD flags = 0;

      if (!dynamic)
         flags = DPNHREGISTERPORTS_FIXEDPORTS;

      nlog(cTransportNL, "  trying port %ld", ntohs(requestedAddress.sin_port));

      // test to see if the port is already registered
      SOCKADDR_IN sourceAddress;
      memset(&sourceAddress, 0, sizeof(sourceAddress));
      sourceAddress.sin_family = AF_INET;
      sourceAddress.sin_addr.S_un.S_addr = INADDR_ANY;      
      SOCKADDR_IN responseAddress;
      memset(&responseAddress, 0, sizeof(responseAddress));
      hr = mNatHelp->QueryAddress((SOCKADDR *)&sourceAddress, (SOCKADDR *)&requestedAddress, (SOCKADDR *)&responseAddress, sizeof(responseAddress), 0);
      nlog(cTransportNL, "  QueryAddress hr %ld, requested port %ld, response port %ld", hr, ntohs(requestedAddress.sin_port), ntohs(responseAddress.sin_port));

      if (!FAILED(hr) && (responseAddress.sin_port != 0))
      {
         nlog(cTransportNL, "  NAT already has a port registered at this address, responseAddress %s:%ld", inet_ntoa(responseAddress.sin_addr), ntohs(responseAddress.sin_port));
         continue;
      }

      hr = mNatHelp->RegisterPorts(ptr1, sizeof(requestedAddress), 1, mLeaseTime, &registeredPortHandle, flags);
      if (hr == DPNH_OK)
      {
         // try and get the public mapping for this port
         // refresh the device   
         caps.dwSize = sizeof(caps);
         hr = gUPnPHelper->mNatHelp->GetCaps(&caps, DPNHGETCAPS_UPDATESERVERSTATUS);
         gUPnPHelper->mLeaseTime = caps.dwRecommendedGetCapsInterval;
         if (gUPnPHelper->mLeaseTime < cMinLeaseTime)
            gUPnPHelper->mLeaseTime = cMinLeaseTime;
         if (gUPnPHelper->mLeaseTime > cMaxLeaseTime)
            gUPnPHelper->mLeaseTime = cMaxLeaseTime;
         if (FAILED(hr))
         {
            hr = DPNHERR_SERVERNOTAVAILABLE;
            nlog(cTransportNL, "  Failed to refresh device");
            break;
         }
         if ( !(caps.dwFlags & DPNHCAPSFLAG_GATEWAYPRESENT) )
         {
            hr = DPNHERR_SERVERNOTAVAILABLE;
            nlog(cTransportNL, "  no NAT device available");
            break;            
         }
   
         // get required buffer size   
         hr = mNatHelp->GetRegisteredAddresses(registeredPortHandle, 0, &size, 0, 0, 0);
         if ((hr != DPNHERR_BUFFERTOOSMALL) && FAILED(hr))
         {
            nlog(cTransportNL, "  GetRegisteredAddresses get size failed");
            continue;
         }
         nlog(cTransportNL, "  size %ld", size);

         if (size != sizeof(SOCKADDR_IN))
         {
            nlog(cTransportNL, "    is bad");
            continue; // er, that's bad
         }
              
         flags = 0;
         DWORD time;
         hr = mNatHelp->GetRegisteredAddresses(registeredPortHandle, (SOCKADDR *)&address, &size, &flags, &time, 0);
         if (FAILED(hr))
         {
            if (hr == DPNHERR_GENERIC)
               nlog(cTransportNL, "  An error occurred while closing.");
            else if (hr == DPNHERR_BUFFERTOOSMALL)
               nlog(cTransportNL, "  The supplied buffer is not large enough to contain the requested data.");
            else if (hr == DPNHERR_INVALIDFLAGS)
               nlog(cTransportNL, "  Invalid flags were specified.");
            else if (hr == DPNHERR_INVALIDOBJECT)
               nlog(cTransportNL, "  The interface object is invalid.");
            else if (hr == DPNHERR_INVALIDPARAM)
               nlog(cTransportNL, "  An invalid parameter was specified.  ");
            else if (hr == DPNHERR_NOMAPPING)
               nlog(cTransportNL, "  The server does not have valid public interfaces.  ");
            else if (hr == DPNHERR_NOTINITIALIZED)
               nlog(cTransportNL, "  The object has not been initialized.  ");
            else if (hr == DPNHERR_OUTOFMEMORY)
               nlog(cTransportNL, "  There is not enough memory to perform this operation.  ");
            else if (hr == DPNHERR_REENTRANT)
               nlog(cTransportNL, "  The interface has been re-entered on the same thread.  ");
            else if (hr == DPNHERR_SERVERNOTAVAILABLE)
               nlog(cTransportNL, "  No servers are currently present.  ");
            else if (hr == DPNHERR_UPDATESERVERSTATUS) 
               nlog(cTransportNL, "  IDirectPlayNATHelp::GetCaps has not been called with the DPNHGETCAPS_UPDATESERVERSTATUS flag set. ");
            else
               nlog(cTransportNL, "  GetRegisteredAddresses failed err=(0x%x)", hr);

            continue;
         }

         if (size < sizeof(SOCKADDR_IN))
         {
            nlog(cTransportNL, "  bad size");
            continue;
         }         

         // otherwise, we have ourselves a good port
         if (internalPort)
            *internalPort = ntohs(requestedAddress.sin_port);

         if (externalPort)
            *externalPort = ntohs(address.sin_port);

         break;
      }
      else if (hr == DPNHERR_PORTALREADYREGISTERED)
      {
         nlog(cTransportNL, "  DPNHERR_PORTALREADYREGISTERED");
         continue;      
      }
   }

   if (FAILED(hr))
      goto end;

   if (i >= portsToTry.getNumber())
   {
      nlog(cTransportNL, "  nothing available");
      // well, I think the best thing we can do here is just fail silently and hope the UPnP device is doing something
      if (internalPort)
         *internalPort = ntohs(requestedAddress.sin_port);

      if (externalPort)
         *externalPort = ntohs(0);

      /*hr = E_FAIL; // nope, nothing available
      goto end;*/
   }   

   nlog(cTransportNL, "  port registered internal %ld, external %ld", ntohs(requestedAddress.sin_port), ntohs(address.sin_port));

   mRegisteredPorts.add(BRegisteredPort(registeredPortHandle, requestedAddress.sin_port, address.sin_port, dynamic));

end:   

   mMutex = false;
   
   if (!FAILED(hr))
   {
      // spin up the thread
      if (!mWorkerThread)
      {
         mWorkerThread = new BThread();
         if (!mWorkerThread->createThread(BUPnPHelper::threadService, NULL, 0, false))
         {
            hr = releasePorts();
            if (SUCCEEDED(hr))
               hr = HRESULT_FROM_WIN32(ERROR_SERVICE_NO_THREAD);
            goto end;
         }

         mWorkerThread->setPriority(THREAD_PRIORITY_TIME_CRITICAL);  
      }   
   }

   if (FAILED(hr))
      releasePorts();

   return hr;
}

//==============================================================================
// 
HRESULT BUPnPHelper::killWorkerThread(void)
{
   HRESULT hr = S_OK;
   if (!mWorkerThread)
      return hr;

   mShutdownThread = true;
   SleepEx(0, true);

   const DWORD cKillThreadTimeout = 1000;
   DWORD now = timeGetTime();
   while (mShutdownThread) // wait for thread to go byebye
   {
      if ((timeGetTime() - now) > cKillThreadTimeout)
      {
         hr = E_FAIL;
         break;
      }
      SleepEx(100, true);
   }

   delete mWorkerThread;
   mWorkerThread = 0;

   mInThread = false;
   mShutdownThread = false;

   return hr;
}

//==============================================================================
// 
HRESULT BUPnPHelper::initNAT(void)
{
   nlog(cTransportNL, "BUPnPHelper::initNAT");

   HRESULT hr = S_OK;

   // Create the Object
   hr = CoCreateInstance( CLSID_DirectPlayNATHelpUPnP, NULL, 
                          CLSCTX_INPROC_SERVER,
                          IID_IDirectPlayNATHelp, 
                          (LPVOID*) &mNatHelp );
   if (FAILED(hr))
   {
      nlog(cTransportNL, "  CoCreateInstance failed");
      releasePorts();
      return hr;
   }

   // Initialize it
   hr = mNatHelp->Initialize(0);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "  Initialize failed");
      releasePorts();
      return hr;
   }

   DPNHCAPS caps;
   caps.dwSize = sizeof(caps);

   // find the upnp device
   hr = mNatHelp->GetCaps(&caps, 0);
   if (FAILED(hr))
   {
      nlog(cTransportNL, "  GetCaps(0) failed");
      releasePorts();
      return hr;
   }
   mLeaseTime = caps.dwRecommendedGetCapsInterval;
   if (mLeaseTime < cMinLeaseTime)
      mLeaseTime = cMinLeaseTime;
   if (mLeaseTime > cMaxLeaseTime)
      mLeaseTime = cMaxLeaseTime;
     
   memset(&mLocalAddress, 0, sizeof(mLocalAddress));   
   mLocalAddress = BSocksHelper::getLocalIP();

   nlog(cTransportNL, "  local address %s", inet_ntoa(mLocalAddress.sin_addr));

   return hr;
}

//==============================================================================
// 
void BUPnPHelper::shutdownNAT(void)
{
   nlog(cTransportNL, "BUPnPHelper::shutdownNAT");

   while (gUPnPHelper->mShutdownThread) SleepEx(0, true); // spin here if needed

   if (gUPnPHelper->mInThread)
   {
      BASSERT(0);
      return; // kill the thread first!
   }   

   if (mNatHelp)
   {
      mNatHelp->Close(0);
      mNatHelp->Release();
      mNatHelp = 0;
   }

   mRegisteredPorts.clear();
}

//==============================================================================
// 
void* _cdecl BUPnPHelper::threadService(void* pVal)
{
   gUPnPHelper->mInThread = true;

   pVal;

   DWORD lastRefresh = 0;

   const DWORD cFudgeFactor = 5000; // just so we stay away from the edge of timing out the lease

   while (!gUPnPHelper->mShutdownThread)
   {      
      while (gUPnPHelper->mMutex) SleepEx(100, true); // wait for main thread 

      gUPnPHelper->mMutex = true;

      if ((timeGetTime() - lastRefresh) > (gUPnPHelper->mLeaseTime - cFudgeFactor))
      {
         if (!gUPnPHelper)
         {
            gUPnPHelper->mStatus = HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST);
            break; // where'd the object go? We're out of here
         }

         if (!gUPnPHelper->mNatHelp)
         {
            gUPnPHelper->mStatus = HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST);
            break; // where'd the object go? We're out of here
         }

         DPNHCAPS caps;
         caps.dwSize = sizeof(caps);

         // refresh the upnp device
         HRESULT hr = gUPnPHelper->mNatHelp->GetCaps(&caps, DPNHGETCAPS_UPDATESERVERSTATUS);
         gUPnPHelper->mLeaseTime = caps.dwRecommendedGetCapsInterval;
         if (gUPnPHelper->mLeaseTime < cMinLeaseTime)
            gUPnPHelper->mLeaseTime = cMinLeaseTime;
         if (gUPnPHelper->mLeaseTime > cMaxLeaseTime)
            gUPnPHelper->mLeaseTime = cMaxLeaseTime;
         if (hr == DPNHSUCCESS_ADDRESSESCHANGED)
         {
            /*
            // attempt to recover by closing and re-opening the NAT interface - per Windows OS Bugs bug #766345    
            gUPnPHelper->shutdownNAT();   
            if (FAILED(gUPnPHelper->initNAT()))
            {
               gUPnPHelper->mStatus = E_FAIL;
               break; // we're dead, we gotta bail
            }

            // re-register ports
            for (long i=0;i<gUPnPHelper->mRegisteredPorts.getNumber();i++)
            {
               long oldInternalPort = gUPnPHelper->mRegisteredPorts[i].internalPort;
               long oldExternalPort = gUPnPHelper->mRegisteredPorts[i].externalPort;
               long newInternalPort=0;
               long newExternalPort=0;

               if (gUPnPHelper->mRegisteredPorts[i].dynamic)
               {
                  BSimpleArray <long> ports;
                  ports.add(gUPnPHelper->mRegisteredPorts[i].internalPort);
                  gUPnPHelper->registerDynamicPort(ports, &newInternalPort, &newExternalPort);
               }
               else
               {
                  BSimpleArray <long> ports;
                  ports.add(gUPnPHelper->mRegisteredPorts[i].internalPort);
                  gUPnPHelper->registerStaticPort(ports, &newInternalPort, &newExternalPort);
               }

               if ((oldInternalPort != newInternalPort) || (oldExternalPort != newExternalPort))
               {                  
                  gUPnPHelper->mStatus = HRESULT_FROM_WIN32(ERROR_MEDIA_CHANGED);
                  break;
               }
            }            */

            // NOTE: We are not going to recover from this at this point - we force restart of the game
            gUPnPHelper->mStatus = HRESULT_FROM_WIN32(ERROR_MEDIA_CHANGED);

            if (gUPnPHelper->mStatus == HRESULT_FROM_WIN32(ERROR_MEDIA_CHANGED))
               break; // address has changed, we have to exit the thread
         }         
         else if (FAILED(hr) && (gUPnPHelper->mStatus == S_OK))
         {
            gUPnPHelper->mStatus = hr;
            break; // failure, we exit the thread
         }

         lastRefresh = timeGetTime();
      }

      gUPnPHelper->mMutex = false;

      SleepEx(100, true);
   }
   
   gUPnPHelper->mMutex = false;
   gUPnPHelper->mShutdownThread = false; // we're done
   gUPnPHelper->mInThread = false; // we're done

   return 0;
}

//==============================================================================
// 
HRESULT BUPnPHelper::releasePorts(void)
{
   nlog(cTransportNL, "BUPnPHelper::releasePorts");

   HRESULT hr = S_OK;

   if (mNatHelp)
   {
      for (long i=0;i<mRegisteredPorts.getNumber();i++)
      {
         hr = mNatHelp->DeregisterPorts(mRegisteredPorts[i].handle, NULL);
         if(FAILED(hr))
            nlog(cTransportNL, "BUPnPHelper::releasePorts -- failed DeregisterPorts 0x%x.", hr);
      }
   }

   hr = killWorkerThread();
   if(FAILED(hr))
      nlog(cTransportNL, "BUPnPHelper::releasePorts -- failed killWorkerThread 0x%x.", hr);

   shutdownNAT();
   mRegisteredPorts.clear();

   return hr;
}


//==============================================================================
// eof: UPnPHelper.cpp
//==============================================================================
