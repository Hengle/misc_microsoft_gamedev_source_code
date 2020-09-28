// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_arena.cpp $
// $DateTime: 2007/08/03 14:24:40 $
// $Change: 15658 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MEMORY_ARENA_H)
#include "granny_memory_arena.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
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

BEGIN_GRANNY_NAMESPACE;

struct memory_arena
{
    int32x Alignment;
    memory_arena *Next;
};

END_GRANNY_NAMESPACE;

memory_arena *GRANNY
NewMemoryArena(void)
{
    memory_arena *NewArena = Allocate(memory_arena);
    if(NewArena)
    {
        NewArena->Alignment = 0;
        NewArena->Next = 0;
    }

    return(NewArena);
}

void GRANNY
SetArenaAlignment(memory_arena& Arena, int32x Align)
{
    CheckCondition(Align > 0, return);

    Arena.Alignment = Align;
}

void GRANNY
ClearArena(memory_arena *Arena)
{
    FreeMemoryArena(Arena->Next);
    Arena->Next = 0;
}

void GRANNY
FreeMemoryArena(memory_arena *Arena)
{
    memory_arena *ArenaIterator = Arena;
    while(ArenaIterator)
    {
        memory_arena *DeleteArena = ArenaIterator;
        ArenaIterator = ArenaIterator->Next;

        Deallocate(DeleteArena);
    }
}

void *GRANNY
MemoryArenaPush(memory_arena &Arena, int32x Size)
{
    void *Result = 0;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);
    if (Arena.Alignment != 0)
        SetAggrAlignment(Allocator, Arena.Alignment);

    memory_arena *NewChunk;
    AggrAllocPtr(Allocator, NewChunk);
    AggrAllocSizePtr(Allocator, Size, Result);
    if(EndAggrAlloc(Allocator))
    {
        NewChunk->Next = Arena.Next;
        Arena.Next = NewChunk;
    }
    else
    {
        Assert(Result == NULL);
    }

    return(Result);
}

char *GRANNY
ArenaPushString(memory_arena &Arena, char const *String)
{
    return((char *)ArenaPushBinary(
        Arena, StringLength(String) + 1, String));
}

void *GRANNY
ArenaPushBinary(memory_arena &Arena, int32x Size, void const *Data)
{
    void *Memory = MemoryArenaPush(Arena, Size);
    if(Memory)
    {
        Copy(Size, Data, Memory);
    }

    return(Memory);
}
