/*********************************************************************

Filename    :   GFxScale9Grid.h
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFxScale9Grid_H
#define INC_GFxScale9Grid_H

#include "GRefCount.h"
#include "GRenderer.h"
#include "GFxStyles.h"


//------------------------------------------------------------------------
struct GFxScale9Grid : public GNewOverrideBase
{
    Float  x,y,w,h;
    const GFxScale9Grid& operator= (const GRectF& r)
    {
        x = r.Left; y = r.Top; w = r.Right-r.Left; h = r.Bottom-r.Top;
        return *this;
    }
};

//------------------------------------------------------------------------
class GCompoundShape;
struct GFxScale9GridInfo : public GRefCountBase<GFxScale9GridInfo>
{
    enum { KeySize = 19 };

    struct ImgAdjust
    {
        Float x1, y1, x2, y2;
        GMatrix2D Matrix;
        ImgAdjust() : x1(1e30f), y1(1e30f), x2(-1e30f), y2(-1e30f) {}
    };

    GFxScale9Grid           Scale9Grid;
    GMatrix2D               S9gMatrix;
    GMatrix2D               ShapeMatrix;
    Float                   PixelScale;
    GRectF                  Bounds;
    bool                    CanUseTiling;

    GRectF                  ResultingGrid;
    GMatrix2D               InverseMatrix;
    GMatrix2D               ResultingMatrices[9];
    GTL::garray<ImgAdjust>  ImgAdjustments;

    GFxScale9GridInfo() {}
    GFxScale9GridInfo(const GFxScale9Grid* gr, 
                      const GMatrix2D& s9gMtx, const GMatrix2D& shapeMtx,
                      Float pixelScale, const GRectF& bounds);

    void    Compute();
    void    Transform(Float* x, Float* y) const;
    GRectF  AdjustBounds(const GRectF& bounds) const;
    void    ComputeImgAdjustRects(const GCompoundShape& cs, 
                                  const GFxFillStyle* pfillStyles,
                                  UPInt fillStylesNum);
    void    ComputeImgAdjustMatrices();

    void MakeKey(Float* values) const
    {
        *values++ = Scale9Grid.x;
        *values++ = Scale9Grid.y;
        *values++ = Scale9Grid.w;
        *values++ = Scale9Grid.h;
        *values++ = S9gMatrix.M_[0][0];
        *values++ = S9gMatrix.M_[0][1];
        *values++ = S9gMatrix.M_[1][0];
        *values++ = S9gMatrix.M_[1][1];
        *values++ = ShapeMatrix.M_[0][0];
        *values++ = ShapeMatrix.M_[0][1];
        *values++ = ShapeMatrix.M_[0][2];
        *values++ = ShapeMatrix.M_[1][0];
        *values++ = ShapeMatrix.M_[1][1];
        *values++ = ShapeMatrix.M_[1][2];
        *values++ = PixelScale;
        *values++ = Bounds.Left;
        *values++ = Bounds.Top;
        *values++ = Bounds.Right;
        *values++ = Bounds.Bottom;
    }
};


// Texture Scale-9-grid calculates and keeps matrices and grid points
// to implement proper texture resizing according to the scaling grid.
//------------------------------------------------------------------------
class GFxDisplayContext;
class GFxFillStyle;
struct GFxTexture9Grid
{
    void Compute(const GFxScale9GridInfo& sg,
                 const GRectF& clipRect,  
                 const GMatrix2D& textureMatrix,
                 Float scaleMultiplier,
                 UInt  fillStyleIdx);

    void Display(GFxDisplayContext &context,
                 const GFxFillStyle& style) const;

    void TransformPoint(UInt idx, 
                        const GMatrix2D& imageMatrix, 
                        const GMatrix2D& invDisplayMatrix, 
                        Float scaleMultiplier, const GRectF& clipRect,
                        Float x, Float y);

    Float       ScaleMultiplierInv;
    SInt16      GridPoints[9*4*2];
    GMatrix2D   TextureMatrices[9];
    UInt        FillStyleIdx;
};




#endif
