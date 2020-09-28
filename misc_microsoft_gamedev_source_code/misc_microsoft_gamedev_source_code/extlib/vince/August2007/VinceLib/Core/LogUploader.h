//-------------------------------------------------------------
//  LogUploader: Uploads a single log file
//               Currently this is the only connection point to
//               TnTLib and this will be revised soon
//
//  Created 2007/03/30 Rich Bonny <rbonny@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) Microsoft Corp.  All rights reserved.
//-------------------------------------------------------------
#pragma once
#include "VinceUtil.h"

namespace Vince 
{
    class LogUploader
    {
    public:
        LogUploader();
        ~LogUploader();
        
        bool UploadFile(
                        const char*    srcFileName,
                        const char*    titleName,
                        bool           bUseLSP,
                        DWORD          serviceID,
                        const char*    webServer,
                        const char*    aspxPage,
                        unsigned short port
                        );

    protected:
        const char* StripPathName(const char* fullPathName);
    };
}