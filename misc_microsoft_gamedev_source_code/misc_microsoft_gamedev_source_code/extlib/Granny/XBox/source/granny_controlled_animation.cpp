// ========================================================================
// $File: //jeffr/granny/rt/granny_controlled_animation.cpp $
// $DateTime: 2007/08/21 13:33:41 $
// $Change: 15778 $
// $Revision: #56 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "granny_controlled_animation.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_MODEL_CONTROL_BINDING_H)
#include "granny_model_control_binding.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_CONTROL_H)
#include "granny_control.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "granny_model_instance.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_BSPLINE_H)
#include "granny_bspline.h"
#endif

#if !defined(GRANNY_BSPLINE_BUFFERS_H)
#include "granny_bspline_buffers.h"
#endif

#if !defined(GRANNY_TRACK_MASK_H)
#include "granny_track_mask.h"
#endif

#if !defined(GRANNY_LOCAL_POSE_H)
#include "granny_local_pose.h"
#endif

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_DEGREE_OF_FREEDOM_H)
#include "granny_degree_of_freedom.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_PERIODIC_LOOP_H)
#include "granny_periodic_loop.h"
#endif

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_ANIMATION_BINDING_H)
#include "granny_animation_binding.h"
#endif

#if !defined(GRANNY_TRACK_SAMPLER_H)
#include "granny_track_sampler.h"
#endif

#if !defined(GRANNY_PREFETCH_H)
#include "granny_prefetch.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode ControlLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct track_target
{
    animation_binding *Binding;
    animation_binding_identifier BindingID;

    model_instance *OnInstance;

    accumulation_mode AccumulationMode;
    real32 LODCopyScaler;

    track_mask *TrackMask;
    track_mask *ModelMask;
};

struct controlled_animation_builder
{
    control *Control;

    real32 CurrentClock;
    animation const *Animation;

    int32x TrackCount;
    track_target *Tracks;
};


// Ewwww. A global.
real32 Global_AllowedLODErrorFadingFactor = 0.8f;


END_GRANNY_NAMESPACE;

static bool
AnimationInitializeBindingState(model_control_binding &Binding,
                                void *InitData)
{
    controlled_animation *State = GET_DERIVED(controlled_animation, Binding);

    track_target *TrackTarget = (track_target *)InitData;
    Assert(TrackTarget);

    if(TrackTarget->Binding)
    {
        State->Binding = AcquireAnimationBinding(TrackTarget->Binding);
    }
    else
    {
        State->Binding = AcquireAnimationBindingFromID(TrackTarget->BindingID);
    }
    State->AccumulationMode = TrackTarget->AccumulationMode;

    State->TrackMask = TrackTarget->TrackMask;
    State->ModelMask = TrackTarget->ModelMask;

    Assert(State->Binding);
    Assert(State->Binding->TrackBindings);

    Binding.ReservedPointer = (void*)ControlledAnim;

    return(State->Binding != 0);
}

void GRANNY
FindAllowedErrorNumbers ( real32 AllowedError, real32 *AllowedErrorEnd, real32 *AllowedErrorScaler )
{
    // So the fade out begins at AllowedError and ends at AllowedErrorEnd - below AllowedErrorEnd, we just sample the first value.
    *AllowedErrorEnd = 0.0f;
    *AllowedErrorScaler = 0.0f;
    if ( AllowedError > 0.0f )
    {
        *AllowedErrorEnd = AllowedError * Global_AllowedLODErrorFadingFactor;
        *AllowedErrorScaler = 1.0f / ( AllowedError - *AllowedErrorEnd );
    }
}

bool GRANNY
CheckLODTransform ( bound_transform_track const &Track, transform_track const &SourceTrack )
{
#if DEBUG && defined(BOUND_TRANSFORM_TRACK_HAS_LOD)
    // To make sure my LOD stuff is working properly.
    transform RealTransform;
    SampleTrackUUULocalAtTime0 ( &SourceTrack, RealTransform );
    transform const &LODTransform = Track.LODTransform;
    triple v;

    Assert ( LODTransform.Flags == RealTransform.Flags );
    if ( RealTransform.Flags & HasOrientation )
    {
        Assert ( InnerProduct4 ( LODTransform.Orientation, RealTransform.Orientation ) > 0.999f );
    }
    if ( RealTransform.Flags & HasPosition )
    {
        VectorSubtract3 ( v, LODTransform.Position, RealTransform.Position );
        Assert ( VectorLengthSquared3 ( v ) < 0.0001f );
    }
    if ( RealTransform.Flags & HasScaleShear )
    {
        VectorSubtract3 ( v, LODTransform.ScaleShear[0], RealTransform.ScaleShear[0] );
        Assert ( VectorLengthSquared3 ( v ) < 0.0001f );
        VectorSubtract3 ( v, LODTransform.ScaleShear[1], RealTransform.ScaleShear[1] );
        Assert ( VectorLengthSquared3 ( v ) < 0.0001f );
        VectorSubtract3 ( v, LODTransform.ScaleShear[2], RealTransform.ScaleShear[2] );
        Assert ( VectorLengthSquared3 ( v ) < 0.0001f );
    }
#endif

    return true;
}


static void
AnimationAccumulateBindingState(model_control_binding &Binding,
                                int32x FirstBone, int32x BoneCount,
                                local_pose &Result, real32 AllowedError,
                                int32x const *SparseBoneArray)
{
    COUNT_BLOCK("AnimationAccumulateBindingState");

    controlled_animation *State = GET_DERIVED(controlled_animation, Binding);

    real32 AllowedErrorEnd;
    real32 AllowedErrorScaler;
    FindAllowedErrorNumbers ( AllowedError, &AllowedErrorEnd, &AllowedErrorScaler );

    real32 const ControlWeight = GetControlEffectiveWeight(*Binding.Control);
    if(ControlWeight > TrackWeightEpsilon)
    {
        sample_context Context;
        Context.LocalClock = GetControlClampedLocalClock(*Binding.Control);
        Context.FrameIndex =
            TruncateReal32ToInt32(Context.LocalClock /
                                  State->Binding->ID.Animation->TimeStep);
        Context.LocalDuration = GetControlLocalDuration(*Binding.Control);

        GetControlLoopState(*Binding.Control,
                            Context.UnderflowLoop,
                            Context.OverflowLoop);
        {for(int32x LocalPoseBoneIndex = FirstBone;
             LocalPoseBoneIndex < (FirstBone + BoneCount);
             ++LocalPoseBoneIndex)
        {
            // Prefetches
            //  Note that this looks pretty hefty, but here's what's going on.
            //  Fetch the bound_transform_track N indices ahead
            //  Fetch the transform_track N-1 indices ahead
            //  Fetch the curves N-2 indices ahead
            const int32x SampleAhead = 4;
            {
                if (LocalPoseBoneIndex < (FirstBone + BoneCount) - SampleAhead)
                {
                    int32x SkeletonBoneIndex = LocalPoseBoneIndex+SampleAhead;
                    if ( SparseBoneArray != NULL )
                        SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex+SampleAhead];
                    PrefetchAddress(&State->Binding->TrackBindings[SkeletonBoneIndex], sizeof(bound_transform_track));
                }
                if (LocalPoseBoneIndex < (FirstBone + BoneCount) - (SampleAhead-1))
                {
                    int32x SkeletonBoneIndex = LocalPoseBoneIndex+(SampleAhead-1);
                    if ( SparseBoneArray != NULL )
                        SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex+(SampleAhead-1)];
                    bound_transform_track &Track = State->Binding->TrackBindings[SkeletonBoneIndex];
                    if (Track.SourceTrack)
                        PrefetchAddress(Track.SourceTrack, sizeof(*Track.SourceTrack));
                }
                if (LocalPoseBoneIndex < (FirstBone + BoneCount) - (SampleAhead-2))
                {
                    int32x SkeletonBoneIndex = LocalPoseBoneIndex+(SampleAhead-2);
                    if ( SparseBoneArray != NULL )
                        SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex+(SampleAhead-2)];
                    bound_transform_track &Track = State->Binding->TrackBindings[SkeletonBoneIndex];
                    if (Track.SourceTrack)
                    {
                        PrefetchAddress(Track.SourceTrack->OrientationCurve.CurveData.Object, 384);
                    }
                }
            }

            int SkeletonBoneIndex = LocalPoseBoneIndex;
            if ( SparseBoneArray != NULL )
            {
                // This is a remapping from the data in the local pose array to the actual skeleton bones.
                SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex];
            }

            bound_transform_track &Track =
                State->Binding->TrackBindings[SkeletonBoneIndex];
            if(Track.SourceTrack)
            {
                transform_track const &SourceTrack = *Track.SourceTrack;

                real32 Weight = ControlWeight;

                if(State->TrackMask)
                {
                    Weight *= GetTrackMaskBoneWeight(*State->TrackMask,
                                                     Track.SourceTrackIndex);
                }

                if(State->ModelMask)
                {
                    Weight *= GetTrackMaskBoneWeight(*State->ModelMask,
                                                     SkeletonBoneIndex);
                }

                if(Weight > TrackWeightEpsilon)
                {
                    transform SampledTransform;
                    // Note careful setting of the == case. If the AllowedError is zero (i.e. they want no LOD),
                    // we can still use the LOD if the error is also zero. This happens
                    // when the track is constant (which is pretty common), and it's
                    // quite a bit faster to do the LOD thing than to actually go
                    // and sample it, because the number of calls and memory hits
                    // is lower.
                    if ( AllowedErrorEnd >= Track.LODError )
                    {
                        // Cool - we don't need to sample this track properly,
                        // just use the most convenient (first) value.
                        Assert ( AllowedError >= Track.LODError );

#if BOUND_TRANSFORM_TRACK_HAS_LOD
                        Assert(CheckLODTransform(Track, SourceTrack));
                        SampledTransform = Track.LODTransform;
#else
                        SampleTrackUUULocalAtTime0 ( &SourceTrack, SampledTransform );
#endif
                    }
                    else if ( AllowedError >= Track.LODError )
                    {
                        // Some blend between correct and approximated. Doesn't have to be in any way
                        // "right" (we're below the error threshold), just has to be smooth.
                        Assert ( AllowedErrorEnd < Track.LODError );

#if BOUND_TRANSFORM_TRACK_HAS_LOD
                        Assert(CheckLODTransform(Track, SourceTrack));
                        transform const &SampledTransformApprox = Track.LODTransform;
#else
                        transform SampledTransformApprox;
                        SampleTrackUUULocalAtTime0 ( &SourceTrack, SampledTransformApprox );
#endif

                        real32 BlendFactor = ( AllowedError - Track.LODError ) * AllowedErrorScaler;
                        SampleTrackUUULocal(Context, &SourceTrack, &Track,
                                            SampledTransform);

                        LinearBlendTransform ( SampledTransform, SampledTransform, BlendFactor, SampledTransformApprox );
                    }
                    else
                    {
                        // No LOD.
                        Assert ( AllowedError < Track.LODError );
                        Assert ( AllowedErrorEnd < Track.LODError );

                        SampledTransform.Flags = 0;
                        SampleTrackUUULocal(Context, &SourceTrack, &Track,
                                            SampledTransform);
                    }

                    if((State->AccumulationMode == VariableDeltaAccumulation) &&
                       (SkeletonBoneIndex == 0))
                    {
                        VectorZero3(SampledTransform.Position);
                        VectorEquals4(SampledTransform.Orientation, GlobalWAxis);
                        // But don't kill any scale/shear.
                    }

                    AccumulateLocalTransform(Result, LocalPoseBoneIndex, SkeletonBoneIndex, Weight,
                                             *GetSourceSkeleton(*Binding.ModelInstance),
                                             quaternion_mode(Track.QuaternionMode),
                                             SampledTransform);
                }
            }
        }}
    }
}


static void
AnimationBuildDirect(model_control_binding &Binding, int32x BoneCount,
                     real32 const *Offset4x4, world_pose &Result, real32 AllowedError)
{
    COUNT_BLOCK("AnimationBuildDirect");

    controlled_animation *State = GET_DERIVED(controlled_animation, Binding);

    real32 AllowedErrorEnd;
    real32 AllowedErrorScaler;
    FindAllowedErrorNumbers ( AllowedError, &AllowedErrorEnd, &AllowedErrorScaler );

    sample_context Context;
    // NOTE!  Keyframed animations are not supported in direct build
    Context.FrameIndex = 0;
    Context.LocalClock = GetControlClampedLocalClockInline(*Binding.Control);
    Context.LocalDuration = GetControlLocalDurationInline(*Binding.Control);
    GetControlLoopStateInline(*Binding.Control,
                              Context.UnderflowLoop,
                              Context.OverflowLoop);

    matrix_4x4 *WorldBuffer = Result.WorldTransformBuffer;

    skeleton *Skeleton = Binding.ModelInstance->Model->Skeleton;
    bone *Bone = Skeleton->Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = Result.CompositeTransformBuffer;  // Could be NULL

    bound_transform_track *Track = State->Binding->TrackBindings;

    if(BoneCount > State->Binding->TrackBindingCount)
    {
        BoneCount = State->Binding->TrackBindingCount;
    }

    int32x BoneIndex = 0;
    if((State->AccumulationMode == VariableDeltaAccumulation) &&
       Track->SourceTrack)
    {
        // VDA means we don't want the orientation or position,
        // but we still want the scale factor.

        GetTrackSamplerIIU()(Context, Track->SourceTrack, Track,
                             Bone->LocalTransform,
                             (real32 *)Bone->InverseWorld4x4,
                             (Bone->ParentIndex == NoParentBone) ?
                             Offset4x4 :
                             (real32 const *)WorldBuffer[Bone->ParentIndex],
                             (real32 *)World, (real32 *)Composite);

        ++BoneIndex;
        ++Bone;
        ++World;
        ++Track;

        // Only advance this if we have a composite buffer
        if (Composite)
            ++Composite;
    }

    // Used in the prefetch logic...
    int32x const Prefetch4x4Count = PLATFORM_CACHE_LINE_SIZE / sizeof(*World);
    int32x const Base4x4Mod       = BoneIndex % Prefetch4x4Count;
    CompileAssert(sizeof(*World) == sizeof(*Composite));

    {for(;
         BoneIndex < BoneCount;
         ++BoneIndex)
    {
        // Note that the prefetch Track+SampleAhead is not a typo.
        // Because we prefetch the source track N-2 ahead, we need to
        // prefetch the bound track N-3 ahead to ensure that we don't
        // stall on that dereference, etc, etc
        const int32x SampleAhead = 4;
        {
            PrefetchAddress(Track+SampleAhead,sizeof(*Track));
            PrefetchAddress(Bone+1,sizeof(*Bone));

            if ((BoneIndex % Prefetch4x4Count) == Base4x4Mod)
            {
                PrefetchAddress(((uint8*)World) + PLATFORM_CACHE_LINE_SIZE, 1);
                if (Composite)
                    PrefetchAddress(((uint8*)Composite) + PLATFORM_CACHE_LINE_SIZE, 1);
            }

            if (BoneIndex < BoneCount-(SampleAhead-1) && (Track+(SampleAhead-1))->SourceTrack != NULL)
            {
                PrefetchAddress((Track+(SampleAhead-1))->SourceTrack, sizeof(*(Track+(SampleAhead-1))->SourceTrack));
            }
            if (BoneIndex < BoneCount-(SampleAhead-2) && (Track+(SampleAhead-2))->SourceTrack != NULL)
            {
                PrefetchAddress((Track+(SampleAhead-2))->SourceTrack->OrientationCurve.CurveData.Object, 384);
            }
        }

        transform_track const *SourceTrack = Track->SourceTrack;

        // If we have a track_mask or a model mask, we need to check to see if the bone is
        // weighted out, in which case, we use the base transform of the bone, rather than
        // the animated transform.
        if ((State->TrackMask && GetTrackMaskBoneWeight(*State->TrackMask,
                                                        Track->SourceTrackIndex) < TrackWeightEpsilon) ||
            (State->ModelMask && GetTrackMaskBoneWeight(*State->ModelMask,
                                                        BoneIndex) < TrackWeightEpsilon))
        {
            if (Composite)
                BuildFullWorldPoseComposite(Bone->LocalTransform,
                                            ((Bone->ParentIndex == NoParentBone) ?
                                             Offset4x4 :
                                             (real32 const *)WorldBuffer[Bone->ParentIndex]),
                                            (real32 const*)Bone->InverseWorld4x4,
                                            (real32*)Composite,
                                            (real32*)World);
            else
                BuildFullWorldPoseOnly(Bone->LocalTransform,
                                       ((Bone->ParentIndex == NoParentBone) ?
                                        Offset4x4 :
                                        (real32 const *)WorldBuffer[Bone->ParentIndex]),
                                       (real32*)World);
        }
        else
        {
            // Note careful setting of the == case. If the AllowedError is zero (i.e. they want no LOD),
            // we can still use the LOD if the error is also zero. This happens
            // when the track is constant (which is pretty common), and it's
            // quite a bit faster to do the LOD thing than to actually go
            // and sample it, because the number of calls and memory hits
            // is lower.
            if ( AllowedErrorEnd >= Track->LODError )
            {
                Assert ( AllowedError >= Track->LODError );
                // Cool - we don't need to sample this track properly,
                // just use the most convenient (first) value.

                SampleTrackUUUAtTime0(Context, SourceTrack, Track,
                                      Bone->LocalTransform,
                                      (real32 *)Bone->InverseWorld4x4,
                                      (Bone->ParentIndex == NoParentBone) ?
                                      Offset4x4 :
                                      (real32 const *)WorldBuffer[Bone->ParentIndex],
                                      (real32 *)World, (real32 *)Composite);
            }
            else if ( AllowedError >= Track->LODError )
            {
                Assert ( AllowedErrorEnd < Track->LODError );
                // Some blend between correct and approximated. Doesn't have to be in any way
                // "right" (we're below the error threshold), just has to be smooth.

                real32 BlendFactor = ( AllowedError - Track->LODError ) * AllowedErrorScaler;
                SampleTrackUUUBlendWithTime0(Context, SourceTrack, Track,
                                             Bone->LocalTransform,
                                             (real32 *)Bone->InverseWorld4x4,
                                             (Bone->ParentIndex == NoParentBone) ?
                                             Offset4x4 :
                                             (real32 const *)WorldBuffer[Bone->ParentIndex],
                                             (real32 *)World, (real32 *)Composite,
                                             BlendFactor);
            }
            else
            {
                Assert ( AllowedError < Track->LODError );
                Assert ( AllowedErrorEnd < Track->LODError );
                // No LOD.

                Track->Sampler(Context, SourceTrack, Track,
                               Bone->LocalTransform,
                               (real32 *)Bone->InverseWorld4x4,
                               (Bone->ParentIndex == NoParentBone) ?
                               Offset4x4 :
                               (real32 const *)WorldBuffer[Bone->ParentIndex],
                               (real32 *)World, (real32 *)Composite);
            }
        }

        ++Bone;
        ++World;
        ++Track;

        // Only advance this if we have a composite buffer
        if (Composite)
            ++Composite;
    }}
}


real32 GRANNY
GetGlobalLODFadingFactor ( void )
{
    return Global_AllowedLODErrorFadingFactor;
}

void GRANNY
SetGlobalLODFadingFactor ( real32 NewValue )
{
    if ( NewValue > 1.0f )
    {
        NewValue = 1.0f;
    }
    else if ( NewValue < 0.0f )
    {
        NewValue = 0.0f;
    }
    Global_AllowedLODErrorFadingFactor = NewValue;
}



static void
AnimationAccumulateLoopTransform(model_control_binding &Binding,
                                 real32 SecondsElapsed,
                                 real32 &TotalWeight,
                                 real32 *ResultTranslation,
                                 real32 *ResultRotation,
                                 bool Inverse)
{
    // Remember that ResultTranslation and ResultRotation are already valid,
    // and we need to accumulate this animation's motion to it.
    // (so it's not a bug that I don't initialize them to zero!)

    // Should also add a bit to the docs saying that these use the control's speed
    // to know how much local time has elapsed. If people are setting the clocks every
    // frame, then normally they don't need to set the speeds - but they still do have to
    // with CME or VDA.

    controlled_animation *State = GET_DERIVED(controlled_animation, Binding);

    if ( State->AccumulationMode == NoAccumulation )
    {
        return;
    }

    // Find out what weight this anim is applied at
    real32 const Weight = GetControlEffectiveWeight(*Binding.Control);
    if(Weight <= TrackWeightEpsilon)
    {
        return;
    }

    // Get bone zero
    bound_transform_track *BoundTrack = State->Binding->TrackBindings;
    if(!BoundTrack)
    {
        return;
    }

    transform_track const *SourceTrack = BoundTrack->SourceTrack;
    if(!SourceTrack)
    {
        return;
    }

    real32 LocalDuration = GetControlLocalDuration(*Binding.Control);

    if ( LocalDuration <= TimeEffectivelyZero )
    {
        // Zero-length animations are not going to have any motion.
        return;
    }

    // Adjust the seconds elapsed to be in the local space of the animation
    SecondsElapsed *= GetControlSpeed(*Binding.Control);

    // Find the local clock start and end.
    real32 EndLocalClock = GetControlRawLocalClock(*Binding.Control);
    real32 StartLocalClock = EndLocalClock - SecondsElapsed;

#if DEBUG
    // Before we start modifying them.
    real32 OriginalStartLocalClock = StartLocalClock;
    real32 OriginalEndLocalClock = EndLocalClock;
    OriginalStartLocalClock = OriginalStartLocalClock;      // Stops silly warnings.
    OriginalEndLocalClock = OriginalEndLocalClock;
#endif

    bool FlipResult = Inverse;
    if ( StartLocalClock > EndLocalClock )
    {
        // Negative SecondsElapsed or control speed.
        Swap ( StartLocalClock, EndLocalClock );
        FlipResult = !FlipResult;
    }

    // Clamp the effective start and end times to the animation start
    // and end times if there is no looping on that end.
    bool UnderflowLoop;
    bool OverflowLoop;
    GetControlLoopState(*Binding.Control,
                        UnderflowLoop,
                        OverflowLoop);
    if ( !UnderflowLoop )
    {
        // Clamp off start.
        if ( EndLocalClock < 0.0f )
        {
            Assert ( StartLocalClock < 0.0f );
            // Easy case - nothing happens.
            return;
        }
        else if ( StartLocalClock < 0.0f )
        {
            StartLocalClock = 0.0f;
        }
    }
    if ( !OverflowLoop )
    {
        // Clamp off end.
        if ( StartLocalClock > LocalDuration )
        {
            Assert ( EndLocalClock > LocalDuration );
            // Easy case - nothing happens.
            return;
        }
        else if ( EndLocalClock > LocalDuration )
        {
            EndLocalClock = LocalDuration;
        }
    }

    // So now clamping has been done, and we can ignore the over/underflow looping.
    // Any loops remaining are real loops and should be performed.
    // Also, start should always be less than end - any flips should be encoded in FlipResult.
    Assert ( StartLocalClock <= EndLocalClock );


    switch(State->AccumulationMode)
    {
        case ConstantExtractionAccumulation:
        {
            // All that CME cares about is actual seconds elapsed.
            // We still need to do all the above work to figure out where it gets clamped.
            float ActualSecondsElapsed = EndLocalClock - StartLocalClock;

            periodic_loop *Loop = GetTrackGroup(State->Binding)->PeriodicLoop;
            if(Loop)
            {
                real32 TheScale = Weight;
                if ( FlipResult )
                {
                    TheScale = -Weight;
                }

                triple Movement;
                ComputePeriodicLoopVector(*Loop, ActualSecondsElapsed, Movement);
                ScaleVectorAdd3(ResultTranslation, TheScale, Movement);

                triple Rotation;
                ComputePeriodicLoopLog(*Loop, ActualSecondsElapsed, Rotation);
                ScaleVectorAdd3(ResultRotation, TheScale, Rotation);
            }
            else
            {
                real32 tWeight = (Weight*ActualSecondsElapsed /
                                  GetControlLocalDuration(*Binding.Control));
                if ( FlipResult )
                {
                    tWeight = -tWeight;
                }

                ScaleVectorAdd3(ResultTranslation, tWeight,
                                GetTrackGroup(State->Binding)->LoopTranslation);
            }

            TotalWeight += Weight;
        } break;

        case VariableDeltaAccumulation:
        {
            // Now move both start and end so they lie in the interval [0,LocalDuration),
            // moving only in multiples of LocalDuration, and recording
            // how many multiples that was.
            int32x LoopCount = 0;
            int32x LocalLoopCount = FloorReal32ToInt32 ( StartLocalClock / LocalDuration );
            StartLocalClock -= LocalLoopCount * LocalDuration;
            LoopCount -= LocalLoopCount;
            Assert ( ( StartLocalClock >= 0.0f ) && ( StartLocalClock <= LocalDuration ) );

            LocalLoopCount = FloorReal32ToInt32 ( EndLocalClock / LocalDuration );
            EndLocalClock -= LocalLoopCount * LocalDuration;
            LoopCount += LocalLoopCount;
            Assert ( ( EndLocalClock >= 0.0f ) && ( EndLocalClock <= LocalDuration ) );


            // Now sample start and end times from the animation and find the delta between them.

            sample_context Context;
            // NOTE: VDA not supported on keyframed animations
            Context.FrameIndex = 0;
            Context.LocalClock = EndLocalClock;
            Context.LocalDuration = LocalDuration;
            Context.UnderflowLoop = false;
            Context.OverflowLoop = false;

            triple EndPosition;
            quad EndOrientation;
            SampleTrackPOLocal(Context, SourceTrack, BoundTrack,
                               EndPosition, EndOrientation);

            Context.LocalClock = StartLocalClock;
            triple StartPosition;
            quad StartOrientation;
            SampleTrackPOLocal(Context, SourceTrack, BoundTrack,
                               StartPosition, StartOrientation);

            triple ResidualPositionDelta;
            VectorSubtract3(ResidualPositionDelta, EndPosition, StartPosition);

            triple ResidualOrientationDelta;
            QuaternionDifferenceToAngularVelocity(ResidualOrientationDelta,
                                                  StartOrientation, EndOrientation);
            if ( LoopCount != 0 )
            {
                // TODO: Note that this is not guaranteed to be accurate
                // for rotational motions, because technically you would
                // have to account for the fact that the motion can loop
                // back on itself.  I could look into supporting this later,
                // although it is unlikely anyone will care because it
                // only occurs with severely large timesteps, which could
                // always be subdivided as a workaround.
                triple TotalPositionDelta;
                triple TotalOrientationDelta;
                ComputeTotalDeltasFromBinding(State->Binding,
                                              &TotalPositionDelta,
                                              &TotalOrientationDelta);

                ScaleVectorAdd3(ResidualOrientationDelta, (real32)LoopCount, TotalOrientationDelta);
                ScaleVectorAdd3(ResidualPositionDelta, (real32)LoopCount, TotalPositionDelta);
            }

            if ( FlipResult )
            {
                VectorNegate3 ( ResidualOrientationDelta );
                VectorNegate3 ( ResidualPositionDelta );
            }

            if ( !CurveIsIdentity ( SourceTrack->OrientationCurve ) )
            {
                quad BaseOrientation;
                CurveExtractKnotValue ( SourceTrack->OrientationCurve, 0, BaseOrientation, CurveIdentityOrientation );
                NormalQuaternionTransform3(ResidualPositionDelta, BaseOrientation);
                NormalQuaternionTransform3(ResidualOrientationDelta, BaseOrientation);
            }
            else
            {
                // Orientation curve is identity.
            }

            Conjugate4(StartOrientation);
            NormalQuaternionTransform3(ResidualPositionDelta, StartOrientation);

            ScaleVectorAdd3(ResultRotation, Weight, ResidualOrientationDelta);
            ScaleVectorAdd3(ResultTranslation, Weight, ResidualPositionDelta);

            TotalWeight += Weight;
        } break;

        case NoAccumulation:
        default:
        {
            InvalidCodePath("Unrecognized accumulation type");
        } break;
    }
}

static void
AnimationCleanupBindingState(model_control_binding &Binding)
{
    controlled_animation *State = GET_DERIVED(controlled_animation, Binding);

    ReleaseAnimationBinding(State->Binding);
}

controlled_animation *AnimationGetControlledAnimation ( model_control_binding &Binding )
{
    controlled_animation *State = GET_DERIVED(controlled_animation, Binding);
    return State;
}

controlled_pose *AnimationGetControlledPose ( model_control_binding &Binding )
{
    // It's not a pose!
    return NULL;
}



static model_control_callbacks ControlledAnimationCallbacks =
{
    AnimationGetControlledAnimation,
    AnimationGetControlledPose,
    AnimationInitializeBindingState,
    AnimationAccumulateBindingState,
    AnimationBuildDirect,
    AnimationAccumulateLoopTransform,
    AnimationCleanupBindingState,
};



control *GRANNY
PlayControlledAnimation(real32 StartTime,
                        animation const &Animation,
                        model_instance &Model)
{
    control *Result = 0;

    int32x TrackGroupIndex;
    if(FindTrackGroupForModel(Animation,
                              GetSourceModel(Model).Name,
                              TrackGroupIndex))
    {
        controlled_animation_builder *Builder =
            BeginControlledAnimation(StartTime, Animation);
        if(Builder)
        {
            SetTrackGroupTarget(*Builder, TrackGroupIndex, Model);
            SetTrackGroupLOD(*Builder, TrackGroupIndex, true, 1.0f);
            Result = EndControlledAnimation(Builder);
        }
    }

    return(Result);
}

control *GRANNY
PlayControlledAnimationBinding(real32 StartTime,
                               animation const &Animation,
                               animation_binding &Binding,
                               model_instance &Model)
{
    control *Result = 0;

    int32x TrackGroupIndex;
    if(FindTrackGroupForModel(Animation,
                              GetSourceModel(Model).Name,
                              TrackGroupIndex))
    {
        controlled_animation_builder *Builder =
            BeginControlledAnimation(StartTime, Animation);
        if(Builder)
        {
            SetTrackGroupBinding(*Builder, TrackGroupIndex, Binding);
            SetTrackGroupTarget(*Builder, TrackGroupIndex, Model);
            SetTrackGroupLOD(*Builder, TrackGroupIndex, true, 1.0f);
            Result = EndControlledAnimation(Builder);
        }
    }

    return(Result);
}

controlled_animation_builder *GRANNY
BeginControlledAnimation(real32 StartTime, animation const &Animation)
{
    COUNT_BLOCK("BeginControlledAnimation");

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    controlled_animation_builder *Builder;
    AggrAllocPtr(Allocator, Builder);
    AggrAllocOffsetArrayPtr(Allocator, Builder,
                            Animation.TrackGroupCount,
                            TrackCount, Tracks);
    if(EndAggrAlloc(Allocator))
    {
        Builder->Control = 0;
        Builder->CurrentClock = StartTime;
        Builder->Animation = &Animation;
        {for(int32x TrackIndex = 0;
             TrackIndex < Builder->TrackCount;
             ++TrackIndex)
        {
            track_target &Track = Builder->Tracks[TrackIndex];

            Track.Binding = 0;
            MakeDefaultAnimationBindingID(Track.BindingID,
                                          &Animation, TrackIndex);

            Track.OnInstance = 0;
            Track.TrackMask = 0;
            Track.ModelMask = 0;
            // Means "no LOD please"
            Track.LODCopyScaler = -1.0f;

            uint32 Flags, VDA;
            GetTrackGroupFlags(*GetTrackGroup(Track.BindingID), Flags, VDA);

            if(Flags & AccumulationExtracted)
            {
                Track.AccumulationMode = ConstantExtractionAccumulation;
            }
            else if(Flags & AccumulationIsVDA)
            {
                Track.AccumulationMode = VariableDeltaAccumulation;
            }
            else
            {
                Track.AccumulationMode = NoAccumulation;
            }
        }}
    }

    return(Builder);
}

control *GRANNY
EndControlledAnimation(controlled_animation_builder *Builder)
{
    COUNT_BLOCK("EndControlledAnimation");

    control *Control = Builder->Control;

    if(Builder)
    {
        if(Control)
        {
            CheckBoundedFloat32(0,
                                Control->LocalDuration,
                                Builder->Animation->Duration,
                                return(0));
        }
        else
        {
            // Create the control
            Control = CreateControl(Builder->CurrentClock,
                                    Builder->Animation->Duration);
        }

        if(Control)
        {
            // Create the bindings
            {for(int32x TrackIndex = 0;
                 TrackIndex < Builder->TrackCount;
                 ++TrackIndex)
            {
                track_target &Track = Builder->Tracks[TrackIndex];
                if(Track.OnInstance)
                {
                    model_control_binding *Binding =
                        CreateModelControlBinding(
                            ControlledAnimationCallbacks, *Control,
                            *Track.OnInstance,
                            ControlIsActive(*Control), &Track);
                    if(Binding)
                    {
                        // All is good
                        if ( Track.LODCopyScaler >= 0.0f )
                        {
                            // Copy over the LOD values.
                            animation_binding *AnimationBinding = GetAnimationBindingFromControlBinding ( *Binding );
                            if ( AnimationBinding != NULL )
                            {
                                CopyLODErrorValuesFromAnimation ( *AnimationBinding, Track.LODCopyScaler );
                            }
                            else
                            {
                                // Something disastrous happened.
                                Assert ( false );
                            }
                        }
                    }
                    else
                    {
                        Log0(ErrorLogMessage, ControlLogMessage,
                             "Unable to bind track group");
                    }
                }
            }}
        }
        else
        {
            Log0(ErrorLogMessage, ControlLogMessage,
                 "Unable to create control");
        }

        Deallocate(Builder);
    }

    return(Control);
}

void GRANNY
UseExistingControlForAnimation(controlled_animation_builder *Builder,
                               control *Control)
{
    Builder->Control = Control;
}

void GRANNY
SetTrackMatchRule(controlled_animation_builder &Builder,
                  int32x TrackGroupIndex,
                  char const *TrackPattern,
                  char const *BonePattern)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.BindingID.TrackPattern = TrackPattern;
    Track.BindingID.BonePattern = BonePattern;
}

void GRANNY
SetTrackGroupTarget(controlled_animation_builder &Builder, int32x TrackGroupIndex,
                    model_instance &Model)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.OnInstance = &Model;
    Track.BindingID.OnModel = &GetSourceModel(Model);
}

void GRANNY
SetTrackGroupBinding(controlled_animation_builder &Builder,
                     int32x TrackGroupIndex,
                     animation_binding &Binding)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.Binding = &Binding;
}

void GRANNY
SetTrackGroupBasisTransform(controlled_animation_builder &Builder, int32x TrackGroupIndex,
                            model &FromModel, model &ToModel)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.BindingID.FromBasis = &FromModel;
    Track.BindingID.ToBasis = &ToModel;
}

void GRANNY
SetTrackGroupTrackMask(controlled_animation_builder &Builder,
                       int32x TrackGroupIndex, track_mask &TrackMask)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.TrackMask = &TrackMask;
}

void GRANNY
SetTrackGroupModelMask(controlled_animation_builder &Builder,
                       int32x TrackGroupIndex, track_mask &ModelMask)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.ModelMask = &ModelMask;
}

void GRANNY
SetTrackGroupAccumulation(controlled_animation_builder &Builder,
                          int32x TrackGroupIndex,
                          accumulation_mode Mode)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.AccumulationMode = Mode;
}
void GRANNY
SetTrackGroupLOD(controlled_animation_builder &Builder,
                 int32x TrackGroupIndex,
                 bool CopyValuesFromAnimation,
                 real32 ManualScaler)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    track_target &Track = Builder.Tracks[TrackGroupIndex];

    if ( CopyValuesFromAnimation )
    {
        Track.LODCopyScaler = ManualScaler;
    }
    else
    {
        Track.LODCopyScaler = -1.0f;
    }
}



animation_binding *GRANNY
GetControlledAnimationBinding ( controlled_animation &ControlledAnimation )
{
    return ControlledAnimation.Binding;
}

animation_binding *GRANNY
GetAnimationBindingFromControlBinding(model_control_binding &Binding)
{
    controlled_animation *Anim = Binding.Callbacks->GetControlledAnimation ( Binding );
    if ( Anim != NULL )
    {
        return GetControlledAnimationBinding ( *Anim );
    }
    else
    {
        return NULL;
    }
}

