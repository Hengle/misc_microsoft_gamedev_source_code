/**********************************************************************

Filename    :   GTimer.cpp
Content     :   Provides static functions for precise timing
Created     :   June 28, 2005
Authors     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GTimer.h"

// *** Win32,Xbox,Xbox360 Specific Timer
#if (defined (GFC_OS_WIN32) || defined (GFC_OS_XBOX) || defined (GFC_OS_XBOX360) || defined(GFC_OS_WINCE))

#if defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
#include <xtl.h>
#else
#include <windows.h>
#endif


UInt64 GTimer::GetTicks()
{
#if defined (GFC_OS_XBOX360) 
    return GetTickCount();
#else
    return timeGetTime();
#endif
}


Double GTimer::TicksToSeconds(UInt64 ticks)
{
    return ((Double)(SInt64)ticks) * (1.0f / 1000.f);
}


UInt64  GTimer::GetProfileTicks()
{
    // TODO: use rdtsc?
    LARGE_INTEGER   li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}


Double  GTimer::ProfileTicksToSeconds(UInt64 ticks)
{
    LARGE_INTEGER   freq;
    QueryPerformanceFrequency(&freq);

    // UInt64 conversion not implemented
    // TBD: Is this precision good enough if Double == Float? (GFC_NO_DOUBLE).
    Double  seconds = (Double) (SInt64)ticks;
    seconds /= (Double) freq.QuadPart;  
    return seconds;
}


#else   // !GFC_OS_WIN32


// *** Other OS Specific Timer

#if defined(GFC_OS_PS3)
#include <sys/sys_time.h>
// Timer frequency
double  GTimer::s_fFreqInv;
bool    GTimer::s_bInitialized = false;

#elif defined(GFC_OS_PSP)
#include <rtcsvc.h>

#elif defined(GFC_OS_WII)
#include <revolution/os.h>

#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)
#include <dk_time.h>

#else
#include <sys/time.h>
#endif // GFC_OS_PS3

// The profile ticks implementation is just fine for a normal timer.
     
     
UInt64 GTimer::GetTicks()
{
    return GetProfileTicks();
}


Double GTimer::TicksToSeconds(UInt64 ticks)
{
    return ProfileTicksToSeconds(ticks);
}
     
// System specific GetProfileTicks

#if defined(GFC_OS_PS3)

UInt64  GTimer::GetProfileTicks()
{
    // get the frequency once
    if( !s_bInitialized )
    {
        UInt64 freq;
        freq = sys_time_get_timebase_frequency();
        s_fFreqInv =  1.0f / static_cast<double>(freq);
        s_bInitialized = 1;
    }

    // read time
    UInt64 ticks;
    asm volatile ("mftb %0" : "=r"(ticks));


    // Return microseconds.
    return ( static_cast<UInt64>( s_fFreqInv * static_cast<double>(ticks) * 1000000.0f ) );
}

#elif defined(GFC_OS_PSP)

UInt64 GTimer::GetProfileTicks()
{
    SceRtcTick ticks;
    sceRtcGetCurrentTick(&ticks);

    return static_cast<UInt64>(ticks.tick);
}

#elif defined(GFC_OS_WII)

UInt64 GTimer::GetProfileTicks()
{
    return OSTicksToMicroseconds(OSGetTime());
}

#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)

UInt64  GTimer::GetProfileTicks()
{
    return dk_tm_getCurrentTime();
}

// Other OS 
#else

UInt64  GTimer::GetProfileTicks()
{
    // TODO: prefer rdtsc when available?

    // Return microseconds.
    struct timeval tv;
    UInt64 result;
    
    gettimeofday(&tv, 0);

    result = tv.tv_sec * 1000000;
    result += tv.tv_usec;

    return result;
}

#endif // GetProfileTicks


Double  GTimer::ProfileTicksToSeconds(UInt64 ticks)
{   
    return ticks / 1000000.0;
}

#endif  // !GFC_OS_WIN32

