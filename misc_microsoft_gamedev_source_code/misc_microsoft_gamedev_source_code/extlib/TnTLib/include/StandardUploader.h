//--------------------------------------------------------------------------------------
//	StandardUploader: Standard upload adapter
//
//	Created 2006/07/05 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "IUploader.h"

namespace TnT 
{
    class StandardUploader : public IUploader
    {
    public:
                 StandardUploader();
        virtual ~StandardUploader();

        virtual IpLookupAgent* GetLookupAgent();
        virtual void           ReleaseLookupAgent();

    private:
        IpLookupAgent* m_pAgent;
    };
}
