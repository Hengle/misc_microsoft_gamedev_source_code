// ========================================================================
// $File: //jeffr/granny/rt/granny_memory.cpp $
// $DateTime: 2007/09/11 10:28:52 $
// $Change: 15926 $
// $Revision: #22 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode MemoryLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct allocation_header
{
    enum {AllocationHeaderMV = 0xCA5ECA5E};
    uint32 MagicValue;
    intaddrx Size;

    void *ActualPointer;
    intaddrx ActualSize;

    char const *SourceFileName;
    int32x SourceLineNumber;

    allocation_header *Next;
    allocation_header *Previous;
};

END_GRANNY_NAMESPACE;

/* ========================================================================
   Basic allocator
   ======================================================================== */
static allocation_header MemorySentinel =
{
    (uint32)allocation_header::AllocationHeaderMV, 0,
    0, 0,
    __FILE__, __LINE__,
    &MemorySentinel,
    &MemorySentinel,
};

static void *
GetMemoryFromHeader(allocation_header const *Header)
{
    return (void*)(Header + 1);
}

static allocation_header *
GetHeaderFromMemory(void *Memory)
{
    // Back up the pointer to the header we stored before it
    allocation_header *Header = (allocation_header *)Memory;
    --Header;

    return(Header);
}

void
SetBlockToBAADF00D(intaddrx Size, void *Pointer)
{
    SetUInt32(Size / 4, 0xBAADF00D, Pointer);
}

void *
BasicTrackedAllocate(char const *File, int32x Line,
                     intaddrx Alignment, intaddrx Size)
{
    uint8 *Memory = 0;

    // Allocate the requested space, plus space for our hidden header,
    // plus space for the alignment we might need to do.
    intaddrx const MemorySize = (sizeof(allocation_header) +
                               (Alignment - 1) + Size);

    void *ActualPointer = PlatformAllocate(MemorySize);
    if(ActualPointer)
    {
        Memory = (uint8 *)ActualPointer;
        Memory += sizeof(allocation_header);
        intaddrx Mod = (intaddrx)Memory % Alignment;
        if(Mod)
        {
            Memory += Alignment - Mod;
        }
        Assert(((intaddrx)Memory % Alignment) == 0);

        allocation_header *Header = GetHeaderFromMemory(Memory);

        // Record the information about this allocation
        Header->MagicValue = (uint32)allocation_header::AllocationHeaderMV;
        Header->ActualPointer = ActualPointer;
        Header->SourceFileName = File;
        Header->SourceLineNumber = Line;
        Header->Size = Size;
        Header->ActualSize = MemorySize;

#if DEBUG_MEMORY
        // Initialize the entire block with bogus values
        SetBlockToBAADF00D(Header->Size, Memory);
#endif

#ifdef GRANNY_THREADED
        AcquireMemorySpinlock();
#endif

        // Link us in with the rest of the memory
        Header->Next = MemorySentinel.Next;
        Header->Previous = &MemorySentinel;
        MemorySentinel.Next->Previous = Header;
        MemorySentinel.Next = Header;

#ifdef GRANNY_THREADED
        ReleaseMemorySpinlock();
#endif
    }
    else
    {
        Log4(ErrorLogMessage, MemoryLogMessage,
             "Unable to allocate %d bytes (%d requested by %s:%d).",
             MemorySize, Size, File, Line);
#if DEBUG_MEMORY
        Assert ( false );
#endif
    }

    return(Memory);
}

static bool
AllocationHeaderIsValid(allocation_header *Header)
{
    return(Header &&
           (Header->MagicValue == allocation_header::AllocationHeaderMV) &&
           (Header->Size != -1));
}

//--- TODOTHREAD: not thread safe, need a lock...
void
BasicTrackedDeallocate(char const *File, int32x Line, void *Memory)
{
    if(Memory)
    {
        // Back up the pointer to the header we stored before it
        allocation_header *Header = GetHeaderFromMemory(Memory);

        // We might be able to grab the spinlock a little later, but
        // this is slightly safer
#ifdef GRANNY_THREADED
        AcquireMemorySpinlock();
#endif

        // Make sure we actually allocated it in the first place
        if(AllocationHeaderIsValid(Header))
        {
#if DEBUG_MEMORY
            // Overwrite the entire block with bogus values
            SetBlockToBAADF00D(Header->Size, Memory);
#endif

            // Overwrite the size so we can catch quick double-frees
            Header->Size = -1;

            // Unlink
            Assert(AllocationHeaderIsValid(Header->Next));
            Assert(AllocationHeaderIsValid(Header->Previous));
            Header->Next->Previous = Header->Previous;
            Header->Previous->Next = Header->Next;

            PlatformDeallocate(Header->ActualPointer);
        }
        else
        {
            Log3(ErrorLogMessage, MemoryLogMessage,
                 "%s:%d Attempted to free 0x%x, which was not allocated "
                 "by this allocator.", File, Line, Memory);
#if DEBUG_MEMORY
            // Annoyingly, the Granny viewer does this all the time.
            //Assert ( false );
#endif
        }

#ifdef GRANNY_THREADED
        ReleaseMemorySpinlock();
#endif
    }
}

allocation_header *GRANNY
AllocationsBegin(void)
{
    return(MemorySentinel.Next);
}

allocation_header *GRANNY
NextAllocation(allocation_header *Current)
{
    // Make sure it's actually a header
    Assert(AllocationHeaderIsValid(Current));

    // Make sure people aren't trying to iterate past the end
    Assert(Current != &MemorySentinel);

    Current = Current->Next;

    // Double check to make sure the next allocation block is valid
    Assert(AllocationHeaderIsValid(Current));

    return(Current);
}

allocation_header *GRANNY
AllocationsEnd(void)
{
    return(&MemorySentinel);
}

void GRANNY
GetAllocationInformation(allocation_header const *Header,
                         allocation_information &Information)
{
    Assert(Header);
    Assert(Header->MagicValue == allocation_header::AllocationHeaderMV);

    // TODO: Report actual size for real
    Information.Memory = GetMemoryFromHeader(Header);
    Information.RequestedSize = Header->Size;
    Information.ActualSize = Header->Size + sizeof(allocation_header);
    Information.SourceFileName = Header->SourceFileName;
    Information.SourceLineNumber = Header->SourceLineNumber;
}

void *GRANNY
BeginAllocationCheck(void)
{
    return(AllocateSize(0));
}

allocation_header *GRANNY
CheckedAllocationsEnd(void *CheckIdentifier)
{
    allocation_header *Header = GetHeaderFromMemory(CheckIdentifier);
    Assert(AllocationHeaderIsValid(Header));

    return(Header);
}

bool GRANNY
EndAllocationCheck(void *CheckIdentifier)
{
    bool Result = (GetHeaderFromMemory(CheckIdentifier) ==
                   MemorySentinel.Next);
    if(!Result)
    {
#ifdef GRANNY_THREADED
        AcquireMemorySpinlock();
#endif
        {for(allocation_header *Current = AllocationsBegin();
             Current != CheckedAllocationsEnd(CheckIdentifier);
             Current = NextAllocation(Current))
        {
            allocation_information Information;
            GetAllocationInformation(Current, Information);
            Log4(WarningLogMessage, MemoryLogMessage,
                 "Unfreed block %s:%d (%d bytes at 0x%x)",
                 Information.SourceFileName,
                 Information.SourceLineNumber,
                 Information.RequestedSize,
                 Information.Memory);
        }}

#ifdef GRANNY_THREADED
        ReleaseMemorySpinlock();
#endif

        Assert(!"Blockable allocation check failed - allocations written to log");
    }

    Deallocate(CheckIdentifier);

    return(Result);
}

/* ========================================================================
   Aggregate semantics (still uses basic allocator)
   ======================================================================== */


static intaddrx
AlignOffset(intaddrx Offset, intaddrx Alignment)
{
    intaddrx Mod = Offset % Alignment;
    if(Mod)
    {
        Offset += Alignment - Mod;
    }

    Assert((Offset % Alignment) == 0);

    return(Offset);
}


void GRANNY
InitializeAggregateAllocation_(aggr_allocator *Allocator,
                               char const *File, int32x Line)
{
    ZeroStructure(*Allocator);

    Allocator->AllocationAlignment     = DefaultAllocationAlignment;
    Allocator->TotalAggregateSize      = 0;
    Allocator->NextUnusedAggregate     = 0;
}

void GRANNY
SetAggrAlignment(aggr_allocator &Allocator, intaddrx Alignment)
{
    CheckCondition(Alignment > 0, return);

    Allocator.AllocationAlignment = Alignment;
}

static int32x
PushPointerAggregate(aggr_allocator &Allocator,
                     void *&ReturnPointer, intaddrx Size)
{
    // Align to the current alignment
    Allocator.TotalAggregateSize = AlignOffset(Allocator.TotalAggregateSize,
                                                Allocator.AllocationAlignment);

    // Grab the next available aggregate record
    Assert(Allocator.NextUnusedAggregate < MaximumAggregateCount);
    int32x const AggregateIndex = Allocator.NextUnusedAggregate++;
    aggregate_allocation &Allocation = Allocator.Aggregates[AggregateIndex];

    // Fill it out
    Allocation.UseRawOffset = false;
    Allocation.WriteToPointer = &ReturnPointer;
    Allocation.WriteToOffset = 0;
    Allocation.Offset = Allocator.TotalAggregateSize;
    Allocation.Count = -1;

    Allocator.TotalAggregateSize += Size;

    // Overwrite the pointer with our offset, in case someone
    // wants to daisy-chain offsets.
    *(intaddrx *)&ReturnPointer = Allocation.Offset;

    // Return the index of the record we used
    return(AggregateIndex);
}

static int32x
PushOffsetAggregatePtr(aggr_allocator &Allocator,
                       intaddrx Offset, intaddrx Size)
{
    // Align to the current alignment
    Allocator.TotalAggregateSize = AlignOffset(Allocator.TotalAggregateSize,
                                                Allocator.AllocationAlignment);

    Assert((Offset >= 0) && (Offset < Allocator.TotalAggregateSize));
    Assert((Offset % 4) == 0);

    // Grab the next available aggregate record
    Assert(Allocator.NextUnusedAggregate < MaximumAggregateCount);
    int32x const AggregateIndex = Allocator.NextUnusedAggregate++;
    aggregate_allocation &Allocation = Allocator.Aggregates[AggregateIndex];

    // Fill it out
    Allocation.WriteToPointer = 0;
    Allocation.UseRawOffset = false;
    Allocation.WriteToOffset = Offset;
    Allocation.Offset = Allocator.TotalAggregateSize;
    Allocation.Count = -1;

    Allocator.TotalAggregateSize += Size;

    // Return the index of the record we used
    return(AggregateIndex);
}

static int32x
PushOffsetAggregatePtr(aggr_allocator &Allocator,
                       intaddrx CountOffset, intaddrx PtrOffset,
                       int32x Count, intaddrx Size)
{
    // Align to the current alignment
    Allocator.TotalAggregateSize = AlignOffset(Allocator.TotalAggregateSize,
                                                Allocator.AllocationAlignment);

    Assert((CountOffset >= 0) && (CountOffset < Allocator.TotalAggregateSize));
    Assert((CountOffset % 4) == 0);

    Assert((PtrOffset >= 0) && (PtrOffset < Allocator.TotalAggregateSize));
    Assert((PtrOffset % 4) == 0);

    // Grab the next available aggregate record
    Assert(Allocator.NextUnusedAggregate < MaximumAggregateCount);
    int32x const AggregateIndex = Allocator.NextUnusedAggregate++;
    aggregate_allocation &Allocation = Allocator.Aggregates[AggregateIndex];

    // Fill it out
    Allocation.WriteToPointer = 0;
    Allocation.UseRawOffset = false;
    Allocation.WriteToOffset = PtrOffset;
    Allocation.Offset = Allocator.TotalAggregateSize;
    Allocation.Count = Count;
    Allocation.CountOffset = CountOffset;

    Allocator.TotalAggregateSize += Size;

    // Return the index of the record we used
    return(AggregateIndex);
}

static int32x
PushOffsetAggregate(aggr_allocator &Allocator,
                    intaddrx CountOffset, intaddrx OffsetOffset,
                    int32x Count, intaddrx Size)
{
    // Align to the current alignment
    Allocator.TotalAggregateSize = AlignOffset(Allocator.TotalAggregateSize,
                                                Allocator.AllocationAlignment);

    Assert((OffsetOffset >= 0) && (OffsetOffset < Allocator.TotalAggregateSize));
    Assert((OffsetOffset % 4) == 0);

    Assert((CountOffset >= 0) && (CountOffset < Allocator.TotalAggregateSize));
    Assert((CountOffset % 4) == 0);

    // Grab the next available aggregate record
    Assert(Allocator.NextUnusedAggregate < MaximumAggregateCount);
    int32x const AggregateIndex = Allocator.NextUnusedAggregate++;
    aggregate_allocation &Allocation = Allocator.Aggregates[AggregateIndex];

    // Fill it out
    Allocation.WriteToPointer = 0;
    Allocation.UseRawOffset = true;
    Allocation.WriteToOffset = OffsetOffset;
    Allocation.Offset = Allocator.TotalAggregateSize;
    Allocation.Count = Count;
    Allocation.CountOffset = CountOffset;

    Allocator.TotalAggregateSize += Size;

    // Return the index of the record we used
    return(AggregateIndex);
}

intaddrx GRANNY
AggregateAllocate_(aggr_allocator &Allocator,
                   void **ReturnPointer, intaddrx Size)
{
    int32x const AggregateIndex = PushPointerAggregate(Allocator,
                                                       *ReturnPointer, Size);
    return (Allocator.Aggregates[AggregateIndex].Offset);
}

intaddrx GRANNY
AggregateAllocate_(aggr_allocator &Allocator,
                   intaddrx Offset, intaddrx Size)
{
    int32x const AggregateIndex =
        PushOffsetAggregatePtr(Allocator, Offset, Size);

    return (Allocator.Aggregates[AggregateIndex].Offset);
}

intaddrx GRANNY
AggregateAllocate_(aggr_allocator &Allocator,
                   void *OwnerPointer, intaddrx Offset, intaddrx Size)
{
    int32x const AggregateIndex =
        PushOffsetAggregatePtr(Allocator, (intaddrx)OwnerPointer + Offset, Size);

    return (Allocator.Aggregates[AggregateIndex].Offset);
}

intaddrx GRANNY
AggregateAllocate_(aggr_allocator &Allocator,
                   void *OwnerPointer, intaddrx CountOffset,
                   intaddrx PtrOffset, int32x Count, intaddrx UnitSize)
{
    int32x const Index =
        PushOffsetAggregatePtr(Allocator,
                               (intaddrx)OwnerPointer + CountOffset,
                               (intaddrx)OwnerPointer + PtrOffset,
                               Count, Count * UnitSize);

    return (Allocator.Aggregates[Index].Offset);
}

intaddrx GRANNY
AggregateAllocateOffset_(aggr_allocator &Allocator,
                         void *OwnerPointer, intaddrx Size, intaddrx Offset)
{
    int32x const Index =
        PushOffsetAggregate(Allocator,
                            0, (intaddrx)OwnerPointer + Offset,
                            -1, Size);

    return (Allocator.Aggregates[Index].Offset);
}

intaddrx GRANNY
AggregateAllocateOffset_(aggr_allocator &Allocator,
                         void *OwnerPointer,
                         int32x Count, intaddrx UnitSize,
                         intaddrx CountOffset, intaddrx OffsetOffset)
{
    int32x const Index =
        PushOffsetAggregate(Allocator,
                            (intaddrx)OwnerPointer + CountOffset,
                            (intaddrx)OwnerPointer + OffsetOffset,
                            Count, Count*UnitSize);

    return (Allocator.Aggregates[Index].Offset);
}

static void
WriteMemoryPointers(aggr_allocator &Allocator, void *Memory)
{
    if(Memory)
    {
        // We got the memory, so fix up all the pointers
        uint8 * const BasePointer = (uint8 *)Memory;
        {for(int32x AggregateIndex = 0;
             AggregateIndex < Allocator.NextUnusedAggregate;
             ++AggregateIndex)
        {
            aggregate_allocation &Aggregate = Allocator.Aggregates[AggregateIndex];

            Assert((Aggregate.Offset % 4) == 0);
            void *Value = Aggregate.UseRawOffset ?
                (void *)(Aggregate.Offset) :
                &BasePointer[Aggregate.Offset];

            if(Aggregate.WriteToPointer)
            {
                // This aggregate is returned by pointer
                *Aggregate.WriteToPointer = Value;
            }
            else
            {
                // This aggregate is written into another part of the
                // allocation.
                Assert((Aggregate.WriteToOffset % 4) == 0);
                void **Write = (void **)&BasePointer[Aggregate.WriteToOffset];
                void *Address = Value;
                *Write = Address;

                if(Aggregate.Count != -1)
                {
                    Assert((Aggregate.CountOffset % 4) == 0);
                    int32x *Write = (int32x *)&BasePointer[Aggregate.CountOffset];
                    *Write = Aggregate.Count;
                }
            }
        }}
    }
    else
    {
        // Set all pointers to 0 so there will be big problems if anyone
        // tries to use the memory we didn't get.

        {for(int32x AggregateIndex = 0;
             AggregateIndex < Allocator.NextUnusedAggregate;
             ++AggregateIndex)
        {
            aggregate_allocation &Aggregate = Allocator.Aggregates[AggregateIndex];
            if(Aggregate.WriteToPointer)
            {
                *Aggregate.WriteToPointer = 0;
            }
        }}
    }
}

void *GRANNY
EndAggregateAllocation_(aggr_allocator *Allocator,
                        char const *File, int32x Line)
{
    // Ensure the final size is a multiple of the specified alignment
    Allocator->TotalAggregateSize =
        AlignOffset(Allocator->TotalAggregateSize, Allocator->AllocationAlignment);

    void *Memory = AllocateCallback(
        File, Line, Allocator->AllocationAlignment, Allocator->TotalAggregateSize);

    WriteMemoryPointers(*Allocator, Memory);
    ZeroStructure(*Allocator);

    return(Memory);
}

void *GRANNY
EndAggregatePlacement_(aggr_allocator *Allocator,
                       char const *File, int32x Line, void *Memory)
{
    WriteMemoryPointers(*Allocator, Memory);
    ZeroStructure(*Allocator);

    return(Memory);
}

intaddrx GRANNY
EndAggregateSize_(aggr_allocator *Allocator,
                  char const *File, int32x Line)
{
    // Ensure the final size is a multiple of the specified alignment
    Allocator->TotalAggregateSize =
        AlignOffset(Allocator->TotalAggregateSize, Allocator->AllocationAlignment);

    intaddrx Result = Allocator->TotalAggregateSize;
    ZeroStructure(*Allocator);

    return(Result);
}

/* ========================================================================
   Utilities
   ======================================================================== */

void GRANNY
SetUInt8(intaddrx Count, uint8 Value, void *BufferInit)
{
    uint8 *Buffer = (uint8 *)BufferInit;

    while(Count--)
    {
        *Buffer++ = Value;
    }
}

void GRANNY
SetInt32(intaddrx Count, int32 Value, void *BufferInit)
{
    int32 *Buffer = (int32 *)BufferInit;

    while(Count--)
    {
        *Buffer++ = Value;
    }
}

void GRANNY
SetUInt32(intaddrx Count, uint32 Value, void *BufferInit)
{
    uint32 *Buffer = (uint32 *)BufferInit;

    while(Count--)
    {
        *Buffer++ = Value;
    }
}

void GRANNY
SetInt32x(intaddrx Count, int32x Value, void *BufferInit)
{
    int32x *Buffer = (int32x *)BufferInit;

    while(Count--)
    {
        *Buffer++ = Value;
    }
}

void GRANNY
SetUInt32x(intaddrx Count, uint32x Value, void *BufferInit)
{
    uint32x *Buffer = (uint32x *)BufferInit;

    while(Count--)
    {
        *Buffer++ = Value;
    }
}

void GRANNY
SetPtrNULL(intaddrx Count, void *BufferInit)
{
    void** Buffer = (void **)BufferInit;

    while(Count--)
    {
        *Buffer++ = NULL;
    }
}

void GRANNY
CopyStrided(intaddrx RowLength, intaddrx RowCount,
            intaddrx SourceStride, void const *SourceInit,
            intaddrx DestStride, void *DestInit)
{
    uint8 const *Source = (uint8 const *)SourceInit;
    uint8 *Dest = (uint8 *)DestInit;

    while(RowCount--)
    {
        Copy(RowLength, Source, Dest);
        Source += SourceStride;
        Dest += DestStride;
    }
}

bool GRANNY
Compare(intaddrx Count, void const *Buffer0, void const *Buffer1)
{
    uint8 const *Check0 = (uint8 const *)Buffer0;
    uint8 const *Check1 = (uint8 const *)Buffer1;
    while(Count--)
    {
        if(*Check0++ != *Check1++)
        {
            return(false);
        }
    }

    return(true);
}

void GRANNY
Reverse64(intaddrx Count, void *BufferInit)
{
    Assert((Count % 8) == 0);
    Count /= 8;

    uint64 *Buffer = (uint64 *)BufferInit;
    while(Count--)
    {
        *Buffer = Reverse64(*Buffer);
        ++Buffer;
    }
}

void GRANNY
Reverse32(intaddrx Count, void *BufferInit)
{
    Assert((Count % 4) == 0);
    Count /= 4;

    uint32 *Buffer = (uint32 *)BufferInit;
    while(Count--)
    {
        *Buffer = Reverse32(*Buffer);
        ++Buffer;
    }
}

void GRANNY
Reverse16(intaddrx Count, void *BufferInit)
{
    Assert((Count % 2) == 0);
    Count /= 2;

    if ( ( Count % 2 ) == 0 )
    {
        // We can reverse 32-bits at a time.
        Count /= 2;
        uint32 *Buffer = (uint32 *)BufferInit;
        while(Count--)
        {
            *Buffer = Reverse16_32(*Buffer);
            ++Buffer;
        }
    }
    else
    {
        uint16 *Buffer = (uint16 *)BufferInit;
        while(Count--)
        {
            *Buffer = Reverse16_16(*Buffer);
            ++Buffer;
        }
    }
}

void GRANNY
FreeBuilderResult(void *Result)
{
    Deallocate(Result);
}

allocate_callback *GRANNY AllocateCallback = BasicTrackedAllocate;
deallocate_callback *GRANNY DeallocateCallback = BasicTrackedDeallocate;

void GRANNY
GetAllocator(allocate_callback *&ReturnAllocateCallback,
             deallocate_callback *&ReturnDeallocateCallback)
{
    ReturnAllocateCallback = AllocateCallback;
    ReturnDeallocateCallback = DeallocateCallback;
}

void GRANNY
SetAllocator(allocate_callback *AllocateCallbackInit,
             deallocate_callback *DeallocateCallbackInit)
{
    AllocateCallback = AllocateCallbackInit;
    DeallocateCallback = DeallocateCallbackInit;
}

