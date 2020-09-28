/**********************************************************************

Filename    :   GFxVertex.cpp
Content     :   Variable-format vertex array implementation.
Created     :   June 25, 2006
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxVertex.h"
#include "GRenderer.h"

// ***** Default GFxVertexInterface

void    GFxVertexInterface::InitVertex(GFxVertexType* pvertex, Float x, Float y, UInt32 color)
{ GUNUSED4(pvertex, x, y, color); }
void    GFxVertexInterface::SetXY(GFxVertexType* pvertex, Float x, Float y)
{ GUNUSED3(pvertex, x, y); }
void    GFxVertexInterface::GetXY(GFxVertexType* pvertex, Float *px, Float *py)
{
    GUNUSED(pvertex);
    *px = 0.0f;
    *py = 0.0f;
}
void    GFxVertexInterface::SetColor(GFxVertexType* pvertex, UInt32 color)
{ GUNUSED2(pvertex, color); }
void    GFxVertexInterface::SetFactor(GFxVertexType* pvertex, UInt32 factor)
{ GUNUSED2(pvertex, factor); }
UInt32  GFxVertexInterface::GetColor(GFxVertexType* pvertex)
{   
    GUNUSED(pvertex); 
    return 0;
}
UInt32  GFxVertexInterface::GetFactor(GFxVertexType* pvertex)
{
    GUNUSED(pvertex); 
    return 0;
}


// *** Other format vertex interfaces.

#ifdef GFC_OS_PSP
# define GFC_VERTEX_Z
#endif

#if (GFC_BYTE_ORDER == GFC_BIG_ENDIAN) && !defined(GFC_OS_XBOX360) && !defined(GFC_OS_PS3) && !defined(GFC_OS_WII)
#define SWAPCOLOR(color) ((color >> 24) | ((color >> 8) & 0xff00) | ((color & 0xff00) << 8) | ((color & 0xff) << 24))
#else
#define SWAPCOLOR(color) color
#endif


class GFxVertexInterface_XY16i : public GFxVertexInterface
{
public: 
    
    struct Vertex
    {
        SInt16 x;
        SInt16 y;
#ifdef GFC_VERTEX_Z
        SInt16 z;
#endif
    };

    virtual void    InitVertex(GFxVertexType* pvertex, Float x, Float y, UInt32 color)
    {
        GUNUSED(color);
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = (SInt16) x;
        p->y = (SInt16) y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
    }
    virtual void    SetXY(GFxVertexType* pvertex, Float x, Float y)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = (SInt16) x;
        p->y = (SInt16) y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
    }
    virtual void    GetXY(GFxVertexType* pvertex, Float *px, Float *py)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        *px = (Float) p->x;
        *py = (Float) p->y;
    }
};

class GFxVertexInterface_XY32f : public GFxVertexInterface
{
public: 
    
    struct Vertex
    {
        Float x;
        Float y;
#ifdef GFC_VERTEX_Z
        Float z;
#endif
    };

    virtual void    InitVertex(GFxVertexType* pvertex, Float x, Float y, UInt32 color)
    {
        GUNUSED(color);
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = x;
        p->y = y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
    }
    virtual void    SetXY(GFxVertexType* pvertex, Float x, Float y)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = x;
        p->y = y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
    }
    virtual void    GetXY(GFxVertexType* pvertex, Float *px, Float *py)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        *px = p->x;
        *py = p->y;
    }
};


#ifndef GFC_NO_FXPLAYER_EDGEAA

class GFxVertexInterface_XY16iC32 : public GFxVertexInterface
{
public: 

    typedef GRenderer::VertexXY16iC32 Vertex;

    virtual void    InitVertex(GFxVertexType* pvertex, Float x, Float y, UInt32 color)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = (SInt16) x;
        p->y = (SInt16) y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
        p->Color = SWAPCOLOR(color);
    }
    virtual void    SetXY(GFxVertexType* pvertex, Float x, Float y)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = (SInt16) x;
        p->y = (SInt16) y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
    }
    virtual void    GetXY(GFxVertexType* pvertex, Float *px, Float *py)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        *px = (Float) p->x;
        *py = (Float) p->y;
    }

    virtual void    SetColor(GFxVertexType* pvertex, UInt32 color)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->Color = SWAPCOLOR(color);
    }
    virtual UInt32  GetColor(GFxVertexType* pvertex)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        return SWAPCOLOR(p->Color);
    }
};

class GFxVertexInterface_XY16iCF32 : public GFxVertexInterface
{
public: 
    
    struct Vertex
    {
        SInt16 x;
        SInt16 y;
#ifdef GFC_VERTEX_Z
        SInt16 z;
#endif
        UInt32 Color;
        UInt32 Factor;
    };

    virtual void    InitVertex(GFxVertexType* pvertex, Float x, Float y, UInt32 color)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = (SInt16) x;
        p->y = (SInt16) y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
        p->Color = SWAPCOLOR(color);
        p->Factor = 0;
    }
    virtual void    SetXY(GFxVertexType* pvertex, Float x, Float y)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->x = (SInt16) x;
        p->y = (SInt16) y;
#ifdef GFC_VERTEX_Z
        p->z = 0;
#endif
    }
    virtual void    GetXY(GFxVertexType* pvertex, Float *px, Float *py)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        *px = (Float) p->x;
        *py = (Float) p->y;
    }

    virtual void    SetColor(GFxVertexType* pvertex, UInt32 color)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->Color = SWAPCOLOR(color);
    }
    virtual UInt32  GetColor(GFxVertexType* pvertex)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        return SWAPCOLOR(p->Color);
    }

    virtual void    SetFactor(GFxVertexType* pvertex, UInt32 factor)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        p->Factor = SWAPCOLOR(factor);
    }
    virtual UInt32  GetFactor(GFxVertexType* pvertex)
    {
        Vertex *p = reinterpret_cast<Vertex*>(pvertex);
        return SWAPCOLOR(p->Factor);
    }
};

#endif // #ifndef GFC_NO_FXPLAYER_EDGEAA


// Gets a descriptor for format
GFxVertexDesc*  GFxVertexArray::GetFormatDesc(VertexFormat vf)
{
    static GFxVertexInterface           viNone;
    static GFxVertexInterface_XY16i     viXY16i;
    static GFxVertexInterface_XY32f     viXY32f;

#ifndef GFC_NO_FXPLAYER_EDGEAA
    static GFxVertexInterface_XY16iC32  viXY16iC32;
    static GFxVertexInterface_XY16iCF32 viXY16iCF32;
#endif

    static GFxVertexDesc descTable[] =
    {
        { GRenderer::Vertex_None,       0, &viNone },
        { GRenderer::Vertex_XY16i,      sizeof(GFxVertexInterface_XY16i::Vertex),       &viXY16i },
        { GRenderer::Vertex_XY32f,      sizeof(GFxVertexInterface_XY32f::Vertex),       &viXY32f },
#ifndef GFC_NO_FXPLAYER_EDGEAA
        { GRenderer::Vertex_XY16iC32,   sizeof(GFxVertexInterface_XY16iC32::Vertex),    &viXY16iC32 },
        { GRenderer::Vertex_XY16iCF32,  sizeof(GFxVertexInterface_XY16iCF32::Vertex),   &viXY16iCF32 }  
#endif
    };

    switch(vf)
    {
        case GRenderer::Vertex_None:        return &descTable[0];
        case GRenderer::Vertex_XY16i:       return &descTable[1];
        case GRenderer::Vertex_XY32f:       return &descTable[2];

#ifndef GFC_NO_FXPLAYER_EDGEAA
        case GRenderer::Vertex_XY16iC32:    return &descTable[3];
        case GRenderer::Vertex_XY16iCF32:   return &descTable[4];
#endif
        default: break;
    }

    // Unsupported format requested.
    GASSERT(0);
    return &descTable[0];
}



GFxVertexArray::GFxVertexArray(VertexFormat vf, UInt size)
{
    Size = AllocSize = 0;
    pData = 0;
    pVertexDesc = GetFormatDesc(vf);
    Resize(size);
}

GFxVertexArray::GFxVertexArray(const GFxVertexArray& src)
{
    Size = AllocSize = 0;
    pData = 0;
    pVertexDesc = src.pVertexDesc;
    AppendVertices(src);
}

const GFxVertexArray& GFxVertexArray::operator = (const GFxVertexArray& src)
{
    // Clear buffer
    if (AllocSize*GetVertexSize() > src.GetSize()*src.GetVertexSize()*2)
        Clear();
    else    
        Resize(0);      

    // Important to call this since it also updates AllocSize
    SetFormat(src.GetFormat());
    AppendVertices(src);
    return *this;
}

GFxVertexArray::~GFxVertexArray()
{
    if (pData)
        GFREE(pData);
}


// Note: SetFormat only works when size is 0.
bool    GFxVertexArray::SetFormat(VertexFormat vf)
{
    if (Size != 0)
    {
        GFC_DEBUG_WARNING(1, "GFxVertexArray::SetFormat called on a non-zero size array");
        return 0;
    }

    UInt oldSize = AllocSize * pVertexDesc->VertexSize;

    pVertexDesc = GetFormatDesc(vf);
    AllocSize   = oldSize / pVertexDesc->VertexSize;
    return 1;
}


void    GFxVertexArray::Resize(UInt vertexCount)
{
    if (vertexCount == Size) // Takes care of 0 case to avoid warning below.
        return;
    if (pVertexDesc->VFormat == GRenderer::Vertex_None)
    {
        GFC_DEBUG_WARNING(1, "GFxVertexArray::Resize called on an array with no vertex format");
        return;
    }

    // If already allocated, done
    if (vertexCount <= AllocSize)
    {
        Size = vertexCount;
        return;
    }
    if (vertexCount <= Size)
    {
        Size = vertexCount;
        return;
    }

    // Must re-allocate
    UByte* pnewData = (UByte*)GALLOC(vertexCount * pVertexDesc->VertexSize);
    if (!pnewData)
        return;
        
    if (pData)
    {
        memcpy(pnewData, pData, Size * pVertexDesc->VertexSize);
        GFREE(pData);
    }

    Size = AllocSize = vertexCount;
    pData = pnewData;
}


void    GFxVertexArray::Reserve(UInt reserveVertexCount, bool shrinkReserve)
{
    if (reserveVertexCount == AllocSize)
        return;
    if (pVertexDesc->VFormat == GRenderer::Vertex_None)
    {
        GFC_DEBUG_WARNING(1, "GFxVertexArray::Reserve called on an array with no vertex format");
        return;
    }

    if (reserveVertexCount < Size)  
        reserveVertexCount = Size;
    
    // If already allocated, done
    if (reserveVertexCount <= AllocSize)
    {
        if (!shrinkReserve || (AllocSize == reserveVertexCount))
            return;
    }

    // Must re-allocate
    UByte* pnewData = reserveVertexCount ? (UByte*)GALLOC(reserveVertexCount * pVertexDesc->VertexSize) : (UByte*)0;
    if (!pnewData)
        return;
        
    if (pData)
    {
        if (pnewData)
            memcpy(pnewData, pData, Size * pVertexDesc->VertexSize);
        GFREE(pData);
    }

    // Size remains the same
    AllocSize = reserveVertexCount;
    pData = pnewData;
}


void    GFxVertexArray::Clear()
{
    Resize(0);
    Reserve(0, 1);
}


// Appends a vertex an returns a pointer used to initialize it.
GFxVertexRef GFxVertexArray::AppendVertex()
{
    if (AllocSize <= Size)
        Reserve(Size + 16);

    if (AllocSize > Size)
    {
        Size++;
        return GetVertexRef(Size-1);
    }

    // Memory failure!
    GASSERT(0);
    static GFxVertexInterface viNone;
    return GFxVertexRef(0, &viNone);
}


// Appends another array to us, must be in the same format.
void        GFxVertexArray::AppendVertices(const GFxVertexArray& src)
{
    if (pVertexDesc->VFormat != src.pVertexDesc->VFormat)
    {
        GFC_DEBUG_WARNING(1, "GFxVertexArray::AppendVertices fails - vertex formats of arrays do not match");
        return;
    }
    if (pVertexDesc->VFormat == GRenderer::Vertex_None)
    {
        // These arrays can have no data; so no-op.
        return;
    }

    Reserve(Size + src.Size);

    if (AllocSize >= (Size + src.Size))
    {
        Size += src.Size; // Increment before copy to avoid ASSERT in GetVertexPtr.

        if (src.pData)
            memcpy(GetVertexPtr(Size - src.Size), src.GetVertexPtr(0),
                   src.Size * pVertexDesc->VertexSize);     
    }
}

void GFxVertexArray::ApplyToRenderer(GRenderer *prenderer, GRenderer::CacheProvider *pcp) const
{
    prenderer->SetVertexData(GetVertexPtr(0), GetSize(), GetFormat(), pcp);
}


