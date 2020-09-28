#if !defined(GRANNY_BSPLINE_SOLVER_CORE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_solver_core.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

void BuildATAAndATb(
    int32x bWidth, int32x Degree, int32x KnotCount, real32 *Knots,
    int32x *KnotStart, int32x bCount, real32 const *b,
    real32 *A, real32 *ATA, real32 *ATb);

void SymmetricBandDiagonalCholeskyFactor(
    int32x ATn, int32x BandWidth, real32 *ATA);

void SymmetricBandDiagonalCholeskySolve(
    int32x ATn, int32x BandWidth,  real32 *ATA,
    int32x ATbStride, real32 *ATb);

real32 const BSplineSolverDiagonalEpsilon = 0.00001f;

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BSPLINE_SOLVER_CORE_H
#endif
