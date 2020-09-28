#pragma once

#include "httpuploadrequest.h"

namespace TnT
{

    class HttpStandardUploadRequest :
        public HttpUploadRequest
    {
    public:
        HttpStandardUploadRequest(void);
        virtual ~HttpStandardUploadRequest(void);

        HRESULT GetNextData(BYTE** ppBuffer, DWORD* pdwSize);
        HRESULT GetIpLookupAgent(IpLookupAgent** ppLookupAgent);
        bool ExpectContinue();

        HRESULT SetUserAgent(const char* strUserAgent);
        HRESULT SetTargetPage(const char* strTargetPage);
        HRESULT SetHost(const char* strHost);
        HRESULT AttachIpLookupAgent(IpLookupAgent* pIpLookupAgent);

    private:
        HttpStandardUploadRequest(const HttpStandardUploadRequest &val);
        HttpStandardUploadRequest& operator=(const HttpStandardUploadRequest&);

        enum DATA_POSITION
        {
            DATA_POSITION_HEADER            = 0x0,
            DATA_POSITION_END               = 0x1
        };

        DATA_POSITION m_dataPosition;
        IpLookupAgent* m_pIpLookupAgent;

        std::string m_strUserAgent;
        std::string m_strTargetPage;
        std::string m_strHost;

        std::string m_strHeader;

        HRESULT GenerateContent();

        template< typename T > friend class TestFixture;
    };

};