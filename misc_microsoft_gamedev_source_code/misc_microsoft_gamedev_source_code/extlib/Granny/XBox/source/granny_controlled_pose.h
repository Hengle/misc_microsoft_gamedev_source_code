#if !defined(GRANNY_CONTROLLED_POSE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_controlled_pose.h $
// $DateTime: 2006/12/01 16:01:17 $
// $Change: 13831 $
// $Revision: #6 $
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
struct local_pose;
struct model_instance;
struct track_mask;

struct controlled_pose
{
    // These data structures all have to fit inside model_control_binding::Derived[]
    local_pose const *Pose;
    track_mask const *ModelMask;
};


EXPAPI GS_MODIFY control *PlayControlledPose(real32 StartTime, real32 Duration,
                                             local_pose const &Pose,
                                             model_instance &Model,
                                             track_mask *ModelMask);

EXPAPI GS_READ local_pose *GetLocalPoseFromControlBinding(model_control_binding &Binding);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CONTROLLED_POSE_H
#endif
