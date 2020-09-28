/**********************************************************************

Filename    :   GFxShape.h
Content     :   SWF (Shockwave Flash) player library
Created     :   July 7, 2005
Authors     :

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSHAPE_H
#define INC_GFXSHAPE_H

#include "GFxStyles.h"


// ***** Declared Classes
class GFxShapeCharacterDef;

// ***** External Classes
class GCompoundShape;
class GFxCompoundShapeAcceptor;
class GFxMeshSet;
class GFxCharacter;
class GFxStream;



// ***** Path Representation

struct GFxPathConsts
{
    enum StyleFlag
    {
        Style_None  = GFC_MAX_UINT,
    };
    enum 
    {
        PathTypeMask = 0x1 // 0 - complex, 1 - path8 + 16bit edges
    };
    enum
    {
        Path8EdgesCountMask     = 0x6,
        Path8EdgesCountShift    = 1,
        Path8EdgeTypesMask      = 0x78,
        Path8EdgeTypesShift     = 3,

        PathSizeMask            = 0x6,         // 0 - new shape, 1 - path 8, 2 - path 16, 3 - path 32
        PathSizeShift           = 1,
        PathEdgeSizeMask        = 0x8,     // 0 - 16 bit, 1 - 32 - bit
        PathEdgeSizeShift       = 3,
        PathEdgesCountMask      = 0xF0,
        PathEdgesCountShift     = 4
    };
};

class GFxPathAllocator : public GNewOverrideBase
{
public:
    enum { Default_PageBufferSize = 8192-2 - 8-4 };

private:
    struct Page
    {
        Page*       pNext;
        UInt32      PageSize;
        //UByte Buffer[];

        UByte* GetBufferPtr()                            { return ((UByte*)(&PageSize + 1)); }
        UByte* GetBufferPtr(UInt freeBytes)              { return ((UByte*)(&PageSize + 1)) + PageSize - freeBytes; }
        const UByte* GetBufferPtr() const                { return ((UByte*)(&PageSize + 1)); }
        const UByte* GetBufferPtr(UInt freeBytes) const  { return ((UByte*)(&PageSize + 1)) + PageSize - freeBytes; }
    };
    Page *pFirstPage, *pLastPage;
    UInt16 FreeBytes;
    UInt16 DefaultPageSize;
public:
    GFxPathAllocator(UInt pageSize = Default_PageBufferSize);
    ~GFxPathAllocator();

    UByte* AllocPath(UInt edgesDataSize, UInt pathSize, UInt edgeSize);
    void   SetDefaultPageSize(UInt dps);
    UInt16 GetPageOffset(const UByte* ptr) const
    {
        GASSERT(ptr - (const UByte*)pLastPage >= 0 && ptr - (const UByte*)pLastPage < 65536);
        return (UInt16)(ptr - (const UByte*)pLastPage);
    }

    static UInt16 GetPageOffset(const void* ppage, const UByte* ptr)
    {
        const Page* p = (const Page*)ppage;
        GASSERT(ptr - (const UByte*)p >= 0 && ptr - (const UByte*)p < 65536);
        return (UInt16)(ptr - (const UByte*)p);
    }
    static const void* GetPagePtr(const UByte* pinitialPtr, UInt16 offtopage) { return pinitialPtr - offtopage; }
    static bool  IsInPage(const void* ppage, const UByte* ptr) 
    { 
        GASSERT(ppage && ptr);
        const Page* p = (const Page*)ppage;
        return (ptr - (const UByte*)(&p->PageSize + 1)) < (SInt)p->PageSize;
    }
    static const void* GetNextPage(const void* ppage, const UByte** pdata)
    {
        GASSERT(ppage && pdata);
        const Page* p = (const Page*)ppage;
        p = p->pNext;
        if (p)
            *pdata = p->GetBufferPtr();
        else
            *pdata = NULL;
        return p;
    }
};

struct GFxPathsInfo
{
    const UByte* pPaths;
    UInt         PathsCount;
    UInt         ShapesCount;
    UInt16       PathsPageOffset;

    GFxPathsInfo():pPaths(NULL), PathsCount(0), ShapesCount(0), PathsPageOffset(0) {}
};

class GFxPathPacker
{
public:
    GFxPathPacker();

    void Pack(GFxPathAllocator* pallocator, GFxPathsInfo* ppathsInfo);
    void SetFill0(UInt fill0) { Fill0 = fill0; }
    void SetFill1(UInt fill1) { Fill1 = fill1; }
    void SetLine (UInt line)  { Line  = line; }

    UInt GetFill0() const { return Fill0; }
    UInt GetFill1() const { return Fill1; }
    UInt GetLine () const { return Line; }

    void SetMoveTo(SInt x, SInt y, UInt numBits = 0);
    void SetMoveToLastVertex() { SetMoveTo(Ex, Ey); }
    void AddLineTo(SInt x, SInt y, UInt numBits = 0);
    void AddCurve (SInt cx, SInt cy, SInt ax, SInt ay, UInt numBits = 0);
    void LineToAbs(SInt x, SInt y);
    void CurveToAbs(SInt cx, SInt cy, SInt ax, SInt ay);
    void ClosePath();
    void GetLastVertex(SInt* x, SInt *y) const { *x = Ex; *y = Ey; }

    void Reset();
    void ResetEdges()    { EdgesIndex = LinesNum = CurvesNum = 0; NewShape = 0; }
    bool IsEmpty() const { return EdgesIndex == 0; }
    void SetNewShape()   { NewShape = 1; }

protected:
    //private:
    UInt                    Fill0, Fill1, Line;
    
    SInt                    Ax, Ay; // moveTo
    SInt                    Ex, Ey;
    
    struct Edge
    {
        // *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
        SInt   Cx, Cy;     // "control" point
        SInt   Ax, Ay;     // "anchor" point
        enum
        {
            Curve,
            Line
        };
        UByte  Type;

        Edge() { Cx = Cy = Ax = Ay = 0; Type = Line; }
        Edge(SInt x, SInt y): Cx(x), Cy(y), Ax(x), Ay(y), Type(Line) {}
        Edge(SInt cx, SInt cy, SInt ax, SInt ay): Cx(cx), Cy(cy), Ax(ax), Ay(ay), Type(Curve) {}

        bool IsLine()  { return Type == Line; }
        bool IsCurve() { return Type == Curve; }
    };
    GTL::garray<Edge>       Edges;
    UInt                    EdgesIndex;
    UInt                    CurvesNum;
    UInt                    LinesNum;
    UByte                   EdgesNumBits;
    bool                    NewShape;

};

// ***** SWF Shape Character Definition

// Represents the outline of one or more shapes, along with
// information on fill and line styles.
class GFxShapeCharacterDef : public GFxCharacterDef
{
public:
    GFxShapeCharacterDef(UInt pageSize = GFxPathAllocator::Default_PageBufferSize);

    virtual ~GFxShapeCharacterDef();

    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst,StackData stackData);

    virtual bool    PointTestLocal(const GPointF &pt, bool testShape = 0, const GFxPointTestCacheProvider *pcache = 0) const;
    virtual GRectF  GetBoundsLocal() const;

    bool            PointInShape(const GCompoundShape& shape, Float x, Float y) const;

    // Creates a definition relying on in-memory image.
    void    SetToImage(GFxImageResource *pimage, bool bilinear);

    void    Read(GFxLoadProcess* p, GFxTagType tagType, bool withStyle);

    void    Display(
        GFxDisplayContext &context,
        const GRenderer::Matrix& mat,
        const GRenderer::Cxform& cx,
        GRenderer::BlendType blend,
        bool  edgeAADisabled,
        const GTL::garray<GFxFillStyle>& fillStyles,
        const GTL::garray<GFxLineStyle>& lineStyles) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    UInt32  ComputeGeometryHash() const;
    bool    IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tesselate(GFxCompoundShapeAcceptor *p, Float tolerance,
                              GFxDisplayContext &renderInfo) const;

    const GRectF&   GetBound() const { return Bound; }
    void            ComputeBound(GRectF* r) const;  // @@ what's the difference between this and GetBound?

//    void    OutputCachedData(GFile* out, const GFxCacheOptions& options);
//    void    InputCachedData(GFile* in);

    const GTL::garray<GFxFillStyle>&    GetFillStyles() const   { return FillStyles; }
    const GTL::garray<GFxLineStyle>&    GetLineStyles() const   { return LineStyles; }

    bool    HasTexturedFill() const { return Flags & Flags_TexturedFill; }

    // Morph uses this
    void    SetBound(const GRectF& r)       { Bound = r; } // should do some verifying?
    void    SetRectBound(const GRectF& r)   { RectBound = r; }
    void    SetValidBoundFlag(bool flag)    { ValidBounds = flag; }
    bool    HasValidBound() const           { return ValidBounds; }

    void StartNewShape()
    {
        GFxPathPacker p;
        p.SetNewShape();
        p.Pack(&PathAllocator, &Paths);
        ValidHitResult = false;
    }

    void AddPath(GFxPathPacker* path)
    {
        if (!path->IsEmpty())
        {
            path->Pack(&PathAllocator, &Paths);
            ValidHitResult = false;
        }
    }

    UInt AddFillStyle(const GFxFillStyle& style)
    {
        ValidHitResult = false;
        FillStyles.push_back(style);
        return (UInt)FillStyles.size();
    }

    UInt                GetNumFillStyles() const { return (UInt)FillStyles.size(); }
    const GFxFillStyle& GetLastFillStyle() const { return FillStyles[FillStyles.size() - 1]; }
          GFxFillStyle& GetLastFillStyle()       { return FillStyles[FillStyles.size() - 1]; }

    UInt AddLineStyle(const GFxLineStyle& style)
    {
        ValidHitResult = false;
        LineStyles.push_back(style);
        return (UInt)LineStyles.size();
    }

    UInt                GetNumLineStyles() const { return (UInt)LineStyles.size(); }
    const GFxLineStyle& GetLastLineStyle() const { return LineStyles[LineStyles.size() - 1]; }
          GFxLineStyle& GetLastLineStyle()       { return LineStyles[LineStyles.size() - 1]; }

    bool IsEmpty() const 
    { 
        return FillStyles.size() == 0 && LineStyles.size() == 0;
    }

    void ResetCache();


    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_ShapeDef); }

protected:
    class PathsIterator;
    class EdgesIterator
    {
        friend class PathsIterator;
        const UByte*    pPath;

        UInt            EdgeIndex;
        UInt            EdgesCount;
        SInt            MoveX, MoveY;
        UInt            CurEdgesInfo;
        UInt            CurEdgesInfoMask;

        const SInt16*   pVertices16;
        const SInt32*   pVertices32;

        Float           Sfactor;

    public:
        EdgesIterator():pPath(0), EdgesCount(0) {}
        EdgesIterator(const PathsIterator&);

        bool IsFinished() const { return EdgeIndex >= EdgesCount; }

        void GetMoveXY(Float* px, Float* py)
        {
            *px = MoveX * Sfactor;
            *py = MoveY * Sfactor;
        }

        struct Edge
        {
            Float Cx, Cy, Ax, Ay;
            bool  Curve;
        };
        struct PlainEdge
        {
            SInt32 Data[4];
            UInt   Size;
        };
        void GetEdge(Edge*, bool doLines2CurveConv = false);
        void GetPlainEdge(PlainEdge*);
        UInt GetEdgesCount() const { return EdgesCount; }
    };
    class PathsIterator
    {
        friend class EdgesIterator;

        const UByte*    CurPathsPtr;
        GFxPathsInfo    Paths;
        UInt            CurIndex;
        UInt            Count;
        Float           Sfactor;
        
        void    SkipComplex();
        void    CheckPage();
    public:
        PathsIterator(const GFxShapeCharacterDef* pdef);

        // adds current path to tesselation and advances the iterator
        void AddForTesselation(GCompoundShape *cs);
        // skip current path
        void Skip()
        {
            if (!IsFinished())
            {
                GASSERT(CurIndex < Count);
                if (IsNewShape())
                {
                    ++CurPathsPtr;
                }
                else
                    SkipComplex();
                ++CurIndex;
                CheckPage();
            }
        }
        // is current path a new shape?
        bool IsNewShape() const 
        { 
            GASSERT(CurPathsPtr);
            return ((*CurPathsPtr & (GFxPathConsts::PathTypeMask | GFxPathConsts::PathSizeMask)) == 0); 
        }
        bool IsFinished() const { return CurIndex >= Count; }

        void GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const;

        EdgesIterator GetEdgesIterator() const { return EdgesIterator(*this); }
        void AdvanceBy(const EdgesIterator& edgeIt);

        UInt GetEdgesCount() const { return EdgesIterator(*this).EdgesCount; }
        UInt GetPathsCount() const { return Count; }
    };
    PathsIterator GetPathsIterator() const { return PathsIterator(this); }
    
    friend class GFxMorphCharacterDef;
    friend class PathsIterator;

    // derived morph classes changes these
    GTL::garray<GFxFillStyle>   FillStyles;
    GTL::garray<GFxLineStyle>   LineStyles;
    GFxPathAllocator            PathAllocator;
    GFxPathsInfo                Paths;            
    
    enum
    {
        Flags_TexturedFill      = 1,
        Flags_Sfactor20         = 2,
        Flags_LocallyAllocated  = 4
    };
    UByte                       Flags;

private:
    //void  SortAndCleanMeshes() const;

    GRectF  Bound;
    GRectF  RectBound;          // Smaller bounds without stroke, SWF 8
    bool    ValidBounds;
    mutable bool    ValidHitResult;
    Float   MaxStrokeExtent;    // Maximum computed distance from bounds due to stroke.

    // Cached pre-tessellated meshes.
    mutable GTL::garray<GFxMeshSet*>    CachedMeshes;
};



#endif // INC_GFXSHAPE_H

