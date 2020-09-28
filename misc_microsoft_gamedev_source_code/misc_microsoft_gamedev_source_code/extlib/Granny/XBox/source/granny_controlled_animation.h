#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_controlled_animation.h $
// $DateTime: 2006/12/01 16:01:17 $
// $Change: 13831 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_MODEL_CONTROL_BINDING_H)
#include "granny_model_control_binding.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ControlledAnimationGroup);

struct control;
struct animation;
struct animation_binding;
struct model_instance;
struct model;
struct track_mask;
struct animation_binding;

EXPTYPE enum accumulation_mode
{
    NoAccumulation,
    ConstantExtractionAccumulation,
    VariableDeltaAccumulation,
};

struct controlled_animation
{
    // These data structures all have to fit inside model_control_binding::Derived[]
    animation_binding *Binding;
    accumulation_mode AccumulationMode;
    track_mask const *TrackMask;
    track_mask const *ModelMask;
};


EXPTYPE struct controlled_animation_builder;

EXPAPI GS_MODIFY control *PlayControlledAnimation(real32 StartTime,
                                                  animation const &Animation,
                                                  model_instance &Model);
EXPAPI GS_MODIFY control *PlayControlledAnimationBinding(real32 StartTime,
                                                         animation const &Animation,
                                                         animation_binding &Binding,
                                                         model_instance &Model);

EXPAPI GS_MODIFY controlled_animation_builder *BeginControlledAnimation(
    real32 StartTime, animation const &Animation);
EXPAPI GS_MODIFY control *EndControlledAnimation(controlled_animation_builder *Builder);

EXPAPI GS_MODIFY void UseExistingControlForAnimation(controlled_animation_builder *Builder,
                                                     control *Control);

EXPAPI GS_MODIFY void SetTrackMatchRule(controlled_animation_builder &Builder,
                                        int32x TrackGroupIndex,
                                        char const *TrackPattern,
                                        char const *BonePattern);

EXPAPI GS_MODIFY void SetTrackGroupTarget(controlled_animation_builder &Builder,
                                          int32x TrackGroupIndex,
                                          model_instance &Model);
EXPAPI GS_MODIFY void SetTrackGroupBinding(controlled_animation_builder &Builder,
                                           int32x TrackGroupIndex,
                                           animation_binding &Binding);
EXPAPI GS_MODIFY void SetTrackGroupBasisTransform(controlled_animation_builder &Builder,
                                                  int32x TrackGroupIndex,
                                                  model &FromModel, model &ToModel);
EXPAPI GS_MODIFY void SetTrackGroupTrackMask(controlled_animation_builder &Builder,
                                             int32x TrackGroupIndex,
                                             track_mask &TrackMask);
EXPAPI GS_MODIFY void SetTrackGroupModelMask(controlled_animation_builder &Builder,
                                             int32x TrackGroupIndex,
                                             track_mask &ModelMask);
EXPAPI GS_MODIFY void SetTrackGroupAccumulation(controlled_animation_builder &Builder,
                                                int32x TrackGroupIndex, accumulation_mode Mode);

EXPAPI GS_MODIFY void SetTrackGroupLOD(controlled_animation_builder &Builder,
                                       int32x TrackGroupIndex,
                                       bool CopyValuesFromAnimation,
                                       real32 ManualScaler);


animation_binding *GetControlledAnimationBinding(controlled_animation &ControlledAnimation);

EXPAPI GS_READ animation_binding *GetAnimationBindingFromControlBinding(model_control_binding &Binding);


EXPAPI GS_READ real32 GetGlobalLODFadingFactor(void);
EXPAPI GS_MODIFY void SetGlobalLODFadingFactor(real32 NewValue);

void FindAllowedErrorNumbers ( real32 AllowedError, real32 *AllowedErrorEnd, real32 *AllowedErrorScaler );

struct bound_transform_track;
struct transform_track;
bool CheckLODTransform ( bound_transform_track const &Track, transform_track const &SourceTrack );

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CONTROLLED_ANIMATION_H
#endif
