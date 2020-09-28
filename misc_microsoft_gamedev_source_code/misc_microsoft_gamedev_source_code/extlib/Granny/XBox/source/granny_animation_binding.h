#if !defined(GRANNY_ANIMATION_BINDING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_animation_binding.h $
// $DateTime: 2007/01/15 10:23:30 $
// $Change: 14204 $
// $Revision: #28 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_TRACK_SAMPLER_H)
#include "granny_track_sampler.h"
#endif

#if !defined(GRANNY_LOCAL_POSE_H)
#include "granny_local_pose.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_MESH_BINDING_H)
#include "granny_mesh_binding.h"
#endif

#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "granny_model_instance.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(AnimationBindingGroup);

struct animation;
struct track_group;
struct model;


// Some people don't use LOD, and are severely memory-constrained.
// (e.g. mobile phones). So they get a special build that shrinks
// the size of the bound_transform_track quite a bit by
// removing the "LODTransform" item.
// If you are finding this to be a problem for you, email us and
// let us know.
// But even if you don't explicitly use LOD, this is still a
// decent speed improvement for tracks that are constant (which
// is a surprisingly large number).
#define BOUND_TRANSFORM_TRACK_HAS_LOD 1

EXPTYPE_EPHEMERAL enum bound_transform_track_flags
{
    // Flags to indicate the curve type associated with the various
    // channels of the track_group.  Using these instead of the more
    // general CurveIs* class of functions prevents a large number of
    // cache misses when the curve is constant (the common case).

    // Position curve
    BoundPositionCurveIsIdentity    = (0x0 << 0),
    BoundPositionCurveIsConstant    = (0x1 << 0),
    BoundPositionCurveIsKeyframed   = (0x2 << 0),
    BoundPositionCurveIsGeneral     = (0x3 << 0),
    BoundPositionCurveFlagMask      = (0x3 << 0),

    // Orientation curve
    BoundOrientationCurveIsIdentity    = (0x0 << 2),
    BoundOrientationCurveIsConstant    = (0x1 << 2),
    BoundOrientationCurveIsKeyframed   = (0x2 << 2),
    BoundOrientationCurveIsGeneral     = (0x3 << 2),
    BoundOrientationCurveFlagMask      = (0x3 << 2),

    // Orientation curve
    BoundScaleShearCurveIsIdentity    = (0x0 << 4),
    BoundScaleShearCurveIsConstant    = (0x1 << 4),
    BoundScaleShearCurveIsKeyframed   = (0x2 << 4),
    BoundScaleShearCurveIsGeneral     = (0x3 << 4),
    BoundScaleShearCurveFlagMask      = (0x3 << 4)
};

EXPTYPE_EPHEMERAL struct bound_transform_track
{
    int16 SourceTrackIndex;
    int8  QuaternionMode;
    int8  Flags;

    transform_track const *SourceTrack;
    track_sampler *Sampler;
    real32 LODError;

    // This can be removed if BOUND_TRANSFORM_TRACK_HAS_LOD is 0.  It's not #if'd here
    // because of the way Granny extracts structures for inclusion in granny.h
    transform LODTransform;
};

// Handy macro for checking the flags field of the bound_transform_track.  Use as:
//  BOUND_CURVE_CHECK(BoundTrack, Position, IsKeyframed)
//  BOUND_CURVE_CHECK(BoundTrack, Orientation, IsConstant)
//  BOUND_CURVE_CHECK(BoundTrack, Orientation, IsGeneral)
// etc.
#define BOUND_CURVE_CHECK(BoundTrack, CurveID, Flag) (((BoundTrack).Flags & Bound ## CurveID ## CurveFlagMask) == Bound ## CurveID ## Curve ## Flag)


EXPTYPE_EPHEMERAL struct animation_binding_identifier
{
    animation const *Animation;
    int32x SourceTrackGroupIndex;

    char const *TrackPattern;
    char const *BonePattern;

    model const *OnModel;

    model *FromBasis;
    model *ToBasis;
};


EXPTYPE_EPHEMERAL struct animation_binding
{
    animation_binding_identifier ID;

    void *RebasingMemory;

    int32x TrackBindingCount;
    bound_transform_track *TrackBindings;

    animation_binding *Left;
    animation_binding *Right;

    int32x UsedBy;
    animation_binding *PreviousUnused;
    animation_binding *NextUnused;
};

EXPAPI GS_READ void MakeDefaultAnimationBindingID(
    animation_binding_identifier &ID,
    animation const *Animation,
    int32x TrackGroupIndex);

EXPAPI GS_MODIFY animation_binding *AcquireAnimationBindingFromID(
    animation_binding_identifier &ID);

EXPAPI GS_MODIFY animation_binding *AcquireAnimationBinding(
    animation_binding *Binding);

EXPAPI GS_MODIFY void ReleaseAnimationBinding(animation_binding *Binding);

EXPAPI GS_READ   int32x GetMaximumAnimationBindingCount(void);
EXPAPI GS_MODIFY void SetMaximumAnimationBindingCount(int32x BindingCountMax);

EXPAPI GS_MODIFY void FlushAllUnusedAnimationBindings(void);
EXPAPI GS_MODIFY void FlushAllBindingsForAnimation(animation const *Animation);
EXPAPI GS_MODIFY void FlushAnimationBinding(animation_binding *Binding);

EXPAPI GS_READ animation_binding *GetFirstBindingForAnimation(animation const *Animation);
EXPAPI GS_READ animation_binding *GetNextBindingForAnimation(animation const *Animation,
                                                             animation_binding *Binding);

EXPAPI GS_READ animation_binding *GetFirstUnusedAnimationBinding(void);
EXPAPI GS_READ animation_binding *GetNextUnusedAnimationBinding(animation_binding *Binding);

EXPAPI GS_READ bool IsAnimationUsed(animation const *Animation);

EXPAPI GS_MODIFY void RemapAnimationBindingPointers(animation_binding *Binding,
                                                    animation const *NewAnimationPointer);
EXPAPI GS_MODIFY void RemapAllAnimationBindingPointers(animation const *OldAnimationPointer,
                                                       animation const *NewAnimationPointer);

animation_binding *GetFirstAnimationBinding(void);
animation_binding *GetNextAnimationBinding(animation_binding *Binding);

void ComputeTotalDeltasFromBinding(animation_binding* Binding,
                                   triple*            TotalPositionDelta,
                                   triple*            TotalOrientationDelta);


// structure actually defined in stat_hud.h, see note there
struct animation_binding_cache_status;

EXPAPI GS_READ void GetAnimationBindingCacheStatus(
    animation_binding_cache_status &Result);

inline track_group *
GetTrackGroup(animation_binding_identifier &ID)
{
    return(ID.Animation->TrackGroups[ID.SourceTrackGroupIndex]);
}

inline track_group *
GetTrackGroup(animation_binding *Binding)
{
    return(GetTrackGroup(Binding->ID));
}

bool AreTrackFlagsConsistent(bound_transform_track const &BoundTrack);


EXPAPI GS_MODIFY void CalculateLODErrorValues(animation_binding &Binding,
                                              mesh_binding const &MeshBinding,
                                              bool AnimationHasScales,
                                              real32 ManualScaler);

EXPAPI GS_MODIFY void CalculateLODErrorValuesAllBindings(model_instance &ModelInstance,
                                                         mesh_binding const &MeshBinding,
                                                         bool AnimationHasScales,
                                                         real32 ManualScaler);

EXPAPI GS_READ real32 GetLODErrorValue(animation_binding const &AnimationBinding,
                                       int32x SkeletonBoneIndex);
EXPAPI GS_PARAM void SetLODErrorValue(animation_binding &AnimationBinding,
                                      int32x SkeletonBoneIndex,
                                      real32 NewError);

EXPAPI GS_PARAM void CopyLODErrorValuesToAnimation(animation_binding &Binding, real32 ManualScaler, bool OnlyCopyIfGreater);
EXPAPI GS_PARAM void CopyLODErrorValuesToAllAnimations(control &Control, real32 ManualScaler, bool OnlyCopyIfGreater);
EXPAPI GS_PARAM void CopyLODErrorValuesFromAnimation(animation_binding &Binding, real32 ManualScaler);
EXPAPI GS_PARAM void CopyLODErrorValuesFromAllAnimations(control &Control, real32 ManualScaler);
EXPAPI GS_MODIFY void SetAllLODErrorValues(animation_binding &AnimationBinding, real32 NewValue);
EXPAPI GS_MODIFY void ResetLODErrorValues(animation_binding &AnimationBinding);

EXPTYPE struct animation_lod_builder;
EXPAPI GS_MODIFY animation_lod_builder *CalculateAnimationLODBegin(animation const &Animation,
                                                                   real32 ManualScaler);

EXPAPI GS_MODIFY void CalculateAnimationLODAddMeshBinding(animation_lod_builder *Builder,
                                                          model const &Model,
                                                          mesh_binding const &MeshBinding,
                                                          real32 ManualScaler);

EXPAPI GS_MODIFY void CalculateAnimationLODEnd(animation_lod_builder *Builder);
EXPAPI GS_MODIFY void CalculateAnimationLODCleanup(animation_lod_builder *Builder);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_ANIMATION_BINDING_H
#endif
