//-------------------------------------------------------------
//  LogUploader.cpp: Uploads a single log file.
//               Currently this is the only connection point to
//               TnTLib and this will be revised soon.
//
//  Created 2007/03/30 Rich Bonny <rbonny@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) Microsoft Corp.  All rights reserved.
//-------------------------------------------------------------
#include "LogUploader.h"
#include "StandardUploader.h"
#include "HttpFileUploadRequest.h"
#include "SynchronousHttpUploadManager.h"
#include "TnTCommon.h"


// LSP is specific to Xbox
#ifdef _XBOX
  #include "LspUploader.h"
#endif

namespace Vince 
{
    //------------------------------------------------------------------------------
    // Log file uploader class
    //------------------------------------------------------------------------------
    LogUploader::LogUploader()
    {
    }

    LogUploader::~LogUploader()
    {
    }

    //------------------------------------------------------------------------------
    // File upload
    //------------------------------------------------------------------------------
    bool LogUploader::UploadFile(
                                 const char*    srcFileName,
                                 const char*    titleName,
                                 bool           bUseLSP,
                                 DWORD          serviceID,
                                 const char*    webServer,
                                 const char*    aspxPage,
                                 unsigned short port
                                )
    {
        TRACE("Upload file: %s ...\n", srcFileName);

        // Get the filename without the leading path.  Allow \ and / as path separators
        const char* pDestFileName = strrchr(srcFileName, '\\');
        if(pDestFileName == NULL )
        {
            pDestFileName = strrchr(srcFileName, '/');
            if(pDestFileName == NULL )
            {
                pDestFileName = srcFileName;
            }
            else
            {
                pDestFileName++;        // Skip '/'
            }
        }
        else
        {
            pDestFileName++;    // Skip '\'
        }

        // Replace TnT calls here
        TnT::IUploader* pUploader;
        
        // If this is an Xbox build, select uploader to create based on the UseLSP flag
#ifdef _XBOX
        if(bUseLSP)
        {
            pUploader = new TnT::LspUploader(serviceID);
        }
        else
        {
            pUploader = new TnT::StandardUploader();
        }
#else
        pUploader = new TnT::StandardUploader();
        serviceID;  // not used on PC
        bUseLSP;    // not used on PC
#endif

        // Get the lookup agent and set the destination fields
        TnT::IpLookupAgent* pAgent = pUploader->GetLookupAgent();
        pAgent->SetDestinationName(webServer);
        pAgent->SetPort(port);

        // Get the IP address string
        char uploadIpAddrStr[32];
        DWORD ipStrSize = sizeof(uploadIpAddrStr);
        HRESULT hr = pAgent->GetIpAddress(uploadIpAddrStr, &ipStrSize);
        if(FAILED(hr))
        {
            delete pUploader;
            TRACE("IpLookupAgent::GetIpAddress() failed, hr = 0x%08x\r\n", hr);
            return false;
        }

        // Create the upload request
        TnT::HttpFileUploadRequest request;
        request.AttachIpLookupAgent(pAgent);
        request.SetBoundaryName("FaBlEiNtErNeTeXpLoDeR");
        request.SetDestinationFileName(pDestFileName);
        request.SetLocalFileName(srcFileName);
        request.SetTargetPage(aspxPage);
        request.SetHost(uploadIpAddrStr);

        // Build the user agent string - Just "TitleName#"
        std::string userAgent;
        userAgent.append(titleName);
        userAgent.push_back('#');
        request.SetUserAgent(userAgent.c_str());

        // Perform the upload
        TnT::SynchronousHttpUploadManager uploader;
        hr = uploader.Initialize();
        if(FAILED(hr))
        {
//            delete pUploader;
            TRACE("SynchronousHttpUploadManager::Initialize() failed, hr = 0x%08x\r\n", hr);
            return false;
        }
        hr = uploader.Upload(&request, NULL);
        if(FAILED(hr))
        {
 //           delete pUploader;
            TRACE("SynchronousHttpUploadManager::Upload() failed, hr = 0x%08x\r\n", hr);
            return false;
        }

        hr = uploader.Terminate();
        if(FAILED(hr))
        {
//            delete pUploader;
            TRACE("SynchronousHttpUploadManager::Terminate() failed, hr = 0x%08x\r\n", hr);
            return false;
        }

        // Delete the file if the option to do so is set
        request.CloseFileHandle();

        // Free the uploader!
        // Except this causes an Access Violation, so skip for now
        // We will tolerate a small memory leak here until we move
        // off of the TnTLib uploader
//        delete pUploader;

        TRACE("Success!\n");
        return true;
    }
}
