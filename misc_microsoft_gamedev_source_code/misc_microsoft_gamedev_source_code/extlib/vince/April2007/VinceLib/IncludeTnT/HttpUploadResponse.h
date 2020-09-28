//--------------------------------------------------------------------------------------
// HttpUploadResponse.h
// 
// This class is used to hold the response from an HTTP upload.  This is less interesting 
// for a fire and forget HTTP request, however, for an asynchronous upload or when 
// you're getting data back from the server for a request, this class is where such 
// information can be stored.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "TnTCommon.h"

namespace TnT
{

    class HttpUploadResponse
    {
    public:
        HttpUploadResponse(void);
        virtual ~HttpUploadResponse(void);

        HRESULT GetResult();
        void SetResult(HRESULT hrResult);
        int GetSocketResult();
        void SetSocketResult(int nResult);
        DWORD GetResponseCount();
        HRESULT GetResponse(DWORD dwIndex, std::string& pResponse);
        HRESULT AddResponse(const char* pResponse);

    private:
        HttpUploadResponse(const HttpUploadResponse &val);
        HttpUploadResponse& operator=(const HttpUploadResponse&);

        HRESULT m_hrResult;
        int m_nSocketResult;

        std::vector<std::string> m_vecResponses;
    };

};