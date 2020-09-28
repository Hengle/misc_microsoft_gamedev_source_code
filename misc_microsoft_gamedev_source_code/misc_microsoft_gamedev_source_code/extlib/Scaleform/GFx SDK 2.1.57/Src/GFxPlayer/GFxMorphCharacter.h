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

class GFxMorphCharacterDef : public GFxShapeWithStylesDef
{
public:
    GFxMorphCharacterDef();
    virtual ~GFxMorphCharacterDef();

    void            Read(GFxLoadProcess* p, const GFxTagInfo& tagInfo, bool withStyle);
    
    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst, StackData stackData);

    void            LerpMatrix(GRenderer::Matrix& t, const GRenderer::Matrix& m1, 
                               const GRenderer::Matrix& m2, const Float ratio);

    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &context,
                               GFxScale9GridInfo* s9g) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0,
                                      const GFxCharacter *pinst = 0) const;
    virtual GRectF  GetRectBoundsLocal() const { GRectF r; ComputeBound(&r); return r; }

    void            ComputeBound(GRectF* r) const;
    void            MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

private:
    GFxConstShapeNoStylesDef*  pShape1;
    GFxConstShapeNoStylesDef*  pShape2;
    GFxFillStyleArray          Shape1FillStyles;
    GFxFillStyleArray          Shape2FillStyles;
    GFxLineStyleArray          Shape1LineStyles;
    GFxLineStyleArray          Shape2LineStyles;

    void            ReadMorphFillStyle(GFxLoadProcess* p, GFxTagType tagType, GFxFillStyle& fs1, GFxFillStyle& fs2);

    // Together with the previous anchor, defines a quadratic curve segment.
    //
    class Edge
    {
    public:
        Edge();
        Edge(Float cx, Float cy, Float ax, Float ay);
        void    AddForTessellation(GCompoundShape *pcompoundShape) const;

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
        void    AddForTessellation(GCompoundShape *pcompoundShape) const;

        //private:
        UInt                    Fill0, Fill1, Line;
        Float                   Ax, Ay;
        GTL::garray<Edge>       Edges;
        bool                    NewShape;
    };
    GTL::garray<Path>   Paths;

    Float               LastRatio;
    GFxMeshSet*         pMesh;
};

#endif // INC_GFXMORPH2_H
