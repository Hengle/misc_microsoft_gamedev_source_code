//--------------------------------------------------------------------------------------
// HttpByteUploadRequest.h
// 
// This is a specialized implementation of HttpCommonUploadRequests.  This class 
// represents a request that uploads buffer.  When the buffer is set, an internal 
// copy is created to ensure the buffer does not go away before it is used.
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

    class HttpByteUploadRequest
        : public HttpCommonUploadRequest
    {
    public:
        HttpByteUploadRequest(void);
        virtual ~HttpByteUploadRequest(void);

        HRESULT SetBuffer(BYTE* pBuffer, DWORD dwSize);

    protected:
        HRESULT InitializeBuffer();
        HRESULT GetBytesRemaining(DWORD* pdwBytesRemaining);
        HRESULT GetNextBuffer(BYTE** ppBuffer, DWORD* pdwSize);

    private:
        HttpByteUploadRequest(const HttpByteUploadRequest &val);
        HttpByteUploadRequest& operator=(const HttpByteUploadRequest&);

        BYTE* m_pBuffer;
        DWORD m_dwPacketSize;
        DWORD m_dwBytesRemaining;
        DWORD m_dwCurrentPosition;

		
		template< typename T > friend class TestFixture;
    };

};