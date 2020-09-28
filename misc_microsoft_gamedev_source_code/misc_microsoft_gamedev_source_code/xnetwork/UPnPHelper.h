#ifdef XBOX
#error Unsupported on Xbox
#endif

//==============================================================================
// UPnPHelper.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _UPnPHelper_H_
#define _UPnPHelper_H_

//==============================================================================
// Includes

#include "thread.h"
#include "dplay8.h"
#include "dpnathlp.h"

//==============================================================================
// Forward declarations

struct IDirectPlayNATHelp;

//==============================================================================
// Const declarations

class BRegisteredPort
{
   public:
      BRegisteredPort(void) : handle(0), internalPort(0), externalPort(0), dynamic(false) {}
      BRegisteredPort(DPNHANDLE h, long ip, long ep, bool d) : handle(h), internalPort(ip), externalPort(ep), dynamic(d) {}
      long internalPort;
      long externalPort;
      DPNHANDLE handle;
      bool dynamic;
};

//==============================================================================
class BUPnPHelper
{
   public:
      // singleton ctor/dtor funcs
      static BUPnPHelper *getInstance(void);
      static void destroyInstance(void);      

      // Functions

      // this func attempts to register a dynamic port on the UPnP device - in other words,
      // say you request local port 2300, this func will tell the UPnP device to open 
      // a dynamically assigned port for you and map that port to your local 2300 - 
      // that port could be anything, but it should stay the same (and open/mapped) 
      // until releasePort is called (it's re-leased continuously on a seperate thread.)
      // params: the port you want, how many ports to try (incremented from portRequested), 
      // and the actual port that got registered
      // example: 2300, 5, returnPort - start at 2300 and if that's taken try 2301-2304
      HRESULT registerDynamicPort(BDynamicSimArray<long> &portsToTry, long *internalPort = 0, long *externalPort = 0)
      { return registerPort(true, portsToTry, internalPort, externalPort); }
      // this function attempts to register a static port on the UPnP device - in other words,
      // if you request port 2301, it will attempt to give you an external UDP port of 2301 on the UPnP device
      // and map that to the local UDP port 2301 - the port is continuously re-leased on a seperate thread, until
      // you call releasePort
      HRESULT registerStaticPort(BDynamicSimArray<long> &portsToTry, long *internalPort = 0, long *externalPort = 0)
      { return registerPort(false, portsToTry, internalPort, externalPort); }      

      // release all ports
      HRESULT releasePorts(void);

      long getPortsRegistered(void) { return mRegisteredPorts.getNumber(); }

      HRESULT getStatus(void) { return mStatus; resetStatus(); }
      void resetStatus(void) { mStatus = S_OK; }
      
      static void* _cdecl threadService(void* pVal);

      // vars for static thread func to access
      IDirectPlayNATHelp *mNatHelp;
      bool mShutdownThread;  
      bool mInThread;
      bool mMutex;
      BDynamicSimArray <BRegisteredPort> mRegisteredPorts;
      HRESULT mStatus;

   private:
      enum { cMinLeaseTime = 30000, cMaxLeaseTime = 120000 }; // time to lease the port for

      HRESULT registerPort(bool dynamic, BDynamicSimArray<long> &portsToTry, long *internalPort = 0, long *externalPort = 0);
      HRESULT initNAT(void);
      HRESULT createNAT(void);
      void shutdownNAT(void);

      // private to ensure singleton
      BUPnPHelper(void);      
      ~BUPnPHelper( void );

      // Functions
      HRESULT killWorkerThread(void);

      // Variables      
      BThread *mWorkerThread;      
      SOCKADDR_IN mLocalAddress;            
      DWORD mLeaseTime;

}; // BUPnPHelper

extern BUPnPHelper *gUPnPHelper;


//==============================================================================
#endif // _UPnPHelper_H_

//==============================================================================
// eof: UPnPHelper.h
//==============================================================================
