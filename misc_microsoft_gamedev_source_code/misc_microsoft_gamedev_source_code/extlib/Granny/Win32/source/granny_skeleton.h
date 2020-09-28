#if !defined(GRANNY_SKELETON_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_skeleton.h $
// $DateTime: 2007/08/17 17:38:16 $
// $Change: 15766 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(SkeletonGroup);
#define NoParentBone -1 EXPMACRO

EXPTYPE enum skeleton_lod_type
{
    NoSkeletonLOD = 0x0,
    EstimatedLOD  = 0x1,
    MeasuredLOD   = 0x2,
};


//
// Bone
//
EXPTYPE struct bone
{
    char const *Name;

    int32 ParentIndex;
    transform LocalTransform;
    matrix_4x4 InverseWorld4x4;
    real32 LODError;

    // light and camera data moved into extended
    variant ExtendedData;
};
EXPCONST EXPGROUP(bone) extern data_type_definition BoneType[];

EXPTYPE struct skeleton
{
    char const *Name;

    int32 BoneCount;
    bone *Bones;

    int32 LODType;
};
EXPCONST EXPGROUP(skeleton) extern data_type_definition SkeletonType[];

EXPAPI GS_READ void BuildSkeletonRelativeTransform(int32x SourceTransformStride,
                                                   transform const *SourceTransforms,
                                                   int32x SourceParentStride,
                                                   int32 const *SourceParents,
                                                   int32x BoneIndex,
                                                   transform &Result);

// Note that this counts on the skeleton being ordered properly
EXPAPI GS_READ void BuildSkeletonRelativeTransforms(int32x SourceTransformStride,
                                                    transform const *SourceTransforms,
                                                    int32x SourceParentStride,
                                                    int32 const *SourceParents,
                                                    int32x Count,
                                                    int32x ResultStride,
                                                    transform *Results);

EXPAPI GS_READ bool FindBoneByName(skeleton const *Skeleton, char const *BoneName,
                                   int32x &BoneIndex);
EXPAPI GS_READ bool FindBoneByNameLowercase(skeleton const *Skeleton, char const *BoneName,
                                            int32x &BoneIndex);

EXPAPI GS_READ bool FindBoneByRule(skeleton const* Skeleton,
                                   char const *ProcessedBoneName,
                                   char const *BonePattern,
                                   int32x &BoneIndex);

EXPAPI GS_READ int GetBoneCountForLOD(skeleton const* Skeleton,
                                      real32          AllowedError);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SKELETON_H
#endif
