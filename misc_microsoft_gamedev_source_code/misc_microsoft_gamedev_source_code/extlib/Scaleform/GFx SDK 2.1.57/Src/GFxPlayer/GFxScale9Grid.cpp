/*********************************************************************

Filename    :   GFxScale9Grid.cpp
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

#include "GCompoundShape.h"
#include "GFxScale9Grid.h"
#include "GFxDisplayContext.h"


//------------------------------------------------------------------------
GFxScale9GridInfo::GFxScale9GridInfo(const GFxScale9Grid* gr, 
                                     const GMatrix2D& s9gMtx, 
                                     const GMatrix2D& shapeMtx,
                                     Float pixelScale, 
                                     const GRectF& bounds)
{
    Scale9Grid = *gr, 
    S9gMatrix  = s9gMtx;
    ShapeMatrix = shapeMtx;
    PixelScale = pixelScale;
    Bounds = bounds;
    CanUseTiling = false; 
}

//------------------------------------------------------------------------
void GFxScale9GridInfo::Compute()
{
    // The images can be drawn with automatic tiling according to the 
    // scale9grid. It is possible when the image is on the level of the 
    // scale9grid movie clip; to be more precise, it's possible when the 
    // ShapeMatrix has no transformations.
    CanUseTiling = (ShapeMatrix == GMatrix2D::Identity);

    InverseMatrix = S9gMatrix.GetInverse();
    InverseMatrix.Append(ShapeMatrix.GetInverse());

    // Scaling Grid
    Float gx1 = Scale9Grid.x;
    Float gy1 = Scale9Grid.y;
    Float gx2 = Scale9Grid.x + Scale9Grid.w;
    Float gy2 = Scale9Grid.y + Scale9Grid.h;

    // Sprite Bounds
    Float bx1 = Bounds.Left;
    Float by1 = Bounds.Top;
    Float bx2 = Bounds.Right;
    Float by2 = Bounds.Bottom;

    // Transformed Bounds
    Float tx1 = bx1;
    Float ty1 = by1;
    Float tx2 = bx2;
    Float ty2 = by1;
    Float tx3 = bx2;
    Float ty3 = by2;
    Float tx4 = bx1;
    Float ty4 = by2;

    // Transform bounds (Y axis goes up)
    //         4                           3 
    //         *----+-----------------+----*
    //        /    /                 /    /
    //       +----+-----------------+----+ 
    //      /    /                 /    /
    //     /    /                 /    /
    //    /    /                 /    /
    //   +----+-----------------+----+ 
    //  /    /                 /    /
    // *----+-----------------+----*
    // 1                           2
    //------------------------------------------
    S9gMatrix.Transform(&tx1, &ty1);
    S9gMatrix.Transform(&tx2, &ty2);
    S9gMatrix.Transform(&tx3, &ty3);
    S9gMatrix.Transform(&tx4, &ty4);

    // Compute pseudo width and height of the bounds
    Float w = GMath2D::CalcDistance(tx1, ty1, tx2, ty2);
    Float h = GMath2D::CalcDistance(tx2, ty2, tx3, ty3);

    // Coefficients for the margins
    Float kx1 = (gx1 - bx1) / w;
    Float ky1 = (gy1 - by1) / h;
    Float kx2 = (bx2 - gx2) / w;
    Float ky2 = (by2 - gy2) / h;

    // Prevent from "overlapping"
    Float d = kx1 + kx2;
    if (d > 1)
    {
        d += 0.05f;
        kx1 /= d;
        kx2 /= d;
    }

    d = ky1 + ky2;
    if (d > 1)
    {
        d += 0.05f;
        ky1 /= d;
        ky2 /= d;
    }

    // Compute the outer points (Y axis goes up)
    //              6                 5
    //         +----*-----------------*----+
    //        /    /                 /    /
    //     7 *----+-----------------+----* 4
    //      /    /                 /    /
    //     /    /                 /    /
    //    /    /                 /    /
    // 8 *----+-----------------+----* 3
    //  /    /                 /    /
    // +----*-----------------*----+
    //      1                 2
    //------------------------------------------
    Float ox1 = tx1 + (tx2-tx1) * kx1;
    Float oy1 = ty1 + (ty2-ty1) * kx1;
    Float ox2 = tx2 - (tx2-tx1) * kx2;
    Float oy2 = ty2 - (ty2-ty1) * kx2;
    Float ox3 = tx2 + (tx3-tx2) * ky1;
    Float oy3 = ty2 + (ty3-ty2) * ky1;
    Float ox4 = tx3 - (tx3-tx2) * ky2;
    Float oy4 = ty3 - (ty3-ty2) * ky2;
    Float ox5 = tx3 - (tx3-tx4) * kx2;
    Float oy5 = ty3 - (ty3-ty4) * kx2;
    Float ox6 = tx4 + (tx3-tx4) * kx1;
    Float oy6 = ty4 + (ty3-ty4) * kx1;
    Float ox7 = tx4 - (tx4-tx1) * ky2;
    Float oy7 = ty4 - (ty4-ty1) * ky2;
    Float ox8 = tx1 + (tx4-tx1) * ky1;
    Float oy8 = ty1 + (ty4-ty1) * ky1;

    // Compute the inner points (Y axis goes up)
    // 
    //         +----+-----------------+----+
    //        /    /                 /    /
    //       +----*-----------------*----+
    //      /    / 4             3 /    /
    //     /    /                 /    /
    //    /    / 1             2 /    /
    //   +----*-----------------*----+
    //  /    /                 /    /
    // +----+-----------------+----+
    // 
    //------------------------------------------
    Float ix1 = ox8 + (tx2-tx1) * kx1;
    Float iy1 = oy8 + (ty2-ty1) * kx1;
    Float ix2 = ox3 - (tx2-tx1) * kx2;
    Float iy2 = oy3 - (ty2-ty1) * kx2;
    Float ix3 = ox4 - (tx3-tx4) * kx2;
    Float iy3 = oy4 - (ty3-ty4) * kx2;
    Float ix4 = ox7 + (tx3-tx4) * kx1;
    Float iy4 = oy7 + (ty3-ty4) * kx1;

// DBG
//GFx_DrawQuad(tx1, ty1, tx2, ty2, tx3, ty3, tx4, ty4, 0x7FFF0000, context.GetRenderer());
//GFx_DrawQuad(ox1, oy1, ox2, oy2, ox5, oy5, ox6, oy6, 0x7F00FF00, context.GetRenderer());
//GFx_DrawQuad(ox8, oy8, ox3, oy3, ox4, oy4, ox7, oy7, 0x7F0000FF, context.GetRenderer());
//GFx_DrawQuad(ix1, iy1, ix2, iy2, ix3, iy3, ix4, iy4, 0x7F000000, context.GetRenderer());


    // Area Matrices (Y-axis goes up)
    //        |        |
    //   m6   |   m7   |  m8
    //        |        |
    // -------+--------+--------
    //        |        |
    //   m3   |   m4   |  m5
    //        |        |
    // -------+--------+--------
    //        |        |
    //   m0   |   m1   |  m2
    //        |        |
    //------------------------------
    Float pr[6];
    #define GFX_PARL(a,b,c,d,e,f) { pr[0]=a; pr[1]=b; pr[2]=c; pr[3]=d; pr[4]=e; pr[5]=f; }

    GFX_PARL(tx1, ty1, ox1, oy1, ix1, iy1); ResultingMatrices[0].SetRectToParl(bx1, by1, gx1, gy1, pr);
    GFX_PARL(ox1, oy1, ox2, oy2, ix2, iy2); ResultingMatrices[1].SetRectToParl(gx1, by1, gx2, gy1, pr);
    GFX_PARL(ox2, oy2, tx2, ty2, ox3, oy3); ResultingMatrices[2].SetRectToParl(gx2, by1, bx2, gy1, pr);
    GFX_PARL(ox8, oy8, ix1, iy1, ix4, iy4); ResultingMatrices[3].SetRectToParl(bx1, gy1, gx1, gy2, pr);
    GFX_PARL(ix1, iy1, ix2, iy2, ix3, iy3); ResultingMatrices[4].SetRectToParl(gx1, gy1, gx2, gy2, pr);
    GFX_PARL(ix2, iy2, ox3, oy3, ox4, oy4); ResultingMatrices[5].SetRectToParl(gx2, gy1, bx2, gy2, pr);
    GFX_PARL(ox7, oy7, ix4, iy4, ox6, oy6); ResultingMatrices[6].SetRectToParl(bx1, gy2, gx1, by2, pr);
    GFX_PARL(ix4, iy4, ix3, iy3, ox5, oy5); ResultingMatrices[7].SetRectToParl(gx1, gy2, gx2, by2, pr);
    GFX_PARL(ix3, iy3, ox4, oy4, tx3, ty3); ResultingMatrices[8].SetRectToParl(gx2, gy2, bx2, by2, pr);
    #undef GFX_PARL

    ResultingGrid.Left   = gx1;
    ResultingGrid.Top    = gy1;
    ResultingGrid.Right  = gx2;
    ResultingGrid.Bottom = gy2;
}

void GFxScale9GridInfo::Transform(Float* x, Float* y) const
{
    // Area Codes (Y-axis goes up)
    //        |        |
    // 0110   |  0010  | 0011
    // 6->m6  |  2->m7 | 3->m8
    // -------+--------+-------- y2
    //        |        |
    // 0100   |  0000  | 0001
    // 4->m3  |  0->m4 | 1->m5
    // -------+--------+-------- y1
    //        |        |
    // 1100   |  1000  | 1001
    // 12->m0 |  8->m1 | 9->m2
    //        x1       x2
    //------------------------------
    enum {X=0};
    static const UInt8 codeToMtx[] = { 4, 5, 7, 8, 3, X, 6, X, 1, 2, X, X, 0 };
    //                                 0  1  2  3  4  5  6  7  8  9  10 11 12

    ShapeMatrix.Transform(x, y);
    UInt areaCode =  (*x > ResultingGrid.Right) |       // See "Area Codes"
                    ((*y > ResultingGrid.Bottom) << 1) | 
                    ((*x < ResultingGrid.Left)   << 2) | 
                    ((*y < ResultingGrid.Top)    << 3);
    ResultingMatrices[codeToMtx[areaCode]].Transform(x, y);
    InverseMatrix.Transform(x, y);
}

//------------------------------------------------------------------------
GRectF GFxScale9GridInfo::AdjustBounds(const GRectF& bounds) const
{
    Float x1 = bounds.Left;
    Float y1 = bounds.Top;
    Float x2 = bounds.Right;
    Float y2 = bounds.Top;
    Float x3 = bounds.Right;
    Float y3 = bounds.Bottom;
    Float x4 = bounds.Left;
    Float y4 = bounds.Bottom;
    Transform(&x1, &y1);
    Transform(&x2, &y2);
    Transform(&x3, &y3);
    Transform(&x4, &y4);

    Float xb1=x1, yb1=y1, xb2=x1, yb2=y1;
    if (x2 < xb1) xb1 = x2; if (y2 < yb1) yb1 = y2;
    if (x2 > xb2) xb2 = x2; if (y2 > yb2) yb2 = y2;
    if (x3 < xb1) xb1 = x3; if (y3 < yb1) yb1 = y3;
    if (x3 > xb2) xb2 = x3; if (y3 > yb2) yb2 = y3;
    if (x4 < xb1) xb1 = x4; if (y4 < yb1) yb1 = y4;
    if (x4 > xb2) xb2 = x4; if (y4 > yb2) yb2 = y4;

    return GRectF(xb1, yb1, xb2, yb2);
}

//------------------------------------------------------------------------
void GFxScale9GridInfo::ComputeImgAdjustRects(const GCompoundShape& cs, 
                                              const GFxFillStyle* pfillStyles,
                                              UPInt fillStylesNum)
{
    UInt i;
    for (i = 0; i < cs.GetNumPaths(); ++i)
    {
        const GCompoundShape::SPath& path = cs.GetPath(i);
        if (path.GetLeftStyle() >= 0)
        {
            GASSERT(path.GetLeftStyle() < (int)fillStylesNum);
            const GFxFillStyle& fill = pfillStyles[path.GetLeftStyle()];
            if (fill.IsGradientFill() || fill.IsImageFill())
            {
                if (ImgAdjustments.size() == 0)
                    ImgAdjustments.resize(fillStylesNum);
                ImgAdjust& adj = ImgAdjustments[path.GetLeftStyle()];
                cs.ExpandPathBounds(path, &adj.x1, &adj.y1, &adj.x2, &adj.y2);
            }
        }
        if (path.GetRightStyle() >= 0)
        {
            GASSERT(path.GetRightStyle() < (int)fillStylesNum);
            const GFxFillStyle& fill = pfillStyles[path.GetRightStyle()];
            if (fill.IsGradientFill() || fill.IsImageFill())
            {
                if (ImgAdjustments.size() == 0)
                    ImgAdjustments.resize(fillStylesNum);
                ImgAdjust& adj = ImgAdjustments[path.GetRightStyle()];
                cs.ExpandPathBounds(path, &adj.x1, &adj.y1, &adj.x2, &adj.y2);
            }
        }
    }
}

//------------------------------------------------------------------------
void GFxScale9GridInfo::ComputeImgAdjustMatrices()
{
    UInt i;
    for(i = 0; i < ImgAdjustments.size(); ++i)
    {
        ImgAdjust& adj = ImgAdjustments[i];
        if (adj.x1 < adj.x2 && adj.y1 < adj.y2)
        {
            Float p[6] = { adj.x1, adj.y1, adj.x2, adj.y1, adj.x2, adj.y2 };
            Transform(&p[0], &p[1]);
            Transform(&p[2], &p[3]);
            Transform(&p[4], &p[5]);
            adj.Matrix.SetRectToParl(adj.x1, adj.y1, adj.x2, adj.y2, p);
        }
    }
}



//------------------------------------------------------------------------
void GFxTexture9Grid::Compute(const GFxScale9GridInfo& sg,
                              const GRectF& clipRect,  
                              const GMatrix2D& textureMatrix,
                              Float scaleMultiplier,
                              UInt  styleIdx)
{
    ScaleMultiplierInv = 1/scaleMultiplier;
    FillStyleIdx       = styleIdx;

    // (Y axis goes up, so, Top and Bottom are inverse)
    // sg.ResultingGrid - the inner scaling grid rectangle
    // sg.Bounds        - the outer Movie Clip bounding box
    // clipRect         - clipping rectangle (typically shape bounding box)
    // m0...m8          - transformation matrices (sg.ResultingMatrices)
    // 0...35           - resulting grid points, transformed accordingly by m[0]...m[8]
    //
    //             *--------*-----------------*--------*
    //            / 27  26 / 31           30 / 35  34 /
    //           /   m6   /        m7       /   m8   /
    //          / 24  25 / 28           29 / 32  33 /
    //         *--------*-----------------*--------* 
    //        / 15  14 / 19           18 / 23  22 /
    //       /   m3   /       m4        /   m5   /
    //      / 12  13 / 16           17 / 20  21 /
    //     *--------*-----------------*--------* 
    //    / 3    2 / 7             6 / 11  10 /
    //   /   m0   /       m1        /   m2   /
    //  / 0    1 / 4             5 / 8    9 /
    // *--------*-----------------*--------*
    // 
    // The idea is to compute the grid points for the triangle pairs.
    // IMPORTANT: the resulting mesh there must use the same affine matrix, 
    // otherwise seam pixels will appear. So, the only possibility to draw 
    // the areas properly is to apply different texture matrices. 
    //---------------------------------


    // The "slices" array corresponds to "sg.ResultingMatrices", then it's 
    // transformed to the GridPoint array.
    //---------------------
    GRectF slices[9];
    slices[0].SetRect(sg.Bounds.Left,         sg.Bounds.Top,           sg.ResultingGrid.Left,  sg.ResultingGrid.Top);
    slices[1].SetRect(sg.ResultingGrid.Left,  sg.Bounds.Top,           sg.ResultingGrid.Right, sg.ResultingGrid.Top);
    slices[2].SetRect(sg.ResultingGrid.Right, sg.Bounds.Top,           sg.Bounds.Right,        sg.ResultingGrid.Top);
    slices[3].SetRect(sg.Bounds.Left,         sg.ResultingGrid.Top,    sg.ResultingGrid.Left,  sg.ResultingGrid.Bottom);
    slices[4].SetRect(sg.ResultingGrid.Left,  sg.ResultingGrid.Top,    sg.ResultingGrid.Right, sg.ResultingGrid.Bottom);
    slices[5].SetRect(sg.ResultingGrid.Right, sg.ResultingGrid.Top,    sg.Bounds.Right,        sg.ResultingGrid.Bottom);
    slices[6].SetRect(sg.Bounds.Left,         sg.ResultingGrid.Bottom, sg.ResultingGrid.Left,  sg.Bounds.Bottom);
    slices[7].SetRect(sg.ResultingGrid.Left,  sg.ResultingGrid.Bottom, sg.ResultingGrid.Right, sg.Bounds.Bottom);
    slices[8].SetRect(sg.ResultingGrid.Right, sg.ResultingGrid.Bottom, sg.Bounds.Right,        sg.Bounds.Bottom);

    UInt i, j;
    for(i = 0; i < 9; ++i)
    {
        // Clip the slice and if it's not empty transform the points, 
        // according to the sg.ResultingMatrices. When fully clipped, assign 
        // the same coordinates to GridPoints[n...n+8]
        //--------------------
        UInt n = i * 4 * 2;
        slices[i].Intersect(clipRect);
        if (!slices[i].IsEmpty())
        {
            const GRectF& r = slices[i];
            Float points[8] = { r.Left, r.Top, r.Right, r.Top, r.Right, r.Bottom, r.Left, r.Bottom };
            for (j = 0; j < 8; j += 2)
            {
                sg.Transform(&points[j], &points[j+1]);
                GridPoints[n+j  ] = SInt16(points[j  ] * scaleMultiplier);
                GridPoints[n+j+1] = SInt16(points[j+1] * scaleMultiplier);
            }

            //// Adjacent points must have exactly the same coordinates.
            //// This logic ensures it is so.
            //// ------------------
            //if (i%3 > 0 && !slices[i-1].IsEmpty())
            //{
            //    // Assign X coordinates from the slice in the left
            //    GridPoints[n+0] = GridPoints[n-8+2];
            //    GridPoints[n+6] = GridPoints[n-8+4];
            //}
            //if (i > 2 && !slices[i-3].IsEmpty())
            //{
            //    // Assign X coordinates from the slice below
            //    GridPoints[n+1] = GridPoints[n-24+5];
            //    GridPoints[n+3] = GridPoints[n-24+7];
            //}
        }
        else
        {
            for (j = 0; j < 8; ++j)
                GridPoints[n+j] = 0;
        }
    }

    // Texture matrices are non-trivial to compute. 
    // The resulting formula was obtained by the trial-and-error method.
    //-------------------------
    GMatrix2D invMatrix = sg.S9gMatrix;
    invMatrix.PrependScaling(ScaleMultiplierInv);
    invMatrix.Invert();

    for (i = 0; i < 9; ++i)
    {
        GRenderer::Matrix mat2 = sg.ResultingMatrices[i];
        mat2.Append(invMatrix);
        mat2.Invert();
        TextureMatrices[i] = textureMatrix;
        TextureMatrices[i].Prepend(mat2); 
    }
}


//------------------------------------------------------------------------
void GFxTexture9Grid::Display(GFxDisplayContext &context,
                              const GFxFillStyle& style) const
{
    GRenderer* ren = context.GetRenderer();

    GRenderer::FillTexture  texture;
    style.GetFillTexture(&texture,  context, ScaleMultiplierInv, 0);

    // (Y axis goes up)
    //             *--------*------------*--------*
    //            / 27  26 / 31      30 / 35  34 /
    //           /        /            /        /
    //          / 24  25 / 28      29 / 32  33 /
    //         *--------*------------*--------* 
    //        / 15  14 / 19      18 / 23  22 /
    //       /        /            /        /
    //      / 12  13 / 16      17 / 20  21 /
    //     *--------*------------*--------* 
    //    / 3    2 / 7        6 / 11  10 /
    //   /        /            /        /
    //  / 0    1 / 4        5 / 8    9 /
    // *--------*------------*--------*
    // 
    // Vertex indexes (mesh) for 9 quadrilaterals. 
    // Each line represents a pair of triangles that correspond to 
    // the respective "slice".
    static const UInt16 tri[9*6] = 
    {
        0,  1,  2,  0,  2,  3, 
        4,  5,  6,  4,  6,  7, 
        8,  9, 10,  8, 10, 11,
       12, 13, 14, 12, 14, 15, 
       16, 17, 18, 16, 18, 19, 
       20, 21, 22, 20, 22, 23,
       24, 25, 26, 24, 26, 27,
       28, 29, 30, 28, 30, 31,
       32, 33, 34, 32, 34, 35
    };
    ren->SetVertexData(GridPoints, 9*4, GRenderer::Vertex_XY16i);
    ren->SetIndexData(tri, 9*6, GRenderer::Index_16);

    for (UInt i = 0; i < 9; i++)
    {
        UInt n = i*4*2;
        if (GridPoints[n]   != GridPoints[n+4] &&
            GridPoints[n+1] != GridPoints[n+5])
        {
            // Set the respective texture matrix
            texture.TextureMatrix = TextureMatrices[i];
            ren->FillStyleBitmap(&texture);

            // Fill the inside
            ren->DrawIndexedTriList(0, 0, 9*4, i*6, 2);
        }
    }
}


