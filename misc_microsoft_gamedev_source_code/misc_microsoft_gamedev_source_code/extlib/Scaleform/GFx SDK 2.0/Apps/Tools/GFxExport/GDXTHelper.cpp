/**********************************************************************

Filename    :   GDXTHelper.cpp
Content     :   Interface to NVidia DXT library 
Created     :   September, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFile.h"
#include "GImage.h"
#include "GFxString.h"
#include "GFxStream.h"
#include "GDXTHelper.h"

// Standard includes
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <zlib.h>

#include <direct.h>

#include <dxtlib/dxtlib.h>

int GDXTHelper::LastError = NV_OK;

static NV_ERROR_CODE nvDXTWriteCallback(const void *buffer, 
                                        size_t count,  
                                        const MIPMapData * mipMapData, 
                                        void * userData)
{
    if (mipMapData)
    {
        // means a MIP map
    }
    else
    {
        // otherwise file header data
    }

    GFile* pfile = reinterpret_cast<GFile*>(userData);
    return (pfile->Write((const UByte*)buffer, count) == int(count)) ? NV_OK : NV_WRITE_FAILED;
}

struct InMemBuf
{
    UByte* pData;
    size_t Size;
};

static NV_ERROR_CODE nvDXTWriteInMemCallback(const void *buffer, 
                                             size_t count,  
                                             const MIPMapData * mipMapData, 
                                             void * userData)
{
    if (mipMapData)
    {
        // means a MIP map
    }
    else
    {
        // otherwise file header data
    }

    InMemBuf& imbuf = *reinterpret_cast<InMemBuf*>(userData);
    size_t oldSize = imbuf.Size;
    imbuf.Size += count;
    imbuf.pData = (UByte*)GREALLOC(imbuf.pData, imbuf.Size);
    memcpy(imbuf.pData+oldSize, buffer, count);

    return NV_OK;
}

static void GImage2NvImage(const GImage& srcImg, RGBAImage* pdstImage)
{
    GASSERT(pdstImage);
    pdstImage->resize(srcImg.Width, srcImg.Height);
    for (UInt y = 0; y < srcImg.Height; y++)
    {
        const UByte* pscanline = srcImg.GetScanline(y);
        switch(srcImg.Format)
        {
        case GImage::Image_RGB_888:
            {
                for (UInt x = 0; x < srcImg.Width; x++, pscanline += 3)
                {
                    rgba_t & pix = (*pdstImage)(y, x);

                    pix.r = pscanline[0];
                    pix.g = pscanline[1];
                    pix.b = pscanline[2];
                    pix.a = 0xFF;

                }
            }
            break;

        case GImage::Image_ARGB_8888:
            {
                for (UInt x = 0; x < srcImg.Width; x++, pscanline += 4)
                {
                    rgba_t & pix = (*pdstImage)(y, x);

                    pix.r = pscanline[0];
                    pix.g = pscanline[1];
                    pix.b = pscanline[2];
                    pix.a = pscanline[3];

                }

            }
            break;

        case GImage::Image_A_8:
            {
                for (UInt x = 0; x < srcImg.Width; x++, pscanline += 1)
                {
                    rgba_t & pix = (*pdstImage)(y, x);

                    pix.r = pscanline[0];
                    pix.g = pscanline[0];
                    pix.b = pscanline[0];
                    pix.a = pscanline[0];

                }
            }
            break;

        }
    }
}

#if 0
static GImage* NvImage2GImage(const RGBAImage& srcImg)
{
    GImage* pdstImage = GImage::CreateImage(GImage::Image_ARGB_8888, srcImg.width(), srcImg.height());
    GASSERT(pdstImage);

    for (UInt y = 0; y < srcImg.height(); y++)
    {
        UByte* pscanline = pdstImage->GetScanline(y);
        for (UInt x = 0; x < srcImg.width(); x++, pscanline += 4)
        {
            const rgba_t & pix = srcImg(y, x);

            pscanline[0] = pix.r;
            pscanline[1] = pix.g;
            pscanline[2] = pix.b;
            pscanline[3] = pix.a;
        }
    }

    return pdstImage;
}
#endif

static nvRescaleTypes NvRescaleConst(GDXTHelper::RescaleTypes rescale)
{
    switch(rescale)
    {
    case GDXTHelper::RescaleNone: return kRescaleNone;
    case GDXTHelper::RescaleNearestPower2: return kRescaleNearestPower2;    // rescale to nearest power of two
    case GDXTHelper::RescaleBiggestPower2: return kRescaleBiggestPower2;   // rescale to next bigger power of 2
    case GDXTHelper::RescaleSmallestPower2: return kRescaleSmallestPower2;  // rescale to smaller power of 2 
    case GDXTHelper::RescaleNextSmallestPower2: return kRescaleNextSmallestPower2;  // rescale to next smaller power of 2
    case GDXTHelper::RescalePreScale: return kRescalePreScale;           // rescale to this size
    case GDXTHelper::RescaleRelScale: return kRescaleRelScale;           // relative rescale
    }
    GASSERT(0);
    return kRescaleNone;
}

static nvMipFilterTypes NvFilterConst(GDXTHelper::FilterTypes filter)
{
    switch(filter)
    {
    case GDXTHelper::FilterPoint:       return kMipFilterPoint;
    case GDXTHelper::FilterBox:         return kMipFilterBox;    
    case GDXTHelper::FilterTriangle:    return kMipFilterTriangle;
    case GDXTHelper::FilterQuadratic:   return kMipFilterQuadratic;
    case GDXTHelper::FilterCubic:       return kMipFilterCubic;
    case GDXTHelper::FilterCatrom:      return kMipFilterCatrom;
    case GDXTHelper::FilterMitchell:    return kMipFilterMitchell;
    case GDXTHelper::FilterGaussian:    return kMipFilterGaussian;
    case GDXTHelper::FilterSinc:        return kMipFilterSinc;
    case GDXTHelper::FilterBessel:      return kMipFilterBessel;
    case GDXTHelper::FilterHanning:     return kMipFilterHanning;
    case GDXTHelper::FilterHamming:     return kMipFilterHamming;
    case GDXTHelper::FilterBlackman:    return kMipFilterBlackman;
    case GDXTHelper::FilterKaiser:      return kMipFilterKaiser;
    }
    GASSERT(0);
    return kMipFilterCubic;
}

static nvQualitySetting NvQualityConst(GDXTHelper::QualitySetting qual)
{
    switch(qual)
    {
    case GDXTHelper::QualityFastest:    return kQualityFastest;
    case GDXTHelper::QualityNormal:     return kQualityNormal;
    case GDXTHelper::QualityProduction: return kQualityProduction;
    case GDXTHelper::QualityHighest:    return kQualityHighest;
    }
    GASSERT(0);
    return kQualityFastest;
}

GImage* GDXTHelper::Rescale(const GImage* psrcimage, 
                            GDXTHelper::RescaleTypes rescale, 
                            GDXTHelper::FilterTypes rescaleFilter, 
                            unsigned targetWidth,
                            unsigned targetHeight,
                            GImage::ImageFormat destImageFormat)
{
    RGBAImage nvImage;
    GImage2NvImage(*psrcimage, &nvImage);

    nvCompressionOptions options;

    InMemBuf imbuf = { NULL, 0 };

    switch (destImageFormat)
    {
    case GImage::Image_ARGB_8888:
        options.textureFormat = k8888;
        break;

    case GImage::Image_RGB_888:
        options.textureFormat = k888;
        break;

    case GImage::Image_A_8:
        options.textureFormat = kA8;
        break;

    default: options.textureFormat = k8888;
    }

    options.user_data       = &imbuf;
    options.DoNotGenerateMIPMaps();

    nvRescaleTypes   nvRescale       = NvRescaleConst(rescale);
    nvMipFilterTypes nvRescaleFilter = NvFilterConst(rescaleFilter);

    if (nvRescale == kRescalePreScale)
    {
        // means we want scale each dimension to be multiple of 4
        options.PreScaleImage(float(int((targetWidth + 3)&(~3))), float(int((targetHeight + 3)&(~3))), nvRescaleFilter);
    }
    else
    {
        options.rescaleImageType = nvRescale;
        options.rescaleImageFilter = nvRescaleFilter;
    }

    NV_ERROR_CODE hres = nvDDS::nvDXTcompress(
        nvImage,
        &options,
        nvDXTWriteInMemCallback);
    LastError = hres;

    GImage* pimage = 0;
    if (hres == NV_OK)
    {
        pimage = GImage::ReadDDSFromMemory(imbuf.pData, imbuf.Size);
    }
    GFREE(imbuf.pData);
    return pimage;
}

bool GDXTHelper::CompressToFile(const GImage* psrcimage, 
                                GFile* pdstFile, 
                                GDXTHelper::QualitySetting quality, 
                                GDXTHelper::RescaleTypes rescale, 
                                GDXTHelper::FilterTypes rescaleFilter, 
                                GDXTHelper::FilterTypes mipFilter, 
                                unsigned targetWidth,
                                unsigned targetHeight,
                                int dxtN,
                                bool isUncompressedDDS,
                                bool doGenMipMap,
                                bool isDxt1Allowed)

{
    RGBAImage nvImage;
    GImage2NvImage(*psrcimage, &nvImage);

    nvRescaleTypes      nvRescale       = NvRescaleConst(rescale);
    nvMipFilterTypes    nvRescaleFilter = NvFilterConst(rescaleFilter);
    nvQualitySetting    nvQuality       = NvQualityConst(quality);
    nvMipFilterTypes    nvMipFilter     = NvFilterConst(mipFilter);

    nvCompressionOptions options;
    options.quality     = nvQuality;
    options.user_data   = pdstFile;
    if (nvRescale == kRescalePreScale)
    {
        // means we want scale each dimension to be multiple of 4
        options.PreScaleImage(float(int((targetWidth + 3)&(~3))), float(int((targetHeight + 3)&(~3))), nvRescaleFilter);
    }
    else
    {
        if (nvRescale == kRescaleNone && (!isUncompressedDDS || doGenMipMap))
            options.rescaleImageType = kRescaleBiggestPower2;
        else
            options.rescaleImageType    = nvRescale;
        options.rescaleImageFilter      = nvRescaleFilter;
    }
    if (doGenMipMap)
    {
        options.GenerateMIPMaps(0);
        options.mipFilterType = nvMipFilter;
    }
    else
        options.DoNotGenerateMIPMaps();

    switch (psrcimage->Format)
    {
        case GImage::Image_ARGB_8888:
            if (isUncompressedDDS)
                options.textureFormat = k8888;
            else
            {
                if (dxtN == 3)
                    options.textureFormat = kDXT3;
                else
                    options.textureFormat = kDXT5;
            }
            break;

        case GImage::Image_RGB_888:
            if (isUncompressedDDS)
            {
                if (!doGenMipMap)
                    options.textureFormat = k888;
                else
                    options.textureFormat = k8888; // mipmaps should be ALWAYS generated as 32-bit and with dimension power of 2
                                                   // to avoid any resampling in the renderer! (AB)
            }
            else
            {
                if (isDxt1Allowed)
                    options.textureFormat = kDXT1;
                else if (dxtN == 3)
                    options.textureFormat = kDXT3;
                else
                    options.textureFormat = kDXT5;
            }
            break;
        case GImage::Image_A_8:
            options.textureFormat = kA8;
            break;
    }

    NV_ERROR_CODE hres = nvDDS::nvDXTcompress(
        nvImage,
        &options,
        nvDXTWriteCallback);

    LastError = hres;
    if (hres != NV_OK)
    {
        return false;
    }
    return true;
}

const char*  GDXTHelper::GetLastErrorString()
{
    return getErrorString((NV_ERROR_CODE)GDXTHelper::LastError);
}