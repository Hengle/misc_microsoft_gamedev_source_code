#if !defined(GRANNY_BINK_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bink.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
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

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TextureGroup);

EXPTYPE enum bink_texture_flags
{
    BinkEncodeAlpha = 0x1,
    BinkUseScaledRGBInsteadOfYUV = 0x2,
    BinkUseBink1 = 0x4,
};

EXPAPI GS_SAFE pixel_layout const &GetBinkPixelLayout(bool Alpha);

EXPAPI GS_SAFE int32x GetMaximumBinkImageSize(int32x Width, int32x Height,
                                              uint32x Flags, int32x Compression);

EXPAPI GS_PARAM int32x BinkCompressTexture(int32x Width, int32x Height,
                                           int32x SourceStride, void const *Source,
                                           uint32x Flags, int32x Compression, void *Dest);
EXPAPI GS_SAFE void BinkDecompressTexture(int32x Width, int32x Height,
                                          uint32x Flags,
                                          int32x SourceSize, void const *Source,
                                          pixel_layout const &DestLayout,
                                          int32x DestStride, void *Dest);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BINK_H
#endif
