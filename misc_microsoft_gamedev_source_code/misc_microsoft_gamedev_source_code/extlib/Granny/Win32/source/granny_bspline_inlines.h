#if !defined(GRANNY_BSPLINE_INLINES_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_inlines.h $
// $DateTime: 2007/07/02 11:41:19 $
// $Change: 15387 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

inline real32
LinearCoefficient(real32 const ti_1,
                  real32 const ti,
                  real32 const t)
{
    real32 const dL0 = (ti - ti_1);
    // TODO: Mac OS X's dynamic link loaded can't handle assertions in the
    // h-file because they come out being multiply defined...
    // Assert(dL0 != 0.0f);
    return((t - ti_1) / dL0);
}

inline void
LinearCoefficients(real32 const ti_1,
                   real32 const ti,
                   real32 const t,
                   real32* NOALIAS ci_1,
                   real32* NOALIAS ci)
{
    *ci = LinearCoefficient(ti_1, ti, t);
    *ci_1 = 1.0f - *ci;
}

inline void
QuadraticCoefficients(real32 const ti_2,
                      real32 const ti_1,
                      real32 const ti,
                      real32 const ti1,
                      real32 const t,
                      real32* NOALIAS ci_2,
                      real32* NOALIAS ci_1,
                      real32* NOALIAS ci)
{
    // * -> 2
    // + -> 1
    // - -> 8
    // / -> 3

    real32 const tmti_1 = (t - ti_1);
    real32 const tmti_2 = (t - ti_2);

    real32 const dL0 = ti - ti_1;
    real32 const dL1_1 = ti - ti_2;
    real32 const dL1_2 = ti1 - ti_1;

    // TODO: Mac OS X's dynamic link loaded can't handle assertions in the
    // h-file because they come out being multiply defined...
#if COMPILER_MSVC
    Assert(dL0 != 0.0f);
    Assert(dL1_1 != 0.0f);
    Assert(dL1_2 != 0.0f);
#endif

    real32 const L0   = FastDivGreater0(tmti_1, dL0);
    real32 const L1_1 = FastDivGreater0(tmti_2, dL1_1);
    real32 const L1_2 = FastDivGreater0(tmti_1, dL1_2);

//     // Original
//     real32 const mL0 =   1.0f - L0;
//     real32 const mL1_1 = 1.0f - L1_1;
//     real32 const mL1_2 = 1.0f - L1_2;
//     ci_2 = mL0 * mL1_1;
//     ci_1 = mL0 * L1_1 + L0 * mL1_2;
//     ci   = L0  * L1_2;

    // Jeffized (and a little caseyized)
    *ci_2 = (L1_1+L0) - L0*L1_1;
    *ci = L0*L1_2;
    *ci_1 = *ci_2 - *ci;
    *ci_2 = 1.0f - *ci_2;
}

inline void
CubicCoefficients(real32 const ti_3,
                  real32 const ti_2,
                  real32 const ti_1,
                  real32 const ti,
                  real32 const ti1,
                  real32 const ti2,
                  real32 const t,
                  real32* NOALIAS ci_3,
                  real32* NOALIAS ci_2,
                  real32* NOALIAS ci_1,
                  real32* NOALIAS ci)
{
    // * -> 12
    // + -> 4
    // - -> 15
    // / -> 6

    real32 const tmti_1 = (t - ti_1);
    real32 const tmti_2 = (t - ti_2);
    real32 const tmti_3 = (t - ti_3);

    real32 const dL0 = ti - ti_1;
    real32 const dL1_1 = ti - ti_2;
    real32 const dL1_2 = ti1 - ti_1;
    real32 const dL2_1 = ti - ti_3;
    real32 const dL2_2 = ti1 - ti_2;
    real32 const dL2_3 = ti2 - ti_1;

    // TODO: Mac OS X's dynamic link loaded can't handle assertions in the
    // h-file because they come out being multiply defined...
#if COMPILER_MSVC
    Assert(dL0 != 0.0f);
    Assert(dL1_1 != 0.0f);
    Assert(dL1_2 != 0.0f);
    Assert(dL2_1 != 0.0f);
    Assert(dL2_2 != 0.0f);
    Assert(dL2_3 != 0.0f);
#endif

    real32 const L0 = tmti_1 / dL0;
    real32 const L1_1 = tmti_2 / dL1_1;
    real32 const L1_2 = tmti_1 / dL1_2;
    real32 const L2_1 = tmti_3 / dL2_1;
    real32 const L2_2 = tmti_2 / dL2_2;
    real32 const L2_3 = tmti_1 / dL2_3;

    real32 const mL0 = 1.0f - L0;
    real32 const mL1_1 = 1.0f - L1_1;
    real32 const mL1_2 = 1.0f - L1_2;
    real32 const mL2_1 = 1.0f - L2_1;
    real32 const mL2_2 = 1.0f - L2_2;
    real32 const mL2_3 = 1.0f - L2_3;

    real32 const mL0mL1_1 = mL0 * mL1_1;
    real32 const mL0L1_1 = mL0 * L1_1;
    real32 const L0mL1_2 = L0 * mL1_2;
    real32 const L0L1_2 = L0 * L1_2;

    *ci_3 = mL0mL1_1 * mL2_1;
    *ci_2 = (mL0mL1_1 * L2_1 +
             mL0L1_1 * mL2_2 +
             L0mL1_2 * mL2_2);
    *ci_1 = (mL0L1_1 * L2_2 +
             L0mL1_2 * L2_2 +
             L0L1_2 * mL2_3);
    *ci = L0L1_2 * L2_3;
}

#include "header_postfix.h"
#define GRANNY_BSPLINE_INLINES_H
#endif
