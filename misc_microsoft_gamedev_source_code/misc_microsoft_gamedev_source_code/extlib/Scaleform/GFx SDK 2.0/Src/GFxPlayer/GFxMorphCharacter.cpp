/**********************************************************************

Filename    :   GFxMorphCharacter.cpp
Content     :   Morphable shape character implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxMorphCharacter.h"
#include "GFxMesh.h"
#include "GFxStream.h"
#include "GMath.h"

#include "GFxCharacter.h"
#include "GFxAction.h"
#include "GFxDisplayContext.h"
#include "GFxLoadProcess.h"
#include "GFxImageResource.h"

#include <stdio.h>


//
// ***** GFxEdge
//

GFxMorphCharacterDef::Edge::Edge()
:
    Cx(0), Cy(0),
    Ax(0), Ay(0)
{}

GFxMorphCharacterDef::Edge::Edge(Float cx, Float cy, Float ax, Float ay)
:
    Cx(cx), Cy(cy),
    Ax(ax), Ay(ay)
{
}

// Send this segment to the tesselator.
void    GFxMorphCharacterDef::Edge::AddForTesselation(GCompoundShape *pcompoundShape) const
{
    pcompoundShape->AddCurve(Cx, Cy, Ax, Ay);
}


//
// ***** GFxPath
//

GFxMorphCharacterDef::Path::Path()
{
    NewShape = 0;
    Reset(0, 0, 0, 0, 0);
}

GFxMorphCharacterDef::Path::Path(Float ax, Float ay, UInt fill0, UInt fill1, UInt line)
{
    Reset(ax, ay, fill0, fill1, line);
}


// Reset all our members to the given values, and clear our GFxEdge list.
void    GFxMorphCharacterDef::Path::Reset(Float ax, Float ay, UInt fill0, UInt fill1, UInt line)
{
    Ax      = ax;
    Ay      = ay;
    Fill0   = fill0;
    Fill1   = fill1;
    Line    = line;

    Edges.resize(0);
    GASSERT(IsEmpty());
}


// Return true if we have no edges.
bool    GFxMorphCharacterDef::Path::IsEmpty() const
{
    return Edges.size() == 0;
}


// Push this GFxPath into the tesselator.
void    GFxMorphCharacterDef::Path::AddForTesselation(GCompoundShape *pcompoundShape) const
{
    pcompoundShape->BeginPath(Fill0 - 1, Fill1 - 1, Line - 1, Ax, Ay);
    for (UPInt i = 0, n = Edges.size(); i < n; i++)
    {
        Edges[i].AddForTesselation(pcompoundShape);
    }
}

GFxMorphCharacterDef::GFxMorphCharacterDef():
    LastRatio(-1.0f), pMesh(0)
{
    pShape1 = new GFxShapeCharacterDef();
    pShape2 = new GFxShapeCharacterDef();
}


GFxMorphCharacterDef::~GFxMorphCharacterDef()
{
    pShape2->Release();
    pShape1->Release();
    if (pMesh)
        delete pMesh;
}

void    GFxMorphCharacterDef::Display(GFxDisplayContext &context, GFxCharacter* inst, StackData stackData)
{
    UPInt i, n;
    Float ratio = inst->GetRatio();

    // bounds
    GRectF  NewBound;
    NewBound.SetLerp(pShape1->GetBound(), pShape2->GetBound(), ratio);
    SetBound(NewBound);

    // fill styles
    for (i = 0, n = FillStyles.size(); i < n; i++)
    {
        GFxFillStyle& fs = FillStyles[i];

        const GFxFillStyle& fs1 = pShape1->GetFillStyles()[i];
        const GFxFillStyle& fs2 = pShape2->GetFillStyles()[i];

        fs.SetLerp(fs1, fs2, ratio);
    }

    // line styles
    for (i = 0, n = LineStyles.size(); i < n; i++)
    {
        GFxLineStyle& ls = LineStyles[i];
        const GFxLineStyle& ls1 = pShape1->GetLineStyles()[i];
        const GFxLineStyle& ls2 = pShape2->GetLineStyles()[i];
        ls.StyleFlags = ls1.StyleFlags; //!AB ???
        ls.Width = (UInt16)gfrnd(gflerp(ls1.GetWidth(), ls2.GetWidth(), ratio));
        ls.Color = GColor::Blend(ls1.GetColor(), ls2.GetColor(), ratio);   
        if (ls1.HasComplexFill())
        {
            GASSERT(ls2.HasComplexFill());
            const GFxFillStyle& fs1 = *ls1.GetComplexFill();
            const GFxFillStyle& fs2 = *ls2.GetComplexFill();
            if (ls.HasComplexFill())
            {
                GFxFillStyle& fs = *ls.GetComplexFill();                
                fs.SetLerp(fs1, fs2, ratio);
            }
            else
            {
                GFxFillStyle* pfs = new GFxFillStyle;
                pfs->SetLerp(fs1, fs2, ratio);
                ls.pComplexFill = pfs;
            }
        }
    }

    // shape
    unsigned pathCount   = 0;
    unsigned rest1 = 0;
    unsigned rest2 = 0;
    UInt     fill0 = 0;
    UInt     fill1 = 0;
    UInt     line  = 0;

    Float ax1 = 0;
    Float ay1 = 0;
    Float ax2 = 0;
    Float ay2 = 0;

    PathsIterator   pathIter1 = pShape1->GetPathsIterator();
    PathsIterator   pathIter2 = pShape2->GetPathsIterator();
    EdgesIterator   edges1, edges2;

    while(!pathIter1.IsFinished() || !pathIter2.IsFinished())
    {
        if(rest1 == 0)
        {
            edges1 = pathIter1.GetEdgesIterator();
            rest1 = edges1.GetEdgesCount();
            edges1.GetMoveXY(&ax1, &ay1);
            pathIter1.GetStyles(&fill0, &fill1, &line);
            pathIter1.Skip();
        }
        if(rest2 == 0)
        {
            edges2 = pathIter2.GetEdgesIterator();
            rest2 = edges2.GetEdgesCount();
            edges2.GetMoveXY(&ax2, &ay2);
            pathIter2.Skip();
        }

        Path& p = Paths[pathCount];
        p.Fill0 = fill0;
        p.Fill1 = fill1;
        p.Line  = line;
        p.Ax    = gflerp(ax1, ax2, ratio);
        p.Ay    = gflerp(ay1, ay2, ratio);

        for(UPInt i = 0, n = p.Edges.size(); i < n; i++)
        {
            EdgesIterator::Edge edge1, edge2;
            edges1.GetEdge(&edge1, true);
            edges2.GetEdge(&edge2, true);
            Edge& edge = p.Edges[i];
            edge.Cx = gflerp(edge1.Cx, edge2.Cx, ratio);
            edge.Cy = gflerp(edge1.Cy, edge2.Cy, ratio);
            edge.Ax = gflerp(edge1.Ax, edge2.Ax, ratio); 
            edge.Ay = gflerp(edge1.Ay, edge2.Ay, ratio); 
        }
        ++pathCount;

        if(rest1 == rest2)
        {
            rest1 = 0;
            rest2 = 0;
        }
        else 
        if(rest1 < rest2)
        {
            rest2 -= rest1;
            rest1  = 0;
        }
        else
        {
            rest1 -= rest2;
            rest2  = 0;
        }
    }

    // display


	// Pass matrix on the stack
	GRenderer::Matrix mat = stackData.stackMatrix;
	GRenderer::Cxform cx = stackData.stackColor;
	
    //GRenderer::Matrix mat;
    //GRenderer::Cxform cx;
    //inst->GetWorldMatrix(&mat);
    //inst->GetWorldCxform(&cx);


    //Float maxError = 20.0f / mat.GetMaxScale() / context.GetPixelScale();
   Float maxError = (20.0f / mat.GetMaxScale()) * context.InvPixelScale;

    bool    meshMismatch    = 0;
    
#ifndef GFC_NO_FXPLAYER_EDGEAA
    // If using EdgeAA, detect when AA settings changed and substitute mesh.
    const GFxRenderConfig &rconfig = *context.GetRenderConfig();
    bool    edgeAADisabled         = (inst->GetClipDepth() > 0);
    bool    useEdgeAA              = (rconfig.IsUsingEdgeAA() &&
                                     (rconfig.IsEdgeAATextured() || !HasTexturedFill()) &&
                                     !edgeAADisabled && !context.MaskRenderCount) ?  1 : 0;
    if (pMesh && (pMesh->IsUsingEdgeAA() != useEdgeAA))
        meshMismatch = 1;
#endif

    if ((ratio != LastRatio) || meshMismatch)
    {
        delete pMesh;
        LastRatio = ratio;

        pMesh = new GFxMeshSet(maxError, maxError * 0.75f, (inst->GetClipDepth() > 0));
        Tesselate(pMesh, pMesh->GetCurveError(), context);
    }
    pMesh->Display(context, mat, cx, inst->GetActiveBlendMode(), FillStyles, LineStyles);
}

void    GFxMorphCharacterDef::Tesselate(GFxCompoundShapeAcceptor *p, Float tolerance,
                                        GFxDisplayContext &context) const
{
    GCompoundShape cs;
    cs.SetCurveTolerance(tolerance);

    // Character shapes may have invalid bounds.
    if (ValidBounds)
        p->SetShapeBounds(Bound, MaxStrokeExtent);

#ifdef GFX_GENERATE_SHAPE_FILE
    // Shape dump logic for external debugging.
    FILE* fd = fopen("shapes", "at");
    fprintf(fd, "=======BeginShape\n");
    GFx_DumpFillStyles(fd, FillStyles);
    GFx_DumpLineStyles(fd, LineStyles);
    fclose(fd);
#endif

    //    for (UInt i = 0, n = Paths.size(); i < n; i++)
    for (UPInt i = 0, n = Paths.size(); i < n; i++)
    {
        if (Paths[i].NewShape == true)
        {
            // Add and tesselate shape, including styles and line strips.
            p->AddTesselatedShape(cs, FillStyles, context);
            // Prepare compound shape for nest set of paths.
            cs.RemoveAll();

#ifdef GFX_GENERATE_SHAPE_FILE
            fd = fopen("shapes", "at");
            fprintf(fd, "!======EndShape\n");
            fprintf(fd, "=======BeginShape\n");
            GFx_DumpFillStyles(fd, FillStyles);
            GFx_DumpLineStyles(fd, LineStyles);
            fclose(fd);
#endif
        }
        else
        {
            Paths[i].AddForTesselation(&cs);

#ifdef GFX_GENERATE_SHAPE_FILE
            // Shape dump logic for external debugging.
            FILE* fd = fopen("shapes", "at");
            fprintf(fd, "Path %d %d %d %f %f\n",
                Paths[i].Fill0 - 1,
                Paths[i].Fill1 - 1,
                Paths[i].Line - 1,
                Paths[i].Ax,
                Paths[i].Ay);

            for (UInt j = 0; j < Paths[i].Edges.size(); j++)
            {
                fprintf(fd, "Curve %f %f %f %f\n",
                    Paths[i].Edges[j].Cx,
                    Paths[i].Edges[j].Cy,
                    Paths[i].Edges[j].Ax,
                    Paths[i].Edges[j].Ay);
            }
            fprintf(fd, "<-------EndPath\n");
            fclose(fd);
#endif
        }
    }

    // Done, store last set of meshes.
    p->AddTesselatedShape(cs, FillStyles, context);

#ifdef GFX_GENERATE_SHAPE_FILE
    fd = fopen("shapes", "at");
    fprintf(fd, "!======EndShape\n");
    fclose(fd);
#endif
}
/*
// Return true if the specified GRenderer::Point is on the interior of our shape.
// Incoming coords are local coords.
bool    GFxMorphCharacterDef::PointTestLocal(const GPointF &pt, bool testShape) const
{
    // Early out.
    if (!Bound.Contains(pt))
        return false;
    // If we are within bounds and not testing shape, return true.
    if (!testShape)
        return true;

    //FILE* fd = fopen("hit", "at");
    //fprintf(fd, "%f %f\n", pt.x, pt.y);
    //fclose(fd);


    // Assuming that the shape coordinates
    // are in TWIPS, we take the curveTolerance=0.5
    // (20*0.5=10). But it has to be redesigned in future.
    GCompoundShape cs;
    cs.SetCurveTolerance(10.0f);

    for (UInt i = 0, n = Paths.size(); i < n; i++)
    {
        if (Paths[i].NewShape == true)
        {
            if (cs.PointInShape(pt.x, pt.y, false))
                return true;
            // Prepare compound shape for next set of paths.
            cs.RemoveAll();
        }
        else
        {
            Paths[i].AddForTesselation(&cs);
        }
    }

    return cs.PointInShape(pt.x, pt.y, false);
}*/

// Return true if the specified GRenderer::Point is on the interior of our shape.
// Incoming coords are local coords.
bool    GFxMorphCharacterDef::PointTestLocal(const GPointF &pt, bool testShape, const GFxPointTestCacheProvider *pcache) const
{
    // Early out.
    if (!Bound.Contains(pt))
        return false;
    // If we are within bounds and not testing shape, return true.
    if (!testShape)
        return true;

    // Assuming that the shape coordinates are in TWIPS,
    // we take the curveTolerance = 1.0 (20*1.0 = 20).
    // But it has to be redesigned in future.
    const Float curveTolerance = 20.0f;
    const Float halfPixel = curveTolerance / 2;

    // Check point coordinate reuse.
    // If coordinates are within the same pixel as last hit-test, return cached value.
    if (pcache)
    {        
        GPointF &lht = pcache->LastHitTest;

        if (((pt.x - halfPixel) < lht.x) &&
            ((pt.x + halfPixel) > lht.x) &&
            ((pt.y - halfPixel) < lht.y) &&
            ((pt.y + halfPixel) > lht.y) )
        {
            return pcache->LastHitTestResult;
        }

        // Store last hit coordinate since we'll be updating it.
        pcache->LastHitTest = pt;     
    }

    bool result = false;
    GFxPointTestCacheProvider::CacheNode* pnode = pcache ? pcache->GetNode() : 0;

    // If no cache node, perform hit-testing logic without cache support.
    // Usually, cache node will be available for preformance-critical cases.
    if (!pnode)
    {
        GCompoundShape cs;
        cs.SetCurveTolerance(curveTolerance);

        for (UInt i = 0; i < Paths.size(); i++)
        {
            if (Paths[i].NewShape == true)
            {
                if (cs.PointInShape(pt.x, pt.y, false))
                {
                    if (pcache)
                        pcache->LastHitTestResult = true;
                    return true;
                }
                // Prepare compound shape for next set of paths.
                cs.RemoveAll();
            }
            else
            {
                Paths[i].AddForTesselation(&cs);
            }
        }

        result = cs.PointInShape(pt.x, pt.y, false);
        if (pcache)
            pcache->LastHitTestResult = result;
        return result;
    }

    // Coordinate not cached, prepare shapes.
    if (!pnode->ShapesValid)
    {        
        UInt i, ishape, shapeCount = 1;

        // Count the number of shapes in ShapeCharacterDef.
        for (i = 0; i < Paths.size(); i++)        
            if (Paths[i].NewShape == true)
                shapeCount++;        

        pnode->Shapes.resize(shapeCount);
        if (pnode->Shapes.size() == shapeCount)
        {
            // Set curve tolerances.
            for (i = 0; i < shapeCount; i++)
                pnode->Shapes[i].SetCurveTolerance(curveTolerance);

            // And initialize shapes.
            ishape = 0;
            for (i = 0; i < Paths.size(); i++)
            {
                if (Paths[i].NewShape == true)
                {
                    ishape++;
                }
                else
                {
                    Paths[i].AddForTesselation(&pnode->Shapes[ishape]);
                }
            }

            pnode->ShapesValid = 1;
        }
    }

    // Run through shapes and do hit-testing.
    UInt ishape;

    for(ishape = 0; ishape < pnode->Shapes.size(); ishape++)
    {
        if (pnode->Shapes[ishape].PointInShape(pt.x, pt.y, false))
        {
            result = true;
            break;
        }
    }

    // Store last hit result and return.
    pcache->LastHitTestResult = result;
    return result;
}

// Find the bounds of this shape, and store them in
// the given rectangle.
void    GFxMorphCharacterDef::ComputeBound(GRectF* r) const
{
    r->Left = 1e10f;
    r->Top = 1e10f;
    r->Right = -1e10f;
    r->Bottom = -1e10f;

    for (UPInt i = 0, n = Paths.size(); i < n; i++)
    {
        const Path&  p = Paths[i];
        r->ExpandToPoint(p.Ax, p.Ay);
        for (UPInt j = 0, jn = p.Edges.size(); j < jn; j++)
        {
            r->ExpandToPoint(p.Edges[j].Ax, p.Edges[j].Ay);
            //                  r->ExpandToPoint(p.Edges[j].Cx, p.Edges[j].Cy);
        }
    }
}

// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeCharacterDef contains.
void    GFxMorphCharacterDef::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    cs->RemoveAll();
    cs->SetCurveTolerance(tolerance);
    for (UPInt i = 0, n = Paths.size(); i < n; i++)
    {
        Paths[i].AddForTesselation(cs);
    }
}

void    GFxMorphCharacterDef::ReadMorphFillStyle(GFxLoadProcess* p, GFxTagType tagType,
                                                 GFxFillStyle& fs1, GFxFillStyle& fs2)
{
    GFxStream* in = p->GetStream();

    fs1.Type = in->ReadU8();
    fs2.Type = fs1.Type;

    in->LogParse("morph fill style type = 0x%X\n", fs1.Type);

    if (fs1.IsSolidFill())
    {
        GColor c1, c2;
        in->ReadRgba(&c1);
        in->ReadRgba(&c2);
        fs1.SetColor(c1);
        fs2.SetColor(c2);

        in->LogParse("morph fill style begin color: "); 
        in->LogParseClass(c1);
        in->LogParse("morph fill style end color: "); 
        in->LogParseClass(c2);
    }

    else if (fs1.IsGradientFill())
    {
        in->ReadMatrix(&fs1.ImageMatrix);
        in->ReadMatrix(&fs2.ImageMatrix);

        // GRADIENT
        UByte  numGradients = in->ReadU8();
        bool   linearRGB    = 0;

        // For DefineMorphShape2 gradient records are interpreted differently.
        if ((tagType == GFxTag_DefineShapeMorph2) ||
            (fs1.GetType() == GFxFill_FocalPointGradient))
        {
            if (numGradients & 0x10)
                linearRGB = 1;
            // TBD: Other bits may be set here, such as 0x80 for focal point
            // Do they need custom processing?
            numGradients &= 0xF;
        }

        GASSERT(numGradients >= 1 && numGradients <= 15);
        fs1.pGradientData = new GFxGradientData((GFxFillType)fs1.Type, numGradients, linearRGB);
        fs2.pGradientData = new GFxGradientData((GFxFillType)fs2.Type, numGradients, linearRGB);
        // Mem check
        if (!fs1.pGradientData || !fs2.pGradientData)
        {
            GASSERT(0);
            fs1.Type = GFxFill_Solid;
            fs2.Type = GFxFill_Solid;
            return;
        }

        for (UInt j = 0; j < numGradients; j++)
        {
            (*fs1.pGradientData)[j].Read(p, tagType);
            (*fs2.pGradientData)[j].Read(p, tagType);
        }

        in->LogParse("morph fsr: numGradients = %d\n", numGradients);

        // SWF 8: Focal point location can morph, so read in those locations.
        if (fs1.Type == GFxFill_FocalPointGradient)
        {
            fs1.pGradientData->SetFocalRatio( (Float) (in->ReadS16()) / 256.0f );
            fs2.pGradientData->SetFocalRatio( (Float) (in->ReadS16()) / 256.0f );
        }        
    }

    else if (fs1.IsImageFill())
    {
        int bitmapCharId = in->ReadU16();
        in->LogParse("morph fsr BitmapChar = %d\n", bitmapCharId);

        // Look up the bitmap character.
        p->GetResourceHandle(&fs1.pImage, GFxResourceId(bitmapCharId));
        fs2.pImage = fs1.pImage;
        
        in->ReadMatrix(&fs1.ImageMatrix);
        in->ReadMatrix(&fs2.ImageMatrix);
    }
}

void    GFxMorphCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType, bool withStyle)
{
    GUNUSED2(tagType, withStyle);

    GFxStream* in = p->GetStream();

    GRectF  bound1, bound2;
    GRectF  rbound1, rbound2;
    in->ReadRect(&bound1);
    in->ReadRect(&bound2);

    // SWF 8 includes extra rectangle bounds.
    if (tagType == 84)
    {
        in->ReadRect(&rbound1);
        in->ReadRect(&rbound2);

        // MA: What does this byte do?
        // It normally takes on a value of 0x01.
        // Seems similar to a similar byte in tag 83 (DefinedShape5).
        //UByte mysteryByte = 
        in->ReadU8();
    }
    else
    {
        rbound1 = bound1;
        rbound2 = bound2;
    }

    pShape1->SetBound(bound1);
    pShape2->SetBound(bound2);
    pShape1->SetRectBound(rbound1);
    pShape2->SetRectBound(rbound2);


    Offset = in->ReadU32();

    FillStyleCount = in->ReadVariableCount();
    
    UInt i;
    for (i = 0; i < FillStyleCount; i++)
    {
        GFxFillStyle fs1, fs2;

		ReadMorphFillStyle(p, tagType, fs1, fs2);

        pShape1->FillStyles.push_back(fs1);
        pShape2->FillStyles.push_back(fs2);
    }

    LineStyleCount = in->ReadVariableCount();
    for (i = 0; i < LineStyleCount; i++)
    {
        pShape1->LineStyles.resize(pShape1->LineStyles.size() + 1);
        pShape2->LineStyles.resize(pShape2->LineStyles.size() + 1);
        GFxLineStyle& ls1 = pShape1->LineStyles[pShape1->LineStyles.size() - 1];
        GFxLineStyle& ls2 = pShape2->LineStyles[pShape2->LineStyles.size() - 1];

        ls1.Width = in->ReadU16();
        ls2.Width = in->ReadU16();

        // SWF 8 - includes line style.
        if (tagType == GFxTag_DefineShapeMorph2)
        {   
            // There is only one line style, since it cannot change in a morph.
            ls1.StyleFlags = ls2.StyleFlags = in->ReadU16();

            // Miter is followed by a special 8.8 pixed point miter value.
            if (ls1.StyleFlags & GFxLineStyle::LineJoin_Miter)
            {
        #ifndef GFC_NO_FXPLAYER_STROKER
                ls1.MiterSize = ls2.MiterSize = (Float)(in->ReadU16()) / 256.0f;
        #else
                in->ReadU16();
        #endif
            }
        }

        if (ls1.StyleFlags & GFxLineStyle::LineFlag_ComplexFill)
        {
            GFxFillStyle fs1, fs2;
            ReadMorphFillStyle(p, tagType, fs1, fs2);
            
            ls1.SetComplexFill(fs1);
            ls1.Color = fs1.IsGradientFill() ? (*fs1.GetGradientData())[0].Color : GColor();
            ls2.SetComplexFill(fs2);
            ls2.Color = fs2.IsGradientFill() ? (*fs2.GetGradientData())[0].Color : GColor();
        }
        else
        {
            p->ReadRgbaTag(&ls1.Color, tagType);
            p->ReadRgbaTag(&ls2.Color, tagType);
        }
    }

    pShape1->Read(p, tagType, false);
    in->Align();
    pShape2->Read(p, tagType, false);

    GASSERT(pShape1->FillStyles.size() == pShape2->FillStyles.size());
    GASSERT(pShape1->LineStyles.size() == pShape2->LineStyles.size());

    // setup GTL::garray size
    FillStyles.resize(pShape1->FillStyles.size());
    for (i = 0; i < FillStyles.size(); i++)
    {
        GFxFillStyle& fs = FillStyles[i];
        GFxFillStyle& fs1 = pShape1->FillStyles[i];

        // Copy type and clear data. The rest will be
        // initialized during a morph.
        fs.Type = fs1.Type;
        fs.pGradientData = 0;
        fs.pImage = 0;
    }
    LineStyles.resize(pShape1->LineStyles.size());

/*
if(pShape1->Paths.size() == 6)
{
FILE* fd = fopen("shapemorph", "at");
fprintf(fd, "=======BeginShape\n");
for (i = 0; i < pShape1->Paths.size(); i++)
{
    const GFxPath& p = pShape1->Paths[i];
    fprintf(fd, "Path %d %d %d %f %f\n", p.Fill0 - 1, p.Fill1 - 1, p.Line - 1, p.Ax, p.Ay);
    unsigned j;
    for(j = 0; j < p.Edges.size(); j++)    
    {
        const GFxEdge& e = p.Edges[j];
        fprintf(fd, "Curve %f %f %f %f\n", e.Cx, e.Cy, e.Ax, e.Ay);
    }
    fprintf(fd, "<-------EndPath\n");
}
fprintf(fd, "!======EndShape\n");
fprintf(fd, "=======BeginShape\n");
for (i = 0; i < pShape2->Paths.size(); i++)
{
    const GFxPath& p = pShape2->Paths[i];
    fprintf(fd, "Path %d %d %d %f %f\n", p.Fill0 - 1, p.Fill1 - 1, p.Line - 1, p.Ax, p.Ay);
    unsigned j;
    for(j = 0; j < p.Edges.size(); j++)    
    {
        const GFxEdge& e = p.Edges[j];
        fprintf(fd, "Curve %f %f %f %f\n", e.Cx, e.Cy, e.Ax, e.Ay);
    }
    fprintf(fd, "<-------EndPath\n");
}
fprintf(fd, "!======EndShape\n");
fclose(fd);
}
*/

    // Paths of shapes might be empty in the case if one (or both) shapes
    // were removed after tween is created. This is not a problem.
    //GASSERT(pShape1->Paths.size() > 0);
    //GASSERT(pShape2->Paths.size() > 0);

    // Pre-allocate the resulting path and normalize it. 
    // The trick here is as follows.
    // Although, both shapes must have exactly the same number of edges
    // they can have different number of paths! That's a bad news because
    // it complicates morphing a lot. On the picture below there's 
    // an example of the situation:
    // Shape1:          *----------*----------
    // Shape2:          *------*-------*------
    // Resulting Shape: *------*---*---*------
    // Here '*' indicates new path, '-' is an edge. That is, for correct 
    // morphing the initial and target shapes must be split and the 
    // resulting shapes can have more paths than Shape1 or Shape2.
    //--------------------------------------
    //unsigned pathCount1 = 0;
    //unsigned pathCount2 = 0;
    unsigned rest1      = 0;
    unsigned rest2      = 0;
    unsigned edgeCount1 = 0;
    unsigned edgeCount2 = 0;
    PathsIterator   pathIter1 = pShape1->GetPathsIterator();
    PathsIterator   pathIter2 = pShape2->GetPathsIterator();

    Paths.resize(0);

    while(!pathIter1.IsFinished() || !pathIter2.IsFinished())
    {
        Paths.push_back(Path());

        // Advance paths.
        if(rest1 == 0)
        {
            rest1 = pathIter1.GetEdgesCount();
            edgeCount1 += rest1;
            pathIter1.Skip();
        }
        if(rest2 == 0)
        {
            rest2 = pathIter2.GetEdgesCount();
            edgeCount2 += rest2;
            pathIter2.Skip();
        }

        // Determine the destination path size
        if(rest1 == rest2)
        {
            Paths[Paths.size() - 1].Edges.resize(rest1);
            rest1 = 0;
            rest2 = 0;
        }
        else 
        if(rest1 < rest2)
        {
            Paths[Paths.size() - 1].Edges.resize(rest1);
            rest2 -= rest1;
            rest1  = 0;
        }
        else
        {
            Paths[Paths.size() - 1].Edges.resize(rest2);
            rest1 -= rest2;
            rest2  = 0;
        }
    }
    GASSERT(edgeCount1 == edgeCount2);
}
