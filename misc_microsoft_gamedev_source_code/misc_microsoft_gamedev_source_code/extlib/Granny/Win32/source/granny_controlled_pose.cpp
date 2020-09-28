// ========================================================================
// $File: //jeffr/granny/rt/granny_controlled_pose.cpp $
// $DateTime: 2007/08/21 13:33:41 $
// $Change: 15778 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_CONTROLLED_POSE_H)
#include "granny_controlled_pose.h"
#endif

#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "granny_model_instance.h"
#endif

#if !defined(GRANNY_TRACK_MASK_H)
#include "granny_track_mask.h"
#endif

#if !defined(GRANNY_LOCAL_POSE_H)
#include "granny_local_pose.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_MODEL_CONTROL_BINDING_H)
#include "granny_model_control_binding.h"
#endif

#if !defined(GRANNY_CONTROL_H)
#include "granny_control.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_WORLD_POSE_H)
#include "granny_world_pose.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "granny_bone_operations.h"
#endif

#if !defined(GRANNY_MATH_H)
#include "granny_math.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

static bool
PoseInitializeBindingState(model_control_binding &Binding,
                           void *InitData)
{
    controlled_pose *State = GET_DERIVED(controlled_pose, Binding);

    controlled_pose *Source = (controlled_pose *)InitData;
    Assert(Source);

    State->Pose = Source->Pose;
    State->ModelMask = Source->ModelMask;

    Binding.ReservedPointer = (void*)ControlledPose;

    return(true);
}

static void
PoseAccumulateBindingState(model_control_binding &Binding,
                           int32x FirstBone, int32x BoneCount,
                           local_pose &Result, real32 AllowedError,
                           int32x const *SparseBoneArray)
{
    controlled_pose *State = GET_DERIVED(controlled_pose, Binding);

    real32 const ControlWeight = GetControlEffectiveWeight(*Binding.Control);
    if(ControlWeight > TrackWeightEpsilon)
    {
        int32x PoseBoneCount = GetLocalPoseBoneCount(*State->Pose);
        int32x OnePastLastBone = FirstBone + BoneCount;
        if(FirstBone < PoseBoneCount)
        {
            if(OnePastLastBone > PoseBoneCount)
            {
                OnePastLastBone = PoseBoneCount;
            }

            {for(int32x LocalPoseBoneIndex = FirstBone;
                 LocalPoseBoneIndex < OnePastLastBone;
                 ++LocalPoseBoneIndex)
            {
                real32 Weight = ControlWeight;
                int32x SkeletonBoneIndex = LocalPoseBoneIndex;
                if ( SparseBoneArray != NULL )
                {
                    SkeletonBoneIndex = SparseBoneArray[LocalPoseBoneIndex];
                }

                if(State->ModelMask)
                {
                    Weight *= GetTrackMaskBoneWeight(*State->ModelMask, SkeletonBoneIndex);
                }

                if(Weight > TrackWeightEpsilon)
                {
                    transform &Transform =
                        *GetLocalPoseTransform(*State->Pose, SkeletonBoneIndex);
                    AccumulateLocalTransform(
                        Result, LocalPoseBoneIndex, SkeletonBoneIndex, Weight,
                        *GetSourceSkeleton(*Binding.ModelInstance),
                        BlendQuaternionNeighborhooded, Transform);
                }
            }
        }}
    }
}

static void
PoseBuildDirect(model_control_binding &Binding,
                int32x BoneCount, real32 const *Offset4x4,
                world_pose &Result, real32 AllowedError)
{
    controlled_pose *State = GET_DERIVED(controlled_pose, Binding);

    int32x PoseBoneCount = GetLocalPoseBoneCount(*State->Pose);
    if(PoseBoneCount > BoneCount)
    {
        PoseBoneCount = BoneCount;
    }

    matrix_4x4 *WorldBuffer = Result.WorldTransformBuffer;
    skeleton *Skeleton = GetSourceSkeleton(*Binding.ModelInstance);
    bone *Bone = Skeleton->Bones;
    matrix_4x4 *World = WorldBuffer;
    matrix_4x4 *Composite = Result.CompositeTransformBuffer; // Could be null

    {for(int32x BoneIndex = 0;
         BoneIndex < PoseBoneCount;
         ++BoneIndex)
    {
        if (Composite)
        {
            BuildFullWorldPoseComposite(*GetLocalPoseTransform(*State->Pose, BoneIndex),
                                        (real32 const *)WorldBuffer[Bone->ParentIndex],
                                        (real32 *)Bone->InverseWorld4x4,
                                        (real32 *)World, (real32 *)Composite);
            ++Composite;
        }
        else
        {
            BuildFullWorldPoseOnly(*GetLocalPoseTransform(*State->Pose, BoneIndex),
                                   (real32 const *)WorldBuffer[Bone->ParentIndex],
                                   (real32 *)World);
        }


        ++Bone;
        ++World;
    }}
}


controlled_animation *PoseGetControlledAnimation ( model_control_binding &Binding )
{
    // It's not an animation!
    return NULL;
}

controlled_pose *PoseGetControlledPose ( model_control_binding &Binding )
{
    controlled_pose *State = GET_DERIVED(controlled_pose, Binding);
    return State;
}


static model_control_callbacks ControlledPoseCallbacks =
{
    PoseGetControlledAnimation,
    PoseGetControlledPose,
    PoseInitializeBindingState,
    PoseAccumulateBindingState,
    PoseBuildDirect,
};

END_GRANNY_NAMESPACE;

control *GRANNY
PlayControlledPose(real32 CurrentSeconds,
                   real32 Duration,
                   local_pose const &Pose,
                   model_instance &Model,
                   track_mask *ModelMask)
{
    // Create the control
    control *Control = CreateControl(CurrentSeconds, Duration);
    if(Control)
    {
        controlled_pose ControlledPose;
        ControlledPose.Pose = &Pose;
        ControlledPose.ModelMask = ModelMask;

        model_control_binding *Binding =
            CreateModelControlBinding(
                ControlledPoseCallbacks, *Control,
                Model, ControlIsActive(*Control),
                &ControlledPose);
        if(Binding)
        {
            // All is good
        }
        else
        {
            FreeControl(Control);
            Control = 0;

            Log0(ErrorLogMessage, ControlLogMessage,
                 "Unable to bind track group");
        }
    }
    else
    {
        Log0(ErrorLogMessage, ControlLogMessage,
             "Unable to create control");
    }

    return(Control);
}


local_pose *GRANNY
GetLocalPoseFromControlBinding(model_control_binding &Binding)
{
    controlled_pose *ControlledPose = Binding.Callbacks->GetControlledPose ( Binding );
    if ( ControlledPose != NULL )
    {
        return (local_pose *)ControlledPose->Pose;
    }
    else
    {
        return NULL;
    }
}



