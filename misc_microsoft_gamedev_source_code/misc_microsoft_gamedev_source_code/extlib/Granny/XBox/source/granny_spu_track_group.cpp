// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_track_group.cpp $
// $DateTime: 2007/09/04 11:07:13 $
// $Change: 15884 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_spu_track_group.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_TRANSFORM_TRACK_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

#undef SubsystemCode
#define SubsystemCode TrackGroupLogMessage

BEGIN_GRANNY_NAMESPACE;

data_type_definition SPUTransformTrackType[] =
{
    {UInt32Member, "FromNameIndex"},
    {UInt32Member, "Flags"},

    {Real32Member,    "AnimLODValue"},
    {TransformMember, "Transform" },

    {UInt32Member,  "PosCurveOffset"},
    {UInt32Member,  "OriCurveOffset"},
    {UInt32Member,  "SSCurveOffset"},

    {UInt32Member, "Reserved"},

    {EndMember},
};

data_type_definition SPUTrackGroupType[] =
{
    { StringMember, "Name" },
    { Int32Member,  "Flags" },

    { ReferenceToArrayMember, "TrackNames",      StringType },
    { ReferenceToArrayMember, "TransformTracks", SPUTransformTrackType },
    { ReferenceToArrayMember, "CurveBytes",      UInt8Type },

    { EndMember },
};

END_GRANNY_NAMESPACE;
