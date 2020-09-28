#if !defined(GRANNY_PERIODIC_LOOP_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_periodic_loop.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(AnimationGroup);

EXPTYPE struct periodic_loop
{
    real32 Radius;
    real32 dAngle;
    real32 dZ;

    triple BasisX;
    triple BasisY;
    triple Axis;
};
EXPCONST EXPGROUP(periodic_loop) extern data_type_definition PeriodicLoopType[];

EXPAPI GS_PARAM void ZeroPeriodicLoop(periodic_loop &Loop);
EXPAPI GS_PARAM void FitPeriodicLoop(real32 const *StartPosition3,
                                     real32 const *StartOrientation4,
                                     real32 const *EndPosition3,
                                     real32 const *EndOrientation4,
                                     real32 Seconds, periodic_loop &Loop);

EXPAPI GS_PARAM void ComputePeriodicLoopVector(periodic_loop const &Loop,
                                               real32 Seconds,
                                               real32 *Result3);
EXPAPI GS_PARAM void ComputePeriodicLoopLog(periodic_loop const &Loop,
                                            real32 Seconds,
                                            real32 *Result4);
EXPAPI GS_PARAM void StepPeriodicLoop(periodic_loop const &Loop, real32 Seconds,
                                      real32 *Position3,
                                      real32 *Orientation4);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_PERIODIC_LOOP_H
#endif
