// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_system_clock.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
#endif

#if !defined(GRANNY_SYSTEM_CLOCK_H)
#include "granny_system_clock.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#define SubsystemCode ANSISubsystemLogMessage

USING_GRANNY_NAMESPACE;

void GRANNY
RequeryTimerFrequency()
{
    // NOP on ansi
}

void GRANNY
GetSystemSeconds(system_clock* Result)
{
    CheckPointerNotNull(Result, return);
    CompileAssert(SizeOf(clock_t) <= SizeOf(system_clock));

    *(clock_t *)Result = clock();
}

real32 GRANNY
GetSecondsElapsed(system_clock const &StartClock,
                  system_clock const &EndClock)
{
    return((real32)(*(clock_t *)&EndClock - *(clock_t *)&StartClock)
           / (real32)CLOCKS_PER_SEC);
}

void GRANNY
SleepForSeconds(real32 Seconds)
{
   clock_t EndTime;
   EndTime = (clock_t)(Seconds * (real32)CLOCKS_PER_SEC) + clock();
   while(clock() < EndTime);
}
