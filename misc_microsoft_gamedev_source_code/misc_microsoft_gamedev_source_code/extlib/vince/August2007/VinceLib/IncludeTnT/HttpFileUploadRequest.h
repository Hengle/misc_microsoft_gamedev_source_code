//--------------------------------------------------------------------------------------
// HttpFileUploadRequest.h
// 
// This is a specialized implementation of HttpCommonUploadRequests.  This class 
// represents a request that uploads a local file.
//
// NOTE: [XNet and] WSA must be started prior to using this class.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HttpCommonUploadRequest.h"

namespace TnT
{

    class HttpFileUploadRequest :
        public HttpCommonUploadRequest
    {
    public:
        HttpFileUploadRequest(void);
        virtual ~HttpFileUploadRequest(void);

        HRESULT SetLocalFileName(const char* strFileName);
		HRESULT CloseFileHandle();

    protected:
        HRESULT InitializeBuffer();
        HRESULT GetBytesRemaining(DWORD* pdwBytesRemaining);
        HRESULT GetNextBuffer(BYTE** ppBuffer, DWORD* pdwSize);

    private:
        HttpFileUploadRequest(const HttpFileUploadRequest &val);
        HttpFileUploadRequest& operator=(const HttpFileUploadRequest&);

        HANDLE m_hFile;
        BYTE* m_pBuffer;
        DWORD m_dwPacketSize;
        DWORD m_dwBytesRemaining;

        std::string m_strLocalFileName;

		template< typename T > friend class TestFixture;
    };

};