// ========================================================================
// $File: //jeffr/granny/rt/xenon/xenon_granny_memory.cpp $
// $DateTime: 2007/06/19 15:05:39 $
// $Change: 15230 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(XBOX_GRANNY_XTL_H)
#include "xenon_granny_xtl.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

void *GRANNY
PlatformAllocate(intaddrx Size)
{
    return(HeapAlloc(GetProcessHeap(), 0, Size));
}

void GRANNY
PlatformDeallocate(void *Memory)
{
    HeapFree(GetProcessHeap(), 0, Memory);
}


// The spinlock is nothing more than an integer
LONG MemorySpinlockInteger = 0;

void GRANNY
AcquireMemorySpinlock()
{
#ifdef GRANNY_THREADED
    //--- todo: slow, should sleep after a certain number of loops?
    while (InterlockedCompareExchangeAcquire(&MemorySpinlockInteger, 1, 0) == 1)
    {
        // loop
    }
#endif
}

void GRANNY
ReleaseMemorySpinlock()
{
#ifdef GRANNY_THREADED

    // The spinlock had better be 1, since we own it.
    Assert(MemorySpinlockInteger == 1);

    // Since we own it, we can just set it to 0
    MemorySpinlockInteger = 0;
#endif
}
