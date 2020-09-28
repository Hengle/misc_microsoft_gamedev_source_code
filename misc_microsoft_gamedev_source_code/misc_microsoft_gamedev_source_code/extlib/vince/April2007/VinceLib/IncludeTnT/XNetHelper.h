//--------------------------------------------------------------------------------------
// XNetHelper.h
// 
// This class can be used to start XNet if an application is not already doing this 
// work.  IsXnetStarted only knows if XNet is started based on the usage of this class.
// If XNetStartup or XNetCleanup is called directly somewhere else in the code this 
// method may not be correct.  Just like the XNet requirements, Startup may be called 
// more than once but it is not ref counted.  One call to Cleanup shuts down XNet.
// 
// Microsoft Game Studios Tools and Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifdef _XBOX

#include "SocketHelper.h"

namespace TnT
{

    class XNetHelper
    {
    public:
        static bool IsXnetStarted();
        static HRESULT Startup();
        static HRESULT Cleanup();

    private:
        static bool s_bIsStarted;
    };

};

#endif // _XBOX