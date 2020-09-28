// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_startup.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
#endif

#if !defined(GRANNY_STARTUP_H)
#include "granny_startup.h"
#endif

#if !defined(WIN32_GRANNY_DLL_H)
#include "win32_granny_dll.h"
#endif

/* @cdep pre
   $requires($clipfilename($file)/win32_granny_msvc_stubs.cpp)
 */

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

extern "C"
{
    BOOL WINAPI
    _DllMainCRTStartup(HANDLE ThisDLL, DWORD Operation, LPVOID)
    {
        bool Result = true;

        if(Operation == DLL_PROCESS_ATTACH)
        {
            // Whenever a process attaches to us, we want to make sure we
            // don't hear from them _every_ time they create a new thread
            // (since we don't care).  DisableThreadLibraryCalls() tells
            // Windows never to bother us with DLL_THREAD_ATTACH and
            // DLL_THREAD_DETACH calls.
            DisableThreadLibraryCalls((HINSTANCE)ThisDLL);

            if(DLLIsNotInWindowsPath(ThisDLL))
            {
                // Now we let the platform non-specific part of Granny do any
                // work it needs to do on startup.
                Result = Startup();
            }
            else
            {
                Result = false;
            }
        }
        else if(Operation == DLL_PROCESS_DETACH)
        {
            // Now we let the platform non-specific part of Granny do any
            // work it needs to do on shutdown.
            Shutdown();
        }

        return(Result);
    }
}
