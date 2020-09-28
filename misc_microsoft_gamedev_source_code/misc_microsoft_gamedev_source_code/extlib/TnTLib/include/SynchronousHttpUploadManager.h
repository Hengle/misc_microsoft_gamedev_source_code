//--------------------------------------------------------------------------------------
// SynchronousHttpUploadManager.h
// 
// This is a specialized upload manager.  It synchronously uploads the request to the 
// server.
//
// NOTE: [XNet and] WSA must be started prior to using this class.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "TnTCommon.h"
#include "HttpUploadManager.h"

namespace TnT
{

    class SynchronousHttpUploadManager
        : public HttpUploadManager
    {
    public:
        SynchronousHttpUploadManager(void);
        virtual ~SynchronousHttpUploadManager(void);

        HRESULT Initialize();
        HRESULT Upload(HttpUploadRequest* pHttpUploadRequest, HttpUploadResponse* pHttpUploadResponse);
        HRESULT Terminate();

    private:
        SynchronousHttpUploadManager(const SynchronousHttpUploadManager &val);
        SynchronousHttpUploadManager& operator=(const SynchronousHttpUploadManager&);

        DWORD m_dwReceiveBufferSize;

        HRESULT CreateSocket(SOCKET* pSocket);
        HRESULT DestroySocket(const SOCKET* pSocket);
        HRESULT ConnectSocket(const SOCKET* pSocket, const SOCKADDR_IN& socketAddress);

		
		template< typename T > friend class TestFixture;
    };

};