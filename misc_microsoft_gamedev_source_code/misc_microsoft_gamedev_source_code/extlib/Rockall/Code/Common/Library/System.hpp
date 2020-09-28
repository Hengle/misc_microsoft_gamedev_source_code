#ifndef _SYSTEM_HPP_
#define _SYSTEM_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard system include files.                             */
    /*                                                                  */
    /*   The standard system include files contain various definitions  */
    /*   used throughout the system.                                    */
    /*                                                                  */
    /********************************************************************/

#include <conio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <new.h>
#include <process.h>
#include <time.h>
#if !defined(_XBOX) && !defined(_XENON)
#include <winsock2.h>
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501
#include <windows.h>
#include <windowsx.h>
#else
#include <xtl.h>
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Automatically set the debugging flag if needed.                */
    /*                                                                  */
    /*   There are various standards for enabling dedugging code.       */
    /*   Here we translate on to the standard used in this              */
    /*   application.                                                   */
    /*                                                                  */
    /********************************************************************/

#if defined(_M_IX86) && !defined(_XENON) && !defined(_PREFIX_)
#define ASSEMBLY_X86				  1
#else
#undef ASSEMBLY_X86
#endif

#ifdef _DEBUG
//#define DEBUGGING                     1
// this is too noisy unless you are really trying to debug rockall itself
#endif

#ifdef UNICODE
#undef CreateSemaphore
#undef OutputDebugString

#define CreateSemaphore				  CreateSemaphoreA
#define OutputDebugString			  OutputDebugStringA
#endif

#ifdef _WIN64
#define WINDOWS64                     1
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Automatically disable anoying warnings.                        */
    /*                                                                  */
    /*   Some of the VC compiler warning are not very helpful so        */
    /*   we disable them here.                                          */
    /*                                                                  */
    /********************************************************************/

#ifndef ALL_COMPLAINTS
#pragma warning( disable : 4073 )
#pragma warning( disable : 4074 )
#pragma warning( disable : 4097 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4121 )
#pragma warning( disable : 4127 )
#pragma warning( disable : 4291 )
#pragma warning( disable : 4505 )
#pragma warning( disable : 4509 )
#pragma warning( disable : 4511 )
#pragma warning( disable : 4512 )
#pragma warning( disable : 4514 )
#pragma warning( disable : 4701 )
#pragma warning( disable : 4702 )
#pragma warning( disable : 4706 )
#pragma warning( disable : 4710 )
#pragma warning( disable : 4711 )
#pragma warning( disable : 4800 )
#pragma warning( disable : 4996 )
#endif
#endif
