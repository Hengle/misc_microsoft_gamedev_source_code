/**********************************************************************

Filename    :   GFxEdgeAAGen.cpp
Content     :   EdgeAA vertex buffer initialization for meshes.
Created     :   May 2, 2006
Authors     :   Michael Antonov

Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxShape.h"
#include "GFxEdgeAAGen.h"
#include "GTessellator.h"


#ifndef GFC_NO_FXPLAYER_EDGEAA


// ***** GFxEdgeAAGenerator Implementation

// Specify pfills = 0 (or empty) to generate a solid.
GFxEdgeAAGenerator::GFxEdgeAAGenerator(GTessellator &tess, const GFxFillStyle *pfills, UPInt fillsNum)
{
    // Default mix factor applied has no texture component and EdgeAA value of 1.
    DefFactorRGB = 0x00000000;

    // Fill size of 0 means no texture fills.
    pFills      = pfills;
    FillsNum    = fillsNum;
    if (pFills && FillsNum == 0)
        pFills = 0;

    UsingTextures = 0;

    // Add vertices
    UInt i;
    for(i = 0; i < tess.GetNumVertices(); i++)
        EdgeAA.AddVertex(tess.GetVertex(i));

    int fillSize = pFills ? (int)FillsNum : 0;

    // And triangles
    for(i = 0; i < tess.GetNumMonotones(); i++)
    {
        // Get the info about the i-th monotone.
        // Well, basically m.style is all you need.
        const GTessellator::MonotoneType& m = tess.GetMonotone(i);

        // Mark textures
        // The tessellator uses zero-based styles, so, make it minus-one-based
        // to conform to the existing policy.
        int style = (int)m.style - 1;

        // 0's are solid fills, the rest depend on images
        // NOTE: Fills can be empty for fonts, so check bounds.
        if ((style < fillSize) && (pFills[style].GetType() != 0))
        {
            style |= StyleTextureBit;
            UsingTextures = 1;
        }

        // Triangulate the i-th monotone polygon. After this call
        // you can iterate through the triangles for this particular monotone.
        tess.TriangulateMonotone(i);

        // Add triangles to AA (with m.style)
        UInt    triangleCount = tess.GetNumTriangles();
        UInt    triangle;
        // Fetch triangle indices for each triangle
        for(triangle = 0; triangle<triangleCount; triangle++)
        {
            const GTessellator::TriangleType& t = tess.GetTriangle(triangle);
            EdgeAA.AddTriangle(t.v1, t.v2, t.v3, style);

            GASSERT(style != -1);
        }
    }
}


GFxEdgeAAGenerator::~GFxEdgeAAGenerator()
{
}

// Processes & sorts edges if necessary
void    GFxEdgeAAGenerator::ProcessAndSortEdges (GCoordType width, GEdgeAA::AA_Method aaMethod)
{
    EdgeAA.ProcessEdges(width, aaMethod);
    if (UsingTextures)
        EdgeAA.SortTrianglesByStyle();
}

// Returns the number of vertices generated,.
UInt    GFxEdgeAAGenerator::GetVertexCount() const
{
    return EdgeAA.GetNumVertices();
}


template <class Vertex>
Float   CalcTriangleColorWeight(const Vertex& v1,  const Vertex& v2, const Vertex& v3)
{
    // ratio - how far V1 is on the way between v2 and v3.
    Float   ratio = 0.5f;

    // V1 color lies between V2 and V3
    Float   dxB = (Float)v3.x - (Float)v2.x;
    Float   dyB = (Float)v3.y - (Float)v2.y;
    Float   dxA = (Float)v1.x - (Float)v2.x;
    Float   dyA = (Float)v1.y - (Float)v2.y;

    Float   dotAB = dxA * dxB + dyA * dyB;
    Float   lenBsq= dxB * dxB + dyB * dyB;

    if (fabs(lenBsq) > 1e-4)
        ratio = GTL::gclamp<Float>(dotAB / lenBsq, 0.0f, 1.0f);

    return ratio;
}

//
GINLINE UInt32  LerpColorChannel(UInt32 c1, UInt32 c2, UInt32 cmask, UInt32 shift, Float ratio)
{
    return ((UInt32)(gflerp((Float)((c1 & cmask) >> shift),
                           (Float)((c2 & cmask) >> shift), ratio))) << shift;
}

#if (GFC_BYTE_ORDER == GFC_BIG_ENDIAN) && !defined(GFC_OS_XBOX360) && !defined(GFC_OS_PS3) && !defined(GFC_OS_WII)

#define COLOR_1110 0xFFFFFF00

GINLINE UInt32  LerpColorRGB(UInt32 c1, UInt32 c2, Float ratio)
{
    return
        LerpColorChannel(c1, c2, 0xFF000000, 24, ratio) |
        LerpColorChannel(c1, c2, 0xFF00, 8, ratio) |
        LerpColorChannel(c1, c2, 0xFF0000, 16, ratio);
}

#else

#define COLOR_1110 0x00FFFFFF

GINLINE UInt32  LerpColorRGB(UInt32 c1, UInt32 c2, Float ratio)
{
    return
        LerpColorChannel(c1, c2, 0xFF, 0, ratio) |
        LerpColorChannel(c1, c2, 0xFF00, 8, ratio) |
        LerpColorChannel(c1, c2, 0xFF0000, 16, ratio);
}

#endif

// Cleans up colors in vertices.
// This function will propagate color channels into the "blank" corners of the triangle.

// For VertexXY16iC32 format, don't propagate Color.Alpha channel because it is (Color.Alpha * EdgeAA).
void    UpdateTriangleColors(GRenderer::VertexXY16iC32& v1, GRenderer::VertexXY16iC32& v2, GRenderer::VertexXY16iC32& v3,
                             int v1id, int v2id, int v3id)
{
    enum {
        EdgeBit = 0x80000000 | GFxEdgeAAGenerator::StyleTextureBit
    };

    if (v1id & EdgeBit)
    {
        if (v2id & EdgeBit)
        {
            if (v3id & EdgeBit)
                return;
            // Propagate color into both vertices
            v1.Color |= v3.Color & COLOR_1110;
            v2.Color |= v3.Color & COLOR_1110;
        }
        else if (v3id & EdgeBit)
        {
            v1.Color |= v2.Color & COLOR_1110;
            v3.Color |= v2.Color & COLOR_1110;
        }
        else
        {
            // ratio - how far V1 is on the way between v2 and v3.
            Float   ratio = CalcTriangleColorWeight(v1, v2, v3);
            // Interpolate color (r,g,b) order agnostic.
            v1.Color |= LerpColorRGB(v2.Color, v3.Color, ratio);
        }
    }
    else
    {
        if (v2id & EdgeBit)
        {
            if (v3id & EdgeBit)
            {
                v2.Color |= v1.Color & COLOR_1110;
                v3.Color |= v1.Color & COLOR_1110;
            }
            else
            {
                // v2 receives color between v1 and v3
                Float   ratio = CalcTriangleColorWeight(v2, v1, v3);
                v2.Color |= LerpColorRGB(v1.Color, v3.Color, ratio);
            }
        }
        else if (v3id & EdgeBit)
        {
            // v3 receives color between v1 and v2
            Float   ratio = CalcTriangleColorWeight(v3, v1, v2);
            v3.Color |= LerpColorRGB(v1.Color, v2.Color, ratio);
        }
    }
}

#undef COLOR_1110

// For VertexXY16iCF32 format, we propagate Color.Alpha channel as well, since EdgeAA is stored instead in Factor.Alpha.
void    UpdateTriangleColors(GRenderer::VertexXY16iCF32& v1, GRenderer::VertexXY16iCF32& v2, GRenderer::VertexXY16iCF32& v3,
                             int v1id, int v2id, int v3id)
{
    enum {
        EdgeBit = 0x80000000 | GFxEdgeAAGenerator::StyleTextureBit
    };

    if (v1id & EdgeBit)
    {
        if (v2id & EdgeBit)
        {
            // Have to check for v3.id & EdgeBit, otherwise it may overwrite previously
            // computed colors from the other side.
            if (v3id & EdgeBit)
                return;
            // Propagate color into both vertices
            v1.Color = v3.Color;
            v2.Color = v3.Color;
        }
        else if (v3id & EdgeBit)
        {
            v1.Color = v2.Color;
            v3.Color = v2.Color;
        }
        else
        {
            if (v2.Color == v3.Color)
                v1.Color = v2.Color;
            else
            {
                // ratio - how fat V1 is on the way between v2 and v3.
                Float   ratio = CalcTriangleColorWeight(v1, v2, v3);
                // Interpolate color (r,g,b) order agnostic.
                v1.Color =
                    LerpColorChannel(v2.Color, v3.Color, 0xFF, 0, ratio) |
                    LerpColorChannel(v2.Color, v3.Color, 0xFF00, 8, ratio) |
                    LerpColorChannel(v2.Color, v3.Color, 0xFF0000, 16, ratio) |
                    LerpColorChannel(v2.Color, v3.Color, 0xFF000000, 24, ratio);
            }
        }
    }
    else
    {
        if (v2id & EdgeBit)
        {
            if (v3id & EdgeBit)
            {
                v2.Color = v1.Color;
                v3.Color = v1.Color;
            }
            else
            {
                if (v1.Color == v3.Color)
                    v2.Color = v1.Color;
                else
                {
                    // v2 receives color between v1 and v3
                    Float   ratio = CalcTriangleColorWeight(v2, v1, v3);
                    v2.Color =
                        LerpColorChannel(v1.Color, v3.Color, 0xFF, 0, ratio) |
                        LerpColorChannel(v1.Color, v3.Color, 0xFF00, 8, ratio) |
                        LerpColorChannel(v1.Color, v3.Color, 0xFF0000, 16, ratio) |
                        LerpColorChannel(v1.Color, v3.Color, 0xFF000000, 24, ratio);
                }
            }
        }
        else if (v3id & EdgeBit)
        {
            if (v1.Color == v2.Color)
                v3.Color = v1.Color;
            else
            {
                // v3 receives color between v1 and v2
                Float   ratio = CalcTriangleColorWeight(v3, v1, v2);
                v3.Color =
                    LerpColorChannel(v1.Color, v2.Color, 0xFF, 0, ratio) |
                    LerpColorChannel(v1.Color, v2.Color, 0xFF00, 8, ratio) |
                    LerpColorChannel(v1.Color, v2.Color, 0xFF0000, 16, ratio) |
                    LerpColorChannel(v1.Color, v2.Color, 0xFF000000, 24, ratio);
            }
        }
    }
}

// Helper for GenerateSolidMesh.
void    GFxEdgeAAGenerator::UpdateEdgeAATriangleColors(GFxVertexArray *pvertices)
{
    GRenderer::VertexFormat vf = pvertices->GetFormat();
    UInt                    i;

    if (!pvertices->GetSize())
        return;

    // Switch to avoid excessive virtual calls in UpdateTriangleColors needed otherwise.
    switch(vf)
    {
    case GRenderer::Vertex_XY16iC32:
        {
            GRenderer::VertexXY16iC32 *prawVertices = (GRenderer::VertexXY16iC32*)
                                                      pvertices->GetVertexPtr(0);

            for (i=0; i< EdgeAA.GetNumTriangles(); i++)
            {
                const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);
                // Assign correct colors to corner vertices
                UpdateTriangleColors(prawVertices[t.v1], prawVertices[t.v2], prawVertices[t.v3],
                                     EdgeAA.GetVertex(t.v1).id, EdgeAA.GetVertex(t.v2).id, EdgeAA.GetVertex(t.v3).id );
            }
        }
        break;

    case GRenderer::Vertex_XY16iCF32:
        {
            GRenderer::VertexXY16iCF32 *prawVertices = (GRenderer::VertexXY16iCF32*)
                                                      pvertices->GetVertexPtr(0);

            for (i=0; i< EdgeAA.GetNumTriangles(); i++)
            {
                const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);

                // Assign correct colors to corner vertices
                UpdateTriangleColors(prawVertices[t.v1], prawVertices[t.v2], prawVertices[t.v3],
                                     EdgeAA.GetVertex(t.v1).id, EdgeAA.GetVertex(t.v2).id, EdgeAA.GetVertex(t.v3).id );
            }
        }
        break;

    default:
    break;
    }
}

void    GFxEdgeAAGenerator::AddTrianglesAndUpdateColors(GFxVertexArray *pvertices, GFxMesh *pmesh)
{
    UInt i;
    for (i=0; i< EdgeAA.GetNumTriangles(); i++)
    {
        const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);
        pmesh->AddTriangle((UInt16)t.v1, (UInt16)t.v2, (UInt16)t.v3);
    }

    UpdateEdgeAATriangleColors(pvertices);
}



bool    GFxEdgeAAGenerator::GenerateSolidMesh(GFxVertexArray *pvertices, GFxMesh *pmesh, Float scaleMultiplier)
{
    // Generate AA meshes
    pvertices->Resize(EdgeAA.GetNumVertices());

    // Should we copy vertices first,
    // and initialize them with color later
    int  fillsSize = pFills ? (int)FillsNum : 0;

    UInt i;
    for (i=0; i< EdgeAA.GetNumVertices(); i++)
    {
        const GEdgeAA::VertexType &v  = EdgeAA.GetVertex(i);
        GFxVertexRef              cv  = pvertices->GetVertexRef(i);

        cv.InitVertex(v.x * scaleMultiplier, v.y * scaleMultiplier);

        if (v.id == -1)
        {
            cv.SetFactor(DefFactorRGB);
        }
        else
        {
            // NOTE: Issue with font: we don't know color ahead of time.
            // Store while for now, but will need another font factor later.

            int id = v.id & ~StyleTextureBit;
            if (id >= fillsSize)
            {
                // Font, don't have fill
                cv.SetColor(0xFFFFFFFF);
            }
            else if (pFills[id].GetType() == 0)
            {
                cv.SetColor(GetProcessedColor(id));
            }

            // Set EdgeAA constant to 1.0f, texture mix factor of 0.
            cv.SetFactor(DefFactorRGB | 0xFF000000);
        }
    }

    // Generate a triangle mesh and assign corner colors.
    AddTrianglesAndUpdateColors(pvertices, pmesh);
    return 1;
}


bool    GFxEdgeAAGenerator::GenerateSolidMesh(GFxVertexArray *pvertices, GFxMesh *pmesh, const GRenderer::Matrix &mat)
{
    // Generate AA meshes
    pvertices->Resize(EdgeAA.GetNumVertices());

    // Should we copy vertices first,
    // and initialize them with color later
    int     fillsSize = pFills ? (int)FillsNum : 0;
    UInt    i;
    GPointF pt;

    for (i=0; i< EdgeAA.GetNumVertices(); i++)
    {
        const GEdgeAA::VertexType &v  = EdgeAA.GetVertex(i);
        GFxVertexRef              cv  = pvertices->GetVertexRef(i);

        // Transform back (usually to shape space from screen space)
        mat.Transform(&pt, GPointF(v.x, v.y));

    #ifdef GFC_BUILD_DEBUG
        if ((pt.x > 32767.0f) || (pt.y > 32767.0f) || (pt.y < -32768.0f) || (pt.x < -32768.0f))
        {
            static bool RangeOverflowWarned = 0;
            if (!RangeOverflowWarned)
            {
                GFC_DEBUG_WARNING2(1,
                        "Stroker target range overflow for EdgeAA: (x,y) = (%g,%g).",
                        pt.x, pt.y);
                RangeOverflowWarned = 1;
            }
            // This clamping should be faster for debug
            if (pt.x > 32767.0f)    pt.x = 32767.0f;
            if (pt.x < -32768.0f)   pt.x = -32768.0f;
            if (pt.y > 32767.0f)    pt.y = 32767.0f;
            if (pt.y < -32768.0f)   pt.y = -32768.0f;
        }

    #else
        // Safety clamping for the case above. CPU should have an fabsf() op.
        if (fabsf(pt.x) > 32767.0f)
            pt.x = (pt.x > 32767.0f) ? 32767.0f : -32768.0f;
        if (fabsf(pt.y) > 32767.0f)
            pt.y = (pt.y > 32767.0f) ? 32767.0f : -32768.0f;
    #endif

        cv.InitVertex(pt.x, pt.y, 0);

        if (v.id == -1)
        {
            cv.SetFactor(DefFactorRGB);
        }
        else
        {
            // NOTE: Issue with font: we don't know color ahead of time.
            // Store white for now, but will need another font factor later.

            int id = v.id & ~StyleTextureBit;
            if (id >= fillsSize)
            {
                // Font, don't have fill
                cv.SetColor(0xFFFFFFFF);
            }
            else if (pFills[id].GetType() == 0)
            {
                cv.SetColor(GetProcessedColor(id));
            }

            // Set EdgeAA constant to 1.0f, texture mix factor of 0.
            cv.SetFactor(DefFactorRGB | 0xFF000000);
        }
    }

    // Generate a triangle mesh and assign corner colors.
    AddTrianglesAndUpdateColors(pvertices, pmesh);
    return 1;
}



// Helper class: collects new vertices, duplicating them if so required for Factor
// uniqueness. This class is used to check if the vertices factor has been assigned
// based on a triangle. If that is the case, the other triangles must either match,
// or a new accommodating vertex will be inserted.
struct GFxEdgeAAGenerator_VertexCollector
{
    typedef GRenderer::VertexXY16iCF32 VertexXY16iCF32;

    GFxVertexArray &    NewVertices;
    // UsedVertex is used both as a vertex-to factor allocated flag and index
    // to the next replicated vertex with the same factor.
    GTL::garray<UInt>   UsedVertices;

    GFxEdgeAAGenerator_VertexCollector(GFxVertexArray *pvertices, UInt size)
        : NewVertices (*pvertices)
    {
        NewVertices.Resize(size);
        UsedVertices.resize(size);
        for (UInt i=0; i<UsedVertices.size(); i++)
            UsedVertices[i] = GFC_MAX_UINT;
    }

    const GFxEdgeAAGenerator_VertexCollector& operator = (const GFxEdgeAAGenerator_VertexCollector&)
    { return *this; }

    GFxVertexRef operator [] (UInt index) const
    { return NewVertices[index]; }

    // Assigns a factor value to a vertex. Returns a potentially different index.
    UInt    AssignFactor(UInt index, UInt32 factor)
    {
        // If vertex does not yet have an assigned factor, use it directly
        GFxVertexRef vertexRef = NewVertices[index];

        if (UsedVertices[index] == GFC_MAX_UINT)
        {
            UsedVertices[index] = index;
            vertexRef.SetFactor(factor);
            return index;
        }

        if (vertexRef.GetFactor() == factor)
            return index;

        // Search index list for next use.
        while (UsedVertices[index] != index)
        {
            index = UsedVertices[index];
            if (NewVertices[index].GetFactor() == factor)
                return index;
        }

        // Use not found, create new vertex.
        vertexRef = NewVertices[index];

        Float   x, y;
        UInt32  color = vertexRef.GetColor();
        vertexRef.GetXY(&x, &y);

        // Adjust list and index.
        UsedVertices[index] = NewVertices.GetSize();
        index = NewVertices.GetSize();

        // Push new values.
        UsedVertices.push_back(index);
        vertexRef = NewVertices.AppendVertex();
        vertexRef.InitVertex(x, y, color);
        vertexRef.SetFactor(factor);

        return index;
    }
};



// Generates textured meshes, returns number of mesh sets if fit, 0 if over 65K (fails).
// If fails, Mesh array remains untouched and 0 is returned.
UInt    GFxEdgeAAGenerator::GenerateTexturedMeshes(GFxVertexArray *pvertices, GTL::garray<GFxMesh> *pmeshes, Float scaleMultiplier)
{
    GASSERT(pmeshes != NULL);

    GFxEdgeAAGenerator_VertexCollector  vertices(pvertices, EdgeAA.GetNumVertices());

    // Should we copy vertices first,
    // and initialize them with color later
    UInt i;
    for (i=0; i< EdgeAA.GetNumVertices(); i++)
    {
        const GEdgeAA::VertexType &v  = EdgeAA.GetVertex(i);
        GFxVertexRef              cv  = pvertices->GetVertexRef(i);

        cv.InitVertex(v.x * scaleMultiplier, v.y * scaleMultiplier);

        if (v.id == -1)
        {
            cv.SetFactor(0);
        }
        else
        {
            // NOTE: Issue with font: we don't know color ahead of time.
            // Store while for now, but will need another font factor later.

            int id = v.id & ~StyleTextureBit;
            if (id >= (int)FillsNum)
            {
                // Font, don't have fill
                cv.SetColor(0xFFFFFFFF);
            }
            else if (pFills[id].GetType() == 0)
            {
                cv.SetColor(GetProcessedColor(id));
            }

            // Set EdgeAA constant to 1.0f, texture mix factors of 0 until updated below.
            cv.SetFactor(0xFF000000);
        }
    }

    // Assign correct colors to corner vertices of each triangle.
    UpdateEdgeAATriangleColors(pvertices);


    // Styles used in these vertices.
    int s1 = -1;
    int s2 = -1;
    int s3 = -1;

    // Store initial mesh size in case we fail.
    //UInt startMeshSize    = pmeshes->size();
    UInt meshCount      = 0;


    // Triangles come out consecutive by style, so we can generate
    // a special MeshSet for each style change
    for (i=0; i< EdgeAA.GetNumTriangles(); i++)
    {
        const GEdgeAA::TriangleType &t = EdgeAA.GetTriangle(i);
        // Style tripple
        const GEdgeAA::VertexType &v1 = EdgeAA.GetVertex(t.v1);
        const GEdgeAA::VertexType &v2 = EdgeAA.GetVertex(t.v2);
        const GEdgeAA::VertexType &v3 = EdgeAA.GetVertex(t.v3);

        bool    v1Texture = ((v1.id & StyleTextureBit) && (v1.id != -1));
        bool    v2Texture = ((v2.id & StyleTextureBit) && (v2.id != -1));
        bool    v3Texture = ((v3.id & StyleTextureBit) && (v3.id != -1));

        bool    haveTextures = v1Texture | v2Texture | v3Texture;


        if (((s1 != v1.id) && v1Texture) ||
            ((s2 != v2.id) && v2Texture) ||
            ((s3 != v3.id) && v3Texture) ||
            (i == 0) )
        {
            // Determine shade mode.
            GRenderer::GouraudFillType gft = GRenderer::GFill_Color;
            if (haveTextures)
            {
                gft = GRenderer::GFill_1TextureColor;

                if (v1Texture && v2Texture && v3Texture)
                {
                    if ((v3.id != v2.id) || (v3.id != v1.id))
                        gft = GRenderer::GFill_2Texture;
                }
            }

            // Detect Textured cases where style change does not require us to generate
            // a new mesh, and can thus share DrawPrimitive calls.
            // Note that due to sorting, we may not detect all combinable cases here.
            bool    sharePrimitives = 0;

            if ((gft == GRenderer::GFill_1TextureColor) ||
                (gft == GRenderer::GFill_2Texture) )
            {
                // Change from 2 styles with texture and one style with texture can share DrawPrimitive.
                bool    s1IsTexture = ((s1 & StyleTextureBit) && (s1 != -1));

                // If s3 did not change and s1 did not change texture
                if ( (s3 == v3.id) &&
                    ((s1 == v1.id) || (v1.id == v3.id || (gft == GRenderer::GFill_1TextureColor && (!v1Texture || !s1IsTexture))))  )
                {
                    // This must be true after the check above.
                    GASSERT ((v2.id != s2) || (v1.id != s1));

                    // The following transitions are allowed (back and forth):
                    //
                    //  v1:   C    < = >    T
                    //  v2:   C    < = >    T
                    //  v3:   T             T
                    //
                    //  or
                    //
                    //  v1:   T1            T1
                    //  v2:   T1   < = >    T2
                    //  v3:   T2            T2

                    // If it is a texture, v2 must now equal either T1 or T2, in order to share primitive.
                    if (v2Texture)
                    {
                        if ((v2.id == v3.id) || (v2.id == v1.id))
                        {
                            bool s2IsTexture = ((s2 & StyleTextureBit) && (s2 != -1));

                            if (!s2IsTexture)
                            {
                                sharePrimitives = 1;
                            }
                            else
                            {
                                // s2IsTetexture check is necessary to avoid transitions such as:
                                //  T1, T2, T3 -> T1, T3, T3
                                //  TBD: This may not be necessary once we handle 3-texture cases?
                                //  GFill_2Texture check allows sow simple 3-different corner
                                //  cases to be batched "incorrectly" (single corner triangle, but less DPs).
                                if ((s2 == v3.id) || (s2 == v1.id) || (gft != GRenderer::GFill_2Texture))
                                    sharePrimitives = 1;
                            }

                        }
                    }
                    else
                    {
                        // Otherwise, if we are a color, the following must be true
                        //   - s1 must also be a color
                        //   - s2 must have been the same texture as s3

                        if (!s1IsTexture && (s1 == s3))
                        {
                            sharePrimitives = 1;
                        }
                    }
                }

            }

            // Triangles added to new mesh.
            // We do this even if we are sharing primitives, so that matching above can work faster.
            s1 = v1.id;
            s2 = v2.id;
            s3 = v3.id;

            if (!sharePrimitives)
            {
                // If texture style changed, new MeshSet
                pmeshes->resize(pmeshes->size()+1);
                meshCount++;
                // Different style tables are used based on different fill rule.
                pmeshes->back().SetGouraudFillType(gft);

                switch(gft)
                {
                case GRenderer::GFill_1TextureColor:
                    // 1 texture: will always be style 3 due to sorting.
                    pmeshes->back().SetEdgeAAStyles(1, (s3 == -1) ? (UInt)GFxPathConsts::Style_None : (s3 & ~StyleTextureBit),
                                                    (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None);
                    break;

                case GRenderer::GFill_2Texture:
                    // 2 textures.
                    // Here, second texture can come from either s2 or s3.
                    {
                        int secondStyle = (s3 != s2) ? s2 : s1;
                        pmeshes->back().SetEdgeAAStyles(2,
                                (s3 == -1) ? (UInt)GFxPathConsts::Style_None : (s3 & ~StyleTextureBit),
                                (secondStyle == -1) ? (UInt)GFxPathConsts::Style_None : (secondStyle & ~StyleTextureBit),
                                (UInt)GFxPathConsts::Style_None);
                    }
                    break;

                case GRenderer::GFill_Color:
                default:
                    // No textures.
                    pmeshes->back().SetEdgeAAStyles(0, (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None, (UInt)GFxPathConsts::Style_None);
                    break;
                }
            }

        }


        // Keep processing vertex: Assign factors.
        if (haveTextures)
        {
            UInt    iv1 = t.v1;
            UInt    iv2 = t.v2;
            UInt    iv3 = t.v3;

            UInt32  v1Factor = vertices[iv1].GetFactor() & 0xFF000000,
                    v2Factor = vertices[iv2].GetFactor() & 0xFF000000,
                    v3Factor = vertices[iv3].GetFactor() & 0xFF000000;

            // 1. Figure out what the factors should be for each vertex
            //    (only necessary if there are ANY textures)

            /* Complex - case blending, need to handle channels differently
            if (v1Texture)
                v1Factor = 0x0000FF00; // G = 1.0
            else
                v1Factor = 0;

            if (v2Texture)
                v2Factor = 0x00FF0000; // B = 1.0
            else
                v2Factor = 0;
            */

            if (v3Texture)
            {
                v3Factor |= 0x00FFFFFF; // RGB = 1.0

                // Account for identical texture cases, where the same texture is
                // assigned to more then one corner.

                // Replicate factors for same texture use in different corner vertices.
                if ((v2.id == v3.id) || (v2.id == -1))
                {
                    v2Factor |= 0x00FFFFFF;
                    if ((v1.id == v2.id) || (v1.id == -1))
                        v1Factor |= 0x00FFFFFF;
                }
                else
                {
                    // because of backwards sort v3 is primary texture and v2 secondary
                    // V3 will be texture

                    if ((v1.id == v3.id) || (v1.id == -1))
                    {
                        // texture2 mode
                        v1Factor |= 0x00FFFFFF;
                    }

                    // Two or more textures we used, need to handle this.
                //  GASSERT(!(v2.id & StyleTextureBit) || (v2.id == -1));
                //  GASSERT(!(v1.id & StyleTextureBit) || (v1.id == -1));
                }
            }
            else
            {
                // v3Factor |= 0;
                GASSERT(0);
            }


            // 2. Assign factors to vertices, duplicating them if necessary
            //    (modify indices in process)

            iv1 = vertices.AssignFactor(iv1, v1Factor);
            iv2 = vertices.AssignFactor(iv2, v2Factor);
            iv3 = vertices.AssignFactor(iv3, v3Factor);

            pmeshes->back().AddTriangle((UInt16)iv1, (UInt16)iv2, (UInt16)iv3);
        }
        else
        {
            UInt    iv1 = t.v1;
            UInt    iv2 = t.v2;
            UInt    iv3 = t.v3;

            // Need to mark factor as used.
    //      iv1 = vertices.AssignFactor(iv1, vertices[iv1].Factors & 0xFF000000);
    //      iv2 = vertices.AssignFactor(iv2, vertices[iv2].Factors & 0xFF000000);
    //      iv3 = vertices.AssignFactor(iv3, vertices[iv3].Factors & 0xFF000000);

            // Store triangle indices into mesh set.
            pmeshes->back().AddTriangle((UInt16)iv1, (UInt16)iv2, (UInt16)iv3);
        }
    }

    return meshCount;
}


#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
