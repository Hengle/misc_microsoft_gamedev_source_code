// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_system_clock.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(XENON_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
#endif

#if !defined(GRANNY_SYSTEM_CLOCK_H)
#include "granny_system_clock.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void GRANNY
RequeryTimerFrequency()
{
    // NOP on xenon
}


void GRANNY
GetSystemSeconds(system_clock* Result)
{
    Assert(Result);
#if 0
    Result->Data[0] = GetTickCount();
#else
    Assert(SizeOf(LARGE_INTEGER) <= SizeOf(system_clock));
    QueryPerformanceCounter((LARGE_INTEGER *)Result);
#endif
}

real32 GRANNY
GetSecondsElapsed(system_clock const &StartClock,
                  system_clock const &EndClock)
{
#if 0
    DWORD Start = StartClock.Data[0];
    DWORD End = EndClock.Data[0];
    if(Start < End)
    {
        return((real32)(End - Start) / 1000.0f);
    }
    else
    {
        // TODO: Proper code for handling wrapping
        return(0);
    }
#else
    static real64x TimerSecondsConversion = 0;
    if(TimerSecondsConversion == 0)
    {
        LARGE_INTEGER Frequency;
        QueryPerformanceFrequency(&Frequency);
        TimerSecondsConversion = 1.0 / (real64x)Frequency.QuadPart;
    }

    LARGE_INTEGER *Start = (LARGE_INTEGER *)&StartClock;
    LARGE_INTEGER *End = (LARGE_INTEGER *)&EndClock;

    if(Start->QuadPart < End->QuadPart)
    {
        real64x const Difference = (real64x)(End->QuadPart - Start->QuadPart);
        real32 const SecondsElapsed = (real32)(Difference * TimerSecondsConversion);
        return(SecondsElapsed);
    }
    else
    {
        // TODO: Proper code for handling wrapping
        return(0);
    }
#endif
}

void GRANNY
SleepForSeconds(real32 Seconds)
{
    Sleep(RoundReal32ToInt32(Seconds * 1000.0f));
}
