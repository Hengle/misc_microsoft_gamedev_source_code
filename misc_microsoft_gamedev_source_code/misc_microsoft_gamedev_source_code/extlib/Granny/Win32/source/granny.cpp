// ========================================================================
// $File: //jeffr/granny/rt/dll_header.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#define BUILDING_GRANNY 1

#if !defined(GRANNY_H)
#include "granny.h"
#endif

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

BEGIN_GRANNY_NAMESPACE;
END_GRANNY_NAMESPACE;
USING_GRANNY_NAMESPACE;

#include "granny_transform.h"
GRANNY_DYNLINK(void) GrannySetTransform(granny_transform * Result,
                                        granny_real32 const * Position3,
                                        granny_real32 const * Orientation4,
                                        granny_real32 const * ScaleShear3x3)
{
    SetTransform((transform &)*Result,
                 (real32 const *)Position3,
                 (real32 const *)Orientation4,
                 (real32 const *)ScaleShear3x3);
}

GRANNY_DYNLINK(void) GrannySetTransformWithIdentityCheck(granny_transform * Result,
                                                         granny_real32 const * Position3,
                                                         granny_real32 const * Orientation4,
                                                         granny_real32 const * ScaleShear3x3)
{
    SetTransformWithIdentityCheck((transform &)*Result,
                                  (real32 const *)Position3,
                                  (real32 const *)Orientation4,
                                  (real32 const *)ScaleShear3x3);
}

GRANNY_DYNLINK(void) GrannyMakeIdentity(granny_transform * Result)
{
    MakeIdentity((transform &)*Result);
}

GRANNY_DYNLINK(void) GrannyZeroTransform(granny_transform * Result)
{
    ZeroTransform((transform &)*Result);
}

GRANNY_DYNLINK(granny_real32) GrannyGetTransformDeterminant(granny_transform const * Transform)
{
    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetTransformDeterminant((transform const &)*Transform));
}

GRANNY_DYNLINK(void) GrannyTransformVectorInPlace(granny_real32 * Result,
                                                  granny_transform const * Transform)
{
    TransformVectorInPlace((real32 *)Result,
                           (transform const &)*Transform);
}

GRANNY_DYNLINK(void) GrannyTransformVectorInPlaceTransposed(granny_real32 * Result,
                                                            granny_transform const * Transform)
{
    TransformVectorInPlaceTransposed((real32 *)Result,
                                     (transform const &)*Transform);
}

GRANNY_DYNLINK(void) GrannyTransformVector(granny_real32 * Dest,
                                           granny_transform const * Transform,
                                           granny_real32 const * Source)
{
    TransformVector((real32 *)Dest,
                    (transform const &)*Transform,
                    (real32 const *)Source);
}

GRANNY_DYNLINK(void) GrannyTransformPointInPlace(granny_real32 * Result,
                                                 granny_transform const * Transform)
{
    TransformPointInPlace((real32 *)Result,
                          (transform const &)*Transform);
}

GRANNY_DYNLINK(void) GrannyTransformPoint(granny_real32 * Dest,
                                          granny_transform const * Transform,
                                          granny_real32 const * Source)
{
    TransformPoint((real32 *)Dest,
                   (transform const &)*Transform,
                   (real32 const *)Source);
}

GRANNY_DYNLINK(void) GrannyPreMultiplyBy(granny_transform * Transform,
                                         granny_transform const * PreMult)
{
    PreMultiplyBy((transform &)*Transform,
                  (transform const &)*PreMult);
}

GRANNY_DYNLINK(void) GrannyPostMultiplyBy(granny_transform * Transform,
                                          granny_transform const * PostMult)
{
    PostMultiplyBy((transform &)*Transform,
                   (transform const &)*PostMult);
}

GRANNY_DYNLINK(void) GrannyMultiply(granny_transform * Result,
                                    granny_transform const * A,
                                    granny_transform const * B)
{
    Multiply((transform &)*Result,
             (transform const &)*A,
             (transform const &)*B);
}

GRANNY_DYNLINK(void) GrannyLinearBlendTransform(granny_transform * Result,
                                                granny_transform const * A,
                                                granny_real32 t,
                                                granny_transform const * B)
{
    LinearBlendTransform((transform &)*Result,
                         (transform const &)*A,
                         (real32)t,
                         (transform const &)*B);
}

GRANNY_DYNLINK(void) GrannyBuildInverse(granny_transform * Result,
                                        granny_transform const * Source)
{
    BuildInverse((transform &)*Result,
                 (transform const &)*Source);
}

GRANNY_DYNLINK(void) GrannySimilarityTransform(granny_transform * Result,
                                               granny_real32 const * Affine3,
                                               granny_real32 const * Linear3x3,
                                               granny_real32 const * InverseLinear3x3)
{
    SimilarityTransform((transform &)*Result,
                        (real32 const *)Affine3,
                        (real32 const *)Linear3x3,
                        (real32 const *)InverseLinear3x3);
}

GRANNY_DYNLINK(void) GrannyBuildCompositeTransform(granny_transform const * Transform,
                                                   granny_int32 Stride,
                                                   granny_real32 * Composite3x3)
{
    BuildCompositeTransform((transform const &)*Transform,
                            (int32)Stride,
                            (real32 *)Composite3x3);
}

GRANNY_DYNLINK(void) GrannyBuildCompositeTransform4x4(granny_transform const * Transform,
                                                      granny_real32 * Composite4x4)
{
    BuildCompositeTransform4x4((transform const &)*Transform,
                               (real32 *)Composite4x4);
}

GRANNY_DYNLINK(void) GrannyBuildCompositeTransform4x3(granny_transform const * Transform,
                                                      granny_real32 * Composite4x3)
{
    BuildCompositeTransform4x3((transform const &)*Transform,
                               (real32 *)Composite4x3);
}

#include "granny_model.h"
GRANNY_DYNLINK(void) GrannyGetModelInitialPlacement4x4(granny_model const * Model,
                                                       granny_real32 * Placement4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetModelInitialPlacement4x4((model const &)*Model,
                                (real32 *)Placement4x4);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyModelMeshBindingType = (granny_data_type_definition *)ModelMeshBindingType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyModelType = (granny_data_type_definition *)ModelType;
#include "granny_model_control_binding.h"
GRANNY_DYNLINK(granny_model_control_binding *) GrannyModelControlsBegin(granny_model_instance * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_control_binding *)&ModelControlsBegin((model_instance &)*Model));
}

GRANNY_DYNLINK(granny_model_control_binding *) GrannyModelControlsNext(granny_model_control_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_control_binding *)&ModelControlsNext((model_control_binding &)*Binding));
}

GRANNY_DYNLINK(granny_model_control_binding *) GrannyModelControlsEnd(granny_model_instance * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_control_binding *)&ModelControlsEnd((model_instance &)*Model));
}

GRANNY_DYNLINK(granny_model_control_binding *) GrannyControlModelsBegin(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_control_binding *)&ControlModelsBegin((control &)*Control));
}

GRANNY_DYNLINK(granny_model_control_binding *) GrannyControlModelsNext(granny_model_control_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_control_binding *)&ControlModelsNext((model_control_binding &)*Binding));
}

GRANNY_DYNLINK(granny_model_control_binding *) GrannyControlModelsEnd(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_control_binding *)&ControlModelsEnd((control &)*Control));
}

GRANNY_DYNLINK(granny_model_instance *) GrannyGetModelInstanceFromBinding(granny_model_control_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_instance *)GetModelInstanceFromBinding((model_control_binding &)*Binding));
}

GRANNY_DYNLINK(granny_control *) GrannyGetControlFromBinding(granny_model_control_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)GetControlFromBinding((model_control_binding &)*Binding));
}

#include "granny_model_instance.h"
GRANNY_DYNLINK(granny_model_instance *) GrannyInstantiateModel(granny_model const * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_instance *)InstantiateModel((model const &)*Model));
}

GRANNY_DYNLINK(void) GrannyFreeModelInstance(granny_model_instance * ModelInstance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeModelInstance((model_instance *)ModelInstance);
}

GRANNY_DYNLINK(granny_model *) GrannyGetSourceModel(granny_model_instance const * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model *)&GetSourceModel((model_instance const &)*Model));
}

GRANNY_DYNLINK(granny_skeleton *) GrannyGetSourceSkeleton(granny_model_instance const * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_skeleton *)GetSourceSkeleton((model_instance const &)*Model));
}

GRANNY_DYNLINK(void) GrannySetModelClock(granny_model_instance const * ModelInstance,
                                         granny_real32 NewClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetModelClock((model_instance const &)*ModelInstance,
                  (real32)NewClock);
}

GRANNY_DYNLINK(void) GrannyFreeCompletedModelControls(granny_model_instance const * ModelInstance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeCompletedModelControls((model_instance const &)*ModelInstance);
}

GRANNY_DYNLINK(void) GrannyAccumulateModelAnimationsLODSparse(granny_model_instance const * ModelInstance,
                                                              granny_int32x FirstBone,
                                                              granny_int32x BoneCount,
                                                              granny_local_pose * Result,
                                                              granny_real32 AllowedError,
                                                              granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AccumulateModelAnimationsLODSparse((model_instance const &)*ModelInstance,
                                       (int32x)FirstBone,
                                       (int32x)BoneCount,
                                       (local_pose &)*Result,
                                       (real32)AllowedError,
                                       (int32x const *)SparseBoneArray);
}

GRANNY_DYNLINK(void) GrannySampleModelAnimationsLODSparse(granny_model_instance const * ModelInstance,
                                                          granny_int32x FirstBone,
                                                          granny_int32x BoneCount,
                                                          granny_local_pose * Result,
                                                          granny_real32 AllowedError,
                                                          granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleModelAnimationsLODSparse((model_instance const &)*ModelInstance,
                                   (int32x)FirstBone,
                                   (int32x)BoneCount,
                                   (local_pose &)*Result,
                                   (real32)AllowedError,
                                   (int32x const *)SparseBoneArray);
}

GRANNY_DYNLINK(bool) GrannySampleSingleModelAnimationLODSparse(granny_model_instance const * ModelInstance,
                                                               granny_control const * Control,
                                                               granny_int32x FirstBone,
                                                               granny_int32x BoneCount,
                                                               granny_local_pose * Result,
                                                               granny_real32 AllowedError,
                                                               granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)SampleSingleModelAnimationLODSparse((model_instance const &)*ModelInstance,
                                                      (control const &)*Control,
                                                      (int32x)FirstBone,
                                                      (int32x)BoneCount,
                                                      (local_pose &)*Result,
                                                      (real32)AllowedError,
                                                      (int32x const *)SparseBoneArray));
}

GRANNY_DYNLINK(void) GrannySampleModelAnimationsAcceleratedLOD(granny_model_instance const * ModelInstance,
                                                               granny_int32x BoneCount,
                                                               granny_real32 const * Offset4x4,
                                                               granny_local_pose * Scratch,
                                                               granny_world_pose * Result,
                                                               granny_real32 AllowedError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleModelAnimationsAcceleratedLOD((model_instance const &)*ModelInstance,
                                        (int32x)BoneCount,
                                        (real32 const *)Offset4x4,
                                        (local_pose &)*Scratch,
                                        (world_pose &)*Result,
                                        (real32)AllowedError);
}

GRANNY_DYNLINK(void) GrannyAccumulateModelAnimations(granny_model_instance const * ModelInstance,
                                                     granny_int32x FirstBone,
                                                     granny_int32x BoneCount,
                                                     granny_local_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AccumulateModelAnimations((model_instance const &)*ModelInstance,
                              (int32x)FirstBone,
                              (int32x)BoneCount,
                              (local_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannySampleModelAnimations(granny_model_instance const * ModelInstance,
                                                 granny_int32x FirstBone,
                                                 granny_int32x BoneCount,
                                                 granny_local_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleModelAnimations((model_instance const &)*ModelInstance,
                          (int32x)FirstBone,
                          (int32x)BoneCount,
                          (local_pose &)*Result);
}

GRANNY_DYNLINK(bool) GrannySampleSingleModelAnimation(granny_model_instance const * ModelInstance,
                                                      granny_control const * Control,
                                                      granny_int32x FirstBone,
                                                      granny_int32x BoneCount,
                                                      granny_local_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)SampleSingleModelAnimation((model_instance const &)*ModelInstance,
                                             (control const &)*Control,
                                             (int32x)FirstBone,
                                             (int32x)BoneCount,
                                             (local_pose &)*Result));
}

GRANNY_DYNLINK(void) GrannySampleModelAnimationsAccelerated(granny_model_instance const * ModelInstance,
                                                            granny_int32x BoneCount,
                                                            granny_real32 const * Offset4x4,
                                                            granny_local_pose * Scratch,
                                                            granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleModelAnimationsAccelerated((model_instance const &)*ModelInstance,
                                     (int32x)BoneCount,
                                     (real32 const *)Offset4x4,
                                     (local_pose &)*Scratch,
                                     (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyAccumulateModelAnimationsLOD(granny_model_instance const * ModelInstance,
                                                        granny_int32x FirstBone,
                                                        granny_int32x BoneCount,
                                                        granny_local_pose * Result,
                                                        granny_real32 AllowedError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AccumulateModelAnimationsLOD((model_instance const &)*ModelInstance,
                                 (int32x)FirstBone,
                                 (int32x)BoneCount,
                                 (local_pose &)*Result,
                                 (real32)AllowedError);
}

GRANNY_DYNLINK(void) GrannySampleModelAnimationsLOD(granny_model_instance const * ModelInstance,
                                                    granny_int32x FirstBone,
                                                    granny_int32x BoneCount,
                                                    granny_local_pose * Result,
                                                    granny_real32 AllowedError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleModelAnimationsLOD((model_instance const &)*ModelInstance,
                             (int32x)FirstBone,
                             (int32x)BoneCount,
                             (local_pose &)*Result,
                             (real32)AllowedError);
}

GRANNY_DYNLINK(bool) GrannySampleSingleModelAnimationLOD(granny_model_instance const * ModelInstance,
                                                         granny_control const * Control,
                                                         granny_int32x FirstBone,
                                                         granny_int32x BoneCount,
                                                         granny_local_pose * Result,
                                                         granny_real32 AllowedError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)SampleSingleModelAnimationLOD((model_instance const &)*ModelInstance,
                                                (control const &)*Control,
                                                (int32x)FirstBone,
                                                (int32x)BoneCount,
                                                (local_pose &)*Result,
                                                (real32)AllowedError));
}

GRANNY_DYNLINK(void) GrannyGetRootMotionVectors(granny_model_instance const * ModelInstance,
                                                granny_real32 SecondsElapsed,
                                                granny_real32 * ResultTranslation3,
                                                granny_real32 * ResultRotation3,
                                                bool Inverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetRootMotionVectors((model_instance const &)*ModelInstance,
                         (real32)SecondsElapsed,
                         (real32 *)ResultTranslation3,
                         (real32 *)ResultRotation3,
                         (bool)Inverse);
}

GRANNY_DYNLINK(void) GrannyClipRootMotionVectors(granny_real32 const * Translation3,
                                                 granny_real32 const * Rotation3,
                                                 granny_uint32 AllowableDOFs,
                                                 granny_real32 * AllowedTranslation3,
                                                 granny_real32 * AllowedRotation3,
                                                 granny_real32 * DisallowedTranslation3,
                                                 granny_real32 * DisallowedRotation3)
{
    ClipRootMotionVectors((real32 const *)Translation3,
                          (real32 const *)Rotation3,
                          (uint32)AllowableDOFs,
                          (real32 *)AllowedTranslation3,
                          (real32 *)AllowedRotation3,
                          (real32 *)DisallowedTranslation3,
                          (real32 *)DisallowedRotation3);
}

GRANNY_DYNLINK(void) GrannyApplyRootMotionVectorsToMatrix(granny_real32 const * ModelMatrix4x4,
                                                          granny_real32 const * Translation3,
                                                          granny_real32 const * Rotation3,
                                                          granny_real32 * DestMatrix4x4)
{
    ApplyRootMotionVectorsToMatrix((real32 const *)ModelMatrix4x4,
                                   (real32 const *)Translation3,
                                   (real32 const *)Rotation3,
                                   (real32 *)DestMatrix4x4);
}

GRANNY_DYNLINK(void) GrannyApplyRootMotionVectorsToLocalPose(granny_local_pose * Pose,
                                                             granny_real32 const * Translation3,
                                                             granny_real32 const * Rotation3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ApplyRootMotionVectorsToLocalPose((local_pose &)*Pose,
                                      (real32 const *)Translation3,
                                      (real32 const *)Rotation3);
}

GRANNY_DYNLINK(void) GrannyUpdateModelMatrix(granny_model_instance const * ModelInstance,
                                             granny_real32 SecondsElapsed,
                                             granny_real32 const * ModelMatrix4x4,
                                             granny_real32 * DestMatrix4x4,
                                             bool Inverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    UpdateModelMatrix((model_instance const &)*ModelInstance,
                      (real32)SecondsElapsed,
                      (real32 const *)ModelMatrix4x4,
                      (real32 *)DestMatrix4x4,
                      (bool)Inverse);
}

GRANNY_DYNLINK(void **) GrannyGetModelUserDataArray(granny_model_instance const * ModelInstance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void **)GetModelUserDataArray((model_instance const &)*ModelInstance));
}

GRANNY_DYNLINK(granny_model_instance *) GrannyGetGlobalModelInstancesBegin(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_instance *)GetGlobalModelInstancesBegin());
}

GRANNY_DYNLINK(granny_model_instance *) GrannyGetGlobalModelInstancesEnd(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_instance *)GetGlobalModelInstancesEnd());
}

GRANNY_DYNLINK(granny_model_instance *) GrannyGetGlobalNextModelInstance(granny_model_instance * Instance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_instance *)GetGlobalNextModelInstance((model_instance *)Instance));
}

#include "granny_data_type_definition.h"
GRANNY_DYNLINK(granny_int32x) GrannyGetMemberUnitSize(granny_data_type_definition const * MemberType)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMemberUnitSize((data_type_definition const &)*MemberType));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMemberTypeSize(granny_data_type_definition const * MemberType)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMemberTypeSize((data_type_definition const &)*MemberType));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetTotalObjectSize(granny_data_type_definition const * TypeDefinition)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetTotalObjectSize((data_type_definition const *)TypeDefinition));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetTotalTypeSize(granny_data_type_definition const * TypeDefinition)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetTotalTypeSize((data_type_definition const *)TypeDefinition));
}

GRANNY_DYNLINK(char const *) GrannyGetMemberTypeName(granny_member_type Type)
{
    return ((char const *)GetMemberTypeName((member_type)Type));
}

GRANNY_DYNLINK(char const *) GrannyGetMemberCTypeName(granny_member_type Type)
{
    return ((char const *)GetMemberCTypeName((member_type)Type));
}

GRANNY_DYNLINK(bool) GrannyMemberHasPointers(granny_data_type_definition const * MemberType)
{
    return ((bool)MemberHasPointers((data_type_definition const &)*MemberType));
}

GRANNY_DYNLINK(bool) GrannyTypeHasPointers(granny_data_type_definition const * TypeDefinition)
{
    return ((bool)TypeHasPointers((data_type_definition const *)TypeDefinition));
}

GRANNY_DYNLINK(granny_uint32) GrannyGetMemberMarshalling(granny_data_type_definition const * MemberType)
{
    uint32 ReturnValue;
    return *(granny_uint32 *)&(ReturnValue = GetMemberMarshalling((data_type_definition const &)*MemberType));
}

GRANNY_DYNLINK(granny_uint32) GrannyGetObjectMarshalling(granny_data_type_definition const * TypeDefinition)
{
    uint32 ReturnValue;
    return *(granny_uint32 *)&(ReturnValue = GetObjectMarshalling((data_type_definition const *)TypeDefinition));
}

GRANNY_DYNLINK(bool) GrannyIsMixedMarshalling(granny_uint32x Marshalling)
{
    return ((bool)IsMixedMarshalling((uint32x)Marshalling));
}

GRANNY_DYNLINK(granny_intaddrx) GrannyMakeEmptyDataTypeMember(granny_data_type_definition const * MemberType,
                                                              void * Member)
{
    intaddrx ReturnValue;
    return *(granny_intaddrx *)&(ReturnValue = MakeEmptyDataTypeMember((data_type_definition const &)*MemberType,
                                                                       (void *)Member));
}

GRANNY_DYNLINK(granny_intaddrx) GrannyMakeEmptyDataTypeObject(granny_data_type_definition const * TypeDefinition,
                                                              void * Object)
{
    intaddrx ReturnValue;
    return *(granny_intaddrx *)&(ReturnValue = MakeEmptyDataTypeObject((data_type_definition const *)TypeDefinition,
                                                                       (void *)Object));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMemberArrayWidth(granny_data_type_definition const * MemberType)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMemberArrayWidth((data_type_definition const &)*MemberType));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetTypeTableCount(granny_data_type_definition const ** TypeTable)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetTypeTableCount((data_type_definition const **)TypeTable));
}

GRANNY_DYNLINK(bool) GrannyDataTypesAreEqual(granny_data_type_definition const * A,
                                             granny_data_type_definition const * B)
{
    return ((bool)DataTypesAreEqual((data_type_definition const *)A,
                                    (data_type_definition const *)B));
}

GRANNY_DYNLINK(bool) GrannyDataTypesAreEqualWithNames(granny_data_type_definition const * A,
                                                      granny_data_type_definition const * B)
{
    return ((bool)DataTypesAreEqualWithNames((data_type_definition const *)A,
                                             (data_type_definition const *)B));
}

GRANNY_DYNLINK(bool) GrannyDataTypesAreEqualWithNameCallback(granny_data_type_definition const * A,
                                                             granny_data_type_definition const * B,
                                                             granny_string_comparison_callback * Callback)
{
    return ((bool)DataTypesAreEqualWithNameCallback((data_type_definition const *)A,
                                                    (data_type_definition const *)B,
                                                    (string_comparison_callback *)Callback));
}

GRANNY_DYNLINK(granny_data_type_definition *) GrannyDataTypeBeginsWith(granny_data_type_definition const * Type,
                                                                       granny_data_type_definition const * Prefix)
{
    return ((granny_data_type_definition *)DataTypeBeginsWith((data_type_definition const *)Type,
                                                              (data_type_definition const *)Prefix));
}

GRANNY_DYNLINK(void) GrannyReverseTypeArray(granny_data_type_definition const * Type,
                                            granny_int32x Count,
                                            void * TypeArray)
{
    ReverseTypeArray((data_type_definition const *)Type,
                     (int32x)Count,
                     (void *)TypeArray);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyEmptyType = (granny_data_type_definition *)EmptyType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyStringType = (granny_data_type_definition *)StringType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyInt16Type = (granny_data_type_definition *)Int16Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyInt32Type = (granny_data_type_definition *)Int32Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyUInt8Type = (granny_data_type_definition *)UInt8Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyUInt16Type = (granny_data_type_definition *)UInt16Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyUInt32Type = (granny_data_type_definition *)UInt32Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyReal32Type = (granny_data_type_definition *)Real32Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTripleType = (granny_data_type_definition *)TripleType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyQuadType = (granny_data_type_definition *)QuadType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTransformType = (granny_data_type_definition *)TransformType;
#include "granny_curve.h"
GRANNY_DYNLINK(void) GrannyCurveInitializeFormat(granny_curve2 * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CurveInitializeFormat((curve2 *)Curve);
}

GRANNY_DYNLINK(bool) GrannyCurveFormatIsInitializedCorrectly(granny_curve2 const * Curve,
                                                             bool CheckTypes)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveFormatIsInitializedCorrectly((curve2 const &)*Curve,
                                                    (bool)CheckTypes));
}

GRANNY_DYNLINK(bool) GrannyCurveIsKeyframed(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveIsKeyframed((curve2 const &)*Curve));
}

GRANNY_DYNLINK(bool) GrannyCurveIsIdentity(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveIsIdentity((curve2 const &)*Curve));
}

GRANNY_DYNLINK(bool) GrannyCurveIsConstantOrIdentity(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveIsConstantOrIdentity((curve2 const &)*Curve));
}

GRANNY_DYNLINK(bool) GrannyCurveIsConstantNotIdentity(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveIsConstantNotIdentity((curve2 const &)*Curve));
}

GRANNY_DYNLINK(granny_int32x) GrannyCurveGetKnotCount(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = CurveGetKnotCount((curve2 const &)*Curve));
}

GRANNY_DYNLINK(granny_int32x) GrannyCurveGetDimension(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = CurveGetDimension((curve2 const &)*Curve));
}

GRANNY_DYNLINK(granny_int32x) GrannyCurveGetDegree(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = CurveGetDegree((curve2 const &)*Curve));
}

GRANNY_DYNLINK(granny_data_type_definition const *) GrannyCurveGetDataTypeDefinition(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_data_type_definition const *)CurveGetDataTypeDefinition((curve2 const &)*Curve));
}

GRANNY_DYNLINK(granny_curve_builder *) GrannyBeginCurve(granny_data_type_definition const * TypeDefinition,
                                                        granny_int32x Degree,
                                                        granny_int32x Dimension,
                                                        granny_int32x KnotCount)
{
    return ((granny_curve_builder *)BeginCurve((data_type_definition const *)TypeDefinition,
                                               (int32x)Degree,
                                               (int32x)Dimension,
                                               (int32x)KnotCount));
}

GRANNY_DYNLINK(granny_curve_builder *) GrannyBeginCurveCopy(granny_curve2 const * SourceCurve)
{
    return ((granny_curve_builder *)BeginCurveCopy((curve2 const &)*SourceCurve));
}

GRANNY_DYNLINK(void) GrannyPushCurveKnotArray(granny_curve_builder * Builder,
                                              granny_real32 const * KnotArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushCurveKnotArray((curve_builder *)Builder,
                       (real32 const *)KnotArray);
}

GRANNY_DYNLINK(void) GrannyPushCurveControlArray(granny_curve_builder * Builder,
                                                 granny_real32 const * ControlArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushCurveControlArray((curve_builder *)Builder,
                          (real32 const *)ControlArray);
}

GRANNY_DYNLINK(void) GrannyPushCurveSampleArrays(granny_curve_builder * Builder,
                                                 granny_int32x SampleCount,
                                                 granny_int32x Dimension,
                                                 granny_real32 const * TransformedSamples,
                                                 granny_real32 const * OriginalSamples)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushCurveSampleArrays((curve_builder *)Builder,
                          (int32x)SampleCount,
                          (int32x)Dimension,
                          (real32 const *)TransformedSamples,
                          (real32 const *)OriginalSamples);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingCurveSize(granny_curve_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingCurveSize((curve_builder const *)Builder));
}

GRANNY_DYNLINK(granny_curve2 *) GrannyEndCurveInPlace(granny_curve_builder * Builder,
                                                      void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 *)EndCurveInPlace((curve_builder *)Builder,
                                             (void *)Memory));
}

GRANNY_DYNLINK(granny_curve2 *) GrannyEndCurve(granny_curve_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 *)EndCurve((curve_builder *)Builder));
}

GRANNY_DYNLINK(void) GrannyAbortCurveBuilder(granny_curve_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AbortCurveBuilder((curve_builder *)Builder);
}

GRANNY_DYNLINK(void) GrannyFreeCurve(granny_curve2 * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeCurve((curve2 *)Curve);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingCurveDataSize(granny_curve_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingCurveDataSize((curve_builder const *)Builder));
}

GRANNY_DYNLINK(granny_curve2 *) GrannyEndCurveDataInPlace(granny_curve_builder * Builder,
                                                          granny_curve2 * Curve,
                                                          void * CurveDataMemory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 *)EndCurveDataInPlace((curve_builder *)Builder,
                                                 (curve2 *)Curve,
                                                 (void *)CurveDataMemory));
}

GRANNY_DYNLINK(bool) GrannyCurveIsTypeDaK32fC32f(granny_curve2 const * SrcCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveIsTypeDaK32fC32f((curve2 const &)*SrcCurve));
}

GRANNY_DYNLINK(granny_curve_data_da_k32f_c32f *) GrannyCurveGetContentsOfDaK32fC32f(granny_curve2 const * SrcCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve_data_da_k32f_c32f *)CurveGetContentsOfDaK32fC32f((curve2 const &)*SrcCurve));
}

GRANNY_DYNLINK(granny_curve2 *) GrannyCurveConvertToDaK32fC32f(granny_curve2 const * SrcCurve,
                                                               granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 *)CurveConvertToDaK32fC32f((curve2 const &)*SrcCurve,
                                                      (real32 const *)IdentityVector));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingDaK32fC32fCurveSize(granny_curve2 const * SrcCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingDaK32fC32fCurveSize((curve2 const &)*SrcCurve));
}

GRANNY_DYNLINK(granny_curve2 *) GrannyCurveConvertToDaK32fC32fInPlace(granny_curve2 const * SrcCurve,
                                                                      void * Memory,
                                                                      granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 *)CurveConvertToDaK32fC32fInPlace((curve2 const &)*SrcCurve,
                                                             (void *)Memory,
                                                             (real32 const *)IdentityVector));
}

GRANNY_DYNLINK(void) GrannyCurveMakeStaticDaK32fC32f(granny_curve2 * Curve,
                                                     granny_curve_data_da_k32f_c32f * CurveData,
                                                     granny_int32x KnotCount,
                                                     granny_int32x Degree,
                                                     granny_int32x Dimension,
                                                     granny_real32 const * Knots,
                                                     granny_real32 const * Controls)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CurveMakeStaticDaK32fC32f((curve2 *)Curve,
                              (curve_data_da_k32f_c32f *)CurveData,
                              (int32x)KnotCount,
                              (int32x)Degree,
                              (int32x)Dimension,
                              (real32 const *)Knots,
                              (real32 const *)Controls);
}

GRANNY_DYNLINK(granny_real32) GrannyCurveExtractKnotValue(granny_curve2 const * Curve,
                                                          granny_int32x KnotIndex,
                                                          granny_real32 * ControlResult,
                                                          granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = CurveExtractKnotValue((curve2 const &)*Curve,
                                                                   (int32x)KnotIndex,
                                                                   (real32 *)ControlResult,
                                                                   (real32 const *)IdentityVector));
}

GRANNY_DYNLINK(void) GrannyCurveExtractKnotValues(granny_curve2 const * Curve,
                                                  granny_int32x KnotIndexStart,
                                                  granny_int32x KnotCount,
                                                  granny_real32 * KnotResults,
                                                  granny_real32 * ControlResults,
                                                  granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CurveExtractKnotValues((curve2 const &)*Curve,
                           (int32x)KnotIndexStart,
                           (int32x)KnotCount,
                           (real32 *)KnotResults,
                           (real32 *)ControlResults,
                           (real32 const *)IdentityVector);
}

GRANNY_DYNLINK(void) GrannyCurveSetAllKnotValues(granny_curve2 * Curve,
                                                 granny_int32x KnotCount,
                                                 granny_int32x Dimension,
                                                 granny_real32 const * KnotSrc,
                                                 granny_real32 const * ControlSrc)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CurveSetAllKnotValues((curve2 *)Curve,
                          (int32x)KnotCount,
                          (int32x)Dimension,
                          (real32 const *)KnotSrc,
                          (real32 const *)ControlSrc);
}

GRANNY_DYNLINK(void) GrannyCurveScaleOffsetSwizzle(granny_curve2 * Curve,
                                                   granny_int32x Dimension,
                                                   granny_real32 const * Scales,
                                                   granny_real32 const * Offsets,
                                                   granny_int32x const * Swizzles)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CurveScaleOffsetSwizzle((curve2 *)Curve,
                            (int32x)Dimension,
                            (real32 const *)Scales,
                            (real32 const *)Offsets,
                            (int32x const *)Swizzles);
}

GRANNY_DYNLINK(granny_int32x) GrannyCurveFindKnot(granny_curve2 const * Curve,
                                                  granny_real32 t)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = CurveFindKnot((curve2 const &)*Curve,
                                                           (real32)t));
}

GRANNY_DYNLINK(granny_int32x) GrannyCurveFindCloseKnot(granny_curve2 const * Curve,
                                                       granny_real32 t,
                                                       granny_int32x StartingKnotIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = CurveFindCloseKnot((curve2 const &)*Curve,
                                                                (real32)t,
                                                                (int32x)StartingKnotIndex));
}

GRANNY_DYNLINK(granny_int32) GrannyCurveGetSize(granny_curve2 const * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32 ReturnValue;
    return *(granny_int32 *)&(ReturnValue = CurveGetSize((curve2 const &)*Curve));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyOldCurveType = (granny_data_type_definition *)OldCurveType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurve2Type = (granny_data_type_definition *)Curve2Type;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataHeaderType = (granny_data_type_definition *)CurveDataHeaderType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataDaKeyframes32fType = (granny_data_type_definition *)CurveDataDaKeyframes32fType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataDaK32fC32fType = (granny_data_type_definition *)CurveDataDaK32fC32fType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataDaK16uC16uType = (granny_data_type_definition *)CurveDataDaK16uC16uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataDaK8uC8uType = (granny_data_type_definition *)CurveDataDaK8uC8uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD3K16uC16uType = (granny_data_type_definition *)CurveDataD3K16uC16uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD3K8uC8uType = (granny_data_type_definition *)CurveDataD3K8uC8uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD4nK16uC15uType = (granny_data_type_definition *)CurveDataD4nK16uC15uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD4nK8uC7uType = (granny_data_type_definition *)CurveDataD4nK8uC7uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataDaIdentityType = (granny_data_type_definition *)CurveDataDaIdentityType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataDaConstant32fType = (granny_data_type_definition *)CurveDataDaConstant32fType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD3Constant32fType = (granny_data_type_definition *)CurveDataD3Constant32fType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD4Constant32fType = (granny_data_type_definition *)CurveDataD4Constant32fType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD9I1K16uC16uType = (granny_data_type_definition *)CurveDataD9I1K16uC16uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD9I3K16uC16uType = (granny_data_type_definition *)CurveDataD9I3K16uC16uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD9I1K8uC8uType = (granny_data_type_definition *)CurveDataD9I1K8uC8uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD9I3K8uC8uType = (granny_data_type_definition *)CurveDataD9I3K8uC8uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD3I1K32fC32fType = (granny_data_type_definition *)CurveDataD3I1K32fC32fType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD3I1K16uC16uType = (granny_data_type_definition *)CurveDataD3I1K16uC16uType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyCurveDataD3I1K8uC8uType = (granny_data_type_definition *)CurveDataD3I1K8uC8uType;
GRANNY_DYNLINKDATA(granny_real32 *) GrannyCurveIdentityPosition = (granny_real32 *)CurveIdentityPosition;
GRANNY_DYNLINKDATA(granny_real32 *) GrannyCurveIdentityNormal = (granny_real32 *)CurveIdentityNormal;
GRANNY_DYNLINKDATA(granny_real32 *) GrannyCurveIdentityOrientation = (granny_real32 *)CurveIdentityOrientation;
GRANNY_DYNLINKDATA(granny_real32 *) GrannyCurveIdentityScaleShear = (granny_real32 *)CurveIdentityScaleShear;
GRANNY_DYNLINKDATA(granny_real32 *) GrannyCurveIdentityScale = (granny_real32 *)CurveIdentityScale;
GRANNY_DYNLINKDATA(granny_real32 *) GrannyCurveIdentityShear = (granny_real32 *)CurveIdentityShear;
#include "granny_material.h"
GRANNY_DYNLINK(granny_texture *) GrannyGetMaterialTextureByType(granny_material const * Material,
                                                                granny_material_texture_type Type)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_texture *)GetMaterialTextureByType((material const *)Material,
                                                       (material_texture_type)Type));
}

GRANNY_DYNLINK(granny_material *) GrannyGetTexturedMaterialByChannelName(granny_material const * Material,
                                                                         char const * ChannelName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_material *)GetTexturedMaterialByChannelName((material const *)Material,
                                                                (char const *)ChannelName));
}

GRANNY_DYNLINK(granny_texture *) GrannyGetMaterialTextureByChannelName(granny_material const * Material,
                                                                       char const * ChannelName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_texture *)GetMaterialTextureByChannelName((material const *)Material,
                                                              (char const *)ChannelName));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyMaterialMapType = (granny_data_type_definition *)MaterialMapType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyMaterialType = (granny_data_type_definition *)MaterialType;
#include "granny_pixel_layout.h"
GRANNY_DYNLINK(bool) GrannyPixelLayoutsAreEqual(granny_pixel_layout const * Operand0,
                                                granny_pixel_layout const * Operand1)
{
    return ((bool)PixelLayoutsAreEqual((pixel_layout const &)*Operand0,
                                       (pixel_layout const &)*Operand1));
}

GRANNY_DYNLINK(bool) GrannyPixelLayoutHasAlpha(granny_pixel_layout const * Layout)
{
    return ((bool)PixelLayoutHasAlpha((pixel_layout const &)*Layout));
}

GRANNY_DYNLINK(void) GrannySetStockSpecification(granny_pixel_layout * Layout,
                                                 granny_int32 const * BitsForComponent,
                                                 granny_int32 const * ComponentPlacement)
{
    SetStockSpecification((pixel_layout &)*Layout,
                          (int32 const *)BitsForComponent,
                          (int32 const *)ComponentPlacement);
}

GRANNY_DYNLINK(void) GrannySetStockRGBASpecification(granny_pixel_layout * Layout,
                                                     granny_int32x RedBits,
                                                     granny_int32x GreenBits,
                                                     granny_int32x BlueBits,
                                                     granny_int32x AlphaBits)
{
    SetStockRGBASpecification((pixel_layout &)*Layout,
                              (int32x)RedBits,
                              (int32x)GreenBits,
                              (int32x)BlueBits,
                              (int32x)AlphaBits);
}

GRANNY_DYNLINK(void) GrannySetStockBGRASpecification(granny_pixel_layout * Layout,
                                                     granny_int32x RedBits,
                                                     granny_int32x GreenBits,
                                                     granny_int32x BlueBits,
                                                     granny_int32x AlphaBits)
{
    SetStockBGRASpecification((pixel_layout &)*Layout,
                              (int32x)RedBits,
                              (int32x)GreenBits,
                              (int32x)BlueBits,
                              (int32x)AlphaBits);
}

GRANNY_DYNLINK(void) GrannySwapRGBAToBGRA(granny_pixel_layout * Layout)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SwapRGBAToBGRA((pixel_layout &)*Layout);
}

GRANNY_DYNLINK(void) GrannyConvertPixelFormat(granny_int32x Width,
                                              granny_int32x Height,
                                              granny_pixel_layout const * SourceLayout,
                                              granny_int32x SourceStride,
                                              void const * SourceBits,
                                              granny_pixel_layout const * DestLayout,
                                              granny_int32x DestStride,
                                              void * DestBits)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ConvertPixelFormat((int32x)Width,
                       (int32x)Height,
                       (pixel_layout const &)*SourceLayout,
                       (int32x)SourceStride,
                       (void const *)SourceBits,
                       (pixel_layout const &)*DestLayout,
                       (int32x)DestStride,
                       (void *)DestBits);
}

GRANNY_DYNLINK(void) GrannyARGB8888SwizzleNGC(granny_uint32 Width,
                                              granny_uint32 Height,
                                              granny_uint32 SourceStride,
                                              void * SourceBits,
                                              void * DestBits)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ARGB8888SwizzleNGC((uint32)Width,
                       (uint32)Height,
                       (uint32)SourceStride,
                       (void *)SourceBits,
                       (void *)DestBits);
}

GRANNY_DYNLINK(void) GrannyAll16SwizzleNGC(granny_uint32 Width,
                                           granny_uint32 Height,
                                           granny_uint32 SourceStride,
                                           void * SourceBits,
                                           void * DestBits)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    All16SwizzleNGC((uint32)Width,
                    (uint32)Height,
                    (uint32)SourceStride,
                    (void *)SourceBits,
                    (void *)DestBits);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPixelLayoutType = (granny_data_type_definition *)PixelLayoutType;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyRGB555PixelFormat = &(granny_pixel_layout const  &)RGB555PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyRGB565PixelFormat = &(granny_pixel_layout const  &)RGB565PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyRGBA5551PixelFormat = &(granny_pixel_layout const  &)RGBA5551PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyRGBA4444PixelFormat = &(granny_pixel_layout const  &)RGBA4444PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyRGB888PixelFormat = &(granny_pixel_layout const  &)RGB888PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyRGBA8888PixelFormat = &(granny_pixel_layout const  &)RGBA8888PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyARGB8888PixelFormat = &(granny_pixel_layout const  &)ARGB8888PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyBGR555PixelFormat = &(granny_pixel_layout const  &)BGR555PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyBGR565PixelFormat = &(granny_pixel_layout const  &)BGR565PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyBGRA5551PixelFormat = &(granny_pixel_layout const  &)BGRA5551PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyBGRA4444PixelFormat = &(granny_pixel_layout const  &)BGRA4444PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyBGR888PixelFormat = &(granny_pixel_layout const  &)BGR888PixelFormat;
GRANNY_DYNLINKDATA(granny_pixel_layout const  *) GrannyBGRA8888PixelFormat = &(granny_pixel_layout const  &)BGRA8888PixelFormat;
#include "granny_animation.h"
GRANNY_DYNLINK(bool) GrannyFindTrackGroupForModel(granny_animation const * Animation,
                                                  char const * ModelName,
                                                  granny_int32x * TrackGroupIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindTrackGroupForModel((animation const &)*Animation,
                                         (char const *)ModelName,
                                         (int32x &)*TrackGroupIndex));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyAnimationType = (granny_data_type_definition *)AnimationType;
#include "granny_animation_binding.h"
GRANNY_DYNLINK(void) GrannyMakeDefaultAnimationBindingID(granny_animation_binding_identifier * ID,
                                                         granny_animation const * Animation,
                                                         granny_int32x TrackGroupIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    MakeDefaultAnimationBindingID((animation_binding_identifier &)*ID,
                                  (animation const *)Animation,
                                  (int32x)TrackGroupIndex);
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyAcquireAnimationBindingFromID(granny_animation_binding_identifier * ID)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)AcquireAnimationBindingFromID((animation_binding_identifier &)*ID));
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyAcquireAnimationBinding(granny_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)AcquireAnimationBinding((animation_binding *)Binding));
}

GRANNY_DYNLINK(void) GrannyReleaseAnimationBinding(granny_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ReleaseAnimationBinding((animation_binding *)Binding);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMaximumAnimationBindingCount(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMaximumAnimationBindingCount());
}

GRANNY_DYNLINK(void) GrannySetMaximumAnimationBindingCount(granny_int32x BindingCountMax)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetMaximumAnimationBindingCount((int32x)BindingCountMax);
}

GRANNY_DYNLINK(void) GrannyFlushAllUnusedAnimationBindings(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FlushAllUnusedAnimationBindings();
}

GRANNY_DYNLINK(void) GrannyFlushAllBindingsForAnimation(granny_animation const * Animation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FlushAllBindingsForAnimation((animation const *)Animation);
}

GRANNY_DYNLINK(void) GrannyFlushAnimationBinding(granny_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FlushAnimationBinding((animation_binding *)Binding);
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyGetFirstBindingForAnimation(granny_animation const * Animation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)GetFirstBindingForAnimation((animation const *)Animation));
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyGetNextBindingForAnimation(granny_animation const * Animation,
                                                                            granny_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)GetNextBindingForAnimation((animation const *)Animation,
                                                                   (animation_binding *)Binding));
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyGetFirstUnusedAnimationBinding(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)GetFirstUnusedAnimationBinding());
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyGetNextUnusedAnimationBinding(granny_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)GetNextUnusedAnimationBinding((animation_binding *)Binding));
}

GRANNY_DYNLINK(bool) GrannyIsAnimationUsed(granny_animation const * Animation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)IsAnimationUsed((animation const *)Animation));
}

GRANNY_DYNLINK(void) GrannyRemapAnimationBindingPointers(granny_animation_binding * Binding,
                                                         granny_animation const * NewAnimationPointer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RemapAnimationBindingPointers((animation_binding *)Binding,
                                  (animation const *)NewAnimationPointer);
}

GRANNY_DYNLINK(void) GrannyRemapAllAnimationBindingPointers(granny_animation const * OldAnimationPointer,
                                                            granny_animation const * NewAnimationPointer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RemapAllAnimationBindingPointers((animation const *)OldAnimationPointer,
                                     (animation const *)NewAnimationPointer);
}

GRANNY_DYNLINK(void) GrannyGetAnimationBindingCacheStatus(granny_animation_binding_cache_status * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetAnimationBindingCacheStatus((animation_binding_cache_status &)*Result);
}

GRANNY_DYNLINK(void) GrannyCalculateLODErrorValues(granny_animation_binding * Binding,
                                                   granny_mesh_binding const * MeshBinding,
                                                   bool AnimationHasScales,
                                                   granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CalculateLODErrorValues((animation_binding &)*Binding,
                            (mesh_binding const &)*MeshBinding,
                            (bool)AnimationHasScales,
                            (real32)ManualScaler);
}

GRANNY_DYNLINK(void) GrannyCalculateLODErrorValuesAllBindings(granny_model_instance * ModelInstance,
                                                              granny_mesh_binding const * MeshBinding,
                                                              bool AnimationHasScales,
                                                              granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CalculateLODErrorValuesAllBindings((model_instance &)*ModelInstance,
                                       (mesh_binding const &)*MeshBinding,
                                       (bool)AnimationHasScales,
                                       (real32)ManualScaler);
}

GRANNY_DYNLINK(granny_real32) GrannyGetLODErrorValue(granny_animation_binding const * AnimationBinding,
                                                     granny_int32x SkeletonBoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetLODErrorValue((animation_binding const &)*AnimationBinding,
                                                              (int32x)SkeletonBoneIndex));
}

GRANNY_DYNLINK(void) GrannySetLODErrorValue(granny_animation_binding * AnimationBinding,
                                            granny_int32x SkeletonBoneIndex,
                                            granny_real32 NewError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetLODErrorValue((animation_binding &)*AnimationBinding,
                     (int32x)SkeletonBoneIndex,
                     (real32)NewError);
}

GRANNY_DYNLINK(void) GrannyCopyLODErrorValuesToAnimation(granny_animation_binding * Binding,
                                                         granny_real32 ManualScaler,
                                                         bool OnlyCopyIfGreater)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyLODErrorValuesToAnimation((animation_binding &)*Binding,
                                  (real32)ManualScaler,
                                  (bool)OnlyCopyIfGreater);
}

GRANNY_DYNLINK(void) GrannyCopyLODErrorValuesToAllAnimations(granny_control * Control,
                                                             granny_real32 ManualScaler,
                                                             bool OnlyCopyIfGreater)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyLODErrorValuesToAllAnimations((control &)*Control,
                                      (real32)ManualScaler,
                                      (bool)OnlyCopyIfGreater);
}

GRANNY_DYNLINK(void) GrannyCopyLODErrorValuesFromAnimation(granny_animation_binding * Binding,
                                                           granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyLODErrorValuesFromAnimation((animation_binding &)*Binding,
                                    (real32)ManualScaler);
}

GRANNY_DYNLINK(void) GrannyCopyLODErrorValuesFromAllAnimations(granny_control * Control,
                                                               granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyLODErrorValuesFromAllAnimations((control &)*Control,
                                        (real32)ManualScaler);
}

GRANNY_DYNLINK(void) GrannySetAllLODErrorValues(granny_animation_binding * AnimationBinding,
                                                granny_real32 NewValue)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetAllLODErrorValues((animation_binding &)*AnimationBinding,
                         (real32)NewValue);
}

GRANNY_DYNLINK(void) GrannyResetLODErrorValues(granny_animation_binding * AnimationBinding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResetLODErrorValues((animation_binding &)*AnimationBinding);
}

GRANNY_DYNLINK(granny_animation_lod_builder *) GrannyCalculateAnimationLODBegin(granny_animation const * Animation,
                                                                                granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_lod_builder *)CalculateAnimationLODBegin((animation const &)*Animation,
                                                                       (real32)ManualScaler));
}

GRANNY_DYNLINK(void) GrannyCalculateAnimationLODAddMeshBinding(granny_animation_lod_builder * Builder,
                                                               granny_model const * Model,
                                                               granny_mesh_binding const * MeshBinding,
                                                               granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CalculateAnimationLODAddMeshBinding((animation_lod_builder *)Builder,
                                        (model const &)*Model,
                                        (mesh_binding const &)*MeshBinding,
                                        (real32)ManualScaler);
}

GRANNY_DYNLINK(void) GrannyCalculateAnimationLODEnd(granny_animation_lod_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CalculateAnimationLODEnd((animation_lod_builder *)Builder);
}

GRANNY_DYNLINK(void) GrannyCalculateAnimationLODCleanup(granny_animation_lod_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CalculateAnimationLODCleanup((animation_lod_builder *)Builder);
}

#include "granny_art_tool_info.h"
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyArtToolInfoType = (granny_data_type_definition *)ArtToolInfoType;
#include "granny_basis_conversion.h"
GRANNY_DYNLINK(bool) GrannyComputeBasisConversion(granny_file_info const * FileInfo,
                                                  granny_real32 DesiredUnitsPerMeter,
                                                  granny_real32 const * DesiredOrigin3,
                                                  granny_real32 const * DesiredRight3,
                                                  granny_real32 const * DesiredUp3,
                                                  granny_real32 const * DesiredBack3,
                                                  granny_real32 * ResultAffine3,
                                                  granny_real32 * ResultLinear3x3,
                                                  granny_real32 * ResultInverseLinear3x3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ComputeBasisConversion((file_info const &)*FileInfo,
                                         (real32)DesiredUnitsPerMeter,
                                         (real32 const *)DesiredOrigin3,
                                         (real32 const *)DesiredRight3,
                                         (real32 const *)DesiredUp3,
                                         (real32 const *)DesiredBack3,
                                         (real32 *)ResultAffine3,
                                         (real32 *)ResultLinear3x3,
                                         (real32 *)ResultInverseLinear3x3));
}

GRANNY_DYNLINK(void) GrannyTransformMesh(granny_mesh * Mesh,
                                         granny_real32 const * Affine3,
                                         granny_real32 const * Linear3x3,
                                         granny_real32 const * InverseLinear3x3,
                                         granny_real32 AffineTolerance,
                                         granny_real32 LinearTolerance,
                                         granny_uint32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformMesh((mesh &)*Mesh,
                  (real32 const *)Affine3,
                  (real32 const *)Linear3x3,
                  (real32 const *)InverseLinear3x3,
                  (real32)AffineTolerance,
                  (real32)LinearTolerance,
                  (uint32x)Flags);
}

GRANNY_DYNLINK(void) GrannyTransformSkeleton(granny_skeleton * Skeleton,
                                             granny_real32 const * Affine3,
                                             granny_real32 const * Linear3x3,
                                             granny_real32 const * InverseLinear3x3,
                                             granny_real32 AffineTolerance,
                                             granny_real32 LinearTolerance,
                                             granny_uint32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformSkeleton((skeleton &)*Skeleton,
                      (real32 const *)Affine3,
                      (real32 const *)Linear3x3,
                      (real32 const *)InverseLinear3x3,
                      (real32)AffineTolerance,
                      (real32)LinearTolerance,
                      (uint32x)Flags);
}

GRANNY_DYNLINK(void) GrannyTransformModel(granny_model * Model,
                                          granny_real32 const * Affine3,
                                          granny_real32 const * Linear3x3,
                                          granny_real32 const * InverseLinear3x3,
                                          granny_real32 AffineTolerance,
                                          granny_real32 LinearTolerance,
                                          granny_uint32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformModel((model &)*Model,
                   (real32 const *)Affine3,
                   (real32 const *)Linear3x3,
                   (real32 const *)InverseLinear3x3,
                   (real32)AffineTolerance,
                   (real32)LinearTolerance,
                   (uint32x)Flags);
}

GRANNY_DYNLINK(void) GrannyTransformAnimation(granny_animation * Animation,
                                              granny_real32 const * Affine3,
                                              granny_real32 const * Linear3x3,
                                              granny_real32 const * InverseLinear3x3,
                                              granny_real32 AffineTolerance,
                                              granny_real32 LinearTolerance,
                                              granny_uint32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformAnimation((animation &)*Animation,
                       (real32 const *)Affine3,
                       (real32 const *)Linear3x3,
                       (real32 const *)InverseLinear3x3,
                       (real32)AffineTolerance,
                       (real32)LinearTolerance,
                       (uint32x)Flags);
}

GRANNY_DYNLINK(void) GrannyTransformFile(granny_file_info * FileInfo,
                                         granny_real32 const * Affine3,
                                         granny_real32 const * Linear3x3,
                                         granny_real32 const * InverseLinear3x3,
                                         granny_real32 AffineTolerance,
                                         granny_real32 LinearTolerance,
                                         granny_uint32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformFile((file_info &)*FileInfo,
                  (real32 const *)Affine3,
                  (real32 const *)Linear3x3,
                  (real32 const *)InverseLinear3x3,
                  (real32)AffineTolerance,
                  (real32)LinearTolerance,
                  (uint32x)Flags);
}

GRANNY_DYNLINK(void) GrannyResortAllAnimationTrackGroups(granny_animation * Animation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResortAllAnimationTrackGroups((animation &)*Animation);
}

GRANNY_DYNLINK(void) GrannyResortAllFileTrackGroups(granny_file_info * FileInfo)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResortAllFileTrackGroups((file_info &)*FileInfo);
}

#include "granny_bink.h"
GRANNY_DYNLINK(granny_pixel_layout const *) GrannyGetBinkPixelLayout(bool Alpha)
{
    return ((granny_pixel_layout const *)&GetBinkPixelLayout((bool)Alpha));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMaximumBinkImageSize(granny_int32x Width,
                                                            granny_int32x Height,
                                                            granny_uint32x Flags,
                                                            granny_int32x Compression)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMaximumBinkImageSize((int32x)Width,
                                                                     (int32x)Height,
                                                                     (uint32x)Flags,
                                                                     (int32x)Compression));
}

GRANNY_DYNLINK(granny_int32x) GrannyBinkCompressTexture(granny_int32x Width,
                                                        granny_int32x Height,
                                                        granny_int32x SourceStride,
                                                        void const * Source,
                                                        granny_uint32x Flags,
                                                        granny_int32x Compression,
                                                        void * Dest)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = BinkCompressTexture((int32x)Width,
                                                                 (int32x)Height,
                                                                 (int32x)SourceStride,
                                                                 (void const *)Source,
                                                                 (uint32x)Flags,
                                                                 (int32x)Compression,
                                                                 (void *)Dest));
}

GRANNY_DYNLINK(void) GrannyBinkDecompressTexture(granny_int32x Width,
                                                 granny_int32x Height,
                                                 granny_uint32x Flags,
                                                 granny_int32x SourceSize,
                                                 void const * Source,
                                                 granny_pixel_layout const * DestLayout,
                                                 granny_int32x DestStride,
                                                 void * Dest)
{
    BinkDecompressTexture((int32x)Width,
                          (int32x)Height,
                          (uint32x)Flags,
                          (int32x)SourceSize,
                          (void const *)Source,
                          (pixel_layout const &)*DestLayout,
                          (int32x)DestStride,
                          (void *)Dest);
}

#include "granny_bspline.h"
GRANNY_DYNLINK(void) GrannySampleBSpline0x1(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline0x1((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline0x2(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline0x2((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline0x3(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline0x3((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline0x4(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline0x4((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline0x9(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline0x9((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x1(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline1x1((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x2(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline1x2((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x3(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline1x3((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x3n(granny_real32 const * ti,
                                             granny_real32 const * pi,
                                             granny_real32 t,
                                             granny_real32 * Result)
{
    SampleBSpline1x3n((real32 const *)ti,
                      (real32 const *)pi,
                      (real32)t,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x4(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline1x4((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x4n(granny_real32 const * ti,
                                             granny_real32 const * pi,
                                             granny_real32 t,
                                             granny_real32 * Result)
{
    SampleBSpline1x4n((real32 const *)ti,
                      (real32 const *)pi,
                      (real32)t,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline1x9(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline1x9((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x1(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline2x1((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x2(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline2x2((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x3(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline2x3((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x3n(granny_real32 const * ti,
                                             granny_real32 const * pi,
                                             granny_real32 t,
                                             granny_real32 * Result)
{
    SampleBSpline2x3n((real32 const *)ti,
                      (real32 const *)pi,
                      (real32)t,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x4(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline2x4((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x4n(granny_real32 const * ti,
                                             granny_real32 const * pi,
                                             granny_real32 t,
                                             granny_real32 * Result)
{
    SampleBSpline2x4n((real32 const *)ti,
                      (real32 const *)pi,
                      (real32)t,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline2x9(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline2x9((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x1(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline3x1((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x2(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline3x2((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x3(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline3x3((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x3n(granny_real32 const * ti,
                                             granny_real32 const * pi,
                                             granny_real32 t,
                                             granny_real32 * Result)
{
    SampleBSpline3x3n((real32 const *)ti,
                      (real32 const *)pi,
                      (real32)t,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x4(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline3x4((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x4n(granny_real32 const * ti,
                                             granny_real32 const * pi,
                                             granny_real32 t,
                                             granny_real32 * Result)
{
    SampleBSpline3x4n((real32 const *)ti,
                      (real32 const *)pi,
                      (real32)t,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline3x9(granny_real32 const * ti,
                                            granny_real32 const * pi,
                                            granny_real32 t,
                                            granny_real32 * Result)
{
    SampleBSpline3x9((real32 const *)ti,
                     (real32 const *)pi,
                     (real32)t,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannySampleBSpline(granny_int32x Degree,
                                         granny_int32x Dimension,
                                         bool Normalize,
                                         granny_real32 const * ti,
                                         granny_real32 const * pi,
                                         granny_real32 t,
                                         granny_real32 * Result)
{
    SampleBSpline((int32x)Degree,
                  (int32x)Dimension,
                  (bool)Normalize,
                  (real32 const *)ti,
                  (real32 const *)pi,
                  (real32)t,
                  (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyUncheckedSampleBSpline(granny_int32x Degree,
                                                  granny_int32x Dimension,
                                                  granny_real32 const * ti,
                                                  granny_real32 const * pi,
                                                  granny_real32 t,
                                                  granny_real32 * Result)
{
    UncheckedSampleBSpline((int32x)Degree,
                           (int32x)Dimension,
                           (real32 const *)ti,
                           (real32 const *)pi,
                           (real32)t,
                           (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyUncheckedSampleBSplineN(granny_int32x Degree,
                                                   granny_int32x Dimension,
                                                   granny_real32 const * ti,
                                                   granny_real32 const * pi,
                                                   granny_real32 t,
                                                   granny_real32 * Result)
{
    UncheckedSampleBSplineN((int32x)Degree,
                            (int32x)Dimension,
                            (real32 const *)ti,
                            (real32 const *)pi,
                            (real32)t,
                            (real32 *)Result);
}

#include "granny_bspline_buffers.h"
GRANNY_DYNLINK(bool) GrannyConstructBSplineBuffers(granny_int32x Dimension,
                                                   granny_curve2 const * PrevCurve,
                                                   granny_curve2 const * Curve,
                                                   granny_curve2 const * NextCurve,
                                                   granny_real32 PrevCurveDuration,
                                                   granny_real32 CurveDuration,
                                                   granny_real32 NextCurveDuration,
                                                   granny_int32x KnotIndex,
                                                   granny_real32 * ti,
                                                   granny_real32 * pi,
                                                   granny_real32 ** tiPtr,
                                                   granny_real32 ** piPtr,
                                                   granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ConstructBSplineBuffers((int32x)Dimension,
                                          (curve2 const *)PrevCurve,
                                          (curve2 const &)*Curve,
                                          (curve2 const *)NextCurve,
                                          (real32)PrevCurveDuration,
                                          (real32)CurveDuration,
                                          (real32)NextCurveDuration,
                                          (int32x)KnotIndex,
                                          (real32 *)ti,
                                          (real32 *)pi,
                                          (real32 *&)*tiPtr,
                                          (real32 *&)*piPtr,
                                          (real32 const *)IdentityVector));
}

GRANNY_DYNLINK(void) GrannyEvaluateCurveAtT(granny_int32x Dimension,
                                            bool Normalize,
                                            bool BackwardsLoop,
                                            granny_curve2 const * Curve,
                                            bool ForwardsLoop,
                                            granny_real32 CurveDuration,
                                            granny_real32 t,
                                            granny_real32 * Result,
                                            granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EvaluateCurveAtT((int32x)Dimension,
                     (bool)Normalize,
                     (bool)BackwardsLoop,
                     (curve2 const &)*Curve,
                     (bool)ForwardsLoop,
                     (real32)CurveDuration,
                     (real32)t,
                     (real32 *)Result,
                     (real32 const *)IdentityVector);
}

GRANNY_DYNLINK(void) GrannyEvaluateCurveAtKnotIndex(granny_int32x Dimension,
                                                    bool Normalize,
                                                    bool BackwardsLoop,
                                                    granny_curve2 const * Curve,
                                                    bool ForwardsLoop,
                                                    granny_real32 CurveDuration,
                                                    granny_int32x KnotIndex,
                                                    granny_real32 t,
                                                    granny_real32 * Result,
                                                    granny_real32 const * IdentityVector)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EvaluateCurveAtKnotIndex((int32x)Dimension,
                             (bool)Normalize,
                             (bool)BackwardsLoop,
                             (curve2 const &)*Curve,
                             (bool)ForwardsLoop,
                             (real32)CurveDuration,
                             (int32x)KnotIndex,
                             (real32)t,
                             (real32 *)Result,
                             (real32 const *)IdentityVector);
}

#include "granny_bspline_solver.h"
GRANNY_DYNLINK(granny_int32x) GrannyGetMaximumKnotCountForSampleCount(granny_int32x MaxDegree,
                                                                      granny_int32x MaxSampleCount)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMaximumKnotCountForSampleCount((int32x)MaxDegree,
                                                                               (int32x)MaxSampleCount));
}

GRANNY_DYNLINK(granny_bspline_solver *) GrannyAllocateBSplineSolver(granny_int32x MaxDegree,
                                                                    granny_int32x MaxSampleCount,
                                                                    granny_int32x MaxDimension)
{
    return ((granny_bspline_solver *)AllocateBSplineSolver((int32x)MaxDegree,
                                                           (int32x)MaxSampleCount,
                                                           (int32x)MaxDimension));
}

GRANNY_DYNLINK(void) GrannyDeallocateBSplineSolver(granny_bspline_solver * Solver)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    DeallocateBSplineSolver((bspline_solver *)Solver);
}

GRANNY_DYNLINK(granny_curve2 * ) GrannyFitBSplineToSamples(granny_bspline_solver * Solver,
                                                           granny_uint32x SolverFlags,
                                                           granny_int32x Degree,
                                                           granny_real32 ErrorThreshold,
                                                           granny_real32 C0Threshold,
                                                           granny_real32 C1Threshold,
                                                           granny_real32 const * Samples,
                                                           granny_int32x Dimension,
                                                           granny_int32x SampleCount,
                                                           granny_real32 dT,
                                                           granny_data_type_definition const * CurveDataType,
                                                           granny_int32x MaximumCurveSizeInBytes,
                                                           bool * AchievedTolerance,
                                                           granny_int32x * CurveSizeInBytes)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 * )FitBSplineToSamples((bspline_solver &)*Solver,
                                                  (uint32x)SolverFlags,
                                                  (int32x)Degree,
                                                  (real32)ErrorThreshold,
                                                  (real32)C0Threshold,
                                                  (real32)C1Threshold,
                                                  (real32 const *)Samples,
                                                  (int32x)Dimension,
                                                  (int32x)SampleCount,
                                                  (real32)dT,
                                                  (data_type_definition const *)CurveDataType,
                                                  (int32x)MaximumCurveSizeInBytes,
                                                  (bool *)AchievedTolerance,
                                                  (int32x *)CurveSizeInBytes));
}

GRANNY_DYNLINK(granny_real32) GrannyOrientationToleranceFromDegrees(granny_real32 Degrees)
{
    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = OrientationToleranceFromDegrees((real32)Degrees));
}

GRANNY_DYNLINK(granny_real32) GrannyDegreesFromOrientationTolerance(granny_real32 Tolerance)
{
    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = DegreesFromOrientationTolerance((real32)Tolerance));
}

GRANNY_DYNLINK(void) GrannyGetSquaredErrorOverCurve(granny_int32x Flags,
                                                    granny_int32x Degree,
                                                    granny_int32x Dimension,
                                                    granny_real32 Duration,
                                                    granny_curve2 const * Curve,
                                                    granny_int32x SampleCount,
                                                    granny_real32 const * Samples,
                                                    granny_real32 const * IdentityVector,
                                                    granny_bspline_error * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetSquaredErrorOverCurve((int32x)Flags,
                             (int32x)Degree,
                             (int32x)Dimension,
                             (real32)Duration,
                             (curve2 const *)Curve,
                             (int32x)SampleCount,
                             (real32 const *)Samples,
                             (real32 const *)IdentityVector,
                             (bspline_error *)Result);
}

#include "granny_camera.h"
GRANNY_DYNLINK(void) GrannyInitializeDefaultCamera(granny_camera * Camera)
{
    InitializeDefaultCamera((camera &)*Camera);
}

GRANNY_DYNLINK(void) GrannySetCameraAspectRatios(granny_camera * Camera,
                                                 granny_real32 PhysicalAspectRatio,
                                                 granny_real32 ScreenWidth,
                                                 granny_real32 ScreenHeight,
                                                 granny_real32 WindowWidth,
                                                 granny_real32 WindowHeight)
{
    SetCameraAspectRatios((camera &)*Camera,
                          (real32)PhysicalAspectRatio,
                          (real32)ScreenWidth,
                          (real32)ScreenHeight,
                          (real32)WindowWidth,
                          (real32)WindowHeight);
}

GRANNY_DYNLINK(void) GrannyMoveCameraRelative(granny_camera * Camera,
                                              granny_real32 X,
                                              granny_real32 Y,
                                              granny_real32 Z)
{
    MoveCameraRelative((camera &)*Camera,
                       (real32)X,
                       (real32)Y,
                       (real32)Z);
}

GRANNY_DYNLINK(void) GrannyBuildCameraMatrices(granny_camera * Camera)
{
    BuildCameraMatrices((camera &)*Camera);
}

GRANNY_DYNLINK(void) GrannyGetCameraLocation(granny_camera const * Camera,
                                             granny_real32 * Result)
{
    GetCameraLocation((camera const &)*Camera,
                      (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetCameraLeft(granny_camera const * Camera,
                                         granny_real32 * Result)
{
    GetCameraLeft((camera const &)*Camera,
                  (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetCameraRight(granny_camera const * Camera,
                                          granny_real32 * Result)
{
    GetCameraRight((camera const &)*Camera,
                   (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetCameraUp(granny_camera const * Camera,
                                       granny_real32 * Result)
{
    GetCameraUp((camera const &)*Camera,
                (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetCameraDown(granny_camera const * Camera,
                                         granny_real32 * Result)
{
    GetCameraDown((camera const &)*Camera,
                  (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetCameraForward(granny_camera const * Camera,
                                            granny_real32 * Result)
{
    GetCameraForward((camera const &)*Camera,
                     (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetCameraBack(granny_camera const * Camera,
                                         granny_real32 * Result)
{
    GetCameraBack((camera const &)*Camera,
                  (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyWindowSpaceToWorldSpace(granny_camera const * Camera,
                                                   granny_real32 Width,
                                                   granny_real32 Height,
                                                   granny_real32 const * ScreenSpacePoint,
                                                   granny_real32 * Result)
{
    WindowSpaceToWorldSpace((camera const &)*Camera,
                            (real32)Width,
                            (real32)Height,
                            (real32 const *)ScreenSpacePoint,
                            (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyWorldSpaceToWindowSpace(granny_camera const * Camera,
                                                   granny_real32 Width,
                                                   granny_real32 Height,
                                                   granny_real32 const * WorldSpacePoint,
                                                   granny_real32 * Result)
{
    WorldSpaceToWindowSpace((camera const &)*Camera,
                            (real32)Width,
                            (real32)Height,
                            (real32 const *)WorldSpacePoint,
                            (real32 *)Result);
}

GRANNY_DYNLINK(void) GrannyGetPickingRay(granny_camera const * Camera,
                                         granny_real32 Width,
                                         granny_real32 Height,
                                         granny_real32 MouseX,
                                         granny_real32 MouseY,
                                         granny_real32 * Origin,
                                         granny_real32 * Normal)
{
    GetPickingRay((camera const &)*Camera,
                  (real32)Width,
                  (real32)Height,
                  (real32)MouseX,
                  (real32)MouseY,
                  (real32 *)Origin,
                  (real32 *)Normal);
}

GRANNY_DYNLINK(void) GrannyGetCameraRelativePlanarBases(granny_camera const * Camera,
                                                        bool ScreenOrthogonal,
                                                        granny_real32 const * PlaneNormal,
                                                        granny_real32 const * PointOnPlane,
                                                        granny_real32 * XBasis,
                                                        granny_real32 * YBasis)
{
    GetCameraRelativePlanarBases((camera const &)*Camera,
                                 (bool)ScreenOrthogonal,
                                 (real32 const *)PlaneNormal,
                                 (real32 const *)PointOnPlane,
                                 (real32 *)XBasis,
                                 (real32 *)YBasis);
}

GRANNY_DYNLINK(granny_real32) GrannyGetMostLikelyPhysicalAspectRatio(granny_int32x ScreenWidth,
                                                                     granny_int32x ScreenHeight)
{
    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetMostLikelyPhysicalAspectRatio((int32x)ScreenWidth,
                                                                              (int32x)ScreenHeight));
}

GRANNY_DYNLINK(granny_real32) GrannyFindAllowedLODError(granny_real32 ErrorInPixels,
                                                        granny_int32x ViewportHeightInPixels,
                                                        granny_real32 CameraFOVY,
                                                        granny_real32 DistanceFromCamera)
{
    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = FindAllowedLODError((real32)ErrorInPixels,
                                                                 (int32x)ViewportHeightInPixels,
                                                                 (real32)CameraFOVY,
                                                                 (real32)DistanceFromCamera));
}

#include "granny_compress_curve.h"
GRANNY_DYNLINK(granny_curve2 *) GrannyCompressCurve(granny_bspline_solver * Solver,
                                                    granny_uint32x SolverFlags,
                                                    granny_compress_curve_parameters const * Params,
                                                    granny_real32 const * Samples,
                                                    granny_int32x Dimension,
                                                    granny_int32x FrameCount,
                                                    granny_real32 dT,
                                                    bool * CurveAcheivedTolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_curve2 *)CompressCurve((bspline_solver &)*Solver,
                                           (uint32x)SolverFlags,
                                           (compress_curve_parameters const &)*Params,
                                           (real32 const *)Samples,
                                           (int32x)Dimension,
                                           (int32x)FrameCount,
                                           (real32)dT,
                                           (bool *)CurveAcheivedTolerance));
}

#include "granny_control.h"
GRANNY_DYNLINK(granny_control *) GrannyCreateControl(granny_real32 CurrentClock,
                                                     granny_real32 LocalDuration)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)CreateControl((real32)CurrentClock,
                                            (real32)LocalDuration));
}

GRANNY_DYNLINK(void) GrannyFreeControl(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeControl((control *)Control);
}

GRANNY_DYNLINK(void) GrannyFreeControlOnceUnused(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeControlOnceUnused((control &)*Control);
}

GRANNY_DYNLINK(void) GrannyCompleteControlAt(granny_control * Control,
                                             granny_real32 AtSeconds)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CompleteControlAt((control &)*Control,
                      (real32)AtSeconds);
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlCompletionClock(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlCompletionClock((control const &)*Control));
}

GRANNY_DYNLINK(bool) GrannyGetControlCompletionCheckFlag(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)GetControlCompletionCheckFlag((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlCompletionCheckFlag(granny_control * Control,
                                                         bool CheckComplete)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlCompletionCheckFlag((control &)*Control,
                                  (bool)CheckComplete);
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlClock(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlClock((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlClock(granny_control * Control,
                                           granny_real32 NewSeconds)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlClock((control &)*Control,
                    (real32)NewSeconds);
}

GRANNY_DYNLINK(void) GrannySetControlClockOnly(granny_control * Control,
                                               granny_real32 NewSeconds)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlClockOnly((control &)*Control,
                        (real32)NewSeconds);
}

GRANNY_DYNLINK(bool) GrannyControlIsComplete(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ControlIsComplete((control const &)*Control));
}

GRANNY_DYNLINK(bool) GrannyFreeControlIfComplete(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FreeControlIfComplete((control *)Control));
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlWeight(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlWeight((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlWeight(granny_control * Control,
                                            granny_real32 Weight)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlWeight((control &)*Control,
                     (real32)Weight);
}

GRANNY_DYNLINK(granny_track_mask const* ) GrannyGetControlTrackGroupModelMask(granny_control const * Control,
                                                                              granny_model_instance const*  ModelInstance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_mask const* )GetControlTrackGroupModelMask((control const &)*Control,
                                                                     (model_instance const* )ModelInstance));
}

GRANNY_DYNLINK(granny_track_mask const* ) GrannyGetControlTrackGroupTrackMask(granny_control const * Control,
                                                                              granny_animation const*  Animation,
                                                                              granny_int32x TrackGroupIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_mask const* )GetControlTrackGroupTrackMask((control const &)*Control,
                                                                     (animation const* )Animation,
                                                                     (int32x)TrackGroupIndex));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetControlLoopCount(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetControlLoopCount((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlLoopCount(granny_control * Control,
                                               granny_int32x LoopCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlLoopCount((control &)*Control,
                        (int32x)LoopCount);
}

GRANNY_DYNLINK(void) GrannyGetControlLoopState(granny_control * Control,
                                               bool * UnderflowLoop,
                                               bool * OverflowLoop)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetControlLoopState((control &)*Control,
                        (bool &)*UnderflowLoop,
                        (bool &)*OverflowLoop);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetControlLoopIndex(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetControlLoopIndex((control &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlLoopIndex(granny_control * Control,
                                               granny_int32x LoopIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlLoopIndex((control &)*Control,
                        (int32x)LoopIndex);
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlSpeed(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlSpeed((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlSpeed(granny_control * Control,
                                           granny_real32 Speed)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlSpeed((control &)*Control,
                    (real32)Speed);
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlDuration(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlDuration((control const &)*Control));
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlDurationLeft(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlDurationLeft((control &)*Control));
}

GRANNY_DYNLINK(bool) GrannyControlIsActive(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ControlIsActive((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlActive(granny_control * Control,
                                            bool Active)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlActive((control &)*Control,
                     (bool)Active);
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlClampedLocalClock(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlClampedLocalClock((control &)*Control));
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlLocalDuration(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlLocalDuration((control const &)*Control));
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlEaseCurveMultiplier(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlEaseCurveMultiplier((control const &)*Control));
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlEffectiveWeight(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlEffectiveWeight((control const &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlEaseIn(granny_control * Control,
                                            bool EaseIn)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlEaseIn((control &)*Control,
                     (bool)EaseIn);
}

GRANNY_DYNLINK(void) GrannySetControlEaseInCurve(granny_control * Control,
                                                 granny_real32 StartSeconds,
                                                 granny_real32 EndSeconds,
                                                 granny_real32 StartValue,
                                                 granny_real32 StartTangent,
                                                 granny_real32 EndTangent,
                                                 granny_real32 EndValue)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlEaseInCurve((control &)*Control,
                          (real32)StartSeconds,
                          (real32)EndSeconds,
                          (real32)StartValue,
                          (real32)StartTangent,
                          (real32)EndTangent,
                          (real32)EndValue);
}

GRANNY_DYNLINK(void) GrannySetControlEaseOut(granny_control * Control,
                                             bool EaseOut)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlEaseOut((control &)*Control,
                      (bool)EaseOut);
}

GRANNY_DYNLINK(void) GrannySetControlEaseOutCurve(granny_control * Control,
                                                  granny_real32 StartSeconds,
                                                  granny_real32 EndSeconds,
                                                  granny_real32 StartValue,
                                                  granny_real32 StartTangent,
                                                  granny_real32 EndTangent,
                                                  granny_real32 EndValue)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlEaseOutCurve((control &)*Control,
                           (real32)StartSeconds,
                           (real32)EndSeconds,
                           (real32)StartValue,
                           (real32)StartTangent,
                           (real32)EndTangent,
                           (real32)EndValue);
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlRawLocalClock(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetControlRawLocalClock((control &)*Control));
}

GRANNY_DYNLINK(void) GrannySetControlRawLocalClock(granny_control * Control,
                                                   granny_real32 LocalClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlRawLocalClock((control &)*Control,
                            (real32)LocalClock);
}

GRANNY_DYNLINK(granny_real32) GrannyEaseControlIn(granny_control * Control,
                                                  granny_real32 Duration,
                                                  bool FromCurrent)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = EaseControlIn((control &)*Control,
                                                           (real32)Duration,
                                                           (bool)FromCurrent));
}

GRANNY_DYNLINK(granny_real32) GrannyEaseControlOut(granny_control * Control,
                                                   granny_real32 Duration)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = EaseControlOut((control &)*Control,
                                                            (real32)Duration));
}

GRANNY_DYNLINK(void **) GrannyGetControlUserDataArray(granny_control const * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void **)GetControlUserDataArray((control const &)*Control));
}

GRANNY_DYNLINK(granny_control *) GrannyGetGlobalControlsBegin(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)GetGlobalControlsBegin());
}

GRANNY_DYNLINK(granny_control *) GrannyGetGlobalControlsEnd(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)GetGlobalControlsEnd());
}

GRANNY_DYNLINK(granny_control *) GrannyGetGlobalNextControl(granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)GetGlobalNextControl((control *)Control));
}

GRANNY_DYNLINK(void) GrannyRecenterControlClocks(granny_control * Control,
                                                 granny_real32 dCurrentClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RecenterControlClocks((control *)Control,
                          (real32)dCurrentClock);
}

GRANNY_DYNLINK(void) GrannyRecenterAllControlClocks(granny_real32 dCurrentClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RecenterAllControlClocks((real32)dCurrentClock);
}

GRANNY_DYNLINK(void) GrannyRecenterAllModelInstanceControlClocks(granny_model_instance * ModelInstance,
                                                                 granny_real32 dCurrentClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RecenterAllModelInstanceControlClocks((model_instance *)ModelInstance,
                                          (real32)dCurrentClock);
}

GRANNY_DYNLINK(void) GrannySetControlForceClampedLooping(granny_control * Control,
                                                         bool Clamp)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlForceClampedLooping((control &)*Control,
                                  (bool)Clamp);
}

GRANNY_DYNLINK(void) GrannySetControlTargetState(granny_control * Control,
                                                 granny_real32 CurrentGlobalTime,
                                                 granny_real32 TargetGlobalTime,
                                                 granny_real32 LocalTime,
                                                 granny_int32x LoopIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetControlTargetState((control &)*Control,
                          (real32)CurrentGlobalTime,
                          (real32)TargetGlobalTime,
                          (real32)LocalTime,
                          (int32x)LoopIndex);
}

#include "granny_controlled_animation.h"
GRANNY_DYNLINK(granny_control *) GrannyPlayControlledAnimation(granny_real32 StartTime,
                                                               granny_animation const * Animation,
                                                               granny_model_instance * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)PlayControlledAnimation((real32)StartTime,
                                                      (animation const &)*Animation,
                                                      (model_instance &)*Model));
}

GRANNY_DYNLINK(granny_control *) GrannyPlayControlledAnimationBinding(granny_real32 StartTime,
                                                                      granny_animation const * Animation,
                                                                      granny_animation_binding * Binding,
                                                                      granny_model_instance * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)PlayControlledAnimationBinding((real32)StartTime,
                                                             (animation const &)*Animation,
                                                             (animation_binding &)*Binding,
                                                             (model_instance &)*Model));
}

GRANNY_DYNLINK(granny_controlled_animation_builder *) GrannyBeginControlledAnimation(granny_real32 StartTime,
                                                                                     granny_animation const * Animation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_controlled_animation_builder *)BeginControlledAnimation((real32)StartTime,
                                                                            (animation const &)*Animation));
}

GRANNY_DYNLINK(granny_control *) GrannyEndControlledAnimation(granny_controlled_animation_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)EndControlledAnimation((controlled_animation_builder *)Builder));
}

GRANNY_DYNLINK(void) GrannyUseExistingControlForAnimation(granny_controlled_animation_builder * Builder,
                                                          granny_control * Control)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    UseExistingControlForAnimation((controlled_animation_builder *)Builder,
                                   (control *)Control);
}

GRANNY_DYNLINK(void) GrannySetTrackMatchRule(granny_controlled_animation_builder * Builder,
                                             granny_int32x TrackGroupIndex,
                                             char const * TrackPattern,
                                             char const * BonePattern)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackMatchRule((controlled_animation_builder &)*Builder,
                      (int32x)TrackGroupIndex,
                      (char const *)TrackPattern,
                      (char const *)BonePattern);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupTarget(granny_controlled_animation_builder * Builder,
                                               granny_int32x TrackGroupIndex,
                                               granny_model_instance * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupTarget((controlled_animation_builder &)*Builder,
                        (int32x)TrackGroupIndex,
                        (model_instance &)*Model);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupBinding(granny_controlled_animation_builder * Builder,
                                                granny_int32x TrackGroupIndex,
                                                granny_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupBinding((controlled_animation_builder &)*Builder,
                         (int32x)TrackGroupIndex,
                         (animation_binding &)*Binding);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupBasisTransform(granny_controlled_animation_builder * Builder,
                                                       granny_int32x TrackGroupIndex,
                                                       granny_model * FromModel,
                                                       granny_model * ToModel)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupBasisTransform((controlled_animation_builder &)*Builder,
                                (int32x)TrackGroupIndex,
                                (model &)*FromModel,
                                (model &)*ToModel);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupTrackMask(granny_controlled_animation_builder * Builder,
                                                  granny_int32x TrackGroupIndex,
                                                  granny_track_mask * TrackMask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupTrackMask((controlled_animation_builder &)*Builder,
                           (int32x)TrackGroupIndex,
                           (track_mask &)*TrackMask);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupModelMask(granny_controlled_animation_builder * Builder,
                                                  granny_int32x TrackGroupIndex,
                                                  granny_track_mask * ModelMask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupModelMask((controlled_animation_builder &)*Builder,
                           (int32x)TrackGroupIndex,
                           (track_mask &)*ModelMask);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupAccumulation(granny_controlled_animation_builder * Builder,
                                                     granny_int32x TrackGroupIndex,
                                                     granny_accumulation_mode Mode)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupAccumulation((controlled_animation_builder &)*Builder,
                              (int32x)TrackGroupIndex,
                              (accumulation_mode)Mode);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupLOD(granny_controlled_animation_builder * Builder,
                                            granny_int32x TrackGroupIndex,
                                            bool CopyValuesFromAnimation,
                                            granny_real32 ManualScaler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupLOD((controlled_animation_builder &)*Builder,
                     (int32x)TrackGroupIndex,
                     (bool)CopyValuesFromAnimation,
                     (real32)ManualScaler);
}

GRANNY_DYNLINK(granny_animation_binding *) GrannyGetAnimationBindingFromControlBinding(granny_model_control_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_animation_binding *)GetAnimationBindingFromControlBinding((model_control_binding &)*Binding));
}

GRANNY_DYNLINK(granny_real32) GrannyGetGlobalLODFadingFactor(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetGlobalLODFadingFactor());
}

GRANNY_DYNLINK(void) GrannySetGlobalLODFadingFactor(granny_real32 NewValue)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetGlobalLODFadingFactor((real32)NewValue);
}

#include "granny_controlled_pose.h"
GRANNY_DYNLINK(granny_control *) GrannyPlayControlledPose(granny_real32 StartTime,
                                                          granny_real32 Duration,
                                                          granny_local_pose const * Pose,
                                                          granny_model_instance * Model,
                                                          granny_track_mask * ModelMask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)PlayControlledPose((real32)StartTime,
                                                 (real32)Duration,
                                                 (local_pose const &)*Pose,
                                                 (model_instance &)*Model,
                                                 (track_mask *)ModelMask));
}

GRANNY_DYNLINK(granny_local_pose *) GrannyGetLocalPoseFromControlBinding(granny_model_control_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)GetLocalPoseFromControlBinding((model_control_binding &)*Binding));
}

#include "granny_crc.h"
GRANNY_DYNLINK(void) GrannyBeginCRC32(granny_uint32 * CRC)
{
    BeginCRC32((uint32 &)*CRC);
}

GRANNY_DYNLINK(void) GrannyAddToCRC32(granny_uint32 * CRC,
                                      granny_uint32 Count,
                                      void const * UInt8s)
{
    AddToCRC32((uint32 &)*CRC,
               (uint32)Count,
               (void const *)UInt8s);
}

GRANNY_DYNLINK(void) GrannyEndCRC32(granny_uint32 * CRC)
{
    EndCRC32((uint32 &)*CRC);
}

#include "granny_dag.h"
GRANNY_DYNLINK(bool) GrannyIsBlendDagLeafType(granny_blend_dag_node const * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)IsBlendDagLeafType((blend_dag_node const *)Node));
}

GRANNY_DYNLINK(granny_blend_dag_node_type) GrannyGetBlendDagNodeType(granny_blend_dag_node const * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    blend_dag_node_type ReturnValue;
    return *(granny_blend_dag_node_type *)&(ReturnValue = GetBlendDagNodeType((blend_dag_node const *)Node));
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyCreateBlendDagNodeAnimationBlend(granny_model_instance * ModelInstance,
                                                                               bool AutoFree)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)CreateBlendDagNodeAnimationBlend((model_instance *)ModelInstance,
                                                                      (bool)AutoFree));
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyCreateBlendDagNodeLocalPose(granny_local_pose * LocalPose,
                                                                          bool AutoFree)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)CreateBlendDagNodeLocalPose((local_pose *)LocalPose,
                                                                 (bool)AutoFree));
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyCreateBlendDagNodeCallback(granny_blend_dag_leaf_callback_sample_callback * SampleCallback,
                                                                         granny_blend_dag_leaf_callback_set_clock_callback * SetClockCallback,
                                                                         granny_blend_dag_leaf_callback_motion_vectors_callback * MotionVectorsCallback,
                                                                         void * UserData)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)CreateBlendDagNodeCallback((blend_dag_leaf_callback_sample_callback *)SampleCallback,
                                                                (blend_dag_leaf_callback_set_clock_callback *)SetClockCallback,
                                                                (blend_dag_leaf_callback_motion_vectors_callback *)MotionVectorsCallback,
                                                                (void *)UserData));
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyCreateBlendDagNodeCrossfade(granny_blend_dag_node * DagNode0,
                                                                          granny_blend_dag_node * DagNode1,
                                                                          granny_real32 WeightNone,
                                                                          granny_real32 WeightAll,
                                                                          granny_track_mask * TrackMask,
                                                                          bool AutoFreeTrackMask,
                                                                          granny_int32x BoneCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)CreateBlendDagNodeCrossfade((blend_dag_node *)DagNode0,
                                                                 (blend_dag_node *)DagNode1,
                                                                 (real32)WeightNone,
                                                                 (real32)WeightAll,
                                                                 (track_mask *)TrackMask,
                                                                 (bool)AutoFreeTrackMask,
                                                                 (int32x)BoneCount));
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyCreateBlendDagNodeWeightedBlend(granny_skeleton * ReferenceSkeleton,
                                                                              granny_real32 FillThreshold,
                                                                              granny_quaternion_mode QuaternionBlendingMode,
                                                                              granny_int32x BoneCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)CreateBlendDagNodeWeightedBlend((skeleton *)ReferenceSkeleton,
                                                                     (real32)FillThreshold,
                                                                     (quaternion_mode)QuaternionBlendingMode,
                                                                     (int32x)BoneCount));
}

GRANNY_DYNLINK(void) GrannyFreeBlendDagNode(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeBlendDagNode((blend_dag_node *)Node);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeAnimationBlend(granny_blend_dag_node * Node,
                                                         granny_model_instance * ModelInstance,
                                                         granny_real32 FillThreshold,
                                                         bool AutoFree)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeAnimationBlend((blend_dag_node *)Node,
                                  (model_instance *)ModelInstance,
                                  (real32)FillThreshold,
                                  (bool)AutoFree);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeLocalPose(granny_blend_dag_node * Node,
                                                    granny_local_pose * LocalPose,
                                                    bool AutoFree)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeLocalPose((blend_dag_node *)Node,
                             (local_pose *)LocalPose,
                             (bool)AutoFree);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeCallbacks(granny_blend_dag_node * Node,
                                                    granny_blend_dag_leaf_callback_sample_callback * SampleCallback,
                                                    granny_blend_dag_leaf_callback_set_clock_callback * SetClockCallback,
                                                    granny_blend_dag_leaf_callback_motion_vectors_callback * MotionVectorsCallback,
                                                    void * UserData)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeCallbacks((blend_dag_node *)Node,
                             (blend_dag_leaf_callback_sample_callback *)SampleCallback,
                             (blend_dag_leaf_callback_set_clock_callback *)SetClockCallback,
                             (blend_dag_leaf_callback_motion_vectors_callback *)MotionVectorsCallback,
                             (void *)UserData);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeCrossfade(granny_blend_dag_node * Node,
                                                    granny_real32 WeightNone,
                                                    granny_real32 WeightAll,
                                                    granny_track_mask * TrackMask,
                                                    bool AutoFreeTrackMask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeCrossfade((blend_dag_node *)Node,
                             (real32)WeightNone,
                             (real32)WeightAll,
                             (track_mask *)TrackMask,
                             (bool)AutoFreeTrackMask);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeCrossfadeWeights(granny_blend_dag_node * Node,
                                                           granny_real32 WeightNone,
                                                           granny_real32 WeightAll)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeCrossfadeWeights((blend_dag_node *)Node,
                                    (real32)WeightNone,
                                    (real32)WeightAll);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeWeightedBlend(granny_blend_dag_node * Node,
                                                        granny_skeleton * ReferenceSkeleton,
                                                        granny_real32 FillThreshold,
                                                        granny_quaternion_mode QuaternionBlendingMode)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeWeightedBlend((blend_dag_node *)Node,
                                 (skeleton *)ReferenceSkeleton,
                                 (real32)FillThreshold,
                                 (quaternion_mode)QuaternionBlendingMode);
}

GRANNY_DYNLINK(granny_model_instance *) GrannyGetBlendDagNodeAnimationBlend(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_model_instance *)GetBlendDagNodeAnimationBlend((blend_dag_node *)Node));
}

GRANNY_DYNLINK(granny_local_pose *) GrannyGetBlendDagNodeLocalPose(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)GetBlendDagNodeLocalPose((blend_dag_node *)Node));
}

GRANNY_DYNLINK(granny_blend_dag_leaf_callback_sample_callback *) GrannyGetBlendDagNodeCallbackSampleCallback(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_leaf_callback_sample_callback *)GetBlendDagNodeCallbackSampleCallback((blend_dag_node *)Node));
}

GRANNY_DYNLINK(granny_blend_dag_leaf_callback_set_clock_callback *) GrannyGetBlendDagNodeCallbackSetClockCallback(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_leaf_callback_set_clock_callback *)GetBlendDagNodeCallbackSetClockCallback((blend_dag_node *)Node));
}

GRANNY_DYNLINK(granny_blend_dag_leaf_callback_motion_vectors_callback *) GrannyGetBlendDagNodeCallbackMotionVectorsCallback(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_leaf_callback_motion_vectors_callback *)GetBlendDagNodeCallbackMotionVectorsCallback((blend_dag_node *)Node));
}

GRANNY_DYNLINK(void *) GrannyGetBlendDagNodeCallbackUserData(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)GetBlendDagNodeCallbackUserData((blend_dag_node *)Node));
}

GRANNY_DYNLINK(granny_track_mask *) GrannyGetBlendDagNodeCrossfadeTrackMask(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_mask *)GetBlendDagNodeCrossfadeTrackMask((blend_dag_node *)Node));
}

GRANNY_DYNLINK(bool) GrannyGetBlendDagNodeCrossfadeWeights(granny_blend_dag_node const * Node,
                                                           granny_real32 *  WeightNone,
                                                           granny_real32 *  WeightAll)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)GetBlendDagNodeCrossfadeWeights((blend_dag_node const *)Node,
                                                  (real32 * )WeightNone,
                                                  (real32 * )WeightAll));
}

GRANNY_DYNLINK(granny_skeleton *) GrannyGetBlendDagNodeWeightedBlendSkeleton(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_skeleton *)GetBlendDagNodeWeightedBlendSkeleton((blend_dag_node *)Node));
}

GRANNY_DYNLINK(void) GrannyBlendDagNodeAnimationBlendFreeCompletedControls(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BlendDagNodeAnimationBlendFreeCompletedControls((blend_dag_node *)Node);
}

GRANNY_DYNLINK(void) GrannyClearBlendDagNodeChildren(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ClearBlendDagNodeChildren((blend_dag_node *)Node);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetBlendDagNodeChildrenCount(granny_blend_dag_node const * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetBlendDagNodeChildrenCount((blend_dag_node const *)Node));
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeChildren(granny_blend_dag_node * Node,
                                                   granny_int32x NumChildren,
                                                   granny_blend_dag_node ** ArrayOfChildren)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeChildren((blend_dag_node *)Node,
                            (int32x)NumChildren,
                            (blend_dag_node **)ArrayOfChildren);
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeChild(granny_blend_dag_node * Node,
                                                granny_int32x ChildNumber,
                                                granny_blend_dag_node * Child)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeChild((blend_dag_node *)Node,
                         (int32x)ChildNumber,
                         (blend_dag_node *)Child);
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyGetBlendDagNodeChild(granny_blend_dag_node * Node,
                                                                   granny_int32x ChildNumber)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)GetBlendDagNodeChild((blend_dag_node *)Node,
                                                          (int32x)ChildNumber));
}

GRANNY_DYNLINK(void) GrannyAddBlendDagNodeChild(granny_blend_dag_node * Node,
                                                granny_blend_dag_node * Child)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddBlendDagNodeChild((blend_dag_node *)Node,
                         (blend_dag_node *)Child);
}

GRANNY_DYNLINK(void) GrannyRemoveBlendDagNodeChild(granny_blend_dag_node * Node,
                                                   granny_blend_dag_node * Child)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RemoveBlendDagNodeChild((blend_dag_node *)Node,
                            (blend_dag_node *)Child);
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyGetBlendDagNodeParent(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)GetBlendDagNodeParent((blend_dag_node *)Node));
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeResultTrackMask(granny_blend_dag_node * Node,
                                                          granny_track_mask * TrackMask,
                                                          bool AutoFree)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeResultTrackMask((blend_dag_node *)Node,
                                   (track_mask *)TrackMask,
                                   (bool)AutoFree);
}

GRANNY_DYNLINK(granny_track_mask *) GrannyGetBlendDagNodeResultTrackMask(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_mask *)GetBlendDagNodeResultTrackMask((blend_dag_node *)Node));
}

GRANNY_DYNLINK(void) GrannySetBlendDagNodeResultWeight(granny_blend_dag_node * Node,
                                                       granny_real32 Weight)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagNodeResultWeight((blend_dag_node *)Node,
                                (real32)Weight);
}

GRANNY_DYNLINK(granny_real32) GrannyGetBlendDagNodeResultWeight(granny_blend_dag_node const * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetBlendDagNodeResultWeight((blend_dag_node const *)Node));
}

GRANNY_DYNLINK(void) GrannyGetBlendDagTreeMotionVectors(granny_blend_dag_node const * Node,
                                                        granny_real32 SecondsElapsed,
                                                        granny_real32 * ResultTranslation,
                                                        granny_real32 * ResultRotation,
                                                        bool Inverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetBlendDagTreeMotionVectors((blend_dag_node const *)Node,
                                 (real32)SecondsElapsed,
                                 (real32 *)ResultTranslation,
                                 (real32 *)ResultRotation,
                                 (bool)Inverse);
}

GRANNY_DYNLINK(void) GrannyUpdateBlendDagTreeMatrix(granny_blend_dag_node const * Node,
                                                    granny_real32 SecondsElapsed,
                                                    granny_real32 const * ModelMatrix4x4,
                                                    granny_real32 * DestMatrix4x4,
                                                    bool Inverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    UpdateBlendDagTreeMatrix((blend_dag_node const *)Node,
                             (real32)SecondsElapsed,
                             (real32 const *)ModelMatrix4x4,
                             (real32 *)DestMatrix4x4,
                             (bool)Inverse);
}

GRANNY_DYNLINK(void) GrannyPrimeBlendDagLocalPoseCache(granny_int32x MaxNumBones,
                                                       granny_int32x MaxTreeDepth)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PrimeBlendDagLocalPoseCache((int32x)MaxNumBones,
                                (int32x)MaxTreeDepth);
}

GRANNY_DYNLINK(void) GrannyFreeBlendDagLocalPoseCache(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeBlendDagLocalPoseCache();
}

GRANNY_DYNLINK(granny_dag_pose_cache *) GrannyCreateDagPoseCache(granny_int32x InitNumBones,
                                                                 granny_int32x InitTreeDepth)
{
    return ((granny_dag_pose_cache *)CreateDagPoseCache((int32x)InitNumBones,
                                                        (int32x)InitTreeDepth));
}

GRANNY_DYNLINK(void) GrannyFreeDagPoseCache(granny_dag_pose_cache * Cache)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeDagPoseCache((dag_pose_cache *)Cache);
}

GRANNY_DYNLINK(granny_local_pose *) GrannySampleBlendDagTreeLODSparseReentrant(granny_blend_dag_node const * RootNode,
                                                                               granny_int32x BoneCount,
                                                                               granny_real32 AllowedError,
                                                                               granny_int32x const * SparseBoneArray,
                                                                               granny_dag_pose_cache * PoseCache)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)SampleBlendDagTreeLODSparseReentrant((blend_dag_node const *)RootNode,
                                                                      (int32x)BoneCount,
                                                                      (real32)AllowedError,
                                                                      (int32x const *)SparseBoneArray,
                                                                      (dag_pose_cache *)PoseCache));
}

GRANNY_DYNLINK(granny_local_pose *) GrannySampleBlendDagTreeLODSparse(granny_blend_dag_node const * RootNode,
                                                                      granny_int32x BoneCount,
                                                                      granny_real32 AllowedError,
                                                                      granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)SampleBlendDagTreeLODSparse((blend_dag_node const *)RootNode,
                                                             (int32x)BoneCount,
                                                             (real32)AllowedError,
                                                             (int32x const *)SparseBoneArray));
}

GRANNY_DYNLINK(granny_local_pose *) GrannySampleBlendDagTree(granny_blend_dag_node const * RootNode,
                                                             granny_int32x BoneCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)SampleBlendDagTree((blend_dag_node const *)RootNode,
                                                    (int32x)BoneCount));
}

GRANNY_DYNLINK(granny_local_pose *) GrannySampleBlendDagTreeLOD(granny_blend_dag_node const * RootNode,
                                                                granny_int32x BoneCount,
                                                                granny_real32 AllowedError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)SampleBlendDagTreeLOD((blend_dag_node const *)RootNode,
                                                       (int32x)BoneCount,
                                                       (real32)AllowedError));
}

GRANNY_DYNLINK(granny_local_pose *) GrannySampleBlendDagTreeReentrant(granny_blend_dag_node const * RootNode,
                                                                      granny_int32x BoneCount,
                                                                      granny_dag_pose_cache * PoseCache)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)SampleBlendDagTreeReentrant((blend_dag_node const *)RootNode,
                                                             (int32x)BoneCount,
                                                             (dag_pose_cache *)PoseCache));
}

GRANNY_DYNLINK(granny_local_pose *) GrannySampleBlendDagTreeLODReentrant(granny_blend_dag_node const * RootNode,
                                                                         granny_int32x BoneCount,
                                                                         granny_real32 AllowedError,
                                                                         granny_dag_pose_cache * PoseCache)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)SampleBlendDagTreeLODReentrant((blend_dag_node const *)RootNode,
                                                                (int32x)BoneCount,
                                                                (real32)AllowedError,
                                                                (dag_pose_cache *)PoseCache));
}

GRANNY_DYNLINK(void) GrannySetBlendDagTreeClock(granny_blend_dag_node * RootNode,
                                                granny_real32 NewClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBlendDagTreeClock((blend_dag_node *)RootNode,
                         (real32)NewClock);
}

GRANNY_DYNLINK(void) GrannyGetBlendDagNodeChildren(granny_blend_dag_node * Node,
                                                   granny_int32x MaxArraySize,
                                                   granny_blend_dag_node ** ArrayOfChildren)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetBlendDagNodeChildren((blend_dag_node *)Node,
                            (int32x)MaxArraySize,
                            (blend_dag_node **)ArrayOfChildren);
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyCreateBlendDagNodeWeightedBlendChildren(granny_skeleton * ReferenceSkeleton,
                                                                                      granny_real32 FillThreshold,
                                                                                      granny_quaternion_mode QuaternionBlendingMode,
                                                                                      granny_int32x BoneCount,
                                                                                      granny_int32x NumChildren,
                                                                                      granny_blend_dag_node ** ArrayOfChildren)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)CreateBlendDagNodeWeightedBlendChildren((skeleton *)ReferenceSkeleton,
                                                                             (real32)FillThreshold,
                                                                             (quaternion_mode)QuaternionBlendingMode,
                                                                             (int32x)BoneCount,
                                                                             (int32x)NumChildren,
                                                                             (blend_dag_node **)ArrayOfChildren));
}

GRANNY_DYNLINK(void) GrannyFreeBlendDagEntireTree(granny_blend_dag_node * RootNode)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeBlendDagEntireTree((blend_dag_node *)RootNode);
}

GRANNY_DYNLINK(void) GrannyBlendDagFreeCompletedControlsEntireTree(granny_blend_dag_node * Node)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BlendDagFreeCompletedControlsEntireTree((blend_dag_node *)Node);
}

GRANNY_DYNLINK(granny_blend_dag_node *) GrannyDuplicateBlendDagTree(granny_blend_dag_node const * SourceTreeRoot,
                                                                    granny_blend_dag_node const ** SourceNodeList,
                                                                    granny_blend_dag_node ** DestNodeList,
                                                                    granny_int32 SizeOfNodeList,
                                                                    bool AutoFreeCreatedModelInstances)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_blend_dag_node *)DuplicateBlendDagTree((blend_dag_node const *)SourceTreeRoot,
                                                           (blend_dag_node const **)SourceNodeList,
                                                           (blend_dag_node **)DestNodeList,
                                                           (int32)SizeOfNodeList,
                                                           (bool)AutoFreeCreatedModelInstances));
}

GRANNY_DYNLINK(bool) GrannyIsBlendDagNodeValid(granny_blend_dag_node const * Node,
                                               char ** ReasonFailed,
                                               granny_blend_dag_node const ** NodeFailed)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)IsBlendDagNodeValid((blend_dag_node const *)Node,
                                      (char **)ReasonFailed,
                                      (blend_dag_node const **)NodeFailed));
}

GRANNY_DYNLINK(bool) GrannyIsBlendDagTreeValid(granny_blend_dag_node const * RootNode,
                                               char ** ReasonFailed,
                                               granny_blend_dag_node const ** NodeFailed)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)IsBlendDagTreeValid((blend_dag_node const *)RootNode,
                                      (char **)ReasonFailed,
                                      (blend_dag_node const **)NodeFailed));
}

GRANNY_DYNLINK(granny_int32x) GrannyFindBlendDagTreeDepth(granny_blend_dag_node const * RootNode)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = FindBlendDagTreeDepth((blend_dag_node const *)RootNode));
}

#include "granny_data_type_conversion.h"
GRANNY_DYNLINK(bool) GrannyFindMatchingMember(granny_data_type_definition const * SourceType,
                                              void const * SourceObject,
                                              char const * DestMemberName,
                                              granny_variant *  Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindMatchingMember((data_type_definition const *)SourceType,
                                     (void const *)SourceObject,
                                     (char const *)DestMemberName,
                                     (variant * )Result));
}

GRANNY_DYNLINK(void) GrannyConvertSingleObject(granny_data_type_definition const * SourceType,
                                               void const * SourceObject,
                                               granny_data_type_definition const * DestType,
                                               void * DestObject)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ConvertSingleObject((data_type_definition const *)SourceType,
                        (void const *)SourceObject,
                        (data_type_definition const *)DestType,
                        (void *)DestObject);
}

GRANNY_DYNLINK(void) GrannyMergeSingleObject(granny_data_type_definition const * SourceType,
                                             void const * SourceObject,
                                             granny_data_type_definition const * DestType,
                                             void * DestObject)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    MergeSingleObject((data_type_definition const *)SourceType,
                      (void const *)SourceObject,
                      (data_type_definition const *)DestType,
                      (void *)DestObject);
}

GRANNY_DYNLINK(void *) GrannyConvertTree(granny_data_type_definition const * SourceType,
                                         void const * SourceTree,
                                         granny_data_type_definition const * DestType)
{
    return ((void *)ConvertTree((data_type_definition const *)SourceType,
                                (void const *)SourceTree,
                                (data_type_definition const *)DestType));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetConvertedTreeSize(granny_data_type_definition const * SourceType,
                                                         void const * SourceTree,
                                                         granny_data_type_definition const * DestType)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetConvertedTreeSize((data_type_definition const *)SourceType,
                                                                  (void const *)SourceTree,
                                                                  (data_type_definition const *)DestType));
}

GRANNY_DYNLINK(void *) GrannyConvertTreeInPlace(granny_data_type_definition const * SourceType,
                                                void const * SourceTree,
                                                granny_data_type_definition const * DestType,
                                                void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)ConvertTreeInPlace((data_type_definition const *)SourceType,
                                       (void const *)SourceTree,
                                       (data_type_definition const *)DestType,
                                       (void *)Memory));
}

GRANNY_DYNLINK(bool) GrannyRebasePointers(granny_data_type_definition const * Type,
                                          void * Data,
                                          granny_intaddrx Offset,
                                          bool RebaseStrings)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)RebasePointers((data_type_definition const *)Type,
                                 (void *)Data,
                                 (intaddrx)Offset,
                                 (bool)RebaseStrings));
}

GRANNY_DYNLINK(bool) GrannyRebasePointersStringCallback(granny_data_type_definition const * Type,
                                                        void * Data,
                                                        granny_intaddrx Offset,
                                                        granny_rebase_pointers_string_callback * Callback,
                                                        void * CallbackData)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)RebasePointersStringCallback((data_type_definition const *)Type,
                                               (void *)Data,
                                               (intaddrx)Offset,
                                               (rebase_pointers_string_callback *)Callback,
                                               (void *)CallbackData));
}

#include "granny_data_type_io.h"
GRANNY_DYNLINK(granny_file_data_tree_writer *) GrannyBeginFileDataTreeWriting(granny_data_type_definition const * RootObjectTypeDefinition,
                                                                              void *  RootObject,
                                                                              granny_int32x DefaultTypeSectionIndex,
                                                                              granny_int32x DefaultObjectSectionIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_data_tree_writer *)BeginFileDataTreeWriting((data_type_definition const *)RootObjectTypeDefinition,
                                                                     (void * )RootObject,
                                                                     (int32x)DefaultTypeSectionIndex,
                                                                     (int32x)DefaultObjectSectionIndex));
}

GRANNY_DYNLINK(void) GrannyPreserveObjectFileSections(granny_file_data_tree_writer *  Writer,
                                                      granny_file const * SourceFile)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PreserveObjectFileSections((file_data_tree_writer & )*Writer,
                               (file const *)SourceFile);
}

GRANNY_DYNLINK(void) GrannyEndFileDataTreeWriting(granny_file_data_tree_writer * Writer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndFileDataTreeWriting((file_data_tree_writer *)Writer);
}

GRANNY_DYNLINK(void) GrannySetFileDataTreeFlags(granny_file_data_tree_writer * Writer,
                                                granny_uint32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileDataTreeFlags((file_data_tree_writer &)*Writer,
                         (uint32x)Flags);
}

GRANNY_DYNLINK(void) GrannySetFileSectionForObjectsOfType(granny_file_data_tree_writer * Writer,
                                                          granny_data_type_definition const * Type,
                                                          granny_int32x SectionIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileSectionForObjectsOfType((file_data_tree_writer &)*Writer,
                                   (data_type_definition const *)Type,
                                   (int32x)SectionIndex);
}

GRANNY_DYNLINK(void) GrannySetFileSectionForObject(granny_file_data_tree_writer * Writer,
                                                   void const * Object,
                                                   granny_int32x SectionIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileSectionForObject((file_data_tree_writer &)*Writer,
                            (void const *)Object,
                            (int32x)SectionIndex);
}

GRANNY_DYNLINK(bool) GrannyWriteDataTreeToFileBuilder(granny_file_data_tree_writer * Writer,
                                                      granny_file_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)WriteDataTreeToFileBuilder((file_data_tree_writer &)*Writer,
                                             (file_builder &)*Builder));
}

GRANNY_DYNLINK(bool) GrannyWriteDataTreeToFile(granny_file_data_tree_writer * Writer,
                                               granny_uint32 FileTypeTag,
                                               granny_grn_file_magic_value const * PlatformMagicValue,
                                               char const * FileName,
                                               granny_int32x FileSectionCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)WriteDataTreeToFile((file_data_tree_writer &)*Writer,
                                      (uint32)FileTypeTag,
                                      (grn_file_magic_value const &)*PlatformMagicValue,
                                      (char const *)FileName,
                                      (int32x)FileSectionCount));
}

GRANNY_DYNLINK(void) GrannySetFileWriterStringCallback(granny_file_data_tree_writer * Writer,
                                                       granny_file_writer_string_callback * Callback,
                                                       void * Data)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileWriterStringCallback((file_data_tree_writer &)*Writer,
                                (file_writer_string_callback *)Callback,
                                (void *)Data);
}

#include "granny_degree_of_freedom.h"
GRANNY_DYNLINK(bool) GrannyClipPositionDOFs(granny_real32 * Position,
                                            granny_uint32x AllowedDOFs)
{
    return ((bool)ClipPositionDOFs((real32 *)Position,
                                   (uint32x)AllowedDOFs));
}

GRANNY_DYNLINK(bool) GrannyClipAngularVelocityDOFs(granny_real32 * Orientation,
                                                   granny_uint32x AllowedDOFs)
{
    return ((bool)ClipAngularVelocityDOFs((real32 *)Orientation,
                                          (uint32x)AllowedDOFs));
}

GRANNY_DYNLINK(bool) GrannyClipOrientationDOFs(granny_real32 * Orientation,
                                               granny_uint32x AllowedDOFs)
{
    return ((bool)ClipOrientationDOFs((real32 *)Orientation,
                                      (uint32x)AllowedDOFs));
}

GRANNY_DYNLINK(void) GrannyClipTransformDOFs(granny_transform * Result,
                                             granny_uint32x AllowedDOFs)
{
    ClipTransformDOFs((transform &)*Result,
                      (uint32x)AllowedDOFs);
}

#include "granny_exporter_info.h"
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyExporterInfoType = (granny_data_type_definition *)ExporterInfoType;
#include "granny_file.h"
GRANNY_DYNLINK(bool) GrannyFileCRCIsValid(char const * FileName)
{
    return ((bool)FileCRCIsValid((char const *)FileName));
}

GRANNY_DYNLINK(granny_file *) GrannyReadEntireFile(char const * FileName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file *)ReadEntireFile((char const *)FileName));
}

GRANNY_DYNLINK(granny_file *) GrannyReadEntireFileFromMemory(granny_int32x MemorySize,
                                                             void * Memory)
{
    return ((granny_file *)ReadEntireFileFromMemory((int32x)MemorySize,
                                                    (void *)Memory));
}

GRANNY_DYNLINK(granny_file *) GrannyReadEntireFileFromReader(granny_file_reader * Reader)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file *)ReadEntireFileFromReader((file_reader &)*Reader));
}

GRANNY_DYNLINK(granny_file *) GrannyReadPartialFileFromReader(granny_file_reader * Reader)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file *)ReadPartialFileFromReader((file_reader &)*Reader));
}

GRANNY_DYNLINK(void) GrannyFreeFileSection(granny_file * File,
                                           granny_int32x SectionIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeFileSection((file &)*File,
                    (int32x)SectionIndex);
}

GRANNY_DYNLINK(void) GrannyFreeAllFileSections(granny_file * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeAllFileSections((file &)*File);
}

GRANNY_DYNLINK(bool) GrannyReadFileSection(granny_file_reader * Reader,
                                           granny_file * File,
                                           granny_int32x SectionIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ReadFileSection((file_reader &)*Reader,
                                  (file &)*File,
                                  (int32x)SectionIndex));
}

GRANNY_DYNLINK(bool) GrannyReadFileSectionInPlace(granny_file_reader * Reader,
                                                  granny_file * File,
                                                  granny_int32x SectionIndex,
                                                  void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ReadFileSectionInPlace((file_reader &)*Reader,
                                         (file &)*File,
                                         (int32x)SectionIndex,
                                         (void *)Memory));
}

GRANNY_DYNLINK(void) GrannyFixupFileSectionPhase1(granny_file * File,
                                                  granny_int32x SectionIndex,
                                                  granny_grn_pointer_fixup const*  PointerFixupArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FixupFileSectionPhase1((file &)*File,
                           (int32x)SectionIndex,
                           (grn_pointer_fixup const* )PointerFixupArray);
}

GRANNY_DYNLINK(bool) GrannyFixupFileSectionPhase2(granny_file_reader * Reader,
                                                  granny_file * File,
                                                  granny_int32x SectionIndex,
                                                  granny_grn_pointer_fixup const*  PointerFixupArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FixupFileSectionPhase2((file_reader &)*Reader,
                                         (file &)*File,
                                         (int32x)SectionIndex,
                                         (grn_pointer_fixup const* )PointerFixupArray));
}

GRANNY_DYNLINK(bool) GrannyLoadFixupArray(granny_file_reader * Reader,
                                          granny_grn_section const * Section,
                                          bool FileIsByteReversed,
                                          granny_grn_pointer_fixup **  Array)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)LoadFixupArray((file_reader &)*Reader,
                                 (grn_section const &)*Section,
                                 (bool)FileIsByteReversed,
                                 (grn_pointer_fixup ** )Array));
}

GRANNY_DYNLINK(void) GrannyFreeFile(granny_file * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeFile((file *)File);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetFileSectionOfLoadedObject(granny_file const * File,
                                                                 void const * Object)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetFileSectionOfLoadedObject((file const &)*File,
                                                                          (void const *)Object));
}

GRANNY_DYNLINK(void) GrannyGetDataTreeFromFile(granny_file const * File,
                                               granny_variant *  Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetDataTreeFromFile((file const &)*File,
                        (variant * )Result);
}

GRANNY_DYNLINK(granny_uint32) GrannyGetFileTypeTag(granny_file const * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    uint32 ReturnValue;
    return *(granny_uint32 *)&(ReturnValue = GetFileTypeTag((file const &)*File));
}

GRANNY_DYNLINK(bool) GrannyRecompressFile(char const * SourceFile,
                                          char const * DestFile,
                                          granny_int32x CompressorMappingCount,
                                          granny_int32x const * CompressorMapping)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)RecompressFile((char const *)SourceFile,
                                 (char const *)DestFile,
                                 (int32x)CompressorMappingCount,
                                 (int32x const *)CompressorMapping));
}

GRANNY_DYNLINK(bool) GrannyConvertFileInfoToRaw(granny_file_info *  SourceFileInfo,
                                                char const * DestFileName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ConvertFileInfoToRaw((file_info * )SourceFileInfo,
                                       (char const *)DestFileName));
}

GRANNY_DYNLINK(bool) GrannyConvertFileToRaw(char const * SourceFileName,
                                            char const * DestFileName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)ConvertFileToRaw((char const *)SourceFileName,
                                   (char const *)DestFileName));
}

GRANNY_DYNLINK(bool) GrannyPlatformConvertReaderToWriter(granny_file_reader * SourceReader,
                                                         granny_file_writer * DestWriter,
                                                         granny_grn_file_magic_value const * DestMagicValue,
                                                         bool ExcludeTypeTree)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)PlatformConvertReaderToWriter((file_reader &)*SourceReader,
                                                (file_writer &)*DestWriter,
                                                (grn_file_magic_value const &)*DestMagicValue,
                                                (bool)ExcludeTypeTree));
}

GRANNY_DYNLINK(granny_uint32) GrannyGetInMemoryFileCRC(granny_file * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    uint32 ReturnValue;
    return *(granny_uint32 *)&(ReturnValue = GetInMemoryFileCRC((file *)File));
}

#include "granny_file_builder.h"
GRANNY_DYNLINK(granny_file_builder *) GrannyBeginFile(granny_int32x SectionCount,
                                                      granny_uint32 FileTypeTag,
                                                      granny_grn_file_magic_value const * PlatformMagicValue,
                                                      char const * TemporaryDirectory,
                                                      char const * TemporaryFileNameRoot)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_builder *)BeginFile((int32x)SectionCount,
                                             (uint32)FileTypeTag,
                                             (grn_file_magic_value const &)*PlatformMagicValue,
                                             (char const *)TemporaryDirectory,
                                             (char const *)TemporaryFileNameRoot));
}

GRANNY_DYNLINK(granny_file_builder *) GrannyBeginFileInMemory(granny_int32x SectionCount,
                                                              granny_uint32 FileTypeTag,
                                                              granny_grn_file_magic_value const * PlatformMagicValue,
                                                              granny_int32x MemoryBlockSize)
{
    return ((granny_file_builder *)BeginFileInMemory((int32x)SectionCount,
                                                     (uint32)FileTypeTag,
                                                     (grn_file_magic_value const &)*PlatformMagicValue,
                                                     (int32x)MemoryBlockSize));
}

GRANNY_DYNLINK(bool) GrannyEndFile(granny_file_builder * Builder,
                                   char const * FileName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndFile((file_builder *)Builder,
                          (char const *)FileName));
}

GRANNY_DYNLINK(bool) GrannyEndFileToWriter(granny_file_builder * Builder,
                                           granny_file_writer * ToFile)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndFileToWriter((file_builder *)Builder,
                                  (file_writer &)*ToFile));
}

GRANNY_DYNLINK(bool) GrannyEndFileRaw(granny_file_builder * Builder,
                                      char const * FileName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndFileRaw((file_builder *)Builder,
                             (char const *)FileName));
}

GRANNY_DYNLINK(bool) GrannyEndFileRawToWriter(granny_file_builder * Builder,
                                              granny_file_writer * ToFile)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndFileRawToWriter((file_builder *)Builder,
                                     (file_writer &)*ToFile));
}

GRANNY_DYNLINK(void) GrannyAbortFile(granny_file_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AbortFile((file_builder *)Builder);
}

GRANNY_DYNLINK(void) GrannySetFileSectionFormat(granny_file_builder * Builder,
                                                granny_int32x SectionIndex,
                                                granny_compression_type Compression,
                                                granny_int32x Alignment)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileSectionFormat((file_builder &)*Builder,
                         (int32x)SectionIndex,
                         (compression_type)Compression,
                         (int32x)Alignment);
}

GRANNY_DYNLINK(void) GrannyPreserveFileSectionFormats(granny_file_builder * Builder,
                                                      granny_file const * SourceFile)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PreserveFileSectionFormats((file_builder &)*Builder,
                               (file const &)*SourceFile);
}

GRANNY_DYNLINK(void) GrannyWriteFileChunk(granny_file_builder * Builder,
                                          granny_int32x InSectionIndex,
                                          granny_uint32 Marshalling,
                                          granny_uint32x Size,
                                          void const * Data,
                                          granny_file_location *  Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    WriteFileChunk((file_builder &)*Builder,
                   (int32x)InSectionIndex,
                   (uint32)Marshalling,
                   (uint32x)Size,
                   (void const *)Data,
                   (file_location * )Result);
}

GRANNY_DYNLINK(void) GrannyOffsetFileLocation(granny_file_builder * Builder,
                                              granny_file_location const * Location,
                                              granny_uint32 AdditionalOffset,
                                              granny_file_location *  Result)
{
    OffsetFileLocation((file_builder &)*Builder,
                       (file_location const &)*Location,
                       (uint32)AdditionalOffset,
                       (file_location * )Result);
}

GRANNY_DYNLINK(granny_file_fixup *) GrannyMarkFileFixup(granny_file_builder * Builder,
                                                        granny_file_location const * From,
                                                        granny_int32x FromOffset,
                                                        granny_file_location const * To)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_fixup *)MarkFileFixup((file_builder &)*Builder,
                                               (file_location const &)*From,
                                               (int32x)FromOffset,
                                               (file_location const &)*To));
}

GRANNY_DYNLINK(void) GrannyAdjustFileFixup(granny_file_builder * Builder,
                                           granny_file_fixup * Fixup,
                                           granny_file_location const * NewTo)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AdjustFileFixup((file_builder &)*Builder,
                    (file_fixup &)*Fixup,
                    (file_location const &)*NewTo);
}

GRANNY_DYNLINK(void) GrannyMarkMarshallingFixup(granny_file_builder * Builder,
                                                granny_file_location const * Type,
                                                granny_file_location const * Object,
                                                granny_int32x ArrayCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    MarkMarshallingFixup((file_builder &)*Builder,
                         (file_location const &)*Type,
                         (file_location const &)*Object,
                         (int32x)ArrayCount);
}

GRANNY_DYNLINK(void) GrannyMarkFileRootObject(granny_file_builder * Builder,
                                              granny_file_location const * TypeLocation,
                                              granny_file_location const * ObjectLocation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    MarkFileRootObject((file_builder &)*Builder,
                       (file_location const &)*TypeLocation,
                       (file_location const &)*ObjectLocation);
}

GRANNY_DYNLINK(void) GrannySetFileStringDatabaseCRC(granny_file_builder * Builder,
                                                    granny_uint32 DatabaseCRC)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileStringDatabaseCRC((file_builder &)*Builder,
                             (uint32)DatabaseCRC);
}

GRANNY_DYNLINK(void) GrannySetFileExtraTag(granny_file_builder * Builder,
                                           granny_int32x Index,
                                           granny_uint32 Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetFileExtraTag((file_builder &)*Builder,
                    (int32x)Index,
                    (uint32)Value);
}

#include "granny_file_compressor.h"
GRANNY_DYNLINK(granny_int32x) GrannyGetCompressedBytesPaddingSize(granny_compression_type Format)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetCompressedBytesPaddingSize((compression_type)Format));
}

GRANNY_DYNLINK(bool) GrannyDecompressData(granny_compression_type Format,
                                          bool FileIsByteReversed,
                                          granny_int32x CompressedBytesSize,
                                          void * CompressedBytes,
                                          granny_int32x Stop0,
                                          granny_int32x Stop1,
                                          granny_int32x Stop2,
                                          void * DecompressedBytes)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)DecompressData((compression_type)Format,
                                 (bool)FileIsByteReversed,
                                 (int32x)CompressedBytesSize,
                                 (void *)CompressedBytes,
                                 (int32x)Stop0,
                                 (int32x)Stop1,
                                 (int32x)Stop2,
                                 (void *)DecompressedBytes));
}

GRANNY_DYNLINK(bool) GrannyDecompressDataChunk(granny_compression_type Format,
                                               bool FileIsByteReversed,
                                               granny_int32x CompressedBytesSize,
                                               void * CompressedBytes,
                                               granny_int32x DecompressedBytesSize,
                                               void * DecompressedBytes)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)DecompressDataChunk((compression_type)Format,
                                      (bool)FileIsByteReversed,
                                      (int32x)CompressedBytesSize,
                                      (void *)CompressedBytes,
                                      (int32x)DecompressedBytesSize,
                                      (void *)DecompressedBytes));
}

GRANNY_DYNLINK(granny_file_compressor *) GrannyBeginFileCompression(granny_uint32x ExpandedDataSize,
                                                                    granny_int32x ContentsCount,
                                                                    granny_compression_type Type,
                                                                    bool WritingForReversedPlatform,
                                                                    granny_file_writer * Writer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_compressor *)BeginFileCompression((uint32x)ExpandedDataSize,
                                                           (int32x)ContentsCount,
                                                           (compression_type)Type,
                                                           (bool)WritingForReversedPlatform,
                                                           (file_writer *)Writer));
}

GRANNY_DYNLINK(bool) GrannyCompressContentsOfFile(granny_file_compressor * Compressor,
                                                  granny_int32x FileSize,
                                                  char const * FileName,
                                                  granny_int32x CopyBufferSize,
                                                  void * CopyBuffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CompressContentsOfFile((file_compressor &)*Compressor,
                                         (int32x)FileSize,
                                         (char const *)FileName,
                                         (int32x)CopyBufferSize,
                                         (void *)CopyBuffer));
}

GRANNY_DYNLINK(bool) GrannyCompressContentsOfReader(granny_file_compressor * Compressor,
                                                    granny_int32x FileSize,
                                                    granny_file_reader * Reader,
                                                    granny_int32x CopyBufferSize,
                                                    void * CopyBuffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CompressContentsOfReader((file_compressor &)*Compressor,
                                           (int32x)FileSize,
                                           (file_reader &)*Reader,
                                           (int32x)CopyBufferSize,
                                           (void *)CopyBuffer));
}

GRANNY_DYNLINK(bool) GrannyCompressContentsOfMemory(granny_file_compressor * Compressor,
                                                    granny_int32x BufferSize,
                                                    void const * Buffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CompressContentsOfMemory((file_compressor &)*Compressor,
                                           (int32x)BufferSize,
                                           (void const *)Buffer));
}

GRANNY_DYNLINK(bool) GrannyEndFileCompression(granny_file_compressor * Compressor,
                                              granny_uint32x * CompressedSize)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndFileCompression((file_compressor *)Compressor,
                                     (uint32x &)*CompressedSize));
}

#include "granny_file_format.h"
GRANNY_DYNLINK(char const *) GrannyGetStandardSectionName(granny_int32x SectionIndex)
{
    return ((char const *)GetStandardSectionName((int32x)SectionIndex));
}

GRANNY_DYNLINK(bool) GrannyIsGrannyFile(granny_grn_file_magic_value const * MagicValue,
                                        granny_uint32x * TotalHeaderSize,
                                        bool * IsByteReversed,
                                        bool * IsPointerSizeDifferent)
{
    return ((bool)IsGrannyFile((grn_file_magic_value const &)*MagicValue,
                               (uint32x &)*TotalHeaderSize,
                               (bool &)*IsByteReversed,
                               (bool &)*IsPointerSizeDifferent));
}

GRANNY_DYNLINK(granny_grn_section *) GrannyGetGRNSectionArray(granny_grn_file_header const * Header)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_grn_section *)GetGRNSectionArray((grn_file_header const &)*Header));
}

GRANNY_DYNLINK(void) GrannyReverseSection(granny_int32x First16Bit,
                                          granny_int32x First8Bit,
                                          granny_int32x End,
                                          void * BufferInit)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ReverseSection((int32x)First16Bit,
                   (int32x)First8Bit,
                   (int32x)End,
                   (void *)BufferInit);
}

GRANNY_DYNLINK(void) GrannyGRNFixUp(granny_uint32x FixupCount,
                                    granny_grn_pointer_fixup const * FixupArray,
                                    void const ** ToSections,
                                    void * FromSection)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GRNFixUp((uint32x)FixupCount,
             (grn_pointer_fixup const *)FixupArray,
             (void const **)ToSections,
             (void *)FromSection);
}

GRANNY_DYNLINK(void) GrannyGRNMarshall(granny_uint32x FixupCount,
                                       granny_grn_mixed_marshalling_fixup const * FixupArray,
                                       void const ** ToSections,
                                       void * FromSection)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GRNMarshall((uint32x)FixupCount,
                (grn_mixed_marshalling_fixup const *)FixupArray,
                (void const **)ToSections,
                (void *)FromSection);
}

GRANNY_DYNLINK(void *) GrannyDecodeGRNReference(void const ** Sections,
                                                granny_grn_reference const * Reference)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)DecodeGRNReference((void const **)Sections,
                                       (grn_reference const &)*Reference));
}

GRANNY_DYNLINK(bool) GrannyGetMagicValueForPlatform(granny_int32x PointerSizeInBits,
                                                    bool LittleEndian,
                                                    granny_grn_file_magic_value * MagicValue)
{
    return ((bool)GetMagicValueForPlatform((int32x)PointerSizeInBits,
                                           (bool)LittleEndian,
                                           (grn_file_magic_value &)*MagicValue));
}

GRANNY_DYNLINK(void) GrannyGetThisPlatformProperties(granny_int32x *  PointerSize,
                                                     bool *  LittleEndian)
{
    GetThisPlatformProperties((int32x * )PointerSize,
                              (bool * )LittleEndian);
}

GRANNY_DYNLINK(bool) GrannyGetMagicValueProperties(granny_grn_file_magic_value const*  MagicValue,
                                                   granny_int32x *  PointerSize,
                                                   bool *    LittleEndian)
{
    return ((bool)GetMagicValueProperties((grn_file_magic_value const& )*MagicValue,
                                          (int32x * )PointerSize,
                                          (bool *   )LittleEndian));
}

GRANNY_DYNLINKDATA(granny_grn_file_magic_value const  *) GrannyGRNFileMV_Old = &(granny_grn_file_magic_value const  &)GRNFileMV_Old;
GRANNY_DYNLINKDATA(granny_grn_file_magic_value const  *) GrannyGRNFileMV_32Bit_LittleEndian = &(granny_grn_file_magic_value const  &)GRNFileMV_32Bit_LittleEndian;
GRANNY_DYNLINKDATA(granny_grn_file_magic_value const  *) GrannyGRNFileMV_32Bit_BigEndian = &(granny_grn_file_magic_value const  &)GRNFileMV_32Bit_BigEndian;
GRANNY_DYNLINKDATA(granny_grn_file_magic_value const  *) GrannyGRNFileMV_64Bit_LittleEndian = &(granny_grn_file_magic_value const  &)GRNFileMV_64Bit_LittleEndian;
GRANNY_DYNLINKDATA(granny_grn_file_magic_value const  *) GrannyGRNFileMV_64Bit_BigEndian = &(granny_grn_file_magic_value const  &)GRNFileMV_64Bit_BigEndian;
GRANNY_DYNLINKDATA(granny_grn_file_magic_value const  *) GrannyGRNFileMV_ThisPlatform = &(granny_grn_file_magic_value const  &)GRNFileMV_ThisPlatform;
#include "granny_file_info.h"
GRANNY_DYNLINK(granny_file_info *) GrannyGetFileInfo(granny_file * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_info *)GetFileInfo((file &)*File));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyFileInfoType = (granny_data_type_definition *)FileInfoType;
#include "granny_file_operations.h"
GRANNY_DYNLINK(char const *) GrannyGetTemporaryDirectory(void)
{
    return ((char const *)GetTemporaryDirectory());
}

#include "granny_file_reader.h"
GRANNY_DYNLINK(void) GrannyInitializeFileReader(char const * SourceFileName,
                                                granny_int32x SourceLineNumber,
                                                granny_close_file_reader_callback * CloseFileReaderCallback,
                                                granny_read_at_most_callback * ReadAtMostCallback,
                                                granny_file_reader * Reader)
{
    InitializeFileReader((char const *)SourceFileName,
                         (int32x)SourceLineNumber,
                         (close_file_reader_callback *)CloseFileReaderCallback,
                         (read_at_most_callback *)ReadAtMostCallback,
                         (file_reader &)*Reader);
}

GRANNY_DYNLINK(granny_file_reader *) GrannyCreatePlatformFileReader(char const * SourceFileName,
                                                                    granny_int32x SourceLineNumber,
                                                                    char const * FileNameToOpen)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_reader *)CreatePlatformFileReader((char const *)SourceFileName,
                                                           (int32x)SourceLineNumber,
                                                           (char const *)FileNameToOpen));
}

GRANNY_DYNLINK(granny_open_file_reader_callback *) GrannyGetDefaultFileReaderOpenCallback(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_open_file_reader_callback *)GetDefaultFileReaderOpenCallback());
}

GRANNY_DYNLINK(void) GrannySetDefaultFileReaderOpenCallback(granny_open_file_reader_callback * OpenFileReaderCallback)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetDefaultFileReaderOpenCallback((open_file_reader_callback *)OpenFileReaderCallback);
}

#include "granny_file_writer.h"
GRANNY_DYNLINK(granny_int32x) GrannyAlignWriter(granny_file_writer * Writer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = AlignWriter((file_writer &)*Writer));
}

GRANNY_DYNLINK(granny_int32x) GrannyPredictWriterAlignment(granny_int32x Position)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = PredictWriterAlignment((int32x)Position));
}

GRANNY_DYNLINK(void) GrannyInitializeFileWriter(char const * SourceFileName,
                                                granny_int32x SourceLineNumber,
                                                granny_delete_file_writer_callback * DeleteFileWriterCallback,
                                                granny_seek_file_writer_callback * SeekWriterCallback,
                                                granny_write_file_writer_callback * WriteCallback,
                                                granny_begincrc_file_writer_callback * BeginCRCCallback,
                                                granny_endcrc_file_writer_callback * EndCRCCallback,
                                                granny_file_writer * Writer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InitializeFileWriter((char const *)SourceFileName,
                         (int32x)SourceLineNumber,
                         (delete_file_writer_callback *)DeleteFileWriterCallback,
                         (seek_file_writer_callback *)SeekWriterCallback,
                         (write_file_writer_callback *)WriteCallback,
                         (begincrc_file_writer_callback *)BeginCRCCallback,
                         (endcrc_file_writer_callback *)EndCRCCallback,
                         (file_writer &)*Writer);
}

GRANNY_DYNLINK(granny_file_writer *) GrannyCreatePlatformFileWriter(char const * SourceFileName,
                                                                    granny_int32x SourceLineNumber,
                                                                    char const * FileNameToOpen,
                                                                    bool EraseExisting)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_file_writer *)CreatePlatformFileWriter((char const *)SourceFileName,
                                                           (int32x)SourceLineNumber,
                                                           (char const *)FileNameToOpen,
                                                           (bool)EraseExisting));
}

GRANNY_DYNLINK(granny_open_file_writer_callback *) GrannyGetDefaultFileWriterOpenCallback(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_open_file_writer_callback *)GetDefaultFileWriterOpenCallback());
}

GRANNY_DYNLINK(void) GrannySetDefaultFileWriterOpenCallback(granny_open_file_writer_callback * OpenFileWriterCallback)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetDefaultFileWriterOpenCallback((open_file_writer_callback *)OpenFileWriterCallback);
}

GRANNY_DYNLINK(granny_int32x) GrannySeekWriterFromStartStub(char const * SourceFileName,
                                                            granny_int32x SourceLineNumber,
                                                            granny_file_writer *  Writer,
                                                            granny_int32x OffsetInUInt8s)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = SeekWriterFromStartStub((char const *)SourceFileName,
                                                                     (int32x)SourceLineNumber,
                                                                     (file_writer & )*Writer,
                                                                     (int32x)OffsetInUInt8s));
}

GRANNY_DYNLINK(granny_int32x) GrannySeekWriterFromEndStub(char const * SourceFileName,
                                                          granny_int32x SourceLineNumber,
                                                          granny_file_writer *  Writer,
                                                          granny_int32x OffsetInUInt8s)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = SeekWriterFromEndStub((char const *)SourceFileName,
                                                                   (int32x)SourceLineNumber,
                                                                   (file_writer & )*Writer,
                                                                   (int32x)OffsetInUInt8s));
}

GRANNY_DYNLINK(granny_int32x) GrannySeekWriterFromCurrentPositionStub(char const * SourceFileName,
                                                                      granny_int32x SourceLineNumber,
                                                                      granny_file_writer *  Writer,
                                                                      granny_int32x OffsetInUInt8s)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = SeekWriterFromCurrentPositionStub((char const *)SourceFileName,
                                                                               (int32x)SourceLineNumber,
                                                                               (file_writer & )*Writer,
                                                                               (int32x)OffsetInUInt8s));
}

#include "granny_find_knot.h"
GRANNY_DYNLINK(granny_int32x) GrannyFindKnot(granny_int32x KnotCount,
                                             granny_real32 const * Knots,
                                             granny_real32 t)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = FindKnot((int32x)KnotCount,
                                                      (real32 const *)Knots,
                                                      (real32)t));
}

GRANNY_DYNLINK(granny_int32x) GrannyFindCloseKnot(granny_int32x KnotCount,
                                                  granny_real32 const * Knots,
                                                  granny_real32 t,
                                                  granny_int32x StartingIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = FindCloseKnot((int32x)KnotCount,
                                                           (real32 const *)Knots,
                                                           (real32)t,
                                                           (int32x)StartingIndex));
}

#include "granny_fixed_allocator.h"
GRANNY_DYNLINK(void *) GrannyAllocateFixed(granny_fixed_allocator * Allocator)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)AllocateFixed((fixed_allocator &)*Allocator));
}

GRANNY_DYNLINK(void) GrannyDeallocateFixed(granny_fixed_allocator * Allocator,
                                           void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    DeallocateFixed((fixed_allocator &)*Allocator,
                    (void *)Memory);
}

GRANNY_DYNLINK(void) GrannyDeallocateAllFixed(granny_fixed_allocator * Allocator)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    DeallocateAllFixed((fixed_allocator &)*Allocator);
}

GRANNY_DYNLINK(void) GrannyInitializeFixedAllocator(granny_fixed_allocator * Allocator,
                                                    granny_int32x UnitSize)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InitializeFixedAllocator((fixed_allocator &)*Allocator,
                             (int32x)UnitSize);
}

#include "granny_float16.h"
GRANNY_DYNLINK(granny_real16) GrannyReal32ToReal16(granny_real32 Value)
{
    real16 ReturnValue;
    return *(granny_real16 *)&(ReturnValue = Real32ToReal16((real32)Value));
}

GRANNY_DYNLINK(void) GrannyReal16ToReal32(granny_real16 Value,
                                          granny_real32 *  Output)
{
    Real16ToReal32((real16)Value,
                   (real32 * )Output);
}

#include "granny_ik.h"
GRANNY_DYNLINK(void) GrannyIKUpdate(granny_int32x LinkCount,
                                    granny_int32x EEBoneIndex,
                                    granny_real32 const * DesiredPosition3,
                                    granny_int32x IterationCount,
                                    granny_skeleton const * Skeleton,
                                    granny_real32 const * ModelRoot4x4,
                                    granny_local_pose * LocalPose,
                                    granny_world_pose * WorldPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    IKUpdate((int32x)LinkCount,
             (int32x)EEBoneIndex,
             (real32 const *)DesiredPosition3,
             (int32x)IterationCount,
             (skeleton const &)*Skeleton,
             (real32 const *)ModelRoot4x4,
             (local_pose &)*LocalPose,
             (world_pose &)*WorldPose);
}

GRANNY_DYNLINK(bool) GrannyIKUpdate2Bone(granny_int32x EEBoneIndex,
                                         granny_real32 const * DesiredPosition3,
                                         granny_real32 const * RestrictedMovementPlaneNormal3,
                                         granny_skeleton const * Skeleton,
                                         granny_real32 const * ModelRootTransform,
                                         granny_local_pose * LocalPose,
                                         granny_world_pose * WorldPose,
                                         granny_real32 HyperExtensionStart,
                                         granny_real32 HyperExtensionScale)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)IKUpdate2Bone((int32x)EEBoneIndex,
                                (real32 const *)DesiredPosition3,
                                (real32 const *)RestrictedMovementPlaneNormal3,
                                (skeleton const &)*Skeleton,
                                (real32 const *)ModelRootTransform,
                                (local_pose &)*LocalPose,
                                (world_pose &)*WorldPose,
                                (real32)HyperExtensionStart,
                                (real32)HyperExtensionScale));
}

#include "granny_image_operations.h"
GRANNY_DYNLINK(void) GrannyScaleImage(granny_pixel_filter_type FilterType,
                                      granny_int32x SourceWidth,
                                      granny_int32x SourceHeight,
                                      granny_int32x SourceStride,
                                      granny_uint8 const * SourcePixels,
                                      granny_int32x DestWidth,
                                      granny_int32x DestHeight,
                                      granny_int32x DestStride,
                                      granny_uint8 * DestPixels)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ScaleImage((pixel_filter_type)FilterType,
               (int32x)SourceWidth,
               (int32x)SourceHeight,
               (int32x)SourceStride,
               (uint8 const *)SourcePixels,
               (int32x)DestWidth,
               (int32x)DestHeight,
               (int32x)DestStride,
               (uint8 *)DestPixels);
}

#include "granny_intersection.h"
GRANNY_DYNLINK(granny_int32x) GrannyRayIntersectsPlaneAt(granny_real32 const * PlaneNormal,
                                                         granny_real32 PlaneD,
                                                         granny_real32 const * RayOrigin,
                                                         granny_real32 const * RayNormal,
                                                         granny_real32 * T)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = RayIntersectsPlaneAt((real32 const *)PlaneNormal,
                                                                  (real32)PlaneD,
                                                                  (real32 const *)RayOrigin,
                                                                  (real32 const *)RayNormal,
                                                                  (real32 &)*T));
}

GRANNY_DYNLINK(bool) GrannyRayIntersectsSphere(granny_real32 const * Center,
                                               granny_real32 Radius,
                                               granny_real32 const * RayOrigin,
                                               granny_real32 const * RayNormal)
{
    return ((bool)RayIntersectsSphere((real32 const *)Center,
                                      (real32)Radius,
                                      (real32 const *)RayOrigin,
                                      (real32 const *)RayNormal));
}

GRANNY_DYNLINK(granny_int32x) GrannyRayIntersectsSphereAt(granny_real32 const * Center,
                                                          granny_real32 Radius,
                                                          granny_real32 const * RayOrigin,
                                                          granny_real32 const * RayNormal,
                                                          granny_real32 * InT,
                                                          granny_real32 * OutT)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = RayIntersectsSphereAt((real32 const *)Center,
                                                                   (real32)Radius,
                                                                   (real32 const *)RayOrigin,
                                                                   (real32 const *)RayNormal,
                                                                   (real32 &)*InT,
                                                                   (real32 &)*OutT));
}

GRANNY_DYNLINK(granny_int32x) GrannyRayIntersectsBox(granny_real32 const * Transform4x4,
                                                     granny_real32 const * Min3,
                                                     granny_real32 const * Max3,
                                                     granny_real32 const * RayOrigin,
                                                     granny_real32 const * RayNormal)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = RayIntersectsBox((real32 const *)Transform4x4,
                                                              (real32 const *)Min3,
                                                              (real32 const *)Max3,
                                                              (real32 const *)RayOrigin,
                                                              (real32 const *)RayNormal));
}

GRANNY_DYNLINK(granny_int32x) GrannyRayIntersectsBoxAt(granny_real32 const * Transform4x4,
                                                       granny_real32 const * Min3,
                                                       granny_real32 const * Max3,
                                                       granny_real32 const * RayOrigin,
                                                       granny_real32 const * RayNormal,
                                                       granny_box_intersection * Intersection)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = RayIntersectsBoxAt((real32 const *)Transform4x4,
                                                                (real32 const *)Min3,
                                                                (real32 const *)Max3,
                                                                (real32 const *)RayOrigin,
                                                                (real32 const *)RayNormal,
                                                                (box_intersection &)*Intersection));
}

GRANNY_DYNLINK(granny_int32x) GrannyRayIntersectsTriangleAt(granny_real32 const * P0,
                                                            granny_real32 const * P1,
                                                            granny_real32 const * P2,
                                                            granny_real32 const * RayOrigin,
                                                            granny_real32 const * RayNormal,
                                                            granny_triangle_intersection * Intersection)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = RayIntersectsTriangleAt((real32 const *)P0,
                                                                     (real32 const *)P1,
                                                                     (real32 const *)P2,
                                                                     (real32 const *)RayOrigin,
                                                                     (real32 const *)RayNormal,
                                                                     (triangle_intersection &)*Intersection));
}

#include "granny_local_pose.h"
GRANNY_DYNLINK(granny_local_pose *) GrannyNewLocalPose(granny_int32x BoneCount)
{
    return ((granny_local_pose *)NewLocalPose((int32x)BoneCount));
}

GRANNY_DYNLINK(void) GrannyFreeLocalPose(granny_local_pose * LocalPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeLocalPose((local_pose *)LocalPose);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingLocalPoseSize(granny_int32x BoneCount)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingLocalPoseSize((int32x)BoneCount));
}

GRANNY_DYNLINK(granny_local_pose *) GrannyNewLocalPoseInPlace(granny_int32x BoneCount,
                                                              void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_local_pose *)NewLocalPoseInPlace((int32x)BoneCount,
                                                     (void *)Memory));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetLocalPoseBoneCount(granny_local_pose const * LocalPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetLocalPoseBoneCount((local_pose const &)*LocalPose));
}

GRANNY_DYNLINK(granny_transform *) GrannyGetLocalPoseTransform(granny_local_pose const * LocalPose,
                                                               granny_int32x BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_transform *)GetLocalPoseTransform((local_pose const &)*LocalPose,
                                                      (int32x)BoneIndex));
}

GRANNY_DYNLINK(void * ) GrannyGetLocalPoseOpaqueTransformArray(granny_local_pose const*  LocalPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void * )GetLocalPoseOpaqueTransformArray((local_pose const& )*LocalPose));
}

GRANNY_DYNLINK(void) GrannyBuildRestLocalPose(granny_skeleton const * Skeleton,
                                              granny_int32x FirstBone,
                                              granny_int32x BoneCount,
                                              granny_local_pose * LocalPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildRestLocalPose((skeleton const &)*Skeleton,
                       (int32x)FirstBone,
                       (int32x)BoneCount,
                       (local_pose &)*LocalPose);
}

GRANNY_DYNLINK(void) GrannyBeginLocalPoseAccumulation(granny_local_pose * LocalPose,
                                                      granny_int32x FirstBone,
                                                      granny_int32x BoneCount,
                                                      granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BeginLocalPoseAccumulation((local_pose &)*LocalPose,
                               (int32x)FirstBone,
                               (int32x)BoneCount,
                               (int32x const *)SparseBoneArray);
}

GRANNY_DYNLINK(void) GrannyAccumulateLocalTransform(granny_local_pose * LocalPose,
                                                    granny_int32x LocalPoseBoneIndex,
                                                    granny_int32x SkeletonBoneIndex,
                                                    granny_real32 Weight,
                                                    granny_skeleton const * ReferenceSkeleton,
                                                    granny_quaternion_mode Mode,
                                                    granny_transform const * Transform)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AccumulateLocalTransform((local_pose &)*LocalPose,
                             (int32x)LocalPoseBoneIndex,
                             (int32x)SkeletonBoneIndex,
                             (real32)Weight,
                             (skeleton const &)*ReferenceSkeleton,
                             (quaternion_mode)Mode,
                             (transform const &)*Transform);
}

GRANNY_DYNLINK(void) GrannyEndLocalPoseAccumulation(granny_local_pose * LocalPose,
                                                    granny_int32x FirstBone,
                                                    granny_int32x BoneCount,
                                                    granny_skeleton const * ReferenceSkeleton,
                                                    granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndLocalPoseAccumulation((local_pose &)*LocalPose,
                             (int32x)FirstBone,
                             (int32x)BoneCount,
                             (skeleton const &)*ReferenceSkeleton,
                             (int32x const *)SparseBoneArray);
}

GRANNY_DYNLINK(void) GrannyEndLocalPoseAccumulationLOD(granny_local_pose * LocalPose,
                                                       granny_int32x FirstBone,
                                                       granny_int32x BoneCount,
                                                       granny_skeleton const * ReferenceSkeleton,
                                                       granny_real32 AllowedError,
                                                       granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndLocalPoseAccumulationLOD((local_pose &)*LocalPose,
                                (int32x)FirstBone,
                                (int32x)BoneCount,
                                (skeleton const &)*ReferenceSkeleton,
                                (real32)AllowedError,
                                (int32x const *)SparseBoneArray);
}

GRANNY_DYNLINK(granny_real32) GrannyGetLocalPoseFillThreshold(granny_local_pose const * LocalPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetLocalPoseFillThreshold((local_pose const &)*LocalPose));
}

GRANNY_DYNLINK(void) GrannySetLocalPoseFillThreshold(granny_local_pose * LocalPose,
                                                     granny_real32 FillThreshold)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetLocalPoseFillThreshold((local_pose &)*LocalPose,
                              (real32)FillThreshold);
}

GRANNY_DYNLINK(void) GrannyGetWorldMatrixFromLocalPose(granny_skeleton const * Skeleton,
                                                       granny_int32x BoneIndex,
                                                       granny_local_pose const * LocalPose,
                                                       granny_real32 const * Offset4x4,
                                                       granny_real32 * Result4x4,
                                                       granny_int32x const * SparseBoneArray,
                                                       granny_int32x const * SparseBoneArrayReverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetWorldMatrixFromLocalPose((skeleton const &)*Skeleton,
                                (int32x)BoneIndex,
                                (local_pose const &)*LocalPose,
                                (real32 const *)Offset4x4,
                                (real32 *)Result4x4,
                                (int32x const *)SparseBoneArray,
                                (int32x const *)SparseBoneArrayReverse);
}

GRANNY_DYNLINK(void) GrannyGetAttachmentOffset(granny_skeleton const * Skeleton,
                                               granny_int32x BoneIndex,
                                               granny_local_pose const * LocalPose,
                                               granny_real32 const * Offset4x4,
                                               granny_real32 * Result4x4,
                                               granny_int32x const * SparseBoneArray,
                                               granny_int32x const * SparseBoneArrayReverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetAttachmentOffset((skeleton const &)*Skeleton,
                        (int32x)BoneIndex,
                        (local_pose const &)*LocalPose,
                        (real32 const *)Offset4x4,
                        (real32 *)Result4x4,
                        (int32x const *)SparseBoneArray,
                        (int32x const *)SparseBoneArrayReverse);
}

GRANNY_DYNLINK(void) GrannyModulationCompositeLocalPoseSparse(granny_local_pose * BasePose,
                                                              granny_real32 WeightNone,
                                                              granny_real32 WeightAll,
                                                              granny_track_mask * Mask,
                                                              granny_local_pose const * CompositePose,
                                                              granny_int32x const * SparseBoneArray)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ModulationCompositeLocalPoseSparse((local_pose &)*BasePose,
                                       (real32)WeightNone,
                                       (real32)WeightAll,
                                       (track_mask *)Mask,
                                       (local_pose const &)*CompositePose,
                                       (int32x const *)SparseBoneArray);
}

GRANNY_DYNLINK(void) GrannyLocalPoseFromWorldPose(granny_skeleton const*  Skeleton,
                                                  granny_local_pose * Result,
                                                  granny_world_pose const * WorldPose,
                                                  granny_real32 const * Offset4x4,
                                                  granny_int32x FirstBone,
                                                  granny_int32x BoneCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    LocalPoseFromWorldPose((skeleton const* )Skeleton,
                           (local_pose *)Result,
                           (world_pose const &)*WorldPose,
                           (real32 const *)Offset4x4,
                           (int32x)FirstBone,
                           (int32x)BoneCount);
}

GRANNY_DYNLINK(void) GrannyLocalPoseFromWorldPoseNoScale(granny_skeleton const*  Skeleton,
                                                         granny_local_pose * Result,
                                                         granny_world_pose const * WorldPose,
                                                         granny_real32 const * Offset4x4,
                                                         granny_int32x FirstBone,
                                                         granny_int32x BoneCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    LocalPoseFromWorldPoseNoScale((skeleton const* )Skeleton,
                                  (local_pose *)Result,
                                  (world_pose const &)*WorldPose,
                                  (real32 const *)Offset4x4,
                                  (int32x)FirstBone,
                                  (int32x)BoneCount);
}

GRANNY_DYNLINK(void) GrannyModulationCompositeLocalPose(granny_local_pose * BasePose,
                                                        granny_real32 WeightNone,
                                                        granny_real32 WeightAll,
                                                        granny_track_mask * Mask,
                                                        granny_local_pose const * CompositePose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ModulationCompositeLocalPose((local_pose &)*BasePose,
                                 (real32)WeightNone,
                                 (real32)WeightAll,
                                 (track_mask *)Mask,
                                 (local_pose const &)*CompositePose);
}

GRANNY_DYNLINK(void) GrannyCopyLocalPose(granny_local_pose const * Src,
                                         granny_local_pose * Dst)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyLocalPose((local_pose const &)*Src,
                  (local_pose &)*Dst);
}

GRANNY_DYNLINK(granny_int32x) GrannySparseBoneArrayCreateSingleBone(granny_skeleton const * Skeleton,
                                                                    granny_int32x FirstBoneRequired,
                                                                    granny_int32x * ResultSparseBoneArray,
                                                                    granny_int32x * ResultSparseBoneArrayReverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = SparseBoneArrayCreateSingleBone((skeleton const &)*Skeleton,
                                                                             (int32x)FirstBoneRequired,
                                                                             (int32x *)ResultSparseBoneArray,
                                                                             (int32x *)ResultSparseBoneArrayReverse));
}

GRANNY_DYNLINK(granny_int32x) GrannySparseBoneArrayAddBone(granny_skeleton const * Skeleton,
                                                           granny_int32x BoneToAdd,
                                                           granny_int32x InitialBoneCount,
                                                           granny_int32x * SparseBoneArray,
                                                           granny_int32x * SparseBoneArrayReverse)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = SparseBoneArrayAddBone((skeleton const &)*Skeleton,
                                                                    (int32x)BoneToAdd,
                                                                    (int32x)InitialBoneCount,
                                                                    (int32x *)SparseBoneArray,
                                                                    (int32x *)SparseBoneArrayReverse));
}

GRANNY_DYNLINK(void) GrannySparseBoneArrayExpand(granny_skeleton const * Skeleton,
                                                 granny_local_pose const * SourceSparseLocalPose,
                                                 granny_int32x SparseBoneCount,
                                                 granny_int32x const * SparseBoneArray,
                                                 granny_int32x const * SparseBoneArrayReverse,
                                                 granny_int32x BoneCount,
                                                 granny_local_pose * DestLocalPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SparseBoneArrayExpand((skeleton const &)*Skeleton,
                          (local_pose const &)*SourceSparseLocalPose,
                          (int32x)SparseBoneCount,
                          (int32x const *)SparseBoneArray,
                          (int32x const *)SparseBoneArrayReverse,
                          (int32x)BoneCount,
                          (local_pose &)*DestLocalPose);
}

GRANNY_DYNLINK(bool) GrannySparseBoneArrayIsValid(granny_int32x SkeletonBoneCount,
                                                  granny_int32x SparseBoneCount,
                                                  granny_int32x const * SparseBoneArray,
                                                  granny_int32x const * SparseBoneArrayReverse)
{
    return ((bool)SparseBoneArrayIsValid((int32x)SkeletonBoneCount,
                                         (int32x)SparseBoneCount,
                                         (int32x const *)SparseBoneArray,
                                         (int32x const *)SparseBoneArrayReverse));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyLocalPoseTransformType = (granny_data_type_definition *)LocalPoseTransformType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyLocalPoseType = (granny_data_type_definition *)LocalPoseType;
#include "granny_log.h"
GRANNY_DYNLINK(bool) GrannySetLogFileName(char const * FileName,
                                          bool Clear)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)SetLogFileName((char const *)FileName,
                                 (bool)Clear));
}

GRANNY_DYNLINK(void) GrannyGetLogCallback(granny_log_callback *  Result)
{
    GetLogCallback((log_callback * )Result);
}

GRANNY_DYNLINK(void) GrannySetLogCallback(granny_log_callback const * LogCallback)
{
    SetLogCallback((log_callback const &)*LogCallback);
}

GRANNY_DYNLINK(char const* ) GrannyGetLogMessageTypeString(granny_log_message_type Type)
{
    return ((char const* )GetLogMessageTypeString((log_message_type)Type));
}

GRANNY_DYNLINK(char const* ) GrannyGetLogMessageOriginString(granny_log_message_origin Origin)
{
    return ((char const* )GetLogMessageOriginString((log_message_origin)Origin));
}

GRANNY_DYNLINK(bool) GrannyLogging(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)Logging());
}

GRANNY_DYNLINK(void) GrannyFilterMessage(granny_log_message_origin Origin,
                                         bool Enabled)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FilterMessage((log_message_origin)Origin,
                  (bool)Enabled);
}

GRANNY_DYNLINK(void) GrannyFilterAllMessages(bool Enabled)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FilterAllMessages((bool)Enabled);
}

GRANNY_DYNLINK(granny_log_message_type) GrannyGetMostSeriousMessageType(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    log_message_type ReturnValue;
    return *(granny_log_message_type *)&(ReturnValue = GetMostSeriousMessageType());
}

GRANNY_DYNLINK(char const *) GrannyGetMostSeriousMessage(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((char const *)GetMostSeriousMessage());
}

GRANNY_DYNLINK(void) GrannyClearMostSeriousMessage(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ClearMostSeriousMessage();
}

#include "granny_math.h"
GRANNY_DYNLINK(void) GrannyEnsureQuaternionContinuity(granny_int32x QuaternionCount,
                                                      granny_real32 * Quaternions)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EnsureQuaternionContinuity((int32x)QuaternionCount,
                               (real32 *)Quaternions);
}

GRANNY_DYNLINK(bool) GrannyPolarDecompose(granny_real32 const * Source3x3,
                                          granny_real32 Tolerance,
                                          granny_real32 * Q3x3,
                                          granny_real32 * S3x3)
{
    return ((bool)PolarDecompose((real32 const *)Source3x3,
                                 (real32)Tolerance,
                                 (real32 *)Q3x3,
                                 (real32 *)S3x3));
}

GRANNY_DYNLINK(void) GrannyColumnMatrixMultiply4x3(granny_real32 * IntoMatrix4x4,
                                                   granny_real32 const * Matrix4x4,
                                                   granny_real32 const * ByMatrix4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ColumnMatrixMultiply4x3((real32 *)IntoMatrix4x4,
                            (real32 const *)Matrix4x4,
                            (real32 const *)ByMatrix4x4);
}

GRANNY_DYNLINK(void) GrannyColumnMatrixMultiply4x3Transpose(granny_real32 * IntoMatrix4x3,
                                                            granny_real32 const * Matrix4x4,
                                                            granny_real32 const * ByMatrix4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ColumnMatrixMultiply4x3Transpose((real32 *)IntoMatrix4x3,
                                     (real32 const *)Matrix4x4,
                                     (real32 const *)ByMatrix4x4);
}

GRANNY_DYNLINK(void) GrannyColumnMatrixMultiply4x4(granny_real32 * IntoMatrix4x4,
                                                   granny_real32 const * Matrix4x4,
                                                   granny_real32 const * ByMatrix4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ColumnMatrixMultiply4x4((real32 *)IntoMatrix4x4,
                            (real32 const *)Matrix4x4,
                            (real32 const *)ByMatrix4x4);
}

GRANNY_DYNLINK(void) GrannyMatrixEqualsQuaternion3x3(granny_real32 * Dest,
                                                     granny_real32 const * Quaternion)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    MatrixEqualsQuaternion3x3((real32 *)Dest,
                              (real32 const *)Quaternion);
}

GRANNY_DYNLINK(void) GrannyQuaternionEqualsMatrix3x3(granny_real32 * Quaternion,
                                                     granny_real32 const * Matrix)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    QuaternionEqualsMatrix3x3((real32 *)Quaternion,
                              (real32 const *)Matrix);
}

GRANNY_DYNLINK(void) GrannyInPlaceSimilarityTransformPosition(granny_real32 const * Affine3,
                                                              granny_real32 const * Linear3x3,
                                                              granny_real32 * Position3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InPlaceSimilarityTransformPosition((real32 const *)Affine3,
                                       (real32 const *)Linear3x3,
                                       (real32 *)Position3);
}

GRANNY_DYNLINK(void) GrannyInPlaceSimilarityTransformOrientation(granny_real32 const * Linear3x3,
                                                                 granny_real32 const * InverseLinear3x3,
                                                                 granny_real32 * Orientation4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InPlaceSimilarityTransformOrientation((real32 const *)Linear3x3,
                                          (real32 const *)InverseLinear3x3,
                                          (real32 *)Orientation4);
}

GRANNY_DYNLINK(void) GrannyInPlaceSimilarityTransformScaleShear(granny_real32 const * Linear3x3,
                                                                granny_real32 const * InverseLinear3x3,
                                                                granny_real32 * ScaleShear3x3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InPlaceSimilarityTransformScaleShear((real32 const *)Linear3x3,
                                         (real32 const *)InverseLinear3x3,
                                         (real32 *)ScaleShear3x3);
}

GRANNY_DYNLINK(void) GrannyInPlaceSimilarityTransform(granny_real32 const * Affine3,
                                                      granny_real32 const * Linear3x3,
                                                      granny_real32 const * InverseLinear3x3,
                                                      granny_real32 * Position3,
                                                      granny_real32 * Orientation4,
                                                      granny_real32 * ScaleShear3x3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InPlaceSimilarityTransform((real32 const *)Affine3,
                               (real32 const *)Linear3x3,
                               (real32 const *)InverseLinear3x3,
                               (real32 *)Position3,
                               (real32 *)Orientation4,
                               (real32 *)ScaleShear3x3);
}

GRANNY_DYNLINK(void) GrannyInPlaceSimilarityTransform4x3(granny_real32 const * Affine3,
                                                         granny_real32 const * Linear3x3,
                                                         granny_real32 const * InverseLinear3x3,
                                                         granny_real32 * Result4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InPlaceSimilarityTransform4x3((real32 const *)Affine3,
                                  (real32 const *)Linear3x3,
                                  (real32 const *)InverseLinear3x3,
                                  (real32 *)Result4x4);
}

#include "granny_memory.h"
GRANNY_DYNLINK(granny_allocation_header *) GrannyAllocationsBegin(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_allocation_header *)AllocationsBegin());
}

GRANNY_DYNLINK(granny_allocation_header *) GrannyNextAllocation(granny_allocation_header * Current)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_allocation_header *)NextAllocation((allocation_header *)Current));
}

GRANNY_DYNLINK(granny_allocation_header *) GrannyAllocationsEnd(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_allocation_header *)AllocationsEnd());
}

GRANNY_DYNLINK(void) GrannyGetAllocationInformation(granny_allocation_header const * Header,
                                                    granny_allocation_information * Information)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetAllocationInformation((allocation_header const *)Header,
                             (allocation_information &)*Information);
}

GRANNY_DYNLINK(void *) GrannyBeginAllocationCheck(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)BeginAllocationCheck());
}

GRANNY_DYNLINK(granny_allocation_header *) GrannyCheckedAllocationsEnd(void * CheckIdentifier)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_allocation_header *)CheckedAllocationsEnd((void *)CheckIdentifier));
}

GRANNY_DYNLINK(bool) GrannyEndAllocationCheck(void * CheckIdentifier)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndAllocationCheck((void *)CheckIdentifier));
}

GRANNY_DYNLINK(void) GrannyGetAllocator(granny_allocate_callback ** AllocateCallback,
                                        granny_deallocate_callback ** DeallocateCallback)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetAllocator((allocate_callback *&)*AllocateCallback,
                 (deallocate_callback *&)*DeallocateCallback);
}

GRANNY_DYNLINK(void) GrannySetAllocator(granny_allocate_callback * AllocateCallback,
                                        granny_deallocate_callback * DeallocateCallback)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetAllocator((allocate_callback *)AllocateCallback,
                 (deallocate_callback *)DeallocateCallback);
}

GRANNY_DYNLINK(void) GrannyFreeBuilderResult(void * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeBuilderResult((void *)Result);
}

GRANNY_DYNLINK(void) GrannyAcquireMemorySpinlock(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AcquireMemorySpinlock();
}

GRANNY_DYNLINK(void) GrannyReleaseMemorySpinlock(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ReleaseMemorySpinlock();
}

#include "granny_memory_arena.h"
GRANNY_DYNLINK(granny_memory_arena *) GrannyNewMemoryArena(void)
{
    return ((granny_memory_arena *)NewMemoryArena());
}

GRANNY_DYNLINK(void) GrannySetArenaAlignment(granny_memory_arena *  Arena,
                                             granny_int32x Align)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetArenaAlignment((memory_arena & )*Arena,
                      (int32x)Align);
}

GRANNY_DYNLINK(void) GrannyClearArena(granny_memory_arena * Arena)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ClearArena((memory_arena *)Arena);
}

GRANNY_DYNLINK(void) GrannyFreeMemoryArena(granny_memory_arena * Arena)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeMemoryArena((memory_arena *)Arena);
}

GRANNY_DYNLINK(void *) GrannyMemoryArenaPush(granny_memory_arena * Arena,
                                             granny_int32x Size)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)MemoryArenaPush((memory_arena &)*Arena,
                                    (int32x)Size));
}

#include "granny_memory_file_reader.h"
GRANNY_DYNLINK(granny_file_reader *) GrannyCreateMemoryFileReader(char const * SourceFileName,
                                                                  granny_int32x SourceLineNumber,
                                                                  granny_int32x Offset,
                                                                  granny_int32x MemorySize,
                                                                  void * Memory)
{
    return ((granny_file_reader *)CreateMemoryFileReader((char const *)SourceFileName,
                                                         (int32x)SourceLineNumber,
                                                         (int32x)Offset,
                                                         (int32x)MemorySize,
                                                         (void *)Memory));
}

#include "granny_memory_file_writer.h"
GRANNY_DYNLINK(granny_file_writer *) GrannyCreateMemoryFileWriter(char const * SourceFileName,
                                                                  granny_int32x SourceLineNumber,
                                                                  granny_int32x BlockSize)
{
    return ((granny_file_writer *)CreateMemoryFileWriter((char const *)SourceFileName,
                                                         (int32x)SourceLineNumber,
                                                         (int32x)BlockSize));
}

GRANNY_DYNLINK(bool) GrannyStealMemoryWriterBuffer(granny_file_writer * Writer,
                                                   granny_uint8 ** BufferPtr,
                                                   granny_int32x * BufferSize)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)StealMemoryWriterBuffer((file_writer &)*Writer,
                                          (uint8 *&)*BufferPtr,
                                          (int32x &)*BufferSize));
}

GRANNY_DYNLINK(void) GrannyFreeMemoryWriterBuffer(granny_uint8 *  Buffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeMemoryWriterBuffer((uint8 * )Buffer);
}

#include "granny_mesh.h"
GRANNY_DYNLINK(granny_int32x) GrannyGetMeshMorphTargetCount(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshMorphTargetCount((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshTriangleGroupCount(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshTriangleGroupCount((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_tri_material_group *) GrannyGetMeshTriangleGroups(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_tri_material_group *)GetMeshTriangleGroups((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_data_type_definition *) GrannyGetMeshVertexType(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_data_type_definition *)GetMeshVertexType((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_data_type_definition *) GrannyGetMeshMorphVertexType(granny_mesh const * Mesh,
                                                                           granny_int32x MorphTargetIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_data_type_definition *)GetMeshMorphVertexType((mesh const &)*Mesh,
                                                                  (int32x)MorphTargetIndex));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshVertexCount(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshVertexCount((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshMorphVertexCount(granny_mesh const * Mesh,
                                                            granny_int32x MorphTargetIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshMorphVertexCount((mesh const &)*Mesh,
                                                                     (int32x)MorphTargetIndex));
}

GRANNY_DYNLINK(void) GrannyCopyMeshVertices(granny_mesh const * Mesh,
                                            granny_data_type_definition const * VertexType,
                                            void * DestVertices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyMeshVertices((mesh const &)*Mesh,
                     (data_type_definition const *)VertexType,
                     (void *)DestVertices);
}

GRANNY_DYNLINK(void) GrannyCopyMeshMorphVertices(granny_mesh const * Mesh,
                                                 granny_int32x MorphTargetIndex,
                                                 granny_data_type_definition const * VertexType,
                                                 void * DestVertices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyMeshMorphVertices((mesh const &)*Mesh,
                          (int32x)MorphTargetIndex,
                          (data_type_definition const *)VertexType,
                          (void *)DestVertices);
}

GRANNY_DYNLINK(void *) GrannyGetMeshVertices(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)GetMeshVertices((mesh const &)*Mesh));
}

GRANNY_DYNLINK(void *) GrannyGetMeshMorphVertices(granny_mesh const * Mesh,
                                                  granny_int32x MorphTargetIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)GetMeshMorphVertices((mesh const &)*Mesh,
                                         (int32x)MorphTargetIndex));
}

GRANNY_DYNLINK(bool) GrannyMeshIsRigid(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)MeshIsRigid((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshIndexCount(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshIndexCount((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshTriangleCount(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshTriangleCount((mesh const &)*Mesh));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshBytesPerIndex(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshBytesPerIndex((mesh const &)*Mesh));
}

GRANNY_DYNLINK(void *) GrannyGetMeshIndices(granny_mesh const * Mesh)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)GetMeshIndices((mesh const &)*Mesh));
}

GRANNY_DYNLINK(void) GrannyCopyMeshIndices(granny_mesh const * Mesh,
                                           granny_int32x BytesPerIndex,
                                           void * DestIndices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    CopyMeshIndices((mesh const &)*Mesh,
                    (int32x)BytesPerIndex,
                    (void *)DestIndices);
}

GRANNY_DYNLINK(void) GrannyTransformBoundingBox(granny_real32 const * Affine3,
                                                granny_real32 const * Linear3x3,
                                                granny_real32 * OBBMin,
                                                granny_real32 * OBBMax)
{
    TransformBoundingBox((real32 const *)Affine3,
                         (real32 const *)Linear3x3,
                         (real32 *)OBBMin,
                         (real32 *)OBBMax);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyBoneBindingType = (granny_data_type_definition *)BoneBindingType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyMaterialBindingType = (granny_data_type_definition *)MaterialBindingType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyMorphTargetType = (granny_data_type_definition *)MorphTargetType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyMeshType = (granny_data_type_definition *)MeshType;
#include "granny_mesh_binding.h"
GRANNY_DYNLINK(granny_mesh_binding *) GrannyNewMeshBinding(granny_mesh const * Mesh,
                                                           granny_skeleton const * FromSkeleton,
                                                           granny_skeleton const * ToSkeleton)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_mesh_binding *)NewMeshBinding((mesh const &)*Mesh,
                                                  (skeleton const &)*FromSkeleton,
                                                  (skeleton const &)*ToSkeleton));
}

GRANNY_DYNLINK(void) GrannyFreeMeshBinding(granny_mesh_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeMeshBinding((mesh_binding *)Binding);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingMeshBindingSize(granny_mesh const * Mesh,
                                                                granny_skeleton const * FromSkeleton,
                                                                granny_skeleton const * ToSkeleton)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingMeshBindingSize((mesh const &)*Mesh,
                                                                         (skeleton const &)*FromSkeleton,
                                                                         (skeleton const &)*ToSkeleton));
}

GRANNY_DYNLINK(granny_mesh_binding *) GrannyNewMeshBindingInPlace(granny_mesh const * Mesh,
                                                                  granny_skeleton const * FromSkeleton,
                                                                  granny_skeleton const * ToSkeleton,
                                                                  void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_mesh_binding *)NewMeshBindingInPlace((mesh const &)*Mesh,
                                                         (skeleton const &)*FromSkeleton,
                                                         (skeleton const &)*ToSkeleton,
                                                         (void *)Memory));
}

GRANNY_DYNLINK(bool) GrannyMeshBindingIsTransferred(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)MeshBindingIsTransferred((mesh_binding const &)*Binding));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshBinding4x4ArraySize(granny_mesh_binding const * Binding,
                                                               granny_int32x BoneCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshBinding4x4ArraySize((mesh_binding const &)*Binding,
                                                                        (int32x)BoneCount));
}

GRANNY_DYNLINK(void) GrannyBuildMeshBinding4x4Array(granny_mesh_binding const * Binding,
                                                    granny_world_pose const * WorldPose,
                                                    granny_int32x FirstBoneIndex,
                                                    granny_int32x BoneCount,
                                                    granny_real32 * Matrix4x4Array)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildMeshBinding4x4Array((mesh_binding const &)*Binding,
                             (world_pose const &)*WorldPose,
                             (int32x)FirstBoneIndex,
                             (int32x)BoneCount,
                             (real32 *)Matrix4x4Array);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshBindingBoneCount(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMeshBindingBoneCount((mesh_binding const &)*Binding));
}

GRANNY_DYNLINK(granny_int32x const *) GrannyGetMeshBindingFromBoneIndices(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_int32x const *)GetMeshBindingFromBoneIndices((mesh_binding const &)*Binding));
}

GRANNY_DYNLINK(granny_skeleton *) GrannyGetMeshBindingFromSkeleton(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_skeleton *)GetMeshBindingFromSkeleton((mesh_binding const &)*Binding));
}

GRANNY_DYNLINK(granny_int32x const *) GrannyGetMeshBindingToBoneIndices(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_int32x const *)GetMeshBindingToBoneIndices((mesh_binding const &)*Binding));
}

GRANNY_DYNLINK(granny_skeleton *) GrannyGetMeshBindingToSkeleton(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_skeleton *)GetMeshBindingToSkeleton((mesh_binding const &)*Binding));
}

GRANNY_DYNLINK(granny_mesh *) GrannyGetMeshBindingSourceMesh(granny_mesh_binding const * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_mesh *)GetMeshBindingSourceMesh((mesh_binding const &)*Binding));
}

#include "granny_mesh_builder.h"
GRANNY_DYNLINK(granny_mesh_builder *) GrannyBeginMesh(granny_int32x VertexCount,
                                                      granny_int32x TriangleCount,
                                                      granny_int32x MaterialCount,
                                                      granny_int32x BoneCount,
                                                      granny_data_type_definition const * VertexType)
{
    return ((granny_mesh_builder *)BeginMesh((int32x)VertexCount,
                                             (int32x)TriangleCount,
                                             (int32x)MaterialCount,
                                             (int32x)BoneCount,
                                             (data_type_definition const *)VertexType));
}

GRANNY_DYNLINK(void) GrannyGenerateTangentSpaceFromUVs(granny_mesh_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GenerateTangentSpaceFromUVs((mesh_builder &)*Builder);
}

GRANNY_DYNLINK(bool) GrannyEndMesh(granny_mesh_builder * Builder,
                                   granny_vertex_data ** VertexData,
                                   granny_tri_topology ** Topology)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndMesh((mesh_builder *)Builder,
                          (vertex_data *&)*VertexData,
                          (tri_topology *&)*Topology));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingVertexCount(granny_mesh_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingVertexCount((mesh_builder const &)*Builder));
}

GRANNY_DYNLINK(void) GrannySerializeResultingCoincidentVertexMap(granny_mesh_builder const * Builder,
                                                                 granny_int32 * Dest)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SerializeResultingCoincidentVertexMap((mesh_builder const &)*Builder,
                                          (int32 *)Dest);
}

GRANNY_DYNLINK(void) GrannySerializeResultingVertexToTriangleMap(granny_mesh_builder const * Builder,
                                                                 granny_int32 * Dest)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SerializeResultingVertexToTriangleMap((mesh_builder const &)*Builder,
                                          (int32 *)Dest);
}

GRANNY_DYNLINK(void) GrannySerializeResultingVertices(granny_mesh_builder const * Builder,
                                                      void * Destination)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SerializeResultingVertices((mesh_builder const &)*Builder,
                               (void *)Destination);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingVertexDataSize(granny_mesh_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingVertexDataSize((mesh_builder const &)*Builder));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingTopologySize(granny_mesh_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingTopologySize((mesh_builder const &)*Builder));
}

GRANNY_DYNLINK(void) GrannyEndMeshInPlace(granny_mesh_builder * Builder,
                                          void * VertexDataMemory,
                                          granny_vertex_data ** VertexData,
                                          void * TopologyMemory,
                                          granny_tri_topology ** Topology)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndMeshInPlace((mesh_builder *)Builder,
                   (void *)VertexDataMemory,
                   (vertex_data *&)*VertexData,
                   (void *)TopologyMemory,
                   (tri_topology *&)*Topology);
}

GRANNY_DYNLINK(void) GrannySetNormalTolerance(granny_mesh_builder * Builder,
                                              granny_real32 Tolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetNormalTolerance((mesh_builder &)*Builder,
                       (real32)Tolerance);
}

GRANNY_DYNLINK(void) GrannySetTangentTolerance(granny_mesh_builder * Builder,
                                               granny_real32 Tolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTangentTolerance((mesh_builder &)*Builder,
                        (real32)Tolerance);
}

GRANNY_DYNLINK(void) GrannySetBinormalTolerance(granny_mesh_builder * Builder,
                                                granny_real32 Tolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBinormalTolerance((mesh_builder &)*Builder,
                         (real32)Tolerance);
}

GRANNY_DYNLINK(void) GrannySetTangentBinormalCrossTolerance(granny_mesh_builder * Builder,
                                                            granny_real32 Tolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTangentBinormalCrossTolerance((mesh_builder &)*Builder,
                                     (real32)Tolerance);
}

GRANNY_DYNLINK(void) GrannySetTangentMergingTolerance(granny_mesh_builder * Builder,
                                                      granny_real32 MinCosine)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTangentMergingTolerance((mesh_builder &)*Builder,
                               (real32)MinCosine);
}

GRANNY_DYNLINK(void) GrannySetChannelTolerance(granny_mesh_builder * Builder,
                                               granny_int32x Channel,
                                               granny_real32 Tolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetChannelTolerance((mesh_builder &)*Builder,
                        (int32x)Channel,
                        (real32)Tolerance);
}

GRANNY_DYNLINK(void) GrannySetVertexChannelComponentNames(granny_mesh_builder * Builder,
                                                          granny_int32x ChannelCount,
                                                          char const ** ChannelNames)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetVertexChannelComponentNames((mesh_builder &)*Builder,
                                   (int32x)ChannelCount,
                                   (char const **)ChannelNames);
}

GRANNY_DYNLINK(void) GrannySetPosition(granny_mesh_builder * Builder,
                                       granny_real32 Px,
                                       granny_real32 Py,
                                       granny_real32 Pz)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetPosition((mesh_builder &)*Builder,
                (real32)Px,
                (real32)Py,
                (real32)Pz);
}

GRANNY_DYNLINK(void) GrannyAddWeight(granny_mesh_builder * Builder,
                                     granny_int32x BoneIndex,
                                     granny_real32 Weight)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddWeight((mesh_builder &)*Builder,
              (int32x)BoneIndex,
              (real32)Weight);
}

GRANNY_DYNLINK(void) GrannyPushVertex(granny_mesh_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushVertex((mesh_builder &)*Builder);
}

GRANNY_DYNLINK(void) GrannySetVertexIndex(granny_mesh_builder * Builder,
                                          granny_int32x Edge,
                                          granny_int32x Index)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetVertexIndex((mesh_builder &)*Builder,
                   (int32x)Edge,
                   (int32x)Index);
}

GRANNY_DYNLINK(void) GrannySetNormal(granny_mesh_builder * Builder,
                                     granny_int32x Edge,
                                     granny_real32 Nx,
                                     granny_real32 Ny,
                                     granny_real32 Nz)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetNormal((mesh_builder &)*Builder,
              (int32x)Edge,
              (real32)Nx,
              (real32)Ny,
              (real32)Nz);
}

GRANNY_DYNLINK(void) GrannySetTangent(granny_mesh_builder * Builder,
                                      granny_int32x Edge,
                                      granny_real32 Tx,
                                      granny_real32 Ty,
                                      granny_real32 Tz)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTangent((mesh_builder &)*Builder,
               (int32x)Edge,
               (real32)Tx,
               (real32)Ty,
               (real32)Tz);
}

GRANNY_DYNLINK(void) GrannySetBinormal(granny_mesh_builder * Builder,
                                       granny_int32x Edge,
                                       granny_real32 Bx,
                                       granny_real32 By,
                                       granny_real32 Bz)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBinormal((mesh_builder &)*Builder,
                (int32x)Edge,
                (real32)Bx,
                (real32)By,
                (real32)Bz);
}

GRANNY_DYNLINK(void) GrannySetTangentBinormalCross(granny_mesh_builder * Builder,
                                                   granny_int32x Edge,
                                                   granny_real32 TBCx,
                                                   granny_real32 TBCy,
                                                   granny_real32 TBCz)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTangentBinormalCross((mesh_builder &)*Builder,
                            (int32x)Edge,
                            (real32)TBCx,
                            (real32)TBCy,
                            (real32)TBCz);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetChannelComponentCount(granny_mesh_builder * Builder,
                                                             granny_int32x ChannelIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetChannelComponentCount((mesh_builder &)*Builder,
                                                                      (int32x)ChannelIndex));
}

GRANNY_DYNLINK(void) GrannySetChannel(granny_mesh_builder * Builder,
                                      granny_int32x Edge,
                                      granny_int32x Channel,
                                      granny_real32 const * Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetChannel((mesh_builder &)*Builder,
               (int32x)Edge,
               (int32x)Channel,
               (real32 const *)Value);
}

GRANNY_DYNLINK(void) GrannySetMaterial(granny_mesh_builder * Builder,
                                       granny_int32x MaterialIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetMaterial((mesh_builder &)*Builder,
                (int32x)MaterialIndex);
}

GRANNY_DYNLINK(void) GrannyPushTriangle(granny_mesh_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushTriangle((mesh_builder &)*Builder);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetOriginalVertex(granny_mesh_builder * Builder,
                                                      granny_int32x V)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetOriginalVertex((mesh_builder &)*Builder,
                                                               (int32x)V));
}

#include "granny_mesh_deformer.h"
GRANNY_DYNLINK(granny_mesh_deformer *) GrannyNewMeshDeformer(granny_data_type_definition const * InputVertexLayout,
                                                             granny_data_type_definition const * OutputVertexLayout,
                                                             granny_deformation_type DeformationType,
                                                             granny_deformer_tail_flags TailFlag)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_mesh_deformer *)NewMeshDeformer((data_type_definition const *)InputVertexLayout,
                                                    (data_type_definition const *)OutputVertexLayout,
                                                    (deformation_type)DeformationType,
                                                    (deformer_tail_flags)TailFlag));
}

GRANNY_DYNLINK(void) GrannyFreeMeshDeformer(granny_mesh_deformer * Deformer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeMeshDeformer((mesh_deformer *)Deformer);
}

GRANNY_DYNLINK(void) GrannyDeformVertices(granny_mesh_deformer const * Deformer,
                                          granny_int32x const * MatrixIndices,
                                          granny_real32 const * MatrixBuffer4x4,
                                          granny_int32x VertexCount,
                                          void const * SourceVertices,
                                          void * DestVertices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    DeformVertices((mesh_deformer const &)*Deformer,
                   (int32x const *)MatrixIndices,
                   (real32 const *)MatrixBuffer4x4,
                   (int32x)VertexCount,
                   (void const *)SourceVertices,
                   (void *)DestVertices);
}

#include "granny_oodle1_compression.h"
GRANNY_DYNLINK(granny_oodle1_state *) GrannyOodle1BeginSimple(granny_uint32x ExpandedDataSize,
                                                              granny_int32x BufferCount)
{
    return ((granny_oodle1_state *)Oodle1BeginSimple((uint32x)ExpandedDataSize,
                                                     (int32x)BufferCount));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetOodle1CompressBufferPaddingSize(void)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetOodle1CompressBufferPaddingSize());
}

GRANNY_DYNLINK(void) GrannyOodle1Compress(granny_oodle1_state * Oodle1State,
                                          granny_int32x BufferSize,
                                          void const * Buffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    Oodle1Compress((oodle1_state &)*Oodle1State,
                   (int32x)BufferSize,
                   (void const *)Buffer);
}

GRANNY_DYNLINK(granny_int32x) GrannyOodle1End(granny_oodle1_state * Oodle1State,
                                              void ** Buffer,
                                              bool WritingForReversedPlatform)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = Oodle1End((oodle1_state &)*Oodle1State,
                                                       (void *&)*Buffer,
                                                       (bool)WritingForReversedPlatform));
}

GRANNY_DYNLINK(void) GrannyOodle1FreeSimple(granny_oodle1_state * Oodle1State)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    Oodle1FreeSimple((oodle1_state &)*Oodle1State);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetOodle1DecompressBufferPaddingSize(void)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetOodle1DecompressBufferPaddingSize());
}

GRANNY_DYNLINK(bool) GrannyOodle1Decompress(bool FileIsByteReversed,
                                            granny_int32x CompressedBytesSize,
                                            void const * CompressedBytes,
                                            granny_int32x DecompressedSize,
                                            void * DecompressedBytes)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)Oodle1Decompress((bool)FileIsByteReversed,
                                   (int32x)CompressedBytesSize,
                                   (void const *)CompressedBytes,
                                   (int32x)DecompressedSize,
                                   (void *)DecompressedBytes));
}

#include "granny_periodic_loop.h"
GRANNY_DYNLINK(void) GrannyZeroPeriodicLoop(granny_periodic_loop * Loop)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ZeroPeriodicLoop((periodic_loop &)*Loop);
}

GRANNY_DYNLINK(void) GrannyFitPeriodicLoop(granny_real32 const * StartPosition3,
                                           granny_real32 const * StartOrientation4,
                                           granny_real32 const * EndPosition3,
                                           granny_real32 const * EndOrientation4,
                                           granny_real32 Seconds,
                                           granny_periodic_loop * Loop)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FitPeriodicLoop((real32 const *)StartPosition3,
                    (real32 const *)StartOrientation4,
                    (real32 const *)EndPosition3,
                    (real32 const *)EndOrientation4,
                    (real32)Seconds,
                    (periodic_loop &)*Loop);
}

GRANNY_DYNLINK(void) GrannyComputePeriodicLoopVector(granny_periodic_loop const * Loop,
                                                     granny_real32 Seconds,
                                                     granny_real32 * Result3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ComputePeriodicLoopVector((periodic_loop const &)*Loop,
                              (real32)Seconds,
                              (real32 *)Result3);
}

GRANNY_DYNLINK(void) GrannyComputePeriodicLoopLog(granny_periodic_loop const * Loop,
                                                  granny_real32 Seconds,
                                                  granny_real32 * Result4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ComputePeriodicLoopLog((periodic_loop const &)*Loop,
                           (real32)Seconds,
                           (real32 *)Result4);
}

GRANNY_DYNLINK(void) GrannyStepPeriodicLoop(granny_periodic_loop const * Loop,
                                            granny_real32 Seconds,
                                            granny_real32 * Position3,
                                            granny_real32 * Orientation4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    StepPeriodicLoop((periodic_loop const &)*Loop,
                     (real32)Seconds,
                     (real32 *)Position3,
                     (real32 *)Orientation4);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPeriodicLoopType = (granny_data_type_definition *)PeriodicLoopType;
#include "granny_pointer_hash.h"
GRANNY_DYNLINK(granny_pointer_hash *) GrannyNewPointerHash(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_pointer_hash *)NewPointerHash());
}

GRANNY_DYNLINK(void) GrannyDeletePointerHash(granny_pointer_hash * Hash)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    DeletePointerHash((pointer_hash *)Hash);
}

GRANNY_DYNLINK(void) GrannyAddPointerToHash(granny_pointer_hash * Hash,
                                            void const * Key,
                                            void * Data)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddPointerToHash((pointer_hash &)*Hash,
                     (void const *)Key,
                     (void *)Data);
}

GRANNY_DYNLINK(bool) GrannySetHashedPointerData(granny_pointer_hash * Hash,
                                                void const * Key,
                                                void * Data)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)SetHashedPointerData((pointer_hash &)*Hash,
                                       (void const *)Key,
                                       (void *)Data));
}

GRANNY_DYNLINK(bool) GrannyGetHashedPointerData(granny_pointer_hash * Hash,
                                                void const * Key,
                                                void ** Data)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)GetHashedPointerData((pointer_hash &)*Hash,
                                       (void const *)Key,
                                       (void *&)*Data));
}

GRANNY_DYNLINK(bool) GrannyHashedPointerKeyExists(granny_pointer_hash * Hash,
                                                  void const * Key)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)HashedPointerKeyExists((pointer_hash &)*Hash,
                                         (void const *)Key));
}

#include "granny_s3tc.h"
GRANNY_DYNLINK(granny_pixel_layout const *) GrannyGetS3TCPixelLayout(granny_s3tc_texture_format Format)
{
    return ((granny_pixel_layout const *)&GetS3TCPixelLayout((s3tc_texture_format)Format));
}

GRANNY_DYNLINK(char const *) GrannyGetS3TCTextureFormatName(granny_int32x Format)
{
    return ((char const *)GetS3TCTextureFormatName((int32x)Format));
}

#include "granny_skeleton.h"
GRANNY_DYNLINK(void) GrannyBuildSkeletonRelativeTransform(granny_int32x SourceTransformStride,
                                                          granny_transform const * SourceTransforms,
                                                          granny_int32x SourceParentStride,
                                                          granny_int32 const * SourceParents,
                                                          granny_int32x BoneIndex,
                                                          granny_transform * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildSkeletonRelativeTransform((int32x)SourceTransformStride,
                                   (transform const *)SourceTransforms,
                                   (int32x)SourceParentStride,
                                   (int32 const *)SourceParents,
                                   (int32x)BoneIndex,
                                   (transform &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildSkeletonRelativeTransforms(granny_int32x SourceTransformStride,
                                                           granny_transform const * SourceTransforms,
                                                           granny_int32x SourceParentStride,
                                                           granny_int32 const * SourceParents,
                                                           granny_int32x Count,
                                                           granny_int32x ResultStride,
                                                           granny_transform * Results)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildSkeletonRelativeTransforms((int32x)SourceTransformStride,
                                    (transform const *)SourceTransforms,
                                    (int32x)SourceParentStride,
                                    (int32 const *)SourceParents,
                                    (int32x)Count,
                                    (int32x)ResultStride,
                                    (transform *)Results);
}

GRANNY_DYNLINK(bool) GrannyFindBoneByName(granny_skeleton const * Skeleton,
                                          char const * BoneName,
                                          granny_int32x * BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindBoneByName((skeleton const *)Skeleton,
                                 (char const *)BoneName,
                                 (int32x &)*BoneIndex));
}

GRANNY_DYNLINK(bool) GrannyFindBoneByNameLowercase(granny_skeleton const * Skeleton,
                                                   char const * BoneName,
                                                   granny_int32x * BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindBoneByNameLowercase((skeleton const *)Skeleton,
                                          (char const *)BoneName,
                                          (int32x &)*BoneIndex));
}

GRANNY_DYNLINK(bool) GrannyFindBoneByRule(granny_skeleton const*  Skeleton,
                                          char const * ProcessedBoneName,
                                          char const * BonePattern,
                                          granny_int32x * BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindBoneByRule((skeleton const* )Skeleton,
                                 (char const *)ProcessedBoneName,
                                 (char const *)BonePattern,
                                 (int32x &)*BoneIndex));
}

GRANNY_DYNLINK(int) GrannyGetBoneCountForLOD(granny_skeleton const*  Skeleton,
                                             granny_real32 AllowedError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((int)GetBoneCountForLOD((skeleton const* )Skeleton,
                                    (real32)AllowedError));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyBoneType = (granny_data_type_definition *)BoneType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannySkeletonType = (granny_data_type_definition *)SkeletonType;
#include "granny_skeleton_builder.h"
GRANNY_DYNLINK(granny_skeleton_builder *) GrannyBeginSkeleton(granny_int32x BoneCount)
{
    return ((granny_skeleton_builder *)BeginSkeleton((int32x)BoneCount));
}

GRANNY_DYNLINK(granny_skeleton *) GrannyEndSkeleton(granny_skeleton_builder * Builder,
                                                    granny_int32x * RemappingTable)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_skeleton *)EndSkeleton((skeleton_builder *)Builder,
                                           (int32x *)RemappingTable));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingSkeletonSize(granny_skeleton_builder const * Builder)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingSkeletonSize((skeleton_builder const &)*Builder));
}

GRANNY_DYNLINK(granny_skeleton *) GrannyEndSkeletonInPlace(granny_skeleton_builder * Builder,
                                                           void * InMemory,
                                                           granny_int32x * RemappingTable)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_skeleton *)EndSkeletonInPlace((skeleton_builder *)Builder,
                                                  (void *)InMemory,
                                                  (int32x *)RemappingTable));
}

GRANNY_DYNLINK(void) GrannyAddBone(granny_skeleton_builder * Builder,
                                   granny_real32 const * LocalPosition3,
                                   granny_real32 const * LocalOrientation4,
                                   granny_real32 const * LocalScaleShear3x3,
                                   granny_real32 const * WorldPosition3,
                                   granny_real32 const * WorldOrientation4,
                                   granny_real32 const * WorldScaleShear3x3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddBone((skeleton_builder &)*Builder,
            (real32 const *)LocalPosition3,
            (real32 const *)LocalOrientation4,
            (real32 const *)LocalScaleShear3x3,
            (real32 const *)WorldPosition3,
            (real32 const *)WorldOrientation4,
            (real32 const *)WorldScaleShear3x3);
}

GRANNY_DYNLINK(void) GrannyAddBoneWithInverse(granny_skeleton_builder * Builder,
                                              granny_real32 const * LocalPosition3,
                                              granny_real32 const * LocalOrientation4,
                                              granny_real32 const * LocalScaleShear3x3,
                                              granny_matrix_4x4 const*  InverseWorld4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddBoneWithInverse((skeleton_builder &)*Builder,
                       (real32 const *)LocalPosition3,
                       (real32 const *)LocalOrientation4,
                       (real32 const *)LocalScaleShear3x3,
                       (matrix_4x4 const* )InverseWorld4x4);
}

GRANNY_DYNLINK(void) GrannySetBoneParent(granny_skeleton_builder * Builder,
                                         granny_int32x BoneIndex,
                                         granny_int32x ParentIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBoneParent((skeleton_builder &)*Builder,
                  (int32x)BoneIndex,
                  (int32x)ParentIndex);
}

GRANNY_DYNLINK(void) GrannySetBoneLODError(granny_skeleton_builder * Builder,
                                           granny_int32x BoneIndex,
                                           granny_real32 LODError)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetBoneLODError((skeleton_builder &)*Builder,
                    (int32x)BoneIndex,
                    (real32)LODError);
}

#include "granny_spu_animation.h"
GRANNY_DYNLINK(bool) GrannyFindSPUTrackGroupForModel(granny_spu_animation const*  Animation,
                                                     char const * ModelName,
                                                     granny_int32x * TrackGroupIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindSPUTrackGroupForModel((spu_animation const& )*Animation,
                                            (char const *)ModelName,
                                            (int32x &)*TrackGroupIndex));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannySPUAnimationType = (granny_data_type_definition *)SPUAnimationType;
#include "granny_spu_animation_binding.h"
GRANNY_DYNLINK(void) GrannyMakeDefaultSPUAnimationBindingID(granny_spu_animation_binding_id * ID,
                                                            granny_spu_animation const*  Animation,
                                                            granny_int32x TrackGroupIndex,
                                                            granny_model const*  Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    MakeDefaultSPUAnimationBindingID((spu_animation_binding_id &)*ID,
                                     (spu_animation const* )Animation,
                                     (int32x)TrackGroupIndex,
                                     (model const* )Model);
}

GRANNY_DYNLINK(granny_spu_animation_binding *) GrannyAcquireSPUAnimationBindingFromID(granny_spu_animation_binding_id * ID)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_spu_animation_binding *)AcquireSPUAnimationBindingFromID((spu_animation_binding_id &)*ID));
}

GRANNY_DYNLINK(granny_spu_animation_binding *) GrannyAcquireSPUAnimationBinding(granny_spu_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_spu_animation_binding *)AcquireSPUAnimationBinding((spu_animation_binding *)Binding));
}

GRANNY_DYNLINK(void) GrannyReleaseSPUAnimationBinding(granny_spu_animation_binding * Binding)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ReleaseSPUAnimationBinding((spu_animation_binding *)Binding);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMaximumSPUAnimationBindingCount(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetMaximumSPUAnimationBindingCount());
}

GRANNY_DYNLINK(void) GrannySetMaximumSPUAnimationBindingCount(granny_int32x BindingCountMax)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetMaximumSPUAnimationBindingCount((int32x)BindingCountMax);
}

#include "granny_spu_animation_info.h"
GRANNY_DYNLINK(granny_spu_animation_info * ) GrannyGetSPUAnimationInfo(granny_file * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_spu_animation_info * )GetSPUAnimationInfo((file &)*File));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannySPUAnimationInfoType = (granny_data_type_definition *)SPUAnimationInfoType;
#include "granny_spu_controlled_animation.h"
GRANNY_DYNLINK(granny_control *) GrannyPlayControlledSPUAnimation(granny_real32 StartTime,
                                                                  granny_spu_animation const * Animation,
                                                                  granny_model_instance * Model)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_control *)PlayControlledSPUAnimation((real32)StartTime,
                                                         (spu_animation const &)*Animation,
                                                         (model_instance &)*Model));
}

#include "granny_spu_track_group.h"
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannySPUTransformTrackType = (granny_data_type_definition *)SPUTransformTrackType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannySPUTrackGroupType = (granny_data_type_definition *)SPUTrackGroupType;
#include "granny_stack_allocator.h"
GRANNY_DYNLINK(void) GrannyStackInitialize(granny_stack_allocator * Allocator,
                                           granny_int32x UnitSize,
                                           granny_int32x UnitsPerBlock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    StackInitialize((stack_allocator &)*Allocator,
                    (int32x)UnitSize,
                    (int32x)UnitsPerBlock);
}

GRANNY_DYNLINK(void) GrannyStackInitializeWithDirectory(granny_stack_allocator * Allocator,
                                                        granny_int32x UnitSize,
                                                        granny_int32x UnitsPerBlock,
                                                        granny_int32x MaxUnits)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    StackInitializeWithDirectory((stack_allocator &)*Allocator,
                                 (int32x)UnitSize,
                                 (int32x)UnitsPerBlock,
                                 (int32x)MaxUnits);
}

GRANNY_DYNLINK(void) GrannyStackCleanUp(granny_stack_allocator * Allocator)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    StackCleanUp((stack_allocator &)*Allocator);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetStackUnitCount(granny_stack_allocator const * Allocator)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetStackUnitCount((stack_allocator const &)*Allocator));
}

GRANNY_DYNLINK(void * ) GrannyNewStackUnit(granny_stack_allocator * Allocator,
                                           granny_int32x * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void * )NewStackUnit((stack_allocator &)*Allocator,
                                  (int32x &)*Result));
}

GRANNY_DYNLINK(void *) GrannyGetStackUnit(granny_stack_allocator * Allocator,
                                          granny_int32x Index)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((void *)GetStackUnit((stack_allocator &)*Allocator,
                                 (int32x)Index));
}

GRANNY_DYNLINK(void) GrannyPopStackUnits(granny_stack_allocator * Allocator,
                                         granny_int32x UnitCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PopStackUnits((stack_allocator &)*Allocator,
                  (int32x)UnitCount);
}

GRANNY_DYNLINK(bool) GrannyMultipleNewStackUnits(granny_stack_allocator * Allocator,
                                                 granny_int32x NumNewIndices,
                                                 granny_int32x * NewIndicesStart,
                                                 void const*  InitialValue)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)MultipleNewStackUnits((stack_allocator &)*Allocator,
                                        (int32x)NumNewIndices,
                                        (int32x &)*NewIndicesStart,
                                        (void const* )InitialValue));
}

GRANNY_DYNLINK(void) GrannySerializeStack(granny_stack_allocator const * Allocator,
                                          void * Dest)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SerializeStack((stack_allocator const &)*Allocator,
                   (void *)Dest);
}

#include "granny_statistics.h"
GRANNY_DYNLINK(granny_int32x) GrannyGetCounterCount(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetCounterCount());
}

GRANNY_DYNLINK(granny_real64x) GrannyGetCounterTicksPerSecond(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real64x ReturnValue;
    return *(granny_real64x *)&(ReturnValue = GetCounterTicksPerSecond());
}

GRANNY_DYNLINK(void) GrannyResetCounters(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResetCounters();
}

GRANNY_DYNLINK(void) GrannyResetCounterPeaks(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResetCounterPeaks();
}

GRANNY_DYNLINK(void) GrannyGetCounterResults(granny_int32x CounterIndex,
                                             granny_counter_results * Results)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetCounterResults((int32x)CounterIndex,
                      (counter_results &)*Results);
}

#include "granny_stat_hud.h"
GRANNY_DYNLINK(granny_stat_hud *) GrannyCaptureCurrentStats(granny_int32x FramesSinceLastCapture)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_stat_hud *)CaptureCurrentStats((int32x)FramesSinceLastCapture));
}

GRANNY_DYNLINK(void) GrannyFreeStats(granny_stat_hud * StatHUD)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeStats((stat_hud *)StatHUD);
}

GRANNY_DYNLINK(char **) GrannyDumpStatHUD(granny_stat_hud * StatHUD)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((char **)DumpStatHUD((stat_hud &)*StatHUD));
}

GRANNY_DYNLINK(void) GrannyFreeStatHUDDump(char ** StatHUDDump)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeStatHUDDump((char **)StatHUDDump);
}

#include "granny_string.h"
GRANNY_DYNLINK(granny_int32x) GrannyStringDifference(char const * StringA,
                                                     char const * StringB)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = StringDifference((char const *)StringA,
                                                              (char const *)StringB));
}

GRANNY_DYNLINK(void) GrannySetStringComparisonCallback(granny_string_comparison_callback * Callback)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetStringComparisonCallback((string_comparison_callback *)Callback);
}

#include "granny_string_database.h"
GRANNY_DYNLINK(granny_string_database *) GrannyGetStringDatabase(granny_file * File)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_string_database *)GetStringDatabase((file &)*File));
}

GRANNY_DYNLINK(bool) GrannyRemapFileStrings(granny_file *  File,
                                            granny_string_database *  StringDatabase)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)RemapFileStrings((file & )*File,
                                   (string_database & )*StringDatabase));
}

GRANNY_DYNLINK(char *) GrannyRebaseToStringDatabase(void * Data,
                                                    granny_uint32 Identifier)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((char *)RebaseToStringDatabase((void *)Data,
                                           (uint32)Identifier));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyStringDatabaseType = (granny_data_type_definition *)StringDatabaseType;
#include "granny_string_table.h"
GRANNY_DYNLINK(granny_string_table *) GrannyNewStringTable(void)
{
    return ((granny_string_table *)NewStringTable());
}

GRANNY_DYNLINK(void) GrannyFreeStringTable(granny_string_table * Table)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeStringTable((string_table *)Table);
}

GRANNY_DYNLINK(char const *) GrannyMapString(granny_string_table * Table,
                                             char const * String)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((char const *)MapString((string_table &)*Table,
                                    (char const *)String));
}

#include "granny_system_clock.h"
GRANNY_DYNLINK(void) GrannyGetSystemSeconds(granny_system_clock *  Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyGetAllowGlobalStateChanges())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: global state modifications not enabled",
                                    LogCallback.UserData);
        }
    }
    else if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetSystemSeconds((system_clock * )Result);
}

GRANNY_DYNLINK(granny_real32) GrannyGetSecondsElapsed(granny_system_clock const * StartClock,
                                                      granny_system_clock const * EndClock)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetSecondsElapsed((system_clock const &)*StartClock,
                                                               (system_clock const &)*EndClock));
}

GRANNY_DYNLINK(void) GrannySleepForSeconds(granny_real32 Seconds)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SleepForSeconds((real32)Seconds);
}

#include "granny_tangent_frame.h"
GRANNY_DYNLINK(bool) GrannyBuildTangentSpace(granny_mesh_builder *  Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)BuildTangentSpace((mesh_builder & )*Builder));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyGBX333VertexType = (granny_data_type_definition *)GBX333VertexType;
#include "granny_texture.h"
GRANNY_DYNLINK(char const *) GrannyGetTextureTypeName(granny_int32x TextureType)
{
    return ((char const *)GetTextureTypeName((int32x)TextureType));
}

GRANNY_DYNLINK(char const *) GrannyGetTextureEncodingName(granny_int32x Encoding)
{
    return ((char const *)GetTextureEncodingName((int32x)Encoding));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetRawImageSize(granny_pixel_layout const * Layout,
                                                    granny_int32x Stride,
                                                    granny_int32x Width,
                                                    granny_int32x Height)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetRawImageSize((pixel_layout const &)*Layout,
                                                             (int32x)Stride,
                                                             (int32x)Width,
                                                             (int32x)Height));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetS3TCImageSize(granny_s3tc_texture_format Format,
                                                     granny_int32x Width,
                                                     granny_int32x Height)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetS3TCImageSize((s3tc_texture_format)Format,
                                                              (int32x)Width,
                                                              (int32x)Height));
}

GRANNY_DYNLINK(bool) GrannyGetRecommendedPixelLayout(granny_texture const * Texture,
                                                     granny_pixel_layout * Layout)
{
    return ((bool)GetRecommendedPixelLayout((texture const &)*Texture,
                                            (pixel_layout &)*Layout));
}

GRANNY_DYNLINK(bool) GrannyCopyTextureImage(granny_texture const * Texture,
                                            granny_int32x ImageIndex,
                                            granny_int32x MIPIndex,
                                            granny_pixel_layout const * Layout,
                                            granny_int32x DestWidth,
                                            granny_int32x DestHeight,
                                            granny_int32x DestStride,
                                            void * Pixels)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CopyTextureImage((texture const &)*Texture,
                                   (int32x)ImageIndex,
                                   (int32x)MIPIndex,
                                   (pixel_layout const &)*Layout,
                                   (int32x)DestWidth,
                                   (int32x)DestHeight,
                                   (int32x)DestStride,
                                   (void *)Pixels));
}

GRANNY_DYNLINK(bool) GrannyTextureHasAlpha(granny_texture const * Texture)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)TextureHasAlpha((texture const &)*Texture));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTextureMIPLevelType = (granny_data_type_definition *)TextureMIPLevelType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTextureImageType = (granny_data_type_definition *)TextureImageType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTextureType = (granny_data_type_definition *)TextureType;
#include "granny_texture_builder.h"
GRANNY_DYNLINK(granny_texture_builder *) GrannyBeginRawTexture(granny_int32x Width,
                                                               granny_int32x Height,
                                                               granny_pixel_layout const * ResultingLayout,
                                                               granny_int32x StrideAlignment)
{
    return ((granny_texture_builder *)BeginRawTexture((int32x)Width,
                                                      (int32x)Height,
                                                      (pixel_layout const &)*ResultingLayout,
                                                      (int32x)StrideAlignment));
}

GRANNY_DYNLINK(granny_texture_builder *) GrannyBeginS3TCTexture(granny_int32x Width,
                                                                granny_int32x Height,
                                                                granny_s3tc_texture_format Format)
{
    return ((granny_texture_builder *)BeginS3TCTexture((int32x)Width,
                                                       (int32x)Height,
                                                       (s3tc_texture_format)Format));
}

GRANNY_DYNLINK(granny_texture_builder *) GrannyBeginBestMatchS3TCTexture(granny_int32x Width,
                                                                         granny_int32x Height)
{
    return ((granny_texture_builder *)BeginBestMatchS3TCTexture((int32x)Width,
                                                                (int32x)Height));
}

GRANNY_DYNLINK(granny_texture_builder *) GrannyBeginBinkTexture(granny_int32x Width,
                                                                granny_int32x Height,
                                                                granny_int32x Compression,
                                                                granny_uint32x Flags)
{
    return ((granny_texture_builder *)BeginBinkTexture((int32x)Width,
                                                       (int32x)Height,
                                                       (int32x)Compression,
                                                       (uint32x)Flags));
}

GRANNY_DYNLINK(granny_texture *) GrannyEndTexture(granny_texture_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_texture *)EndTexture((texture_builder *)Builder));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingTextureSize(granny_texture_builder const * Builder)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingTextureSize((texture_builder const &)*Builder));
}

GRANNY_DYNLINK(granny_texture *) GrannyEndTextureInPlace(granny_texture_builder * Builder,
                                                         void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_texture *)EndTextureInPlace((texture_builder *)Builder,
                                                (void *)Memory));
}

GRANNY_DYNLINK(void) GrannySetImageScalingFilter(granny_texture_builder * Builder,
                                                 granny_pixel_filter_type UpsamplingFilter,
                                                 granny_pixel_filter_type DownsamplingFilter)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetImageScalingFilter((texture_builder &)*Builder,
                          (pixel_filter_type)UpsamplingFilter,
                          (pixel_filter_type)DownsamplingFilter);
}

GRANNY_DYNLINK(void) GrannyEncodeImage(granny_texture_builder * Builder,
                                       granny_int32x Width,
                                       granny_int32x Height,
                                       granny_int32x Stride,
                                       granny_int32x MIPLevelCount,
                                       void const * RGBAData)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EncodeImage((texture_builder &)*Builder,
                (int32x)Width,
                (int32x)Height,
                (int32x)Stride,
                (int32x)MIPLevelCount,
                (void const *)RGBAData);
}

#include "granny_thread_safety.h"
GRANNY_DYNLINK(void) GrannySetAllowGlobalStateChanges(bool Allowed)
{
    SetAllowGlobalStateChanges((bool)Allowed);
}

GRANNY_DYNLINK(bool) GrannyGetAllowGlobalStateChanges(void)
{
    return ((bool)GetAllowGlobalStateChanges());
}

GRANNY_DYNLINK(granny_thread_id_callback *) GrannySetThreadIDCallback(granny_thread_id_callback * Callback)
{
    return ((granny_thread_id_callback *)SetThreadIDCallback((thread_id_callback *)Callback));
}

GRANNY_DYNLINK(bool) GrannyThreadAllowedToCallGranny(void)
{
    return ((bool)ThreadAllowedToCallGranny());
}

#include "granny_track_group.h"
GRANNY_DYNLINK(void) GrannyGetTrackGroupInitialPlacement4x4(granny_track_group * TrackGroup,
                                                            granny_real32 * Placement4x4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetTrackGroupInitialPlacement4x4((track_group &)*TrackGroup,
                                     (real32 *)Placement4x4);
}

GRANNY_DYNLINK(void) GrannyTransformCurve3(granny_transform const * Transform,
                                           granny_int32x Count,
                                           granny_real32 * Curve3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformCurve3((transform const &)*Transform,
                    (int32x)Count,
                    (real32 *)Curve3);
}

GRANNY_DYNLINK(void) GrannyTransformCurve4(granny_transform const * Transform,
                                           granny_int32x Count,
                                           granny_real32 * Curve4)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformCurve4((transform const &)*Transform,
                    (int32x)Count,
                    (real32 *)Curve4);
}

GRANNY_DYNLINK(void) GrannyTransformCurve3x3(granny_transform const * Transform,
                                             granny_int32x Count,
                                             granny_real32 * Curve3x3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformCurve3x3((transform const &)*Transform,
                      (int32x)Count,
                      (real32 *)Curve3x3);
}

GRANNY_DYNLINK(void) GrannyGetTrackInitialTransform(granny_transform_track const * Track,
                                                    granny_transform * Transform)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetTrackInitialTransform((transform_track const &)*Track,
                             (transform &)*Transform);
}

GRANNY_DYNLINK(void) GrannyRemoveTrackInitialTransform(granny_transform_track * Track)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RemoveTrackInitialTransform((transform_track &)*Track);
}

GRANNY_DYNLINK(bool) GrannyBasisConversionRequiresCurveDecompression(granny_real32 * Affine3,
                                                                     granny_real32 * Linear3x3,
                                                                     granny_real32 * InverseLinear3x3,
                                                                     granny_real32 AffineTolerance,
                                                                     granny_real32 LinearTolerance,
                                                                     bool RoundToTolerance)
{
    return ((bool)BasisConversionRequiresCurveDecompression((real32 *)Affine3,
                                                            (real32 *)Linear3x3,
                                                            (real32 *)InverseLinear3x3,
                                                            (real32)AffineTolerance,
                                                            (real32)LinearTolerance,
                                                            (bool)RoundToTolerance));
}

GRANNY_DYNLINK(void) GrannyTransformCurveVec3(granny_real32 const * Affine3,
                                              granny_real32 const * Linear3x3,
                                              granny_real32 AffineTolerance,
                                              granny_real32 LinearTolerance,
                                              granny_curve2 * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformCurveVec3((real32 const *)Affine3,
                       (real32 const *)Linear3x3,
                       (real32)AffineTolerance,
                       (real32)LinearTolerance,
                       (curve2 *)Curve);
}

GRANNY_DYNLINK(void) GrannySimilarityTransformCurvePosition(granny_real32 const * Affine3,
                                                            granny_real32 const * Linear3x3,
                                                            granny_real32 const * InverseLinear3x3,
                                                            granny_real32 AffineTolerance,
                                                            granny_real32 LinearTolerance,
                                                            granny_curve2 * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SimilarityTransformCurvePosition((real32 const *)Affine3,
                                     (real32 const *)Linear3x3,
                                     (real32 const *)InverseLinear3x3,
                                     (real32)AffineTolerance,
                                     (real32)LinearTolerance,
                                     (curve2 *)Curve);
}

GRANNY_DYNLINK(void) GrannySimilarityTransformCurveQuaternion(granny_real32 const * Affine3,
                                                              granny_real32 const * Linear3x3,
                                                              granny_real32 const * InverseLinear3x3,
                                                              granny_real32 AffineTolerance,
                                                              granny_real32 LinearTolerance,
                                                              granny_curve2 * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SimilarityTransformCurveQuaternion((real32 const *)Affine3,
                                       (real32 const *)Linear3x3,
                                       (real32 const *)InverseLinear3x3,
                                       (real32)AffineTolerance,
                                       (real32)LinearTolerance,
                                       (curve2 *)Curve);
}

GRANNY_DYNLINK(void) GrannySimilarityTransformCurveScaleShear(granny_real32 const * Affine3,
                                                              granny_real32 const * Linear3x3,
                                                              granny_real32 const * InverseLinear3x3,
                                                              granny_real32 AffineTolerance,
                                                              granny_real32 LinearTolerance,
                                                              granny_curve2 * Curve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SimilarityTransformCurveScaleShear((real32 const *)Affine3,
                                       (real32 const *)Linear3x3,
                                       (real32 const *)InverseLinear3x3,
                                       (real32)AffineTolerance,
                                       (real32)LinearTolerance,
                                       (curve2 *)Curve);
}

GRANNY_DYNLINK(void) GrannySimilarityTransformTrackGroup(granny_track_group * TrackGroup,
                                                         granny_real32 const * Affine3,
                                                         granny_real32 const * Linear3x3,
                                                         granny_real32 const * InverseLinear3x3,
                                                         granny_real32 AffineTolerance,
                                                         granny_real32 LinearTolerance)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SimilarityTransformTrackGroup((track_group &)*TrackGroup,
                                  (real32 const *)Affine3,
                                  (real32 const *)Linear3x3,
                                  (real32 const *)InverseLinear3x3,
                                  (real32)AffineTolerance,
                                  (real32)LinearTolerance);
}

GRANNY_DYNLINK(void) GrannyGetVectorDifferences(granny_int32x VectorDimension,
                                                granny_int32x VectorCount,
                                                granny_real32 const * Vectors,
                                                granny_real32 const * Identity,
                                                granny_vector_diff_mode DiffMode,
                                                granny_real32 * IdentityDifference,
                                                granny_real32 * ConstantDifference)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetVectorDifferences((int32x)VectorDimension,
                         (int32x)VectorCount,
                         (real32 const *)Vectors,
                         (real32 const *)Identity,
                         (vector_diff_mode)DiffMode,
                         (real32 &)*IdentityDifference,
                         (real32 &)*ConstantDifference);
}

GRANNY_DYNLINK(bool) GrannyKnotsAreReducible(granny_int32x Degree,
                                             granny_int32x Dimension,
                                             granny_int32x KnotCount,
                                             granny_real32 const * Knots,
                                             granny_real32 const * Controls,
                                             granny_real32 const * Identity,
                                             granny_real32 Epsilon,
                                             granny_vector_diff_mode DiffMode,
                                             granny_int32x * RequiredDegree,
                                             granny_int32x * RequiredKnotCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)KnotsAreReducible((int32x)Degree,
                                    (int32x)Dimension,
                                    (int32x)KnotCount,
                                    (real32 const *)Knots,
                                    (real32 const *)Controls,
                                    (real32 const *)Identity,
                                    (real32)Epsilon,
                                    (vector_diff_mode)DiffMode,
                                    (int32x &)*RequiredDegree,
                                    (int32x &)*RequiredKnotCount));
}

GRANNY_DYNLINK(bool) GrannyCurveIsReducible(granny_curve2 const * Curve,
                                            granny_real32 const * Identity,
                                            granny_real32 Epsilon,
                                            granny_vector_diff_mode DiffMode,
                                            granny_int32x * RequiredDegree,
                                            granny_int32x * RequiredKnotCount)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)CurveIsReducible((curve2 const *)Curve,
                                   (real32 const *)Identity,
                                   (real32)Epsilon,
                                   (vector_diff_mode)DiffMode,
                                   (int32x &)*RequiredDegree,
                                   (int32x &)*RequiredKnotCount));
}

GRANNY_DYNLINK(bool) GrannyTransformTrackHasKeyframedCurves(granny_transform_track const * Track)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)TransformTrackHasKeyframedCurves((transform_track const &)*Track));
}

GRANNY_DYNLINK(bool) GrannyTransformTrackIsAnimated(granny_transform_track const * Track)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)TransformTrackIsAnimated((transform_track const &)*Track));
}

GRANNY_DYNLINK(bool) GrannyTransformTrackIsIdentity(granny_transform_track const * Track)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)TransformTrackIsIdentity((transform_track const &)*Track));
}

GRANNY_DYNLINK(bool) GrannyFindTrackByName(granny_track_group const * TrackGroup,
                                           char const * TrackName,
                                           granny_int32x * TrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindTrackByName((track_group const *)TrackGroup,
                                  (char const *)TrackName,
                                  (int32x &)*TrackIndex));
}

GRANNY_DYNLINK(bool) GrannyFindTrackByRule(granny_track_group const * TrackGroup,
                                           char const * ProcessedTrackName,
                                           char const * TrackPattern,
                                           granny_int32x * TrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindTrackByRule((track_group const *)TrackGroup,
                                  (char const *)ProcessedTrackName,
                                  (char const *)TrackPattern,
                                  (int32x &)*TrackIndex));
}

GRANNY_DYNLINK(bool) GrannyFindVectorTrackByName(granny_track_group const * TrackGroup,
                                                 char const * TrackName,
                                                 granny_int32x * VectorTrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindVectorTrackByName((track_group const *)TrackGroup,
                                        (char const *)TrackName,
                                        (int32x &)*VectorTrackIndex));
}

GRANNY_DYNLINK(bool) GrannyFindVectorTrackByRule(granny_track_group const * TrackGroup,
                                                 char const * ProcessedTrackName,
                                                 char const * TrackPattern,
                                                 granny_int32x * VectorTrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)FindVectorTrackByRule((track_group const *)TrackGroup,
                                        (char const *)ProcessedTrackName,
                                        (char const *)TrackPattern,
                                        (int32x &)*VectorTrackIndex));
}

GRANNY_DYNLINK(void) GrannyGetTrackGroupFlags(granny_track_group const * TrackGroup,
                                              granny_uint32 * Flags,
                                              granny_uint32 * VDADOFs)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetTrackGroupFlags((track_group const &)*TrackGroup,
                       (uint32 &)*Flags,
                       (uint32 &)*VDADOFs);
}

GRANNY_DYNLINK(void) GrannySetTrackGroupFlags(granny_track_group * TrackGroup,
                                              granny_uint32 Flags,
                                              granny_uint32 VDADOFs)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackGroupFlags((track_group &)*TrackGroup,
                       (uint32)Flags,
                       (uint32)VDADOFs);
}

GRANNY_DYNLINK(granny_uint32) GrannyVectorTrackKeyForBone(granny_skeleton *    Skeleton,
                                                          granny_int32x BoneIndex,
                                                          char const*  TrackName)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    uint32 ReturnValue;
    return *(granny_uint32 *)&(ReturnValue = VectorTrackKeyForBone((skeleton &   )*Skeleton,
                                                                   (int32x)BoneIndex,
                                                                   (char const* )TrackName));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyVectorTrackType = (granny_data_type_definition *)VectorTrackType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTransformTrackType = (granny_data_type_definition *)TransformTrackType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTextTrackEntryType = (granny_data_type_definition *)TextTrackEntryType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTextTrackType = (granny_data_type_definition *)TextTrackType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTrackGroupType = (granny_data_type_definition *)TrackGroupType;
#include "granny_track_group_builder.h"
GRANNY_DYNLINK(granny_track_group_builder *) GrannyBeginTrackGroup(char const * Name,
                                                                   granny_int32x VectorTrackCount,
                                                                   granny_int32x TransformTrackCount,
                                                                   granny_int32x TextTrackCount,
                                                                   bool IncludeLODErrorSpace)
{
    return ((granny_track_group_builder *)BeginTrackGroup((char const *)Name,
                                                          (int32x)VectorTrackCount,
                                                          (int32x)TransformTrackCount,
                                                          (int32x)TextTrackCount,
                                                          (bool)IncludeLODErrorSpace));
}

GRANNY_DYNLINK(granny_track_group *) GrannyEndTrackGroup(granny_track_group_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_group *)EndTrackGroup((track_group_builder *)Builder));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingTrackGroupSize(granny_track_group_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingTrackGroupSize((track_group_builder const &)*Builder));
}

GRANNY_DYNLINK(granny_track_group *) GrannyEndTrackGroupInPlace(granny_track_group_builder * Builder,
                                                                void * Memory)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_group *)EndTrackGroupInPlace((track_group_builder *)Builder,
                                                       (void *)Memory));
}

GRANNY_DYNLINK(void) GrannyPushVectorTrackCurve(granny_track_group_builder * Builder,
                                                char const * Name,
                                                granny_uint32 TrackKey,
                                                granny_curve2 const * SourceCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushVectorTrackCurve((track_group_builder &)*Builder,
                         (char const *)Name,
                         (uint32)TrackKey,
                         (curve2 const *)SourceCurve);
}

GRANNY_DYNLINK(void) GrannyBeginTransformTrack(granny_track_group_builder * Builder,
                                               char const * Name,
                                               granny_int32x Flags)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BeginTransformTrack((track_group_builder &)*Builder,
                        (char const *)Name,
                        (int32x)Flags);
}

GRANNY_DYNLINK(void) GrannySetTransformTrackPositionCurve(granny_track_group_builder * Builder,
                                                          granny_curve2 const * SourceCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTransformTrackPositionCurve((track_group_builder &)*Builder,
                                   (curve2 const *)SourceCurve);
}

GRANNY_DYNLINK(void) GrannySetTransformTrackOrientationCurve(granny_track_group_builder * Builder,
                                                             granny_curve2 const * SourceCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTransformTrackOrientationCurve((track_group_builder &)*Builder,
                                      (curve2 const *)SourceCurve);
}

GRANNY_DYNLINK(void) GrannySetTransformTrackScaleShearCurve(granny_track_group_builder * Builder,
                                                            granny_curve2 const * SourceCurve)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTransformTrackScaleShearCurve((track_group_builder &)*Builder,
                                     (curve2 const *)SourceCurve);
}

GRANNY_DYNLINK(void) GrannyEndTransformTrack(granny_track_group_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndTransformTrack((track_group_builder &)*Builder);
}

GRANNY_DYNLINK(void) GrannyBeginTextTrack(granny_track_group_builder * Builder,
                                          char const * Name)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BeginTextTrack((track_group_builder &)*Builder,
                   (char const *)Name);
}

GRANNY_DYNLINK(void) GrannyAddTextEntry(granny_track_group_builder * Builder,
                                        granny_real32 TimeStamp,
                                        char const * Text)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddTextEntry((track_group_builder &)*Builder,
                 (real32)TimeStamp,
                 (char const *)Text);
}

GRANNY_DYNLINK(void) GrannyEndTextTrack(granny_track_group_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndTextTrack((track_group_builder &)*Builder);
}

GRANNY_DYNLINK(void) GrannyResortTrackGroup(granny_track_group * Group)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResortTrackGroup((track_group &)*Group);
}

GRANNY_DYNLINK(void) GrannyAllocateLODErrorSpace(granny_track_group * Group)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AllocateLODErrorSpace((track_group &)*Group);
}

GRANNY_DYNLINK(void) GrannyFreeLODErrorSpace(granny_track_group * Group)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeLODErrorSpace((track_group &)*Group);
}

GRANNY_DYNLINK(void) GrannySetAllLODErrorSpace(granny_track_group * Group,
                                               granny_real32 Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetAllLODErrorSpace((track_group &)*Group,
                        (real32)Value);
}

GRANNY_DYNLINK(void) GrannyResetLODErrorSpace(granny_track_group * Group)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ResetLODErrorSpace((track_group &)*Group);
}

#include "granny_track_group_sampler.h"
GRANNY_DYNLINK(granny_track_group_sampler *) GrannyBeginSampledAnimation(granny_int32x TransformCurveCount,
                                                                         granny_int32x SampleCount)
{
    return ((granny_track_group_sampler *)BeginSampledAnimation((int32x)TransformCurveCount,
                                                                (int32x)SampleCount));
}

GRANNY_DYNLINK(void) GrannyEndSampledAnimation(granny_track_group_sampler * Sampler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EndSampledAnimation((track_group_sampler *)Sampler);
}

GRANNY_DYNLINK(granny_track_group_sampler *) GrannyBeginSampledAnimationNonBlocked(granny_int32x TransformCurveCount,
                                                                                   granny_int32x SampleCount)
{
    return ((granny_track_group_sampler *)BeginSampledAnimationNonBlocked((int32x)TransformCurveCount,
                                                                          (int32x)SampleCount));
}

GRANNY_DYNLINK(granny_real32 *) GrannyGetPositionSamples(granny_track_group_sampler * Sampler,
                                                         granny_int32x TrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_real32 *)GetPositionSamples((track_group_sampler &)*Sampler,
                                                (int32x)TrackIndex));
}

GRANNY_DYNLINK(granny_real32 *) GrannyGetOrientationSamples(granny_track_group_sampler * Sampler,
                                                            granny_int32x TrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_real32 *)GetOrientationSamples((track_group_sampler &)*Sampler,
                                                   (int32x)TrackIndex));
}

GRANNY_DYNLINK(granny_real32 *) GrannyGetScaleShearSamples(granny_track_group_sampler * Sampler,
                                                           granny_int32x TrackIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_real32 *)GetScaleShearSamples((track_group_sampler &)*Sampler,
                                                  (int32x)TrackIndex));
}

GRANNY_DYNLINK(void) GrannySetTransformSample(granny_track_group_sampler * Sampler,
                                              granny_int32x TrackIndex,
                                              granny_real32 const * Position3,
                                              granny_real32 const * Orientation4,
                                              granny_real32 const * ScaleShear3x3)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTransformSample((track_group_sampler &)*Sampler,
                       (int32x)TrackIndex,
                       (real32 const *)Position3,
                       (real32 const *)Orientation4,
                       (real32 const *)ScaleShear3x3);
}

GRANNY_DYNLINK(void) GrannyPushSampledFrame(granny_track_group_sampler * Sampler)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    PushSampledFrame((track_group_sampler &)*Sampler);
}

#include "granny_track_mask.h"
GRANNY_DYNLINK(granny_track_mask *) GrannyNewTrackMask(granny_real32 DefaultWeight,
                                                       granny_int32x BoneCount)
{
    return ((granny_track_mask *)NewTrackMask((real32)DefaultWeight,
                                              (int32x)BoneCount));
}

GRANNY_DYNLINK(granny_extract_track_mask_result) GrannyExtractTrackMask(granny_track_mask * TrackMask,
                                                                        granny_int32x BoneCount,
                                                                        granny_skeleton const * Skeleton,
                                                                        char * MaskName,
                                                                        granny_real32 DefaultWeight,
                                                                        bool UseParentForDefault)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    extract_track_mask_result ReturnValue;
    return *(granny_extract_track_mask_result *)&(ReturnValue = ExtractTrackMask((track_mask *)TrackMask,
                                                                                 (int32x)BoneCount,
                                                                                 (skeleton const &)*Skeleton,
                                                                                 (char *)MaskName,
                                                                                 (real32)DefaultWeight,
                                                                                 (bool)UseParentForDefault));
}

GRANNY_DYNLINK(granny_real32) GrannyGetTrackMaskBoneWeight(granny_track_mask const * Mask,
                                                           granny_int32x BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    real32 ReturnValue;
    return *(granny_real32 *)&(ReturnValue = GetTrackMaskBoneWeight((track_mask const &)*Mask,
                                                                    (int32x)BoneIndex));
}

GRANNY_DYNLINK(void) GrannySetTrackMaskBoneWeight(granny_track_mask * Mask,
                                                  granny_int32x BoneIndex,
                                                  granny_real32 Weight)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetTrackMaskBoneWeight((track_mask &)*Mask,
                           (int32x)BoneIndex,
                           (real32)Weight);
}

GRANNY_DYNLINK(void) GrannyFreeTrackMask(granny_track_mask * Mask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeTrackMask((track_mask *)Mask);
}

GRANNY_DYNLINK(granny_track_mask *) GrannyCopyTrackMask(granny_track_mask const * Mask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_mask *)CopyTrackMask((track_mask const &)*Mask));
}

GRANNY_DYNLINK(void) GrannyInvertTrackMask(granny_track_mask * Mask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InvertTrackMask((track_mask &)*Mask);
}

GRANNY_DYNLINK(void) GrannySetSkeletonTrackMaskFromTrackGroup(granny_track_mask * Mask,
                                                              granny_skeleton const * Skeleton,
                                                              granny_track_group const * TrackGroup,
                                                              granny_real32 IdentityValue,
                                                              granny_real32 ConstantValue,
                                                              granny_real32 AnimatedValue)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetSkeletonTrackMaskFromTrackGroup((track_mask &)*Mask,
                                       (skeleton const &)*Skeleton,
                                       (track_group const &)*TrackGroup,
                                       (real32)IdentityValue,
                                       (real32)ConstantValue,
                                       (real32)AnimatedValue);
}

GRANNY_DYNLINK(void) GrannySetSkeletonTrackMaskChainUpwards(granny_track_mask * Mask,
                                                            granny_skeleton const * Skeleton,
                                                            granny_int32x ChainLeafBoneIndex,
                                                            granny_real32 Weight)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetSkeletonTrackMaskChainUpwards((track_mask &)*Mask,
                                     (skeleton const &)*Skeleton,
                                     (int32x)ChainLeafBoneIndex,
                                     (real32)Weight);
}

GRANNY_DYNLINK(void) GrannySetSkeletonTrackMaskChainDownwards(granny_track_mask * Mask,
                                                              granny_skeleton const * Skeleton,
                                                              granny_int32x ChainRootBoneIndex,
                                                              granny_real32 Weight)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetSkeletonTrackMaskChainDownwards((track_mask &)*Mask,
                                       (skeleton const &)*Skeleton,
                                       (int32x)ChainRootBoneIndex,
                                       (real32)Weight);
}

GRANNY_DYNLINK(granny_int32x) GrannyFindMaskIndexForName(granny_unbound_track_mask *  UnboundMask,
                                                         char const*  Name)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = FindMaskIndexForName((unbound_track_mask & )*UnboundMask,
                                                                  (char const* )Name));
}

GRANNY_DYNLINK(void) GrannyBindTrackmaskToModel(granny_unbound_track_mask *  UnboundMask,
                                                granny_model *  Model,
                                                granny_track_mask *  Mask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BindTrackmaskToModel((unbound_track_mask & )*UnboundMask,
                         (model & )*Model,
                         (track_mask & )*Mask);
}

GRANNY_DYNLINK(void) GrannyBindTrackmaskToTrackGroup(granny_unbound_track_mask *  UnboundMask,
                                                     granny_track_group *  Model,
                                                     granny_track_mask *  Mask)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BindTrackmaskToTrackGroup((unbound_track_mask & )*UnboundMask,
                              (track_group & )*Model,
                              (track_mask & )*Mask);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTrackMaskType = (granny_data_type_definition *)TrackMaskType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyUnboundTrackMaskType = (granny_data_type_definition *)UnboundTrackMaskType;
GRANNY_DYNLINKDATA(granny_track_mask *) GrannyIdentityTrackMask = &(granny_track_mask &)IdentityTrackMask;
GRANNY_DYNLINKDATA(granny_track_mask *) GrannyNullTrackMask = &(granny_track_mask &)NullTrackMask;
#include "granny_track_sampler.h"
GRANNY_DYNLINK(void) GrannySampleTrackUUULocal(granny_sample_context const * Context,
                                               granny_transform_track const * SourceTrack,
                                               granny_bound_transform_track * Track,
                                               granny_transform * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleTrackUUULocal((sample_context const &)*Context,
                        (transform_track const *)SourceTrack,
                        (bound_transform_track *)Track,
                        (transform &)*Result);
}

GRANNY_DYNLINK(void) GrannySampleTrackPOLocal(granny_sample_context const * Context,
                                              granny_transform_track const * SourceTrack,
                                              granny_bound_transform_track * Track,
                                              granny_real32 * ResultPosition,
                                              granny_real32 * ResultOrientation)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleTrackPOLocal((sample_context const &)*Context,
                       (transform_track const *)SourceTrack,
                       (bound_transform_track *)Track,
                       (real32 *)ResultPosition,
                       (real32 *)ResultOrientation);
}

GRANNY_DYNLINK(void) GrannySampleTrackUUULocalAtTime0(granny_transform_track const * SourceTrack,
                                                      granny_transform * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SampleTrackUUULocalAtTime0((transform_track const *)SourceTrack,
                               (transform &)*Result);
}

GRANNY_DYNLINK(granny_track_sampler *) GrannyGetTrackSamplerFor(granny_transform_track const * Track)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_sampler *)GetTrackSamplerFor((transform_track const &)*Track));
}

GRANNY_DYNLINK(granny_track_sampler *) GrannyGetTrackSamplerUUU(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_sampler *)GetTrackSamplerUUU());
}

GRANNY_DYNLINK(granny_track_sampler *) GrannyGetTrackSamplerSSS(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_sampler *)GetTrackSamplerSSS());
}

GRANNY_DYNLINK(granny_track_sampler *) GrannyGetTrackSamplerIII(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_sampler *)GetTrackSamplerIII());
}

GRANNY_DYNLINK(granny_track_sampler *) GrannyGetTrackSamplerIIU(void)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_track_sampler *)GetTrackSamplerIIU());
}

#include "granny_tri_topology.h"
GRANNY_DYNLINK(void) GrannyInvertTriTopologyWinding(granny_tri_topology * Topology)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    InvertTriTopologyWinding((tri_topology &)*Topology);
}

GRANNY_DYNLINK(void) GrannyRemapTopologyMaterials(granny_tri_topology * Topology,
                                                  granny_int32x RemapCount,
                                                  granny_int32x * RemapTable)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    RemapTopologyMaterials((tri_topology &)*Topology,
                           (int32x)RemapCount,
                           (int32x *)RemapTable);
}

GRANNY_DYNLINK(void) GrannyConvertIndices(granny_int32x IndexCount,
                                          granny_int32x FromBytesPerIndex,
                                          void const * FromIndices,
                                          granny_int32x ToBytesPerIndex,
                                          void * ToIndices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ConvertIndices((int32x)IndexCount,
                   (int32x)FromBytesPerIndex,
                   (void const *)FromIndices,
                   (int32x)ToBytesPerIndex,
                   (void *)ToIndices);
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTriMaterialGroupType = (granny_data_type_definition *)TriMaterialGroupType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTriAnnotationSetType = (granny_data_type_definition *)TriAnnotationSetType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyTriTopologyType = (granny_data_type_definition *)TriTopologyType;
#include "granny_type_listing.h"
GRANNY_DYNLINK(granny_int32x) GrannyGetDefinedTypeCount(void)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetDefinedTypeCount());
}

GRANNY_DYNLINKDATA(granny_defined_type *) GrannyDefinedTypes = (granny_defined_type *)DefinedTypes;
#include "granny_variant_builder.h"
GRANNY_DYNLINK(granny_variant_builder *) GrannyBeginVariant(granny_string_table * StringTableBuilder)
{
    return ((granny_variant_builder *)BeginVariant((string_table &)*StringTableBuilder));
}

GRANNY_DYNLINK(bool) GrannyEndVariant(granny_variant_builder * Builder,
                                      granny_data_type_definition ** Type,
                                      void ** Object)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndVariant((variant_builder *)Builder,
                             (data_type_definition *&)*Type,
                             (void *&)*Object));
}

GRANNY_DYNLINK(void) GrannyAbortVariant(granny_variant_builder * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AbortVariant((variant_builder *)Builder);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingVariantTypeSize(granny_variant_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingVariantTypeSize((variant_builder const &)*Builder));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingVariantObjectSize(granny_variant_builder const * Builder)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingVariantObjectSize((variant_builder const &)*Builder));
}

GRANNY_DYNLINK(bool) GrannyEndVariantInPlace(granny_variant_builder * Builder,
                                             void * TypeMemory,
                                             granny_data_type_definition ** Type,
                                             void * ObjectMemory,
                                             void ** Object)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((bool)EndVariantInPlace((variant_builder *)Builder,
                                    (void *)TypeMemory,
                                    (data_type_definition *&)*Type,
                                    (void *)ObjectMemory,
                                    (void *&)*Object));
}

GRANNY_DYNLINK(void) GrannyAddBoolMember(granny_variant_builder * Builder,
                                         char const * Name,
                                         granny_bool32 Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddBoolMember((variant_builder &)*Builder,
                  (char const *)Name,
                  (bool32)Value);
}

GRANNY_DYNLINK(void) GrannyAddIntegerMember(granny_variant_builder * Builder,
                                            char const * Name,
                                            granny_int32 Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddIntegerMember((variant_builder &)*Builder,
                     (char const *)Name,
                     (int32)Value);
}

GRANNY_DYNLINK(void) GrannyAddIntegerArrayMember(granny_variant_builder * Builder,
                                                 char const * Name,
                                                 granny_int32x Width,
                                                 granny_int32 *  Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddIntegerArrayMember((variant_builder &)*Builder,
                          (char const *)Name,
                          (int32x)Width,
                          (int32 * )Value);
}

GRANNY_DYNLINK(void) GrannyAddUnsignedIntegerMember(granny_variant_builder * Builder,
                                                    char const * Name,
                                                    granny_uint32 Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddUnsignedIntegerMember((variant_builder &)*Builder,
                             (char const *)Name,
                             (uint32)Value);
}

GRANNY_DYNLINK(void) GrannyAddScalarMember(granny_variant_builder * Builder,
                                           char const * Name,
                                           granny_real32 Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddScalarMember((variant_builder &)*Builder,
                    (char const *)Name,
                    (real32)Value);
}

GRANNY_DYNLINK(void) GrannyAddScalarArrayMember(granny_variant_builder * Builder,
                                                char const * Name,
                                                granny_int32x Width,
                                                granny_real32 const * Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddScalarArrayMember((variant_builder &)*Builder,
                         (char const *)Name,
                         (int32x)Width,
                         (real32 const *)Value);
}

GRANNY_DYNLINK(void) GrannyAddStringMember(granny_variant_builder * Builder,
                                           char const * Name,
                                           char const * Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddStringMember((variant_builder &)*Builder,
                    (char const *)Name,
                    (char const *)Value);
}

GRANNY_DYNLINK(void) GrannyAddReferenceMember(granny_variant_builder * Builder,
                                              char const * Name,
                                              granny_data_type_definition * Type,
                                              void * Value)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddReferenceMember((variant_builder &)*Builder,
                       (char const *)Name,
                       (data_type_definition *)Type,
                       (void *)Value);
}

GRANNY_DYNLINK(void) GrannyAddDynamicArrayMember(granny_variant_builder * Builder,
                                                 char const * Name,
                                                 granny_int32x Count,
                                                 granny_data_type_definition * EntryType,
                                                 void * ArrayEntries)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    AddDynamicArrayMember((variant_builder &)*Builder,
                          (char const *)Name,
                          (int32x)Count,
                          (data_type_definition *)EntryType,
                          (void *)ArrayEntries);
}

#include "granny_version_checking.h"
GRANNY_DYNLINK(char const *) GrannyGetVersionString(void)
{
    return ((char const *)GetVersionString());
}

GRANNY_DYNLINK(void) GrannyGetVersion(granny_int32x * MajorVersion,
                                      granny_int32x * MinorVersion,
                                      granny_int32x * Customization,
                                      granny_int32x * BuildNumber)
{
    GetVersion((int32x &)*MajorVersion,
               (int32x &)*MinorVersion,
               (int32x &)*Customization,
               (int32x &)*BuildNumber);
}

GRANNY_DYNLINK(bool) GrannyVersionsMatch_(granny_int32x MajorVersion,
                                          granny_int32x MinorVersion,
                                          granny_int32x Customization,
                                          granny_int32x BuildNumber)
{
    return ((bool)VersionsMatch_((int32x)MajorVersion,
                                 (int32x)MinorVersion,
                                 (int32x)Customization,
                                 (int32x)BuildNumber));
}

#include "granny_vertex_data.h"
GRANNY_DYNLINK(void) GrannyConvertVertexLayouts(granny_int32x VertexCount,
                                                granny_data_type_definition const * SourceLayout,
                                                void const * SourceVertices,
                                                granny_data_type_definition const * DestLayout,
                                                void * DestVertices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    ConvertVertexLayouts((int32x)VertexCount,
                         (data_type_definition const *)SourceLayout,
                         (void const *)SourceVertices,
                         (data_type_definition const *)DestLayout,
                         (void *)DestVertices);
}

GRANNY_DYNLINK(void) GrannyEnsureExactOneNorm(granny_data_type_definition const * WeightsType,
                                              void * WeightsInit)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    EnsureExactOneNorm((data_type_definition const &)*WeightsType,
                       (void *)WeightsInit);
}

GRANNY_DYNLINK(void) GrannyOneNormalizeWeights(granny_int32x VertexCount,
                                               granny_data_type_definition const * Layout,
                                               void * Vertices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    OneNormalizeWeights((int32x)VertexCount,
                        (data_type_definition const *)Layout,
                        (void *)Vertices);
}

GRANNY_DYNLINK(void) GrannyTransformVertices(granny_int32x VertexCount,
                                             granny_data_type_definition const * Layout,
                                             void * Vertices,
                                             granny_real32 const * Affine3,
                                             granny_real32 const * Linear3x3,
                                             granny_real32 const * InverseLinear3x3,
                                             bool Renormalize,
                                             bool TreatAsDeltas)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    TransformVertices((int32x)VertexCount,
                      (data_type_definition const *)Layout,
                      (void *)Vertices,
                      (real32 const *)Affine3,
                      (real32 const *)Linear3x3,
                      (real32 const *)InverseLinear3x3,
                      (bool)Renormalize,
                      (bool)TreatAsDeltas);
}

GRANNY_DYNLINK(void) GrannyNormalizeVertices(granny_int32x VertexCount,
                                             granny_data_type_definition const * LayoutType,
                                             void * Vertices)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    NormalizeVertices((int32x)VertexCount,
                      (data_type_definition const *)LayoutType,
                      (void *)Vertices);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexTextureCoordinatesName(granny_int32x Index,
                                                                    char * Buffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexTextureCoordinatesName((int32x)Index,
                                                                             (char *)Buffer));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexDiffuseColorName(granny_int32x Index,
                                                              char * Buffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexDiffuseColorName((int32x)Index,
                                                                       (char *)Buffer));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexSpecularColorName(granny_int32x Index,
                                                               char * Buffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexSpecularColorName((int32x)Index,
                                                                        (char *)Buffer));
}

GRANNY_DYNLINK(bool) GrannyIsSpatialVertexMember(char const * Name)
{
    return ((bool)IsSpatialVertexMember((char const *)Name));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexBoneCount(granny_data_type_definition const * VertexType)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexBoneCount((data_type_definition const *)VertexType));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexChannelCount(granny_data_type_definition const * VertexType)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexChannelCount((data_type_definition const *)VertexType));
}

GRANNY_DYNLINK(void) GrannyGetSingleVertex(granny_vertex_data const * VertexData,
                                           granny_int32x VertexIndex,
                                           granny_data_type_definition const * As,
                                           void * Dest)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    GetSingleVertex((vertex_data const &)*VertexData,
                    (int32x)VertexIndex,
                    (data_type_definition const *)As,
                    (void *)Dest);
}

GRANNY_DYNLINK(void) GrannySetVertexPosition(granny_data_type_definition const * VertexLayout,
                                             void * VertexPointer,
                                             granny_real32 const * Position)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetVertexPosition((data_type_definition const *)VertexLayout,
                      (void *)VertexPointer,
                      (real32 const *)Position);
}

GRANNY_DYNLINK(void) GrannySetVertexNormal(granny_data_type_definition const * VertexLayout,
                                           void * VertexPointer,
                                           granny_real32 const * Normal)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetVertexNormal((data_type_definition const *)VertexLayout,
                    (void *)VertexPointer,
                    (real32 const *)Normal);
}

GRANNY_DYNLINK(void) GrannySetVertexColor(granny_data_type_definition const  * VertexLayout,
                                          void * VertexPointer,
                                          granny_int32x ColorIndex,
                                          granny_real32 const * Color)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetVertexColor((data_type_definition const  *)VertexLayout,
                   (void *)VertexPointer,
                   (int32x)ColorIndex,
                   (real32 const *)Color);
}

GRANNY_DYNLINK(void) GrannySetVertexUVW(granny_data_type_definition const * VertexLayout,
                                        void * VertexPointer,
                                        granny_int32x UVWIndex,
                                        granny_real32 const * UVW)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    SetVertexUVW((data_type_definition const *)VertexLayout,
                 (void *)VertexPointer,
                 (int32x)UVWIndex,
                 (real32 const *)UVW);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexComponentCount(granny_data_type_definition const * VertexLayout)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexComponentCount((data_type_definition const *)VertexLayout));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetVertexComponentIndex(char const * ComponentName,
                                                            granny_data_type_definition const * VertexLayout)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetVertexComponentIndex((char const *)ComponentName,
                                                                     (data_type_definition const *)VertexLayout));
}

GRANNY_DYNLINK(char const * ) GrannyGetVertexComponentToolName(char const * ComponentName,
                                                               granny_vertex_data const * VertexData)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((char const * )GetVertexComponentToolName((char const *)ComponentName,
                                                      (vertex_data const *)VertexData));
}

GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyVertexAnnotationSetType = (granny_data_type_definition *)VertexAnnotationSetType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyVertexDataType = (granny_data_type_definition *)VertexDataType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyP3VertexType = (granny_data_type_definition *)P3VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPT32VertexType = (granny_data_type_definition *)PT32VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPN33VertexType = (granny_data_type_definition *)PN33VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNG333VertexType = (granny_data_type_definition *)PNG333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNGT3332VertexType = (granny_data_type_definition *)PNGT3332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNTG3323VertexType = (granny_data_type_definition *)PNTG3323VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNGB3333VertexType = (granny_data_type_definition *)PNGB3333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNT332VertexType = (granny_data_type_definition *)PNT332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNGBT33332VertexType = (granny_data_type_definition *)PNGBT33332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNT333VertexType = (granny_data_type_definition *)PNT333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPNGBT33333VertexType = (granny_data_type_definition *)PNGBT33333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWN313VertexType = (granny_data_type_definition *)PWN313VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNG3133VertexType = (granny_data_type_definition *)PWNG3133VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGT31332VertexType = (granny_data_type_definition *)PWNGT31332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGB31333VertexType = (granny_data_type_definition *)PWNGB31333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNT3132VertexType = (granny_data_type_definition *)PWNT3132VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGBT313332VertexType = (granny_data_type_definition *)PWNGBT313332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWN323VertexType = (granny_data_type_definition *)PWN323VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNG3233VertexType = (granny_data_type_definition *)PWNG3233VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGT32332VertexType = (granny_data_type_definition *)PWNGT32332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGB32333VertexType = (granny_data_type_definition *)PWNGB32333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNT3232VertexType = (granny_data_type_definition *)PWNT3232VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGBT323332VertexType = (granny_data_type_definition *)PWNGBT323332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWN343VertexType = (granny_data_type_definition *)PWN343VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNG3433VertexType = (granny_data_type_definition *)PWNG3433VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGT34332VertexType = (granny_data_type_definition *)PWNGT34332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGB34333VertexType = (granny_data_type_definition *)PWNGB34333VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNT3432VertexType = (granny_data_type_definition *)PWNT3432VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyPWNGBT343332VertexType = (granny_data_type_definition *)PWNGBT343332VertexType;
GRANNY_DYNLINKDATA(granny_data_type_definition *) GrannyVertexWeightArraysType = (granny_data_type_definition *)VertexWeightArraysType;
#include "granny_world_pose.h"
GRANNY_DYNLINK(granny_world_pose *) GrannyNewWorldPose(granny_int32x BoneCount)
{
    return ((granny_world_pose *)NewWorldPose((int32x)BoneCount));
}

GRANNY_DYNLINK(granny_world_pose *) GrannyNewWorldPoseNoComposite(granny_int32x BoneCount)
{
    return ((granny_world_pose *)NewWorldPoseNoComposite((int32x)BoneCount));
}

GRANNY_DYNLINK(void) GrannyFreeWorldPose(granny_world_pose * WorldPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    FreeWorldPose((world_pose *)WorldPose);
}

GRANNY_DYNLINK(granny_int32x) GrannyGetWorldPoseBoneCount(granny_world_pose const * WorldPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetWorldPoseBoneCount((world_pose const &)*WorldPose));
}

GRANNY_DYNLINK(granny_int32x) GrannyGetResultingWorldPoseSize(granny_int32x BoneCount,
                                                              granny_composite_flag CompositeFlag)
{
    int32x ReturnValue;
    return *(granny_int32x *)&(ReturnValue = GetResultingWorldPoseSize((int32x)BoneCount,
                                                                       (composite_flag)CompositeFlag));
}

GRANNY_DYNLINK(granny_world_pose *) GrannyNewWorldPoseInPlace(granny_int32x BoneCount,
                                                              granny_composite_flag CompositeFlag,
                                                              void * Memory)
{
    return ((granny_world_pose *)NewWorldPoseInPlace((int32x)BoneCount,
                                                     (composite_flag)CompositeFlag,
                                                     (void *)Memory));
}

GRANNY_DYNLINK(granny_real32 *) GrannyGetWorldPose4x4(granny_world_pose const * WorldPose,
                                                      granny_int32x BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_real32 *)GetWorldPose4x4((world_pose const &)*WorldPose,
                                             (int32x)BoneIndex));
}

GRANNY_DYNLINK(granny_real32 *) GrannyGetWorldPoseComposite4x4(granny_world_pose const * WorldPose,
                                                               granny_int32x BoneIndex)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_real32 *)GetWorldPoseComposite4x4((world_pose const &)*WorldPose,
                                                      (int32x)BoneIndex));
}

GRANNY_DYNLINK(granny_matrix_4x4 *) GrannyGetWorldPose4x4Array(granny_world_pose const * WorldPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_matrix_4x4 *)GetWorldPose4x4Array((world_pose const &)*WorldPose));
}

GRANNY_DYNLINK(granny_matrix_4x4 *) GrannyGetWorldPoseComposite4x4Array(granny_world_pose const * WorldPose)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    return ((granny_matrix_4x4 *)GetWorldPoseComposite4x4Array((world_pose const &)*WorldPose));
}

GRANNY_DYNLINK(void) GrannyBuildWorldPose(granny_skeleton const * Skeleton,
                                          granny_int32x FirstBone,
                                          granny_int32x BoneCount,
                                          granny_local_pose const * LocalPose,
                                          granny_real32 const * Offset4x4,
                                          granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPose((skeleton const &)*Skeleton,
                   (int32x)FirstBone,
                   (int32x)BoneCount,
                   (local_pose const &)*LocalPose,
                   (real32 const *)Offset4x4,
                   (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildWorldPoseLOD(granny_skeleton const * Skeleton,
                                             granny_int32x FirstBone,
                                             granny_int32x BoneCount,
                                             granny_int32x FirstValidLocalBone,
                                             granny_int32x ValidLocalBoneCount,
                                             granny_local_pose const * LocalPose,
                                             granny_real32 const * Offset4x4,
                                             granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPoseLOD((skeleton const &)*Skeleton,
                      (int32x)FirstBone,
                      (int32x)BoneCount,
                      (int32x)FirstValidLocalBone,
                      (int32x)ValidLocalBoneCount,
                      (local_pose const &)*LocalPose,
                      (real32 const *)Offset4x4,
                      (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildWorldPoseSparse(granny_skeleton const * Skeleton,
                                                granny_int32x FirstBone,
                                                granny_int32x BoneCount,
                                                granny_int32x const * SparseBoneArray,
                                                granny_int32x const * SparseBoneArrayReverse,
                                                granny_local_pose const * LocalPose,
                                                granny_real32 const * Offset4x4,
                                                granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPoseSparse((skeleton const &)*Skeleton,
                         (int32x)FirstBone,
                         (int32x)BoneCount,
                         (int32x const *)SparseBoneArray,
                         (int32x const *)SparseBoneArrayReverse,
                         (local_pose const &)*LocalPose,
                         (real32 const *)Offset4x4,
                         (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildRestWorldPose(granny_skeleton const * Skeleton,
                                              granny_int32x FirstBone,
                                              granny_int32x BoneCount,
                                              granny_real32 const * Offset4x4,
                                              granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildRestWorldPose((skeleton const &)*Skeleton,
                       (int32x)FirstBone,
                       (int32x)BoneCount,
                       (real32 const *)Offset4x4,
                       (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildWorldPoseNoComposite(granny_skeleton const * Skeleton,
                                                     granny_int32x FirstBone,
                                                     granny_int32x BoneCount,
                                                     granny_local_pose const * LocalPose,
                                                     granny_real32 const * Offset4x4,
                                                     granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPoseNoComposite((skeleton const &)*Skeleton,
                              (int32x)FirstBone,
                              (int32x)BoneCount,
                              (local_pose const &)*LocalPose,
                              (real32 const *)Offset4x4,
                              (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildWorldPoseNoCompositeLOD(granny_skeleton const * Skeleton,
                                                        granny_int32x FirstBone,
                                                        granny_int32x BoneCount,
                                                        granny_int32x FirstValidLocalBone,
                                                        granny_int32x ValidLocalBoneCount,
                                                        granny_local_pose const * LocalPose,
                                                        granny_real32 const * Offset4x4,
                                                        granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPoseNoCompositeLOD((skeleton const &)*Skeleton,
                                 (int32x)FirstBone,
                                 (int32x)BoneCount,
                                 (int32x)FirstValidLocalBone,
                                 (int32x)ValidLocalBoneCount,
                                 (local_pose const &)*LocalPose,
                                 (real32 const *)Offset4x4,
                                 (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildWorldPoseNoCompositeSparse(granny_skeleton const * Skeleton,
                                                           granny_int32x FirstBone,
                                                           granny_int32x BoneCount,
                                                           granny_int32x const * SparseBoneArray,
                                                           granny_int32x const * SparseBoneArrayReverse,
                                                           granny_local_pose const * LocalPose,
                                                           granny_real32 const * Offset4x4,
                                                           granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPoseNoCompositeSparse((skeleton const &)*Skeleton,
                                    (int32x)FirstBone,
                                    (int32x)BoneCount,
                                    (int32x const *)SparseBoneArray,
                                    (int32x const *)SparseBoneArrayReverse,
                                    (local_pose const &)*LocalPose,
                                    (real32 const *)Offset4x4,
                                    (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildWorldPoseComposites(granny_skeleton const * Skeleton,
                                                    granny_int32x FirstBone,
                                                    granny_int32x BoneCount,
                                                    granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildWorldPoseComposites((skeleton const &)*Skeleton,
                             (int32x)FirstBone,
                             (int32x)BoneCount,
                             (world_pose &)*Result);
}

GRANNY_DYNLINK(void) GrannyBuildCompositeBuffer(granny_skeleton const * Skeleton,
                                                granny_int32x FirstBone,
                                                granny_int32x BoneCount,
                                                granny_world_pose const * Pose,
                                                granny_matrix_4x4 *  CompositeBuffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildCompositeBuffer((skeleton const &)*Skeleton,
                         (int32x)FirstBone,
                         (int32x)BoneCount,
                         (world_pose const &)*Pose,
                         (matrix_4x4 * )CompositeBuffer);
}

GRANNY_DYNLINK(void) GrannyBuildCompositeBufferTransposed(granny_skeleton const * Skeleton,
                                                          granny_int32x FirstBone,
                                                          granny_int32x BoneCount,
                                                          granny_world_pose const * Pose,
                                                          granny_matrix_3x4 *  CompositeBuffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildCompositeBufferTransposed((skeleton const &)*Skeleton,
                                   (int32x)FirstBone,
                                   (int32x)BoneCount,
                                   (world_pose const &)*Pose,
                                   (matrix_3x4 * )CompositeBuffer);
}

GRANNY_DYNLINK(void) GrannyBuildIndexedCompositeBuffer(granny_skeleton const * Skeleton,
                                                       granny_world_pose const * Pose,
                                                       granny_int32x const*  Indices,
                                                       granny_int32x IndexCount,
                                                       granny_matrix_4x4 *  CompositeBuffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildIndexedCompositeBuffer((skeleton const &)*Skeleton,
                                (world_pose const &)*Pose,
                                (int32x const* )Indices,
                                (int32x)IndexCount,
                                (matrix_4x4 * )CompositeBuffer);
}

GRANNY_DYNLINK(void) GrannyBuildIndexedCompositeBufferTransposed(granny_skeleton const * Skeleton,
                                                                 granny_world_pose const * Pose,
                                                                 granny_int32x const*  Indices,
                                                                 granny_int32x IndexCount,
                                                                 granny_matrix_3x4 *  CompositeBuffer)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    BuildIndexedCompositeBufferTransposed((skeleton const &)*Skeleton,
                                          (world_pose const &)*Pose,
                                          (int32x const* )Indices,
                                          (int32x)IndexCount,
                                          (matrix_3x4 * )CompositeBuffer);
}

GRANNY_DYNLINK(void) GrannyUpdateWorldPoseChildren(granny_skeleton const * Skeleton,
                                                   granny_int32x ParentBone,
                                                   granny_local_pose * LocalPose,
                                                   granny_real32 const * Offset4x4,
                                                   granny_world_pose * Result)
{
#ifdef GRANNY_THREAD_CHECKS
    if (!GrannyThreadAllowedToCallGranny())
    {
        granny_log_callback LogCallback;
        GrannyGetLogCallback(&LogCallback);
        if (LogCallback.Function)
        {
            (*LogCallback.Function)(GrannyErrorLogMessage, GrannyThreadSafetyLogMessage,
                                    "ThreadingError: attempted to access granny from the wrong thread",
                                    LogCallback.UserData);
        }
    }
#endif // GRANNY_THREAD_CHECKS

    UpdateWorldPoseChildren((skeleton const &)*Skeleton,
                            (int32x)ParentBone,
                            (local_pose &)*LocalPose,
                            (real32 const *)Offset4x4,
                            (world_pose &)*Result);
}

