#if !defined(GRANNY_TRACK_SAMPLER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_track_sampler.h $
// $DateTime: 2007/08/17 17:38:16 $
// $Change: 15766 $
// $Revision: #19 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TrackSamplerGroup);

struct transform_track;
struct bound_transform_track;
struct transform;
struct curve2;

EXPTYPE_EPHEMERAL struct sample_context
{
    real32 LocalClock;
    real32 LocalDuration;
    bool UnderflowLoop;
    bool OverflowLoop;

    // This is for keyframe animations only
    int32x FrameIndex;
};

EXPAPI typedef void track_sampler(
    sample_context const &Context,
    transform_track const *SourceTrack,
    bound_transform_track *Track,
    transform const &RestTransform,
    real32 const *InverseWorld4x4,
    real32 const *ParentMatrix,
    real32 *WorldResult,
    real32 *CompositeResult);

#define TrackSampler(Name) \
static void Name(sample_context const &Context, \
                 transform_track const *SourceTrack, \
                 bound_transform_track *Track, \
                 transform const &RestTransform, \
                 real32 const *InverseWorld4x4Aliased, \
                 real32 const *ParentMatrixAliased, \
                 real32 *WorldResultAliased, \
                 real32 *CompositeResultAliased)

EXPAPI GS_READ void SampleTrackUUULocal(sample_context const &Context,
                                        transform_track const *SourceTrack,
                                        bound_transform_track *Track,
                                        transform &Result);
EXPAPI GS_READ void SampleTrackPOLocal(sample_context const &Context,
                                       transform_track const *SourceTrack,
                                       bound_transform_track *Track,
                                       real32 *ResultPosition,
                                       real32 *ResultOrientation);

EXPAPI GS_READ void SampleTrackUUULocalAtTime0(transform_track const *SourceTrack,
                                               transform &Result);

EXPAPI GS_READ track_sampler *GetTrackSamplerFor(transform_track const &Track);
EXPAPI GS_READ track_sampler *GetTrackSamplerUUU(void);
EXPAPI GS_READ track_sampler *GetTrackSamplerSSS(void);
EXPAPI GS_READ track_sampler *GetTrackSamplerIII(void);
EXPAPI GS_READ track_sampler *GetTrackSamplerIIU(void);



track_sampler *GetTrackSamplerUUUAtTime0(void);

void SampleTrackUUUAtTime0(sample_context const &Context,
                 transform_track const *SourceTrack,
                 bound_transform_track *Track,
                 transform const &RestTransform,
                 real32 const *InverseWorld4x4,
                 real32 const *ParentMatrix,
                 real32 *WorldResult,
                 real32 *CompositeResult);
void SampleTrackUUUBlendWithTime0(sample_context const &Context,
                 transform_track const *SourceTrack,
                 bound_transform_track *Track,
                 transform const &RestTransform,
                 real32 const *InverseWorld4x4,
                 real32 const *ParentMatrix,
                 real32 *WorldResult,
                 real32 *CompositeResult,
                 real32 BlendAmount);



int32x CurveKnotCharacter(curve2 const &Curve);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TRACK_SAMPLER_H
#endif
