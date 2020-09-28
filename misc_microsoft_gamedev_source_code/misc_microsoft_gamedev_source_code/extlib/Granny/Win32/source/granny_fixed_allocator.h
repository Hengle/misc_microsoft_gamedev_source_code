#if !defined(GRANNY_FIXED_ALLOCATOR_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_fixed_allocator.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
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

EXPTYPE_EPHEMERAL struct fixed_allocator_unit
{
    fixed_allocator_unit *Next;
};

EXPTYPE_EPHEMERAL struct fixed_allocator_block
{
    int32x UnitCount;
    uint8 *Units;

    fixed_allocator_unit *FirstFreeUnit;

    fixed_allocator_block *Next;
    fixed_allocator_block *Previous;
};

EXPTYPE_EPHEMERAL struct fixed_allocator
{
    // You MUST set this in your static definition
    int32x UnitSize;

    // This is optional - if you don't set it, it will automatically
    // be set to an appropriate size
    int32x UnitsPerBlock;

    // These will all be initialized to zero by the static constructor
    fixed_allocator_block Sentinel;
};

EXPAPI GS_PARAM void *AllocateFixed(fixed_allocator &Allocator);
EXPAPI GS_PARAM void DeallocateFixed(fixed_allocator &Allocator, void *Memory);

EXPAPI GS_PARAM void DeallocateAllFixed(fixed_allocator &Allocator);

// If you don't do it with the static initializer (like, you allocated
// one of these or something), you have to call this before using.
EXPAPI GS_PARAM void InitializeFixedAllocator(fixed_allocator &Allocator, int32x UnitSize);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FIXED_ALLOCATOR_H
#endif
