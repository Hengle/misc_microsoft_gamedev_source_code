//-------------------------------------------------------------
//  LogUploadManager: Uploads log files on a separate thread
//
//  Created 2006/07/20 Dan Berke <dberke@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//-------------------------------------------------------------
#pragma once

#include "VinceUtil.h"
#include <string>
#include <queue>

namespace Vince 
{
    class LogUploadManager
    {
    public:
        LogUploadManager();
        ~LogUploadManager();
        
        void Init();
        void QueueFileUpload(const char* srcFileName);
        void WaitForAllFinished();

    private:
        static DWORD WINAPI UploadThreadProc(void *param);
        DWORD               UploadThreadProcImpl();

        std::string      m_titleName;
        std::string      m_webServer;
        std::string      m_aspxPage;
        unsigned short   m_port;
        bool             m_bDeleteLogAfterUpload;
        bool             m_bUseLSP;
        unsigned int     m_serviceID;

        bool                    m_killThread;
        HANDLE                  m_threadHandle;
        HANDLE                  m_emptyEvent;
        HANDLE                  m_wakeupEvent;
        CRITICAL_SECTION        m_lock;
        std::queue<std::string> m_uploadQueue;
    };
}