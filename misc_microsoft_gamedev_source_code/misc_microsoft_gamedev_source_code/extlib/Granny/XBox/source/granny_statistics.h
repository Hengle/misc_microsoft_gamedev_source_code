#if !defined(GRANNY_STATISTICS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_statistics.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(StatisticsGroup);

#if GRANNY_STATISTICS
#if PROCESSOR_X86

#define FORCE_INLINE __forceinline

// X86 timer
FORCE_INLINE __int64 GetTimeStampCounter()
{
    __int64 Temp;
    __asm
    {
        rdtsc
        mov dword ptr [Temp],eax
        mov dword ptr [Temp+4],edx
    }

    return Temp;
}

#else

#define FORCE_INLINE inline

// Unsupported processor
inline int64x GetTimeStampCounter() { return 0;}

#undef GRANNY_STATISTICS
#define GRANNY_STATISTICS 0
#define NO_GRANNY_STATISTICS_ON_THIS_PLATFORM 1

#endif

#else

#define FORCE_INLINE inline

// No statistics
inline int64x GetTimeStampCounter() { return 0;}

#endif

#if GRANNY_STATISTICS
#define COUNT_BLOCK(Name) static counter_statistic *Counter__ = AddCounter(Name); timer_block TimerBlock__(Counter__)
#define COUNT_TRIGGER(Name) {static counter_statistic *Counter__ = AddCounter(Name); ++Counter__->Count;}
#else
#define COUNT_BLOCK(Name)
#define COUNT_TRIGGER(Name)
#endif

struct counter_statistic
{
    char const *Name;

    int32 Count;
    int64 TotalTime;
    int64 ChildTime;

    int32 PeakCount;
    int64 PeakTotalTime;
    int64 PeakTimeWithoutChildren;
};

EXPAPI GS_READ int32x GetCounterCount(void);
counter_statistic &GetCounter(int32x CounterIndex);
EXPAPI GS_READ real64x GetCounterTicksPerSecond(void);

counter_statistic *AddCounter(char const *Name);
EXPAPI GS_MODIFY void ResetCounters(void);
EXPAPI GS_MODIFY void ResetCounterPeaks(void);

EXPTYPE_EPHEMERAL struct counter_results
{
    char const *Name;

    int32x  Count;
    real64x TotalSeconds;
    real64x SecondsWithoutChildren;

    real64x TotalCycles;
    real64x TotalCyclesWithoutChildren;

    int32x  PeakCount;
    real64x PeakTotalSeconds;
    real64x PeakSecondsWithoutChildren;
};
EXPAPI GS_READ void GetCounterResults(int32x CounterIndex,
                                      counter_results &Results);

inline void
Reset(counter_statistic &Statistic)
{
    if(Statistic.PeakCount < Statistic.Count)
    {
        Statistic.PeakCount = Statistic.Count;
    }

    if(Statistic.PeakTotalTime < Statistic.TotalTime)
    {
        Statistic.PeakTotalTime = Statistic.TotalTime;
    }

    int64 TimeWithoutChildren = Statistic.TotalTime - Statistic.ChildTime;
    if(Statistic.PeakTimeWithoutChildren < TimeWithoutChildren)
    {
        Statistic.PeakTimeWithoutChildren = TimeWithoutChildren;
    }

    Statistic.Count = 0;
    Statistic.TotalTime = 0;
    Statistic.ChildTime = 0;
}

inline void
ResetPeaks(counter_statistic &Statistic)
{
    Statistic.PeakCount = 0;
    Statistic.PeakTotalTime = 0;
    Statistic.PeakTimeWithoutChildren = 0;
}

struct timer_block
{
    static counter_statistic *CurrentCounter;

    counter_statistic *ParentCounter;
    int64 StartClocks;

    FORCE_INLINE timer_block(counter_statistic *Counter)
    {
        // Remember who our parent was
        ParentCounter = CurrentCounter;

        // Make ourselves the active counter
        CurrentCounter = Counter;

        // Get the starting clock time
        StartClocks = GetTimeStampCounter();
    }

    FORCE_INLINE ~timer_block(void)
    {
        // Get the finishing clock time and compute the delta
        int64 EndClocks;
        EndClocks = GetTimeStampCounter();
        int64 const dCounter = EndClocks - StartClocks;

        // Add the elapsed time to our counter
        ++CurrentCounter->Count;
        CurrentCounter->TotalTime += dCounter;

        // Remove the elapsed time from our parent counter and make
        // our parent the current counter again
        CurrentCounter = ParentCounter;
        CurrentCounter->ChildTime += dCounter;
    }
};

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STATISTICS_H
#endif
