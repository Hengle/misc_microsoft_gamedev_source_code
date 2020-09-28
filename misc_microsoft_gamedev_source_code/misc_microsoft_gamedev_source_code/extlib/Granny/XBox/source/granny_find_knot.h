#if !defined(GRANNY_FIND_KNOT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_find_knot.h $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #1 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(BSplineBufferGroup);

EXPAPI GS_READ int32x FindKnot(int32x KnotCount, real32 const *Knots, real32 t);
EXPAPI GS_READ int32x FindCloseKnot(int32x KnotCount, real32 const *Knots, real32 t, int32x StartingIndex);

// And the 8 and 16-bit versions.
int32x FindKnotUint16(int32x KnotCount, uint16 const *Knots, uint16 t);
int32x FindKnotUint8 (int32x KnotCount, uint8  const *Knots, uint8  t);
int32x FindCloseKnotUint16(int32x KnotCount, uint16 const *Knots, uint16 t, int32x StartingIndex);
int32x FindCloseKnotUint8 (int32x KnotCount, uint8  const *Knots, uint8  t, int32x StartingIndex);

END_GRANNY_NAMESPACE;


#include "header_postfix.h"
#define GRANNY_FIND_KNOT_H
#endif
