#if !defined(GRANNY_ANIMATION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_animation.h $
// $DateTime: 2007/08/03 14:25:32 $
// $Change: 15661 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(AnimationGroup);

struct track_group;

EXPTYPE struct animation
{
    char const *Name;

    real32 Duration;
    real32 TimeStep;
    real32 Oversampling;

    int32 TrackGroupCount;
    track_group **TrackGroups;
};
EXPCONST EXPGROUP(animation) extern data_type_definition AnimationType[];

EXPAPI GS_READ bool FindTrackGroupForModel(animation const &Animation,
                                           char const *ModelName,
                                           int32x &TrackGroupIndex);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_ANIMATION_H
#endif
