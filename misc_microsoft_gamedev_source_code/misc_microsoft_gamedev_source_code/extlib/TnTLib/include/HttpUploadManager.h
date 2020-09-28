//--------------------------------------------------------------------------------------
// HttpUploadManager.h
// 
// This abstract base class serves as the base interface for HTTP upload managers.  An 
// upload manager uploads a request to a server and optional provides the response.
//
// NOTE: [XNet and] WSA must be started prior to using this class.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "HttpUploadRequest.h"
#include "HttpUploadResponse.h"

namespace TnT
{

    class HttpUploadManager
    {
    public:
        HttpUploadManager(void);
        virtual ~HttpUploadManager(void);

        //------------------------------------------------------------------------------
        // Name: HttpUploadManager::Initialize
        // Desc: Initializes the class.  This method must be called before Upload.
        //------------------------------------------------------------------------------
        virtual HRESULT Initialize() = 0;

        //------------------------------------------------------------------------------
        // Name: HttpUploadManager::Upload
        // Desc: Uploads a request and provides the response.  If the response 
        //       parameter is null, only an HRESULT is returned.
        //------------------------------------------------------------------------------
        virtual HRESULT Upload(HttpUploadRequest* pHttpUploadRequest, HttpUploadResponse* pHttpUploadResponse) = 0;

        //------------------------------------------------------------------------------
        // Name: HttpUploadManager::Terminate
        // Desc: Releases an resources acquired by Initialize and Upload.  This can 
        //       be called explicitly if the client cares about the result of this call.
        //       Specialized classes should call Terminate in their destructor.
        //------------------------------------------------------------------------------
        virtual HRESULT Terminate() = 0;

    private:
        HttpUploadManager(const HttpUploadManager &val);
        HttpUploadManager& operator=(const HttpUploadManager&);
    };

};