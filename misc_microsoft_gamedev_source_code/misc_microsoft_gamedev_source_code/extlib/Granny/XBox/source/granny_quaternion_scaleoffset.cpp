// ========================================================================
// $File: //jeffr/granny/rt/granny_quaternion_scaleoffset.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #1 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_QUATERNION_SCALEOFFSET_H)
#include "granny_quaternion_scaleoffset.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

#define OneOverSqrt2 (0.707106781f)

// Which components of this table are and are not included was found very much by trial & error!
// Don't change it, or old stuff will break horribly :-)
ALIGN16(real32) const QuaternionCurveScaleOffsetTable[16 * 2] =
{
    OneOverSqrt2 * 2.0f, -OneOverSqrt2,
    OneOverSqrt2 * 1.0f, -OneOverSqrt2 * 0.5f,
    OneOverSqrt2 * 0.5f, -OneOverSqrt2 * 0.75f,
    OneOverSqrt2 * 0.5f, -OneOverSqrt2 * 0.25f,
    OneOverSqrt2 * 0.5f,  OneOverSqrt2 * 0.25f,
    OneOverSqrt2 * 0.25f, -OneOverSqrt2 * 0.250f,
    OneOverSqrt2 * 0.25f, -OneOverSqrt2 * 0.125f,
    OneOverSqrt2 * 0.25f,  OneOverSqrt2 * 0.000f,

    // And the same table, negated.
    -OneOverSqrt2 * 2.0f, OneOverSqrt2,
    -OneOverSqrt2 * 1.0f, OneOverSqrt2 * 0.5f,
    -OneOverSqrt2 * 0.5f, OneOverSqrt2 * 0.75f,
    -OneOverSqrt2 * 0.5f, OneOverSqrt2 * 0.25f,
    -OneOverSqrt2 * 0.5f,  -OneOverSqrt2 * 0.25f,
    -OneOverSqrt2 * 0.25f, OneOverSqrt2 * 0.250f,
    -OneOverSqrt2 * 0.25f, OneOverSqrt2 * 0.125f,
    -OneOverSqrt2 * 0.25f, -OneOverSqrt2 * 0.000f
};

END_GRANNY_NAMESPACE;
