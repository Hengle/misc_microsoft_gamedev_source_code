#if !defined(GRANNY_BSPLINE_SOLVER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_solver.h $
// $DateTime: 2007/07/09 13:14:09 $
// $Change: 15460 $
// $Revision: #23 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif


BEGIN_GRANNY_NAMESPACE EXPGROUP(BSplineSolverGroup);

// The solver can solve multiple dimension splines within the same
// scratch space.  The scratch space is always O(Degree * SampleCount)
// (with some fun constants in there we won't discuss here in the
// header file :).  So the "right" thing to do is use the largest
// degree you plan to solve, and the largest number of samples,
// and then you can just solve and solve and solve until you're
// blue in the face.
EXPTYPE struct bspline_solver;
EXPAPI GS_SAFE int32x GetMaximumKnotCountForSampleCount(int32x MaxDegree,
                                                        int32x MaxSampleCount);

EXPAPI GS_SAFE bspline_solver *AllocateBSplineSolver(int32x MaxDegree,
                                                     int32x MaxSampleCount,
                                                     int32x MaxDimension);
EXPAPI GS_PARAM void DeallocateBSplineSolver(bspline_solver *Solver);

EXPTYPE enum bspline_solver_flags
{
    BSplineSolverEvaluateAsQuaternions = 0x1,
    BSplineSolverAllowC0Splitting = 0x2,
    BSplineSolverAllowC1Splitting = 0x4,
    //BSplineSolverInsertKnotsAtMaxError_OBSOLETE = 0x8,
    BSplineSolverExtraDOFKnotZero = 0x10,
    BSplineSolverForceEndpointAlignment = 0x20,
    BSplineSolverAllowReduceKeys = 0x40

};

EXPAPI GS_MODIFY curve2* FitBSplineToSamples(bspline_solver &Solver,
                                             uint32x SolverFlags,
                                             int32x Degree,
                                             real32 ErrorThreshold,
                                             real32 C0Threshold,
                                             real32 C1Threshold,
                                             real32 const *Samples,
                                             int32x Dimension,
                                             int32x SampleCount,
                                             real32 dT,
                                             data_type_definition const *CurveDataType,
                                             int32x MaximumCurveSizeInBytes,
                                             bool *AchievedTolerance,
                                             int32x *CurveSizeInBytes);

EXPAPI GS_SAFE real32 OrientationToleranceFromDegrees(real32 Degrees);
EXPAPI GS_SAFE real32 DegreesFromOrientationTolerance(real32 Tolerance);


//
// These are functions you shouldn't be calling unless you're trying
// to do some solver debugging
//
void SolveSpline(bspline_solver &Solver,
                 int32x Dimension, int32x Degree,
                 int32x KnotCount, real32 *Knots,
                 int32x SampleCount, real32 const *Samples,
                 real32 *ResultControls);

EXPTYPE_EPHEMERAL struct bspline_error
{
    int32x MaximumSquaredErrorKnotIndex;
    int32x MaximumSquaredErrorSampleIndex;

    real32 MaximumSquaredError;
    real32 AccumulatedSquaredError;
};

EXPAPI GS_READ void GetSquaredErrorOverCurve(int32x Flags,
                                             int32x Degree, int32x Dimension,
                                             real32 Duration,
                                             curve2 const *Curve,
                                             int32x SampleCount, real32 const *Samples,
                                             real32 const *IdentityVector,
                                             bspline_error *Result);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BSPLINE_SOLVER_H
#endif
