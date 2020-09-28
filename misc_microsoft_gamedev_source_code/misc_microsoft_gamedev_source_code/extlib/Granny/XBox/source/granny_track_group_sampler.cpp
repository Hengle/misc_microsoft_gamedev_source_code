// ========================================================================
// $File: //jeffr/granny/rt/granny_track_group_sampler.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TRACK_GROUP_SAMPLER_H)
#include "granny_track_group_sampler.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode TrackGroupLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct track_group_sampler
{
    bool   BlockedAllocation;
    int32x TransformCurveCount;

    int32x MaxSampleCount;
    int32x SampleCount;

    // Valid in all allocations
    real32** TrackPositionSamples;
    real32** TrackOrientationSamples;
    real32** TrackScaleShearSamples;

    // Only valid in blocked allocations
    int32x PositionSampleCount;
    real32 *PositionSamples;

    int32x OrientationSampleCount;
    real32 *OrientationSamples;

    int32x ScaleShearSampleCount;
    real32 *ScaleShearSamples;
};

END_GRANNY_NAMESPACE;

track_group_sampler *GRANNY
BeginSampledAnimation(int32x TransformCurveCount,
                      int32x SampleCount)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    track_group_sampler *Sampler;
    AggrAllocPtr(Allocator, Sampler);

    AggrAllocOffsetCountlessArrayPtr(Allocator, Sampler, TransformCurveCount, TrackPositionSamples);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Sampler, TransformCurveCount, TrackOrientationSamples);
    AggrAllocOffsetCountlessArrayPtr(Allocator, Sampler, TransformCurveCount, TrackScaleShearSamples);

    AggrAllocOffsetArrayPtr(Allocator, Sampler, SampleCount * TransformCurveCount * 3,
                            PositionSampleCount, PositionSamples);
    AggrAllocOffsetArrayPtr(Allocator, Sampler, SampleCount * TransformCurveCount * 4,
                            OrientationSampleCount, OrientationSamples);
    AggrAllocOffsetArrayPtr(Allocator, Sampler, SampleCount * TransformCurveCount * 9,
                            ScaleShearSampleCount, ScaleShearSamples);

    if(EndAggrAlloc(Allocator))
    {
        Sampler->BlockedAllocation = true;
        Sampler->TransformCurveCount = TransformCurveCount;
        Sampler->MaxSampleCount = SampleCount;
        Sampler->SampleCount = 0;

        {for(int32x Track = 0; Track < Sampler->TransformCurveCount; ++Track)
        {
            Sampler->TrackPositionSamples[Track] =
                (Sampler->PositionSamples + Track * (Sampler->MaxSampleCount * 3));
            Sampler->TrackOrientationSamples[Track] =
                (Sampler->OrientationSamples + Track * (Sampler->MaxSampleCount * 4));
            Sampler->TrackScaleShearSamples[Track] =
                (Sampler->ScaleShearSamples + Track * (Sampler->MaxSampleCount * 9));
        }}
    }

    return(Sampler);
}

track_group_sampler* GRANNY
BeginSampledAnimationNonBlocked(int32x TransformCurveCount, int32x SampleCount)
{
    track_group_sampler* Sampler = Allocate(track_group_sampler);
    if (Sampler)
    {
        ZeroStructure(*Sampler);
        Sampler->BlockedAllocation = false;
        Sampler->TransformCurveCount = TransformCurveCount;
        Sampler->MaxSampleCount = SampleCount;
        Sampler->SampleCount = 0;

        Sampler->TrackPositionSamples    = AllocateArray(TransformCurveCount, real32*);
        Sampler->TrackOrientationSamples = AllocateArray(TransformCurveCount, real32*);
        Sampler->TrackScaleShearSamples  = AllocateArray(TransformCurveCount, real32*);
        if (!Sampler->TrackPositionSamples ||
            !Sampler->TrackOrientationSamples ||
            !Sampler->TrackScaleShearSamples)
        {
            // Oh crap.  Bail.
            Deallocate(Sampler->TrackPositionSamples);
            Deallocate(Sampler->TrackOrientationSamples);
            Deallocate(Sampler->TrackScaleShearSamples);
            Deallocate(Sampler);
            Sampler = NULL;
        }
        else
        {
            ZeroArray(TransformCurveCount, Sampler->TrackPositionSamples);
            ZeroArray(TransformCurveCount, Sampler->TrackOrientationSamples);
            ZeroArray(TransformCurveCount, Sampler->TrackScaleShearSamples);

            // At this point, if we fail, we can bail using the EndSampledAnimation
            // function, so we have a slightly easier time of it.
            {for(int32x Track = 0; Track < TransformCurveCount; ++Track)
            {
                Sampler->TrackPositionSamples[Track]    = AllocateArray(SampleCount * 3, real32);
                Sampler->TrackOrientationSamples[Track] = AllocateArray(SampleCount * 4, real32);
                Sampler->TrackScaleShearSamples[Track]  = AllocateArray(SampleCount * 9, real32);

                if (Sampler->TrackPositionSamples[Track] &&
                    Sampler->TrackOrientationSamples[Track] &&
                    Sampler->TrackScaleShearSamples[Track])
                {
                    ZeroArray(SampleCount * 3, Sampler->TrackPositionSamples[Track]);
                    ZeroArray(SampleCount * 4, Sampler->TrackOrientationSamples[Track]);
                    ZeroArray(SampleCount * 9, Sampler->TrackScaleShearSamples[Track]);
                }
                else
                {
                    EndSampledAnimation(Sampler);
                    Sampler = NULL;
                    break;
                }
            }}
        }
    }

    return Sampler;
}

void GRANNY
EndSampledAnimation(track_group_sampler *Sampler)
{
    if (Sampler)
    {
        if (Sampler->BlockedAllocation)
        {
            Deallocate(Sampler);
        }
        else
        {
            // Have to tear this down piece by piece.
            Assert(Sampler->PositionSamples == NULL);
            Assert(Sampler->OrientationSamples == NULL);
            Assert(Sampler->ScaleShearSamples == NULL);

            {for(int32x Track = 0; Track < Sampler->TransformCurveCount; ++Track)
            {
                Deallocate(Sampler->TrackPositionSamples[Track]);
                Deallocate(Sampler->TrackOrientationSamples[Track]);
                Deallocate(Sampler->TrackScaleShearSamples[Track]);
            }}
            Deallocate(Sampler->TrackPositionSamples);
            Deallocate(Sampler->TrackOrientationSamples);
            Deallocate(Sampler->TrackScaleShearSamples);
            Deallocate(Sampler);
        }
    }
}

real32 *GRANNY
GetPositionSamples(track_group_sampler &Sampler, int32x TrackIndex)
{
    CheckCountedInt32(TrackIndex, Sampler.TransformCurveCount, return NULL);
    CheckPointerNotNull(Sampler.TrackPositionSamples[TrackIndex], return NULL);

    return Sampler.TrackPositionSamples[TrackIndex];
}

real32 *GRANNY
GetOrientationSamples(track_group_sampler &Sampler, int32x TrackIndex)
{
    CheckCountedInt32(TrackIndex, Sampler.TransformCurveCount, return NULL);
    CheckPointerNotNull(Sampler.TrackOrientationSamples[TrackIndex], return NULL);

    return Sampler.TrackOrientationSamples[TrackIndex];
}

real32 *GRANNY
GetScaleShearSamples(track_group_sampler &Sampler, int32x TrackIndex)
{
    CheckCountedInt32(TrackIndex, Sampler.TransformCurveCount, return NULL);
    CheckPointerNotNull(Sampler.TrackScaleShearSamples[TrackIndex], return NULL);

    return Sampler.TrackScaleShearSamples[TrackIndex];
}

void GRANNY
SetTransformSample(track_group_sampler &Sampler,
                   int32x TrackIndex,
                   real32 const *Position3,
                   real32 const *Orientation4,
                   real32 const *ScaleShear3x3)
{
    Assert(TrackIndex < Sampler.TransformCurveCount);
    Assert(Sampler.SampleCount < Sampler.MaxSampleCount);

    real32* PositionSamples    = GetPositionSamples(Sampler, TrackIndex);
    real32* OrientationSamples = GetOrientationSamples(Sampler, TrackIndex);
    real32* ScaleShearSamples  = GetScaleShearSamples(Sampler, TrackIndex);

    Copy32(3, Position3,     &PositionSamples[Sampler.SampleCount * 3]);
    Copy32(4, Orientation4,  &OrientationSamples[Sampler.SampleCount * 4]);
    Copy32(9, ScaleShear3x3, &ScaleShearSamples[Sampler.SampleCount * 9]);
}

void GRANNY
PushSampledFrame(track_group_sampler &Sampler)
{
    Assert(Sampler.SampleCount < Sampler.MaxSampleCount);
    ++Sampler.SampleCount;
}
