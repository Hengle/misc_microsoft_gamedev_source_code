//--------------------------------------------------------------------------------------
//	UploadInfo: Packaged upload information
//
//	Created 2006/06/08 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "IUploader.h"

namespace TicketTracker 
{
    class UploadInfo
    {
    public:
                 UploadInfo(const char *name);
        virtual ~UploadInfo();

        void SetHostName(const char* hostName);
        void SetPort(int port);
        void SetAspxPage(const char* aspxPage);
        void SetUploader(TnT::IUploader* uploader);

        const char*     GetName();
        const char*     GetHostName();
        int             GetPort();
        const char*     GetAspxPage();
        TnT::IUploader* GetUploader();

    private:
        char*           m_name;
        char*           m_hostName;
        int             m_port;
        char*           m_aspxPage;
        TnT::IUploader* m_pUploader;    
    };
}
