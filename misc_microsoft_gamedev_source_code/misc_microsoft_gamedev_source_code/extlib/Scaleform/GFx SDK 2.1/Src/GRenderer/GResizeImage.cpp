/**********************************************************************

Filename    :   GResizeImage.cpp
Content     :   Bitmap Image resizing with filtering
Created     :   2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of these classes was taken from the Anti-Grain Geometry
Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
See http://antigtain.com for details.
**********************************************************************/

#include "GResizeImage.h"

//--------------------------------------------------------------------
void GImageFilterLut::reallocLut(Float radius)
{
    Radius = radius;
    Diameter = unsigned(ceilf(radius)) * 2;
    Start = -int(Diameter / 2 - 1);
    unsigned size = Diameter << GImgSubpixelShift;
    if (size > WeightArray.size())
        WeightArray.allocate(size);
}



//--------------------------------------------------------------------
// This function normalizes integer values and corrects the rounding 
// errors. It corrects the filter values according to the rule 
// of 1.0 which means that any sum of pixel weights must be equal to 1.0.
// So, the filter function must produce a graph of the proper shape.
//--------------------------------------------------------------------
void GImageFilterLut::Normalize()
{
    unsigned i;
    int flip = 1;

    for(i = 0; i < GImgSubpixelScale; i++)
    {
        for(;;)
        {
            int sum = 0;
            unsigned j;
            for(j = 0; j < Diameter; j++)
            {
                sum += WeightArray[j * GImgSubpixelScale + i];
            }

            if(sum == GImgFilterScale) break;

            Float k = Float(GImgFilterScale) / Float(sum);
            sum = 0;
            for(j = 0; j < Diameter; j++)
            {
                sum +=           WeightArray[j * GImgSubpixelScale + i] = 
                    SInt16(gfrnd(WeightArray[j * GImgSubpixelScale + i] * k));
            }

            sum -= GImgFilterScale;
            int inc = (sum > 0) ? -1 : 1;

            for(j = 0; j < Diameter && sum; j++)
            {
                flip ^= 1;
                unsigned idx = flip ? Diameter/2 + j/2 : Diameter/2 - j/2;
                int v = WeightArray[idx * GImgSubpixelScale + i];
                if(v < GImgFilterScale)
                {
                    WeightArray[idx * GImgSubpixelScale + i] = 
                        SInt16(WeightArray[idx * GImgSubpixelScale + i] + inc);
                    sum += inc;
                }
            }
        }
    }

    unsigned pivot = Diameter << (GImgSubpixelShift - 1);

    for(i = 0; i < pivot; i++)
    {
        WeightArray[pivot + i] = WeightArray[pivot - i];
    }
    unsigned end = (Diameter << GImgSubpixelShift) - 1;
    WeightArray[0] = WeightArray[end];
}


//------------------------------------------------------------------------
#if defined(GFC_CC_MWERKS) && defined(GFC_OS_PS2)
#pragma optimization_level 0
#endif


//------------------------------------------------------------------------
static GINLINE void GPixelFilterBoxGray8(UByte* pDst, const UByte* pSrc)
{
    *pDst = *pSrc;
}

//------------------------------------------------------------------------
static GINLINE void GPixelFilterBoxRGB24(UByte* pDst, const UByte* pSrc)
{
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
}

//------------------------------------------------------------------------
static GINLINE void GPixelFilterBoxRGBA32(UByte* pDst, const UByte* pSrc)
{
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
    pDst[3] = pSrc[3];
}


//------------------------------------------------------------------------
static GINLINE void GPixelFilterBoxRGBtoRGBA32(UByte* pDst, const UByte* pSrc)
{
    pDst[0] = pSrc[0];
    pDst[1] = pSrc[1];
    pDst[2] = pSrc[2];
    pDst[3] = 255;
}


//------------------------------------------------------------------------
template<class FilterFunction>
static void 
GImageResizeBox(      UByte* pDst, int dstWidth, int dstHeight, int dstPitch, int dstBpp,
                const UByte* pSrc, int srcWidth, int srcHeight, int srcPitch, int srcBpp,
                FilterFunction filter)
{
    GPodVector<int> srcCoordX;
    srcCoordX.allocate(dstWidth);
    int x, y;
    int offset = GImgSubpixelOffset * srcWidth / dstWidth;

    // Prepare the X-interpolator and fill the interpolation array for faster access
    GLinearInterpolator ix(offset, 
                           offset + (srcWidth << GImgSubpixelShift),
                           dstWidth);

    for(x = 0; x < dstWidth; x++)
    {
        srcCoordX[x] = ix.y();
        ++ix;
    }

    // Prepare the Y-interpolator
    offset = GImgSubpixelOffset * srcHeight / dstHeight;
    GLinearInterpolator iy(offset, 
                           offset + (srcHeight << GImgSubpixelShift),
                           dstHeight);

    for(y = 0; y < dstHeight; y++)
    {
        int xInt;
        int yInt = iy.y() >> GImgSubpixelShift;
        if(yInt < 0)          yInt = 0;
        if(yInt >= srcHeight) yInt = srcHeight - 1;
        const UByte* pSrcRow = pSrc + srcPitch * yInt;
              UByte* pDstRow = pDst + dstPitch * y;

        // Filter left pixels, while the filtering window 
        // is out of the left bound of the source image.
        for(x = 0; x < dstWidth; x++)
        {
            xInt = srcCoordX[x] >> GImgSubpixelShift;
            if(xInt >= 0) break;
            filter(pDstRow, pSrcRow);
            pDstRow += dstBpp;
        }

        // Filter central pixels, while the filtering window 
        // is fully visible in the source image.
        int srcWidth1 = srcWidth - 1;
        for(; x < dstWidth; x++)
        {
            xInt = srcCoordX[x] >> GImgSubpixelShift;
            if(xInt >= srcWidth1) break;
            filter(pDstRow, pSrcRow + xInt * srcBpp);
            pDstRow += dstBpp;
        }

        // Filter right pixels, when the filtering window 
        // is out of the right bound of the source image.
        pSrcRow += (srcWidth - 1) * srcBpp;
        for(; x < dstWidth; x++)
        {
            filter(pDstRow, pSrcRow);
            pDstRow += dstBpp;
        }
        ++iy;
    }
}


//------------------------------------------------------------------------
void GResizeImageBox(UByte* pDst, 
                     int dstWidth, int dstHeight, int dstPitch,
                     const UByte* pSrc, 
                     int srcWidth, int srcHeight, int srcPitch,
                     GResizeImageType type)
{
    if (dstWidth <= 0 || dstHeight <= 0 ||
        srcWidth <= 0 || srcHeight <= 0) return;

    switch(type)
    {
    case GResizeRgbToRgb:
        GImageResizeBox(pDst, dstWidth, dstHeight, dstPitch, 3,
                        pSrc, srcWidth, srcHeight, srcPitch, 3,
                        GPixelFilterBoxRGB24);
        break;

    case GResizeRgbaToRgba: 
        GImageResizeBox(pDst, dstWidth, dstHeight, dstPitch, 4,
                        pSrc, srcWidth, srcHeight, srcPitch, 4,
                        GPixelFilterBoxRGBA32);
        break;

    case GResizeRgbToRgba:
        GImageResizeBox(pDst, dstWidth, dstHeight, dstPitch, 4,
                        pSrc, srcWidth, srcHeight, srcPitch, 3,
                        GPixelFilterBoxRGBtoRGBA32);
        break;

    case GResizeGray:
        GImageResizeBox(pDst, dstWidth, dstHeight, dstPitch, 1,
                        pSrc, srcWidth, srcHeight, srcPitch, 1,
                        GPixelFilterBoxGray8);
        break;
    }
}



//------------------------------------------------------------------------
static GINLINE void GPixelFilterBilinearRGBA32(UByte* pDst, 
                                               const UByte* pSrc1,
                                               const UByte* pSrc2,
                                               const UByte* pSrc3,
                                               const UByte* pSrc4,
                                               int xFract, int yFract)
{
    int dst[4] = { GImgSubpixelInitial, GImgSubpixelInitial, 
                   GImgSubpixelInitial, GImgSubpixelInitial };
    int weight;

    weight = (GImgSubpixelScale - xFract) * (GImgSubpixelScale - yFract);
    dst[0] += weight * pSrc1[0];
    dst[1] += weight * pSrc1[1];
    dst[2] += weight * pSrc1[2];
    dst[3] += weight * pSrc1[3];

    weight = xFract * (GImgSubpixelScale - yFract);
    dst[0] += weight * pSrc2[0];
    dst[1] += weight * pSrc2[1];
    dst[2] += weight * pSrc2[2];
    dst[3] += weight * pSrc2[3];

    weight = (GImgSubpixelScale - xFract) * yFract;
    dst[0] += weight * pSrc3[0];
    dst[1] += weight * pSrc3[1];
    dst[2] += weight * pSrc3[2];
    dst[3] += weight * pSrc3[3];

    weight = xFract * yFract;
    dst[0] += weight * pSrc4[0];
    dst[1] += weight * pSrc4[1];
    dst[2] += weight * pSrc4[2];
    dst[3] += weight * pSrc4[3];

    pDst[0] = (UByte)(dst[0] >> GImgSubpixelShift2);
    pDst[1] = (UByte)(dst[1] >> GImgSubpixelShift2);
    pDst[2] = (UByte)(dst[2] >> GImgSubpixelShift2);
    pDst[3] = (UByte)(dst[3] >> GImgSubpixelShift2);
}


//------------------------------------------------------------------------
static GINLINE void GPixelFilterBilinearRGB24(UByte* pDst, 
                                              const UByte* pSrc1,
                                              const UByte* pSrc2,
                                              const UByte* pSrc3,
                                              const UByte* pSrc4,
                                              int xFract, int yFract)
{
    int dst[3] = { GImgSubpixelInitial, GImgSubpixelInitial, GImgSubpixelInitial };
    int weight;

    weight = (GImgSubpixelScale - xFract) * (GImgSubpixelScale - yFract);
    dst[0] += weight * pSrc1[0];
    dst[1] += weight * pSrc1[1];
    dst[2] += weight * pSrc1[2];

    weight = xFract * (GImgSubpixelScale - yFract);
    dst[0] += weight * pSrc2[0];
    dst[1] += weight * pSrc2[1];
    dst[2] += weight * pSrc2[2];

    weight = (GImgSubpixelScale - xFract) * yFract;
    dst[0] += weight * pSrc3[0];
    dst[1] += weight * pSrc3[1];
    dst[2] += weight * pSrc3[2];

    weight = xFract * yFract;
    dst[0] += weight * pSrc4[0];
    dst[1] += weight * pSrc4[1];
    dst[2] += weight * pSrc4[2];

    pDst[0] = (UByte)(dst[0] >> GImgSubpixelShift2);
    pDst[1] = (UByte)(dst[1] >> GImgSubpixelShift2);
    pDst[2] = (UByte)(dst[2] >> GImgSubpixelShift2);
}


//------------------------------------------------------------------------
static GINLINE void GPixelFilterBilinearRGBtoRGBA32(UByte* pDst, 
                                                    const UByte* pSrc1,
                                                    const UByte* pSrc2,
                                                    const UByte* pSrc3,
                                                    const UByte* pSrc4,
                                                    int xFract, int yFract)
{
    GPixelFilterBilinearRGB24(pDst, pSrc1, pSrc2, pSrc3, pSrc4, xFract, yFract);
    pDst[3] = 255;
}


//------------------------------------------------------------------------
static GINLINE void GPixelFilterBilinearGray8(UByte* pDst, 
                                              const UByte* pSrc1,
                                              const UByte* pSrc2,
                                              const UByte* pSrc3,
                                              const UByte* pSrc4,
                                              int xFract, int yFract)
{
    *pDst = (UByte)((GImgSubpixelInitial +
        *pSrc1 * (GImgSubpixelScale - xFract) * (GImgSubpixelScale - yFract) +
        *pSrc2 *                      xFract  * (GImgSubpixelScale - yFract) +
        *pSrc3 * (GImgSubpixelScale - xFract) *                      yFract  +
        *pSrc4 *                      xFract  *                      yFract) >> GImgSubpixelShift2);
}




//------------------------------------------------------------------------
template<class FilterFunction>
static void 
GImageResizeFilter2x2(      UByte* pDst, int dstWidth, int dstHeight, int dstPitch, int dstBpp,
                      const UByte* pSrc, int srcWidth, int srcHeight, int srcPitch, int srcBpp,
                      FilterFunction filter)
{
    GPodVector<int> srcCoordX;
    srcCoordX.allocate(dstWidth);
    int x, y;
    int offset = GImgSubpixelOffset * srcWidth / dstWidth;

    // Prepare the X-interpolator and fill the interpolation array for faster acceess
    GLinearInterpolator ix(offset, 
                           offset + (srcWidth << GImgSubpixelShift),
                           dstWidth);

    for(x = 0; x < dstWidth; x++)
    {
        srcCoordX[x] = ix.y() - GImgSubpixelOffset;
        ++ix;
    }

    // Prepare the Y-interpolator
    offset = GImgSubpixelOffset * srcHeight / dstHeight;
    GLinearInterpolator iy(offset, 
                           offset + (srcHeight << GImgSubpixelShift),
                           dstHeight);

    UByte* pDst2 = pDst;

    for(y = 0; y < dstHeight; y++)
    {
        int yFract = iy.y() - GImgSubpixelOffset;
        int yInt   = yFract >> GImgSubpixelShift;
        yFract    &= GImgSubpixelMask;

        const UByte* pSrcRow1 = (yInt < 0) ? pSrc : pSrc + srcPitch * yInt;
        ++yInt;
        if(yInt >= srcHeight) yInt = srcHeight - 1;
        const UByte* pSrcRow2 = pSrc + srcPitch * yInt;

        UByte* pDst3 = pDst2;
        int xInt, xFract, x1, x2;

        // Filter left pixels, while the filtering window 
        // is out of the left bound of the source image.
        for(x = 0; x < dstWidth; x++)
        {
            xFract  = srcCoordX[x];
            xInt    = xFract >> GImgSubpixelShift;
            xFract &= GImgSubpixelMask;
            if(xInt >= 0) break;
            filter(pDst3, 
                   pSrcRow1, pSrcRow1, 
                   pSrcRow2, pSrcRow2, 
                   xFract, yFract);
            pDst3 += dstBpp;
        }

        // Filter central pixels, while the filtering window 
        // is fully visible in the source image.
        int srcWidth1 = srcWidth - 1;
        for(; x < dstWidth; x++)
        {
            xFract  = srcCoordX[x];
            xInt    = xFract >> GImgSubpixelShift;
            xFract &= GImgSubpixelMask;
            if(xInt >= srcWidth1) break;
            x1 =  xInt      * srcBpp;
            x2 = (xInt + 1) * srcBpp;
            filter(pDst3, 
                   pSrcRow1 + x1, pSrcRow1 + x2, 
                   pSrcRow2 + x1, pSrcRow2 + x2, 
                   xFract, yFract);
            pDst3 += dstBpp;
        }

        // Filter right pixels, when the filtering window 
        // is out of the right bound of the source image.
        pSrcRow1 += (srcWidth - 1) * srcBpp;
        pSrcRow2 += (srcWidth - 1) * srcBpp;
        for(; x < dstWidth; x++)
        {
            xFract  = srcCoordX[x] & GImgSubpixelMask;
            filter(pDst3, 
                   pSrcRow1, pSrcRow1, 
                   pSrcRow2, pSrcRow2, 
                   xFract, yFract);
            pDst3 += dstBpp;
        }

        ++iy;
        pDst2 += dstPitch;
    }
}


//--------------------------------------------------------------------
void GResizeImageBilinear(UByte* pDst, 
                          int dstWidth, int dstHeight, int dstPitch,
                          const UByte* pSrc, 
                          int srcWidth, int srcHeight, int srcPitch,
                          GResizeImageType type)
{
    if (dstWidth <= 0 || dstHeight <= 0 ||
        srcWidth <= 0 || srcHeight <= 0) return;

    switch(type)
    {
    case GResizeRgbToRgb:
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 3,
                              pSrc, srcWidth, srcHeight, srcPitch, 3,
                              GPixelFilterBilinearRGB24);
        break;

    case GResizeRgbaToRgba: 
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 4,
                              pSrc, srcWidth, srcHeight, srcPitch, 4,
                              GPixelFilterBilinearRGBA32);
        break;

    case GResizeRgbToRgba:
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 4,
                              pSrc, srcWidth, srcHeight, srcPitch, 3,
                              GPixelFilterBilinearRGBtoRGBA32);
        break;

    case GResizeGray:
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 1,
                              pSrc, srcWidth, srcHeight, srcPitch, 1,
                              GPixelFilterBilinearGray8);
        break;
    }
}





//--------------------------------------------------------------------
struct GPixelFilterGray8
{
    enum { SrcBpp = 1, DstBpp = 1 };

    static GINLINE void CopySrcPixel(UByte* pDst, const UByte* pSrc)
    {
        *pDst = *pSrc;
    }

    static GINLINE void Filter(UByte* pDst, 
                               const UByte* pSrc,
                               const SInt16* weightArray,
                               int xFilter, int xCounter)
    {
        int dst = GImgFilterScale/2;
        for(;;)
        {
            dst += weightArray[xFilter] * *pSrc++;
            if(--xCounter == 0) break;
            xFilter += GImgSubpixelScale;
        }
        dst >>= GImgFilterShift;
        if(dst < 0)   dst = 0;
        if(dst > 255) dst = 255;
        *pDst = (UByte)dst;
    }
};


//--------------------------------------------------------------------
struct GPixelFilterRGB24
{
    enum { SrcBpp = 3, DstBpp = 3 };

    static GINLINE void CopySrcPixel(UByte* pDst, const UByte* pSrc)
    {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
    }

    static GINLINE void Filter(UByte* pDst, 
                               const UByte* pSrc,
                               const SInt16* weightArray,
                               int xFilter, int xCounter)
    {
        int dst[] = { GImgFilterScale/2, GImgFilterScale/2, GImgFilterScale/2 }; 
        for(;;)
        {
            int weight = weightArray[xFilter];
            dst[0] += weight * pSrc[0];
            dst[1] += weight * pSrc[1];
            dst[2] += weight * pSrc[2];
            if(--xCounter == 0) break;
            xFilter += GImgSubpixelScale;
            pSrc    += SrcBpp;
        }

        dst[0] >>= GImgFilterShift;
        dst[1] >>= GImgFilterShift;
        dst[2] >>= GImgFilterShift;

        if(dst[0] < 0) dst[0] = 0;
        if(dst[1] < 0) dst[1] = 0;
        if(dst[2] < 0) dst[2] = 0;

        if(dst[0] > 255) dst[0] = 255;
        if(dst[1] > 255) dst[1] = 255;
        if(dst[2] > 255) dst[2] = 255;

        pDst[0] = (UByte)dst[0];
        pDst[1] = (UByte)dst[1];
        pDst[2] = (UByte)dst[2];
    }
};


//--------------------------------------------------------------------
struct GPixelFilterRGBA32
{
    enum { SrcBpp = 4, DstBpp = 4 };

    static GINLINE void CopySrcPixel(UByte* pDst, const UByte* pSrc)
    {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
        pDst[3] = pSrc[3];
    }

    static GINLINE void Filter(UByte* pDst, 
                               const UByte* pSrc,
                               const SInt16* weightArray,
                               int xFilter, int xCounter)
    {
        int dst[4] = { GImgFilterScale/2, GImgFilterScale/2, 
                       GImgFilterScale/2, GImgFilterScale/2 }; 

        for(;;)
        {
            int weight = weightArray[xFilter];
            dst[0] += weight * pSrc[0];
            dst[1] += weight * pSrc[1];
            dst[2] += weight * pSrc[2];
            dst[3] += weight * pSrc[3];
            if(--xCounter == 0) break;
            xFilter += GImgSubpixelScale;
            pSrc    += SrcBpp;
        }

        dst[0] >>= GImgFilterShift;
        dst[1] >>= GImgFilterShift;
        dst[2] >>= GImgFilterShift;
        dst[3] >>= GImgFilterShift;

        if(dst[0] < 0) dst[0] = 0;
        if(dst[1] < 0) dst[1] = 0;
        if(dst[2] < 0) dst[2] = 0;
        if(dst[3] < 0) dst[3] = 0;

        if(dst[0] > 255) dst[0] = 255;
        if(dst[1] > 255) dst[1] = 255;
        if(dst[2] > 255) dst[2] = 255;
        if(dst[3] > 255) dst[3] = 255;

        pDst[0] = (UByte)dst[0];
        pDst[1] = (UByte)dst[1];
        pDst[2] = (UByte)dst[2];
        pDst[3] = (UByte)dst[3];
    }
};


//--------------------------------------------------------------------
struct GPixelFilterRGB24toRGBA32
{
    enum { SrcBpp = 3, DstBpp = 4 };

    static GINLINE void CopySrcPixel(UByte* pDst, const UByte* pSrc)
    {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
    }

    static GINLINE void Filter(UByte* pDst, 
                               const UByte* pSrc,
                               const SInt16* weightArray,
                               int xFilter, int xCounter)
    {
        GPixelFilterRGB24::Filter(pDst, pSrc, weightArray, xFilter, xCounter);
        pDst[3] = 255;
    }
};



//--------------------------------------------------------------------
template<class FilterType>
static void GResizeImageRow(      UByte* pDst, unsigned dstWidth, int dstInc,
                            const UByte* pSrc, unsigned srcWidth, 
                            const int* srcCoordX,
                            const FilterType& filter,
                            const GImageFilterLut& lut)
{
    GUNUSED(filter); // Suppress stupid warning
    unsigned      diameter = lut.GetDiameter();
    int           start    = lut.GetStart();
    const SInt16* weights  = lut.GetWeightArray();

    unsigned i;
    unsigned x;
    int xFract;
    int xInt;
    int j;
    UByte tmpSrc[(GImgMaxFilterRadius*2+2) * FilterType::SrcBpp];

    // Filter left pixels, while the filtering window 
    // is out of the left bound of the source image.
    for(x = 0; x < dstWidth; ++x)
    {
        xFract = srcCoordX[x];
        xInt   = (xFract >> GImgSubpixelShift) + start;
        if (xInt >= 0) break;

        for (i = 0; i < diameter; ++i)
        {
            j = xInt + int(i);
            if (j < 0) j = 0;
            filter.CopySrcPixel(tmpSrc + i * FilterType::SrcBpp, 
                                pSrc   + j * FilterType::SrcBpp);
        }
        filter.Filter(pDst, tmpSrc, weights, 
                      GImgSubpixelMask - (xFract & GImgSubpixelMask), 
                      diameter);
        pDst += dstInc;
    }

    // Filter central pixels, while the filtering window 
    // is fully visible in the source image.
    for(; x < dstWidth; ++x)
    {
        xFract = srcCoordX[x];
        xInt   = (xFract >> GImgSubpixelShift) + start;
        if (xInt + diameter > srcWidth) break;
        filter.Filter(pDst, 
                      pSrc + FilterType::SrcBpp * xInt, 
                      weights, 
                      GImgSubpixelMask - (xFract & GImgSubpixelMask), 
                      diameter);
        pDst += dstInc;
    }

    // Filter right pixels, when the filtering window 
    // is out of the right bound of the source image.
    for(; x < dstWidth; ++x)
    {
        xFract  = srcCoordX[x];
        xInt    = (xFract >> GImgSubpixelShift) + start;

        for (i = 0; i < diameter; ++i)
        {
            j = xInt + int(i);
            if (j >= int(srcWidth)) j = int(srcWidth) - 1;
            filter.CopySrcPixel(tmpSrc + i * FilterType::SrcBpp, 
                                pSrc   + j * FilterType::SrcBpp);
        }
        filter.Filter(pDst, tmpSrc, weights, 
                      GImgSubpixelMask - (xFract & GImgSubpixelMask), 
                      diameter);
        pDst += dstInc;
    }
}


//--------------------------------------------------------------------
static void GCreateResizeInterpolationArray(GPodVector<int>& srcCoord, 
                                            int dstWidth, int srcWidth)
{
    int x;
    int offset = GImgSubpixelOffset * srcWidth / dstWidth;
    GLinearInterpolator ix(offset, 
                           offset + (srcWidth << GImgSubpixelShift),
                           dstWidth);

    srcCoord.allocate(dstWidth);
    for(x = 0; x < dstWidth; ++x)
    {
        srcCoord[x] = ix.y() - GImgSubpixelOffset;
        ++ix;
    }
}


//--------------------------------------------------------------------
template<class FilterType1, class FilterType2>
static void GResizeImageTwoPass(UByte* pDst, 
                                int dstWidth, int dstHeight, int dstPitch,
                                const UByte* pSrc, 
                                int srcWidth, int srcHeight, int srcPitch,
                                const FilterType1& filter1,
                                const FilterType2& filter2,
                                const GImageFilterLut& lut)
{
    GPodVector<int>   srcCoordX;
    GPodVector<UByte> tmpImg;
    int y;

    // This function resizes the image with filtering in two passes,
    // by X and Y with transposition for faster access to the memory. 
    // It requires a temporary buffer of dstWidth * srcHeight, but
    // reduces the computational complexity from R*R to R+R, where 
    // R is the radius of the filter.
    //-----------------------------
    tmpImg.allocate(dstWidth * srcHeight * FilterType1::DstBpp);

    // Filter the image in the X direction and transpose it.
    UByte* pTmp = &tmpImg[0];
    GCreateResizeInterpolationArray(srcCoordX, dstWidth, srcWidth);
    for(y = 0; y < srcHeight; ++y)
    {
        GResizeImageRow(pTmp, dstWidth, srcHeight * FilterType1::DstBpp,
                        pSrc, srcWidth, 
                        &srcCoordX[0], filter1, lut);
        pSrc += srcPitch;
        pTmp += FilterType1::DstBpp;
    }

    // Filter the image in the Y direction and transpose it back.
    pTmp = &tmpImg[0];
    GCreateResizeInterpolationArray(srcCoordX, dstHeight, srcHeight);
    for(y = 0; y < dstWidth; ++y)
    {
        GResizeImageRow(pDst, dstHeight, dstPitch,
                        pTmp, srcHeight, 
                        &srcCoordX[0], filter2, lut);
        pTmp += srcHeight * FilterType1::DstBpp;
        pDst += FilterType2::DstBpp;
    }
}


//--------------------------------------------------------------------
void GResizeImage(UByte* pDst, 
                  int dstWidth, int dstHeight, int dstPitch,
                  const UByte* pSrc, 
                  int srcWidth, int srcHeight, int srcPitch,
                  GResizeImageType type,
                  const GImageFilterLut& lut)
{
    if (dstWidth <= 0 || dstHeight <= 0 ||
        srcWidth <= 0 || srcHeight <= 0) return;

    switch(type)
    {
        case GResizeRgbToRgb:
        {
            GPixelFilterRGB24 filter;
            GResizeImageTwoPass(pDst, dstWidth, dstHeight, dstPitch,
                                pSrc, srcWidth, srcHeight, srcPitch,
                                filter, filter, lut);
        }
        break;

        case GResizeRgbaToRgba: 
        {
            GPixelFilterRGBA32 filter;
            GResizeImageTwoPass(pDst, dstWidth, dstHeight, dstPitch,
                                pSrc, srcWidth, srcHeight, srcPitch,
                                filter, filter, lut);
        }
        break;

        case GResizeRgbToRgba:
        {
            GPixelFilterRGB24         filter1;
            GPixelFilterRGB24toRGBA32 filter2;
            GResizeImageTwoPass(pDst, dstWidth, dstHeight, dstPitch,
                                pSrc, srcWidth, srcHeight, srcPitch,
                                filter1, filter2, lut);
        }
        break;

    case GResizeGray:
        {
            GPixelFilterGray8 filter;
            GResizeImageTwoPass(pDst, dstWidth, dstHeight, dstPitch,
                                pSrc, srcWidth, srcHeight, srcPitch,
                                filter, filter, lut);
        }
        break;
    }
}




