#if !defined(GRANNY_MODEL_CONTROL_BINDING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_model_control_binding.h $
// $DateTime: 2007/08/23 11:05:46 $
// $Change: 15817 $
// $Revision: #16 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ControlGroup);

struct transform;
struct control;
struct model_instance;
struct local_pose;
struct world_pose;

EXPTYPE struct model_control_binding;
EXPTYPE struct controlled_animation;
EXPTYPE struct controlled_pose;

typedef bool initialize_binding_state(model_control_binding &Binding,
                                      void *InitData);
typedef void accumulate_binding_state(model_control_binding &Binding,
                                      int32x FirstBone, int32x BoneCount,
                                      local_pose &Result, real32 AllowedError,
                                      int32x const *SparseBoneArray);
typedef void build_binding_direct(model_control_binding &Binding,
                                  int32x BoneCount,
                                  real32 const *Offset4x4,
                                  world_pose &Result,
                                  real32 AllowedError);
typedef void accumulate_loop_transform(model_control_binding &Binding,
                                       real32 SecondsElapsed,
                                       real32 &TotalWeight,
                                       real32 *ResultTranslation,
                                       real32 *ResultRotation,
                                       bool Inverse);
typedef void cleanup_binding_state(model_control_binding &Binding);
typedef controlled_animation *get_controlled_animation ( model_control_binding &Binding );
typedef controlled_pose *get_controlled_pose ( model_control_binding &Binding );

struct model_control_callbacks
{
    get_controlled_animation *GetControlledAnimation;
    get_controlled_pose *GetControlledPose;
    initialize_binding_state *InitializeBindingState;
    accumulate_binding_state *AccumulateBindingState;
    build_binding_direct *BuildBindingDirect;
    accumulate_loop_transform *AccumulateLoopTransform;
    cleanup_binding_state *CleanupBindingState;
};

// This enumeration value is stored in the ReservedPointer member of the
// model_control_binding to indicate what sort of binding it is.  This is slightly
// suspect, since technically, the Callbacks member servse the same purpose, but we need a
// method that doesn't depend on structure addresses to work across the PPU/SPU boundary
// on the PS3.
enum model_control_binding_type
{
    ControlledPose    = 0,
    ControlledAnim    = 1,
    SPUControlledAnim = 2
};

struct model_control_binding
{
    // This is the ring of bindings that share the same control
    control *Control;
    model_control_binding *ControlPrev;
    model_control_binding *ControlNext;

    // This is the ring of bindings that share the same model
    model_instance *ModelInstance;
    model_control_binding *ModelPrev;
    model_control_binding *ModelNext;

    model_control_callbacks const *Callbacks;

    // This is where either a controlled_pose or a controlled_animation structure live.
    // Note that this field must be larger on 64 bit platforms to accommodate the larger
    //  pointer sizes
    uint32 Derived[4 * (SizeOf(void*) / SizeOf(uint32))];

    // See note on binding_type above...
    void* ReservedPointer;
};
CompileAssert(IS_ALIGNED_16(sizeof(model_control_binding)));

void UnlinkModelControlBinding(model_control_binding &Binding);
void LinkModelControlBinding(model_control_binding &Binding,
                             control &Control,
                             model_instance &ModelInstance,
                             bool Active);
void RelinkModelSideOfControlBinding(model_control_binding &Binding,
                                     bool Active);

void InitializeSentinel(model_control_binding &Binding);

model_control_binding *CreateModelControlBinding(
    model_control_callbacks const &Callbacks,
    control &Control, model_instance &ModelInstance,
    bool Active, void *InitData);
void FreeModelControlBinding(model_control_binding *Binding);

void SampleModelControlBindingLODSparse(model_control_binding &Binding,
                                        int32x FirstBone, int32x BoneCount,
                                        local_pose &Result, real32 AllowedError,
                                        int32x const *SparseBoneArray);
void BuildBindingDirect(model_control_binding &Binding,
                        int32x BoneCount,
                        real32 const *Offset4x4,
                        world_pose &Result,
                        real32 AllowedError);
void AccumulateModelControlBindingLoopTransform(
    model_control_binding &Binding, real32 SecondsElapsed,
    real32 &TotalWeight, real32 *ResultTranslation, real32 *ResultRotation, bool Inverse);

void FreeControlRing(model_control_binding &Sentinel);
void FreeModelRing(model_control_binding &Sentinel);

EXPAPI GS_READ model_control_binding &ModelControlsBegin(model_instance &Model);
EXPAPI GS_READ model_control_binding &ModelControlsNext(model_control_binding &Binding);
EXPAPI GS_READ model_control_binding &ModelControlsEnd(model_instance &Model);

EXPAPI GS_READ model_control_binding &ControlModelsBegin(control &Control);
EXPAPI GS_READ model_control_binding &ControlModelsNext(model_control_binding &Binding);
EXPAPI GS_READ model_control_binding &ControlModelsEnd(control &Control);

EXPAPI GS_READ model_instance *GetModelInstanceFromBinding(model_control_binding &Binding);
EXPAPI GS_READ control *GetControlFromBinding(model_control_binding &Binding);




#define GET_DERIVED(type, Binding) (type *)&(Binding).Derived; Assert(SizeOf(type) <= SizeOf((Binding).Derived))

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MODEL_CONTROL_BINDING_H
#endif
