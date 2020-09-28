// ========================================================================
// $File: //jeffr/granny/rt/granny_animation.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition AnimationType[] =
{
    {StringMember, "Name"},

    {Real32Member, "Duration"},
    {Real32Member, "TimeStep"},
    {Real32Member, "Oversampling"},

    {ArrayOfReferencesMember, "TrackGroups", TrackGroupType},

    {EndMember},
};

END_GRANNY_NAMESPACE;

bool GRANNY
FindTrackGroupForModel(animation const &Animation,
                       char const *ModelName,
                       int32x &TrackGroupIndex)
{
    {for(TrackGroupIndex = 0;
         TrackGroupIndex < Animation.TrackGroupCount;
         ++TrackGroupIndex)
    {
        if (StringsAreEqualLowercaseOrCallback(
                Animation.TrackGroups[TrackGroupIndex]->Name,
                ModelName))
        {
            return true;
        }
    }}

    return(false);
}
