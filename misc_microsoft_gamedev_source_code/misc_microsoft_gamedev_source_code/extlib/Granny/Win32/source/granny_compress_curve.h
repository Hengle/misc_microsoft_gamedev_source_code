#if !defined(GRANNY_COMPRESS_CURVE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_compress_curve.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(BSplineSolverGroup);


struct curve2;
struct bspline_solver;
struct data_type_definition;

EXPTYPE_EPHEMERAL struct compress_curve_parameters
{
    int32x DesiredDegree;                 // Requested degree of the curve
    bool AllowDegreeReduction;            // Allow the solver to attempt to do constant/identity instead
    bool AllowReductionOnMissedTolerance; // Allows the solver to aggressively reduce curve size on tolerance failure
    real32 ErrorTolerance;                // Maximum allowed error
    real32 C0Threshold;                   // Threshold for curve jumps (if BSplineSolverAllowC0Splitting & Flags)
    real32 C1Threshold;                   // Threshold for curve creases (if BSplineSolverAllowC1Splitting & Flags)

    // The types of compression to try
    data_type_definition **PossibleCompressionTypes;
    int32x PossibleCompressionTypesCount;

    // Constant and identity types for this curve, along with the identity vector
    data_type_definition *ConstantCompressionType;
    data_type_definition *IdentityCompressionType;
    real32 const *IdentityVector;
};



EXPAPI GS_MODIFY curve2 *CompressCurve(bspline_solver &Solver, // from AllocateBSplineSolver
                                       uint32x SolverFlags,    // since we reuse the solver, pass the flags in manually

                                       // see above
                                       compress_curve_parameters const &Params,

                                       real32 const *Samples,            // The samples to fit
                                       int32x Dimension,           // The dimension of the curve
                                       int32x FrameCount,          // Number of sample frames
                                       real32 dT,                  // Timestep between samples
                                       bool *CurveAcheivedTolerance);   // Did we hit our target (may be NULL)

END_GRANNY_NAMESPACE;


#include "header_postfix.h"
#define GRANNY_COMPRESS_CURVE_H
#endif
