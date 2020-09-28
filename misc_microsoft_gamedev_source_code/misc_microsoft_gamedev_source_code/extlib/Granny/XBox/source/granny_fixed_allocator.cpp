// ========================================================================
// $File: //jeffr/granny/rt/granny_fixed_allocator.cpp $
// $DateTime: 2007/08/17 17:38:16 $
// $Change: 15766 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FIXED_ALLOCATOR_H)
#include "granny_fixed_allocator.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

static fixed_allocator_unit *
GetUnit(void *Memory)
{
    return((fixed_allocator_unit *)Memory);
}

static fixed_allocator_block *
GetOpenBlock(fixed_allocator &Allocator)
{
    {for(fixed_allocator_block *Block = Allocator.Sentinel.Next;
         Block != &Allocator.Sentinel;
         Block = Block->Next)
    {
        if(Block->FirstFreeUnit ||
           (Block->UnitCount < Allocator.UnitsPerBlock))
        {
            return(Block);
        }
    }}

    aggr_allocator AggrAllocator;
    InitializeAggrAlloc(AggrAllocator);
    SetAggrAlignment(AggrAllocator, DefaultFixedAllocatorBlockAlignment);

    fixed_allocator_block *NewBlock;
    AggrAllocPtr(AggrAllocator, NewBlock);
    AggrAllocOffsetSizePtr(AggrAllocator, NewBlock,
                           Allocator.UnitsPerBlock*Allocator.UnitSize,
                           Units);
    if(EndAggrAlloc(AggrAllocator))
    {
        NewBlock->UnitCount = 0;
        NewBlock->FirstFreeUnit = 0;
        NewBlock->Next = &Allocator.Sentinel;
        NewBlock->Previous = Allocator.Sentinel.Previous;
        NewBlock->Next->Previous = NewBlock;
        NewBlock->Previous->Next = NewBlock;
    }

    return(NewBlock);
}

static void *
AddToBlock(fixed_allocator &Allocator, fixed_allocator_block &Block)
{
    void *Result = 0;

    if(Block.FirstFreeUnit)
    {
        Result = Block.FirstFreeUnit;
        Block.FirstFreeUnit = Block.FirstFreeUnit->Next;
    }
    else
    {
        Assert(Block.UnitCount < Allocator.UnitsPerBlock);
        Result = Block.Units + (Allocator.UnitSize * Block.UnitCount);
    }

    ++Block.UnitCount;

    return(Result);
}

static fixed_allocator_block *
FindBlockFor(fixed_allocator &Allocator, void *Memory)
{
    {for(fixed_allocator_block *Block = Allocator.Sentinel.Next;
         Block != &Allocator.Sentinel;
         Block = Block->Next)
    {
        if((Memory >= Block->Units) &&
           (Memory < (Block->Units + (Allocator.UnitsPerBlock *
                                      Allocator.UnitSize))))
        {
            return(Block);
        }
    }}

    return(0);
}

static void
RemoveBlock(fixed_allocator_block *Block)
{
    if(Block)
    {
        Block->Next->Previous = Block->Previous;
        Block->Previous->Next = Block->Next;

        Deallocate(Block);
    }
}

static void
RemoveFromBlock(fixed_allocator &Allocator, fixed_allocator_block &Block,
                void *Memory)
{
    if(--Block.UnitCount == 0)
    {
        RemoveBlock(&Block);
    }
    else
    {
        fixed_allocator_unit *Unit = GetUnit(Memory);
        Unit->Next = Block.FirstFreeUnit;
        Block.FirstFreeUnit = Unit;
    }
}

void *GRANNY
AllocateFixed(fixed_allocator &Allocator)
{
    void *Result = 0;

    if(Allocator.UnitSize)
    {
        if(!(Allocator.UnitsPerBlock && Allocator.Sentinel.Next))
        {
            if(Allocator.UnitSize < SizeOf(fixed_allocator_unit))
            {
                Allocator.UnitSize = SizeOf(fixed_allocator_unit);
            }

            if(Allocator.UnitsPerBlock != 0)
            {
                Allocator.UnitsPerBlock =
                    (ExpectedUsablePageSize / Allocator.UnitSize);
            }

            if(Allocator.UnitsPerBlock < MinimumAllocationsPerFixedBlock)
            {
                Allocator.UnitsPerBlock = MinimumAllocationsPerFixedBlock;
            }

            Allocator.Sentinel.Next = &Allocator.Sentinel;
            Allocator.Sentinel.Previous = &Allocator.Sentinel;
        }

        fixed_allocator_block *Block = GetOpenBlock(Allocator);
        if(Block)
        {
            Result = AddToBlock(Allocator, *Block);
        }
    }
    else
    {
        Log0(ErrorLogMessage, FixedAllocatorLogMessage,
             "The fixed allocator cannot be used with a unit size of 0.");
    }

    return(Result);
}

void GRANNY
DeallocateFixed(fixed_allocator &Allocator, void *Memory)
{
    if(Memory)
    {
        fixed_allocator_block *Block = FindBlockFor(Allocator, Memory);
        if(Block)
        {
            RemoveFromBlock(Allocator, *Block, Memory);
        }
        else
        {
            Log1(ErrorLogMessage, FixedAllocatorLogMessage,
                 "Memory block 0x%x was not allocated by this allocator.",
                 Memory);
        }
    }
}

void GRANNY
DeallocateAllFixed(fixed_allocator &Allocator)
{
    if(Allocator.Sentinel.Next)
    {
        while(Allocator.Sentinel.Next != &Allocator.Sentinel)
        {
            RemoveBlock(Allocator.Sentinel.Next);
        }
    }
}

void GRANNY
InitializeFixedAllocator(fixed_allocator &Allocator, int32x UnitSize)
{
    SetUInt8(SizeOf(Allocator), 0, &Allocator);
    Allocator.UnitSize = UnitSize;
}
