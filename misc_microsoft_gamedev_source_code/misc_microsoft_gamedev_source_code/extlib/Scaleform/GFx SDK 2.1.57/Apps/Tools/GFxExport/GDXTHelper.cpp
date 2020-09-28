/**********************************************************************

Filename    :   GDXTHelper.cpp
Content     :   Interface to NVidia DXT library 
Created     :   September, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#define NOMINMAX

#include "GFile.h"
#include "GImage.h"
#include "GFxString.h"
#include "GFxStream.h"
#include "GDXTHelper.h"
#include "GContainers.h"
#include "GResizeImage.h"
// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <locale.h>
#include <zlib.h>

#include <nvimage/nvtt/nvtt.h>

/*
union nvColor32 {
    struct {
    UInt8 b, g, r, a;
    };
    UInt32 u;
};
*/
struct nvColor32 {
    UInt8 b, g, r, a;
};

int GDXTHelper::LastError =nvtt::Error_Unknown;


void GDXTHelper::CalculateResizeFilter(RescaleFilterTypes FilterType)
{
    bool norm = true;
    if (pRescaleFilterLut)
    {
        GASSERT(0);
    }
    pRescaleFilterLut= new GImageFilterLut;
    switch(FilterType)
    {
    case FilterBicubic : pRescaleFilterLut->Calculate(GImageFilterBicubic (), norm); break;
    case FilterSpline16: pRescaleFilterLut->Calculate(GImageFilterSpline16(), norm); break;
    case FilterSpline36: pRescaleFilterLut->Calculate(GImageFilterSpline36(), norm); break;
    case FilterHanning : pRescaleFilterLut->Calculate(GImageFilterHanning (), norm); break;
    case FilterHamming : pRescaleFilterLut->Calculate(GImageFilterHamming (), norm); break;
    case FilterHermite : pRescaleFilterLut->Calculate(GImageFilterHermite (), norm); break;
    case FilterKaiser  : pRescaleFilterLut->Calculate(GImageFilterKaiser  (), norm); break;
    case FilterQuadric : pRescaleFilterLut->Calculate(GImageFilterQuadric (), norm); break;
    case FilterCatrom  : pRescaleFilterLut->Calculate(GImageFilterCatrom  (), norm); break;
    case FilterGaussian: pRescaleFilterLut->Calculate(GImageFilterGaussian(), norm); break;
    case FilterBessel  : pRescaleFilterLut->Calculate(GImageFilterBessel  (), norm); break;
    case FilterMitchell: pRescaleFilterLut->Calculate(GImageFilterMitchell(), norm); break;
    case FilterSinc    : pRescaleFilterLut->Calculate(GImageFilterSinc    (), norm); break;
    case FilterLanczos : pRescaleFilterLut->Calculate(GImageFilterLanczos (), norm); break;
    case FilterBlackman: pRescaleFilterLut->Calculate(GImageFilterBlackman(), norm); break;
    case FilterBox:
    case FilterBilinear:
        break;
    default:
        GASSERT(0);

    }
}

void GDXTHelper::CalculateWH( UInt32 srcW, UInt32 srcH, UInt32 &destW, UInt32 &destH, GDXTHelper::RescaleTypes rescale )
{
    //calculation of the next higher or lower power or 2 can be implemented without using log, if necessary
    double wp=log(double(srcW))/log((double)2.0), hp=log(double(srcH))/log((double)2.0);
    switch (rescale)
    {
    case GDXTHelper::RescaleNearestPower2 :
        destW=1<<(int)floor(wp+0.5); destH=1<<(int)floor(hp+0.5);
        break;

    case GDXTHelper::RescaleBiggestPower2 :
        destW=1<<(int)ceil(wp); destH=1<<(int)ceil(hp);
        break;

    case GDXTHelper::RescaleSmallestPower2 :
        destW=1<<(int)floor(wp); destH=1<<(int)floor(hp);
        break;
    case GDXTHelper::RescaleNextSmallestPower2 :
        if (wp > 1)
            destW = 1<<(int)floor(wp-1);
        else
            destW = 1;
        if (hp > 1)
            destH = 1<<(int)floor(hp-1);
        else
            destH = 1;
        break;

    case GDXTHelper::RescalePreScale :
        destW=int((srcW + 3)&(~3)); destH=int((srcH + 3)&(~3));
        break;

    case GDXTHelper::RescaleNone :
        destW=srcW; destH=srcH;
        break;
    default:
        GASSERT(0);
    }
}

GImage* GDXTHelper:: CreateResizedImage(const GImage *psrc, GDXTHelper::RescaleTypes rescale, 
               GDXTHelper::RescaleFilterTypes filterType /*,  unsigned targetWidth, unsigned targetHeight*/) 
{
    if (rescale == GDXTHelper::RescaleNone)
        return (new GImage(*psrc));

    UInt32 w=0, h=0; //destination width and height  
    
    CalculateWH( psrc->Width, psrc->Height, w, h, rescale );

    GImage* pdstImage = GImage::CreateImage(GImage::Image_ARGB_8888, w, h);

    switch(filterType)
    {
    case GDXTHelper::FilterBox:      
        GResizeImageBox(pdstImage->pData, w, h, pdstImage->Pitch, 
                        psrc->pData, psrc->Width,psrc->Height,psrc->Pitch, GResizeRgbaToRgba); 
        break;
    case GDXTHelper::FilterBilinear: 
        GResizeImageBilinear(pdstImage->pData, w, h, pdstImage->Pitch, 
                             psrc->pData, psrc->Width,psrc->Height,psrc->Pitch, GResizeRgbaToRgba); 
        break;
    default:       
        GResizeImage(pdstImage->pData, w, h, pdstImage->Pitch, 
                     psrc->pData, psrc->Width,psrc->Height,psrc->Pitch, GResizeRgbaToRgba, 
                     *pRescaleFilterLut); 
        break;
    }
 return pdstImage;
}

UInt32 GDXTHelper::CalculateDDSSize( UInt32 srcW, UInt32 srcH, GImage::ImageFormat format,                              int dxtN,
                                  bool isUncompressedDDS,
                                  bool doGenMipMap,
                                  bool isDxt1Allowed )
{
    nvtt::InputOptions       inputOptions;
    nvtt::CompressionOptions compressionOptions;
    inputOptions.setTextureLayout(nvtt::TextureType_2D, srcW, srcH);
    inputOptions.setWrapMode(nvtt::WrapMode_Clamp);
    inputOptions.setNormalMap(false);
    inputOptions.setConvertToNormalMap(false);
    inputOptions.setNormalizeMipmaps(false);
    if (doGenMipMap)
        inputOptions.setMipmapping(true,nvtt::MipmapFilter_Box);
    else
        inputOptions.setMipmapping(false);

    switch (format)
    {
    case GImage::Image_ARGB_8888:
        if (isUncompressedDDS)
            compressionOptions.setFormat(nvtt::Format_RGBA);
        else
        {
            if (dxtN == 3)
                compressionOptions.setFormat(nvtt::Format_DXT3);
            else
                compressionOptions.setFormat(nvtt::Format_DXT5);
        }
        break;

    case GImage::Image_RGB_888:
        if (isUncompressedDDS)
        {
            if (!doGenMipMap)
                compressionOptions.setFormat(nvtt::Format_RGB); //for current nvtt implementation, same as RGBA
            else
                compressionOptions.setFormat(nvtt::Format_RGBA); // mipmaps should be ALWAYS generated as 32-bit and with dimension power of 2
            // to avoid any resampling in the renderer! (AB)
        }
        else
        {
            if (isDxt1Allowed)
                compressionOptions.setFormat(nvtt::Format_DXT1);
            else if (dxtN == 3)
                compressionOptions.setFormat(nvtt::Format_DXT3);
            else
                compressionOptions.setFormat(nvtt::Format_DXT5);
        }
        break;
    case GImage::Image_A_8:
        compressionOptions.setFormat(nvtt::Format_RGBA); //no A_8 format in current implementation of nvtt
        break;

    default:
        break;
    }
    //int tmp=nvtt::estimateSize(inputOptions, compressionOptions);
    return nvtt::estimateSize(inputOptions, compressionOptions);
}

//OutputHandler is used by nvtt::compress to write output DDS file
struct GOutputHandler : public nvtt::OutputHandler
{
    GOutputHandler() : Total(0), Progress(0), Percentage(0), pOutFile(NULL) {}
    GOutputHandler(GFile *pFile ) : Total(0), Progress(0), Percentage(0) 
    {
        pOutFile=pFile;
    }
    virtual ~GOutputHandler() { }

    virtual void setTotal(int t)
    {
        Total = t;
    }

    virtual void mipmap(int size, int width, int height, int depth, int face, int miplevel)
    {
        // ignore.
    }

    // Output data.
    virtual void writeData(const void * data, int size)
    {
        pOutFile->Write((const UByte*)data, (SInt)size); 
    }

    int     Total;
    int     Progress;
    int     Percentage;
    GFile * pOutFile;
};

//OutputHandler is used by nvtt::compress
struct GErrorHandler : public nvtt::ErrorHandler
{
    virtual void error(nvtt::Error e)
    {
        GDXTHelper::LastError=e;    
        GFC_DEBUG_BREAK;//
    }
};

static void GImage2NvColor32Buff(const GImage& srcImg, nvColor32* nvImage)
{
    GASSERT(nvImage);
    for (UInt y = 0; y < srcImg.Height; y++)
    {
        const UByte* pscanline = srcImg.GetScanline(y);
        switch(srcImg.Format)
        {
        case GImage::Image_RGB_888:
            {
                for (UInt x = 0; x < srcImg.Width; x++, pscanline += 3)
                {
                    nvColor32 & pix = nvImage[y*srcImg.Width+x];
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
                    nvColor32 & pix = nvImage[y*srcImg.Width+x];
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
                    nvColor32 & pix = nvImage[y*srcImg.Width+x];

                    pix.r = pscanline[0];
                    pix.g = pscanline[0];
                    pix.b = pscanline[0];
                    pix.a = pscanline[0];

                }
            }
            break;

                default:
                    GASSERT(0);
        }
    }
}

static nvtt::MipmapFilter NvFilterConst(GDXTHelper::MipFilterTypes filter)
{
    switch(filter)
    {
    case GDXTHelper::nvFilterBox:         return nvtt::MipmapFilter_Box;    
    case GDXTHelper::nvFilterTriangle:    return nvtt::MipmapFilter_Triangle;
    case GDXTHelper::nvFilterKaiser:      return nvtt::MipmapFilter_Kaiser;
    }
    return nvtt::MipmapFilter_Box;
}


static nvtt::Quality NvQualityConst(GDXTHelper::QualitySetting qual)
{
    switch(qual)
    {
    case GDXTHelper::QualityFastest:    return nvtt::Quality_Fastest;
    case GDXTHelper::QualityNormal:     return nvtt::Quality_Normal;
    case GDXTHelper::QualityProduction: return nvtt::Quality_Production;
    case GDXTHelper::QualityHighest:    return nvtt::Quality_Highest;
    }
    GASSERT(0);
    return nvtt::Quality_Fastest;
}

GImage* GDXTHelper::Rescale(const GImage* psrcimage, 
                            GDXTHelper::RescaleTypes rescale, 
                            GDXTHelper::RescaleFilterTypes rescaleFilter, 
                            GImage::ImageFormat destImageFormat)
{
    GPtr<GImage> pimage = *new GImage(*psrcimage);
    GPtr<GImage> pargbImage = *pimage->ConvertImage(GImage::Image_ARGB_8888);
    GImage * scaledImage=CreateResizedImage(pargbImage,rescale, rescaleFilter /*, targetWidth, targetHeight*/);
    return scaledImage;
}

bool GDXTHelper::CompressToFile(const GImage* psrcimage, 
                                GFile* pdstFile, 
                                GDXTHelper::QualitySetting quality, 
                                GDXTHelper::RescaleTypes rescale, 
                                GDXTHelper::RescaleFilterTypes rescaleFilter, 
                                GDXTHelper::MipFilterTypes mipFilter, 
                                int dxtN,
                                bool isUncompressedDDS,
                                bool doGenMipMap,
                                bool isDxt1Allowed)

{
    // If we compress dds or generate mipmaps image must be rescaled
    if (rescale==RescaleNone && (!isUncompressedDDS || doGenMipMap ))
        rescale=RescaleBiggestPower2;

    GPtr<GImage> pimage     = *new GImage(*psrcimage);
    GPtr<GImage> pargbImage = *pimage->ConvertImage(GImage::Image_ARGB_8888);
    GPtr<GImage> scaledImage= *CreateResizedImage(pargbImage,rescale, rescaleFilter);

    // Do not compress DDS for images smaller then 4x4
    if ((scaledImage->Width < 4) || (scaledImage->Height < 4))
        isUncompressedDDS = true;

    nvtt::InputOptions       inputOptions;
    nvtt::CompressionOptions compressionOptions;
    
    nvtt::MipmapFilter  nvMipFilter = NvFilterConst(mipFilter);
    nvtt::Quality       nvQuality   = NvQualityConst(quality);


    nvColor32 *pimgBuff=new nvColor32[scaledImage->Width*scaledImage->Height];
    GImage2NvColor32Buff(*scaledImage,pimgBuff);

    inputOptions.setTextureLayout(nvtt::TextureType_2D, scaledImage->Width, scaledImage->Height);
    inputOptions.setMipmapData(pimgBuff, scaledImage->Width, scaledImage->Height);
    delete [] pimgBuff;

    inputOptions.setWrapMode(nvtt::WrapMode_Clamp);

    inputOptions.setNormalMap(false);
    inputOptions.setConvertToNormalMap(false);
    inputOptions.setGamma(2.2f, 2.2f); //values from nvidia code
    inputOptions.setNormalizeMipmaps(false);

    compressionOptions.setQuality(nvQuality);
    compressionOptions.enableHardwareCompression(false); //no cuda
    compressionOptions.setColorWeights(1, 1, 1);

    if (doGenMipMap)
        inputOptions.setMipmapping(true,nvMipFilter);
    else
        inputOptions.setMipmapping(false);

    switch (psrcimage->Format)
    {
    case GImage::Image_ARGB_8888:
        if (isUncompressedDDS)
            compressionOptions.setFormat(nvtt::Format_RGBA);
        else
        {
            if (dxtN == 3)
                compressionOptions.setFormat(nvtt::Format_DXT3);
            else
                compressionOptions.setFormat(nvtt::Format_DXT5);
        }
        break;

    case GImage::Image_RGB_888:
        if (isUncompressedDDS)
        {
            if (!doGenMipMap)
                compressionOptions.setFormat(nvtt::Format_RGB); //for current nvtt implementation, same as RGBA
            else
                compressionOptions.setFormat(nvtt::Format_RGBA); // mipmaps should be ALWAYS generated as 32-bit and with dimension power of 2
            // to avoid any resampling in the renderer! (AB)
        }
        else
        {
            if (isDxt1Allowed)
                compressionOptions.setFormat(nvtt::Format_DXT1);
            else if (dxtN == 3)
                compressionOptions.setFormat(nvtt::Format_DXT3);
            else
                compressionOptions.setFormat(nvtt::Format_DXT5);
        }
        break;
    case GImage::Image_A_8:
        compressionOptions.setFormat(nvtt::Format_A8); 
        break;

        default:
            break;
    }

    GOutputHandler outputHandler(pdstFile);
    GErrorHandler  errorHandler;
    outputHandler.setTotal(nvtt::estimateSize(inputOptions, compressionOptions));
    nvtt::OutputOptions outputOptions(&outputHandler, &errorHandler);
    nvtt::compress(inputOptions, outputOptions, compressionOptions);
    return true;    
}

const char*  GDXTHelper::GetLastErrorString()
{
    return nvtt::errorString( (nvtt::Error)GDXTHelper::LastError);
}

