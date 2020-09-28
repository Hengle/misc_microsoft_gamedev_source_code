/**********************************************************************

Filename    :   GFxShape.cpp
Content     :   Shape character definition with paths and edges
Created     :
Authors     :   Michael Antonov, Artem Bolgar

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.
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
#include "GFxShape.h"
#include "GFxMesh.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "GRenderer.h"
#include "GTessellator.h"
#include "GStroker.h"

#include "GFxLoadProcess.h"

#include "GFxImageResource.h"
#include "GFxDisplayContext.h"

#include <float.h>
#include <stdlib.h>

//#if defined(GFC_OS_WIN32)
  //  #include <windows.h>
//#endif

#ifdef GFC_BUILD_DEBUG
//#define GFX_SHAPE_MEM_TRACKING
#endif  //GFC_BUILD_DEBUG

#ifdef GFX_SHAPE_MEM_TRACKING
UInt GFx_MT_SwfGeomMem = 0;
UInt GFx_MT_PathsCount = 0;
UInt GFx_MT_GeomMem = 0;
UInt GFx_MT_ShapesCount = 0;
#endif //GFX_SHAPE_MEM_TRACKING

// Debugging macro: Define this to generate "shapes" file, used for
// shape extraction and external tessellator testing.
//#define    GFX_GENERATE_SHAPE_FILE
//#define    GFX_GENERATE_STROKE_SHAPE_FILE

//
// Shape I/O helper functions.
//

// Read fill styles, and push them onto the given style GTL::garray.
static int GFx_ReadFillStyles(GTL::garray<GFxFillStyle>* styles, GFxLoadProcess* p, GFxTagType tagType)
{
    //GASSERT(styles);

    // Get the count.
    UInt fillStyleCount = p->ReadU8();
    if (tagType > GFxTag_DefineShape)
    {
        if (fillStyleCount == 0xFF)
            fillStyleCount = p->ReadU16();
    }
    int off = p->Tell();

    if (styles)
    {
        p->LogParse("  GFx_ReadFillStyles: count = %d\n", fillStyleCount);

        // Read the styles.
        UInt baseIndex = (UInt)styles->size();
        if (fillStyleCount)
            styles->resize(baseIndex + fillStyleCount);
        for (UInt i = 0; i < fillStyleCount; i++)
        {
            p->AlignStream(); //!AB
            (*styles)[baseIndex + i].Read(p, tagType);
        }
    }
    else if (fillStyleCount > 0)
    {
        p->LogError("Error: GFx_ReadFillStyles, trying to read %d fillstyles into no-style shape\n", fillStyleCount);
    }
    return off;
}

// Read line styles and push them onto the back of the given GTL::garray.
static int GFx_ReadLineStyles(GTL::garray<GFxLineStyle>* styles, GFxLoadProcess* p, GFxTagType tagType)
{
    //GASSERT(styles);

    // Get the count.
    UInt lineStyleCount = p->ReadU8();

    p->LogParse("  GFx_ReadLineStyles: count = %d\n", lineStyleCount);

    // @@ does the 0xFF flag apply to all tag types?
    // if (TagType > GFxTag_DefineShape)
    // {
    if (lineStyleCount == 0xFF)
    {
        lineStyleCount = p->ReadU16();
        p->LogParse("  GFx_ReadLineStyles: count2 = %d\n", lineStyleCount);
    }
    // }
    int off = p->Tell();

    if (styles)
    {
        // Read the styles.
        UInt baseIndex = (UInt)styles->size();
        styles->resize(baseIndex + lineStyleCount);

        for (UInt i = 0; i < lineStyleCount; i++)
        {
            p->AlignStream(); //!AB
            (*styles)[baseIndex + i].Read(p, tagType);
        }
    }
    else if (lineStyleCount > 0)
    {
        p->LogError("Error: GFx_ReadLineStyles, trying to read %d linestyles into no-style shape\n", lineStyleCount);
    }
    return off;
}

static GINLINE UInt GFx_GetFloatExponent(Float scale)
{
    union { Float f; UInt32 i; } u;
    u.f = scale;
    return (u.i >> 23) & 0xFF;
}

static GINLINE int GFx_ComputeShapeScaleKey(Float scale, bool useEdgeAA)
{
    // Compute the scaleKey. It is calculated empirically, as an approximate
    // binary logarithm of the overall scale (world-to-pixels). 
    // The direct use of the scale produces a new key for approximately 
    // every 2x scale value. scale*0.707 is a bias, which means the use
    // of the same scale key from 0.707 to 1.41. 
    //
    // For more accurate EdgeAA use:
    // ts2 = scale * 0.841; GFx_GetFloatExponent(ts2*ts2);     // Scales 0.841...1.189
    // ts2 = scale * 0.891; GFx_GetFloatExponent(ts2*ts2*ts2); // Scales 0.891...1.122
    //-------------------
    if (useEdgeAA)
    {
        Float ts2 = scale * 0.841f;
        return GFx_GetFloatExponent(ts2*ts2);
    }

    // When EdgeAA is not used, the accuracy requirements are less strict. 
    // It is good enough to have a mesh for every x4 scaling value.
    //-------------------
    return  GFx_GetFloatExponent(scale) / 2 * 2; 
}

// Implementation of virtual path iterator, that may be used for shapes
// with unknown or mixed path data formats.
//
template<class ShapeDef, class PathData>
class GFxVirtualPathIterator : public GFxShapeCharacterDef::PathsIterator
{
    typedef typename PathData::PathsIterator NativePathsIterator;
    typedef typename PathData::EdgesIterator NativeEdgesIterator;
    NativePathsIterator PathIter;
    NativeEdgesIterator EdgeIter;
    StateInfo State;
public:
    GFxVirtualPathIterator(const ShapeDef* pdef) :
    PathIter(pdef->GetNativePathsIterator())
    { 
        if(PathIter.IsFinished())
        {
            State.State = StateInfo::St_Finished;
        }
        else
        {
            State.State = StateInfo::St_Setup;
            PathIter.GetStyles(&State.Setup.Fill0, &State.Setup.Fill1, &State.Setup.Line);
            EdgeIter = PathIter.GetEdgesIterator();
            EdgeIter.GetMoveXY(&State.Setup.MoveX, &State.Setup.MoveY);
        }
    }
    virtual bool GetNext(StateInfo* pcontext)
    {
        *pcontext = State;
        switch(State.State)
        {
        case StateInfo::St_Setup:
        case StateInfo::St_Edge:
            if (EdgeIter.IsFinished())
            {
                State.State = StateInfo::St_NewPath;
                PathIter.AdvanceBy(EdgeIter);
            }
            else
            {
                typename NativeEdgesIterator::Edge edge;
                EdgeIter.GetEdge(&edge);
                State.State = StateInfo::St_Edge;
                State.Edge.Cx = edge.Cx;
                State.Edge.Cy = edge.Cy;
                State.Edge.Ax = edge.Ax;
                State.Edge.Ay = edge.Ay;
                State.Edge.Curve = edge.Curve;
            }
            break;
        case StateInfo::St_NewPath:
            if (PathIter.IsFinished())
                State.State = StateInfo::St_Finished;
            else
            {
                if (PathIter.IsNewShape())
                    PathIter.Skip();
                State.State = StateInfo::St_Setup;
                PathIter.GetStyles(&State.Setup.Fill0, &State.Setup.Fill1, &State.Setup.Line);
                EdgeIter = NativeEdgesIterator(PathIter);
                EdgeIter.GetMoveXY(&State.Setup.MoveX, &State.Setup.MoveY);
            }
            break;
        default:;
        }
        return pcontext->State != StateInfo::St_Finished;
    }
};

//////////////////////////////////////////////////////////////////////////
// GFxShapeCharacterDef
//
GFxShapeCharacterDef::GFxShapeCharacterDef()
{
    Bound = GRectF(0,0,0,0);
    PreTessellatedScaleKey = GFC_MAX_SINT32;
    Flags = 0;
    HintedGlyphSize = 0;
}

GFxShapeCharacterDef::~GFxShapeCharacterDef()
{
    ResetCache();
}

void GFxShapeCharacterDef::ResetCache()
{
    // Free our MeshSets.
    for (int i = CachedMeshes.GetLowerBound(); i <= CachedMeshes.GetUpperBound(); i++)
    {
        delete CachedMeshes[i];
        CachedMeshes[i] = 0;
    }
    PreTessellatedScaleKey = GFC_MAX_SINT32;
}

void GFxShapeCharacterDef::ResetS9GCache(MeshSetPtrArray*& pcachedMeshesS9g)
{
    if (pcachedMeshesS9g)
    {
        for (UPInt j = 0, n = pcachedMeshesS9g->size(); j < n; ++j)
        {
            delete (*pcachedMeshesS9g)[j];
        }
        pcachedMeshesS9g->clear();
        delete pcachedMeshesS9g;
        pcachedMeshesS9g = NULL;
    }
}

const GFxFillStyle* GFxShapeCharacterDef::GetFillStyles(UInt* pstylesNum) const
{
    *pstylesNum = 0;
    return NULL;
}

const GFxLineStyle* GFxShapeCharacterDef::GetLineStyles(UInt* pstylesNum) const
{
    *pstylesNum = 0;
    return NULL;
}

void GFxShapeCharacterDef::GetFillAndLineStyles
    (const GFxFillStyle** ppfillStyles, UInt* pfillStylesNum, 
     const GFxLineStyle** pplineStyles, UInt* plineStylesNum) const
{
    *ppfillStyles   = NULL;
    *pfillStylesNum = 0;
    *pplineStyles   = NULL;
    *plineStylesNum = 0;
}

void GFxShapeCharacterDef::GetFillAndLineStyles(GFxDisplayParams* pparams) const
{
    pparams->pFillStyles = NULL;
    pparams->pLineStyles = NULL;
    pparams->FillStylesNum = 0;
    pparams->LineStylesNum = 0;
}

GRectF GFxShapeCharacterDef::GetBoundsLocal() const
{
    return Bound;
}

void    GFxShapeCharacterDef::ApplyScale9Grid(GCompoundShape* shape,
                                         const GFxScale9GridInfo& sg)
{
    UInt i;
    for (i = 0; i < shape->GetNumVertices(); ++i)
    {
        GPointType& v = shape->GetVertex(i);
        sg.Transform(&v.x, &v.y);
    }
}

// Draw the shape using our own inherent styles.
void    GFxShapeCharacterDef::Display(GFxDisplayContext &context, GFxCharacter* inst, StackData stackdata)
{   
   GFxDisplayParams params(context, stackdata.stackMatrix, stackdata.stackColor, inst->GetActiveBlendMode());
 
   /*
    GFxDisplayParams params(context, inst->GetWorldMatrix(), 
                            inst->GetWorldCxform(), inst->GetActiveBlendMode());
                            */

    // Do viewport culling if bounds are available.
    if (HasValidBounds())
    {
        GRectF           tbounds;

        params.Mat.EncloseTransform(&tbounds, Bound);
        if (!inst->GetMovieRoot()->GetVisibleFrameRectInTwips().Intersects(tbounds))
        {
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;
        }
    }

    if (Flags & Flags_StylesSupport)
    {
        // fill fill and line styles
        GetFillAndLineStyles(&params.pFillStyles, &params.FillStylesNum, 
                             &params.pLineStyles, &params.LineStylesNum);
    }
    Display(params, (inst->GetClipDepth() > 0), inst);
}

// Display our shape.  Use the FillStyles arg to
// override our default set of fill Styles (e.G. when
// rendering text).
void    GFxShapeCharacterDef::Display (GFxDisplayParams& params,
                                       bool edgeAADisabled,
                                       GFxCharacter* inst)
{

    const GFxRenderConfig &rconfig = *params.Context.GetRenderConfig();

#ifdef GFC_NO_FXPLAYER_EDGEAA
    GUNUSED(edgeAADisabled);
    bool    useEdgeAA   = 0;
#else
    bool    useEdgeAA   =   rconfig.IsUsingEdgeAA() &&
        (rconfig.IsEdgeAATextured() || !HasTexturedFill()) &&
        !edgeAADisabled && params.Context.MaskRenderCount == 0;
#endif

    GFxMeshSet* meshSet;
    GPtr<GFxScale9GridInfo> s9g;

    if (inst && inst->DoesScale9GridExist())
        s9g  = *inst->CreateScale9Grid(params.Context.GetPixelScale() * rconfig.GetMaxCurvePixelError());

    // Check to see if it's possible to use the pre-tessellated mesh. 
    // The pre-tessellated mesh must exist, plus there should not be 
    // scale9grid, plus, if the shape is used as a mask, there should not 
    // be EdgeAA.
    if (PreTessellatedScaleKey != GFC_MAX_SINT32 && s9g.GetPtr() == 0)
    {
        meshSet = CachedMeshes[PreTessellatedScaleKey];
        if (meshSet && useEdgeAA == meshSet->IsUsingEdgeAA())
        {
            meshSet->Display(params, false);
            return;
        }
    }

    // Compute the error tolerance in object-space.
    Float   maxScale = params.Mat.GetMaxScale();

    //Float sx = mat.M_[0][0] + mat.M_[0][1];
    //Float sy = mat.M_[1][0] + mat.M_[1][1];
    //Float maxScale = sqrtf(sx*sx + sy*sy) * 0.707106781f;

    if (fabsf(maxScale) < 1e-6f)
    {
        // Scale is essentially zero.
        return;
    }

    bool    cxformHasAddAlpha = (fabs(params.Cx.M_[3][1]) >= 1.0f);

    // If Alpha is Zero, don't draw.
    // Though, if the mask is being drawn, don't consider the alpha. Tested with MovieClip.setMask (!AB)
    if ((fabs(params.Cx.M_[3][0]) < 0.001f) && !cxformHasAddAlpha && params.Context.MaskRenderCount == 0)
        return;

    Float masterScale = params.Context.GetPixelScale() * maxScale;
    int   scaleKey    = GFx_ComputeShapeScaleKey(masterScale * rconfig.GetMaxCurvePixelError(), useEdgeAA);

    meshSet = 0;
    Float scale9GridKey[GFxScale9GridInfo::KeySize];
    UInt  s9gIdx = ~0U;

    // Find proper meshSet. If scale9grid is in use we look for the 
    // meshSet whose key fits the given scale9grid. 
    // Otherwise just use the mesh that corresponds to the scaleKey. 
    //-------------------
    MeshSetPtrArray* pcachedMeshesS9g = NULL;
    if ((Flags & Flags_S9GSupport) && s9g.GetPtr() && (pcachedMeshesS9g = GetCachedMeshesS9G()) != NULL)
    {
        // When using scale9Grid, clean up all regular meshSets.
        for (int m = CachedMeshes.GetLowerBound(); m <= CachedMeshes.GetUpperBound(); m++)
        {
            delete CachedMeshes[m];
            CachedMeshes[m] = 0;
        }

        s9g->MakeKey(scale9GridKey);
        for (UInt i = 0, n = (UInt)pcachedMeshesS9g->size(); i < n; ++i)
        {
            // Check to see if the mesh fits the given scale9grid key.
            //----------------------
            GFxMeshSet* m2 = (*pcachedMeshesS9g)[i];
            if (m2->Scale9GridFits(scale9GridKey))
            {
                meshSet = m2;
                s9gIdx  = i;
                break;
            }

            // The scaling grid uses only one GFxMeshSet per instance to avoid 
            // excessive memory consumption. So, when we found the situation the 
            // mesh does not fit the scale9grid, and the mesh belongs to the same 
            // instance we delete the mesh and re-tessellate it.
            // It means that animated scale9grid-ed shapes will always be 
            // re-tessellated. Different instances create different meshes unless 
            // the scale9GridKeys are the same. When the instance is deleted the 
            // respective mesh still exists in memory. 
            //-----------------------
            if (inst == m2->GetCharacterInstance())
            {
                pcachedMeshesS9g->remove(i);
                delete m2;
                break;
            }
        }
    }
    else
    {
        meshSet = CachedMeshes[scaleKey];
    }

    if (meshSet)
    {
        // The mesh is found. Here we have to check if it fits the existing
        // visual parameters. If not, remove the mesh and re-tessellate it.
        // If it fits, just display it and hasta la vista.
        //-----------------------
        if ((cxformHasAddAlpha && !meshSet->GetAllowCxformAddAlpha()) ||
            (meshSet->IsUsingEdgeAA() != useEdgeAA && !meshSet->HasEdgeAAFailed()) ||
            (rconfig.IsOptimizingTriangles() != meshSet->IsOptimizingTriangles()))
        {
            delete meshSet;
            if (pcachedMeshesS9g)
                pcachedMeshesS9g->remove(s9gIdx);
            else
                CachedMeshes[scaleKey] = 0;
        }
        else
        {
            meshSet->Display(params, (pcachedMeshesS9g != NULL));
            return;
        }
    }

    // Construct A new GFxMesh to handle this pixel size and the 
    // other visual parameters. Then tessellate it and store in
    // the proper cache, depending on whether or not Scale9grid is in use.
    //----------------
    Float screenPixelSize = 20.0f / masterScale;
    Float curveError      = screenPixelSize * 0.75f * rconfig.GetMaxCurvePixelError();
    meshSet = new GFxMeshSet(screenPixelSize, 
        curveError, 
        !useEdgeAA, 
        rconfig.IsOptimizingTriangles());

#ifndef GFC_NO_FXPLAYER_EDGEAA
    meshSet->SetAllowCxformAddAlpha(cxformHasAddAlpha);
#endif
    Tessellate(meshSet, curveError, params.Context, s9g);
    if (pcachedMeshesS9g)
    {
        meshSet->SetScale9GridKey(scale9GridKey, inst);
        pcachedMeshesS9g->push_back(meshSet);
    }
    else
    {
        CachedMeshes[scaleKey] = meshSet;
    }

    meshSet->Display(params, pcachedMeshesS9g != 0);

    // DBG
    //printf("T");
}

// Find the bounds of this shape, and store them in
// the given rectangle.
template <class PathData>
void    GFxShapeCharacterDef::ComputeBoundImpl(GRectF* r) const
{
    r->Left = 1e10f;
    r->Top = 1e10f;
    r->Right = -1e10f;
    r->Bottom = -1e10f;

    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            it.Skip();
            continue;
        }

        typename PathData::EdgesIterator edgesIt = it.GetEdgesIterator();
        Float ax, ay;
        edgesIt.GetMoveXY(&ax, &ay);
        r->ExpandToPoint(ax, ay);

        while (!edgesIt.IsFinished())
        {
            typename PathData::EdgesIterator::Edge edge;
            edgesIt.GetEdge(&edge);
            if (edge.Curve)
            {
                Float t, x, y;
                t = GMath2D::CalcQuadCurveExtremum(ax, edge.Cx, edge.Ax);
                if (t > 0 && t < 1)
                {
                    GMath2D::CalcPointOnQuadCurve(ax, ay, 
                        edge.Cx, edge.Cy, 
                        edge.Ax, edge.Ay,
                        t, &x, &y);
                    r->ExpandToPoint(x, y);
                }
                t = GMath2D::CalcQuadCurveExtremum(ay, edge.Cy, edge.Ay);
                if (t > 0 && t < 1)
                {
                    GMath2D::CalcPointOnQuadCurve(ax, ay, 
                        edge.Cx, edge.Cy, 
                        edge.Ax, edge.Ay,
                        t, &x, &y);
                    r->ExpandToPoint(x, y);
                }

            }
            r->ExpandToPoint(edge.Ax, edge.Ay);
            ax = edge.Ax;
            ay = edge.Ay;
        }
        it.AdvanceBy(edgesIt);
    }
}


// Return true if the specified GRenderer::Point is on the interior of our shape.
// Incoming coords are local coords.
template <class PathData>
bool    GFxShapeCharacterDef::DefPointTestLocalImpl(const GPointF &pt, 
                                                    bool testShape, 
                                                    const GFxCharacter* pinst) const
{
    const GFxPointTestCacheProvider *pcache = (pinst) ? pinst->GetPointTestCacheProvider() : NULL;

    GPtr<GFxScale9GridInfo> s9g;

    if (pinst && pinst->DoesScale9GridExist())
    {
        s9g = *pinst->CreateScale9Grid(1.0f);
        if (s9g.GetPtr())
            s9g->Compute();
    }

    GRectF b2 = Bound;
    if (!HasValidBounds())
        ComputeBound(&b2);

    if (s9g.GetPtr())
        b2 = s9g->AdjustBounds(b2);

    // Early out.
    if (!b2.Contains(pt))
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
    if (pcache && HasValidHitResult())
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
    // Usually, cache node will be available for performance-critical cases.
    // Scale9Grid also requires explicit checking without caching.
    if (!pnode || s9g.GetPtr())
    {
        GCompoundShape cs;
        cs.SetCurveTolerance(curveTolerance);

        typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
        while(!it.IsFinished())
        {
            if (it.IsNewShape())
            {
                if (s9g.GetPtr())
                    ApplyScale9Grid(&cs, *s9g);

                if (PointInShape(cs, pt.x, pt.y))
                {
                    if (pcache)
                    {
                        pcache->LastHitTest       = pt;
                        pcache->LastHitTestResult = true;
                    }
                    SetValidHitResultFlag(true);
                    return true;
                }
                // Prepare compound shape for next set of paths.
                cs.RemoveAll();
                it.Skip();
            }
            else
            {
                it.AddForTessellation(&cs);
            }
        }

        if (s9g.GetPtr())
            ApplyScale9Grid(&cs, *s9g);

        result = PointInShape(cs, pt.x, pt.y);
        if (pcache)
        {
            pcache->LastHitTest       = pt;
            pcache->LastHitTestResult = result;
        }
        SetValidHitResultFlag(true);
        return result;
    }

    // Coordinate not cached, prepare shapes.
    if (!pnode->ShapesValid)
    {        
        UInt i, ishape, shapesCnt;

        GetShapeAndPathCounts(&shapesCnt, NULL);
        pnode->Shapes.resize(shapesCnt);
        if (pnode->Shapes.size() == shapesCnt)
        {
            // Set curve tolerances.
            for (i = 0; i < shapesCnt; i++)
                pnode->Shapes[i].SetCurveTolerance(curveTolerance);

            // And initialize shapes.
            ishape = 0;
            typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
            while(!it.IsFinished())
            {
                if (it.IsNewShape())
                {
                    ishape++;
                    it.Skip();
                }
                else
                {
                    it.AddForTessellation(&pnode->Shapes[ishape]);
                }
            }

            pnode->ShapesValid = 1;
        }
    }

    // Run through shapes and do hit-testing.
    UInt ishape;

    for(ishape = 0; ishape < pnode->Shapes.size(); ishape++)
    {
        if (PointInShape(pnode->Shapes[ishape], pt.x, pt.y))
        {
            result = true;
            break;
        }
    }

    // Store last hit result and return.
    pcache->LastHitTest       = pt;
    pcache->LastHitTestResult = result;
    SetValidHitResultFlag(true);
    return result;
}


bool GFxShapeCharacterDef::PointInShape(const GCompoundShape& shape, Float x, Float y) const
{
    if (shape.PointInShape(x, y, false))
        return true;

#ifndef GFC_NO_FXPLAYER_STROKER
    if (Flags & Flags_StylesSupport)
    {
        GStroker        stroker;
        GCompoundShape  strokeShape;
        UInt            lineStylesNum = 0;
        const GFxLineStyle* lineStyles = GetLineStyles(&lineStylesNum);

        for (UInt i = 0; i < lineStylesNum; i++)
        {
            const GFxLineStyle& style = lineStyles[i];

            stroker.SetWidth(style.GetWidth());
            stroker.SetLineJoin(GFx_SWFToFxStroke_LineJoin(style.GetJoin()));
            stroker.SetStartLineCap(GFx_SWFToFxStroke_LineCap(style.GetStartCap()));
            stroker.SetEndLineCap(GFx_SWFToFxStroke_LineCap(style.GetEndCap()));
            if (style.GetJoin() == GFxLineStyle::LineJoin_Miter) 
                stroker.SetMiterLimit(style.GetMiterSize());

            strokeShape.RemoveAll();
            strokeShape.SetCurveTolerance(shape.GetCurveTolerance());
            stroker.GenerateStroke(shape, (int)i, strokeShape);
            if (strokeShape.PointInShape(x, y, true))
                return true;
        }
    }
#endif
    return false;
}


// Push our shape data through the tessellator.
template<class PathData>
void GFxShapeCharacterDef::TessellateImpl(GFxMeshSet *meshSet, Float tolerance,
                                          GFxDisplayContext &context,
                                          GFxScale9GridInfo* s9g,
                                          Float maxStrokeExtent) const
{
    GCompoundShape cs;
    cs.SetCurveTolerance(tolerance);
    cs.SetNonZeroFill(IsNonZeroFill());

    // Character shapes may have invalid bounds.
    if (HasValidBounds())
        meshSet->SetShapeBounds(Bound, maxStrokeExtent);

#ifdef GFX_GENERATE_SHAPE_FILE
    // Shape dump logic for external debugging.
    FILE* fd = fopen("shapes", "at");
    fprintf(fd, "=======BeginShape\n");
    GFx_DumpFillStyles(fd, GetFillStyles());
    GFx_DumpLineStyles(fd, GetLineStyles());
    fclose(fd);
#endif

    if (s9g)
    {
        s9g->Compute();
        if (HasValidBounds())
            meshSet->SetShapeBounds(s9g->AdjustBounds(Bound), maxStrokeExtent);
    }

    //PathData::PathsIterator it = GetPathsIterator<PathData>();
    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            // Process the compound shape and prepare it for next set of paths.
            AddShapeToMesh(meshSet, &cs, context, s9g);
            cs.RemoveAll();

#ifdef GFX_GENERATE_SHAPE_FILE
            fd = fopen("shapes", "at");
            fprintf(fd, "!======EndShape\n");
            fprintf(fd, "=======BeginShape\n");
            GFx_DumpFillStyles(fd, GetFillStyles());
            GFx_DumpLineStyles(fd, GetLineStyles());
            fclose(fd);
#endif

            it.Skip();
        }
        else
        {
#ifdef GFX_GENERATE_SHAPE_FILE
            // Shape dump logic for external debugging.
            FILE* fd = fopen("shapes", "at");
            {
                EdgesIterator eit = it.GetEdgesIterator();

                Float ax, ay;
                UInt fill0, fill1, line;
                it.GetStyles(&fill0, &fill1, &line);
                eit.GetMoveXY(&ax, &ay);

                fprintf(fd, "Path %d %d %d %f %f\n",
                    fill0 - 1,
                    fill1 - 1,
                    line - 1,
                    ax,
                    ay);

                EdgesIterator::Edge edge;
                while(!eit.IsFinished())
                {
                    eit.GetEdge(&edge);
                    if (edge.Curve)
                    {
                        fprintf(fd, "Curve %f %f %f %f\n",
                            edge.Cx,
                            edge.Cy,
                            edge.Ax,
                            edge.Ay);
                    }
                    else
                    {
                        fprintf(fd, "Line %f %f\n",
                            edge.Ax,
                            edge.Ay);
                    }
                }
            }
            fprintf(fd, "<-------EndPath\n");
            fclose(fd);
#endif // GFX_GENERATE_SHAPE_FILE

            it.AddForTessellation(&cs);
        }
    }

    // Done, store last set of meshes.
    AddShapeToMesh(meshSet, &cs, context, s9g);

    if (s9g && s9g->ImgAdjustments.size())
    {
        s9g->ComputeImgAdjustMatrices();
        meshSet->SetImgAdjustMatrices(s9g->ImgAdjustments);
    }

#ifdef GFX_GENERATE_SHAPE_FILE
    fd = fopen("shapes", "at");
    fprintf(fd, "!======EndShape\n");
    fclose(fd);
#endif
}

template<class PathData>
void GFxShapeCharacterDef::PreTessellateImpl
    (Float masterScale, const GFxRenderConfig& config, Float maxStrokeExtent)
{
#ifdef GFC_NO_FXPLAYER_EDGEAA
    bool    useEdgeAA        = 0;
#else
    bool    useEdgeAA        =  config.IsUsingEdgeAA() &&
        (config.IsEdgeAATextured() || !HasTexturedFill());
#endif

    PreTessellatedScaleKey = GFx_ComputeShapeScaleKey(masterScale, useEdgeAA);

    Float screenPixelSize = 20.0f / masterScale;
    Float curveError      = screenPixelSize * 0.75f * config.GetMaxCurvePixelError();

    // Construct A new GFxMesh to handle this error tolerance.
    GFxMeshSet* meshSet = new GFxMeshSet(screenPixelSize, 
        curveError, 
        !useEdgeAA,
        false);
    GCompoundShape cs;
    cs.SetCurveTolerance(curveError);
    cs.SetNonZeroFill(IsNonZeroFill());

    // Character shapes may have invalid bounds.
    if (HasValidBounds())
        meshSet->SetShapeBounds(Bound, maxStrokeExtent);

    //PathsIterator it = GetPathsIterator();
    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    UInt fillStylesNum = 0;
    const GFxFillStyle* pfillStyles = GetFillStyles(&fillStylesNum);
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            // Add and Tessellate shape, including styles and line strips.
            cs.RemoveShortSegments(curveError / 4);
            meshSet->AddTessellatedShape(cs, pfillStyles, fillStylesNum, (GFxDisplayContext*)0, config);

            // Prepare compound shape for nest set of paths.
            cs.RemoveAll();
            it.Skip();
        }
        else
        {
            it.AddForTessellation(&cs);
        }
    }
    cs.RemoveShortSegments(curveError / 4);
    meshSet->AddTessellatedShape(cs, pfillStyles, fillStylesNum, (GFxDisplayContext*)0, config);
    CachedMeshes[PreTessellatedScaleKey] = meshSet;

    // DBG
    //printf("P");
}

// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeCharacterDef contains.
template<class PathData>
void    GFxShapeCharacterDef::MakeCompoundShapeImpl(GCompoundShape *cs, Float tolerance) const
{
    cs->RemoveAll();
    cs->SetCurveTolerance(tolerance);
    cs->SetNonZeroFill(IsNonZeroFill());
    typename PathData::PathsIterator it = typename PathData::PathsIterator(this);
    while(!it.IsFinished())
    {
        it.AddForTessellation(cs);
    }
}


//------------------------------------------------------------------------------
static UInt32 GFx_ComputeBernsteinHash(const void* buf, UInt size, UInt32 seed = 5381)
{
    const UByte*    pdata   = (const UByte*) buf;
    UInt32          hash    = seed;
    while (size)
    {
        size--;
        hash = ((hash << 5) + hash) ^ (UInt)pdata[size];
    }
    return hash;
}

template<class PathData>
UInt32 GFxShapeCharacterDef::ComputeGeometryHashImpl() const
{
    UInt32 hash = 5381;
    UInt shapesCnt = 0, pathsCnt = 0;
    GetShapeAndPathCounts(&shapesCnt, &pathsCnt);
    hash = GFx_ComputeBernsteinHash(&shapesCnt, sizeof(shapesCnt), hash);
    hash = GFx_ComputeBernsteinHash(&pathsCnt,  sizeof(pathsCnt),  hash);
    typename PathData::PathsIterator pit = typename PathData::PathsIterator(this);

    while(!pit.IsFinished())
    {
        UInt styles[3];
        pit.GetStyles(&styles[0], &styles[1], &styles[2]);
        hash = GFx_ComputeBernsteinHash(&styles,  sizeof(styles),  hash);

        typename PathData::EdgesIterator eit = pit.GetEdgesIterator();

        Float mx[2];
        eit.GetMoveXY(&mx[0], &mx[1]);
        hash = GFx_ComputeBernsteinHash(&mx,  sizeof(mx),  hash);

        // Counting num of edges might be expensive! Especially for Swf paths.
        //UInt numEdges = eit.GetEdgesCount();
        //hash = GFx_ComputeBernsteinHash(&numEdges, sizeof(numEdges), hash);
        while (!eit.IsFinished())
        {
            typename PathData::EdgesIterator::PlainEdge edge;
            eit.GetPlainEdge(&edge);
            hash = GFx_ComputeBernsteinHash(&edge.Data, edge.Size * sizeof(SInt32), hash);
        }
        pit.AdvanceBy(eit);
    }
    return hash;
}

template<class PathData>
bool    GFxShapeCharacterDef::IsEqualGeometryImpl(const GFxShapeCharacterDef& cmpWith) const
{
    UInt shapesCnt = 0, pathsCnt = 0;
    GetShapeAndPathCounts(&shapesCnt, &pathsCnt);
    UInt cmpWithShapesCnt = 0, cmpWithPathsCnt = 0;
    cmpWith.GetShapeAndPathCounts(&cmpWithShapesCnt, &cmpWithPathsCnt);
    if (shapesCnt != cmpWithShapesCnt || pathsCnt  != cmpWithPathsCnt)
        return false;

    typename PathData::PathsIterator pit1 = typename PathData::PathsIterator(this);
    typename PathData::PathsIterator pit2 = typename PathData::PathsIterator(&cmpWith);
    while(!pit1.IsFinished())
    {
        if (pit2.IsFinished()) 
            return false;

        UInt styles1[3], styles2[3];
        pit1.GetStyles(&styles1[0], &styles1[1], &styles1[2]);
        pit2.GetStyles(&styles2[0], &styles2[1], &styles2[2]);
        if (memcmp(styles1, styles2, sizeof(styles1)) != 0)
            return false;

        // !AB: theoretically, we need to compare fill styles too, but
        // for font glyphs this is not critical.

        typename PathData::EdgesIterator eit1 = pit1.GetEdgesIterator();
        typename PathData::EdgesIterator eit2 = pit2.GetEdgesIterator();

        Float ax1, ay1, ax2, ay2;
        eit1.GetMoveXY(&ax1, &ay1);
        eit2.GetMoveXY(&ax2, &ay2);
        if (ax1 != ax2 || ay1 != ay2)
            return false;

        if (eit1.GetEdgesCount() != eit2.GetEdgesCount())
            return false;

        typename PathData::EdgesIterator::PlainEdge edge1;
        typename PathData::EdgesIterator::PlainEdge edge2;
        while (!eit1.IsFinished())
        {
            if (eit2.IsFinished()) 
                return false;

            eit1.GetPlainEdge(&edge1);
            eit2.GetPlainEdge(&edge2);

            if (edge1.Size != edge2.Size ||
                memcmp(edge1.Data, edge2.Data, edge1.Size * sizeof(SInt)) != 0)
                return false;
        }
        pit1.AdvanceBy(eit1);
        pit2.AdvanceBy(eit2);
    }
    return true;
}

void GFxShapeCharacterDef::AddShapeToMesh(GFxMeshSet *meshSet, 
                                          GCompoundShape* cs, 
                                          GFxDisplayContext &context,
                                          GFxScale9GridInfo* s9g) const
{
    GRenderer::FillTexture  texture;
    GFxTexture9Grid         t9g;
    GRectF                  t9gClip;
    bool                    useTiling = false;

    const GFxFillStyle* pfillStyles = NULL;
    UInt fillStylesNum = 0;
    if (Flags & Flags_StylesSupport)
    {
        pfillStyles = GetFillStyles(&fillStylesNum);
    }

    // Add and tessellate shape, including styles and line strips.

    if ((Flags & (Flags_S9GSupport | Flags_StylesSupport)) && s9g && s9g->CanUseTiling)
    {
        // The images can be drawn with automatic tiling according to the 
        // scale9grid. It is possible when: 
        // 1. The image is on the level of the scale9grid movie clip 
        //    (s9g->CanUseTiling == true), and
        // 2. The shape has only one path, one fill style, and no line style 
        //    (which basically represents a Flash image as opposed to a shape 
        //    with image fill style), and
        // 3. The shape fill is ClippedImageFill (not tiled), and
        // 4. The texture matrix has no free rotation.
        //----------------------
        int texture9GridStyle = GetTexture9GridStyle(*cs);
        if (texture9GridStyle >= 0)
        {
            pfillStyles[texture9GridStyle].GetFillTexture(&texture, context, 1, 0);
            if (!texture.TextureMatrix.IsFreeRotation())
            {
                // The clipping bounds for the texture9grid must be calculated for this 
                // particular part of the shape. It's incorrect to take just RectBound.
                //--------------------------
                cs->PerceiveBounds(&t9gClip.Left, &t9gClip.Top, &t9gClip.Right, &t9gClip.Bottom);
                t9g.Compute(*s9g,
                    t9gClip,
                    texture.TextureMatrix, 
                    meshSet->GetScaleMultiplier(), 
                    texture9GridStyle);
                meshSet->AddTexture9Grid(t9g);
                useTiling = true;
            }
        }
    }

    if (!useTiling)
    {
        if (s9g)
        {
            s9g->ComputeImgAdjustRects(*cs, pfillStyles, fillStylesNum);
            ApplyScale9Grid(cs, *s9g);
        }
        cs->RemoveShortSegments(meshSet->GetCurveError() / 4);
        meshSet->AddTessellatedShape(*cs, pfillStyles, fillStylesNum, context);
    }
}

int    GFxShapeCharacterDef::GetTexture9GridStyle(const GCompoundShape& cs) const
{
    int styleIdx = -1;
    if ((Flags & Flags_StylesSupport) && cs.GetNumPaths() == 1)
    {
        const GCompoundShape::SPath& path = cs.GetPath(0);
        if ((path.GetLeftStyle() < 0) != (path.GetRightStyle() < 0) && path.GetLineStyle() < 0)
        {
            styleIdx = (path.GetLeftStyle() >= 0) ? path.GetLeftStyle() : path.GetRightStyle();

            UInt fillStylesNum = 0;
            const GFxFillStyle* pfillStyles = GetFillStyles(&fillStylesNum);
            if (styleIdx < (int)fillStylesNum && !pfillStyles[styleIdx].IsClippedImageFill())
                styleIdx = -1;
        }
    }
    return styleIdx;
}

GFxPathAllocator::GFxPathAllocator(UInt pageSize) : 
    pFirstPage(0), pLastPage(0), FreeBytes(0), DefaultPageSize((UInt16)pageSize)
{
}

GFxPathAllocator::~GFxPathAllocator()
{
    for(Page* pcurPage = pFirstPage; pcurPage; )
    {
        Page* pnextPage = pcurPage->pNext;
        GFREE(pcurPage);
        pcurPage = pnextPage;
    }
}

// edgesDataSize - size of geometrical data including the optional edge count, edge infos and vertices.
// Can be 0 if new shape.
// pathSize - 0 - new shape, 1 - 8-bit, 2 - 16-bit (aligned), 4 - 32-bit (aligned) 
// edgeSize - 2 - 16-bit, 4 - 32-bit
UByte* GFxPathAllocator::AllocPath(UInt edgesDataSize, UInt pathSize, UInt edgeSize)
{
    UInt freeBytes = FreeBytes;
    UInt size = 1 + pathSize*3 + edgesDataSize;
    UInt sizeForCurrentPage = size;

    if (edgesDataSize > 0)
    {
        // calculate the actual size of path taking into account both alignments (for path and for edges)
        UInt delta = 0;
        if (pLastPage)
        {
            UByte* ptr = pLastPage->GetBufferPtr(freeBytes) + 1; // +1 is for first byte (bit flags)

            // The first delta represents alignment for path
            delta = (UInt)(((UPInt)ptr) & (pathSize - 1));
            delta = ((delta + pathSize - 1) & (~(pathSize - 1))) - delta; // aligned delta

            // The second delta (delta2) represents the alignment for edges, taking into account
            // the previous alignment.
            UInt delta2 = (UInt)((UPInt)ptr + delta + pathSize*3) & (edgeSize - 1);
            delta2 = ((delta2 + edgeSize - 1) & (~(edgeSize - 1))) - delta2;

            delta += delta2;
        }
        sizeForCurrentPage += delta;
        if (!pLastPage || FreeBytes < sizeForCurrentPage)
        {
            // new page will be allocated, calculate new delta size
            delta = (1 + (pathSize - 1)) & (~(pathSize - 1));
            delta = (delta + pathSize*3 + (edgeSize - 1)) & (~(edgeSize - 1)) + edgesDataSize - size;
        }
        size += delta;
    }
    UByte* ptr = AllocMemoryBlock(sizeForCurrentPage, size);
    return ptr;
}

UByte* GFxPathAllocator::AllocRawPath(UInt32 sizeInBytes)
{
    return AllocMemoryBlock(sizeInBytes, sizeInBytes);
}

UByte* GFxPathAllocator::AllocMemoryBlock(UInt32 sizeForCurrentPage, UInt32 sizeInNewPage)
{
    UInt size = sizeInNewPage;
    UInt freeBytes = FreeBytes;
    if (pLastPage == NULL || FreeBytes < sizeForCurrentPage)
    {
        UInt pageSize = DefaultPageSize;
        Page* pnewPage;
        if (size > pageSize)
        {
            pageSize = size;
        }
        freeBytes           = pageSize;
        pnewPage            = (Page*)GALLOC(sizeof(Page) + pageSize);
#ifdef GFC_BUILD_DEBUG
        memset(pnewPage, 0xba, sizeof(Page) + pageSize);
#endif
        pnewPage->pNext     = NULL;
        pnewPage->PageSize  = pageSize;
        if (pLastPage)
        {
            pLastPage->pNext    = pnewPage;
            // correct page size
            pLastPage->PageSize = pLastPage->PageSize - FreeBytes;
        }
        pLastPage = pnewPage;
        if (!pFirstPage)
            pFirstPage = pnewPage;
    }
    else
    {
        size = sizeForCurrentPage;
    }

    UByte* ptr  = pLastPage->GetBufferPtr(freeBytes);
#ifdef GFC_BUILD_DEBUG
    memset(ptr, 0xcc, size);
#endif
    FreeBytes   = (UInt16)(freeBytes - size);

    return ptr;
}

bool   GFxPathAllocator::ReallocLastBlock(UByte* ptr, UInt32 oldSize, UInt32 newSize)
{
    GASSERT(newSize <= oldSize);
    if (newSize < oldSize && pLastPage && IsInPage(pLastPage, ptr))
    {
        UPInt off = ptr - pLastPage->GetBufferPtr();
        if (pLastPage->PageSize - (off + oldSize) == FreeBytes)
        {
            UPInt newFreeBytes = pLastPage->PageSize - (off + newSize);
            if (newFreeBytes < 65536)
                FreeBytes = UInt16(newFreeBytes);
        }
    }
    return false;
}

void GFxPathAllocator::SetDefaultPageSize(UInt dps)
{
    if (pFirstPage == NULL)
    {
        GASSERT(dps < 65536);
        DefaultPageSize = (UInt16)dps;
    }
}

GFxPathPacker::GFxPathPacker()
{
    Reset();
}

void GFxPathPacker::Reset()
{
    Fill0 = Fill1 = Line = 0;
    Ax = Ay = Ex = Ey = 0; // moveTo
    EdgesIndex = 0;
    CurvesNum = 0;
    LinesNum = 0;
    EdgesNumBits = 0;
    NewShape = 0;
}

static inline UInt GFx_BitsToUInt(SInt value)
{
    //return (value >= 0) ? (UInt)value : (((UInt)GTL::gabs(value))<<1);
    // always need to add one bit for a sign, even if value is positive,
    // since this value will be assigned to signed int (8, 16 or 32 bits)
    return (value == 0) ? 0 : (((UInt)GTL::gabs(value))<<1);
}

static inline UByte GFx_BitCountSInt(SInt value)
{
    return GBitsUtil::BitCount32(GFx_BitsToUInt(value));
}

static inline UByte GFx_BitCountSInt(SInt value1, SInt value2)
{
    return GBitsUtil::BitCount32(GFx_BitsToUInt(value1) | GFx_BitsToUInt(value2));
}

static inline UByte GFx_BitCountSInt(SInt value1, SInt value2, SInt value3, SInt value4)
{
    return GBitsUtil::BitCount32(GFx_BitsToUInt(value1) | GFx_BitsToUInt(value2) | 
                                 GFx_BitsToUInt(value3) | GFx_BitsToUInt(value4));
}

template <size_t alignSize, typename T>
inline static T GFx_AlignedPtr(T ptr)
{
    return (T)(size_t(ptr + alignSize - 1) & (~(alignSize - 1)));
}

template <typename T>
inline static T GFx_AlignedPtr(T ptr, size_t alignSize)
{
    return (T)(size_t(ptr + alignSize - 1) & (~(alignSize - 1)));
}

void GFxPathPacker::Pack(GFxPathAllocator* pallocator, GFxPathData::PathsInfo* ppathsInfo)
{
    /* There are several types of packed paths:
       1) Path8
       2) Path16
       3) Path32
       4) NewShape
       5) Path8 + up to 4 16-bit edges.

       Octet 0 represents bit flags:
            Bit 0:      0 - complex type (path types 1 - 4)
                        1 - path type 5 (Path8 + up to 4 16-bit edges)
         For complex type (if bit 0 is 0):
            Bits 1-2:   type:
                        0 - NewShape
                        1 - path 8
                        2 - path 16
                        3 - path 32
            Bit 3:      edge size:
                        0 - 16 bit
                        1 - 32 bit
            Bits 4-7:   edge count:
                        0 - edge count is stored separately
                        1 - 15 - actual edge count (no additional counter is stored)
        For path type 5:
            Bits 1-2:   edge count
                        0 - 1
                        1 - 2
                        2 - 3
                        3 - 4 edges
            Bits 3-6:   edge types mask, bit set to 1 indicates curve, otherwise - line.
            Bit 7:      spare.
        Flags' octet is not aligned.
        The following part is aligned accrodingly to path type: path8 - 8-bit alignment, path16 - 16-bit, path32 - 32-bit 
        (aligned accordingly);
        NewShape consists of only one octet.
        Octet 1/Word 1/Dword 1 - Fill0
        Octet 2/Word 2/Dword 2 - Fill1
        Octet 3/Word 3/Dword 3 - Line
        
        Edges aligned accordingly to bit 3 ("edge size").
        Edges are encoded as follows: optional edge count, "moveto" starting point, edge info, edge coordinates, edge info, edge coords, etc.
        Edge count is encoded for path types 1 - 3, if it is greater than 15 (see bits 4-7 for complex type path).
        The size of count depends on "edge size" bit - 16 or 32-bit.

        "moveto" starting point is encoded as set of 2 16 or 32-bit integers.

        Edge info represented by bit array encoded as 16 or 32-bit integer (depending on "edge size" bit).
        Each bit indicates the type of edge - 1 - curve, 0 - line.

        Line edges are encoded as set of 2 16- or 32-bit integers (depending on "edge size" bit).
        Curve edges are encoded as set of 4 16- or 32-bit integers (depending on "edge size" bit).

        If there are more than 16 (32) edges then another edge info bit array is encoded for the next 16 (32) edges 
        followed by encoded edges.
    */
    GASSERT((Fill0 & 0x80000000) == 0); // prevent from "negative" styles
    GASSERT((Fill1 & 0x80000000) == 0); // prevent from "negative" styles
    GASSERT((Line  & 0x80000000) == 0); // prevent from "negative" styles

    UInt fillNumBits = GBitsUtil::BitCount32(Fill0 | Fill1 | Line);
    UByte* pbuf = 0;

    if (NewShape)
    {
        pbuf = pallocator->AllocPath(0, 0, 0);
        *pbuf = 0;
    }
    else if (fillNumBits <= 8 && EdgesNumBits <= 16 && EdgesIndex <= 4)
    {
        // special case, Path8 and up to 4 16-bits edges
        // encode flags
        UByte flags = 0;
        flags |= GFxPathConsts::PathTypeMask;
        flags |= ((EdgesIndex - 1) << GFxPathConsts::Path8EdgesCountShift) & GFxPathConsts::Path8EdgesCountMask;
        UByte edgesTypes = 0;
        pbuf = pallocator->AllocPath((LinesNum + 1)*2*2 + CurvesNum*4*2, 1, 2);
        pbuf[1] = (UByte)Fill0;
        pbuf[2] = (UByte)Fill1;
        pbuf[3] = (UByte)Line;
        
        SInt16* pbuf16 = (SInt16*)(GFx_AlignedPtr<2>(pbuf + 4));

        *pbuf16++ = (SInt16)Ax; // move to
        *pbuf16++ = (SInt16)Ay;

        UByte i, mask = 0x01;
        for(i = 0; i < EdgesIndex; ++i, mask <<= 1)
        {
            if (Edges[i].IsCurve())
            {
                *pbuf16++ = (SInt16)Edges[i].Cx;
                *pbuf16++ = (SInt16)Edges[i].Cy;
                edgesTypes |= mask;
            }
            *pbuf16++ = (SInt16)Edges[i].Ax;
            *pbuf16++ = (SInt16)Edges[i].Ay;
        }
        flags |= (edgesTypes << GFxPathConsts::Path8EdgeTypesShift) & GFxPathConsts::Path8EdgeTypesMask;
        pbuf[0] = flags;
    }
    else
    {  
        UByte flags = 0;
        UInt pathSizeFactor = 1;
        if (fillNumBits <= 8)
            flags |= (1 << GFxPathConsts::PathSizeShift);
        else if (fillNumBits <= 16)
        {
            flags |= (2 << GFxPathConsts::PathSizeShift);
            pathSizeFactor = 2;
        }
        else
        {   // 32-bit path
            flags |= (3 << GFxPathConsts::PathSizeShift);
            pathSizeFactor = 4;
        }
        UInt bytesToAllocate = 0;
        // encode edge count
        UInt edgesSizeFactor;
        if (EdgesNumBits > 16 || EdgesIndex > 65535)
        {
            flags |= GFxPathConsts::PathEdgeSizeMask; // 32-bit edges
            edgesSizeFactor = 4;
        }
        else
            edgesSizeFactor = 2;
        if (EdgesIndex < 16)
            flags |= (EdgesIndex & 0xF) << GFxPathConsts::PathEdgesCountShift;
        else
            bytesToAllocate += edgesSizeFactor; // space for size

        // calculate space needed for edgeinfo
        if (EdgesNumBits <= 16)
            bytesToAllocate += (EdgesIndex + 15)/16*2;
        else
            bytesToAllocate += (EdgesIndex + 31)/32*4;
        //
        pbuf = pallocator->AllocPath(bytesToAllocate + (LinesNum + 1)*2*edgesSizeFactor + CurvesNum * 4 *edgesSizeFactor, pathSizeFactor, edgesSizeFactor);
        pbuf[0] = flags;
        UByte* palignedBuf = GFx_AlignedPtr(pbuf + 1, pathSizeFactor);
        switch (pathSizeFactor)
        {
        case 1:  
            palignedBuf[0] = (UByte)Fill0;
            palignedBuf[1] = (UByte)Fill1;
            palignedBuf[2] = (UByte)Line;
            break;
        case 2:
            {
                SInt16* pbuf16 = (SInt16*)palignedBuf;
                pbuf16[0] = (SInt16)Fill0;
                pbuf16[1] = (SInt16)Fill1;
                pbuf16[2] = (SInt16)Line;
                break;
            }
        case 4:
            {
                SInt32* pbuf32 = (SInt32*)palignedBuf;
                pbuf32[0] = (SInt32)Fill0;
                pbuf32[1] = (SInt32)Fill1;
                pbuf32[2] = (SInt32)Line;
                break;
            }
        }
        UInt bufIndex = 3 * pathSizeFactor;
        if (edgesSizeFactor == 2)
        {
            SInt16* pbuf16 = (SInt16*)(GFx_AlignedPtr(palignedBuf + bufIndex, edgesSizeFactor));
            if (EdgesIndex >= 16)
                *pbuf16++ = (SInt16)EdgesIndex; // edge count

            *pbuf16++ = (SInt16)Ax; // moveto
            *pbuf16++ = (SInt16)Ay;

            SInt16* pedgeInfo = 0;
            UInt i, mask = 0;
            for(i = 0; i < EdgesIndex; ++i, mask <<= 1)
            {
                if (i % 16 == 0)
                {
                    // edge info
                    pedgeInfo = pbuf16++;
                    *pedgeInfo = 0;
                    mask = 1;
                }
                if (Edges[i].IsCurve())
                {
                    *pbuf16++ = (SInt16)Edges[i].Cx;
                    *pbuf16++ = (SInt16)Edges[i].Cy;
                    *pedgeInfo |= mask;
                }
                *pbuf16++ = (SInt16)Edges[i].Ax;
                *pbuf16++ = (SInt16)Edges[i].Ay;
            }
        }
        else
        {
            SInt32* pbuf32 = (SInt32*)(GFx_AlignedPtr(palignedBuf + bufIndex, edgesSizeFactor));
            if (EdgesIndex >= 16)
                *pbuf32++ = (SInt32)EdgesIndex;

            *pbuf32++ = (SInt32)Ax; // moveto
            *pbuf32++ = (SInt32)Ay;

            SInt32* pedgeInfo = 0;
            UInt i, mask = 0;
            for(i = 0; i < EdgesIndex; ++i, mask <<= 1)
            {
                if (i % 32 == 0)
                {
                    // edge info
                    pedgeInfo = pbuf32++;
                    *pedgeInfo = 0;
                    mask = 1;
                }
                if (Edges[i].IsCurve())
                {
                    *pbuf32++ = (SInt32)Edges[i].Cx;
                    *pbuf32++ = (SInt32)Edges[i].Cy;
                    *pedgeInfo |= mask;
                }
                *pbuf32++ = (SInt32)Edges[i].Ax;
                *pbuf32++ = (SInt32)Edges[i].Ay;
            }
        }
    }
    if (pbuf && ppathsInfo)
    {
        if (ppathsInfo->pPaths == NULL)
        {
            ppathsInfo->PathsPageOffset = pallocator->GetPageOffset(pbuf);
            ppathsInfo->pPaths = pbuf;
        }
        if (NewShape)
            ++ppathsInfo->ShapesCount;
        ++ppathsInfo->PathsCount;
    }
    ResetEdges();
}

void GFxPathPacker::SetMoveTo(SInt x, SInt y, UInt numBits)
{
    Ax = Ex = x;
    Ay = Ey = y;
    if (numBits > EdgesNumBits)
        EdgesNumBits = (UByte)numBits;
    else if (numBits == 0)
        EdgesNumBits = GTL::gmax(GFx_BitCountSInt(x, y), EdgesNumBits);
}

void GFxPathPacker::AddLineTo(SInt x, SInt y, UInt numBits)
{
    Edge e(x, y);
    if (EdgesIndex < Edges.size())
        Edges[EdgesIndex] = e;
    else
        Edges.push_back(e);
    ++EdgesIndex;
    ++LinesNum;
    if (numBits > EdgesNumBits)
        EdgesNumBits = (UByte)numBits;
    else if (numBits == 0)
        EdgesNumBits = GTL::gmax(GFx_BitCountSInt(x, y), EdgesNumBits);
    Ex += x;
    Ey += y;
}

void GFxPathPacker::AddCurve (SInt cx, SInt cy, SInt ax, SInt ay, UInt numBits)
{
    Edge e(cx, cy, ax, ay);
    if (EdgesIndex < Edges.size())
        Edges[EdgesIndex] = e;
    else
        Edges.push_back(e);
    ++EdgesIndex;
    ++CurvesNum;
    if (numBits > EdgesNumBits)
        EdgesNumBits = (UByte)numBits;
    else if (numBits == 0)
        EdgesNumBits = GTL::gmax(GFx_BitCountSInt(cx, cy, ax, ay), EdgesNumBits);
    Ex += cx + ax;
    Ey += cy + ay;
}



void GFxPathPacker::LineToAbs(SInt x, SInt y)
{
    AddLineTo(x - Ex, y - Ey, 0);
}

void GFxPathPacker::CurveToAbs(SInt cx, SInt cy, SInt ax, SInt ay)
{
    AddCurve(cx - Ex, cy - Ey, ax - cx, ay - cy, 0);
}

void GFxPathPacker::ClosePath() 
{ 
    if(Ex != Ax || Ey != Ay)
    {
        LineToAbs(Ax, Ay); 
    }
}

//////////////////////////////////////////////////////////////////////////
// GFxConstShapeNoStylesDef
//
bool    GFxConstShapeNoStylesDef::DefPointTestLocal(const GPointF &pt, 
                                                    bool testShape, 
                                                    const GFxCharacter* pinst) const
{
    return DefPointTestLocalImpl<GFxSwfPathData>(pt, testShape, pinst);
}

void GFxConstShapeNoStylesDef::PreTessellate(Float masterScale, const GFxRenderConfig& config)
{
    PreTessellateImpl<GFxSwfPathData>(masterScale, config);
}

// Push our shape data through the tessellator.
void    GFxConstShapeNoStylesDef::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                             GFxDisplayContext &context,
                                             GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxSwfPathData>(meshSet, tolerance, context, s9g);
}


// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeCharacterDef contains.
void    GFxConstShapeNoStylesDef::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    MakeCompoundShapeImpl<GFxSwfPathData>(cs, tolerance);
}


UInt32 GFxConstShapeNoStylesDef::ComputeGeometryHash() const
{
    // just compute hash of piece of memory
    UInt memsz = GFxSwfPathData::GetMemorySize(pPaths);
    return GFx_ComputeBernsteinHash(pPaths, memsz, 0);
}

bool    GFxConstShapeNoStylesDef::IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const
{
    if (cmpWith.GetPathDataType() == GFxConstShapeNoStylesDef::GetPathDataType())
    {
        // just compare two memory blocks
        const GFxConstShapeNoStylesDef* cmpWithShape = 
            static_cast<const GFxConstShapeNoStylesDef*>(&cmpWith);
        UInt memsz          = GFxSwfPathData::GetMemorySize(pPaths);
        UInt cmpWithMemsz   = GFxSwfPathData::GetMemorySize(cmpWithShape->pPaths);
        if (memsz != cmpWithMemsz)
            return false;
        bool res = (memcmp(pPaths, cmpWithShape->pPaths, memsz) == 0);
        GASSERT(res == IsEqualGeometryImpl<GFxSwfPathData>(cmpWith));
        return res;
    }
    return IsEqualGeometryImpl<GFxSwfPathData>(cmpWith);
}



// Find the bounds of this shape, and store them in
// the given rectangle.
void    GFxConstShapeNoStylesDef::ComputeBound(GRectF* r) const
{
    ComputeBoundImpl<GFxSwfPathData>(r);
}

void    GFxConstShapeNoStylesDef::Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle)
{
    Read(p, tagType, lenInBytes, withStyle, NULL, NULL);
}

static UByte* GFx_WriteUInt(UByte* p, UInt value, UInt sz)
{
    UByte* dp = p;
    for (UInt i = 0, shift = 0; i < sz; ++i, shift += 8)
    {
        *dp++ = UByte((value >> shift) & 0xFF);
    }
    return dp;
}

static UInt GFx_ReadUInt(const UByte* p, UInt sz)
{
    UInt res = 0;
    for (UInt i = 0, shift = 0; i < sz; ++i, shift += 8)
    {
        res |= ((UInt)p[i]) << shift;
    }
    return res;
}

void    GFxConstShapeNoStylesDef::Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle, 
                                       GFxFillStyleArray* pfillStyles, GFxLineStyleArray* plineStyles)
{
    SetValidHitResultFlag(false);
    GFxPathAllocator* ppathAllocator = p->GetPathAllocator();
    GFxStream*  in = p->GetStream();
    GASSERT(ppathAllocator);

    int stylesLen;
    if (withStyle)
    {
        int curOffset = in->Tell();

        SetValidBoundsFlag(true);
        in->ReadRect(&Bound);

        // SWF 8 contains bounds without a stroke: MovieClip.getRect()
        if ((tagType == GFxTag_DefineShape4) || (tagType == GFxTag_DefineFont3))
        {
            GRectF rectBound;
            in->ReadRect(&rectBound);
            SetRectBoundsLocal(rectBound);
            // MA: What does this byte do?
            // I've seen it take on values of 0x01 and 0x00
            //UByte mysteryByte =
            in->ReadU8();
        }
        else
            SetRectBoundsLocal(Bound);

        GFx_ReadFillStyles(pfillStyles, p, tagType);
        GFx_ReadLineStyles(plineStyles, p, tagType);
        stylesLen = in->Tell() - curOffset;
        GASSERT((int)stylesLen >= 0);
        GASSERT((int)(lenInBytes - stylesLen) >= 0);
    }
    else
        stylesLen = 0;

#ifndef GFX_USE_CUSTOM_PACKER
    // Format of shape data is as follows:
    // 1 byte - PathFlags (see GFxSwfPathData::Flags_<>)
    // 4 bytes - signature (in DEBUG build only)
    // N bytes [1..4] - size of whole shape data memory block (N depends on Mask_NumBytesInMemCnt)
    // X bytes - original SWF shape data
    // Y bytes [1..4] - number of shapes in swf shape data (Y depends on Mask_NumBytesInGeomCnt)
    // Z bytes [1..4] - number of paths in swf shape data (Z depends on Mask_NumBytesInGeomCnt)

    UInt shapeLen = (UInt)lenInBytes - stylesLen;
    UInt memBlockSize = shapeLen;
    UInt originalShapeLen = shapeLen;
    memBlockSize += 1; // mandatory 1 extra byte with flags (see GFxSwfPathData::Flags enum)
    UByte pathFlags = (UByte)
        ((tagType > GFxTag_DefineShape) ? GFxSwfPathData::Flags_HasExtendedFillNum : 0);
    if (withStyle)
    {
        if (pfillStyles && pfillStyles->size() > 0)
        {
            pathFlags |= GFxSwfPathData::Flags_HasFillStyles;
            memBlockSize += 2;
        }
        if (plineStyles && plineStyles->size() > 0)
        {
            pathFlags |= GFxSwfPathData::Flags_HasLineStyles;
            memBlockSize += 2;
        }
    }
#ifdef GFC_BUILD_DEBUG
    // put a 4-bytes signature at the beginning for debugging purposes
    // to catch improper iterator usage.
    memBlockSize += 4;
#endif

    // we need to reserve space for shapes & paths counter
    // these counter might be 1, 2, 3 or 4 bytes length
    // depending on their size. PathFlags should contain 
    // appropriate bits set (see Mask_NumBytesInGeomCnt).
    // At the beginning we will reserve maximum - 4 bytes per each
    // and will correct the final size at the end of read.
    // These counters will be stored at the end of whole memory block.
    // To access them it would be necessary to decode memory counter
    // first and add it the pointer and subtract two sizes of geometry
    // counters.
    UInt geomCntInBytes = 4;
    memBlockSize += geomCntInBytes * 2;

    // we need to reserve space for memory counter
    // memory counter might be 1, 2, 3 or 4 bytes length
    // depending on its size. PathFlags should contain 
    // appropriate bits set (see Mask_NumBytesInMemCnt).
    UInt memBlockSizeInBytes;
    if (memBlockSize < (1U<<8))
        memBlockSizeInBytes = 1;
    else if (memBlockSize < (1U<<16))
        memBlockSizeInBytes = 2;
    else if (memBlockSize < (1U<<24))
        memBlockSizeInBytes = 3;
    else
        memBlockSizeInBytes = 4;
    memBlockSize += memBlockSizeInBytes;
    pathFlags |= ((memBlockSizeInBytes - 1) << GFxSwfPathData::Shift_NumBytesInMemCnt) & 
        GFxSwfPathData::Mask_NumBytesInMemCnt;

    UByte *pmemBlock = ppathAllocator->AllocRawPath(memBlockSize);
    UByte *ptr = pmemBlock;

#ifdef GFC_BUILD_DEBUG
    // put a 4-bytes signature at the beginning for debugging purposes
    // to catch improper iterator usage. Put it in little endian format
    // like all swf's integers.
    *ptr++ = GFxSwfPathData::Signature & 0xFF;
    *ptr++ = (GFxSwfPathData::Signature >> 8) & 0xFF;
    *ptr++ = (GFxSwfPathData::Signature >> 16) & 0xFF;
    *ptr++ = (GFxSwfPathData::Signature >> 24) & 0xFF;
#endif

    *ptr++ = pathFlags;
    
    // actual value of memory size will be written at the end of read
    // after all corrections are done.
    ptr += memBlockSizeInBytes;

    if (pathFlags & GFxSwfPathData::Flags_HasFillStyles)
    {
        UPInt fn = pfillStyles->size();
        GASSERT(fn <= 65535);
        *ptr++ = UInt8(fn & 0xFF);
        *ptr++ = UInt8((fn >> 8) & 0x7F);
    }
    if (pathFlags & GFxSwfPathData::Flags_HasLineStyles)
    {
        UPInt fn = plineStyles->size();
        GASSERT(fn <= 65535);
        *ptr++ = UInt8(fn & 0xFF);
        *ptr++ = UInt8((fn >> 8) & 0x7F);
    }

    in->Align();
    in->ReadToBuffer(ptr, shapeLen);

    // Multiplier used for scaling.
    // In SWF 8, font shape resolution (tag 75) was increased by 20.
    Float sfactor = 1.0f;
    if (tagType == GFxTag_DefineFont3)
    {
        sfactor = 1.0f / 20.0f;
        Flags |= Flags_Sfactor20;
    }

    GFxStream* poriginalStream = in;
    GFxStream ss(ptr, shapeLen, in->GetLog(), in->GetParseControl());
    in = &ss;
    p->SetAltStream(in);

    // need to preprocess shape to parse and remove all additional fill & line styles.
    //
    // SHAPE
    //
    in->Align(); //!AB
    int numFillBits = in->ReadUInt(4);
    int numLineBits = in->ReadUInt(4);

    if (withStyle) // do not trace, if !withStyle (for example, for font glyphs)
        in->LogParse("  ShapeCharacter read: nfillbits = %d, nlinebits = %d\n", numFillBits, numLineBits);

    // These are state variables that keep the
    // current position & style of the shape
    // outline, and vary as we read the GFxEdge data.
    //
    // At the moment we just store each GFxEdge with
    // the full necessary info to render it, which
    // is simple but not optimally efficient.
    int fillBase = 0;
    int lineBase = 0;
    int moveX = 0, moveY = 0;
    SInt numMoveBits = 0;
    UInt totalPathsCount = 0;
    UInt totalShapesCount = 0;
    int curEdgesCount = 0;
    int curPathsCount = 0;
    bool shapeIsCorrupted = false;

#define SHAPE_LOG 0
#ifdef GFX_SHAPE_MEM_TRACKING
    int __startingFileOffset = in->Tell();
#endif //GFX_SHAPE_MEM_TRACKING
    // SHAPERECORDS
    while(!shapeIsCorrupted) 
    {
        int typeFlag = in->ReadUInt1();
        if (typeFlag == 0)
        {
            // Parse the record.
            int flags = in->ReadUInt(5);
            if (flags == 0) {
                // End of shape records.
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }
                break;
            }
            if (flags & 0x01)
            {
                // MoveTo = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                numMoveBits = in->ReadUInt(5);
                moveX = in->ReadSInt(numMoveBits);
                moveY = in->ReadSInt(numMoveBits);

                if (in->IsVerboseParseShape())
                    in->LogParseShape("  ShapeCharacter read: moveto %4g %4g\n", (Float) moveX * sfactor, (Float) moveY * sfactor);
            }
            if ((flags & 0x02) && numFillBits > 0)
            {
                // FillStyle0_change = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                int style = in->ReadUInt(numFillBits);
                if (in->IsVerboseParseShape())
                {
                    if (style > 0)
                    {
                        style += fillBase;
                    }
                    in->LogParseShape("  ShapeCharacter read: fill0 = %d\n", style);
                }
            }
            if ((flags & 0x04) && numFillBits > 0)
            {
                // FillStyle1_change = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                int style = in->ReadUInt(numFillBits);
                if (in->IsVerboseParseShape())
                {
                    if (style > 0)
                    {
                        style += fillBase;
                    }
                    in->LogParseShape("  ShapeCharacter read: fill1 = %d\n", style);
                }
            }
            if ((flags & 0x08) && numLineBits > 0)
            {
                // LineStyleChange = 1;
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

                int style = in->ReadUInt(numLineBits);
                if (style > 0)
                {
                    style += lineBase;
                }

                if (in->IsVerboseParseShape())
                    in->LogParseShape("  ShapeCharacter read: line = %d\n", style);
            }
            if (flags & 0x10)
            {
                GASSERT(tagType >= 22);

                in->LogParse("  ShapeCharacter read: more fill styles\n");

                if (curPathsCount > 0)
                {
                    ++totalShapesCount;
                    curPathsCount = 0;
                }
                if (curEdgesCount > 0)
                {
                    ++totalPathsCount;
                    curEdgesCount = 0;
                    ++curPathsCount;
                }

#ifdef GFX_SHAPE_MEM_TRACKING
                int __offset = in->Tell();
#endif
                fillBase = pfillStyles ? (int)pfillStyles->size() : 0;
                lineBase = plineStyles ? (int)plineStyles->size() : 0;

                int foff = GFx_ReadFillStyles(pfillStyles, p, tagType);
                int moff = in->Tell();
                int loff = GFx_ReadLineStyles(plineStyles, p, tagType);
                int eoff = in->Tell();

                if (moff != foff)
                {
                    if (foff > moff || UInt(moff) > originalShapeLen)
                    {
                        shapeIsCorrupted = true;
                        break;
                    }
                    else
                    {
                        // collapse array of fill styles and correct all offsets
                        memmove(ptr + foff, ptr + moff, loff - moff);
                        loff -= moff - foff;
                        moff = foff;
                    }
                }
                if (loff != eoff)
                {
                    if (loff > eoff || UInt(eoff) > originalShapeLen)
                    {
                        shapeIsCorrupted = true;
                        break;
                    }
                    else
                    {
                        // collapse array of lines
                        memmove(ptr + loff, ptr + eoff, shapeLen - eoff);
                        shapeLen -= eoff - loff;
                    }
                }
                in->SetPosition(loff); // do we need to set new DataSize in the stream? (!AB)

                numFillBits = in->ReadUInt(4);
                numLineBits = in->ReadUInt(4);
#ifdef GFX_SHAPE_MEM_TRACKING
                __startingFileOffset -= (in->Tell() - __offset);
#endif
            }
        }
        else
        {
            // EDGERECORD
            int edgeFlag = in->ReadUInt1();
            ++curEdgesCount;
            if (edgeFlag == 0)
            {
                // curved GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                SInt cx = in->ReadSInt(numbits);
                SInt cy = in->ReadSInt(numbits);
                SInt ax = in->ReadSInt(numbits);
                SInt ay = in->ReadSInt(numbits);

                if (in->IsVerboseParseShape())
                {
                    in->LogParseShape("  ShapeCharacter read: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", 
                        (Float) moveX * sfactor, (Float) moveY * sfactor, 
                        (Float) (moveX + cx) * sfactor, (Float) (moveY + cy) * sfactor, 
                        (Float) (moveX + cx + ax ) * sfactor, (Float) (moveY + cy + ay ) * sfactor);
                }
                moveX += ax + cx;
                moveY += ay + cy;
            }
            else
            {
                // straight GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                int lineFlag = in->ReadUInt1();
                SInt dx = 0, dy = 0;
                if (lineFlag)
                {
                    // General line.
                    dx = in->ReadSInt(numbits);
                    dy = in->ReadSInt(numbits);
                }
                else
                {
                    int vertFlag = in->ReadUInt1();
                    if (vertFlag == 0) {
                        // Horizontal line.
                        dx = in->ReadSInt(numbits);
                    } else {
                        // Vertical line.
                        dy = in->ReadSInt(numbits);
                    }
                }

                if (in->IsVerboseParseShape())
                {
                    in->LogParseShape("  ShapeCharacter read: straight edge = %4g %4g - %4g %4g\n", 
                        (Float)moveX * sfactor, (Float)moveY * sfactor, 
                        (Float)(moveX + dx) * sfactor, (Float)(moveY + dy) * sfactor);
                }
                moveX += dx;
                moveY += dy;
            }
        }
        // detect if shape record is corrupted
        shapeIsCorrupted = ((UInt)in->Tell() > originalShapeLen);
    }
    if (!shapeIsCorrupted)
    {
        // make correction for new shape length (if built-in fill & line styles
        // were removed).
        UInt newMemBlockSize = memBlockSize - (originalShapeLen - shapeLen);
        
        // now we need to calculate how much memory would be necessary
        // to store totalShapeCount & totalPathCount
        UInt maxGeomCnt = GTL::gmax(totalShapesCount, totalPathsCount);
        if (maxGeomCnt < (1U<<8))
            geomCntInBytes = 1;
        else if (maxGeomCnt < (1U<<16))
            geomCntInBytes = 2;
        else if (maxGeomCnt < (1U<<24))
            geomCntInBytes = 3;
        else
            geomCntInBytes = 4;

        // correct allocated memory once again
        newMemBlockSize = newMemBlockSize - 2*4 + 2*geomCntInBytes;
        UByte* pgeomCnt = pmemBlock + newMemBlockSize - 2*geomCntInBytes;
        pgeomCnt = GFx_WriteUInt(pgeomCnt, totalShapesCount, geomCntInBytes);
        pgeomCnt = GFx_WriteUInt(pgeomCnt, totalPathsCount, geomCntInBytes);

        if (memBlockSize > newMemBlockSize)
        {
            ppathAllocator->ReallocLastBlock(pmemBlock, memBlockSize, newMemBlockSize); 
        }
        // now we can put correct memory size
        UInt offset = 0; // path flags
#ifdef GFC_BUILD_DEBUG
        // erase tail of reallocated memory block to make sure data is not used
        if (memBlockSize > newMemBlockSize)
            memset(pmemBlock + newMemBlockSize, 0xFB, memBlockSize - newMemBlockSize);
        offset += 4; // signature
#endif
        // update PathFlags to reflect correct size of shape&path counters
        pmemBlock[offset++] |= ((geomCntInBytes - 1) << GFxSwfPathData::Shift_NumBytesInGeomCnt) & 
                                GFxSwfPathData::Mask_NumBytesInGeomCnt;

        GASSERT(newMemBlockSize != 0);
        GFx_WriteUInt(pmemBlock + offset, newMemBlockSize, memBlockSizeInBytes);

        // just make sure all values are stored correctly
        GASSERT(newMemBlockSize == GFxSwfPathData::GetMemorySize(pmemBlock));
#ifdef GFC_BUILD_DEBUG
        { 
        UInt sc = 0, pc = 0;
        GFxSwfPathData::GetShapeAndPathCounts(pmemBlock, &sc, &pc);
        GASSERT(totalShapesCount == sc && totalPathsCount == pc);
        }
#endif
    }
    else
    {
        // corrupted shape; just make an empty shape and display warning message.
        in->LogWarning("Error: Corrupted shape detected in file %s\n", 
            poriginalStream->GetFileName().ToCStr());

        UInt offset = 0;
        pmemBlock[offset++] = 0; // pathFlags, just set to 0 

        // pathFlags, memsz, numbits fill and line (1 b), end-of-shape record (1 b), shapes cnt, paths cnt
        UInt newMemBlockSize = 1 + 1 + 1 + 1 + 1 + 1; 
#ifdef GFC_BUILD_DEBUG
        newMemBlockSize += 4; // signature
        offset += 4;
#endif
        pmemBlock[offset++] = 0; // mem size
        pmemBlock[offset++] = 0; // num bits in fill & line styles
        pmemBlock[offset++] = 0; // end-of-shape record
        pmemBlock[offset++] = 0; // shapes cnt
        pmemBlock[offset++] = 0; // paths cnt
        if (memBlockSize > newMemBlockSize)
        {
            ppathAllocator->ReallocLastBlock(pmemBlock, memBlockSize, newMemBlockSize); 
        }
    }
    pPaths = pmemBlock;
    p->SetAltStream(NULL);
#else // GFX_USE_CUSTOM_PACKER
    //
    // SHAPE
    //
    in->Align(); //!AB
    int NumFillBits = in->ReadUInt(4);
    int NumLineBits = in->ReadUInt(4);

    if (withStyle) // do not trace, if !withStyle (for example, for font glyphs)
        in->LogParse("  ShapeCharacter read: nfillbits = %d, nlinebits = %d\n", NumFillBits, NumLineBits);

    // These are state variables that keep the
    // current position & style of the shape
    // outline, and vary as we read the GFxEdge data.
    //
    // At the moment we just store each GFxEdge with
    // the full necessary info to render it, which
    // is simple but not optimally efficient.
    int FillBase = 0;
    int LineBase = 0;
    int moveX = 0, moveY = 0;
    SInt numMoveBits = 0;
    GFxPathPacker CurrentPath;

#define SHAPE_LOG 0
#ifdef GFX_SHAPE_MEM_TRACKING
    int __startingFileOffset = in->Tell();
#endif //GFX_SHAPE_MEM_TRACKING
    // SHAPERECORDS
    for (;;) {
        int TypeFlag = in->ReadUInt1();
        if (TypeFlag == 0)
        {
            // Parse the record.
            int flags = in->ReadUInt(5);
            if (flags == 0) {
                // End of shape records.

                // Store the current GFxPath if any.
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                }

                break;
            }
            if (flags & 0x01)
            {
                // MoveTo = 1;

                // Store the current GFxPath if any, and prepare a fresh one.
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                }

                numMoveBits = in->ReadUInt(5);
                moveX = in->ReadSInt(numMoveBits);
                moveY = in->ReadSInt(numMoveBits);

                // Set the beginning of the GFxPath.
                CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);

                in->LogParseShape("  ShapeCharacter read: moveto %4g %4g\n", (Float) moveX * sfactor, (Float) moveY * sfactor);
            }
            if ((flags & 0x02)
                && NumFillBits > 0)
            {
                // FillStyle0_change = 1;
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);
                }
                int style = in->ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                CurrentPath.SetFill0(style);

                in->LogParseShape("  ShapeCharacter read: fill0 = %d\n", style);
            }
            if ((flags & 0x04)
                && NumFillBits > 0)
            {
                // FillStyle1_change = 1;
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);
                }
                int style = in->ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                CurrentPath.SetFill1(style);

                in->LogParseShape("  ShapeCharacter read: fill1 = %d\n", style);
            }
            if ((flags & 0x08)
                && NumLineBits > 0)
            {
                // LineStyleChange = 1;
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    CurrentPath.SetMoveTo(moveX, moveY, numMoveBits);
                }
                int style = in->ReadUInt(NumLineBits);
                if (style > 0)
                {
                    style += LineBase;
                }
                CurrentPath.SetLine(style);

                in->LogParseShape("  ShapeCharacter read: line = %d\n", style);
            }
            if (flags & 0x10)
            {
                GASSERT(tagType >= 22);

                in->LogParse("  ShapeCharacter read: more fill styles\n");

                // Store the current GFxPath if any.
                if (! CurrentPath.IsEmpty())
                {
                    CurrentPath.Pack(ppathAllocator, &Paths);
                    // Clear styles.
                    CurrentPath.SetFill0(0);
                    CurrentPath.SetFill1(0);
                    CurrentPath.SetLine(0);
                }
                // Tack on an empty GFxPath signaling a new shape.
                // @@ need better understanding of whether this is correct??!?!!
                // @@ i.E., we should just start a whole new shape here, right?

                CurrentPath.SetNewShape();
                CurrentPath.Pack(ppathAllocator, &Paths);
#ifdef GFX_SHAPE_MEM_TRACKING
                int __offset = in->Tell();
#endif
                FillBase = pfillStyles ? (int)pfillStyles->size() : 0;
                LineBase = plineStyles ? (int)plineStyles->size() : 0;
                GFx_ReadFillStyles(pfillStyles, p, tagType);
                GFx_ReadLineStyles(plineStyles, p, tagType);
                NumFillBits = in->ReadUInt(4);
                NumLineBits = in->ReadUInt(4);
#ifdef GFX_SHAPE_MEM_TRACKING
                __startingFileOffset -= (in->Tell() - __offset);
#endif
            }
        }
        else
        {
            // EDGERECORD
            int EdgeFlag = in->ReadUInt1();
            if (EdgeFlag == 0)
            {
                // curved GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                SInt cx = in->ReadSInt(numbits);
                SInt cy = in->ReadSInt(numbits);
                SInt ax = in->ReadSInt(numbits);
                SInt ay = in->ReadSInt(numbits);

                CurrentPath.AddCurve(cx, cy, ax, ay, numbits);

                in->LogParseShape("  ShapeCharacter read: curved edge   = %4g %4g - %4g %4g - %4g %4g\n", 
                    (Float) moveX * sfactor, (Float) moveY * sfactor, 
                    (Float) (moveX + cx) * sfactor, (Float) (moveY + cy) * sfactor, 
                    (Float) (moveX + cx + ax ) * sfactor, (Float) (moveY + cy + ay ) * sfactor);

                moveX += ax + cx;
                moveY += ay + cy;
            }
            else
            {
                // straight GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                int LineFlag = in->ReadUInt1();
                //Float   dx = 0, dy = 0;
                SInt dx = 0, dy = 0;
                if (LineFlag)
                {
                    // General line.
                    dx = in->ReadSInt(numbits);
                    dy = in->ReadSInt(numbits);
                }
                else
                {
                    int VertFlag = in->ReadUInt1();
                    if (VertFlag == 0) {
                        // Horizontal line.
                        dx = in->ReadSInt(numbits);
                    } else {
                        // Vertical line.
                        dy = in->ReadSInt(numbits);
                    }
                }

                in->LogParseShape("  ShapeCharacter read: straight edge = %4g %4g - %4g %4g\n", 
                    (Float)moveX * sfactor, (Float)moveY * sfactor, 
                    (Float)(moveX + dx) * sfactor, (Float)(moveY + dy) * sfactor);

                // Must have a half of dx,dy for the curve control point
                // to perform shape tween (morph) correctly.
                CurrentPath.AddLineTo(dx, dy, numbits);

                moveX += dx;
                moveY += dy;
            }
        }
    }
#endif // GFX_USE_CUSTOM_PACKER

#ifdef GFX_SHAPE_MEM_TRACKING
    GFx_MT_SwfGeomMem += (in->Tell() - __startingFileOffset);
    if(withStyle)
    {
        printf("Shapes memory statistics:\n"
            "   ShapesCount = %d, PathsCount = %d, SwfGeomMem = %d, AllocatedMem = %d\n", 
            GFx_MT_ShapesCount, GFx_MT_PathsCount, GFx_MT_SwfGeomMem, GFx_MT_GeomMem);
    }
#endif

    UPInt i, n;
    // Determine if we have any non-solid color fills.
    Flags &= (~Flags_TexturedFill);
    if (pfillStyles)
    {
        for (i = 0, n = pfillStyles->size(); i < n; i++)
        {
            if ((*pfillStyles)[i].GetType() != 0)
            {
                Flags |= Flags_TexturedFill;
                break;
            }
        }
    }
    // Compete maximum stroke extent
    if (Flags & Flags_StylesSupport)
    {
        Float maxStrokeExtent = 1.0f;
        if (plineStyles)
        {
            for(i = 0, n = plineStyles->size(); i < n; i++)
                maxStrokeExtent = GTL::gmax(maxStrokeExtent, (*plineStyles)[i].GetStrokeExtent());
        }
        SetMaxStrokeExtent(maxStrokeExtent);
    }
}

void GFxConstShapeNoStylesDef::GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const
{
    return GFxSwfPathData::GetShapeAndPathCounts(pPaths, pshapesCnt, ppathsCnt);
}

GFxShapeCharacterDef::PathsIterator* GFxConstShapeNoStylesDef::GetPathsIterator() const
{
    GFxVirtualPathIterator<GFxConstShapeNoStylesDef, GFxSwfPathData>* pswfIter = 
        new GFxVirtualPathIterator<GFxConstShapeNoStylesDef, GFxSwfPathData>(this);
    return pswfIter;
}

//////////////////////////////////////////////////////////////////////////
//
// GFxSwfPathData
//
GFxSwfPathData::PathsIterator::PathsIterator(const GFxShapeCharacterDef* pdef) :
    pShapeDef(static_cast<const GFxConstShapeNoStylesDef*>(pdef)),
    Stream(pShapeDef->GetNativePathData(), INT_MAX, NULL, NULL), 
    Record(Rec_None),
    Sfactor((pdef->GetFlags() & GFxShapeCharacterDef::Flags_Sfactor20) ? 1.0f/20.0f : 1.0f)
{
#ifdef GFC_BUILD_DEBUG
    // Read a 4-bytes signature to ensure we use correct path format.
    UInt32 sig;
    if (Stream.ReadU32(&sig))
    {
        GUNUSED(sig);
        GASSERT(sig == Signature);
    }
#endif

    PathFlags = Stream.ReadU8();
    // need to skip memory counter
    UInt memSizeInBytes = ((PathFlags & GFxSwfPathData::Mask_NumBytesInMemCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInMemCnt) + 1;
    Stream.SetPosition(Stream.Tell() + memSizeInBytes); // just skip for now

    if (PathFlags & Flags_HasFillStyles)
        CurFillStylesNum = Stream.ReadU16();
    else
        CurFillStylesNum = 0;
    if (PathFlags & Flags_HasLineStyles)
        CurLineStylesNum = Stream.ReadU16();
    else
        CurLineStylesNum = 0;
    FillBase = LineBase = 0;
    NumFillBits = Stream.ReadUInt(4);
    NumLineBits = Stream.ReadUInt(4);
    MoveX = MoveY = 0;
    CurEdgesCount = 0;
    CurPathsCount = 0;
    Fill0 = Fill1 = Line = 0;
    ReadNextEdge();
}

GFxSwfPathData::PathsIterator::PathsIterator(const GFxSwfPathData::PathsIterator& p) :
    Stream(p.Stream), Fill0(p.Fill0), Fill1(p.Fill1), Line(p.Line), Record(p.Record),
    PathsCount(p.PathsCount), ShapesCount(p.ShapesCount), MoveX(p.MoveX), MoveY(p.MoveY),
    FillBase(p.FillBase), LineBase(p.LineBase), CurFillStylesNum(p.CurFillStylesNum),
    CurLineStylesNum(p.CurLineStylesNum), NumFillBits(p.NumFillBits), 
    NumLineBits(p.NumLineBits), CurEdgesCount(p.CurEdgesCount), CurPathsCount(p.CurPathsCount),
    PathFlags(p.PathFlags), Ax(p.Ax), Ay(p.Ay), Cx(p.Cx), Cy(p.Cy), Dx(p.Dx), Dy(p.Dy),
    Sfactor(p.Sfactor)
{}

void GFxSwfPathData::PathsIterator::AddForTessellation(GCompoundShape *cs)
{
    EdgesIterator it = GetEdgesIterator();

    Float ax, ay;
    it.GetMoveXY(&ax, &ay);
    UInt fill0, fill1, line;
    GetStyles(&fill0, &fill1, &line);
    cs->BeginPath(fill0 - 1, fill1 - 1, line - 1, ax, ay);

    EdgesIterator::Edge edge;
    while(!it.IsFinished())
    {
        it.GetEdge(&edge);
        if (edge.Curve)
            cs->AddCurve(edge.Cx, edge.Cy, edge.Ax, edge.Ay);
        else
            cs->AddVertex(edge.Ax, edge.Ay);
    }
    AdvanceBy(it);
}

void GFxSwfPathData::PathsIterator::SkipComplex()
{
    // this call will skip whole path
    EdgesIterator(*this).CalcEdgesCount(); 
}

void GFxSwfPathData::PathsIterator::ReadNextEdge()
{
    while(!IsFinished())
    {
        ReadNext();
        if (Record != Rec_NonEdge)
            break;
    }
}

#if 0//defined(GFC_BUILD_DEBUG)
#define LOG_SHAPE0(s)                   printf(s)
#define LOG_SHAPE1(s,p1)                printf(s,p1)
#define LOG_SHAPE2(s,p1,p2)             printf(s,p1,p2)
#define LOG_SHAPE3(s,p1,p2,p3)          printf(s,p1,p2,p3)
#define LOG_SHAPE4(s,p1,p2,p3,p4)       printf(s,p1,p2,p3,p4)
#define LOG_SHAPE5(s,p1,p2,p3,p4,p5)    printf(s,p1,p2,p3,p4,p5)
#define LOG_SHAPE6(s,p1,p2,p3,p4,p5,p6) printf(s,p1,p2,p3,p4,p5,p6)
#else
#define LOG_SHAPE0(s)                   
#define LOG_SHAPE1(s,p1)                
#define LOG_SHAPE2(s,p1,p2)             
#define LOG_SHAPE3(s,p1,p2,p3)          
#define LOG_SHAPE4(s,p1,p2,p3,p4)       
#define LOG_SHAPE5(s,p1,p2,p3,p4,p5)    
#define LOG_SHAPE6(s,p1,p2,p3,p4,p5,p6) 
#endif

void GFxSwfPathData::PathsIterator::ReadNext()
{
    GFxStream* in = &Stream;

    LOG_SHAPE0("SHAPE decode: start\n");

    // These are state variables that keep the
    // current position & style of the shape
    // outline, and vary as we read the GFxEdge data.
    //
    // At the moment we just store each GFxEdge with
    // the full necessary info to render it, which
    // is simple but not optimally efficient.

    // SHAPERECORDS
    do
    {
        int typeFlag = in->ReadUInt1();
        if (typeFlag == 0)
        {
            // Parse the record.
            int flags = in->ReadUInt(5);
            if (flags == 0) 
            {
                // End of shape records.
                Record = Rec_EndOfShape;
                LOG_SHAPE0("  SHAPE decode: end-of-shape\n");
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }
                break;
            }
            if (flags & 0x01)
            {
                // MoveTo = 1;
                Record = Rec_NonEdge;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                UInt numMoveBits = in->ReadUInt(5);
                MoveX = in->ReadSInt(numMoveBits);
                MoveY = in->ReadSInt(numMoveBits);

                LOG_SHAPE3("  SHAPE decode: numbits = %d, moveto %d %d\n", 
                    numMoveBits, MoveX, MoveY);
            }
            if ((flags & 0x02) && NumFillBits > 0)
            {
                Record = Rec_NonEdge;
                // FillStyle0_change = 1;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                UInt style = in->ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                Fill0 = style;

                LOG_SHAPE1("  SHAPE decode: fill0 = %d\n", style);
            }
            if ((flags & 0x04) && NumFillBits > 0)
            {
                Record = Rec_NonEdge;
                // FillStyle1_change = 1;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                int style = in->ReadUInt(NumFillBits);
                if (style > 0)
                {
                    style += FillBase;
                }
                Fill1 = style;

                LOG_SHAPE1("  SHAPE decode: fill1 = %d\n", style);
            }
            if ((flags & 0x08) && NumLineBits > 0)
            {
                Record = Rec_NonEdge;
                // LineStyleChange = 1;
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                int style = in->ReadUInt(NumLineBits);
                if (style > 0)
                {
                    style += LineBase;
                }
                Line = style;

                LOG_SHAPE1("  SHAPE decode: line = %d\n", style);
            }
            if (flags & 0x10)
            {
                Record = Rec_NewShape;

                if (CurPathsCount > 0)
                {
                    ++ShapesCount;
                    CurPathsCount = 0;
                }
                if (CurEdgesCount > 0)
                {
                    ++PathsCount;
                    CurEdgesCount = 0;
                    ++CurPathsCount;
                }

                Fill0 = Fill1 = Line = 0;

                in->Align();
                UInt fsn = in->ReadU8();
                if (fsn == 0xFF && PathFlags & Flags_HasExtendedFillNum)
                    fsn = in->ReadU16();
                UInt lsn = in->ReadU8();
                if (lsn == 0xFF) // ? PathFlags & Flags_HasExtendedFillNum ?
                    lsn = in->ReadU16();
                FillBase = CurFillStylesNum;
                LineBase = CurLineStylesNum;
                CurFillStylesNum += fsn;
                CurLineStylesNum += lsn;

                NumFillBits = in->ReadUInt(4);
                NumLineBits = in->ReadUInt(4);

                LOG_SHAPE2("  SHAPE decode: more fill styles, numfillbits = %d, numlinebint = %d\n", 
                    NumFillBits, NumLineBits);
            }
        }
        else
        {
            // EDGERECORD
            int edgeFlag = in->ReadUInt1();
            ++CurEdgesCount;
            if (edgeFlag == 0)
            {
                // curved GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                Cx = in->ReadSInt(numbits);
                Cy = in->ReadSInt(numbits);
                Ax = in->ReadSInt(numbits);
                Ay = in->ReadSInt(numbits);

                Record = Rec_CurveEdge;

                LOG_SHAPE5("  SHAPE decode: curved edge, numbits = %d,  %d %d - %d %d\n", 
                    numbits, Cx, Cy, Ax, Ay);
            }
            else
            {
                // straight GFxEdge
                UInt numbits = 2 + in->ReadUInt(4);
                int lineFlag = in->ReadUInt1();
                //Float   dx = 0, dy = 0;
                Dx = 0, Dy = 0;
                if (lineFlag)
                {
                    // General line.
                    Dx = in->ReadSInt(numbits);
                    Dy = in->ReadSInt(numbits);
                }
                else
                {
                    int vertFlag = in->ReadUInt1();
                    if (vertFlag == 0) {
                        // Horizontal line.
                        Dx = in->ReadSInt(numbits);
                    } else {
                        // Vertical line.
                        Dy = in->ReadSInt(numbits);
                    }
                }

                LOG_SHAPE4("  SHAPE decode: straight edge, numbits = %d, lineFl = %d, Dx = %d, Dy = %d\n", 
                    numbits, lineFlag, Dx, Dy);
                Record = Rec_StraightEdge;
            }
        }
    } while(0);        
    LOG_SHAPE0("SHAPE decode: end\n");
}

UInt GFxSwfPathData::PathsIterator::GetEdgesCount() const 
{ 
    PathsIterator pit(*this);
    return EdgesIterator(pit).CalcEdgesCount(); 
}

GFxSwfPathData::EdgesIterator::EdgesIterator(GFxSwfPathData::PathsIterator& pathIter) :
    pPathIter(&pathIter)
{
    EdgeIndex = 0;
    if (!(pPathIter->Record & PathsIterator::Rec_EdgeMask) && !pPathIter->IsFinished())
        pPathIter->ReadNextEdge();
}

void GFxSwfPathData::EdgesIterator::GetMoveXY(Float* px, Float* py)
{
    *px = pPathIter->MoveX * pPathIter->Sfactor;
    *py = pPathIter->MoveY * pPathIter->Sfactor;
}

bool GFxSwfPathData::EdgesIterator::IsFinished() const 
{ 
    return pPathIter->IsFinished () || !(pPathIter->Record & PathsIterator::Rec_EdgeMask); 
}

UInt GFxSwfPathData::EdgesIterator::GetEdgesCount() const
{
    PathsIterator pit(*pPathIter);
    EdgesIterator eit = pit.GetEdgesIterator();
    return eit.CalcEdgesCount();
}

UInt GFxSwfPathData::EdgesIterator::CalcEdgesCount()
{
    while(!IsFinished())
    {
        PlainEdge edge;
        GetPlainEdge(&edge);
    }
    return EdgeIndex;
}

void GFxSwfPathData::EdgesIterator::GetEdge(Edge* pedge, bool doLines2CurveConv)
{
    GASSERT(pPathIter->Record == PathsIterator::Rec_StraightEdge || 
            pPathIter->Record == PathsIterator::Rec_CurveEdge);
    if (pPathIter->Record == PathsIterator::Rec_CurveEdge)
    {
        pedge->Cx = (pPathIter->Cx + pPathIter->MoveX) * pPathIter->Sfactor;
        pedge->Cy = (pPathIter->Cy + pPathIter->MoveY) * pPathIter->Sfactor;
        pedge->Ax = (pPathIter->Ax + pPathIter->Cx + pPathIter->MoveX) * pPathIter->Sfactor;
        pedge->Ay = (pPathIter->Ay + pPathIter->Cy + pPathIter->MoveY) * pPathIter->Sfactor;
        pedge->Curve = 1;
        pPathIter->MoveX += pPathIter->Cx + pPathIter->Ax;
        pPathIter->MoveY += pPathIter->Cy + pPathIter->Ay;
    }
    else if (pPathIter->Record == PathsIterator::Rec_StraightEdge)
    {
        pedge->Ax = (pPathIter->Dx + pPathIter->MoveX) * pPathIter->Sfactor;
        pedge->Ay = (pPathIter->Dy + pPathIter->MoveY) * pPathIter->Sfactor;
        if (doLines2CurveConv)
        {
            pedge->Cx = (pPathIter->Dx * 0.5f + pPathIter->MoveX) * pPathIter->Sfactor;
            pedge->Cy = (pPathIter->Dy * 0.5f + pPathIter->MoveY) * pPathIter->Sfactor;
            pedge->Curve = 1;
        }
        else
            pedge->Curve = 0;
        pPathIter->MoveX += pPathIter->Dx;
        pPathIter->MoveY += pPathIter->Dy;
    }
    ++EdgeIndex;
    pPathIter->ReadNext();
}


void GFxSwfPathData::EdgesIterator::GetPlainEdge(PlainEdge* pedge)
{
    GASSERT(pPathIter->Record == PathsIterator::Rec_StraightEdge || 
        pPathIter->Record == PathsIterator::Rec_CurveEdge);
    SInt32* p = pedge->Data;
    if (pPathIter->Record == PathsIterator::Rec_CurveEdge)
    {
        *p++ = pPathIter->Cx;
        *p++ = pPathIter->Cy;
        *p++ = pPathIter->Ax;
        *p++ = pPathIter->Ay;
    }
    else if (pPathIter->Record == PathsIterator::Rec_StraightEdge)
    {
        *p++ = pPathIter->Dx;
        *p++ = pPathIter->Dy;
    }
    pedge->Size = (UInt)(p - pedge->Data);
    ++EdgeIndex;
    pPathIter->ReadNext();
}

UInt GFxSwfPathData::GetMemorySize(const UByte* ppaths)
{
    UInt offset = 0;
#ifdef GFC_BUILD_DEBUG
    offset += 4; // signature
#endif
    UByte pathFlags = ppaths[offset];
    ++offset; // path flags
    UInt memSizeInBytes = ((pathFlags & GFxSwfPathData::Mask_NumBytesInMemCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInMemCnt) + 1;
    const UByte* pmemSize = ppaths + offset;
    UInt memSize = GFx_ReadUInt(pmemSize, memSizeInBytes);
    return memSize;
}

void GFxSwfPathData::GetShapeAndPathCounts(const UByte* ppaths, UInt* pshapesCnt, UInt* ppathsCnt)
{
    UInt offset = 0;
#ifdef GFC_BUILD_DEBUG
    offset += 4; // signature
#endif
    UByte pathFlags = ppaths[offset];
    ++offset; // path flags
    UInt memSizeInBytes = ((pathFlags & GFxSwfPathData::Mask_NumBytesInMemCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInMemCnt) + 1;
    UInt geomCntInBytes = ((pathFlags & GFxSwfPathData::Mask_NumBytesInGeomCnt) >> 
                            GFxSwfPathData::Shift_NumBytesInGeomCnt) + 1;
    const UByte* pmemSize = ppaths + offset;
    UInt memSize = GFx_ReadUInt(pmemSize, memSizeInBytes);
    const UByte* pcnts = ppaths + memSize - 2 * geomCntInBytes;
    if (pshapesCnt)
        *pshapesCnt = GFx_ReadUInt(pcnts, geomCntInBytes);
    pcnts += geomCntInBytes;
    if (ppathsCnt)
        *ppathsCnt = GFx_ReadUInt(pcnts, geomCntInBytes);
}

///////////////////////////////////
// GFxShapeNoStylesDef
//
GFxShapeNoStylesDef::GFxShapeNoStylesDef(UInt pageSize) :
    PathAllocator(pageSize)
{

}

// Push our shape data through the tessellator.
void    GFxShapeNoStylesDef::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                        GFxDisplayContext &context,
                                        GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxPathData>(meshSet, tolerance, context, s9g);
}

bool    GFxShapeNoStylesDef::DefPointTestLocal(const GPointF &pt, 
                                               bool testShape, 
                                               const GFxCharacter* pinst) const
{
    return DefPointTestLocalImpl<GFxPathData>(pt, testShape, pinst);
}



// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeCharacterDef contains.
void    GFxShapeNoStylesDef::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    MakeCompoundShapeImpl<GFxPathData>(cs, tolerance);
}


UInt32 GFxShapeNoStylesDef::ComputeGeometryHash() const
{
    return ComputeGeometryHashImpl<GFxPathData>();
}


bool    GFxShapeNoStylesDef::IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const
{
    return IsEqualGeometryImpl<GFxPathData>(cmpWith);
}



// Find the bounds of this shape, and store them in
// the given rectangle.
void    GFxShapeNoStylesDef::ComputeBound(GRectF* r) const
{
    ComputeBoundImpl<GFxPathData>(r);
}

GFxShapeCharacterDef::PathsIterator* GFxShapeNoStylesDef::GetPathsIterator() const
{
    GFxVirtualPathIterator<GFxShapeNoStylesDef, GFxPathData>* pswfIter = 
        new GFxVirtualPathIterator<GFxShapeNoStylesDef, GFxPathData>(this);
    return pswfIter;
}

void GFxShapeNoStylesDef::GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const
{
    if (pshapesCnt)
        *pshapesCnt = Paths.ShapesCount;
    if (ppathsCnt)
        *ppathsCnt = Paths.PathsCount;
}

//////////////////////////////////////////////////////////////////////////
// GFxConstShapeWithStylesDef
//
GFxConstShapeWithStylesDef::GFxConstShapeWithStylesDef()
{
    MaxStrokeExtent = 100.0f; // Max Flash width = 200; so divide by 2.
    Flags |= Flags_StylesSupport | Flags_S9GSupport;
    pCachedMeshesS9g = NULL;
    pFillStyles = NULL;
    pLineStyles = NULL;
    FillStylesNum = LineStylesNum = 0;
    RectBound = GRectF(0,0,0,0);
}

GFxConstShapeWithStylesDef::~GFxConstShapeWithStylesDef()
{
    ResetS9GCache(pCachedMeshesS9g);
    for (UInt i = 0; i < (UInt)FillStylesNum; ++i)
        pFillStyles[i].~GFxFillStyle();
    GFREE(const_cast<GFxFillStyle*>(pFillStyles));

    for (UInt i = 0; i < (UInt)LineStylesNum; ++i)
        pLineStyles[i].~GFxLineStyle();
    GFREE(const_cast<GFxLineStyle*>(pLineStyles));
}

GFxConstShapeWithStylesDef::MeshSetPtrArray* GFxConstShapeWithStylesDef::GetCachedMeshesS9G()
{
    if (!pCachedMeshesS9g)
        pCachedMeshesS9g = new MeshSetPtrArray;
    return pCachedMeshesS9g;
}

void GFxConstShapeWithStylesDef::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                            GFxDisplayContext &context,
                                            GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxSwfPathData>(meshSet, tolerance, context, s9g, MaxStrokeExtent);
}

void GFxConstShapeWithStylesDef::PreTessellate(Float masterScale, const GFxRenderConfig& config)
{
    PreTessellateImpl<GFxSwfPathData>(masterScale, config, MaxStrokeExtent);
}

void    GFxConstShapeWithStylesDef::Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle)
{
    GFxFillStyleArray fillStyles;
    GFxLineStyleArray lineStyles;
    GFxConstShapeNoStylesDef::Read(p, tagType, lenInBytes, withStyle, &fillStyles, &lineStyles);

    // convert fillStyles & lineStyles to regular arrays
    if (fillStyles.size())
    {
        GASSERT(fillStyles.size() < 65536);
        GASSERT(FillStylesNum == 0 && pFillStyles == NULL);
        FillStylesNum = (UInt32)fillStyles.size();
        GFxFillStyle* pfillStyles = (GFxFillStyle*)GALLOC(sizeof(GFxFillStyle)*FillStylesNum);
        pFillStyles = pfillStyles;
        for(UInt i = 0; i < (UInt)FillStylesNum; ++i)
        {
            GTL::gconstruct<GFxFillStyle>(&pfillStyles[i], fillStyles[i]);
        }
    }
    if (lineStyles.size())
    {
        GASSERT(lineStyles.size() < 65536);
        GASSERT(LineStylesNum == 0 && pLineStyles == NULL);
        LineStylesNum = (UInt32)lineStyles.size();
        GFxLineStyle* plineStyles = (GFxLineStyle*)GALLOC(sizeof(GFxLineStyle)*LineStylesNum);
        pLineStyles = plineStyles;
        for(UInt i = 0; i < (UInt)LineStylesNum; ++i)
        {
            GTL::gconstruct<GFxLineStyle>(&plineStyles[i], lineStyles[i]);
        }
    }
}

void GFxShapeNoStylesDef::PreTessellate(Float masterScale, const GFxRenderConfig& config)
{
    PreTessellateImpl<GFxPathData>(masterScale, config);
}


////////////////////////////////
// GFxShapeWithStylesDef

GFxShapeWithStylesDef::GFxShapeWithStylesDef(UInt pageSize) : GFxShapeNoStylesDef(pageSize)
{
    MaxStrokeExtent = 100.0f; // Max Flash width = 200; so divide by 2.
    Flags |= Flags_StylesSupport | Flags_S9GSupport;
    pCachedMeshesS9g = NULL;
    RectBound = GRectF(0,0,0,0);
}

GFxShapeWithStylesDef::~GFxShapeWithStylesDef()
{
    ResetS9GCache(pCachedMeshesS9g);
}

GFxShapeWithStylesDef::MeshSetPtrArray* GFxShapeWithStylesDef::GetCachedMeshesS9G()
{
    if (!pCachedMeshesS9g)
        pCachedMeshesS9g = new MeshSetPtrArray;
    return pCachedMeshesS9g;
}


// Creates a definition relying on in-memory image.
void    GFxShapeWithStylesDef::SetToImage(GFxImageResource *pimage, bool bilinear)
{
    SInt iw = pimage->GetWidth();
    SInt ih = pimage->GetHeight();
    Float width  = 20.0f * ((Float)iw);
    Float height = 20.0f * ((Float)ih);

    // Create fill style
    FillStyles.resize(1);
    FillStyles[0].SetImageFill(bilinear ? GFxFill_ClippedSmoothImage : GFxFill_ClippedImage,
        pimage, GMatrix2D());
    Flags |= Flags_TexturedFill;

    // Set bounds
    Bound.SetRect(width, height);
    SetRectBoundsLocal(Bound);
    SetValidBoundsFlag(true);
    MaxStrokeExtent = 0;

    // Path with fill 0 on one side; clockwise.
    PathAllocator.SetDefaultPageSize(32);
    Flags |= Flags_LocallyAllocated;

    GFxPathPacker currentPath;
    currentPath.SetFill0(0);
    currentPath.SetFill1(1);
    currentPath.SetLine(0);

    currentPath.SetMoveTo(0, 0);
    currentPath.AddLineTo(PixelsToTwips(iw), 0);
    currentPath.AddLineTo(0, PixelsToTwips(ih));
    currentPath.AddLineTo(-PixelsToTwips(iw), 0);
    currentPath.AddLineTo(0, -PixelsToTwips(ih));

    currentPath.Pack(&PathAllocator, &Paths);
}

void GFxShapeWithStylesDef::Tessellate(GFxMeshSet *meshSet, Float tolerance,
                                       GFxDisplayContext &context,
                                       GFxScale9GridInfo* s9g) const
{
    TessellateImpl<GFxPathData>(meshSet, tolerance, context, s9g, MaxStrokeExtent);
}

void GFxShapeWithStylesDef::PreTessellate(Float masterScale, const GFxRenderConfig& config)
{
    PreTessellateImpl<GFxPathData>(masterScale, config, MaxStrokeExtent);
}


//
// ***** GFxPathData::EdgesIterator
//

GFxPathData::EdgesIterator::EdgesIterator(const GFxPathData::PathsIterator& pathIter)
{
    // pre-parse the path
    pPath = pathIter.CurPathsPtr;

    EdgeIndex = 0;
    Sfactor = pathIter.Sfactor;
    CurEdgesInfoMask = 1;
    if (!pPath) 
    {
        EdgesCount   = 0;
        CurEdgesInfo = 0;
        pVertices16  = 0;
        pVertices32  = 0;
        return;
    }

    UByte flags = *pPath;
    
    if (flags & GFxPathConsts::PathTypeMask)
    {
        EdgesCount = ((flags & GFxPathConsts::Path8EdgesCountMask) >> GFxPathConsts::Path8EdgesCountShift) + 1;
        CurEdgesInfo = (flags & GFxPathConsts::Path8EdgeTypesMask) >> GFxPathConsts::Path8EdgeTypesShift;
        pVertices16 = (const SInt16*)(GFx_AlignedPtr<2>(pPath + 4));
        MoveX = *pVertices16++;
        MoveY = *pVertices16++;
    }
    else
    {
        UInt vertexSFactor = (flags & GFxPathConsts::PathEdgeSizeMask) ? 4 : 2;
        UByte pathSizeType = (UByte)((flags & GFxPathConsts::PathSizeMask) >> GFxPathConsts::PathSizeShift);
        if (pathSizeType == 0) // newShape
        {
            EdgesCount = 0;
            return;
        }
        static const UInt pathSFactorTable[] = { 0, 1, 2, 4 };
        const UByte* paligned = GFx_AlignedPtr(pPath + 1, pathSizeType);
        paligned              = GFx_AlignedPtr(paligned + pathSFactorTable[pathSizeType] * 3, vertexSFactor);
        EdgesCount            = (flags & GFxPathConsts::PathEdgesCountMask) >> GFxPathConsts::PathEdgesCountShift;
        if (vertexSFactor == 2)
        {
            pVertices16 = (const SInt16*)(paligned);
            if (EdgesCount == 0)
                EdgesCount = (UInt16)(*pVertices16++);
            GASSERT(EdgesCount > 0);
            MoveX = *pVertices16++;
            MoveY = *pVertices16++;
            CurEdgesInfo = (UInt16)(*pVertices16++);

            pVertices32 = 0;
        }
        else
        {
            pVertices32 = (const SInt32*)(paligned);
            if (EdgesCount == 0)
                EdgesCount = (UInt32)(*pVertices32++);
            GASSERT(EdgesCount > 0);
            MoveX = *pVertices32++;
            MoveY = *pVertices32++;
            CurEdgesInfo = (UInt32)(*pVertices32++);

            pVertices16 = 0;
        }
    }
    GASSERT(EdgesCount < (1UL<<31));
}

void GFxPathData::EdgesIterator::GetEdge(Edge* pedge, bool doLines2CurveConv)
{
    if (EdgeIndex < EdgesCount)
    {
        SInt32 cx, cy, ax, ay;
        bool curve = false;
        if (pVertices16)
        {
            if (CurEdgesInfoMask > 0x8000)
            {
                CurEdgesInfo = *pVertices16++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                cx = *pVertices16++;
                cy = *pVertices16++;
                ax = *pVertices16++;
                ay = *pVertices16++;
                curve = true;
            }
            else
            {
                cx = ax = *pVertices16++;
                cy = ay = *pVertices16++;
            }
        }
        else
        {
            GASSERT(pVertices32);

            if ((CurEdgesInfoMask & 0xFFFFFFFFu) == 0) // care just about 32-bits
            {
                CurEdgesInfo = *pVertices32++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                cx = *pVertices32++;
                cy = *pVertices32++;
                ax = *pVertices32++;
                ay = *pVertices32++;
                curve = true;
            }
            else
            {
                cx = ax = *pVertices32++;
                cy = ay = *pVertices32++;
            }
        }
        if (curve)
        {
            pedge->Cx = (cx + MoveX) * Sfactor;
            pedge->Cy = (cy + MoveY) * Sfactor;
            pedge->Ax = (ax + cx + MoveX) * Sfactor;
            pedge->Ay = (ay + cy + MoveY) * Sfactor;
            pedge->Curve = 1;
            MoveX += cx + ax;
            MoveY += cy + ay;
        }
        else
        {
            pedge->Ax = (ax + MoveX) * Sfactor;
            pedge->Ay = (ay + MoveY) * Sfactor;
            if (doLines2CurveConv)
            {
                pedge->Cx = (ax * 0.5f + MoveX) * Sfactor;
                pedge->Cy = (ay * 0.5f + MoveY) * Sfactor;
                pedge->Curve = 1;
            }
            else
                pedge->Curve = 0;
            MoveX += ax;
            MoveY += ay;
        }
        ++EdgeIndex;
        CurEdgesInfoMask <<= 1;
    }
}


void GFxPathData::EdgesIterator::GetPlainEdge(PlainEdge* pedge)
{
    if (EdgeIndex < EdgesCount)
    {
        SInt32* p = pedge->Data;
        if (pVertices16)
        {
            if (CurEdgesInfoMask > 0x8000)
            {
                CurEdgesInfo = *pVertices16++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
            }
            else
            {
                *p++ = *pVertices16++;
                *p++ = *pVertices16++;
            }
        }
        else
        {
            GASSERT(pVertices32);

            if ((CurEdgesInfoMask & 0xFFFFFFFFu) == 0) // care just about 32-bits
            {
                CurEdgesInfo = *pVertices32++;
                CurEdgesInfoMask = 1;
            }
            if (CurEdgesInfo & CurEdgesInfoMask) // curve
            {
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
            }
            else
            {
                *p++ = *pVertices32++;
                *p++ = *pVertices32++;
            }
        }
        ++EdgeIndex;
        CurEdgesInfoMask <<= 1;
        pedge->Size = (UInt)(p - pedge->Data);
    }
}



//
// ***** GFxPathData::PathsIterator
//

GFxPathData::PathsIterator::PathsIterator(const GFxShapeCharacterDef* pdef) : 
    pShapeDef(static_cast<const GFxShapeNoStylesDef*>(pdef)),
    CurPathsPtr(pShapeDef->GetNativePathData()->pPaths), Paths(*pShapeDef->GetNativePathData()), 
    CurIndex(0), Count(pShapeDef->GetNativePathData()->PathsCount),
    Sfactor((pdef->GetFlags() & GFxShapeCharacterDef::Flags_Sfactor20) ? 1.0f/20.0f : 1.0f)
{
}

void GFxPathData::PathsIterator::AddForTessellation(GCompoundShape *cs)
{
    EdgesIterator it = GetEdgesIterator();

    Float ax, ay;
    it.GetMoveXY(&ax, &ay);
    UInt fill0, fill1, line;
    GetStyles(&fill0, &fill1, &line);
    cs->BeginPath(fill0 - 1, fill1 - 1, line - 1, ax, ay);

    EdgesIterator::Edge edge;
    while(!it.IsFinished())
    {
        it.GetEdge(&edge);
        if (edge.Curve)
            cs->AddCurve(edge.Cx, edge.Cy, edge.Ax, edge.Ay);
        else
            cs->AddVertex(edge.Ax, edge.Ay);
    }
    AdvanceBy(it);
}

void GFxPathData::PathsIterator::GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const
{
    UByte flags = *CurPathsPtr;
    if (flags & GFxPathConsts::PathTypeMask || 
        (flags & GFxPathConsts::PathSizeMask) == (1 << GFxPathConsts::PathSizeShift))
    {
        const UByte* ptr = CurPathsPtr + 1;
        *pfill0 = *ptr++;
        *pfill1 = *ptr++;
        *pline  = *ptr;
    }
    else if (!IsNewShape()) 
    {
        if ((flags & GFxPathConsts::PathSizeMask) == (2 << GFxPathConsts::PathSizeShift))
        {
            const UInt16* ptr16 = (const UInt16*)GFx_AlignedPtr<2>(CurPathsPtr + 1);
            *pfill0 = *ptr16++;
            *pfill1 = *ptr16++;
            *pline  = *ptr16;
        }
        else
        {
            const UInt32* ptr32 = (const UInt32*)GFx_AlignedPtr<4>(CurPathsPtr + 1);
            *pfill0 = *ptr32++;
            *pfill1 = *ptr32++;
            *pline  = *ptr32;
        }
    }
    else
        GASSERT(0);
}

void GFxPathData::PathsIterator::AdvanceBy(const EdgesIterator& edgeIt)
{
    if (edgeIt.IsFinished())
    {
        CurPathsPtr = (edgeIt.pVertices16) ? (const UByte*)edgeIt.pVertices16 : (const UByte*)edgeIt.pVertices32;
        ++CurIndex;
        CheckPage();
    }
}

void GFxPathData::PathsIterator::CheckPage()
{
    const void* ppage = GFxPathAllocator::GetPagePtr(Paths.pPaths, Paths.PathsPageOffset);
    if (!GFxPathAllocator::IsInPage(ppage, CurPathsPtr))
    {
        const void* pnextPage;
        if ((pnextPage = GFxPathAllocator::GetNextPage(ppage, &CurPathsPtr)) != NULL)
        {
            Paths.pPaths = CurPathsPtr;
            Paths.PathsPageOffset = GFxPathAllocator::GetPageOffset(pnextPage, CurPathsPtr);
        }
    }
}

void GFxPathData::PathsIterator::SkipComplex()
{
    UInt32 edgesCount, curEdgesInfo = 0;
    UInt32 curEdgesInfoMask = 1;
    UByte flags = *CurPathsPtr;
    const SInt16* pvertices16 = 0;
    const SInt32* pvertices32 = 0;

    if (flags & GFxPathConsts::PathTypeMask)
    {
        edgesCount = ((flags & GFxPathConsts::Path8EdgesCountMask) >> GFxPathConsts::Path8EdgesCountShift) + 1;
        curEdgesInfo = (flags & GFxPathConsts::Path8EdgeTypesMask) >> GFxPathConsts::Path8EdgeTypesShift;
        pvertices16 = (const SInt16*)(GFx_AlignedPtr<2>(CurPathsPtr + 4));
        // skip moveX, moveY
        pvertices16 += 2;
    }
    else
    {
        UInt vertexSFactor = (flags & GFxPathConsts::PathEdgeSizeMask) ? 4 : 2;
        UByte pathSizeType = (UByte)((flags & GFxPathConsts::PathSizeMask) >> GFxPathConsts::PathSizeShift);
        if (pathSizeType == 0) // newShape
        {
            edgesCount = 0;
            ++CurPathsPtr;
            return;
        }
        static const UInt pathSFactorTable[] = { 0, 1, 2, 4 };
        const UByte* paligned = GFx_AlignedPtr(CurPathsPtr + 1, pathSizeType);
        paligned              = GFx_AlignedPtr(paligned + pathSFactorTable[pathSizeType] * 3, vertexSFactor);
        edgesCount            = (flags & GFxPathConsts::PathEdgesCountMask) >> GFxPathConsts::PathEdgesCountShift;
        if (vertexSFactor == 2)
        {
            pvertices16 = (const SInt16*)(paligned);
            if (edgesCount == 0)
                edgesCount = (UInt16)(*pvertices16++);
            GASSERT(edgesCount > 0);
            // skip moveX, Y
            pvertices16 += 2;
            curEdgesInfo = (UInt16)(*pvertices16++);
        }
        else
        {
            pvertices32 = (const SInt32*)(paligned);
            if (edgesCount == 0)
                edgesCount = (UInt32)(*pvertices32++);
            GASSERT(edgesCount > 0);
            // skip moveX, Y
            pvertices32 += 2;
            curEdgesInfo = (UInt32)(*pvertices32++);
        }
    }
    // skip vertices, edgeinfos
    if (pvertices16)
    {
        for (UInt i = 0; i < edgesCount; ++i)
        {
            if (curEdgesInfoMask > 0x8000)
            {
                curEdgesInfo = *pvertices16++;
                curEdgesInfoMask = 1;
            }
            if (curEdgesInfo & curEdgesInfoMask) // curve
            {
                // skip curve
                pvertices16 += 4;
            }
            else
            {
                // skip line
                pvertices16 += 2;
            }
            curEdgesInfoMask <<= 1;
        }
        CurPathsPtr = (const UByte*)pvertices16;
    }
    else
    {
        GASSERT(pvertices32);
        for (UInt i = 0; i < edgesCount; ++i)
        {
            if ((curEdgesInfoMask & 0xFFFFFFFFu) == 0) // care just about 32-bits
            {
                curEdgesInfo = *pvertices16++;
                curEdgesInfoMask = 1;
            }
            if (curEdgesInfo & curEdgesInfoMask) // curve
            {
                // skip curve
                pvertices32 += 4;
            }
            else
            {
                // skip line
                pvertices32 += 2;
            }
            curEdgesInfoMask <<= 1;
        }
        CurPathsPtr = (const UByte*)pvertices32;
    }
}

#if 0
// DBG
//static void GFX_DrawQuad(Float x1, Float y1, Float x2, Float y2,
//                         Float x3, Float y3, Float x4, Float y4,
//                         UInt32 rgba, 
//                         GRenderer* prenderer)
//{
//    const SInt16  icoords[18] = 
//    {
//        // Strip (fill in)
//        (SInt16) x1, (SInt16) y1,
//        (SInt16) x2, (SInt16) y2,
//        (SInt16) x3, (SInt16) y3,
//        (SInt16) x4, (SInt16) y4
//    };
//    static const UInt16     indices[6] = { 0, 1, 2, 0, 2, 3 };
//
//    prenderer->FillStyleColor(rgba);
//    prenderer->SetVertexData(icoords, 9, GRenderer::Vertex_XY16i);
//
//    // Fill the inside
//    prenderer->SetIndexData(indices, 6, GRenderer::Index_16);
//    prenderer->DrawIndexedTriList(0, 0, 6, 0, 2);
//
//    // Done
//    prenderer->SetVertexData(0, 0, GRenderer::Vertex_None);
//    prenderer->SetIndexData(0, 0, GRenderer::Index_None);
//}
//static void GFX_DrawParl(Float x1, Float y1, Float x2, Float y2,
//                         Float x3, Float y3, 
//                         UInt32 rgba, 
//                         GRenderer* prenderer)
//{
//    GFX_DrawQuad(x1, y1, x2, y2, x3, y3, x1 + (x3-x2), y1 + (y3-y2), rgba, prenderer);
//}






template<class Container>
static void GFx_DumpFillStyles(FILE* fd, const Container& s)
{
    unsigned i;
    for(i = 0; i < s.size(); i++)
    {
        fprintf(fd, "Fill %d %d %d %d %d\n",
                i,
                s[i].GetColor().GetRed(),
                s[i].GetColor().GetGreen(),
                s[i].GetColor().GetBlue(),
                s[i].GetColor().GetAlpha());
    }
}

template<class Container>
static void GFx_DumpLineStyles(FILE* fd, const Container& s)
{
    unsigned i;
    for(i = 0; i < s.size(); i++)
    {
        fprintf(fd, "Stroke %d %d.0 %u %d %d %d %d\n",
                i,
                s[i].GetWidth(),
                s[i].GetStyle(),
                s[i].GetColor().GetRed(),
                s[i].GetColor().GetGreen(),
                s[i].GetColor().GetBlue(),
                s[i].GetColor().GetAlpha());
    }
}

/*
// Dump our precomputed GFxMesh data to the given GFxStream.
void    GFxShapeCharacterDef::OutputCachedData(GFile* out, const GFxCacheOptions& options)
{
    GUNUSED(options);

    UInt    n = CachedMeshes.size();
    out->WriteSInt32(n);

    for (UInt i = 0; i < n; i++)
    {
        CachedMeshes[i]->OutputCachedData(out);
    }
}


// Initialize our GFxMesh data from the given GFxStream.
void    GFxShapeCharacterDef::InputCachedData(GFile* in)
{
    int n = in->ReadSInt32();

    CachedMeshes.resize(n);

    for (int i = 0; i < n; i++)
    {
        GFxMeshSet* ms = new GFxMeshSet();
        ms->InputCachedData(in);
        CachedMeshes[i] = ms;
    }
}
*/

#endif
