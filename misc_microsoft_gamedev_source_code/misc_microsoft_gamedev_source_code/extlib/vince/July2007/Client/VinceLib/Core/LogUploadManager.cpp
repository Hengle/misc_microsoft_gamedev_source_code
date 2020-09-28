//-------------------------------------------------------------
//  LogUploader: Uploads log files on a separate thread
//
//  Created 2006/07/20 Dan Berke <dberke@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//-------------------------------------------------------------
#include "LogUploadManager.h"
#include "StandardUploader.h"
#include "HttpFileUploadRequest.h"
#include "SynchronousHttpUploadManager.h"

// LSP is specific to Xbox
#ifdef _XBOX
  #include "LspUploader.h"
#endif

#ifdef _DEBUG
    #define MSGOUT(err)     OutputDebugStringA(err)
#else 
    #define MSGOUT(err)
#endif 

namespace Vince 
{
    //------------------------------------------------------------------------------
    // Log file uploader class
    //------------------------------------------------------------------------------
    LogUploadManager::LogUploadManager(const char* titleName, const char* webServer, const char* aspxPage, unsigned short port,
                                       bool bDeleteLog, bool bUseLSP, unsigned int serviceID) :
        m_titleName(titleName),
        m_webServer(webServer),
        m_aspxPage(aspxPage),
        m_port(port),
        m_bDeleteLogAfterUpload(bDeleteLog),
        m_bUseLSP(bUseLSP),
        m_serviceID(serviceID),
        m_killThread(false),
        m_threadHandle(0),
        m_emptyEvent(0),
        m_wakeupEvent(0)
    {
    }

    LogUploadManager::~LogUploadManager()
    {
        WaitForAllFinished();
        m_killThread = true;
        SetEvent(m_wakeupEvent);

        if(m_threadHandle) {
            WaitForSingleObject(m_threadHandle, INFINITE);
            CloseHandle(m_threadHandle);
        }
        DeleteCriticalSection(&m_lock);    
    }

    //------------------------------------------------------------------------------
    // Initialization
    //------------------------------------------------------------------------------
    void LogUploadManager::Init(int cpuNum)
    {
        InitializeCriticalSection(&m_lock);

        // Create events to signal queue state
        m_emptyEvent   = CreateEvent(NULL, TRUE, TRUE,  NULL);
        m_wakeupEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

        // Create the upload thread
        m_threadHandle = CreateThread(NULL, 0, UploadThreadProc, this, CREATE_SUSPENDED, NULL);

        // On Xbox, move thread to the specified CPU
#ifdef _XBOX
        if(cpuNum > MAXIMUM_PROCESSORS - 1) {
            cpuNum = -1;
        }
        if(cpuNum < 0) {
            cpuNum = GetCurrentProcessorNumber();
        }
        XSetThreadProcessor(m_threadHandle, cpuNum);
#else
        cpuNum = cpuNum;    // Avoid compiler warning
#endif

        ResumeThread(m_threadHandle);
    }

    //------------------------------------------------------------------------------
    // File queueing and waiting for completion
    //------------------------------------------------------------------------------
    void LogUploadManager::QueueFileUpload(const char* filename)
    {
        EnterCriticalSection(&m_lock);
        m_uploadQueue.push(filename);
        ResetEvent(m_emptyEvent);
        SetEvent(m_wakeupEvent);
        LeaveCriticalSection(&m_lock);
    }

    void LogUploadManager::WaitForAllFinished()
    {
        WaitForSingleObject(m_emptyEvent, INFINITE);
    }

    //------------------------------------------------------------------------------
    // Uploader thread
    //------------------------------------------------------------------------------
    DWORD WINAPI LogUploadManager::UploadThreadProc(void* param)
    {
        LogUploadManager* pUploadManager = (LogUploadManager*)param;
        return(pUploadManager->UploadThreadProcImpl());
    }

    DWORD LogUploadManager::UploadThreadProcImpl()
    {
        while(!m_killThread) {
            // sleep until wake up event is signaled
            WaitForSingleObject(m_wakeupEvent, INFINITE);
            bool isEmpty = false;

            do {
                std::string filename = "";

                EnterCriticalSection(&m_lock);
                isEmpty = m_uploadQueue.empty();
                if(!isEmpty) {
                    filename = m_uploadQueue.front();
                    m_uploadQueue.pop();
                } else {
                    SetEvent(m_emptyEvent);
                    ResetEvent(m_wakeupEvent);
                }
                LeaveCriticalSection(&m_lock);

                // Upload the file
                if(filename.length() != 0) {
                    UploadFile(filename.c_str());
                }

                Sleep(1000);
            } while(!isEmpty);
        }

        return 0;
    }

    //------------------------------------------------------------------------------
    // File upload
    //------------------------------------------------------------------------------
    void LogUploadManager::UploadFile(const char* srcFileName)
    {
        MSGOUT("Upload file: ");
        MSGOUT(srcFileName);
        MSGOUT(" ... ");

        // Get the filename without the leading path.  Allow \ and / as path separators
        const char* pDestFileName = strrchr(srcFileName, '\\');
        if(pDestFileName == NULL )
        {
            pDestFileName = strrchr(srcFileName, '/');
            if(pDestFileName == NULL ) {
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

        TnT::IUploader* pUploader;
        
        // If this is an Xbox build, select uploader to create based on the UseLSP flag
#ifdef _XBOX
        if(m_bUseLSP)
        {
            pUploader = new TnT::LspUploader(m_serviceID);
        }
        else
        {
            pUploader = new TnT::StandardUploader();
        }
#else
        pUploader = new TnT::StandardUploader();
#endif

        // Get the lookup agent and set the destination fields
        TnT::IpLookupAgent* pAgent = pUploader->GetLookupAgent();
        pAgent->SetDestinationName(m_webServer.c_str());
        pAgent->SetPort(m_port);

        // Get the IP address string
        char uploadIpAddrStr[32];
        DWORD ipStrSize = sizeof(uploadIpAddrStr);
        HRESULT hr = pAgent->GetIpAddress(uploadIpAddrStr, &ipStrSize);
        if(FAILED(hr))
        {
            delete pUploader;
            char err[256];
            _snprintf_s(err, 256, 256, "IpLookupAgent::GetIpAddress() failed, hr = 0x%08x\r\n", hr);
            MSGOUT(err);
            return;
        }

        // Create the upload request
        TnT::HttpFileUploadRequest request;
        request.AttachIpLookupAgent(pAgent);
        request.SetBoundaryName("FaBlEiNtErNeTeXpLoDeR");
        request.SetDestinationFileName(pDestFileName);
        request.SetLocalFileName(srcFileName);
        request.SetTargetPage(m_aspxPage.c_str());
        request.SetHost(uploadIpAddrStr);

        // Build the user agent string - Just "TitleName#"
        std::string userAgent;
        userAgent.append(m_titleName);
        userAgent.push_back('#');
        request.SetUserAgent(userAgent.c_str());

        // Perform the upload
        TnT::SynchronousHttpUploadManager uploader;
        hr = uploader.Initialize();
        if(FAILED(hr))
        {
//            delete pUploader;
            char err[256];
            _snprintf_s(err, 256, 256, "SynchronousHttpUploadManager::Initialize() failed, hr = 0x%08x\r\n", hr);
            MSGOUT(err);
            return;
        }
        hr = uploader.Upload(&request, NULL);
        if(FAILED(hr))
        {
 //           delete pUploader;
            char err[256];
            _snprintf_s(err, 256, 256, "SynchronousHttpUploadManager::Upload() failed, hr = 0x%08x\r\n", hr);
            MSGOUT(err);
            return;
        }

        hr = uploader.Terminate();
        if(FAILED(hr))
        {
//            delete pUploader;
            char err[256];
            _snprintf_s(err, 256, 256, "SynchronousHttpUploadManager::Terminate() failed, hr = 0x%08x\r\n", hr);
            MSGOUT(err);
            return;
        }

        // Delete the file if the option to do so is set
        request.CloseFileHandle();
        if(m_bDeleteLogAfterUpload)
        {
            DeleteFile(srcFileName);
        }

        // Free the uploader!
        // Except this causes an Access Violation, so skip for now
        // We will tolerate a small memory leak here until we move
        // off of the TnTLib uploader
//        delete pUploader;

        MSGOUT("Succeess!\n");
    }
}
