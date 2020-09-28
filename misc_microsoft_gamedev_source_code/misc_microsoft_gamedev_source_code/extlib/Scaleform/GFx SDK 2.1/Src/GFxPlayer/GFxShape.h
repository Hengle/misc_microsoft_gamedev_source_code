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
#include "GContainers.h"
#include "GFxScale9Grid.h"
#include "GRenderer.h"


// ***** Declared Classes
class GFxConstShapeNoStylesDef;
class GFxShapeNoStylesDef;
class GFxShapeCharacterDef;

// ***** External Classes
class GCompoundShape;
class GFxMeshSet;
class GFxCharacter;
class GFxStream;

typedef GTL::garray<GFxFillStyle> GFxFillStyleArray;
typedef GTL::garray<GFxLineStyle> GFxLineStyleArray;

// Struct to pass parameters to shape's and mesh's Display
//
struct GFxDisplayParams
{
    GFxDisplayContext&          Context;
    const GFxFillStyle*         pFillStyles;
    const GFxLineStyle*         pLineStyles;
    GRenderer::Matrix           Mat;
    GRenderer::Cxform           Cx;
    GRenderer::BlendType        Blend;
    UInt                        FillStylesNum;
    UInt                        LineStylesNum;

    GFxDisplayParams(GFxDisplayContext& ctxt) : Context(ctxt), pFillStyles(NULL), pLineStyles(NULL)
    {
        FillStylesNum = LineStylesNum = 0;
    }
    GFxDisplayParams(GFxDisplayContext& ctxt, 
                     const GRenderer::Matrix& mat, 
                     const GRenderer::Cxform& cx,
                     GRenderer::BlendType blend, 
                     const GFxFillStyle* pfillStyles = NULL, UPInt fillStylesNum = 0, 
                     const GFxLineStyle* plineStyles = NULL, UPInt lineStylesNum = 0) : 
    Context(ctxt), pFillStyles(pfillStyles), pLineStyles(plineStyles), Mat(mat), 
    Cx(cx), Blend(blend), FillStylesNum((UInt)fillStylesNum), LineStylesNum((UInt)lineStylesNum)
    {}

    GFxDisplayParams(GFxDisplayContext& ctxt, 
        const GRenderer::Matrix& mat, 
        const GRenderer::Cxform& cx,
        GRenderer::BlendType blend, 
        const GFxFillStyleArray& fillStyles,
        const GFxLineStyleArray& lineStyles) : 
    Context(ctxt), pFillStyles(fillStyles.size() ? &fillStyles[0] : NULL), 
    pLineStyles(lineStyles.size() ? &lineStyles[0] : NULL), Mat(mat), 
    Cx(cx), Blend(blend), FillStylesNum((UInt)fillStyles.size()), LineStylesNum((UInt)lineStyles.size())
    {}

    GFxDisplayParams& operator=(const GFxDisplayParams&) { GASSERT(0); return *this; }
};


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

// A path allocator class. Used to allocate memory for shapes to avoid
// fragmentation.
//
class GFxPathAllocator : public GNewOverrideBase
{
public:
    enum { Default_PageBufferSize = 8192-2 - 8-4 -16};

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
    UByte* AllocRawPath(UInt32 sizeInBytes);
    UByte* AllocMemoryBlock(UInt32 sizeForCurrentPage, UInt32 sizeInNewPage);
    bool   ReallocLastBlock(UByte* ptr, UInt32 oldSize, UInt32 newSize);
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

class GFxShapeCharacterDef;

// Class that provides iterators to work with GFx path data (created by GFxPathPacker)
//
class GFxPathData
{
public:
    enum { Signature = 0x12345678 };

    // Structure that stores path info
    //
    struct PathsInfo
    {
        const UByte* pPaths;
        UInt         PathsCount;
        UInt         ShapesCount;
        UInt16       PathsPageOffset;

        PathsInfo():pPaths(NULL), PathsCount(0), ShapesCount(0), PathsPageOffset(0) {}
    };

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
        friend class GFxShapeCharacterDef;
        friend class GFxShapeNoStylesDef;

        const GFxShapeNoStylesDef* pShapeDef;
        const UByte*    CurPathsPtr;
        GFxPathData::PathsInfo Paths;
        UInt            CurIndex;
        UInt            Count;
        Float           Sfactor;

        void            SkipComplex();
        void            CheckPage();

        PathsIterator(const GFxShapeCharacterDef* pdef);
    public:

        // adds current path to tessellation and advances the iterator
        void AddForTessellation(GCompoundShape *cs);
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

        UInt GetEdgesCount() const { return EdgesIterator(*this).GetEdgesCount(); }
    };
};

// Class that provides iterators to work with native flash path data
//
class GFxSwfPathData
{
public:
    enum { Signature = 0x07654321 };

    enum Flags
    {
        Flags_HasFillStyles         = 0x1,
        Flags_HasLineStyles         = 0x2,
        Flags_HasExtendedFillNum    = 0x4,

        Mask_NumBytesInMemCnt       = 0x18, // mask for number of bytes in memory counter
        Shift_NumBytesInMemCnt      = 3,

        Mask_NumBytesInGeomCnt      = 0x60,  // mask for number of bytes in shapes & paths counter
        Shift_NumBytesInGeomCnt     = 5
    };
    class PathsIterator;
    class EdgesIterator
    {
        friend class PathsIterator;
        PathsIterator*  pPathIter;

        UInt            EdgeIndex;
    
        UInt CalcEdgesCount();
    public:
        EdgesIterator(): pPathIter(NULL), EdgeIndex(0) {}
        EdgesIterator(PathsIterator&);

        bool IsFinished() const;

        void GetMoveXY(Float* px, Float* py);

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
        UInt GetEdgesCount() const;
    };
    class PathsIterator
    {
        friend class GFxShapeCharacterDef;
        friend class GFxConstShapeNoStylesDef;
        friend class EdgesIterator;

        const GFxConstShapeNoStylesDef* pShapeDef;
        GFxStream       Stream;
        UInt            Fill0, Fill1, Line;
        enum RecordType
        {
            Rec_None            = 0,
            Rec_EndOfShape      = 1,
            Rec_NewShape        = 2,
            Rec_NonEdge         = 3,
            Rec_EdgeMask        = 0x80,
            Rec_StraightEdge    = 4 | Rec_EdgeMask,
            Rec_CurveEdge       = 5 | Rec_EdgeMask
        };               
        UByte           Record;
        UInt            PathsCount;
        UInt            ShapesCount;
        SInt            MoveX, MoveY;
        UInt            FillBase;
        UInt            LineBase;
        UInt            CurFillStylesNum;
        UInt            CurLineStylesNum;
        UInt            NumFillBits;
        UInt            NumLineBits;
        UInt            CurEdgesCount;
        UInt            CurPathsCount;
        UByte           PathFlags; // bits, see enum Flags_
        SInt            Ax, Ay, Cx, Cy, Dx, Dy;
        Float           Sfactor;

        void            SkipComplex();
        void            ReadNext();
        void            ReadNextEdge();

        PathsIterator(const GFxShapeCharacterDef* pdef);
    public:
        PathsIterator(const PathsIterator& p);

        // adds current path to tessellation and advances the iterator
        void AddForTessellation(GCompoundShape *cs);
        // skip current path
        void Skip()
        {
            if (!IsFinished())
            {
                if (IsNewShape())
                {
                    ReadNextEdge();
                }
                else
                    SkipComplex();
            }
        }
        // is current path a new shape?
        bool IsNewShape() const 
        { 
            return Record == Rec_NewShape; 
        }
        bool IsFinished() const { return Record == Rec_EndOfShape; }

        void GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const
        {
            *pfill0 = Fill0; *pfill1 = Fill1; *pline = Line;
        }

        EdgesIterator GetEdgesIterator() { return EdgesIterator(*this); }
        void AdvanceBy(const EdgesIterator& ) {}

        UInt GetEdgesCount() const;
    };

    static UInt GetMemorySize(const UByte* p);
    static void GetShapeAndPathCounts(const UByte* p, UInt* pshapesCnt, UInt* ppathsCnt);
};

// A path packer class. Used for shapes' creation other than loaded from SWF files, 
// for example for drawing API or for system font glyphs.
//
class GFxPathPacker
{
public:
    GFxPathPacker();

    void Pack(GFxPathAllocator* pallocator, GFxPathData::PathsInfo* ppathsInfo);
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

// This is the base shape definition class that provides basic
// shape functionality. This is an abstract class that should be
// inherited and pure virtual functions should be implemented.

class GFxShapeCharacterDef : public GFxCharacterDef
{

public:
    typedef GTL::garray<GFxMeshSet*> MeshSetPtrArray;
    enum
    {
        Flags_TexturedFill       = 1,
        Flags_Sfactor20          = 2,
        Flags_LocallyAllocated   = 4,
        Flags_NonZeroFill        = 8,
        Flags_ValidBounds        = 0x10,
        Flags_ValidHitResult     = 0x20,

        Flags_StylesSupport      = 0x40,
        Flags_S9GSupport         = 0x80
    };
    enum PathDataFormatType
    {
        PathData_Swf,
        PathData_PathPacker
    };

    GFxShapeCharacterDef();
    virtual         ~GFxShapeCharacterDef();

    void            ResetCache();
    static void     ResetS9GCache(MeshSetPtrArray*&);

    virtual void    Display(GFxDisplayContext &context, GFxCharacter* inst, StackData stackdata);
    void            Display(GFxDisplayParams &params,
                            bool  edgeAADisabled,
                            GFxCharacter* inst);

    virtual GRectF  GetBoundsLocal() const;
    
    // These methods are implemented only in shapes with styles, i.e.
    // it is not needed for glyph shapes.
    virtual GRectF  GetRectBoundsLocal() const { return GFxShapeCharacterDef::GetBoundsLocal(); }
    virtual void    SetRectBoundsLocal(const GRectF&) {}

    bool            PointInShape(const GCompoundShape& shape, Float x, Float y) const;

    // Determine texture 9-grid style if exists
    int             GetTexture9GridStyle(const GCompoundShape& cs) const;

    void    AddShapeToMesh(GFxMeshSet *meshSet, GCompoundShape* cs, 
                           GFxDisplayContext &context, 
                           GFxScale9GridInfo* s9g) const;

    const GRectF&   GetBound() const { return Bound; }

    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const = 0;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
        GFxDisplayContext &renderInfo,
        GFxScale9GridInfo* s9g) const = 0;

    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config) = 0;
    // calculate exact bounds, since Bounds may contain null or inexact info
    virtual void    ComputeBound(GRectF* r) const = 0;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const = 0;

    virtual UInt32  ComputeGeometryHash() const = 0;
    virtual bool    IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const = 0;

    virtual MeshSetPtrArray* GetCachedMeshesS9G() { return NULL; }

    // Stubs for work with fill & line styles
    virtual const GFxFillStyle* GetFillStyles(UInt* pstylesNum) const;
    virtual const GFxLineStyle* GetLineStyles(UInt* pstylesNum) const;
    virtual void GetFillAndLineStyles(const GFxFillStyle**, UInt* pfillStylesNum, 
        const GFxLineStyle**, UInt* plineStylesNum) const;
    virtual void GetFillAndLineStyles(GFxDisplayParams*) const;


    // Morph uses this
    void    SetBound(const GRectF& r)       { Bound = r; } // should do some verifying?
    void    SetValidBoundsFlag(bool flag)   { (flag) ? Flags |= Flags_ValidBounds : Flags &= ~Flags_ValidBounds; }
    bool    HasValidBounds() const          { return (Flags & Flags_ValidBounds) != 0; }

    void    SetNonZeroFill(bool flag) 
    { 
        if (flag) Flags |= Flags_NonZeroFill; else Flags &= ~Flags_NonZeroFill;
    }
    bool    IsNonZeroFill() const { return (Flags & Flags_NonZeroFill) != 0; }

    bool    HasTexturedFill() const { return Flags & Flags_TexturedFill; }

    static void ApplyScale9Grid(GCompoundShape* shape, const GFxScale9GridInfo& sg);

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_ShapeDef); }

    void SetHintedGlyphSize(UInt s) { HintedGlyphSize = UByte(s); }
    UInt GetHintedGlyphSize() const { return HintedGlyphSize; }

    virtual const UByte* GetPathData() const = 0;
    virtual UInt         GetPathDataType() const = 0; // see PathDataFormatType enum
    virtual void         GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const = 0;
    UInt                 GetFlags() const        { return Flags; }
public:
    // implementation of public virtual iterators. This implementation is
    // necessary if the path data format is unknown or may be mixed. 
    // Works slower than native iterators (GFxPathDats::PathsIterator & 
    // GFxSwfPathData::PathsIterator) because of virtual calls.
    class PathsIterator : public GRefCountBase<PathsIterator>
    {
    public:
        struct StateInfo
        {
            enum StateT
            {
                St_Setup,
                St_NewShape,
                St_NewPath,
                St_Edge,
                St_Finished
            } State;
            union
            {
                struct // State == St_Setup
                {
                    Float MoveX, MoveY;
                    UInt  Fill0, Fill1, Line;
                } Setup;
                struct  // State == St_Edge 
                {
                    Float Cx, Cy, Ax, Ay;
                    bool  Curve;
                } Edge;
            };
        };
        virtual bool GetNext(StateInfo* pcontext) = 0;
    };
    virtual PathsIterator* GetPathsIterator() const = 0;
protected:
    template<class PathData>
    bool    DefPointTestLocalImpl(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    template<class PathData>
    void            TessellateImpl(GFxMeshSet *p, Float tolerance,
                                   GFxDisplayContext &renderInfo,
                                   GFxScale9GridInfo* s9g,
                                   Float maxStrokeExtent = 100.f) const;

    template<class PathData>
    void            PreTessellateImpl(Float masterScale, 
                                      const GFxRenderConfig& config,
                                      Float maxStrokeExtent = 100.f);

    template<class PathData>
    void            ComputeBoundImpl(GRectF* r) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    template<class PathData>
    void            MakeCompoundShapeImpl(GCompoundShape *cs, Float tolerance) const;

    template<class PathData>
    UInt32          ComputeGeometryHashImpl() const;

    template<class PathData>
    bool            IsEqualGeometryImpl(const GFxShapeCharacterDef& cmpWith) const;

    void            SetValidHitResultFlag(bool v = true) const { (v) ? Flags |= Flags_ValidHitResult : Flags &= ~Flags_ValidHitResult; }
    bool            HasValidHitResult() const { return (Flags & Flags_ValidHitResult) != 0; }

    // Maximum computed distance from bounds due to stroke.
    virtual void    SetMaxStrokeExtent(Float) {}    
    virtual Float   GetMaxStrokeExtent() const { return 100.f; }
protected:

    GRectF                      Bound;
    int                         PreTessellatedScaleKey;

    // Cached pre-tessellated meshes.
    GPodVectorAdaptiveBounds<GFxMeshSet*> CachedMeshes;

    mutable UByte               Flags; // See enum Flags_...

    // If the shape represents a natively hinted glyph, this size can 
    // be used to indicate the actual glyph size in pixels.
    // In all other cases it must be zero.
    UByte                       HintedGlyphSize;


};

// An implementation of shape without styles and without
// ability to be dynamically created. Useful for pre-loaded
// font glyph shapes. This shape doesn't have path packer and
// uses Flash native shape format.

class GFxConstShapeNoStylesDef : public GFxShapeCharacterDef
{
public:
    GFxConstShapeNoStylesDef() : pPaths(NULL) {}

    void            Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle);

    virtual void    ComputeBound(GRectF* r) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    virtual UInt32  ComputeGeometryHash() const;
    virtual bool    IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;
    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config);

    virtual PathsIterator* GetPathsIterator() const;

    GFxSwfPathData::PathsIterator GetNativePathsIterator() const
    { 
        return GFxSwfPathData::PathsIterator::PathsIterator(this); 
    }

    virtual const UByte* GetPathData() const { return pPaths; }
    virtual UInt         GetPathDataType() const { return PathData_Swf; }
    const UByte*         GetNativePathData() const { return pPaths; }
    virtual void         GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const;
protected:
    void            Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle, 
                         GFxFillStyleArray*, GFxLineStyleArray*);
protected:
    UByte*          pPaths;
};

// An implementation of shape without styles but the one that could 
// be dynamically created. Useful for system font glyph shapes which
// are created by the system font provider.

class GFxShapeNoStylesDef : public GFxShapeCharacterDef
{
public:
    GFxShapeNoStylesDef(UInt pageSize = GFxPathAllocator::Default_PageBufferSize);

    // calculate exact bounds, since Bounds may contain null or inexact info
    virtual void    ComputeBound(GRectF* r) const;
    virtual bool    DefPointTestLocal(const GPointF &pt, bool testShape = 0, const GFxCharacter *pinst = 0) const;

    // Convert the paths to GCompoundShape, flattening the curves.
    virtual void    MakeCompoundShape(GCompoundShape *cs, Float tolerance) const;

    virtual UInt32  ComputeGeometryHash() const;
    virtual bool    IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const;

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;
    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config);

    // methods for dynamic shape creation
    void            StartNewShape()
    {
        GFxPathPacker p;
        p.SetNewShape();
        p.Pack(&PathAllocator, &Paths);
        SetValidHitResultFlag(false);
    }

    void            AddPath(GFxPathPacker* path)
    {
        if (!path->IsEmpty())
        {
            path->Pack(&PathAllocator, &Paths);
            SetValidHitResultFlag(false);
        }
    }

    GFxPathData::PathsIterator GetNativePathsIterator() const
    { 
        return GFxPathData::PathsIterator::PathsIterator(this); 
    }

    virtual PathsIterator* GetPathsIterator() const;

    virtual const UByte*   GetPathData() const { return (const UByte*)&Paths; }
    virtual UInt           GetPathDataType() const { return PathData_PathPacker; }
    const GFxPathData::PathsInfo* GetNativePathData() const { return &Paths; }
    virtual void           GetShapeAndPathCounts(UInt* pshapesCnt, UInt* ppathsCnt) const;
protected:
    GFxPathAllocator            PathAllocator;
    GFxPathData::PathsInfo      Paths;
};

// An implementation of shape with styles but without
// ability to be dynamically created. Useful for pre-loaded
// Flash shapes. This shape doesn't have path packer and
// uses Flash native shape format.

class GFxConstShapeWithStylesDef : public GFxConstShapeNoStylesDef
{
public:
    GFxConstShapeWithStylesDef();
    ~GFxConstShapeWithStylesDef();

    void          Read(GFxLoadProcess* p, GFxTagType tagType, UInt lenInBytes, bool withStyle);

    virtual GRectF GetRectBoundsLocal() const           { return RectBound; }
    virtual void   SetRectBoundsLocal(const GRectF& r)  { RectBound = r; }

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;

    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config);
    virtual const GFxFillStyle* GetFillStyles(UInt* pstylesNum) const
    {
        *pstylesNum = FillStylesNum;
        return pFillStyles;
    }
    virtual const GFxLineStyle* GetLineStyles(UInt* pstylesNum) const
    {
        *pstylesNum = LineStylesNum;
        return pLineStyles;
    }
    virtual void GetFillAndLineStyles(const GFxFillStyle** ppfillStyles, UInt* pfillStylesNum, 
        const GFxLineStyle** pplineStyles, UInt* plineStylesNum) const
    {
        *ppfillStyles   = pFillStyles;
        *pfillStylesNum = FillStylesNum;
        *pplineStyles   = pLineStyles;
        *plineStylesNum = LineStylesNum;
    }
    virtual void GetFillAndLineStyles(GFxDisplayParams* pparams) const
    {
        pparams->pFillStyles = pFillStyles;
        pparams->pLineStyles = pLineStyles;
        pparams->FillStylesNum = FillStylesNum;
        pparams->LineStylesNum = LineStylesNum;
    }
    virtual MeshSetPtrArray* GetCachedMeshesS9G();

    bool IsEmpty() const 
    { 
        return FillStylesNum == 0 && LineStylesNum == 0;
    }

protected:
    // Maximum computed distance from bounds due to stroke.
    virtual void  SetMaxStrokeExtent(Float m) { MaxStrokeExtent = m; }    
    virtual Float GetMaxStrokeExtent() const { return MaxStrokeExtent; }
protected:
    MeshSetPtrArray*            pCachedMeshesS9g;
    const GFxFillStyle*         pFillStyles;
    const GFxLineStyle*         pLineStyles;
    UInt32                      FillStylesNum; 
    UInt32                      LineStylesNum;
    Float                       MaxStrokeExtent;    // Maximum computed distance from bounds due to stroke.
    GRectF                      RectBound; // Smaller bounds without stroke, SWF 8 (!AB never used!)
};

// An implementation of shape with styles and with
// ability to be dynamically created. Useful for DrawingAPI, 
// uses our own path packer.

class GFxShapeWithStylesDef : public GFxShapeNoStylesDef
{
public:
    GFxShapeWithStylesDef(UInt pageSize = GFxPathAllocator::Default_PageBufferSize);
    ~GFxShapeWithStylesDef();

    virtual GRectF GetRectBoundsLocal() const           { return RectBound; }
    virtual void   SetRectBoundsLocal(const GRectF& r)  { RectBound = r; }

    // Push our shape data through the tessellator to mesh.
    virtual void    Tessellate(GFxMeshSet *p, Float tolerance,
                               GFxDisplayContext &renderInfo,
                               GFxScale9GridInfo* s9g) const;

    virtual void    PreTessellate(Float masterScale, const GFxRenderConfig& config);
    
    virtual const GFxFillStyle* GetFillStyles(UInt* pstylesNum) const
    {
        *pstylesNum = (UInt)FillStyles.size();
        return ((UInt)FillStyles.size() > 0) ? &FillStyles[0] : NULL;
    }
    virtual const GFxLineStyle* GetLineStyles(UInt* pstylesNum) const
    {
        *pstylesNum = (UInt)LineStyles.size();
        return ((UInt)LineStyles.size() > 0) ? &LineStyles[0] : NULL;
    }
    virtual void GetFillAndLineStyles(const GFxFillStyle** ppfillStyles, UInt* pfillStylesNum, 
        const GFxLineStyle** pplineStyles, UInt* plineStylesNum) const
    {
        *pfillStylesNum = (UInt)FillStyles.size();
        *plineStylesNum = (UInt)LineStyles.size();
        *ppfillStyles   = ((UInt)FillStyles.size() > 0) ? &FillStyles[0] : NULL;
        *pplineStyles   = ((UInt)LineStyles.size() > 0) ? &LineStyles[0] : NULL;
    }
    virtual void GetFillAndLineStyles(GFxDisplayParams* pparams) const
    {
        pparams->FillStylesNum = (UInt)FillStyles.size();
        pparams->LineStylesNum = (UInt)LineStyles.size();
        pparams->pFillStyles = (pparams->FillStylesNum > 0) ? &FillStyles[0] : NULL;
        pparams->pLineStyles = (pparams->LineStylesNum > 0) ? &LineStyles[0] : NULL;
    }

    virtual MeshSetPtrArray* GetCachedMeshesS9G();

    // Creates a definition relying on in-memory image.
    void    SetToImage(GFxImageResource *pimage, bool bilinear);

    // methods for dynamic shape creation
    UInt AddFillStyle(const GFxFillStyle& style)
    {
        SetValidHitResultFlag(false);
        FillStyles.push_back(style);
        return (UInt)FillStyles.size();
    }

    UInt                GetNumFillStyles() const { return (UInt)FillStyles.size(); }
    const GFxFillStyle& GetLastFillStyle() const { return FillStyles[FillStyles.size() - 1]; }
    GFxFillStyle&       GetLastFillStyle()       { return FillStyles[FillStyles.size() - 1]; }

    UInt AddLineStyle(const GFxLineStyle& style)
    {
        SetValidHitResultFlag(false);
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

protected:
    // Maximum computed distance from bounds due to stroke.
    virtual void  SetMaxStrokeExtent(Float m) { MaxStrokeExtent = m; }    
    virtual Float GetMaxStrokeExtent() const { return MaxStrokeExtent; }
protected:
    MeshSetPtrArray*            pCachedMeshesS9g;
    GFxPathAllocator            PathAllocator;
    GFxFillStyleArray           FillStyles;
    GFxLineStyleArray           LineStyles;
    Float                       MaxStrokeExtent;    // Maximum computed distance from bounds due to stroke.
    GRectF                      RectBound; // Smaller bounds without stroke, SWF 8 (!AB never used!)
};

#endif // INC_GFXSHAPE_H

