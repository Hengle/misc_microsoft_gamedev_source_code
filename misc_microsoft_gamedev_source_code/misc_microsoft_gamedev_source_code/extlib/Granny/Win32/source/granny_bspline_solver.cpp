// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_solver.cpp $
// $DateTime: 2007/08/23 09:22:51 $
// $Change: 15808 $
// $Revision: #36 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_BSPLINE_SOLVER_H)
#include "granny_bspline_solver.h"
#endif

#if !defined(GRANNY_BSPLINE_SOLVER_DEBUGGING_H)
#include "granny_bspline_solver_debugging.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_BSPLINE_H)
#include "granny_bspline.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_FIND_KNOT_H)
#include "granny_find_knot.h"
#endif

#if !defined(GRANNY_BSPLINE_BUFFERS_H)
#include "granny_bspline_buffers.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_BSPLINE_SOLVER_CORE_H)
#include "granny_bspline_solver_core.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_UNITS_H)
#include "granny_units.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


#undef SubsystemCode
#define SubsystemCode CurveLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct bspline_solver
{
    int32x MaxKnotCount;

    int32x *KnotStart;
    real32 *KnotBuffer0;
    real32 *KnotBuffer1;
    real32 *ControlBuffer0;
    real32 *A;
    real32 *ATA;
    void *CurveDataMemory;
};

END_GRANNY_NAMESPACE;

int32x GRANNY
GetMaximumKnotCountForSampleCount(int32x MaxDegree, int32x MaxSampleCount)
{
    return(MaxSampleCount * (MaxDegree + 1));
}

bspline_solver *GRANNY
AllocateBSplineSolver(int32x Degree, int32x SampleCount, int32x Dimension)
{
    int32x BandWidth = Degree + 1;
    int32x SampleCountPlusOverhang = (SampleCount + (2 * (BandWidth)));
    // NOTE THE EXTRA PLUS ONE!!!
    int32x KnotCountPlusOverhang = (SampleCount*(Degree + 1) +
                                    (2 * (BandWidth))) + 1;


    // Temporary curve data chunk. Assume it's the largest format (uncompressed).
    curve_builder CurveBuilder;
    BeginCurveInPlace ( &CurveBuilder, CurveDataDaK32fC32fType, Degree, Dimension, KnotCountPlusOverhang );
    int32x CurveDataBytesRequired = GetResultingCurveDataSize ( &CurveBuilder );
    AbortCurveBuilder ( &CurveBuilder );

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    bspline_solver *Solver;
    AggrAllocPtr(Allocator, Solver);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Solver,
                                     KnotCountPlusOverhang,
                                     KnotStart);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Solver,
                                     KnotCountPlusOverhang,
                                     KnotBuffer0);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Solver,
                                     KnotCountPlusOverhang,
                                     KnotBuffer1);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Solver,
                                     KnotCountPlusOverhang * Dimension,
                                     ControlBuffer0);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Solver,
                                     SampleCountPlusOverhang * BandWidth,
                                     A);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Solver,
                                     KnotCountPlusOverhang * BandWidth,
                                     ATA);
    AggrAllocOffsetSizePtr( Allocator, Solver,
                            CurveDataBytesRequired,
                            CurveDataMemory );
    if(EndAggrAlloc(Allocator))
    {
#if DEBUG
        SetUInt32(KnotCountPlusOverhang, 0xCCCCCCCC, Solver->KnotStart);
        SetUInt32(KnotCountPlusOverhang, 0xCCCCCCCC, Solver->KnotBuffer0);
        SetUInt32(KnotCountPlusOverhang*Dimension, 0xCCCCCCCC, Solver->ControlBuffer0);
        SetUInt32(KnotCountPlusOverhang, 0xCCCCCCCC, Solver->KnotBuffer1);
        SetUInt32(SampleCountPlusOverhang * BandWidth, 0xCCCCCCCC, Solver->A);
        SetUInt32(KnotCountPlusOverhang * BandWidth, 0xCCCCCCCC, Solver->ATA);
#endif

        Solver->MaxKnotCount = GetMaximumKnotCountForSampleCount ( Degree, SampleCount );

        Solver->KnotStart += BandWidth;
        Solver->KnotBuffer0 += BandWidth;
        Solver->KnotBuffer1 += BandWidth;
        Solver->A += BandWidth * BandWidth;
        Solver->ATA += BandWidth * BandWidth;
    }

    return(Solver);
}

void GRANNY
DeallocateBSplineSolver(bspline_solver *Solver)
{
    Deallocate(Solver);
}

static void
InsertKnot(int32x AtIndex,
           int32x &KnotCount,
           real32 *Knots)
{
    Assert((AtIndex >= 0) && AtIndex <= KnotCount);

    ++KnotCount;
    {for(int32x KnotIndex = KnotCount - 1;
         KnotIndex > AtIndex;
         --KnotIndex)
    {
        Knots[KnotIndex] = Knots[KnotIndex - 1];
    }}
}


static real32
GetSquaredError(uint32x Flags,
                int32x Degree, int32x Dimension,
                real32 Duration,
                curve2 const *Curve,
                real32 const *IdentityVector,
                int32x KnotIndex, real32 t,
                real32 const *Samples, int32x SampleIndex)
{
    Assert ( Samples != NULL );

#if DEBUG
    // Precision errors can cause this sometimes
    //int32x TestKnotIndex = CurveFindKnot ( *Curve, t );
    //Assert ( TestKnotIndex == KnotIndex );
#endif

    real32 SamplingBuffer[MaximumBSplineDimension];

    EvaluateCurveAtKnotIndex(Dimension,
                             (Flags & BSplineSolverEvaluateAsQuaternions) != 0,
                             false, *Curve, false, Duration, KnotIndex,
                             t, SamplingBuffer, IdentityVector );

    if(Flags & BSplineSolverEvaluateAsQuaternions)
    {
        return(Square(
                   1.0f -
                   InnerProductN(Dimension, &Samples[SampleIndex * Dimension], SamplingBuffer)));
    }
    else
    {
        return(SquaredDistanceBetweenN(
                   Dimension, &Samples[SampleIndex * Dimension], SamplingBuffer));
    }
}


void GRANNY
GetSquaredErrorOverCurve(int32x Flags,
                         int32x Degree, int32x Dimension,
                         real32 Duration,
                         curve2 const *Curve,
                         int32x SampleCount, real32 const *Samples,
                         real32 const *IdentityVector,
                         bspline_error* Result)
{
    CheckPointerNotNull(Result, return);

    bspline_error Error;

    Error.MaximumSquaredError = 0.0f;
    Error.AccumulatedSquaredError = 0.0f;

    Error.MaximumSquaredErrorKnotIndex = -1;
    Error.MaximumSquaredErrorSampleIndex = -1;

    Assert ( SampleCount > 1 );
    real32 DurationScale = Duration / (real32)( SampleCount - 1 );

    int32x KnotIndex = 0;
    // QUERY: Why does this start from sample 1 and not sample 0?
    {for(int32x SampleIndex = 1;
         SampleIndex < SampleCount;
         ++SampleIndex)
    {
        real32 SampleTime = DurationScale * real32(SampleIndex);
        KnotIndex = CurveFindKnot ( *Curve, SampleTime );

        real32  SampleError = GetSquaredError(
            Flags,
            Degree, Dimension, Duration,
            Curve, IdentityVector,
            KnotIndex, SampleTime,
            Samples, SampleIndex);
        Error.AccumulatedSquaredError += SampleError;

        if(Error.MaximumSquaredError < SampleError)
        {
            Error.MaximumSquaredError = SampleError;
            Error.MaximumSquaredErrorSampleIndex = SampleIndex;
            Error.MaximumSquaredErrorKnotIndex = KnotIndex;
        }
    }}

    *Result = Error;
}


static bool
AddKnotIfAboveError(uint32x Flags,
                    int32x *KnotCount, real32 *NewKnots, int32x const MaxKnotCount,
                    curve2 *Curve, real32 const *IdentityVector,
                    real32 *TempKnotSpace,
                    real32 Tolerance,
                    int32x SampleCount, real32 const *Samples,
                    bool *HitKnotLimit)
{
    Assert(Curve);

    bool Added = false;
    *HitKnotLimit = false;

    int32x Degree    = CurveGetDegree(*Curve);
    int32x Dimension = CurveGetDimension(*Curve);

    // We need to know where the curve put the knot values (after quantisation, etc).
    CurveExtractKnotValues ( *Curve, 0, *KnotCount, TempKnotSpace, NULL, IdentityVector );

    real32 DesiredTolerance = Square(Tolerance);

    real32 MaximumError = DesiredTolerance;
    int32x MaximumIndex = -1;
    int32x MaximumKnotIndex = -1;
    int32x OldKnotCount = *KnotCount;

    real32 Duration = (real32)( SampleCount - 1 );

    int32x KnotIndex = 0;
    real32 *WriteKnot = NewKnots;
    {for(int32x SampleIndex = 1;
         SampleIndex < SampleCount;
         ++SampleIndex)
    {
        real32  SampleTime = real32(SampleIndex);

        // Although this should be true in theory, in practice rounding errors
        // on the more aggressive compressions can give you a final knot of
        // 2067.5 when there are 2069 samples.
        // The code protects itself against this case anyway.
        //Assert ( KnotIndex < OldKnotCount );

        while((KnotIndex < OldKnotCount) && (TempKnotSpace[KnotIndex] <= SampleTime))
        {
            Assert ( KnotIndex < OldKnotCount );
            if(MaximumIndex != -1)
            {
                real32  ThisControlt = TempKnotSpace[MaximumKnotIndex];
                real32  PreviousControlt = TempKnotSpace[MaximumKnotIndex - 1];

                *WriteKnot++ = 0.5f * (ThisControlt + PreviousControlt);

                MaximumError = DesiredTolerance;
                MaximumIndex = -1;
                MaximumKnotIndex = -1;

                Added = true;
            }

            *WriteKnot++ = TempKnotSpace[KnotIndex++];
            Assert ( KnotIndex <= OldKnotCount );
        }

        if((KnotIndex > 0)&&(KnotIndex < OldKnotCount))
        {
            Assert ( TempKnotSpace[KnotIndex-1] <= SampleTime );
            Assert ( TempKnotSpace[KnotIndex] > SampleTime );

#if DEBUG
            // Argh. Yet again, this is true, but might not be quite true due to precision errors.
            //int32x TestKnotIndex = CurveFindKnot ( *Curve, SampleTime );
            //Assert ( TestKnotIndex == KnotIndex );
#endif

            real32  SampleError = GetSquaredError(
                Flags, Degree, Dimension, Duration,
                Curve, IdentityVector,
                KnotIndex, SampleTime, Samples,
                SampleIndex);
            if(MaximumError < SampleError)
            {
                real32 MinDistance = 3.0f;
                if((KnotIndex < 2) ||
                   (TempKnotSpace[KnotIndex - 2] == TempKnotSpace[KnotIndex - 1]))
                {
                    MinDistance = 6.0f;
                }

                // The following cryptic test makes sure that we'll have space left to write out
                //  all of the old knot values before running into MaxKnotCount.
                // (WriteKnot - NewKnots) + (OldKnotCount - KnotIndex) < MaxKnotCount)

                if(((TempKnotSpace[KnotIndex] - TempKnotSpace[KnotIndex - 1]) >= MinDistance) &&
                   ((real32)SampleIndex != TempKnotSpace[KnotIndex]) &&
                   (WriteKnot - NewKnots) + (OldKnotCount - KnotIndex) < MaxKnotCount)
                {
                    MaximumError = SampleError;
                    MaximumIndex = SampleIndex;
                    MaximumKnotIndex = KnotIndex;
                }
                else
                {
                    // We can't add a knot - they would be too closely spaced.
                    *HitKnotLimit = true;
                }
            }
        }
    }}

    if((MaximumIndex != -1)&&(KnotIndex < OldKnotCount))
    {
        real32  ThisControlt = TempKnotSpace[MaximumKnotIndex];
        real32  PreviousControlt = TempKnotSpace[MaximumKnotIndex - 1];

        *WriteKnot++ = 0.5f * (ThisControlt + PreviousControlt);

        MaximumError = DesiredTolerance;
        MaximumIndex = -1;
        MaximumKnotIndex = -1;

        Added = true;
    }


//  if (MaximumIndex != -1)
//    {
//      if (KnotIndex < OldKnotCount)
//      {
//          real32  ThisControlt = TempKnotSpace[MaximumKnotIndex];
//          real32  PreviousControlt = TempKnotSpace[MaximumKnotIndex - 1];
//
//          *WriteKnot++ = 0.5f * (ThisControlt + PreviousControlt);
//          MaximumError = DesiredTolerance;
//          MaximumIndex = -1;
//          MaximumKnotIndex = -1;
//          Added = true;
//      }
//      else
//      {
//          InvalidCodePath("nyi");
//      }
//    }

    Assert ( KnotIndex <= *KnotCount );
    while ( KnotIndex < *KnotCount )
    {
        // Depending on knot-time quantisation, we may have already added the last few knots, or not.
        *WriteKnot++ = TempKnotSpace[KnotIndex++];
    }

    intaddrx NumNewKnots = WriteKnot - NewKnots;
    CheckConvertToInt32(*KnotCount, NumNewKnots, return 0);
    Assert(*KnotCount <= MaxKnotCount);

    return(Added);
}


static curve2*
RemoveUnnecessaryKnots(bspline_solver &Solver, uint32x const SolverFlags,
                       curve2 *Curve, real32 *ControlBuffer,
                       real32 const ErrorThreshold,
                       real32 const *IdentityVector,
                       real32 const *TransformedSamples,
                       real32 const *OriginalSamples,
                       int32x const Dimension,
                       int32x const SampleCount)
{
    Assert(Curve);
    Assert(TransformedSamples);
    Assert(OriginalSamples);
    Assert(CurveGetDimension(*Curve) == Dimension);
    Assert(SampleCount > 0);

    int32x const Degree   = CurveGetDegree(*Curve);

    // At this stage, the knots haven't been scaled to reflect the
    // desired dT.
    real32 const Duration = (real32)SampleCount - 1;

    // -----------------------------------------------------------------
    // This is much simpler than AddKnotIfAboveError, since we know
    // that:
    //  - we've already met our error threshold
    //  - we're only removing knots, so we don't have to worry
    //    about the knot limit.

    // Our strategy here is to just run through the knots in order,
    // and see if it violates the ErrorThreshold if we remove it.
    // This wound up being the simplest strategy that I tested, and
    // it's much faster than the alternatives.  For the record, here's
    // what was tried:
    //  - Sequentially test each knot (this strategy)
    //  - Test a random permutation of the knots.  This wound up being
    //  equivalent, and it's not nearly as nice, since it's
    //  non-deterministic.
    //  - Test all of the knots.  Remove the one that causes the least
    //  increase in the maximum error.  Loop until you cannot remove
    //  any more knots.  /Very/ slightly more efficient than the
    //  sequential test, but grotesquely slow in comparison.  By the
    //  time this algo has removed one knot, we're already done.
    //  Usually finds a way to remove maybe 1 or two knots extra in my
    //  tests.
    //  - Test all of the knots.  Remove the one that causes the least
    //  increase in the average (RMS) error.  Loop until you cannot
    //  remove any more knots.  Same deal as the previous algo.
    //  Slightly better at removing knots, but way too slow.

    // Simple wins for now.  Possibly some middle ground between
    // BrainDead and TooSlow can be found.  (Pick N random knots,
    // remove the lowest error knot).
    // -----------------------------------------------------------------

    // Figure out the allowed removal range.  We protect the first and
    // last knots, and if ExtraDOF is set, then we protect the second
    // knot as well.
    int32x const StartOffset = (SolverFlags & BSplineSolverExtraDOFKnotZero) ? 2 : 1;
    int32x const EndOffset   = 1;

    real32 *CurrKnots     = Solver.KnotBuffer0;
    real32 *OtherBuffer   = Solver.KnotBuffer1;

    int32x CurrKnotIdx   = StartOffset;
    int32x CurrKnotCount = CurveGetKnotCount(*Curve);

    // Prime the loop.  We start with the values from the original
    // curve.
    curve2 *CurrCurve     = NULL;
    CurveExtractKnotValues(*Curve, 0, CurrKnotCount, CurrKnots, NULL, IdentityVector);

    while (CurrKnotIdx < (CurrKnotCount - EndOffset) && CurrKnotCount > 3)
    {
        // Copy the knots into OtherBuffer, missing the current idx, of course...
        Copy32(CurrKnotIdx, CurrKnots, OtherBuffer);
        Copy32(CurrKnotCount - CurrKnotIdx - 1,
               CurrKnots + CurrKnotIdx + 1,
               OtherBuffer + CurrKnotIdx);
        int32x const NewKnotCount = CurrKnotCount - 1;

        // Solve for the new controls
        curve2 *PotentialCurve;
        {
            SolveSpline(Solver,
                        Dimension, Degree,
                        NewKnotCount, OtherBuffer,
                        SampleCount, TransformedSamples,
                        ControlBuffer);

            if(SolverFlags & BSplineSolverForceEndpointAlignment)
            {
                {for(int32x Element = 0;
                     Element < Dimension;
                     ++Element)
                {
                    ControlBuffer[Element] = TransformedSamples[Element];
                    ControlBuffer[Dimension*(NewKnotCount - 1) + Element] =
                        TransformedSamples[Dimension*(SampleCount - 1) + Element];
                }}
            }

            // Now make a curve out of those new control points.
            // That way, the error-finder can take all the quantisation, etc into account.
            curve_builder CurveBuilder;
            BeginCurveInPlace ( &CurveBuilder,
                                CurveGetDataTypeDefinition(*Curve),
                                Degree, Dimension,
                                NewKnotCount );
            PushCurveKnotArray ( &CurveBuilder, OtherBuffer );
            PushCurveControlArray ( &CurveBuilder, ControlBuffer );
            PushCurveSampleArrays(&CurveBuilder,
                                  SampleCount, Dimension,
                                  TransformedSamples, OriginalSamples);
            PotentialCurve = EndCurve( &CurveBuilder );
        }

        bspline_error ThisSampleError;
        GetSquaredErrorOverCurve(SolverFlags, Degree, Dimension,
                                 Duration, PotentialCurve,
                                 SampleCount, OriginalSamples,
                                 IdentityVector,
                                 &ThisSampleError);

        if (ThisSampleError.MaximumSquaredError <= Square(ErrorThreshold))
        {
            // Yes.
            //  Replace CurrCurve with PotentialCurve.
            if (CurrCurve)
            {
                FreeCurve(CurrCurve);
            }
            CurrCurve = PotentialCurve;

            //  Swap the knot buffers
            real32 *Temp = CurrKnots;
            CurrKnots = OtherBuffer;
            OtherBuffer = Temp;

            //  Set CurrKnotCount
            CurrKnotCount = NewKnotCount;

            //  Don't increment the index
        }
        else
        {
            // No.
            //  Free PotentialCurve
            FreeCurve(PotentialCurve);

            //  Leave CurrKnotCount alone
            //  Leave the knot buffers

            //  increment the index
            CurrKnotIdx++;
        }
    }

    // We need to restore the ControlBuffer to it's proper state.  This
    // applies whether or not we removed any knots, since we've called
    // SolveSpline many times.  One more here won't hurt.
    {
        // We never found a knot we could remove.  We have to put the
        // ControlBuffer back to it previous state.  Just resolve the
        // spline with the original knot vector.  (Still in CurrKnots
        // in this case.)
        SolveSpline(Solver,
                    Dimension, Degree,
                    CurrKnotCount, CurrKnots,
                    SampleCount, TransformedSamples,
                    ControlBuffer);

        if(SolverFlags & BSplineSolverForceEndpointAlignment)
        {
            {for(int32x Element = 0;
                 Element < Dimension;
                 ++Element)
            {
                ControlBuffer[Element] = TransformedSamples[Element];
                ControlBuffer[Dimension*(CurrKnotCount - 1) + Element] =
                    TransformedSamples[Dimension*(SampleCount - 1) + Element];
            }}
        }
    }

    return CurrCurve;
}



static void
AssignUniformKnots(int32x SampleCount, int32x KnotCount, real32 *Knots)
{
    real32  dt = (real32)(SampleCount - 1) / (real32)(KnotCount - 1);
    {for(int32x KnotIndex = 0;
         KnotIndex < KnotCount;
         ++KnotIndex)
    {
        Knots[KnotIndex] = (real32)KnotIndex * dt;
    }}
}

static real32
FirstDerivative(int32x Dimension, real32 *Samples, int32x SampleIndex)
{
    real32 ResultA = 0;
    real32 ResultB = 0;
    {for(int32x Element = 0;
         Element < Dimension;
         ++Element)
    {
        ResultA += Square(Samples[Dimension*SampleIndex + Element]
                          - Samples[Dimension*(SampleIndex - 1) + Element]);
        ResultB += Square(Samples[Dimension*(SampleIndex + 1) + Element]
                          - Samples[Dimension*SampleIndex + Element]);
    }}
    return(Maximum(ResultA, ResultB));
}

static real32
SecondDerivative(int32x Dimension, real32 *Samples, int32x SampleIndex)
{
    real32 Result = 0;
    {for(int32x Element = 0;
         Element < Dimension;
         ++Element)
    {
        Result += Square(2*Samples[Dimension*SampleIndex + Element]
                         -
                         (Samples[Dimension*(SampleIndex - 1) + Element]
                          + Samples[Dimension*(SampleIndex + 1) + Element]));
    }}
    return(Result);
}

static void
AssignMultipleKnots(uint32x Flags,
                    int32x Degree, int32x Dimension,
                    int32x SampleCount,
                    real32 *Samples,
                    int32x &KnotCount,
                    real32 *Knots,
                    real32 FirstDerivativeThreshold,
                    real32 SecondDerivativeThreshold)
{
    if(Flags & BSplineSolverExtraDOFKnotZero)
    {
        //BSplineSolverExtraDOFKnotZero:
        //
        //What this does is allow the solver some extra degrees of
        //freedom for the first knot. This gives it some knots to
        //help match the starting velocity of the value. Without this,
        //it adds a lot of extra knots (because of the binary subdivision)
        //when all it actually wants is one right after the start knot.
        //
        //Unfortunately, this adds a discontinuity in looping animations
        //that is visible under heavy compression, because without adding
        //any knots, just using the first, last, and this 0.1th-frame knot,
        //it gets a motion that is the right sort of shape, but is pretty
        //discontinuous at the start, so it doesn't loop at all smoothly.
        //So it's a troublesome thing.
        //
        //
        //Alternatives:
        //
        //1. Don't do it, and let the solver/corrector add the extra knots.
        //   Not that attractive - takes lots of memory.
        //
        //2. Add extra knots at -ve times to let the solver control the shape of
        //   the curve. The problem with this is that large chunks of the system
        //   is geared up so that the first- listed knot is at time 0.0, so that
        //   would all have to change.
        //
        //3. Add a "looping" flag so that if the anim is looping, we don't add
        //   this, and if it is, do. But users get that wrong all the time, and
        //   it's still not a perfect fix, because the _solver_ doesn't know that
        //   the anim loops, and modifying it so it does means writing a new solver
        //   (matrix is no longer diagonal).
        //
        //4. Just add it (back) as a flag to the exporter settings and let
        //   users play with it. Again, not the Right Thing, but might do as
        //   a short-term fix.
        //
        //5. Double-up the samples, so it loops twice, solve as normal, ensuring
        //   that whenever a knot is added in the second half, it's also added
        //   in the first half (in fact, never even measure error in the first
        //   half, which speeds things up a bit), and then just take the knots
        //   in the second half. Hopefully the solver should solve the first
        //   and second halves nearly identically. May still need an "add extra
        //   DOF" exporter flag for non-looping anims though - not sure.


        int32x InsertionCount = Degree - 1;
        while(InsertionCount > 0)
        {
            // Note - the reason this is 0.125f rather than 0.0f is that it avoids
            // nasty divide-by-zeros.
            // And the reason it's 0.125f rather than something like 0.1f is that
            // 0.125f is precisely representable. Very very slightly better compression.
            Knots[0] = (1.0f/8.0f)*InsertionCount;
            InsertKnot(0, KnotCount, Knots);
            Knots[0] = 0.0f;

            InsertionCount--;
        }
    }

    {for(int32x SampleIndex = 1;
         SampleIndex < (SampleCount - 1);
         ++SampleIndex)
    {
        real32 SD = SecondDerivative(Dimension, Samples, SampleIndex);
        real32 FD = FirstDerivative(Dimension, Samples, SampleIndex);
        int32x InsertionCount = 0;

        if((Flags & BSplineSolverAllowC1Splitting) &&
           (SD > Square(SecondDerivativeThreshold)))
        {
            InsertionCount = Degree;
        }

        if((Flags & BSplineSolverAllowC0Splitting) &&
           (FD > Square(FirstDerivativeThreshold)))
        {
            InsertionCount = Degree + 1;
        }

        int32x KnotIndex =
            FindKnot(KnotCount, Knots, (real32)SampleIndex);

        while(InsertionCount--)
        {
            InsertKnot(KnotIndex, KnotCount, Knots);
            Knots[KnotIndex] = (real32)SampleIndex;
        }
    }}
}


curve2* GRANNY
FitBSplineToSamples(bspline_solver &Solver,
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
                    int32x *CurveSizeInBytes)
{
    Assert(Dimension < MaximumBSplineDimension);

    // Now that we're pretransforming these, we need a copy of the
    // samples to work with.
    //--- todo: add a query to see if this is an identity transform?
    real32 *TransformedSamples = AllocateArray(Dimension * SampleCount, real32);
    {
        if (TransformedSamples == 0)
        {
            // OO memory.  Ouch
            InvalidCodePath("Out of memory");
            *AchievedTolerance = false;
            *CurveSizeInBytes = -1;

            return NULL;
        }

        Copy32(Dimension * SampleCount, Samples, TransformedSamples);
        TransformSamplesForCurve(CurveDataType, Dimension, SampleCount,
                                 TransformedSamples);
    }

    bool Dummy1;
    int32x Dummy2;
    if ( AchievedTolerance == NULL )
    {
        AchievedTolerance = &Dummy1;
    }
    if ( CurveSizeInBytes == NULL )
    {
        CurveSizeInBytes = &Dummy2;
    }


    *AchievedTolerance = true;
    *CurveSizeInBytes = -1;

    if ( CurveDataType == NULL )
    {
        CurveDataType = CurveDataDaK32fC32fType;
    }

    int32x KnotCount = 2;
    real32 *OldKnots = Solver.KnotBuffer0;
    real32 *NewKnots = Solver.KnotBuffer1;
    real32 *ResultControls = Solver.ControlBuffer0;
    AssignUniformKnots(SampleCount, KnotCount, OldKnots);
    AssignMultipleKnots(SolverFlags, Degree, Dimension,
                        SampleCount, TransformedSamples,
                        KnotCount, OldKnots,
                        C0Threshold, C1Threshold);
    // TODO: Should I do this?
    if(KnotCount == 2)
    {
        KnotCount = Degree ? Degree + 1 : 2;
        AssignUniformKnots(SampleCount, KnotCount, OldKnots);
    }

    int32x CurrentCurveKnotCount = -1;
    curve2 *ResultCurve = NULL;
    while(1)
    {
        // Clear out the previous result before we start again...
        if (ResultCurve)
        {
            FreeCurve(ResultCurve);
            ResultCurve = NULL;
        }

        SolveSpline(Solver,
                    Dimension, Degree,
                    KnotCount, OldKnots,
                    SampleCount, TransformedSamples,
                    ResultControls);

        if(SolverFlags & BSplineSolverForceEndpointAlignment)
        {
            {for(int32x Element = 0;
                 Element < Dimension;
                 ++Element)
            {
                ResultControls[Element] = TransformedSamples[Element];
                ResultControls[Dimension*(KnotCount - 1) + Element] =
                    TransformedSamples[Dimension*(SampleCount - 1) + Element];
            }}
        }

        // Now make a curve out of those new control points.
        // That way, the error-finder can take all the quantisation, etc into account.
        curve_builder* CurveBuilder = BeginCurve( CurveDataType, Degree, Dimension, KnotCount );
        PushCurveKnotArray ( CurveBuilder, OldKnots );
        PushCurveControlArray ( CurveBuilder, ResultControls );
        PushCurveSampleArrays ( CurveBuilder, SampleCount, Dimension, TransformedSamples, Samples );
        int32x CurveSize = GetResultingCurveSize ( CurveBuilder );
        *CurveSizeInBytes = CurveSize;
        ResultCurve = EndCurve( CurveBuilder );
        CurrentCurveKnotCount = KnotCount;

        if ( ResultCurve == NULL )
        {
            // Well that's exceptionally bad.  You've run out of memory
            InvalidCodePath("Out of memory");
            *AchievedTolerance = false;
            *CurveSizeInBytes = -1;

            return NULL;
        }

        if ( ( MaximumCurveSizeInBytes > 0 ) && ( CurveSize > MaximumCurveSizeInBytes ) )
        {
            // Oh, we already exceeded the size allowed. Oh well.
            *AchievedTolerance = false;
            *CurveSizeInBytes = -1;
            break;
        }

        if(KnotCount >= Solver.MaxKnotCount)
        {
            // We are not allowed to add any more knots than this, so
            // why bother trying?  Life is now futile for the lowly
            // spline solver; a cruel joke perpetuated by the calling
            // code for its own sadistic amusement.  Let us end this
            // here, now.
            *AchievedTolerance = false;
            *CurveSizeInBytes = -1;
            break;
        }

        bool HitKnotLimit;
        if(AddKnotIfAboveError(SolverFlags,
                               &KnotCount, NewKnots, Solver.MaxKnotCount,
                               ResultCurve, GlobalZero16, OldKnots,
                               ErrorThreshold, SampleCount,
                               Samples,  // note: not transformedsamples!
                               &HitKnotLimit))
        {
            real32 *Temp = NewKnots;
            NewKnots = OldKnots;
            OldKnots = Temp;
        }
        else
        {
            // We don't need to add any more knots to meet our
            // tolerance requirements.
            Assert ( KnotCount == CurrentCurveKnotCount );
            if ( HitKnotLimit )
            {
                // Well actually we do, but there were too many to begin with.
                *AchievedTolerance = false;
                *CurveSizeInBytes = -1;
            }
            else
            {
                *AchievedTolerance = true;

                if (SolverFlags & BSplineSolverAllowReduceKeys)
                {
                    // Excellent!  Now, let's go through and remove knots
                    // introduced by the solver that aren't necessary.
                    // Note that the knot scratch buffers are used by this
                    // function, so they're garbage after this point.
                    // We're about to bail on the loop anyway.
                    //   NOTE! The ResultControls are handled specially.  If
                    //   the curve should be replaced (CleanedCurve !=
                    //   NULL), then the ResultControls will be set to the
                    //   new values.  Otherwise, the result is the same as
                    //   the above SolveSpline.  (In fact, to save the
                    //   alloc, we simply run that routine again.)
                    curve2 *CleanedCurve =
                        RemoveUnnecessaryKnots(Solver, SolverFlags,
                                               ResultCurve, ResultControls, ErrorThreshold, GlobalZero16,
                                               TransformedSamples, Samples, Dimension, SampleCount);
                    if (CleanedCurve)
                    {
                        FreeCurve(ResultCurve);
                        ResultCurve = CleanedCurve;

                        // reset KnotCount, and the curve size...
                        CurrentCurveKnotCount = KnotCount = CurveGetKnotCount(*ResultCurve);
                        *CurveSizeInBytes = CurveGetSize(*ResultCurve);
                    }
                }
            }
            break;
        }
    }
    Assert ( KnotCount == CurrentCurveKnotCount );

    // We always return a result curve, even if we've missed our
    // tolerance, except in the case of allocation failure, which
    // returns NULL above.
    Assert(ResultCurve);

    // This is non-optimal, but recreate the curve with the knot
    // values scaled correctly.  Need to scale by the actual
    // inter-frame time delta.
    {
        CurveExtractKnotValues ( *ResultCurve, 0, KnotCount, NewKnots, NULL, GlobalZero16 );
        {for(int32x KnotIndex = 0; KnotIndex < KnotCount; ++KnotIndex)
        {
            NewKnots[KnotIndex] *= dT;
        }}
        FreeCurve(ResultCurve);

        curve_builder *CurveBuilder = BeginCurve( CurveDataType, Degree, Dimension, KnotCount );
        PushCurveKnotArray ( CurveBuilder, NewKnots );
        PushCurveControlArray ( CurveBuilder, ResultControls );
        PushCurveSampleArrays ( CurveBuilder, SampleCount, Dimension, TransformedSamples, Samples );
        ResultCurve = EndCurve( CurveBuilder );
    }

    // Release our temp transformed samples
    Deallocate(TransformedSamples);

    return ResultCurve;
}


void GRANNY
SolveSpline(bspline_solver &Solver,
            int32x Dimension, int32x Degree,
            int32x KnotCount, real32 *Knots,
            int32x SampleCount, real32 const *Samples,
            real32 *ResultControls)
{
    DebugBeginSolve();

    int32x *KnotStart = Solver.KnotStart;
    // NOTE THE EXTRA PLUS ONE!!!!!!!
    ZeroArray(KnotCount + Degree + 1 + 1, KnotStart);

    real32 *AScratchSpace = Solver.A;
    real32 *ATAScratchSpace = Solver.ATA;

    real32 ClampValue = 0.0f;
    if(KnotCount)
    {
        ClampValue = Knots[KnotCount - 1];
    }

    {for(int32x Overflow = 0;
         Overflow < Degree;
         ++Overflow)
    {
        Knots[KnotCount + Overflow] = ClampValue;
        Knots[-Overflow - 1] = -((real32)Overflow + 1.0f);
    }}

    // We leave room at the end of A and on both ends of
    // ATA so it can read/write freely
    real32 *Clear;
    int32x ClearCount;
    int32x OverhangSize = (Degree + 1) * (Degree + 1);

    // We leave room at the end of A so it can read/write freely
    // off the end
    Clear = AScratchSpace + SampleCount * (Degree + 1);
    ClearCount = OverhangSize;
    while(ClearCount--)
    {
        *Clear++ = 0.0f;
    }

    // Clear the top overhang
    Clear = ATAScratchSpace;
    ClearCount = OverhangSize;
    while(ClearCount--)
    {
        *Clear++ = 0.0f;
    }

    // Clear the bottom overhang
    Clear = ATAScratchSpace + OverhangSize + KnotCount*(Degree + 1);
    ClearCount = OverhangSize;
    while(ClearCount--)
    {
        *Clear++ = 0.0f;
    }

    // Start ATA after the top overhang
    real32 *ATA = ATAScratchSpace + OverhangSize;

    // Build
    BuildATAAndATb(Dimension, Degree,
                   KnotCount, Knots, KnotStart,
                   SampleCount, Samples,
                   AScratchSpace, ATA, ResultControls);

    // Solve
    SymmetricBandDiagonalCholeskyFactor(KnotCount, Degree + 1, ATA);
    SymmetricBandDiagonalCholeskySolve(KnotCount, Degree + 1, ATA,
                                       Dimension, ResultControls);

    DebugEndSolve();
}

real32 GRANNY
OrientationToleranceFromDegrees(real32 Degrees)
{
    CheckBoundedFloat32(0.0f, Degrees, GetReal32AlmostInfinity(), return 0.0f);

    return(1.0f - Cos(0.5f * RadiansFromDegrees(Degrees)));
}

real32 GRANNY
DegreesFromOrientationTolerance(real32 Tolerance)
{
    CheckBoundedFloat32(0.0f, Tolerance, 2.0f, return 180.0f);

    return DegreesFromRadians(2 * ACos(1.0f - Tolerance));
}
