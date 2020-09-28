#if !defined(GRANNY_S3TC_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_s3tc.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

/*
  This S3TC codec code is adapted from code Jeff Roberts wrote for me.
 */

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TextureGroup);

struct pixel_layout;

EXPTYPE enum s3tc_texture_format
{
    S3TCBGR565, // Equivalent to DXT1 (w/out alpha)
    S3TCBGRA5551, // Equivalent to DXT1 (w/ alpha)
    S3TCBGRA8888MappedAlpha, // Equivalent to DXT2/3
    S3TCBGRA8888InterpolatedAlpha, // Equivalent to DXT4/5

    OnePastLastS3TCTextureFormat
};

EXPAPI GS_SAFE pixel_layout const &GetS3TCPixelLayout(s3tc_texture_format Format);
EXPAPI GS_SAFE char const *GetS3TCTextureFormatName(int32x Format);

// Compressors
unsigned int to_S3TC1(void *output, const void *input,
                      unsigned int instride,
                      unsigned int width, unsigned int height);
unsigned int to_S3TC1a(void *output, const void *input,
                       unsigned int instride,
                       unsigned int width, unsigned int height);
unsigned int to_S3TC2or3(void *output, const void *input,
                         unsigned int instride,
                         unsigned int width, unsigned int height);
unsigned int to_S3TC4or5(void *output, const void *input,
                         unsigned int instride,
                         unsigned int width, unsigned int height);
unsigned int to_S3TC2or3vs4or5(void *output, const void *input,
                               unsigned int instride,
                               unsigned int width, unsigned int height,
                               unsigned int *type);

// Decompressors
void from_S3TC1(unsigned int *outbuf, unsigned short *s3buf,
                unsigned int width, unsigned int height);
void from_S3TC2or3(unsigned int *outbuf, unsigned short *s3buf,
                   unsigned int width, unsigned int height);
void from_S3TC4or5(unsigned int *outbuf, unsigned short *s3buf,
                   unsigned int width, unsigned int height);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_S3TC_H
#endif
