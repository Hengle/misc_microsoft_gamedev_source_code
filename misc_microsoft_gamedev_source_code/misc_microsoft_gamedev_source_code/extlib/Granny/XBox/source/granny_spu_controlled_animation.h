#if !defined(GRANNY_SPU_CONTROLLED_ANIMATION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_controlled_animation.h $
// $DateTime: 2007/09/07 12:15:29 $
// $Change: 15917 $
// $Revision: #2 $
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

#if !defined(GRANNY_CONTROLLED_ANIMATION_H)
#include "granny_controlled_animation.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(SPUGroup);

struct control;
struct spu_animation;
struct spu_animation_binding;
struct model_instance;
struct track_mask;

EXPTYPE struct controlled_spu_animation_builder;

struct spu_controlled_animation
{
    // These data structures all have to fit inside model_control_binding::Derived[]
    spu_animation_binding* Binding;
    accumulation_mode AccumulationMode;
    track_mask const *TrackMask;
    track_mask const *ModelMask;
};
CompileAssert(SizeOf(spu_controlled_animation) <= SizeOf(((model_control_binding*)0)->Derived));


EXPAPI GS_MODIFY control *PlayControlledSPUAnimation(real32 StartTime,
                                                     spu_animation const &Animation,
                                                     model_instance &Model);


controlled_spu_animation_builder* BeginControlledSPUAnimation(real32 StartTime,
                                                              spu_animation const &Animation);
control* EndControlledSPUAnimation(controlled_spu_animation_builder *Builder);

void SetSPUTrackGroupTarget(controlled_spu_animation_builder& Builder,
                            int32x TrackGroupIndex,
                            model_instance& Model);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SPU_CONTROLLED_ANIMATION_H
#endif
