/**********************************************************************

Filename    :   GFxTagLoaders.cpp
Content     :   GFxPlayer tag loaders implementation
Created     :   June 30, 2005
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxTagLoaders.h"

#include "GFile.h"
#include "GZlibFile.h"
#include "GImage.h"
#include "GFxImageResource.h"

#include "GJPEGUtil.h"

#include "GRenderer.h"

#include "GFxCharacter.h"

#include "GFxLoaderImpl.h"
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"

#include "GFxAction.h"
#include "GFxButton.h"

#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxMorphCharacter.h"

#include "GFxShape.h"
#include "GFxStream.h"
#include "GFxStyles.h"
#include "GFxDlist.h"
#include "GFxTimers.h"
#include "GConfig.h"

#include "GFxLoadProcess.h"

#include <string.h> // for memset
#include <float.h>

#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

#ifdef GFC_USE_ZLIB
#include <zlib.h>
#endif


// ***** Tag Loader Tables

GFx_TagLoaderFunction GFx_SWF_TagLoaderTable[GFxTag_SWF_TagTableEnd] =
{
    // [0]
    GFx_EndLoader,
    0,
    GFx_DefineShapeLoader,
    0,
    GFx_PlaceObject2Loader,
    GFx_RemoveObject2Loader,
    GFx_DefineBitsJpegLoader,
    GFx_ButtonCharacterLoader,
    GFx_JpegTablesLoader,
    GFx_SetBackgroundColorLoader,

    // [10]
    GFx_DefineFontLoader,
    GFx_DefineTextLoader,
    GFx_DoActionLoader,
    GFx_DefineFontInfoLoader,
#ifndef GFC_NO_SOUND
    GFx_DefineSoundLoader,
    GFx_StartSoundLoader,
    0,
    GFx_ButtonSoundLoader,
#else
    0,
    0,
    0,
    0,
#endif
    0,
    0,

    // [20]
    GFx_DefineBitsLossless2Loader,
    GFx_DefineBitsJpeg2Loader,
    GFx_DefineShapeLoader,
    0,
    GFx_NullLoader,   // "protect" tag; we're not an authoring tool so we don't care.
    0,
    GFx_PlaceObject2Loader,
    0,
    GFx_RemoveObject2Loader,
    0,

    // [30]
    0,
    0,
    GFx_DefineShapeLoader,
    GFx_DefineTextLoader,
    GFx_ButtonCharacterLoader,
    GFx_DefineBitsJpeg3Loader,
    GFx_DefineBitsLossless2Loader,
    GFx_DefineEditTextLoader,
    0,    
    GFx_SpriteLoader,

    // [40]
    0,
    0,
    0,
    GFx_FrameLabelLoader,
    0,
    0,
    GFx_DefineShapeMorphLoader,
    0,
    GFx_DefineFontLoader,
    0,

    // [50]
    0,
    0,
    0,
    0,
    0,
    0,
    GFx_ExportLoader,
    GFx_ImportLoader,
    0,
    GFx_DoInitActionLoader,

    // [60]
    0,
    0,
    GFx_DefineFontInfoLoader,
    0,
    0,
    0,
    0,
    0,
    0,
    GFx_FileAttributesLoader,

    // [70]
    GFx_PlaceObject2Loader,
    GFx_ImportLoader,
    0,
    0,
    GFx_CSMTextSettings,
    GFx_DefineFontLoader,
    0,
    GFx_MetadataLoader,
    0,
    0,

    // [80]
    0,
    0,
    0,
    GFx_DefineShapeLoader,
    GFx_DefineShapeMorphLoader    
};

GFx_TagLoaderFunction GFx_GFX_TagLoaderTable[GFxTag_GFX_TagTableEnd - GFxTag_GFX_TagTableBegin] =
{
    // [1000]
    GFx_ExporterInfoLoader,
    GFx_DefineExternalImageLoader,
    GFx_FontTextureInfoLoader,
    GFx_DefineExternalGradientImageLoader,
    GFx_DefineGradientMapLoader
};

//
// Tag implementations
//

// Silently ignore the contents of this tag.
void    GSTDCALL GFx_NullLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED2(p, tagInfo);
}

// Label the current frame of m with the name from the GFxStream.
void    GSTDCALL GFx_FrameLabelLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    char*   ps = p->GetStream()->ReadString();
    p->AddFrameName(ps, p->GetLog());
    GFREE(ps);
}


void    GSTDCALL GFx_SetBackgroundColorLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_SetBackgroundColor);
    GASSERT(p);

    GFxSetBackgroundColor* pt = p->AllocTag<GFxSetBackgroundColor>();
    pt->Read(p);
    p->AddExecuteTag(pt);
}


// Load JPEG compression tables that can be used to load
// images further along in the GFxStream.
void    GSTDCALL GFx_JpegTablesLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_JpegTables);

#ifdef GFC_USE_LIBJPEG
    // When Flash CS3 saves SWF8-, it puts this tag with zero length (no data in it).
    // In this case we should do nothing here, and create the JPEG image differently
    // in GFx_DefineBitsJpegLoader (AB)
    if (tagInfo.TagLength > 0)
    {
        GJPEGInput* pjin = GJPEGInput::CreateSwfJpeg2HeaderOnly(p->GetStream()->GetUnderlyingFile());
        GASSERT(pjin);
        p->SetJpegLoader(pjin);
    }
#else
    GUNUSED(p);
#endif
}


// A JPEG image without included tables; those should be in an
// existing GJPEGInput object stored in the pMovie.
void    GSTDCALL GFx_DefineBitsJpegLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsJpeg);

    UInt16  bitmapResourceId = p->GetStream()->ReadU16();

    //
    // Read the image data.
    //
    GPtr<GImage>         pimage;

    if (p->IsLoadingImageData())
    {
#ifdef GFC_USE_LIBJPEG
        p->GetStream()->SyncFileStream();
        GJPEGInput* pjin = p->GetJpegLoader();
        if (!pjin)
        {
            // if tag 8 was not loaded or has zero length - just read
            // jpeg as usually.
            pimage = *GImage::ReadJpeg(p->GetStream()->GetUnderlyingFile());
        }
        else
        {
            pjin->DiscardPartialBuffer();

            pimage = *GImage::ReadSwfJpeg2WithTables(pjin);        
        }
        // MA: We don't have renderer during loading? -> what do we do
#else
        p->GetStream()->LogError("Error: jpeglib is not linked - can't load jpeg image data\n");
#endif
    }
    else
    {
        // Empty image.
    }

    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);
}


void    GSTDCALL GFx_DefineBitsJpeg2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsJpeg2);
    
    UInt16  bitmapResourceId = p->ReadU16();

    p->LogParse("  GFx_DefineBitsJpeg2Loader: charid = %d pos = 0x%x\n",
                bitmapResourceId, p->Tell());

    //
    // Read the image data.
    //
    GPtr<GImage>         pimage;

    if (p->IsLoadingImageData())
    {
#ifdef GFC_USE_LIBJPEG
        pimage = *GImage::ReadSwfJpeg2(p->GetUnderlyingFile());
#else
        p->LogError("Error: jpeglib is not linked - can't load jpeg image data\n");
#endif
    }
    else
    {
        // Empty image.
    }

    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);
}


#ifdef GFC_USE_ZLIB
// Wrapper function -- uses Zlib to uncompress InBytes worth
// of data from the input file into BufferBytes worth of data
// into *buffer.
void    GFx_InflateWrapper(GFxStream* pinStream, void* buffer, int BufferBytes)
{
    GASSERT(buffer);
    GASSERT(BufferBytes > 0);

    int err;
    z_stream D_stream; /* decompression GFxStream */

    D_stream.zalloc = (alloc_func)0;
    D_stream.zfree = (free_func)0;
    D_stream.opaque = (voidpf)0;

    D_stream.next_in  = 0;
    D_stream.avail_in = 0;

    D_stream.next_out = (Byte*) buffer;
    D_stream.avail_out = (uInt) BufferBytes;

    err = inflateInit(&D_stream);
    if (err != Z_OK) {
        pinStream->LogError("Error: GFx_InflateWrapper() InflateInit() returned %d\n", err);
        return;
    }

    UByte   buf[1];

    for (;;) {
        // Fill a one-Byte (!) buffer.
        buf[0] = pinStream->ReadU8();
        D_stream.next_in = &buf[0];
        D_stream.avail_in = 1;

        err = inflate(&D_stream, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END) break;
        if (err != Z_OK)
        {
            pinStream->LogError("Error: GFx_InflateWrapper() Inflate() returned %d\n", err);
            break;
        }
    }

    err = inflateEnd(&D_stream);
    if (err != Z_OK)
    {
        pinStream->LogError("Error: GFx_InflateWrapper() InflateEnd() return %d\n", err);
    }
}
#endif // GFC_USE_ZLIB


// Redundant code to compute GFx_UndoPremultiplyTable.
//#if defined(GFC_USE_ZLIB)
//// Helpers used for un-premultiplication.
//// SFW RGB+A data is stored with RGB channels pre-multiplied by alpha.
//// This lookup table is used to convert those efficiently.
//static SInt*    GFx_GetPremultiplyAlphaUndoTable()
//{
//    static  SInt    PremultiplyTable[256];
//    static  bool    TableValid = 0;
//
//    if (!TableValid)
//    {
//        PremultiplyTable[0] = 255 * 256;
//        for(UInt i=1; i<=255; i++)
//        {
//            PremultiplyTable[i] = (255 * 256) / i;
//        }
//        TableValid = 1;
//
//FILE* fd = fopen("dt", "wt");
//for(UInt i=0; i<=255; i++)
//{
//if(i)
//{
//    if(((i * PremultiplyTable[i]) >> 8) > 255)
//    {
//fprintf(fd, "\n========", PremultiplyTable[i]);
//    }
//}
//fprintf(fd, "%5d,", PremultiplyTable[i]);
//}
//fclose(fd);
//
//    }
//    return PremultiplyTable;
//}
//#endif


static UInt16 GFx_UndoPremultiplyTable[256] = 
{
    65280,65280,32640,21760,16320,13056,10880, 9325, 8160, 7253, 6528, 5934, 5440, 5021, 4662, 4352, 
     4080, 3840, 3626, 3435, 3264, 3108, 2967, 2838, 2720, 2611, 2510, 2417, 2331, 2251, 2176, 2105, 
     2040, 1978, 1920, 1865, 1813, 1764, 1717, 1673, 1632, 1592, 1554, 1518, 1483, 1450, 1419, 1388, 
     1360, 1332, 1305, 1280, 1255, 1231, 1208, 1186, 1165, 1145, 1125, 1106, 1088, 1070, 1052, 1036, 
     1020, 1004,  989,  974,  960,  946,  932,  919,  906,  894,  882,  870,  858,  847,  836,  826,  
      816,  805,  796,  786,  777,  768,  759,  750,  741,  733,  725,  717,  709,  701,  694,  687,  
      680,  672,  666,  659,  652,  646,  640,  633,  627,  621,  615,  610,  604,  598,  593,  588,  
      582,  577,  572,  567,  562,  557,  553,  548,  544,  539,  535,  530,  526,  522,  518,  514,  
      510,  506,  502,  498,  494,  490,  487,  483,  480,  476,  473,  469,  466,  462,  459,  456,  
      453,  450,  447,  444,  441,  438,  435,  432,  429,  426,  423,  421,  418,  415,  413,  410,  
      408,  405,  402,  400,  398,  395,  393,  390,  388,  386,  384,  381,  379,  377,  375,  373,  
      370,  368,  366,  364,  362,  360,  358,  356,  354,  352,  350,  349,  347,  345,  343,  341,  
      340,  338,  336,  334,  333,  331,  329,  328,  326,  324,  323,  321,  320,  318,  316,  315,  
      313,  312,  310,  309,  307,  306,  305,  303,  302,  300,  299,  298,  296,  295,  294,  292,  
      291,  290,  288,  287,  286,  285,  283,  282,  281,  280,  278,  277,  276,  275,  274,  273,  
      272,  270,  269,  268,  267,  266,  265,  264,  263,  262,  261,  260,  259,  258,  257,  256
};


// Helper routine to undo pre-multiply by alpha, stored in SWF files.
GINLINE void    UndoPremultiplyAlpha(UByte *prgb, UByte a)
{
    //SInt undoVal = GFx_PremultiplyTable[a]; //GFx_GetPremultiplyAlphaUndoTable()[a];
    //// This can probably be optimized... (with binary ops and/or SSE/MMX)
    //prgb[0] =  UByte( GTL::gclamp<SInt>((undoVal * (SInt)prgb[0]) >> 8, 0, 255) );
    //prgb[1] =  UByte( GTL::gclamp<SInt>((undoVal * (SInt)prgb[1]) >> 8, 0, 255) );
    //prgb[2] =  UByte( GTL::gclamp<SInt>((undoVal * (SInt)prgb[2]) >> 8, 0, 255) );

    UInt undoVal = GFx_UndoPremultiplyTable[a];
    prgb[0] = UByte((undoVal * ((prgb[0] <= a) ? prgb[0] : a)) >> 8);
    prgb[1] = UByte((undoVal * ((prgb[1] <= a) ? prgb[1] : a)) >> 8);
    prgb[2] = UByte((undoVal * ((prgb[2] <= a) ? prgb[2] : a)) >> 8);
}


// A special filter to restore lost colors after the alpha-premultiplication operation
void UndoAndFilterPremultiplied(GImage* pimage)
{
    if (pimage->Format != GImage::Image_ARGB_8888) return;
    UInt x,y;
    UInt pitch = pimage->Width * 4 + 8;

    GTL::garray<UByte> buf;
    buf.resize((pimage->Height + 2) * pitch);

    if (buf.size() == 0) return;

    memset(&buf[0],                            0, pitch);
    memset(&buf[(pimage->Height + 1) * pitch], 0, pitch);

    for (y = 0; y < pimage->Height; ++y)
    {
        UByte* sl = &buf[(y + 1) * pitch];
        memcpy(sl + 4, pimage->GetScanline(y), pimage->Width * 4);
        sl[0] = sl[1] = sl[2] = sl[3] = 0;
        sl += pitch - 4;
        sl[0] = sl[1] = sl[2] = sl[3] = 0;
    }

    for (y = 0; y < pimage->Height; ++y)
    {
              UByte* imgSl = pimage->GetScanline(y);
        const UByte* bufSl = &buf[(y + 1) * pitch];
        for (x = 0; x < pimage->Width; ++x)
        {
            const UByte* p5 = bufSl + x * 4 + 4;
                  UByte* pi = imgSl + x * 4;

            if (pi[3] < 16)
            {
                const UByte* p1 = p5 - pitch - 4;
                const UByte* p2 = p5 - pitch;
                const UByte* p3 = p5 - pitch + 4;
                const UByte* p4 = p5 - 4;
                const UByte* p6 = p5 + 4;
                const UByte* p7 = p5 + pitch - 4;
                const UByte* p8 = p5 + pitch;
                const UByte* p9 = p5 + pitch + 4;

                // Calculate the weighted mean of 9 pixels. Sum(x[i] * w[i]) / Sum(w[i]);
                // Here x[i] is the color component, w[i] is alpha. So that, we calculate 
                // the denomonator once and the numerators for each channel. Note that we 
                // do not need to multiply by alpha because the components are already premultiplied.
                // We only need to scale the color values to the appropriate range (multiply by 256).
                UInt den = UInt(p1[3]) + p2[3] + p3[3] + p4[3] + p5[3] + p6[3] + p7[3] + p8[3] + p9[3];
                if (den)
                {
                    UInt r = ((p1[0] + p2[0] + p3[0] + p4[0] + p5[0] + p6[0] + p7[0] + p8[0] + p9[0]) << 8) / den;
                    UInt g = ((p1[1] + p2[1] + p3[1] + p4[1] + p5[1] + p6[1] + p7[1] + p8[1] + p9[1]) << 8) / den;
                    UInt b = ((p1[2] + p2[2] + p3[2] + p4[2] + p5[2] + p6[2] + p7[2] + p8[2] + p9[2]) << 8) / den;
                    pi[0] = UByte((r <= 255) ? r : 255);
                    pi[1] = UByte((g <= 255) ? g : 255);
                    pi[2] = UByte((b <= 255) ? b : 255);
                }

                // An alternative method. Just fing the neighbor pixel with the maximal alpha
                //UInt  a = p5[3];
                //if(p1[3] > a) { pi[0] = p1[0]; pi[1] = p1[1]; pi[2] = p1[2]; a = p1[3]; }
                //if(p2[3] > a) { pi[0] = p2[0]; pi[1] = p2[1]; pi[2] = p2[2]; a = p2[3]; }
                //if(p3[3] > a) { pi[0] = p3[0]; pi[1] = p3[1]; pi[2] = p3[2]; a = p3[3]; }
                //if(p4[3] > a) { pi[0] = p4[0]; pi[1] = p4[1]; pi[2] = p4[2]; a = p4[3]; }
                //if(p6[3] > a) { pi[0] = p6[0]; pi[1] = p6[1]; pi[2] = p6[2]; a = p6[3]; }
                //if(p7[3] > a) { pi[0] = p7[0]; pi[1] = p7[1]; pi[2] = p7[2]; a = p7[3]; }
                //if(p8[3] > a) { pi[0] = p8[0]; pi[1] = p8[1]; pi[2] = p8[2]; a = p8[3]; }
                //if(p9[3] > a) { pi[0] = p9[0]; pi[1] = p9[1]; pi[2] = p9[2]; a = p9[3]; }
            }
            else
            {
                UndoPremultiplyAlpha(pi, pi[3]);
            }
        }
    }
}



// loads a DefineBitsJpeg3 tag. This is a jpeg file with an alpha
// channel using zlib compression.
void    GSTDCALL GFx_DefineBitsJpeg3Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsJpeg3);

    UInt16  bitmapResourceId = p->ReadU16();
    p->LogParse("  GFx_DefineBitsJpeg3Loader: charid = %d pos = 0x%x\n",
                bitmapResourceId, p->Tell());

    UInt32  jpegSize      = p->ReadU32();
    UInt32  alphaPosition = p->Tell() + jpegSize;

    GPtr<GImage>         pimage;
 
    if (p->IsLoadingImageData())
    {
#if !defined(GFC_USE_ZLIB) || !defined(GFC_USE_LIBJPEG)
        p->LogError("Error: jpeglib/zlib is not linked - can't load jpeg/zipped image data!\n");
        GUNUSED(alphaPosition);
#else
        //
        // Read the image data.
        //
                
        pimage = *GImage::ReadSwfJpeg3(p->GetUnderlyingFile());

        // Read alpha channel.
        p->SetPosition(alphaPosition);

        int     bufferBytes = pimage->Width * pimage->Height;
        UByte*  buffer      = (UByte*)GALLOC(bufferBytes);

        GFx_InflateWrapper(p->GetStream(), buffer, bufferBytes);

        for (int i = 0; i < bufferBytes; i++)
        {
            pimage->pData[4*i+3] = buffer[i];
        }
        // RGB comes in pre-multiplied by alpha. Undo and filter pre-multiplication.
        UndoAndFilterPremultiplied(pimage);
        GFREE(buffer);
#endif
    }
    else
    {
        // Empty image data.
    }


    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);
}


void    GSTDCALL GFx_DefineBitsLossless2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_DefineBitsLossless || tagInfo.TagType == GFxTag_DefineBitsLossless2);

    UInt16  bitmapResourceId = p->ReadU16();
    UByte   BitmapFormat     = p->ReadU8();    // 3 == 8 bit, 4 == 16 bit, 5 == 32 bit
    UInt16  width            = p->ReadU16();
    UInt16  height           = p->ReadU16();

    p->LogParse("  DefBitsLossless2: tagInfo.TagType = %d, id = %d, fmt = %d, w = %d, h = %d\n",
            tagInfo.TagType,
            bitmapResourceId,
            BitmapFormat,
            width,
            height);


    GPtr<GImage> pimage;

    if (p->IsLoadingImageData())
    {
#ifndef GFC_USE_ZLIB
        p->LogError("Error: zlib is not linked - can't load zipped image data\n");
#else
        if (tagInfo.TagType == GFxTag_DefineBitsLossless)
        {
            // RGB image data.
            pimage = *GImage::CreateImage(GImage::Image_RGB_888, width, height);

            if (BitmapFormat == 3)
            {
                // 8-bit data, preceded by a palette.

                const int   BytesPerPixel = 1;
                int ColorTableSize = p->ReadU8();
                ColorTableSize += 1;    // !! SWF stores one less than the actual size

                int pitch = (width * BytesPerPixel + 3) & ~3;

                int BufferBytes = ColorTableSize * 3 + pitch * height;
                UByte*  buffer = (UByte*)GALLOC(BufferBytes);

                GFx_InflateWrapper(p->GetStream(), buffer, BufferBytes);
                GASSERT(p->Tell() <= p->GetTagEndPosition());

                UByte*  ColorTable = buffer;

                for (int j = 0; j < height; j++)
                {
                    UByte*  ImageInRow = buffer + ColorTableSize * 3 + j * pitch;
                    UByte*  ImageOutRow = pimage->GetScanline(j);
                    for (int i = 0; i < width; i++)
                    {
                        UByte   pixel = ImageInRow[i * BytesPerPixel];
                        ImageOutRow[i * 3 + 0] = ColorTable[pixel * 3 + 0];
                        ImageOutRow[i * 3 + 1] = ColorTable[pixel * 3 + 1];
                        ImageOutRow[i * 3 + 2] = ColorTable[pixel * 3 + 2];
                    }
                }

                GFREE(buffer);
            }
            else if (BitmapFormat == 4)
            {
                // 16 bits / pixel
                const int   BytesPerPixel = 2;
                int pitch = (width * BytesPerPixel + 3) & ~3;

                int BufferBytes = pitch * height;
                UByte*  buffer = (UByte*)GALLOC(BufferBytes);

                GFx_InflateWrapper(p->GetStream(), buffer, BufferBytes);
                GASSERT(p->Tell() <= p->GetTagEndPosition());
        
                for (int j = 0; j < height; j++)
                {
                    UByte*  ImageInRow = buffer + j * pitch;
                    UByte*  ImageOutRow = pimage->GetScanline(j);
                    for (int i = 0; i < width; i++)
                    {
                        UInt16  pixel = ImageInRow[i * 2] | (ImageInRow[i * 2 + 1] << 8);
                
                        // Format must be RGB 555.
                        ImageOutRow[i * 3 + 0] = UByte( (pixel >> 7) & 0xF8 );  // red
                        ImageOutRow[i * 3 + 1] = UByte( (pixel >> 2) & 0xF8 );  // green
                        ImageOutRow[i * 3 + 2] = UByte( (pixel << 3) & 0xF8 );  // blue
                    }
                }
        
                GFREE(buffer);
            }
            else if (BitmapFormat == 5)
            {
                // 32 bits / pixel, input is ARGB Format (???)
                const int   BytesPerPixel = 4;
                int pitch = width * BytesPerPixel;

                int BufferBytes = pitch * height;
                UByte*  buffer = (UByte*)GALLOC(BufferBytes);

                GFx_InflateWrapper(p->GetStream(), buffer, BufferBytes);
                GASSERT(p->Tell() <= p->GetTagEndPosition());
        
                // Need to re-arrange ARGB into RGB.
                for (int j = 0; j < height; j++)
                {
                    UByte*  ImageInRow = buffer + j * pitch;
                    UByte*  ImageOutRow = pimage->GetScanline(j);
                    for (int i = 0; i < width; i++)
                    {
                        UByte   a = ImageInRow[i * 4 + 0];
                        UByte   r = ImageInRow[i * 4 + 1];
                        UByte   g = ImageInRow[i * 4 + 2];
                        UByte   b = ImageInRow[i * 4 + 3];
                        ImageOutRow[i * 3 + 0] = r;
                        ImageOutRow[i * 3 + 1] = g;
                        ImageOutRow[i * 3 + 2] = b;
                        a = a;  // Inhibit warning.
                    }
                }

                GFREE(buffer);
                }

                //icreateInfo.SetImage(pimage);
            }
            else
            {
                // RGBA image data.
                GASSERT(tagInfo.TagType == GFxTag_DefineBitsLossless2);

                pimage = *GImage::CreateImage(GImage::Image_ARGB_8888, width, height);

                if (BitmapFormat == 3)
                {
                    // 8-bit data, preceded by a palette.

                    const int   BytesPerPixel = 1;
                    int ColorTableSize = p->ReadU8();
                    ColorTableSize += 1;    // !! SWF stores one less than the actual size

                    int pitch = (width * BytesPerPixel + 3) & ~3;

                    int BufferBytes = ColorTableSize * 4 + pitch * height;
                    UByte*  buffer = (UByte*)GALLOC(BufferBytes);

                    GFx_InflateWrapper(p->GetStream(), buffer, BufferBytes);
                    GASSERT(p->Tell() <= p->GetTagEndPosition());

                    UByte*  ColorTable = buffer;

                    for (int j = 0; j < height; j++)
                    {
                        UByte*  ImageInRow = buffer + ColorTableSize * 4 + j * pitch;
                        UByte*  ImageOutRow = pimage->GetScanline(j);
                        for (int i = 0; i < width; i++)
                        {
                            UByte   pixel = ImageInRow[i * BytesPerPixel];
                            ImageOutRow[i * 4 + 0] = ColorTable[pixel * 4 + 0];
                            ImageOutRow[i * 4 + 1] = ColorTable[pixel * 4 + 1];
                            ImageOutRow[i * 4 + 2] = ColorTable[pixel * 4 + 2];
                            ImageOutRow[i * 4 + 3] = ColorTable[pixel * 4 + 3];
                            
                            // Should we do this for color-mapped table?
                        //  UndoPremultiplyAlpha(ImageOutRow + i * 4, ImageOutRow[i * 4 + 3], undoAlphaTable);
                        }
                    }

                    GFREE(buffer);
                }
                else if (BitmapFormat == 4)
                {
                    // should be 555.
                    // Is this combination not supported?

                    // 16 bits / pixel
                    const int   BytesPerPixel = 2;
                    int pitch = (width * BytesPerPixel + 3) & ~3;

                    int BufferBytes = pitch * height;
                    UByte*  buffer = (UByte*)GALLOC(BufferBytes);

                    GFx_InflateWrapper(p->GetStream(), buffer, BufferBytes);
                    GASSERT(p->Tell() <= p->GetTagEndPosition());
            
                    for (int j = 0; j < height; j++)
                    {
                        UByte*  ImageInRow = buffer + j * pitch;
                        UByte*  ImageOutRow = pimage->GetScanline(j);
                        for (int i = 0; i < width; i++)
                        {
                            UInt16  pixel = ImageInRow[i * 2] | (ImageInRow[i * 2 + 1] << 8);
                    
                            // Format is RGB 555.
                            ImageOutRow[i * 4 + 0] = UByte( 255 );                  // alpha
                            ImageOutRow[i * 4 + 1] = UByte( (pixel >> 7) & 0xF8 );  // red
                            ImageOutRow[i * 4 + 2] = UByte( (pixel >> 2) & 0xF8 );  // green
                            ImageOutRow[i * 4 + 3] = UByte( (pixel << 3) & 0xF8 );  // blue
                        }
                    }
            
                    GFREE(buffer);
                }
                else if (BitmapFormat == 5)
                {
                    // 32 bits / pixel, input is ARGB format

                    GFx_InflateWrapper(p->GetStream(), pimage->pData, width * height * 4);
                    GASSERT(p->Tell() <= p->GetTagEndPosition());
            
                    // Need to re-arrange ARGB into RGBA.
                    for (int j = 0; j < height; j++)
                    {
                        UByte*  ImageRow = pimage->GetScanline(j);
                        for (int i = 0; i < width; i++)
                        {
                            UByte   a = ImageRow[i * 4 + 0];
                            UByte   r = ImageRow[i * 4 + 1];
                            UByte   g = ImageRow[i * 4 + 2];
                            UByte   b = ImageRow[i * 4 + 3];
                            ImageRow[i * 4 + 0] = r;
                            ImageRow[i * 4 + 1] = g;
                            ImageRow[i * 4 + 2] = b;
                            ImageRow[i * 4 + 3] = a;
                        }
                    }
                    // Undo and filter pre-multiplied format result.
                    UndoAndFilterPremultiplied(pimage);
                }

                // RGBA
                //icreateInfo.SetImage(pimage);
            }
#endif // GFC_USE_ZLIB
    }
    else
    {
        // Empty image in icreateInfo.
    }
    

    // Create a unique resource for the image and add it.
    p->AddImageResource(GFxResourceId(bitmapResourceId), pimage);    
}


void    GSTDCALL GFx_DefineShapeLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_DefineShape  ||
             tagInfo.TagType == GFxTag_DefineShape2 ||
             tagInfo.TagType == GFxTag_DefineShape3 ||
             tagInfo.TagType == GFxTag_DefineShape4 );

    UInt16  characterId = p->ReadU16();
    p->LogParse("  ShapeLoader: id = %d\n", characterId);

    GPtr<GFxShapeCharacterDef>  ch = *new GFxShapeCharacterDef;
    ch->Read(p, tagInfo.TagType, true);

    p->LogParse("  bound rect:");      
    p->GetStream()->LogParseClass(ch->GetBound());

    p->AddResource(GFxResourceId(characterId), ch.GetPtr());
}

void    GSTDCALL GFx_DefineShapeMorphLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_DefineShapeMorph ||
             tagInfo.TagType == GFxTag_DefineShapeMorph2 );
    UInt16 characterId = p->ReadU16();
    p->LogParse("  ShapeMorphLoader: id = %d\n", characterId);

    GPtr<GFxMorphCharacterDef> morph = *new GFxMorphCharacterDef;
    morph->Read(p, tagInfo.TagType, true);
    p->AddResource(GFxResourceId(characterId), morph);
}


//
// GFxFontResource loaders
//

// Load a DefineFont or DefineFont2 tag.
void    GSTDCALL GFx_DefineFontLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_DefineFont  ||
             tagInfo.TagType == GFxTag_DefineFont2 ||
             tagInfo.TagType == GFxTag_DefineFont3 );

    UInt16  fontId = p->ReadU16();

    p->LogParse("  Font: id = %d\n", fontId);
    // Note that we always create FontData separately from font. In addition to
    // allowing for different font texture bindings, this lets us substitute
    // fonts based on names even when they are not imported.
    GPtr<GFxFontData>   pfd = *new GFxFontData;
//    pfd->SetFontProvider(p->pStates->pBindStates->pFontProvider);
    pfd->Read(p, tagInfo.TagType);
   
    p->AddFontDataResource(GFxResourceId(fontId), pfd);
}


// Load a DefineFontInfo tag.  This adds information to an
// existing GFxFontResource.
void    GSTDCALL GFx_DefineFontInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineFontInfo || tagInfo.TagType == GFxTag_DefineFontInfo2);

    UInt16  fontId = p->ReadU16();

    // We need to get at the font data somehow!!!
   
    GFxFontData* pfd = p->GetDataDef()->GetFontData(GFxResourceId(fontId));
    if (pfd)
    {
        pfd->ReadFontInfo(p->GetStream(), tagInfo.TagType);
    }
    else
    {
        p->LogError("GFx_DefineFontInfoLoader: can't find GFxFontResource w/ id %d\n", fontId);
    }
}
    
void    GSTDCALL GFx_PlaceObject2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_PlaceObject  ||
             tagInfo.TagType == GFxTag_PlaceObject2 ||
             tagInfo.TagType == GFxTag_PlaceObject3 );

    if (tagInfo.TagType == GFxTag_PlaceObject)
        p->LogParse("  PlaceObject\n");
    else if (tagInfo.TagType == GFxTag_PlaceObject2)
        p->LogParse("  PlaceObject2\n");
    else if (tagInfo.TagType == GFxTag_PlaceObject3)
        p->LogParse("  PlaceObject3\n");

    GFxPlaceObject2* ptag = p->AllocTag<GFxPlaceObject2>();
    ptag->Read(p, tagInfo.TagType);
    p->AddExecuteTag(ptag);
}



// Create and initialize a sprite, and add it to the pMovie.
void    GSTDCALL GFx_SpriteLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_Sprite);
            
    GFxResourceId characterId = GFxResourceId(p->ReadU16());
    p->LogParse("  sprite\n  char id = %d\n", characterId.GetIdIndex());

    // @@ combine GFxSpriteDef with GFxMovieDefImpl
    GPtr<GFxSpriteDef> ch = *new GFxSpriteDef(p->GetDataDef());
    ch->Read(p, characterId);
    p->AddCharacter(characterId, ch);
}





//
// EndTag
//

// EndTag doesn't actually need to exist.

void    GSTDCALL GFx_EndLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED2(p, tagInfo.TagType);
    GASSERT(tagInfo.TagType == GFxTag_End);
    GASSERT(p->Tell() == p->GetTagEndPosition());
}


void    GSTDCALL GFx_RemoveObject2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_RemoveObject  ||
             tagInfo.TagType == GFxTag_RemoveObject2 );

    GFxRemoveObject2*   ptag = p->AllocTag<GFxRemoveObject2>();       
    ptag->Read(p, tagInfo.TagType);
    p->LogParse("  RemoveObject2(%d)\n", ptag->Depth);
    p->AddExecuteTag(ptag);
}


void    GSTDCALL GFx_ButtonSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_ButtonSound);

    int                 buttonCharacterId = p->ReadU16();
    GFxResourceHandle   rh;
    GFxResource*        pres = 0;
    
    if (p->GetResourceHandle(&rh, GFxResourceId(buttonCharacterId)))
        pres = rh.GetResourcePtr();

    if (pres)
    {
        if (pres->GetResourceType() == GFxResource::RT_ButtonDef)
        {
            GFxButtonCharacterDef* ch = (GFxButtonCharacterDef*) pres;
            GASSERT(ch != NULL);
            ch->Read(p, tagInfo.TagType);
        }
    }
    else
    {
        p->LogError("Error: ButtonDef %d referenced in ButtonSound tag not found", buttonCharacterId);
    }    
}


void    GSTDCALL GFx_ButtonCharacterLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT( tagInfo.TagType == GFxTag_ButtonCharacter ||
             tagInfo.TagType == GFxTag_ButtonCharacter2 );

    int characterId = p->ReadU16();
    p->LogParse("  button GFxCharacter loader: CharId = %d\n", characterId);

    GPtr<GFxButtonCharacterDef> ch = *new GFxButtonCharacterDef;
    ch->Read(p, tagInfo.TagType);
    p->AddResource(GFxResourceId(characterId), ch);
}


//
// *** Export and Import Tags
//

// Load an export Tag (for exposing internal resources of m)
void    GSTDCALL GFx_ExportLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_Export);

    UInt count = p->ReadU16();
    p->LogParse("  export: count = %d\n", count);

    // Read the exports.
    for (UInt i = 0; i < count; i++)
    {
        UInt16  id          = p->ReadU16();
        char*   psymbolName = p->GetStream()->ReadString();
        
        p->LogParse("  export: id = %d, name = %s\n", id, psymbolName);

        GFxResourceId     rid(id);
        GFxResourceHandle hres;
        if (p->GetResourceHandle(&hres, rid))
        {
            // Add export to the list.
            p->GetDataDef()->ExportResource(GFxString(psymbolName), rid, hres);

            // Should we check export types?
            // This may be impossible with handles.
        }
        else         
        {
            // This is really a debug error, since we expect Flash files to
            // be consistent and include their exported characters.
           // GFC_DEBUG_WARNING1(1, "Export loader failed to export resource '%s'",
           //                       psymbolName );
            p->LogError("Export error: don't know how to export GFxResource '%s'\n",
                        psymbolName);
        }
        GFREE(psymbolName);
    }
}


// Load an import Tag (for pulling in external resources)
void    GSTDCALL GFx_ImportLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GASSERT(tagInfo.TagType == GFxTag_Import || tagInfo.TagType == GFxTag_Import2);

    GFxStream *         pin = p->GetStream();
    GFxMovieDataDef*    pdataDef = p->GetDataDef();

    char*   psourceUrl = pin->ReadString();
    int     count      = pin->ReadU16();

    p->LogParse( ((tagInfo.TagType != GFxTag_Import2) ?
                    "  importAssets: SourceUrl = %s, count = %d\n" :
                    "  importAssets2: SourceUrl = %s, count = %d\n" ),
                    psourceUrl, count);

    if (tagInfo.TagType == GFxTag_Import2)
    {
        UInt val = p->ReadU16();
        GUNUSED(val);
        GFC_DEBUG_WARNING1(val != 1, "Unexpected attribute in ImportAssets2 - 0x%X instead of 1", val);
    }
    
    GFxImportData* pimportData = new GFxImportData(psourceUrl,
                                                   pdataDef->GetLoadingFrame());   

    // Get the imports.
    for (int i = 0; i < count; i++)
    {
        UInt16  id          = pin->ReadU16();
        char*   psymbolName = pin->ReadString();
        p->LogParse("  import: id = %d, name = %s\n", id, psymbolName);

        // Add import symbol to data.
        pimportData->AddSymbol(psymbolName, id);
        // And add ResourceId so that it's handle can be referenced by new load operations.
        // TBD: May not need to pass import data
        pdataDef->AddImportResource(GFxResourceId(id), pimportData);

        GFREE(psymbolName);
    }

    // Pass completed GFxImportData to GFxMovieDataDef.
    pdataDef->AddImport(pimportData);
    GFREE(psourceUrl);


    // Add InitAction import tag, so that init actions are processed correctly.
    GFxInitImportActions*  ptag = p->AllocTag<GFxInitImportActions>();
    ptag->SetImportIndex((UInt)pdataDef->ImportData.size()-1);
    p->AddInitAction(GFxResourceId(), ptag);  
}


//
// *** SWF 8
//

void    GSTDCALL GFx_FileAttributesLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_FileAttributes);

    UInt16 attrVal = p->ReadU16();
    p->GetDataDef()->SetFileAttributes(attrVal);

    // Log values for the user.
#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE
    if (attrVal)
    {
        p->LogParse("  fileAttr:");

        char separator = ' ';
        if (attrVal & GFxMovieDef::FileAttr_UseNetwork)
        {
            p->LogParse("%cUseNetwork", separator);
            separator = ',';
        }
        if (attrVal & GFxMovieDef::FileAttr_HasMetadata)
        {
            p->LogParse("%cHasMetadata", separator);
        }
        p->LogParse("\n");
    }
#endif

}

void    GSTDCALL GFx_MetadataLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_Metadata);

    // Determine the size of meta-data and read it into the buffer.
    int     size = p->GetTagEndPosition() - p->Tell();
    UByte*  pbuff = (UByte*)GALLOC(size + 1);
    if (pbuff)
    {           
        for(int i=0; i<size; i++)
            pbuff[i] = p->ReadU8();
        p->GetDataDef()->SetMetadata(pbuff, (UInt)size);

        pbuff[GTL::gmin<int>(size,255)] = 0;        
        p->LogParse("  metadata: %s\n", pbuff);
        GFREE(pbuff);       
    }
}

void    GSTDCALL GFx_ExporterInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{   
    GASSERT(tagInfo.TagType == GFxTag_ExporterInfo);    

    // This tag should no longer be called because it is assumed to always be processed 
    // by the file-header loader through the GFxExporterInfoImpl::ReadExporterInfoTag
    // function.
    GASSERT(0);    
    GUNUSED2(p, tagInfo.TagType);

    // Utilizes the tag 1000 (unused in normal SWF):
    // See GFxExporterInfoImpl::ReadExporterInfoTag for implementation.

    //GFxExporterInfoImpl sinfo;
    // sinfo.ReadExporterInfoTag(p, tagInfo.TagType);
    // sinfo.SetData(version, (GFxLoader::FileFormatType)bitmapFormat, pswf, pfx, flags);
    // m->SetExporterInfo(sinfo);
}


// A helper method that generates image file info and resource data for the
// image based on specified arguments, returning a handle.
static GFxResourceHandle GFx_CreateImageFileResourceHandle(
                            GFxLoadProcess* p, GFxResourceId rid,
                            const char* pimageFileName, const char* pimageExportName, 
                            UInt16 bitmapFormat,
                            UInt16 targetWidth, UInt16 targetHeight)
{
    GFxResourceHandle rh;
    
    // Create a concatenated file name.
    /* // WE no longer translate the filename here, as it is done through
       // a local translator interface during image creation -
       // see (GFxImageResourceCreator::CreateResource).

    GFxString         relativePath;

    // Determine the file name we should use base on a relative path.
    if (GFxURLBuilder::IsPathAbsolute(pimageFileName))
        relativePath = pimageFileName;
    else
    {
        relativePath = p->GetFileURL();
        if (!GFxURLBuilder::ExtractFilePath(&relativePath))
            relativePath = "";
        relativePath += pimageFileName;
    }
    */

    // Fill in file information.
    GPtr<GFxImageFileInfo> pfi = *new GFxImageFileInfo;
    if (pfi)
    {
        pfi->FileName       = pimageFileName;
        pfi->ExportName     = pimageExportName;
        pfi->pExporterInfo  = p->GetExporterInfo();
        pfi->Format         = (GFxFileConstants::FileFormatType) bitmapFormat;
        pfi->TargetWidth    = targetWidth;
        pfi->TargetHeight   = targetHeight;

        // !AB: need to set Use_FontTextures for images used as
        // font textures to provide correct conversion to A8 format,
        // if necessary.
        if (rid.GetIdType() == GFxResourceId::IdType_FontImage)
            pfi->Use = GFxResource::Use_FontTexture;

        // Add resource id and data.      
        rh = p->AddDataResource(rid,
            GFxImageFileResourceCreator::CreateImageFileResourceData(pfi));
    }
    return rh;
}


void    GSTDCALL GFx_DefineExternalImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalImage);

    // Utilizes the tag 1001 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1001
    // CharacterID      UI16
    // BitmapsFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // TargetWidth      UI16
    // TargetHeight     UI16
    // FileNameLen      UI8             without extension, only name
    // FileName         UI8[FileNameLen]

    GFxStream* pin = p->GetStream();

    UInt    bitmapResourceId    = p->ReadU16();
    UInt16  bitmapFormat        = p->ReadU16();
    UInt16  origWidth           = p->ReadU16();
    UInt16  origHeight          = p->ReadU16();
    char*   pimageExportName    = pin->ReadStringWithLength();
    char*   pimageFileName      = pin->ReadStringWithLength();

    pin->LogParse("  DefineExternalImage: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', exp = '%s', w = %d, h = %d\n",
        tagInfo.TagType,
        bitmapResourceId,
        bitmapFormat,
        pimageFileName,
        pimageExportName,
        origWidth,
        origHeight);

    // Register the image file info.
    GFx_CreateImageFileResourceHandle(p, GFxResourceId(bitmapResourceId),
                                      pimageFileName, pimageExportName, bitmapFormat,
                                      origWidth, origHeight);
    if (pimageFileName)
        GFREE(pimageFileName);
    if (pimageExportName)
        GFREE(pimageExportName);
}


void    GSTDCALL GFx_FontTextureInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_FontTextureInfo);

    // Glyphs' texture info tags
    // utilizes the tag 1002 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1002
    // TextureID        UI32            Texture ID
    // TextureFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // FileNameLen      UI8             name of file with texture's image (without extension)
    // FileName         UI8[FileNameLen]
    // TextureWidth     UI16
    // TextureHeight    UI16
    // PadPixels        UI8             
    // NominalGlyphSz   UI16            Nominal height of glyphs
    // NumTexGlyphs     UI16            Number of texture glyphs
    // TexGlyphs        TEXGLYPH[NumTexGlyphs]
    // NumFonts         UI16            Number of fonts using this texture
    // Fonts            FONTINFO[NumFonts]  Font infos
    //
    // FONTINFO
    // FontId           UI16
    // NumGlyphs        UI16            Number of texture glyphs in the font from the current texture
    // GlyphIndices     GLYPHIDX[NumGlyphs] Mapping of font glyph's indices to textures' ones (TexGlyphs)
    //
    // GLYPHIDX
    // IndexInFont      UI16            Index in font
    // IndexInTexture   UI16            Index in texture
    //
    // TEXGLYPH:
    // UvBoundsLeft     FLOAT
    // UvBoundsTop      FLOAT
    // UvBoundsRight    FLOAT
    // UvBoundsBottom   FLOAT
    // UvOriginX        FLOAT
    // UvOriginY        FLOAT

    GFxStream*  pin = p->GetStream();    
    
    UInt    textureId       = pin->ReadU32();
    UInt16  bitmapFormat    = pin->ReadU16();
    char*   pimageFileName  = pin->ReadStringWithLength();

    // Load texture glyph configuration.
    GFxFontPackParams::TextureConfig  tgc;
    tgc.TextureWidth   = pin->ReadU16();
    tgc.TextureHeight  = pin->ReadU16();
    tgc.PadPixels      = pin->ReadU8();
    tgc.NominalSize    = pin->ReadU16();

    pin->LogParse("  FontTextureInfo: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', w = %d, h = %d\n",
                  tagInfo.TagType, textureId,
                  bitmapFormat, pimageFileName,
                  tgc.TextureWidth, tgc.TextureHeight);


    // Fill in file information.
    GASSERT((textureId & GFxResourceId::IdType_FontImage) == GFxResourceId::IdType_FontImage);
    GFxResourceHandle rh = GFx_CreateImageFileResourceHandle(
                                p, GFxResourceId(textureId),
                                pimageFileName, "", bitmapFormat,
                                (UInt16) tgc.TextureWidth, (UInt16) tgc.TextureHeight);   

    // Load texture glyphs info first
    GTL::garray<GPtr<GFxTextureGlyph> > texGlyphsInTexture;
    UInt numTexGlyphs = pin->ReadU16();
    UInt i;

    pin->LogParse("  PadPixels = %d, nominal glyph size = %d, numTexGlyphs = %d\n",
                  tgc.PadPixels, tgc.NominalSize, numTexGlyphs);

    for (i = 0; i < numTexGlyphs; ++i)
    {
        // load TEXGLYPH
        Float uvBoundsLeft      = pin->ReadFloat();
        Float uvBoundsTop       = pin->ReadFloat();
        Float uvBoundsRight     = pin->ReadFloat();
        Float uvBoundsBottom    = pin->ReadFloat();
        Float uvOriginX         = pin->ReadFloat();
        Float uvOriginY         = pin->ReadFloat();

        pin->LogParse("  TEXGLYPH[%d]: uvBnd.Left = %f, uvBnd.Top = %f,"
                      " uvBnd.Right = %f, uvBnd.Bottom = %f\n",
                      i, uvBoundsLeft, uvBoundsTop, uvBoundsRight, uvBoundsBottom);
        pin->LogParse("                uvOrigin.x = %f, uvOrigin.y = %f\n",
                      uvOriginX, uvOriginY);

        // create GFxTextureGlyph
        GPtr<GFxTextureGlyph> ptexGlyph = *new GFxTextureGlyph();
        ptexGlyph->SetImageResource(rh);
        ptexGlyph->UvBounds = GRenderer::Rect(uvBoundsLeft, uvBoundsTop,
                                              uvBoundsRight, uvBoundsBottom);
        ptexGlyph->UvOrigin = GRenderer::Point(uvOriginX, uvOriginY);
        texGlyphsInTexture.push_back(ptexGlyph);
    }

    // load fonts' info
    UInt numFonts = pin->ReadU16();
    pin->LogParse("  NumFonts = %d\n", numFonts);
    
    for (i = 0; i < numFonts; ++i)
    {
        // Load FONTINFO
        UInt16       fontId = pin->ReadU16();
        GFxFontData* pfd    = p->GetDataDef()->GetFontData(GFxResourceId(fontId));
        if (!pfd)
        {
            GFC_DEBUG_WARNING1(1, "FontTextureInfoLoader: can't find font, fontId = (%d)\n",
                                  fontId);
            continue;
        }

        // Get/Create font texture data.
        // When fonts are spread among multiple textures, texture glyphs would
        // already be created by the previous tag.
        GPtr<GFxTextureGlyphData> ptextureGlyphData = pfd->GetTextureGlyphData();
        if (!ptextureGlyphData)
        {
            if (ptextureGlyphData = *new GFxTextureGlyphData(pfd->GetGlyphShapeCount(), true))
            {
                ptextureGlyphData->SetTextureConfig(tgc);
                pfd->SetTextureGlyphData(ptextureGlyphData);
            }
        }
        
        if (ptextureGlyphData)
            ptextureGlyphData->AddTexture(GFxResourceId(textureId), rh);
        
        UInt numGlyphsInFont = pin->ReadU16();
        for (UInt i = 0; i < numGlyphsInFont; ++i)
        {
            // load GLYPHIDX
            UInt indexInFont    = pin->ReadU16();
            UInt indexInTexture = pin->ReadU16();
            
            if (ptextureGlyphData)
                ptextureGlyphData->AddTextureGlyph(
                    indexInFont, *texGlyphsInTexture[indexInTexture].GetPtr());
        }
    }

    pin->LogParse("\n");

    if (pimageFileName)
        GFREE(pimageFileName);
}


void    GSTDCALL GFx_DefineExternalGradientImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);
    GASSERT(tagInfo.TagType == GFxTag_DefineExternalGradient);

    // Utilizes the tag 1003 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1001
    // GradientID       UI16
    // BitmapsFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // GradientSize     UI16
    // FileNameLen      UI8             without extension, only name
    // FileName         UI8[FileNameLen]

    GFxStream*              pin = p->GetStream();

    UInt    gradientId          = pin->ReadU16();
    UInt    bitmapResourceId    = gradientId | GFxResourceId::IdType_GradientImage;
    UInt16  bitmapFormat        = pin->ReadU16();
    UInt16  gradSize            = pin->ReadU16();
    char*   pimageFileName      = pin->ReadStringWithLength();

    pin->LogParse("  DefineExternalGradientImage: tagInfo.TagType = %d, id = 0x%X, fmt = %d, name = '%s', size = %d\n",
        tagInfo.TagType,
        bitmapResourceId,
        bitmapFormat,
        pimageFileName,
        gradSize);

    // Register image resource data.
    GFx_CreateImageFileResourceHandle(p, GFxResourceId(bitmapResourceId),
                                      pimageFileName, "", bitmapFormat,
                                      gradSize, gradSize); 
    if (pimageFileName)
        GFREE(pimageFileName);
}


void    GSTDCALL GFx_DefineGradientMapLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo)
{
    GUNUSED(tagInfo);    
    GASSERT(tagInfo.TagType == GFxTag_DefineGradientMap);

    p->LogWarning("Deprecated tag 1004 - DefineGradientMapLoader encountered, ignored\n");

    /*

    // Utilizes the tag 1004 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1004
    // NumGradients     UI16            
    // Indices          UI16[NumGradients]

    UInt    numGradients = pin->ReadU16();

    pin->LogParse("  DefineGradientMap: tagInfo.TagType = %d, num = %d\n",
        tagInfo.TagType,
        numGradients);
    GFxGradientParams* pgrParams = pin->GetLoader()->GetGradientParams();
    if (pgrParams == 0)
    {
        GFxGradientParams grParams;
        pin->GetLoader()->SetGradientParams(&grParams);
        pgrParams = pin->GetLoader()->GetGradientParams();
    }
    GASSERT(pgrParams);
    pgrParams->GradientMap.SetSize(numGradients);

    for (UInt i = 0; i < numGradients; ++i)
    {
        UInt16 v = pin->ReadU16();
        pgrParams->GradientMap.pData[i] = v;
    }
    */
}

