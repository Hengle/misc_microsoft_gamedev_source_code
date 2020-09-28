// ========================================================================
// $File: //jeffr/granny/rt/granny_statistics.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_SYSTEM_CLOCK_H)
#include "granny_system_clock.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

static counter_statistic IgnoredInitialParent;
counter_statistic *timer_block::CurrentCounter = &IgnoredInitialParent;
static int32x CounterCount = 0;
#if GRANNY_STATISTICS
static int32x const MaximumCounterCount = 512;
static counter_statistic Counters[MaximumCounterCount];
#else
static int32x const MaximumCounterCount = 0;
static counter_statistic *Counters = 0;
#endif

int32x GRANNY
GetCounterCount(void)
{
    return(CounterCount);
}

counter_statistic &GRANNY
GetCounter(int32x CounterIndex)
{
    Assert(CounterIndex < CounterCount);
    Assert(Counters);

    return(Counters[CounterIndex]);
}

real64x GRANNY
GetCounterTicksPerSecond(void)
{
    static real64x TicksPerSecond = 0;
    if(TicksPerSecond == 0)
    {
        int64 StartCycles;
        StartCycles = GetTimeStampCounter();
        system_clock StartClock;
        GetSystemSeconds(&StartClock);

        SleepForSeconds(1.0f);

        int64 EndCycles;
        EndCycles = GetTimeStampCounter();
        system_clock EndClock;
        GetSystemSeconds(&EndClock);

        real64x const CycleDelta = (real64x)(EndCycles - StartCycles);
        real64x const SecondsDelta =
            (real64x)(GetSecondsElapsed(StartClock, EndClock));
        TicksPerSecond = (CycleDelta / SecondsDelta);
    }

    return(TicksPerSecond);
}

counter_statistic *GRANNY
AddCounter(char const *Name)
{
    // Add the counter
    Assert(Counters && (CounterCount < MaximumCounterCount));

    counter_statistic &NewCounter = Counters[CounterCount++];
    NewCounter.Name = Name;

    return(&NewCounter);
}

void GRANNY
ResetCounters(void)
{
    if(Counters)
    {
        {for(int32x CounterIndex = 0;
             CounterIndex < CounterCount;
             ++CounterIndex)
        {
            counter_statistic &CurrentCounter = Counters[CounterIndex];
            Reset(CurrentCounter);
        }}
    }
}

void GRANNY
ResetCounterPeaks(void)
{
    if(Counters)
    {
        {for(int32x CounterIndex = 0;
             CounterIndex < CounterCount;
             ++CounterIndex)
        {
            counter_statistic &CurrentCounter = Counters[CounterIndex];
            ResetPeaks(CurrentCounter);
        }}
    }
}

void GRANNY
GetCounterResults(int32x CounterIndex, counter_results &Results)
{
    counter_statistic &Counter = GetCounter(CounterIndex);

    Results.Name = Counter.Name;

    Results.Count = Counter.Count;
    Results.TotalSeconds = ((real64x)Counter.TotalTime /
                            GetCounterTicksPerSecond());
    Results.SecondsWithoutChildren = ((real64x)(Counter.TotalTime -
                                               Counter.ChildTime) /
                                      GetCounterTicksPerSecond());

    Results.TotalCycles = (real64x)Counter.TotalTime;
    Results.TotalCyclesWithoutChildren = (real64x)(Counter.TotalTime -
                                                  Counter.ChildTime);

    Results.PeakCount = Counter.PeakCount;
    Results.PeakTotalSeconds = ((real64x)Counter.PeakTotalTime /
                                GetCounterTicksPerSecond());
    Results.PeakSecondsWithoutChildren = ((real64x)Counter.PeakTimeWithoutChildren /
                                          GetCounterTicksPerSecond());
}
