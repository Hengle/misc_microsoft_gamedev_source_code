/**********************************************************************

Filename    :   GSysFile_sys.h
Content     :   GFileUtilWin32 wrapper class implementation
Created     :   April 5, 2003
Authors     :   Brendan Iribe

Copyright   :   (c) 2003 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GSYSFILE_SYS_H
#define INC_GSYSFILE_SYS_H

#include "GTypes.h"

// this is a win32 class only
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360) || defined(GFC_OS_XBOX) || defined(GFC_OS_WINCE)
// For XBox we include a different version of the same header
#if defined(GFC_OS_WIN32) || defined(GFC_OS_WINCE)
#include <windows.h>
#endif
//#include "GTypes.h"
#include <time.h>


// ***** Declared Classes
class GFileUtilWin32;



// *** Internal thin OS wrapper for the directory interface
class GFileUtilWin32
{
public:

#ifdef GFC_OS_WIN32
    // A simple helper class to diable/enable system error mode, if necessary
    // Disabling happens conditionally only if a drive name is involved
    class SysErrorModeDisabler
    {
        BOOL    Disabled;
        UINT    OldMode;
    public:
        SysErrorModeDisabler(const char* pfileName)
        {
            if (pfileName && (pfileName[0]!=0) && pfileName[1]==':')
            {
                Disabled = 1;
                OldMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
            }
            else
                Disabled = 0;
        }

        ~SysErrorModeDisabler()
        {
            if (Disabled) ::SetErrorMode(OldMode);
        }
    };
#else
    class SysErrorModeDisabler
    {
    public:
        SysErrorModeDisabler(const char* pfileName) { }
    };
#endif // GFC_OS_WIN32

    // Portable Unicode Win32 API wrappers

    static  HANDLE  CreateFile(const char* fileName,    DWORD desiredAccess, DWORD shareMode, LPSECURITY_ATTRIBUTES psa, 
                                                        DWORD creationDisposition, DWORD flagsAndAttributes, HANDLE htemplateFile);

    static  BOOL    DeleteFile(const char* fileName);       
    static  BOOL    MoveFile(const char* existingFileName, const char* newFileName);
    
//  static  UINT    GetTempFileName(const char* pathName, const char* prefix, UINT unique, GString* ptempFileName);

    static  DWORD   GetFileAttributes(const char* fileName);
    static  BOOL    SetFileAttributes(const char* fileName, DWORD fileAttributes);
    
    // Must use result only temporarily
    static  const char *GetCurrentDirectory();

    static  BOOL    CreateDirectory(const char* path, LPSECURITY_ATTRIBUTES psa);
    static  BOOL    RemoveDirectory(const char* path);

    static  HANDLE  FindFirstFile(const char* fileName, LPWIN32_FIND_DATA pfindData);
    static  BOOL    FindNextFile(HANDLE hfile, LPWIN32_FIND_DATA pfindData);

//  static void     FileTimeToDateTime(const FILETIME *pt, GDateTime *pdt);

    static DWORD    GetLogicalDrives(); 

};

#endif // win32 related OS

#endif // INC_GSYSFILE_SYS_H
