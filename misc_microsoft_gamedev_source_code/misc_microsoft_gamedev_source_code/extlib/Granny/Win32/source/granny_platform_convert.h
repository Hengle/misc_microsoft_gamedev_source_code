#if !defined(GRANNY_PLATFORM_CONVERT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_platform_convert.h $
// $DateTime: 2007/10/24 13:15:43 $
// $Change: 16361 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

BEGIN_GRANNY_NAMESPACE;

struct file;

struct temp_grn_reference
{
    uint32x SectionIndex;
    int32x  SubsectionIndex;
    uint32x Offset;
};

struct fixup_remap
{
    grn_reference      OldLocation;
    temp_grn_reference NewLocation;

    fixup_remap *Left;
    fixup_remap *Right;
};

struct temp_pointer_fixup
{
    temp_grn_reference PointerLocation;
    temp_grn_reference ObjectLocation;
};

struct temp_mixed_marshall
{
    temp_grn_reference ObjectLocation;
    temp_grn_reference TypeLocation;
    int32x ArrayCount;
};


bool
PlatformConversion(int32x SourcePointerSize,
                   bool   SourceIsLittleEndian,
                   int32x DestPointerSize,
                   bool   DestIsLittleEndian,
                   bool   ExcludeTypeTree,
                   file*  ConvertFile,
                   grn_pointer_fixup **SectionFixups,
                   grn_mixed_marshalling_fixup **SectionMarshalls);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_PLATFORM_CONVERT_H
#endif
