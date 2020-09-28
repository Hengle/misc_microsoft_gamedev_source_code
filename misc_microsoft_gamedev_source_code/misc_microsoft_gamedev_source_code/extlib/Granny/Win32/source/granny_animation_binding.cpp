// ========================================================================
// $File: //jeffr/granny/rt/granny_animation_binding.cpp $
// $DateTime: 2007/11/09 12:58:26 $
// $Change: 16497 $
// $Revision: #54 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_ANIMATION_BINDING_H)
#include "granny_animation_binding.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_FLOATS_H)
#include "granny_floats.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_TRACK_SAMPLER_H)
#include "granny_track_sampler.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "granny_controlled_animation.h"
#endif

#if !defined(GRANNY_CONTROL_H)
#include "granny_control.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_BUILDER_H)
#include "granny_track_group_builder.h"
#endif

#if !defined(GRANNY_STAT_HUD_H)
#include "granny_stat_hud.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode AnimationLogMessage

USING_GRANNY_NAMESPACE;

inline intaddrx
IDDifference(animation_binding_identifier &A,
             animation_binding_identifier &B)
{
    intaddrx Diff;

    Diff = PtrDiffSignOnly(A.Animation, B.Animation);
    if(Diff) return Diff;

    Diff = A.SourceTrackGroupIndex - B.SourceTrackGroupIndex;
    if(Diff) return Diff;

    Diff = PtrDiffSignOnly(A.OnModel, B.OnModel);
    if(Diff) return Diff;

    Diff = PtrDiffSignOnly(A.FromBasis, B.FromBasis);
    if(Diff) return Diff;

    Diff = PtrDiffSignOnly(A.ToBasis, B.ToBasis);
    if(Diff) return Diff;

    Diff = StringDifference(A.TrackPattern, B.TrackPattern);
    if(Diff) return Diff;

    Diff = StringDifference(A.BonePattern, B.BonePattern);
    return Diff;
}

#define CONTAINER_NAME binding_cache
#define CONTAINER_ITEM_TYPE animation_binding
#define CONTAINER_COMPARE_RESULT_TYPE intaddrx
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) IDDifference((Item1)->ID, (Item2)->ID)
#define CONTAINER_FIND_FIELDS animation_binding_identifier ID
#define CONTAINER_COMPARE_FIND_FIELDS(Item) IDDifference(ID, (Item)->ID)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 0
#define CONTAINER_DO_ALLOCATION 0
#include "granny_contain.inl"

#define CONTAINER_NAME binding_cache_free_list
#define CONTAINER_ITEM_TYPE animation_binding
#define CONTAINER_SORTED 0
#define CONTAINER_KEEP_LINKED_LIST 1
#define CONTAINER_SUPPORT_DUPES 0
#define CONTAINER_PREV_NAME PreviousUnused
#define CONTAINER_NEXT_NAME NextUnused
#define CONTAINER_DO_ALLOCATION 0
#define CONTAINER_NEED_FIND 0
#define CONTAINER_NEED_FINDFIRSTLT 0
#define CONTAINER_NEED_FINDFIRSTGT 0
#include "granny_contain.inl"


static int32x BindingCacheCountMax = 0;
static binding_cache BindingCache;
static binding_cache_free_list BindingCacheFreeList;
static animation_binding_cache_status CacheStatus = { 0 };




int32x GRANNY
GetMaximumAnimationBindingCount(void)
{
    return(BindingCacheCountMax);
}

void GRANNY
SetMaximumAnimationBindingCount(int32x BindingCountMax)
{
    BindingCacheCountMax = BindingCountMax;
}


static void
Rebase(animation_binding &Binding,
       transform_track const &SourceTrack,
       curve2 const &SourceCurve,
       bound_transform_track &Track,
       transform const &PreTransform,
       transform const &PostTransform,
       int32x Dimension,
       curve2 &DestCurve,
       real32 const *IdentityVector)
{
    sample_context Context;
    // NOTE: Rebasing not supported on keyframe animations
    Context.FrameIndex = 0;
    Context.LocalDuration = Binding.ID.Animation->Duration;
    Context.UnderflowLoop = Context.OverflowLoop = false;

    Assert ( !CurveIsKeyframed ( SourceCurve ) );
    Assert ( !CurveIsKeyframed ( DestCurve ) );
    Assert ( Dimension == CurveGetDimensionUnchecked ( DestCurve ) );
    Assert ( CurveIsIdentity( SourceCurve ) || Dimension == CurveGetDimensionUnchecked ( SourceCurve ) );
    // And the dest curve must be of type DaK32fC32f, because I don't want to re-quantise
    // and waste both time and precision.
    Assert ( CurveIsTypeDaK32fC32f ( DestCurve ) );
    curve_data_da_k32f_c32f *CurveDataDest = CurveGetContentsOfDaK32fC32f ( DestCurve );
    Assert ( CurveDataDest != NULL );

    int32x KnotCount = CurveGetKnotCount ( SourceCurve );
    if ( CurveIsIdentity ( SourceCurve ) )
    {
        KnotCount = 1;
        Assert ( CurveGetKnotCount ( DestCurve ) == 1 );
        Assert ( CurveDataDest->KnotCount == 1 );
        CurveDataDest->Knots[0] = 0.0f;
        Copy32 ( Dimension, IdentityVector, CurveDataDest->Controls );
    }
    else
    {
        Assert ( CurveGetKnotCount ( DestCurve ) == KnotCount );
        // Extract the source curve into the dest curve.
        // The knot values won't be changed, but we will be changing
        // the control points later. But by reading them from the dest, we won't have to decompress them again.
        CurveExtractKnotValues ( SourceCurve, 0, KnotCount, CurveDataDest->Knots, CurveDataDest->Controls, NULL );
    }

    transform Result;
    real32 *Write = CurveDataDest->Controls;
    {for(int32x KnotIndex = 0;
         KnotIndex < KnotCount;
         ++KnotIndex)
    {
        Context.LocalClock = CurveDataDest->Knots[KnotIndex];
        SampleTrackUUULocal(Context, &SourceTrack, &Track, Result);

        // And of course that just sampled the _value_ at that point,
        // whereas what we want to mangle is the _control_ value of the curve.
        // So we still want the other parts of the transform (Pos/Orn/Scale/Shear) _sampled_,
        // but we do want the actual control value for the item we're interested in.
        // Fortunately, that has already been extracted into the dest curve by the code above.
        if ( CurveIsIdentity ( SourceCurve ) )
        {
            // Well that was the identity that was returned. Which is fine.
        }
        else
        {
            switch(Dimension)
            {
                case 3:
                {
                    VectorEquals3(Result.Position, Write);
                } break;

                case 4:
                {
                    VectorEquals4(Result.Orientation, Write);
                } break;

                case 9:
                {
                    MatrixEquals3x3((real32 *)Result.ScaleShear, Write);
                } break;

                default:
                {
                    Assert("Hey this is not a supported curve dimension.");
                } break;
            }
        }

        // Now mangle.
        PreMultiplyBy(Result, PreTransform);
        PostMultiplyBy(Result, PostTransform);

        switch(Dimension)
        {
            case 3:
            {
                VectorEquals3(Write, Result.Position);
                Write += 3;
            } break;

            case 4:
            {
                VectorEquals4(Write, Result.Orientation);
                Write += 4;
            } break;

            case 9:
            {
                MatrixEquals3x3(Write, (real32 *)Result.ScaleShear);
                Write += 9;
            } break;

            default:
            {
                Assert("Hey this is not a supported curve dimension.");
            } break;
        }
    }}

    if(CurveDataDest->KnotCount == 1)
    {
        switch(Dimension)
        {
            case 3:
            {
                if(AreEqual(3, CurveDataDest->Controls, GlobalZeroVector, 0.001f))
                {
                    CurveDataDest->KnotCount = 0;
                    CurveDataDest->ControlCount = 0;
                }
            } break;

            case 4:
            {
                if(AreEqual(4, CurveDataDest->Controls, GlobalWAxis, 0.0001f))
                {
                    CurveDataDest->KnotCount = 0;
                    CurveDataDest->ControlCount = 0;
                }
            } break;

            case 9:
            {
                if(AreEqual(9, CurveDataDest->Controls, (real32 const *)GlobalIdentity3x3, 0.001f))
                {
                    CurveDataDest->KnotCount = 0;
                    CurveDataDest->ControlCount = 0;
                }
            } break;
        }
    }
}


static int32x
RebaseMemorySizeForCurve(curve2 const &Curve, int32x Dimension)
{
    int32x KnotCount = 0;
    if ( CurveIsIdentity ( Curve ) )
    {
        // It's going to be transformed, so probably won't be identity any more.
        KnotCount = 1;
    }
    else
    {
        Assert ( Dimension == CurveGetDimensionUnchecked ( Curve ) );
        KnotCount = CurveGetKnotCount ( Curve );
        Assert ( KnotCount > 0 );
    }

    return GetResultingDaK32fC32fCurveDataSize(KnotCount, Dimension);
}



// Creates a DaK32fC32k curve that can be used to hold a copy (transformed or otherwise) of SourceCurve.
// Does not (despite the name) actually copy the contents of the curve over!
static void
CopyCurveRebase(curve2 *DestCurve, curve2 const &SourceCurve,
                int32x Dimension, uint8 **RebasePtr)
{
    int32x KnotCount = CurveGetKnotCount ( SourceCurve );
    int32x Degree = CurveGetDegreeInline ( SourceCurve );
    if ( KnotCount <= 0 )
    {
        KnotCount = 1;
    }
    else
    {
        Assert ( CurveGetDimensionUnchecked ( SourceCurve ) == Dimension );
    }
    CurveCreateDaK32fC32fInPlace ( KnotCount, Degree, Dimension, DestCurve, *RebasePtr );
    int32x BytesUsed = GetResultingDaK32fC32fCurveDataSize ( KnotCount, Dimension );
    Assert ( RebaseMemorySizeForCurve ( SourceCurve, Dimension ) == BytesUsed );
    *RebasePtr += BytesUsed;
}

void GRANNY
ComputeTotalDeltasFromBinding(animation_binding* Binding,
                              triple*            TotalPositionDelta,
                              triple*            TotalOrientationDelta)
{
    CheckPointerNotNull(Binding, return);
    CheckPointerNotNull(TotalPositionDelta, return);
    CheckPointerNotNull(TotalOrientationDelta, return);

    triple& PositionDelta    = *TotalPositionDelta;
    triple& OrientationDelta = *TotalOrientationDelta;

    bound_transform_track *Track = Binding->TrackBindings;
    if(Binding->TrackBindingCount && Track->SourceTrack)
    {
        {
            curve2 const &Curve = Track->SourceTrack->PositionCurve;
            if(CurveIsConstantOrIdentity(Curve))
            {
                VectorZero3 ( PositionDelta );
            }
            else
            {
                triple FirstPosition;
                triple LastPosition;
                // Question - is this correct? These are the control points,
                // but that's not 100% guaranteed to be where the _curve_ goes through!
                int32x KnotCount = CurveGetKnotCount ( Curve );
                CurveExtractKnotValue ( Curve, 0,           FirstPosition, NULL );
                CurveExtractKnotValue ( Curve, KnotCount-1, LastPosition, NULL );
                VectorSubtract3(PositionDelta, LastPosition, FirstPosition);
            }
        }

        {
            curve2 const &Curve = Track->SourceTrack->OrientationCurve;
            if(CurveIsConstantOrIdentity(Curve))
            {
                VectorZero3 ( OrientationDelta );
            }
            else
            {
                quad FirstQuaternion;
                quad LastQuaternion;
                // Question - is this correct? These are the control points,
                // but that's not 100% guaranteed to be where the _curve_ goes through!
                int32x KnotCount = CurveGetKnotCount ( Curve );
                CurveExtractKnotValue ( Curve, 0,           FirstQuaternion, NULL );
                CurveExtractKnotValue ( Curve, KnotCount-1, LastQuaternion, NULL );
                QuaternionDifferenceToAngularVelocity(OrientationDelta,
                                                      FirstQuaternion, LastQuaternion);
            }
        }
    }
    else
    {
        VectorZero3(PositionDelta);
        VectorZero3(OrientationDelta);
    }
}

bool GRANNY
AreTrackFlagsConsistent(bound_transform_track const &BoundTrack)
{
    // If there's no source track, the flags are considered consistent
    if (BoundTrack.SourceTrack == 0)
        return true;

    bool Valid = true;

    if (BOUND_CURVE_CHECK(BoundTrack, Position, IsIdentity))
        Valid = Valid && CurveIsIdentity(BoundTrack.SourceTrack->PositionCurve);
    else if (BOUND_CURVE_CHECK(BoundTrack, Position, IsConstant))
        Valid = Valid && CurveIsConstantNotIdentity(BoundTrack.SourceTrack->PositionCurve);
    else if (BOUND_CURVE_CHECK(BoundTrack, Position, IsKeyframed))
        Valid = Valid && CurveIsKeyframed(BoundTrack.SourceTrack->PositionCurve);
    else
    {
        Assert(BOUND_CURVE_CHECK(BoundTrack, Position, IsGeneral));
        Valid = Valid && !(CurveIsConstantOrIdentity(BoundTrack.SourceTrack->PositionCurve) ||
                           CurveIsKeyframed(BoundTrack.SourceTrack->PositionCurve));
    }

    if (BOUND_CURVE_CHECK(BoundTrack, Orientation, IsIdentity))
        Valid = Valid && CurveIsIdentity(BoundTrack.SourceTrack->OrientationCurve);
    else if (BOUND_CURVE_CHECK(BoundTrack, Orientation, IsConstant))
        Valid = Valid && CurveIsConstantNotIdentity(BoundTrack.SourceTrack->OrientationCurve);
    else if (BOUND_CURVE_CHECK(BoundTrack, Orientation, IsKeyframed))
        Valid = Valid && CurveIsKeyframed(BoundTrack.SourceTrack->OrientationCurve);
    else
    {
        Assert(BOUND_CURVE_CHECK(BoundTrack, Orientation, IsGeneral));
        Valid = Valid && !(CurveIsConstantOrIdentity(BoundTrack.SourceTrack->OrientationCurve) ||
                           CurveIsKeyframed(BoundTrack.SourceTrack->OrientationCurve));
    }

    if (BOUND_CURVE_CHECK(BoundTrack, ScaleShear, IsIdentity))
        Valid = Valid && CurveIsIdentity(BoundTrack.SourceTrack->ScaleShearCurve);
    else if (BOUND_CURVE_CHECK(BoundTrack, ScaleShear, IsConstant))
        Valid = Valid && CurveIsConstantNotIdentity(BoundTrack.SourceTrack->ScaleShearCurve);
    else if (BOUND_CURVE_CHECK(BoundTrack, ScaleShear, IsKeyframed))
        Valid = Valid && CurveIsKeyframed(BoundTrack.SourceTrack->ScaleShearCurve);
    else
    {
        Assert(BOUND_CURVE_CHECK(BoundTrack, ScaleShear, IsGeneral));
        Valid = Valid && !(CurveIsConstantOrIdentity(BoundTrack.SourceTrack->ScaleShearCurve) ||
                           CurveIsKeyframed(BoundTrack.SourceTrack->ScaleShearCurve));
    }

    return Valid;
}

static void
ComputeTrackFlags(bound_transform_track& Track)
{
    if (Track.SourceTrack == 0)
    {
        Track.Flags = 0;

#if BOUND_TRANSFORM_TRACK_HAS_LOD
        // We'll need to do this after any rebasing as well.
        MakeIdentity(Track.LODTransform);
#endif
    }
    else
    {
        Track.Flags = 0;

#define SetBoundTrackFlag(Var, WhichCurve, Flag)                                                    \
        do { (Var) = int8(((Var) & ~Bound ## WhichCurve ## CurveFlagMask) | Bound ## WhichCurve ## CurveIs ## Flag); } while (false)

        //Track.Flags = (Track.Flags & ~BoundPositionCurveFlagMask) | BoundPositionCurveIsIdentity;
        if (CurveIsIdentity(Track.SourceTrack->PositionCurve))
            SetBoundTrackFlag(Track.Flags, Position, Identity);
        else if (CurveIsConstantNotIdentity(Track.SourceTrack->PositionCurve))
            SetBoundTrackFlag(Track.Flags, Position, Constant);
        else if (CurveIsKeyframed(Track.SourceTrack->PositionCurve))
            SetBoundTrackFlag(Track.Flags, Position, Keyframed);
        else
            SetBoundTrackFlag(Track.Flags, Position, General);


        if (CurveIsIdentity(Track.SourceTrack->OrientationCurve))
            SetBoundTrackFlag(Track.Flags, Orientation, Identity);
        else if (CurveIsConstantNotIdentity(Track.SourceTrack->OrientationCurve))
            SetBoundTrackFlag(Track.Flags, Orientation, Constant);
        else if (CurveIsKeyframed(Track.SourceTrack->OrientationCurve))
            SetBoundTrackFlag(Track.Flags, Orientation, Keyframed);
        else
            SetBoundTrackFlag(Track.Flags, Orientation, General);

        if (CurveIsIdentity(Track.SourceTrack->ScaleShearCurve))
            SetBoundTrackFlag(Track.Flags, ScaleShear, Identity);
        else if (CurveIsConstantNotIdentity(Track.SourceTrack->ScaleShearCurve))
            SetBoundTrackFlag(Track.Flags, ScaleShear, Constant);
        else if (CurveIsKeyframed(Track.SourceTrack->ScaleShearCurve))
            SetBoundTrackFlag(Track.Flags, ScaleShear, Keyframed);
        else
            SetBoundTrackFlag(Track.Flags, ScaleShear, General);

#undef SetBoundTrackFlag


#if BOUND_TRANSFORM_TRACK_HAS_LOD
        // We'll need to do this after any rebasing as well.
        SampleTrackUUULocalAtTime0 ( Track.SourceTrack, Track.LODTransform );
#endif
    }
}

static animation_binding *
NewAnimationBinding(animation_binding_identifier &ID)
{
    Assert(ID.OnModel);
    skeleton *OnSkeleton = ID.OnModel->Skeleton;

    Assert(OnSkeleton);
    int32x BoneCount = OnSkeleton->BoneCount;

    bool Rebasing = (ID.FromBasis != ID.ToBasis);

    int32x RebasingMemorySize = 0;
    animation_binding *Binding;

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);
    AggrAllocPtr(Allocator, Binding);
    AggrAllocOffsetArrayPtr(Allocator, Binding, BoneCount,
                            TrackBindingCount, TrackBindings);
    if(EndAggrAlloc(Allocator))
    {
        Binding->ID = ID;
        Binding->UsedBy = 0;

        bool KeyframedTracks = false;
        {for(int32x BoneIndex = 0;
             BoneIndex < Binding->TrackBindingCount;
             ++BoneIndex)
        {
            bound_transform_track &Track = Binding->TrackBindings[BoneIndex];

            bone const &Bone = OnSkeleton->Bones[BoneIndex];
            char const *BoneName = Bone.Name;
            bool Found = false;

            int32x SourceTrackIndex;
            if(StringComparisonCallback ||
               (IsPlainWildcard(Binding->ID.BonePattern) &&
                IsPlainWildcard(Binding->ID.TrackPattern)))
            {
                Found = FindTrackByName(GetTrackGroup(Binding),
                                        BoneName,
                                        SourceTrackIndex);
            }
            else
            {
                char BoneNameBuffer[MaximumBoneNameLength + 1];
                WildCardMatch(BoneName, Binding->ID.BonePattern,
                              BoneNameBuffer);
                Found = FindTrackByRule(GetTrackGroup(Binding),
                                        BoneNameBuffer,
                                        Binding->ID.TrackPattern,
                                        SourceTrackIndex);
            }

            if(Found)
            {
                transform_track const &SourceTrack =
                    GetTrackGroup(Binding)->
                    TransformTracks[SourceTrackIndex];

                CheckConvertToInt16(Track.SourceTrackIndex,
                                    SourceTrackIndex,
                                    Track.SourceTrackIndex = int16(-1));
                Track.SourceTrack = &SourceTrack;
                Track.LODError = GetReal32AlmostInfinity();
                ComputeTrackFlags(Track);

                if ( CurveIsConstantOrIdentity ( SourceTrack.PositionCurve ) &&
                     CurveIsConstantOrIdentity ( SourceTrack.OrientationCurve ) &&
                     CurveIsConstantOrIdentity ( SourceTrack.ScaleShearCurve ) )
                {
                    // This curve is constant, so we can always use the fast LOD path.
                    Track.LODError = 0.0f;
                }

                if ( Rebasing )
                {
                    RebasingMemorySize += SizeOf(transform_track);
                    RebasingMemorySize += RebaseMemorySizeForCurve(SourceTrack.PositionCurve, 3);
                    RebasingMemorySize += RebaseMemorySizeForCurve(SourceTrack.OrientationCurve, 4);
                    RebasingMemorySize += RebaseMemorySizeForCurve(SourceTrack.ScaleShearCurve, 9);
                }

                Track.Sampler = GetTrackSamplerFor(SourceTrack);

                if(TransformTrackHasKeyframedCurves(SourceTrack))
                {
                    KeyframedTracks = true;
                }
                else
                {
                    quad FirstQuaternion;
                    quad LastQuaternion;
                    curve2 const &Curve = SourceTrack.OrientationCurve;
                    if ( !CurveIsIdentity ( Curve ) )
                    {
                        int32x KnotCount = CurveGetKnotCount ( Curve );
                        CurveExtractKnotValue ( Curve, 0, FirstQuaternion, CurveIdentityOrientation );
                        CurveExtractKnotValue ( Curve, KnotCount-1, LastQuaternion, CurveIdentityOrientation );
                    }
                    else
                    {
                        VectorEquals4 ( FirstQuaternion, GlobalWAxis );
                        VectorEquals4 ( LastQuaternion, GlobalWAxis );
                    }

                    transform const &Rest = Bone.LocalTransform;

                    bool InvertedFromRest =
                        (InnerProduct4(Rest.Orientation,
                                       FirstQuaternion) < 0.0f);
                    bool InversionDuring =
                        (InnerProduct4(FirstQuaternion,
                                       LastQuaternion) < 0.0f);

                    if ((SourceTrack.Flags & UseAccumulatorNeighborhood) != 0)
                    {
                        Track.QuaternionMode = BlendQuaternionAccumNeighborhooded;
                    }
                    else
                    {
                        if(InversionDuring)
                        {
                            Track.QuaternionMode = BlendQuaternionNeighborhooded;
                        }
                        else if(InvertedFromRest)
                        {
                            Track.QuaternionMode = BlendQuaternionInverted;
                        }
                        else
                        {
                            Track.QuaternionMode = BlendQuaternionDirectly;
                        }
                    }
                }
            }
            else
            {
                // Not found.
                Track.Sampler = GetTrackSamplerSSS();
                Track.SourceTrackIndex = -1;
                Track.SourceTrack = 0;
                Track.QuaternionMode = BlendQuaternionDirectly;
                Track.LODError = GetReal32AlmostInfinity();
                ComputeTrackFlags(Track);
            }
        }}

        if(KeyframedTracks)
        {
            Log1(WarningLogMessage, AnimationLogMessage,
                 "The track group \"%s\" has keyframed animation tracks, "
                 "which are meant for preprocessing only.  They will be "
                 "ignored during playback.", GetTrackGroup(Binding)->Name);
        }

        if(Rebasing)
        {
            uint8 *StartOfRebasingMemory = (uint8 *)AllocateSize(RebasingMemorySize);
            Binding->RebasingMemory = StartOfRebasingMemory;
            if(Binding->RebasingMemory)
            {
                uint8 *RebasePtr = (uint8 *)Binding->RebasingMemory;
                {for(int32x BoneIndex = 0;
                     BoneIndex < Binding->TrackBindingCount;
                     ++BoneIndex)
                {
                    bound_transform_track &Track = Binding->TrackBindings[BoneIndex];
                    bone const &Bone = OnSkeleton->Bones[BoneIndex];
                    char const *BoneName = Bone.Name;

                    if(Track.SourceTrack &&
                       !TransformTrackHasKeyframedCurves(*Track.SourceTrack))
                    {
                        transform_track const &SourceTrack = *Track.SourceTrack;
                        model *FromBasis = Binding->ID.FromBasis;
                        model *ToBasis = Binding->ID.ToBasis;

                        int32x FromIndex, ToIndex;
                        if(FindBoneByName(FromBasis->Skeleton, BoneName, FromIndex) &&
                           FindBoneByName(ToBasis->Skeleton, BoneName, ToIndex))
                        {
                            transform_track *RebasedTrack =
                                (transform_track *)RebasePtr;
                            RebasePtr += SizeOf(*RebasedTrack);

                            CopyCurveRebase(&(RebasedTrack->PositionCurve),
                                            SourceTrack.PositionCurve,
                                            3, &RebasePtr);
                            CopyCurveRebase(&(RebasedTrack->OrientationCurve),
                                            SourceTrack.OrientationCurve,
                                            4, &RebasePtr);
                            CopyCurveRebase(&(RebasedTrack->ScaleShearCurve),
                                            SourceTrack.ScaleShearCurve,
                                            9, &RebasePtr);

                            transform RebasingTransform;
                            BuildInverse(
                                RebasingTransform,
                                FromBasis->Skeleton->Bones[FromIndex].LocalTransform);
                            PostMultiplyBy(
                                RebasingTransform,
                                ToBasis->Skeleton->Bones[ToIndex].LocalTransform);

                            transform RotatingRebaseTransform;
                            MakeIdentity(RotatingRebaseTransform);
                            Copy32(4, RebasingTransform.Orientation,
                                   RotatingRebaseTransform.Orientation);
                            RotatingRebaseTransform.Flags = HasOrientation;

                            RebasedTrack->Name = SourceTrack.Name;
                            Assert(CurveGetDegree(RebasedTrack->PositionCurve) ==
                                   CurveGetDegree(SourceTrack.PositionCurve));
                            Assert(CurveGetDegree(RebasedTrack->OrientationCurve) ==
                                   CurveGetDegree(SourceTrack.OrientationCurve));
                            Assert(CurveGetDegree(RebasedTrack->ScaleShearCurve) ==
                                   CurveGetDegree(SourceTrack.ScaleShearCurve));

                            // NewPos = Anim * Rebase
                            {
                                Rebase(*Binding, SourceTrack, SourceTrack.PositionCurve,
                                       Track, GlobalIdentityTransform, RebasingTransform,
                                       3, RebasedTrack->PositionCurve, CurveIdentityPosition);
                            }

                            // NewRot = Anim * RotateOfRebase
                            {
                                Rebase(*Binding, SourceTrack, SourceTrack.OrientationCurve,
                                       Track, GlobalIdentityTransform, RotatingRebaseTransform,
                                       4, RebasedTrack->OrientationCurve, CurveIdentityOrientation);
                            }

                            // NewScale = InvRotateOfRebase * Anim * Rebase
                            {
                                transform InvRotatingRebaseTransform;
                                BuildInverse(InvRotatingRebaseTransform,
                                             RotatingRebaseTransform);
                                Rebase(*Binding, SourceTrack, SourceTrack.ScaleShearCurve,
                                       Track, InvRotatingRebaseTransform, RebasingTransform,
                                       9, RebasedTrack->ScaleShearCurve, CurveIdentityScaleShear);
                            }

                            Track.SourceTrack = RebasedTrack;
                            ComputeTrackFlags(Track);
                            Track.Sampler = GetTrackSamplerFor(*RebasedTrack);

                            if ( CurveIsConstantOrIdentity ( SourceTrack.PositionCurve ) &&
                                 CurveIsConstantOrIdentity ( SourceTrack.OrientationCurve ) &&
                                 CurveIsConstantOrIdentity ( SourceTrack.ScaleShearCurve ) )
                            {
                                // This curve is constant, so we can always use the fast LOD path.
                                Track.LODError = 0.0f;
                            }
                        }
                    }
                }}

                Assert ( ( StartOfRebasingMemory + RebasingMemorySize ) == RebasePtr );
            }
            else
            {
                Log1(WarningLogMessage, AnimationLogMessage,
                     "Out of memory for rebasing track group \"%s\".",
                     GetTrackGroup(Binding)->Name);
            }
        }
        else
        {
            Binding->RebasingMemory = 0;
        }

        ++CacheStatus.TotalBindingsCreated;
        ++CacheStatus.CurrentTotalBindingCount;
    }

    return(Binding);
}






#if DEBUG_CONTAINER
#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

static file_writer *Writer = 0;
static void
Dump(char *Operation, animation_binding_identifier &ID)
{
    if(!Writer)
    {
        Writer = NewFileWriter("d:/treedebug.txt", true);
    }
    char Buffer[64];
    int32x Length = ConvertToStringVar(SizeOf(Buffer), Buffer,
                                       "%s %x %x\n", Operation, ID.SourceTrackGroupIndex, ID.OnModel);
    Write(*Writer, Length, Buffer);

    Assert(StringsAreEqual(ID.TrackPattern, "*"));
    Assert(StringsAreEqual(ID.BonePattern, "*"));
    Assert(!ID.FromBasis);
    Assert(!ID.ToBasis);
}

#define ValidateTree()
#else
#define Dump(a, b)
#define ValidateTree()
#endif

static void
FreeAnimationBinding(animation_binding *Binding)
{
    if(Binding)
    {
        Assert(Binding->UsedBy == 0);

        Dump("REMOVE", Binding->ID);

        Remove(&BindingCache, Binding);
        ValidateTree();

        Remove(&BindingCacheFreeList, Binding);
        ValidateTree();

        Deallocate(Binding->RebasingMemory);
        Deallocate(Binding);
        ValidateTree();

        --CacheStatus.CurrentTotalBindingCount;
        ++CacheStatus.TotalBindingsDestroyed;
    }
}

static void
FreeCacheOverflow(void)
{
    while(CacheStatus.CurrentTotalBindingCount > BindingCacheCountMax)
    {
        animation_binding *FreeBinding = Last(&BindingCacheFreeList);
        if(FreeBinding)
        {
            ++CacheStatus.OverflowFrees;
            FreeAnimationBinding(FreeBinding);
        }
        else
        {
            break;
        }
    }
}

static void
IncUsedBy(animation_binding *Binding)
{
    if(Binding)
    {
        if(Binding->UsedBy == 0)
        {
            ++CacheStatus.CurrentUsedBindingCount;
            Remove(&BindingCacheFreeList, Binding);
            ValidateTree();
        }

        ++Binding->UsedBy;
        Assert(Binding->UsedBy > 0);
    }
}

static void
DecUsedBy(animation_binding *Binding)
{
    if(Binding)
    {
        Assert(Binding->UsedBy > 0);
        --Binding->UsedBy;

        if(Binding->UsedBy == 0)
        {
            --CacheStatus.CurrentUsedBindingCount;
            Add(&BindingCacheFreeList, Binding);
            ValidateTree();
            FreeCacheOverflow();
        }
    }
}

void GRANNY
MakeDefaultAnimationBindingID(animation_binding_identifier &ID,
                              animation const *Animation,
                              int32x TrackGroupIndex)
{
    ID.Animation = Animation;
    ID.SourceTrackGroupIndex = TrackGroupIndex;
    ID.TrackPattern = "*";
    ID.BonePattern = "*";
    ID.OnModel = 0;
    ID.FromBasis = 0;
    ID.ToBasis = 0;
}

animation_binding *GRANNY
AcquireAnimationBindingFromID(animation_binding_identifier &ID)
{
    ++CacheStatus.IndirectAcquireCount;
    animation_binding *Binding = Find(&BindingCache, ID);

    if(Binding)
    {
        ++CacheStatus.CacheHits;
        IncUsedBy(Binding);
    }
    else
    {
        ++CacheStatus.CacheMisses;
        Binding = NewAnimationBinding(ID);
        if(Binding)
        {
            Dump("ADD", ID);
            Add(&BindingCache, Binding);
            ValidateTree();

            Add(&BindingCacheFreeList, Binding);
            ValidateTree();

            IncUsedBy(Binding);
            FreeCacheOverflow();
        }
    }

    return(Binding);
}

animation_binding *GRANNY
AcquireAnimationBinding(animation_binding *Binding)
{
    ++CacheStatus.DirectAcquireCount;
    ++CacheStatus.CacheHits;
    IncUsedBy(Binding);
    return(Binding);
}

void GRANNY
ReleaseAnimationBinding(animation_binding *Binding)
{
    ++CacheStatus.ReleaseCount;
    DecUsedBy(Binding);
}

void GRANNY
FlushAllUnusedAnimationBindings(void)
{
    ++CacheStatus.ExplicitFlushCount;

    {for(animation_binding *Binding = First(&BindingCacheFreeList);
         Binding;
         )
    {
        animation_binding *NextBinding =
            Next(&BindingCacheFreeList, Binding);
        if(!Binding->UsedBy)
        {
            ++CacheStatus.ExplicitFlushFrees;
            FreeAnimationBinding(Binding);
        }
        Binding = NextBinding;
    }}
}

void GRANNY
FlushAllBindingsForAnimation(animation const *Animation)
{
    animation_binding_identifier MinID = {Animation};

    while(1)
    {
        animation_binding *Binding = FindGT(&BindingCache, MinID);
        if(!Binding || (Binding->ID.Animation != Animation))
        {
            break;
        }

        if(!Binding->UsedBy)
        {
            FreeAnimationBinding(Binding);
        }
        else
        {
            Log1(ErrorLogMessage, AnimationLogMessage,
                 "FlushAllBindingsForAnimation() tried to flush an "
                 "animation still in use (%s)",
                 Animation->Name);
        }
    }
}

void GRANNY
FlushAnimationBinding(animation_binding *Binding)
{
    if(Binding)
    {
        if(!Binding->UsedBy)
        {
            FreeAnimationBinding(Binding);
        }
        else
        {
            Log1(ErrorLogMessage, AnimationLogMessage,
                 "FlushAnimationBinding() tried to flush an "
                 "animation still in use (%s)",
                 Binding->ID.Animation->Name);
        }
    }
}

animation_binding *GRANNY
GetFirstBindingForAnimation(animation const *Animation)
{
    animation_binding_identifier MinID = {Animation};
    animation_binding *Binding = FindGT(&BindingCache, MinID);
    if(Binding && (Binding->ID.Animation == Animation))
    {
        return(Binding);
    }
    else
    {
        return(0);
    }
}

animation_binding *GRANNY
GetNextBindingForAnimation(animation const *Animation, animation_binding *Binding)
{
    if(Binding)
    {
        Binding = Next(&BindingCache, Binding);
        if(Binding && (Binding->ID.Animation == Animation))
        {
            return(Binding);
        }
    }

    return(0);
}

animation_binding *GRANNY
GetFirstUnusedAnimationBinding(void)
{
    return(First(&BindingCacheFreeList));
}

animation_binding *GRANNY
GetNextUnusedAnimationBinding(animation_binding *Binding)
{
    if(Binding)
    {
        return(Next(&BindingCacheFreeList, Binding));
    }
    else
    {
        return(0);
    }
}

bool GRANNY
IsAnimationUsed(animation const *Animation)
{
    {for(animation_binding *Binding = GetFirstBindingForAnimation(Animation);
         Binding;
         Binding = GetNextBindingForAnimation(Animation, Binding))
    {
        if(Binding->UsedBy)
        {
            return(true);
        }
    }}

    return(false);
}

void GRANNY
RemapAnimationBindingPointers(animation_binding *Binding,
                              animation const *NewAnimationPointer)
{
    CheckPointerNotNull(NewAnimationPointer, return);

    Remove(&BindingCache, Binding);

    Binding->ID.Animation = NewAnimationPointer;
    track_group *TrackGroup =
        NewAnimationPointer->TrackGroups[Binding->ID.SourceTrackGroupIndex];

    {for(int32x TrackBindingIndex = 0;
         TrackBindingIndex < Binding->TrackBindingCount;
         ++TrackBindingIndex)
    {
        bound_transform_track *BoundTrack =
            &Binding->TrackBindings[TrackBindingIndex];

        if(BoundTrack->SourceTrackIndex == -1)
        {
            Assert(BoundTrack->SourceTrack == 0);
        }
        else
        {
            Assert(BoundTrack->SourceTrackIndex >= 0);
            Assert(BoundTrack->SourceTrackIndex < TrackGroup->TransformTrackCount);

            transform_track const *SourceTrack =
                &TrackGroup->TransformTracks[BoundTrack->SourceTrackIndex];
            BoundTrack->SourceTrack = SourceTrack;
            ComputeTrackFlags(*BoundTrack);
        }
    }}

    Add(&BindingCache, Binding);
}

void GRANNY
RemapAllAnimationBindingPointers(animation const *OldAnimationPointer,
                                 animation const *NewAnimationPointer)
{
    {for(animation_binding *Binding = GetFirstBindingForAnimation(OldAnimationPointer);
         Binding;
         )
    {
        animation_binding *Next =
            GetNextBindingForAnimation(OldAnimationPointer, Binding);
        RemapAnimationBindingPointers(Binding, NewAnimationPointer);
        Binding = Next;
    }}
}

animation_binding *GRANNY
GetFirstAnimationBinding(void)
{
    return(First(&BindingCache));
}

animation_binding *GRANNY
GetNextAnimationBinding(animation_binding *Binding)
{
    return(Next(&BindingCache, Binding));
}

void GRANNY
GetAnimationBindingCacheStatus(animation_binding_cache_status &Result)
{
    Result = CacheStatus;
}



void GRANNY
CalculateLODErrorValues ( animation_binding &AnimationBinding,
                          mesh_binding const &MeshBinding,
                          bool AnimationHasScales,
                          real32 ManualScaler )
{
    // For now, we do it the absolutely correct way. May be a bit slow though (not half!)

    // TODO:
    // Actually, the absolutely correct way would be to:
    // Calculate the errors of approximating each bone.
    // Find the bone that causes the least error.
    // Do that approximation.
    // Recalculate the errors for all the parent bones of that bone.
    // Repeat until all bones approximated.
    // But that's an nlogn algo, whereas at least this is n.
    // Although each error check is also proportional to n (have to check the children's bboxes too),
    // so actually it's nnlogn rather than nn. Hmmm... maybe there's not much difference.

    // TODO: Don't sample every frame, figure out some better way, e.g. sample at each knot?

    animation_binding_identifier &ID = AnimationBinding.ID;
    CheckPointerNotNull ( ID.OnModel, return );
    skeleton *OnSkeleton = ID.OnModel->Skeleton;
    CheckPointerNotNull ( OnSkeleton, return );
    int32x BoneCount = AnimationBinding.TrackBindingCount;
    CheckBoundedInt32 ( 1, BoneCount, OnSkeleton->BoneCount, return );
    CheckPointerNotNull ( AnimationBinding.TrackBindings, return );

    CheckPointersEqual (GetMeshBindingToSkeleton((mesh_binding&)MeshBinding), OnSkeleton, return );

    mesh const &Mesh = *GetMeshBindingSourceMesh ( MeshBinding );
    int32x const *ToBoneIndices = GetMeshBindingToBoneIndices ( MeshBinding );
    Assert ( GetMeshBindingBoneCount ( MeshBinding ) == Mesh.BoneBindingCount );

    local_pose* LocalPose[3];
    world_pose* WorldPose[3];
    {for ( int32x Pose = 0; Pose < 3; Pose ++ )
    {
        LocalPose[Pose] = NewLocalPose ( BoneCount );
        WorldPose[Pose] = NewWorldPose ( BoneCount );
    }}

    local_pose* LocalPose_Time0  = LocalPose[0];
    local_pose* LocalPose_Approx = LocalPose[1];
    local_pose* LocalPose_Real   = LocalPose[2];

    world_pose* WorldPose_Approx = WorldPose[1];
    world_pose* WorldPose_Real   = WorldPose[2];


    sample_context Context;
    Context.UnderflowLoop = false;
    Context.OverflowLoop = false;

    // Create the t = 0 pose, and max out the lod errors
    {for(int32x BoneIndex = 0;
         BoneIndex < BoneCount;
         ++BoneIndex)
    {
        bound_transform_track &TestedTrack = AnimationBinding.TrackBindings[BoneIndex];

        if ( TestedTrack.SourceTrack != NULL )
        {
#if BOUND_TRANSFORM_TRACK_HAS_LOD
            TestedTrack.LODError = 0.0f;
            SampleTrackUUULocalAtTime0(TestedTrack.SourceTrack, TestedTrack.LODTransform);
            Copy(sizeof(transform),
                 &TestedTrack.LODTransform,
                 GetLocalPoseTransform(*LocalPose_Time0, BoneIndex));
#else
            SampleTrackUUULocalAtTime0(TestedTrack.SourceTrack,
                                       *GetLocalPoseTransform(*LocalPose_Time0, BoneIndex));
#endif
        }
        else
        {
            TestedTrack.LODError = GetReal32AlmostInfinity();
        }
    }}

    int32x NumFrames = TruncateReal32ToInt32( ( ID.Animation->Duration / ID.Animation->TimeStep ) + 0.5f );
    {for ( int32x FrameNum = 0; FrameNum <= NumFrames; FrameNum++ )
    {
        real32 FrameTime = (real32)FrameNum * ID.Animation->TimeStep;

        Context.FrameIndex = 0;
        Context.LocalClock = FrameTime;
        Context.LocalDuration = ID.Animation->Duration;

        // Sample the animation at this time to create the canonical local pose
        {for(int32x BoneIndex = 0;
             BoneIndex < BoneCount;
             ++BoneIndex)
        {
            bound_transform_track &TestedTrack = AnimationBinding.TrackBindings[BoneIndex];
            transform* DestTransform = GetLocalPoseTransform(*LocalPose_Real, BoneIndex);
            bone& OnBone = OnSkeleton->Bones[BoneIndex];

            if ( TestedTrack.SourceTrack )
            {
                SampleTrackUUULocal(Context,
                                    TestedTrack.SourceTrack,
                                    &TestedTrack,
                                    *DestTransform);
                Copy(sizeof(transform),
                        GetLocalPoseTransform(*LocalPose_Real,   BoneIndex),
                        GetLocalPoseTransform(*LocalPose_Approx, BoneIndex));

            }
            else
            {
                Copy(sizeof(transform), &OnBone.LocalTransform, DestTransform);
            }
        }}

        {for(int32x TestedBoneIndex = 0;
             TestedBoneIndex < BoneCount;
             ++TestedBoneIndex)
        {
            bound_transform_track &TestedTrack = AnimationBinding.TrackBindings[TestedBoneIndex];

            if ( TestedTrack.SourceTrack != NULL )
            {
                // Create the approximate local pose for this bone test
                CopyLocalPose(*LocalPose_Real, *LocalPose_Approx);
                Copy(sizeof(transform),
                     GetLocalPoseTransform(*LocalPose_Time0,  TestedBoneIndex),
                     GetLocalPoseTransform(*LocalPose_Approx, TestedBoneIndex));

                // Ok.  At this point, LocalPose_Real represents the /actual/ state of the
                // model, and _Approx is the test state.  Create world poses for these.
                // There is quite a bit of math overlap here if that becomes critical...
                BuildWorldPose(*OnSkeleton,
                               0, BoneCount, *LocalPose_Real,
                               NULL, *WorldPose_Real);
                BuildWorldPose(*OnSkeleton,
                               0, BoneCount, *LocalPose_Approx,
                               NULL, *WorldPose_Approx);


                // We now have the two poses, one with the approximation, one without.
                // So transform each bone's bbox by the two and compare the differences.
                matrix_4x4 *WorldApprox = WorldPose_Approx->WorldTransformBuffer;
                matrix_4x4 *WorldReal   = WorldPose_Real->WorldTransformBuffer;

                // Remember - the mesh bone index is not the same as the skeleton bone index,
                // and they're not well-ordered.
                {for(int32x MeshBoneIndex = 0;
                     MeshBoneIndex < Mesh.BoneBindingCount;
                     ++MeshBoneIndex)
                {
                    int32x SkeletonBoneIndex = ToBoneIndices[MeshBoneIndex];
                    if ( SkeletonBoneIndex < TestedBoneIndex )
                    {
                        // This is definitely not a child of the tested bone,
                        // so no need to check it.
                        continue;
                    }

                    triple &OBBMin = Mesh.BoneBindings[MeshBoneIndex].OBBMin;
                    triple &OBBMax = Mesh.BoneBindings[MeshBoneIndex].OBBMax;

                    {for ( int32x BBoxVert = 0; BBoxVert < 8; BBoxVert++ )
                    {
                        triple OriginalPos;
                        VectorEquals3 ( OriginalPos, OBBMin );
                        if ( BBoxVert & 1 )
                        {
                            OriginalPos[0] = OBBMax[0];
                        }
                        if ( BBoxVert & 2 )
                        {
                            OriginalPos[1] = OBBMax[1];
                        }
                        if ( BBoxVert & 4 )
                        {
                            OriginalPos[2] = OBBMax[2];
                        }

                        triple RealPos;
                        triple ApproxPos;
                        VectorTransform4x3 ( RealPos  , OriginalPos, 1.0f, (real32*)WorldReal  [SkeletonBoneIndex] );
                        VectorTransform4x3 ( ApproxPos, OriginalPos, 1.0f, (real32*)WorldApprox[SkeletonBoneIndex] );

                        real32 DistanceSq = SquaredDistanceBetween3 ( RealPos, ApproxPos );
                        real32 CurrDistSq = Square(TestedTrack.LODError);
                        if ( CurrDistSq < DistanceSq )
                        {
                            TestedTrack.LODError = SquareRoot(DistanceSq);
                        }
                    }}
                }}
            }
        }}
    }}

    {for ( int32x Pose = 0; Pose < 3; Pose ++ )
    {
        FreeLocalPose(LocalPose[Pose]);
        FreeWorldPose(WorldPose[Pose]);
    }}
}


void GRANNY
CalculateLODErrorValuesAllBindings ( model_instance &ModelInstance,
                                     mesh_binding const &MeshBinding,
                                     bool AnimationHasScales,
                                     real32 ManualScaler )
{
    model_control_binding *Sentinel = &GetModelBindingSentinel(ModelInstance);
    {for(model_control_binding *Iterator = Sentinel->ModelNext;
         Iterator != Sentinel;
         Iterator = Iterator->ModelNext)
    {
        model_control_binding &ControlBinding = *Iterator;
        Assert ( ControlBinding.Callbacks->GetControlledAnimation != NULL );
        controlled_animation *ControlledAnim = ControlBinding.Callbacks->GetControlledAnimation ( ControlBinding );
        if ( ControlledAnim == NULL )
        {
            // Oh - wasn't an animation (may have been a pose).
        }
        else
        {
            animation_binding &AnimationBinding = *GetControlledAnimationBinding ( *ControlledAnim );
            CalculateLODErrorValues ( AnimationBinding, MeshBinding, AnimationHasScales, ManualScaler );
        }
    }}
}

real32 GRANNY
GetLODErrorValue ( animation_binding const &AnimationBinding,
                   int32x SkeletonBoneIndex )
{
    return AnimationBinding.TrackBindings[SkeletonBoneIndex].LODError;
}

void GRANNY
SetLODErrorValue ( animation_binding &AnimationBinding, int32x SkeletonBoneIndex, real32 NewError )
{
    AnimationBinding.TrackBindings[SkeletonBoneIndex].LODError = NewError;
}


void GRANNY
CopyLODErrorValuesToAnimation(animation_binding &AnimationBinding, real32 ManualScaler, bool OnlyCopyIfGreater)
{
    track_group &TrackGroup = *GetTrackGroup(&AnimationBinding);
    if ( TrackGroup.TransformLODErrorCount > 0 )
    {
        Assert ( TrackGroup.TransformLODErrorCount == TrackGroup.TransformTrackCount );

        {for(int32x BoneIndex = 0;
             BoneIndex < AnimationBinding.TrackBindingCount;
             ++BoneIndex)
        {
            bound_transform_track &Track = AnimationBinding.TrackBindings[BoneIndex];
            int32x SourceTrackIndex = Track.SourceTrackIndex;
            if ( SourceTrackIndex >= 0 )
            {
                Assert ( SourceTrackIndex < TrackGroup.TransformTrackCount );
                //transform_track const &SourceTrack = TrackGroup.TransformTracks[SourceTrackIndex];

                real32 NewLODError = Track.LODError * ManualScaler;
                if ( OnlyCopyIfGreater )
                {
                    if ( TrackGroup.TransformLODErrors[SourceTrackIndex] < NewLODError )
                    {
                        TrackGroup.TransformLODErrors[SourceTrackIndex] = NewLODError;
                    }
                }
                else
                {
                    TrackGroup.TransformLODErrors[SourceTrackIndex] = NewLODError;
                }
            }
        }}
    }
}


void GRANNY
CopyLODErrorValuesToAllAnimations(control &Control, real32 ManualScaler, bool OnlyCopyIfGreater)
{
    {for(model_control_binding *ModelControlBinding = &ControlModelsBegin(Control);
         ModelControlBinding != &ControlModelsEnd(Control);
         ModelControlBinding = &ControlModelsNext(*ModelControlBinding))
    {
        animation_binding *AnimationBinding = GetAnimationBindingFromControlBinding ( *ModelControlBinding );
        if ( AnimationBinding != NULL )
        {
            CopyLODErrorValuesToAnimation ( *AnimationBinding, ManualScaler, OnlyCopyIfGreater );
        }
    }}
}

void GRANNY
CopyLODErrorValuesFromAnimation(animation_binding &AnimationBinding, real32 ManualScaler)
{
    track_group &TrackGroup = *GetTrackGroup(&AnimationBinding);
    if ( TrackGroup.TransformLODErrorCount > 0 )
    {
        Assert ( TrackGroup.TransformLODErrorCount == TrackGroup.TransformTrackCount );

        {for(int32x BoneIndex = 0;
             BoneIndex < AnimationBinding.TrackBindingCount;
             ++BoneIndex)
        {
            bound_transform_track &Track = AnimationBinding.TrackBindings[BoneIndex];
            int32x SourceTrackIndex = Track.SourceTrackIndex;
            if ( SourceTrackIndex >= 0 )
            {
                Assert ( SourceTrackIndex < TrackGroup.TransformTrackCount );
                //transform_track const &SourceTrack = TrackGroup.TransformTracks[SourceTrackIndex];

                Track.LODError = TrackGroup.TransformLODErrors[SourceTrackIndex] * ManualScaler;
#if BOUND_TRANSFORM_TRACK_HAS_LOD
                SampleTrackUUULocalAtTime0 ( Track.SourceTrack, Track.LODTransform );
#endif
            }
        }}
    }
}

void GRANNY
CopyLODErrorValuesFromAllAnimations(control &Control, real32 ManualScaler)
{
    {for(model_control_binding *ModelControlBinding = &ControlModelsBegin(Control);
         ModelControlBinding != &ControlModelsEnd(Control);
         ModelControlBinding = &ControlModelsNext(*ModelControlBinding))
    {
        animation_binding *AnimationBinding = GetAnimationBindingFromControlBinding ( *ModelControlBinding );
        if ( AnimationBinding != NULL )
        {
            CopyLODErrorValuesFromAnimation ( *AnimationBinding, ManualScaler );
        }
    }}
}

void GRANNY
SetAllLODErrorValues(animation_binding &AnimationBinding, real32 NewValue)
{
    {for(int32x BoneIndex = 0;
         BoneIndex < AnimationBinding.TrackBindingCount;
         ++BoneIndex)
    {
        bound_transform_track &Track = AnimationBinding.TrackBindings[BoneIndex];
        Track.LODError = NewValue;
        if ( Track.SourceTrack != NULL )
        {
#if BOUND_TRANSFORM_TRACK_HAS_LOD
            SampleTrackUUULocalAtTime0 ( Track.SourceTrack, Track.LODTransform );
#endif
        }
    }}
}

void GRANNY
ResetLODErrorValues(animation_binding &AnimationBinding)
{
    // This says "if you do any approximations, the error is colossal". Which is a good default.
    SetAllLODErrorValues ( AnimationBinding, GetReal32AlmostInfinity() );
}


BEGIN_GRANNY_NAMESPACE;
struct animation_lod_builder
{
    animation const *Animation;
    bool AllocatedLODErrorSpace;
    bool AnimationHasScales;
    real32 ManualScaler;
};
END_GRANNY_NAMESPACE;

animation_lod_builder *GRANNY
CalculateAnimationLODBegin(animation const &Animation,
                           real32 ManualScaler)
{
    animation_lod_builder *Builder = Allocate ( animation_lod_builder );

    Builder->Animation = &Animation;
    Builder->AllocatedLODErrorSpace = false;
    Builder->AnimationHasScales = true;
    Builder->ManualScaler = ManualScaler;

    bool DidAllAllocations = true;
    bool DidNoAllocations = true;

    {for ( int32x TrackGroupNum = 0; TrackGroupNum < Animation.TrackGroupCount; TrackGroupNum++ )
    {
        track_group &TrackGroup = *Animation.TrackGroups[TrackGroupNum];
        if ( TrackGroup.TransformTrackCount == 0 )
        {
            // Whatever.
        }
        else
        {
            if ( TrackGroup.TransformLODErrorCount == TrackGroup.TransformTrackCount )
            {
                // Already got the LOD space.
                DidAllAllocations = false;
            }
            else
            {
                Assert ( TrackGroup.TransformLODErrorCount == 0 );
                AllocateLODErrorSpace ( TrackGroup );
                DidNoAllocations = false;
            }
        }
    }}

    if ( Animation.TrackGroupCount > 0 )
    {
        // If we had to allocate some and not others, that's probably bad - cleanup won't work properly.
        Assert ( DidAllAllocations || DidNoAllocations );
        // However, it won't crash, it will just leak memory.
        Builder->AllocatedLODErrorSpace = DidAllAllocations;

        // So now we reset all the space to a negative number (any one will do).
        // Then the app does (multiple) calls to CalculateAnimationLODAddMeshBinding
        // which only copies data back if it's greater than the existing error.
        // And then at the end, if there's any -1.0f values left, then
        // the app didn't give us any meshes that used those bones, so
        // we can't estimate any error, so we set them to AlmostInf.
        {for ( int32x TrackGroupNum = 0; TrackGroupNum < Animation.TrackGroupCount; TrackGroupNum++ )
        {
            track_group &TrackGroup = *Animation.TrackGroups[TrackGroupNum];
            SetAllLODErrorSpace ( TrackGroup, -31415.0f );
        }}
    }

    return Builder;
}

void GRANNY
CalculateAnimationLODAddMeshBinding(animation_lod_builder *Builder,
                                    model const &Model,
                                    mesh_binding const &MeshBinding,
                                    real32 ManualScaler)
{
    ManualScaler *= Builder->ManualScaler;

    // Create the binding.
    model_instance *ModelInstance = InstantiateModel ( Model );
    control *Control = PlayControlledAnimation ( 0.0f, *Builder->Animation, *ModelInstance );
    // Calculate the LOD values for it.
    CalculateLODErrorValuesAllBindings ( *ModelInstance, MeshBinding, Builder->AnimationHasScales, ManualScaler );

    // And copy it back to the animation.
    CopyLODErrorValuesToAllAnimations ( *Control, 1.0f, true );

    FreeControl ( Control );
    FreeModelInstance ( ModelInstance );
}

void GRANNY
CalculateAnimationLODEnd(animation_lod_builder *Builder)
{
    animation const &Animation = *Builder->Animation;
    {for ( int32x TrackGroupNum = 0; TrackGroupNum < Animation.TrackGroupCount; TrackGroupNum++ )
    {
        track_group &TrackGroup = *Animation.TrackGroups[TrackGroupNum];
        {for ( int32 TrackNum = 0; TrackNum < TrackGroup.TransformLODErrorCount; TrackNum++ )
        {
            real32 &LODError = TrackGroup.TransformLODErrors[TrackNum];
            if ( LODError < 0.0f )
            {
                Assert ( LODError == -31415.0f );
                // The app didn't give us any meshes that used this bone,
                // so we can't accurately estimate any error value. So be pessimistic.
                // (this is common for attachment points and suchlike, and you don't want any LOD for them anyway).
                LODError = GetReal32AlmostInfinity();
            }
        }}
    }}
}

void GRANNY
CalculateAnimationLODCleanup(animation_lod_builder *Builder)
{
    animation const &Animation = *Builder->Animation;
    if ( Builder->AllocatedLODErrorSpace )
    {
        // Clean up the error space we allocated.
        {for ( int32x TrackGroupNum = 0; TrackGroupNum < Animation.TrackGroupCount; TrackGroupNum++ )
        {
            track_group &TrackGroup = *Animation.TrackGroups[TrackGroupNum];
            if ( TrackGroup.TransformLODErrorCount > 0 )
            {
                Assert ( TrackGroup.TransformLODErrorCount == TrackGroup.TransformTrackCount );
                FreeLODErrorSpace ( TrackGroup );
            }
        }}
    }

    DeallocateSafe ( Builder );
}




