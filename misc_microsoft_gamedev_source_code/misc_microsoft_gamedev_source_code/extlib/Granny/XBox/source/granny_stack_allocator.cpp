// ========================================================================
// $File: //jeffr/granny/rt/granny_stack_allocator.cpp $
// $DateTime: 2007/10/17 16:02:02 $
// $Change: 16313 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_STACK_ALLOCATOR_H)
#include "granny_stack_allocator.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct allocated_block
{
    int32x UsedUnitCount;
    uint8 *Base;

    int32x FirstIndex;

    allocated_block *Previous;
};

END_GRANNY_NAMESPACE;

static allocated_block *
AllocateBlock(stack_allocator &Allocator)
{
    int32x BlockSize = Allocator.UnitSize * Allocator.UnitsPerBlock;

    aggr_allocator AggrAllocator;
    InitializeAggrAlloc(AggrAllocator);

    allocated_block *Block;
    AggrAllocPtr(AggrAllocator, Block);
    AggrAllocOffsetSizePtr(AggrAllocator, Block, BlockSize, Base);
    if(EndAggrAlloc(AggrAllocator))
    {
        Block->UsedUnitCount = 0;
        Block->FirstIndex = 0;
        Block->Previous = 0;
    }

    return(Block);
}

void GRANNY
StackInitialize(stack_allocator &Allocator, int32x UnitSize, int32x UnitsPerBlock)
{
    Allocator.UnitSize = UnitSize;
    Allocator.UnitsPerBlock = UnitsPerBlock;
    Allocator.TotalUsedUnitCount = 0;
    Allocator.LastBlock = 0;
    Allocator.MaxUnits = -1;
    Allocator.ActiveBlocks = -1;
    Allocator.MaxActiveBlocks = -1;
    Allocator.BlockDirectory = 0;
}

void GRANNY
StackInitializeWithDirectory(stack_allocator &Allocator, int32x UnitSize, int32x UnitsPerBlock, int32x MaxUnits)
{
    Assert(MaxUnits >= 0);

    Allocator.UnitSize = UnitSize;
    Allocator.UnitsPerBlock = UnitsPerBlock;
    Allocator.TotalUsedUnitCount = 0;
    Allocator.LastBlock = 0;

    Allocator.MaxUnits = MaxUnits;
    Allocator.ActiveBlocks = 0;
    Allocator.MaxActiveBlocks = (MaxUnits + (UnitsPerBlock-1)) / UnitsPerBlock;;
    Allocator.BlockDirectory = AllocateArray(Allocator.MaxActiveBlocks, allocated_block*);
    ZeroArray(Allocator.MaxActiveBlocks, Allocator.BlockDirectory);
}

void GRANNY
StackCleanUp(stack_allocator &Allocator)
{
    PopStackUnits(Allocator, Allocator.TotalUsedUnitCount);
    Assert(Allocator.TotalUsedUnitCount == 0);
    Assert(Allocator.LastBlock == 0);

    if (Allocator.BlockDirectory)
    {
        Deallocate(Allocator.BlockDirectory);
        Allocator.BlockDirectory = 0;
    }
}

int32x GRANNY
GetStackUnitCount(stack_allocator const &Allocator)
{
    return(Allocator.TotalUsedUnitCount);
}

void* GRANNY
NewStackUnit(stack_allocator &Allocator, int32x &Result)
{
    allocated_block *InBlock = Allocator.LastBlock;

    if(!InBlock || (InBlock->UsedUnitCount == Allocator.UnitsPerBlock))
    {
        if (Allocator.BlockDirectory == 0 ||
            (Allocator.ActiveBlocks < Allocator.MaxActiveBlocks))
        {
            InBlock = AllocateBlock(Allocator);
        }

        if(InBlock)
        {
            InBlock->FirstIndex = Allocator.TotalUsedUnitCount;
            InBlock->Previous = Allocator.LastBlock;
            Allocator.LastBlock = InBlock;

            if (Allocator.BlockDirectory != 0)
            {
                Allocator.BlockDirectory[Allocator.ActiveBlocks++] = InBlock;
            }
        }
    }

    void* NewItem = NULL;
    Result = -1;
    if(InBlock)
    {
        NewItem = InBlock->Base + (InBlock->UsedUnitCount * Allocator.UnitSize);
        ++InBlock->UsedUnitCount;
        Result = Allocator.TotalUsedUnitCount++;
    }

    return NewItem;
}


bool GRANNY
MultipleNewStackUnits(stack_allocator &Allocator,
                      int32x NumNewIndices,
                      int32x &NewIndicesStart,
                      void const* InitialValue)
{
    // Todo: work in blocks...
    // Very simple implementation for now

    if (NumNewIndices <= 0)
        return false;

    // First index is special
    int32x CurrentIndex;
    if (!NewStackUnit(Allocator, CurrentIndex))
        return false;

    if (InitialValue != 0)
    {
        Copy(Allocator.UnitSize, InitialValue, GetStackUnit(Allocator, CurrentIndex));
    }
    NewIndicesStart = CurrentIndex;
    NumNewIndices--;

    while (NumNewIndices--)
    {
        void* Item = NewStackUnit(Allocator, CurrentIndex);
        if (!Item)
            return false;

        if (InitialValue != 0)
            Copy(Allocator.UnitSize, InitialValue, Item);
    }

    return true;
}


void *GRANNY
GetStackUnit(stack_allocator &Allocator, int32x Index)
{
    Assert(Index >= 0);
    Assert(Index < Allocator.TotalUsedUnitCount);

    allocated_block *Block;
    if (Allocator.BlockDirectory == 0)
    {
        Block = Allocator.LastBlock;
        while(Block->FirstIndex > Index)
        {
            Block = Block->Previous;
        }

    }
    else
    {
        int32x DirectoryIndex = (Index / Allocator.UnitsPerBlock);
        Assert(DirectoryIndex < Allocator.ActiveBlocks);

        Block = Allocator.BlockDirectory[DirectoryIndex];
        Assert(Block->FirstIndex <= Index);
    }

    Index -= Block->FirstIndex;
    return(Block->Base + (Index * Allocator.UnitSize));
}

void GRANNY
PopStackUnits(stack_allocator &Allocator, int32x UnitCount)
{
    Assert(UnitCount <= Allocator.TotalUsedUnitCount);

    while(Allocator.LastBlock &&
          (Allocator.LastBlock->UsedUnitCount <= UnitCount))
    {
        Allocator.TotalUsedUnitCount -= Allocator.LastBlock->UsedUnitCount;
        UnitCount -= Allocator.LastBlock->UsedUnitCount;

        allocated_block *DeleteBlock = Allocator.LastBlock;
        Allocator.LastBlock = Allocator.LastBlock->Previous;
        Deallocate(DeleteBlock);

        if (Allocator.BlockDirectory != 0)
        {
            Assert(Allocator.ActiveBlocks > 0);
            Allocator.BlockDirectory[Allocator.ActiveBlocks - 1] = 0;
            Allocator.ActiveBlocks--;
        }
    }

    if(Allocator.LastBlock)
    {
        Assert(Allocator.LastBlock->UsedUnitCount > UnitCount);
        Allocator.TotalUsedUnitCount -= UnitCount;
        Allocator.LastBlock->UsedUnitCount -= UnitCount;
        UnitCount = 0;
    }
    else
    {
        Assert(UnitCount == 0);
    }

    Assert(UnitCount == 0);
}

void GRANNY
SerializeStack(stack_allocator const &Allocator, void *DestInit)
{
    uint8 *Dest = (uint8 *)DestInit;
    Dest += GetStackUnitCount(Allocator) * Allocator.UnitSize;
    {for(allocated_block *Block = Allocator.LastBlock;
         Block;
         Block = Block->Previous)
    {
        int32x const BlockSize = (Block->UsedUnitCount *
                                  Allocator.UnitSize);
        Dest -= BlockSize;
        Copy(BlockSize, Block->Base, Dest);
    }}

    Assert(Dest == DestInit);
}
