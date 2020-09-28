// ========================================================================
// $File: //jeffr/granny/rt/granny_track_sampler.cpp $
// $DateTime: 2007/07/02 11:41:19 $
// $Change: 15387 $
// $Revision: #34 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRACK_SAMPLER_H)
#include "granny_track_sampler.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_BSPLINE_BUFFERS_H)
#include "granny_bspline_buffers.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_BSPLINE_H)
#include "granny_bspline.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_ANIMATION_BINDING_H)
#include "granny_animation_binding.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "granny_controlled_animation.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

#if 1
#define SAMPLE_TRACK_COUNT_BLOCK(a)
#else
#define SAMPLE_TRACK_COUNT_BLOCK(a) COUNT_BLOCK(a)
#endif


static void
EvaluateCurve(sample_context const &Context, int32x Dimension,
              curve2 const &Curve, real32 *Result, real32 const *IdentityVector,
              bool CurveIsNormalized)
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
        real32 tiBufferStack[6];
        real32 piBufferStack[6*9];

        real32 *tiBuffer;
        real32 *piBuffer;

        int32x Degree = CurveGetDegreeInline ( Curve );
        Assert ( ( Degree + 1 ) < ArrayLength ( tiBufferStack ) );
        Assert ( ( Degree + 1 ) * Dimension < ArrayLength ( piBufferStack ) );

        int32x KnotIndex = CurveFindKnot(Curve, Context.LocalClock);
        Assert ( Dimension == CurveGetDimensionUnchecked ( Curve ) );
        if(ConstructBSplineBuffers(Dimension,
                                   Context.UnderflowLoop ? &Curve : NULL,
                                   Curve,
                                   Context.OverflowLoop ? &Curve : NULL,
                                   Context.LocalDuration, Context.LocalDuration, Context.LocalDuration,
                                   KnotIndex,
                                   tiBufferStack, piBufferStack,
                                   tiBuffer, piBuffer, IdentityVector))
        {
            if ( CurveIsNormalized && ( Dimension == 4 ) )
            {
                EnsureQuaternionContinuity(Degree + 1, piBufferStack);
            }
        }

        if ( ( KnotIndex != CurveGetKnotCount ( Curve ) - 1 ) )
        {
            //Assert ( tiBuffer[0] > Context.LocalClock );            // This very very occasionally hits.
            Assert ( tiBuffer[0] > Context.LocalClock - 0.00001f );
        }
        //Assert ( tiBuffer[-1] <= Context.LocalClock );          // This very very occasionally hits.
        Assert ( tiBuffer[-1] <= Context.LocalClock + 0.00001f );

        SampleBSpline(Degree, Dimension, CurveIsNormalized,
                      tiBuffer, piBuffer,
                      Context.LocalClock, Result);
    }
}

#define SamplePosition(Position)                                                \
    {                                                                           \
        curve2 const &Curve = SourceTrack->PositionCurve;                       \
        int32x Degree = CurveGetDegreeInline ( Curve );                         \
        int32x const Dimension = 3;                                             \
        int32x KnotIndex = CurveFindKnot(Curve, Context.LocalClock);            \
        real32 tiBufferStack[6];                                                \
        real32 piBufferStack[6*Dimension];                                      \
        real32 *tiBuffer;                                                       \
        real32 *piBuffer;                                                       \
        Assert ( ( Degree + 1 ) < ArrayLength ( tiBufferStack ) );              \
        Assert ( ( Degree + 1 ) * Dimension < ArrayLength ( piBufferStack ) );  \
        ConstructBSplineBuffers(Dimension,                                      \
                                Context.UnderflowLoop ? &Curve : NULL,          \
                                Curve,                                          \
                                Context.OverflowLoop ? &Curve : NULL,           \
                                Context.LocalDuration,                          \
                                Context.LocalDuration,                          \
                                Context.LocalDuration,                          \
                                KnotIndex,                                      \
                                tiBufferStack, piBufferStack,                   \
                                tiBuffer, piBuffer, CurveIdentityPosition);     \
        UncheckedSampleBSpline(Degree, Dimension, tiBuffer, piBuffer,           \
                               Context.LocalClock, Position);                   \
    }

#define SampleOrientation(Orientation)                                              \
    {                                                                               \
        curve2 const &Curve = SourceTrack->OrientationCurve;                        \
        int32x Degree = CurveGetDegreeInline ( Curve );                             \
        int32x const Dimension = 4;                                                 \
        int32x KnotIndex = CurveFindKnot(Curve, Context.LocalClock);                \
        real32 tiBufferStack[6];                                                    \
        real32 piBufferStack[6*Dimension];                                          \
        real32 *tiBuffer;                                                           \
        real32 *piBuffer;                                                           \
        Assert ( ( Degree + 1 ) < ArrayLength ( tiBufferStack ) );                  \
        Assert ( ( Degree + 1 ) * Dimension < ArrayLength ( piBufferStack ) );      \
        if(ConstructBSplineBuffers(Dimension,                                       \
                                   Context.UnderflowLoop ? &Curve : NULL,           \
                                   Curve,                                           \
                                   Context.OverflowLoop ? &Curve : NULL,            \
                                   Context.LocalDuration,                           \
                                   Context.LocalDuration,                           \
                                   Context.LocalDuration,                           \
                                   KnotIndex,                                       \
                                   tiBufferStack, piBufferStack,                    \
                                   tiBuffer, piBuffer, CurveIdentityOrientation))   \
        {                                                                           \
            EnsureQuaternionContinuity(Degree + 1, piBufferStack);                  \
        }                                                                           \
                                                                                    \
        UncheckedSampleBSplineN(Degree, Dimension, tiBuffer, piBuffer,              \
                                Context.LocalClock, Orientation);                   \
    }


// Constant samplers...
#ifdef BOUND_TRANSFORM_TRACK_HAS_LOD
    #define SampleConstantPosition(Var)                     \
        VectorEquals3(Var, Track->LODTransform.Position)
    #define SampleConstantOrientation(Var)                      \
        VectorEquals4(Var, Track->LODTransform.Orientation)
#else
    #define SampleConstantPosition(Var)                                                     \
        CurveExtractKnotValue ( SourceTrack->PositionCurve, 0, Var, CurveIdentityPosition )
    #define SampleConstantOrientation(Var)                                                              \
        CurveExtractKnotValue ( SourceTrack->OrientationCurve, 0, Var, CurveIdentityOrientation )
#endif


#define BuildIdentity                                                       \
    {                                                                       \
        if (CompositeResult)                                                \
            BuildIdentityWorldPoseComposite(ParentMatrix, InverseWorld4x4,  \
                                            CompositeResult, WorldResult);  \
        else                                                                \
            BuildIdentityWorldPoseOnly(ParentMatrix, WorldResult);          \
    } typedef int Require__SemiColon

#define BuildPosition                                                                   \
    {                                                                                   \
        if (CompositeResult)                                                            \
            BuildPositionWorldPoseComposite(Position, ParentMatrix, InverseWorld4x4,    \
                                            CompositeResult, WorldResult);              \
        else                                                                            \
            BuildPositionWorldPoseOnly(Position, ParentMatrix, WorldResult);            \
    } typedef int Require__SemiColon

#define BuildPositionOrientation                                                        \
    {                                                                                   \
        if (CompositeResult)                                                            \
            BuildPositionOrientationWorldPoseComposite(Position, Orientation,           \
                                                       ParentMatrix, InverseWorld4x4,   \
                                                       CompositeResult, WorldResult);   \
        else                                                                            \
            BuildPositionOrientationWorldPoseOnly(Position, Orientation,                \
                                                  ParentMatrix, WorldResult);           \
    } typedef int Require__SemiColon

#define BuildTransform                                                              \
    {                                                                               \
        if (CompositeResult)                                                        \
            BuildFullWorldPoseComposite(Transform, ParentMatrix, InverseWorld4x4,   \
                                        CompositeResult, WorldResult);              \
        else                                                                        \
            BuildFullWorldPoseOnly(Transform, ParentMatrix, WorldResult);           \
    } typedef int Require__SemiColon

#define UNALIASED_SAMPLER_ARGS()                                    \
    real32 const* NOALIAS InverseWorld4x4 = InverseWorld4x4Aliased; \
    real32 const* NOALIAS ParentMatrix = ParentMatrixAliased;       \
    real32* NOALIAS WorldResult = WorldResultAliased;               \
    real32* NOALIAS CompositeResult = CompositeResultAliased


TrackSampler(SampleTrackCAI)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackCAI");
    UNALIASED_SAMPLER_ARGS();

    Assert(AreTrackFlagsConsistent(*Track));
    Assert(CurveIsConstantNotIdentity(SourceTrack->PositionCurve));
    Assert(!CurveIsConstantOrIdentity(SourceTrack->OrientationCurve));
    Assert(CurveIsIdentity(SourceTrack->ScaleShearCurve));
    Assert(CheckLODTransform( *Track, *SourceTrack ));

    triple Position;
    SampleConstantPosition(Position);

    quad Orientation;
    SampleOrientation(Orientation);

    // The default normalisation is enough now for 4n
    // Normalize4 ( Orientation );

    BuildPositionOrientation;
}

TrackSampler(SampleTrackCII)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackCII");
    UNALIASED_SAMPLER_ARGS();

    Assert(AreTrackFlagsConsistent(*Track));
    Assert(CurveIsConstantNotIdentity(SourceTrack->PositionCurve));
    Assert(CurveIsIdentity(SourceTrack->OrientationCurve));
    Assert(CurveIsIdentity(SourceTrack->ScaleShearCurve));
    Assert(CheckLODTransform(*Track, *SourceTrack));

    triple Position;
    SampleConstantPosition(Position);

    BuildPosition;
}

TrackSampler(SampleTrackCCI)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackCCI");
    UNALIASED_SAMPLER_ARGS();

    Assert(AreTrackFlagsConsistent(*Track));
    Assert(CurveIsConstantNotIdentity(SourceTrack->PositionCurve));
    Assert(CurveIsConstantNotIdentity(SourceTrack->OrientationCurve));
    Assert(CurveIsIdentity(SourceTrack->ScaleShearCurve));
    Assert(CheckLODTransform(*Track, *SourceTrack));

    triple Position;
    quad Orientation;
    SampleConstantPosition(Position);
    SampleConstantOrientation(Orientation);

    // I will trust that the orientation is already properly normalised,
    // so no need to call Normalize4 ( Orientation )

    BuildPositionOrientation;
}

TrackSampler(SampleTrackIII)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackIII");
    UNALIASED_SAMPLER_ARGS();

    // VDA calls this when the track does have data.
    Assert(AreTrackFlagsConsistent(*Track));
    //Assert(SourceTrack->PositionCurve.KnotCount == 0);
    //Assert(SourceTrack->OrientationCurve.KnotCount == 0);
    //Assert(SourceTrack->ScaleShearCurve.KnotCount == 0);

    BuildIdentity;
}


TrackSampler(SampleTrackAAI)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackAAI");
    UNALIASED_SAMPLER_ARGS();

    Assert(AreTrackFlagsConsistent(*Track));
    Assert(!CurveIsConstantOrIdentity(SourceTrack->PositionCurve));
    Assert(!CurveIsConstantOrIdentity(SourceTrack->OrientationCurve));
    Assert(CurveIsIdentity(SourceTrack->ScaleShearCurve));

    triple Position;
    SamplePosition(Position);

    quad Orientation;
    SampleOrientation(Orientation);

    // The default normalisation is enough now for 4n
    // Normalize4 ( Orientation );

    BuildPositionOrientation;
}

TrackSampler(SampleTrackAII)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackAII");
    UNALIASED_SAMPLER_ARGS();

    Assert(AreTrackFlagsConsistent(*Track));
    Assert(!CurveIsConstantOrIdentity(SourceTrack->PositionCurve));
    Assert(CurveIsIdentity(SourceTrack->OrientationCurve));
    Assert(CurveIsIdentity(SourceTrack->ScaleShearCurve));

    triple Position;
    SamplePosition(Position);

    BuildPosition;
}

TrackSampler(SampleTrackACI)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackACI");
    UNALIASED_SAMPLER_ARGS();

    Assert(AreTrackFlagsConsistent(*Track));
    Assert(!CurveIsConstantOrIdentity(SourceTrack->PositionCurve));
    Assert(CurveIsConstantNotIdentity(SourceTrack->OrientationCurve));
    Assert(CurveIsIdentity(SourceTrack->ScaleShearCurve));
    Assert(CheckLODTransform(*Track, *SourceTrack));

    triple Position;
    SamplePosition(Position);

    quad Orientation;
    SampleConstantOrientation(Orientation);

    // I will trust that the orientation is already properly normalised,
    // so no need to call Normalize4 ( Orientation )

    BuildPositionOrientation;
}


// 0 = assert on overruns, but don't do anything about them.
// 1 = clamp overruns to the allowed limits.
#define CLAMP_OVERRUNS 1


void GRANNY
SampleTrackUUULocal(sample_context const &Context,
                    transform_track const *SourceTrack,
                    bound_transform_track *Track,
                    transform &Result)
{
    Assert(AreTrackFlagsConsistent(*Track));
    Assert(CheckLODTransform(*Track, *SourceTrack));

    if ( BOUND_CURVE_CHECK(*Track, Position, IsKeyframed) )
    {
        // Just sample it directly.
        Result.Flags |= HasPosition;
        CurveExtractKnotValue ( SourceTrack->PositionCurve, Context.FrameIndex, Result.Position, CurveIdentityPosition );
    }
    else if ( BOUND_CURVE_CHECK(*Track, Position, IsIdentity) )
    {
        VectorZero3(Result.Position);
    }
#if BOUND_TRANSFORM_TRACK_HAS_LOD
    else if ( BOUND_CURVE_CHECK(*Track, Position, IsConstant) )
    {
        Result.Flags |= HasPosition;
        VectorEquals3(Result.Position, Track->LODTransform.Position);
    }
#endif
    else
    {
        Result.Flags |= HasPosition;
        EvaluateCurve(Context, 3, SourceTrack->PositionCurve, Result.Position, CurveIdentityPosition, false);
    }


    if ( BOUND_CURVE_CHECK(*Track, Orientation, IsKeyframed) )
    {
        // Just sample it directly.
        Result.Flags |= HasOrientation;
        CurveExtractKnotValue ( SourceTrack->OrientationCurve, Context.FrameIndex, Result.Orientation, CurveIdentityOrientation );
    }
    else if ( BOUND_CURVE_CHECK(*Track, Orientation, IsIdentity) )
    {
        VectorSet4(Result.Orientation, 0, 0, 0, 1);
    }
#if BOUND_TRANSFORM_TRACK_HAS_LOD
    else if ( BOUND_CURVE_CHECK(*Track, Orientation, IsConstant) )
    {
        Result.Flags |= HasOrientation;
        VectorEquals4(Result.Orientation, Track->LODTransform.Orientation);
    }
#endif
    else
    {
        Result.Flags |= HasOrientation;
        EvaluateCurve(Context, 4, SourceTrack->OrientationCurve, Result.Orientation, CurveIdentityOrientation, true);
    }


    if ( BOUND_CURVE_CHECK(*Track, ScaleShear, IsKeyframed) )
    {
        // Just sample it directly.
        Result.Flags |= HasScaleShear;
        CurveExtractKnotValue ( SourceTrack->ScaleShearCurve, Context.FrameIndex, Result.ScaleShear[0], CurveIdentityScaleShear );
    }
    else if ( BOUND_CURVE_CHECK(*Track, ScaleShear, IsIdentity) )
    {
        MatrixIdentity3x3(Result.ScaleShear);
    }
#if BOUND_TRANSFORM_TRACK_HAS_LOD
    else if ( BOUND_CURVE_CHECK(*Track, ScaleShear, IsConstant) )
    {
        Result.Flags |= HasScaleShear;
        MatrixEquals3x3((real32*)Result.ScaleShear,
                        (real32*)Track->LODTransform.ScaleShear);
    }
#endif
    else
    {
        Result.Flags |= HasScaleShear;
        EvaluateCurve(Context, 9, SourceTrack->ScaleShearCurve,
                      Result.ScaleShear[0], CurveIdentityScaleShear, false);
    }
}


void GRANNY
SampleTrackUUULocalAtTime0 ( transform_track const *SourceTrack, transform &Result )
{
    // NOTE! It is important that /this/ function not use the
    // LODTransform member to accelerate the sample.  We'll be using
    // this to /create/ LODTransform.

    // Subtlety: This is slightly mis-named, as it doesn't actually sample
    // the track at time 0, it takes the first control point.
    // If the track loops, and there isn't a forced multiple-knot
    // at the start, this may not actually be the value of the _curve_,
    // just the value of the _knot_.
    // But the calculation of the error metric should take
    // this into account, since it just compares the results of using
    // SampleTrackUUULocalAtTime0 vs SampleTrackUUULocal.
    Result.Flags = 0;
    Assert ( SourceTrack != NULL );
    if ( !CurveIsIdentity ( SourceTrack->PositionCurve ) )
    {
        Result.Flags |= HasPosition;
        CurveExtractKnotValue ( SourceTrack->PositionCurve, 0, Result.Position, CurveIdentityPosition );
    }
    else
    {
        VectorZero3 ( Result.Position );
    }

    if ( !CurveIsIdentity ( SourceTrack->OrientationCurve ) )
    {
        Result.Flags |= HasOrientation;
        CurveExtractKnotValue ( SourceTrack->OrientationCurve, 0, Result.Orientation, CurveIdentityOrientation );
    }
    else
    {
        VectorSet4 ( Result.Orientation, 0, 0, 0, 1 );
    }

    if ( !CurveIsIdentity ( SourceTrack->ScaleShearCurve ) )
    {
        Result.Flags |= HasScaleShear;
        CurveExtractKnotValue ( SourceTrack->ScaleShearCurve, 0, Result.ScaleShear[0], CurveIdentityScaleShear );
    }
    else
    {
        MatrixIdentity3x3 ( Result.ScaleShear );
    }
}



void GRANNY
SampleTrackPOLocal(sample_context const &Context,
                   transform_track const *SourceTrack,
                   bound_transform_track *Track,
                   real32 *ResultPosition,
                   real32 *ResultOrientation)
{
    if ( !CurveIsIdentity ( SourceTrack->PositionCurve ) )
    {
        EvaluateCurve(Context, 3, SourceTrack->PositionCurve, ResultPosition, CurveIdentityPosition, false);
    }
    else
    {
        VectorZero3(ResultPosition);
    }

    if ( !CurveIsIdentity ( SourceTrack->OrientationCurve ) )
    {
        EvaluateCurve(Context, 4, SourceTrack->OrientationCurve, ResultOrientation, CurveIdentityOrientation, true);
    }
    else
    {
        VectorSet4(ResultOrientation, 0, 0, 0, 1);
    }
}

TrackSampler(SampleTrackUUU)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackUUU");
    UNALIASED_SAMPLER_ARGS();

    transform Transform;
    Transform.Flags = 0;

    SampleTrackUUULocal(Context, SourceTrack, Track, Transform);

    BuildTransform;
}


TrackSampler(SampleTrackIIU)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackIIU");
    UNALIASED_SAMPLER_ARGS();
    Assert(AreTrackFlagsConsistent(*Track));
    Assert(CheckLODTransform(*Track, *SourceTrack));

    // This is called on the root bone for VDA, where we don't want the
    // pos or orn, just any scale/shear that exists.
    if ( CurveIsKeyframed ( SourceTrack->ScaleShearCurve ) )
    {
        // Just sample it directly.
        transform Transform;
        Transform.Flags = HasScaleShear;
        CurveExtractKnotValue ( SourceTrack->ScaleShearCurve, Context.FrameIndex, Transform.ScaleShear[0], CurveIdentityScaleShear );

        BuildTransform;
    }
    else if ( CurveIsIdentity ( SourceTrack->ScaleShearCurve ) )
    {
        BuildIdentity;
    }
    else
    {
        transform Transform;
        Transform.Flags = HasScaleShear;
        EvaluateCurve(Context, 9, SourceTrack->ScaleShearCurve, Transform.ScaleShear[0], CurveIdentityScaleShear, false);

        BuildTransform;
    }
}


void GRANNY
SampleTrackUUUAtTime0(sample_context const &Context,
                 transform_track const *SourceTrack,
                 bound_transform_track *Track,
                 transform const &RestTransform,
                 real32 const* InverseWorld4x4,
                 real32 const* ParentMatrix,
                 real32* WorldResult,
                 real32* CompositeResult)
{
    // TODO: get this better SSE-accelerated?
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackUUUAtTime0");

#if BOUND_TRANSFORM_TRACK_HAS_LOD
    Assert(CheckLODTransform(*Track, *SourceTrack));
    transform const &Transform = Track->LODTransform;
#else
    transform Transform;
    SampleTrackUUULocalAtTime0(SourceTrack, Transform);
#endif

    BuildTransform;
}

void GRANNY
SampleTrackUUUBlendWithTime0(sample_context const &Context,
        transform_track const *SourceTrack,
        bound_transform_track *Track,
        transform const &RestTransform,
        real32 const *InverseWorld4x4,
        real32 const *ParentMatrix,
        real32 *WorldResult,
        real32 *CompositeResult,
        real32 BlendAmount)
{
    // TODO: get this better SSE-accelerated? Hopefully it won't be called much at all.
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackUUUBlendWithTime0");

#if BOUND_TRANSFORM_TRACK_HAS_LOD
    Assert(CheckLODTransform(*Track, *SourceTrack));
    transform const &TransformApprox = Track->LODTransform;
#else
    transform TransformApprox;
    SampleTrackUUULocalAtTime0(SourceTrack, TransformApprox);
#endif

    transform Transform;
    Transform.Flags = 0;
    SampleTrackUUULocal(Context, SourceTrack, Track, Transform);
    LinearBlendTransform ( Transform, Transform, BlendAmount, TransformApprox );

    BuildTransform;
}

TrackSampler(SampleTrackSSS)
{
    SAMPLE_TRACK_COUNT_BLOCK("SampleTrackSSS");
    UNALIASED_SAMPLER_ARGS();
    Assert(AreTrackFlagsConsistent(*Track));

    transform const &Transform = RestTransform;

    BuildTransform;
}

#if 0       // Never actually used.
TrackSampler(StubSampler)
{
    SAMPLE_TRACK_COUNT_BLOCK("StubSampler");
    UNALIASED_SAMPLER_ARGS();
    Assert(AreTrackFlagsConsistent(*Track));

    MatrixIdentity4x4(WorldResult);
    MatrixIdentity4x4(CompositeResult);
}
#endif

track_sampler *Samplers[3][3][3] =
{
    // [Position][Orientation][Scale/Shear]
    // I = identity.
    // C = constant.
    // A = actual curve.
    // U = universal (any type)

    // [0][*][*]
    {{SampleTrackIII, SampleTrackUUU, SampleTrackUUU}, // [0][0][*]
     {SampleTrackUUU, SampleTrackUUU, SampleTrackUUU}, // [0][1][*]
     {SampleTrackUUU, SampleTrackUUU, SampleTrackUUU}},// [0][2][*]

    // [1][*][*]
    {{SampleTrackCII, SampleTrackUUU, SampleTrackUUU}, // [1][0][*]
     {SampleTrackCCI, SampleTrackUUU, SampleTrackUUU}, // [1][1][*]
     {SampleTrackCAI, SampleTrackUUU, SampleTrackUUU}},// [1][2][*]

    // [2][*][*]
    {{SampleTrackAII, SampleTrackUUU, SampleTrackUUU}, // [2][0][*]
     {SampleTrackACI, SampleTrackUUU, SampleTrackUUU}, // [2][1][*]
     {SampleTrackAAI, SampleTrackUUU, SampleTrackUUU}} // [2][2][*]
};

int32x GRANNY
CurveKnotCharacter(curve2 const &Curve)
{
    if ( CurveIsIdentity ( Curve ) )
    {
        return 0;
    }
    else if ( CurveIsConstantOrIdentity ( Curve ) )
    {
        return 1;
    }
    else
    {
        return 2;
    }
}

track_sampler *GRANNY
GetTrackSamplerFor(transform_track const &Track)
{
    if(TransformTrackHasKeyframedCurves(Track))
    {
        return(SampleTrackUUU);
    }

    return(Samplers
           [CurveKnotCharacter(Track.PositionCurve)]
           [CurveKnotCharacter(Track.OrientationCurve)]
           [CurveKnotCharacter(Track.ScaleShearCurve)]);
}

track_sampler *GRANNY
GetTrackSamplerUUU(void)
{
    return(SampleTrackUUU);
}

track_sampler *GRANNY
GetTrackSamplerSSS(void)
{
    return(SampleTrackSSS);
}

track_sampler *GRANNY
GetTrackSamplerIII(void)
{
    return(SampleTrackIII);
}

track_sampler *GRANNY
GetTrackSamplerIIU(void)
{
    return(SampleTrackIIU);
}

track_sampler *GRANNY
GetTrackSamplerUUUAtTime0(void)
{
    return(SampleTrackUUUAtTime0);
}



