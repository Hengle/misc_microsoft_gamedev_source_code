// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_controlled_animation.cpp $
// $DateTime: 2007/08/31 17:29:59 $
// $Change: 15878 $
// $Revision: #2 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_SPU_CONTROLLED_ANIMATION_H)
#include "granny_spu_controlled_animation.h"
#endif

#if !defined(GRANNY_SPU_ANIMATION_H)
#include "granny_spu_animation.h"
#endif

#if !defined(GRANNY_SPU_ANIMATION_BINDING_H)
#include "granny_spu_animation_binding.h"
#endif

#if !defined(GRANNY_SPU_TRACK_GROUP_H)
#include "granny_spu_track_group.h"
#endif

#if !defined(GRANNY_CONTROL_H)
#include "granny_control.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_MODEL_INSTANCE_H)
#include "granny_model_instance.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode ControlLogMessage

USING_GRANNY_NAMESPACE;
BEGIN_GRANNY_NAMESPACE;

struct spu_track_target
{
    spu_animation_binding* Binding;
    spu_animation_binding_id ID;

    model_instance *OnInstance;
    accumulation_mode AccumulationMode;

    track_mask *TrackMask;
    track_mask *ModelMask;
};

struct controlled_spu_animation_builder
{
    real32 CurrentClock;
    spu_animation const *Animation;

    int32x TrackCount;
    spu_track_target *Tracks;
};

END_GRANNY_NAMESPACE;

static bool
SPUAnimationInitializeBindingState(model_control_binding &Binding,
                                   void *InitData)
{
    spu_controlled_animation *State = GET_DERIVED(spu_controlled_animation, Binding);
    spu_track_target *TrackTarget = (spu_track_target *)InitData;
    Assert(TrackTarget);

    State->Binding = AcquireSPUAnimationBindingFromID(TrackTarget->ID);
    State->AccumulationMode = TrackTarget->AccumulationMode;
    State->TrackMask = TrackTarget->TrackMask;
    State->ModelMask = TrackTarget->ModelMask;

    Assert(State->Binding);
    Assert(State->Binding->TrackNameRemaps);

    Binding.ReservedPointer = (void*)SPUControlledAnim;

    return(State->Binding != 0);
}

static model_control_callbacks ControlledAnimationCallbacks =
{
    NULL,                               // AnimationGetControlledAnimation,
    NULL,                               // AnimationGetControlledPose,
    SPUAnimationInitializeBindingState,
    NULL,                               // AnimationAccumulateBindingState,
    NULL,                               // AnimationBuildDirect,
    NULL,                               // AnimationAccumulateLoopTransform,
    NULL,                               // AnimationCleanupBindingState,
};


control* GRANNY
PlayControlledSPUAnimation(real32 StartTime,
                           spu_animation const& Animation,
                           model_instance& Model)
{
    control *Result = 0;

    int32x TrackGroupIndex;
    if (FindSPUTrackGroupForModel(Animation,
                                  GetSourceModel(Model).Name,
                                  TrackGroupIndex))
    {
        controlled_spu_animation_builder *Builder =
            BeginControlledSPUAnimation(StartTime, Animation);
        if(Builder)
        {
            SetSPUTrackGroupTarget(*Builder, TrackGroupIndex, Model);
            Result = EndControlledSPUAnimation(Builder);
        }
    }

    return Result;
}


controlled_spu_animation_builder *GRANNY
BeginControlledSPUAnimation(real32 StartTime, spu_animation const &Animation)
{
    COUNT_BLOCK("BeginControlledSPUAnimation");

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    controlled_spu_animation_builder* Builder;
    AggrAllocPtr(Allocator, Builder);
    AggrAllocOffsetArrayPtr(Allocator, Builder,
                            Animation.TrackGroupCount,
                            TrackCount, Tracks);
    if(EndAggrAlloc(Allocator))
    {
        Builder->CurrentClock = StartTime;
        Builder->Animation = &Animation;

        {for(int32x TrackIndex = 0;
             TrackIndex < Builder->TrackCount;
             ++TrackIndex)
        {
            spu_track_target &Track = Builder->Tracks[TrackIndex];
            ZeroStructure(Track);

            // TODO: accumulation
        }}
    }

    return Builder;
}


control *GRANNY
EndControlledSPUAnimation(controlled_spu_animation_builder *Builder)
{
    COUNT_BLOCK("EndControlledAnimation");

    control* Control = 0;
    if(Builder)
    {
        // Create the control
        Control = CreateControl(Builder->CurrentClock,
                                Builder->Animation->Duration);
        if(Control)
        {
            // Create the bindings
            {for (int32x TrackIndex = 0;
                  TrackIndex < Builder->TrackCount;
                  ++TrackIndex)
            {
                spu_track_target &Track = Builder->Tracks[TrackIndex];
                if (Track.OnInstance)
                {
                     model_control_binding *Binding =
                         CreateModelControlBinding(ControlledAnimationCallbacks, *Control,
                                                   *Track.OnInstance,
                                                   ControlIsActive(*Control), &Track);
                     if (!Binding)
                         Log0(ErrorLogMessage, ControlLogMessage,
                              "Unable to bind track group");
                }
            }}
        }
        else
        {
            Log0(ErrorLogMessage, ControlLogMessage,
                 "Unable to create control");
        }

        Deallocate(Builder);
    }

    return Control;
}

void GRANNY
SetSPUTrackGroupTarget(controlled_spu_animation_builder& Builder,
                       int32x TrackGroupIndex,
                       model_instance& Model)
{
    CheckCountedInt32(TrackGroupIndex, Builder.TrackCount, return);
    spu_track_target &Track = Builder.Tracks[TrackGroupIndex];

    Track.OnInstance = &Model;

    MakeDefaultSPUAnimationBindingID(Track.ID, Builder.Animation, TrackGroupIndex, &GetSourceModel(Model));
//     Track.ID.Model = &GetSourceModel(Model);
//     Track.ID.Animation = Builder.Animation;
//     Track.ID.TrackGroup = Builder.Animation->TrackGroups[TrackGroupIndex];
//     Track.ID.TransformTrackCount = Track.ID.TrackGroup->TransformTrackCount;
//     Track.ID.TransformTracks     = Track.ID.TrackGroup->TransformTracks;
//     Track.ID.CurveByteCount      = Track.ID.TrackGroup->CurveByteCount;
//     Track.ID.CurveBytes          = Track.ID.TrackGroup->CurveBytes;
}

