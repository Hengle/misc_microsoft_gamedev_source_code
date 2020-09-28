#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_model_instance.h $
// $DateTime: 2007/09/04 11:07:13 $
// $Change: 15884 $
// $Revision: #17 $
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

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ModelInstanceGroup);

struct model;
struct skeleton;
struct bone;
struct model_control_binding;
struct local_pose;
struct world_pose;
struct transform;

EXPTYPE struct model_instance;

struct model_instance
{
    model const* Model;

    // These cache pointers help cut down on DMAs in the SPU version...
    skeleton const* CachedSkeleton;
    bone const*     CachedBones;
    intaddrx        CachedBoneCount;

    model_control_binding BindingSentinel;

    model_instance* Next;
    model_instance* Prev;

    void* UserData[MaximumUserData];

    // For alignment purposes...
    void*       Reserved0;
    void*       Reserved1;
};
CompileAssert(IS_ALIGNED_16(SizeOf(model_instance)));

EXPAPI GS_MODIFY model_instance *InstantiateModel(model const &Model);
EXPAPI GS_MODIFY void FreeModelInstance(model_instance *ModelInstance);

EXPAPI GS_READ model &GetSourceModel(model_instance const &Model);
EXPAPI GS_READ skeleton *GetSourceSkeleton(model_instance const &Model);

model_control_binding &GetModelBindingSentinel(
    model_instance const &ModelInstance);

EXPAPI GS_MODIFY void SetModelClock(model_instance const &ModelInstance,
                                    real32 NewClock);
EXPAPI GS_MODIFY void FreeCompletedModelControls(model_instance const &ModelInstance);

EXPAPI GS_PARAM void AccumulateModelAnimationsLODSparse(model_instance const &ModelInstance,
                                                        int32x FirstBone, int32x BoneCount,
                                                        local_pose &Result, real32 AllowedError,
                                                        int32x const *SparseBoneArray);
EXPAPI GS_PARAM void SampleModelAnimationsLODSparse(model_instance const &ModelInstance,
                                                    int32x FirstBone, int32x BoneCount,
                                                    local_pose &Result, real32 AllowedError,
                                                    int32x const *SparseBoneArray);
EXPAPI GS_PARAM bool SampleSingleModelAnimationLODSparse(model_instance const &ModelInstance,
                                                         control const &Control,
                                                         int32x FirstBone, int32x BoneCount,
                                                         local_pose &Result, real32 AllowedError,
                                                         int32x const *SparseBoneArray);
EXPAPI GS_PARAM void SampleModelAnimationsAcceleratedLOD(model_instance const &ModelInstance,
                                                         int32x BoneCount, real32 const *Offset4x4,
                                                         local_pose &Scratch, world_pose &Result,
                                                         real32 AllowedError);

void GS_READ GetUnnormalizedRootMotionVectors(model_instance const &ModelInstance,
                                              real32 SecondsElapsed,
                                              real32 &TotalWeight,
                                              real32 *ResultTranslation,
                                              real32 *ResultRotation,
                                              bool Inverse);


inline
EXPAPI GS_PARAM void AccumulateModelAnimations(model_instance const &ModelInstance,
                                               int32x FirstBone, int32x BoneCount,
                                               local_pose &Result)
{
    AccumulateModelAnimationsLODSparse ( ModelInstance, FirstBone, BoneCount, Result, 0.0f, NULL );
}

inline
EXPAPI GS_PARAM void SampleModelAnimations(model_instance const &ModelInstance,
                                           int32x FirstBone, int32x BoneCount,
                                           local_pose &Result)
{
    SampleModelAnimationsLODSparse ( ModelInstance, FirstBone, BoneCount, Result, 0.0f, NULL );
}

inline
EXPAPI GS_PARAM bool SampleSingleModelAnimation(model_instance const &ModelInstance,
                                                control const &Control,
                                                int32x FirstBone, int32x BoneCount,
                                                local_pose &Result)
{
    return SampleSingleModelAnimationLODSparse ( ModelInstance, Control, FirstBone, BoneCount, Result, 0.0f, NULL );
}

inline
EXPAPI GS_PARAM void SampleModelAnimationsAccelerated(model_instance const &ModelInstance,
                                                      int32x BoneCount, real32 const *Offset4x4,
                                                      local_pose &Scratch, world_pose &Result)
{
    SampleModelAnimationsAcceleratedLOD ( ModelInstance, BoneCount, Offset4x4, Scratch, Result, 0.0f );
}


inline
EXPAPI GS_PARAM void AccumulateModelAnimationsLOD(model_instance const &ModelInstance,
                                                  int32x FirstBone, int32x BoneCount,
                                                  local_pose &Result, real32 AllowedError)
{
    AccumulateModelAnimationsLODSparse ( ModelInstance, FirstBone, BoneCount, Result, AllowedError, NULL );
}

inline
EXPAPI GS_PARAM void SampleModelAnimationsLOD(model_instance const &ModelInstance,
                                              int32x FirstBone, int32x BoneCount,
                                              local_pose &Result, real32 AllowedError)
{
    SampleModelAnimationsLODSparse ( ModelInstance, FirstBone, BoneCount, Result, AllowedError, NULL );
}

inline
EXPAPI GS_PARAM bool SampleSingleModelAnimationLOD(model_instance const &ModelInstance,
                                                   control const &Control,
                                                   int32x FirstBone, int32x BoneCount,
                                                   local_pose &Result, real32 AllowedError)
{
    return SampleSingleModelAnimationLODSparse ( ModelInstance, Control, FirstBone, BoneCount, Result, AllowedError, NULL );
}



EXPAPI GS_READ void GetRootMotionVectors(model_instance const &ModelInstance,
                                         real32 SecondsElapsed,
                                         real32 *ResultTranslation3,
                                         real32 *ResultRotation3,
                                         bool Inverse);
EXPAPI GS_SAFE void ClipRootMotionVectors(real32 const *Translation3,
                                  real32 const *Rotation3,
                                  uint32 AllowableDOFs,
                                  real32 *AllowedTranslation3,
                                  real32 *AllowedRotation3,
                                  real32 *DisallowedTranslation3,
                                  real32 *DisallowedRotation3);
EXPAPI GS_SAFE void ApplyRootMotionVectorsToMatrix(real32 const *ModelMatrix4x4,
                                                   real32 const *Translation3,
                                                   real32 const *Rotation3,
                                                   real32 *DestMatrix4x4);
EXPAPI GS_PARAM void ApplyRootMotionVectorsToLocalPose(local_pose &Pose,
                                                       real32 const *Translation3,
                                                       real32 const *Rotation3);

EXPAPI GS_READ void UpdateModelMatrix(model_instance const &ModelInstance,
                                      real32 SecondsElapsed,
                                      real32 const *ModelMatrix4x4,
                                      real32 *DestMatrix4x4,
                                      bool Inverse);

EXPAPI GS_READ void **GetModelUserDataArray(model_instance const &ModelInstance);

EXPAPI GS_READ model_instance *GetGlobalModelInstancesBegin(void);
EXPAPI GS_READ model_instance *GetGlobalModelInstancesEnd(void);
EXPAPI GS_READ model_instance *GetGlobalNextModelInstance(model_instance *Instance);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MODEL_INSTANCE_H
#endif
