// ========================================================================
// $File: //jeffr/granny/rt/granny_string_table.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_STRING_TABLE_H)
#include "granny_string_table.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
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

#undef SubsystemCode
#define SubsystemCode StringTableLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct string_tree_entry
{
    char *String;
    string_tree_entry *Left;
    string_tree_entry *Right;
};

#define CONTAINER_NAME string_tree
#define CONTAINER_ITEM_TYPE string_tree_entry
#define CONTAINER_ADD_FIELDS char *String
#define CONTAINER_ADD_ASSIGN(Item) (Item)->String = String;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) StringDifference((Item1)->String, (Item2)->String)
#define CONTAINER_FIND_FIELDS char const *String
#define CONTAINER_COMPARE_FIND_FIELDS(Item) StringDifference(String, (Item)->String)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_FUNCTION_DECORATE(return_type) return_type
#define CONTAINER_SUPPORT_DUPES 0
#define CONTAINER_LEFT_NAME  Left
#define CONTAINER_RIGHT_NAME Right
#define CONTAINER_ASSERT Assert
#define CONTAINER_USE_OVERLOADING 1
#define CONTAINER_EMIT_CODE 1
#define CONTAINER_MALLOC(Size) AllocateSize(Size)
#define CONTAINER_FREE(Pointer) Deallocate(Pointer)
#include "contain.inl"

struct string_table_block
{
    char *DataStart;
    char *OnePastLastData;
    string_table_block *Previous;
};

struct string_table
{
    string_tree Tree;

    int32x BlockSize;
    string_table_block *LastBlock;
};

END_GRANNY_NAMESPACE;

string_table *GRANNY
NewStringTable(void)
{
    string_table *Table = Allocate(string_table);
    if (Table)
    {
        Initialize(&Table->Tree, 0);
        Table->BlockSize = ExpectedUsablePageSize;
        Table->LastBlock = 0;
    }

    return(Table);
}

void GRANNY
FreeStringTable(string_table *Table)
{
    if(Table)
    {
        while(Table->LastBlock)
        {
            string_table_block *FreeBlock = Table->LastBlock;
            if(FreeBlock)
            {
                Table->LastBlock = FreeBlock->Previous;
                Deallocate(FreeBlock);
            }
        }

        FreeMemory(&Table->Tree);
        Deallocate(Table);
    }
}

static string_table_block *
AllocateStringBlock(int32x Size)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    string_table_block *Block;
    AggrAllocPtr(Allocator, Block);
    AggrAllocOffsetSizePtr(Allocator, Block, Size, DataStart);
    if(EndAggrAlloc(Allocator))
    {
        Block->OnePastLastData = Block->DataStart;
        Block->Previous = 0;
    }

    return(Block);
}

char *
PushLengthString(string_table &Table, int32x LengthWithoutNullTerminator, char const *String)
{
    char *Result = 0;
    string_table_block *Block = 0;

    if(!Table.BlockSize)
    {
        // This stack was never initialized
        // TODO: Platform-specific page size determination
        Table.BlockSize = 4000;
        Table.LastBlock = 0;
    }

    int32x Length = LengthWithoutNullTerminator + 1;
    if(Length > Table.BlockSize)
    {
        // This string is too big to fit in a block, so we allocate a block specifically for it
        Block = AllocateStringBlock(Length);
        if(Block)
        {
            if(Table.LastBlock)
            {
                // Since this block can't have anything else in it, insert it behind the
                // current last block so any empty space in that block can still be used
                Block->Previous = Table.LastBlock->Previous;
                Table.LastBlock->Previous = Block;
            }
            else
            {
                // There are no other blocks, so there's no harm in just shoving this one
                // in the front.
                Block->Previous = 0;
                Table.LastBlock = Block;
            }
        }
    }
    else
    {
        // See if there is space in the current block.
        int32x SpaceLeft = 0;
        if(Table.LastBlock)
        {
            const intaddrx WideSpaceLeft = (Table.BlockSize -
                                           (Table.LastBlock->OnePastLastData - Table.LastBlock->DataStart));
            CheckConvertToInt32(SpaceLeft, WideSpaceLeft, return 0);
        }

        if(Length > SpaceLeft)
        {
            // We don't have room, so make a new standard block
            Block = AllocateStringBlock(Table.BlockSize);
            if(Block)
            {
                Block->Previous = Table.LastBlock;
                Table.LastBlock = Block;
            }
        }
        else
        {
            Assert(Table.LastBlock);
            Block = Table.LastBlock;
        }
    }

    if(Block)
    {
        Result = Block->OnePastLastData;
        if(String)
        {
            Copy(LengthWithoutNullTerminator, String, Block->OnePastLastData);
        }

        // Note that we don't COPY the null terminator, we manually add it, because we
        // may have been called by something that doesn't have a null terminator on
        // the end.
        Block->OnePastLastData[LengthWithoutNullTerminator] = '\0';
        Block->OnePastLastData += Length;
    }

    return(Result);
}

char *
PushBoundedString(string_table &Table, char const *FirstChar, char const *OnePastLastChar)
{
    int32 StringLen;
    CheckConvertToInt32(StringLen, OnePastLastChar - FirstChar, return 0);
    return(PushLengthString(Table, StringLen, FirstChar));
}

char *
PushZString(string_table &Table, char const *String)
{
    return(PushLengthString(Table, StringLength(String), String));
}

char *
PushSize(string_table &Table, int32x Size)
{
    return(PushLengthString(Table, Size, 0));
}

char const *GRANNY
MapString(string_table &Table, char const *String)
{
    string_tree_entry *Entry = Find(&Table.Tree, String);
    if(Entry)
    {
        return(Entry->String);
    }
    else
    {
        char *PushedString = PushZString(Table, String);
        Add(&Table.Tree, PushedString);
        return(PushedString);
    }
}
