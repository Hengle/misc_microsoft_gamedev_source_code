#if !defined(GRANNY_ART_TOOL_INFO_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_art_tool_info.h $
// $DateTime: 2006/12/01 16:45:14 $
// $Change: 13833 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ArtToolInfoGroup);

EXPTYPE struct art_tool_info
{
    char const *FromArtToolName;
    int32 ArtToolMajorRevision;
    int32 ArtToolMinorRevision;

    real32 UnitsPerMeter;
    triple Origin;
    triple RightVector;
    triple UpVector;
    triple BackVector;

    variant ExtendedData;
};
EXPCONST EXPGROUP(art_tool_info) extern data_type_definition ArtToolInfoType[];

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_ART_TOOL_INFO_H
#endif
