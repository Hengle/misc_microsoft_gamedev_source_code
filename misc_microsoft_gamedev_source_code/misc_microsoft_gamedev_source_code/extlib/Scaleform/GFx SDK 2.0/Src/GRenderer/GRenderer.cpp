/**********************************************************************

Filename    :   GRenderer.cpp
Content     :   Vector graphics 2D renderer implementation
Created     :   June 29, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRenderer.h"
#include "GContainers.h"

#include "GImage.h"
#include "GMath.h"
#include "GStd.h"
#include <stdio.h>
#include <string.h>

#define GFX_STREAM_BUFFER_SIZE  512


// ***** Nested types


//
// cxform
//


GRenderer::Cxform   GRenderer::Cxform::Identity;


// Initialize to identity transform.
GRenderer::Cxform::Cxform()
{
    SetIdentity();
}

// Concatenate c's transform onto ours.  When
// transforming colors, c's transform is applied
// first, then ours.
void    GRenderer::Cxform::Concatenate(const GRenderer::Cxform& c)
{
    M_[0][1] += M_[0][0] * c.M_[0][1];
    M_[1][1] += M_[1][0] * c.M_[1][1];
    M_[2][1] += M_[2][0] * c.M_[2][1];
    M_[3][1] += M_[3][0] * c.M_[3][1];

    M_[0][0] *= c.M_[0][0];
    M_[1][0] *= c.M_[1][0];
    M_[2][0] *= c.M_[2][0];
    M_[3][0] *= c.M_[3][0];
}
GRenderer::Cxform operator * (const GRenderer::Cxform &c1, const GRenderer::Cxform &c2)
{
	GRenderer::Cxform N;
    N.M_[0][1] = c1.M_[0][1] + c1.M_[0][0] * c2.M_[0][1];
    N.M_[1][1] = c1.M_[1][1] + c1.M_[1][0] * c2.M_[1][1];
    N.M_[2][1] = c1.M_[2][1] + c1.M_[2][0] * c2.M_[2][1];
    N.M_[3][1] = c1.M_[3][1] + c1.M_[3][0] * c2.M_[3][1];

    N.M_[0][0] = c1.M_[0][0] * c2.M_[0][0];
    N.M_[1][0] = c1.M_[1][0] * c2.M_[1][0];
    N.M_[2][0] = c1.M_[2][0] * c2.M_[2][0];
    N.M_[3][0] = c1.M_[3][0] * c2.M_[3][0];
	return N;
}

// Apply our transform to the given color; return the result.
GColor  GRenderer::Cxform::Transform(const GColor in) const
{
    GColor  result(
        (UByte) GTL::gclamp<Float>(in.GetRed() * M_[0][0] + M_[0][1], 0, 255),
        (UByte) GTL::gclamp<Float>(in.GetGreen() * M_[1][0] + M_[1][1], 0, 255),
        (UByte) GTL::gclamp<Float>(in.GetBlue() * M_[2][0] + M_[2][1], 0, 255),
        (UByte) GTL::gclamp<Float>(in.GetAlpha() * M_[3][0] + M_[3][1], 0, 255) );

    return result;
}


// Debug log.
void    GRenderer::Cxform::Format(char *pbuffer) const
{

    gfc_sprintf(pbuffer, GFX_STREAM_BUFFER_SIZE,
            "    *         +\n"
            "| %4.4f %4.4f|\n"
            "| %4.4f %4.4f|\n"
            "| %4.4f %4.4f|\n"
            "| %4.4f %4.4f|\n",
            M_[0][0], M_[0][1],
            M_[1][0], M_[1][1],
            M_[2][0], M_[2][1],
            M_[3][0], M_[3][1] );
}

void    GRenderer::Cxform::SetIdentity()
{
    M_[0][0] = 1;
    M_[1][0] = 1;
    M_[2][0] = 1;
    M_[3][0] = 1;
    M_[0][1] = 0;
    M_[1][1] = 0;
    M_[2][1] = 0;
    M_[3][1] = 0;
}

bool    GRenderer::Cxform::IsIdentity() const
{
    return (M_[0][0] == 1 &&  M_[1][0] == 1 && M_[2][0] == 1 && M_[3][0] == 1 &&
            M_[0][1] == 0 &&  M_[1][1] == 0 && M_[2][1] == 0 && M_[3][1] == 0);
}






//------------------------------------------------------------------------
// The code of these classes and functions was taken from the Anti-Grain 
// Geometry Project and modified for the use by Scaleform. 
// Permission to use without restrictions is hereby granted to 
// Scaleform Corporation by the author of Anti-Grain Geometry Project.
// See http://antigtain.com for details.
//------------------------------------------------------------------------





//------------------------------------------------------------------------
class GLinearInterpolator
{
public:
    //--------------------------------------------------------------------
    GLinearInterpolator() {}
    GLinearInterpolator(int y1, int y2, int count) :
        Cnt(count),
        Lft((y2 - y1) / Cnt),
        Rem((y2 - y1) % Cnt),
        Mod(Rem),
        m_y(y1)
    {
        if(Mod <= 0)
        {
            Mod += count;
            Rem += count;
            Lft--;
        }
        Mod -= count;
    }

    //--------------------------------------------------------------------
    void operator++()
    {
        Mod += Rem;
        m_y   += Lft;
        if(Mod > 0)
        {
            Mod -= Cnt;
            m_y++;
        }
    }

    //--------------------------------------------------------------------
    int y() const { return m_y; }

private:
    int Cnt;
    int Lft;
    int Rem;
    int Mod;
    int m_y;
};



//------------------------------------------------------------------------
enum 
{
    GImgSubpixelShift   = 8,
    GImgSubpixelShift2  = GImgSubpixelShift * 2,
    GImgSubpixelScale   = 1 << GImgSubpixelShift,
    GImgSubpixelMask    = GImgSubpixelScale - 1,
    GImgSubpixelOffset  = GImgSubpixelScale / 2,
    GImgSubpixelInitial = GImgSubpixelScale * GImgSubpixelScale / 2
};


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

    GLinearInterpolator ix(offset, 
                           offset + (srcWidth << GImgSubpixelShift),
                           dstWidth);

    for(x = 0; x < dstWidth; x++)
    {
        srcCoordX[x] = ix.y() - GImgSubpixelOffset;
        ++ix;
    }

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

        const UByte* pSrcRow1 = (yInt < 0)? 
                                         pSrc: 
                                         pSrc + srcPitch * yInt;
        ++yInt;
        if(yInt >= srcHeight) yInt = srcHeight - 1;
        const UByte* pSrcRow2 = pSrc + srcPitch * yInt;

        UByte* pDst3 = pDst2;
        int xInt, xFract, x1, x2;

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


void GRenderer::ResizeImage(UByte* pDst, 
                            int dstWidth, int dstHeight, int dstPitch,
                            const UByte* pSrc, 
                            int srcWidth, int srcHeight, int srcPitch,
                            ResizeImageType type)
{
    switch(type)
    {
    case ResizeRgbToRgb:
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 3,
                              pSrc, srcWidth, srcHeight, srcPitch, 3,
                              GPixelFilterBilinearRGB24);
        break;


    case ResizeRgbaToRgba: 
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 4,
                              pSrc, srcWidth, srcHeight, srcPitch, 4,
                              GPixelFilterBilinearRGBA32);
        break;

    case ResizeRgbToRgba:
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 4,
                              pSrc, srcWidth, srcHeight, srcPitch, 3,
                              GPixelFilterBilinearRGBtoRGBA32);
        break;

    case ResizeGray:
        GImageResizeFilter2x2(pDst, dstWidth, dstHeight, dstPitch, 1,
                              pSrc, srcWidth, srcHeight, srcPitch, 1,
                              GPixelFilterBilinearGray8);
        break;
    }
}

