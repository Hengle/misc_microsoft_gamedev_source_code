#if !defined(GRANNY_CONTROL_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_control.h $
// $DateTime: 2006/12/01 16:01:17 $
// $Change: 13831 $
// $Revision: #18 $
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

BEGIN_GRANNY_NAMESPACE EXPGROUP(ControlGroup);

EXPTYPE struct control;
struct track_mask;
struct animation;

struct control
{
    model_control_binding BindingSentinel;

    enum
    {
        ActiveFlag            = 0x01,
        KillOnceCompleteFlag  = 0x02,
        KillOnceUnusedFlag    = 0x04,
        EaseInFlag            = 0x08,
        EaseOutFlag           = 0x10,
        ForceClampedLoopsFlag = 0x20,
    };
    uint32x Flags;

    // The current clock, used for computing dT, has its origin at the
    // start of global clock time.
    real32 CurrentClock;

    // The local clock, used for actual animation, has its origin at the
    // start of the animation
    // (dTLocalClockPending is really badly named. It should be called
    // dTGlobalClockPending, because you multiply it by Speed before adding it
    // to LocalClock).
    real32 dTLocalClockPending;
    real32 LocalClock;

    // How we update our local clock - dt*Speed
    real32 Speed;

    // How we deal with our boundaries
    real32 LocalDuration;
    int32x LoopIndex;
    int32x LoopCount;

    // When we die, if KillOnceComplete flag is set
    real32 KillClock;

    // How are we blending?
    real32 CurrentWeight;

    // Active based on EaseIn flag
    real32 EaseInStartClock;
    real32 EaseInEndClock;
    uint32x EaseInValues;

    // Active based on EaseOut flag
    real32 EaseOutStartClock;
    real32 EaseOutEndClock;
    uint32x EaseOutValues;

    void *UserData[MaximumUserData];

    control *Next;
    control *Prev;
};

model_control_binding &GetControlBindingSentinel(control const &Control);

EXPAPI GS_MODIFY control *CreateControl(real32 CurrentClock, real32 LocalDuration);

EXPAPI GS_MODIFY void FreeControl(control *Control);
EXPAPI GS_MODIFY void FreeControlOnceUnused(control &Control);

EXPAPI GS_PARAM void CompleteControlAt(control &Control, real32 AtSeconds);
EXPAPI GS_READ real32 GetControlCompletionClock(control const &Control);
EXPAPI GS_READ bool GetControlCompletionCheckFlag(control const &Control);
EXPAPI GS_PARAM void SetControlCompletionCheckFlag(control &Control, bool CheckComplete);

bool ControlHasEffect(control &Control);

EXPAPI GS_READ real32 GetControlClock(control const &Control);
EXPAPI GS_PARAM void SetControlClock(control &Control, real32 NewSeconds);
EXPAPI GS_PARAM void SetControlClockOnly(control &Control, real32 NewSeconds);
EXPAPI GS_READ bool ControlIsComplete(control const &Control);
EXPAPI GS_MODIFY bool FreeControlIfComplete(control *Control);
bool ControlIsUnused(control const &Control);
bool FreeControlIfUnused(control *Control);

EXPAPI GS_READ real32 GetControlWeight(control const &Control);
EXPAPI GS_MODIFY void SetControlWeight(control &Control, real32 Weight);

EXPAPI GS_READ track_mask const* GetControlTrackGroupModelMask(control const &Control,
                                                               model_instance const* ModelInstance);
EXPAPI GS_READ track_mask const* GetControlTrackGroupTrackMask(control const &Control,
                                                               animation const* Animation,
                                                               int32x TrackGroupIndex);

EXPAPI GS_READ int32x GetControlLoopCount(control const &Control);
EXPAPI GS_PARAM void SetControlLoopCount(control &Control, int32x LoopCount);
EXPAPI GS_READ void GetControlLoopState(control &Control,
                                        bool &UnderflowLoop, bool &OverflowLoop);

EXPAPI GS_READ int32x GetControlLoopIndex(control &Control);
EXPAPI GS_PARAM void SetControlLoopIndex(control &Control, int32x LoopIndex);

EXPAPI GS_READ real32 GetControlSpeed(control const &Control);
EXPAPI GS_PARAM void SetControlSpeed(control &Control, real32 Speed);

EXPAPI GS_READ real32 GetControlDuration(control const &Control);
EXPAPI GS_READ real32 GetControlDurationLeft(control &Control);

EXPAPI GS_READ bool ControlIsActive(control const &Control);
EXPAPI GS_MODIFY void SetControlActive(control &Control, bool Active);

EXPAPI GS_READ real32 GetControlClampedLocalClock(control &Control);
EXPAPI GS_READ real32 GetControlLocalDuration(control const &Control);
EXPAPI GS_READ real32 GetControlEaseCurveMultiplier(control const &Control);
EXPAPI GS_READ real32 GetControlEffectiveWeight(control const &Control);

EXPAPI GS_PARAM void SetControlEaseIn(control &Control, bool EaseIn);
EXPAPI GS_PARAM void SetControlEaseInCurve(control &Control,
                                           real32 StartSeconds, real32 EndSeconds,
                                           real32 StartValue, real32 StartTangent,
                                           real32 EndTangent, real32 EndValue);

EXPAPI GS_PARAM void SetControlEaseOut(control &Control, bool EaseOut);
EXPAPI GS_PARAM void SetControlEaseOutCurve(control &Control,
                                            real32 StartSeconds, real32 EndSeconds,
                                            real32 StartValue, real32 StartTangent,
                                            real32 EndTangent, real32 EndValue);

EXPAPI GS_READ real32 GetControlRawLocalClock(control &Control);
EXPAPI GS_PARAM void SetControlRawLocalClock(control &Control, real32 LocalClock);

EXPAPI GS_PARAM real32 EaseControlIn(control &Control, real32 Duration,
                                     bool FromCurrent);
EXPAPI GS_PARAM real32 EaseControlOut(control &Control, real32 Duration);

EXPAPI GS_READ void **GetControlUserDataArray(control const &Control);

EXPAPI GS_READ control *GetGlobalControlsBegin(void);
EXPAPI GS_READ control *GetGlobalControlsEnd(void);
EXPAPI GS_READ control *GetGlobalNextControl(control *Control);

EXPAPI GS_PARAM void RecenterControlClocks(control *Control, real32 dCurrentClock);
EXPAPI GS_MODIFY void RecenterAllControlClocks(real32 dCurrentClock);
EXPAPI GS_MODIFY void RecenterAllModelInstanceControlClocks(model_instance *ModelInstance, real32 dCurrentClock);

EXPAPI GS_PARAM void SetControlForceClampedLooping(control &Control, bool Clamp);

EXPAPI GS_PARAM void SetControlTargetState(control &Control, real32 CurrentGlobalTime, real32 TargetGlobalTime, real32 LocalTime, int32x LoopIndex);

//
// Inlines
//

inline void
SetFlags(control &Control, uint32 Flags, bool Value)
{
    if(Value)
    {
        Control.Flags |= Flags;
    }
    else
    {
        Control.Flags &= ~Flags;
    }
}

inline bool
GetFlags(control const &Control, uint32 Flags)
{
    return((Control.Flags & Flags) == Flags);
}

inline void
SetControlClockInline(control &Control, real32 NewClock)
{
    real32 const dT = NewClock - Control.CurrentClock;
    Control.dTLocalClockPending += dT;

    Control.CurrentClock = NewClock;
}

inline real32
GetControlClampedLocalClockInline(control &Control)
{
    real32 Result = GetControlRawLocalClock(Control);
    if(Result < 0)
    {
        Result = 0;
    }
    else if(Result > Control.LocalDuration)
    {
        Result = Control.LocalDuration;
    }

    return(Result);
}

inline real32
GetControlLocalDurationInline(control const &Control)
{
    return(Control.LocalDuration);
}

// IMPORTANT: NEVER call this without having retrieved the local
// clock first, otherwise you might get incorrect loop results
// since the deferred local clock updating might not have happened.
inline void
GetControlLoopStateInline(control const &Control,
                          bool &UnderflowLoop, bool &OverflowLoop)
{
    if(GetFlags(Control, control::ForceClampedLoopsFlag))
    {
        UnderflowLoop = OverflowLoop = false;
    }
    else
    {
        if(Control.LoopCount == 0)
        {
            UnderflowLoop = OverflowLoop = true;
        }
        else
        {
            UnderflowLoop = (Control.LoopIndex > 0);
            OverflowLoop = (Control.LoopIndex < (Control.LoopCount - 1));
        }
    }
}

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CONTROL_H
#endif
