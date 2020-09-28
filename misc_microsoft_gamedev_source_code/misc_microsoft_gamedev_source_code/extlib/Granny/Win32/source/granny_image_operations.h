#if !defined(GRANNY_IMAGE_OPERATIONS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_image_operations.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

/*
  I adapted this code from code that Jeff Roberts' gave me, which he
  adapted from Graphics Gems III.  So by now there's sure to be quite a
  few replicative fade errors in it :)
*/

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TextureGroup);

EXPTYPE enum pixel_filter_type
{
    CubicPixelFilter,
    LinearPixelFilter,
    BoxPixelFilter,

    OnePastLastPixelFilterType
};

EXPAPI GS_PARAM void ScaleImage(pixel_filter_type FilterType,
                                int32x SourceWidth, int32x SourceHeight,
                                int32x SourceStride, uint8 const *SourcePixels,
                                int32x DestWidth, int32x DestHeight,
                                int32x DestStride, uint8 *DestPixels);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_IMAGE_OPERATIONS_H
#endif
