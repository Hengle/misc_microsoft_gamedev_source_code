// ========================================================================
// $File: //jeffr/granny/rt/granny_sieve.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_SIEVE_H)
#include "granny_sieve.h"
#endif

#if !defined(GRANNY_STACK_ALLOCATOR_H)
#include "granny_stack_allocator.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct sieve
{
    int32x GroupCount;
    stack_allocator *Groups;
};

END_GRANNY_NAMESPACE;

sieve *GRANNY
NewSieve(int32x GroupCount, int32x UnitSize)
{
    sieve *Sieve;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    AggrAllocPtr(Allocator, Sieve);
    AggrAllocOffsetArrayPtr(Allocator, Sieve, GroupCount, GroupCount, Groups);
    if(EndAggrAlloc(Allocator))
    {
        {for(int32x GroupIndex = 0;
             GroupIndex < GroupCount;
             ++GroupIndex)
        {
            StackInitialize(Sieve->Groups[GroupIndex], UnitSize, 1024);
        }}
    }

    return(Sieve);
}

int32x GRANNY
GetUsedGroupCount(sieve &Sieve)
{
    int32x Count = 0;

    {for(int32x GroupIndex = 0;
         GroupIndex < Sieve.GroupCount;
         ++GroupIndex)
    {
        if(GetStackUnitCount(Sieve.Groups[GroupIndex]) > 0)
        {
            ++Count;
        }
    }}

    return(Count);
}

void GRANNY
ClearSieve(sieve &Sieve)
{
    {for(int32x GroupIndex = 0;
         GroupIndex < Sieve.GroupCount;
         ++GroupIndex)
    {
        StackCleanUp(Sieve.Groups[GroupIndex]);
    }}
}

void GRANNY
FreeSieve(sieve *Sieve)
{
    if(Sieve)
    {
        ClearSieve(*Sieve);
        Deallocate(Sieve);
    }
}

void *GRANNY
AddSieveUnit(sieve &Sieve, int32x GroupIndex)
{
    Assert(GroupIndex < Sieve.GroupCount);

    int32x UnitIndex;
    if(NewStackUnit(Sieve.Groups[GroupIndex], UnitIndex))
    {
        return(GetStackUnit(Sieve.Groups[GroupIndex], UnitIndex));
    }

    return(0);
}

int32x GRANNY
GetSieveGroupUnitCount(sieve &Sieve, int32x GroupIndex)
{
    Assert(GroupIndex < Sieve.GroupCount);

    return(GetStackUnitCount(Sieve.Groups[GroupIndex]));
}

void GRANNY
SerializeSieveGroup(sieve &Sieve, int32x GroupIndex, void *Dest)
{
    Assert(GroupIndex < Sieve.GroupCount);

    SerializeStack(Sieve.Groups[GroupIndex], Dest);
}
