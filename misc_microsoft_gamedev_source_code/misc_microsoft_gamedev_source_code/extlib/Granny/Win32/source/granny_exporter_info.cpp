// ========================================================================
// $File: //jeffr/granny/rt/granny_exporter_info.cpp $
// $DateTime: 2006/12/01 16:45:14 $
// $Change: 13833 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_EXPORTER_INFO_H)
#include "granny_exporter_info.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition ExporterInfoType[] =
{
    {StringMember, "ExporterName"},

    {Int32Member, "ExporterMajorRevision"},
    {Int32Member, "ExporterMinorRevision"},
    {Int32Member, "ExporterCustomization"},
    {Int32Member, "ExporterBuildNumber"},

    {VariantReferenceMember, "ExtendedData" },

    {EndMember},
};

END_GRANNY_NAMESPACE;
