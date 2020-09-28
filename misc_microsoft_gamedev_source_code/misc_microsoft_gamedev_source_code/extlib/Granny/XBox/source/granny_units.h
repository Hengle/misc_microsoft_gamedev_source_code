#if !defined(GRANNY_UNITS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_units.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

BEGIN_GRANNY_NAMESPACE;

real32 MetersPerSecondFromMilesPerHour(real32 MilesPerHour);
real32 MetersFromFeet(real32 Feet);

real32 RadiansFromDegrees(real32 Degrees);
real32 DegreesFromRadians(real32 Radians);

// TODO: Move these to a _inlines.h file

inline real32
MetersPerSecondFromMilesPerHour(real32 MilesPerHour)
{
    return(MilesPerHour * 0.447041f);
}

inline real32
MetersFromFeet(real32 Feet)
{
    return(Feet * 0.3048f);
}

inline real32
RadiansFromDegrees(real32 Degrees)
{
    return(Degrees * Pi32 / 180.0f);
}

inline real32
DegreesFromRadians(real32 Radians)
{
    return(Radians * 180.0f / Pi32);
}

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_UNITS_H
#endif
