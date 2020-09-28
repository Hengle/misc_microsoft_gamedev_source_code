#if !defined(GRANNY_TYPE_LISTING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_type_listing.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(DataTypeGroup);

struct data_type_definition;
EXPTYPE_EPHEMERAL struct defined_type
{
    int32x UIID;
    char *Name;
    data_type_definition *Definition;
};

EXPAPI GS_SAFE int32x GetDefinedTypeCount(void);
EXPCONST extern defined_type DefinedTypes[];

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TYPE_LISTING_H
#endif
