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
#include "GResizeImage.h"
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


// This is a compatibility function. The renderers should call 
// GResizeImageBilinear() directly.
void GRenderer::ResizeImage(UByte* pDst, 
                            int dstWidth, int dstHeight, int dstPitch,
                            const UByte* pSrc, 
                            int srcWidth, int srcHeight, int srcPitch,
                            ResizeImageType type)
{
    switch(type)
    {
    case ResizeRgbToRgb:
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeRgbToRgb);
        break;


    case ResizeRgbaToRgba: 
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeRgbaToRgba);
        break;

    case ResizeRgbToRgba:
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeRgbToRgba);
        break;

    case ResizeGray:
        GResizeImageBilinear(pDst, dstWidth, dstHeight, dstPitch,
                             pSrc, srcWidth, srcHeight, srcPitch,
                             GResizeGray);
        break;
    }
}

