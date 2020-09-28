/**********************************************************************

Filename    :   GRendererD3D9Xbox360.cpp
Content     :   Sample GRenderer implementation for XBox 360
Created     :   February 8, 2006
Authors     :   

Copyright   :   (c) 2001-2005 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GImage.h"
#include "GAtomic.h"
#include "GTLTypes.h"

#if defined(GFC_OS_WIN32)
    #include <windows.h>
#endif

#include "GRendererXbox360.h"
#include "GRendererCommonImpl.h"

#include <string.h> // for memset()


#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#undef new
#endif

#include <d3d9.h>
#include <d3dx9.h>

#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif


// ***** Classes implemented
class GRendererXbox360Impl;
class GTextureXbox360Impl;

// #define GRENDERER_USE_PS20
#ifdef GRENDERER_USE_PS20
#include "ShadersD3D9/asm20.cpp"
#else
#include "ShadersD3D9/hlsl.cpp"
#endif


// ***** GTextureXbox360Impl implementation

// GTextureXbox360Impl declaration
class GTextureXbox360Impl : public GTextureD3D9
{
public:

    // Renderer 
    GRendererXbox360Impl*   pRenderer;
    SInt                    Width, Height;
    
    // D3D9 Texture pointer
    IDirect3DTexture9*  pD3DTexture;
    D3DFORMAT           TextureFormat;


    GTextureXbox360Impl(GRendererXbox360Impl *prenderer);
    ~GTextureXbox360Impl();

    // Obtains the renderer that create TextureInfo 
    virtual GRenderer*  GetRenderer() const;        
    virtual bool        IsDataValid() const;
    
    // Texture loading logic.
    virtual bool        InitTexture(IDirect3DTexture9 *ptex, SInt width, SInt height);
    virtual bool        InitTexture(GImageBase* pim, int targetWidth, int targetHeight);
    virtual bool        InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight); 

    virtual bool        InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps, int targetWidth, int targetHeight);
    virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim);

    // Remove texture from renderer, notifies of renderer destruction
    void                RemoveFromRenderer();

    virtual IDirect3DTexture9*  GetD3DTexture() const { return pD3DTexture; }
};


// ***** Vertex Declarations and Shaders

// Our vertex coords consist of two signed 16-bit integers, for (x,y) position only.
static D3DVERTEXELEMENT9 StripVertexDecl[] =
{
    {0, 0, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    D3DDECL_END()
};

static D3DVERTEXELEMENT9 GlyphVertexDecl[] =
{
    {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    {0, 24, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
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

// Vertex declaration lookup table, must correspond to VDeclType +1
static const D3DVERTEXELEMENT9 *VertexDeclTypeTable[VD_Count - 1] =
{
    StripVertexDecl,
    GlyphVertexDecl,
    VertexDeclXY16iC32,
    VertexDeclXY16iCF32
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
static const char *VertexShaderTextTable[VS_Count - 1] =
{
    pStripVShaderText,
    pGlyphVShaderText,
    pStripVShaderXY16iC32Text,
    pStripVShaderXY16iCF32Text,
    pStripVShaderXY16iCF32_T2Text
};



// Vertex buffer structure used for glyphs.
struct GGlyphVertex
{
    float x,y,z,w;
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
    PS_CountSource = PS_AcCxformTexture,
    PS_AcOffset = PS_AcCxformTexture-PS_CxformTexture
};



struct PixelShaderDesc
{
    // Shader, used if not mull.
    const char*                 pShader;
};

PixelShaderDesc PixelShaderInitTable[PS_Count] =
{   
    // Non-AA Shaders.
    { pSource_PS_SolidColor},
    { pSource_PS_CxformTexture},
    { pSource_PS_CxformTextureMultiply},
    { pSource_PS_TextTextureColor},

    // AA Shaders.
    { pSource_PS_CxformGauraud},
    { pSource_PS_CxformGauraudNoAddAlpha},
    // Texture stage fixed-function fall-backs only (their implementation is incorrect).
    { pSource_PS_CxformGauraudTexture},
    { pSource_PS_Cxform2Texture},
    { pSource_PS_CxformGauraudMultiply},
    { pSource_PS_CxformGauraudMultiplyNoAddAlpha},
    { pSource_PS_CxformGauraudMultiplyTexture},
    { pSource_PS_CxformMultiply2Texture},

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
};




class GRendererXbox360Impl : public GRendererXbox360
{
public:

    // Some renderer state.
    bool                ModeSet;
    SInt                RenderMode;

    // Shaders and declarations that are currently set.
    VDeclType           VDeclIndex;
    VShaderType         VShaderIndex;

    // Current pixel shader index
    int                 PShaderIndex;

    // Video Mode Configuration Flags (VMConfigFlags)
    UInt32              VMCFlags;

    // Created vertex declarations and shaders.
    GPtr<IDirect3DVertexDeclaration9>   VertexDecls[VD_Count];
    GPtr<IDirect3DVertexShader9>        VertexShaders[VS_Count];

    // Allocated pixel shaders
    GPtr<IDirect3DPixelShader9>         PixelShaders[PS_Count];

    
    // Direct3DDevice
    IDirect3DDevice9*   pDevice;
    
    
    // This flag indicates whether we've checked for stencil after BeginDisplay or not.
    bool                StencilChecked;
    // This flag is stencil is available, after check.
    bool                StencilAvailable;
    DWORD               StencilCounter;
    
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

    // Vertex data pointer
    const void*             pVertexData;
    const void*             pIndexData;
    VertexFormat            VertexFmt;
    _D3DFORMAT              IndexFmt;

    // Current sample mode
    BitmapSampleMode        SampleMode;

    // Current blend mode
    BlendType               BlendMode;
    GTL::garray<BlendType>  BlendModeStack;


    // Presentation parameters specified to configure the mode.
    D3DPRESENT_PARAMETERS   PresentParams;
    HWND                    hWnd;

    // Buffer used to pass along glyph vertices
    enum { GlyphVertexBufferSize = 6 * 48 };
    GGlyphVertex            GlyphVertexBuffer[GlyphVertexBufferSize];

    // Linked list used for buffer cache testing, otherwise holds no data.
    CacheNode               CacheList;
    
    GRendererEventHandlerImpl   Handlers;

    // resources to retain during tiling frames
    GTL::garray<GPtr<GTexture> > FrameResources;

    // all D3D call should be done on the main thread, but textures can be released from 
    // different threads so we collect system resources and release them on the main thread
    GTL::garray<IDirect3DTexture9*> ResourceReleasingQueue;
    GLock                           ResourceReleasingQueueLock;

    // this method is called from GTextureXXX destructor to put a system resource into releasing queue
    void AddResourceForReleasing(IDirect3DTexture9* ptexture)
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

    GRendererXbox360Impl()
    {
        ModeSet             = 0;

        VMCFlags            = 0;

        StencilAvailable    = 0;
        StencilChecked      = 0;
        StencilCounter      = 0;        
        
        VDeclIndex          = VD_None;
        VShaderIndex        = VS_None;
        PShaderIndex        = PS_None;      

        SampleMode          = Sample_Linear;
        IndexFmt            = D3DFMT_UNKNOWN;
        VertexFmt           = Vertex_None;

        pDevice = 0;
        hWnd    = 0;

        pVertexData = 0;
        pIndexData  = 0;

        // Glyph buffer z, w components are always 0 and 1.
        UInt i;
        for(i = 0; i < GlyphVertexBufferSize; i++)
        {
            GlyphVertexBuffer[i].z = 0.0f;
            GlyphVertexBuffer[i].w = 1.0f;
        }

        RenderMode = 0;
    }

    ~GRendererXbox360Impl()
    {
        Clear();
    }

    void Clear()
    {
        // Remove/notify all textures
        {
            GLock::Locker guard(&TexturesLock);
            while (Textures.pFirst != &Textures)
                ((GTextureXbox360Impl*)Textures.pFirst)->RemoveFromRenderer();
        }

        CacheList.ReleaseList();

        if (ModeSet)
            ResetVideoMode();

        Handlers.CallHandlers(this, EventHandler::Event_RendererReleased);
        ReleaseQueuedResources();
    }

    void ReleaseResources()
    {
        Clear();
    }

    // Helpers used to create vertex declarations and shaders. 
    bool    CreateVertexDeclaration(GPtr<IDirect3DVertexDeclaration9> *pdeclPtr, const D3DVERTEXELEMENT9 *pvertexElements)
    {
        if (!*pdeclPtr)
        {
            if (pDevice->CreateVertexDeclaration(pvertexElements, &pdeclPtr->GetRawRef()) != S_OK)
            {
                GFC_DEBUG_WARNING(1, "GRendererXbox360 - failed to create vertex declaration");
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
            HRESULT hr;

#ifdef GRENDERER_VSHADER_PROFILE
            hr = D3DXCompileShader(pshaderText, (UInt)strlen(pshaderText),
                NULL, NULL, "main", GRENDERER_VSHADER_PROFILE, D3DXSHADER_DEBUG, &pshader.GetRawRef(), NULL, NULL);
#else
            hr = D3DXAssembleShader(pshaderText, (UInt)strlen(pshaderText),
                NULL, NULL, D3DXSHADER_DEBUG, &pshader.GetRawRef(), NULL);
#endif

            if (FAILED(hr))
                return 0;

            if (!pshader ||
                ((hr = pDevice->CreateVertexShader( (DWORD*)pshader->GetBufferPointer(),
                &pshaderPtr->GetRawRef())) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererXbox360 - Can't create D3D9 vshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }

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
                GFC_DEBUG_WARNING1(1, "GRendererXbox360 - PixelShader errors:\n %s ", pmsg->GetBufferPointer() );
                return 0;
            }

            if (!pshader ||
                ((hr = pDevice->CreatePixelShader(  (DWORD*)pshader->GetBufferPointer(),
                &pshaderPtr->GetRawRef())) != S_OK) )
            {
                GFC_DEBUG_WARNING1(1, "GRendererXbox360 - Can't create D3D9 pshader; error code = %d", hr);
                return 0;
            }
        }
        return 1;
    }


    // Initialize the vshader we use for SWF mesh rendering.
    bool    InitShaders()
    {
        bool success = 1;
        UInt i;

        for (i = 1; i < VD_Count; i++)
            if (!CreateVertexDeclaration(&VertexDecls[i], VertexDeclTypeTable[i-1]))
            {
                success = 0;
                break;
            }

        for (i = 1; i < VS_Count; i++)
            if (!CreateVertexShader(&VertexShaders[i], VertexShaderTextTable[i-1]))
            {
                success = 0;
                break;
            }

        // We only use pixel shaders conditionally if their support is available.       
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
        
        if (!success)
        {
            ReleaseShaders();
            return 0;
        }

        return 1;
    }


    void    ReleaseShaders()
    {
        if (pDevice)
        {
            pDevice->SetVertexShader(0);
            pDevice->SetPixelShader(0);
            pDevice->SetVertexDeclaration(0);
        }

        UInt i;
        for (i=0; i< VD_Count; i++)
            if (VertexDecls[i])
                VertexDecls[i] = 0;

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
            pDevice->SetVertexDeclaration(VertexDecls[vd]);
        }
    }
    void    SetVertexShader(VShaderType vt)
    {
        if (VShaderIndex != vt)
        {
            VShaderIndex = vt;
            pDevice->SetVertexShader(VertexShaders[vt]);
        }
    }


    // Set shader to a device based on a constant
    void    SetPixelShader(PixelShaderType shaderTypein)
    {
        int shaderType = shaderTypein;

        if (RenderMode & GViewport::View_AlphaComposite)
            shaderType += int(PS_AcOffset);

        if (shaderType != PShaderIndex)
        {           
            pDevice->SetPixelShader(PixelShaders[shaderType]);          
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

        int NewW = *width >> 1;
        int NewH = *height >> 1;
        if (NewW < 1) NewW = 1;
        if (NewH < 1) NewH = 1;
        
        if (NewW * 2 != *width   || NewH * 2 != *height)
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
            for (int j = 0; j < NewH; j++) {
                UByte*  out = ((UByte*) data) + j * NewW;
                UByte*  in = ((UByte*) data) + (j << 1) * *width;
                for (int i = 0; i < NewW; i++) {
                    int a;
                    a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
                    *(out) = a >> 2;
                    out++;
                    in += 2;
                }
            }
        }

        // Munge parameters to reflect the shrunken image.
        *width = NewW;
        *height = NewH;
    }


    // Fill helper function:
    // Applies fill texture by setting it to the specified stage, initializing samplers and vertex constants
    void    ApplyFillTexture(const FillTexture &fill, UInt stageIndex, UInt matrixVertexConstIndex)
    {
        GASSERT (fill.pTexture != 0);
        if (fill.pTexture == 0) return; // avoid crash in release build

        GTextureXbox360Impl* ptexture = ((GTextureXbox360Impl*)fill.pTexture);

        if (!ptexture->pD3DTexture)
            ptexture->CallRecreate();

        pDevice->SetTexture(stageIndex, ptexture->pD3DTexture);
        if (VMCFlags & VMConfig_SupportTiling)
            FrameResources.push_back(ptexture);

        // Set sampling
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

        // Set up the bitmap matrix for texgen.
        float   InvWidth = 1.0f / ptexture->Width;
        float   InvHeight = 1.0f / ptexture->Height;

        const Matrix&   m = fill.TextureMatrix;
        Float   p[4] = { 0, 0, 0, 0 };
        p[0] = m.M_[0][0] * InvWidth;
        p[1] = m.M_[0][1] * InvWidth;
        p[3] = m.M_[0][2] * InvWidth;
        pDevice->SetVertexShaderConstantF(matrixVertexConstIndex, p, 1);

        p[0] = m.M_[1][0] * InvHeight;
        p[1] = m.M_[1][1] * InvHeight;
        p[3] = m.M_[1][2] * InvHeight;
        pDevice->SetVertexShaderConstantF(matrixVertexConstIndex + 1, p, 1);
    }


    void    ApplyPShaderCxform(const Cxform &cxform, UInt cxformPShaderConstIndex) const
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

        pDevice->SetPixelShaderConstantF(cxformPShaderConstIndex, cxformData, 2);       
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

        FillTexture             Fill;
        FillTexture             Fill2;


        GFxFillStyle()
        {
            Mode            = FM_None;
            GouraudType     = GFill_Color;
            Fill.pTexture   = 0;
            Fill2.pTexture  = 0;            
        }


        // Push our style into Direct3D
        void    Apply(GRendererXbox360Impl *prenderer) const
        {
            IDirect3DDevice9* pdevice = prenderer->pDevice;
            GASSERT(Mode != FM_None);


            // Complex
            prenderer->ApplyBlendMode(prenderer->BlendMode);

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
                // Gradient and texture fills.
                GASSERT(Fill.pTexture != NULL);

                prenderer->ApplyColor(Color);
                prenderer->ApplySampleMode(Fill.SampleMode);


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


                    if (prenderer->VertexFmt == Vertex_XY16iC32)
                    {
                        // Cxform Alpha Add can not be non-zero is this state because 
                        // cxform blend equations can not work correctly.
                        // If we hit this assert, it means Vertex_XY16iCF32 should have been used.
                        GASSERT (BitmapColorTransform.M_[3][1] < 1.0f);

                        shader = PS_CxformGauraudNoAddAlpha;                        
                    }
                    else
                    {
                        GASSERT(prenderer->VertexFmt != Vertex_XY16i);
                        shader = PS_CxformGauraud;
                    }

                    prenderer->ApplyPShaderCxform(BitmapColorTransform, 2);

                }

                // We have a textured or multi-textured gouraud case.
                else
                {   

                    pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

                    prenderer->ApplyPShaderCxform(BitmapColorTransform, 2);
                    // Set texture to stage 0; matrix to constants 4 and 5
                    prenderer->ApplyFillTexture(Fill, 0, 4);

                    if ((GouraudType == GFill_1TextureColor) ||
                        (GouraudType == GFill_1Texture))
                    {
                        shader = PS_CxformGauraudTexture;
                    }
                    else
                    {
                        shader = PS_Cxform2Texture;

                        // Set second texture to stage 1; matrix to constants 5 and 6
                        prenderer->ApplyFillTexture(Fill2, 1, 6);
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
    GTextureD3D9*   CreateTexture()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GTextureXbox360Impl(this);
    }

    // Helper function to query renderer capabilities.
    bool        GetRenderCaps(RenderCaps *pcaps)
    {
        if (!ModeSet)
        {
            GFC_DEBUG_WARNING(1, "GRendererXBox360::GetRenderCaps fails - video mode not set");
            pcaps->CapBits |= Cap_NoTexOverwrite;
            return 0;
        }

        pcaps->CapBits      = Cap_Index16 | Cap_Index32 | 
                              Cap_FillGouraud |Cap_FillGouraudTex |
                              Cap_CxformAdd | Cap_NestedMasks;

        pcaps->BlendModes   = (1<<Blend_None) | (1<<Blend_Normal) |
                              (1<<Blend_Multiply) | (1<<Blend_Lighten) | (1<<Blend_Darken) |
                              (1<<Blend_Add) | (1<<Blend_Subtract);

        pcaps->VertexFormats= (1<<Vertex_None) | (1<<Vertex_XY16i) |
                              (1<<Vertex_XY16iC32) | (1<<Vertex_XY16iCF32);

        if (VMCFlags & VMConfig_SupportTiling)
            pcaps->CapBits |= Cap_NoTexOverwrite;

        D3DCAPS9 caps9;     
        pDevice->GetDeviceCaps(&caps9);     
        pcaps->MaxTextureSize = GTL::gmin(caps9.MaxTextureWidth, caps9.MaxTextureWidth);
        return 1;
    }

    void EndFrame()
    {
        FrameResources.clear();
        Handlers.CallHandlers(this, EventHandler::Event_EndFrame);
    }
    

    // Set up to render a full frame from a movie and fills the
    // background.  Sets up necessary transforms, to scale the
    // movie to fit within the given dimensions.  Call
    // EndDisplay() when you're done.
    //
    // The Rectangle (ViewportX0, ViewportY0, ViewportX0 +
    // ViewportWidth, ViewportY0 + ViewportHeight) defines the
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
            GFC_DEBUG_WARNING(1, "GRendererD3D9::BeginDisplay failed - video mode not set");
            return;
        }
        
        RenderMode = (vpin.Flags & GViewport::View_AlphaComposite);

        DisplayWidth = fabsf(x1 - x0);
        DisplayHeight = fabsf(y1 - y0);

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
        if (viewportX0_save + viewportW_save > (int) vpin.BufferWidth)
        {
            clipX1 = x0 + ((vpin.BufferWidth - viewportX0_save) * dx) / (float) viewportW_save;
            viewport.Width = vpin.BufferWidth - viewport.Left;
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
        if (viewportY0_save + viewportH_save > (int) vpin.BufferHeight)
        {
            clipY1 = y0 + ((vpin.BufferHeight - viewportY0_save) * dy) / (float) viewportH_save;
            viewport.Height = vpin.BufferHeight - viewport.Top;
        }

        dx  = clipX1 - clipX0;
        dy  = clipY1 - clipY0;

        // Viewport.
        D3DVIEWPORT9    vp;

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
        pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        SampleMode = Sample_Linear;
        pDevice->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, bias.d );

        // No ZWRITE by default
        pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    
        pDevice->SetRenderState(D3DRS_CLIPPLANEENABLE,  0);
        pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);

        // Turn of back-face culling.
        pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        
        pDevice->SetTexture(0, 0);
        pDevice->SetTexture(1, 0); // No texture
        
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

        // Init test for depth-stencil presence.
        StencilChecked  = 0;
        StencilAvailable= 0;
        StencilCounter  = 0;

        // Clear the background, if background color has alpha > 0.
        if (backgroundColor.GetAlpha() > 0)
            {
                // @@ for testing
                /*
                static SInt testColor = 0;
                pDevice->Clear(
                    0,
                    NULL,
                    D3DCLEAR_TARGET | (StencilAvailable ? (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) : 0),
                    testColor += 5,
                    0.0f,
                    0);
                */

                // Draw a big quad.
                ApplyColor(backgroundColor);
                SetMatrix(Matrix::Identity);
                ApplyMatrix(CurrentMatrix);

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

                pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, strip, sizeof(Vertex));
            }
        }

    
    // Clean up after rendering a frame.  Client program is still
    // responsible for calling Present or glSwapBuffers()
    void    EndDisplay()
    {
        // End Scene
        if (pDevice)
        {
            SetPixelShader(PS_None);
            SetVertexShader(VS_None);
            pDevice->SetTexture(0,0);

            if (!(VMCFlags&VMConfig_NoSceneCalls))
                pDevice->EndScene();
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

    void    ApplyBlendMode(BlendType mode)
    {
        static BlendModeDesc modes[15] =
        {
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // None
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Normal
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Layer

            { D3DBLENDOP_ADD, D3DBLEND_DESTCOLOR, D3DBLEND_ZERO },          // Multiply
            // (For multiply, should src be pre-multiplied by its inverse alpha?)
            
            { D3DBLENDOP_ADD, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA },    // Screen *??

            { D3DBLENDOP_MAX, D3DBLEND_SRCALPHA, D3DBLEND_ONE },            // Lighten
            { D3DBLENDOP_MIN, D3DBLEND_SRCALPHA, D3DBLEND_ONE },            // Darken

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

            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Alpha *??
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Erase *??
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Overlay *??
            { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA, D3DBLEND_ONE, D3DBLEND_INVSRCALPHA },    // Hardlight *??
        };

        // For debug build
        GASSERT(((UInt) mode) < 15);
        // For release
        if (((UInt) mode) >= 15)
            mode = Blend_None;

        if (!pDevice)
            return;
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

    void ApplySampleMode(BitmapSampleMode mode)
    {
        if (SampleMode != mode)
        {
            SampleMode = mode;
            _D3DTEXTUREFILTERTYPE filter =
                    (mode == Sample_Point) ? D3DTEXF_POINT : D3DTEXF_LINEAR;            
            pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
            pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);

            pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, filter);
            pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, filter);
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
        GFC_DEBUG_WARNING(1, "GRendererD3D9::PopBlendMode - blend mode stack is empty");
    }
    

    // multiply current GRenderer::Matrix with d3d GRenderer::Matrix
    void    ApplyMatrix(const Matrix& matIn)
    {
        Matrix  m(ViewportMatrix);
        m *= matIn;

        float   row0[4];
        float   row1[4];
        float   row2[4];
        float   row3[4];

        row0[0] = m.M_[0][0];
        row0[1] = m.M_[0][1];
        row0[2] = 0;
        row0[3] = m.M_[0][2];

        row1[0] = m.M_[1][0];
        row1[1] = m.M_[1][1];
        row1[2] = 0;
        row1[3] = m.M_[1][2];

        row2[0] = 0;
        row2[1] = 0;
        row2[2] = 1.0f;
        row2[3] = 0;

        row3[0] = 0;
        row3[1] = 0;
        row3[2] = 0;
        row3[3] = 1.0f;

        pDevice->SetVertexShaderConstantF(0, row0, 1);
        pDevice->SetVertexShaderConstantF(1, row1, 1);

        pDevice->SetVertexShaderConstantF(2, row2, 1);
        pDevice->SetVertexShaderConstantF(3, row3, 1);
    }

    // Set the given color. 
    void    ApplyColor(GColor c)
    {       
        const float mult = 1.0f / 255.0f;
        const float alpha = c.GetAlpha() * mult;
        // Set color.
        float rgba[4] = 
        {
            c.GetRed() * mult,  
            c.GetGreen() * mult,
            c.GetBlue() * mult, alpha
        };      

        if ((BlendMode == Blend_Multiply) ||
            (BlendMode == Blend_Darken))
        {
            rgba[0] = gflerp(1.0f, rgba[0], alpha);
            rgba[1] = gflerp(1.0f, rgba[1], alpha);
            rgba[2] = gflerp(1.0f, rgba[2], alpha);
        }

        pDevice->SetPixelShaderConstantF(0, rgba, 1);
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
        pVertexData = pvertices;
        VertexFmt   = vf;
        // Note: this could populate a static/dynamic buffer instead (if not null).

        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Vertex, (pvertices!=0),
                                    numVertices, (((pvertices!=0)&&numVertices) ? *((SInt16*)pvertices) : 0) );
    }

    void    SetIndexData(const void* pindices, int numIndices, IndexFormat idxf, CacheProvider *pcache)
    {
        pIndexData  = pindices;
        switch(idxf)
        {
            case Index_None:    IndexFmt = D3DFMT_UNKNOWN; break;
            case Index_16:      IndexFmt = D3DFMT_INDEX16; break;
            case Index_32:      IndexFmt = D3DFMT_INDEX32; break;
        }       
        // Note: this could populate a dynamic buffer instead (if not null).

        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Index, (pindices!=0),
                                    numIndices, (((pindices!=0)&&numIndices) ? *((SInt16*)pindices) : 0) );
    }
    
    void    ReleaseCachedData(CachedData *pdata, CachedDataType type)
    {
        // Releases cached data that was allocated from the cache providers.
        CacheList.ReleaseCachedData(pdata, type);
    }


    void    DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices,
                               int startIndex, int triangleCount)
    {
        if (!ModeSet)
            return;
        if (!pVertexData || !pIndexData || (IndexFmt == D3DFMT_UNKNOWN)) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(!pVertexData, "GRendererXBox360::DrawIndexedTriList failed, vertex data not specified");
            GFC_DEBUG_WARNING(!pIndexData, "GRendererXBox360::DrawIndexedTriList failed, index data not specified");
            GFC_DEBUG_WARNING((IndexFmt == D3DFMT_UNKNOWN), "GRendererXBox360::DrawIndexedTriList failed, index buffer format not specified");
            return;
        }

        bool    isGouraud = (CurrentStyles[FILL_STYLE].Mode == GFxFillStyle::FM_Gouraud);
        int     vertexSize= 2 * sizeof(SInt16);

        // Ensure we have the right vertex size. 
        // MA: Should loosen some rules to allow for other decl/shader combinations?
        if (isGouraud)
        {
            if (VertexFmt == Vertex_XY16iC32)
            {
                SetVertexDecl(VD_XY16iC32);
                SetVertexShader(VS_XY16iC32);
                vertexSize = sizeof(VertexXY16iC32);
            }
            else if (VertexFmt == Vertex_XY16iCF32)
            {
                SetVertexDecl(VD_XY16iCF32);

                if (CurrentStyles[FILL_STYLE].GouraudType != GFill_2Texture)
                    SetVertexShader(VS_XY16iCF32);
                else
                    SetVertexShader(VS_XY16iCF32_T2);

                vertexSize = sizeof(VertexXY16iCF32);
            }
        }
        else
        {
            SetVertexDecl(VD_Strip);
            SetVertexShader(VS_Strip);

            GASSERT(VertexFmt == Vertex_XY16i);
            vertexSize= 2 * sizeof(SInt16);
        }

        // Set up current style.
        CurrentStyles[FILL_STYLE].Apply(this);

        ApplyMatrix(CurrentMatrix);

        // TODO - should use a VB instead, and use DrawPrimitive().
        // or at least use a Dynamic buffer for now...
        const int     bytesInIndex = ((IndexFmt == D3DFMT_INDEX16) ? sizeof(UInt16) : sizeof(UInt32));
        const UByte* pindices = (UByte*)pIndexData + startIndex * bytesInIndex;

        // Draw the mesh.
        // XBox360 refuses to render more than 65535 indexes.
        const int maxTriangles = 65535/3;

        RenderStats.Triangles += triangleCount;

        while (triangleCount > maxTriangles)
        {
            pDevice->DrawIndexedPrimitiveUP(
                D3DPT_TRIANGLELIST, minVertexIndex, numVertices, maxTriangles,
                pindices, IndexFmt,
                ((UByte*)pVertexData) + baseVertexIndex * vertexSize,
                vertexSize );

            triangleCount -= maxTriangles;
            pindices      += bytesInIndex * maxTriangles * 3;
            RenderStats.Primitives++;   
        }

        if (triangleCount > 0)
        {
            pDevice->DrawIndexedPrimitiveUP(
                D3DPT_TRIANGLELIST, minVertexIndex, numVertices, triangleCount,
                pindices, IndexFmt,
                ((UByte*)pVertexData) + baseVertexIndex * vertexSize,
                vertexSize );

            RenderStats.Primitives++;
        }        
            
    }


    // Draw the line strip formed by the sequence of points.
    void    DrawLineStrip(int baseVertexIndex, int lineCount)
    {
        if (!ModeSet)
            return;

        if (!pVertexData) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(!pVertexData, "GRendererXBox360::DrawLineStrip failed, vertex data not specified");           
            return;
        }

        // MA: Should loosen some rules to allow for other decl/shader combinations?
        GASSERT(VertexFmt == Vertex_XY16i);
        SetVertexDecl(VD_Strip);
        SetVertexShader(VS_Strip);

        // Set up current style.
        CurrentStyles[LINE_STYLE].Apply(this);

        ApplyMatrix(CurrentMatrix);

        pDevice->DrawPrimitiveUP(
            D3DPT_LINESTRIP, lineCount,         
            ((UByte*)pVertexData) + baseVertexIndex * 2 * sizeof(SInt16), sizeof(SInt16)*2 );

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

        GTextureXbox360Impl* ptexture = (GTextureXbox360Impl*)pti;
        // Texture must be valid for rendering
        if (!ptexture->pD3DTexture && !ptexture->CallRecreate())
        {
            GFC_DEBUG_WARNING(1, "GRendererXBox360::DrawBitmaps failed, empty texture specified/could not recreate texture");
            return;     
        }

        // Test cache buffer management support.
        // Optimized implementation could use this spot to set vertex buffer and/or initialize offset in it.
        // Note that since bitmap lists are usually short, it would be a good idea to combine many of them
        // into one buffer, instead of creating individual buffers for each pbitmapList data instance.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_BitmapList, (pbitmapList!=0),
                                    listSize, (((pbitmapList!=0)&&listSize) ? ((SInt)pbitmapList->Coords.Left) : 0) );

        ApplyPShaderCxform(CurrentCxform, 2);
        ApplyBlendMode(BlendMode);
        ApplySampleMode(Sample_Linear);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  TRUE);

        // Set texture.     
        SetPixelShader(PS_TextTexture);
        pDevice->SetTexture(0, ptexture->pD3DTexture);
        if (VMCFlags & VMConfig_SupportTiling)
            FrameResources.push_back(ptexture);
        
        // Switch to non-texgen shader.
        SetVertexDecl(VD_Glyph);
        SetVertexShader(VS_Glyph);
        ApplyMatrix(m);

        SInt ibitmap = 0, ivertex = 0;
        GCOMPILER_ASSERT((GlyphVertexBufferSize%6) == 0);

        while (ibitmap < count)
        {
            for(ivertex = 0; (ivertex < GlyphVertexBufferSize) && (ibitmap<count); ibitmap++, ivertex+= 6)
            {
                BitmapDesc &  bd = pbitmapList[ibitmap + startIndex];
                GGlyphVertex* pv = GlyphVertexBuffer + ivertex;

                // Triangle 1.
                pv[0].SetVertex2D(bd.Coords.Left, bd.Coords.Top,    bd.TextureCoords.Left, bd.TextureCoords.Top, bd.Color);
                pv[1].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[2].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                // Triangle 2.
                pv[3].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                pv[4].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[5].SetVertex2D(bd.Coords.Right, bd.Coords.Bottom,bd.TextureCoords.Right, bd.TextureCoords.Bottom, bd.Color);
            }

            // Draw the generated triangles.
            if (ivertex)
            {
                pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, ivertex / 3, GlyphVertexBuffer, sizeof(GGlyphVertex));
                RenderStats.Primitives++;
            }
        }
        RenderStats.Triangles += count * 2;
    }


    void BeginSubmitMask(SubmitMaskMode maskMode)
    {
        if (!ModeSet)
            return;

        if (!StencilAvailable)
        {
            if (!StencilChecked)
            {
                // Test for depth-stencil presence.
                IDirect3DSurface9 *pdepthStencilSurface = 0;
                pDevice->GetDepthStencilSurface(&pdepthStencilSurface);
                if (pdepthStencilSurface)
                {
                    D3DSURFACE_DESC sd;
                    pdepthStencilSurface->GetDesc(&sd);

                    switch(sd.Format)
                    { // Xbox360 formats
                    case D3DFMT_D24S8:
                    case D3DFMT_LIN_D24S8:
                    case D3DFMT_D24FS8:
                    case D3DFMT_LIN_D24FS8:
                        StencilAvailable = 1;
                        break;
                    }

                    pdepthStencilSurface->Release();
                    pdepthStencilSurface = 0;
                }
                else
                    StencilAvailable = 0;

                StencilChecked = 1;
            }
            
        if (!StencilAvailable)
            {
#ifdef GFC_BUILD_DEBUG
                static bool StencilWarned = 0;
                if (!StencilWarned)
                {
                    GFC_DEBUG_WARNING(1, "GRendererD3D9::BeginSubmitMask used, but stencil is not available");
                    StencilWarned = 1;
                }
#endif
            return;
            }
        }


        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);            
        pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        pDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);
        pDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
        pDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
       
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
            pDevice->SetRenderState(D3DRS_STENCILREF, StencilCounter);
            pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
            pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
            StencilCounter++;
            break;
        case Mask_Decrement:                           
            pDevice->SetRenderState(D3DRS_STENCILREF, StencilCounter);
            pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
            pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
            StencilCounter--;                       
            break;
        }
    }
    
    void EndSubmitMask()
    {
        if (!ModeSet)
            return;
        if (!StencilAvailable)
            return;

        // Enable frame-buffer writes.        
        pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);

        // We draw only where the (stencil == StencilCounter), i.e. where the latest mask was drawn.
        // However, we don't change the stencil buffer.
        pDevice->SetRenderState(D3DRS_STENCILREF, StencilCounter);
        pDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);
        pDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
        pDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    }
    
    void DisableMask()
    {
        if (!ModeSet)
            return;
        if (!StencilAvailable)
            return;
        
        StencilCounter = 0;
        pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    }

    
    virtual void        GetRenderStats(Stats *pstats, bool resetStats)
    {
        if (pstats)
            memcpy(pstats, &RenderStats, sizeof(Stats));
        if (resetStats)
            RenderStats.Clear();
    }


        
    // GRendererD3D9 interface implementation
    
    virtual bool    SetDependentVideoMode(
                                LPDIRECT3DDEVICE9 pd3dDevice,
                                D3DPRESENT_PARAMETERS* ppresentParams,
                                UInt32 vmConfigFlags,
                                HWND hwnd)
    {
        if (!pd3dDevice || !ppresentParams)     
            return 0;

        // TODO: Need to check device caps ?        
        pDevice = pd3dDevice;
        pDevice->AddRef();

        if (!InitShaders())
        {
            ReleaseShaders();
            pDevice->Release();
            pDevice = 0;
            return 0;
        }

        VMCFlags = vmConfigFlags;

        memcpy(&PresentParams, ppresentParams, sizeof(D3DPRESENT_PARAMETERS));
        hWnd = hwnd;
                    
        ModeSet = 1;
        return 1;
    }
                                        
    // Returns back to original mode (cleanup)
    virtual bool                ResetVideoMode()
    {
        if (!ModeSet)
            return 1;

        pDevice->SetTexture(0, 0);
        pDevice->SetTexture(1, 0);
        
        // Not that we no longer generate a texture DataLost event on 360;
        // since textures can survive the reset.      
        
        ReleaseShaders();
        pDevice->Release();
        pDevice = 0;
        hWnd    = 0;        

        ModeSet = 0;
        return 1;
    }

    virtual DisplayStatus       CheckDisplayStatus() const
    {
        return ModeSet ? DisplayStatus_Ok : DisplayStatus_NoModeSet;
    }

    
    // Direct3D9 Access
    // Return various Dirext3D related information
    virtual LPDIRECT3D9         GetDirect3D() const
    {
        LPDIRECT3D9 pd3d = 0;
        if (pDevice)
            pDevice->GetDirect3D(&pd3d);
        return pd3d;
    }
    virtual LPDIRECT3DDEVICE9   GetDirect3DDevice() const
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

    virtual bool        AddEventHandler(EventHandler *phandler) { return Handlers.AddHandler(phandler); }
    virtual void        RemoveEventHandler(EventHandler *phandler) { Handlers.RemoveHandler(phandler); }


}; // class GRendererXbox360Impl




// ***** GTextureD3D9 implementation


GTextureXbox360Impl::GTextureXbox360Impl(GRendererXbox360Impl *prenderer)
    : GTextureD3D9(&prenderer->Textures)
{
    pRenderer   = prenderer;
    Width       = 
    Height      = 0;
    pD3DTexture = 0;    
}

GTextureXbox360Impl::~GTextureXbox360Impl()
{
    if (pD3DTexture)
    {   
        if (pRenderer && pRenderer->pDevice && pD3DTexture->IsSet(pRenderer->pDevice))
        {
            pRenderer->pDevice->SetTexture(0,0);
            pRenderer->pDevice->SetTexture(1,0);
        }
        GASSERT(pRenderer);
        if (pRenderer)
            // because all D3D operation should be done from the main thread
            // we add this resource to renderer deletion queue
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
GRenderer*  GTextureXbox360Impl::GetRenderer() const
    { return pRenderer; }
bool        GTextureXbox360Impl::IsDataValid() const
    { return (pD3DTexture != 0);    }


// Remove texture from renderer, notifies renderer destruction
void    GTextureXbox360Impl::RemoveFromRenderer()
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
        if (pNext) // We may have been released by user
            RemoveNode();
        Release();
    }
    else
    {
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


bool GTextureXbox360Impl::InitTexture(IDirect3DTexture9 *ptex, SInt width, SInt height)
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
bool GTextureXbox360Impl::InitTexture(GImageBase* pim, int targetWidth, int targetHeight)
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

    switch(pim->Format)
    {
    case GImage::Image_ARGB_8888:
        bytesPerPixel = 4;        
        TextureFormat = D3DFMT_A8R8G8B8;
        break;   
    case GImage::Image_RGB_888:
        bytesPerPixel   = 3;
        TextureFormat   = D3DFMT_DXT1;
        //TextureFormat = D3DFMT_A8R8G8B8;
        break;

    case GImage::Image_A_8:
        bytesPerPixel   = 1;
        TextureFormat   = D3DFMT_A8;
        break;

    case GImage::Image_DXT1:
        TextureFormat = D3DFMT_LIN_DXT1;
        bytesPerPixel = 1;
        break;
    case GImage::Image_DXT3:
        TextureFormat = D3DFMT_LIN_DXT3;
        bytesPerPixel = 1;
        break;
    case GImage::Image_DXT5:
        TextureFormat = D3DFMT_LIN_DXT5;
        bytesPerPixel = 1;
        break;

    default:
        // Unsupported format
        GASSERT(0);
        return 0;
    }


    Width   = (targetWidth == 0)  ? pim->Width : targetWidth;
    Height  = (targetHeight == 0) ? pim->Height: targetHeight;

    // Don't use DXT1 for thin textures (it must be a multiple of 4 for DirectX)
    /*
    if (((TextureFormat== D3DFMT_DXT1) || (TextureFormat== D3DFMT_DXT3)) && 
        ((Width<4) || (Height<4)))
        TextureFormat = D3DFMT_A8R8G8B8;
    */

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
                                     (pim->Format == GImage::Image_RGB_888) ||
                                     (pim->Format == GImage::Image_ARGB_8888)) )  
    {
        // Resample the image to new size
        if (presampleImage = *new GImage(
            ((pim->Format == GImage::Image_RGB_888) ? GImage::Image_ARGB_8888 : pim->Format), w, h))
        {
            GASSERT_ON_RENDERER_RESAMPLING;

            if (w != pim->Width || h != pim->Height)
            {
                // Resample will store an extra Alpha byte for RGB_888 -> RGBA_8888
                SoftwareResample(presampleImage->pData, w, h, presampleImage->Pitch,
                             pim->pData, pim->Width, pim->Height, pim->Pitch, bytesPerPixel);
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
                        pdest[x * 4 + 0] = scanline[x * 3 + 0]; // red
                        pdest[x * 4 + 1] = scanline[x * 3 + 1]; // green
                        pdest[x * 4 + 2] = scanline[x * 3 + 2]; // blue
                        pdest[x * 4 + 3] = 255; // alpha
                    }
                }
            }
            else if (pim->Format == GImage::Image_ARGB_8888)
            {
                XMemCpy(presampleImage->GetScanline(0), pim->GetScanline(0),
                        pim->GetPitch() * pim->Height);
            }
            // HACK: disable mipmaps, need to do something with them! (AB)
            if (pim->MipMapCount > 1)
            {
                GFC_DEBUG_WARNING(1, "GRendererD3D9 - Existing mipmap levels have been skipped due to resampling");
                levelsNeeded = 1;
            }

            // Swap byte order for Big-Endian XBox 360
            if  (presampleImage->Format == GImage::Image_ARGB_8888)
            {
                for (UInt y = 0; y < h; y++)
                {                   
                    UInt32* pdest = (UInt32*)presampleImage->GetScanline(y);
                    for (UInt x = 0; x < w; x++)
                        pdest[x] = _byteswap_ulong(pdest[x]);
                }

                // The new image has 4 bytes/pixel.
                bytesPerPixel = 4;          
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

    HRESULT result = pRenderer->pDevice->CreateTexture(
                        w, h,
                        levelsNeeded, 
                        0, // XBox: D3DUSAGE_BORDERSOURCE_TEXTURE                       
                        TextureFormat,
                        D3DPOOL_DEFAULT,                        
                        &pD3DTexture, NULL);
    if (result != S_OK)
    {    
        GFC_DEBUG_ERROR(1, "GTextureD3D9 - Can't create texture");
        Width = Height = 0;
        return 0;
    }
    
    
     if (pim->IsDataCompressed() || pim->MipMapCount > 1)
     {
         IDirect3DSurface9*  psurface    = NULL; 
         UInt                level;  
         GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pim;
         RECT                sourceRect  = { 0,0, w,h};
         D3DFORMAT           sourceSurfaceFormat = TextureFormat;

         // Determine source data format correctly (it may be different from texture).         
         if (TextureFormat == D3DFMT_A8R8G8B8)
             sourceSurfaceFormat = D3DFMT_LIN_A8B8G8R8;
         else if (TextureFormat == D3DFMT_A8)
             sourceSurfaceFormat = D3DFMT_LIN_A8;

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
                 sourceSurfaceFormat,
                 (pim->IsDataCompressed()) ? 
                    GImageBase::GetMipMapLevelSize(psourceImage->Format, mipW, 1) : 
                    GImageBase::GetPitch(psourceImage->Format, mipW),
                 NULL,
                 &sourceRect,
                 FALSE, 0, 0, // no packed mipmap for now
                 D3DX_DEFAULT, 0);
             psurface->Release();
             psurface = 0;

             if (result != S_OK)
             {
                 GFC_DEBUG_ERROR1(1, "GTextureD3D9 - Can't load surface from memory, result = %d", result);
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
             XMemCpy(presampleImage->pData, pim->pData, pim->Height * pim->Pitch);       
         }

         IDirect3DSurface9*  psurface    = NULL; 
         UInt                level;  
         GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pim;
         RECT                sourceRect  = { 0,0, w,h};

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
                 (bytesPerPixel == 1) ? D3DFMT_LIN_A8 : D3DFMT_LIN_A8B8G8R8, // NOTE: format order conversion from GL
                 (bytesPerPixel == 1) ? sourceRect.right : psourceImage->Pitch,
                 NULL,
                 &sourceRect,
                 FALSE, 0, 0, // no packed mipmap for now
                 D3DX_DEFAULT, 0);           
             psurface->Release();
             psurface = 0;

             if (result != S_OK)
             {
                 GFC_DEBUG_ERROR1(1, "GTextureD3D9 - Can't load surface from memory, result = %d", result);
                 pD3DTexture->Release();
                 pD3DTexture = 0;
                 Width = Height = 0;
                 return 0;
             }

             // For Alpha-only images, we might need to generate the next mipmap level.
             if (level< (levelsNeeded-1))
             {
                 GCOMPILER_ASSERT(sizeof(sourceRect.right) == sizeof(int));
                 GRendererXbox360Impl::MakeNextMiplevel((int*) &sourceRect.right, (int*) &sourceRect.bottom,
                     psourceImage->pData);
             }
         }
     }
    

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}


bool GTextureXbox360Impl::InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight)
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
        GFC_DEBUG_ERROR1(1, "GTextureXbox360 - Can't create texture from file %s", pfilename);
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

bool GTextureXbox360Impl::InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                   int targetWidth , int targetHeight)
{
    if (!pRenderer || !pRenderer->pDevice)
        return 0;

    if (pD3DTexture)
        pD3DTexture->Release();

    if (format == GImage::Image_ARGB_8888)
        TextureFormat   = D3DFMT_LIN_A8R8G8B8;
    else if (format == GImage::Image_RGB_888)
        TextureFormat   = D3DFMT_LIN_A8R8G8B8;
    else if (format == GImage::Image_A_8)
        TextureFormat   = D3DFMT_LIN_A8;
    else 
    { // Unsupported format
        GASSERT(0);
        return 0;
    }

    Width   = (targetWidth == 0)  ? width : targetWidth;
    Height  = (targetHeight == 0) ? height: targetHeight;

    HRESULT result = pRenderer->pDevice->CreateTexture(width, height, mipmaps+1, 0, TextureFormat, D3DPOOL_DEFAULT,
        &pD3DTexture, NULL);
    if (result != S_OK)
        return 0;

    return 1;
}

void GTextureXbox360Impl::Update(int level, int n, const UpdateRect *rects, const GImageBase *pim)
{
    UInt     bytesPerPixel, convert = 0;

    if (pim->Format == GImage::Image_ARGB_8888)
        bytesPerPixel   = 4;
    else if (pim->Format == GImage::Image_RGB_888)
    {
        convert = 1;
        bytesPerPixel   = 4;
    }
    else if (pim->Format == GImage::Image_A_8)
        bytesPerPixel   = 1;
    else
        GASSERT(0);

    if (!pD3DTexture && !CallRecreate())
    {
        GFC_DEBUG_WARNING(1, "GTextureXBox360::Update failed, could not recreate texture");
        return;     
    }

    // Ensure that our texture isn't set on the device during Update.
    pRenderer->pDevice->SetTexture(0, 0);
    pRenderer->pDevice->SetTexture(1, 0);

    for (int i = 0; i < n; i++)
    {
        GRect<int> rect = rects[i].src;
        GPoint<int> dest = rects[i].dest;

        D3DLOCKED_RECT      lr;
        RECT                destr;
        UInt32              lockFlags = (pRenderer->VMCFlags & GRendererXbox360::VMConfig_SupportTiling) ? D3DLOCK_NOOVERWRITE : 0;

        destr.left   = dest.x;
        destr.bottom = dest.y + rect.Height() - 1;
        destr.right  = dest.x + rect.Width() - 1;
        destr.top    = dest.y;

        if (FAILED( pD3DTexture->LockRect(level, &lr, &destr, lockFlags) ))
            return;

        if (convert && pim->Format == GImage::Image_RGB_888)
        {
            UByte *pdest = (UByte*)lr.pBits;
            //pdest += dest.y * lr.Pitch + dest.x * bytesPerPixel;

            for (int j = 0; j < rect.Height(); j++)
                for (int k = 0; k < rect.Width(); k++)
                {
                    pdest[j * lr.Pitch + k * bytesPerPixel +0] = pim->pData[(j + rect.Top) * pim->Pitch + (k + rect.Left) * 3 +0];
                    pdest[j * lr.Pitch + k * bytesPerPixel +1] = pim->pData[(j + rect.Top) * pim->Pitch + (k + rect.Left) * 3 +1];
                    pdest[j * lr.Pitch + k * bytesPerPixel +2] = pim->pData[(j + rect.Top) * pim->Pitch + (k + rect.Left) * 3 +2];
                    pdest[j * lr.Pitch + k * bytesPerPixel +3] = 255;
                }
        }
        else
        {
            UByte *pdest = (UByte*)lr.pBits;
        
            for (int j = 0; j < rect.Height(); j++)
                memcpy(pdest + j * lr.Pitch,
                       pim->GetScanline(j + rect.Top) + bytesPerPixel * rect.Left,
                       rect.Width() * bytesPerPixel);
        }

        pD3DTexture->UnlockRect(level);
    }
}


// Factory.
GRendererXbox360*   GRendererXbox360::CreateRenderer()
{
    return new GRendererXbox360Impl;
}


