// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation.cpp $
// $DateTime: 2007/08/21 13:45:48 $
// $Change: 15779 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(PS3_GRANNY_ANIMATION_H)
#include "granny_spu_animation.h"
#endif

#if !defined(PS3_GRANNY_TRACK_GROUP_H)
#include "granny_spu_track_group.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

data_type_definition GRANNY SPUAnimationType[] =
{
    {StringMember, "Name"},

    {Real32Member, "Duration"},
    {Real32Member, "TimeStep"},
    {Real32Member, "Oversampling"},

    {ArrayOfReferencesMember, "TrackGroups", SPUTrackGroupType},

    {EndMember},
};

bool GRANNY
FindSPUTrackGroupForModel(spu_animation const& Animation,
                          char const* ModelName,
                          int32x& TrackGroupIndex)
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

    return false;
}
