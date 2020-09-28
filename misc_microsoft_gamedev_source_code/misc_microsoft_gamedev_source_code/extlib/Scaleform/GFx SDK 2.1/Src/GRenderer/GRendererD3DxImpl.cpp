/**********************************************************************

Filename    :   GRendererD3DxImpl.cpp
Content     :   Direct3D 8 & 9 Sample renderer implementation
Created     :   
Authors     :   Michael Antonov

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

#ifndef GFC_D3D_VERSION
#define GFC_D3D_VERSION 9
#endif

#if (GFC_D3D_VERSION == 9)

#include "GRendererD3D9.h"

#define GRENDERER_USE_PS11
//#define GRENDERER_USE_PS20

#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#undef new
#endif
#include <d3d9.h>
#include <d3dx9.h>
#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#ifdef GRENDERER_USE_PS11
#include "ShadersD3D9/asm11.cpp"
#elif defined(GRENDERER_USE_PS20)
#include "ShadersD3D9/asm20.cpp"
#else
#include "ShadersD3D9/hlsl.cpp"
#endif

#define GRendererD3Dx           GRendererD3D9
#define GTextureD3Dx            GTextureD3D9
#define GRendererD3DxImpl       GRendererD3D9Impl
#define GTextureD3DxImpl        GTextureD3D9Impl

#define IDirect3DDeviceX        IDirect3DDevice9
#define IDirect3DTextureX       IDirect3DTexture9
#define IDirect3DSurfaceX       IDirect3DSurface9
#define IDirect3DIndexBufferX   IDirect3DIndexBuffer9
#define IDirect3DVertexBufferX  IDirect3DVertexBuffer9
#define D3DCAPSx                D3DCAPS9
#define D3DVIEWPORTx            D3DVIEWPORT9
#define LPDIRECT3Dx             LPDIRECT3D9
#define LPDIRECT3DDEVICEx       LPDIRECT3DDEVICE9

#define SetVertexShaderConstant SetVertexShaderConstantF
#define SetPixelShaderConstant  SetPixelShaderConstantF

// Used for functions that have an extra last argument of NULL in D3D9 only
#define NULL9                   , NULL


#elif (GFC_D3D_VERSION == 8)

#include "GRendererD3D8.h"

#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#undef new
#endif
#include <d3d8.h>
#include <d3dx8.h>
#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#define GRENDERER_USE_PS11
#include "ShadersD3D8/asm11.cpp"

#define GRendererD3Dx           GRendererD3D8
#define GTextureD3Dx            GTextureD3D8
#define GRendererD3DxImpl       GRendererD3D8Impl
#define GTextureD3DxImpl        GTextureD3D8Impl

#define IDirect3DDeviceX        IDirect3DDevice8
#define IDirect3DTextureX       IDirect3DTexture8
#define IDirect3DSurfaceX       IDirect3DSurface8
#define IDirect3DIndexBufferX   IDirect3DIndexBuffer8
#define IDirect3DVertexBufferX  IDirect3DVertexBuffer8
#define D3DCAPSx                D3DCAPS8
#define D3DVIEWPORTx            D3DVIEWPORT8
#define LPDIRECT3Dx             LPDIRECT3D8
#define LPDIRECT3DDEVICEx       LPDIRECT3DDEVICE8

// Used for functions that have an extra last argument of NULL in D3D9 only
#define NULL9                   

#else
#error Define GFC_D3D_VERSION to 8 or 9
#endif


// ***** Classes implemented
class GRendererD3DxImpl;
class GTextureD3DxImpl;


// ***** GTextureD3DxImpl implementation


// GTextureD3DxImpl declaration
class GTextureD3DxImpl : public GTextureD3Dx
{
    D3DFORMAT           TextureFormat;

public:

    // Renderer 
    GRendererD3DxImpl*  pRenderer;
    SInt                Width, Height;

    // D3Dx Texture pointer
    IDirect3DTextureX*  pD3DTexture;

    GTextureD3DxImpl(GRendererD3DxImpl *prenderer);
    ~GTextureD3DxImpl();

    // Obtains the renderer that create TextureInfo 
    virtual GRenderer*  GetRenderer() const;        
    virtual bool        IsDataValid() const;
    
    // Init Texture implementation.
    virtual bool        InitTexture(IDirect3DTextureX *ptex, SInt width, SInt height);
    virtual bool        InitTexture(GImageBase* pim, int targetWidth, int targetHeight);    
    virtual bool        InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                    int targetWidth, int targetHeight);
    virtual bool        InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight);
    virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim);

    // Remove texture from renderer, notifies of renderer destruction
    void                RemoveFromRenderer();

    virtual IDirect3DTextureX*  GetD3DTexture() const { return pD3DTexture; }
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


#if (GFC_D3D_VERSION == 9)

// Our vertex coords consist of two signed 16-bit integers, for (x,y) position only.
static D3DVERTEXELEMENT9 StripVertexDecl[] =
{
    {0, 0, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    D3DDECL_END()
};
static D3DVERTEXELEMENT9 GlyphVertexDecl[] =
{
    {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    {0,16, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    D3DDECL_END()
};

// Gouraud vertices used with Edge AA
static D3DVERTEXELEMENT9 VertexDeclXY16iC32[] =
{
    {0, 0, D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    D3DDECL_END()
};
static D3DVERTEXELEMENT9 VertexDeclXY16iCF32[] =
{
    {0, 0, D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    {0, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1 },
    D3DDECL_END()
};

// old shaders
static D3DVERTEXELEMENT9 GlyphVertexDecl_Szc[] =
{
    {0, 0, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 8, D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    {0,16, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    D3DDECL_END()
};
static D3DVERTEXELEMENT9 VertexDeclXY16iC32_Szc[] =
{
    {0, 0, D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 4, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    D3DDECL_END()
};
static D3DVERTEXELEMENT9 VertexDeclXY16iCF32_Szc[] =
{
    {0, 0, D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 4, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
    {0, 8, D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1 },
    D3DDECL_END()
};

// Vertex declaration lookup table, must correspond to VDeclType +1
static const D3DVERTEXELEMENT9 *VertexDeclTypeTable[VD_Count*2 - 2] =
{
    StripVertexDecl,
    GlyphVertexDecl,
    VertexDeclXY16iC32,
    VertexDeclXY16iCF32,

    StripVertexDecl,
    GlyphVertexDecl_Szc,
    VertexDeclXY16iC32_Szc,
    VertexDeclXY16iCF32_Szc
};


#elif (GFC_D3D_VERSION == 8)

static DWORD StripVertexDecl[] = {
    D3DVSD_STREAM(0),
    D3DVSD_REG(0, D3DVSDT_SHORT2),
    D3DVSD_END()
};

static DWORD GlyphVertexDecl_Szc[] = {
    D3DVSD_STREAM(0),
    D3DVSD_REG(0, D3DVSDT_FLOAT2),
    D3DVSD_REG(7, D3DVSDT_FLOAT2),
    D3DVSD_REG(5, D3DVSDT_D3DCOLOR),
    D3DVSD_END()    
};

static DWORD VertexDeclXY16iC32_Szc[] = {
    D3DVSD_STREAM(0),
    D3DVSD_REG(0, D3DVSDT_SHORT2),
    D3DVSD_REG(5, D3DVSDT_D3DCOLOR),
    D3DVSD_END()
};

static DWORD VertexDeclXY16iCF32_Szc[] = {
    D3DVSD_STREAM(0),
    D3DVSD_REG(0, D3DVSDT_SHORT2),
    D3DVSD_REG(5, D3DVSDT_D3DCOLOR),
    D3DVSD_REG(6, D3DVSDT_D3DCOLOR),
    D3DVSD_END()
};

static DWORD* VertexDeclTable[VD_Count] =
{
    0,
    StripVertexDecl,
    GlyphVertexDecl_Szc,
    VertexDeclXY16iC32_Szc,
    VertexDeclXY16iCF32_Szc
};

#endif


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
static const char *VertexShaderTextTable[VS_Count*2 - 2] =
{
    pStripVShaderText,
    pGlyphVShaderText,
    pStripVShaderXY16iC32Text,
    pStripVShaderXY16iCF32Text,
    pStripVShaderXY16iCF32_T2Text,

    pStripVShaderText,
    pGlyphVShaderSzcText,
    pStripVShaderXY16iC32SzcText,
    pStripVShaderXY16iCF32SzcText,
    pStripVShaderXY16iCF32Szc_T2Text
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

    // premultiplied alpha
    PS_AcNone,
    PS_AcSolidColor,
    PS_AcCxformTexture,
    PS_AcCxformTextureMultiply,
    PS_AcTextTexture,

    PS_AcCxformGouraud,
    PS_AcCxformGouraudNoAddAlpha,
    PS_AcCxformGouraudTexture,
    PS_AcCxform2Texture,

    PS_AcCxformGouraudMultiply,
    PS_AcCxformGouraudMultiplyNoAddAlpha,
    PS_AcCxformGouraudMultiplyTexture,
    PS_AcCxformMultiply2Texture,

    PS_Count,
    PS_CountSource = PS_AcNone,
    PS_AcOffset = PS_AcCxformTexture-PS_CxformTexture,
};



// Pixel shader constants assignments, maintained through driver
//
//  c0 -> constant solid color
// 
//  c2, c3 -> cxform
//


struct TextureStageDesc
{
    DWORD                       Stage; // -1 for last state
    D3DTEXTURESTAGESTATETYPE    State;
    DWORD                       Value;
};

struct PixelShaderDesc
{
    // Shader, used if not mull.
    const char*                 pShader;

    // Texture stage descriptors otherwise.
    const TextureStageDesc*     pPass0Desc;
    const TextureStageDesc*     pPass1Desc;
};


/* *** Fixed Function Pipeline Support Notes

  Texture stages below try to replicate the necessary shader behavior with fixed
  function pipeline texture stages. However, there a number of FF limitations that
  make this challenging or impossible.

  Specifically:

    1. Only one texture factor (D3DTA_TFACTOR) means that we can only
       implement Multiply color matrix component. We try to work around
       that by adding a second additive pass, but the math does not work
       out correctly; which means that all Cxforms are not supported right.

    2. Cxform ARE supported correctly for Non-EdgeAA solid fills, but
       only because they are implemented in software.

    3. EdgeAA with texturing is difficult to support due to the fact
       that we need (1) Color with alpha (2) Texture/Color mixing factor
       and (3) EdgeAA blend value. The (1) comes in as DIFFUSE, while
       (2) + (3) are packed into SPECULAR. However, there does not seem
       to be a direct way to use SPECULAR RGB channels to control blending.

    There is probably a way to support FF pipeline at least partially with
    textures, however, it would require complicated setup and would probably
    produce even more problems with color matrix.  For now, we just do not
    report the Cap_FillGouraudTex flag, which means that player will not
    pass textures to FillStyleGouraud. Hence, solid-color using shapes and
    strokes will get EdgeAA, while textured shapes will not.
*/


// Solid color: take color from factor.
static const TextureStageDesc pStates_TS_SolidColor[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_ALPHAOP,    D3DTOP_SELECTARG1   },
    { 0, D3DTSS_ALPHAARG1,  D3DTA_TFACTOR       },
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};

// Texture with color matrix.
//  Factor = ColorMatrix multiply
static const TextureStageDesc pStates_TS_CxformTexture[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_MODULATE     },  // C * Mc
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_COLORARG2,  D3DTA_TEXTURE       },
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // A * Ma
    { 0, D3DTSS_ALPHAARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_ALPHAARG2,  D3DTA_TEXTURE       },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};
// Second stage, attempts to add Ac and Aa of color matrix, but incorrectly
// (correct implementation impossible without shaders since we only have 1 texture factor).
static const TextureStageDesc pStates_TS_CxformTexture_Add[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },  // + Ac (from factor)
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },  
    { 0, D3DTSS_ALPHAOP,    D3DTOP_SELECTARG1   },  // + (Ca)
    { 0, D3DTSS_ALPHAARG1,  D3DTA_TEXTURE       },
    //{ 0, D3DTSS_ALPHAARG2,    D3DTA_TFACTOR       },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};


// Texture with color matrix.
static const TextureStageDesc pStates_TS_CxformTextureMultiply[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_MODULATE     },  // C * Mc
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_COLORARG2,  D3DTA_TEXTURE       },
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // A * Ma
    { 0, D3DTSS_ALPHAARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_ALPHAARG2,  D3DTA_TEXTURE       },
    // Hacky:
    // We need to get WHITE from somewhere, but diffuse, specular and TFACTOR are
    // already in use. Per-stage constants are not supported on most HW.
    // D3DTA_TEMP, however, is supported and defined to have a default value of 0. 
    // Alternative: use single-colored textures.
    { 1, D3DTSS_COLOROP,    D3DTOP_BLENDDIFFUSEALPHA        },
    { 1, D3DTSS_COLORARG1,  D3DTA_CURRENT                   },
    { 1, D3DTSS_COLORARG2,  D3DTA_TEMP | D3DTA_COMPLEMENT   }, // 1.0
    { 1, D3DTSS_ALPHAOP,    D3DTOP_SELECTARG1               },  
    { 1, D3DTSS_ALPHAARG1,  D3DTA_CURRENT                   },
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};


// Text rendering.
static const TextureStageDesc pStates_TS_TextTexture[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },  // Text color
    { 0, D3DTSS_COLORARG1,  D3DTA_CURRENT       },  
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // Glyph bitmap alpha * Text alpha
    { 0, D3DTSS_ALPHAARG1,  D3DTA_CURRENT       },
    { 0, D3DTSS_ALPHAARG2,  D3DTA_TEXTURE       },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};

static const TextureStageDesc pStates_TS_TextTextureColor[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },  // Text color
    { 0, D3DTSS_COLORARG1,  D3DTA_DIFFUSE       },  
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // Glyph bitmap alpha * Text alpha
    { 0, D3DTSS_ALPHAARG1,  D3DTA_DIFFUSE       },
    { 0, D3DTSS_ALPHAARG2,  D3DTA_TEXTURE       },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};

// Last stage for alpha compositing.
static const TextureStageDesc pStates_TS_AlphaComposite[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_MODULATE                           },
    { 0, D3DTSS_COLORARG1,  D3DTA_CURRENT                             },  
    { 0, D3DTSS_COLORARG2,  D3DTA_CURRENT | D3DTA_ALPHAREPLICATE      },  
    { 0, D3DTSS_ALPHAOP,    D3DTOP_SELECTARG1                         },
    { 0, D3DTSS_ALPHAARG1,  D3DTA_CURRENT                             },
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};


// ***** Edge-AA versions.

// Texture with color matrix.
//  Factor = ColorMatrix multiply
static const TextureStageDesc pStates_TS_CxformGauraud[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_MODULATE     },  // C * Mc
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_COLORARG2,  D3DTA_DIFFUSE       },
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // A * Ma
    { 0, D3DTSS_ALPHAARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_ALPHAARG2,  D3DTA_DIFFUSE       },  
    // EdgeAA:
    { 1, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },
    { 1, D3DTSS_COLORARG1,  D3DTA_CURRENT       },  
    { 1, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // A * Ma
    { 1, D3DTSS_ALPHAARG1,  D3DTA_CURRENT       },
    { 1, D3DTSS_ALPHAARG2,  D3DTA_SPECULAR      },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};
static const TextureStageDesc pStates_TS_CxformGauraud_Add[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },  // + Ac (from factor)
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },  
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // + (Ca)
    { 0, D3DTSS_ALPHAARG1,  D3DTA_DIFFUSE       },  
    { 0, D3DTSS_ALPHAARG2,  D3DTA_SPECULAR      },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};

 
//  Factor = ColorMatrix multiply
static const TextureStageDesc pStates_TS_CxformGauraudNoAddAlpha[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_MODULATE     },  // C * Mc
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_COLORARG2,  D3DTA_DIFFUSE       },
    { 0, D3DTSS_ALPHAOP,    D3DTOP_MODULATE     },  // A * Ma
    { 0, D3DTSS_ALPHAARG1,  D3DTA_TFACTOR       },
    { 0, D3DTSS_ALPHAARG2,  D3DTA_DIFFUSE       },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};
static const TextureStageDesc pStates_TS_CxformGauraudNoAddAlpha_Add[] =
{
    { 0, D3DTSS_COLOROP,    D3DTOP_SELECTARG1   },  // + Ac (from factor)
    { 0, D3DTSS_COLORARG1,  D3DTA_TFACTOR       },  
    { 0, D3DTSS_ALPHAOP,    D3DTOP_SELECTARG1   },  // + (Ca)
    { 0, D3DTSS_ALPHAARG1,  D3DTA_DIFFUSE       },  
    //{ 0, D3DTSS_ALPHAARG2,    D3DTA_SPECULAR      },  
    { (DWORD)-1, D3DTSS_FORCE_DWORD, 0 }
};



PixelShaderDesc PixelShaderInitTable[PS_Count] =
{   
    // Non-AA Shaders.
    { pSource_PS_SolidColor,                pStates_TS_SolidColor,  0                               },
    { pSource_PS_CxformTexture,             pStates_TS_CxformTexture, pStates_TS_CxformTexture_Add  },
    { pSource_PS_CxformTextureMultiply,     pStates_TS_CxformTextureMultiply, 0                     },
    { pSource_PS_TextTextureColor,          pStates_TS_TextTextureColor, 0                          },

    // AA Shaders.
    { pSource_PS_CxformGauraud,                     pStates_TS_CxformGauraud, pStates_TS_CxformGauraud_Add },
    { pSource_PS_CxformGauraudNoAddAlpha,           pStates_TS_CxformGauraudNoAddAlpha, pStates_TS_CxformGauraudNoAddAlpha_Add },
    // Texture stage fixed-function fall-backs only (their implementation is incorrect).
    { pSource_PS_CxformGauraudTexture,              pStates_TS_CxformTexture,           0 },
    { pSource_PS_Cxform2Texture,                    pStates_TS_CxformTexture,           0 },
    { pSource_PS_CxformGauraudMultiply,             pStates_TS_CxformGauraud,           0 },
    { pSource_PS_CxformGauraudMultiplyNoAddAlpha,   pStates_TS_CxformGauraudNoAddAlpha, 0 },
    { pSource_PS_CxformGauraudMultiplyTexture,      pStates_TS_CxformTextureMultiply,   0 },
    { pSource_PS_CxformMultiply2Texture,            pStates_TS_CxformTextureMultiply,   0 },

#ifndef GRENDERER_USE_PS11
    {0},

    // alpha compositing shaders
    { pSource_PS_AcSolidColor},
    { pSource_PS_AcCxformTexture},
    { pSource_PS_AcCxformTextureMultiply},
    { pSource_PS_AcTextTextureColor},
    { pSource_PS_AcCxformGauraud},
    { pSource_PS_AcCxformGauraudNoAddAlpha},
    { pSource_PS_AcCxformGauraudTexture},
    { pSource_PS_AcCxform2Texture},
    { pSource_PS_AcCxformGauraudMultiply},
    { pSource_PS_AcCxformGauraudMultiplyNoAddAlpha},
    { pSource_PS_AcCxformGauraudMultiplyTexture},
    { pSource_PS_AcCxformMultiply2Texture},
#endif
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
    IDirect3DDeviceX*       pDevice;

    // Flag set if buffers are released due to lost device.
    bool                    LostDevice;

    // Vertex data pointer
    const void*             pVertexData;
    const void*             pIndexData;
    VertexFormat            VertexFmt;
    UInt                    VertexSize;
    _D3DFORMAT              IndexFmt;
    UInt                    VertexCount;
    UInt                    IndexCount;

    // Dynamic Vertex/Index buffer state 
    GPtr<IDirect3DVertexBufferX>    pVertexBuffer;
    GPtr<IDirect3DIndexBufferX>     pIndexBuffer;
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
    bool    Initialize(IDirect3DDeviceX *pdevice)
    {
        pDevice = pdevice;
        if (CreateDynamicBuffers())        
            return 1;        
        pDevice = 0;
        return 0;
    }

    void    Reset()
    {
        ReleaseDynamicBuffers();
        pDevice = 0;
    }

    void    BeginDisplay()
    {
        if (LostDevice)
            CreateDynamicBuffers();

#if (GFC_D3D_VERSION == 9)
        pDevice->SetStreamSourceFreq(0, 1);
        pDevice->SetIndices(pIndexBuffer);
#endif
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
    _D3DFORMAT      GetD3DIndexFormat() const   { return IndexFmt; }
    UInt            GetStartIndex() const       { return VBDataIndex; }
};


// *** Dynamic Vertex Stream implementation

GDynamicVertexStream::GDynamicVertexStream()
{
    pDevice             = 0;
    LostDevice          = 0;

    pVertexData         = 0;
    pIndexData          = 0;
    IndexFmt            = D3DFMT_UNKNOWN;
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

        // Min acceptable buffer must be large enough for glyphs due to logic used in DrawBitmaps.
        GCOMPILER_ASSERT(DefaultVertexBufferSize/8 >= GlyphBufferSize);

        // Loop trying several vertex buffer sizes.
        do
        {
            createResult = pDevice->CreateVertexBuffer(
                                    VertexBufferSize, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,                                    
                                    0, D3DPOOL_DEFAULT, &pVertexBuffer.GetRawRef() NULL9);
            
            // If failed, try smaller size while reasonable.
            if (FAILED(createResult))
            {                
                if (VertexBufferSize > DefaultVertexBufferSize/8)
                    VertexBufferSize = VertexBufferSize / 2;
                else
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3Dx - Failed to create dynamic vertex buffer");
                    VertexBufferSize = 0;
                    return 0;
                }
            }

        } while(FAILED(createResult));
    }

    if (!pIndexBuffer)
    {
        HRESULT createResult;

        do
        {
            createResult = pDevice->CreateIndexBuffer(
                                IndexBufferSize, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,
                                D3DFMT_INDEX16, D3DPOOL_DEFAULT,
                                &pIndexBuffer.GetRawRef() NULL9);

            // If failed, try smaller size while reasonable.
            if (FAILED(createResult))
            {
                if (IndexBufferSize > DefaultIndexBufferSize/4)
                    IndexBufferSize = IndexBufferSize / 2;
                else
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3Dx - Failed to create dynamic index buffer");
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
    case GRenderer::Index_None:    IndexFmt = D3DFMT_UNKNOWN; break;
    case GRenderer::Index_16:      IndexFmt = D3DFMT_INDEX16; break;
    case GRenderer::Index_32:      IndexFmt = D3DFMT_INDEX32; break;
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
            UInt    indexSize = (IndexFmt == D3DFMT_INDEX16) ? sizeof(UInt16) : sizeof(UInt32);

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
        GASSERT(IndexFmt == D3DFMT_INDEX16);

#if (GFC_D3D_VERSION == 9)
        pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            Primitive.BaseVertexIndex, Primitive.MinVertexIndex, Primitive.NumVertices,
            (IBDataOffset / sizeof(UInt16)) + Primitive.StartIndex, Primitive.TriangleCount );

#elif (GFC_D3D_VERSION == 8)
        pDevice->SetIndices(pIndexBuffer, Primitive.BaseVertexIndex + VBDataIndex);
        pDevice->DrawIndexedPrimitive(
            D3DPT_TRIANGLELIST,
            Primitive.MinVertexIndex, Primitive.NumVertices,
            (IBDataOffset / sizeof(UInt16)) + Primitive.StartIndex, Primitive.TriangleCount );
#endif
    }

    else if (RenderMethod == RM_NotIndexed)
    {
        // Vertex buffer could not fit, render in chunks with software indexing.       
        int    triangles = 0;

        // For now, InitVerticesFromIndex doesn't handle 32 bit buffers.
        GASSERT(IndexFmt == D3DFMT_INDEX16);

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
            pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, GetStartIndex(), batch);
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
            UInt    indexSize = (IndexFmt == D3DFMT_INDEX16) ? sizeof(UInt16) : sizeof(UInt32);
            void *  pbuffer = LockIndexBuffer(batch * 3, indexSize);
            if (!pbuffer)
                return;
            
            // Copy this indices for this batch.
            memcpy( pbuffer,
                    ((UByte*)pIndexData) + (Primitive.StartIndex +(triangles * 3)) * indexSize,
                    batch * 3 * indexSize );
            UnlockIndexBuffer();

            // Draw using uploaded data.

#if (GFC_D3D_VERSION == 9)
            pDevice->DrawIndexedPrimitive(
                D3DPT_TRIANGLELIST,
                Primitive.BaseVertexIndex, Primitive.MinVertexIndex, Primitive.NumVertices,
                (IBDataOffset / sizeof(UInt16)), batch);

#elif (GFC_D3D_VERSION == 8)
            pDevice->SetIndices(pIndexBuffer, Primitive.BaseVertexIndex + VBDataIndex);
            pDevice->DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST,
                    Primitive.MinVertexIndex, Primitive.NumVertices,
                    (IBDataOffset / sizeof(UInt16)), batch);
#endif

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

    DWORD   lockFlags; 
    void*   pbuffer = 0;
    UInt    offset;

    VBDataInBuffer = 0;

    // Determine where in the buffer we go.
    if (size + NextVBOffset + vertexSize < VertexBufferSize)
    {
        offset          = NextVBOffset;
#if (GFC_D3D_VERSION == 8)
        if (offset % vertexSize)
            offset += vertexSize - (offset % vertexSize);
#endif
        NextVBOffset    = offset + size;
        lockFlags       = D3DLOCK_NOOVERWRITE;
    }
    else
    {
        offset          = 0;
        NextVBOffset    = size;
        lockFlags       = D3DLOCK_DISCARD;
    }

#if (GFC_D3D_VERSION == 9)
    if (FAILED(pVertexBuffer->Lock(offset, size, (void**)&pbuffer, lockFlags)))   

#elif (GFC_D3D_VERSION == 8)
    if (FAILED(pVertexBuffer->Lock(offset, size, (BYTE**)&pbuffer, lockFlags)))   
#endif
        return 0;
    // Some drivers may return bad pointers here for dynamic buffers (i.e. old NVidia drivers)
    if (::IsBadWritePtr(pbuffer, size))
    {
        static bool warningReported = 0;
        GFC_DEBUG_WARNING(!warningReported,
                          "GD3DxRenderer::LockVertexData - Direct3D Lock returned bad pointer, function failed");
        warningReported = 1;
        pVertexBuffer->Unlock();
        pbuffer = 0;
        return 0;
    }

    VBDataOffset       = offset;
#if (GFC_D3D_VERSION == 8)
    VBDataIndex        = offset / vertexSize;
#endif
    VertexSizeInBuffer = vertexSize;
    return pbuffer;
}


void    GDynamicVertexStream::UnlockVertexBuffer()
{
    pVertexBuffer->Unlock();

#if (GFC_D3D_VERSION == 9)
    // Set this buffer offset on a device.
    pDevice->SetStreamSource(0, pVertexBuffer, VBDataOffset, VertexSizeInBuffer);

#elif (GFC_D3D_VERSION == 8)
    // GetStartIndex() is used instead of stream offsets
    pDevice->SetStreamSource(0, pVertexBuffer, VertexSizeInBuffer);
#endif
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

    DWORD   lockFlags; 
    void*   pbuffer = 0;
    UInt    offset;

    IBDataInBuffer = 0;

    // Determine where in the buffer we go.
    if ((IndexBufferSize - NextIBOffset) > size)
    {
        offset          = NextIBOffset;
        NextIBOffset    = NextIBOffset + size;
        lockFlags       = D3DLOCK_NOOVERWRITE;
    }
    else
    {
        offset          = 0;
        NextIBOffset    = size;
        lockFlags       = D3DLOCK_DISCARD;
    }

#if (GFC_D3D_VERSION == 9)
    if (FAILED(pIndexBuffer->Lock(offset, size, (void**)&pbuffer, lockFlags)))   

#elif (GFC_D3D_VERSION == 8)
    if (FAILED(pIndexBuffer->Lock(offset, size, (BYTE**)&pbuffer, lockFlags)))   
#endif
        return 0;
    if (::IsBadWritePtr(pbuffer, size))
    {        
        pIndexBuffer->Unlock();
        pbuffer = 0;
        return 0;
    }

    IBDataOffset = offset;
    return pbuffer;
}

void    GDynamicVertexStream::UnlockIndexBuffer()
{
    pIndexBuffer->Unlock();        
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





// ***** GRendererD3Dx Implementation

class GRendererD3DxImpl : public GRendererD3Dx
{
public:

    // Some renderer state. 
    bool                ModeSet;
    SInt                RenderMode;

    // This flag is set if pixel shader support is available.
    bool                UsePixelShaders;
    // Use BlendFuncSeparate
    bool                UseAcBlend;

    // Shaders and declarations that are currently set.
    VDeclType           VDeclIndex;
    VShaderType         VShaderIndex;

    // Current pixel shader index
    PixelShaderType     PShaderIndex;
    int                 PShaderPass;

    // Video Mode Configuration Flags (VMConfigFlags)
    UInt32              VMCFlags;

#if (GFC_D3D_VERSION == 9)
    // Created vertex declarations and shaders.
    GPtr<IDirect3DVertexDeclaration9>   VertexDecls[VD_Count];
    GPtr<IDirect3DVertexShader9>        VertexShaders[VS_Count];

    // Allocated pixel shaders
    GPtr<IDirect3DPixelShader9>         PixelShaders[PS_Count];

#elif (GFC_D3D_VERSION == 8)
    DWORD               VertexDecls[VD_Count];
    DWORD               VertexShaders[VS_Count];
    DWORD               PixelShaders[PS_Count];
#endif

    // Direct3DDevice
    IDirect3DDeviceX*   pDevice;
    
    // This flag indicates whether we've checked for stencil after BeginDisplay or not.
    bool                StencilChecked;
    // This flag is stencil is available, after check.
    bool                StencilAvailable;
    bool                MultiBitStencil;
    bool                DepthBufferAvailable;
    bool                DrawingMask;
    DWORD               StencilCounter;
    // Increment/decreent ops we need for stencil.
    D3DSTENCILOP        StencilOpInc;
    D3DSTENCILOP        StencilOpDec;

    // format for alpha textures - old cards don't support all formats
    D3DFORMAT           AlphaTextureFormat;
    
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


    // Presentation parameters specified to configure the mode.
    D3DPRESENT_PARAMETERS   PresentParams;
    HWND                    hWnd;

    // Linked list used for buffer cache testing, otherwise holds no data.
    CacheNode               CacheList;

    // Surface for updating textures
    SInt                    TempW, TempH;
    GPtr<IDirect3DTextureX> pTempTextureA, pTempTextureLA, pTempTextureRGBA;
    GPtr<IDirect3DSurfaceX> pTempSurfaceA, pTempSurfaceLA, pTempSurfaceRGBA;
    bool                    UseDynamicTex;

    // resources to retain during tiling frames
    GTL::garray<GPtr<GTexture> > FrameResources;

    // all D3D calls should be done on the main thread, but textures can be released from 
    // different threads so we collect system resources and release them on the main thread
    GTL::garray<IDirect3DTextureX*> ResourceReleasingQueue;
    GLock                           ResourceReleasingQueueLock;

    // this method is called from GTextureXXX destructor to put a system resource into releasing queue
    void AddResourceForReleasing(IDirect3DTextureX* ptexture)
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

    GRendererD3DxImpl()
    {
        ModeSet             = 0;
        UsePixelShaders     = 0;
        UseDynamicTex       = 0;
        
        VMCFlags            = 0;

        StencilChecked      = 0;
        StencilAvailable    = 0;
        DepthBufferAvailable= 0;
        MultiBitStencil     = 0;
        StencilCounter      = 0;
        // Increment/decreent ops; replace means not available (until mode config).
        StencilOpInc        = D3DSTENCILOP_REPLACE;
        StencilOpDec        = D3DSTENCILOP_REPLACE;

        VDeclIndex          = VD_None;
        VShaderIndex        = VS_None;
        PShaderIndex        = PS_None;
        PShaderPass         = 0;

        SampleMode[0]       = Sample_Linear;      
        SampleMode[1]       = Sample_Linear;      
        
        pDevice             = 0;
        hWnd                = 0;
        BlendModeStack.set_size_policy(GTL::garray<BlendType>::Buffer_NoShrink);

        RenderMode          = 0;
        TempW               = 512;
        TempH               = 512;
        AlphaTextureFormat  = D3DFMT_A8;

        for (int i=0; i< VS_Count; i++)
            VertexShaders[i] = 0;
        for (int i=0; i< PS_Count; i++)
            PixelShaders[i] = 0;
    }

    ~GRendererD3DxImpl()
    {
        Clear();
    }
    
    void Clear() 
    {
        // Remove/notify all textures
        {
        GLock::Locker guard(&TexturesLock);
        while (Textures.pFirst != &Textures)
            ((GTextureD3DxImpl*)Textures.pFirst)->RemoveFromRenderer();
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


#if (GFC_D3D_VERSION == 9)
    // Helpers used to create vertex declarations and shaders. 
    bool    CreateVertexDeclaration(GPtr<IDirect3DVertexDeclaration9> *pdeclPtr, const D3DVERTEXELEMENT9 *pvertexElements)
    {
        if (!*pdeclPtr)
        {
            if (pDevice->CreateVertexDeclaration(pvertexElements, &pdeclPtr->GetRawRef()) != S_OK)
            {
                GFC_DEBUG_WARNING(1, "GRendererD3Dx - failed to create vertex declaration");
                return 0;
            }
        }
        return 1;
    }

    bool    CreateVertexShader(GPtr<IDirect3DVertexShader9> *pshaderPtr, const char* pshaderText)
    {
        if (!*pshaderPtr)
        {
            GPtr<ID3DXBuffer> pshader;
            GPtr<ID3DXBuffer> pmsg;
            HRESULT hr;

#ifdef GRENDERER_VSHADER_PROFILE
            hr = D3DXCompileShader(pshaderText, (UInt)strlen(pshaderText),
                NULL, NULL, "main", GRENDERER_VSHADER_PROFILE, D3DXSHADER_DEBUG, &pshader.GetRawRef(), NULL, NULL);
#else
            hr = D3DXAssembleShader(pshaderText, (UInt)strlen(pshaderText),
                NULL, NULL, D3DXSHADER_DEBUG, &pshader.GetRawRef(), NULL);
#endif

            if (FAILED(hr))
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3Dx - VertexShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }
            if (!pshader ||
                ((hr = pDevice->CreateVertexShader((DWORD*)pshader->GetBufferPointer(), &pshaderPtr->GetRawRef())) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3Dx - Can't create D3Dx vshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }

#elif (GFC_D3D_VERSION == 8)
    bool    CreateVertexShader(DWORD *pshaderPtr, const char* pshaderText, DWORD *pDecl)
    {
        if (!*pshaderPtr)
        {
            GPtr<ID3DXBuffer> pshader;
            GPtr<ID3DXBuffer> pmsg;
            HRESULT hr;

            hr = D3DXAssembleShader(pshaderText, (UInt)strlen(pshaderText), 0, NULL, &pshader.GetRawRef(), &pmsg.GetRawRef());
            if (FAILED(hr))
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3Dx - VertexShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }
            if (!pshader ||
                ((hr = pDevice->CreateVertexShader( pDecl, (DWORD*)pshader->GetBufferPointer(),
                                                    pshaderPtr, 0)) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3Dx - Can't create D3Dx vshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }
#endif

#if (GFC_D3D_VERSION == 9)
    bool    CreatePixelShader(GPtr<IDirect3DPixelShader9> *pshaderPtr, const char* pshaderText)
    {
        if (!*pshaderPtr)
        {
            GPtr<ID3DXBuffer> pmsg;
            GPtr<ID3DXBuffer> pshader;
            HRESULT hr;

#ifdef GRENDERER_PSHADER_PROFILE
            hr = D3DXCompileShader(pshaderText, (UInt)strlen(pshaderText),
                NULL, NULL, "main", GRENDERER_PSHADER_PROFILE, D3DXSHADER_DEBUG, &pshader.GetRawRef(), &pmsg.GetRawRef(), NULL);
#else
            hr = D3DXAssembleShader(pshaderText, (UInt)strlen(pshaderText),
                NULL, NULL, D3DXSHADER_DEBUG, &pshader.GetRawRef(), &pmsg.GetRawRef());
#endif

            if (FAILED(hr))
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3D9 - PixelShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }
            if (!pshader ||
                ((hr = pDevice->CreatePixelShader(  (DWORD*)pshader->GetBufferPointer(),
                &pshaderPtr->GetRawRef())) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3D9 - Can't create D3D9 pshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }

#elif (GFC_D3D_VERSION == 8)
    bool    CreatePixelShader(DWORD *pshaderPtr, const char* pshaderText)
    {
        if (!*pshaderPtr)
        {
            GPtr<ID3DXBuffer> pmsg;
            GPtr<ID3DXBuffer> pshader;
            HRESULT hr;

            hr = D3DXAssembleShader(pshaderText, (UInt)strlen(pshaderText), 0, NULL, &pshader.GetRawRef(), &pmsg.GetRawRef());

            if (FAILED(hr))
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3Dx - PixelShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }
            if (!pshader ||
                ((hr = pDevice->CreatePixelShader(  (DWORD*)pshader->GetBufferPointer(),
                                                    pshaderPtr)) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererD3Dx - Can't create D3Dx pshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }
#endif


    // Initialize the shaders we use for SWF mesh rendering.
    bool    InitShaders(D3DCAPSx *caps)
    {
        bool success = 1;
        UInt i;

#if (GFC_D3D_VERSION == 9)
        UInt vstart = 0;

        if ((caps->DeclTypes & D3DDTCAPS_UBYTE4N) == 0)
            vstart = 1;

        for (i = 1; i < VD_Count; i++)
            if (!CreateVertexDeclaration(&VertexDecls[i], VertexDeclTypeTable[vstart*(VD_Count-1)+i-1]))
            {
                success = 0;
                break;
            }

        for (i = 1; i < VS_Count; i++)
            if (!CreateVertexShader(&VertexShaders[i], VertexShaderTextTable[vstart*(VS_Count-1)+i-1]))
            {
                success = 0;
                break;
            }

#elif (GFC_D3D_VERSION == 8)
        GUNUSED(caps);
        for (i = 1; i < VS_Count; i++)
            if (!CreateVertexShader(&VertexShaders[i], VertexShaderTextTable[(VS_Count-1)+i-1],
                VertexDeclTable[VertexShaderDeclTable[i]]))
            {
                success = 0;
                break;
            }
#endif

        // We only use pixel shaders conditionally if their support is available.
        if (UsePixelShaders)
        {
#if (GRENDERER_SHADER_VERSION == 0x0101) && !defined(GRENDERER_PSHADER_PROFILE)
            for (i = 1; i < PS_CountSource; i++)
            {           
                if (PixelShaderInitTable[i-1].pShader)
                {
                    if (!CreatePixelShader(&PixelShaders[i], PixelShaderInitTable[i-1].pShader))
                    {
                        success = 0;
                        break;
                    }

                    char pSourceAc[512];

                    gfc_strcpy(pSourceAc, 512,  PixelShaderInitTable[i-1].pShader);
                    gfc_strcat(pSourceAc, 512, "mul  r0.rgb, r0, r0.a\n");

                    if (!CreatePixelShader(&PixelShaders[i+PS_AcOffset], pSourceAc))
                    {
                        success = 0;
                        break;
                    }
                }
            }
#else
            for (i = 1; i < PS_Count; i++)
            {           
                if (PixelShaderInitTable[i-1].pShader)
                {
                    if (!CreatePixelShader(&PixelShaders[i], PixelShaderInitTable[i-1].pShader))
                    {
                        success = 0;
                        break;
                    }
                }
            }
#endif
        }

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

#if (GFC_D3D_VERSION == 8)
        for (i=0; i< VS_Count; i++)
            if (VertexShaders[i])
                pDevice->DeleteVertexShader(VertexShaders[i]);

        for (i=0; i< PS_Count; i++)
            if (PixelShaders[i])
                pDevice->DeletePixelShader(PixelShaders[i]);

#elif (GFC_D3D_VERSION == 9)
        for (i=0; i< VD_Count; i++)
            if (VertexDecls[i])
                VertexDecls[i] = 0;
#endif

        for (i=0; i< VS_Count; i++)
            if (VertexShaders[i])
                VertexShaders[i] = 0;

        for (i=0; i< PS_Count; i++)
            if (PixelShaders[i])
                PixelShaders[i] = 0;
    }

    // Sets a shader to specified type.
    void    SetVertexDecl(VDeclType vd)
    {
        if (VDeclIndex != vd)
        {
            VDeclIndex = vd;
#if (GFC_D3D_VERSION == 9)
            pDevice->SetVertexDeclaration(VertexDecls[vd]);
#endif
        }
    }
    void    SetVertexShader(VShaderType vt)
    {
        if (VShaderIndex != vt)
        {
            GASSERT(VertexShaderDeclTable[vt] == VDeclIndex);
            VShaderIndex = vt;
            pDevice->SetVertexShader(VertexShaders[vt]);
        }
    }


    void    ApplyTextureStageConstants(const TextureStageDesc* plist)
    {
        DWORD endingStage = 0;

        if (plist)
        {       
            while (plist->Stage != -1)
            {
                pDevice->SetTextureStageState(plist->Stage, plist->State, plist->Value);
                if (plist->Stage >= endingStage)
                    endingStage = plist->Stage + 1;
                plist++;
            }
        }

        if (RenderMode & GViewport::View_AlphaComposite)
        {
            const TextureStageDesc* plist = pStates_TS_AlphaComposite;

            while (plist->Stage != -1)
            {
                pDevice->SetTextureStageState(endingStage, plist->State, plist->Value);
                plist++;
            }

            endingStage++;
        }

        // Disable other stages.
        pDevice->SetTextureStageState(endingStage, D3DTSS_COLOROP, D3DTOP_DISABLE);
        pDevice->SetTextureStageState(endingStage, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    }


    // Set shader to a device based on a constant
    void    SetPixelShader(PixelShaderType shaderType, int pass = 0)
    {
        if ((shaderType != PShaderIndex) || (PShaderPass != pass))
        {
            if (UsePixelShaders)
            {
                if (RenderMode & GViewport::View_AlphaComposite)
                    pDevice->SetPixelShader(PixelShaders[int(shaderType) + int(PS_AcOffset)]);
                else
                    pDevice->SetPixelShader(PixelShaders[shaderType]);
            }
            else
            {
                GASSERT(pass <= 1);

                if (shaderType == PS_None)
                    ApplyTextureStageConstants(0);
                else
                {
                    if (pass == 0)
                    {
                        ApplyTextureStageConstants(PixelShaderInitTable[shaderType-1].pPass0Desc);
                    }
                    else // if (pass == 1)
                    {
                        ApplyTextureStageConstants(PixelShaderInitTable[shaderType-1].pPass1Desc);
                    }
                }
            }

            PShaderIndex = shaderType;
            PShaderPass  = pass;
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
    void    ApplyFillTexture(const FillTexture &fill, UInt stageIndex, UInt matrixVertexConstIndex)
    {
        GASSERT (fill.pTexture != 0);
        if (fill.pTexture == 0) return; // avoid crash in release build

        GTextureD3DxImpl* ptexture = ((GTextureD3DxImpl*)fill.pTexture);

        if (!ptexture->pD3DTexture)
            ptexture->CallRecreate();

        pDevice->SetTexture(stageIndex, ptexture->pD3DTexture);

        // Set sampling
#if (GFC_D3D_VERSION == 9)
        if (fill.WrapMode == Wrap_Clamp)
        {                       
            pDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            pDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        }
        else
        {
            GASSERT(fill.WrapMode == Wrap_Repeat);
            pDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
            pDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        }

#elif (GFC_D3D_VERSION == 8)
        if (fill.WrapMode == Wrap_Clamp)
        {                       
            pDevice->SetTextureStageState(stageIndex, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
            pDevice->SetTextureStageState(stageIndex, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
        }
        else
        {
            GASSERT(fill.WrapMode == Wrap_Repeat);
            pDevice->SetTextureStageState(stageIndex, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
            pDevice->SetTextureStageState(stageIndex, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
        }
#endif

        // Set up the bitmap matrix for texgen.
        float   InvWidth = 1.0f / ptexture->Width;
        float   InvHeight = 1.0f / ptexture->Height;

        const Matrix&   m = fill.TextureMatrix;
        Float   p[4] = { 0, 0, 0, 0 };
        p[0] = m.M_[0][0] * InvWidth;
        p[1] = m.M_[0][1] * InvWidth;
        p[3] = m.M_[0][2] * InvWidth;
        pDevice->SetVertexShaderConstant(matrixVertexConstIndex, p, 1);

        p[0] = m.M_[1][0] * InvHeight;
        p[1] = m.M_[1][1] * InvHeight;
        p[3] = m.M_[1][2] * InvHeight;
        pDevice->SetVertexShaderConstant(matrixVertexConstIndex + 1, p, 1);
    }


    void    ApplyPShaderCxform(const Cxform &cxform, UInt cxformPShaderConstIndex) const
    {
        if (UsePixelShaders)
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

            pDevice->SetPixelShaderConstant(cxformPShaderConstIndex, cxformData, 2);
        }

        else
        {
            // For fixed - function states we store Multiply into TEXTUREFACTOR.
            // Add can not be supported correctly, although we try to 'fake' it with second pass.
            D3DCOLOR color = D3DCOLOR_ARGB(
                        (UByte)(cxform.M_[3][0] * 255.0f),
                        (UByte)(cxform.M_[0][0] * 255.0f),
                        (UByte)(cxform.M_[1][0] * 255.0f),
                        (UByte)(cxform.M_[2][0] * 255.0f)
                         );
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);
        }   
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
        void    Apply(GRendererD3DxImpl *prenderer) const
        {
            IDirect3DDeviceX* pdevice = prenderer->pDevice;
            GASSERT(Mode != FM_None);


            // Complex
            prenderer->ApplyBlendMode(prenderer->BlendMode, (Mode == FM_Gouraud));

            pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 
                ((Color.GetAlpha()== 0xFF) &&
                (prenderer->BlendMode <= Blend_Normal) &&
                (Mode != FM_Gouraud))  ?  FALSE : TRUE);

            
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
                prenderer->ApplySampleMode(0, Fill.SampleMode);

                
                if (Fill.pTexture == NULL)
                {
                    // Just in case, for release build.
                    prenderer->SetPixelShader(PS_None);
                }
                else
                {
                    pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

                    if ((prenderer->BlendMode == Blend_Multiply) ||
                        (prenderer->BlendMode == Blend_Darken) )
                        prenderer->SetPixelShader(PS_CxformTextureMultiply);
                    else
                        prenderer->SetPixelShader(PS_CxformTexture);

                    prenderer->ApplyPShaderCxform(BitmapColorTransform, 2);

                    // Set texture to stage 0; matrix to constants 4 and 5
                    prenderer->ApplyFillTexture(Fill, 0, 4);
                }
            }
            

            // Edge AA - relies on Gouraud shading and texture mixing
            else if (Mode == FM_Gouraud)
            {

                PixelShaderType shader = PS_None;
                
                // No texture: generate color-shaded triangles.
                if (Fill.pTexture == NULL)
                {
                    pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);


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
                    pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

                    // Set texture to stage 0; matrix to constants 4 and 5
                    prenderer->ApplyFillTexture(Fill, 0, 4);
                    prenderer->ApplySampleMode(0, Fill.SampleMode);

                    if ((GouraudType == GFill_1TextureColor) ||
                        (GouraudType == GFill_1Texture))
                    {
                        shader = PS_CxformGauraudTexture;
                    }
                    else
                    {
                        shader = PS_Cxform2Texture;

                        // Set second texture to stage 1; matrix to constants 6 and 7
                        prenderer->ApplyFillTexture(Fill2, 1, 6);
                        prenderer->ApplySampleMode(1, Fill2.SampleMode);
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
                prenderer->ApplyPShaderCxform(BitmapColorTransform, 2);
            }
        
        }


        
        // Return true if we need to do a second pass to make a valid color.
        bool    NeedsSecondPass(GRendererD3DxImpl *prenderer) const
        {
            if (prenderer->UsePixelShaders)
                return 0;
            if (Mode != FM_Bitmap && Mode != FM_Gouraud)
                return 0;

            // Second pass must be enabled.
            if (!PixelShaderInitTable[prenderer->PShaderIndex-1].pPass1Desc)
                return 0;
            
            return HasNonzeroAdditiveCxform;
        }

        // Set D3D state for a necessary second pass.
        void    ApplySecondPass(GRendererD3DxImpl* prenderer) const
        {
            GASSERT(NeedsSecondPass(prenderer));
            GASSERT(!prenderer->UsePixelShaders);

            // Additive color. Already pre-multiplied by 255
            D3DCOLOR color = D3DCOLOR_ARGB(
                    (UByte)BitmapColorTransform.M_[3][1],
                    (UByte)BitmapColorTransform.M_[0][1],
                    (UByte)BitmapColorTransform.M_[1][1],
                    (UByte)BitmapColorTransform.M_[2][1] );

            prenderer->pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);

            // Second pass shader.
            prenderer->SetPixelShader(prenderer->PShaderIndex, 1);

            // Color add, but multiply added color by alpha to cull non-blended pixels.
            prenderer->pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            prenderer->pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
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
    GTextureD3Dx*   CreateTexture()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GTextureD3DxImpl(this);
    }

    
    // Helper function to query renderer capabilities.
    bool        GetRenderCaps(RenderCaps *pcaps)
    {
        pcaps->CapBits      = Cap_Index16 | Cap_FillGouraud | Cap_CanLoseData;
        pcaps->BlendModes   = (1<<Blend_None) | (1<<Blend_Normal) |
                              (1<<Blend_Multiply) | (1<<Blend_Lighten) | (1<<Blend_Darken) |
                              (1<<Blend_Add) | (1<<Blend_Subtract);
        pcaps->VertexFormats= (1<<Vertex_None) | (1<<Vertex_XY16i) | (1<<Vertex_XY16iC32) | (1<<Vertex_XY16iCF32);

        if (!ModeSet)
        {
            GFC_DEBUG_WARNING(1, "GRendererD3Dx::GetRenderCaps fails - video mode not set");
            return 0;
        }

        D3DCAPSx caps;     
        pDevice->GetDeviceCaps(&caps);
        // Not supported with software indexing (does not matter for GFxPlayer so far). 
        //if (caps.MaxVertexIndex > 0xFFFF)
        //    pcaps->CapBits |= Cap_Index32;
        
        if (UsePixelShaders)
        {
            GASSERT((caps.PixelShaderVersion & 0xFFFF) >= 0x0101);
            pcaps->CapBits |= Cap_CxformAdd;
            pcaps->CapBits |= Cap_FillGouraudTex;
        }
      
        if (caps.StencilCaps & (D3DSTENCILCAPS_INCR|D3DSTENCILCAPS_INCRSAT))
        {
            // Need to also consider current back-buffer surface format?
            pcaps->CapBits |= Cap_NestedMasks;
        }

        pcaps->MaxTextureSize = GTL::gmin(caps.MaxTextureWidth, caps.MaxTextureWidth);        
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
            GFC_DEBUG_WARNING(1, "GRendererD3Dx::BeginDisplay failed - video mode not set");
            return;
        }

        RenderMode = (vpin.Flags & GViewport::View_AlphaComposite);

        DisplayWidth = fabsf(x1 - x0);
        DisplayHeight = fabsf(y1 - y0);

        // Need to get surface size, so that we can clamp viewport
        D3DSURFACE_DESC surfaceDesc;
        surfaceDesc.Width = vpin.Width;
        surfaceDesc.Height= vpin.Height;
        {
            GPtr<IDirect3DSurfaceX> psurface;

#if (GFC_D3D_VERSION == 9)
            pDevice->GetRenderTarget(0, &psurface.GetRawRef());

#elif (GFC_D3D_VERSION == 8)
            pDevice->GetRenderTarget(&psurface.GetRawRef());
#endif
            if (psurface)
                psurface->GetDesc(&surfaceDesc);
        }

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
        D3DVIEWPORTx    vp;

        vp.MinZ     = 0.0f;
        vp.MaxZ     = 0.0f;
    
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

            vp.X        = viewport.ScissorLeft;
            vp.Y        = viewport.ScissorTop;
            vp.Width    = viewport.ScissorWidth;
            vp.Height   = viewport.ScissorHeight;

            clipX0 = clipX0 + dx / viewport.Width * (viewport.ScissorLeft - viewport.Left);
            clipY0 = clipY0 + dy / viewport.Height * (viewport.ScissorTop - viewport.Top);
            dx = dx / viewport.Width * viewport.ScissorWidth;
            dy = dy / viewport.Height * viewport.ScissorHeight;
        }
        else
        {
            vp.X        = viewport.Left;
            vp.Y        = viewport.Top;
            vp.Width    = viewport.Width;
            vp.Height   = viewport.Height;
        }
        pDevice->SetViewport(&vp);

        ViewportMatrix.SetIdentity();
        ViewportMatrix.M_[0][0] = 2.0f  / dx;
        ViewportMatrix.M_[1][1] = -2.0f / dy;

        // Adjust by -0.5 pixel to match DirectX pixel coverage rules.
        Float xhalfPixelAdjust = (vp.Width > 0) ? (1.0f / (Float) vp.Width) : 0.0f;
        Float yhalfPixelAdjust = (vp.Height> 0) ? (1.0f / (Float) vp.Height) : 0.0f;

        // MA: it does not seem necessary to subtract viewport.
        // Need to work through details of viewport cutting, there still seems to
        // be a 1-pixel issue at edges. Could that be due to edge fill rules ?
        ViewportMatrix.M_[0][2] = -1.0f - ViewportMatrix.M_[0][0] * (clipX0) - xhalfPixelAdjust; 
        ViewportMatrix.M_[1][2] = 1.0f  - ViewportMatrix.M_[1][1] * (clipY0) + yhalfPixelAdjust;

        ViewportMatrix *= UserMatrix;

        // Blending render states.
        BlendModeStack.clear();
        BlendModeStack.reserve(16);
        BlendMode = Blend_None;
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        ApplyBlendMode(BlendMode);      

        // Not necessary of not alpha testing:
        //pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
        //pDevice->SetRenderState(D3DRS_ALPHAREF, 0x00);
        pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);  // Important!

        union 
        {
            float fbias;
            DWORD d;
        } bias;
        bias.fbias = -0.5f;

        // Must reset both stages since ApplySampleMode modifies both.
        SampleMode[0]       = Sample_Linear;      
        SampleMode[1]       = Sample_Linear;      

#if (GFC_D3D_VERSION == 9)
        pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);

        pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, bias.d );

        pDevice->SetSamplerState(0, D3DSAMP_ELEMENTINDEX, 0);
        pDevice->SetSamplerState(1, D3DSAMP_ELEMENTINDEX, 0);

#elif (GFC_D3D_VERSION == 8)
        pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetTextureStageState(0, D3DTSS_MIPMAPLODBIAS, bias.d );
#endif

        // Textures off by default.
        pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);

        // No ZWRITE by default
        pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        // Turn off D3D lighting, since we are providing our own vertex colors
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

        // Clear more states..
        pDevice->SetRenderState(D3DRS_FOGENABLE,        FALSE );
        pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE,  0);
        pDevice->SetRenderState(D3DRS_SPECULARENABLE,   FALSE);
    //  pDevice->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
    //  pDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);

        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
            D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

        // Need to clear textures to avoid potential warnings.
        pDevice->SetTexture(0, 0);
        pDevice->SetTexture(1, 0);
        pDevice->SetTexture(2, 0); // Can't set texture
        // Texture stage clears not necessary, as SetPixelShader does so.
        // pDevice->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
        // pDevice->SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
        // Coord index 0 -> 0, 1 -> 1
        pDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX,  0);
        pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,  D3DTTFF_DISABLE);
        pDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX,  1);
        pDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS,  D3DTTFF_DISABLE);


        // Turn of back-face culling.
        pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

        // Start the scene
        if (!(VMCFlags&VMConfig_NoSceneCalls))
            pDevice->BeginScene();

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

        // Init test for depth-stencil presence.
        StencilChecked  = 0;
        StencilAvailable= 0;
        MultiBitStencil = 0;
        DepthBufferAvailable = 0;

        StencilCounter = 0;
        DrawingMask = 0;

        // Clear the background, if background color has alpha > 0.
        if (backgroundColor.GetAlpha() > 0)
        {
            // Draw a big quad.
            ApplyColor(backgroundColor);
            SetMatrix(Matrix::Identity);
            ApplyMatrix(CurrentMatrix);

            pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, (backgroundColor.GetAlpha()== 0xFF) ? FALSE : TRUE);

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
                pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, VertexStream.GetStartIndex(), 2);
            } 
        }
    }

    
    // Clean up after rendering a frame.  Client program is still
    // responsible for calling Present() or glSwapBuffers()
    void    EndDisplay()
    {
        // End Scene
        if (pDevice)
        {
            SetPixelShader(PS_None);
#if (GFC_D3D_VERSION == 9)
            pDevice->SetVertexShader(0);
#elif (GFC_D3D_VERSION == 8)
            pDevice->SetVertexShader(D3DFVF_XYZ);
#endif
            pDevice->SetTexture(0,0);

            if (!(VMCFlags&VMConfig_NoSceneCalls))
                pDevice->EndScene();

            GFC_DEBUG_WARNING(StencilCounter > 0, "GRenderer::EndDisplay - BeginSubmitMask/DisableSubmitMask mismatch");
        }
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
    

    // Structure describing color combines applied for a given blend mode.
    struct BlendModeDesc
    {
        D3DBLENDOP  BlendOp;
        D3DBLEND    SrcArg, DestArg;
    };
    
    struct BlendModeDescAlpha
    {
        D3DBLENDOP  BlendOp;
        D3DBLEND    SrcArg, DestArg;
        D3DBLEND    SrcAlphaArg, DestAlphaArg;
    };

    void    ApplyBlendMode(BlendType mode, bool premultiplyFill = 0)
    {
        GUNUSED(premultiplyFill);

        static BlendModeDesc modes[15] =
        {
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // None
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Normal
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Layer

            { D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO },          // Multiply
            // (For multiply, should src be pre-multiplied by its inverse alpha?)
            
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Screen *??

            { D3DBLENDOP_MAX, D3DBLEND_SRCALPHA, D3DBLEND_ONE },            // Lighten
            { D3DBLENDOP_MIN, D3DBLEND_SRCALPHA, D3DBLEND_ONE },                    // Darken

            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Difference *??

            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_ONE },            // Add
            { D3DBLENDOP_REVSUBTRACT, D3DBLEND_SRCALPHA, D3DBLEND_ONE },    // Subtract

            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Invert *??

            { D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE },                // Alpha *??
            { D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ONE },                // Erase *??  What color do we erase to?
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Overlay *??
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // HardLight *??
        };

        static BlendModeDescAlpha acmodes[15] =
        {
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // None
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Normal
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Layer

            { D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO },      // Multiply
            // (For multiply, should src be pre-multiplied by its inverse alpha?)

            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Screen *??

            { D3DBLENDOP_MAX, D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_ONE },          // Lighten
            { D3DBLENDOP_MIN, D3DBLEND_SRCALPHA, D3DBLEND_ONE, D3DBLEND_SRCALPHA, D3DBLEND_ONE },          // Darken

            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Difference *??

            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLEND_ONE },                   // Add
            { D3DBLENDOP_REVSUBTRACT, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_ZERO, D3DBLEND_ONE },           // Subtract

            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Invert *??

            { D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_ONE               },    // Alpha *??
            { D3DBLENDOP_ADD, D3DBLEND_ZERO, D3DBLEND_ZERO, D3DBLEND_ONE, D3DBLEND_ONE               },    // Erase *??
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Overlay *??
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Hardlight *??
        };

        if (!pDevice)
            return;

        // For debug build
        GASSERT(((UInt) mode) < 15);
        // For release
        if (((UInt) mode) >= 15)
            mode = Blend_None;

#if (GFC_D3D_VERSION == 9)
        if (UseAcBlend)
        {
            if (RenderMode & GViewport::View_AlphaComposite)
            {
                pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
                pDevice->SetRenderState(D3DRS_BLENDOP, acmodes[mode].BlendOp);
                pDevice->SetRenderState(D3DRS_BLENDOPALPHA, acmodes[mode].BlendOp);
                pDevice->SetRenderState(D3DRS_SRCBLEND, acmodes[mode].SrcArg);        
                pDevice->SetRenderState(D3DRS_DESTBLEND, acmodes[mode].DestArg);
                pDevice->SetRenderState(D3DRS_SRCBLENDALPHA, acmodes[mode].SrcAlphaArg);
                pDevice->SetRenderState(D3DRS_DESTBLENDALPHA, acmodes[mode].DestAlphaArg);
            }
            else
            {
                pDevice->SetRenderState(D3DRS_BLENDOP, modes[mode].BlendOp);
                pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
                pDevice->SetRenderState(D3DRS_SRCBLEND, modes[mode].SrcArg);        
                pDevice->SetRenderState(D3DRS_DESTBLEND, modes[mode].DestArg);
            }
        }
        else
#endif
        {
            if (RenderMode & GViewport::View_AlphaComposite)
            {
                pDevice->SetRenderState(D3DRS_BLENDOP, acmodes[mode].BlendOp);
                pDevice->SetRenderState(D3DRS_SRCBLEND, acmodes[mode].SrcArg);        
                pDevice->SetRenderState(D3DRS_DESTBLEND, acmodes[mode].DestArg);

                if (!DrawingMask)
                {
                    if (mode == Blend_Add)
                        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
                        D3DCOLORWRITEENABLE_BLUE);
                    else
                        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
                        D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
                }
            }
            else
            {
                pDevice->SetRenderState(D3DRS_BLENDOP, modes[mode].BlendOp);
                pDevice->SetRenderState(D3DRS_SRCBLEND, modes[mode].SrcArg);        
                pDevice->SetRenderState(D3DRS_DESTBLEND, modes[mode].DestArg);
            }
        }
    }


    void ApplySampleMode(UInt stage, BitmapSampleMode mode)
    {
        if (SampleMode[stage] != mode)
        {
            SampleMode[stage] = mode;
            _D3DTEXTUREFILTERTYPE filter =
                    (mode == Sample_Point) ? D3DTEXF_POINT : D3DTEXF_LINEAR;            

#if (GFC_D3D_VERSION == 9)
            pDevice->SetSamplerState(stage, D3DSAMP_MINFILTER, filter);
            pDevice->SetSamplerState(stage, D3DSAMP_MAGFILTER, filter);

#elif (GFC_D3D_VERSION == 8)
            pDevice->SetTextureStageState(stage, D3DTSS_MINFILTER, filter);
            pDevice->SetTextureStageState(stage, D3DTSS_MAGFILTER, filter);
#endif
        }       
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
        GFC_DEBUG_WARNING(1, "GRendererD3Dx::PopBlendMode - blend mode stack is empty");
    }
    

    // multiply current GRenderer::Matrix with d3d GRenderer::Matrix
    void    ApplyMatrix(const Matrix& matIn)
    {
        Matrix  m(ViewportMatrix);
        m *= matIn;

        float   mrows[4][4];

        mrows[0][0] = m.M_[0][0];
        mrows[0][1] = m.M_[0][1];
        mrows[0][2] = 0;
        mrows[0][3] = m.M_[0][2];

        mrows[1][0] = m.M_[1][0];
        mrows[1][1] = m.M_[1][1];
        mrows[1][2] = 0;
        mrows[1][3] = m.M_[1][2];

        mrows[2][0] = 0;
        mrows[2][1] = 0;
        mrows[2][2] = 1.0f;
        mrows[2][3] = 0;

        mrows[3][0] = 0;
        mrows[3][1] = 0;
        mrows[3][2] = 0;
        mrows[3][3] = 1.0f;

        pDevice->SetVertexShaderConstant(0, &mrows[0][0], 4);
    }

    
    // Set the given color. 
    void    ApplyColor(GColor c)
    {           
        if (UsePixelShaders)
        {
            const float mult = 1.0f / 255.0f;
            const float alpha = c.GetAlpha() * mult;
            // Set color.
            float rgba[4] = 
            {
                c.GetRed() * mult,  c.GetGreen() * mult,
                c.GetBlue() * mult, alpha
            };      

            if ((BlendMode == Blend_Multiply) ||
                (BlendMode == Blend_Darken))
            {
                rgba[0] = gflerp(1.0f, rgba[0], alpha);
                rgba[1] = gflerp(1.0f, rgba[1], alpha);
                rgba[2] = gflerp(1.0f, rgba[2], alpha);
            }

            pDevice->SetPixelShaderConstant(0, rgba, 1);
        }
        else
        {
            // With no shaders, color goes into a texture factor constant.
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, c.Raw);
        }
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
            (VertexStream.GetD3DIndexFormat() == D3DFMT_UNKNOWN))
        {
            GFC_DEBUG_WARNING(!VertexStream.HasVertexData(), "GRendererD3Dx::DrawIndexedTriList failed, vertex data not specified");
            GFC_DEBUG_WARNING(!VertexStream.HasIndexData(), "GRendererD3Dx::DrawIndexedTriList failed, index data not specified");
            GFC_DEBUG_WARNING((VertexStream.GetD3DIndexFormat() == D3DFMT_UNKNOWN),
                              "GRendererD3Dx::DrawIndexedTriList failed, index buffer format not specified");
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

        // Upload data to buffers and draw.
        GDynamicVertexStream::PrimitiveDesc prim(baseVertexIndex, minVertexIndex, numVertices, startIndex, triangleCount);
        if (!VertexStream.PrepareVertexData(prim))
            return;

        VertexStream.DrawTriangles();
        RenderStats.Triangles += triangleCount;
        RenderStats.Primitives++;
    
        // Second pass might be necessary for some effects on older HW.
        if (CurrentStyles[FILL_STYLE].NeedsSecondPass(this))
        {
            CurrentStyles[FILL_STYLE].ApplySecondPass(this);

            VertexStream.DrawTriangles();            
            RenderStats.Triangles += triangleCount;
            RenderStats.Primitives++;
            //CurrentStyles[FILL_STYLE].CleanupSecondPass(this);
        }
    }


    // Draw the line strip formed by the sequence of points.
    void    DrawLineStrip(int baseVertexIndex, int lineCount)
    {
        if (!ModeSet)
            return;
        if (!VertexStream.HasVertexData()) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(1, "GRendererD3Dx::DrawLineStrip failed, vertex data not specified");
            return;
        }

        // MA: Should loosen some rules to allow for other decl/shader combinations?
        GASSERT(VertexStream.GetVertexFormat() == Vertex_XY16i);
        SetVertexDecl(VD_Strip);
        SetVertexShader(VS_Strip);

        // Set up current style.
        CurrentStyles[LINE_STYLE].Apply(this);

        ApplyMatrix(CurrentMatrix);

        if (!VertexStream.InitVertexBufferData(baseVertexIndex, lineCount + 1))
            return;
        pDevice->DrawPrimitive(D3DPT_LINESTRIP, VertexStream.GetStartIndex(), lineCount);

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

        GTextureD3DxImpl* ptexture = (GTextureD3DxImpl*)pti;
        // Texture must be valid for rendering
        if (!ptexture->pD3DTexture && !ptexture->CallRecreate())
        {
            GFC_DEBUG_WARNING(1, "GRendererD3Dx::DrawBitmaps failed, empty texture specified/could not recreate texture");
            return;     
        }

        // Test cache buffer management support.
        // Optimized implementation could use this spot to set vertex buffer and/or initialize offset in it.
        // Note that since bitmap lists are usually short, it would be a good idea to combine many of them
        // into one buffer, instead of creating individual buffers for each pbitmapList data instance.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_BitmapList, (pbitmapList!=0),
                                    listSize, (((pbitmapList!=0)&&listSize) ? ((SInt)pbitmapList->Coords.Left) : 0) );


        if (UsePixelShaders)
            ApplyPShaderCxform(CurrentCxform, 2);

        // Complex
        ApplyBlendMode(BlendMode);

        ApplySampleMode(0, Sample_Linear);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  TRUE);
        
        // Set texture.
        SetPixelShader(PS_TextTexture);
        pDevice->SetTexture(0, ptexture->pD3DTexture);

#if (GFC_D3D_VERSION == 9)
        pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

#elif (GFC_D3D_VERSION == 8)
        pDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
        pDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
#endif

        // Switch to non-texgen shader.
        SetVertexDecl(VD_Glyph);
        SetVertexShader(VS_Glyph);

        ApplyMatrix(m);


        int ibitmap = 0, ivertex = 0;
        GCOMPILER_ASSERT((GDynamicVertexStream::GlyphBufferVertexCount%6) == 0);
        

        while (ibitmap < count)
        {
            // Lock the vertex buffer for glyphs.            
            int    vertexCount = GTL::gmin<int>((count-ibitmap) * 6, GDynamicVertexStream::GlyphBufferVertexCount);
            void *  pbuffer     = VertexStream.LockVertexBuffer(vertexCount, sizeof(GGlyphVertex));
            if (!pbuffer)
                return;

            if (UsePixelShaders)
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
            else
                for(ivertex = 0; (ivertex < vertexCount) && (ibitmap<count); ibitmap++, ivertex+= 6)
                {
                    BitmapDesc &  bd = pbitmapList[ibitmap + startIndex];
                    GGlyphVertex* pv = ((GGlyphVertex*)pbuffer) + ivertex;                
                    GColor Color = CurrentCxform.Transform(bd.Color);

                    // Triangle 1.
                    pv[0].SetVertex2D(bd.Coords.Left, bd.Coords.Top,    bd.TextureCoords.Left, bd.TextureCoords.Top, Color);
                    pv[1].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, Color);
                    pv[2].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, Color);
                    // Triangle 2.
                    pv[3].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, Color);
                    pv[4].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, Color);
                    pv[5].SetVertex2D(bd.Coords.Right, bd.Coords.Bottom,bd.TextureCoords.Right, bd.TextureCoords.Bottom, Color);
                }

            VertexStream.UnlockVertexBuffer();     

            // Draw the generated triangles.
            if (ivertex)
            {
                pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, VertexStream.GetStartIndex(), ivertex / 3);
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

        if (!StencilAvailable && !DepthBufferAvailable)
        {
            if (!StencilChecked)
            {
                // Test for depth-stencil presence.
                IDirect3DSurfaceX *pdepthStencilSurface = 0;
                pDevice->GetDepthStencilSurface(&pdepthStencilSurface);
                if (pdepthStencilSurface)
                {
                    D3DSURFACE_DESC sd;
                    pdepthStencilSurface->GetDesc(&sd);

                    switch(sd.Format)
                    {                    
                    case D3DFMT_D24S8:
                    case D3DFMT_D24X4S4:
#if (GFC_D3D_VERSION == 9)
                    case D3DFMT_D24FS8:
#endif
                        MultiBitStencil = 1;
                    case D3DFMT_D15S1:
                        StencilAvailable = 1;
                        break;
                    }

                    pdepthStencilSurface->Release();
                    pdepthStencilSurface = 0;
                    DepthBufferAvailable = 1;
                }
                else
                    StencilAvailable = 0;

                StencilChecked = 1;
            }
            
            if (!StencilAvailable && !DepthBufferAvailable)
            {
#ifdef GFC_BUILD_DEBUG
                static bool StencilWarned = 0;
                if (!StencilWarned)
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3Dx::BeginSubmitMask used, but stencil is not available");
                    StencilWarned = 1;
                }
#endif
                return;
            }
        }

        if (StencilAvailable)
        {
            pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);            
            pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);

            pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
            pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);

            // Check that we have appropriate stencil functionality for nested masks.
            bool canIncDec = (MultiBitStencil && (StencilOpInc != D3DSTENCILOP_REPLACE));

            switch(maskMode)
            {
            case Mask_Clear:
                pDevice->Clear(0, 0, D3DCLEAR_STENCIL, 0, 0.0f, 0);
                pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
                pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
                pDevice->SetRenderState(D3DRS_STENCILREF, 1);
                StencilCounter = 1;
                break;

            case Mask_Increment:
                 // Increment only if we support it.
                if (canIncDec)
                {
                    pDevice->SetRenderState(D3DRS_STENCILREF, StencilCounter);
                    pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
                    pDevice->SetRenderState(D3DRS_STENCILPASS, StencilOpInc);
                    StencilCounter++;
                }
                else
                {   // If not supported, no change.
                    pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
                }                
                break;

            case Mask_Decrement:                
                if (canIncDec)
                {
                    pDevice->SetRenderState(D3DRS_STENCILREF, StencilCounter);
                    pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
                    pDevice->SetRenderState(D3DRS_STENCILPASS, StencilOpDec);
                    StencilCounter--;
                }
                else
                {
                    pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
                }                
                break;
            }

        }
        else if (DepthBufferAvailable)
        {
            // Clear the Z-buffer
            if (maskMode == Mask_Clear)
            {
                pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
                
                // Set the correct render states in order to not modify the color buffer
                // but write the default Z-value everywhere. According to the shader code: should be 0.
                pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);
                pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
                pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
                pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
            }
            else
            {
                // No color write. Incr/Decr not supported.
                pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
                pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
                pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            }
        }
    }


    // Defines all D3Dx color channels for D3DRS_COLORWRITEENABLE
    #define D3DCOLORWRITEENABLE_ALL D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | \
                                    D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN
    
    void EndSubmitMask()
    {
        if (!ModeSet)
            return;

        DrawingMask = 0;
        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);

        if (StencilAvailable)
        {

            // We draw only where the (stencil == StencilCounter), i.e. where the latest mask was drawn.
            // However, we don't change the stencil buffer
            pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
            pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP); 
            // Stencil counter.
            pDevice->SetRenderState(D3DRS_STENCILREF, StencilCounter);
            pDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);

        }
        else if (DepthBufferAvailable)
        {
            // Disable the Z-write and write only where the mask had written
            pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
        }
    }
    
    void DisableMask()
    {
        if (!ModeSet)
            return;

        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);

        if (StencilAvailable)
        {            
            StencilCounter = 0;
            pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }
        else if (DepthBufferAvailable)
        {
            // Disable the Z-write and write only where the mask had written
            pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
        }
    }


    
    virtual void    GetRenderStats(Stats *pstats, bool resetStats)
    {
        if (pstats)
            memcpy(pstats, &RenderStats, sizeof(Stats));
        if (resetStats)
            RenderStats.Clear();
    }


        
    // GRendererD3Dx interface implementation
    
    virtual bool    SetDependentVideoMode(
                                LPDIRECT3DDEVICEx pd3dDevice,
                                D3DPRESENT_PARAMETERS* ppresentParams,
                                UInt32 vmConfigFlags,
                                HWND hwnd)
    {
        if (!pd3dDevice || !ppresentParams)     
            return 0;

        // TODO: Need to check device caps ?        
        pDevice = pd3dDevice;
        pDevice->AddRef();

        // Detect if we can use shaders.
        D3DCAPSx caps;
        pDevice->GetDeviceCaps(&caps);     

        if ((caps.PixelShaderVersion & 0xFFFF) >= GRENDERER_SHADER_VERSION)        
            UsePixelShaders = 1;

        // Detect stencil op.
        if (caps.StencilCaps & D3DSTENCILCAPS_INCR)
        {
            StencilOpInc = D3DSTENCILOP_INCR;
            StencilOpDec = D3DSTENCILOP_DECR;
        }
        else if (caps.StencilCaps & D3DSTENCILCAPS_INCRSAT)
        {
            StencilOpInc = D3DSTENCILOP_INCRSAT;
            StencilOpDec = D3DSTENCILOP_DECRSAT;
        }
        else
        {   // Stencil ops not available.
            StencilOpInc = D3DSTENCILOP_REPLACE;
            StencilOpDec = D3DSTENCILOP_REPLACE;
        }

#if (GFC_D3D_VERSION == 9)
        UseAcBlend = (caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) != 0;
#else
        UseAcBlend = 0;
#endif

        if (!InitShaders(&caps) || !VertexStream.Initialize(pd3dDevice))
        {
            ReleaseShaders();
            pDevice->Release();
            pDevice = 0;
            return 0;
        }

        VMCFlags = vmConfigFlags;

        if ((caps.Caps2 & D3DCAPS2_DYNAMICTEXTURES) && (VMCFlags & VMConfig_UseDynamicTex))
            UseDynamicTex = 1;
        else
            UseDynamicTex = 0;

        AlphaTextureFormat  = D3DFMT_A8;

        memcpy(&PresentParams, ppresentParams, sizeof(D3DPRESENT_PARAMETERS));
        hWnd = hwnd;
                    
        ModeSet = 1;
        return 1;
    }
                                        
    // Returns back to original mode (cleanup)
    virtual bool    ResetVideoMode()
    {
        if (!ModeSet)
            return 1;

        // Release texture resources.
        {
            GLock::Locker guard(&TexturesLock);
            
            GTextureD3DxImpl* ptexture = (GTextureD3DxImpl*)Textures.pFirst;
            while (ptexture && ptexture != &Textures)
            {
                // Save next pointer, since ptexture->Release() after CallHandlers
                // can potentially destroy the texture if user released in in the handler.                
                GTextureD3DxImpl* pnext = (GTextureD3DxImpl*)ptexture->pNext;

                // Save texture in case CallHandlers clear them out.
                if (ptexture->AddRef_NotZero())
                {
                    if (ptexture->pD3DTexture)
                    {
                        ptexture->pD3DTexture->Release();
                        ptexture->pD3DTexture = 0;
                        ptexture->Width = ptexture->Height = 0;
                        ptexture->CallHandlers(GTexture::ChangeHandler::Event_DataLost);
                    }
                    ptexture->Release();                    
                }

                ptexture = pnext;
            }
        }

        VertexStream.Reset();
        ReleaseShaders();
        pDevice->Release();
        pDevice = 0;
        hWnd    = 0;        

        ModeSet = 0;
        return 1;
    }

    virtual DisplayStatus       CheckDisplayStatus() const
    {
        if (!ModeSet)
            return DisplayStatus_NoModeSet;
        
        HRESULT cooperativeLevel = pDevice->TestCooperativeLevel();
        if (cooperativeLevel == D3D_OK)
            return DisplayStatus_Ok;

        // Ensure that textures are released.
        pDevice->SetTexture(0, 0);
        
        // Release texture resources.
        {   GLock::Locker guard(&TexturesLock);
            
            GTextureD3DxImpl* ptexture = (GTextureD3DxImpl*)Textures.pFirst;
            while (ptexture != &Textures)
            {
                // Save next ahead of time because CallHandles can delete the texture.
                GTextureD3DxImpl* pnext = (GTextureD3DxImpl*)ptexture->pNext;
                if (ptexture->AddRef_NotZero())
                {
                    if (ptexture->pD3DTexture)
                    {
                        ptexture->pD3DTexture->Release();
                        ptexture->pD3DTexture = 0;
                        ptexture->Width = ptexture->Height = 0;
                        ptexture->CallHandlers(GTexture::ChangeHandler::Event_DataLost);
                    }
                    ptexture->Release();
                }
                ptexture = pnext;
            }
        }
        // Release vertex buffers (pass LostDevice flag).
        // TBD: CheckDisplayStatus() probably should not be const.
        const_cast<GDynamicVertexStream&>(VertexStream).ReleaseDynamicBuffers(1);

        if (cooperativeLevel == D3DERR_DEVICENOTRESET)
            return DisplayStatus_NeedsReset;
        
        //else: (cooperativeLevel == D3DERR_DEVICELOST)
        return DisplayStatus_Unavailable;
    }

    
    // Direct3D9 Access
    // Return various Dirext3D related information
    virtual LPDIRECT3Dx         GetDirect3D() const
    {
        LPDIRECT3Dx pd3d = 0;
        if (pDevice)
            pDevice->GetDirect3D(&pd3d);
        return pd3d;
    }
    virtual LPDIRECT3DDEVICEx   GetDirect3DDevice() const
    {
        return pDevice;
    }
    virtual bool                GetDirect3DPresentParameters(D3DPRESENT_PARAMETERS* ppresentParams) const
    {
        if (!ModeSet)
            return 0;
        memcpy(ppresentParams, &PresentParams, sizeof(D3DPRESENT_PARAMETERS));
        return 1;
    }



}; // class GRendererD3DxImpl




// ***** GTextureD3Dx implementation


GTextureD3DxImpl::GTextureD3DxImpl(GRendererD3DxImpl *prenderer)
    : GTextureD3Dx(&prenderer->Textures)
{
    pRenderer   = prenderer;
    Width       = 
    Height      = 0;
    pD3DTexture = 0;    
}

GTextureD3DxImpl::~GTextureD3DxImpl()
{
    if (pD3DTexture)
    {
        GASSERT(pRenderer);
        if (pRenderer)
            pRenderer->AddResourceForReleasing(pD3DTexture);
        else
            // it should never happen
            pD3DTexture->Release();
    }
    if (!pRenderer)
        return;
    GLock::Locker guard(&pRenderer->TexturesLock);
    if (pFirst)
        RemoveNode();
}

// Obtains the renderer that create TextureInfo 
GRenderer*  GTextureD3DxImpl::GetRenderer() const
    { return pRenderer; }
bool        GTextureD3DxImpl::IsDataValid() const
    { return (pD3DTexture != 0);    }


// Remove texture from renderer, notifies renderer destruction
void    GTextureD3DxImpl::RemoveFromRenderer()
{
    // Clear pRenderer so that CallHandlers passes null
    // renderer as an internal argument for Event_RendererReleased.
    pRenderer = 0;

    // Texture could be in the process of being killed on another
    // thread (if its destructor is being called but hasn't had a chance
    // to clean us out from the list); if that is the case we can not
    // call AddRef to revive the object.
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
        if (pNext) // We may have been released by user.
            RemoveNode();
        Release();
    }
    else
    {
        if (pNext) // We may have been released by user.
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

bool GTextureD3DxImpl::InitTexture(IDirect3DTextureX *ptex, SInt width, SInt height)
{
    if (!pRenderer || !pRenderer->pDevice)
        return 0;

    if (pD3DTexture)
    {
        pD3DTexture->Release();
        pD3DTexture = 0;
    }
    if (ptex)
    {
        Width = width;
        Height = height;
        pD3DTexture = ptex;
        ptex->AddRef();
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

// NOTE: This function destroys pim's data in the process of making mipmaps.
bool GTextureD3DxImpl::InitTexture(GImageBase* pim, int targetWidth, int targetHeight)
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
        TextureFormat   = D3DFMT_A8R8G8B8;
    }
    else if (pim->Format == GImage::Image_RGB_888)
    {
        bytesPerPixel   = 3;
        TextureFormat   = D3DFMT_A8R8G8B8;
    }
    else if (pim->Format == GImage::Image_A_8)
    {
        bytesPerPixel   = 1;
        TextureFormat   = pRenderer->AlphaTextureFormat;
    }
    else if (pim->Format == GImage::Image_DXT1)
    {
        TextureFormat = D3DFMT_DXT1;
        bytesPerPixel = 1;
    }
    else if (pim->Format == GImage::Image_DXT3)
    {
        TextureFormat = D3DFMT_DXT3;
        bytesPerPixel = 1;
    }
    else if (pim->Format == GImage::Image_DXT5)
    {
        TextureFormat = D3DFMT_DXT5;
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
        pim->Format == GImage::Image_RGB_888
#if (GFC_D3D_VERSION == 8)
        || pim->Format == GImage::Image_ARGB_8888
#endif
        ))
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

#if (GFC_D3D_VERSION == 8)
                for (UInt y = 0; y < h; y++)
                {
                    UInt32*  pdest    = (UInt32*) presampleImage->GetScanline(y);
                    for (UInt x = 0; x < w; x++)
                        pdest[x] = (pdest[x] & 0xff00ff00) | ((pdest[x] & 0xff) << 16) | ((pdest[x] >> 16) & 0xff);
                }
#endif
            }
            else if (pim->Format == GImage::Image_RGB_888)
            {
                // Need to insert a dummy alpha byte in the image data, for D3DXLoadSurfaceFromMemory.      
                for (UInt y = 0; y < h; y++)
                {
                    UByte*  scanline = pim->GetScanline(y);
                    UByte*  pdest    = presampleImage->GetScanline(y);
                    for (UInt x = 0; x < w; x++)
                    {
#if (GFC_D3D_VERSION == 9)
                        pdest[x * 4 + 0] = scanline[x * 3 + 0]; // red
                        pdest[x * 4 + 1] = scanline[x * 3 + 1]; // green
                        pdest[x * 4 + 2] = scanline[x * 3 + 2]; // blue
#else
                        pdest[x * 4 + 2] = scanline[x * 3 + 0]; // red
                        pdest[x * 4 + 1] = scanline[x * 3 + 1]; // green
                        pdest[x * 4 + 0] = scanline[x * 3 + 2]; // blue
#endif
                        pdest[x * 4 + 3] = 255; // alpha
                    }
                }
            }
#if (GFC_D3D_VERSION == 8)
            else if (pim->Format == GImage::Image_ARGB_8888)
            {
                for (UInt y = 0; y < h; y++)
                {
                    UInt32*  pdest    = (UInt32*) presampleImage->GetScanline(y);
                    UInt32*  psrc    = (UInt32*) pim->GetScanline(y);
                    for (UInt x = 0; x < w; x++)
                        pdest[x] = (psrc[x] & 0xff00ff00) | ((psrc[x] & 0xff) << 16) | ((psrc[x] >> 16) & 0xff);
                }
            }
#endif
            if (pim->MipMapCount > 1)
            {
                GFC_DEBUG_WARNING(1, "GRendererD3Dx - Existing mipmap levels have been skipped due to resampling");
                levelsNeeded = 1;
            }
        }
    }

    // Create the texture.
    // For now, only generate mipmaps for alpha textures.
    // MA: For some reason we need to specify levelsNeeded-1, otherwise
    // surface accesses may crash (when running with Gamebryo).
    // So, 256x256 texture seems to have levelCount of 8 (not 9).
    if (pim->MipMapCount <= 1 && bytesPerPixel != 1)
        levelsNeeded = 1;
    else
        levelsNeeded = GTL::gmax(1u, levelsNeeded - 1);
    //levelsNeeded = GTL::gmax(1u, (bytesPerPixel == 1) ? UInt(levelsNeeded - 1) : 1u);

td3d9_retry_create_texture: 
    HRESULT result = pRenderer->pDevice->CreateTexture(
                        w, h,
                        levelsNeeded, 
                        0, // XBox: D3DUSAGE_BORDERSOURCE_TEXTURE                       
                        TextureFormat,
                        D3DPOOL_DEFAULT,                        
                        &pD3DTexture NULL9);
    if (result != S_OK)
    {
        // If texture format A8 failed, try another format
        // Workaround for some Nvidia drivers
        if (TextureFormat == D3DFMT_A8)
        {
            TextureFormat = pRenderer->AlphaTextureFormat = D3DFMT_A8L8;
            goto td3d9_retry_create_texture;
        }
        else if (TextureFormat == D3DFMT_A8L8)
        {
            TextureFormat = pRenderer->AlphaTextureFormat = D3DFMT_A4R4G4B4; // Geforce4 MX
            goto td3d9_retry_create_texture;
        }

        GFC_DEBUG_ERROR(1, "GTextureD3Dx - Can't create texture");
        Width = Height = 0;
        return 0;
    }

    if (pim->IsDataCompressed() || pim->MipMapCount > 1)
    {
        IDirect3DSurfaceX*  psurface    = NULL; 
        UInt                level;  
        GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pim;
        RECT                sourceRect  = { 0,0, w,h};  
#if (GFC_D3D_VERSION == 9)
        D3DFORMAT           sourceSurfaceFormat = (TextureFormat == D3DFMT_A8R8G8B8) ? D3DFMT_A8B8G8R8 : TextureFormat;

#elif (GFC_D3D_VERSION == 8)
        D3DFORMAT           sourceSurfaceFormat = TextureFormat;
#endif

        for(level = 0; level < levelsNeeded; level++)
        {           
            // Load all levels...
            if (pD3DTexture->GetSurfaceLevel(level, &psurface) != S_OK)
            {
                pD3DTexture->Release();
                pD3DTexture = 0;
                Width = Height = 0;
                return 0;
            }       
            GASSERT(psurface);

            UInt mipW, mipH;
            const UByte* pmipData = psourceImage->GetMipMapLevelData(level, &mipW, &mipH);
            
            sourceRect.right = mipW;
            sourceRect.bottom = mipH;

            result = D3DXLoadSurfaceFromMemory(
                psurface, NULL, NULL,
                pmipData,
                sourceSurfaceFormat, // NOTE: format order conversion from GL
                (pim->IsDataCompressed()) ? 
                    GImageBase::GetMipMapLevelSize(psourceImage->Format, mipW, 1) : 
                    GImageBase::GetPitch(psourceImage->Format, mipW),
                NULL,
                &sourceRect,
                D3DX_DEFAULT, 0);
            psurface->Release();
            psurface = 0;

            if (result != S_OK)
            {
                GFC_DEBUG_ERROR1(1, "GTextureD3Dx - Can't load surface from memory, result = %d", result);
                pD3DTexture->Release();
                pD3DTexture = 0;
                Width = Height = 0;
                return 0;
            }
        }

    }
    else
    {
        // Will need a buffer for destructive mipmap generation
        if ((bytesPerPixel == 1) && (levelsNeeded >1) && !presampleImage)
        {
            GASSERT_ON_RENDERER_MIPMAP_GEN;

            // A bit hacky, needs to be more general
            presampleImage = *GImage::CreateImage(pim->Format, pim->Width, pim->Height);
            GASSERT(pim->Pitch == presampleImage->Pitch);
            memcpy(presampleImage->pData, pim->pData, pim->Height * pim->Pitch);        
        }

        IDirect3DSurfaceX*  psurface    = NULL; 
        UInt                level;  
        GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pim;
        RECT                sourceRect  = { 0,0, w,h};

#if (GFC_D3D_VERSION == 9)
        D3DFORMAT           sourceSurfaceFormat = (bytesPerPixel == 1) ? D3DFMT_A8 : D3DFMT_A8B8G8R8;

#elif (GFC_D3D_VERSION == 8)
        D3DFORMAT           sourceSurfaceFormat = (bytesPerPixel == 1) ? D3DFMT_A8 : D3DFMT_A8R8G8B8;
#endif

        for(level = 0; level<levelsNeeded; level++)
        {           
            // Load all levels...
            if (pD3DTexture->GetSurfaceLevel(level, &psurface) != S_OK)
            {
                pD3DTexture->Release();
                pD3DTexture = 0;
                Width = Height = 0;
                return 0;
            }       
            GASSERT(psurface);
                
            result = D3DXLoadSurfaceFromMemory(
                        psurface, NULL, NULL,
                        psourceImage->pData,
                        sourceSurfaceFormat, // NOTE: format order conversion from GL
                        (bytesPerPixel == 1) ? sourceRect.right : psourceImage->Pitch,
                        NULL,
                        &sourceRect,
                        D3DX_DEFAULT, 0);
            psurface->Release();
            psurface = 0;
                
            if (result != S_OK)
            {
                GFC_DEBUG_ERROR1(1, "GTextureD3Dx - Can't load surface from memory, result = %d", result);
                pD3DTexture->Release();
                pD3DTexture = 0;
                Width = Height = 0;
                return 0;
            }
            
            // For Alpha-only images, we might need to generate the next mipmap level.
            if (level< (levelsNeeded-1))
            {
                if (psourceImage == pim)
                {
                    presampleImage = new GImage(*pim);
                    psourceImage = presampleImage.GetPtr();
                }
                GCOMPILER_ASSERT(sizeof(sourceRect.right) == sizeof(int));
                GRendererD3DxImpl::MakeNextMiplevel((int*) &sourceRect.right, (int*) &sourceRect.bottom,
                                                    psourceImage->pData);
            }
        }
    }    

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool GTextureD3DxImpl::InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                  int targetWidth , int targetHeight)
{
    if (!pRenderer || !pRenderer->pDevice)
        return 0;

    if (pD3DTexture)
        pD3DTexture->Release();

    if (format == GImage::Image_ARGB_8888)
        TextureFormat   = D3DFMT_A8R8G8B8;
    else if (format == GImage::Image_RGB_888)
        TextureFormat   = D3DFMT_A8R8G8B8;
    else if (format == GImage::Image_A_8)
        TextureFormat   = pRenderer->AlphaTextureFormat;
    else 
    { // Unsupported format
        GASSERT(0);
        return 0;
    }

    Width   = (targetWidth == 0)  ? width : targetWidth;
    Height  = (targetHeight == 0) ? height: targetHeight;

td3d9_retry_create_texture: 
    HRESULT result = pRenderer->pDevice->CreateTexture(width, height, mipmaps+1, pRenderer->UseDynamicTex ? D3DUSAGE_DYNAMIC : 0,
        TextureFormat, D3DPOOL_DEFAULT, &pD3DTexture NULL9);
    if (result != S_OK)
    {
        // If texture format A8 failed, try another format
        // Workaround for some Nvidia drivers
        if (TextureFormat == D3DFMT_A8)
        {
            TextureFormat = pRenderer->AlphaTextureFormat = D3DFMT_A8L8;
            goto td3d9_retry_create_texture;
        }
        else if (TextureFormat == D3DFMT_A8L8)
        {
            TextureFormat = pRenderer->AlphaTextureFormat = D3DFMT_A4R4G4B4; // Geforce4 MX
            goto td3d9_retry_create_texture;
        }

        GFC_DEBUG_ERROR(1, "GTextureD3Dx - Can't create texture");
        Width = Height = 0;
        return 0;
    }

    return 1;
}

void GTextureD3DxImpl::Update(int level, int n, const UpdateRect *rects, const GImageBase *pim)
{
    UInt     bytesPerPixel = 0;
    IDirect3DSurfaceX *ptempsurface;

    if (pim->Format == GImage::Image_ARGB_8888)
    {
        bytesPerPixel   = 4;
    }
    else if (pim->Format == GImage::Image_RGB_888)
    {
        bytesPerPixel   = 3;
    }
    else if (pim->Format == GImage::Image_A_8)
    {
        bytesPerPixel   = 1;
    }
    else
        GASSERT(0);

    if (!pD3DTexture && !CallRecreate())
    {
        GFC_DEBUG_WARNING(1, "GTextureD3Dx::Update failed, could not recreate texture");
        return;     
    }

    if (pRenderer->UseDynamicTex)
    {
        for (int i = 0; i < n; i++)
        {
            GRect<int> rect = rects[i].src;
            GPoint<int> dest = rects[i].dest;

            D3DLOCKED_RECT      lr;
            RECT                destr;

            destr.left   = dest.x;
            destr.bottom = dest.y + rect.Height() - 1;
            destr.right  = dest.x + rect.Width() - 1;
            destr.top    = dest.y;

            if (FAILED( pD3DTexture->LockRect(level, &lr, &destr, 0) ))
                return;

            UByte *pdest = (UByte*)lr.pBits;

            for (int j = 0; j < rect.Height(); j++)
                memcpy(pdest + j * lr.Pitch,
                pim->GetScanline(j + rect.Top) + bytesPerPixel * rect.Left,
                rect.Width() * bytesPerPixel);

            pD3DTexture->UnlockRect(level);
        }

        return;
    }
    else if (TextureFormat == D3DFMT_A8)
    {
        if (!pRenderer->pTempTextureA)
        {
            pRenderer->pDevice->CreateTexture(pRenderer->TempW, pRenderer->TempH,
                1, 0, D3DFMT_A8, D3DPOOL_SYSTEMMEM, &pRenderer->pTempTextureA.GetRawRef() NULL9);
            pRenderer->pTempTextureA->GetSurfaceLevel(0, &pRenderer->pTempSurfaceA.GetRawRef());
        }
        ptempsurface = pRenderer->pTempSurfaceA;
    }
    else if (TextureFormat == D3DFMT_A8L8)
    {
        if (!pRenderer->pTempTextureLA)
        {
            pRenderer->pDevice->CreateTexture(pRenderer->TempW, pRenderer->TempH,
                1, 0, D3DFMT_A8L8, D3DPOOL_SYSTEMMEM, &pRenderer->pTempTextureLA.GetRawRef() NULL9);
            pRenderer->pTempTextureLA->GetSurfaceLevel(0, &pRenderer->pTempSurfaceLA.GetRawRef());
        }
        ptempsurface = pRenderer->pTempSurfaceLA;
    }
    else if (TextureFormat == D3DFMT_A4R4G4B4)
    {
        if (!pRenderer->pTempTextureRGBA)
        {
            pRenderer->pDevice->CreateTexture(pRenderer->TempW, pRenderer->TempH,
                1, 0, D3DFMT_A4R4G4B4, D3DPOOL_SYSTEMMEM, &pRenderer->pTempTextureRGBA.GetRawRef() NULL9);
            pRenderer->pTempTextureRGBA->GetSurfaceLevel(0, &pRenderer->pTempSurfaceRGBA.GetRawRef());
        }
        ptempsurface = pRenderer->pTempSurfaceRGBA;
    }
    else
    {
        GASSERT(0);
        return;
    }

    IDirect3DSurfaceX *psurface;

    if (pD3DTexture->GetSurfaceLevel(level, &psurface) != S_OK)
        return;

    for (int i = 0; i < n; i++)
    {
        GRect<int> rect = rects[i].src;
        GPoint<int> dest = rects[i].dest;

        for(int x = rect.Left; x<rect.Right; x+= pRenderer->TempW)
            for(int y = rect.Top; y<rect.Bottom; y+= pRenderer->TempH)
            {           
                GRect<int> sourceRect(x, y, x + pRenderer->TempW, y + pRenderer->TempH);
                if (sourceRect.Right > rect.Right)
                    sourceRect.Right = rect.Right;
                if (sourceRect.Bottom > rect.Bottom)
                    sourceRect.Bottom = rect.Bottom;

                // ** First, copy the section to the system buffer
                D3DLOCKED_RECT      lr;         
                GSize<int>          size = sourceRect.Size();
                //RECT                destr = {dest.x, dest.y, dest.x + size.Width, dest.y + size.Height};

                if (FAILED( ptempsurface->LockRect(&lr, NULL, 0) ))
                {
                    psurface->Release();
                    return;
                }

                switch (TextureFormat)
                {
                case D3DFMT_A8L8:
                    for (int j = 0; j < size.Height; j++)
                        for (int k = 0; k < size.Width; k++)
                        {
                            ((UByte*)lr.pBits)[j * lr.Pitch + k * 2 + 1] = pim->pData[pim->Pitch * (j + sourceRect.Top) + k + sourceRect.Left];
                            ((UByte*)lr.pBits)[j * lr.Pitch + k * 2] = 255;
                        }
                    break;

                case D3DFMT_A4R4G4B4:
                    for (int j = 0; j < size.Height; j++)
                        for (int k = 0; k < size.Width; k++)
                        {
                            ((UByte*)lr.pBits)[j * lr.Pitch + k * 2] = 255;
                            ((UByte*)lr.pBits)[j * lr.Pitch + k * 2 + 1] = (pim->pData[pim->Pitch * (j + sourceRect.Top) + k + sourceRect.Left]) | 0xf;
                        }
                    break;

                case D3DFMT_A8:
                    for (int j = 0; j < size.Height; j++)
                        memcpy(((UByte*)lr.pBits) + j * lr.Pitch,
                            pim->pData + pim->Pitch * (j + sourceRect.Top) + pim->GetBytesPerPixel() * sourceRect.Left,
                            size.Width * pim->GetBytesPerPixel());
                    break;
                }

                ptempsurface->UnlockRect();

                // ** Now, copy the buffer to D3D Texture surface
                RECT    sr = { 0, 0, size.Width, size.Height}; // changed for floats
                POINT   pos= { sourceRect.Left - rect.Left + dest.x, sourceRect.Top - rect.Top + dest.y};

#if (GFC_D3D_VERSION == 9)
                pRenderer->pDevice->UpdateSurface(ptempsurface, &sr, psurface, &pos);

#elif (GFC_D3D_VERSION == 8)
                pRenderer->pDevice->CopyRects(ptempsurface, &sr, 1, psurface, &pos);
#endif
            }
    }

    psurface->Release();
}

bool GTextureD3DxImpl::InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight)
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
    if (!pfilename)
    {
        // Kill texture     
        CallHandlers(ChangeHandler::Event_DataChange);
        return 1;
    }

    // Create the texture.
    HRESULT result;
    result = D3DXCreateTextureFromFileEx(
                pRenderer->pDevice, 
                pfilename, 
                D3DX_DEFAULT, 
                D3DX_DEFAULT, 
                D3DX_DEFAULT, 0, 
                D3DFMT_UNKNOWN, 
                D3DPOOL_DEFAULT, 
                D3DX_DEFAULT, 
                D3DX_DEFAULT, 0, NULL, NULL, 
                &pD3DTexture);

    if (result != S_OK)
    {
        GFC_DEBUG_ERROR1(1, "GTextureD3Dx - Can't create texture from file %s", pfilename);
        Width = Height = 0;
        return 0;
    }

    Width   = targetWidth;
    Height  = targetHeight;

    if ((Width == 0) || (Height==0))
    {
        D3DSURFACE_DESC desc;
        pD3DTexture->GetLevelDesc(0, &desc);
        Width   = desc.Width;
        Height  = desc.Height;
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}


// Factory.
GRendererD3Dx*  GRendererD3Dx::CreateRenderer()
{
    return new GRendererD3DxImpl;
}


