#if !defined(GRANNY_TEXTURE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_texture.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

#if !defined(GRANNY_S3TC_H)
#include "granny_s3tc.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(TextureGroup);

EXPTYPE enum texture_type
{
    ColorMapTextureType,
    CubeMapTextureType,
    OnePastLastTextureType
};

EXPTYPE enum texture_encoding
{
    UserTextureEncoding,
    RawTextureEncoding,
    S3TCTextureEncoding,
    BinkTextureEncoding,
    OnePastLastTextureEncoding
};

EXPTYPE struct texture_mip_level
{
    int32 Stride;
    int32  PixelByteCount;
    void  *PixelBytes;
};
EXPCONST EXPGROUP(texture_mip_level) extern data_type_definition TextureMIPLevelType[];

EXPTYPE struct texture_image
{
    int32              MIPLevelCount;
    texture_mip_level *MIPLevels;
};
EXPCONST EXPGROUP(texture_image) extern data_type_definition TextureImageType[];

EXPTYPE struct texture
{
    char const *FromFileName;

    int32 TextureType;
    int32 Width;
    int32 Height;

    int32 Encoding;
    int32 SubFormat;
    pixel_layout Layout;

    int32          ImageCount;
    texture_image *Images;

    variant ExtendedData;
};
EXPCONST EXPGROUP(texture) extern data_type_definition TextureType[];

EXPAPI GS_SAFE char const *GetTextureTypeName(int32x TextureType);
EXPAPI GS_SAFE char const *GetTextureEncodingName(int32x Encoding);

EXPAPI GS_SAFE int32x GetRawImageSize(pixel_layout const &Layout, int32x Stride,
                                      int32x Width, int32x Height);
EXPAPI GS_SAFE int32x GetS3TCImageSize(s3tc_texture_format Format,
                                       int32x Width, int32x Height);

EXPAPI GS_SAFE bool GetRecommendedPixelLayout(texture const &Texture,
                                              pixel_layout &Layout);
EXPAPI GS_PARAM bool CopyTextureImage(texture const &Texture,
                                      int32x ImageIndex, int32x MIPIndex,
                                      pixel_layout const &Layout,
                                      int32x DestWidth, int32x DestHeight,
                                      int32x DestStride, void *Pixels);

enum texture_creation_flags
{
    TextureWidthMustBePowerOf2 = 0x1,
    TextureHeightMustBePowerOf2 = 0x2
};
int32x ClosestPowerOf2(int32x Value);
void ClipTextureDimensions(int32x MinimumWidth, int32x MinimumHeight,
                           int32x MaximumWidth, int32x MaximumHeight,
                           int32x Flags, int32x &Width, int32x &Height);

EXPAPI GS_READ bool TextureHasAlpha(texture const &Texture);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_TEXTURE_H
#endif
