/**********************************************************************

Filename    :   GSysFile_sys.h
Content     :   Win32 implementation of GFileUtilWin32 wrapper class
Created     :   April 5, 2003
Authors     :   Brendan Iribe

Copyright   :   (c) 2003-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GSysFile_sys.h"

#include <time.h>


// *** Internal thin OS wrapper for the directory interface


HANDLE  GFileUtilWin32::CreateFile(const char* pfileName,   DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES psa, 
                                  DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE htemplateFile)
{
    SysErrorModeDisabler disabler(pfileName);
    return ::CreateFile(pfileName, desiredAccess, shareMode, psa, creationDisposition, flagsAndAttributes, htemplateFile);
}

BOOL    GFileUtilWin32::DeleteFile(const char* pfileName)
{
    SysErrorModeDisabler disabler(pfileName);
    return ::DeleteFile(pfileName);
}

BOOL    GFileUtilWin32::MoveFile(const char* pexistingFileName, const char* pnewFileName)
{
    SysErrorModeDisabler disabler(pexistingFileName);
    return ::MoveFile(pexistingFileName, pnewFileName);
}

/*
UINT    GFileUtilWin32::GetTempFileName(const char* pathName, const char* prefix, UINT unique, GString* ptempFileName)
{
    if (GSystem_OSInfo.Features == GFC_OSF_PLATFORM_WIN32_WINDOWS)
    {
        GCString    pn((GCString)pathName);
        GCString    pr((GCString)prefix);
        Char        ch[MAX_PATH*2+1];

        UINT retVal = ::GetTempFileName(pn.GetPtr(), pr.GetPtr(), unique, ch);
        if (retVal==0)
            return 0;

        *ptempFileName = GCString(ch);
        return retVal;
    }
    else
    {
        wchar_t     ch[MAX_PATH*2+1];
        
        UINT retVal = ::GetTempFileNameW(pathName.GetPtr(), prefix.GetPtr(), unique, ch);
        if (retVal==0)
            return 0;

        *ptempFileName = ch;
        return retVal;
    }
}
*/

DWORD   GFileUtilWin32::GetFileAttributes(const char* pfileName)
{
    SysErrorModeDisabler disabler(pfileName);
    return ::GetFileAttributes(pfileName);
}


BOOL    GFileUtilWin32::SetFileAttributes(const char* pfileName, DWORD fileAttributes)
{
    return ::SetFileAttributes(pfileName, fileAttributes);
}


#define GFC_DIR_PATHLENGTH      1024
static char GFileUtilWin32_FilenameBuff[GFC_DIR_PATHLENGTH];

const char * GFileUtilWin32::GetCurrentDirectory()
{
#if defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
    // For XBox, we return current directory as drive 'D:'
    return "D:\\";
#else
    GFileUtilWin32_FilenameBuff[0] = 0;
    ::GetCurrentDirectory(GFC_DIR_PATHLENGTH, GFileUtilWin32_FilenameBuff);
    return GFileUtilWin32_FilenameBuff;
#endif
}


BOOL    GFileUtilWin32::CreateDirectory(const char* ppath, LPSECURITY_ATTRIBUTES psa)
{
    SysErrorModeDisabler disabler(ppath);
    return ::CreateDirectory(ppath, psa);
}

BOOL    GFileUtilWin32::RemoveDirectory(const char* ppath)
{
    SysErrorModeDisabler disabler(ppath);
    return ::RemoveDirectory(ppath);
}


HANDLE  GFileUtilWin32::FindFirstFile(const char* pfileName, LPWIN32_FIND_DATA pfindData)
{
    SysErrorModeDisabler disabler(pfileName);
    return ::FindFirstFile(pfileName, pfindData);
}
BOOL    GFileUtilWin32::FindNextFile(HANDLE hfile, LPWIN32_FIND_DATA pfindData)
{
    return  ::FindNextFile(hfile, pfindData);
}

/*
void        GFileUtilWin32::FileTimeToDateTime(const FILETIME *pt, GDateTime *pdt)
{
    FILETIME    lc;
    ::FileTimeToLocalFileTime(pt, &lc);
    SYSTEMTIME  st;
    ::FileTimeToSystemTime(&lc, &st);

    pdt->SetDateTime(st.wYear, GDate::MonthType(st.wMonth-1), (UInt8)st.wDay, 
                     (UInt8)st.wHour, (UInt8)st.wMinute, (UInt8)st.wSecond, 
                     (UInt16)st.wMilliseconds);
}
*/

// WinBase Portable Logical drives query
DWORD   GFileUtilWin32::GetLogicalDrives()
{
#if !defined(GFC_OS_XBOX) && !defined(GFC_OS_XBOX360)
    return ::GetLogicalDrives();
#else
    DWORD   drives = 0;     
    char    text[4] = "A:\\";
    DWORD   bit;

    enum XBOXDriveConstants {
        // Legitimate drive mask:
        // D
        // F, G, H, I, J, K, L, M, N, O, P, Q, R
        // T, U
        // W, X
        // Z
        XBOX_DriveMask =    (1 <<       ('D'-'A')) |
        (0x1FFF <<  ('F'-'A')) |
        (1 <<       ('T'-'A')) |
        (1 <<       ('U'-'A')) |
        (1 <<       ('W'-'A')) |
        (1 <<       ('X'-'A')) |
        (1 <<       ('Z'-'A'))
    };

    for(bit=1; bit<=XBOX_DriveMask; bit<<=1, text[0]++)
    {
        if (bit & XBOX_DriveMask)
            if (::GetFileAttributes(text) != -1)
                drives |= bit;
    }
    ::SetLastError(0);
    return drives;
#endif
}
