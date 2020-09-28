/**********************************************************************

Filename    :   GFxLog.cpp
Content     :   
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxLog.h"

#include <stdio.h>
#include <stdarg.h>

#if defined(GFC_OS_WIN32)
    #include <windows.h>
#endif


// This virtual function receives all the messages  
void    GFxLog::LogMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
{   
    GUNUSED(messageType);

    // Generate a format string 
    char    formatBuff[2048];
    gfc_vsprintf(formatBuff, 2048, pfmt, argList);
    // Output the string
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX)
        ::OutputDebugStringA(formatBuff);
#else
        printf(formatBuff);
#endif
        
}

