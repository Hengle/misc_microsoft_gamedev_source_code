//--------------------------------------------------------------------------------------
// IpLookupAgent.h
// 
// This abstract base class serves as the base interface for IpLookupAgents.  The 
// specialized class controls what data is required by the lookup based on the needs 
// of the platform and security.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "TnTCommon.h"

namespace TnT
{

    class IpLookupAgent
    {
    public:
        IpLookupAgent(void);
        virtual ~IpLookupAgent(void);

        //------------------------------------------------------------------------------
        // Name: IpLookupAgent::GetIpAddress
        // Desc: This overload copies the IP address string into a buffer provided 
        //       by the caller.  This follows traditional Win32 calling convention.
        //       If the string buffer is null, calling the method will return the 
        //       size required by IP address string and null terminator through the out 
        //       parameter and a success code.  If the buffer is not large enough, the 
        //       method will return the size required by the IP address string and null 
        //       terminator through the out parameter and an error code.  Since an IP4 
        //       address is 15 characters allocating a buffer of at least 16 characters 
        //       is usually good enough.
        //------------------------------------------------------------------------------
        virtual HRESULT GetIpAddress(char* pIpAddressBuffer, DWORD* pdwSize) = 0;

        //------------------------------------------------------------------------------
        // Name: IpLookupAgent::GetIpAddress
        // Desc: This overload copies the IP address to a SOCKADDR_IN structure.
        //------------------------------------------------------------------------------
        virtual HRESULT GetIpAddress(SOCKADDR_IN* pSocketAddressIn) = 0;

        //------------------------------------------------------------------------------
        // Name: IpLookupAgent::SetDestinationName
        // Desc: Sets the destination host name
        //------------------------------------------------------------------------------
        virtual void SetDestinationName(const char* strDestinationName) = 0;

        //------------------------------------------------------------------------------
        // Name: IpLookupAgent::GetDestinationName
        // Desc: Gets the destination host name
        //------------------------------------------------------------------------------
        virtual const char* GetDestinationName() = 0;

        //------------------------------------------------------------------------------
        // Name: IpLookupAgent::SetPort
        // Desc: Sets the destination port
        //------------------------------------------------------------------------------
        virtual void SetPort(unsigned short nPort) = 0;

        //------------------------------------------------------------------------------
        // Name: IpLookupAgent::GetPort
        // Desc: Gets the destination port
        //------------------------------------------------------------------------------
        virtual unsigned short GetPort() = 0;

    protected:
        virtual HRESULT CopyIpAddressToBuffer(char* pDestinationBuffer, DWORD* pdwDestinationSize, const char* pSourceBuffer, DWORD dwSourceSize);

    private:
        IpLookupAgent(const IpLookupAgent &val);
        IpLookupAgent& operator=(const IpLookupAgent&);
    };

};