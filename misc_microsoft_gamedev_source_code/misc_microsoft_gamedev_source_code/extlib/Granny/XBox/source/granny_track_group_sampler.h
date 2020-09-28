#if !defined(GRANNY_TRACK_GROUP_SAMPLER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_track_group_sampler.h $
// $DateTime: 2007/04/24 15:06:35 $
// $Change: 14841 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TrackGroupSamplerGroup);

struct track_group;
EXPTYPE struct track_group_sampler;

EXPAPI GS_SAFE track_group_sampler *BeginSampledAnimation(int32x TransformCurveCount,
                                                          int32x SampleCount);
EXPAPI GS_PARAM void EndSampledAnimation(track_group_sampler *Sampler);

EXPAPI GS_SAFE track_group_sampler *BeginSampledAnimationNonBlocked(int32x TransformCurveCount,
                                                                    int32x SampleCount);


EXPAPI GS_PARAM real32 *GetPositionSamples(track_group_sampler &Sampler,
                                           int32x TrackIndex);
EXPAPI GS_PARAM real32 *GetOrientationSamples(track_group_sampler &Sampler,
                                              int32x TrackIndex);
EXPAPI GS_PARAM real32 *GetScaleShearSamples(track_group_sampler &Sampler,
                                             int32x TrackIndex);

EXPAPI GS_PARAM void SetTransformSample(track_group_sampler &Sampler,
                                        int32x TrackIndex,
                                        real32 const *Position3,
                                        real32 const *Orientation4,
                                        real32 const *ScaleShear3x3);
EXPAPI GS_PARAM void PushSampledFrame(track_group_sampler &Sampler);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TRACK_GROUP_SAMPLER_H
#endif
