#if !defined(GRANNY_WORLD_POSE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_world_pose.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(WorldPoseGroup);

struct skeleton;
struct local_pose;

EXPTYPE_EPHEMERAL struct world_pose;
struct world_pose
{
    int32x BoneCount;
    matrix_4x4 *WorldTransformBuffer;
    matrix_4x4 *CompositeTransformBuffer;
};

EXPTYPE enum composite_flag
{
    IncludeComposites = 0,
    ExcludeComposites = 1
};

EXPAPI GS_SAFE world_pose *NewWorldPose(int32x BoneCount);
EXPAPI GS_SAFE world_pose *NewWorldPoseNoComposite(int32x BoneCount);
EXPAPI GS_PARAM void FreeWorldPose(world_pose *WorldPose);

EXPAPI GS_READ int32x GetWorldPoseBoneCount(world_pose const &WorldPose);

EXPAPI GS_SAFE int32x GetResultingWorldPoseSize(int32x BoneCount, composite_flag CompositeFlag);
EXPAPI GS_SAFE world_pose *NewWorldPoseInPlace(int32x BoneCount, composite_flag CompositeFlag, void *Memory);

EXPAPI GS_PARAM real32 *GetWorldPose4x4(world_pose const &WorldPose,
                                        int32x BoneIndex);
EXPAPI GS_PARAM real32 *GetWorldPoseComposite4x4(world_pose const &WorldPose,
                                                 int32x BoneIndex);
EXPAPI GS_PARAM matrix_4x4 *GetWorldPose4x4Array(world_pose const &WorldPose);
EXPAPI GS_PARAM matrix_4x4 *GetWorldPoseComposite4x4Array(world_pose const &WorldPose);

EXPAPI GS_PARAM void BuildWorldPose(skeleton const &Skeleton,
                                    int32x FirstBone, int32x BoneCount,
                                    local_pose const &LocalPose,
                                    real32 const *Offset4x4,
                                    world_pose &Result);
EXPAPI GS_PARAM void BuildWorldPoseLOD(skeleton const &Skeleton,
                                       int32x FirstBone, int32x BoneCount,
                                       int32x FirstValidLocalBone, int32x ValidLocalBoneCount,
                                       local_pose const &LocalPose,
                                       real32 const *Offset4x4,
                                       world_pose &Result);
EXPAPI GS_PARAM void BuildWorldPoseSparse(skeleton const &Skeleton,
                                          int32x FirstBone, int32x BoneCount,
                                          int32x const *SparseBoneArray,
                                          int32x const *SparseBoneArrayReverse,
                                          local_pose const &LocalPose,
                                          real32 const *Offset4x4,
                                          world_pose &Result);

EXPAPI GS_PARAM void BuildRestWorldPose(skeleton const &Skeleton,
                                        int32x FirstBone, int32x BoneCount,
                                        real32 const *Offset4x4,
                                        world_pose &Result);

EXPAPI GS_PARAM void BuildWorldPoseNoComposite(skeleton const &Skeleton,
                                               int32x FirstBone, int32x BoneCount,
                                               local_pose const &LocalPose,
                                               real32 const *Offset4x4,
                                               world_pose &Result);
EXPAPI GS_PARAM void BuildWorldPoseNoCompositeLOD(skeleton const &Skeleton,
                                                  int32x FirstBone, int32x BoneCount,
                                                  int32x FirstValidLocalBone, int32x ValidLocalBoneCount,
                                                  local_pose const &LocalPose,
                                                  real32 const *Offset4x4,
                                                  world_pose &Result);
EXPAPI GS_PARAM void BuildWorldPoseNoCompositeSparse(skeleton const &Skeleton,
                                                     int32x FirstBone, int32x BoneCount,
                                                     int32x const *SparseBoneArray,
                                                     int32x const *SparseBoneArrayReverse,
                                                     local_pose const &LocalPose,
                                                     real32 const *Offset4x4,
                                                     world_pose &Result);
EXPAPI GS_PARAM void BuildWorldPoseComposites(skeleton const &Skeleton,
                                              int32x FirstBone, int32x BoneCount,
                                              world_pose &Result);

EXPAPI GS_PARAM void BuildCompositeBuffer(skeleton const &Skeleton,
                                          int32x FirstBone, int32x BoneCount,
                                          world_pose const &Pose,
                                          matrix_4x4* CompositeBuffer);

EXPAPI GS_PARAM void BuildCompositeBufferTransposed(skeleton const &Skeleton,
                                                    int32x FirstBone, int32x BoneCount,
                                                    world_pose const &Pose,
                                                    matrix_3x4* CompositeBuffer);

EXPAPI GS_PARAM void BuildIndexedCompositeBuffer(skeleton const &Skeleton,
                                                 world_pose const &Pose,
                                                 int32x const* Indices,
                                                 int32x IndexCount,
                                                 matrix_4x4* CompositeBuffer);

EXPAPI GS_PARAM void BuildIndexedCompositeBufferTransposed(skeleton const &Skeleton,
                                                           world_pose const &Pose,
                                                           int32x const* Indices,
                                                           int32x IndexCount,
                                                           matrix_3x4* CompositeBuffer);

EXPAPI GS_PARAM void UpdateWorldPoseChildren(skeleton const &Skeleton,
                                             int32x ParentBone,
                                             local_pose &LocalPose,
                                             real32 const *Offset4x4,
                                             world_pose &Result);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_WORLD_POSE_H
#endif
