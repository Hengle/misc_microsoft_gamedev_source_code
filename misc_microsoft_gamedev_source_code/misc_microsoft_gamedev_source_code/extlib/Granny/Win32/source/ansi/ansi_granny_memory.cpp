// ========================================================================
// $File: //jeffr/granny/rt/ansi/ansi_granny_memory.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(ANSI_GRANNY_STD_H)
#include "ansi_granny_std.h"
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
    return(malloc(Size));
}

void GRANNY
PlatformDeallocate(void *Memory)
{
    free(Memory);
}

// We don't support multithreading with the default ansi platform

void GRANNY
AcquireMemorySpinlock()
{
#ifdef GRANNY_THREADED
#error "Granny doesn't support multithreading on this platform!"
#endif
}

void GRANNY
ReleaseMemorySpinlock()
{
#ifdef GRANNY_THREADED
#error "Granny doesn't support multithreading on this platform!"
#endif
}
