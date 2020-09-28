//-------------------------------------------------------------
//  LogUploader: Uploads log files on a separate thread
//
//  Created 2006/07/20 Dan Berke <dberke@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//-------------------------------------------------------------
#include "VinceCore.h"
#include "LogUploadManager.h"
#include "LogUploader.h"
#include "Settings.h"
#include "VinceUtil.h"

namespace Vince 
{
    //------------------------------------------------------------------------------
    // Log file uploader class
    //------------------------------------------------------------------------------
    LogUploadManager::LogUploadManager() :
        m_port(80),
        m_bDeleteLogAfterUpload(false),
        m_bUseLSP(false),
        m_serviceID(0),
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

        if(m_threadHandle)
        {
            WaitForSingleObject(m_threadHandle, INFINITE);
            CloseHandle(m_threadHandle);
        }
        DeleteCriticalSection(&m_lock);    
    }

    //------------------------------------------------------------------------------
    // Initialization
    //------------------------------------------------------------------------------
    void LogUploadManager::Init()
    {
        // Load configuration settings
        Settings* pSettings = VinceCore::Instance()->GetSettings();

        m_titleName             = pSettings->Fetch("Project", "Unknown");
        m_webServer             = pSettings->Fetch("UploadWebServer", "");
        m_aspxPage              = pSettings->Fetch("UploadAspxPage", "");
        m_port                  = (unsigned short)(pSettings->Fetch("WebPort", 80));
        m_bDeleteLogAfterUpload = pSettings->Fetch("DeleteLogAfterUpload", false);
        m_bUseLSP               = pSettings->Fetch("UseLSP", false);
        m_serviceID             = pSettings->Fetch("TitleID", 0x00000000);
        int cpuNum              = pSettings->Fetch("UploadCPU", -1);

        InitializeCriticalSection(&m_lock);

        // Create events to signal queue state
        m_emptyEvent   = CreateEvent(NULL, TRUE, TRUE,  NULL);
        m_wakeupEvent  = CreateEvent(NULL, TRUE, FALSE, NULL);

        // Create the upload thread
        m_threadHandle = CreateThread(NULL, 0, UploadThreadProc, this, CREATE_SUSPENDED, NULL);

        // On Xbox, move thread to the specified CPU
#ifdef _XBOX
        if(cpuNum > MAXIMUM_PROCESSORS - 1)
        {
            cpuNum = -1;
        }
        if(cpuNum < 0)
        {
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

            do
            {
                std::string filename = "";

                EnterCriticalSection(&m_lock);
                isEmpty = m_uploadQueue.empty();
                if(!isEmpty)
                {
                    filename = m_uploadQueue.front();
                    m_uploadQueue.pop();
                }
                else 
                {
                    SetEvent(m_emptyEvent);
                    ResetEvent(m_wakeupEvent);
                }
                LeaveCriticalSection(&m_lock);

                // Upload the file
                if(filename.length() != 0)
                {
                    LogUploader fileUploader;
                    bool success = fileUploader.UploadFile(filename.c_str(),
                                            m_titleName.c_str(),
                                            m_bUseLSP,
                                            m_serviceID,
                                            m_webServer.c_str(),
                                            m_aspxPage.c_str(),
                                            m_port);
                    
                    // Delete the file if the option to do so is set,
                    // but not if the upload failed.
                    if(m_bDeleteLogAfterUpload && success)
                    {
                        DeleteFile(filename.c_str());
                    }
                }
                Sleep(1000);
            } while(!isEmpty);
        }

        return 0;
    }
}
