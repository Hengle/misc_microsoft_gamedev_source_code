// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_assert.cpp $
// $DateTime: 2007/03/05 10:53:12 $
// $Change: 14497 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if DEBUG
#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

static char AssertionBuffer[MaximumMessageBufferSize];
void GRANNY
DisplayAssertion(char const * const Expression,
                 char const * const File,
                 int32x const LineNumber,
                 char const * const Function,
                 bool* IgnoredPtr)
{
    bool& Ignored = *IgnoredPtr;
    if(!Ignored)
    {
        wsprintf(
            AssertionBuffer,
            "Expression: %s\r\n"
            "File: %s\r\n"
            "Line: %d\r\n"
            "Function: %s\r\n"
            "\r\n"
            "(Abort = debug, Retry = ignore once, Ignore = ignore forever)\r\n",
            Expression, File, LineNumber, Function);

        DWORD ThreadID;
        MSGBOXPARAMS MessageBoxParameters;
        MessageBoxParameters.cbSize = sizeof(ThreadID);
        MessageBoxParameters.hwndOwner = 0;
        MessageBoxParameters.hInstance = 0;
        MessageBoxParameters.lpszText = AssertionBuffer;
        MessageBoxParameters.lpszCaption = "Run-time Error";
        MessageBoxParameters.dwStyle = MB_ABORTRETRYIGNORE | MB_TASKMODAL;
        MessageBoxParameters.lpszIcon = IDI_ERROR;
        MessageBoxParameters.dwContextHelpId = 0;
        MessageBoxParameters.lpfnMsgBoxCallback = 0;
        MessageBoxParameters.dwLanguageId = 0;

        // Many thanks to Todd Laney for pointing out the following "correct"
        // way to pop up an assertion box such that the asserting thread
        // will no longer get paint requests or other events that could
        // trigger recursive asserting.

        HANDLE Thread =
            CreateThread(0, 0,
                         (LPTHREAD_START_ROUTINE)MessageBoxIndirect,
                         (LPVOID)&MessageBoxParameters,
                         0, &ThreadID);
        if(Thread)
        {
            WaitForSingleObject(Thread, INFINITE);
            DWORD ExitCode = IDABORT;
            GetExitCodeThread(Thread, &ExitCode);
            CloseHandle(Thread);

            switch(ExitCode)
            {
                case IDABORT:
                {
                    // We break into the debugger
//                    wsprintf(AssertionBuffer,
//                             "d:\\apps\\Emacs\\EmacsW32\\bin\\gnudoit.exe "
//                             "(find-file \\\"%s\\\") (goto-line %d) (raise-frame)",
//                             File, LineNumber);
//                    WinExec(AssertionBuffer, SW_MINIMIZE);

                    __asm { int 3 }
                } break;

                case IDIGNORE:
                {
                    // We don't ever assert here again
                    Ignored = true;
                } break;

                case IDRETRY:
                {
                    // We let things go
                } break;
            }
        }
        else
        {
            // We're unable to assert
            __asm { int 3 }
        }
    }
}
#endif
