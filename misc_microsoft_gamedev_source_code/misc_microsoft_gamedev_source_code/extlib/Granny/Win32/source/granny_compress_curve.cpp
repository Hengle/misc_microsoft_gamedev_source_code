// ========================================================================
// $File: //jeffr/granny/rt/granny_compress_curve.cpp $
// $DateTime: 2007/11/26 14:34:09 $
// $Change: 16612 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_COMPRESS_CURVE_H)
#include "granny_compress_curve.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_BSPLINE_SOLVER_H)
#include "granny_bspline_solver.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode CurveLogMessage


USING_GRANNY_NAMESPACE;

#if DEBUG
#define DEBUG_COMPRESSOR 1
#else
#define DEBUG_COMPRESSOR 0
#endif


// Not crazy thorough, but should catch the worst of it.
static bool
CompressionParamsAreValid(compress_curve_parameters const &Params,
                          uint32x SolverFlags)
{
    if (Params.DesiredDegree < 0)
        return false;

    if (Params.ErrorTolerance < 0.0f)
        return false;

    if ((SolverFlags & BSplineSolverAllowC0Splitting) && Params.C0Threshold < 0.0f)
        return false;
    if ((SolverFlags & BSplineSolverAllowC1Splitting) && Params.C1Threshold < 0.0f)
        return false;

    if (Params.PossibleCompressionTypesCount < 0)
        return false;
    {for(int32x Type = 0; Type < Params.PossibleCompressionTypesCount; ++Type)
    {
        if (Params.PossibleCompressionTypes[Type] == NULL)
            return false;
    }}

    if (Params.IdentityCompressionType != NULL && Params.IdentityVector == NULL)
        return false;

    return true;
}


static curve2*
Handle01KnotSampleArray(uint32x SolverFlags,
                        compress_curve_parameters const &Params,
                        real32 const *Samples,
                        int32x Dimension,
                        int32x FrameCount,
                        real32 dT,
                        bool *CurveAcheivedTolerance)
{
    vector_diff_mode DiffMode = ((SolverFlags & BSplineSolverEvaluateAsQuaternions) != 0 ?
                                 DiffAsQuaternions : DiffAsVectors);

    real32 IdentityDifference = 0.0f;
    real32 ConstantDifference = 0.0f;
    GetVectorDifferences(Dimension, FrameCount, Samples,
                         Params.IdentityVector,
                         DiffMode,
                         IdentityDifference, ConstantDifference);

    data_type_definition* CompressionType = 0;
    int32x ResultKnotCount = -1;
    if (FrameCount == 0 || IdentityDifference < Params.ErrorTolerance)
    {
        // return an identity curve...
        CompressionType = Params.IdentityCompressionType;
        ResultKnotCount = 0;
    }
    else
    {
        // Create a constant curve...
        Assert(FrameCount != 0);
        CompressionType = Params.ConstantCompressionType;
        ResultKnotCount = 1;
    }

    // Identity and Constant curves ignore the knot values, so we
    // only need to create one bogus knot.  The time doesn't
    // matter, so just pick 0
    real32 BogusKnot = 0.0f;

    // Create the curve, using the samples as the controls
    curve_builder* CurveBuilder = BeginCurve(CompressionType, 0, Dimension, ResultKnotCount);
    PushCurveKnotArray ( CurveBuilder, &BogusKnot );
    PushCurveControlArray ( CurveBuilder, Samples );
    PushCurveSampleArrays ( CurveBuilder, FrameCount, Dimension, Samples, Samples );

    return EndCurve( CurveBuilder );
}


curve2* GRANNY
CompressCurve(bspline_solver &Solver,
              uint32x SolverFlags,
              compress_curve_parameters const &Params,

              real32 const *Samples,
              int32x Dimension,
              int32x FrameCount,
              real32 dT,
              bool *CurveAcheivedTolerance)
{
    Assert ( Dimension < MaximumBSplineDimension );
    CheckCondition(CompressionParamsAreValid(Params, SolverFlags), return NULL);

    // To avoid cluttering up this function even more, ship this case out to it's own
    // function...
    if (FrameCount <= 1)
    {
        return Handle01KnotSampleArray(SolverFlags, Params, Samples, Dimension,
                                       FrameCount, dT, CurveAcheivedTolerance);
    }


    real32 const Duration = dT * (FrameCount - 1);

    curve2* ResultCurve = NULL;

    // Always do the CurveDataDaK32fC32fType curve type as our base
    bool AchievedTolerance = false;
    int32x SmallestCurveSize = 0;
    ResultCurve = FitBSplineToSamples(Solver, SolverFlags,
                                      Params.DesiredDegree,
                                      Params.ErrorTolerance,
                                      Params.C0Threshold,
                                      Params.C1Threshold,
                                      Samples, Dimension, FrameCount, dT,
                                      CurveDataDaK32fC32fType,
                                      0, &AchievedTolerance, &SmallestCurveSize);
    Assert(ResultCurve);

    // Even if this fails to achieve the tolerance, it will:
    // -still produce a valid curve.
    // -will have bailed because of some inbuilt limit (usually too many knots)
    // -for the given number of knots, got as close as it can.
    // So it's still the best solution available, it just might not
    // meet the tolerance (because the tolerance might have been too tight).

#if DEBUG_COMPRESSOR
    {
        int32x ResultKnotCount = CurveGetKnotCount(*ResultCurve);
        real32 *ResultKnots    = AllocateArray(ResultKnotCount, real32);
        real32 *ResultControls = AllocateArray(ResultKnotCount * Dimension, real32);

        if (ResultKnots && ResultControls)
        {
            CurveExtractKnotValues(*ResultCurve,
                                   0, ResultKnotCount,
                                   ResultKnots, ResultControls,
                                   Params.IdentityVector);

            curve2 TempCurve;
            curve_data_da_k32f_c32f TempCurveData;
            CurveMakeStaticDaK32fC32f ( &TempCurve, &TempCurveData, ResultKnotCount, Params.DesiredDegree,
                                        Dimension, ResultKnots, ResultControls );
            bspline_error InitialSampleError;
            GetSquaredErrorOverCurve(SolverFlags, Params.DesiredDegree, Dimension,
                                     Duration, &TempCurve, FrameCount,
                                     Samples, Params.IdentityVector, &InitialSampleError);
            if ( AchievedTolerance )
            {
                // Todo: has precision problems if ErrorTolerance is tiny (1e-06 or so).  Fix please.
                //Assert ( InitialSampleError.MaximumSquaredError <= Square ( Params.ErrorTolerance ) );
            }
        }

        Deallocate(ResultKnots);
        Deallocate(ResultControls);
    }
#endif

    int32x ResultDegree    = CurveGetDegree(*ResultCurve);
    int32x ResultKnotCount = CurveGetKnotCount(*ResultCurve);
    if(Params.AllowDegreeReduction)
    {
        vector_diff_mode DiffMode = ((SolverFlags & BSplineSolverEvaluateAsQuaternions) != 0 ?
                                     DiffAsQuaternions : DiffAsVectors);
        CurveIsReducible(ResultCurve, Params.IdentityVector, Params.ErrorTolerance,
                         DiffMode, ResultDegree, ResultKnotCount);
    }

    if ( ResultKnotCount == 0 || ResultKnotCount == 1)
    {
        data_type_definition *CompressionType = NULL;
        if (ResultKnotCount == 0)
        {
            Assert ( Params.IdentityCompressionType != NULL );
            Assert ( Params.IdentityVector != NULL );
            CompressionType = Params.IdentityCompressionType;
        }
        else
        {
            CompressionType = Params.ConstantCompressionType;
        }

        // Recreate the curve to be an identity of this type...
        FreeBuilderResult(ResultCurve);

        // If we're here, we certainly acheived our tolerance...
        if (CurveAcheivedTolerance != NULL)
        {
            *CurveAcheivedTolerance = true;
        }


        // Identity and Constant curves ignore the knot values, so we
        // only need to create one bogus knot.  The time doesn't
        // matter, so just pick 0
        real32 BogusKnot = 0.0f;

        // Create the curve, using the samples as the controls
        curve_builder* CurveBuilder = BeginCurve( CompressionType,
                                                  ResultDegree, Dimension, ResultKnotCount );
        PushCurveKnotArray ( CurveBuilder, &BogusKnot );
        PushCurveControlArray ( CurveBuilder, Samples );
        PushCurveSampleArrays ( CurveBuilder, FrameCount, Dimension, Samples, Samples );

        return EndCurve( CurveBuilder );
    }


    // We still need to find the size, because the FitBSplineToSamples above might have failed to
    // hit the tolerance required, in which case it won't know the size.
    int32x BaseCurveSize;
    bspline_error FPCurveError;
    {
        BaseCurveSize = CurveGetSize(*ResultCurve);
        Assert ( ( SmallestCurveSize == -1 ) || ( SmallestCurveSize == BaseCurveSize ) );
        SmallestCurveSize = BaseCurveSize;

        // And get the error bound achieved.
        GetSquaredErrorOverCurve(SolverFlags, ResultDegree, Dimension,
                                 Duration, ResultCurve, FrameCount,
                                 Samples, Params.IdentityVector, &FPCurveError);
    }


    // Now try the other formats.
    {for ( int32x CurveTypeNumber = 0;
           CurveTypeNumber < Params.PossibleCompressionTypesCount;
           CurveTypeNumber++ )
    {
        // We've already done this one, ignore it if it's passed in...
        if (Params.PossibleCompressionTypes[CurveTypeNumber] == CurveDataDaK32fC32fType)
            continue;

        bool ThisCurveAchievedTolerance = false;
        int32x NewCurveSize = 0;
        curve2 *PotentialCurve = FitBSplineToSamples(Solver, SolverFlags,
                                                     Params.DesiredDegree,
                                                     Params.ErrorTolerance,
                                                     Params.C0Threshold,
                                                     Params.C1Threshold,
                                                     Samples, Dimension, FrameCount, dT,
                                                     Params.PossibleCompressionTypes[CurveTypeNumber],
                                                     SmallestCurveSize,
                                                     &ThisCurveAchievedTolerance,
                                                     &NewCurveSize);
        Assert(PotentialCurve);

        if (Params.AllowReductionOnMissedTolerance &&
            ( !ThisCurveAchievedTolerance && !AchievedTolerance ) )
        {
            // If the floating point curve wasn't able to acheive it's
            // tolerance, then we might accept this.
            int32x MaybeSize = CurveGetSize(*PotentialCurve);

            if (MaybeSize < SmallestCurveSize)
            {
                bspline_error QuantizedError;
                GetSquaredErrorOverCurve(SolverFlags, ResultDegree, Dimension,
                                         Duration, PotentialCurve, FrameCount,
                                         Samples, Params.IdentityVector, &QuantizedError);


                float const SizeFactor = float(MaybeSize) / float(BaseCurveSize);
                Assert(SizeFactor > 0.0f);

                float const MaxFloatError = (real32)IntrinsicSquareRoot(FPCurveError.MaximumSquaredError);
                float const MaxQuantError = (real32)IntrinsicSquareRoot(QuantizedError.MaximumSquaredError);
                Assert(MaxFloatError > 0.0f);
                Assert(MaxQuantError > 0.0f);

                // This is a little odd, so here's what's going on.
                // We never allow the error to exceed 2x what the
                // float curve achieves, but a quantized curve has to
                // beat that by a factor relating to the resulting
                // curve size to qualify for replacement.  (2 - SF)
                // should always be between 1.0 and 2.0, and increases
                // as the size of the quantized curve goes down
                // relative to the float error.
                float const QualifyingError = MaxFloatError * (2.0f - SizeFactor);

                if (MaxQuantError <= QualifyingError)
                {
                    ThisCurveAchievedTolerance = true;
                    NewCurveSize = MaybeSize;
                }
            }
        }

        // We're going to take this one if it's smaller
        if ( ThisCurveAchievedTolerance && NewCurveSize < SmallestCurveSize )
        {
            Assert(NewCurveSize > 0);

            // And it's smaller.
            SmallestCurveSize = NewCurveSize;

            FreeBuilderResult(ResultCurve);
            ResultCurve = PotentialCurve;
            PotentialCurve = NULL;
        }
        else
        {
            // Lose the curve, it's not going to be the replacement (if it
            // was, we snatch it above, and null the pointer)
            if (PotentialCurve != NULL)
            {
                FreeBuilderResult(PotentialCurve);
            }
        }
    }}


    if (CurveAcheivedTolerance != NULL)
    {
        *CurveAcheivedTolerance = AchievedTolerance;
    }
    return ResultCurve;
}


