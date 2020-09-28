//--------------------------------------------------------------------------------------
//	IUploader: Interface to an uploade adapter
//
//	Created 2006/07/05 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "IpLookupAgent.h"

namespace TnT 
{
    class IUploader
    {
    public:
                 IUploader() {}
        virtual ~IUploader() {}

        virtual IpLookupAgent* GetLookupAgent()     = 0;
        virtual void           ReleaseLookupAgent() = 0;
    };
}
