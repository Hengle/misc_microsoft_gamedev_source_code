//--------------------------------------------------------------------------------------
//	LSPUploader: LSP upload adapter
//
//	Created 2006/07/05 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "IUploader.h"

#ifdef _XBOX

namespace TnT 
{
    class LspUploader : public IUploader
    {
    public:
                 LspUploader(DWORD serviceID);
        virtual ~LspUploader();

        virtual IpLookupAgent* GetLookupAgent();
        virtual void           ReleaseLookupAgent();

    private:
        DWORD          m_serviceID;
        IpLookupAgent* m_pAgent;
    };
}

#endif