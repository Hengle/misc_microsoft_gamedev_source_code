/**********************************************************************

Filename    :   GFxEdgeAAGen.h
Content     :   EdgeAA vertex buffer initialization for meshes.
Created     :   May 2, 2006
Authors     :   Michael Antonov

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.
                Patent Pending. Contact Scaleform for more information.

History     :   June 27, 2006 : Pulled out from GFxShape.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXEDGEAAGEN_H
#define INC_GFXEDGEAAGEN_H

#include "GFxMesh.h"
#include "GEdgeAA.h"

#ifndef GFC_NO_FXPLAYER_EDGEAA


// ***** Declared Classes
class GFxEdgeAAGenerator;

// ***** External Classes
class GTessellator;


/* *****  Edge AA Vertex data formats

    Edge AA support requires vertices to include additional Color and Factor components
    necessary to achieve correct shading for the edges. Factor component is optional
    depending on whether texture mixing is used, or Cxform's Add Alpha channel is non-zero.

    Formats:

    struct Vertex_XY16iC32
    {
        SInt16  x,y;
        UInt32  Color;

        // 8 bytes:
        // Color.rgb    - Original linear color channels (NOT pre-multiplied).
        // Color.a      - Source alpha * EdgeAA interpolation alpha.
    };

    struct Vertex_XY16iCF32
    {
        SInt16  x,y;
        UInt32  Color;
        UInt32  Factor;

        // 12 bytes:
        // Color.rgba   - Original linear alpha and color channels (NOT pre-multiplied).
        // Factor.a     - EdgeAA interpolation alpha.
        // Factor.rgb   - Texture 1 mixing factor (1.0 = use texture, 0.0 = use color).
    };

    The smaller Vertex_XY16iC32 format can be used GFill_Color FillType, as long as
    color matrix Add Alpha component is zero. If Cxform add alpha != 0, Vertex_XY16iCF32
    must be used instead, since EdgeAA must then be separate from source alpha.

    Vertex_XY16iCF32 is also used for formats which include textures. Here Factor.rgb
    channels contain the blending factor of the first texture.

    Optimizations - TBD: Some texture cases could use Vertex_XY16iC32 format where
    color is treated as Factor. Also, multi-texture techniques could be devised that
    would combine multi-texture cases into one DrawPrimitive call.

*/


// Edge generator helper class, used to generate meshes out of tessellations
class GFxEdgeAAGenerator
{
    GEdgeAA                         EdgeAA;
    const GFxFillStyle*             pFills;
    UPInt                           FillsNum;
    // Flag set if textured fill styles are used.
    bool                            UsingTextures;
    // Default texture mix factor (used for gradient stroke).
    UInt32                          DefFactorRGB;


    UInt32  GetProcessedColor(int fillIndex) const
    {
        GColor c = pFills[fillIndex].GetColor();
        GColor nc = c;

        // Pre-multiply alpha
        /*
        UInt alpha = ((UInt)c.GetAlpha()) + 1;
        nc.SetGreen((c.GetGreen() * alpha) >> 8);
        nc.SetBlue( (c.GetRed() * alpha) >> 8); // Swap G & B
        nc.SetRed(  (c.GetBlue() * alpha) >> 8);
        */
        nc.SetBlue(c.GetRed() ); // Swap G & B
        nc.SetRed( c.GetBlue());
        return nc.Raw;
    }

    // Helper for GenerateSolidMesh.
    void    AddTrianglesAndUpdateColors(GFxVertexArray *pvertices, GFxMesh *pmesh);
    void    UpdateEdgeAATriangleColors(GFxVertexArray *pvertices);

    GFxEdgeAAGenerator(const GFxEdgeAAGenerator&)
    { pFills = 0; FillsNum = 0; }

    const GFxEdgeAAGenerator& operator = (const GFxEdgeAAGenerator&)
    { return *this; }

public:

    enum {
        StyleTextureBit = 0x40000000
    };


    // Regular AA
    // Specify pfills = 0 (or empty) to generate a solid .
    GFxEdgeAAGenerator(GTessellator &tess, const GFxFillStyle *pfills = 0, UPInt fillsNum = 0);
    ~GFxEdgeAAGenerator();

    void    SetDefaultFactorRGB(UInt32 factor) { DefFactorRGB = factor; }

    // Processes & sorts edges if necessary
    void    ProcessAndSortEdges (GCoordType width, GEdgeAA::AA_Method aaMethod);

    // Returns 1 if uses textures.
    bool    IsUsingTextures() const { return UsingTextures; }

    // Returns the number of vertices generated
    UInt    GetVertexCount() const;

    // Generate a vertex array and data; fills in a mesh.
    bool    GenerateSolidMesh(GFxVertexArray *pvertices, GFxMesh *pmesh, Float scaleMultiplier);
    // Generate a vertex array and data; fills in a mesh.
    // Applies a matrix to each coordinate before transform, together with clamping.
    bool    GenerateSolidMesh(GFxVertexArray *pvertices, GFxMesh *pmesh, const GRenderer::Matrix &mat);


    // Generates textured meshes, returns nmuber of mesh sets if fit, 0 if over 65K (fails).
    // If fails, Mesh array remains untouched and 0 is returned.
    UInt    GenerateTexturedMeshes(GFxVertexArray *pvertices, GTL::garray<GFxMesh> *pmeshes, Float scaleMultiplier);
};

#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA
#endif // INC_GFXEDGEAAGEN_H

