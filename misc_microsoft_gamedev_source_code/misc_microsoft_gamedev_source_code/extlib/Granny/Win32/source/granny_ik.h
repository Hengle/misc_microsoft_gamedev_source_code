#if !defined(GRANNY_IK_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_ik.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
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

BEGIN_GRANNY_NAMESPACE EXPGROUP(IKGroup);

struct skeleton;
struct local_pose;
struct world_pose;

EXPAPI GS_PARAM void IKUpdate(int32x LinkCount, int32x EEBoneIndex,
                              real32 const *DesiredPosition3,
                              int32x IterationCount,
                              skeleton const &Skeleton,
                              real32 const *ModelRoot4x4,
                              local_pose &LocalPose,
                              world_pose &WorldPose);


EXPAPI GS_PARAM bool IKUpdate2Bone(int32x EEBoneIndex,
                                   real32 const *DesiredPosition3,
                                   real32 const *RestrictedMovementPlaneNormal3,
                                   skeleton const &Skeleton,
                                   real32 const *ModelRootTransform,
                                   local_pose &LocalPose,
                                   world_pose &WorldPose,
                                   real32 HyperExtensionStart,
                                   real32 HyperExtensionScale);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_IK_H
#endif
