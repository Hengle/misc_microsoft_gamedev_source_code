#if !defined(GRANNY_BONE_OPERATIONS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bone_operations.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_PLATFORM_H)
#include "granny_platform.h"
#endif

#define FORCE_SSE_VERSION PLATFORM_XBOX

BEGIN_GRANNY_NAMESPACE;

struct transform;

#define OPTIMIZED_DISPATCH_TYPE(function) function##_type

#if FORCE_SSE_VERSION
#define OPTIMIZED_DISPATCH(function) function##_type function
#define SSE_VERSION(function) void GRANNY function
#define XENON_VERSION(function) --THIS IS AN ERROR--
#define CELL_VERSION(function) --THIS IS AN ERROR--
#else
#define OPTIMIZED_DISPATCH(function) function##_type *function
#define SSE_VERSION(function) static void function##_SSE
#define XENON_VERSION(function) static void function##_Xenon
#define CELL_VERSION(function) static void function##_Cell
#endif

typedef void OPTIMIZED_DISPATCH_TYPE(BuildIdentityWorldPoseComposite)(real32 const *ParentMatrix,
                                                                      real32 const *InverseWorld4x4,
                                                                      real32 *Result, real32 *ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildIdentityWorldPoseComposite);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildPositionWorldPoseComposite)(real32 const *Position,
                                                                      real32 const *ParentMatrix,
                                                                      real32 const *InverseWorld4x4,
                                                                      real32 *Result, real32 *ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildPositionWorldPoseComposite);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildPositionOrientationWorldPoseComposite)(real32 const *Position, real32 const *Orientation,
                                                                                 real32 const *ParentMatrix, real32 const *InverseWorld4x4,
                                                                                 real32 *Result, real32 * ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildPositionOrientationWorldPoseComposite);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildFullWorldPoseComposite)(transform const &Transform,
                                                                  real32 const *ParentMatrix, real32 const *InverseWorld4x4,
                                                                  real32 *Result, real32 * ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildFullWorldPoseComposite);


typedef void OPTIMIZED_DISPATCH_TYPE(BuildIdentityWorldPoseOnly)(real32 const *ParentMatrix,
                                                                real32 *ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildIdentityWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildPositionWorldPoseOnly)(real32 const *Position,
                                                                 real32 const *ParentMatrix,
                                                                 real32 *ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildPositionWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildPositionOrientationWorldPoseOnly)(real32 const *Position, real32 const *Orientation,
                                                                            real32 const *ParentMatrix,
                                                                            real32 * ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildPositionOrientationWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildFullWorldPoseOnly)(transform const &Transform,
                                                             real32 const *ParentMatrix,
                                                             real32 * ResultWorldMatrix);
extern OPTIMIZED_DISPATCH(BuildFullWorldPoseOnly);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildSingleCompositeFromWorldPose)(real32 const *InverseWorld4x4,
                                                                        real32 const *WorldMatrix,
                                                                        real32 *ResultComposite );
extern OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPose);

typedef void OPTIMIZED_DISPATCH_TYPE(BuildSingleCompositeFromWorldPoseTranspose)(real32 const *InverseWorld4x4,
                                                                                 real32 const *WorldMatrix,
                                                                                 real32 *ResultComposite);
extern OPTIMIZED_DISPATCH(BuildSingleCompositeFromWorldPoseTranspose);


// Routines that only generate the world-pose, not the composite.
void BuildIdentityWorldPoseOnly_Generic(real32 const *ParentMatrix,
                                        real32 *ResultWorldMatrix);
void BuildPositionWorldPoseOnly_Generic(real32 const *Position,
                                        real32 const *ParentMatrix,
                                        real32 *ResultWorldMatrix);
void BuildPositionOrientationWorldPoseOnly_Generic(real32 const *Position, real32 const *Orientation,
                                                   real32 const *ParentMatrix,
                                                   real32 * ResultWorldMatrix);
void BuildFullWorldPoseOnly_Generic(transform const &Transform,
                                    real32 const *ParentMatrix,
                                    real32 * ResultWorldMatrix);


void BuildSingleCompositeFromWorldPose_Generic(
    real32 const *InverseWorld4x4, real32 const *WorldMatrix,
    real32 *ResultComposite );
void BuildSingleCompositeFromWorldPoseTranspose_Generic(
    real32 const *InverseWorld4x4, real32 const *WorldMatrix,
    real32 *ResultComposite3x4 );

// Routines that generate both world-pose and composite at the same time.
void BuildIdentityWorldPoseComposite_Generic(
    real32 const *ParentMatrix,
    real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 *ResultWorldMatrix);
void BuildPositionWorldPoseComposite_Generic(
    real32 const *Position,
    real32 const *ParentMatrix,
    real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 *ResultWorldMatrix);
void BuildPositionOrientationWorldPoseComposite_Generic(
    real32 const *Position, real32 const *Orientation,
    real32 const *ParentMatrix, real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 * ResultWorldMatrix);
void BuildFullWorldPoseComposite_Generic(
    transform const &Transform,
    real32 const *ParentMatrix, real32 const *InverseWorld4x4,
    real32 *ResultComposite, real32 * ResultWorldMatrix);


END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BONE_OPERATIONS_H
#endif
