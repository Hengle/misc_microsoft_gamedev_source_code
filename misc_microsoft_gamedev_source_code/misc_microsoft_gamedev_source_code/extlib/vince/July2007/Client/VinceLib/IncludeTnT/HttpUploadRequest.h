//--------------------------------------------------------------------------------------
// HttpUploadRequest.h
// 
// This abstract base class serves as the base interface for HTTP upload requests.  The 
// specialized classes control additional request details e.g. if additional content is 
// being uploaded, if the additional content is in a buffer or file, etc.  Expect 
// Continue is a feature supported by HTTP uploads with additional content.  The caller 
// can send just the header packet to let the server know additional content is coming 
// and the size of the content.  The server can reject or allow the additional content 
// before the client sends all the data to the server.  If a specialized class 
// supports ExpectContinue, the first packet returned by GetNextData must be the 
// complete HTTP header before the first MIME boundary.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "TnTCommon.h"
#include "IpLookupAgent.h"

namespace TnT
{

    class HttpUploadRequest
    {
    public:
        HttpUploadRequest(void);
        virtual ~HttpUploadRequest(void);

        //------------------------------------------------------------------------------
        // Name: HttpUploadRequest::GetNextData
        // Desc: This method can be called one or more times to retrieve an HTTP 
        //       request that can be sent to the server.  When the method returns 
        //       E_END_OF_DATA there is no more data to retrieve.  An error was chosen 
        //       so a client could loop while SUCCEEDED or !FAILED.  After such a loop 
        //       the exit result could be compared to E_END_OR_DATA to determine if 
        //       an error occurred.
        //------------------------------------------------------------------------------
        virtual HRESULT GetNextData(BYTE** ppBuffer, DWORD* pdwSize) = 0;


        //------------------------------------------------------------------------------
        // Name: HttpUploadRequest::GetNextData
        // Desc: This returns the IpLookupAgent for this request.  This abstraction 
        //       allows the lookup to be delayed until the request is ready to be 
        //       processed.  This is mostly useful for the Xbox LSP enabled uploads 
        //       whose IP last for a limited time if not used.  Windows and unsecure 
        //       Xbox requests can use a cached IP address.
        //------------------------------------------------------------------------------
        virtual HRESULT GetIpLookupAgent(IpLookupAgent** ppLookupAgent) = 0;

        //------------------------------------------------------------------------------
        // Name: HttpUploadRequest::GetNextData
        // Desc: See file notes.  By default ExpectContinue returns false unless a 
        //       specialized class chooses to support this feature.
        //------------------------------------------------------------------------------
        virtual bool ExpectContinue();

    private:
        HttpUploadRequest(const HttpUploadRequest &val);
        HttpUploadRequest& operator=(const HttpUploadRequest&);
    };

};