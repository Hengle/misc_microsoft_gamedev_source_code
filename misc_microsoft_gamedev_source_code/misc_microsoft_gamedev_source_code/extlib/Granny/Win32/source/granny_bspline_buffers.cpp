// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_buffers.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #23 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BSPLINE_BUFFERS_H)
#include "granny_bspline_buffers.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_FIND_KNOT_H)
#include "granny_find_knot.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_BSPLINE_H)
#include "granny_bspline.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode BSplineLogMessage

USING_GRANNY_NAMESPACE;


#if 0
#define WriteKnot(Curve, KnotIndex, Offset) \
*tiWrite++ = (Curve).Knots[KnotIndex] + Offset; \
Copy32(Dimension, &(Curve).Controls[(KnotIndex) * Dimension], piWrite); \
piWrite += Dimension;
#endif

bool GRANNY
ConstructBSplineBuffers(int32x Dimension,
                        curve2 const *PrevCurve,
                        curve2 const &Curve,
                        curve2 const *NextCurve,
                        real32 PrevCurveDuration,
                        real32 CurveDuration,
                        real32 NextCurveDuration,
                        int32x KnotIndex,
                        real32 * NOALIAS ti, real32 * NOALIAS pi,
                        real32 *&tiPtr, real32 *&piPtr,
                        real32 const *IdentityVector)
{
    // ---------------------------------------------
    // REMOVEDCODE: Old-style raw curves (3/28/2006)
    // ---------------------------------------------

    // However, PrevCurve and NextCurve _can_ be identity, constant, etc.
    Assert ( !CurveIsIdentity ( Curve ) );
    Assert ( !CurveIsConstantOrIdentity ( Curve ) );

    bool OverOrUnderFlow = false;
    int32x Degree = CurveGetDegreeInline ( Curve );
    // I don't really want to check every single time because of the perf. hit. Asserts will have to do.
    Assert ( ( PrevCurve == NULL ) || ( Degree == CurveGetDegree ( *PrevCurve ) ) );
    Assert ( ( NextCurve == NULL ) || ( Degree == CurveGetDegree ( *NextCurve ) ) );

    int32x KnotCount = CurveGetKnotCount ( Curve );
    Assert ( KnotIndex >= 0 );
    Assert ( KnotIndex <= KnotCount );
    if ( ( KnotCount > 0 ) && ( NextCurve != NULL ) )
    {
        // When NextCurve is present, it means we are forward-wrapping.
        // Thus, we need to treat our final knot as simply a timing
        // value, because in reality it's really the first knot of
        // the next curve, and we want to treat it as an overflow
        // if we would have to use it (since we should grab it from
        // NextCurve, not Curve, and thus we need to use the overflow
        // copying).
        --KnotCount;
    }


    // To evaluate a curve of degree n, centered on knot I,
    // we need knot times I-n to I+n-1, and control values I-n to I.
    // So for a curve of degree 3, where KnotIndex==8, we need:
    // Knot times 5 to 10
    // Control values 5 to 8
    // However, it's a lot easier to just get both knots and controls for 5 to 10
    // TODO: Make the slight speed optimisation to only get controls 5 to 8

    int32x KnotWindow = 2*Degree;
    int32x BaseKnotIndex = KnotIndex - Degree;
    bool const Underflow = (BaseKnotIndex < 0);
    bool const Overflow = ((BaseKnotIndex + KnotWindow) > KnotCount);
    OverOrUnderFlow = (Underflow || Overflow);

    if(OverOrUnderFlow)
    {
        // This is the slow path, which only happens on boundary
        // conditions. We need to sample parts of the adjacent curves,
        // or replicate the first/last knots of the subject curve
        // if there are no adjacent curves.

        tiPtr = ti + Degree;
        piPtr = pi + (Degree * Dimension);


        // Insert the correct chunk of Curve into the buffer.
        int32x FirstCurveIndex = BaseKnotIndex;
        int32x NumCurveKnots = Degree * 2;
        int32x FirstLocalIndex = 0;
        if ( Underflow )
        {
            // FirstCurveIndex is -ve, so we need to shift the place we write to up a bit.
            int32x UnderflowBy = -FirstCurveIndex;
            Assert ( UnderflowBy > 0 );
            NumCurveKnots -= UnderflowBy;
            FirstLocalIndex += UnderflowBy;
            FirstCurveIndex = 0;
        }

        if ( Overflow )
        {
            int32x OverflowBy = (FirstCurveIndex + NumCurveKnots) - KnotCount;
            Assert ( OverflowBy > 0 );
            NumCurveKnots -= OverflowBy;
        }

        Assert ( NumCurveKnots >= 0 );
        Assert ( FirstCurveIndex >= 0 );
        Assert ( FirstCurveIndex + NumCurveKnots <= KnotCount );
        Assert ( FirstLocalIndex >= 0 );
        Assert ( FirstLocalIndex + NumCurveKnots <= Degree * 2 );
        if ( NumCurveKnots > 0 )
        {
            CurveExtractKnotValues ( Curve, FirstCurveIndex, NumCurveKnots, ti + FirstLocalIndex, pi + FirstLocalIndex * Dimension, IdentityVector );
        }
        else
        {
            Assert ( !"Surprising, but not fatal" );
        }


        // Fill in the underflow.
        if ( Underflow )
        {
            if ( PrevCurve == NULL )
            {
                // Simply replicate the first knot of the curve.
                real32 *tiDst = ti;
                real32 *piDst = pi;
                real32 *tiSrc = ti + FirstLocalIndex;
                real32 *piSrc = pi + FirstLocalIndex * Dimension;
                {for ( int32x CurIndex = 0; CurIndex < FirstLocalIndex; CurIndex++ )
                {
                    *tiDst++ = *tiSrc;
                    Copy32 ( Dimension, piSrc, piDst );
                    piDst += Dimension;
                }}
            }
            else if ( CurveIsIdentity ( *PrevCurve ) )
            {
                // Copy in the caller's idea of what "identity" is.
                // TODO: What times should the knots be given?
                real32 *tiDst = ti;
                real32 *piDst = pi;
                real32 CurrentTime = ti[FirstLocalIndex] - (real32)FirstLocalIndex * PrevCurveDuration;
                {for ( int32x CurIndex = 0; CurIndex < FirstLocalIndex; CurIndex++ )
                {
                    *tiDst++ = CurrentTime;
                    Copy32 ( Dimension, IdentityVector, piDst );
                    piDst += Dimension;
                    CurrentTime += PrevCurveDuration;
                }}
            }
            else
            {
                // There's an actual curve before it.
                // This could be slightly faster and get multiple knots at a time, but this way it's extremely robust.
                int32x KnotCountPrev = CurveGetKnotCount ( *PrevCurve );
                // Note that the last knot value is not used when wrapping -
                // it is simply a time reference that says how long the curve is.
                if ( KnotCountPrev > 1 )
                {
                    KnotCountPrev--;
                }
                Assert ( KnotCountPrev > 0 );
                int32x CurrentKnot = KnotCountPrev - 1;
                real32 *tiDst = ti + ( FirstLocalIndex - 1 );
                real32 *piDst = pi + ( FirstLocalIndex - 1 ) * Dimension;
                real32 TimeOffset = -PrevCurveDuration;
                {for ( int32x CurLocalIndex = FirstLocalIndex - 1; CurLocalIndex >= 0; CurLocalIndex-- )
                {
                    CurveExtractKnotValues ( *PrevCurve, CurrentKnot, 1, tiDst, piDst, IdentityVector );
                    *tiDst += TimeOffset;
                    Assert ( *tiDst <= 0.0f );
                    tiDst--;
                    piDst -= Dimension;
                    CurrentKnot--;
                    if ( CurrentKnot < 0 )
                    {
                        // Wrap yet again!
                        Assert ( !"Not sure if this is even possible, so never tested" );
                        CurrentKnot = KnotCountPrev - 1;
                        TimeOffset -= PrevCurveDuration;
                    }
                }}
            }
        }

        // Fill in the overflow.
        if ( Overflow )
        {
            int32x LastLocalIndex = FirstLocalIndex + NumCurveKnots - 1;
            if ( NextCurve == NULL )
            {
                // Simply replicate the last knot of the curve.
                real32 *tiSrc = ti + LastLocalIndex;
                real32 *piSrc = pi + LastLocalIndex * Dimension;
                real32 *tiDst = tiSrc + 1;
                real32 *piDst = piSrc + Dimension;
                {for ( int32x CurIndex = LastLocalIndex + 1; CurIndex < KnotWindow; CurIndex++ )
                {
                    *tiDst++ = *tiSrc;
                    Copy32 ( Dimension, piSrc, piDst );
                    piDst += Dimension;
                }}
            }
            else if ( CurveIsIdentity ( *NextCurve ) )
            {
                // Copy in the caller's idea of what "identity" is.
                // TODO: What times should the knots be given?
                real32 *tiDst = ti + LastLocalIndex + 1;
                real32 *piDst = pi + ( LastLocalIndex + 1 ) * Dimension;
                real32 CurrentTime = ti[LastLocalIndex];
                {for ( int32x CurIndex = 0; CurIndex < FirstLocalIndex; CurIndex++ )
                {
                    CurrentTime += NextCurveDuration;
                    *tiDst++ = CurrentTime;
                    Copy32 ( Dimension, IdentityVector, piDst );
                    piDst += Dimension;
                }}
            }
            else
            {
                // There's an actual curve after it.
                // This could be slightly faster and get multiple knots at a time, but this way it's extremely robust.
                int32x KnotCountNext = CurveGetKnotCount ( *NextCurve );
                // Note that the last knot value is not used when wrapping -
                // it is simply a time reference that says how long the curve is.
                if ( KnotCountNext > 1 )
                {
                    KnotCountNext--;
                }
                Assert ( KnotCountNext > 0 );
                int32x CurrentKnot = 0;
                real32 *tiDst = ti + ( LastLocalIndex + 1 );
                real32 *piDst = pi + ( LastLocalIndex + 1 ) * Dimension;
                real32 TimeOffset = CurveDuration;
                {for ( int32x CurIndex = LastLocalIndex + 1; CurIndex < KnotWindow; CurIndex++ )
                {
                    CurveExtractKnotValues ( *NextCurve, CurrentKnot, 1, tiDst, piDst, IdentityVector );
                    *tiDst += TimeOffset;
                    tiDst++;
                    piDst += Dimension;
                    CurrentKnot++;
                    if ( CurrentKnot >= KnotCountNext )
                    {
                        // Wrap yet again!
                        CurrentKnot = 0;
                        TimeOffset += NextCurveDuration;
                    }
                }}
            }
        }
    }
    else
    {
        // Fast path - no thinking, just extract the knots and controls we need.
        curve_data_da_k32f_c32f *CurveDataDaK32fC32f = CurveGetContentsOfDaK32fC32f ( Curve );
        if ( CurveDataDaK32fC32f != NULL )
        {
            // Even faster - it's an uncompressed curve, so we can just point into the middle of it.
            tiPtr = &(CurveDataDaK32fC32f->Knots[KnotIndex]);
            piPtr = &(CurveDataDaK32fC32f->Controls[Dimension * KnotIndex]);
        }
        else
        {
            // Some decompression required.
            tiPtr = ti + Degree;
            piPtr = pi + (Degree * Dimension);
            CurveExtractKnotValues ( Curve, BaseKnotIndex, Degree * 2, ti, pi, IdentityVector );
        }
    }

#if DEBUG
    // Just check the time values are increasing (or constant).
    {for ( int32x KnotNum = -Degree + 1; KnotNum < Degree; KnotNum++ )
    {
        Assert ( tiPtr[KnotNum-1] <= tiPtr[KnotNum] );
    }}
#endif

    return(OverOrUnderFlow);
}

void GRANNY
EvaluateCurveAtT(int32x Dimension, bool Normalize,
                 bool BackwardsLoop,
                 curve2 const &Curve,
                 bool ForwardsLoop,
                 real32 CurveDuration,
                 real32 t, real32 *Result,
                 real32 const *IdentityVector)
{
    EvaluateCurveAtKnotIndex(Dimension, Normalize, BackwardsLoop, Curve, ForwardsLoop,
                             CurveDuration, CurveFindKnot(Curve, t),
                             t, Result, IdentityVector);
}

void GRANNY
EvaluateCurveAtKnotIndex(int32x Dimension, bool Normalize,
                         bool BackwardsLoop,
                         curve2 const &Curve,
                         bool ForwardsLoop,
                         real32 CurveDuration,
                         int32x KnotIndex, real32 t,
                         real32 *Result,
                         real32 const *IdentityVector)
{
    if ( CurveIsIdentity ( Curve ) )
    {
        Copy32 ( Dimension, IdentityVector, Result );
    }
    else if ( CurveIsConstantOrIdentity ( Curve ) )
    {
        CurveExtractKnotValue ( Curve, 0, Result, IdentityVector );
    }
    else
    {
        real32 tiBufferSpace[8];
        real32 piBufferSpace[4 * MaximumBSplineDimension];

        int32x Degree = CurveGetDegreeInline ( Curve );
        Assert ( ( Degree + 1 ) < ArrayLength ( tiBufferSpace ) );
        Assert ( ( Degree + 1 ) * Dimension < ArrayLength ( piBufferSpace ) );

        real32 *tiBuffer;
        real32 *piBuffer;

        // If the time was exactly at the end of the curve, curve compression and
        // numerical precision can cause the local clock to be very slightly greater,
        // so it runs off the end. Which is bad.
        int32x KnotCount = CurveGetKnotCount ( Curve );
        if ( KnotIndex == KnotCount )
        {
            KnotIndex--;
        }
        Assert ( KnotIndex < KnotCount );


        if(ConstructBSplineBuffers(Dimension,
                                   BackwardsLoop ? &Curve : NULL,
                                   Curve,
                                   ForwardsLoop ? &Curve : NULL,
                                   CurveDuration, CurveDuration, CurveDuration,
                                   KnotIndex,
                                   tiBufferSpace, piBufferSpace,
                                   tiBuffer, piBuffer,
                                   IdentityVector))
        {
            if(Normalize && (Dimension == 4))
            {
                EnsureQuaternionContinuity(Degree + 1,
                                           piBufferSpace);
            }
        }

        SampleBSpline(Degree, Dimension, Normalize,
                      tiBuffer, piBuffer, t, Result);
    }
}

// ----------------------------------------------
// REMOVEDCODE: CurveFindNextCrossing (3/28/2006)
//  Never actually finished...
// ----------------------------------------------
