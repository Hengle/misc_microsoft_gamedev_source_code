//--------------------------------------------------------------------------------------
// StandardIpLookupAgent.h
// 
// This is a specialized implementation of IpLookupAgent.  This version works on both 
// Windows and the Xbox 360.  The Xbox 360 implementation is only for unsecured network 
// activity.  If you need an LSP enabled version use that specialized class instead. 
// Lookup requires a destination name.  The destination name can be an IP address string 
// or a URL without the transport e.g. www.microsoft.com instead of 
// http://www.microsoft.com.  The port defaults to 80 but you can specify a different 
// port.  The port is used when initializing the SOCKADDR_IN version of GetIpAddress.
// Lookup is called automatically by a call to either version of GetIpAddress if it 
// has not yet been called.  Calling Lookup directly subsequent times refreshes the 
// address.
//
// NOTE: [XNet and] WSA must be started prior to using this class.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "iplookupagent.h"

namespace TnT
{

    class StandardIpLookupAgent :
        public IpLookupAgent
    {
    public:
        StandardIpLookupAgent(void);
        virtual ~StandardIpLookupAgent(void);

        HRESULT GetIpAddress(char* pIpAddressBuffer, DWORD* pdwSize);
        HRESULT GetIpAddress(SOCKADDR_IN* pSocketAddressIn);

        virtual const char* GetDestinationName();
        virtual void SetDestinationName(const char* strDestinationName);
        virtual unsigned short GetPort();
        virtual void SetPort(unsigned short nPort);

        HRESULT Lookup();

    private:
        StandardIpLookupAgent(const StandardIpLookupAgent &val);
        StandardIpLookupAgent& operator=(const StandardIpLookupAgent&);

        std::string m_strDestinationName;
        bool m_bInitialized;
        std::string m_strIpAddress;
        SOCKADDR_IN m_socketAddressIn;
        unsigned short m_nPort;

        template< typename T > friend class TestFixture;
    };

};
