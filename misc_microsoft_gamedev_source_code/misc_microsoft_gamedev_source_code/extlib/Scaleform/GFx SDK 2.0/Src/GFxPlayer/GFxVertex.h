/**********************************************************************

Filename    :   GFxVertex.h
Content     :   Variable-format vertex and vertex array types.
Created     :   June 25, 2006
Authors     :   Michael Antonov

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXVERTEX_H
#define INC_GFXVERTEX_H

#include "GRenderer.h"


// ***** Declared Classes
class GFxVertexInterface;
class GFxVertexRef;
class GFxVertexArray;
struct GFxVertexType;
struct GFxVertexDesc;



// ***** GFxVertexRef and GFxVertexInterface

struct  GFxVertexType { UByte ByteVal; };

// Wrapper interface providing format-independent access to a vertex in memory.
class GFxVertexInterface
{
public: 
    virtual ~GFxVertexInterface () { }

    virtual void    InitVertex(GFxVertexType* pvertex, Float x, Float y, UInt32 color);
    virtual void    SetXY(GFxVertexType* pvertex, Float x, Float y);
    virtual void    GetXY(GFxVertexType* pvertex, Float *px, Float *py);
    virtual void    SetColor(GFxVertexType* pvertex, UInt32 color);
    virtual void    SetFactor(GFxVertexType* pvertex, UInt32 factor);
    virtual UInt32  GetColor(GFxVertexType* pvertex);
    virtual UInt32  GetFactor(GFxVertexType* pvertex);
};

// Vertex wrapper objects used to access individual vertices through an interface.
class GFxVertexRef
{
    GFxVertexType *     pVertex;
    GFxVertexInterface* pVI;    
public:

    GFxVertexRef(GFxVertexType *pvertex, GFxVertexInterface* pinterface) { pVertex = pvertex; pVI = pinterface; }
    GFxVertexRef(const GFxVertexRef &src)                   { pVertex = src.pVertex; pVI = src.pVI; }
    const GFxVertexRef& operator = (const GFxVertexRef &src){ pVertex = src.pVertex; pVI = src.pVI; return *this; }

    GINLINE void    InitVertex(Float x, Float y, UInt32 color = 0) { pVI->InitVertex(pVertex, x, y, color); }
    GINLINE void    SetXY(Float x, Float y)                 { pVI->SetXY(pVertex, x, y); }
    GINLINE void    GetXY(Float *px, Float *py)             { pVI->GetXY(pVertex, px, py); }
    GINLINE void    SetColor(UInt32 color)                  { pVI->SetColor(pVertex, color); }
    GINLINE void    SetFactor(UInt32 factor)                { pVI->SetFactor(pVertex, factor); }
    GINLINE UInt32  GetColor() const                        { return pVI->GetColor(pVertex); }
    GINLINE UInt32  GetFactor() const                       { return pVI->GetFactor(pVertex); }
};

struct GFxVertexDesc
{
    GRenderer::VertexFormat VFormat;
    UInt                    VertexSize;
    GFxVertexInterface*     pInterface;
};



// ***** GFxVertexArray

// Flexible array used to store different vertex formats.
class GFxVertexArray : public GNewOverrideBase
{
public:
    typedef GRenderer::VertexFormat VertexFormat;

private:
    UInt            Size;
    UInt            AllocSize;
    UByte*          pData;
    GFxVertexDesc*  pVertexDesc;

    // Gets a descriptor for format
    GFxVertexDesc*  GetFormatDesc(VertexFormat vf);

public: 

    GFxVertexArray(VertexFormat vf = GRenderer::Vertex_None, UInt size = 0);
    GFxVertexArray(const GFxVertexArray& src);
    ~GFxVertexArray();

    const GFxVertexArray& operator = (const GFxVertexArray& src);
    
    // Note: SetFormat only works when size is 0.
    bool            SetFormat(VertexFormat vf);
    VertexFormat    GetFormat() const { return pVertexDesc->VFormat; }  

    void            Resize(UInt vertexCount);
    void            Reserve(UInt reserveVertexCount, bool shrinkReserve = 0);
    void            Clear();
    GINLINE UInt    GetSize() const;    
    // Appends another array to us, must be in the same format.
    void            AppendVertices(const GFxVertexArray& src);

    // Appends a vertex an returns a pointer used to initialize it.
    GFxVertexRef    AppendVertex();

    // Inline accessors.    
    GINLINE void*       GetVertexPtr(UInt vertexIndex) const;   
    GINLINE GFxVertexRef GetVertexRef(UInt vertexIndex) const;  
    GINLINE UInt        GetVertexSize() const;  

    GFxVertexRef operator [] (UInt index) const { return GetVertexRef(index); }

    void            ApplyToRenderer(GRenderer *prenderer, GRenderer::CacheProvider *pcp) const; 
};


// ** Inline Implementation

GINLINE UInt    GFxVertexArray::GetSize() const
{
    return Size;
}
GINLINE void*   GFxVertexArray::GetVertexPtr(UInt vertexIndex) const
{
    GASSERT(vertexIndex < Size);
    return (pData + (vertexIndex * pVertexDesc->VertexSize));
}
GINLINE GFxVertexRef GFxVertexArray::GetVertexRef(UInt vertexIndex) const
{       
    return GFxVertexRef((GFxVertexType*)GetVertexPtr(vertexIndex), pVertexDesc->pInterface);
}
GINLINE UInt    GFxVertexArray::GetVertexSize() const
{
    return pVertexDesc->VertexSize;
}

// ** End Inline Implementation



#endif // INC_GFXVERTEX_H

