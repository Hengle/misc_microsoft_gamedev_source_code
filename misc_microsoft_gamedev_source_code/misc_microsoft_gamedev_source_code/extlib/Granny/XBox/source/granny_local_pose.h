#if !defined(GRANNY_LOCAL_POSE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_local_pose.h $
// $DateTime: 2007/09/21 09:55:56 $
// $Change: 16039 $
// $Revision: #25 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(LocalPoseGroup);

struct skeleton;
struct world_pose;
struct transform;
struct track_mask;

struct local_pose_transform
{
    // These values all used by Begin/Accumulate/EndLocalTransform. Anything else only cares about Transform
    real32 Weight;
    int32 Count;
    transform Transform;
    int32 TraversalID;
};
CompileAssert(IS_ALIGNED_16(sizeof(local_pose_transform)));

struct local_pose
{
    int32 BoneCount;
    local_pose_transform *Transforms;

    real32 FillThreshold;
    int32  TraversalID;
};

EXPCONST EXPGROUP(local_pose) extern data_type_definition LocalPoseTransformType[];
EXPCONST EXPGROUP(local_pose) extern data_type_definition LocalPoseType[];


EXPTYPE struct local_pose;
struct local_pose_transform;

EXPAPI GS_SAFE local_pose *NewLocalPose(int32x BoneCount);
EXPAPI GS_PARAM void FreeLocalPose(local_pose *LocalPose);

EXPAPI GS_SAFE int32x GetResultingLocalPoseSize(int32x BoneCount);
EXPAPI GS_PARAM local_pose *NewLocalPoseInPlace(int32x BoneCount, void *Memory);

EXPAPI GS_READ int32x GetLocalPoseBoneCount(local_pose const &LocalPose);
EXPAPI GS_READ transform *GetLocalPoseTransform(local_pose const &LocalPose,
                                                int32x BoneIndex);
EXPAPI GS_READ void* GetLocalPoseOpaqueTransformArray(local_pose const& LocalPose);

EXPAPI GS_PARAM void BuildRestLocalPose(skeleton const &Skeleton,
                                        int32x FirstBone, int32x BoneCount,
                                        local_pose &LocalPose);


EXPAPI GS_PARAM void BeginLocalPoseAccumulation(local_pose &LocalPose,
                                                int32x FirstBone, int32x BoneCount, int32x const *SparseBoneArray);

// NB: These are stored in an int8 in the bound_transform_track, so don't add any values
// outside that range...
EXPTYPE enum quaternion_mode
{
    BlendQuaternionDirectly            = 0,
    BlendQuaternionInverted            = 1,
    BlendQuaternionNeighborhooded      = 2,
    BlendQuaternionAccumNeighborhooded = 3,
};
EXPAPI GS_PARAM void AccumulateLocalTransform(local_pose &LocalPose,
                                              int32x LocalPoseBoneIndex, int32x SkeletonBoneIndex,
                                              real32 Weight,
                                              skeleton const &ReferenceSkeleton,
                                              quaternion_mode Mode,
                                              transform const &Transform);
EXPAPI GS_PARAM void EndLocalPoseAccumulation(local_pose &LocalPose,
                                              int32x FirstBone, int32x BoneCount,
                                              skeleton const &ReferenceSkeleton,
                                              int32x const *SparseBoneArray);
EXPAPI GS_PARAM void EndLocalPoseAccumulationLOD(local_pose &LocalPose,
                                                 int32x FirstBone, int32x BoneCount,
                                                 skeleton const &ReferenceSkeleton,
                                                 real32 AllowedError,
                                                 int32x const *SparseBoneArray);

EXPAPI GS_READ real32 GetLocalPoseFillThreshold(local_pose const &LocalPose);
EXPAPI GS_PARAM void SetLocalPoseFillThreshold(local_pose &LocalPose,
                                               real32 FillThreshold);

EXPAPI GS_PARAM void GetWorldMatrixFromLocalPose(skeleton const &Skeleton,
                                                 int32x BoneIndex,
                                                 local_pose const &LocalPose,
                                                 real32 const *Offset4x4,
                                                 real32 *Result4x4,
                                                 int32x const *SparseBoneArray,
                                                 int32x const *SparseBoneArrayReverse);
EXPAPI GS_PARAM void GetAttachmentOffset(skeleton const &Skeleton,
                                         int32x BoneIndex,
                                         local_pose const &LocalPose,
                                         real32 const *Offset4x4,
                                         real32 *Result4x4,
                                         int32x const *SparseBoneArray,
                                         int32x const *SparseBoneArrayReverse);

EXPAPI GS_PARAM void ModulationCompositeLocalPoseSparse(local_pose &BasePose,
                                                        real32 WeightNone, real32 WeightAll,
                                                        track_mask *Mask,
                                                        local_pose const &CompositePose,
                                                        int32x const *SparseBoneArray);

EXPAPI GS_PARAM void LocalPoseFromWorldPose(skeleton const* Skeleton,
                                            local_pose *Result,
                                            world_pose const &WorldPose,
                                            real32 const *Offset4x4,
                                            int32x FirstBone,
                                            int32x BoneCount);
EXPAPI GS_PARAM void LocalPoseFromWorldPoseNoScale(skeleton const* Skeleton,
                                                   local_pose *Result,
                                                   world_pose const &WorldPose,
                                                   real32 const *Offset4x4,
                                                   int32x FirstBone,
                                                   int32x BoneCount);

inline
EXPAPI GS_PARAM void ModulationCompositeLocalPose(local_pose &BasePose,
                                                  real32 WeightNone, real32 WeightAll, track_mask *Mask,
                                                  local_pose const &CompositePose)
{
    ModulationCompositeLocalPoseSparse ( BasePose, WeightNone, WeightAll, Mask, CompositePose, NULL );
}

EXPAPI GS_PARAM void CopyLocalPose(local_pose const &Src,local_pose &Dst);

// Used by the ResultSparseBoneArrayReverse to indicate that this bone
// is not present in the sparse local pose.
#define NoSparseBone -1 EXPMACRO

EXPAPI GS_READ int32x SparseBoneArrayCreateSingleBone(skeleton const &Skeleton,
                                                      int32x FirstBoneRequired,
                                                      int32x *ResultSparseBoneArray,
                                                      int32x *ResultSparseBoneArrayReverse);
EXPAPI GS_READ int32x SparseBoneArrayAddBone(skeleton const &Skeleton,
                                             int32x BoneToAdd, int32x InitialBoneCount,
                                             int32x *SparseBoneArray,
                                             int32x *SparseBoneArrayReverse);
EXPAPI GS_READ void SparseBoneArrayExpand(skeleton const &Skeleton,
                                          local_pose const &SourceSparseLocalPose,
                                          int32x SparseBoneCount,
                                          int32x const *SparseBoneArray, int32x const *SparseBoneArrayReverse,
                                          int32x BoneCount, local_pose &DestLocalPose);
EXPAPI GS_SAFE bool SparseBoneArrayIsValid(int32x SkeletonBoneCount,
                                           int32x SparseBoneCount,
                                           int32x const *SparseBoneArray,
                                           int32x const *SparseBoneArrayReverse);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_LOCAL_POSE_H
#endif
