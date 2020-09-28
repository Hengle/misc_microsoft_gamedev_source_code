// ========================================================================
// $File: //jeffr/granny/rt/win32/win32_granny_memory.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #10 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(WIN32_GRANNY_WINDOWS_H)
#include "win32_granny_windows.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#define SubsystemCode MemoryLogMessage

USING_GRANNY_NAMESPACE;

#if DEBUG_MEMORY

static int32x ExtraBytes = 12;
static int32x PageSize = 0;
static uint32x VirtualAllocationMagicValue = 0xCA5ECA5E;
static uint32x RegularAllocationMagicValue = 0xCA5ECA5F;
static int32x TotalAllocationCount = 0;

void *GRANNY
PlatformAllocate(intaddrx Size)
{
    void *Result = 0;

    // Find out roughly how much memory is free.
    // Though memory may be really fragmented, so this will be an underestimate.
    static bool FirstTime = true;
    static int LessMemoryThan = 0;
    static int MoreMemoryThan = 0;
    if ( FirstTime )
    {
        FirstTime = false;
        // Only reliable way to find out roughly how much memory is available!
        HANDLE CurrentProcess = GetProcessHeap();
        int ChunkSize = 1024;
        while ( true )
        {
            void *Result = HeapAlloc(CurrentProcess, 0, ChunkSize);
            if ( Result != NULL )
            {
                MoreMemoryThan = ChunkSize;
                HeapFree ( CurrentProcess, 0, Result );
            }
            else
            {
                LessMemoryThan = ChunkSize;
                break;
            }
            ChunkSize += ChunkSize >> 1;
        }
    }

    if(TotalAllocationCount < 10000)
    {
        // Ask the operating system for the vm page size, which we store
        // in a global so we only have to grab it once.
        if(PageSize == 0)
        {
            SYSTEM_INFO si;
            GetSystemInfo(&si);
            PageSize = si.dwPageSize;
        }

        // To make our guard page work, we need to allocate enough space for
        // our extra storage bytes, the allocation, and at least one full
        // page of extra memory.  We use 2*PageSize to guarentee we get a
        // full extra page, but you could theoretically use just PageSize
        // if you trust the virtual allocator never to allocate out of the
        // remainder of your trailing page (which it probably never does on
        // most OSes).
        intaddrx AllocateSize = ((Size + ExtraBytes + 2*PageSize - 1) &
                                 ~(PageSize-1));

        void *PagePointer = VirtualAlloc(0, AllocateSize,
                                         MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

        if(PagePointer)
        {
            // Calculate the offset from our base page pointer to the
            // boundary between the part you can touch (all bytes before)
            // and the guard page (this byte and all bytes after)
            uintaddrx PageBoundaryOffset = (Size + ExtraBytes +
                                            (PageSize - 1)) & ~(PageSize - 1);

            // Use the offset to protect the entire guard page against
            // reading and writing
            DWORD Ignored;
            VirtualProtect((uint8 *)PagePointer + PageBoundaryOffset,
                           PageSize, PAGE_NOACCESS, &Ignored);

            // The actual memory location returned is the location of the
            // page boundary minus the requested size, thus ensuring that
            // a single byte overrun will immediately fault.  This may
            // unalign the memory location, so pre-align your allocation
            // sizes if you care.
            Result = (uint8 *)PagePointer + (PageBoundaryOffset - Size);

            // Now we shove our original base pointer, the total size we
            // allocated, and a super-secret magic value that no one knows
            // but us into the preceeding extra bytes.  PlatformDeallocate
            // will grab these and use them.
            ((uint32 *)Result)[-1] = VirtualAllocationMagicValue;
            ((uint32 *)Result)[-2] = (uint32)AllocateSize;
            ((void **)Result)[-3] = PagePointer;

            Assert((intaddrx)((uint32 *)Result)[-2] == AllocateSize);
        }
    }
    else
    {
        Assert(!"Out of virtual memory for debug allocator - "
               "all further allocations will not be guarded.");

        Result = HeapAlloc(GetProcessHeap(), 0, Size + 4);
        if(Result)
        {
            Result = (uint8 *)Result + 4;
            ((uint32 *)Result)[-1] = RegularAllocationMagicValue;
        }
    }

    if(Result)
    {
        ++TotalAllocationCount;
    }

    return(Result);
}

void GRANNY
PlatformDeallocate(void *Memory)
{
    if(Memory)
    {
        uint32x MagicValue = ((uint32 *)Memory)[-1];
        if(MagicValue == VirtualAllocationMagicValue)
        {
            int32x AllocateSize = ((uint32 *)Memory)[-2];
            void *PagePointer = ((void **)Memory)[-3];

            Assert(AllocateSize != 0);
            Assert(PagePointer != 0);

            if(PagePointer)
            {
#if 0
                VirtualFree(PagePointer, AllocateSize, MEM_DECOMMIT);

                DWORD Ignored;
                VirtualProtect(PagePointer, AllocateSize,
                               PAGE_NOACCESS, &Ignored);
#else
                VirtualFree(PagePointer, 0, MEM_RELEASE);
#endif

                --TotalAllocationCount;
            }
        }
        else
        {
            Assert(MagicValue == RegularAllocationMagicValue);
            HeapFree(GetProcessHeap(), 0, (uint8 *)Memory - 4);

            --TotalAllocationCount;
        }
    }
}

#else

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

#endif


// The spinlock is nothing more than an integer
LONG volatile MemorySpinlockInteger = 0;

void GRANNY
AcquireMemorySpinlock()
{
#ifdef GRANNY_THREADED

#if _MSC_VER > 1200
    //--- todo: slow, should sleep after a certain number of loops?
    while (InterlockedCompareExchange(&MemorySpinlockInteger, 1, 0) == 1)
    {
        // loop
    }
#else
    CompileAssert(sizeof(PVOID) == sizeof(LONG));

    //--- todo: slow, should sleep after a certain number of loops?
    while (InterlockedCompareExchange((PVOID*)&MemorySpinlockInteger, (PVOID)1, (PVOID)0) == (PVOID)1)
    {
        // loop
    }

#endif

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

