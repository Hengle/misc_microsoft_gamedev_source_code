#if !defined(GRANNY_TEXTURE_BUILDER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_texture_builder.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_TEXTURE_H)
#include "granny_texture.h"
#endif

#if !defined(GRANNY_IMAGE_OPERATIONS_H)
#include "granny_image_operations.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TextureBuilderGroup);

EXPTYPE struct texture_builder;

EXPAPI GS_SAFE texture_builder *BeginRawTexture(int32x Width, int32x Height,
                                                pixel_layout const &ResultingLayout,
                                                int32x StrideAlignment);
EXPAPI GS_SAFE texture_builder *BeginS3TCTexture(int32x Width, int32x Height,
                                                 s3tc_texture_format Format);
EXPAPI GS_SAFE texture_builder *BeginBestMatchS3TCTexture(int32x Width, int32x Height);

EXPAPI GS_SAFE texture_builder *BeginBinkTexture(int32x Width, int32x Height,
                                                 int32x Compression, uint32x Flags);

EXPAPI GS_PARAM texture *EndTexture(texture_builder *Builder);

EXPAPI GS_SAFE int32x GetResultingTextureSize(texture_builder const &Builder);
EXPAPI GS_PARAM texture *EndTextureInPlace(texture_builder *Builder, void *Memory);

EXPAPI GS_PARAM void SetImageScalingFilter(texture_builder &Builder,
                                           pixel_filter_type UpsamplingFilter,
                                           pixel_filter_type DownsamplingFilter);

EXPAPI GS_PARAM void EncodeImage(texture_builder &Builder,
                                 int32x Width, int32x Height, int32x Stride,
                                 int32x MIPLevelCount, void const *RGBAData);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TEXTURE_BUILDER_H
#endif
