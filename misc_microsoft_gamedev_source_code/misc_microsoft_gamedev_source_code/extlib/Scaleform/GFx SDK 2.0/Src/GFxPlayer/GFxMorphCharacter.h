/**********************************************************************

Filename    :   GFxMorphCharacter.h
Content     :   Morphable shape character definition
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXMORPHCHARACTER_H
#define INC_GFXMORPHCHARACTER_H

#include "GFxShape.h"


// ***** Declared Classes
class GFxMorphCharacterDef;



// ***** Morph Character Def class

class GFxMorphCharacterDef : public GFxShapeCharacterDef
{
public:
    GFxMorphCharacterDef();
    virtual ~GFxMorphCharacterDef();

    void            Read(GFxLoadProcess* p, GFxTagType tagType, bool withStyle);
    
    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst, StackData stackData);

    void            LerpMatrix(GRenderer::Matrix& t, const GRenderer::Matrix& m1, 
                               const GRenderer::Matrix& m2, const Float ratio);

    virtual void    Tesselate(GFxCompoundShapeAcceptor *p, Float tolerance,
                              GFxDisplayContext &context) const;
    virtual bool    PointTestLocal(const GPointF &pt, bool testShape = 0,
                                   const GFxPointTestCacheProvider *pcache = 0) const;
    void            ComputeBound(GRectF* r) const;
    void            MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

private:
    GFxShapeCharacterDef*   pShape1;
    GFxShapeCharacterDef*   pShape2;

    void            ReadMorphFillStyle(GFxLoadProcess* p, GFxTagType tagType, GFxFillStyle& fs1, GFxFillStyle& fs2);

    // Together with the previous anchor, defines a quadratic curve segment.
    //
    class Edge
    {
    public:
        Edge();
        Edge(Float cx, Float cy, Float ax, Float ay);
        void    AddForTesselation(GCompoundShape *pcompoundShape) const;

        //private:
        // *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
        Float   Cx, Cy;     // "control" point
        Float   Ax, Ay;     // "anchor" point
    };


    // Compound path: a series of edges sharing a single set of styles.
    class Path
    {
    public:
        Path();
        Path(Float ax, Float ay, UInt fill0, UInt fill1, UInt line);

        void    Reset(Float ax, Float ay, UInt fill0, UInt fill1, UInt line);
        bool    IsEmpty() const;

        // Push the path into the tessellator.
        void    AddForTesselation(GCompoundShape *pcompoundShape) const;

        //private:
        UInt                    Fill0, Fill1, Line;
        Float                   Ax, Ay;
        GTL::garray<Edge>       Edges;
        bool                    NewShape;
    };
    GTL::garray<Path>   Paths;

    UInt                Offset;
    UInt                FillStyleCount;
    UInt                LineStyleCount;
    Float               LastRatio;
    GFxMeshSet*         pMesh;
};

#endif // INC_GFXMORPH2_H
