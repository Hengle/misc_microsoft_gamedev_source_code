#if !defined(GRANNY_STACK_ALLOCATOR_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_stack_allocator.h $
// $DateTime: 2007/06/01 11:51:36 $
// $Change: 15075 $
// $Revision: #10 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MemoryAllocatorGroup);

EXPTYPE_EPHEMERAL struct allocated_block;
EXPTYPE_EPHEMERAL struct stack_allocator
{
    int32x UnitSize;
    int32x UnitsPerBlock;

    int32x TotalUsedUnitCount;
    allocated_block *LastBlock;

    // If we pass a maximum unit count, we can do random access
    // through the block directory.
    int32x MaxUnits;
    int32x ActiveBlocks;
    int32x MaxActiveBlocks;
    allocated_block **BlockDirectory;
};

EXPAPI GS_PARAM void StackInitialize(stack_allocator &Allocator,
                                     int32x UnitSize, int32x UnitsPerBlock);
EXPAPI GS_PARAM void StackInitializeWithDirectory(stack_allocator &Allocator,
                                                  int32x UnitSize,
                                                  int32x UnitsPerBlock,
                                                  int32x MaxUnits);
EXPAPI GS_PARAM void StackCleanUp(stack_allocator &Allocator);

EXPAPI GS_PARAM int32x GetStackUnitCount(stack_allocator const &Allocator);
EXPAPI GS_PARAM void* NewStackUnit(stack_allocator &Allocator, int32x &Result);
EXPAPI GS_READ void *GetStackUnit(stack_allocator &Allocator, int32x Index);
EXPAPI GS_PARAM void PopStackUnits(stack_allocator &Allocator, int32x UnitCount);

// Initial value may be NULL
EXPAPI GS_PARAM bool MultipleNewStackUnits(stack_allocator &Allocator,
                                           int32x NumNewIndices,
                                           int32x &NewIndicesStart,
                                           void const* InitialValue);


EXPAPI GS_PARAM void SerializeStack(stack_allocator const &Allocator, void *Dest);

// Handy macro for accessing stack units
#define STACK_UNIT_AS(Allocator, Index, Type) (*((Type*)GetStackUnit(Allocator, Index)))

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STACK_ALLOCATOR_H
#endif
