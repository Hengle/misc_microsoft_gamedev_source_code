//--------------------------------------------------------------------------------------
// CachedIpLookupAgent.h
// 
// This is a specialized implementation of the IpLookupAgent.  The IP address is cached 
// and does not change.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "iplookupagent.h"

namespace TnT
{

    class CachedIpLookupAgent :
        public IpLookupAgent
    {
    public:
        CachedIpLookupAgent(void);
        virtual ~CachedIpLookupAgent(void);

        virtual HRESULT GetIpAddress(char* pIpAddressBuffer, DWORD* pdwSize);
        virtual HRESULT GetIpAddress(SOCKADDR_IN* pSocketAddressIn);
        void SetIpAddress(const char* pIpAddressBuffer);
        void SetIpAddress(const SOCKADDR_IN& socketAddressIn);

        virtual void SetDestinationName(const char* strDestinationName);
        virtual const char* GetDestinationName();
        virtual unsigned short GetPort();
        virtual void SetPort(unsigned short nPort);

    private:
        CachedIpLookupAgent(const CachedIpLookupAgent &val);
        CachedIpLookupAgent& operator=(const CachedIpLookupAgent&);

        std::string m_strIpAddress;
        SOCKADDR_IN m_socketAddressIn;

		template< typename T > friend class TestFixture;
    };

}