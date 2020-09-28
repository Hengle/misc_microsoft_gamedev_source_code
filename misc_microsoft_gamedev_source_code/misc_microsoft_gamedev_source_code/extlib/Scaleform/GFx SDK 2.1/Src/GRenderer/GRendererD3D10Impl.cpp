/**********************************************************************

Filename    :   GRendererD3D10Impl.cpp
Content     :   Direct3D 10 Sample renderer implementation
Created     :   Oct 1, 2007
Authors     :   Andrew Reisse

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GImage.h"
#include "GAtomic.h"
#include "GTLTypes.h"
#include "GStd.h"
#if defined(GFC_OS_WIN32)
    #include <windows.h>
#endif

#include "GRendererCommonImpl.h"
#include <string.h> // for memset()

#include "GRendererD3D10.h"

#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#undef new
#endif
#include <D3D10.h>
#include <D3DX10.h>
#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#include "ShadersD3D10/hlsl.cpp"

// ***** Classes implemented
class GRendererD3D10Impl;
class GTextureD3D10Impl;


// ***** GTextureD3D10Impl implementation


// GTextureD3D10Impl declaration
class GTextureD3D10Impl : public GTextureD3D10
{
    D3D10_TEXTURE2D_DESC Desc;

public:

    // Renderer 
    GRendererD3D10Impl*  pRenderer;
    SInt                 Width, Height;

    // D3D10 Texture pointer
    ID3D10Texture2D*  pD3DTexture;
    ID3D10ShaderResourceView* pShaderView;

    GTextureD3D10Impl(GRendererD3D10Impl *prenderer);
    ~GTextureD3D10Impl();

    // Obtains the renderer that create TextureInfo 
    virtual GRenderer*  GetRenderer() const;        
    virtual bool        IsDataValid() const;
    
    // Init Texture implementation.
    virtual bool        InitTexture(ID3D10Texture2D *ptex, SInt width, SInt height);
    virtual bool        InitTexture(GImageBase* pim, int targetWidth, int targetHeight);    
    virtual bool        InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                    int targetWidth, int targetHeight);
    virtual bool        InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight);
    virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim);

    // Remove texture from renderer, notifies of renderer destruction
    void                RemoveFromRenderer();

    virtual ID3D10Texture2D*  GetD3DTexture() const { return pD3DTexture; }
};



// ***** Vertex Declarations and Shaders

// Vertex shader declarations we can use
enum VDeclType
{
    VD_None,
    VD_Strip,
    VD_Glyph,
    VD_XY16iC32,
    VD_XY16iCF32,
    VD_Count
};


// Our vertex coords consist of two signed 16-bit integers, for (x,y) position only.
static D3D10_INPUT_ELEMENT_DESC StripVertexDecl[] =
{
    {"POSITION",  0, DXGI_FORMAT_R16G16_SINT,     0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
};
static D3D10_INPUT_ELEMENT_DESC GlyphVertexDecl[] =
{
    {"POSITION",  0, DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0,  8, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",     0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 16, D3D10_INPUT_PER_VERTEX_DATA, 0},
};

// Gouraud vertices used with Edge AA
static D3D10_INPUT_ELEMENT_DESC VertexDeclXY16iC32[] =
{
    {"POSITION",  0, DXGI_FORMAT_R16G16_SINT,     0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",     0, DXGI_FORMAT_R8G8B8A8_UNORM,  0,  4, D3D10_INPUT_PER_VERTEX_DATA, 0},
};
static D3D10_INPUT_ELEMENT_DESC VertexDeclXY16iCF32[] =
{
    {"POSITION",  0, DXGI_FORMAT_R16G16_SINT,     0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",     0, DXGI_FORMAT_R8G8B8A8_UNORM,  0,  4, D3D10_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",     1, DXGI_FORMAT_R8G8B8A8_UNORM,  0,  8, D3D10_INPUT_PER_VERTEX_DATA, 0},
};

// Vertex declaration lookup table, must correspond to VDeclType +1
struct VDeclSpec
{
     const D3D10_INPUT_ELEMENT_DESC* pDecl;
     UInt                            Count;
};
static VDeclSpec VertexDeclTypeTable[VD_Count-1] =
{
    {StripVertexDecl, 1},
    {GlyphVertexDecl, 3},
    {VertexDeclXY16iC32, 2},
    {VertexDeclXY16iCF32, 3}
};

enum VShaderType
{
    VS_None,
    VS_Strip,
    VS_Glyph,
    VS_XY16iC32,
    VS_XY16iCF32,
    VS_XY16iCF32_T2,
    VS_Count
};

// Vertex shader text lookup table, must correspond to VShaderType +1
static const char *VertexShaderTextTable[VS_Count-1] =
{
    pStripVShaderText,
    pGlyphVShaderText,
    pStripVShaderXY16iC32Text,
    pStripVShaderXY16iCF32Text,
    pStripVShaderXY16iCF32_T2Text,
};

static const VDeclType  VertexShaderDeclTable[VS_Count] =
{
    VD_None,
    VD_Strip,
    VD_Glyph,
    VD_XY16iC32,
    VD_XY16iCF32,
    VD_XY16iCF32,
};

// Vertex buffer structure used for glyphs.
struct GGlyphVertex
{
    float x,y;
    float u,v;
    GColor color;

    void SetVertex2D(float xx, float yy, float uu, float vv, GColor c)
    {
        x = xx; y = yy; u = uu; v = vv;    
        color = c;
    }
};



// Pixel shaders
enum PixelShaderType
{
    PS_None                 = 0,
    PS_SolidColor,
    PS_CxformTexture,
    PS_CxformTextureMultiply,
    PS_TextTexture,

    
    PS_CxformGauraud,
    PS_CxformGauraudNoAddAlpha,
    PS_CxformGauraudTexture,
    PS_Cxform2Texture,
    
    // Multiplies - must come in same order as other gourauds
    PS_CxformGauraudMultiply,
    PS_CxformGauraudMultiplyNoAddAlpha,
    PS_CxformGauraudMultiplyTexture,
    PS_CxformMultiply2Texture,

    PS_Count,
};



static const char* PixelShaderInitTable[PS_Count-1] =
{   
    // Non-AA Shaders.
    pSource_PS_SolidColor,
    pSource_PS_CxformTexture,
    pSource_PS_CxformTextureMultiply,
    pSource_PS_TextTextureColor,

    // AA Shaders.
    pSource_PS_CxformGauraud, 
    pSource_PS_CxformGauraudNoAddAlpha,
    pSource_PS_CxformGauraudTexture, 
    pSource_PS_Cxform2Texture, 
    pSource_PS_CxformGauraudMultiply,       
    pSource_PS_CxformGauraudMultiplyNoAddAlpha, 
    pSource_PS_CxformGauraudMultiplyTexture,
    pSource_PS_CxformMultiply2Texture
};

// Blending

enum BlendStateTypes
{
    BlendDesc_Disable,
    BlendDesc_Normal,
    BlendDesc_Add,
    BlendDesc_Subtract,
    BlendDesc_Multiply,
    BlendDesc_Lighten,
    BlendDesc_Darken,
    BlendDesc_Stencil,
    BlendDesc_Count
};

static const D3D10_BLEND_DESC BlendDescs[BlendDesc_Count] =
{
    { 0, {0}, D3D10_BLEND_ONE,       D3D10_BLEND_ZERO,          D3D10_BLEND_OP_ADD,
              D3D10_BLEND_ONE,       D3D10_BLEND_ZERO,          D3D10_BLEND_OP_ADD, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {1}, D3D10_BLEND_SRC_ALPHA, D3D10_BLEND_INV_SRC_ALPHA, D3D10_BLEND_OP_ADD,
              D3D10_BLEND_ONE,       D3D10_BLEND_INV_SRC_ALPHA, D3D10_BLEND_OP_ADD, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {1}, D3D10_BLEND_SRC_ALPHA, D3D10_BLEND_ONE,           D3D10_BLEND_OP_ADD,
              D3D10_BLEND_ZERO,      D3D10_BLEND_ONE,           D3D10_BLEND_OP_ADD, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {1}, D3D10_BLEND_SRC_ALPHA, D3D10_BLEND_ONE,           D3D10_BLEND_OP_REV_SUBTRACT,
              D3D10_BLEND_ZERO,      D3D10_BLEND_ONE,           D3D10_BLEND_OP_REV_SUBTRACT, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {1}, D3D10_BLEND_DEST_COLOR,D3D10_BLEND_ZERO,          D3D10_BLEND_OP_ADD,
              D3D10_BLEND_DEST_ALPHA,D3D10_BLEND_ZERO,          D3D10_BLEND_OP_ADD, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {1}, D3D10_BLEND_SRC_ALPHA, D3D10_BLEND_ONE,           D3D10_BLEND_OP_MAX,
              D3D10_BLEND_ONE,       D3D10_BLEND_ONE,           D3D10_BLEND_OP_MAX, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {1}, D3D10_BLEND_SRC_ALPHA, D3D10_BLEND_ONE,           D3D10_BLEND_OP_MIN,
              D3D10_BLEND_ONE,       D3D10_BLEND_ONE,           D3D10_BLEND_OP_MIN, {D3D10_COLOR_WRITE_ENABLE_ALL} },

    { 0, {0}, D3D10_BLEND_SRC_ALPHA, D3D10_BLEND_ONE,           D3D10_BLEND_OP_ADD,
              D3D10_BLEND_ONE,       D3D10_BLEND_ONE,           D3D10_BLEND_OP_ADD, {0} }
};

static const int BlendModeStates[] =
{
    BlendDesc_Normal,
    BlendDesc_Normal,
    BlendDesc_Normal,
    BlendDesc_Multiply,
    BlendDesc_Normal,
    BlendDesc_Lighten,
    BlendDesc_Darken,
    BlendDesc_Normal,
    BlendDesc_Add,
    BlendDesc_Subtract,
    BlendDesc_Normal,
    BlendDesc_Stencil,
    BlendDesc_Stencil,
    BlendDesc_Normal,
    BlendDesc_Normal
};


// **** GDynamicVertexStream - Dynamic vertex buffer streaming manager

// This class tracks the vertex and index buffers that were set on the 
// renderer and streams them into dynamic buffers intelligently.
// Extra logic is included to handle buffer overruns; for example if
// the index buffer passed does not fit into allocated buffer, it
// will be streamed and rendered in pieces.

class GDynamicVertexStream
{
public:

    // Delegate used types for convenience.
    typedef GRenderer::IndexFormat   IndexFormat;
    typedef GRenderer::VertexFormat  VertexFormat;
    typedef GRenderer::VertexXY16iC32  VertexXY16iC32;
    typedef GRenderer::VertexXY16iCF32  VertexXY16iCF32;

    // Default sizes of various buffers - in bytes; Change these to control memory that is used    
    enum {
        // Warning: if vertex buffer size is not large enough for 65K vertices,
        // we will fall back to SW indexing for those buffers that don't fit.
        DefaultVertexBufferSize     = 0x10000 * sizeof(VertexXY16iCF32),
        DefaultIndexBufferSize      = 0x18000 * sizeof(UInt16),

        GlyphBufferVertexCount      = 6 * 192,
        GlyphBufferSize             = GlyphBufferVertexCount * sizeof(GGlyphVertex)
    };

    
    // PrimitiveDesc - package structure for DrawPrimitive args.
    struct PrimitiveDesc
    {
        int    BaseVertexIndex, MinVertexIndex, NumVertices;
        int    StartIndex, TriangleCount;

        PrimitiveDesc()
        {
            BaseVertexIndex = 0;
            MinVertexIndex  = 0;
            NumVertices     = 0;
            StartIndex      = 0;
            TriangleCount   = 0;
        }
        PrimitiveDesc(int baseVertexIndex, int minVertexIndex, int numVertices,
                      int startIndex, int triangleCount )
        {
            BaseVertexIndex = baseVertexIndex;
            MinVertexIndex  = minVertexIndex;
            NumVertices     = numVertices;
            StartIndex      = startIndex;
            TriangleCount   = triangleCount;
        }
    };
    
private:
    // Direct3D Device we are using.
    ID3D10Device*           pDevice;

    // Flag set if buffers are released due to lost device.
    bool                    LostDevice;

    // Vertex data pointer
    const void*             pVertexData;
    const void*             pIndexData;
    VertexFormat            VertexFmt;
    UInt                    VertexSize;
    DXGI_FORMAT             IndexFmt;
    UInt                    VertexCount;
    UInt                    IndexCount;

    // Dynamic Vertex/Index buffer state 
    GPtr<ID3D10Buffer>      pVertexBuffer;
    GPtr<ID3D10Buffer>      pIndexBuffer;
    // Allocated sizes
    UInt                    VertexBufferSize;
    UInt                    IndexBufferSize;

    // Next available offset in vertex / index buffers
    UInt                    NextIBOffset;
    UInt                    NextVBOffset;
    // If vertex data was uploaded into VB, VBDataInBuffer flag is set
    // and VBDataOffset points to its first byte.
    bool                    VBDataInBuffer;
    bool                    IBDataInBuffer;
    UInt                    VBDataOffset;
    UInt                    VBDataIndex; // Used in D3D8 to support multiple arrays in vertex buffer
    UInt                    IBDataOffset;

    // Vertex size of last data in buffer. Can be different from VertexSize
    // if data was uploaded directly through lock.
    UInt                    VertexSizeInBuffer;

    PrimitiveDesc           Primitive;     

    enum RenderMethodType
    {
        RM_None,
        RM_Indexed,
        RM_IndexedInChunks,
        RM_NotIndexed,
    };

    RenderMethodType        RenderMethod;
    int                     MaxTriangleCount;

    // Helper functions to create/release vertex and index buffers.
    bool    CreateDynamicBuffers(); 

    // Helper, performs un-indexing and copy of data into the target vertex buffer.    
    void    InitVerticesFromIndex(void *pvertexDest, int baseVertexIndex, int vertexSize,
                                  int startIndex, int triangleCount );
public:


    GDynamicVertexStream();   
    ~GDynamicVertexStream();


    // Initializes stream, creating buffers. Called at renderer initialization.
    bool    Initialize(ID3D10Device *pdevice)
    {
        pDevice = pdevice;
        if (CreateDynamicBuffers())        
            return 1;        
        pDevice = 0;
        return 0;
    }

    void    BeginDisplay()
    {
        if (LostDevice)
            CreateDynamicBuffers();
    }

    // Called for a lost device.
    void    ReleaseDynamicBuffers(bool lostDevice = 0);


    // *** Vertex Upload / Streaming interfaces.

    // We allow for two types of streaming:
    //
    //  1.  Direct upload by calling Lock/UnloadVertexBuffer. Such data
    //      can be rendered by calling DrawPrimitive after unlock.
    //
    //  2.  Indexed upload through SetVertexData + SetIndexData. This data is
    //      rendered through PrepareTriangleData / DrawTriangles.
    //

    // Raw buffer locking; also configures the stream.
    void*   LockVertexBuffer(UInt vertexCount, UInt vertexSize);
    void    UnlockVertexBuffer();
    void*   LockIndexBuffer(UInt indexCount, UInt indexSize);
    void    UnlockIndexBuffer();
    // Copies vertices directly into buffer; uses lock.
    bool    InitVertexBufferData(int vertexIndex, int vertexCount);

    // Indexed data specification, used by PrepareVertexData.
    void    SetVertexData(const void* pvertices, int numVertices, VertexFormat vf);
    void    SetIndexData(const void* pindices, int numIndices, IndexFormat idxf);

    // Specifies data ranges for DrawTriangles.
    bool    PrepareVertexData(const PrimitiveDesc &prim);    
    // Renders triangles prepared in buffer.
    void    DrawTriangles();    


    // Data availability check methods.
    bool            HasVertexData() const       { return (pVertexData != 0); }
    bool            HasIndexData() const        { return (pIndexData != 0); }
    VertexFormat    GetVertexFormat() const     { return VertexFmt; }
    DXGI_FORMAT     GetD3DIndexFormat() const   { return IndexFmt; }
    UInt            GetStartIndex() const       { return VBDataIndex; }
};


// *** Dynamic Vertex Stream implementation

GDynamicVertexStream::GDynamicVertexStream()
{
    pDevice             = 0;
    LostDevice          = 0;

    pVertexData         = 0;
    pIndexData          = 0;
    IndexFmt            = DXGI_FORMAT_R16_UINT;
    VertexFmt           = GRenderer::Vertex_None;
    VertexCount         = 0;
    IndexCount          = 0;

    NextIBOffset        = 0;
    NextVBOffset        = 0;
    VBDataInBuffer      = 0;
    IBDataInBuffer      = 0;
    VBDataOffset        = 0;
    IBDataOffset        = 0;
    VBDataIndex         = 0;

    GCOMPILER_ASSERT(DefaultVertexBufferSize >= GlyphBufferSize);
}

GDynamicVertexStream::~GDynamicVertexStream()
{
    ReleaseDynamicBuffers();
}


// Helper function to create vertex and index buffers
bool    GDynamicVertexStream::CreateDynamicBuffers()
{
    VertexBufferSize = DefaultVertexBufferSize;
    IndexBufferSize  = DefaultIndexBufferSize;
    
    if (!pVertexBuffer)
    {
        HRESULT createResult;
        D3D10_BUFFER_DESC bd;

        // Min acceptable buffer must be large enough for glyphs due to logic used in DrawBitmaps.
        GCOMPILER_ASSERT(DefaultVertexBufferSize/8 >= GlyphBufferSize);

        memset(&bd, 0, sizeof(D3D10_BUFFER_DESC));
        bd.Usage = D3D10_USAGE_DYNAMIC;
        bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

        // Loop trying several vertex buffer sizes.
        do
        {
            bd.ByteWidth = VertexBufferSize;

            createResult = pDevice->CreateBuffer(&bd, NULL, &pVertexBuffer.GetRawRef());
            
            // If failed, try smaller size while reasonable.
            if (FAILED(createResult))
            {                
                if (VertexBufferSize > DefaultVertexBufferSize/8)
                    VertexBufferSize = VertexBufferSize / 2;
                else
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3D10 - Failed to create dynamic vertex buffer");
                    VertexBufferSize = 0;
                    return 0;
                }
            }

        } while(FAILED(createResult));
    }

    if (!pIndexBuffer)
    {
        HRESULT createResult;
        D3D10_BUFFER_DESC bd;

        memset(&bd, 0, sizeof(D3D10_BUFFER_DESC));
        bd.Usage = D3D10_USAGE_DYNAMIC;
        bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

        do
        {
            bd.ByteWidth = IndexBufferSize;
            createResult = pDevice->CreateBuffer(&bd, NULL, &pIndexBuffer.GetRawRef());

            // If failed, try smaller size while reasonable.
            if (FAILED(createResult))
            {
                if (IndexBufferSize > DefaultIndexBufferSize/4)
                    IndexBufferSize = IndexBufferSize / 2;
                else
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3D10 - Failed to create dynamic index buffer");
                    pVertexBuffer    = 0; // Release VB.
                    IndexBufferSize  = 0;
                    VertexBufferSize = 0;
                    return 0;
                }
            }

        } while(FAILED(createResult));
    }    

    // Initialize buffer pointers to the beginning.
    NextIBOffset        = 0;
    NextVBOffset        = 0;
    VBDataInBuffer      = 0;
    IBDataInBuffer      = 0;
    VBDataOffset        = 0;
    IBDataOffset        = 0;

    VertexSizeInBuffer  = 0;

    RenderMethod        = RM_None;
    MaxTriangleCount    = 0;

    // After successful creation buffers are no longer lost.
    LostDevice          = 0;

    return 1;
}


void    GDynamicVertexStream::ReleaseDynamicBuffers(bool lostDevice)
{
    pVertexBuffer       = 0;
    pIndexBuffer        = 0;
    VertexBufferSize    = 0;
    IndexBufferSize     = 0;
    RenderMethod        = RM_None;
    MaxTriangleCount    = 0;

    LostDevice          = lostDevice;
}



void    GDynamicVertexStream::SetVertexData(const void* pvertices, int numVertices, VertexFormat vf)
{
    pVertexData = pvertices;
    VertexFmt   = vf;
    VertexCount = numVertices;
    VBDataInBuffer = 0;

    VertexSize  = 2 * sizeof(SInt16);
    switch(VertexFmt)
    {
    case    GRenderer::Vertex_None:        VertexSize = 0;    break;
    case    GRenderer::Vertex_XY16iC32:    VertexSize = sizeof(GRenderer::VertexXY16iC32);    break;
    case    GRenderer::Vertex_XY16iCF32:   VertexSize = sizeof(GRenderer::VertexXY16iCF32);   break;
    }     
}

void    GDynamicVertexStream::SetIndexData(const void* pindices, int numIndices, IndexFormat idxf)
{
    pIndexData  = pindices;
    IndexCount  = numIndices;
    IBDataInBuffer  = 0;
    switch(idxf)
    {
    case GRenderer::Index_None:    IndexFmt = DXGI_FORMAT_UNKNOWN; break;
    case GRenderer::Index_16:      IndexFmt = DXGI_FORMAT_R16_UINT; break;
    case GRenderer::Index_32:      IndexFmt = DXGI_FORMAT_R32_UINT; break;
    }       
}



bool    GDynamicVertexStream::PrepareVertexData(const PrimitiveDesc &prim)
{
    if (LostDevice)
        return 0;

    Primitive = prim;

    // Simple heuristic for performance: If VBs are large, use indexed buffers;
    // otherwise un-index and upload vertices manually. This keeps the Lock
    // overhead lower, since we only have to lock one buffer instead of two
    // for small objects.
    if (VertexCount < 32)
    {
        MaxTriangleCount = VertexBufferSize / (VertexSize * 3);
        RenderMethod     = RM_NotIndexed;
    }

    else
    {
        // Upload vertex data if it is not there.
        if (!VBDataInBuffer)
        {         
            // If vertex buffer is not large enough, we must render without indexing.
            if (VertexCount * VertexSize > VertexBufferSize)
            {
                // Will need to render 3 vertices at a time.
                MaxTriangleCount = VertexBufferSize / (VertexSize * 3);
                RenderMethod     = RM_NotIndexed;
                return 1;
            }

            void *  pbuffer = LockVertexBuffer(VertexCount, VertexSize);
            if (pbuffer)
            {
                memcpy(pbuffer, pVertexData, VertexCount * VertexSize);
                UnlockVertexBuffer();
                VBDataInBuffer = 1;
            }
            else
            {
                RenderMethod = RM_None;
                return 0;
            }
        }

        // Upload index data.
        if (!IBDataInBuffer)
        {   
            UInt    indexSize = (IndexFmt == DXGI_FORMAT_R16_UINT) ? sizeof(UInt16) : sizeof(UInt32);

            if (IndexCount * indexSize > IndexBufferSize)
            {
                MaxTriangleCount = IndexBufferSize / (indexSize * 3);
                RenderMethod     = RM_IndexedInChunks;
                return 1;
            }

            void *  pbuffer = LockIndexBuffer(IndexCount, indexSize);
            if (pbuffer)
            {
                // Need to know the right index buffer spot.
                memcpy(pbuffer, pIndexData, IndexCount * indexSize);
                UnlockIndexBuffer();
                IBDataInBuffer = 1;
            }
            else
            {
               RenderMethod = RM_None;
               return 0;
            }
        }

        RenderMethod = RM_Indexed;
    }
   
    return 1;
}


void    GDynamicVertexStream::DrawTriangles()
{
    if (RenderMethod == RM_Indexed)
    {
        // Draw the mesh with indexed buffers.
        GASSERT(IndexFmt == DXGI_FORMAT_R16_UINT);

        pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pDevice->DrawIndexed(Primitive.TriangleCount * 3, (IBDataOffset / sizeof(UInt16)) + Primitive.StartIndex,
            Primitive.BaseVertexIndex);
    }

    else if (RenderMethod == RM_NotIndexed)
    {
        // Vertex buffer could not fit, render in chunks with software indexing.       
        int    triangles = 0;

        // For now, InitVerticesFromIndex doesn't handle 32 bit buffers.
        GASSERT(IndexFmt == DXGI_FORMAT_R16_UINT);

        while(triangles < Primitive.TriangleCount)
        {
            int     batch   = GTL::gmin<int>(Primitive.TriangleCount - triangles, MaxTriangleCount);
            void *  pbuffer = LockVertexBuffer(batch * 3, VertexSize);
            if (!pbuffer)
                return;

            // Copy this batches indices.
            InitVerticesFromIndex(pbuffer, Primitive.BaseVertexIndex, VertexSize,
                                  Primitive.StartIndex + (triangles * 3), batch);
            UnlockVertexBuffer();

            // Draw using uploaded data.
            pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pDevice->Draw(batch * 3, GetStartIndex());
            triangles += batch;
        }
    }

    else if (RenderMethod == RM_IndexedInChunks)
    {
        // We have vertex buffer, but not index buffer.
        int    triangles = 0;
        
        while(triangles < Primitive.TriangleCount)
        {
            int    batch   = GTL::gmin<int>(Primitive.TriangleCount - triangles, MaxTriangleCount);
            UInt    indexSize = (IndexFmt == DXGI_FORMAT_R16_UINT) ? sizeof(UInt16) : sizeof(UInt32);
            void *  pbuffer = LockIndexBuffer(batch * 3, indexSize);
            if (!pbuffer)
                return;
            
            // Copy this indices for this batch.
            memcpy( pbuffer,
                    ((UByte*)pIndexData) + (Primitive.StartIndex +(triangles * 3)) * indexSize,
                    batch * 3 * indexSize );
            UnlockIndexBuffer();

            // Draw using uploaded data.
            pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            pDevice->DrawIndexed(batch * 3, (IBDataOffset / sizeof(UInt16)), Primitive.BaseVertexIndex);

            triangles += batch;
        }
    }    

}



// Lock / Unlock logic.
void*   GDynamicVertexStream::LockVertexBuffer(UInt vertexCount, UInt vertexSize)
{
    if (LostDevice)
        return 0;

    UInt    size = vertexCount * vertexSize;
    if ((size > VertexBufferSize) || !pVertexBuffer)
    {
        GASSERT(0);
        return 0;
    }

    D3D10_MAP lockFlags; 
    void*   pbuffer = 0;
    UInt    offset;

    VBDataInBuffer = 0;

    // Determine where in the buffer we go.
    if (size + NextVBOffset + vertexSize < VertexBufferSize)
    {
        offset          = NextVBOffset;
        NextVBOffset    = offset + size;
        lockFlags       = D3D10_MAP_WRITE_NO_OVERWRITE;
    }
    else
    {
        offset          = 0;
        NextVBOffset    = size;
        lockFlags       = D3D10_MAP_WRITE_DISCARD;
    }

    if (FAILED(pVertexBuffer->Map(lockFlags, 0, &pbuffer)))
        return 0;

    // Some drivers may return bad pointers here for dynamic buffers (i.e. old NVidia drivers)
    if (::IsBadWritePtr(pbuffer, size))
    {
        static bool warningReported = 0;
        GFC_DEBUG_WARNING(!warningReported,
                          "GD3D10Renderer::LockVertexData - Direct3D Lock returned bad pointer, function failed");
        warningReported = 1;
        pVertexBuffer->Unmap();
        pbuffer = 0;
        return 0;
    }

    VBDataOffset       = offset;
    VertexSizeInBuffer = vertexSize;
    return ((UByte*)pbuffer) + offset;
}


void    GDynamicVertexStream::UnlockVertexBuffer()
{
    pVertexBuffer->Unmap();

    // Set this buffer offset on a device.
    pDevice->IASetVertexBuffers(0, 1, &pVertexBuffer.GetRawRef(), &VertexSizeInBuffer, &VBDataOffset);
}


void*   GDynamicVertexStream::LockIndexBuffer(UInt indexCount, UInt indexSize)
{
    if (LostDevice)
        return 0;

    UInt    size = indexCount * indexSize;
    if ((size > IndexBufferSize) || !pIndexBuffer)
    {
        GASSERT(0);
        return 0;
    }

    D3D10_MAP   lockFlags; 
    void*   pbuffer = 0;
    UInt    offset;

    IBDataInBuffer = 0;

    // Determine where in the buffer we go.
    if ((IndexBufferSize - NextIBOffset) > size)
    {
        offset          = NextIBOffset;
        NextIBOffset    = NextIBOffset + size;
        lockFlags       = D3D10_MAP_WRITE_NO_OVERWRITE;
    }
    else
    {
        offset          = 0;
        NextIBOffset    = size;
        lockFlags       = D3D10_MAP_WRITE_DISCARD;
    }

    if (FAILED(pIndexBuffer->Map(lockFlags, 0, (void**)&pbuffer)))   
        return 0;
    if (::IsBadWritePtr(pbuffer, size))
    {        
        pIndexBuffer->Unmap();
        pbuffer = 0;
        return 0;
    }

    IBDataOffset = offset;
    return ((UByte*)pbuffer) + offset;
}

void    GDynamicVertexStream::UnlockIndexBuffer()
{
    pIndexBuffer->Unmap();        
    pDevice->IASetIndexBuffer(pIndexBuffer, IndexFmt, 0);
}

bool    GDynamicVertexStream::InitVertexBufferData(int vertexIndex, int vertexCount)
{
    GASSERT(UInt(vertexCount + vertexIndex) <= VertexCount);

    void *  pbuffer = LockVertexBuffer(vertexCount, VertexSize);
    if (!pbuffer)
        return 0;    
    memcpy(pbuffer, ((UByte*)pVertexData) + vertexIndex * VertexSize, vertexCount * VertexSize);
    UnlockVertexBuffer();
    return 1;
}


// Performs un-indexing and copy of data into the target vertex buffer.    
void    GDynamicVertexStream::InitVerticesFromIndex( void *pvertexDest, int baseVertexIndex, int vertexSize,
                                                     int startIndex, int triangleCount )
{
    UInt16* pindices  = ((UInt16*)pIndexData) + startIndex;        
    UByte*  pvertices = ((UByte*)pVertexData) + baseVertexIndex * vertexSize;
    UByte*  pdest     = (UByte*) pvertexDest;
    // 32-bit pointer versions.
    UInt32* pv32      = (UInt32*) pvertices;
    UInt32* pd32      = (UInt32*) pdest;


    GCOMPILER_ASSERT(sizeof(UInt32) == 4);       
    int     i;

    switch(vertexSize)
    {
    case 4:

        for(i = 0; i< triangleCount; i++, pd32 += 3)
        {
            pd32[0] = pv32[*(pindices + 0)];
            pd32[1] = pv32[*(pindices + 1)];
            pd32[2] = pv32[*(pindices + 2)];
            pindices += 3;
        }
        break;

    case 8:
        GCOMPILER_ASSERT(sizeof(VertexXY16iC32) == 8);

        for(i = 0; i< triangleCount; i++, pd32 += 3 * 2)
        {
            pd32[0] = pv32[*pindices * 2];
            pd32[1] = pv32[*pindices * 2 + 1];
            pindices ++;
            pd32[2] = pv32[*pindices * 2];
            pd32[3] = pv32[*pindices * 2 + 1];
            pindices ++;
            pd32[4] = pv32[*pindices * 2];
            pd32[5] = pv32[*pindices * 2 + 1];
            pindices ++;
        }
        break;

    case 12:
        GCOMPILER_ASSERT(sizeof(VertexXY16iCF32) == 12);

        for(i = 0; i< triangleCount; i++, pd32 += 3 * 3)
        {
            pd32[0] = pv32[*pindices * 3];
            pd32[1] = pv32[*pindices * 3 + 1];
            pd32[2] = pv32[*pindices * 3 + 2];
            pindices ++;
            pd32[3] = pv32[*pindices * 3];
            pd32[4] = pv32[*pindices * 3 + 1];
            pd32[5] = pv32[*pindices * 3 + 2];
            pindices ++;
            pd32[6] = pv32[*pindices * 3];
            pd32[7] = pv32[*pindices * 3 + 1];
            pd32[8] = pv32[*pindices * 3 + 2];
            pindices ++;
        }
        break;            

    default:             
        for(i = 0; i< triangleCount; i++, pdest += vertexSize*3)
        {
            memcpy(pdest, pvertices + *pindices * vertexSize, vertexSize);
            pindices ++;
            memcpy(pdest, pvertices + *pindices * vertexSize, vertexSize);
            pindices ++;
            memcpy(pdest, pvertices + *pindices * vertexSize, vertexSize);
            pindices ++;
        }
        break;
    }
}





// ***** GRendererD3D10 Implementation

class GRendererD3D10Impl : public GRendererD3D10
{
public:

    // Some renderer state. 
    bool                ModeSet;
    SInt                RenderMode;

    // Shaders and declarations that are currently set.
    VDeclType           VDeclIndex;
    VShaderType         VShaderIndex;

    // Current pixel shader index
    PixelShaderType     PShaderIndex;

    // Video Mode Configuration Flags (VMConfigFlags)
    UInt32              VMCFlags;

    // render target
    GPtr<ID3D10RenderTargetView> pRenderTarget;
    GPtr<ID3D10DepthStencilView> pDepthStencil;

    // Created vertex declarations and shaders.
    GPtr<ID3D10InputLayout>      VertexDecls[VS_Count];
    GPtr<ID3D10VertexShader>     VertexShaders[VS_Count];
    GPtr<ID3D10Buffer>           VShaderConst;

    struct VShaderConstData
    {
        float mvp[4][4];
        float texgen[16];
    };
    VShaderConstData             VShaderData;

    // Allocated pixel shaders
    GPtr<ID3D10PixelShader>      PixelShaders[PS_Count];
    GPtr<ID3D10Buffer>           PShaderConst;
    GPtr<ID3D10SamplerState>     SamplerStates[4];

    GPtr<ID3D10BlendState>       BlendStates[BlendDesc_Count];
    GPtr<ID3D10RasterizerState>  RasterStates[2]; // No multisample, multisample
    GPtr<ID3D10DepthStencilState> StencilDisabled, StencilSet, StencilIncr, StencilDecr, StencilTest;

    // Direct3DDevice
    ID3D10Device*       pDevice;
    
    bool                DrawingMask;
    UInt32              StencilCounter;
    
    // Output size.
    Float               DisplayWidth;
    Float               DisplayHeight;
    
    Matrix              UserMatrix;
    Matrix              ViewportMatrix;
    Matrix              CurrentMatrix;
    Cxform              CurrentCxform;

    // Link list of all allocated textures
    GRendererNode       Textures;
    mutable GLock       TexturesLock;

    Stats               RenderStats;

   
    // Vertex stream / dynamic buffers container.
    GDynamicVertexStream    VertexStream;

  
    // Current sample mode
    BitmapSampleMode        SampleMode[2];


    // Current blend mode
    BlendType               BlendMode;
    GTL::garray<BlendType>  BlendModeStack;


    // Linked list used for buffer cache testing, otherwise holds no data.
    CacheNode               CacheList;

    // all D3D calls should be done on the main thread, but textures can be released from 
    // different threads so we collect system resources and release them on the main thread
    GTL::garray<ID3D10Texture2D*> ResourceReleasingQueue;
    GLock                           ResourceReleasingQueueLock;

    // this method is called from GTextureXXX destructor to put a system resource into releasing queue
    void AddResourceForReleasing(ID3D10Texture2D* ptexture)
    {
        GASSERT(ptexture);
        if (!ptexture) return;
        GLock::Locker guard(&ResourceReleasingQueueLock);
        ResourceReleasingQueue.push_back(ptexture);
    }

    // this method is called from GetTexture, EndDisplay and destructor to actually release
    // collected system resources
    void ReleaseQueuedResources()
    {
        GLock::Locker guard(&ResourceReleasingQueueLock);
        for (UInt i = 0; i < ResourceReleasingQueue.size(); ++i)
            ResourceReleasingQueue[i]->Release();
        ResourceReleasingQueue.clear();
    }

    GRendererD3D10Impl()
    {
        ModeSet             = 0;
        VMCFlags            = 0;
        StencilCounter      = 0;
        VDeclIndex          = VD_None;
        VShaderIndex        = VS_None;
        PShaderIndex        = PS_None;

        SampleMode[0]       = Sample_Linear;      
        SampleMode[1]       = Sample_Linear;      
        
        pDevice             = 0;
        BlendModeStack.set_size_policy(GTL::garray<BlendType>::Buffer_NoShrink);
        BlendMode           = Blend_None;

        RenderMode          = 0;

        for (int i = 0; i < 16; i++)
        {
            VShaderData.mvp[i>>2][i&3] = 0;
            VShaderData.texgen[i] = 0;
        }
    }

    ~GRendererD3D10Impl()
    {
        Clear();
    }

    void Clear()
    {
        if (pDevice)
        {
            UInt n = 0;
            ID3D10Buffer *buf = 0;
            ID3D10SamplerState* samp[2] = {0,0};
            ID3D10ShaderResourceView* srs[2] = {0,0};

            pDevice->VSSetShader(0);
            pDevice->VSSetConstantBuffers(0, 1, &buf);
            pDevice->PSSetConstantBuffers(0, 1, &buf);
            pDevice->PSSetSamplers(0, 2, samp);
            pDevice->PSSetShaderResources(0, 2, srs);

            pDevice->RSSetState(0);
            pDevice->OMSetBlendState(0, 0, 0xffffffff);
            pDevice->OMSetDepthStencilState(0, 0);
            pDevice->IASetIndexBuffer(0, DXGI_FORMAT_UNKNOWN, 0);
            pDevice->IASetInputLayout(0);
            pDevice->IASetVertexBuffers(0, 1, &buf, &n, &n);
            pDevice->PSSetShader(0);
            pDevice->VSSetShader(0);
        }
        pDevice = NULL;

        // Remove/notify all textures
        
        {
        GLock::Locker guard(&TexturesLock);
        while (Textures.pFirst != &Textures)
            ((GTextureD3D10Impl*)Textures.pFirst)->RemoveFromRenderer();
        }

        CacheList.ReleaseList();

        if (ModeSet)
            ResetVideoMode();
        ReleaseQueuedResources();
    }

    void ReleaseResources()
    {
        Clear();
    }

    bool    CreateVertexShader(GPtr<ID3D10VertexShader> *pshaderPtr, GPtr<ID3D10InputLayout> *pdeclPtr, const VDeclSpec *pdeclspec,
        const char* pshaderText)
    {
        if (!*pshaderPtr)
        {
            GPtr<ID3D10Blob> pshader;
            GPtr<ID3D10Blob> pmsg;
            HRESULT hr;

            hr = D3D10CompileShader(pshaderText, strlen(pshaderText), NULL,
                NULL, NULL, "main", GRENDERER_VSHADER_PROFILE, D3D10_SHADER_DEBUG, &pshader.GetRawRef(), &pmsg.GetRawRef());

            if (FAILED(hr))
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3D10 - VertexShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }
            if (!pshader ||
                ((hr = pDevice->CreateVertexShader(pshader->GetBufferPointer(), pshader->GetBufferSize(),
                    &pshaderPtr->GetRawRef())) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3D10 - Can't create D3D10 vshader; error code = %d", hr);
                return 0;
            }
            if (!*pdeclPtr)
            {
                if (pDevice->CreateInputLayout(pdeclspec->pDecl, pdeclspec->Count,
                    pshader->GetBufferPointer(), pshader->GetBufferSize(), &pdeclPtr->GetRawRef()) != S_OK)
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3D10 - failed to create input layout");
                    return 0;
                }
            }
        }
        return 1;
    }

    bool    CreatePixelShader(GPtr<ID3D10PixelShader> *pshaderPtr, const char* pshaderText)
    {
        if (!*pshaderPtr)
        {
            GPtr<ID3D10Blob> pshader;
            GPtr<ID3D10Blob> pmsg;
            HRESULT hr;

            hr = D3D10CompileShader(pshaderText, strlen(pshaderText), NULL,
                NULL, NULL, "main", GRENDERER_PSHADER_PROFILE, D3D10_SHADER_DEBUG, &pshader.GetRawRef(), &pmsg.GetRawRef());

            if (FAILED(hr))
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3D10 - PixelShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }
            if (!pshader ||
                ((hr = pDevice->CreatePixelShader(pshader->GetBufferPointer(), pshader->GetBufferSize(),
                &pshaderPtr->GetRawRef())) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3D10 - Can't create D3D10 pshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }


    // Initialize the shaders we use for SWF mesh rendering.
    bool    InitShaders()
    {
        bool success = 1;
        UInt i;

        for (i = 1; i < VS_Count; i++)
            if (!CreateVertexShader(&VertexShaders[i], &VertexDecls[VertexShaderDeclTable[i]], &VertexDeclTypeTable[VertexShaderDeclTable[i]-1],
                VertexShaderTextTable[i-1]))
            {
                success = 0;
                break;
            }

        for (i = 1; i < PS_Count; i++)
        {           
            if (PixelShaderInitTable[i-1])
            {
                if (!CreatePixelShader(&PixelShaders[i], PixelShaderInitTable[i-1]))
                {
                    success = 0;
                    break;
                }
            }
        }

        // constant buffers
        D3D10_BUFFER_DESC bd;
        memset(&bd, 0, sizeof(D3D10_BUFFER_DESC));
        bd.Usage = D3D10_USAGE_DEFAULT;
        bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

        bd.ByteWidth = sizeof(VShaderConstData);
        success &= (pDevice->CreateBuffer(&bd, NULL, &VShaderConst.GetRawRef()) == S_OK);

        bd.ByteWidth = 32;
        success &= (pDevice->CreateBuffer(&bd, NULL, &PShaderConst.GetRawRef()) == S_OK);

        // blending
        for (i = 1; i < BlendDesc_Count; i++)
            success &= (pDevice->CreateBlendState(&BlendDescs[i], &BlendStates[i].GetRawRef()) == S_OK);

        D3D10_DEPTH_STENCIL_DESC ds;
        memset(&ds, 0, sizeof(D3D10_DEPTH_STENCIL_DESC));
        ds.DepthFunc = D3D10_COMPARISON_ALWAYS;
        ds.FrontFace.StencilDepthFailOp = ds.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
        ds.FrontFace.StencilFailOp = ds.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
        ds.FrontFace.StencilPassOp = ds.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
        ds.FrontFace.StencilFunc = ds.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
        success &= (pDevice->CreateDepthStencilState(&ds, &StencilDisabled.GetRawRef()) == S_OK);

        ds.StencilEnable = 1;
        ds.StencilReadMask = 0xff;
        ds.StencilWriteMask = 0xff;
        ds.FrontFace.StencilPassOp = ds.BackFace.StencilPassOp = D3D10_STENCIL_OP_REPLACE;
        ds.FrontFace.StencilFunc = ds.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
        success &= (pDevice->CreateDepthStencilState(&ds, &StencilSet.GetRawRef()) == S_OK);

        ds.StencilReadMask = 0xff;
        ds.StencilWriteMask = 0xff;
        ds.FrontFace.StencilPassOp = ds.BackFace.StencilPassOp = D3D10_STENCIL_OP_INCR;
        ds.FrontFace.StencilFunc = ds.BackFace.StencilFunc = D3D10_COMPARISON_EQUAL;
        success &= (pDevice->CreateDepthStencilState(&ds, &StencilIncr.GetRawRef()) == S_OK);

        ds.FrontFace.StencilPassOp = ds.BackFace.StencilPassOp = D3D10_STENCIL_OP_DECR;
        ds.FrontFace.StencilFunc = ds.BackFace.StencilFunc = D3D10_COMPARISON_EQUAL;
        success &= (pDevice->CreateDepthStencilState(&ds, &StencilDecr.GetRawRef()) == S_OK);

        ds.StencilWriteMask = 0;
        ds.FrontFace.StencilPassOp = ds.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
        ds.FrontFace.StencilFunc = ds.BackFace.StencilFunc = D3D10_COMPARISON_EQUAL;
        success &= (pDevice->CreateDepthStencilState(&ds, &StencilTest.GetRawRef()) == S_OK);

        // samplers
        D3D10_SAMPLER_DESC sd;
        memset(&sd, 0, sizeof(D3D10_SAMPLER_DESC));
        sd.MipLODBias = -0.5;
        sd.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
        sd.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
        sd.MinLOD = 0;
        sd.MaxLOD = 0;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
        success &= (pDevice->CreateSamplerState(&sd, &SamplerStates[0].GetRawRef()) == S_OK);
        sd.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
        success &= (pDevice->CreateSamplerState(&sd, &SamplerStates[1].GetRawRef()) == S_OK);

        sd.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
        success &= (pDevice->CreateSamplerState(&sd, &SamplerStates[2].GetRawRef()) == S_OK);
        sd.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = sd.AddressV = sd.AddressW = D3D10_TEXTURE_ADDRESS_CLAMP;
        success &= (pDevice->CreateSamplerState(&sd, &SamplerStates[3].GetRawRef()) == S_OK);

        if (!success)
        {
            ReleaseShaders();
            return 0;
        }

        return 1;
    }


    void    ReleaseShaders()
    {
        UInt i;

        for (i=0; i< VD_Count; i++)
                VertexDecls[i] = 0;

        for (i=0; i< VS_Count; i++)
                VertexShaders[i] = 0;

        for (i=0; i< PS_Count; i++)
                PixelShaders[i] = 0;

        for (i=0; i< BlendDesc_Count; i++)
            BlendStates[i] = 0;

        for (i=0; i< 4; i++)
            SamplerStates[i] = 0;

        VShaderConst = 0;
        PShaderConst = 0;
        RasterStates[0] = 0;
        RasterStates[1] = 0;
        StencilDisabled = StencilSet = StencilIncr = StencilDecr = StencilTest = 0;
    }

    // Sets a shader to specified type.
    void    SetVertexDecl(VDeclType vd)
    {
        if (VDeclIndex != vd)
        {
            VDeclIndex = vd;
            pDevice->IASetInputLayout(VertexDecls[vd]);
        }
    }
    void    SetVertexShader(VShaderType vt)
    {
        if (VShaderIndex != vt)
        {
            GASSERT(VertexShaderDeclTable[vt] == VDeclIndex);
            VShaderIndex = vt;
            pDevice->VSSetShader(VertexShaders[vt]);
        }
    }


    // Set shader to a device based on a constant
    void    SetPixelShader(PixelShaderType shaderType)
    {
        if (shaderType != PShaderIndex)
        {
            pDevice->PSSetShader(PixelShaders[shaderType]);
            PShaderIndex = shaderType;
        }
    }



    // Utility.  Mutates *width, *height and *data to create the
    // next mip level.
    static void MakeNextMiplevel(int* width, int* height, UByte* data)
    {
        GASSERT(width);
        GASSERT(height);
        GASSERT(data);
        GASSERT_ON_RENDERER_MIPMAP_GEN;

        int newW = *width >> 1;
        int newH = *height >> 1;
        if (newW < 1) newW = 1;
        if (newH < 1) newH = 1;
        
        if (newW * 2 != *width   || newH * 2 != *height)
        {
            // Image can't be shrunk Along (at least) one
            // of its dimensions, so don't bother
            // resampling.  Technically we should, but
            // it's pretty useless at this point.  Just
            // change the image dimensions and leave the
            // existing pixels.
        }
        else
        {
            // Resample.  Simple average 2x2 --> 1, in-place.
            for (int j = 0; j < newH; j++) 
            {
                UByte*  out = ((UByte*) data) + j * newW;
                UByte*  in = ((UByte*) data) + (j << 1) * *width;
                for (int i = 0; i < newW; i++) 
                {
                    int a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
                    *(out) = UByte(a >> 2);
                    out++;
                    in += 2;
                }
            }
        }

        // Munge parameters to reflect the shrunken image.
        *width = newW;
        *height = newH;
    }


    


    // Fill helper function:
    // Applies fill texture by setting it to the specified stage, initializing samplers and vertex constants
    void    ApplyFillTexture(const FillTexture &fill, UInt stageIndex)
    {
        GASSERT (fill.pTexture != 0);
        if (fill.pTexture == 0) return; // avoid crash in release build

        GTextureD3D10Impl* ptexture = ((GTextureD3D10Impl*)fill.pTexture);

        ApplySampleMode(stageIndex, fill.SampleMode, fill.WrapMode);
        pDevice->PSSetShaderResources(stageIndex, 1, &ptexture->pShaderView);

        // Set up the bitmap matrix for texgen.
        float   InvWidth = 1.0f / ptexture->Width;
        float   InvHeight = 1.0f / ptexture->Height;

        const Matrix&   m = fill.TextureMatrix;
        VShaderData.texgen[stageIndex*8+0] = m.M_[0][0] * InvWidth;
        VShaderData.texgen[stageIndex*8+1] = m.M_[0][1] * InvWidth;
        VShaderData.texgen[stageIndex*8+3] = m.M_[0][2] * InvWidth;

        VShaderData.texgen[stageIndex*8+4] = m.M_[1][0] * InvHeight;
        VShaderData.texgen[stageIndex*8+5] = m.M_[1][1] * InvHeight;
        VShaderData.texgen[stageIndex*8+7] = m.M_[1][2] * InvHeight;

        // actual data upload occurs in SetShaderConstants
    }


    void    ApplyPShaderCxform(const Cxform &cxform) const
    {
        const float mult = 1.0f / 255.0f;

        float   cxformData[4 * 2] =
        { 
            cxform.M_[0][0], cxform.M_[1][0],
            cxform.M_[2][0], cxform.M_[3][0],
            // Cxform color is already pre-multiplied
            cxform.M_[0][1] * mult, cxform.M_[1][1] * mult,
            cxform.M_[2][1] * mult, cxform.M_[3][1] * mult
        };

        pDevice->UpdateSubresource(PShaderConst, 0, NULL, cxformData, 0, 0);
    }

    void    SetShaderConstants()
    {
        pDevice->UpdateSubresource(VShaderConst, 0, NULL, &VShaderData, 0, 0);
    }


    class GFxFillStyle
    {
    public:
        enum FillMode
        {
            FM_None,
            FM_Color,
            FM_Bitmap,
            FM_Gouraud
        };

        FillMode                Mode;
    
        GouraudFillType         GouraudType;
        GColor                  Color;
        
        Cxform                  BitmapColorTransform;       
        bool                    HasNonzeroAdditiveCxform;

        FillTexture             Fill;
        FillTexture             Fill2;

        
        GFxFillStyle()
        {
            Mode            = FM_None;
            GouraudType     = GFill_Color;
            Fill.pTexture   = 0;
            Fill2.pTexture  = 0;
            HasNonzeroAdditiveCxform = 0;
        }

        
        // Push our style into Direct3D
        void    Apply(GRendererD3D10Impl *prenderer) const
        {
            GASSERT(Mode != FM_None);


            // Complex
            prenderer->ApplyBlendMode(((Color.GetAlpha()== 0xFF) &&
                (prenderer->BlendMode <= Blend_Normal) &&
                (Mode != FM_Gouraud))  ? Blend_None : prenderer->BlendMode);
            
            // Non-Edge AA Rendering.

            if (Mode == FM_Color)
            {
                // Solid color fills.
                prenderer->SetPixelShader(PS_SolidColor);
                prenderer->ApplyColor(Color);
            }

            else if (Mode == FM_Bitmap)
            {
                // Gradiend and texture fills.
                GASSERT(Fill.pTexture != NULL);

                prenderer->ApplyColor(Color);
                
                if (Fill.pTexture == NULL)
                {
                    // Just in case, for release build.
                    prenderer->SetPixelShader(PS_None);
                }
                else
                {
                    if ((prenderer->BlendMode == Blend_Multiply) ||
                        (prenderer->BlendMode == Blend_Darken) )
                        prenderer->SetPixelShader(PS_CxformTextureMultiply);
                    else
                        prenderer->SetPixelShader(PS_CxformTexture);

                    prenderer->ApplyPShaderCxform(BitmapColorTransform);
                    prenderer->ApplyFillTexture(Fill, 0);
                }
            }
            

            // Edge AA - relies on Gouraud shading and texture mixing
            else if (Mode == FM_Gouraud)
            {

                PixelShaderType shader = PS_None;
                
                // No texture: generate color-shaded triangles.
                if (Fill.pTexture == NULL)
                {
                    if (prenderer->VertexStream.GetVertexFormat() == Vertex_XY16iC32)
                    {
                        // Cxform Alpha Add can not be non-zero is this state because 
                        // cxform blend equations can not work correctly.
                        // If we hit this assert, it means Vertex_XY16iCF32 should have been used.
                        GASSERT (BitmapColorTransform.M_[3][1] < 1.0f);

                        shader = PS_CxformGauraudNoAddAlpha;                        
                    }
                    else
                    {
                        shader = PS_CxformGauraud;                      
                    }
                }
                
                // We have a textured or multi-textured gouraud case.
                else
                {   
                    prenderer->ApplyFillTexture(Fill, 0);

                    if ((GouraudType == GFill_1TextureColor) ||
                        (GouraudType == GFill_1Texture))
                    {
                        shader = PS_CxformGauraudTexture;
                    }
                    else
                    {
                        shader = PS_Cxform2Texture;

                        prenderer->ApplyFillTexture(Fill2, 1);
                    }
                }

                if ((prenderer->BlendMode == Blend_Multiply) ||
                    (prenderer->BlendMode == Blend_Darken) )
                { 
                    shader = (PixelShaderType)(shader + (PS_CxformGauraudMultiply - PS_CxformGauraud));

                    // For indexing to work, these should hold:
                    GCOMPILER_ASSERT( (PS_Cxform2Texture - PS_CxformGauraud) ==
                                      (PS_CxformMultiply2Texture - PS_CxformGauraudMultiply));
                    GCOMPILER_ASSERT( (PS_CxformGauraudMultiply - PS_CxformGauraud) ==
                                      (PS_Cxform2Texture - PS_CxformGauraud + 1) );
                }
                prenderer->SetPixelShader(shader);
                prenderer->ApplyPShaderCxform(BitmapColorTransform);
            }
        }


        void    Disable()
        {
            Mode          = FM_None;
            Fill.pTexture = 0;
        }
        void    SetColor(GColor color)
        {
            Mode = FM_Color; Color = color;
        }

        void    SetCxform(const Cxform& colorTransform)
        {
            BitmapColorTransform = colorTransform;

            if ( BitmapColorTransform.M_[0][1] > 1.0f ||
                 BitmapColorTransform.M_[1][1] > 1.0f ||
                 BitmapColorTransform.M_[2][1] > 1.0f ||
                 BitmapColorTransform.M_[3][1] > 1.0f )         
                HasNonzeroAdditiveCxform = true;            
            else            
                HasNonzeroAdditiveCxform = false;
        }

        void    SetBitmap(const FillTexture* pft, const Cxform& colorTransform)
        {
            Mode            = FM_Bitmap;
            Fill            = *pft;
            Color           = GColor(0xFFFFFFFF);
            
            SetCxform(colorTransform);
        }

        
        // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
        // The specified textures are applied to vertices {0, 1, 2} of each triangle based
        // on factors of Complex vertex. Any or all subsequent pointers can be NULL, in which case
        // texture is not applied and vertex colors used instead.
        void    SetGouraudFill(GouraudFillType gfill,
                                 const FillTexture *ptexture0,
                                 const FillTexture *ptexture1,
                                 const FillTexture *ptexture2, const Cxform& colorTransform)
        {

            // Texture2 is not yet used.
            if (ptexture0 || ptexture1 || ptexture2)
            {
                const FillTexture *p = ptexture0;
                if (!p) p = ptexture1;              

                SetBitmap(p, colorTransform);
                // Used in 2 texture mode
                if (ptexture1)
                    Fill2 = *ptexture1;             
            }
            else
            {
                SetCxform(colorTransform);              
                Fill.pTexture = 0;  
            }
            
            Mode        = GFxFillStyle::FM_Gouraud;
            GouraudType = gfill;
        }


        bool    IsValid() const
        {
            return Mode != FM_None;
        }

    }; // class GFxFillStyle


    
    // Style state.
    enum StyleIndex
    {
        FILL_STYLE = 0,     
        LINE_STYLE,
        STYLE_COUNT
    };
    GFxFillStyle    CurrentStyles[STYLE_COUNT];


    // Given an image, returns a Pointer to a GTexture struct
    // that can later be passed to FillStyleX_bitmap(), to set a
    // bitmap fill style.
    GTextureD3D10*   CreateTexture()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GTextureD3D10Impl(this);
    }

    
    // Helper function to query renderer capabilities.
    bool        GetRenderCaps(RenderCaps *pcaps)
    {
        if (!ModeSet)
        {
            GFC_DEBUG_WARNING(1, "GRendererD3D10::GetRenderCaps fails - video mode not set");
            return 0;
        }

        pcaps->CapBits      = Cap_Index16 | Cap_FillGouraud | Cap_CxformAdd | Cap_FillGouraudTex | Cap_NestedMasks;
        pcaps->BlendModes   = (1<<Blend_None) | (1<<Blend_Normal) |
                              (1<<Blend_Multiply) | (1<<Blend_Lighten) | (1<<Blend_Darken) |
                              (1<<Blend_Add) | (1<<Blend_Subtract);
        pcaps->VertexFormats= (1<<Vertex_None) | (1<<Vertex_XY16i) | (1<<Vertex_XY16iC32) | (1<<Vertex_XY16iCF32);

        pcaps->MaxTextureSize = 2048;        
        return 1;
    }


    

    // Set up to render a full frame from a movie and fills the
    // background.  Sets up necessary transforms, to scale the
    // movie to fit within the given dimensions.  Call
    // EndDisplay() when you're done.
    //
    // The Rectangle (viewport.Left, viewport.Top, viewport.Left +
    // viewportWidth, viewport.Top + viewportHeight) defines the
    // window coordinates taken up by the movie.
    //
    // The Rectangle (x0, y0, x1, y1) defines the pixel
    // coordinates of the movie that correspond to the viewport
    // bounds.
    void    BeginDisplay(
        GColor backgroundColor, const GViewport &vpin,
        Float x0, Float x1, Float y0, Float y1)

    {
        if (!ModeSet)
        {
            GFC_DEBUG_WARNING(1, "GRendererD3D10::BeginDisplay failed - video mode not set");
            return;
        }

        RenderMode = (vpin.Flags & GViewport::View_AlphaComposite);

        DisplayWidth = fabsf(x1 - x0);
        DisplayHeight = fabsf(y1 - y0);

        pDevice->OMGetRenderTargets(1, &pRenderTarget.GetRawRef(), &pDepthStencil.GetRawRef());

        // clamp viewport to buffer
        GViewport surfaceDesc(vpin);
        surfaceDesc.Width = vpin.BufferWidth;
        surfaceDesc.Height= vpin.BufferHeight;

        // Matrix to map from SWF Movie (TWIPs) coords to
        // viewport coordinates.
        float   dx = x1 - x0;
        float   dy = y1 - y0;
        if (dx < 1) { dx = 1; }
        if (dy < 1) { dy = 1; }
        

        float clipX0 = x0;
        float clipX1 = x1;
        float clipY0 = y0;
        float clipY1 = y1;

        // In some cases destination source coordinates can be outside D3D viewport.
        // D3D will not allow that; however, it is useful to support.
        // So if this happens, clamp the viewport and re-calculate source values.
        GViewport viewport(vpin);
        int viewportX0_save = viewport.Left;
        int viewportY0_save = viewport.Top;
        int viewportW_save = viewport.Width;
        int viewportH_save = viewport.Height;

        if (viewport.Left < 0)
        {
            clipX0 = x0 + ((-viewport.Left) * dx) / (float) viewport.Width;
            if ((-viewport.Left) >= viewport.Width)
                viewport.Width = 0;
            else
                viewport.Width += viewport.Left;
            viewport.Left = 0;
        }
        if (viewportX0_save + viewportW_save > (int) surfaceDesc.Width)
        {
            clipX1 = x0 + ((surfaceDesc.Width - viewportX0_save) * dx) / (float) viewportW_save;
            viewport.Width = surfaceDesc.Width - viewport.Left;
        }

        if (viewport.Top < 0)
        {
            clipY0 = y0 + ((-viewport.Top) * dy) / (float) viewport.Height;
            if ((-viewport.Top) >= viewport.Height)
                viewport.Height = 0;
            else
                viewport.Height += viewport.Top;
            viewport.Top = 0;
        }
        if (viewportY0_save + viewportH_save > (int) surfaceDesc.Height)
        {
            clipY1 = y0 + ((surfaceDesc.Height - viewportY0_save) * dy) / (float) viewportH_save;
            viewport.Height = surfaceDesc.Height - viewport.Top;
        }
        dx  = clipX1 - clipX0;
        dy  = clipY1 - clipY0;


        // Viewport.
        D3D10_VIEWPORT    vp;

        vp.MinDepth     = 0.0f;
        vp.MaxDepth     = 0.0f;
    
        if (viewport.Flags & GViewport::View_UseScissorRect)
        {
            int scissorX0_save = viewport.ScissorLeft;
            int scissorY0_save = viewport.ScissorTop;

            if (viewport.ScissorLeft < viewport.Left) viewport.ScissorLeft = viewport.Left;
            if (viewport.ScissorTop < viewport.Top) viewport.ScissorTop = viewport.Top;
            viewport.ScissorWidth -= (viewport.ScissorLeft - scissorX0_save);
            viewport.ScissorHeight -= (viewport.ScissorTop - scissorY0_save);
            if (viewport.ScissorLeft + viewport.ScissorWidth > viewport.Left + viewport.Width) 
                viewport.ScissorWidth = viewport.Left + viewport.Width - viewport.ScissorLeft;
            if (viewport.ScissorTop + viewport.ScissorHeight > viewport.Top + viewport.Height) 
                viewport.ScissorHeight = viewport.Top + viewport.Height - viewport.ScissorTop;

            vp.TopLeftX = viewport.ScissorLeft;
            vp.TopLeftY = viewport.ScissorTop;
            vp.Width    = viewport.ScissorWidth;
            vp.Height   = viewport.ScissorHeight;

            clipX0 = clipX0 + dx / viewport.Width * (viewport.ScissorLeft - viewport.Left);
            clipY0 = clipY0 + dy / viewport.Height * (viewport.ScissorTop - viewport.Top);
            dx = dx / viewport.Width * viewport.ScissorWidth;
            dy = dy / viewport.Height * viewport.ScissorHeight;
        }
        else
        {
            vp.TopLeftX = viewport.Left;
            vp.TopLeftY = viewport.Top;
            vp.Width    = viewport.Width;
            vp.Height   = viewport.Height;
        }
        pDevice->RSSetViewports(1, &vp);

        ViewportMatrix.SetIdentity();
        ViewportMatrix.M_[0][0] = 2.0f  / dx;
        ViewportMatrix.M_[1][1] = -2.0f / dy;

        // MA: it does not seem necessary to subtract viewport.
        // Need to work through details of viewport cutting, there still seems to
        // be a 1-pixel issue at edges. Could that be due to edge fill rules ?
        ViewportMatrix.M_[0][2] = -1.0f - ViewportMatrix.M_[0][0] * (clipX0); 
        ViewportMatrix.M_[1][2] = 1.0f  - ViewportMatrix.M_[1][1] * (clipY0);

        ViewportMatrix *= UserMatrix;

        // Blending render states.
        BlendModeStack.clear();
        BlendModeStack.reserve(16);
        ApplyBlendMode(BlendMode);      

        // Must reset both stages since ApplySampleMode modifies both.
        SampleMode[0]       = Sample_Linear;      
        SampleMode[1]       = Sample_Linear;      

        pDevice->RSSetState(RasterStates[VMCFlags & VMConfig_Multisample ? 1 : 0]);
        pDevice->OMSetDepthStencilState(StencilDisabled, 0);

        pDevice->VSSetConstantBuffers(0, 1, &VShaderConst.GetRawRef());
        pDevice->PSSetConstantBuffers(0, 1, &PShaderConst.GetRawRef());

        // Vertex format.
        // Set None so that shader is forced to be set and then set to default: strip.
        VDeclIndex   = VD_None;
        VShaderIndex = VS_None;
        PShaderIndex = PS_None;
        SetVertexDecl(VD_Strip);
        SetVertexShader(VS_Strip);

        // Start with solid shader.
        SetPixelShader(PS_SolidColor);

        // Configure vertex/index buffers.
        VertexStream.BeginDisplay();        

        StencilCounter = 0;
        DrawingMask = 0;

        // Clear the background, if background color has alpha > 0.
        if (backgroundColor.GetAlpha() > 0)
        {
            // Draw a big quad.
            ApplyColor(backgroundColor);
            SetMatrix(Matrix::Identity);
            ApplyMatrix(CurrentMatrix);
            SetShaderConstants();

            // The and x/y matrix are scaled by 20 twips already, so it
            // should ok to just convert these values into integers.
            struct Vertex
            {
                SInt16 x, y;
            };
            
            Vertex strip[4] =
            {
                { (SInt16)x0, (SInt16)y0 },
                { (SInt16)x1, (SInt16)y0 },
                { (SInt16)x0, (SInt16)y1 },
                { (SInt16)x1, (SInt16)y1 }
            };

            // Copy vertices to dynamic buffer and draw.
            void *pbuffer = VertexStream.LockVertexBuffer(4, sizeof(Vertex));
            if (pbuffer)
            {
                memcpy(pbuffer, &strip[0], 4 * sizeof(Vertex));
                VertexStream.UnlockVertexBuffer();
                pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                pDevice->Draw(4, VertexStream.GetStartIndex());
            } 
        }
    }


    // Clean up after rendering a frame.  Client program is still
    // responsible for calling Present() or glSwapBuffers()
    void    EndDisplay()
    {
        if (pDevice)
        {
            GFC_DEBUG_WARNING(StencilCounter > 0, "GRenderer::EndDisplay - BeginSubmitMask/DisableSubmitMask mismatch");
        }

        pRenderTarget = 0;
        pDepthStencil = 0;
        ReleaseQueuedResources();
    }


    // Set the current transform for mesh & line-strip rendering.
    void    SetMatrix(const Matrix& m)
    {
        CurrentMatrix = m;
    }

    void    SetUserMatrix(const Matrix& m)
    {
        UserMatrix = m;
    }


    // Set the current color transform for mesh & line-strip rendering.
    void    SetCxform(const Cxform& cx)
    {
        CurrentCxform = cx;
    }
    

    void    ApplyBlendMode(BlendType mode)
    {
        if (!pDevice)
            return;

        if (DrawingMask)
            return;

        // For debug build
        GASSERT(((UInt) mode) < 15);
        // For release
        if (((UInt) mode) >= 15)
            mode = Blend_None;

        const float one[4] = {1,1,1,1};
        pDevice->OMSetBlendState(BlendStates[BlendModeStates[mode]], one, 0xffffffff);
    }


    void ApplySampleMode(UInt stage, BitmapSampleMode mode, BitmapWrapMode wrap)
    {
        pDevice->PSSetSamplers(stage, 1, &SamplerStates[mode+wrap*2].GetRawRef());
    }

            



    // Pushes a Blend mode onto renderer.
    virtual void    PushBlendMode(BlendType mode)
    {
        // Blend modes need to be applied cumulatively, ideally through an extra RT texture.
        // If the nested clip has a "Multiply" effect, and its parent has an "Add", the result
        // of Multiply should be written to a buffer, and then used in Add.

        // For now we only simulate it on one level -> we apply the last effect on top of stack.
        // (Note that the current "top" element is BlendMode, it is not actually stored on stack).
        // Although incorrect, this will at least ensure that parent's specified effect
        // will be applied to children.

        BlendModeStack.push_back(BlendMode);

        if ((mode > Blend_Layer) && (BlendMode != mode))
        {
            BlendMode = mode;
            ApplyBlendMode(BlendMode);
        }
    }

    // Pops a blend mode, restoring it to the previous one. 
    virtual void    PopBlendMode()
    {
        if (BlendModeStack.size() != 0)
        {                       
            // Find the next top interesting mode.
            BlendType   newBlendMode = Blend_None;
            
            for (SInt i = (SInt)BlendModeStack.size()-1; i>=0; i--)         
                if (BlendModeStack[i] > Blend_Layer)
                {
                    newBlendMode = BlendModeStack[i];
                    break;
                }

            BlendModeStack.pop_back();

            if (newBlendMode != BlendMode)
            {
                BlendMode = newBlendMode;
                ApplyBlendMode(BlendMode);
            }

            return;
        }
        
        // Stack was empty..
        GFC_DEBUG_WARNING(1, "GRendererD3D10::PopBlendMode - blend mode stack is empty");
    }
    

    // multiply current GRenderer::Matrix with d3d GRenderer::Matrix
    void    ApplyMatrix(const Matrix& matIn)
    {
        Matrix  m(ViewportMatrix);
        m *= matIn;

        VShaderData.mvp[0][0] = m.M_[0][0];
        VShaderData.mvp[0][1] = m.M_[0][1];
        VShaderData.mvp[0][2] = 0;
        VShaderData.mvp[0][3] = m.M_[0][2];

        VShaderData.mvp[1][0] = m.M_[1][0];
        VShaderData.mvp[1][1] = m.M_[1][1];
        VShaderData.mvp[1][2] = 0;
        VShaderData.mvp[1][3] = m.M_[1][2];

        VShaderData.mvp[2][0] = 0;
        VShaderData.mvp[2][1] = 0;
        VShaderData.mvp[2][2] = 1.0f;
        VShaderData.mvp[2][3] = 0;

        VShaderData.mvp[3][0] = 0;
        VShaderData.mvp[3][1] = 0;
        VShaderData.mvp[3][2] = 0;
        VShaderData.mvp[3][3] = 1.0f;
    }

    
    // Set the given color. 
    void    ApplyColor(GColor c)
    {
        const float mult = 1.0f / 255.0f;
        const float alpha = c.GetAlpha() * mult;
        // Set color.
        float rgba[8] = 
        {
            c.GetRed() * mult,  c.GetGreen() * mult,
            c.GetBlue() * mult, alpha,
            0,0,0,0
        };      

        if ((BlendMode == Blend_Multiply) ||
            (BlendMode == Blend_Darken))
        {
            rgba[0] = gflerp(1.0f, rgba[0], alpha);
            rgba[1] = gflerp(1.0f, rgba[1], alpha);
            rgba[2] = gflerp(1.0f, rgba[2], alpha);
        }

        pDevice->UpdateSubresource(PShaderConst, 0, NULL, rgba, 0, 0);
    }

    
    // Don't fill on the {0 == left, 1 == right} side of a path.
    void    FillStyleDisable()
    {
        CurrentStyles[FILL_STYLE].Disable();
    }

    // Don't draw a line on this path.
    void    LineStyleDisable()
    {
        CurrentStyles[LINE_STYLE].Disable();
    }


    // Set fill style for the left interior of the shape.  If
    // enable is false, turn off fill for the left interior.
    void    FillStyleColor(GColor color)
    {
        CurrentStyles[FILL_STYLE].SetColor(CurrentCxform.Transform(color));
    }

    // Set the line style of the shape.  If enable is false, turn
    // off lines for following curve segments.
    void    LineStyleColor(GColor color)
    {
        CurrentStyles[LINE_STYLE].SetColor(CurrentCxform.Transform(color));
    }

    void    FillStyleBitmap(const FillTexture *pfill)
    {       
        CurrentStyles[FILL_STYLE].SetBitmap(pfill, CurrentCxform);
    }
    
    // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
    void    FillStyleGouraud(GouraudFillType gfill,
                             const FillTexture *ptexture0,
                             const FillTexture *ptexture1,
                             const FillTexture *ptexture2)
    {
        CurrentStyles[FILL_STYLE].SetGouraudFill(gfill, ptexture0, ptexture1, ptexture2, CurrentCxform);
    }


    void    SetVertexData(const void* pvertices, int numVertices, VertexFormat vf, CacheProvider *pcache)
    {
        // Dynamic buffer stream update.
        VertexStream.SetVertexData(pvertices, numVertices, vf);

        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Vertex, (pvertices!=0),
                                    numVertices, (((pvertices!=0)&&numVertices) ? *((SInt16*)pvertices) : 0) );
    }

    void    SetIndexData(const void* pindices, int numIndices, IndexFormat idxf, CacheProvider *pcache)
    {
        VertexStream.SetIndexData(pindices, numIndices, idxf);     

        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Index, (pindices!=0),
                                    numIndices, (((pindices!=0)&&numIndices) ? *((UInt16*)pindices) : 0) );
    }
    
    void    ReleaseCachedData(CachedData *pdata, CachedDataType type)
    {
        // Releases cached data that was allocated from the cache providers.
        CacheList.ReleaseCachedData(pdata, type);
    }



    void    DrawIndexedTriList(int baseVertexIndex,
                               int minVertexIndex, int numVertices,
                               int startIndex, int triangleCount)
    {       
        if (!ModeSet) 
            return;
        // Must have vertex data.
        if (!VertexStream.HasVertexData() || !VertexStream.HasIndexData() ||
            (VertexStream.GetD3DIndexFormat() == DXGI_FORMAT_UNKNOWN))
        {
            GFC_DEBUG_WARNING(!VertexStream.HasVertexData(), "GRendererD3D10::DrawIndexedTriList failed, vertex data not specified");
            GFC_DEBUG_WARNING(!VertexStream.HasIndexData(), "GRendererD3D10::DrawIndexedTriList failed, index data not specified");
            GFC_DEBUG_WARNING((VertexStream.GetD3DIndexFormat() == DXGI_FORMAT_UNKNOWN),
                              "GRendererD3D10::DrawIndexedTriList failed, index buffer format not specified");
            return;
        }
        
        bool        isGouraud = (CurrentStyles[FILL_STYLE].Mode == GFxFillStyle::FM_Gouraud);        
        VertexFormat vfmt     = VertexStream.GetVertexFormat();

        // Ensure we have the right vertex size. 
        // MA: Should we loosen some rules to allow for other decl/shader combinations?

        if (isGouraud)
        {           
            if (vfmt == Vertex_XY16iC32)
            {
                SetVertexDecl(VD_XY16iC32);
                SetVertexShader(VS_XY16iC32);                
            }
            else if (vfmt == Vertex_XY16iCF32)
            {
                SetVertexDecl(VD_XY16iCF32);

                if (CurrentStyles[FILL_STYLE].GouraudType != GFill_2Texture)
                    SetVertexShader(VS_XY16iCF32);
                else
                    SetVertexShader(VS_XY16iCF32_T2);
            }
        }
        else
        {
            GASSERT(vfmt == Vertex_XY16i);
            SetVertexDecl(VD_Strip);
            SetVertexShader(VS_Strip);
        }

        // Set up current style.
        CurrentStyles[FILL_STYLE].Apply(this);
        ApplyMatrix(CurrentMatrix);
        SetShaderConstants();

        // Upload data to buffers and draw.
        GDynamicVertexStream::PrimitiveDesc prim(baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount);
        if (!VertexStream.PrepareVertexData(prim))
            return;

        VertexStream.DrawTriangles();
        RenderStats.Triangles += triangleCount;
        RenderStats.Primitives++;
    }


    // Draw the line strip formed by the sequence of points.
    void    DrawLineStrip(int baseVertexIndex, int lineCount)
    {
        if (!ModeSet)
            return;
        if (!VertexStream.HasVertexData()) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(1, "GRendererD3D10::DrawLineStrip failed, vertex data not specified");
            return;
        }

        // MA: Should loosen some rules to allow for other decl/shader combinations?
        GASSERT(VertexStream.GetVertexFormat() == Vertex_XY16i);
        SetVertexDecl(VD_Strip);
        SetVertexShader(VS_Strip);

        // Set up current style.
        CurrentStyles[LINE_STYLE].Apply(this);

        ApplyMatrix(CurrentMatrix);
        SetShaderConstants();

        if (!VertexStream.InitVertexBufferData(baseVertexIndex, lineCount + 1))
            return;
        pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_LINESTRIP);
        pDevice->Draw(lineCount + 1, VertexStream.GetStartIndex());

        RenderStats.Lines += lineCount;
        RenderStats.Primitives++;
    }

    

    // Draw a set of rectangles textured with the given bitmap, with the given color.
    // Apply given transform; ignore any currently set transforms.  
    //
    // Intended for textured glyph rendering.
    void    DrawBitmaps(BitmapDesc* pbitmapList, int listSize, int startIndex, int count,
                        const GTexture* pti, const Matrix& m,
                        CacheProvider *pcache)
    {
        if (!ModeSet || !pbitmapList || !pti)
            return;

        GTextureD3D10Impl* ptexture = (GTextureD3D10Impl*)pti;
        // Texture must be valid for rendering
        if (!ptexture->pD3DTexture && !ptexture->CallRecreate())
        {
            GFC_DEBUG_WARNING(1, "GRendererD3D10::DrawBitmaps failed, empty texture specified/could not recreate texture");
            return;     
        }

        // Test cache buffer management support.
        // Optimized implementation could use this spot to set vertex buffer and/or initialize offset in it.
        // Note that since bitmap lists are usually short, it would be a good idea to combine many of them
        // into one buffer, instead of creating individual buffers for each pbitmapList data instance.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_BitmapList, (pbitmapList!=0),
                                    listSize, (((pbitmapList!=0)&&listSize) ? ((SInt)pbitmapList->Coords.Left) : 0) );

        ApplyPShaderCxform(CurrentCxform);

        // Complex
        ApplyBlendMode(BlendMode);
        ApplySampleMode(0, Sample_Linear, Wrap_Clamp);
        
        // Set texture.
        SetPixelShader(PS_TextTexture);
        pDevice->PSSetShaderResources(0, 1, &ptexture->pShaderView);

        // Switch to non-texgen shader.
        SetVertexDecl(VD_Glyph);
        SetVertexShader(VS_Glyph);

        ApplyMatrix(m);
        SetShaderConstants();

        int ibitmap = 0, ivertex = 0;
        GCOMPILER_ASSERT((GDynamicVertexStream::GlyphBufferVertexCount%6) == 0);
        
        while (ibitmap < count)
        {
            // Lock the vertex buffer for glyphs.            
            int    vertexCount = GTL::gmin<int>((count-ibitmap) * 6, GDynamicVertexStream::GlyphBufferVertexCount);
            void *  pbuffer     = VertexStream.LockVertexBuffer(vertexCount, sizeof(GGlyphVertex));
            if (!pbuffer)
                return;

            for(ivertex = 0; (ivertex < vertexCount) && (ibitmap<count); ibitmap++, ivertex+= 6)
            {
                BitmapDesc &  bd = pbitmapList[ibitmap + startIndex];
                GGlyphVertex* pv = ((GGlyphVertex*)pbuffer) + ivertex;                

                // Triangle 1.
                pv[0].SetVertex2D(bd.Coords.Left, bd.Coords.Top,    bd.TextureCoords.Left, bd.TextureCoords.Top, bd.Color);
                pv[1].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[2].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                // Triangle 2.
                pv[3].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                pv[4].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[5].SetVertex2D(bd.Coords.Right, bd.Coords.Bottom,bd.TextureCoords.Right, bd.TextureCoords.Bottom, bd.Color);
            }

            VertexStream.UnlockVertexBuffer();     

            // Draw the generated triangles.
            if (ivertex)
            {
                pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                pDevice->Draw(ivertex, VertexStream.GetStartIndex());
                RenderStats.Primitives++;
            }
        }
        RenderStats.Triangles += count * 2;
    }


    void BeginSubmitMask(SubmitMaskMode maskMode)
    {
        if (!ModeSet)
            return;

        DrawingMask = 1;
        // disable color writes
        const float one[4] = {1,1,1,1};
        pDevice->OMSetBlendState(BlendStates[BlendDesc_Stencil], one, 0xffffffff);

        switch(maskMode)
        {
        case Mask_Clear:
            pDevice->ClearDepthStencilView(pDepthStencil, D3D10_CLEAR_STENCIL, 0, 0);
            pDevice->OMSetDepthStencilState(StencilSet, 1);
            StencilCounter = 1;
            break;

        case Mask_Increment:
            pDevice->OMSetDepthStencilState(StencilIncr, StencilCounter);
            StencilCounter++;
            break;

        case Mask_Decrement:                
            pDevice->OMSetDepthStencilState(StencilDecr, StencilCounter);
            StencilCounter--;
            break;
        }
    }

    void EndSubmitMask()
    {
        if (!ModeSet)
            return;

        DrawingMask = 0;
        ApplyBlendMode(BlendMode);
        pDevice->OMSetDepthStencilState(StencilTest, StencilCounter);
    }

    void DisableMask()
    {
        if (!ModeSet)
            return;

        DrawingMask = 0;
        StencilCounter = 0;
        ApplyBlendMode(BlendMode);
        pDevice->OMSetDepthStencilState(StencilDisabled, 0);
    }


    
    virtual void    GetRenderStats(Stats *pstats, bool resetStats)
    {
        if (pstats)
            memcpy(pstats, &RenderStats, sizeof(Stats));
        if (resetStats)
            RenderStats.Clear();
    }


        
    // GRendererD3D10 interface implementation
    
    virtual bool    SetDependentVideoMode(
                                ID3D10Device* pd3dDevice,
                                UInt32 vmConfigFlags)
    {
        if (!pd3dDevice)     
            return 0;

        VMCFlags = vmConfigFlags;

        if (pDevice && pd3dDevice != pDevice)
        {
            GFC_DEBUG_ERROR(1, "GRendererD3D10 - different device passed to SetDependentVideoMode");
            return 0;
        }
        else if (!pDevice)
        {
            pDevice = pd3dDevice;
            pDevice->AddRef();

            if (!InitShaders() || !VertexStream.Initialize(pd3dDevice))
            {
                ReleaseShaders();
                pDevice->Release();
                pDevice = 0;
                return 0;
            }
        }

        UInt rasterstate = VMCFlags & VMConfig_Multisample ? 1 : 0;
        if (!RasterStates[rasterstate])
        {
            D3D10_RASTERIZER_DESC rs;
            memset(&rs, 0, sizeof(D3D10_RASTERIZER_DESC));
            rs.FillMode = D3D10_FILL_SOLID;
            rs.CullMode = D3D10_CULL_NONE;
            rs.ScissorEnable = 0;
            rs.MultisampleEnable = VMCFlags & VMConfig_Multisample ? 1 : 0;
            rs.AntialiasedLineEnable = 1;
            if (FAILED(pDevice->CreateRasterizerState(&rs, &RasterStates[rasterstate].GetRawRef())))
                return 0;
        }

        ModeSet = 1;
        return 1;
    }
                                        
    // Returns back to original mode (cleanup)
    virtual bool                ResetVideoMode()
    {
        if (!ModeSet)
            return 1;

        ModeSet = 0;
        return 1;
    }

    virtual DisplayStatus       CheckDisplayStatus() const
    {
        if (!ModeSet)
            return DisplayStatus_NoModeSet;
        
        return DisplayStatus_Ok;
    }

    
    // Direct3D Access
    // Return various Dirext3D related information

    virtual ID3D10Device*   GetDirect3DDevice() const
    {
        return pDevice;
    }

}; // class GRendererD3D10Impl




// ***** GTextureD3D10 implementation


GTextureD3D10Impl::GTextureD3D10Impl(GRendererD3D10Impl *prenderer)
    : GTextureD3D10(&prenderer->Textures)
{
    pRenderer   = prenderer;
    Width       = 
    Height      = 0;
    pD3DTexture = 0;    
    pShaderView = 0;

    memset (&Desc, 0, sizeof(Desc));
}

GTextureD3D10Impl::~GTextureD3D10Impl()
{
    if (pD3DTexture)
    {
        GASSERT(pRenderer);
        if (pRenderer)
            pRenderer->AddResourceForReleasing(pD3DTexture);
        else
            // this should never happen
            pD3DTexture->Release(); 
    }
    if (!pRenderer)
        return;
    GLock::Locker guard(&pRenderer->TexturesLock);
    if(pFirst)
        RemoveNode();
}

// Obtains the renderer that create TextureInfo 
GRenderer*  GTextureD3D10Impl::GetRenderer() const
    { return pRenderer; }
bool        GTextureD3D10Impl::IsDataValid() const
    { return (pD3DTexture != 0);    }


// Remove texture from renderer, notifies renderer destruction
void    GTextureD3D10Impl::RemoveFromRenderer()
{
    pRenderer = 0;
    if (AddRef_NotZero())
    {
        if (pD3DTexture)
        {
            pD3DTexture->Release();
            pD3DTexture = 0;
            Width       = 
            Height      = 0;
        }
        
        CallHandlers(ChangeHandler::Event_RendererReleased);
        // we don't need to take textureslock here because this method
        // is called from within lock
        if (pNext) // We may have been released by user
            RemoveNode();
        Release();
    } else {
        if (pNext) // We may have been released by user
            RemoveNode();
    }
}



// Creates a D3D texture of the specified dest dimensions, from a
// resampled version of the given src image.  Does a bilinear
// resampling to create the dest image.
// Source can be 4,3, or 1 bytes/pixel. Destination is either 1 or 4 bytes/pixel.
void    SoftwareResample(
                UByte* pDst, int dstWidth, int dstHeight, int dstPitch,
                UByte* pSrc, int srcWidth, int srcHeight, int srcPitch,
                int bytesPerPixel )
{
    switch(bytesPerPixel)
    {
    case 4: 
        GRenderer::ResizeImage(pDst, dstWidth, dstHeight, dstPitch,
                               pSrc, srcWidth, srcHeight, srcPitch,
                               GRenderer::ResizeRgbaToRgba);
        break;

    case 3:
        GRenderer::ResizeImage(pDst, dstWidth, dstHeight, dstPitch,
                               pSrc, srcWidth, srcHeight, srcPitch,
                               GRenderer::ResizeRgbToRgba);
        break;

    case 1:
        GRenderer::ResizeImage(pDst, dstWidth, dstHeight, dstPitch,
                               pSrc, srcWidth, srcHeight, srcPitch,
                               GRenderer::ResizeGray);
        break;
    }
}

bool GTextureD3D10Impl::InitTexture(ID3D10Texture2D *ptex, SInt width, SInt height)
{
    if (!pRenderer || !pRenderer->pDevice)
        return 0;

    if (pD3DTexture)
    {
        pD3DTexture->Release();
        pD3DTexture = 0;
    }
    if (pShaderView)
    {
        pShaderView->Release();
        pShaderView = 0;
    }
    if (ptex)
    {
        Width = width;
        Height = height;
        pD3DTexture = ptex;
        ptex->AddRef();

        HRESULT result = pRenderer->pDevice->CreateShaderResourceView(pD3DTexture, NULL, &pShaderView);
        if (result != S_OK)
        {
            GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create shader view");
            Width = Height = 0;
            pD3DTexture->Release();
            pD3DTexture = 0;
            return 0;
        }
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

// NOTE: This function destroys pim's data in the process of making mipmaps.
bool GTextureD3D10Impl::InitTexture(GImageBase* pim, int targetWidth, int targetHeight)
{   
    if (!pRenderer || !pRenderer->pDevice)
        return 0;

    // Delete old data
    if (pD3DTexture)
    {
        pD3DTexture->Release();
        pD3DTexture = 0;
        Width = Height = 0;
    }
    if (!pim || !pim->pData)
    {
        // Kill texture     
        CallHandlers(ChangeHandler::Event_DataChange);
        return 1;
    }

    // Determine format
    UInt    bytesPerPixel = 0;  

    if (pim->Format == GImage::Image_ARGB_8888)
    {
        bytesPerPixel   = 4;
        Desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else if (pim->Format == GImage::Image_RGB_888)
    {
        bytesPerPixel   = 3;
        Desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    else if (pim->Format == GImage::Image_A_8)
    {
        bytesPerPixel   = 1;
        Desc.Format     = DXGI_FORMAT_A8_UNORM;
    }
    else if (pim->Format == GImage::Image_DXT1)
    {
        Desc.Format   = DXGI_FORMAT_BC1_UNORM;
        bytesPerPixel = 1;
    }
    else if (pim->Format == GImage::Image_DXT3)
    {
        Desc.Format   = DXGI_FORMAT_BC3_UNORM;
        bytesPerPixel = 1;
    }
    else if (pim->Format == GImage::Image_DXT5)
    {
        Desc.Format   = DXGI_FORMAT_BC5_UNORM;
        bytesPerPixel = 1;
    }
    else 
    { // Unsupported format
        GASSERT(0);
        return 0;
    }

    Width   = (targetWidth == 0)  ? pim->Width : targetWidth;
    Height  = (targetHeight == 0) ? pim->Height: targetHeight;

    UInt    wlevels = 1, hlevels = 1;
    UInt    w = 1; while (w < pim->Width) { w <<= 1; wlevels++; }
    UInt    h = 1; while (h < pim->Height) { h <<= 1; hlevels++; }
    UInt    levelsNeeded;

    if (pim->IsDataCompressed() || pim->MipMapCount > 1)
        levelsNeeded = GTL::gmax<UInt>(1, pim->MipMapCount);
    else
        levelsNeeded = GTL::gmax<UInt>(wlevels, hlevels);

    GPtr<GImage> presampleImage;
    
    // Set the actual data.
    if (!pim->IsDataCompressed() && (w != pim->Width || h != pim->Height ||
        pim->Format == GImage::Image_RGB_888))
    {
        GASSERT_ON_RENDERER_RESAMPLING;

        // Resample the image to new size
        if (presampleImage = *new GImage(
            ((pim->Format == GImage::Image_RGB_888) ? GImage::Image_ARGB_8888 : pim->Format), w, h))
        {
            if (w != pim->Width || h != pim->Height)
            {
                // Resample will store an extra Alpha byte for RGB_888 -> RGBA_8888
                SoftwareResample(presampleImage->pData, w, h, presampleImage->Pitch,
                             pim->pData, pim->Width, pim->Height, pim->Pitch, bytesPerPixel);
            }
            else if (pim->Format == GImage::Image_RGB_888)
            {
                // Need to insert an alpha byte in the image data
                for (UInt y = 0; y < h; y++)
                {
                    UByte*  scanline = pim->GetScanline(y);
                    UByte*  pdest    = presampleImage->GetScanline(y);
                    for (UInt x = 0; x < w; x++)
                    {
                        pdest[x * 4 + 0] = scanline[x * 3 + 0]; // red
                        pdest[x * 4 + 1] = scanline[x * 3 + 1]; // green
                        pdest[x * 4 + 2] = scanline[x * 3 + 2]; // blue
                        pdest[x * 4 + 3] = 255; // alpha
                    }
                }
            }

            if (pim->MipMapCount > 1)
            {
                GFC_DEBUG_WARNING(1, "GRendererD3D10 - Existing mipmap levels have been skipped due to resampling");
                levelsNeeded = 1;
            }
        }
    }

    if (pim->MipMapCount <= 1 && bytesPerPixel != 1)
        levelsNeeded = 1;
//    else
  //      levelsNeeded = GTL::gmax(1u, levelsNeeded - 1);

    Desc.ArraySize = 1;
    Desc.MipLevels = levelsNeeded;
    Desc.Width = w;
    Desc.Height = h;
    Desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    Desc.Usage = D3D10_USAGE_DEFAULT;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;

    if (pim->IsDataCompressed() || pim->MipMapCount > 1)
    {
        GImageBase*            psourceImage= presampleImage ? presampleImage.GetPtr() : pim;
        D3D10_SUBRESOURCE_DATA data[16];

        for(UInt level = 0; level < levelsNeeded; level++)
        {           
            UInt mipW, mipH, mipPitch;
            data[level].pSysMem = psourceImage->GetMipMapLevelData(level, &mipW, &mipH, &mipPitch);
            data[level].SysMemPitch = mipPitch;
        }

        HRESULT result = pRenderer->pDevice->CreateTexture2D(&Desc, data, &pD3DTexture);
        if (result != S_OK)
        {
            GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create texture");
            Width = Height = 0;
            return 0;
        }
    }
    else if (levelsNeeded == 1)
    {
        D3D10_SUBRESOURCE_DATA data;
        data.pSysMem = presampleImage ? presampleImage->pData : pim->pData;
        data.SysMemPitch = presampleImage ? presampleImage->Pitch : pim->Pitch;

        HRESULT result = pRenderer->pDevice->CreateTexture2D(&Desc, &data, &pD3DTexture);
        if (result != S_OK)
        {
            GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create texture");
            Width = Height = 0;
            return 0;
        }
    }
    else
    {
        HRESULT result = pRenderer->pDevice->CreateTexture2D(&Desc, NULL, &pD3DTexture);
        if (result != S_OK)
        {
            GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create texture");
            Width = Height = 0;
            return 0;
        }

        // Will need a buffer for destructive mipmap generation
        if ((bytesPerPixel == 1) && (levelsNeeded >1) && !presampleImage)
        {
            GASSERT_ON_RENDERER_MIPMAP_GEN;

            // A bit hacky, needs to be more general
            presampleImage = *GImage::CreateImage(pim->Format, pim->Width, pim->Height);
            GASSERT(pim->Pitch == presampleImage->Pitch);
            memcpy(presampleImage->pData, pim->pData, pim->Height * pim->Pitch);        
        }

        GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pim;
        int mipw = psourceImage->Width, miph;

        for(UInt level = 0; level<levelsNeeded; level++)
        {           
            pRenderer->pDevice->UpdateSubresource(pD3DTexture, level, NULL, psourceImage->pData, mipw * bytesPerPixel, 0);
            
            // For Alpha-only images, we might need to generate the next mipmap level.
            if (level< (levelsNeeded-1))
            {
                if (psourceImage == pim)
                {
                    presampleImage = new GImage(*pim);
                    psourceImage = presampleImage.GetPtr();
                }

                GRendererD3D10Impl::MakeNextMiplevel(&mipw, &miph, psourceImage->pData);
            }
        }
    }    

    HRESULT result = pRenderer->pDevice->CreateShaderResourceView(pD3DTexture, NULL, &pShaderView);
    if (result != S_OK)
    {
        GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create shader view");
        Width = Height = 0;
        pD3DTexture->Release();
        pD3DTexture = 0;
        return 0;
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool GTextureD3D10Impl::InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                  int targetWidth , int targetHeight)
{
    if (!pRenderer || !pRenderer->pDevice)
        return 0;

    if (pD3DTexture)
        pD3DTexture->Release();

    if (format == GImage::Image_ARGB_8888)
        Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (format == GImage::Image_A_8)
        Desc.Format = DXGI_FORMAT_A8_UNORM;
    else 
    { // Unsupported format
        GASSERT(0);
        return 0;
    }

    Width   = (targetWidth == 0)  ? width : targetWidth;
    Height  = (targetHeight == 0) ? height: targetHeight;

    Desc.ArraySize = 1;
    Desc.MipLevels = 1+mipmaps;
    Desc.Width = Width;
    Desc.Height = Height;
    Desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    Desc.Usage = D3D10_USAGE_DEFAULT;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;

    UByte *p = (UByte*) GALLOC(Width*Height);
    memset (p, 64, Width*Height);
    D3D10_SUBRESOURCE_DATA data;
    data.pSysMem = p;
    data.SysMemPitch = Width;

    HRESULT result = pRenderer->pDevice->CreateTexture2D(&Desc, &data, &pD3DTexture);
    GFREE(p);
    if (result != S_OK)
    {
        GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create texture");
        Width = Height = 0;
        return 0;
    }
    result = pRenderer->pDevice->CreateShaderResourceView(pD3DTexture, NULL, &pShaderView);
    if (result != S_OK)
    {
        GFC_DEBUG_ERROR(1, "GTextureD3D10 - Can't create shader view");
        Width = Height = 0;
        pD3DTexture->Release();
        pD3DTexture = 0;
        return 0;
    }

    return 1;
}

void GTextureD3D10Impl::Update(int level, int n, const UpdateRect *rects, const GImageBase *pim)
{
    if (!pD3DTexture)
    {
        GFC_DEBUG_WARNING(1, "GTextureD3D10::Update failed, no texture");
        return;     
    }

    for (int i = 0; i < n; i++)
    {
        D3D10_BOX dest = { rects[i].dest.x, rects[i].dest.y, 0,
            rects[i].dest.x + rects[i].src.Width(), rects[i].dest.y + rects[i].src.Height(), 1 };

        pRenderer->pDevice->UpdateSubresource(pD3DTexture, level, &dest,
            pim->pData + pim->Pitch * rects[i].src.Top + pim->GetBytesPerPixel() * rects[i].src.Left, pim->Pitch, 0);
    }
}

bool GTextureD3D10Impl::InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight)
{ 
    GUNUSED3(pfilename,targetHeight,targetWidth);
    return 0;
}


// Factory.
GRendererD3D10*  GRendererD3D10::CreateRenderer()
{
    return new GRendererD3D10Impl;
}
