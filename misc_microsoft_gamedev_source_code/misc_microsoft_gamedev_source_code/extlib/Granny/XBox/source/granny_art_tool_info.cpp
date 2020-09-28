// ========================================================================
// $File: //jeffr/granny/rt/granny_art_tool_info.cpp $
// $DateTime: 2006/12/01 16:45:14 $
// $Change: 13833 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_ART_TOOL_INFO_H)
#include "granny_art_tool_info.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition ArtToolInfoType[] =
{
    {StringMember, "FromArtToolName"},
    {Int32Member, "ArtToolMajorRevision"},
    {Int32Member, "ArtToolMinorRevision"},

    {Real32Member, "UnitsPerMeter"},
    {Real32Member, "Origin", 0, 3},
    {Real32Member, "RightVector", 0, 3},
    {Real32Member, "UpVector", 0, 3},
    {Real32Member, "BackVector", 0, 3},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;
