//--------------------------------------------------------------------------------------
// HttpCommonUploadRequest.h
// 
// This is a specialized implementation of HttpUploadRequest.  This abstract base class 
// servers as the base interface and common implementation of HTTP requests that 
// contain additional content.  Specialized classes must implement the protected 
// members InitializeBuffer, GetBytesRemaining, and GetNextBuffer to supply the 
// additional content to this common implementation.  The data provided to this common 
// implementation is used to create the HTTP header, MIME header, and MIME footer.
// The only data provided by the specialized class for the header is acquired after 
// InitializeBuffer is called by calling GetBytesRemaining.  This essentially provides 
// the content size needed for the Content Length attribute of the HTTP header.  This 
// implementation only allows you to upload one boundary of additional content.
//
// DestinationFileName is the desired name of the file when it is saved on the server. 
// The server can ignore this name at its discretion.  BoundaryName is the name to 
// use for the MIME boundary.  Unless the server expects a specific boundary name 
// this name is arbitrary.  Host is the name of the target server.  Target page is 
// the path and name of the page being targeted by the request e.g. 
// /MyVirtualDirectory/upload.aspx.  UserAgent is typically the name of the calling 
// client.
//
// Vince and TicketTracker use the User Agent attribute to specify subdirectories 
// where the file is stored.  Vince uses TitleName#SubDirectory.  TicketTracker 
// uses TitleName#SessionId.
//
// NOTE: [XNet and] WSA must be started prior to using this class.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HttpUploadRequest.h"

namespace TnT
{

    class HttpCommonUploadRequest
        : public HttpUploadRequest
    {
    public:
        HttpCommonUploadRequest(void);
        virtual ~HttpCommonUploadRequest(void);

        HRESULT GetNextData(BYTE** ppBuffer, DWORD* pdwSize);
        HRESULT GetIpLookupAgent(IpLookupAgent** ppLookupAgent);
        bool ExpectContinue();

        HRESULT SetDestinationFileName(const char* strDestinationFileName);
        HRESULT SetUserAgent(const char* strUserAgent);
        HRESULT SetBoundaryName(const char* strBoundaryName);
        HRESULT SetTargetPage(const char* strTargetPage);
        HRESULT SetHost(const char* strHost);
        HRESULT AttachIpLookupAgent(IpLookupAgent* pIpLookupAgent);

    protected:
        virtual HRESULT InitializeBuffer() = 0;
        virtual HRESULT GetBytesRemaining(DWORD* pdwBytesRemaining) = 0;
        virtual HRESULT GetNextBuffer(BYTE** ppBuffer, DWORD* pdwSize) = 0;

    private:
        HttpCommonUploadRequest(const HttpCommonUploadRequest &val);
        HttpCommonUploadRequest& operator=(const HttpCommonUploadRequest&);

        enum DATA_POSITION
        {
            DATA_POSITION_HEADER            = 0x0,
            DATA_POSITION_MIME_HEADER       = 0x1,
            DATA_POSITION_FILE              = 0x2,
            DATA_POSITION_MIME_FOOTER       = 0x3,
            DATA_POSITION_END               = 0x4
        };

        DATA_POSITION m_dataPosition;
        IpLookupAgent* m_pIpLookupAgent;

        std::string m_strDestinationFileName;
        std::string m_strUserAgent;
        std::string m_strBoundaryName;
        std::string m_strTargetPage;
        std::string m_strHost;

        std::string m_strHeader;
        std::string m_strMimeHeader;
        std::string m_strMimeFooter;

        HRESULT GenerateContent();

		template< typename T > friend class TestFixture;
    };

};