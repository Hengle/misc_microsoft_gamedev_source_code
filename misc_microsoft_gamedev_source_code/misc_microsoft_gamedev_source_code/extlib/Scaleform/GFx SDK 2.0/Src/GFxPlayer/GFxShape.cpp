/**********************************************************************

Filename    :   GFxShape.cpp
Content     :   Shape character definition with paths and edges
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

#if defined(GFC_OS_WIN32)
    #include <windows.h>
#endif

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
// shape extraction and external tesselator testing.
//#define    GFX_GENERATE_SHAPE_FILE
//#define    GFX_GENERATE_STROKE_SHAPE_FILE


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
            UByte* ptr          = pLastPage->GetBufferPtr(freeBytes) + 1; // +1 is for first byte (bit flags)

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

    UByte* ptr  = pLastPage->GetBufferPtr(freeBytes);
#ifdef GFC_BUILD_DEBUG
    memset(ptr, 0xcc, size);
#endif
    FreeBytes   = (UInt16)(freeBytes - size);

#ifdef GFX_SHAPE_MEM_TRACKING
    if (edgesDataSize == 0) // newshape
        ++GFx_MT_ShapesCount;
    else
        ++GFx_MT_PathsCount;
    GFx_MT_GeomMem += size;
#endif //GFX_SHAPE_MEM_TRACKING
    return ptr;
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

static UByte GFx_BitCountUInt(UInt value)
{ 
    /* Binary search - decision tree (5 tests, rarely 6) */
    return
        ((value < 1UL<<15) ?
        ((value < 1UL<<7) ?
        ((value < 1UL<<3) ?
        ((value < 1UL<<1) ? ((value < 1UL<<0) ? 0 : 1) : ((value < 1UL<<2) ? 2 : 3)) :
    ((value < 1UL<<5) ? ((value < 1UL<<4) ? 4 : 5) : ((value < 1UL<<6) ? 6 : 7))) :
    ((value < 1UL<<11) ?
        ((value < 1UL<<9) ? ((value < 1UL<<8) ? 8 : 9) : ((value < 1UL<<10) ? 10 : 11)) :
    ((value < 1UL<<13) ? ((value < 1UL<<12) ? 12 : 13) : ((value < 1UL<<14) ? 14 : 15)))) :
    ((value < 1UL<<23) ?
        ((value < 1UL<<19) ?
        ((value < 1UL<<17) ? ((value < 1UL<<16) ? 16 : 17) : ((value < 1UL<<18) ? 18 : 19)) :
    ((value < 1UL<<21) ? ((value < 1UL<<20) ? 20 : 21) : ((value < 1UL<<22) ? 22 : 23))) :
    ((value < 1UL<<27) ?
        ((value < 1UL<<25) ? ((value < 1UL<<24) ? 24 : 25) : ((value < 1UL<<26) ? 26 : 27)) :
    ((value < 1UL<<29) ? ((value < 1UL<<28) ? 28 : 29) : ((value < 1UL<<30) ? 30 : 
    ((value < 1UL<<31) ? 31 : 32))))));
}

static inline UInt GFx_BitsToUInt(SInt value)
{
    return (value >= 0) ? (UInt)value : (((UInt)GTL::gabs(value))<<1);
}

static inline UByte GFx_BitCountSInt(SInt value)
{
    return GFx_BitCountUInt(GFx_BitsToUInt(value));
}

static inline UByte GFx_BitCountSInt(SInt value1, SInt value2)
{
    return GFx_BitCountUInt(GFx_BitsToUInt(value1) | GFx_BitsToUInt(value2));
}

static inline UByte GFx_BitCountSInt(SInt value1, SInt value2, SInt value3, SInt value4)
{
    return GFx_BitCountUInt(GFx_BitsToUInt(value1) | GFx_BitsToUInt(value2) | 
                            GFx_BitsToUInt(value3) | GFx_BitsToUInt(value4));
}

template <UInt alignSize, typename T>
inline static T GFx_AlignedPtr(T ptr)
{
    return (T)(size_t(ptr + alignSize - 1) & (~(alignSize - 1)));
}

template <typename T>
inline static T GFx_AlignedPtr(T ptr, size_t alignSize)
{
    return (T)(size_t(ptr + alignSize - 1) & (~(alignSize - 1)));
}

void GFxPathPacker::Pack(GFxPathAllocator* pallocator, GFxPathsInfo* ppathsInfo)
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

    UInt fillNumBits = GFx_BitCountUInt(Fill0 | Fill1 | Line);
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

//
// ***** GFxShapeCharacterDef
//


GFxShapeCharacterDef::GFxShapeCharacterDef(UInt pageSize):
    PathAllocator(pageSize)
{
    Bound = GRectF(0,0,0,0);
    RectBound = GRectF(0,0,0,0);
    ValidBounds = 0;
    ValidHitResult = 0;
    MaxStrokeExtent = 100.0f; // Max Flash width = 200; so divide by 2.
    Flags = 0;
}


GFxShapeCharacterDef::~GFxShapeCharacterDef()
{
    ResetCache();
}

void GFxShapeCharacterDef::ResetCache()
{
    // Free our MeshSets.
    for (UInt i = 0; i < CachedMeshes.size(); i++)
    {
        delete CachedMeshes[i];
    }
    CachedMeshes.clear();
}


// Creates a definition relying on in-memory image.
void    GFxShapeCharacterDef::SetToImage(GFxImageResource *pimage, bool bilinear)
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
    RectBound = Bound;
    ValidBounds = 1;
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



//
// Shape I/O helper functions.
//

// Read fill styles, and push them onto the given style GTL::garray.
static void GFx_ReadFillStyles(GTL::garray<GFxFillStyle>* styles, GFxLoadProcess* p, GFxTagType tagType)
{
    GASSERT(styles);

    // Get the count.
    UInt fillStyleCount = p->ReadU8();
    if (tagType > GFxTag_DefineShape)
    {
        if (fillStyleCount == 0xFF)
            fillStyleCount = p->ReadU16();
    }

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

// Read line styles and push them onto the back of the given GTL::garray.
static void GFx_ReadLineStyles(GTL::garray<GFxLineStyle>* styles, GFxLoadProcess* p, GFxTagType tagType)
{
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

    // Read the styles.
    UInt baseIndex = (UInt)styles->size();
    styles->resize(baseIndex + lineStyleCount);

    for (UInt i = 0; i < lineStyleCount; i++)
    {
        p->AlignStream(); //!AB
        (*styles)[baseIndex + i].Read(p, tagType);
    }
}

void    GFxShapeCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType, bool withStyle)
{
    ValidHitResult = false;
    GFxPathAllocator* ppathAllocator = p->GetPathAllocator();
    GFxStream*  in = p->GetStream();

    if (withStyle)
    {
        ValidBounds = 1;
        in->ReadRect(&Bound);
        RectBound = Bound;

        // SWF 8 contains bounds without a stroke: MovieClip.getRect()
        if ((tagType == GFxTag_DefineShape4) || (tagType == GFxTag_DefineFont3))
        {
            in->ReadRect(&RectBound);
            // MA: What does this byte do?
            // I've seen it take on values of 0x01 and 0x00
            //UByte mysteryByte =
            in->ReadU8();
        }

        GFx_ReadFillStyles(&FillStyles, p, tagType);
        GFx_ReadLineStyles(&LineStyles, p, tagType);
    }


    // Multiplier used for scaling.
    // In SWF 8, font shape resolution (tag 75) was increased by 20.
    Float sfactor = 1.0f;
    if (tagType == GFxTag_DefineFont3)
    {
		sfactor = 1.0f / 20.0f;
        Flags |= Flags_Sfactor20;
    }

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
                FillBase = (int)FillStyles.size();
                LineBase = (int)LineStyles.size();
                GFx_ReadFillStyles(&FillStyles, p, tagType);
                GFx_ReadLineStyles(&LineStyles, p, tagType);
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
#ifdef GFX_SHAPE_MEM_TRACKING
    GFx_MT_SwfGeomMem += (in->Tell() - __startingFileOffset);
    printf("Shapes memory statistics:\n"
           "   ShapesCount = %d, PathsCount = %d, SwfGeomMem = %d, AllocatedMem = %d\n", 
           GFx_MT_ShapesCount, GFx_MT_PathsCount, GFx_MT_SwfGeomMem, GFx_MT_GeomMem);
#endif

    UPInt i, n;
    // Determine if we have any non-solid color fills.
    Flags &= (~Flags_TexturedFill);
    for (i = 0, n = FillStyles.size(); i < n; i++)
        if (FillStyles[i].GetType() != 0)
        {
            Flags |= Flags_TexturedFill;
            break;
        }

    // Compete maximum stroke extent
    MaxStrokeExtent = 1.0f;
    for(i = 0, n = LineStyles.size(); i < n; i++)
        MaxStrokeExtent = GTL::gmax(MaxStrokeExtent, LineStyles[i].GetStrokeExtent());
}


// Draw the shape using our own inherent styles.
void    GFxShapeCharacterDef::Display(GFxDisplayContext &context, GFxCharacter* inst,StackData stackData)
{
    //GRenderer::Matrix       mat     = inst->GetWorldMatrix();
    // Do viewport culling if bounds are available.
    if (ValidBounds)
    {
        GRectF           tbounds;

        GRenderer::Matrix mat = stackData.stackMatrix;

        mat.EncloseTransform(&tbounds, Bound);
        if (!inst->GetMovieRoot()->GetVisibleFrameRectInTwips().Intersects(tbounds))
        {
            if (!(context.GetRenderFlags() & GFxRenderConfig::RF_NoViewCull))
                return;
        }
    }

   /*
   GRenderer::Cxform       cx      = inst->GetWorldCxform();
   Display(context, mat, cx, blend, (inst->GetClipDepth() > 0), FillStyles, LineStyles);
	*/

	GRenderer::BlendType    blend   = inst->GetActiveBlendMode();
	Display(context, stackData.stackMatrix , stackData.stackColor , blend, (inst->GetClipDepth() > 0), FillStyles, LineStyles);
}


// Display our shape.  Use the FillStyles arg to
// override our default set of fill Styles (e.G. when
// rendering text).
void    GFxShapeCharacterDef::Display(
        GFxDisplayContext &context,
        const GRenderer::Matrix& mat,
        const GRenderer::Cxform& cx,
        GRenderer::BlendType blend,
        bool edgeAADisabled,
        const GTL::garray<GFxFillStyle>& fillStyles,
        const GTL::garray<GFxLineStyle>& lineStyles) const
{
	const GFxRenderConfig &rconfig = *context.GetRenderConfig();

if( !rconfig.DoDynamicTessellation() )
{

	if( CachedMeshes.size() == 0 )
	{


		bool    useEdgeAA        = (rconfig.IsUsingEdgeAA() &&
                                (rconfig.IsEdgeAATextured() || !HasTexturedFill()) &&
                                 !edgeAADisabled && !context.MaskRenderCount) ?  1 : 0;  

		Float   maxScale = mat.GetMaxScale();
		Float   screenPixelSize  = 20.0f / maxScale / context.GetPixelScale();
		Float   curveError       = screenPixelSize * 0.75f * rconfig.GetMaxCurvePixelError();
		bool    cxformHasAddAlpha = (fabs(cx.M_[3][1]) >= 1.0f);

		    // Construct A new GFxMesh to handle this error tolerance.
		GFxMeshSet* m = new GFxMeshSet(screenPixelSize, 
									   curveError, 
									   !useEdgeAA, 
									   rconfig.IsOptimizingTriangles());

	#ifndef GFC_NO_FXPLAYER_EDGEAA
		m->SetAllowCxformAddAlpha(cxformHasAddAlpha);
	#endif
		Tesselate(m, curveError, context);
		CachedMeshes.push_back(m);
		//m->Display(context, mat, cx, blend, fillStyles, lineStyles);
		m->Display(context, mat , cx, blend, fillStyles, lineStyles);

	}
    // See if we have an acceptable GFxMesh available; if so then render with it.
    for (UPInt i = 0, n = CachedMeshes.size(); i < n; i++)
    {
        GFxMeshSet* pcandidate = CachedMeshes[i];

        if (rconfig.IsOptimizingTriangles() != pcandidate->IsOptimizingTriangles()) 
            continue;
         pcandidate->Display(context, mat, cx, blend, fillStyles, lineStyles);
              return;
	}
}
else
{
	
    // Compute the error tolerance in object-space.
    Float   maxScale = mat.GetMaxScale();
    if (fabsf(maxScale) < 1e-6f)
    {
        // Scale is essentially zero.
        return;
    }

    // This is essentially the size of a screen pixel in shape coordinates.
    const GFxRenderConfig &rconfig = *context.GetRenderConfig();
    Float   screenPixelSize  = 20.0f / maxScale / context.GetPixelScale();
    Float   curveError       = screenPixelSize * 0.75f * rconfig.GetMaxCurvePixelError();
    bool    cxformHasAddAlpha = (fabs(cx.M_[3][1]) >= 1.0f);

#ifdef GFC_NO_FXPLAYER_EDGEAA
    bool    useEdgeAA        = 0;
    GUNUSED(edgeAADisabled);
#else
    bool    useEdgeAA        = (rconfig.IsUsingEdgeAA() &&
                                (rconfig.IsEdgeAATextured() || !HasTexturedFill()) &&
                                 !edgeAADisabled && !context.MaskRenderCount) ?  1 : 0;    
#endif

    // If Alpha is Zero, don't draw.
    // Though, if the mask is being drawn, don't consider the alpha. Tested with MovieClip.setMask (!AB)
    if ((fabs(cx.M_[3][0]) < 0.001f) && !cxformHasAddAlpha && context.MaskRenderCount == 0)
        return;

    // See if we have an acceptable GFxMesh available; if so then render with it.
    for (UPInt i = 0, n = CachedMeshes.size(); i < n; i++)
    {
        GFxMeshSet* pcandidate = CachedMeshes[i];

        if (rconfig.IsOptimizingTriangles() != pcandidate->IsOptimizingTriangles()) 
            continue;

#ifndef GFC_NO_FXPLAYER_EDGEAA
        if (pcandidate->IsUsingEdgeAA() != useEdgeAA)
        {
            // Check this to avoid repetitive excessive mesh re-generation
            // in case EdgeAA failed due to excessive vertex count.
            if (!pcandidate->HasEdgeAAFailed())
                continue;
        }

        if (useEdgeAA)
        {
            // For AA Edge we allow stretching from -0.60 (1.6x) to 1.35 (x.76). Other sizes
            // will require a new mesh.

            if ((screenPixelSize > pcandidate->GetScreenPixelSize() * 0.60f) &&
                (screenPixelSize < pcandidate->GetScreenPixelSize() * 1.35f) )
            {
                // Curve error causes more precision.
                // Note: We need to check for lower bound then the original curveError,
                // since otherwise we can get a worst-case scenario where gradual decreases
                // in curve error cause new meshes to be generated every frame.
                if ( (curveError >= pcandidate->GetCurveError() * 0.9f) &&
                     (curveError < pcandidate->GetCurveError() * 3) )
                {

                    // If the originally created mesh does not support AddAlpha, just re-create it
                    if (cxformHasAddAlpha && !pcandidate->GetAllowCxformAddAlpha())
                    {
                        CachedMeshes.remove(i);
                        delete pcandidate;
                        // We'll create the right candidate.
                        break;
                    }

                    pcandidate->Display(context, mat, cx, blend, fillStyles, lineStyles);
                    return;
                }
            }
        }
        else
#endif
        {
            // No edge AA, use curve error only.
            if ( (curveError >= pcandidate->GetCurveError() * 0.9f) &&
                 (curveError < pcandidate->GetCurveError() * 3) )
            {
                pcandidate->Display(context, mat, cx, blend, fillStyles, lineStyles);
                return;
            }
        }
    }

    // Construct A new GFxMesh to handle this error tolerance.
    GFxMeshSet* m = new GFxMeshSet(screenPixelSize, 
                                   curveError, 
                                   !useEdgeAA, 
                                   rconfig.IsOptimizingTriangles());

#ifndef GFC_NO_FXPLAYER_EDGEAA
    m->SetAllowCxformAddAlpha(cxformHasAddAlpha);
#endif
    Tesselate(m, curveError, context);
    CachedMeshes.push_back(m);
    m->Display(context, mat, cx, blend, fillStyles, lineStyles);

    // SortAndCleanMeshes();
}

}

GFxShapeCharacterDef::EdgesIterator::EdgesIterator(const GFxShapeCharacterDef::PathsIterator& pathIter)
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

void GFxShapeCharacterDef::EdgesIterator::GetEdge(Edge* pedge, bool doLines2CurveConv)
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


void GFxShapeCharacterDef::EdgesIterator::GetPlainEdge(PlainEdge* pedge)
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
                *p++  = *pVertices16++;
                *p++  = *pVertices16++;
                *p++  = *pVertices16++;
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


GFxShapeCharacterDef::PathsIterator::PathsIterator(const GFxShapeCharacterDef* pdef) : 
    CurPathsPtr(pdef->Paths.pPaths), Paths(pdef->Paths), CurIndex(0), Count(pdef->Paths.PathsCount),
    Sfactor((pdef->Flags & GFxShapeCharacterDef::Flags_Sfactor20) ? 1.0f/20.0f : 1.0f)
{
}

void GFxShapeCharacterDef::PathsIterator::AddForTesselation(GCompoundShape *cs)
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

void GFxShapeCharacterDef::PathsIterator::GetStyles(UInt* pfill0, UInt* pfill1, UInt* pline) const
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

void GFxShapeCharacterDef::PathsIterator::AdvanceBy(const EdgesIterator& edgeIt)
{
    if (edgeIt.IsFinished())
    {
        CurPathsPtr = (edgeIt.pVertices16) ? (const UByte*)edgeIt.pVertices16 : (const UByte*)edgeIt.pVertices32;
        ++CurIndex;
        CheckPage();
    }
}

void GFxShapeCharacterDef::PathsIterator::CheckPage()
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

void GFxShapeCharacterDef::PathsIterator::SkipComplex()
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

// Convert the paths to GCompoundShape, flattening the curves.
// The function ignores the NewShape flag and adds all paths
// GFxShapeCharacterDef contains.
void    GFxShapeCharacterDef::MakeCompoundShape(GCompoundShape *cs, Float tolerance) const
{
    cs->RemoveAll();
    cs->SetCurveTolerance(tolerance);
    PathsIterator it = GetPathsIterator();
    while(!it.IsFinished())
    {
        it.AddForTesselation(cs);
    }
}



//------------------------------------------------------------------------------
//
// This code implements the AUTODIN II polynomial
// The variable corresponding to the macro argument "crc" should
// be an unsigned long.
// Oroginal code  by Spencer Garrett <srg@quick.com>
//

// generated using the AUTODIN II polynomial
//   x^32 + x^26 + x^23 + x^22 + x^16 +
//   x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
//
//------------------------------------------------------------------------------
static const UInt32 GFx_Crc32Table[256] = 
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


//------------------------------------------------------------------------------
static UInt32 GFx_ComputeCrc32(const void* buf, UInt size, UInt32 crc=~0U)
{
    const UByte* p;
    UInt  len = 0; 
    UInt  nr = size;

    for (len += nr, p = (const UByte*)buf; nr--; ++p) 
    {
        crc = (crc >> 8) ^ GFx_Crc32Table[(crc ^ *p) & 0xff];
    }
    return crc;
}


UInt32 GFxShapeCharacterDef::ComputeGeometryHash() const
{
    UInt32 crc = ~0U;
    crc = GFx_ComputeCrc32(&Paths.ShapesCount, sizeof(Paths.ShapesCount), crc);
    crc = GFx_ComputeCrc32(&Paths.PathsCount,  sizeof(Paths.PathsCount),  crc);
    PathsIterator pit = GetPathsIterator();

    while(!pit.IsFinished())
    {
        EdgesIterator eit = pit.GetEdgesIterator();
        UInt numEdges = eit.GetEdgesCount();
        crc = GFx_ComputeCrc32(&numEdges, sizeof(numEdges), crc);
        while (!eit.IsFinished())
        {
            EdgesIterator::PlainEdge edge;
            eit.GetPlainEdge(&edge);
            crc = GFx_ComputeCrc32(&edge.Data, edge.Size * sizeof(SInt32), crc);
        }
        pit.AdvanceBy(eit);
    }
    return crc;
}


bool    GFxShapeCharacterDef::IsEqualGeometry(const GFxShapeCharacterDef& cmpWith) const
{
    if (Paths.ShapesCount != cmpWith.Paths.ShapesCount || 
        Paths.PathsCount  != cmpWith.Paths.PathsCount)
        return false;

    PathsIterator pit1 =         GetPathsIterator();
    PathsIterator pit2 = cmpWith.GetPathsIterator();
    while(!pit1.IsFinished())
    {
        if (pit2.IsFinished()) 
            return false;

        EdgesIterator eit1 = pit1.GetEdgesIterator();
        EdgesIterator eit2 = pit2.GetEdgesIterator();

        if (eit1.GetEdgesCount() != eit2.GetEdgesCount())
            return false;

        EdgesIterator::PlainEdge edge1;
        EdgesIterator::PlainEdge edge2;
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


// Push our shape data through the tesselator.
void    GFxShapeCharacterDef::Tesselate(GFxCompoundShapeAcceptor *p, Float tolerance,
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
    PathsIterator it = GetPathsIterator();
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
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

            it.AddForTesselation(&cs);
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

bool GFxShapeCharacterDef::PointInShape(const GCompoundShape& shape, Float x, Float y) const
{
    if (shape.PointInShape(x, y, false))
        return true;

#ifndef GFC_NO_FXPLAYER_STROKER
    GStroker        stroker;
    GCompoundShape  strokeShape;

    for (unsigned i = 0; i < LineStyles.size(); i++)
    {
        const GFxLineStyle& style = LineStyles[i];

        stroker.SetWidth(style.GetWidth());
        stroker.SetLineJoin(GFx_SWFToFxStroke_LineJoin(style.GetJoin()));
        stroker.SetStartLineCap(GFx_SWFToFxStroke_LineCap(style.GetStartCap()));
        stroker.SetEndLineCap(GFx_SWFToFxStroke_LineCap(style.GetEndCap()));
        if (style.GetJoin() == GFxLineStyle::LineJoin_Miter) 
            stroker.SetMiterLimit(style.GetMiterSize());

        strokeShape.RemoveAll();
        strokeShape.SetCurveTolerance(shape.GetCurveTolerance());
        stroker.GenerateStroke(shape, i, strokeShape);
        if (strokeShape.PointInShape(x, y, true))
            return true;
    }
#endif
    return false;
}

// Return true if the specified GRenderer::Point is on the interior of our shape.
// Incoming coords are local coords.
bool    GFxShapeCharacterDef::PointTestLocal(const GPointF &pt, 
                                             bool testShape, 
                                             const GFxPointTestCacheProvider *pcache) const
{
    GRectF b2 = Bound;

    if (!ValidBounds)
        ComputeBound(&b2);

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
    if (pcache && ValidHitResult)
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

        PathsIterator it = GetPathsIterator();
        while(!it.IsFinished())
        {
            if (it.IsNewShape())
            {
                if (PointInShape(cs, pt.x, pt.y))
                {
                    if (pcache)
                    {
                        pcache->LastHitTest       = pt;
                        pcache->LastHitTestResult = true;
                    }
                    ValidHitResult = true;
                    return true;
                }
                // Prepare compound shape for next set of paths.
                cs.RemoveAll();
                it.Skip();
            }
            else
            {
                it.AddForTesselation(&cs);
            }
        }

        result = PointInShape(cs, pt.x, pt.y);
        if (pcache)
        {
            pcache->LastHitTest       = pt;
            pcache->LastHitTestResult = result;
        }
        ValidHitResult = true;
        return result;
    }

    // Coordinate not cached, prepare shapes.
    if (!pnode->ShapesValid)
    {        
        UInt i, ishape;
        
        pnode->Shapes.resize(Paths.ShapesCount);
        if (pnode->Shapes.size() == Paths.ShapesCount)
        {
            // Set curve tolerances.
            for (i = 0; i < Paths.ShapesCount; i++)
                pnode->Shapes[i].SetCurveTolerance(curveTolerance);

            // And initialize shapes.
            ishape = 0;
            PathsIterator it = GetPathsIterator();
            while(!it.IsFinished())
            {
                if (it.IsNewShape())
                {
                    ishape++;
                    it.Skip();
                }
                else
                {
                    it.AddForTesselation(&pnode->Shapes[ishape]);
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
    ValidHitResult = true;
    return result;
}


GRectF GFxShapeCharacterDef::GetBoundsLocal() const
{
    return Bound;
}


// Find the bounds of this shape, and store them in
// the given rectangle.
void    GFxShapeCharacterDef::ComputeBound(GRectF* r) const
{
    r->Left = 1e10f;
    r->Top = 1e10f;
    r->Right = -1e10f;
    r->Bottom = -1e10f;

    PathsIterator it = GetPathsIterator();
    while(!it.IsFinished())
    {
        if (it.IsNewShape())
        {
            it.Skip();
            continue;
        }

        EdgesIterator edgesIt = it.GetEdgesIterator();
        Float ax, ay;
        edgesIt.GetMoveXY(&ax, &ay);
        r->ExpandToPoint(ax, ay);

        while (!edgesIt.IsFinished())
        {
            EdgesIterator::Edge edge;
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

