//--------------------------------------------------------------------------------------
// XTLOnPC.cpp
//
// This module contains functions that allow most of the samples framework to compile
// on Windows using the Win32 XDK libraries.
//
// For Visual Studio.NET users, some of the secure string library is emulated here
// since on Windows, the secure string library is only present in Visual Studio 2005.
// Additionally, the ARRAYSIZE macro is included here since it is only present in VS 2005.
//
// Some of the XTL memory functions are implemented here since the XTL libraries are
// not implemented in the Win32 XDK libraries.  The memory functions simply call
// VirtualAlloc and VirtualFree on Windows.
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include <stdio.h>

#ifdef _PC

#ifndef __STDC_SECURE_LIB__

errno_t strcpy_s(
                 char *strDestination,
                 const char *strSource 
                 )
{
    strcpy( strDestination, strSource );
    return 0;
}
errno_t strcpy_s(
                 char *strDestination,
                 size_t sizeInBytes,
                 const char *strSource 
                 )
{
    strcpy( strDestination, strSource );
    return 0;
}
errno_t wcscpy_s(
                 wchar_t *strDestination,
                 size_t sizeInWords,
                 const wchar_t *strSource 
                 )
{
    wcscpy( strDestination, strSource );
    return 0;
}
errno_t wcscpy_s(
                 wchar_t *strDestination,
                 const wchar_t *strSource 
                 )
{
    wcscpy( strDestination, strSource );
    return 0;
}
int vsnprintf_s(
                char *buffer,
                size_t sizeOfBuffer,
                size_t count,
                const char *format,
                va_list argptr 
                )
{
    return _vsnprintf( buffer, sizeOfBuffer, format, argptr );
}
int vsprintf_s(
               char *buffer,
               const char *format,
               va_list argptr 
               )
{
    return vsprintf( buffer, format, argptr );
}
errno_t wcsncpy_s(
                  wchar_t *strDest,
                  const wchar_t *strSource,
                  size_t count 
                  )
{
    wcsncpy( strDest, strSource, count );
    strDest[count] = L'\0';
    return 0;
}
#define swprintf_s swprintf
#define swscanf_s swscanf
errno_t _wcslwr_s(
                  wchar_t *str
                  )
{
    _wcslwr( str );
    return 0;
}
errno_t strcat_s(
                 char *strDestination,
                 size_t sizeInBytes,
                 const char *strSource 
                 )
{
    strcat( strDestination, strSource );
    return 0;
}
errno_t strcat_s(
                 char *strDestination,
                 const char *strSource 
                 )
{
    strcat( strDestination, strSource );
    return 0;
}
errno_t wcsncat_s(
                  wchar_t *strDest,
                  const wchar_t *strSource,
                  size_t count 
                  )
{
    wcsncat( strDest, strSource, count );
    return 0;
}

#endif // __STDC_SECURE_LIB__

LPVOID
WINAPI
XPhysicalAlloc(
               IN      SIZE_T                      dwSize,
               IN      ULONG_PTR                   ulPhysicalAddress,
               IN      ULONG_PTR                   ulAlignment,
               IN      DWORD                       flProtect
               )
{
    return VirtualAlloc( NULL, dwSize, MEM_COMMIT, PAGE_READWRITE );
}

VOID
WINAPI
XPhysicalFree(
              IN      LPVOID                      lpAddress
              )
{
    VirtualFree( lpAddress, 0, MEM_RELEASE );
}

LPVOID
WINAPI
XMemAlloc(
          IN      SIZE_T                      dwSize,
          IN      DWORD                       dwAllocAttributes
          )
{
    return VirtualAlloc( NULL, dwSize, MEM_COMMIT, PAGE_READWRITE );
}

VOID
WINAPI
XMemFree(
         IN OUT  PVOID                       pAddress,
         IN      DWORD                       dwAllocAttributes
         )
{
    VirtualFree( pAddress, 0, MEM_RELEASE );
}

VOID
WINAPI
XGetVideoMode(
              OUT     PXVIDEO_MODE                pVideoMode
              )
{
    ZeroMemory( pVideoMode, sizeof( XVIDEO_MODE ) );
}

#endif // ifdef _PC
