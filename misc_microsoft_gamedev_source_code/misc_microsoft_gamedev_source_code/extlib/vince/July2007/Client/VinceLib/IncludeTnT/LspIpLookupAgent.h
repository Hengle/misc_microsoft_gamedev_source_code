//--------------------------------------------------------------------------------------
// LspIpLookupAgent.h
// 
// This is a specialized implementation of the IpLookupAgent.  This version works on 
// the Xbox 360 with security enabled.  This is the only version of an IpLookupAgent 
// that works with Xbox 360 retail.  The lookup over LSP uses a title ID to get a list 
// of servers.  The list of servers returned can be searched by name and a specific 
// server can be used to get a target IP address.  If multiple servers are returned 
// bearing the same name, the algorithm randomly picks one in a crude form of load 
// balancing.  The IP is only valid while in use and for a limited time thereafter.  
// The port defaults to 80 but you can specify a different 
// port.  The port is used when initializing the SOCKADDR_IN version of GetIpAddress.
// Lookup is called automatically by a call to either version of GetIpAddress if it 
// has not yet been called.  Calling Lookup directly subsequent time refreshes the 
// address.  Calling Release directly invalidates the IP address.  Release will be 
// called when the class destructs.
//
// NOTE: XNet and WSA must be started prior to using this class.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifdef _XBOX

#include "IpLookupAgent.h"

//#define LSP_V1
#ifndef LSP_V1
    #define BUILD_PLATFORM_XENON
    #include "XtsLspUtils.h"
#endif

namespace TnT
{

    class LspIpLookupAgent : public IpLookupAgent
    {
    public:
        LspIpLookupAgent(void);
        virtual ~LspIpLookupAgent(void);

        HRESULT GetIpAddress(char* pIpAddressBuffer, DWORD* pdwSize);
        HRESULT GetIpAddress(SOCKADDR_IN* pSocketAddressIn);

        DWORD GetServiceId();
        void SetServiceId(DWORD dwServiceId);

        virtual void SetDestinationName(const char* strDestinationName);
        virtual const char* GetDestinationName();
        virtual unsigned short GetPort();
        virtual void SetPort(unsigned short nPort);
        const char* GetServiceString();
        void SetServiceString(const char* strServiceString);

        HRESULT Lookup();


    private:
        LspIpLookupAgent(const LspIpLookupAgent &val);
        LspIpLookupAgent& operator=(const LspIpLookupAgent&);
        HRESULT Release();

        DWORD m_dwServiceId;
        bool m_bInitialized;
        std::string m_strIpAddress;
        SOCKADDR_IN m_socketAddressIn;
        unsigned short m_nPort;
        std::string m_strServiceString;

#ifdef LSP_V1
        XNKID m_xnkid;
#else
        Ref<ISocketData> m_pSocketData;
#endif

    };

#if _DEBUG
	class LspDebugLog : public ILogEvent
	{
	public:
					 LspDebugLog() : m_refCount(0) {}
		virtual		~LspDebugLog() {}
	    virtual void AddRef(const char* sName);
		virtual void Release(const char* sName);
	    virtual void Log(bool logAlways, Severity sev, DWORD dwID, const char* message);
	private:
		int m_refCount;
	};
#endif

};

#endif // _XBOX