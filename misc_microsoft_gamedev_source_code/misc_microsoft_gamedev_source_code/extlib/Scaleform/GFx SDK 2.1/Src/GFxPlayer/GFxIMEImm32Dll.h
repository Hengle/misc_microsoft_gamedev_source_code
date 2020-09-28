/**********************************************************************

Filename    :   GFxIMEImm32Dll.h
Content     :   Win32 Imm32 DLL wrapper for dynamic linking
Created     :   Mar 27, 2008
Authors     :   A. Bolgar

Copyright   :   (c) 2001-2008 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_IMEImm32Dll_H
#define INC_IMEImm32Dll_H

#include "GTypes.h"

#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
#include <windows.h>
#include <Imm.h>

class GFxIMEImm32Dll
{
    typedef HIMC (WINAPI *ImmGetContextPtrType)(HWND);
    typedef BOOL (WINAPI *ImmReleaseContextPtrType)(HWND, HIMC);
    typedef LONG (WINAPI *ImmGetCompositionStringAPtrType)(HIMC, DWORD, LPVOID, DWORD);
    typedef LONG (WINAPI *ImmGetCompositionStringWPtrType)(HIMC, DWORD, LPVOID, DWORD);
    typedef BOOL (WINAPI *ImmIsUIMessageAPtrType)(HWND, UINT, WPARAM, LPARAM);
    typedef BOOL (WINAPI *ImmIsUIMessageWPtrType)(HWND, UINT, WPARAM, LPARAM);
    typedef HWND (WINAPI *ImmGetDefaultIMEWndPtrType)(HWND);

    ImmGetContextPtrType            ImmGetContextPtr;
    ImmReleaseContextPtrType        ImmReleaseContextPtr;
    ImmGetCompositionStringAPtrType ImmGetCompositionStringAPtr;
    ImmGetCompositionStringWPtrType ImmGetCompositionStringWPtr;
    ImmIsUIMessageAPtrType          ImmIsUIMessageAPtr;
    ImmIsUIMessageWPtrType          ImmIsUIMessageWPtr;
    ImmGetDefaultIMEWndPtrType      ImmGetDefaultIMEWndPtr;
    HMODULE                         hImm32;
    DWORD                           ErrorCode;
public:
    GFxIMEImm32Dll() 
    {
        hImm32 = ::LoadLibraryA("imm32.dll");
        if (hImm32)
        {
            ImmGetContextPtr            = (ImmGetContextPtrType)::GetProcAddress(hImm32, "ImmGetContext");    
            ImmReleaseContextPtr        = (ImmReleaseContextPtrType)::GetProcAddress(hImm32, "ImmReleaseContext");    
            ImmGetCompositionStringAPtr = (ImmGetCompositionStringAPtrType)::GetProcAddress(hImm32, "ImmGetCompositionStringA");    
            ImmGetCompositionStringWPtr = (ImmGetCompositionStringWPtrType)::GetProcAddress(hImm32, "ImmGetCompositionStringW");    
            ImmIsUIMessageAPtr          = (ImmIsUIMessageAPtrType)::GetProcAddress(hImm32, "ImmIsUIMessageA");    
            ImmIsUIMessageWPtr          = (ImmIsUIMessageWPtrType)::GetProcAddress(hImm32, "ImmIsUIMessageW");    
            ImmGetDefaultIMEWndPtr      = (ImmGetDefaultIMEWndPtrType)::GetProcAddress(hImm32, "ImmGetDefaultIMEWnd");    
            ErrorCode = 0;
        }
        else
        {
            ErrorCode = GetLastError();
        }
    }
    ~GFxIMEImm32Dll()
    {
        if (hImm32)
            ::FreeLibrary(hImm32);
    }
    bool IsAvailable()  const   { return hImm32 != NULL; }
    UInt32 GetErrorCode() const { return (UInt32)ErrorCode; }
    void ResetErrorCode()       { ErrorCode = 0;    }

    // Imm32 functions. Note, before calling any of the following function
    // make sure Imm32 was loaded by calling to IsAvailable.
    HIMC ImmGetContext(HWND hwnd) 
    { 
        return (ImmGetContextPtr) ? ImmGetContextPtr(hwnd) : (HIMC)0; 
    }
    BOOL ImmReleaseContext(HWND hwnd, HIMC himc) 
    { 
        return (ImmReleaseContextPtr) ? ImmReleaseContextPtr(hwnd, himc) : FALSE;
    } 
    LONG ImmGetCompositionStringA(HIMC himc, DWORD p1, LPVOID p2, DWORD p3)
    {
        return (ImmGetCompositionStringAPtr) ? ImmGetCompositionStringAPtr(himc, p1, p2, p3) : 0;
    }
    LONG ImmGetCompositionStringW(HIMC himc, DWORD p1, LPVOID p2, DWORD p3)
    {
        return (ImmGetCompositionStringWPtr) ? ImmGetCompositionStringWPtr(himc, p1, p2, p3) : 0;
    }
    BOOL ImmIsUIMessageA(HWND hwnd, UINT p1, WPARAM p2, LPARAM p3)
    {
        return (ImmIsUIMessageAPtr) ? ImmIsUIMessageAPtr(hwnd, p1, p2, p3) : FALSE;
    }
    BOOL ImmIsUIMessageW(HWND hwnd, UINT p1, WPARAM p2, LPARAM p3)
    {
        return (ImmIsUIMessageWPtr) ? ImmIsUIMessageWPtr(hwnd, p1, p2, p3) : FALSE;
    }
    HWND ImmGetDefaultIMEWnd(HWND hwnd)
    {
        return (ImmGetDefaultIMEWndPtr) ? ImmGetDefaultIMEWndPtr(hwnd) : (HWND)0;
    }
};
#endif
#endif //INC_IMEImm32Dll_H
