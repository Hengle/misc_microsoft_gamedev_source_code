// ========================================================================
// $File: //jeffr/granny/rt/granny_texture.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TEXTURE_H)
#include "granny_texture.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

#if !defined(GRANNY_S3TC_H)
#include "granny_s3tc.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_BINK_H)
#include "granny_bink.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode TextureLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

static char const *TextureTypeNames[] =
{
    "Color map",
    "Cube map",
};

static char const *TextureEncodingNames[] =
{
    "User-defined texture format",
    "Raw texture format",
    "S3TC format",
    "Bink texture format",
};

data_type_definition TextureMIPLevelType[] =
{
    {Int32Member, "Stride"},
    {ReferenceToArrayMember, "PixelBytes", UInt8Type},
    {EndMember},
};

data_type_definition TextureImageType[] =
{
    {ReferenceToArrayMember, "MIPLevels", TextureMIPLevelType},
    {EndMember},
};

data_type_definition TextureType[] =
{
    {StringMember, "FromFileName"},

    {Int32Member, "TextureType"},
    {Int32Member, "Width"},
    {Int32Member, "Height"},

    {Int32Member, "Encoding"},
    {Int32Member, "SubFormat"},
    {InlineMember, "Layout", PixelLayoutType},

    {ReferenceToArrayMember, "Images", TextureImageType},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

char const *GRANNY
GetTextureTypeName(int32x TextureType)
{
    if(TextureType < OnePastLastTextureType)
    {
        return(TextureTypeNames[TextureType]);
    }
    else
    {
        return("Unknown texture type");
    }
}

char const *GRANNY
GetTextureEncodingName(int32x Encoding)
{
    if(Encoding < OnePastLastTextureEncoding)
    {
        return(TextureEncodingNames[Encoding]);
    }
    else
    {
        return("Unknown texture format");
    }
}

int32x GRANNY
GetRawImageSize(pixel_layout const &Layout, int32x Stride,
                int32x Width, int32x Height)
{
    return(Stride * Height);
}

int32x GRANNY
GetS3TCImageSize(s3tc_texture_format Format, int32x Width, int32x Height)
{
    if (Width  < 4) Width  = 4;
    if (Height < 4) Height = 4;

    switch(Format)
    {
        case S3TCBGR565:
        case S3TCBGRA5551:
        {
            // These formats are 4 bits per pixel
            Assert(((Height*Width) % 2) == 0);
            return((Height*Width) / 2);
        } //break;

        case S3TCBGRA8888MappedAlpha:
        case S3TCBGRA8888InterpolatedAlpha:
        {
            // These formats are 8 bits per pixel
            return(Height * Width);
        } //break;

        default:
        {
            Log1(ErrorLogMessage, TextureLogMessage,
                 "Unrecognized S3TC texture format (%d)", Format);
            return(0);
        } //break;
    }
}

bool GRANNY
GetRecommendedPixelLayout(texture const &Texture, pixel_layout &Layout)
{
    bool Result = false;

    switch(Texture.Encoding)
    {
        case RawTextureEncoding:
        case BinkTextureEncoding:
        {
            Layout = Texture.Layout;
            Result = true;
        } break;

        case S3TCTextureEncoding:
        {
            Layout = GetS3TCPixelLayout((s3tc_texture_format)
                                        Texture.SubFormat);
            Result = true;
        } break;
    }

    return(Result);
}

bool GRANNY
CopyTextureImage(texture const &Texture,
                 int32x ImageIndex,
                 int32x MIPIndex,
                 pixel_layout const &Layout,
                 int32x DestWidth, int32x DestHeight, int32x DestStride,
                 void *Pixels)
{
    bool Result = false;

    CheckCountedInt32(ImageIndex, Texture.ImageCount, return(false));
    texture_image Image = Texture.Images[ImageIndex];

    CheckCountedInt32(MIPIndex, Image.MIPLevelCount, return(false));
    texture_mip_level MIPLevel = Image.MIPLevels[MIPIndex];

    int32x SourceWidth = Texture.Width >> MIPIndex;
    if(SourceWidth < 1)
    {
        SourceWidth = 1;
    }

    int32x SourceHeight = Texture.Height >> MIPIndex;
    if(SourceHeight < 1)
    {
        SourceHeight = 1;
    }

    switch(Texture.Encoding)
    {
        case RawTextureEncoding:
        {
            // TODO: Fast path for images that are exactly equal
            if((SourceWidth == DestWidth) && (SourceHeight == DestHeight))
            {
                ConvertPixelFormat(SourceWidth, SourceHeight,
                                   Texture.Layout, MIPLevel.Stride,
                                   MIPLevel.PixelBytes,
                                   Layout, DestStride, Pixels);
                Result = true;
            }
            else
            {
                LogNotImplemented("Automatic rescaling not supported");
            }
        } break;

        case S3TCTextureEncoding:
        {
            uint8 *TempBuffer = (uint8 *)AllocateSize(
                SourceWidth * SourceHeight * 4);
            if(TempBuffer)
            {
                switch(Texture.SubFormat)
                {
                    case S3TCBGR565:
                    case S3TCBGRA5551:
                    {
                        from_S3TC1((unsigned int *)TempBuffer,
                                   (unsigned short *)MIPLevel.PixelBytes,
                                   SourceWidth, SourceHeight);
                    } break;

                    case S3TCBGRA8888MappedAlpha:
                    {
                        from_S3TC2or3((unsigned int *)TempBuffer,
                                      (unsigned short *)MIPLevel.PixelBytes,
                                      SourceWidth, SourceHeight);
                    } break;

                    case S3TCBGRA8888InterpolatedAlpha:
                    {
                        from_S3TC4or5((unsigned int *)TempBuffer,
                                      (unsigned short *)MIPLevel.PixelBytes,
                                      SourceWidth, SourceHeight);
                    } break;

                    default:
                    {
                        Log1(ErrorLogMessage, TextureLogMessage,
                             "Unrecognized S3 texture format (%d)",
                             Texture.SubFormat);
                    } break;
                }

                if((SourceWidth == DestWidth) &&
                   (SourceHeight == DestHeight))
                {
                    ConvertPixelFormat(SourceWidth, SourceHeight,
                                       BGRA8888PixelFormat,
                                       SourceWidth *
                                       RGBA8888PixelFormat.BytesPerPixel,
                                       TempBuffer,
                                       Layout, DestStride, Pixels);
                    Result = true;
                }
                else
                {
                    LogNotImplemented("Automatic rescaling not supported");
                }

                Deallocate(TempBuffer);
            }
        } break;

        case BinkTextureEncoding:
        {
            if((SourceWidth == DestWidth) && (SourceHeight == DestHeight))
            {
                uint32x Flags = Texture.SubFormat;
                if(PixelLayoutHasAlpha(Texture.Layout))
                {
                    Flags |= BinkEncodeAlpha;
                }

                BinkDecompressTexture(SourceWidth, SourceHeight,
                                      Flags,
                                      MIPLevel.PixelByteCount, MIPLevel.PixelBytes,
                                      Layout, DestStride, Pixels);
                Result = true;
            }
            else
            {
                LogNotImplemented("Automatic rescaling not supported");
            }
        } break;
    }

    return(Result);
}

int32x GRANNY
ClosestPowerOf2(int32x Value)
{
    int32x Power = 0;
    int32x Test = Value;
    while(Test >>= 1)
    {
        ++Power;
    }

    // This is a rickety algorithm, so don't push it.
    Assert(Power <= 30);

    int32x LowPowerOfTwo = 1 << Power;
    int32x HighPowerOfTwo = 2 << Power;

    if((Value - LowPowerOfTwo) < (HighPowerOfTwo - Value))
    {
        return(LowPowerOfTwo);
    }
    else
    {
        return(HighPowerOfTwo);
    }
}

static int32x
ClipToMinMax(int32x Minimum, int32x Value, int32x Maximum)
{
    if(Value < Minimum)
    {
        Value = Minimum;
    }

    if(Value > Maximum)
    {
        Value = Maximum;
    }

    return(Value);
}

void GRANNY
ClipTextureDimensions(int32x MinimumWidth, int32x MinimumHeight,
                      int32x MaximumWidth, int32x MaximumHeight,
                      int32x Flags, int32x &Width, int32x &Height)
{
    if(Flags & TextureWidthMustBePowerOf2)
    {
        Width = ClosestPowerOf2(Width);
    }
    Width = ClipToMinMax(MinimumWidth, Width, MaximumWidth);

    if(Flags & TextureHeightMustBePowerOf2)
    {
        Height = ClosestPowerOf2(Height);
    }
    Height = ClipToMinMax(MinimumHeight, Height, MaximumHeight);
}

bool GRANNY
TextureHasAlpha(texture const &Texture)
{
    return(PixelLayoutHasAlpha(Texture.Layout));
}
