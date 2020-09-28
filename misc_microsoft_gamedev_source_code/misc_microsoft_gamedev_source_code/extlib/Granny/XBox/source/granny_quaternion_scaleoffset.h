#if !defined(GRANNY_QUATERNION_SCALEOFFSET_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_quaternion_scaleoffset.h $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #1 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

#define QUATERNION_SCALE_ENTRY(Index, Entry) (QuaternionCurveScaleOffsetTable[(Index)*2 + (Entry)])
extern ALIGN16(real32) const QuaternionCurveScaleOffsetTable[16*2];

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_QUATERNION_SCALEOFFSET_H
#endif
