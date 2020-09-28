//--------------------------------------------------------------------------------------
// SocketHelper.h
// 
// This class can be used to start WSA if an application is not already doing this 
// work.  This class keeps a ref count of the number of times Startup/Cleanup is called.  
// The ref count is used by IsWsaStarted to determine as far as this class knows is 
// WSA started.  The ref count does not include any calls to WSAStartup made elsewhere 
// in the code.  Just like the WSA requirements, every call to Startup must be matched 
// by a call to Cleanup.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "TnTCommon.h"

namespace TnT
{

    class SocketHelper
    {
    public:
        SocketHelper(void);
        ~SocketHelper(void);

        static bool IsWsaStarted();
        static HRESULT Startup();
        static HRESULT Cleanup();

    private:
        static DWORD s_dwRefCount;
    };

};