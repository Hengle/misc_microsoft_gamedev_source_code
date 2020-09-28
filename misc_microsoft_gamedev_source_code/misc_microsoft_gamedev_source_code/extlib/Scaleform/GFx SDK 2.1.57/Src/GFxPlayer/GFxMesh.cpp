/**********************************************************************

Filename    :   GFxMesh.cpp
Content     :   Mesh structure implementation for shape characters.
Created     :
Authors     :

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFile.h"
#include "GFxLog.h"
#include "GFxStream.h"
#include "GFxMesh.h"
#include "GFxShape.h"
#include "GFxEdgeAAGen.h"
#include "GTessellator.h"
#include "GRenderer.h"
#include "GFxDisplayContext.h"
#include "GFxScale9Grid.h"
#include "GMatrix2D.h"

#include <float.h>
#include <stdlib.h>

#if defined(GFC_OS_WIN32)
    #include <windows.h>
#endif


// Utility.

// Dump the given coordinate GTL::garray into the given GFxStream.
static void    GFx_WriteCoordArray(GFile* out, const GTL::garray<SInt16>& PtArray)
{
    UPInt n = PtArray.size();

    out->WriteSInt32(SInt32(n));
    for (UPInt i = 0; i < n; i++)
    {
        out->WriteUInt16((UInt16) PtArray[i]);
    }
}


// Read the coordinate GTL::garray data from the GFxStream into *PtArray.
static void    GFx_ReadCoordArray(GFile* in, GTL::garray<SInt16>* PtArray)
{
    int n = in->ReadSInt32();

    PtArray->resize(n);
    for (int i = 0; i < n; i ++)
    {
        (*PtArray)[i] = (SInt16) in->ReadUInt16();
    }
}


//
// ***** GFxMesh
//


GFxMesh::GFxMesh()
{   
    StyleCount= 0;
    Styles[0] = (UInt)GFxPathConsts::Style_None;

#ifndef GFC_NO_FXPLAYER_EDGEAA
    Styles[1] = (UInt)GFxPathConsts::Style_None;
    Styles[2] = (UInt)GFxPathConsts::Style_None;  
    FillType  = GRenderer::GFill_Color;
#endif
}

void    GFxMesh::AddTriangles(UInt style, const GTessellator &sourceTess)
{
    // Assign style.
    if (TriangleIndices.size() == 0)
    {
        Styles[0]   = style;
        StyleCount  = 1;
    }
    else
    {   // Styles must match.
        GASSERT(Styles[0] == style);
    }

    UInt    triangleCount = sourceTess.GetNumTriangles();
    UInt    index = (UInt)TriangleIndices.size();
    UInt    triangle;
    TriangleIndices.resize(index + triangleCount * 3);

    for(triangle = 0; triangle<triangleCount; triangle++)
    {
        // Fetch triangle indices for each triangle
        const GTessellator::TriangleType& t = sourceTess.GetTriangle(triangle);
        TriangleIndices[index++] = (SInt16) t.v1;
        TriangleIndices[index++] = (SInt16) t.v2;
        TriangleIndices[index++] = (SInt16) t.v3;
    }
}

#ifndef GFC_NO_FXPLAYER_EDGEAA

// Sets the styles
void    GFxMesh::SetEdgeAAStyles(UInt styleCount, UInt style0, UInt style1, UInt style2)
{
    StyleCount = styleCount;
    Styles[0] = style0;
    Styles[1] = style1;
    Styles[2] = style2;
}
void    GFxMesh::SetGouraudFillType(GRenderer::GouraudFillType gft)
{
    FillType = gft;
}

#endif

void    GFxMesh::Clear()
{
    TriangleIndicesCache.ReleaseData(GRenderer::Cached_Index);
    TriangleIndices.clear();

    StyleCount= 0;
    Styles[0] = (UInt)GFxPathConsts::Style_None;

#ifndef GFC_NO_FXPLAYER_EDGEAA
    Styles[1] = (UInt)GFxPathConsts::Style_None;
    Styles[2] = (UInt)GFxPathConsts::Style_None;  
    FillType  = GRenderer::GFill_Color;
#endif
}


void    GFxMesh::Display(const GFxDisplayContext& dc,
                         const GFxFillStyle* pfills, UInt fillsSize,
                         UInt vertexStartIndex, UInt vertexCount, Float scaleMultiplier,
                         const GRenderer::Matrix* imgAdjustMatrices,
                         bool useEdgeAA)
{
    GRenderer *prenderer = dc.GetRenderer();

    // Pass GFxMesh to renderer.
    if (TriangleIndices.size() > 0)
    {
        const GRenderer::Matrix* imgAdjustMatrix;

#ifndef GFC_NO_FXPLAYER_EDGEAA
        if (useEdgeAA)
        {
            // Compute & apply style
            GRenderer::FillTexture  ft[3];
            GRenderer::FillTexture* pft[3] = {0, 0, 0};

            if (fillsSize)  
            {
                // In case of Font shapes, size can be 0.
                UInt styleIndex;
                for (styleIndex = 0; styleIndex < StyleCount; styleIndex++)
                {
                    imgAdjustMatrix = 
                        imgAdjustMatrices ?
                        &imgAdjustMatrices[Styles[styleIndex]] : 0;

                    if (Styles[styleIndex] != GFxPathConsts::Style_None &&
                        pfills[Styles[styleIndex]].GetFillTexture(&ft[styleIndex], 
                                                                  dc, scaleMultiplier, 
                                                                  imgAdjustMatrix))
                    {
                        pft[styleIndex] = &ft[styleIndex];
                    }
                }
            }
            
            // Apply style textures.        
            prenderer->FillStyleGouraud(FillType, pft[0], pft[1], pft[2]);
        }
        else
#else
        // useEdgeAA must always be zero if this is compiled 
        GASSERT(useEdgeAA == 0);
        GUNUSED2(fillsSize,useEdgeAA);
#endif
        // else (!useEdgeAA)
        {
            // No EdgeAA: single style.
            GASSERT(GetStyle() <= fillsSize);

            if (fillsSize)
            {
                imgAdjustMatrix = 
                    imgAdjustMatrices ?
                    &imgAdjustMatrices[GetStyle()] : 0;
                pfills[GetStyle()].Apply(dc, scaleMultiplier, imgAdjustMatrix);
            }
        }
    
        // Draw triangles.
        GRenderer::CacheProvider cp(&TriangleIndicesCache);
        prenderer->SetIndexData(&TriangleIndices[0], (int)TriangleIndices.size(), GRenderer::Index_16, &cp);
        prenderer->DrawIndexedTriList(vertexStartIndex, 0, vertexCount, 0, (int)(TriangleIndices.size() / 3));
    }
}



// Dump our data to *out.
void    GFxMesh::OutputCachedData(GFile* pout)
{
    GUNUSED(pout);
//  GFx_WriteCoordArray(out, TriangleIndices);
}

// Slurp our data from *out.
void    GFxMesh::InputCachedData(GFile* pin)
{
    GUNUSED(pin);
//  GFx_ReadCoordArray(in, &TriangleIndices);
}



//
// ***** GFxCachedStroke
//

// Default constructor, for GTL::garray<>.
GFxCachedStroke::GFxCachedStroke()
{
    Style = GFxPathConsts::Style_None;
#ifndef GFC_NO_FXPLAYER_STROKER
    pCachedMesh = 0;
    CachedWidth = -4.0f;
    CachedScaleFlag      = 0;
    CachedStrokeMatrixSx = 0;
    CachedStrokeMatrixSy = 0;

#ifdef GFC_BUILD_DEBUG
    RangeOverflowWarned = 0;
#endif

    EdgeAADisabled      = 0;

    UsedStrokerAA       = 0;

#ifndef GFC_NO_FXPLAYER_EDGEAA
    EdgeAA              = 0;
    AllowCxformAddAlpha = 0;
#endif

#endif
}
GFxCachedStroke::~GFxCachedStroke()
{ 
#ifndef GFC_NO_FXPLAYER_STROKER
    VerticesCache.ReleaseData(GRenderer::Cached_Vertex);
    CoordsCache.ReleaseData(GRenderer::Cached_Vertex);

    if (pCachedMesh)
        delete pCachedMesh;
#endif
}


void    GFxCachedStroke::AddShapePathVertices(const GCompoundShape& shape, UInt pathIndex, Float scaleMultiplier)
{
    GASSERT(pathIndex < shape.GetNumPaths());

    const GCompoundShape::SPath& p = shape.GetPath(pathIndex);

    if (Coords.size() == 0)
        Style = p.GetLineStyle();
    else
    {
        // All paths added must share a single line style.
        GASSERT(Style == p.GetLineStyle());
    }

    UInt    vertexCount = p.GetNumVertices();
    UInt    startIndex  = (UInt)Coords.size();
    UInt    j;

    Coords.resize(startIndex + vertexCount * 2);

    // Copy the vertices from the path.
    for (j = 0; j < vertexCount; j++)
    {
        Coords[startIndex + j*2 + 0] = (SInt16) (p.GetVertex(j).x * scaleMultiplier);
        Coords[startIndex + j*2 + 1] = (SInt16) (p.GetVertex(j).y * scaleMultiplier);
    }

    // And store the count.
    CoordCounts.push_back(vertexCount * 2);
}

// Render this line strip in the given style.
void    GFxCachedStroke::Display(GFxDisplayContext &context, const GFxLineStyle& style,
                                 const Matrix &mat, Float scaleMultiplier, Float errorTolerance,
                                 bool cxformAddAlpha, bool canBlend, bool scale9Grid)
{
    GASSERT(Coords.size() > 1);
    GASSERT((Coords.size() & 1) == 0);

    const GFxRenderConfig& rconfig = *context.GetRenderConfig();

#ifdef GFC_NO_FXPLAYER_STROKER
    GUNUSED(mat);
    GUNUSED4(canBlend,cxformAddAlpha,errorTolerance,scaleMultiplier);
    GUNUSED(scale9Grid);
#else

    // Width * 20.
    Float   strokeScaleFactor   = 1.0f; // Multiplier applied to scale width.
    Float   resultScaleFactor   = 1.0f; // Resulting scale factor which would include a matrix.
    Float   width               = style.GetWidth();
    bool    matrixPreMultiply   = 1;
    bool    useCachedStroke     = 0;
    Float   sx, sy;
    Float   rot, rotf;
    Float   screenSpaceWidth;
    
    bool    useStrokerAA = !canBlend;
    // If line style has non-255 alpha values, no stroker AA.
    if (style.CanBlend())
        useStrokerAA = 0;

# ifdef GFC_NO_FXPLAYER_EDGEAA
    GUNUSED(cxformAddAlpha);
# else
    bool    useEdgeAA = rconfig.IsUsingEdgeAA(); 
    if (style.HasComplexFill() && !rconfig.IsEdgeAATextured())
        useEdgeAA = 0;
# endif // GFC_NO_FXPLAYER_EDGEAA

    // Hairline rendering occurs in Flash under two conditions:
    //  1) The original width value is 0.
    //  2) The final computed width after transforms is <= 1. This means that
    //     lines are never actually drawn with a narrower width.
    // In our implementation, hairline rendering can also be forced with RF_StrokeHairline flag.

    if ((rconfig.GetStrokeRenderFlags() == GFxRenderConfig::RF_StrokeHairline) || (style.GetWidth() == 0))
        goto render_hairline_stroke;
    
    sx = scale9Grid ? (Float)1 : (Float)mat.GetXScale();
    sy = scale9Grid ? (Float)1 : (Float)mat.GetYScale();

    switch(style.GetScaling())
    {
    case GFxLineStyle::LineScaling_None:
        strokeScaleFactor = 1.0f;
        resultScaleFactor = sqrtf(sx*sx + sy*sy) * (Float)GFC_MATH_SQRT1_2;
        break;

    case GFxLineStyle::LineScaling_Normal:
        resultScaleFactor = sqrtf(sx*sx + sy*sy) * (Float)GFC_MATH_SQRT1_2;

        if (rconfig.GetStrokeRenderFlags() != GFxRenderConfig::RF_StrokeCorrect)
        {
            // In normal mode, simply use original scale factor;
            // it will be scaled anisotropically by matrix.
            matrixPreMultiply = 0;
            strokeScaleFactor = 1.0f;
        }
        else
        {
            strokeScaleFactor = resultScaleFactor;
        }
        break;

    // Try to mimic vertical and horizontal stroke scaling calculation.
    case GFxLineStyle::LineScaling_Vertical:
        {
            // Note that these H/V scaling equations are not always correct, just most of the time;
            // they model the axis-aligned cases and a lot of the rotation/scaling behavior.
            // Use stroke_rotation_v8.swf for testing.

            rot     = (Float)mat.GetRotation();
            rotf    = GTL::gmax<Float>(cosf(rot - (Float)GFC_MATH_PI_4),0) * (Float) GFC_MATH_SQRT2;

            Float sx2 = sx * sinf(rot);
            Float sy2 = sy * cosf(rot);
            strokeScaleFactor = rotf * sqrtf(sx2*sx2 + sy2*sy2);
            resultScaleFactor = strokeScaleFactor;
        }
        break;

    case GFxLineStyle::LineScaling_Horizontal:
        {
            rot     = (Float)mat.GetRotation();
            rotf    = GTL::gmax<Float>(cosf(rot + (Float)GFC_MATH_PI_4),0) * (Float) GFC_MATH_SQRT2;

            Float sx2 = sx * cosf(rot);
            Float sy2 = sy * sinf(rot);
            strokeScaleFactor = rotf * sqrtf(sx2*sx2 + sy2*sy2);
            resultScaleFactor = strokeScaleFactor;
        }
        break;

    default:
        break;
    }

    screenSpaceWidth = width * resultScaleFactor * context.GetPixelScale();

    // Blended connection points bigger then 1 pixel (20 twips) are definitely noticeable,
    // so we can not use strokerAA for them.
    if (screenSpaceWidth <= rconfig.GetStrokerAAWidth() * 20.0f)
    {
        // Stroker AA can be used for relatively narrow lines where overlap is
        // not a visible problem.
        useStrokerAA = 1;
    }

//useStrokerAA = 0; // To enforce the regular stroker

    // If less then one pixel - use hairline.
    if (screenSpaceWidth < 20.0f)
    {
        // Approximate hairline stroke by setting screen-space width to 1.
        // [Actual hairline is slower and does not provide smooth AA look].
        width = 20.0f / (resultScaleFactor * context.GetPixelScale());
    }

    // Compute the new width
    width *= strokeScaleFactor;

    // See if we already have a correctly cached mesh.
    if (pCachedMesh)
    {
        // If we require "correct scale", cached mesh must have matched that requirement.
        // EdgeAA start must also match (if disabling/enabling EdgeAA, tessellation is recomputed).
        if ((matrixPreMultiply == CachedScaleFlag) &&
            (!UsedStrokerAA || (useStrokerAA == UsedStrokerAA))
        //  (useStrokerAA == UsedStrokerAA)
# ifndef GFC_NO_FXPLAYER_EDGEAA
            && (useEdgeAA == EdgeAA) &&
            (!cxformAddAlpha || AllowCxformAddAlpha)
# endif
            )
        {
			
			if( rconfig.DoDynamicTessellation() )
			{
            // Widths must be within 1/5 pixel now = 4 twips.
            if ((width>(CachedWidth - 4)) && (width < (CachedWidth + 4)))
            {
                if (!matrixPreMultiply)
                {
                    // For Edge AA we technically would have to re-calculate this when width edge
                    // ratio changes; however, this is not necessary because GFxCachedStroke is a
                    // part of a mesh which is already discriminated based on scale.
                    useCachedStroke = 1;
                }
                else
                {
                    // Allow up to 1/2 pixel error due to scale.
                    // Since shape is (64K / multiplier) pixels wide, allow that much tolerance
                    Float scaleTolerance = scaleMultiplier * (1.0f / 128000.0f);

                    // Matrix scale and aspect ratio must match.
                    // Note that a flip in the sign of scale is allowed (only occurs in GetXScale of our matrix impl).
                    if ( (sy >= (CachedStrokeMatrixSy - scaleTolerance)) && (sy <= (CachedStrokeMatrixSy + scaleTolerance)) &&
                        ((sx >= (CachedStrokeMatrixSx - scaleTolerance)) && (sx <= (CachedStrokeMatrixSx + scaleTolerance)) ||
                         (sx >= (-CachedStrokeMatrixSx - scaleTolerance)) && (sx <= (-CachedStrokeMatrixSx + scaleTolerance)))  )
                         useCachedStroke = 1;

                    // Note: Correct stroking cache performance could be better if we allowed ourselves
                    // to cache multiple strokes for different scale {sx,sy} pairs (right now some scenes
                    // are more expensive because some strokes are rebuilt every frame if they are
                    // simultaneously displayed at different scales).
                }
            }
        }
			else
				useCachedStroke = 1;

		}


        if (!useCachedStroke)
        {
            VerticesCache.ReleaseData(GRenderer::Cached_Vertex);
            pCachedMesh->Clear();
            Vertices.Clear();
        }
    }
    else
    {
        if ((pCachedMesh = new GFxMesh)==0)
            return;
    }

    if (!useCachedStroke)
    {
        // Set scale/rotate/skew only matrix.
        Matrix  m, mInv;

# ifndef GFC_NO_FXPLAYER_EDGEAA
        bool    needEdgeAA = rconfig.IsUsingEdgeAA() && !EdgeAADisabled;
        if (style.HasComplexFill() && !rconfig.IsEdgeAATextured())
            needEdgeAA = 0;
        // use EdgeAA without cxform add alpha if it isn't supported anyway
        if (cxformAddAlpha && !rconfig.HasVertexFormat(GRenderer::Vertex_XY16iCF32))
        {
            if (rconfig.HasCxformAddAlpha())
                needEdgeAA = 0;
            else
                cxformAddAlpha = 0;
        }
# endif

        GASSERT(Vertices.GetSize() == 0);

        if (matrixPreMultiply)
        {
            m.M_[0][0] = mat.M_[0][0];
            m.M_[1][0] = mat.M_[1][0];
            m.M_[0][1] = mat.M_[0][1];
            m.M_[1][1] = mat.M_[1][1];
            mInv.SetInverse(m);
        }

        // Compute the stroked mesh
        GCompoundShape    lineShape;        

        // Populate line shape
        UInt        i, j, c, index;
        GPointF     pt;

        for(j = 0, index = 0; j<CoordCounts.size(); j++)
        {
            c = CoordCounts[j];
            lineShape.BeginPath(-1, -1, Style);

            if (matrixPreMultiply)
            {
                for (i = 0; i<c; i+=2, index+=2)
                {
                    // Add vertex transformed by our M
                    m.Transform(&pt, GPointF(Coords[index], Coords[index + 1]));
                    lineShape.AddVertex(pt.x, pt.y);
                }
            }
            else
            {
                for (i = 0; i<c; i+=2, index+=2)
                    lineShape.AddVertex(Coords[index], Coords[index + 1]);
            }
        }

        
        // Use StrokerAA instead of Stroker+Tessellator if possible.
        if (useStrokerAA)   
        {
            GStrokerAA strokerAA;

            strokerAA.SetStartLineCap(GFx_SWFToFxStroke_LineCap(style.GetStartCap()));
            strokerAA.SetEndLineCap(GFx_SWFToFxStroke_LineCap(style.GetEndCap()));
            strokerAA.SetLineJoin(GFx_SWFToFxStroke_LineJoin(style.GetJoin()));
            if (style.GetJoin() == GFxLineStyle::LineJoin_Miter)
                strokerAA.SetMiterLimit(style.GetMiterSize());

            strokerAA.SetCurveTolerance(scaleMultiplier * errorTolerance);

# ifndef GFC_NO_FXPLAYER_EDGEAA
            if (needEdgeAA)
            {
                // AA Width is half a pixel.
                GCoordType aaWidth = scaleMultiplier * 20.f / context.GetPixelScale();
                if (!matrixPreMultiply)
                {
                    // If no matrix multiply used (i.e. this is Normal Stroke),
                    // un-scale width by the matrix transform scale.
                    aaWidth /= resultScaleFactor;
                }

                strokerAA.SetAntiAliasWidth(aaWidth);
                strokerAA.SetSolidWidth(GTL::gmax<Float>(0.0f, width * scaleMultiplier - aaWidth ));
            }
            else
# endif
            {
                strokerAA.SetAntiAliasWidth(0.0f); // No AA.
                strokerAA.SetSolidWidth(width * scaleMultiplier);
            }

            // Generate stroke triangles.
            strokerAA.Tessellate(lineShape, Style);

////@TODO: Remove
////Test for the "long rays" erroneously produced by the stroker
//Float x1, y1, x2, y2;
//lineShape.PerceiveBounds(&x1, &y1, &x2, &y2);
//x1 -= strokerAA.GetAntiAliasWidth() + strokerAA.GetSolidWidth() * 5;
//y1 -= strokerAA.GetAntiAliasWidth() + strokerAA.GetSolidWidth() * 5;
//x2 += strokerAA.GetAntiAliasWidth() + strokerAA.GetSolidWidth() * 5;
//y2 += strokerAA.GetAntiAliasWidth() + strokerAA.GetSolidWidth() * 5;
//for(unsigned jj = 0; jj < strokerAA.GetNumVertices(); jj++)
//{
//    const GStrokerAA::VertexType& v = strokerAA.GetVertex(jj);
//    if(v.x < x1 || v.x > x2 || v.y < y1 || v.y > y2)
//    {
//        FILE* fd = fopen("stroke", "at");
//        fprintf(fd, "mc.lineStyle(%f);\n", width * scaleMultiplier / 20);
//        fprintf(fd, "mc.moveTo(%f, %f);\n", lineShape.GetVertex(0).x/20, lineShape.GetVertex(0).y/20);
//        for(unsigned kk = 1; kk < lineShape.GetNumVertices(); kk++)
//        {
//        fprintf(fd, "mc.lineTo(%f, %f);\n", lineShape.GetVertex(kk).x/20, lineShape.GetVertex(kk).y/20);
//        }
//        fclose(fd);
//        break;
//    }
//}


//===================

# ifndef GFC_NO_FXPLAYER_EDGEAA
            // Antialiased version of StrokerAA use.
            if (needEdgeAA)
            {
                // Determine the color that will be used for sides ahead of time.               
                UInt32 fillColor, outsideColor;
                UInt32 fillFactor = 0, outsideFactor = 0;
                    
                AllowCxformAddAlpha = cxformAddAlpha;

                // Currently needs swapping. TBD.
                GColor c  = style.GetColor();
                GColor nc = c;
                nc.SetBlue(c.GetRed() ); // Swap G & B
                nc.SetRed( c.GetBlue());                            
                fillColor   = nc.Raw;
                


                if (AllowCxformAddAlpha || style.HasComplexFill())
                {
                    Vertices.SetFormat(GRenderer::Vertex_XY16iCF32);

                    // In this vertex format Alpha comes from Factor.Alpha.
                    // Use fill texture everywhere.
                    if (style.HasComplexFill())
                    {
                        // Color is irrelevant, use texture channels.
                        fillFactor      = 0xFFFFFFFF;
                        outsideFactor   = 0x00FFFFFF;
                    }
                    else
                    {                           
                        // Always use colors, factor only provides alpha.
                        fillFactor      = 0xFF000000;
                        outsideFactor   = 0x00000000;
                    }

                    // No alpha masking: avoid double blending.
                    outsideColor = fillColor;
                }
                else                    
                {
                    Vertices.SetFormat(GRenderer::Vertex_XY16iC32);

                    outsideColor = fillColor & 0x00FFFFFF; // No alpha outside.
                }

                pCachedMesh->SetEdgeAAStyles(1, 0, (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None);
                pCachedMesh->SetGouraudFillType(style.HasComplexFill() ?
                                    GRenderer::GFill_1Texture : GRenderer::GFill_Color);                

                if (strokerAA.GetNumVertices() <= 0xFFFE)
                {
                    Vertices.Resize(strokerAA.GetNumVertices());

                    if (matrixPreMultiply)
                    {
                        for(j = 0; j < strokerAA.GetNumVertices(); j++)
                        {
                            const GStrokerAA::VertexType& vpt = strokerAA.GetVertex(j);
                            // Transform back
                            mInv.Transform(&pt, GPointF(vpt.x, vpt.y));
                            BoundCheckPoint(&pt, sx, sy);

                            GFxVertexRef v = Vertices.GetVertexRef(j);
                            UInt32       c = (vpt.id == -1) ? outsideColor : fillColor;
                            UInt32       f = (vpt.id == -1) ? outsideFactor : fillFactor;
                            v.InitVertex(pt.x, pt.y, c);
                            v.SetFactor(f);
                        }
                    }
                    else
                    {
                        for(j = 0; j < strokerAA.GetNumVertices(); j++)
                        {
                            const GStrokerAA::VertexType& vpt = strokerAA.GetVertex(j);

                            GFxVertexRef v = Vertices.GetVertexRef(j);
                            UInt32       c = (vpt.id == -1) ? outsideColor : fillColor;
                            UInt32       f = (vpt.id == -1) ? outsideFactor : fillFactor;
                            v.InitVertex(vpt.x, vpt.y, c);
                            v.SetFactor(f);
                        }
                    }

                // We are using EdgeAA.
                EdgeAA = 1;             
                }
                
            }
            else
# endif // #ifndef GFC_NO_FXPLAYER_EDGEAA

            // Non-antialiased StrokerAA use.
            {               
                Vertices.SetFormat(GRenderer::Vertex_XY16i);
                Vertices.Resize(strokerAA.GetNumVertices());
                // Use style 0 everywhere in stroke mesh.
                pCachedMesh->SetStyle(0);

                if (matrixPreMultiply)
                {
                    for(j = 0; j < strokerAA.GetNumVertices(); j++)
                    {
                        const GStrokerAA::VertexType& vpt = strokerAA.GetVertex(j);
                        // Transform back
                        mInv.Transform(&pt, GPointF(vpt.x, vpt.y));
                        
                        BoundCheckPoint(&pt, sx, sy);
                        Vertices.GetVertexRef(j).InitVertex(pt.x, pt.y);
                    }
                }
                else
                {
                    for(j = 0; j < strokerAA.GetNumVertices(); j++)
                    {
                        const GStrokerAA::VertexType& vpt = strokerAA.GetVertex(j);
                        Vertices.GetVertexRef(j).InitVertex(vpt.x, vpt.y);
                    }
                }
            }


            // Copy the triangles into the mesh.
            if (Vertices.GetSize())
            {
                for(j = 0; j < strokerAA.GetNumTriangles(); j++)
                {
                    const GStrokerAA::TriangleType& t = strokerAA.GetTriangle(j);
                    pCachedMesh->AddTriangle(UInt16(t.v1), UInt16(t.v2), UInt16(t.v3));
                }
            }

            // Tess Statistics.
            context.AddTessTriangles(pCachedMesh->GetTriangleCount());            
        }


        else // if (!useStrokerAA)
        {
            GTessellator      strokeTess;
            GStroker          stroker;
            GCompoundShape    strokeShape;
              
            // If we use AA_OuterEdges we need to modify stroke width; 
            GCoordType edgeAAWidth = scaleMultiplier * -10.f / context.GetPixelScale();
            if (!matrixPreMultiply)
            {
                // If no matrix multiply used (i.e. this is Normal Stroke),
                // un-scale width by the matrix transform scale.
                edgeAAWidth /= resultScaleFactor;
            }
            // *** Tessellate and render as stroke

            GCoordType strokeWidth = width * scaleMultiplier;
#ifndef GFC_NO_FXPLAYER_EDGEAA
            if (needEdgeAA)
            {
                strokeWidth += edgeAAWidth * 2;
                if (strokeWidth < 1) 
                    strokeWidth = 1;
            }
#endif

            stroker.SetLineJoin(GFx_SWFToFxStroke_LineJoin(style.GetJoin()));
            stroker.SetStartLineCap(GFx_SWFToFxStroke_LineCap(style.GetStartCap()));
            stroker.SetEndLineCap(GFx_SWFToFxStroke_LineCap(style.GetEndCap()));
            stroker.SetWidth(strokeWidth);
            // Miter size is 1/2 the multiple of width, so pass it directly with no multipliers.
            // NOTE: Stroker does not like non-default miter limits if we are not doing MiterJoin.
            if (style.GetJoin() == GFxLineStyle::LineJoin_Miter)
                stroker.SetMiterLimit(style.GetMiterSize());

            strokeShape.SetCurveTolerance(scaleMultiplier * errorTolerance);
            strokeShape.RemoveAll();
            stroker.GenerateStroke(lineShape, Style, strokeShape);

            //// Test stroke. This thing just draws the stroke outline.
            //for(UInt k = 0; k < strokeShape.GetNumPaths(); k++)
            //{
            //    GFxCachedStroke lineStrip;
            //    lineStrip.SetShapePathVertices(strokeShape, k, 1.0f);
            //    lineStrip.Display(prenderer, lineStyles[style], 1.0f);
            //}


            // *** Triangulation of the stroke multi-path

            // Setup for optimal stroke triangulation
            strokeTess.SetFillRule(GTessellator::FillNonZero);

            // It all became redundant with Tessellator v2
            //strokeTess.SetReduceWinding(true);
            //strokeTess.SetProcessJunctions(false);
            //strokeTess.SetPreserveStyleCoherence(false);
            //strokeTess.SetRoundOffScale(1/16.0f); // [fixes bug in scale, does round-off]

            bool optFlag = rconfig.IsOptimizingTriangles();
            strokeTess.SetDirOptimization(true);
            strokeTess.SetCoherenceOptimization(optFlag);
            strokeTess.SetMonotoneOptimization(optFlag);
            strokeTess.SetPilotTriangulation(optFlag);
            strokeTess.Monotonize(strokeShape, 1);


    # ifdef GFX_GENERATE_STROKE_SHAPE_FILE
            {
                FILE* fd = fopen("strokes", "at");
                fprintf(fd, "=======BeginShape\n");
                unsigned i, j;
                for(i = 0; i < strokeShape.GetNumPaths(); i++)
                {
                    const GCompoundShape::SPath& path = strokeShape.GetPath(i);
                    const GPointType& p0 = path.GetVertex(0);
                    fprintf(fd, "Path 0 -1 -1 %f %f\n", p0.x, p0.y);
                    for(j = 1; j < path.GetNumVertices(); j++)
                    {
                        const GPointType& p = path.GetVertex(j);
                        fprintf(fd, "Line %f %f\n", p.x, p.y);
                    }
                    fprintf(fd, "<-------EndPath\n");
                }
                fprintf(fd, "!======EndShape\n");
                fclose(fd);
            }
    # endif

            

            if (strokeTess.GetNumVertices())
            {
                // If renderer cached vertices, we must release them.
                VerticesCache.ReleaseData(GRenderer::Cached_Vertex);

# ifndef GFC_NO_FXPLAYER_EDGEAA
                // EdgeAA starts out at 0, in case it fails.
                // TBD: Do we need to check the renderer for caps?
                EdgeAA = 0;

                // Edge AA enabled
                if (needEdgeAA)
                {
                    // Hack to assign correct color channel in shapes.              
                    GTL::garray<GFxFillStyle> strokeFillStyles;
                    
                    if (style.HasComplexFill())
                        strokeFillStyles.push_back(*style.GetComplexFill());
                    else
                    {
                        
                        GFxFillStyle strokeFillStyle;
                        strokeFillStyle.SetColor(style.GetColor());
                        strokeFillStyles.push_back(strokeFillStyle);
                    }

                    GFxEdgeAAGenerator aaGen(strokeTess, &strokeFillStyles[0], strokeFillStyles.size());

                    // Always mix texture for gradient stroke.
                    if (style.HasComplexFill())
                        aaGen.SetDefaultFactorRGB(0x00FFFFFF);

                    aaGen.ProcessAndSortEdges(edgeAAWidth, GEdgeAA::AA_OuterEdges);

                    // If over 65K vertices limit, can't use AA for now.
                    if (aaGen.GetVertexCount() <= 0xFFFE)
                    {                   
                        bool edgeGenSuccess = 0;

                        AllowCxformAddAlpha = cxformAddAlpha;
                        
                        if (AllowCxformAddAlpha || style.HasComplexFill())
                            Vertices.SetFormat(GRenderer::Vertex_XY16iCF32);
                        else                    
                            Vertices.SetFormat(GRenderer::Vertex_XY16iC32);


                        if (matrixPreMultiply)
                            edgeGenSuccess = aaGen.GenerateSolidMesh(&Vertices, pCachedMesh, mInv);
                        else
                            edgeGenSuccess = aaGen.GenerateSolidMesh(&Vertices, pCachedMesh, 1.0f);

                        // Tess Statistics.
                        context.AddTessTriangles(pCachedMesh->GetTriangleCount());


                        // Done, use AA.
                        if (edgeGenSuccess)
                        {
                            EdgeAA = 1;
                            // Set edgeAA style, so that rendering actually applies the texture.
                            if (style.HasComplexFill())
                            {
                                pCachedMesh->SetEdgeAAStyles(1, 0, (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None);
                                pCachedMesh->SetGouraudFillType(GRenderer::GFill_1Texture);
                            }
                        }
                    }
                }

# endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
                
                if (!IsUsingEdgeAA())
                {   
                    Vertices.SetFormat(GRenderer::Vertex_XY16i);
                    Vertices.Resize(strokeTess.GetNumVertices());

                    if (matrixPreMultiply)
                    {
                        for(j = 0; j < strokeTess.GetNumVertices(); j++)
                        {
                            const GPointType& vpt = strokeTess.GetVertex(j);
                            // Transform back
                            mInv.Transform(&pt, GPointF(vpt.x, vpt.y));
                            
                            BoundCheckPoint(&pt, sx, sy);
                            Vertices.GetVertexRef(j).InitVertex(pt.x, pt.y);
                        }
                    }
                    else
                    {
                        for(j = 0; j < strokeTess.GetNumVertices(); j++)
                        {
                            const GPointType& vpt = strokeTess.GetVertex(j);

                            Vertices.GetVertexRef(j).InitVertex(vpt.x, vpt.y);
                        }
                    }


                    // We have to create it here because it accumulates the triangles.
                    for(j = 0; j < strokeTess.GetNumMonotones(); j++)
                    {
                        strokeTess.TriangulateMonotone(j);
                        pCachedMesh->AddTriangles(0, strokeTess); // Use a fake style, 0
                    }

                    // Tess Statistics.
                    context.AddTessTriangles(pCachedMesh->GetTriangleCount());

                } // if (!EdgeAA)

            } // if (strokeTess.GetNumVertices())


        } // else if (!useStrokerAA)

        

        // Update cache matching constants.
        UsedStrokerAA           = useStrokerAA;
        CachedWidth             = width;
        CachedScaleFlag         = matrixPreMultiply;
        CachedStrokeMatrixSx    = sx;
        CachedStrokeMatrixSy    = sy;
        AllowCxformAddAlpha     = cxformAddAlpha;
    }


    // Draw the mesh.
    if (Vertices.GetSize())
    {
        GRenderer::CacheProvider cp(&VerticesCache);
        Vertices.ApplyToRenderer(rconfig.GetRenderer(), &cp);

        // Handle complex fill styles.
        GFxFillStyle    strokeFillStyle;
        GFxFillStyle*   pfill = 0;
        UInt            fillsSize = 0;
        Float           multiplier = 1.0f;

        if (style.HasComplexFill())
        {
            pfill       = style.GetComplexFill();
            fillsSize   = 1;
            // Note: scaleMultiplier only applies to fills; it needs to be correct so that
            // complex fills for stroke show up right.
            multiplier  = 1.0f / scaleMultiplier;
        }
        else if (!IsUsingEdgeAA())
        {
            pfill       = &strokeFillStyle;
            fillsSize   = 1;
            strokeFillStyle.SetColor(style.GetColor());
        }
        else
        {
            // Note that at this point null pfill is ok, because it would indicate
            // that colors come from vertices, as expected for EdgeAA. If that is
            // so vertex format can NOT be Vertex_XY16i.
            GASSERT(Vertices.GetFormat() != GRenderer::Vertex_XY16i);
        }
        
        pCachedMesh->Display(context, pfill, fillsSize,
                             0, Vertices.GetSize(), 
                             multiplier, (const GRenderer::Matrix*)0, 
                             IsUsingEdgeAA());
    }

    return;

    // We get here if we only render hairline.
    render_hairline_stroke: ;

#endif // GFC_NO_FXPLAYER_STROKER

    // Draw line strips for hairline.
    style.Apply(rconfig.GetRenderer());
    UInt        j, index;

    GRenderer::CacheProvider cp(&CoordsCache);
    rconfig.GetRenderer()->SetVertexData(&Coords[0], (int)(Coords.size() / 2), GRenderer::Vertex_XY16i, &cp);

    for(index = 0, j = 0; j<CoordCounts.size(); j++)
    {
        rconfig.GetRenderer()->DrawLineStrip(index >> 1, (CoordCounts[j] >> 1) - 1);
        index += CoordCounts[j];
    }
}


#ifndef GFC_NO_FXPLAYER_STROKER

// Do a bounds check/clamp on a point, displaying warning.
void    GFxCachedStroke::BoundCheckPoint(GPointF *ppt, Float sx, Float sy)
{
    GPointF &pt = *ppt;

#ifdef GFC_BUILD_DEBUG
    if ((pt.x > 32767.0f) || (pt.y > 32767.0f) || (pt.y < -32768.0f) || (pt.x < -32768.0f))
    {
        if (!RangeOverflowWarned)
        {
            GFC_DEBUG_WARNING4(1,
                    "Stroker target range overflow: (x,y) = (%g,%g). (Sx,Sy) = (%g,%g)",
                    pt.x, pt.y, sx, sy);
            RangeOverflowWarned = 1;
        }
        // This clamping should be faster for debug
        if (pt.x > 32767.0f)    pt.x = 32767.0f;
        if (pt.x < -32768.0f)   pt.x = -32768.0f;
        if (pt.y > 32767.0f)    pt.y = 32767.0f;
        if (pt.y < -32768.0f)   pt.y = -32768.0f;
    }

#else
    GUNUSED2(sx,sy);
    // Safety clamping for the case above. CPU should have an fabsf() op.
    if (fabsf(pt.x) > 32767.0f)
        pt.x = (pt.x > 32767.0f) ? 32767.0f : -32768.0f;
    if (fabsf(pt.y) > 32767.0f)
        pt.y = (pt.y > 32767.0f) ? 32767.0f : -32768.0f;
#endif
}

#endif // #ifndef GFC_NO_FXPLAYER_STROKER


// Dump our data to *out.
void    GFxCachedStroke::OutputCachedData(GFile* out)
{
    out->WriteSInt32(Style);
    GFx_WriteCoordArray(out, Coords);
}

// Slurp our data from *out.
void    GFxCachedStroke::InputCachedData(GFile* in)
{
    Style = in->ReadSInt32();
    GFx_ReadCoordArray(in, &Coords);
}



//
// ***** GFxMeshSet
//

// Tessellate the shape's paths into a different GFxMesh for each fill style.
GFxMeshSet::GFxMeshSet(Float screenPixelSize, Float curveError, bool aaDisabled, bool optTriangles)
{
    ScreenPixelSize     = screenPixelSize;
    PixelCurveError     = curveError;
    ScaleMultiplier     = 1.0f;
    ScaleMultiplierInv  = 1.0f;
    EdgeAADisabled      = aaDisabled;
#ifndef GFC_NO_FXPLAYER_EDGEAA
    EdgeAA              = 0;
    EdgeAAFailed        = 0;
    AllowCxformAddAlpha = 0;    
#endif
    OptimizeTriangles   = optTriangles;
    Scale9GridKey       = 0;
    pInstance           = 0;
    pTexture9Grids      = 0;
    pImgAdjustMatrices  = 0;
}

GFxMeshSet::~GFxMeshSet() 
{ 
    VerticesCache.ReleaseData(GRenderer::Cached_Vertex); 
    if (Scale9GridKey)
        GFREE(Scale9GridKey);
    delete pTexture9Grids;
    delete pImgAdjustMatrices;
}


// Throw our meshes at the renderer.
void    GFxMeshSet::Display(GFxDisplayParams& params, bool scale9Grid)

{
    GASSERT(PixelCurveError > 0);

    GRenderer* prenderer = params.Context.GetRenderer();

#if defined _XBOX

	GRenderer::Matrix m;
	XMVECTOR scale = { { ScaleMultiplierInv, ScaleMultiplierInv, 1, 0 } };
	m.r[0] = XMVectorMultiply( scale , params.Mat.r[0] );
	m.r[1] = XMVectorMultiply( scale , params.Mat.r[1] );
#else

    // Setup transforms.
    GRenderer::Matrix m(params.Mat);
    m.PrependScaling(ScaleMultiplierInv);
#endif

    prenderer->SetCxform(params.Cx);

    // Determine if blendMode/cxform can cause forced blending.
    bool    canBlend = 0;
    if (params.Blend > GRenderer::Blend_Layer) 
        canBlend = 1;
    else if (params.Cx.M_[3][0] < 0.997f)      
        canBlend = 1;   


    UInt    ishape, imesh, istrip;
    bool    needMeshData = 1;

    // 5.0 is quarter of a pixel
    Float strokeOffset = 5.0f / params.Context.GetPixelScale();
    for (ishape = 0, imesh=0, istrip=0 ; ishape < Shapes.size(); ishape++)
    {
        UInt i;
        if (Shapes[ishape].Texture9GridIdx != ~0U)
        {
            prenderer->SetMatrix(m);
            const GFxTexture9Grid& t9g = (*pTexture9Grids)[Shapes[ishape].Texture9GridIdx];
            GASSERT(params.pFillStyles && t9g.FillStyleIdx < params.FillStylesNum);
            t9g.Display(params.Context, params.pFillStyles[t9g.FillStyleIdx]);
            needMeshData = 1;
        }
        else
        {
            if (needMeshData)
            {
                GRenderer::CacheProvider cp(&VerticesCache);

                if (Vertices.GetSize())
                    Vertices.ApplyToRenderer(prenderer, &cp);
                else if (cp.GetCachedData(prenderer))
                    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None, &cp);

                prenderer->SetMatrix(m);
                needMeshData = 0;

                // Can technically destroy buffer if this is returned.
                //if (cp.CanDiscardData())
                //{
                    // Vertices.clear();
                    // Set flag
                //}
            }

            const GRenderer::Matrix* imgAdjust = 0;
            if (pImgAdjustMatrices)
                imgAdjust = &(*pImgAdjustMatrices)[0];


            // Dump meshes into renderer, one GFxMesh per style.
            for (i = 0; (i < Shapes[ishape].MeshCount) && (imesh < Meshes.size()); i++, imesh++)
            {

                Meshes[imesh].Display(params.Context, params.FillStylesNum ? &params.pFillStyles[0] : 0, 
                                      (UInt)params.FillStylesNum,
                                      Shapes[ishape].VertexStartIndex, Shapes[ishape].VertexCount,
                                      ScaleMultiplierInv, imgAdjust,
                                      IsUsingEdgeAA());
            }

            // Dump strokes/line-strips into renderer.
            if (Shapes[ishape].StrokeCount)
            {                        
                // HACK: Quarter pixel translate for strokes.
                // Flash studio puts all lines at pixel edges by default, which is wrong
                // because it would cause them to be blurry. To adjusts for that, it snaps
                // horizontal and vertical lines "up", causing incorrect shapes.
                // Since we don't do that, adjust lines forward by quarter pixel.
                // (No adjustment could would cause lines to disappear, while 0.5 adjustment
                //  may produce sand and bleed-through on the bottom-right of shapes).
                GRenderer::Matrix strokeMat(m);  
                strokeMat.M_[0][2] += strokeOffset;
                strokeMat.M_[1][2] += strokeOffset;
                prenderer->SetMatrix(strokeMat);

                for (i = 0; (i < Shapes[ishape].StrokeCount) && (istrip < Strokes.size()); i++, istrip++)
                {
                    UInt style = Strokes[istrip].GetStyle();
                    GASSERT(params.pLineStyles && style < params.LineStylesNum);
                    Strokes[istrip].Display(params.Context, params.pLineStyles[style], params.Mat,
                                            ScaleMultiplier, PixelCurveError, GetAllowCxformAddAlpha(), canBlend,
                                            scale9Grid);
                }
                needMeshData = 1;
            }
        }
    }
    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
    prenderer->SetIndexData(0, 0, GRenderer::Index_None);
}



// Hints at bounds, before adding any shapes.
// Can be used to determine the scale precision multiplier.
void    GFxMeshSet::SetShapeBounds(const GRectF &r, Float strokeExtent)
{
    // Determine the maximum value of rectangle scale.
    Float maxf = GTL::gmax( GTL::gmax(GTL::gabs(r.Left), GTL::gabs(r.Right)),
                            GTL::gmax(GTL::gabs(r.Top), GTL::gabs(r.Bottom)) );

    // Convert scale [0.0f - maxf] to [0.0f - 32767]
    // Avoid divide by zero
    if (maxf < 0.01f)
        ScaleMultiplier = 1.0f;
    else
    {
        // Correct: maxf += 2.0f + strokeExtent;

        // MA - TBD
        // NOTE: strokeExtent*2 is a fudge multiplier (should not be here)
        // The reason for it is when the object scales disproportionately, while
        // generating a correct stroke, inverse-transformed coordinates may produce
        // a very wide stroke, which will go out of bounds in the *other* axis coordinate.
        // A correct solution would be to use FP, to not do the inverse transform on
        // stroke, or handle stroke separately from shape.

        // MA: Should GetIntersectionMiterLimit be multiplied by width? What is it then?
        Float edgeAAAdjust = 0.0f;
#ifndef GFC_NO_FXPLAYER_EDGEAA
        edgeAAAdjust = GEdgeAA::GetIntersectionMiterLimit()*2;
#endif

        maxf += GTL::gmax(2000.0f, 50.0f + strokeExtent * 2 + edgeAAAdjust);
        ScaleMultiplier = 32766.0f / maxf;
    }

    ScaleMultiplierInv = 1.0f / ScaleMultiplier;
}


void  GFxMeshSet::AddTessellatedShape(const GCompoundShape &shape, 
                                      const GFxFillStyle* pfills,
                                      UPInt fillsNum,
                                      GFxDisplayContext* context, 
                                      const GFxRenderConfig& rconfig)
{
    UInt            i, j;
    GTessellator    tess;
    // Sub-shape we will produce.
    MeshSubShape    subShape;

    subShape.Texture9GridIdx = ~0U;

    // *** Tessellate a shape.

    // Setup the tessellator for shape triangulation
    tess.SetFillRule(shape.IsNonZeroFill() ? GTessellator::FillNonZero : GTessellator::FillEvenOdd);

    // It all became redundant with Tessellator v2
    //tess.SetReduceWinding(false);
    //tess.SetProcessJunctions(false);
    //tess.SetPreserveStyleCoherence(false);

    // Setup optimization
    tess.SetDirOptimization(true);
    tess.SetCoherenceOptimization(OptimizeTriangles);
    tess.SetMonotoneOptimization(OptimizeTriangles);
    tess.SetPilotTriangulation(OptimizeTriangles);

    // Add 1 to styles to conform to the existing policy. 
    // The tessellator uses zero-based styles, while the policy 
    // is to use minus-one-besed.
    tess.Monotonize(shape, 1);

    // Right now, 65K limit
    if (tess.GetNumVertices() > 0xFFFE)
    {
        GFC_DEBUG_WARNING(1, "GFxMeshSet::AddTessellatedShape failes - more then 65K vertices generated");
        return;
    }


    // *** Generate and Add Meshes

    // Sort the monotone pieces by their style
    // in order to reduce the number of switches between colors
    tess.SortMonotonesByStyle();

#ifdef GFC_NO_FXPLAYER_EDGEAA
    GUNUSED3(pfills, fillsNum, rconfig);
#else
    bool useEdgeAA = 0; 
    bool hasVF_CF32 = rconfig.HasVertexFormat(GRenderer::Vertex_XY16iCF32);

    if (rconfig.IsUsingEdgeAA() && !EdgeAADisabled)
    {
        useEdgeAA = 1;
        if (AllowCxformAddAlpha && rconfig.HasCxformAddAlpha() && !hasVF_CF32)
            useEdgeAA = 0;
        else if (!rconfig.IsEdgeAATextured())
            for (i = 0; i < fillsNum; i++)
                if (pfills[i].GetType() != GFxFill_Solid)
                {
                    useEdgeAA = 0;
                    break;
                }
    }

    if (useEdgeAA)
    {
        useEdgeAA = 0;

        // Try to generate edge AA
        GFxEdgeAAGenerator aaGen(tess, pfills, fillsNum);
        GFxVertexArray     newVertices;
        UInt               meshCount = 0;       

        aaGen.ProcessAndSortEdges(ScreenPixelSize * -0.5f, GEdgeAA::AA_AllEdges);

        GASSERT(rconfig.IsEdgeAATextured() || !aaGen.IsUsingTextures());

        // If over 65K vertices limit, can't use AA for now.
        // Also textured EdgeAA must be supported by renderer.
        if  ((Vertices.GetSize() + aaGen.GetVertexCount()) <= 0xFFFE)
        {           
            if (Vertices.GetFormat() != GRenderer::Vertex_None)
            {
                newVertices.SetFormat(Vertices.GetFormat());
            }
            else            
            {
                // If previous Vertices are empty, determine buffer format.
                GRenderer::VertexFormat vf = GRenderer::Vertex_XY16iC32; 

                if (hasVF_CF32)
                {
                    if (AllowCxformAddAlpha)
                        vf = GRenderer::Vertex_XY16iCF32;
                    else
                    {
                        for (UPInt i = 0; i < fillsNum; i++)
                            if (pfills[i].GetType() != 0)
                            {
                                vf = GRenderer::Vertex_XY16iCF32;
                                break;
                            }
                    }

                    // If in larger format, we support Add-Alpha by default.
                    if (vf == GRenderer::Vertex_XY16iCF32)
                        AllowCxformAddAlpha = 1;
                }
                newVertices.SetFormat(vf);
            }

            // Generate meshes and vertices.    
            if (!aaGen.IsUsingTextures() || (newVertices.GetFormat() == GRenderer::Vertex_XY16iC32))
            {
                // Will always have solid vertices only.
                Meshes.resize(Meshes.size()+1);
                meshCount++;
                aaGen.GenerateSolidMesh(&newVertices, &Meshes.back(), ScaleMultiplier);             
            }
            else
            {
                GASSERT(newVertices.GetFormat() == GRenderer::Vertex_XY16iCF32);
                // Can have solid or textured vertices. Both must go into a CF buffer.          
                meshCount = aaGen.GenerateTexturedMeshes(&newVertices, &Meshes, ScaleMultiplier);
            }

            if (context)
            {
                // Tess statistics.
                UInt count = 0;
                for (count =0; count < meshCount; count++)
                    context->AddTessTriangles(Meshes[Meshes.size()-1-count].GetTriangleCount());
            }
                        
            // If new vertices can't use AA for now, restore meshes to original size.
            if (Vertices.GetSize() + newVertices.GetSize() > 0xFFFE)
            {                   
                // Didn't fit in, restore to prev state.
                Meshes.resize(Meshes.size() - meshCount);
            }
            else
            {               
                // Size ok, add extra vertices.
                subShape.VertexStartIndex   = Vertices.GetSize();
                subShape.VertexCount        = newVertices.GetSize();
                subShape.MeshCount          = meshCount;
                
                if (Vertices.GetFormat() == GRenderer::Vertex_None)
                    Vertices.SetFormat(newVertices.GetFormat());
                Vertices.AppendVertices(newVertices);
                // Done, use AA.
                useEdgeAA = 1;
            }
        }
        else
        {
            EdgeAAFailed = 1;
        }
    }

    if (useEdgeAA)
        EdgeAA = 1;

#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
    
    // If we did not use EdgeAA, generate normal meshes
    if (!IsUsingEdgeAA())
    {       
        subShape.VertexStartIndex   = Vertices.GetSize();
        subShape.VertexCount        = tess.GetNumVertices();

        if (Vertices.GetFormat() == GRenderer::Vertex_None)
            Vertices.SetFormat(GRenderer::Vertex_XY16i);
        GASSERT(Vertices.GetFormat() == GRenderer::Vertex_XY16i);

        Vertices.Resize(subShape.VertexStartIndex + subShape.VertexCount);

        if (Vertices.GetSize() == subShape.VertexStartIndex + subShape.VertexCount)
        {
            for (i=0, j = subShape.VertexStartIndex; i<subShape.VertexCount; i++, j++)
            {
                const GPointType& pt = tess.GetVertex(i);
                Vertices.GetVertexRef(j).InitVertex(pt.x * ScaleMultiplier, pt.y * ScaleMultiplier);
            }
        }
        else
        {
            return;
        }

        // The main loop of emitting the triangles. Iterate
        // through the monotone pieces and emit triangles.
        for(i = 0; i < tess.GetNumMonotones(); i++)
        {
            // Get the info about the i-th monotone.
            // Well, basically m.style is all you need.
            const GTessellator::MonotoneType& m = tess.GetMonotone(i);

            // The tessellator uses zero-based styles, so, make it minus-one-based
            // to conform to the existing policy.
            int style = (int)m.style - 1;

            // Triangulate the i-th monotone polygon. After this call
            // you can iterate through the triangles for this particular monotone.
            tess.TriangulateMonotone(i);

            // If same style add to the last mesh, otherwise, to new one.
            if ((i == 0) || ((int)Meshes.back().GetStyle() != style))
            {
                Meshes.resize(Meshes.size()+1);
                subShape.MeshCount++;
            }

            Meshes.back().AddTriangles(style, tess);

            // Tess statistics.
            if (context)
                context->AddTessTriangles(tess.GetNumTriangles());
        }
    }

    
    // *** Generate Path line strips

    // Now the line part.
    GTL::garray<bool> pathsMatched;
    pathsMatched.resize(shape.GetNumPaths());
    for(i = 0; i < shape.GetNumPaths(); i++)
        pathsMatched[i] = 0;

    for(i = 0; i < shape.GetNumPaths(); i++)
    {
        if (pathsMatched[i])
            continue;
        const GCompoundShape::SPath& p = shape.GetPath(i);

        if(p.GetLineStyle() >= 0 && p.GetNumVertices() > 1)
        {
            // Add a line strip.
            Strokes.resize(Strokes.size() + 1);
            subShape.StrokeCount++;
            // Copy the vertices from the path into the strip.
            Strokes.back().SetEdgeAADisabled(EdgeAADisabled);
            Strokes.back().AddShapePathVertices(shape, i, ScaleMultiplier);

            // Here we accumulate all paths with the same style so that we can render
            // them as a single whole stroke. It's not a caprice, it's the way
            // Flash renders the strokes. That is, it combines all strokes
            // with the same style in order to correctly rasterize overlapping
            // lines. Otherwise the overlaps will be visible on translucent strokes.
            // 

            // Combine other paths with the same style.
            for(j = i+1; j < shape.GetNumPaths(); j++)
            {
                const GCompoundShape::SPath& p2 = shape.GetPath(j);
                if(p2.GetLineStyle() == p.GetLineStyle())
                {
                    // Mark so that we don't add this path again
                    pathsMatched[j] = 1;

                    if (p2.GetLineStyle() >= 0 && p2.GetNumVertices() > 1)
                    {
                        // Copy the vertices from the path into the strip.
                        Strokes.back().AddShapePathVertices(shape, j, ScaleMultiplier);
                    }
                }
            }
        }
    }

    // Add a sub-shape. This is significant to ensure that
    // separate sub-shapes are drawn on top of each other.
    Shapes.push_back(subShape);
}


void    GFxMeshSet::AddTexture9Grid(const GFxTexture9Grid& t9g)
{
    MeshSubShape subShape;

    if (pTexture9Grids == 0)
        pTexture9Grids = new GTL::garray<GFxTexture9Grid>;

    subShape.MeshCount        = 0;
    subShape.StrokeCount      = 0;
    subShape.VertexStartIndex = 0;
    subShape.VertexCount      = 0;
    subShape.Texture9GridIdx  = (UInt)pTexture9Grids->size();

    pTexture9Grids->push_back(t9g);
    Shapes.push_back(subShape);

#ifndef GFC_NO_FXPLAYER_EDGEAA
    EdgeAA = !EdgeAADisabled;
#endif
}


// Dump our data to the output GFxStream.
void    GFxMeshSet::OutputCachedData(GFile* out)
{
    out->WriteFloat(ScreenPixelSize);

    UInt    i;

    UInt    meshCount = (UInt)Meshes.size();
    out->WriteUInt32(meshCount);
    for (i = 0; i < meshCount; i++)
    {
        Meshes[i].OutputCachedData(out);
    }

    UInt    lineCount = (UInt)Strokes.size();
    out->WriteUInt32(lineCount);
    for (i = 0; i < lineCount; i++)
    {
        Strokes[i].OutputCachedData(out);
    }

    UInt    shapeCount = (UInt)Shapes.size();
    out->WriteSInt32(shapeCount);

    for (i = 0; i < shapeCount; i++)
    {
        out->WriteUInt32(Shapes[i].MeshCount);
        out->WriteUInt32(Shapes[i].StrokeCount);
    }
}


// Grab our data from the input GFxStream.
void    GFxMeshSet::InputCachedData(GFile* in)
{
    ScreenPixelSize = in->ReadFloat();

    UInt    i;
    UInt    meshCount = in->ReadUInt32();
    Meshes.resize(meshCount);
    for (i = 0; i < meshCount; i++)
    {
        Meshes[i].InputCachedData(in);
    }

    UInt    lineCount = in->ReadUInt32();
    Strokes.resize(lineCount);
    for (i = 0; i < lineCount; i++)
    {
        Strokes[i].InputCachedData(in);
    }

    UInt    shapeCount = in->ReadUInt32();
    Shapes.resize(shapeCount);
    for (i = 0; i < shapeCount; i++)
    {
        Shapes[i].MeshCount = in->ReadUInt32();
        Shapes[i].StrokeCount = in->ReadUInt32();
    }
}

void    GFxMeshSet::SetScale9GridKey(const Float* key, GFxCharacter* inst)
{
    if (Scale9GridKey == 0)
        Scale9GridKey = (Float*)GALLOC(GFxScale9GridInfo::KeySize * sizeof(Float));

    for (UInt i = 0; i < GFxScale9GridInfo::KeySize; ++i)
        Scale9GridKey[i] = key[i];

    pInstance = inst;
}

bool    GFxMeshSet::Scale9GridFits(const Float* key) const
{
    if (Scale9GridKey == 0)
        return false;

    return memcmp(Scale9GridKey, key, GFxScale9GridInfo::KeySize * sizeof(Float)) == 0;

    //for (UInt i = 0; i < GFxScale9GridInfo::KeySize; ++i)
    //    if (Scale9GridKey[i] != key[i]) 
    //        return false;
    //
    //return true;
}


void   GFxMeshSet::SetImgAdjustMatrices(const GTL::garray<GFxScale9GridInfo::ImgAdjust>& adj)
{
    if (pImgAdjustMatrices == 0)
        pImgAdjustMatrices = new GTL::garray<GRenderer::Matrix>;
    pImgAdjustMatrices->resize(adj.size());
    for (UInt i = 0; i < adj.size(); ++i)
        (*pImgAdjustMatrices)[i] = adj[i].Matrix;
}
