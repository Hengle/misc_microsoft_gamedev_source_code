#if !defined(GRANNY_SPU_ANIMATION_BINDING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation_binding.h $
// $DateTime: 2007/09/07 12:15:29 $
// $Change: 15917 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(SPUGroup);

struct spu_animation;
struct spu_track_group;
struct spu_transform_track;
struct model;
struct skeleton;

EXPTYPE_EPHEMERAL struct spu_animation_binding_id
{
    spu_animation const*   Animation;
    spu_track_group const* TrackGroup;

    model const*           Model;

    char const *TrackPattern;
    char const *BonePattern;

    // This is a cache to ease the dma stall requirements on the SPU side...
    int32                TransformTrackCount;
    spu_transform_track* TransformTracks;
    int32                CurveByteCount;
    uint8*               CurveBytes;
};

EXPTYPE_EPHEMERAL struct spu_animation_binding
{
    spu_animation_binding_id ID;

    int32  TrackNameRemapCount;
    int32* TrackNameRemaps;

    spu_animation_binding *Left;
    spu_animation_binding *Right;

    int32 UsedBy;
    spu_animation_binding *PreviousUnused;
    spu_animation_binding *NextUnused;
};

// We want the animation binding to be aligned to 16 bytes on the PS3, but we really don't
// care about the other platforms.  (X64, in particular)
#if PROCESSOR_32BIT_POINTER
CompileAssert(IS_ALIGNED_16(sizeof(spu_animation_binding)));
#endif



EXPAPI GS_READ void MakeDefaultSPUAnimationBindingID(
    spu_animation_binding_id &ID,
    spu_animation const* Animation,
    int32x TrackGroupIndex,
    model const* Model);

EXPAPI GS_MODIFY spu_animation_binding *AcquireSPUAnimationBindingFromID(
    spu_animation_binding_id &ID);

EXPAPI GS_MODIFY spu_animation_binding *AcquireSPUAnimationBinding(
    spu_animation_binding *Binding);

EXPAPI GS_MODIFY void ReleaseSPUAnimationBinding(spu_animation_binding *Binding);

EXPAPI GS_READ   int32x GetMaximumSPUAnimationBindingCount(void);
EXPAPI GS_MODIFY void SetMaximumSPUAnimationBindingCount(int32x BindingCountMax);

// EXPAPI GS_MODIFY void FlushAllUnusedAnimationBindings(void);
// EXPAPI GS_MODIFY void FlushAllBindingsForAnimation(animation const *Animation);
// EXPAPI GS_MODIFY void FlushAnimationBinding(animation_binding *Binding);

// EXPAPI GS_READ animation_binding *GetFirstBindingForAnimation(animation const *Animation);
// EXPAPI GS_READ animation_binding *GetNextBindingForAnimation(animation const *Animation,
//                                                              animation_binding *Binding);

// EXPAPI GS_READ animation_binding *GetFirstUnusedAnimationBinding(void);
// EXPAPI GS_READ animation_binding *GetNextUnusedAnimationBinding(animation_binding *Binding);

// EXPAPI GS_READ bool IsAnimationUsed(animation const *Animation);

// EXPAPI GS_MODIFY void RemapAnimationBindingPointers(animation_binding *Binding,
//                                                     animation const *NewAnimationPointer);
// EXPAPI GS_MODIFY void RemapAllAnimationBindingPointers(animation const *OldAnimationPointer,
//                                                        animation const *NewAnimationPointer);

// animation_binding *GetFirstAnimationBinding(void);
// animation_binding *GetNextAnimationBinding(animation_binding *Binding);

// void ComputeTotalDeltasFromBinding(animation_binding* Binding,
//                                    triple*            TotalPositionDelta,
//                                    triple*            TotalOrientationDelta);


// // structure actually defined in stat_hud.h, see note there
// struct animation_binding_cache_status;

// EXPAPI GS_READ void GetAnimationBindingCacheStatus(
//     animation_binding_cache_status &Result);

// inline track_group *
// GetTrackGroup(animation_binding_identifier &ID)
// {
//     return(ID.Animation->TrackGroups[ID.SourceTrackGroupIndex]);
// }

// inline track_group *
// GetTrackGroup(animation_binding *Binding)
// {
//     return(GetTrackGroup(Binding->ID));
// }

// bool AreTrackFlagsConsistent(bound_transform_track const &BoundTrack);


// EXPAPI GS_MODIFY void CalculateLODErrorValues(animation_binding &Binding,
//                                               mesh_binding const &MeshBinding,
//                                               bool AnimationHasScales,
//                                               real32 ManualScaler);

// EXPAPI GS_MODIFY void CalculateLODErrorValuesAllBindings(model_instance &ModelInstance,
//                                                          mesh_binding const &MeshBinding,
//                                                          bool AnimationHasScales,
//                                                          real32 ManualScaler);

// EXPAPI GS_READ real32 GetLODErrorValue(animation_binding const &AnimationBinding,
//                                        int32x SkeletonBoneIndex);
// EXPAPI GS_PARAM void SetLODErrorValue(animation_binding &AnimationBinding,
//                                       int32x SkeletonBoneIndex,
//                                       real32 NewError);

// EXPAPI GS_PARAM void CopyLODErrorValuesToAnimation(animation_binding &Binding, real32 ManualScaler, bool OnlyCopyIfGreater);
// EXPAPI GS_PARAM void CopyLODErrorValuesToAllAnimations(control &Control, real32 ManualScaler, bool OnlyCopyIfGreater);
// EXPAPI GS_PARAM void CopyLODErrorValuesFromAnimation(animation_binding &Binding, real32 ManualScaler);
// EXPAPI GS_PARAM void CopyLODErrorValuesFromAllAnimations(control &Control, real32 ManualScaler);
// EXPAPI GS_MODIFY void SetAllLODErrorValues(animation_binding &AnimationBinding, real32 NewValue);
// EXPAPI GS_MODIFY void ResetLODErrorValues(animation_binding &AnimationBinding);

// EXPTYPE struct animation_lod_builder;
// EXPAPI GS_MODIFY animation_lod_builder *CalculateAnimationLODBegin(animation const &Animation,
//                                                                    real32 ManualScaler);

// EXPAPI GS_MODIFY void CalculateAnimationLODAddMeshBinding(animation_lod_builder *Builder,
//                                                           model const &Model,
//                                                           mesh_binding const &MeshBinding,
//                                                           real32 ManualScaler);

// EXPAPI GS_MODIFY void CalculateAnimationLODEnd(animation_lod_builder *Builder);
// EXPAPI GS_MODIFY void CalculateAnimationLODCleanup(animation_lod_builder *Builder);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SPU_ANIMATION_BINDING_H
#endif
