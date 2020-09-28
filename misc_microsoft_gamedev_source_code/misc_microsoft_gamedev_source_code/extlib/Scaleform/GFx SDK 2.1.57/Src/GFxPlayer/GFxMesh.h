/**********************************************************************

Filename    :   GFxMesh.h
Content     :   Mesh structure header for shape characters.
Created     :   July 7, 2005
Authors     :   Michael Antonov

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXMESH_H
#define INC_GFXMESH_H

#include "GFxStyles.h"
#include "GFxVertex.h"
#include "GStroker.h"
#include "GStrokerAA.h"
#include "GFxScale9Grid.h"
#include "GFxDisplayContext.h"


// ***** Declared Classes
class GFxMesh;
class GFxCachedStroke;
class GFxMeshSet;

// ***** External Classes
class GFxCharacter;
class GFxStream;
class GFxShapeCharacterDef;
class GCompoundShape;
class GTessellator;
struct GFxDisplayParams;


// ***** GFxMesh


// For holding a pre-tessellated shape.
class GFxMesh : public GNewOverrideBase
{
public:
    GFxMesh();

    GINLINE ~GFxMesh()
    {
        TriangleIndicesCache.ReleaseData(GRenderer::Cached_Index);
    }

    // Note: subsequent styles must be the same
    void    AddTriangles(UInt style, const GTessellator &sourceTess);

#ifndef GFC_NO_FXPLAYER_EDGEAA
    // Sets the styles
    void    SetEdgeAAStyles(UInt count, UInt style0, UInt style1, UInt style2);
    void    SetGouraudFillType(GRenderer::GouraudFillType gft);
#endif

    void    AddTriangle(UInt16 v1, UInt16 v2, UInt16 v3)
    {
        TriangleIndices.push_back(v1);
        TriangleIndices.push_back(v2);
        TriangleIndices.push_back(v3);

        //UInt16* p = &TriangleIndices[triIndex*3];
        //p[0] = v1; p[1] = v2; p[2] = v3;  
    }

    // Resets the mesh.
    void    Clear();

    // Display the mesh using specified fill style table
    void    Display(const GFxDisplayContext& dc,
                    const GFxFillStyle* pfills, UInt fillsSize,
                    UInt vertexStartIndex, UInt vertexCount, Float scaleMultiplier,
                    const GRenderer::Matrix* imgAdjustMatrices,
                    bool useEdgeAA);

    UInt    GetTriangleCount() const { return (UInt)(TriangleIndices.size() /3); }

    UInt    GetStyle() const            { return Styles[0]; }
    void    SetStyle(UInt style)        { Styles[0] = style; StyleCount = 1; }

    void    OutputCachedData(GFile* out);
    void    InputCachedData(GFile* in);
private:


    // Fill styles used for this mesh. 
    // For standard case only Styles[0] is normally used to determine the fill.
    // For EdgeAA, since colored vertices are used, StyleCount can be 0. It can also
    // be up to 3, controlling how texture vertices are assigned. Generally,
    // StyleCount can be determined based on FillType.
    UInt                StyleCount;

#ifdef GFC_NO_FXPLAYER_EDGEAA
    UInt                Styles[1];
#else   
    UInt                Styles[3];  
    // Shading mode that should be used for EdgeAA
    GRenderer::GouraudFillType  FillType;
#endif

    
    // Point index list for triangles; size must be a multiple of 3
    GTL::garray<UInt16>     TriangleIndices;
    GRenderer::CachedData   TriangleIndicesCache;
};


// Cached stroke holds a line-strip or a stroke, often calculated based
// on a specified transform matrix.

class GFxCachedStroke : public GNewOverrideBase
{
public:
    GFxCachedStroke();
    ~GFxCachedStroke();

    typedef GRenderer::Matrix Matrix;

    
    // Copies the path vertices from a compound shape into the line strip.
    // Calling this multiple times will add separate paths that are meant to be combined into a shape.
    void    AddShapePathVertices(const GCompoundShape& shape, UInt pathIndex, Float scaleMultiplier);

    
    UInt    GetStyle() const { return Style; }
    void    OutputCachedData(GFile* out);
    void    InputCachedData(GFile* in);

#ifndef GFC_NO_FXPLAYER_STROKER
    void    SetEdgeAADisabled(bool edgeAADisabled) {  EdgeAADisabled = edgeAADisabled; }
#else
    void    SetEdgeAADisabled(bool edgeAADisabled) { GUNUSED(edgeAADisabled); }
#endif
    

#if !defined(GFC_NO_FXPLAYER_EDGEAA) && !defined(GFC_NO_FXPLAYER_STROKER)
    bool    IsUsingEdgeAA() const   { return EdgeAA; }
#else
    bool    IsUsingEdgeAA() const   { return 0; }
#endif

    
    // Rendering.
    void    Display(GFxDisplayContext &context, const GFxLineStyle& style,
                    const Matrix &mat, Float scaleMultiplier, Float errorTolerance,
                    bool cxformAddAlpha, bool canBlend, bool scale9Grid);

private:

    SInt                    Style;
    GTL::garray<SInt16>     Coords;
    GRenderer::CachedData   CoordsCache;
    GTL::garray<UInt>       CoordCounts;    // Number of coordinates in each path.

#ifndef GFC_NO_FXPLAYER_STROKER

    // Stroke width value we have cached in a mesh
    GFxMesh*            pCachedMesh;    
    Float               CachedWidth;

    // Set when this shape will be used for a mask (No EdgeAA necessary)
    bool                EdgeAADisabled;
#ifndef GFC_NO_FXPLAYER_EDGEAA
    // If set, ComplexVertices are used instead of 
    bool                EdgeAA;
    bool                AllowCxformAddAlpha;
#endif
    bool                UsedStrokerAA;

    // Zero-translation matrix applied to the stroke?
    // Unless using anisotropic scaling, stroke would need to be re-computed if other parameters change 
    bool                CachedScaleFlag;
    Float               CachedStrokeMatrixSx;
    Float               CachedStrokeMatrixSy;
    
    GFxVertexArray          Vertices;   
    GRenderer::CachedData   VerticesCache;

#ifdef GFC_BUILD_DEBUG
    bool                RangeOverflowWarned;    
#endif

    // Do a bounds check/clamp on a point, displaying warning.
    void                BoundCheckPoint(GPointF *ppt, Float sx, Float sy);

#endif
    
};

// A whole shape, tessellated to a certain error tolerance.
class GFxMeshSet : public GNewOverrideBase
{
public:
    GFxMeshSet(Float screenPixelSize = 20.0f, 
               Float curveError = 20.0f * 0.75, 
               bool edgeAADisabled = 0,
               bool optTriangles = true);
    virtual ~GFxMeshSet();

    Float   GetCurveError() const               { return PixelCurveError; }
    Float   GetScreenPixelSize() const          { return ScreenPixelSize; }

    // Return EdgeAA use, necessary for cache matching on renderer EdgeAA flag change
#ifndef GFC_NO_FXPLAYER_EDGEAA
    bool    IsUsingEdgeAA() const               { return EdgeAA; }  
    void    SetAllowCxformAddAlpha(bool allow)  { AllowCxformAddAlpha = allow; }
    bool    GetAllowCxformAddAlpha() const      { return AllowCxformAddAlpha; }
    bool    HasEdgeAAFailed() const             { return EdgeAAFailed; }
#else
    bool    IsUsingEdgeAA() const               { return 0; }   
    bool    GetAllowCxformAddAlpha() const      { return 1; }
    bool    HasEdgeAAFailed() const             { return 0; }
#endif
    bool    IsOptimizingTriangles() const       { return OptimizeTriangles; }

    void    Display(GFxDisplayParams& params, bool scale9Grid);
    
    // Adds a shape and tessellates it.
    void    AddTessellatedShape(const GCompoundShape &shape,
                                const GFxFillStyle* pfills,
                                UPInt fillsNum,
                                GFxDisplayContext* context, 
                                const GFxRenderConfig& rconfig);

    void    AddTessellatedShape(const GCompoundShape &shape,
                                const GTL::garray<GFxFillStyle> &fills,
                                GFxDisplayContext* context, 
                                const GFxRenderConfig& rconfig)
    {
        UPInt sz = fills.size();
        AddTessellatedShape(shape, (sz > 0) ? &fills[0] : NULL, sz, context, rconfig);
    }

    void    AddTessellatedShape(const GCompoundShape &shape,
                                const GFxFillStyle* pfills,
                                UPInt fillsNum,
                                GFxDisplayContext &context)
    {
        AddTessellatedShape(shape, pfills, fillsNum, &context, *context.GetRenderConfig());
    }
   
    void    AddTessellatedShape(const GCompoundShape &shape,
                                const GTL::garray<GFxFillStyle> &fills,
                                GFxDisplayContext &context)
    {
        UPInt sz = fills.size();
        AddTessellatedShape(shape, (sz > 0) ? &fills[0] : NULL, sz, context);
    }

    // Hints at bounds, before adding any shapes.
    // Can be used to determine the scale precision multiplier.
    void    SetShapeBounds(const GRectF &r, Float strokeExtent);

    // Adds a prepared texture 9-grid
    void            AddTexture9Grid(const GFxTexture9Grid& t9g);

    // Sets the image adjustment matrices for scale9grid
    void            SetImgAdjustMatrices(const GTL::garray<GFxScale9GridInfo::ImgAdjust>& adj);

    // I/O
    void    OutputCachedData(GFile* out);
    void    InputCachedData(GFile* in);

    void            SetScale9GridKey(const Float* key, GFxCharacter* inst);
    bool            Scale9GridFits(const Float* key) const;
    void            SetCharacterInstance(GFxCharacter* inst) { pInstance = inst; }
    GFxCharacter*   GetCharacterInstance() const { return pInstance; }

    Float           GetScaleMultiplier()    const { return ScaleMultiplier; }
    Float           GetScaleMultiplierInv() const { return ScaleMultiplierInv; }

private:
//      int LastFrameRendered;  // @@ we shouldn't spontaneously drop cached data I don't think...
    
    // Size of screen pixel in shape coordinates.
    Float                           ScreenPixelSize;
    Float                           PixelCurveError;

    Float                           ScaleMultiplier;
    Float                           ScaleMultiplierInv;

    // Set when this shape will be used for a mask (No EdgeAA necessary)
    bool                            EdgeAADisabled;

#ifndef GFC_NO_FXPLAYER_EDGEAA
    // Set if we are using EdgeAA vertices instead of regular.
    bool                            EdgeAA;
    bool                            EdgeAAFailed;
    // Set if CxForm Alpha of non-zero is supported. This means that 8-byte EdgeAA buffers can not be used.
    bool                            AllowCxformAddAlpha;
#endif
    bool                            OptimizeTriangles;

    GTL::garray<GFxMesh>            Meshes; // One mesh per style.
    GTL::garray<GFxCachedStroke>    Strokes;


    typedef GRenderer::VertexXY16iCF32 VertexXY16iCF32;

    // Vertices shared by all meshes
    GFxVertexArray              Vertices;   
    GRenderer::CachedData       VerticesCache;

    // There can be multiple sub-shapes in a mesh, which are
    // drawn in an overlapping Z-order. This array describes how many elements
    // belong to each such piece.
    struct MeshSubShape
    {
        UInt    MeshCount;
        UInt    StrokeCount;
        UInt    VertexStartIndex;   // First index in Vertices
        UInt    VertexCount;        // VertexCount, IndexCount / 2
        UInt    Texture9GridIdx;    // Index of 9-grid texture, or ~0 

        MeshSubShape()
            { MeshCount = StrokeCount = VertexStartIndex = VertexCount = 0; }

    };

    GTL::garray<MeshSubShape>       Shapes;

    Float*                          Scale9GridKey;
    GFxCharacter*                   pInstance;
    GTL::garray<GFxTexture9Grid>*   pTexture9Grids;

    // When using scale9grid with gradients or images, 
    // the image matrix has to be adjusted according to the 
    // "logical difference" between the world matrix and the 
    // respective part of the scale9grid. 
    // See also SetImgAdjustMatrices().
    GTL::garray<GRenderer::Matrix>* pImgAdjustMatrices;
};


#ifndef GFC_NO_FXPLAYER_STROKER

static GINLINE GStroker::LineCapType GFx_SWFToFxStroke_LineCap(GFxLineStyle::LineStyle s)
{
    switch(s)
    {
    case GFxLineStyle::LineCap_None:    return GStroker::ButtCap;
    case GFxLineStyle::LineCap_Square:  return GStroker::SquareCap;
    case GFxLineStyle::LineCap_Round:
    default:
        break;
    }
    return GStroker::RoundCap;
}

static GINLINE GStroker::LineJoinType GFx_SWFToFxStroke_LineJoin(GFxLineStyle::LineStyle s)
{
    switch(s)
    {
    case GFxLineStyle::LineJoin_Bevel:  return GStroker::BevelJoin;
    case GFxLineStyle::LineJoin_Miter:  return GStroker::MiterJoin;
    case GFxLineStyle::LineJoin_Round:
    default:
        break;
    }
    return GStroker::RoundJoin;
}

#endif


#endif // INC_GFXMESH_H

