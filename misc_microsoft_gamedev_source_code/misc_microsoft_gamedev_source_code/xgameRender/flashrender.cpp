//============================================================================
// flashrender.h
// Ensemble Studios (C) 2007
//============================================================================

#include "xgameRender.h"
#include "flashrender.h"
#include "scaleformIncludes.h"
#include "renderDraw.h"
#include "flashmanager.h"
#include "flashtexture.h"
#include "asyncFileManager.h"
#include "reloadManager.h"
#include "render.h"
#include "dynamicGPUBuffer.h"
#include "configsgamerender.h"

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

#ifdef BUILD_DEBUG
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libjpeg.lib")
#else
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libjpeg.lib")
#endif

#pragma comment(lib, "GFX.lib")
#pragma comment(lib, "GFx_XBox360.lib")

// #define GRENDERER_USE_PS20
#ifdef GRENDERER_USE_PS20
   #include "..\Src\GRenderer\ShadersD3D9/asm20.cpp"
#else
   #include "..\Src\GRenderer\ShadersD3D9/hlsl.cpp"
#endif

// rg [1/10/08] - One global dynamic GPU buffer for all BFlashRender's (not that there will be more than one, right?)
BDynamicGPUBuffer* BFlashRender::mpFlashDynamicGPUBuffer;

// ***** Vertex Declarations and Shaders
// Our vertex coords consist of two signed 16-bit integers, for (x,y) position only.
static D3DVERTEXELEMENT9 BFlashStripVertexDecl[] =
{
   {0, 0, D3DDECLTYPE_SHORT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
   D3DDECL_END()
};

static D3DVERTEXELEMENT9 BFlashGlyphVertexDecl[] =
{
   {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
   {0, 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
   {0, 24, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
   D3DDECL_END()
};

// Gouraud vertices used with Edge AA
static D3DVERTEXELEMENT9 BFlashVertexDeclXY16iC32[] =
{
   {0, 0, D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
   {0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
   D3DDECL_END()
};
static D3DVERTEXELEMENT9 BFlashVertexDeclXY16iCF32[] =
{
   {0, 0, D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
   {0, 4, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
   {0, 8, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 1 },
   D3DDECL_END()
};

static D3DVERTEXELEMENT9 BFlashUberVertexDecl[] = 
{
   {0, 0,   D3DDECLTYPE_SHORT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,   0 },
   {0, 4,   D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,      0 },
   {0, 8,   D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,      1 },
   {0, 12,  D3DDECLTYPE_FLOAT1,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   0 },

   {1, 0,   D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   3 },  
   {1, 16,  D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   4 }, 
   {1, 32,  D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   5 },  
   {1, 48,  D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   6 }, 
   {1, 64,  D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   7 },  
   {1, 80,  D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   8 }, 
   {1, 96,  D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   9 },  
   {1, 112, D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,   10},
   {1, 128, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 11},

   D3DDECL_END()
};

static D3DVERTEXELEMENT9 BFlashUberGlyphVertexDecl[] = 
{
   {0, 0,  D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
   {0, 8,  D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
   {0, 16, D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
   {0, 20, D3DDECLTYPE_FLOAT1,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
 
   {2, 0,   D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 },  
   {2, 16,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }, 
   {2, 32,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4 },  
   {2, 48,  D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 5 },    

   D3DDECL_END()
};

// Vertex declaration lookup table, must correspond to VDeclType +1
static const D3DVERTEXELEMENT9* BFlashVertexDeclTypeTable[BFlashRender::VD_Count - 1] =
{
   BFlashStripVertexDecl,
   BFlashGlyphVertexDecl,
   BFlashVertexDeclXY16iC32,
   BFlashVertexDeclXY16iCF32,

   //-- Uber Start
   BFlashUberVertexDecl,
   BFlashUberGlyphVertexDecl,
};

// Vertex shader text lookup table, must correspond to VShaderType +1
static const DWORD *BFlashVertexShaderTextTable[BFlashRender::VS_Count - 1] =
{
   g_xvs_VShaderStrip,
   g_xvs_VShaderGlyph,
   g_xvs_VShaderStripXY16iC32,
   g_xvs_VShaderStripXY16iCF32,
   g_xvs_VShaderStripXY16iCF32_T2
};



struct PixelShaderDesc
{
   // Shader, used if not mull.
   const DWORD*                 pShader;
};

PixelShaderDesc BFlashPixelShaderInitTable[BFlashRender::PS_Count] =
{   
   // Non-AA Shaders.
   { g_xps_PS_SolidColor},
   { g_xps_PS_CxformTexture},
   { g_xps_PS_CxformTextureMultiply},
   { g_xps_PS_TextTextureColor},

   // AA Shaders.
   { g_xps_PS_CxformGauraud},
   { g_xps_PS_CxformGauraudNoAddAlpha},
   // Texture stage fixed-function fall-backs only (their implementation is incorrect).
   { g_xps_PS_CxformGauraudTexture},
   { g_xps_PS_Cxform2Texture},
   { g_xps_PS_CxformGauraudMultiply},
   { g_xps_PS_CxformGauraudMultiplyNoAddAlpha},
   { g_xps_PS_CxformGauraudMultiplyTexture},
   { g_xps_PS_CxformMultiply2Texture},

   // alpha compositing shaders
   { g_xps_PS_AcSolidColor},
   { g_xps_PS_AcCxformTexture},
   { g_xps_PS_AcCxformTextureMultiply},
   { g_xps_PS_AcTextTextureColor},
   { g_xps_PS_AcCxformGauraud},
   { g_xps_PS_AcCxformGauraudNoAddAlpha},
   { g_xps_PS_AcCxformGauraudTexture},
   { g_xps_PS_AcCxform2Texture},
   { g_xps_PS_AcCxformGauraudMultiply},
   { g_xps_PS_AcCxformGauraudMultiplyNoAddAlpha},
   { g_xps_PS_AcCxformGauraudMultiplyTexture},
   { g_xps_PS_AcCxformMultiply2Texture},
};

bool gEnableScaleformDraws2 = true;

//#define DEBUG_TRACE_RECORD_FLASHRENDER

#ifdef  DEBUG_TRACE_RECORD_FLASHRENDER
#include "tracerecording.h"
#include "xbdm.h"
#pragma comment( lib, "tracerecording.lib" )
#endif


//============================================================================
//============================================================================
BFlashFillStyle::BFlashFillStyle()
{
   clear();         
}

//============================================================================
//============================================================================
BFlashFillStyle::~BFlashFillStyle()
{
}

//============================================================================
//============================================================================
void BFlashFillStyle::clear()
{
   mMode            = FM_None;
   mGouraudType     = GRenderer::GFill_Color;
   mBitmapColorTransform.SetIdentity();
   mFill.pTexture   = NULL;
   mFill2.pTexture  = NULL;    
   mColor           = 0xFFFFFFFF;   
}

//============================================================================
// void BFlashFillStyle::Apply
// Push our style into Direct3D
//============================================================================   
void BFlashFillStyle::Apply(BFlashRender* prenderer) const
{
   IDirect3DDevice9* pdevice = prenderer->mpDevice;
   GASSERT(mMode != FM_None);

   // Complex
   prenderer->ApplyBlendMode(prenderer->mBlendMode);

   pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 
      ((mColor.GetAlpha()== 0xFF) &&
      (prenderer->mBlendMode <= GRenderer::Blend_Normal) &&
      (mMode != FM_Gouraud))  ?  FALSE : TRUE);


   // Non-Edge AA Rendering.
   if (mMode == FM_Color)
   {
      // Solid color fills.
      prenderer->SetPixelShader(BFlashRender::PS_SolidColor);
      prenderer->ApplyColor(mColor);

#ifndef BUILD_FINAL
      gFlashManager.mWorkerStats.mPSDrawCalls[BFlashRender::PS_SolidColor]++;
#endif 
   }

   else if (mMode == FM_Bitmap)
   {
      // Gradient and texture fills.
      GASSERT(mFill.pTexture != NULL);

      prenderer->ApplyColor(mColor);
      prenderer->ApplySampleMode(mFill.SampleMode);

      if (mFill.pTexture == NULL)
      {
         // Just in case, for release build.
         prenderer->SetPixelShader(BFlashRender::PS_None);
#ifndef BUILD_FINAL
         gFlashManager.mWorkerStats.mPSDrawCalls[BFlashRender::PS_None]++;
#endif 
      }
      else
      {
         pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

         if ((prenderer->mBlendMode == GRenderer::Blend_Multiply) ||
            (prenderer->mBlendMode == GRenderer::Blend_Darken) )
         {
            prenderer->SetPixelShader(BFlashRender::PS_CxformTextureMultiply);

#ifndef BUILD_FINAL
            gFlashManager.mWorkerStats.mPSDrawCalls[BFlashRender::PS_CxformTextureMultiply]++;
#endif 
         }
         else
         {
            prenderer->SetPixelShader(BFlashRender::PS_CxformTexture);
#ifndef BUILD_FINAL
            gFlashManager.mWorkerStats.mPSDrawCalls[BFlashRender::PS_CxformTexture]++;
#endif 
         }

         prenderer->ApplyPShaderCxform(mBitmapColorTransform, 2);

         // Set texture to stage 0; matrix to constants 4 and 5
         if (!prenderer->ApplyFillTexture(mFill, 0, 4))
         {
            BDEBUG_ASSERT(0);
            return;
         }
      }
   }


   // Edge AA - relies on Gouraud shading and texture mixing
   else if (mMode == FM_Gouraud)
   {

      BFlashRender::BFlashRenderPixelShaderType shader = BFlashRender::PS_None;

      // No texture: generate color-shaded triangles.
      if (mFill.pTexture == NULL)
      {
         pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);


         if (prenderer->mVertexFmt == GRenderer::Vertex_XY16iC32)
         {
            // Cxform Alpha Add can not be non-zero is this state because 
            // cxform blend equations can not work correctly.
            // If we hit this assert, it means Vertex_XY16iCF32 should have been used.
            GASSERT (mBitmapColorTransform.M_[3][1] < 1.0f);

            shader = BFlashRender::PS_CxformGauraudNoAddAlpha;                        
         }
         else
         {
            GASSERT(prenderer->mVertexFmt != GRenderer::Vertex_XY16i);
            shader = BFlashRender::PS_CxformGauraud;
         }

         prenderer->ApplyPShaderCxform(mBitmapColorTransform, 2);

      }

      // We have a textured or multi-textured gouraud case.
      else
      {   

         pdevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

         prenderer->ApplyPShaderCxform(mBitmapColorTransform, 2);
         // Set texture to stage 0; matrix to constants 4 and 5
         if (!prenderer->ApplyFillTexture(mFill, 0, 4))
         {
            BDEBUG_ASSERT(0);
            return;
         }

         if ((mGouraudType == GRenderer::GFill_1TextureColor) ||
            (mGouraudType == GRenderer::GFill_1Texture))
         {
            shader = BFlashRender::PS_CxformGauraudTexture;
         }
         else
         {
            shader = BFlashRender::PS_Cxform2Texture;

            // Set second texture to stage 1; matrix to constants 5 and 6
            if (!prenderer->ApplyFillTexture(mFill2, 1, 6))
            {
               BDEBUG_ASSERT(0);
               return;
            }
         }

      }


      if ((prenderer->mBlendMode == GRenderer::Blend_Multiply) ||
         (prenderer->mBlendMode == GRenderer::Blend_Darken) )
      { 
         shader = (BFlashRender::BFlashRenderPixelShaderType)(shader + (BFlashRender::PS_CxformGauraudMultiply - BFlashRender::PS_CxformGauraud));

         // For indexing to work, these should hold:
         GCOMPILER_ASSERT( (BFlashRender::PS_Cxform2Texture - BFlashRender::PS_CxformGauraud) ==
            (BFlashRender::PS_CxformMultiply2Texture - BFlashRender::PS_CxformGauraudMultiply));
         GCOMPILER_ASSERT( (BFlashRender::PS_CxformGauraudMultiply - BFlashRender::PS_CxformGauraud) ==
            (BFlashRender::PS_Cxform2Texture - BFlashRender::PS_CxformGauraud + 1) );
      }

#ifndef BUILD_FINAL
      gFlashManager.mWorkerStats.mPSDrawCalls[shader]++;
#endif 
      prenderer->SetPixelShader(shader);
   }
}

//============================================================================
// void BFlashFillStyle::Disable
//============================================================================
void BFlashFillStyle::Disable()
{
   mMode          = FM_None;
   mFill.pTexture = 0;
}

//============================================================================
// void BFlashFillStyle::SetColor
//============================================================================
void BFlashFillStyle::SetColor(GColor color)
{
   mMode = FM_Color; 
   mColor = color;
}

//============================================================================
// void BFlashFillStyle::SetCxform
//============================================================================
void BFlashFillStyle::SetCxform(const GRenderer::Cxform& colorTransform)
{
   mBitmapColorTransform = colorTransform;      
}

//============================================================================
// void BFlashFillStyle::SetBitmap
//============================================================================
void BFlashFillStyle::SetBitmap(const GRenderer::FillTexture* pft, const GRenderer::Cxform& colorTransform)
{
   mMode = FM_Bitmap;
   mFill = *pft;
   mColor = GColor(0xFFFFFFFF);
   SetCxform(colorTransform);
}

//============================================================================
// void BFlashFillStyle::SetGouraudFill()
// Sets the interpolated color/texture fill style used for shapes with EdgeAA.
// The specified textures are applied to vertices {0, 1, 2} of each triangle based
// on factors of Complex vertex. Any or all subsequent pointers can be NULL, in which case
// texture is not applied and vertex colors used instead.
//============================================================================
void BFlashFillStyle::SetGouraudFill(GRenderer::GouraudFillType gfill,
   const GRenderer::FillTexture *ptexture0,
   const GRenderer::FillTexture *ptexture1,
   const GRenderer::FillTexture *ptexture2, const GRenderer::Cxform& colorTransform)
{
   // Texture2 is not yet used.
   if (ptexture0 || ptexture1 || ptexture2)
   {
      const GRenderer::FillTexture *p = ptexture0;
      if (!p) p = ptexture1;              

      SetBitmap(p, colorTransform);
      // Used in 2 texture mode
      if (ptexture1)
         mFill2 = *ptexture1;             
   }
   else
   {
      SetCxform(colorTransform);              
      mFill.pTexture = 0;  
   }

   mMode        = BFlashFillStyle::FM_Gouraud;
   mGouraudType = gfill;
}

//============================================================================
// bool BFlashFillStyle::IsValid() const
//============================================================================
bool BFlashFillStyle::IsValid() const
{
   return mMode != FM_None;
}

//============================================================================
//============================================================================
BFlashRender::BFlashRender() :
   mbInitialized(false),
   mbBatchingEnabled(true),
   mbWireframeEnabled(false)
{
   mModeSet = 0;
   mVMCFlags = 0;

   mStencilAvailable = 0;
   mStencilChecked = 0;
   mStencilCounter = 0;        
   mVDeclIndex = VD_None;
   mVShaderIndex = VS_None;
   mPShaderIndex = PS_None;      

   mSampleMode = Sample_Linear;
   mIndexFmt = D3DFMT_UNKNOWN;
   mVertexFmt = Vertex_None;

   mpDevice = 0;
   mhWnd = 0;

   mpVertexData = 0;
   mpIndexData  = 0;
   mIndexCount = 0;
   
   // VAT: 11/10/08 - set the garray to be noshrink to prevent 
   // memory churn on any push / pop operations
   mBlendModeStack.set_size_policy(GTL::garray<BlendType>::Buffer_NoShrink);
   mBlendModeStack.reserve(16);

   renderBatchingInit();

   // Glyph buffer z, w components are always 0 and 1.
   UInt i;
   for(i = 0; i < GlyphVertexBufferSize; i++)
   {
      mGlyphVertexBuffer[i].z = 0.0f;
      mGlyphVertexBuffer[i].w = 1.0f;
   }

   mRenderMode = 0; 

   const int cInitialBatchArraySize = 100;
   mBatches.reserve(cInitialBatchArraySize);
      
   eventReceiverInit(cThreadIndexRender, true);
   commandListenerInit();
   reloadInit();
   loadEffect();
}

//============================================================================
//============================================================================
BFlashRender::~BFlashRender()
{
   if (gRenderThread.getHasD3DOwnership())
      gRenderThread.blockUntilGPUIdle();

   ReleaseResources();
      
   reloadDeinit();
   commandListenerDeinit();
   eventReceiverDeinit(true);   
}

//============================================================================
//============================================================================
void BFlashRender::ReleaseResources()
{
   // Remove/notify all textures
   GLock::Locker guard(&mTexturesLock);
   while (mTextures.pFirst != &mTextures)
       ((BFlashTexture*)mTextures.pFirst)->RemoveFromRenderer();

   mCacheList.ReleaseList();

  if (mModeSet)
      ResetVideoMode();  
}

//============================================================================
// bool BFlashRender::CreateVertexDeclaration(GPtr<IDirect3DVertexDeclaration9> *pdeclPtr, const D3DVERTEXELEMENT9 *pvertexElements)
// Helpers used to create vertex declarations and shaders. 
//============================================================================
bool BFlashRender::CreateVertexDeclaration(GPtr<IDirect3DVertexDeclaration9> *pdeclPtr, const D3DVERTEXELEMENT9 *pvertexElements)
{
   if (!*pdeclPtr)
   {
      if (mpDevice->CreateVertexDeclaration(pvertexElements, &pdeclPtr->GetRawRef()) != S_OK)
      {
         GFC_DEBUG_WARNING(1, "GRendererXbox360 - failed to create vertex declaration");
         return 0;
      }
   }
   return 1;
}

//============================================================================
// bool BFlashRender::CreateVertexShader(GPtr<IDirect3DVertexShader9> *pshaderPtr, const char* pshaderText)
//============================================================================
bool BFlashRender::CreateVertexShader(GPtr<IDirect3DVertexShader9> *pshaderPtr, const DWORD* pshaderText)
{
   if (!*pshaderPtr)
   {
      HRESULT hr = mpDevice->CreateVertexShader( pshaderText, &pshaderPtr->GetRawRef() );

      if (hr != S_OK)
      {
         GFC_DEBUG_WARNING1(1, "GRendererXbox360 - Can't create D3D9 vshader; error code = %d", hr);
         return 0;
      }
   }
   return 1;
}

//============================================================================
// bool BFlashRender::CreatePixelShader(GPtr<IDirect3DPixelShader9> *pshaderPtr, const char* pshaderText)
//============================================================================
bool BFlashRender::CreatePixelShader(GPtr<IDirect3DPixelShader9> *pshaderPtr, const DWORD* pshaderText)
{
   if (!*pshaderPtr)
   {
      HRESULT hr = mpDevice->CreatePixelShader( pshaderText, &pshaderPtr->GetRawRef() );

      if (hr != S_OK)
      {
         GFC_DEBUG_WARNING1(1, "GRendererXbox360 - Can't create D3D9 pshader; error code = %d", hr);
         return 0;
      }
   }
   return 1;
}

//============================================================================
// bool BFlashRender::InitShaders()
// Initialize the vshader we use for SWF mesh rendering.
//============================================================================   
bool BFlashRender::InitShaders()
{
   bool success = 1;
   UInt i;

   for (i = 1; i < VD_Count; i++)
      if (!CreateVertexDeclaration(&mVertexDecls[i], BFlashVertexDeclTypeTable[i-1]))
      {
         success = 0;
         break;
      }

   for (i = 1; i < VS_Count; i++)
      if (!CreateVertexShader(&mVertexShaders[i], BFlashVertexShaderTextTable[i-1]))
      {
         success = 0;
         break;
      }

   // We only use pixel shaders conditionally if their support is available.       
   for (i = 1; i < PS_Count; i++)
   {
      if (BFlashPixelShaderInitTable[i-1].pShader)
      {
         if (!CreatePixelShader(&mPixelShaders[i], BFlashPixelShaderInitTable[i-1].pShader))
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

//============================================================================
// void BFlashRender::ReleaseShaders()
//============================================================================
void BFlashRender::ReleaseShaders()
{
   if (mpDevice)
   {
      mpDevice->SetVertexShader(0);
      mpDevice->SetPixelShader(0);
      mpDevice->SetVertexDeclaration(0);
   }

   UInt i;
   for (i=0; i< VD_Count; i++)
      if (mVertexDecls[i])
         mVertexDecls[i] = 0;

   for (i=0; i< VS_Count; i++)
      if (mVertexShaders[i])
         mVertexShaders[i] = 0;

   for (i=0; i< PS_Count; i++)
      if (mPixelShaders[i])
         mPixelShaders[i] = 0;
}

//============================================================================
// void BFlashRender::SetVertexDecl(VDeclType vd)
// Sets a shader to specified type.
//============================================================================
void BFlashRender::SetVertexDecl(BFlashRenderVDeclType vd)
{   
   if (mVDeclIndex != vd)
   {
      mVDeclIndex = vd;
      mpDevice->SetVertexDeclaration(mVertexDecls[vd]);
   }
}
//============================================================================
// void BFlashRender::SetVertexShader
//============================================================================
void BFlashRender::SetVertexShader(BFlashRenderVShaderType vt)
{
   if (mVShaderIndex != vt)
   {
      mVShaderIndex = vt;
      mpDevice->SetVertexShader(mVertexShaders[vt]);
   }
}

//============================================================================
// void BFlashRender::SetPixelShader
// Set shader to a device based on a constant
//============================================================================  
void BFlashRender::SetPixelShader(BFlashRenderPixelShaderType shaderTypein)
{
   int shaderType = shaderTypein;

   if (mRenderMode & GViewport::View_AlphaComposite)
      shaderType += int(PS_AcOffset);

   if (shaderType != mPShaderIndex)
   {           
      mpDevice->SetPixelShader(mPixelShaders[shaderType]);          
      mPShaderIndex = shaderType;
   }
}

//============================================================================
// static void BFlashRender::MakeNextMiplevel
// Utility.  Mutates *width, *height and *data to create the
// next mip level.
//============================================================================
void BFlashRender::MakeNextMiplevel(int* width, int* height, UByte* data)
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
            *(out) = (UByte) a >> 2;
            out++;
            in += 2;
         }
      }
   }

   // Munge parameters to reflect the shrunken image.
   *width = NewW;
   *height = NewH;
}

//============================================================================
// void BFlashRender::ApplyFillTexture
// Fill helper function:
// Applies fill texture by setting it to the specified stage, initializing samplers and vertex constants
//============================================================================
bool BFlashRender::ApplyFillTexture(const FillTexture &fill, UInt stageIndex, UInt matrixVertexConstIndex)
{
   BFlashTexture* pFlashTexture = reinterpret_cast<BFlashTexture*>(fill.pTexture);
   if (!pFlashTexture)
      return false;

   IDirect3DTexture9* pD3DTexture = pFlashTexture->GetD3DTexture();
   if (!pD3DTexture)
   {
      pFlashTexture->CallRecreate();
      return false;
   }

   mpDevice->SetTexture(stageIndex, pD3DTexture);
   
   // Set sampling
   if (fill.WrapMode == Wrap_Clamp)
   {                       
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   }
   else
   {
      GASSERT(fill.WrapMode == Wrap_Repeat);
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
   }
   // Set up the bitmap matrix for texgen.
   float   InvWidth = 1.0f / pFlashTexture->getWidth();
   float   InvHeight = 1.0f / pFlashTexture->getHeight();

   const Matrix&   m = fill.TextureMatrix;
   Float   p[4] = { 0, 0, 0, 0 };
   p[0] = m.M_[0][0] * InvWidth;
   p[1] = m.M_[0][1] * InvWidth;
   p[3] = m.M_[0][2] * InvWidth;
   mpDevice->SetVertexShaderConstantF(matrixVertexConstIndex, p, 1);

   p[0] = m.M_[1][0] * InvHeight;
   p[1] = m.M_[1][1] * InvHeight;
   p[3] = m.M_[1][2] * InvHeight;
   mpDevice->SetVertexShaderConstantF(matrixVertexConstIndex + 1, p, 1);

   return true;
}

//============================================================================
// void BFlashRender::ApplyFillTexture
// Fill helper function:
// Applies fill texture by setting it to the specified stage
//============================================================================
bool BFlashRender::ApplyFillTexture(const FillTexture &fill, UInt stageIndex)
{   
   BFlashTexture* pFlashTexture = reinterpret_cast<BFlashTexture*>(fill.pTexture);
   if (!pFlashTexture)
      return false;

   IDirect3DTexture9* pD3DTexture = pFlashTexture->GetD3DTexture();
   if (!pD3DTexture)
   {
      pFlashTexture->CallRecreate();
      return false;
   }
   
   if (stageIndex == 0)
      mTextureParam0 = pD3DTexture;
   else if (stageIndex == 1)
      mTextureParam1 = pD3DTexture;
   else
      mpDevice->SetTexture(stageIndex, pD3DTexture);

   // Set sampling
   if (fill.WrapMode == Wrap_Clamp)
   {                       
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   }
   else
   {
      GASSERT(fill.WrapMode == Wrap_Repeat);
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
      mpDevice->SetSamplerState(stageIndex, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
   }

   return true;
}

//============================================================================
// void BFlashRender::ApplyPShaderCxform
//============================================================================
void BFlashRender::ApplyPShaderCxform(const Cxform &cxform, UInt cxformPShaderConstIndex) const
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

   mpDevice->SetPixelShaderConstantF(cxformPShaderConstIndex, cxformData, 2);       
}

//============================================================================
// GTextureD3D9* BFlashRender::CreateTexture()
// Given an image, returns a Pointer to a GTexture struct
// that can later be passed to FillStyleX_bitmap(), to set a
// bitmap fill style.
//============================================================================
GTextureD3D9* BFlashRender::CreateTexture()
{
   return new BFlashTexture(this);
}

//============================================================================
// bool BFlashRender::GetRenderCaps(RenderCaps *pcaps)
// Helper function to query renderer capabilities.
//============================================================================
bool BFlashRender::GetRenderCaps(RenderCaps *pcaps)
{
   if (!mModeSet)
   {
      GFC_DEBUG_WARNING(1, "BFlashRender::GetRenderCaps fails - video mode not set");
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

   if (mVMCFlags & VMConfig_SupportTiling)
      pcaps->CapBits |= Cap_NoTexOverwrite;

   D3DCAPS9 caps9;     
   mpDevice->GetDeviceCaps(&caps9);     
   pcaps->MaxTextureSize = GTL::gmin(caps9.MaxTextureWidth, caps9.MaxTextureWidth);
   return 1;
}

//============================================================================
// void BFlashRender::BeginDisplay(GColor backgroundColor, const GViewport &vpin, Float x0, Float x1, Float y0, Float y1)
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
//============================================================================
void BFlashRender::BeginDisplay(GColor backgroundColor, const GViewport &vpin, Float x0, Float x1, Float y0, Float y1)
{
   SCOPEDSAMPLEID(BFlashRender_BeginDisplay, 0xFFFF0000);

   if (!mbInitialized)
      return;

   if (!mModeSet)
   {
      GFC_DEBUG_WARNING(1, "GRendererD3D9::BeginDisplay failed - video mode not set");
      return;
   }

   mRenderMode = (vpin.Flags & GViewport::View_AlphaComposite);

   mDisplayWidth = fabsf(x1 - x0);
   mDisplayHeight = fabsf(y1 - y0);

   // Need to get surface size, so that we can clamp viewport
   D3DSURFACE_DESC surfaceDesc;
   surfaceDesc.Width = vpin.Width;
   surfaceDesc.Height= vpin.Height;
   {
      GPtr<IDirect3DSurface9> psurface;           
      mpDevice->GetRenderTarget(0, &psurface.GetRawRef());
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
   mpDevice->SetViewport(&vp);


   mViewportMatrix.SetIdentity();
   mViewportMatrix.M_[0][0] = 2.0f  / dx;
   mViewportMatrix.M_[1][1] = -2.0f / dy;

   // Adjust by -0.5 pixel to match DirectX pixel coverage rules.
   Float xhalfPixelAdjust = (vp.Width > 0) ? (1.0f / (Float) vp.Width) : 0.0f;
   Float yhalfPixelAdjust = (vp.Height> 0) ? (1.0f / (Float) vp.Height) : 0.0f;

   // MA: it does not seem necessary to subtract viewport.
   // Need to work through details of viewport cutting, there still seems to
   // be a 1-pixel issue at edges. Could that be due to edge fill rules ?
   mViewportMatrix.M_[0][2] = -1.0f - mViewportMatrix.M_[0][0] * (clipX0) - xhalfPixelAdjust; 
   mViewportMatrix.M_[1][2] = 1.0f  - mViewportMatrix.M_[1][1] * (clipY0) + yhalfPixelAdjust;

   mViewportMatrix *= mUserMatrix;

   // Blending render states.
   mBlendModeStack.clear();
   mBlendMode = Blend_None;
   mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   ApplyBlendMode(mBlendMode);

   mMaskMode = eMaskModeNone;

   // Not necessary of not alpha testing:
   //pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER); 
   //pDevice->SetRenderState(D3DRS_ALPHAREF, 0x00);
   mpDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);  // Important!

   union 
   {
      float fbias;
      DWORD d;
   } bias;
   bias.fbias = -0.5f;

   // Must reset both stages since ApplySampleMode modifies both.
   mpDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   mpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   mpDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
   mpDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   mpDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   mpDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
   mSampleMode = Sample_Linear;
   mpDevice->SetSamplerState(0, D3DSAMP_MIPMAPLODBIAS, bias.d );

   // No ZWRITE by default
   mpDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0);
   mpDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
   mpDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);

   mpDevice->SetRenderState(D3DRS_CLIPPLANEENABLE,  0);
   mpDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);

   // Turn of back-face culling.
   mpDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   mpDevice->SetTexture(0, 0);
   mpDevice->SetTexture(1, 0); // No texture

   // Start the scene
   if (!(mVMCFlags&VMConfig_NoSceneCalls))
      mpDevice->BeginScene();

   // Vertex format.
   // Set None so that shader is forced to be set and then set to default: strip.
   mVDeclIndex   = VD_None;
   mVShaderIndex = VS_None;
   mPShaderIndex = PS_None;
   SetVertexDecl(VD_Strip);
   SetVertexShader(VS_Strip);
   // Start with solid shader.
   SetPixelShader(PS_SolidColor);

   // Init test for depth-stencil presence.
   mStencilChecked  = 0;
   mStencilAvailable= 0;
   mStencilCounter  = 0;

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
      ApplyMatrix(mCurrentMatrix);

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

      mpDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, strip, sizeof(Vertex));
   }
   
#if 0
   mTimer.start();
   BTimer beginBatchTimer;
   beginBatchTimer.start();
#endif

   if (mbBatchingEnabled)
      renderBatchingInit();

#if 0
   beginBatchTimer.stop();
   static bool bTraceBeginBatch = false;
   if (bTraceBeginBatch)
   {
      double elapsed = beginBatchTimer.getElapsedSeconds() * 1000.0f;
      trace("BeginBatchTime: %4.3f", elapsed);
   }
#endif
}
//============================================================================
// void BFlashRender::EndDisplay()
// Clean up after rendering a frame.  Client program is still
// responsible for calling Present or glSwapBuffers()
//============================================================================
void BFlashRender::EndDisplay()
{
   if (!mbInitialized)
      return;

   if (mbBatchingEnabled)
      renderBatchingFlush();
      
#if 0
   mTimer.stop();
   double elaspedTime = mTimer.getElapsedSeconds() * 1000.0f;

   static bool bTraceTime = false;
   if (bTraceTime)
      trace("FlashRender Time: %4.3f (ms)", elaspedTime);
#endif
      
   // End Scene
   if (mpDevice)
   {
      SetPixelShader(PS_None);
      SetVertexShader(VS_None);
      mpDevice->SetTexture(0,0);

      if (!(mVMCFlags&VMConfig_NoSceneCalls))
         mpDevice->EndScene();
   }
}
//============================================================================
// void BFlashRender::SetMatrix(const Matrix& m)
// Set the current transform for mesh & line-strip rendering.
//============================================================================
void BFlashRender::SetMatrix(const Matrix& m)
{
   mCurrentMatrix = m;
}

//============================================================================
// void BFlashRender::SetUserMatrix(const Matrix& m)
//============================================================================
void BFlashRender::SetUserMatrix(const Matrix& m)
{
   mUserMatrix = m;
}

//============================================================================
// Set the current color transform for mesh & line-strip rendering.
//============================================================================
void BFlashRender::SetCxform(const Cxform& cx)
{
   mCurrentCxform = cx;
}   

//============================================================================
// void BFlashRender::ApplyBlendMode(BlendType mode)
//============================================================================
void BFlashRender::ApplyBlendMode(BlendType mode)
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

      { D3DBLENDOP_ADD, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_DESTALPHA, D3DBLEND_ZERO },             // Add
      { D3DBLENDOP_REVSUBTRACT, D3DBLEND_ONE, D3DBLEND_ONE, D3DBLEND_DESTALPHA, D3DBLEND_ZERO },     // Subtract

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

   if (!mpDevice)
      return;

   if (mRenderMode & GViewport::View_AlphaComposite)
   {
      mpDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
      mpDevice->SetRenderState(D3DRS_BLENDOP, acmodes[mode].BlendOp);
      mpDevice->SetRenderState(D3DRS_BLENDOPALPHA, acmodes[mode].BlendOp);
      mpDevice->SetRenderState(D3DRS_SRCBLEND, acmodes[mode].SrcArg);        
      mpDevice->SetRenderState(D3DRS_DESTBLEND, acmodes[mode].DestArg);
      mpDevice->SetRenderState(D3DRS_SRCBLENDALPHA, acmodes[mode].SrcAlphaArg);
      mpDevice->SetRenderState(D3DRS_DESTBLENDALPHA, acmodes[mode].DestAlphaArg);
   }
   else
   {
      mpDevice->SetRenderState(D3DRS_BLENDOP, modes[mode].BlendOp);
      mpDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
      mpDevice->SetRenderState(D3DRS_SRCBLEND, modes[mode].SrcArg);        
      mpDevice->SetRenderState(D3DRS_DESTBLEND, modes[mode].DestArg);
   }
}

//============================================================================
// void BFlashRender::ApplySampleMode(BitmapSampleMode mode)
//============================================================================
void BFlashRender::ApplySampleMode(BitmapSampleMode mode)
{
   if (mSampleMode != mode)
   {
      mSampleMode = mode;
      _D3DTEXTUREFILTERTYPE filter =
         (mode == Sample_Point) ? D3DTEXF_POINT : D3DTEXF_LINEAR;            
      mpDevice->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
      mpDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);

      mpDevice->SetSamplerState(1, D3DSAMP_MINFILTER, filter);
      mpDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, filter);
   }       
}


//============================================================================
// void BFlashRender::PushBlendMode(BlendType mode)
// Pushes a Blend mode onto renderer.
//============================================================================
void BFlashRender::PushBlendMode(BlendType mode)
{
   // Blend modes need to be applied cumulatively, ideally through an extra RT texture.
   // If the nested clip has a "Multiply" effect, and its parent has an "Add", the result
   // of Multiply should be written to a buffer, and then used in Add.

   // For now we only simulate it on one level -> we apply the last effect on top of stack.
   // (Note that the current "top" element is BlendMode, it is not actually stored on stack).
   // Although incorrect, this will at least ensure that parent's specified effect
   // will be applied to children.

   mBlendModeStack.push_back(mBlendMode);

   if ((mode > Blend_Layer) && (mBlendMode != mode))
   {
      mBlendMode = mode;
      ApplyBlendMode(mBlendMode);
   }
}

//============================================================================
// void BFlashRender::PopBlendMode()
// Pops a blend mode, restoring it to the previous one. 
//============================================================================
void BFlashRender::PopBlendMode()
{
   if (mBlendModeStack.size() != 0)
   {                       
      // Find the next top interesting mode.
      BlendType   newBlendMode = Blend_None;

      for (SInt i = (SInt)mBlendModeStack.size()-1; i>=0; i--)         
         if (mBlendModeStack[i] > Blend_Layer)
         {
            newBlendMode = mBlendModeStack[i];
            break;
         }

      mBlendModeStack.pop_back();

      if (newBlendMode != mBlendMode)
      {
         mBlendMode = newBlendMode;
         ApplyBlendMode(mBlendMode);
      }

      return;
   }

   // Stack was empty..
   GFC_DEBUG_WARNING(1, "GRendererD3D9::PopBlendMode - blend mode stack is empty");
}

//============================================================================
// void BFlashRender::ApplyMatrix
// multiply current GRenderer::Matrix with d3d GRenderer::Matrix
//============================================================================
void BFlashRender::ApplyMatrix(const Matrix& matIn)
{
   Matrix  m(mViewportMatrix);
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

   mpDevice->SetVertexShaderConstantF(0, row0, 1);
   mpDevice->SetVertexShaderConstantF(1, row1, 1);

   mpDevice->SetVertexShaderConstantF(2, row2, 1);
   mpDevice->SetVertexShaderConstantF(3, row3, 1);
}

//============================================================================
// void BFlashRender::ApplyColor
// Set the given color. 
//============================================================================
void BFlashRender::ApplyColor(GColor c)
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

   if ((mBlendMode == Blend_Multiply) ||
      (mBlendMode == Blend_Darken))
   {
      rgba[0] = gflerp(1.0f, rgba[0], alpha);
      rgba[1] = gflerp(1.0f, rgba[1], alpha);
      rgba[2] = gflerp(1.0f, rgba[2], alpha);
   }

   mpDevice->SetPixelShaderConstantF(0, rgba, 1);
}   

//============================================================================
// void BFlashRender::FillStyleDisable()
// Don't fill on the {0 == left, 1 == right} side of a path.
//============================================================================
void BFlashRender::FillStyleDisable()
{
   mCurrentStyles[FILL_STYLE].Disable();
}

//============================================================================
// void BFlashRender::LineStyleDisable
// Don't draw a line on this path.
//============================================================================
void BFlashRender::LineStyleDisable()
{
   mCurrentStyles[LINE_STYLE].Disable();
}

//============================================================================
// void BFlashRender::FillStyleColor(GColor color)
// Set fill style for the left interior of the shape.  If
// enable is false, turn off fill for the left interior.
//============================================================================
void BFlashRender::FillStyleColor(GColor color)
{
   mCurrentStyles[FILL_STYLE].SetColor(mCurrentCxform.Transform(color));
}

//============================================================================
// void BFlashRender::LineStyleColor
// Set the line style of the shape.  If enable is false, turn
// off lines for following curve segments.
//============================================================================
void BFlashRender::LineStyleColor(GColor color)
{
   mCurrentStyles[LINE_STYLE].SetColor(mCurrentCxform.Transform(color));
}

//============================================================================
// void BFlashRender::FillStyleBitmap
//============================================================================
void BFlashRender::FillStyleBitmap(const FillTexture *pfill)
{       
   mCurrentStyles[FILL_STYLE].SetBitmap(pfill, mCurrentCxform);
}

//============================================================================
// void BFlashRender::FillStyleGouraud
// Sets the interpolated color/texture fill style used for shapes with EdgeAA.
//============================================================================
void BFlashRender::FillStyleGouraud(GouraudFillType gfill, const FillTexture *ptexture0, const FillTexture *ptexture1, const FillTexture *ptexture2)
{
   mCurrentStyles[FILL_STYLE].SetGouraudFill(gfill, ptexture0, ptexture1, ptexture2, mCurrentCxform);
}

//============================================================================
// void BFlashRender::SetVertexData
//============================================================================
void BFlashRender::SetVertexData(const void* pvertices, int numVertices, VertexFormat vf, CacheProvider *pcache)
{
   mpVertexData = pvertices;
   mVertexFmt   = vf;
   // Note: this could populate a static/dynamic buffer instead (if not null).

   // Test cache buffer management support.
   mCacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Vertex, (pvertices!=0),
      numVertices, (((pvertices!=0)&&numVertices) ? *((SInt16*)pvertices) : 0) );
}

//============================================================================
// void BFlashRender::SetIndexData
//============================================================================
void BFlashRender::SetIndexData(const void* pindices, int numIndices, IndexFormat idxf, CacheProvider *pcache)
{
   mpIndexData  = pindices;
   mIndexCount  = numIndices;
   switch(idxf)
   {
   case Index_None: mIndexFmt = D3DFMT_UNKNOWN; break;
   case Index_16: mIndexFmt = D3DFMT_INDEX16; break;
   case Index_32: mIndexFmt = D3DFMT_INDEX32; break;
   }       
   // Note: this could populate a dynamic buffer instead (if not null).

   // Test cache buffer management support.
   mCacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Index, (pindices!=0),
      numIndices, (((pindices!=0)&&numIndices) ? *((SInt16*)pindices) : 0) );
}

//============================================================================
// void BFlashRender::ReleaseCachedData
//============================================================================
void BFlashRender::ReleaseCachedData(CachedData *pdata, CachedDataType type)
{
   // Releases cached data that was allocated from the cache providers.
   mCacheList.ReleaseCachedData(pdata, type);
}

//============================================================================
// void BFlashRender::DrawIndexedTriList
//============================================================================
void BFlashRender::DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices,
   int startIndex, int triangleCount)
{
   if (!gEnableScaleformDraws2)
      return;

   if (!mModeSet)
      return;
   if (!mpVertexData || !mpIndexData || (mIndexFmt == D3DFMT_UNKNOWN)) // Must have vertex data.
   {
      GFC_DEBUG_WARNING(!mpVertexData, "GRendererXBox360::DrawIndexedTriList failed, vertex data not specified");
      GFC_DEBUG_WARNING(!mpIndexData, "GRendererXBox360::DrawIndexedTriList failed, index data not specified");
      GFC_DEBUG_WARNING((mIndexFmt == D3DFMT_UNKNOWN), "GRendererXBox360::DrawIndexedTriList failed, index buffer format not specified");
      return;
   }

   if (mbBatchingEnabled)
   {
      #ifndef BUILD_FINAL
      static bool bAllowBatch = true;
      if (!bAllowBatch)
         return;
      #endif

      BBatchData batch;
      batch.mBatchType = eBatchTriangleLists;      
      batch.mFillStyle = mCurrentStyles[FILL_STYLE];

      batch.mMatrix    = mCurrentMatrix;
      batch.mBlendMode = mBlendMode;
      batch.mVertexFormat = mVertexFmt;
      batch.mpVertexData = (void*) mpVertexData;
      batch.mVertexMinIndex = minVertexIndex;
      batch.mVertexStartIndex = baseVertexIndex;
      batch.mVertexCount = numVertices;
      batch.mIndexFormat = mIndexFmt;
      batch.mIndexStartIndex = startIndex;
      batch.mpIndexData = (void*) mpIndexData;
      batch.mIndexCount = mIndexCount;
      batch.mPrimCount = triangleCount;     
      batch.mMaskMode = mMaskMode;

      addBatch(batch);
      return;
   }

#if 0
   BTimer timer;
   timer.start();
#endif

   bool    isGouraud = (mCurrentStyles[FILL_STYLE].mMode == BFlashFillStyle::FM_Gouraud);
   int     vertexSize= 2 * sizeof(SInt16);

   // Ensure we have the right vertex size. 
   // MA: Should loosen some rules to allow for other decl/shader combinations?
   if (isGouraud)
   {
      if (mVertexFmt == Vertex_XY16iC32)
      {
         SetVertexDecl(VD_XY16iC32);
         SetVertexShader(VS_XY16iC32);
         vertexSize = sizeof(VertexXY16iC32);

#ifndef BUILD_FINAL
         gFlashManager.mWorkerStats.mVSDrawCalls[VS_XY16iC32]++;
#endif
      }
      else if (mVertexFmt == Vertex_XY16iCF32)
      {
         SetVertexDecl(VD_XY16iCF32);

         if (mCurrentStyles[FILL_STYLE].mGouraudType != GFill_2Texture)
         {
            SetVertexShader(VS_XY16iCF32);
#ifndef BUILD_FINAL
            gFlashManager.mWorkerStats.mVSDrawCalls[VS_XY16iCF32]++;
#endif
         }
         else
         {
            SetVertexShader(VS_XY16iCF32_T2);
#ifndef BUILD_FINAL
            gFlashManager.mWorkerStats.mVSDrawCalls[VS_XY16iCF32_T2]++;
#endif
         }

         vertexSize = sizeof(VertexXY16iCF32);
      }
   }
   else
   {
      SetVertexDecl(VD_Strip);
      SetVertexShader(VS_Strip);

      GASSERT(mVertexFmt == Vertex_XY16i);
      vertexSize= 2 * sizeof(SInt16);

#ifndef BUILD_FINAL
      gFlashManager.mWorkerStats.mVSDrawCalls[VS_Strip]++;
#endif
   }

   // Set up current style.
   mCurrentStyles[FILL_STYLE].Apply(this);

   ApplyMatrix(mCurrentMatrix);

   // TODO - should use a VB instead, and use DrawPrimitive().
   // or at least use a Dynamic buffer for now...
   const int bytesInIndex = ((mIndexFmt == D3DFMT_INDEX16) ? sizeof(UInt16) : sizeof(UInt32));
   const UByte* pindices = (UByte*)mpIndexData + startIndex * bytesInIndex;

   // Draw the mesh.
   // XBox360 refuses to render more than 65535 indexes.
   const int maxTriangles = 65535/3;

   mRenderStats.Triangles += triangleCount;

   while (triangleCount > maxTriangles)
   {      
      mpDevice->DrawIndexedPrimitiveUP(
         D3DPT_TRIANGLELIST, minVertexIndex, numVertices, maxTriangles,
         pindices, mIndexFmt,
         ((UByte*)mpVertexData) + baseVertexIndex * vertexSize,
         vertexSize );

      triangleCount -= maxTriangles;
      pindices      += bytesInIndex * maxTriangles * 3;
      mRenderStats.Primitives++;   
   }

   if (triangleCount > 0)
   {
      mpDevice->DrawIndexedPrimitiveUP(
         D3DPT_TRIANGLELIST, minVertexIndex, numVertices, triangleCount,
         pindices, mIndexFmt,
         ((UByte*)mpVertexData) + baseVertexIndex * vertexSize,
         vertexSize );

#ifndef BUILD_FINAL
      gFlashManager.mWorkerStats.mDrawCallsPerFrame++;
      gFlashManager.mWorkerStats.mTriangles += triangleCount;
      gFlashManager.mWorkerStats.mPrimitives++;


#endif
      mRenderStats.Triangles += triangleCount;
      mRenderStats.Primitives++;
   }        

#if 0
   timer.stop();   
   double t = timer.getElapsedSeconds();
   static double maxT;
   static double aveT;
   static uint count;
   count++;
   aveT+=t;
   if (t>maxT)
   {
      maxT=t;
      trace("Max: %1.6fms", maxT*1000.0f);
   }
   if ((count&511)==511)
   {
      trace("Ave: %1.6fms", aveT/count*1000.0f);
   }
#endif

}

//============================================================================
// void BFlashRender::DrawLineStrip
// Draw the line strip formed by the sequence of points.
//============================================================================
void BFlashRender::DrawLineStrip(int baseVertexIndex, int lineCount)
{
   SCOPEDSAMPLEID(BFlashRender_DrawLineStrip, 0xFFFF0000);
   if (!gEnableScaleformDraws2)
      return;

   if (!mModeSet)
      return;

   if (!mpVertexData) // Must have vertex data.
   {
      GFC_DEBUG_WARNING(!mpVertexData, "GRendererXBox360::DrawLineStrip failed, vertex data not specified");           
      return;
   }
  
   if (mbBatchingEnabled)
   {      
      BBatchData batch;
      batch.clear();
      batch.mBatchType = eBatchLineStrip;
      batch.mBlendMode = mBlendMode;
      batch.mpVertexData = (void*) mpVertexData;
      batch.mVertexDecl = VD_Strip;
      batch.mVSIndex = VS_Strip;
      batch.mFillStyle = mCurrentStyles[LINE_STYLE];
      batch.mMatrix = mCurrentMatrix;
      batch.mVertexStartIndex = baseVertexIndex;
      batch.mPrimCount = lineCount;
      batch.mMaskMode = mMaskMode;
      addBatch(batch);
      return;
   }
   
   // MA: Should loosen some rules to allow for other decl/shader combinations?
   GASSERT(mVertexFmt == Vertex_XY16i);
   SetVertexDecl(VD_Strip);
   SetVertexShader(VS_Strip);
   
   // Set up current style.
   mCurrentStyles[LINE_STYLE].Apply(this);

   ApplyMatrix(mCurrentMatrix);

   mpDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, lineCount, ((UByte*)mpVertexData) + baseVertexIndex * 2 * sizeof(SInt16), sizeof(SInt16)*2);

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mDrawCallsPerFrame++;
   gFlashManager.mWorkerStats.mLines+=lineCount;
   gFlashManager.mWorkerStats.mPrimitives++;
#endif
   mRenderStats.Lines += lineCount;
   mRenderStats.Primitives++;
}

//============================================================================
// void BFlashRender::DrawBitmaps
// Draw a set of rectangles textured with the given bitmap, with the given color.
// Apply given transform; ignore any currently set transforms.  
//
// Intended for textured glyph rendering.
//============================================================================
void BFlashRender::DrawBitmaps(BitmapDesc* pbitmapList, int listSize, int startIndex, int count,
   const GTexture* pti, const Matrix& m,
   CacheProvider *pcache)
{
   SCOPEDSAMPLEID(BFlashRender_DrawBitmaps, 0xFFFF0000);

   if (!gEnableScaleformDraws2)
      return;

   if (!mModeSet || !pbitmapList || !pti)
      return;

   BFlashTexture* ptexture = (BFlashTexture*)pti;
   // Texture must be valid for rendering
   if (!ptexture->GetD3DTexture() && !ptexture->CallRecreate())
   {
      GFC_DEBUG_WARNING(1, "GRendererXBox360::DrawBitmaps failed, empty texture specified/could not recreate texture");
      return;     
   }

   if (mbBatchingEnabled)
   {
      #ifndef BUILD_FINAL
      static bool bAllowBatch = true;
      if (!bAllowBatch)
         return;
      #endif

      BBatchData batch;
      batch.mBatchType = eBatchBitmaps;
      batch.mCxForm = mCurrentCxform;
      batch.mBlendMode = mBlendMode;
      batch.mBitmapSampleMode = Sample_Linear;
      batch.mPSIndex = PS_TextTexture;
      batch.mVSIndex = VS_Glyph;
      batch.mVertexDecl = VD_Glyph;
      batch.mMatrix = m;
      batch.mpBitmapList = pbitmapList;
      batch.mBitmapListCount = listSize;
      batch.mVertexStartIndex = startIndex;
      batch.mpTexture = ptexture->GetD3DTexture();
      batch.mpVertexData = mGlyphVertexBuffer;
      batch.mPrimCount = count;
      batch.mMaskMode = mMaskMode;
      addBatch(batch);
      return;
   }

   // Test cache buffer management support.
   // Optimized implementation could use this spot to set vertex buffer and/or initialize offset in it.
   // Note that since bitmap lists are usually short, it would be a good idea to combine many of them
   // into one buffer, instead of creating individual buffers for each pbitmapList data instance.
   mCacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_BitmapList, (pbitmapList!=0),
      listSize, (((pbitmapList!=0)&&listSize) ? ((SInt)pbitmapList->Coords.Left) : 0) );

   ApplyPShaderCxform(mCurrentCxform, 2);
   ApplyBlendMode(mBlendMode);
   ApplySampleMode(Sample_Linear);
   mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  TRUE);

   // Set texture.     
   SetPixelShader(PS_TextTexture);
   mpDevice->SetTexture(0, ptexture->GetD3DTexture());

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
         BFlashGlyphVertex* pv = mGlyphVertexBuffer + ivertex;

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
         mpDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, ivertex / 3, mGlyphVertexBuffer, sizeof(BFlashGlyphVertex));
         mRenderStats.Primitives++;

#ifndef BUILD_FINAL
         gFlashManager.mWorkerStats.mPrimitives++;
#endif
      }
   }
   mRenderStats.Triangles += count * 2;

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mTriangles += count*2;
#endif
}

//============================================================================
// void BFlashRender::BeginSubmitMask(SubmitMaskMode maskMode)
//============================================================================
void BFlashRender::BeginSubmitMask(SubmitMaskMode maskMode)
{
   if (mbBatchingEnabled)
   {
      switch (maskMode)
      {
         case Mask_Clear:
            mMaskMode = eMaskModeClear; 
            break;
         case Mask_Increment : 
            mMaskMode = eMaskModeIncrement; 
            break;
         case Mask_Decrement : 
            mMaskMode = eMaskModeDecrement; 
            break;
         default: mMaskMode = eMaskModeNone; 
            break;
      }

      BBatchData batch;
      batch.clear();
      batch.mBatchType = eBatchBeginSubmitMask;      
      batch.mMaskMode = mMaskMode;
      addBatch(batch);
      return;
   }


   if (!mModeSet)
      return;

   if (!mStencilAvailable)
   {
      if (!mStencilChecked)
      {
         // Test for depth-stencil presence.
         IDirect3DSurface9 *pdepthStencilSurface = 0;
         mpDevice->GetDepthStencilSurface(&pdepthStencilSurface);
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
               mStencilAvailable = 1;
               break;
            }

            pdepthStencilSurface->Release();
            pdepthStencilSurface = 0;
         }
         else
            mStencilAvailable = 0;

         mStencilChecked = 1;
      }

      if (!mStencilAvailable)
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


   mpDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);            
   mpDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
   mpDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);
   mpDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
   mpDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);

   switch(maskMode)
   {
   case Mask_Clear:
      mpDevice->Clear(0, 0, D3DCLEAR_STENCIL, 0, 0.0f, 0);
      mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
      mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
      mpDevice->SetRenderState(D3DRS_STENCILREF, 1);
      mStencilCounter = 1;
      break;

   case Mask_Increment:
      mpDevice->SetRenderState(D3DRS_STENCILREF, mStencilCounter);
      mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
      mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
      mStencilCounter++;
      break;
   case Mask_Decrement:                           
      mpDevice->SetRenderState(D3DRS_STENCILREF, mStencilCounter);
      mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
      mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
      mStencilCounter--;                       
      break;
   }
}

//============================================================================
// void BFlashRender::EndSubmitMask()
//============================================================================
void BFlashRender::EndSubmitMask()
{
   if (mbBatchingEnabled)
   {
      mMaskMode = eMaskModeKeep;

      BBatchData batch;
      batch.clear();
      batch.mBatchType = eBatchEndSubmitMask;      
      batch.mMaskMode = eMaskModeKeep;
      addBatch(batch);
      return;
   }

   if (!mModeSet)
      return;
   if (!mStencilAvailable)
      return;

   // Enable frame-buffer writes.        
   mpDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);

   // We draw only where the (stencil == StencilCounter), i.e. where the latest mask was drawn.
   // However, we don't change the stencil buffer.
   mpDevice->SetRenderState(D3DRS_STENCILREF, mStencilCounter);
   mpDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);
   mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
   mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
}

//============================================================================
// void BFlashRender::DisableMask()
//============================================================================
void BFlashRender::DisableMask()
{
   if (mbBatchingEnabled)
   {
      mMaskMode = eMaskModeNone;

      BBatchData batch;
      batch.clear();
      batch.mBatchType = eBatchDisableMask;      
      batch.mMaskMode = eMaskModeNone;
      addBatch(batch);
      return;
   }

   if (!mModeSet)
      return;
   if (!mStencilAvailable)
      return;

   mStencilCounter = 0;
   mpDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
}

//============================================================================
//============================================================================
void BFlashRender::ApplyMaskMode(int maskMode, bool bForce)
{
   if (!mbBatchingEnabled)
      return;

   if (maskMode == mMaskMode && !bForce)
      return;

   if (maskMode == eMaskModeNone)
   {
      mStencilCounter = 0;
      mpDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
      mpDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);      
   }
   else if (maskMode == eMaskModeKeep)
   {
      // Enable frame-buffer writes.        
      mpDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
      // We draw only where the (stencil == StencilCounter), i.e. where the latest mask was drawn.
      // However, we don't change the stencil buffer.
      mpDevice->SetRenderState(D3DRS_STENCILREF, mStencilCounter);
      mpDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);
      mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
      mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
   }
   else
   {
      mpDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);            
      mpDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
      mpDevice->SetRenderState(D3DRS_STENCILMASK, 0xFF);
      mpDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
      mpDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);

      if (maskMode == eMaskModeClear)
      {
         mpDevice->Clear(0, 0, D3DCLEAR_STENCIL, 0, 0.0f, 0);
         mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
         mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
         mpDevice->SetRenderState(D3DRS_STENCILREF, 1);
         mStencilCounter = 1;
      }
      else if (maskMode == eMaskModeIncrement)
      {
         mpDevice->SetRenderState(D3DRS_STENCILREF, mStencilCounter);
         mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
         mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
         mStencilCounter++;
      }
      else if (maskMode == eMaskModeDecrement)
      {

         mpDevice->SetRenderState(D3DRS_STENCILREF, mStencilCounter);
         mpDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
         mpDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
         mStencilCounter--;                       
      }
   }

   mMaskMode = maskMode;
}

//============================================================================
// BFlashRender::GetRenderStats(Stats *pstats, bool resetStats)
//============================================================================
void BFlashRender::GetRenderStats(Stats *pstats, bool resetStats)
{
   if (pstats)
      memcpy(pstats, &mRenderStats, sizeof(Stats));
   if (resetStats)
      mRenderStats.Clear();
}
//============================================================================
// BFlashRender::SetDependentVideoMode(
// GRendererD3D9 interface implementation
//============================================================================
bool BFlashRender::SetDependentVideoMode(
   LPDIRECT3DDEVICE9 pd3dDevice,
   D3DPRESENT_PARAMETERS* ppresentParams,
   UInt32 vmConfigFlags,
   HWND hwnd)
{
   if (!pd3dDevice || !ppresentParams)     
      return 0;

   // TODO: Need to check device caps ?        
   mpDevice = pd3dDevice;
   mpDevice->AddRef();

   if (!InitShaders())
   {
      ReleaseShaders();
      mpDevice->Release();
      mpDevice = 0;
      return 0;
   }

   mVMCFlags = vmConfigFlags;

   memcpy(&mPresentParams, ppresentParams, sizeof(D3DPRESENT_PARAMETERS));
   mhWnd = hwnd;

   mModeSet = 1;
   return 1;
}

//============================================================================
// BFlashRender::ResetVideoMode()
// Returns back to original mode (cleanup)
//============================================================================
bool BFlashRender::ResetVideoMode()
{
   if (!mModeSet)
      return 1;

   mpDevice->SetTexture(0, 0);
   mpDevice->SetTexture(1, 0);

   // Not that we no longer generate a texture DataLost event on 360;
   // since textures can survive the reset.      

   ReleaseShaders();
   mpDevice->Release();
   mpDevice = 0;
   mhWnd    = 0;        

   mModeSet = 0;
   return 1;
}

//============================================================================
// BFlashRender::GetDirect3D() const
// Direct3D9 Access
// Return various Dirext3D related information
//============================================================================
LPDIRECT3D9 BFlashRender::GetDirect3D() const
{
   LPDIRECT3D9 pd3d = 0;
   if (mpDevice)
      mpDevice->GetDirect3D(&pd3d);
   return pd3d;
}

//============================================================================
// virtual LPDIRECT3DDEVICE9 BFlashRender::GetDirect3DDevice() const
//============================================================================
LPDIRECT3DDEVICE9 BFlashRender::GetDirect3DDevice() const
{
   return mpDevice;
}

//============================================================================
// BFlashRender::GetDirect3DPresentParameters(D3DPRESENT_PARAMETERS* ppresentParams) const
//============================================================================
bool BFlashRender::GetDirect3DPresentParameters(D3DPRESENT_PARAMETERS* ppresentParams) const
{
   if (!mModeSet)
      return 0;
   memcpy(ppresentParams, &mPresentParams, sizeof(D3DPRESENT_PARAMETERS));
   return 1;
}

//==============================================================================
// BFlashRender::receiveEvent
//==============================================================================
bool BFlashRender::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
         {
            ASSERT_RENDER_THREAD
            initEffect(event);
            break;
         }

      case cEventClassClientRemove:
         {
            ASSERT_RENDER_THREAD
            killEffect();
            break;
         }

      case eEventClassReloadEffects:
         {
            loadEffect();
            break;
         }
   }
   return false;
}

//==============================================================================
//==============================================================================
void BFlashRender::loadEffect(void)
{   
   BFixedString256 filename;
   filename.format("flashrender\\flashrender0.bin");

   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

   pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
   pPacket->setFilename(filename);
   pPacket->setReceiverHandle(mEventHandle);
   pPacket->setPrivateData0(0); //-- effect slot;
   pPacket->setSynchronousReply(true);
   gAsyncFileManager.submitRequest(pPacket);
}

//==============================================================================
// BFlashRender::initEffect
//==============================================================================
void BFlashRender::initEffect(const BEvent& event)
{
   ASSERT_RENDER_THREAD
   BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);
   BASSERT(pPacket!=NULL);

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BFlashRender::initEffect: Async load of file %s failed", pPacket->getFilename().c_str());
   }
   else
   {
      mEffect.clear();
      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, pPacket->getData());
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BFlashRender::initEffect: Effect creation of file %s failed", pPacket->getFilename().c_str());
      }
      else
      {
         trace("BFlashRender::initEffect: Effect creation of file %s succeeded", pPacket->getFilename().c_str());
      }

      BASSERT(mEffect.getNumTechniques() == 2);

      // Get techniques and params
      mTechnique      = mEffect.getTechnique("flashTechnique");
      mTechniqueAC    = mEffect.getTechnique("flashTechniqueAC");
      mTextureParam0  = mEffect("gTex0Sampler");
      mTextureParam1  = mEffect("gTex1Sampler");
      mMatrixV0Param  = mEffect("gMatrixV0");
      mMatrixV1Param  = mEffect("gMatrixV1");
      mCxForm0Param   = mEffect("gCxForm0");
      mCxForm1Param   = mEffect("gCxForm1");


      BASSERT(mTechnique.getValid());
      BASSERT(mTechniqueAC.getValid());
      BASSERT(mTextureParam0.getValid());
      BASSERT(mTextureParam1.getValid());
      BASSERT(mMatrixV0Param.getValid());
      BASSERT(mMatrixV1Param.getValid());
      BASSERT(mCxForm0Param.getValid());
      BASSERT(mCxForm1Param.getValid());
      mbInitialized = true;
   }
}

//==============================================================================
//==============================================================================
void BFlashRender::killEffect()
{   
   mTechnique.clear();
   mEffect.clear();   

   mbInitialized = false;
}

//==============================================================================
// BFlashRender::reloadInit
//==============================================================================
void BFlashRender::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result =gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(cFME_SUCCESS == result);

   strPathAddBackSlash(effectFilename);
   effectFilename += "flashrender\\flashrender*.bin";
   paths.pushBack(effectFilename);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}

//==============================================================================
// BFlashRender::reloadDeinit
//==============================================================================
void BFlashRender::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}

//==============================================================================
//==============================================================================
void BFlashRender::initDeviceData()
{
   if (!mpFlashDynamicGPUBuffer)
   {
      mpFlashDynamicGPUBuffer = ALIGNED_NEW(BDynamicGPUBuffer, gRenderHeap);

      const uint cDynamicGPUBufferSize = (uint)(3.5f * 1024.0f * 1024.0f);
      mpFlashDynamicGPUBuffer->init(cDynamicGPUBufferSize, 64);
   }      
}

//==============================================================================
//==============================================================================
void BFlashRender::frameBegin(void)
{
}

//==============================================================================
//==============================================================================
void BFlashRender::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
}

//==============================================================================
//==============================================================================
void BFlashRender::frameEnd(void)
{
}

//==============================================================================
//==============================================================================
void BFlashRender::deinitDeviceData(void)
{
   if (mpFlashDynamicGPUBuffer)
   {
      ALIGNED_DELETE(mpFlashDynamicGPUBuffer, gRenderHeap);
      mpFlashDynamicGPUBuffer = NULL;
   }   
}

//==============================================================================
//==============================================================================
void BFlashRender::addBatch(BBatchData& batch)
{     
//   BScopedPIXNamedEvent PIXNamedEvent("BFlashRender_addBatch");

   //-- is this a stencil operation? then just add it 
   if ((batch.mBatchType == eBatchEndSubmitMask)   ||
       (batch.mBatchType == eBatchBeginSubmitMask) ||
       (batch.mBatchType == eBatchDisableMask))
   {      
      mBatches.add(batch);
      return;
   }

   //-- a draw call batch operation
   prepCommonRenderData(batch);

   //-- now check whether we can combine the new batch with the previous batch.
   int count = mBatches.getNumber();
   bool bCombineBatch = false;
   static bool bCombineBatches = true;
   if (count > 0 && bCombineBatches)
   {
      bCombineBatch = mBatches[count-1].compare(batch);
   }

   switch (batch.mBatchType)
   {
      case eBatchLineStrip:
         prepLineStripBatch(batch);
         break;
      case eBatchBitmaps:         
         prepBitmapBatch(batch, bCombineBatch);
         break;
      case eBatchTriangleLists:
         prepTriangleListBatch(batch, bCombineBatch);
         break;
   };      

}

//==============================================================================
//==============================================================================
void BFlashRender::prepCommonRenderData(BBatchData& batch)
{
   //-- default values
   batch.mTechniquePassIndex = -1;
   batch.mEnableAlphaBlending = false;
   batch.mbApplyFillTexture1 = false;
   batch.mbApplyFillTexture2 = false;

   batch.mEnableAlphaBlending = ((batch.mFillStyle.mColor.GetAlpha()== 0xFF) &&
                                 (batch.mBlendMode <= GRenderer::Blend_Normal) &&
                                 (batch.mFillStyle.mMode != BFlashFillStyle::FM_Gouraud));
         
   int techniquePassIndex = -1;
   if (batch.mFillStyle.mMode == BFlashFillStyle::FM_Color)
   {
      techniquePassIndex = 0;
      batch.mbApplyColor = true;
   }
   else if (batch.mFillStyle.mMode == BFlashFillStyle::FM_Bitmap)
   {      
      batch.mbApplyColor = true;
      if (batch.mFillStyle.mFill.pTexture == NULL)
      {
         return;
      }
      else
      {
         batch.mEnableAlphaBlending = true;

         if ((batch.mBlendMode == GRenderer::Blend_Multiply) ||
             (batch.mBlendMode == GRenderer::Blend_Darken) )
         {
            techniquePassIndex = 1;
         }
         else
         {
            techniquePassIndex = 2;
         }

         batch.mbApplyFillTexture1 = true;                  
      }
   }
   // Edge AA - relies on Gouraud shading and texture mixing
   else if (batch.mFillStyle.mMode == BFlashFillStyle::FM_Gouraud)
   {
      BFlashRender::BFlashRenderPixelShaderType shader = BFlashRender::PS_None;
      // No texture: generate color-shaded triangles.
      if (batch.mFillStyle.mFill.pTexture == NULL)
      {
         batch.mEnableAlphaBlending = true;
         techniquePassIndex = 0;

         if(batch.mVertexFormat == GRenderer::Vertex_XY16iC32)
         {
            shader = BFlashRender::PS_CxformGauraudNoAddAlpha;                        
         }
         else
         {
            shader = BFlashRender::PS_CxformGauraud;
         }         
      }
      // We have a textured or multi-textured gouraud case.
      else
      {   
         batch.mEnableAlphaBlending = true;
         batch.mbApplyFillTexture1 = true;

         if ((batch.mFillStyle.mGouraudType == GRenderer::GFill_1TextureColor) ||
             (batch.mFillStyle.mGouraudType == GRenderer::GFill_1Texture))
         {
            techniquePassIndex = 2;
            shader = BFlashRender::PS_CxformGauraudTexture;

         }
         else
         {
            techniquePassIndex = 4;
            shader = BFlashRender::PS_Cxform2Texture;

            batch.mbApplyFillTexture2 = true;
         }
      }

      if ((batch.mBlendMode == GRenderer::Blend_Multiply) ||
          (batch.mBlendMode == GRenderer::Blend_Darken) )
      { 
         shader = (BFlashRender::BFlashRenderPixelShaderType)(shader + (BFlashRender::PS_CxformGauraudMultiply - BFlashRender::PS_CxformGauraud));

         // For indexing to work, these should hold:
         GCOMPILER_ASSERT( (BFlashRender::PS_Cxform2Texture - BFlashRender::PS_CxformGauraud) ==
            (BFlashRender::PS_CxformMultiply2Texture - BFlashRender::PS_CxformGauraudMultiply));
         GCOMPILER_ASSERT( (BFlashRender::PS_CxformGauraudMultiply - BFlashRender::PS_CxformGauraud) ==
            (BFlashRender::PS_Cxform2Texture - BFlashRender::PS_CxformGauraud + 1) );

         switch (shader)
         {
            case PS_None:
               techniquePassIndex = -1;
               break;
            case PS_SolidColor:
            case PS_CxformGauraud:
            case PS_CxformGauraudNoAddAlpha:
               techniquePassIndex = 0;
               break;

            case PS_CxformTextureMultiply:
            case PS_CxformGauraudMultiply:
            case PS_CxformGauraudMultiplyNoAddAlpha:
               techniquePassIndex = 1;
               break;

            case PS_CxformTexture:
            case PS_CxformGauraudTexture:
               techniquePassIndex = 2;
               break;

            case PS_CxformGauraudMultiplyTexture:
               techniquePassIndex = 3;
               break;
            
            case PS_TextTexture:
               BASSERT(0);
               break;
                                    
            case PS_Cxform2Texture:
               techniquePassIndex = 4;
               break;

            case PS_CxformMultiply2Texture:
               techniquePassIndex = 5;
               break;
         }         
      }
   }

   batch.mTechniquePassIndex = techniquePassIndex;
}

//==============================================================================
//==============================================================================
void BFlashRender::prepLineStripBatch(BBatchData& batch)
{
   //convert the line strip into a line list for batching
   const uint vertexSize  = sizeof(SInt16) * 2;
   const uint bytesPerLine = vertexSize * 2;
   const uint bytesToCopy = bytesPerLine * batch.mPrimCount; // 2 verts per line
   if (mDynamicVB1BytesLeft < bytesToCopy)
   {
      renderBatchingFlush();
      renderBatchingLock(bytesToCopy, 0, 0, 0);
   }

   BASSERT(mDynamicVB1BytesLeft >= bytesToCopy);
   UByte* pVB = ((UByte*)mpDynamicVB1Buffer) + mDynamicVB1Offset;   
   batch.mRenderStream0VertexOffset = mDynamicVB1Offset;

   UByte* pSourceVB = ((UByte*)batch.mpVertexData) + batch.mVertexStartIndex * vertexSize;
   uint   bytesCopied = 0;
   
   for (int i = 0; i < batch.mPrimCount; ++i)
   {
      memcpy(pVB, pSourceVB, bytesPerLine);
      pSourceVB += vertexSize;
      pVB += bytesPerLine;
      bytesCopied += bytesPerLine;
   }
   BASSERT(bytesCopied == bytesToCopy);

   mDynamicVB1Offset += bytesToCopy;
   mDynamicVB1BytesLeft -= bytesToCopy;
   
   mBatches.add(batch);   
}

//==============================================================================
//==============================================================================
void BFlashRender::prepBitmapBatch(BBatchData& batch, bool bCombineBatch)
{
//   BScopedPIXNamedEvent PIXNamedEvent("BFlashRender_prepBitmapBatch");

#if 0
   BTimer timer;
   timer.start();
#endif

   uint count = batch.mPrimCount;
   
   uint vertexSize = sizeof(BFlashGlyphUberVertex);
   uint bytesToCopyStream0 = 6 * vertexSize * count;
   uint bytesToCopyStream2 = sizeof(BFlashGlyphUberParameterVertex);

   if (mDynamicVB1BytesLeft < bytesToCopyStream0 ||
       mDynamicVB3BytesLeft < bytesToCopyStream2)
   {
      renderBatchingFlush();
      renderBatchingLock(bytesToCopyStream0, 0, bytesToCopyStream2, 0);
      
      bCombineBatch = false;
   }
   
   BASSERT((mDynamicVB1BytesLeft >= bytesToCopyStream0) && (mDynamicVB3BytesLeft >= bytesToCopyStream2));

   BFlashGlyphUberVertex* pVB = (BFlashGlyphUberVertex*) (((UByte*)mpDynamicVB1Buffer) + mDynamicVB1Offset);      
   
   // cache off the render batch offset 
   batch.mRenderStream0VertexOffset = mDynamicVB1Offset;
   batch.mRenderStream2VertexOffset = mDynamicVB3Offset;
   batch.mRenderStream0IndexOffset = -1;

   BFlashGlyphUberParameterVertex paramVertex;

   //-- compute matrix
   Matrix finalMatrix(mViewportMatrix);
   finalMatrix *= batch.mMatrix;

   paramVertex.mMatrixV0[0] = finalMatrix.M_[0][0];
   paramVertex.mMatrixV0[1] = finalMatrix.M_[0][1];
   paramVertex.mMatrixV0[2] = 0.0f;
   paramVertex.mMatrixV0[3] = finalMatrix.M_[0][2];

   paramVertex.mMatrixV1[0] = finalMatrix.M_[1][0];
   paramVertex.mMatrixV1[1] = finalMatrix.M_[1][1];
   paramVertex.mMatrixV1[2] = 0.0f;
   paramVertex.mMatrixV1[3] = finalMatrix.M_[1][2];
      
   paramVertex.mCxForm0[0] = batch.mCxForm.M_[0][0];
   paramVertex.mCxForm0[1] = batch.mCxForm.M_[1][0];
   paramVertex.mCxForm0[2] = batch.mCxForm.M_[2][0];
   paramVertex.mCxForm0[3] = batch.mCxForm.M_[3][0];

   // Cxform color is already pre-multiplied
   paramVertex.mCxForm1[0] = batch.mCxForm.M_[0][1] * cOneOver255;
   paramVertex.mCxForm1[1] = batch.mCxForm.M_[1][1] * cOneOver255;
   paramVertex.mCxForm1[2] = batch.mCxForm.M_[2][1] * cOneOver255;
   paramVertex.mCxForm1[3] = batch.mCxForm.M_[3][1] * cOneOver255;
   
   
//-- FIXING PREFIX BUG ID 6685
   const BitmapDesc* pBitmapDesc = &batch.mpBitmapList[batch.mVertexStartIndex];
//--
   for (int i = count; i > 0; --i)
   {
      const float top    = pBitmapDesc->Coords.Top;
      const float bottom = pBitmapDesc->Coords.Bottom;
      const float left   = pBitmapDesc->Coords.Left;
      const float right  = pBitmapDesc->Coords.Right;
      const float uvTop    = pBitmapDesc->TextureCoords.Top;
      const float uvBottom = pBitmapDesc->TextureCoords.Bottom;
      const float uvLeft   = pBitmapDesc->TextureCoords.Left;
      const float uvRight  = pBitmapDesc->TextureCoords.Right;

      const DWORD color  = pBitmapDesc->Color.Raw;

      pBitmapDesc++;
         
      pVB[0].x = left; 
      _ReadWriteBarrier(); 
      pVB[0].y = top; 
      _ReadWriteBarrier();         
      pVB[0].u = uvLeft; 
      _ReadWriteBarrier(); 
      pVB[0].v = uvTop; 
      _ReadWriteBarrier(); 
      pVB[0].color= color; 
      _ReadWriteBarrier(); 
      pVB[0].paramIndex = mGlyphParamIndex;
      _ReadWriteBarrier(); 

      pVB[1].x = right; 
      _ReadWriteBarrier(); 
      pVB[1].y = top; 
      _ReadWriteBarrier();         
      pVB[1].u = uvRight; 
      _ReadWriteBarrier(); 
      pVB[1].v = uvTop; 
      _ReadWriteBarrier(); 
      pVB[1].color = color; 
      _ReadWriteBarrier();
      pVB[1].paramIndex = mGlyphParamIndex;
      _ReadWriteBarrier(); 

      pVB[2].x = left; 
      _ReadWriteBarrier(); 
      pVB[2].y = bottom; 
      _ReadWriteBarrier();         
      pVB[2].u = uvLeft; 
      _ReadWriteBarrier(); 
      pVB[2].v = uvBottom; 
      _ReadWriteBarrier(); 
      pVB[2].color = color; 
      _ReadWriteBarrier();
      pVB[2].paramIndex = mGlyphParamIndex;
      _ReadWriteBarrier(); 

      pVB[3].x = left; 
      _ReadWriteBarrier(); 
      pVB[3].y = bottom; 
      _ReadWriteBarrier();         
      pVB[3].u = uvLeft; 
      _ReadWriteBarrier(); 
      pVB[3].v = uvBottom; 
      _ReadWriteBarrier(); 
      pVB[3].color = color; 
      _ReadWriteBarrier();
      pVB[3].paramIndex = mGlyphParamIndex;
      _ReadWriteBarrier(); 

      pVB[4].x = right; 
      _ReadWriteBarrier(); 
      pVB[4].y = top; 
      _ReadWriteBarrier();         
      pVB[4].u = uvRight; 
      _ReadWriteBarrier(); 
      pVB[4].v = uvTop; 
      _ReadWriteBarrier(); 
      pVB[4].color = color; 
      _ReadWriteBarrier();
      pVB[4].paramIndex = mGlyphParamIndex;
      _ReadWriteBarrier(); 

      pVB[5].x = right; 
      _ReadWriteBarrier(); 
      pVB[5].y = bottom; 
      _ReadWriteBarrier();         
      pVB[5].u = uvRight; 
      _ReadWriteBarrier(); 
      pVB[5].v = uvBottom; 
      _ReadWriteBarrier(); 
      pVB[5].color = color; 
      _ReadWriteBarrier();
      pVB[5].paramIndex = mGlyphParamIndex;
      _ReadWriteBarrier(); 

      pVB+=6;
   }

   mDynamicVB1Offset += bytesToCopyStream0;
   mDynamicVB1BytesLeft -= bytesToCopyStream0;  

   //-- Add one Vertex to stream 1 for the batch parameter Data;
   BFlashGlyphUberParameterVertex* pParamVB = (BFlashGlyphUberParameterVertex*) (((UByte*)mpDynamicVB3Buffer) + mDynamicVB3Offset);
   memcpy(pParamVB, &paramVertex, sizeof(BFlashGlyphUberParameterVertex));

   mDynamicVB3Offset += bytesToCopyStream2;
   mDynamicVB3BytesLeft -= bytesToCopyStream2;
   mGlyphParamIndex+=1.0f;

   if (bCombineBatch)
   {
      int index = mBatches.getNumber() - 1;
      mBatches[index].mPrimCount += batch.mPrimCount;
   }
   else
   {
      if (batch.mbApplyFillTexture1 && batch.mFillStyle.mFill.pTexture)
         batch.mFillStyle.mFill.pTexture->AddRef();

      if (batch.mbApplyFillTexture2 && batch.mFillStyle.mFill2.pTexture)
         batch.mFillStyle.mFill2.pTexture->AddRef();

      mBatches.add(batch);
   }

#if 0
   timer.stop();   
   double t = timer.getElapsedSeconds();
   static double maxT;
   static double aveT;
   static uint counter;
   counter++;
   aveT+=t;
   if (t>maxT)
   {
      maxT=t;
      trace("Max: %1.6fms", maxT*1000.0f);
   }
   if ((counter&511)==511)
   {
      trace("Ave: %1.6fms", aveT/counter*1000.0f);
   }
#endif   
}

//==============================================================================
//==============================================================================
void BFlashRender::prepTriangleListBatch(BBatchData& batch, bool bCombineBatch)
{
//   BScopedPIXNamedEvent PIXNamedEvent("BFlashRender_prepTriangleListBatch");
   
#if 0
   BTimer timer;
   timer.start();
#endif

#ifdef DEBUG_TRACE_RECORD_FLASHRENDER
   static bool bTraceDump = false;
   if ((bTraceDump) && (batch.mVertexCount > 40))
   {
      DmMapDevkitDrive();
      XTraceStartRecording( "e:\\FlashRender.bin" );
   }
#endif

   const bool    isGouraud = (batch.mFillStyle.mMode == BFlashFillStyle::FM_Gouraud);
   const uint     stream0VertexSize = sizeof(BFlashUberVertex);
   const uint     stream1VertexSize = sizeof(BFlashUberParameterVertex);
   const uint     streamIndexSize   = sizeof(UInt16);

   uint     bytesToCopyStream0 = stream0VertexSize * batch.mVertexCount;
   uint     bytesToCopyStream1 = stream1VertexSize;
   uint     bytesToCopyIndices = streamIndexSize * batch.mIndexCount;

   //-- do we need to flush because we don't have enough room for this batch?
   if ((bytesToCopyStream0 > mDynamicVB1BytesLeft) ||
       (bytesToCopyStream1 > mDynamicVB2BytesLeft) ||
       (bytesToCopyIndices > mDynamicIB1BytesLeft))
   {
#ifndef BUILD_FINAL   
      static bool bDumpStatus = false;
      if (bDumpStatus)
      {
         trace("VB1 Full: %d -- VB2 Full: %d -- IB Full %d", bytesToCopyStream0>mDynamicVB1BytesLeft? 1 : 0 , bytesToCopyStream1>mDynamicVB2BytesLeft? 1 : 0, bytesToCopyIndices > mDynamicIB1BytesLeft ? 1 : 0);
      }
#endif      

      renderBatchingFlush();
      renderBatchingLock(bytesToCopyStream0, bytesToCopyStream1, 0, bytesToCopyIndices);
                  
      //-- if we had to flush then we can't combine anymore
      bCombineBatch = false;
   }
   
   BASSERT((bytesToCopyStream0 <= mDynamicVB1BytesLeft) && (bytesToCopyStream1 <= mDynamicVB2BytesLeft) && (bytesToCopyIndices <= mDynamicIB1BytesLeft));

   BFlashUberParameterVertex paramVertex;
   //-- compute matrix
   Matrix finalMatrix(mViewportMatrix);
   finalMatrix *= batch.mMatrix;

   paramVertex.mMatrixV0[0] = finalMatrix.M_[0][0];
   paramVertex.mMatrixV0[1] = finalMatrix.M_[0][1];
   paramVertex.mMatrixV0[2] = 0.0f;
   paramVertex.mMatrixV0[3] = finalMatrix.M_[0][2];

   paramVertex.mMatrixV1[0] = finalMatrix.M_[1][0];
   paramVertex.mMatrixV1[1] = finalMatrix.M_[1][1];
   paramVertex.mMatrixV1[2] = 0.0f;
   paramVertex.mMatrixV1[3] = finalMatrix.M_[1][2];
   
//-- FIXING PREFIX BUG ID 6686
   const BFlashTexture* pTexture = ((BFlashTexture*)batch.mFillStyle.mFill.pTexture);
//--
   if (batch.mbApplyFillTexture1 && pTexture)
   {
      float InvWidth =  pTexture->getOneOverWidth();
      float InvHeight = pTexture->getOneOverHeight();

      paramVertex.mTexGen0[0] = batch.mFillStyle.mFill.TextureMatrix.M_[0][0] * InvWidth;
      paramVertex.mTexGen0[1] = batch.mFillStyle.mFill.TextureMatrix.M_[0][1] * InvWidth;
      paramVertex.mTexGen0[2] = 0.0f;
      paramVertex.mTexGen0[3] = batch.mFillStyle.mFill.TextureMatrix.M_[0][2] * InvWidth;

      paramVertex.mTexGen1[0] = batch.mFillStyle.mFill.TextureMatrix.M_[1][0] * InvHeight;
      paramVertex.mTexGen1[1] = batch.mFillStyle.mFill.TextureMatrix.M_[1][1] * InvHeight;
      paramVertex.mTexGen1[2] = 0.0f;
      paramVertex.mTexGen1[3] = batch.mFillStyle.mFill.TextureMatrix.M_[1][2] * InvHeight;
   }
   else
   {
      paramVertex.mTexGen0[0] = 1.0f;
      paramVertex.mTexGen0[1] = 1.0f;
      paramVertex.mTexGen0[2] = 1.0f;
      paramVertex.mTexGen0[3] = 1.0f;

      paramVertex.mTexGen1[0] = 1.0f;
      paramVertex.mTexGen1[1] = 1.0f;
      paramVertex.mTexGen1[2] = 1.0f;
      paramVertex.mTexGen1[3] = 1.0f;
   }
   
//-- FIXING PREFIX BUG ID 6687
   const BFlashTexture* pTexture2 = ((BFlashTexture*)batch.mFillStyle.mFill2.pTexture);
//--
   if (batch.mbApplyFillTexture2 && pTexture2)
   {
      float InvWidth2 =  pTexture2->getOneOverWidth();
      float InvHeight2 = pTexture2->getOneOverHeight();
      paramVertex.mTexGen2[0] = batch.mFillStyle.mFill2.TextureMatrix.M_[0][0] * InvWidth2;
      paramVertex.mTexGen2[1] = batch.mFillStyle.mFill2.TextureMatrix.M_[0][1] * InvWidth2;
      paramVertex.mTexGen2[2] = 0.0f;
      paramVertex.mTexGen2[3] = batch.mFillStyle.mFill2.TextureMatrix.M_[0][2] * InvWidth2;

      paramVertex.mTexGen3[0] = batch.mFillStyle.mFill2.TextureMatrix.M_[1][0] * InvHeight2;
      paramVertex.mTexGen3[1] = batch.mFillStyle.mFill2.TextureMatrix.M_[1][1] * InvHeight2;
      paramVertex.mTexGen3[2] = 0.0f;
      paramVertex.mTexGen3[3] = batch.mFillStyle.mFill2.TextureMatrix.M_[1][2] * InvHeight2;
   }
   else
   {
      paramVertex.mTexGen2[0] = 1.0f;
      paramVertex.mTexGen2[1] = 1.0f;
      paramVertex.mTexGen2[2] = 1.0f;
      paramVertex.mTexGen2[3] = 1.0f;

      paramVertex.mTexGen3[0] = 1.0f;
      paramVertex.mTexGen3[1] = 1.0f;
      paramVertex.mTexGen3[2] = 1.0f;
      paramVertex.mTexGen3[3] = 1.0f;
   }

   //const float mult = 1.0f / 255.0f;
   paramVertex.mCxForm0[0] = batch.mFillStyle.mBitmapColorTransform.M_[0][0];
   paramVertex.mCxForm0[1] = batch.mFillStyle.mBitmapColorTransform.M_[1][0];
   paramVertex.mCxForm0[2] = batch.mFillStyle.mBitmapColorTransform.M_[2][0];
   paramVertex.mCxForm0[3] = batch.mFillStyle.mBitmapColorTransform.M_[3][0];

   // Cxform color is already pre-multiplied
   paramVertex.mCxForm1[0] = batch.mFillStyle.mBitmapColorTransform.M_[0][1] * cOneOver255;
   paramVertex.mCxForm1[1] = batch.mFillStyle.mBitmapColorTransform.M_[1][1] * cOneOver255;
   paramVertex.mCxForm1[2] = batch.mFillStyle.mBitmapColorTransform.M_[2][1] * cOneOver255;
   paramVertex.mCxForm1[3] = batch.mFillStyle.mBitmapColorTransform.M_[3][1] * cOneOver255;

   paramVertex.mColor = 0xFFFFFFFF; // default is white
   if (batch.mbApplyColor)
   {
      //-- we probably should optimize THIS!!
      const float alpha = batch.mFillStyle.mColor.GetAlpha() * cOneOver255;
      // Set color.
      float rgba[4] = 
      {
         batch.mFillStyle.mColor.GetRed() * cOneOver255,  
         batch.mFillStyle.mColor.GetGreen() * cOneOver255,
         batch.mFillStyle.mColor.GetBlue() * cOneOver255, 
         alpha
      };      

      if ((batch.mBlendMode == Blend_Multiply) || (batch.mBlendMode == Blend_Darken))
      {
         rgba[0] = gflerp(1.0f, rgba[0], alpha);
         rgba[1] = gflerp(1.0f, rgba[1], alpha);
         rgba[2] = gflerp(1.0f, rgba[2], alpha);
      }

      XMVECTOR colorV = XMVectorSet(rgba[0], rgba[1], rgba[2], rgba[3]);
      XMStoreColor(&paramVertex.mColor, colorV);
   }
   
   BASSERT(bytesToCopyStream0 <= mDynamicVB1BytesLeft);
   BASSERT(bytesToCopyStream1 <= mDynamicVB2BytesLeft);
   BASSERT(bytesToCopyIndices <= mDynamicIB1BytesLeft);

   //-- if we can't combine then adjust our start offsets for the new batch
   batch.mRenderStream0VertexOffset= mDynamicVB1Offset;
   batch.mRenderStream1VertexOffset= mDynamicVB2Offset;
   batch.mRenderStream0IndexOffset = mIBStartIndex;   
      
   //-- Fill the Stream 0 with the vertex data
   BFlashUberVertex* pVB = (BFlashUberVertex*) (((UByte*)mpDynamicVB1Buffer) + mDynamicVB1Offset);      
   // Ensure we have the right vertex size. 
   // MA: Should loosen some rules to allow for other decl/shader combinations?
   if (isGouraud)
   {
      if (batch.mVertexFormat == Vertex_XY16iC32)
      {
         int sourceVertexSize = sizeof(VertexXY16iC32);
                  
         VertexXY16iC32* pSourceVB = (VertexXY16iC32*) (((UByte*)batch.mpVertexData) + batch.mVertexStartIndex * sourceVertexSize);

         for (int i = batch.mVertexCount >> 2; i > 0; --i)
         {  
            const DWORD x0 = *(DWORD*)(&pSourceVB[0].x);
            const DWORD y0 = pSourceVB[0].Color;
            const DWORD x1 = *(DWORD*)(&pSourceVB[1].x);
            const DWORD y1 = pSourceVB[1].Color;
            const DWORD x2 = *(DWORD*)(&pSourceVB[2].x);
            const DWORD y2 = pSourceVB[2].Color;
            const DWORD x3 = *(DWORD*)(&pSourceVB[3].x);
            const DWORD y3 = pSourceVB[3].Color;
            pSourceVB += 4;

            *(DWORD*)(&pVB[0].x) =  x0;
            _ReadWriteBarrier(); 
            pVB[0].Color = y0; 
            _ReadWriteBarrier(); 
            pVB[0].Factors = 0xFFFFFFFF; 
            _ReadWriteBarrier(); 
            pVB[0].paramIndex = mParamIndex; 
            _ReadWriteBarrier();   

            *(DWORD*)(&pVB[1].x) =  x1;
            _ReadWriteBarrier(); 
            pVB[1].Color = y1; 
            _ReadWriteBarrier(); 
            pVB[1].Factors = 0xFFFFFFFF; 
            _ReadWriteBarrier(); 
            pVB[1].paramIndex = mParamIndex; 
            _ReadWriteBarrier();  

            *(DWORD*)(&pVB[2].x) =  x2;
            _ReadWriteBarrier(); 
            pVB[2].Color = y2; 
            _ReadWriteBarrier(); 
            pVB[2].Factors = 0xFFFFFFFF; 
            _ReadWriteBarrier(); 
            pVB[2].paramIndex = mParamIndex; 
            _ReadWriteBarrier();  

            *(DWORD*)(&pVB[3].x) =  x3;
            _ReadWriteBarrier(); 
            pVB[3].Color = y3; 
            _ReadWriteBarrier(); 
            pVB[3].Factors = 0xFFFFFFFF; 
            _ReadWriteBarrier(); 
            pVB[3].paramIndex = mParamIndex; 
            _ReadWriteBarrier();  

            pVB += 4;
         }         
        
         for (int i = batch.mVertexCount & 3; i > 0; --i)
         {  
            const DWORD x0 = *(DWORD*)(&pSourceVB[0].x);
            const DWORD y0 = pSourceVB[0].Color;
            pSourceVB++;

            *(DWORD*)(&pVB[0].x) = x0;
            _ReadWriteBarrier();
            pVB[0].Color = y0;
            _ReadWriteBarrier();
            pVB[0].Factors = 0xFFFFFFFF;
            _ReadWriteBarrier();
            pVB[0].paramIndex = mParamIndex;
            _ReadWriteBarrier();

            pVB++;
         }         
      }
      else if (batch.mVertexFormat == Vertex_XY16iCF32)
      {
         int sourceVertexSize = sizeof(VertexXY16iCF32);
         VertexXY16iCF32* pSourceVB = (VertexXY16iCF32*) (((UByte*)batch.mpVertexData) + batch.mVertexStartIndex * sourceVertexSize);

         
         for (int i = batch.mVertexCount >> 2; i > 0; --i)
         {
            const DWORD x0 = *(DWORD*)(&pSourceVB[0].x);
            const DWORD y0 = pSourceVB[0].Color;
            const DWORD z0 = pSourceVB[0].Factors;

            const DWORD x1 = *(DWORD*)(&pSourceVB[1].x);
            const DWORD y1 = pSourceVB[1].Color;
            const DWORD z1 = pSourceVB[1].Factors;

            const DWORD x2 = *(DWORD*)(&pSourceVB[2].x);
            const DWORD y2 = pSourceVB[2].Color;
            const DWORD z2 = pSourceVB[2].Factors;

            const DWORD x3 = *(DWORD*)(&pSourceVB[3].x);
            const DWORD y3 = pSourceVB[3].Color;
            const DWORD z3 = pSourceVB[3].Factors;

            pSourceVB += 4;

            *(DWORD*)(&pVB[0].x) = x0; 
            _ReadWriteBarrier(); 
            pVB[0].Color = y0; 
            _ReadWriteBarrier(); 
            pVB[0].Factors = z0; 
            _ReadWriteBarrier(); 
            pVB[0].paramIndex = mParamIndex; 
            _ReadWriteBarrier();

            *(DWORD*)(&pVB[1].x) = x1; 
            _ReadWriteBarrier(); 
            pVB[1].Color = y1; 
            _ReadWriteBarrier(); 
            pVB[1].Factors = z1; 
            _ReadWriteBarrier(); 
            pVB[1].paramIndex = mParamIndex; 
            _ReadWriteBarrier();

            *(DWORD*)(&pVB[2].x) = x2; 
            _ReadWriteBarrier(); 
            pVB[2].Color = y2; 
            _ReadWriteBarrier(); 
            pVB[2].Factors = z2; 
            _ReadWriteBarrier(); 
            pVB[2].paramIndex = mParamIndex; 
            _ReadWriteBarrier();

            *(DWORD*)(&pVB[3].x) = x3; 
            _ReadWriteBarrier(); 
            pVB[3].Color = y3; 
            _ReadWriteBarrier(); 
            pVB[3].Factors = z3; 
            _ReadWriteBarrier(); 
            pVB[3].paramIndex = mParamIndex; 
            _ReadWriteBarrier();

            pVB += 4;
         }      

         for (int i = batch.mVertexCount & 3; i > 0; --i)
         {  
            const DWORD x0 = *(DWORD*)(&pSourceVB[0].x);
            const DWORD y0 = pSourceVB[0].Color;
            const DWORD z0 = pSourceVB[0].Factors;
            pSourceVB++;

            *(DWORD*)(&pVB[0].x) = x0;
            _ReadWriteBarrier();
            pVB[0].Color = y0;
            _ReadWriteBarrier();
            pVB[0].Factors = z0;
            _ReadWriteBarrier();
            pVB[0].paramIndex = mParamIndex;
            _ReadWriteBarrier();

            pVB++;
         }       
      }
   }
   else
   {
      int sourceVertexSize = sizeof(VertexXY16i);
      VertexXY16i* pSourceVB = (VertexXY16i*) (((UByte*)batch.mpVertexData) + batch.mVertexStartIndex * sourceVertexSize);
      
      for (int i = batch.mVertexCount >> 2; i > 0; --i)
      {  
         const DWORD x0 = *(DWORD*)(&pSourceVB[0].x);
         const DWORD x1 = *(DWORD*)(&pSourceVB[1].x);
         const DWORD x2 = *(DWORD*)(&pSourceVB[2].x);
         const DWORD x3 = *(DWORD*)(&pSourceVB[3].x);
         pSourceVB += 4;

         *(DWORD*)(&pVB[0].x) =  x0;
         _ReadWriteBarrier(); 
         pVB[0].Color = 0xFFFFFFFF; 
         _ReadWriteBarrier(); 
         pVB[0].Factors = 0x00FFFFFF; 
         _ReadWriteBarrier(); 
         pVB[0].paramIndex = mParamIndex; 
         _ReadWriteBarrier();   

         *(DWORD*)(&pVB[1].x) =  x1;
         _ReadWriteBarrier(); 
         pVB[1].Color = 0xFFFFFFFF; 
         _ReadWriteBarrier(); 
         pVB[1].Factors = 0x00FFFFFF; 
         _ReadWriteBarrier(); 
         pVB[1].paramIndex = mParamIndex; 
         _ReadWriteBarrier();  

         *(DWORD*)(&pVB[2].x) =  x2;
         _ReadWriteBarrier(); 
         pVB[2].Color = 0xFFFFFFFF; 
         _ReadWriteBarrier(); 
         pVB[2].Factors = 0x00FFFFFF; 
         _ReadWriteBarrier(); 
         pVB[2].paramIndex = mParamIndex; 
         _ReadWriteBarrier();  

         *(DWORD*)(&pVB[3].x) =  x3;
         _ReadWriteBarrier(); 
         pVB[3].Color = 0xFFFFFFFF; 
         _ReadWriteBarrier(); 
         pVB[3].Factors = 0x00FFFFFF; 
         _ReadWriteBarrier(); 
         pVB[3].paramIndex = mParamIndex; 
         _ReadWriteBarrier();  

         pVB += 4;
      }         
     
      for (int i = batch.mVertexCount & 3; i > 0; --i)
      {  
         const DWORD x0 = *(DWORD*)(&pSourceVB[0].x);
         pSourceVB++;

         *(DWORD*)(&pVB[0].x) = x0;
         _ReadWriteBarrier();
         pVB[0].Color = 0xFFFFFFFF;
         _ReadWriteBarrier();
         pVB[0].Factors = 0x00FFFFFF;
         _ReadWriteBarrier();
         pVB[0].paramIndex = mParamIndex;
         _ReadWriteBarrier();

         pVB++;
      }         
   }

   //-- adjust our offset and used counter for stream 0
   mDynamicVB1Offset += bytesToCopyStream0;
   mDynamicVB1BytesLeft -= bytesToCopyStream0;
   
   //-- Add one Vertex to stream 1 for the batch parameter Data;
   BFlashUberParameterVertex* pParamVB = (BFlashUberParameterVertex*) (((UByte*)mpDynamicVB2Buffer) + mDynamicVB2Offset);
   memcpy(pParamVB, &paramVertex, sizeof(BFlashUberParameterVertex));

   mDynamicVB2Offset += bytesToCopyStream1;
   mDynamicVB2BytesLeft -= bytesToCopyStream1;
   mParamIndex+=1.0f;

   //-- Now fill the index buffer data that corresponds to stream 0
   if (bCombineBatch)
   {
      BDEBUG_ASSERT(batch.mIndexFormat == D3DFMT_INDEX16);   
      const WORD* pSource = (WORD*)batch.mpIndexData;
      WORD* pIB = (WORD*) ((((UByte*)mpDynamicIB1Buffer) + mDynamicIB1Offset));

      int oldBatchIndex = mBatches.getNumber() - 1;
      const BBatchData& bLastBatch = mBatches[oldBatchIndex];

      WORD vertCount = (WORD)bLastBatch.mVertexCount;
      const DWORD vertCount32 = vertCount | (vertCount << 16);
      uint numIndices = batch.mIndexCount;

      if (((DWORD)pIB) & 3)
      {
         *pIB++ = *pSource++ + vertCount;
         _ReadWriteBarrier();
         numIndices--;
      }

      while (numIndices >= 8)
      {
         DWORD a = ((const DWORD*)pSource)[0] + vertCount32;
         DWORD b = ((const DWORD*)pSource)[1] + vertCount32;
         DWORD c = ((const DWORD*)pSource)[2] + vertCount32;
         DWORD d = ((const DWORD*)pSource)[3] + vertCount32;

         ((DWORD*)pIB)[0] = a;
         _ReadWriteBarrier();
         ((DWORD*)pIB)[1] = b;
         _ReadWriteBarrier();
         ((DWORD*)pIB)[2] = c;
         _ReadWriteBarrier();
         ((DWORD*)pIB)[3] = d;
         _ReadWriteBarrier();

         pIB += 8;
         pSource += 8;
         numIndices -= 8;
      }

      while (numIndices)
      {
         *pIB++ = *pSource++ + vertCount;
         _ReadWriteBarrier();

         numIndices--;
      }

      mBatches[oldBatchIndex].mVertexCount += batch.mVertexCount;
   }
   else   
   {

      BDEBUG_ASSERT(batch.mIndexFormat == D3DFMT_INDEX16);   
      const UByte* pSource = (UByte*)batch.mpIndexData;   
      UByte* pIB = (((UByte*)mpDynamicIB1Buffer) + mDynamicIB1Offset);
      memcpy((void*) pIB, (void*) pSource, bytesToCopyIndices);
   }
   
   mDynamicIB1Offset += bytesToCopyIndices;
   mDynamicIB1BytesLeft -= bytesToCopyIndices;
   mIBStartIndex += batch.mIndexCount;

   if (bCombineBatch)
   {
      int index = mBatches.getNumber() - 1;
      mBatches[index].mIndexCount += batch.mIndexCount;
   }
   else
   {      
      //-- if we are adding a new batch then add to the ref count of the fill textures so they can't 
      //-- deleted while we are batching up render calls.  we only need to do this when a new batch gets
      //-- added -- shared batches have the ref count already bumped
      if (batch.mbApplyFillTexture1 && batch.mFillStyle.mFill.pTexture)
         batch.mFillStyle.mFill.pTexture->AddRef();

      if (batch.mbApplyFillTexture2 && batch.mFillStyle.mFill2.pTexture)
         batch.mFillStyle.mFill2.pTexture->AddRef();

      mBatches.add(batch);
   }

#ifdef DEBUG_TRACE_RECORD_FLASHRENDER
   if ((bTraceDump) && (batch.mVertexCount > 40))
   {
      XTraceStopRecording();
      DebugBreak();
      bTraceDump = false;
   }   
#endif

#if 0
   timer.stop();   
   double t = timer.getElapsedSeconds();
   static double maxT;
   static double aveT;
   static uint count;
   count++;
   aveT+=t;
   if (t>maxT)
   {
      maxT=t;
      trace("Max: %1.6fms", maxT*1000.0f);
   }
   if ((count&511)==511)
   {
      trace("Ave: %1.6fms", aveT/count*1000.0f);
   }
#endif
}

//==============================================================================
//==============================================================================
void BFlashRender::renderBatchingInit(void)
{
   mDynamicVB1Size = 0;
   mDynamicVB1BytesLeft = 0;
   mDynamicVB1Offset = 0;
   mpDynamicVB1 = NULL;
   mpDynamicVB1Buffer = NULL;

   mDynamicVB2Size = 0;
   mDynamicVB2BytesLeft = 0;
   mDynamicVB2Offset = 0;
   mpDynamicVB2 = NULL;
   mpDynamicVB2Buffer = NULL;

   mDynamicVB3Size = 0;
   mDynamicVB3BytesLeft = 0;
   mDynamicVB3Offset = 0;
   mpDynamicVB3 = NULL;
   mpDynamicVB3Buffer = NULL;

   mDynamicIB1Size = 0;
   mDynamicIB1BytesLeft = 0;
   mDynamicIB1Offset = 0;
   mIBStartIndex = 0;
   mpDynamicIB1 = NULL;
   mpDynamicIB1Buffer = NULL;

   mParamIndex = 0.0f;
   mGlyphParamIndex = 0.0f;
}

//==============================================================================
//==============================================================================
void BFlashRender::renderBatchingLock(uint vb1Size, uint vb2Size, uint vb3Size, uint ibSize)
{   
   const uint vbBuffer1Size = vb1Size ? Math::Max(vb1Size, 32U * 1024U) : 0;
   const uint vbBuffer2Size = vb2Size ? Math::Max(vb2Size, 8U * 1024U) : 0;
   const uint vbBuffer3Size = vb3Size ? Math::Max(vb3Size, 8U * 1024U) : 0;
   const uint ibBuffer1Size = ibSize ? Math::Max(ibSize, 8U * 1024U) : 0;
   
   mDynamicVB1Size = vbBuffer1Size;
   mDynamicVB1BytesLeft = vbBuffer1Size;
   mDynamicVB1Offset = 0;
   if (vbBuffer1Size)
   {
      mpDynamicVB1 = mpFlashDynamicGPUBuffer->createVB(vbBuffer1Size);
      mpDynamicVB1->Lock(0,0, &mpDynamicVB1Buffer, 0);
   }
   else
   {
      mpDynamicVB1 = NULL;
      mpDynamicVB1Buffer = NULL;
   }
   
   mDynamicVB2Size = vbBuffer2Size;
   mDynamicVB2BytesLeft = vbBuffer2Size;
   mDynamicVB2Offset = 0;
   if (vbBuffer2Size)
   {
      mpDynamicVB2 = mpFlashDynamicGPUBuffer->createVB(vbBuffer2Size);
      mpDynamicVB2->Lock(0,0, &mpDynamicVB2Buffer, 0);   
   }      
   else
   {
      mpDynamicVB2 = NULL;
      mpDynamicVB2Buffer = NULL;
   }
   
   mDynamicVB3Size = vbBuffer3Size;
   mDynamicVB3BytesLeft = vbBuffer3Size;
   mDynamicVB3Offset = 0;
   if (vbBuffer3Size)
   {
      mpDynamicVB3 = mpFlashDynamicGPUBuffer->createVB(vbBuffer3Size);
      mpDynamicVB3->Lock(0,0, &mpDynamicVB3Buffer, 0);
   }
   else
   {
      mpDynamicVB3 = NULL;
      mpDynamicVB3Buffer = NULL;
   }

   mDynamicIB1Size = ibBuffer1Size;
   mDynamicIB1BytesLeft = ibBuffer1Size;
   mDynamicIB1Offset = 0;
   mIBStartIndex = 0;
   if (ibBuffer1Size)
   {
      mpDynamicIB1 = mpFlashDynamicGPUBuffer->createIB(ibBuffer1Size, D3DFMT_INDEX16);   
      mpDynamicIB1->Lock(0,0, &mpDynamicIB1Buffer, 0);
   }
   else
   {
      mpDynamicIB1 = NULL;
      mpDynamicIB1Buffer = NULL;
   }

   mParamIndex = 0.0f;
   mGlyphParamIndex = 0.0f;
}

//==============================================================================
//==============================================================================
void BFlashRender::renderBatchingUnlock()
{
   if (mpDynamicVB1Buffer) 
   { 
      mpDynamicVB1->Unlock(); 
      mpDynamicVB1Buffer = NULL; 
      mDynamicVB1Size = 0;
      mDynamicVB1BytesLeft = 0;
      mDynamicVB1Offset = 0;
   }
   
   if (mpDynamicVB2Buffer) 
   { 
      mpDynamicVB2->Unlock(); 
      mpDynamicVB2Buffer = NULL; 
      mDynamicVB2Size = 0;
      mDynamicVB2BytesLeft = 0;
      mDynamicVB2Offset = 0;
   }
   
   if (mpDynamicVB3Buffer) 
   { 
      mpDynamicVB3->Unlock(); 
      mpDynamicVB3Buffer = NULL; 
      mDynamicVB3Size = 0;
      mDynamicVB3BytesLeft = 0;
      mDynamicVB3Offset = 0;
   }
   
   if (mpDynamicIB1Buffer) 
   { 
      mpDynamicIB1->Unlock(); 
      mpDynamicIB1Buffer = NULL; 
      mDynamicIB1Size = 0;
      mDynamicIB1BytesLeft = 0;
      mDynamicIB1Offset = 0;
   }
}

//==============================================================================
//==============================================================================
void BFlashRender::renderBatchingEnd()
{  
   mpDevice->SetStreamSource(0, NULL, 0, 0);
   mpDevice->SetStreamSource(1, NULL, 0, 0);
   mpDevice->SetStreamSource(2, NULL, 0, 0);
   mpDevice->SetIndices(NULL);
   mpDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
         
   if (mpFlashDynamicGPUBuffer->getLocked())
      mpFlashDynamicGPUBuffer->unlock();
   
   mBatches.resize(0);
}

//==============================================================================
//==============================================================================
void BFlashRender::renderBatchingFlush()
{
   //BScopedPIXNamedEvent PIXNamedEvent("BFlashRender_renderBatchingFlush");
   
   if (mBatches.isEmpty())
      return;

#if 0
   BTimer timer;
   timer.start();
#endif

   renderBatchingUnlock();
   
   mpDevice->SetRenderState(D3DRS_FILLMODE, mbWireframeEnabled ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
   
#ifndef BUILD_FINAL
   static bool draw = true;
   if (draw)
#endif   
   {
      for (int i = 0; i < mBatches.getNumber(); ++i)
      {
         //-- render the batches and do draws

         switch (mBatches[i].mBatchType)
         {
            case eBatchBeginSubmitMask:
            case eBatchEndSubmitMask:
            case eBatchDisableMask:
               ApplyMaskMode(mBatches[i].mMaskMode, true);
               break;
            case eBatchLineStrip:
               drawLineStripBatch(mBatches[i]);
               break;
            case eBatchTriangleLists:
               drawTriangleListBatch(mBatches[i]);
               break;
            case eBatchBitmaps:
               drawBitmapsBatch(mBatches[i]);
               break;
         }     
         
         if (mBatches[i].mbApplyFillTexture1 && mBatches[i].mFillStyle.mFill.pTexture)
            mBatches[i].mFillStyle.mFill.pTexture->Release();

         if (mBatches[i].mbApplyFillTexture2 && mBatches[i].mFillStyle.mFill2.pTexture)
            mBatches[i].mFillStyle.mFill2.pTexture->Release();
      }
   }
      
   renderBatchingEnd();
         
#if 0
   timer.stop();
   double elapsedTime = timer.getElapsedSeconds() * 1000.0f;
   static bool bTrace = false;
   if (bTrace)
      trace("renderBatchingFlush: %4.3f", elapsedTime);
#endif
}

//==============================================================================
//==============================================================================
void BFlashRender::drawLineStripBatch(BBatchData& b)
{
#ifndef BUILD_FINAL
   static bool bDrawStripBatch = true;
   if (!bDrawStripBatch)
      return;
#endif

   mpDevice->SetVertexDeclaration(mVertexDecls[VD_Strip]);
   mpDevice->SetVertexShader(mVertexShaders[VD_Strip]);

   b.mFillStyle.Apply(this);
   ApplyMatrix(b.mMatrix);

   BASSERT(mpDynamicVB1);
   mpDevice->SetStreamSource(0, mpDynamicVB1, b.mRenderStream0VertexOffset, sizeof(SInt16) * 2);   
   mpDevice->DrawPrimitive(D3DPT_LINELIST, 0, b.mPrimCount); 


   mpDevice->SetVertexDeclaration(NULL);
   mpDevice->SetStreamSource(0, NULL, 0, 0);
   mpDevice->SetStreamSource(1, NULL, 0, 0);
   mpDevice->SetPixelShader(NULL);
   mpDevice->SetVertexShader(NULL);
   mpDevice->SetIndices(NULL); 
   mVDeclIndex   = VD_None;
   mVShaderIndex = VS_None;
   mPShaderIndex = PS_None;

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mDrawCallsPerFrame++;
   gFlashManager.mWorkerStats.mLines+=b.mPrimCount;
#endif
}

//==============================================================================
//==============================================================================
void BFlashRender::drawBitmapsBatch(BBatchData& b)
{
#ifndef BUILD_FINAL
   static bool bDrawBitmapsBatch = true;
   if (!bDrawBitmapsBatch)
      return;
#endif

   mpDevice->SetVertexDeclaration(mVertexDecls[VD_UberGlyphVertex]);
   mpDevice->SetStreamSource(0, mpDynamicVB1, b.mRenderStream0VertexOffset, sizeof(BFlashGlyphUberVertex));
   mpDevice->SetStreamSource(2, mpDynamicVB3, 0, sizeof(BFlashGlyphUberParameterVertex));
   BASSERT(mpDynamicVB1 && mpDynamicVB3);

   ApplyMaskMode(b.mMaskMode, false);
   ApplyBlendMode(b.mBlendMode);
   ApplySampleMode(b.mBitmapSampleMode);

   mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  TRUE);
      
   mTextureParam0 = b.mpTexture;
 
   int techniquePassIndex = 7;
   if (mRenderMode & GViewport::View_AlphaComposite)
   {
      mTechniqueAC.beginRestoreDefaultState();
         mTechniqueAC.beginPass(techniquePassIndex);
            mTechniqueAC.commit();
            mpDevice->DrawVertices(D3DPT_TRIANGLELIST, 0, b.mPrimCount * 6);
         mTechniqueAC.endPass();
      mTechniqueAC.end();
   }
   else
   {
      mTechnique.beginRestoreDefaultState();
         mTechnique.beginPass(techniquePassIndex);
            mTechnique.commit();
            mpDevice->DrawVertices(D3DPT_TRIANGLELIST, 0, b.mPrimCount * 6);
         mTechnique.endPass();
      mTechnique.end();
   }


   mpDevice->SetVertexDeclaration(NULL);
   mpDevice->SetStreamSource(0, NULL, 0, 0);
   mpDevice->SetStreamSource(1, NULL, 0, 0);
   mpDevice->SetPixelShader(NULL);
   mpDevice->SetVertexShader(NULL);
   mpDevice->SetIndices(NULL); 
   mVDeclIndex   = VD_None;
   mVShaderIndex = VS_None;
   mPShaderIndex = PS_None;

   #ifndef BUILD_FINAL
      gFlashManager.mWorkerStats.mPrimitives++;
      gFlashManager.mWorkerStats.mDrawCallsPerFrame++;
      gFlashManager.mWorkerStats.mTriangles+= b.mPrimCount * 2;
   #endif
}

//==============================================================================
//==============================================================================
void BFlashRender::drawTriangleListBatch(BBatchData& b)
{
#ifndef BUILD_FINAL
   static bool bDrawTriangleBatch = true;
   if (!bDrawTriangleBatch)
      return;
#endif

   ApplyMaskMode(b.mMaskMode, false);
   ApplyBlendMode(b.mBlendMode);
   
   int techniquePassIndex = -1;

   static bool bUseCommonRenderData = true;
if (bUseCommonRenderData)
{
   if (b.mbApplyFillTexture1)
   {
      ApplySampleMode(b.mFillStyle.mFill.SampleMode);
      if (!ApplyFillTexture(b.mFillStyle.mFill, 0))
      {
         BDEBUG_ASSERT(0);
         return;
      }
   }

   if (b.mbApplyFillTexture2)
   {
      ApplySampleMode(b.mFillStyle.mFill2.SampleMode);
      if (!ApplyFillTexture(b.mFillStyle.mFill2, 1))
      {
         BDEBUG_ASSERT(0);
         return;
      }
   }
   
   mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, b.mEnableAlphaBlending ? TRUE : FALSE);

   techniquePassIndex = b.mTechniquePassIndex;
}
else
{
   // Non-Edge AA Rendering.

   mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 
      ((b.mFillStyle.mColor.GetAlpha()== 0xFF) &&
      (b.mBlendMode <= GRenderer::Blend_Normal) &&
      (b.mFillStyle.mMode != BFlashFillStyle::FM_Gouraud))  ?  FALSE : TRUE);
   
   if (b.mFillStyle.mMode == BFlashFillStyle::FM_Color)
   {
      techniquePassIndex = 0;
   }
   else if (b.mFillStyle.mMode == BFlashFillStyle::FM_Bitmap)
   {
      ApplySampleMode(b.mFillStyle.mFill.SampleMode);

      if (b.mFillStyle.mFill.pTexture == NULL)
      {
         return;
      }
      else
      {
         mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

         if ((b.mBlendMode == GRenderer::Blend_Multiply) ||
            (b.mBlendMode == GRenderer::Blend_Darken) )
         {
            techniquePassIndex = 1;
         }
         else
         {
            techniquePassIndex = 2;
         }
         
         // Set texture to stage 0; matrix to constants 4 and 5
         if (!ApplyFillTexture(b.mFillStyle.mFill, 0))
         {
            BDEBUG_ASSERT(0);
            return;
         }
      }
   }
   // Edge AA - relies on Gouraud shading and texture mixing
   else if (b.mFillStyle.mMode == BFlashFillStyle::FM_Gouraud)
   {
      BFlashRender::BFlashRenderPixelShaderType shader = BFlashRender::PS_None;
      // No texture: generate color-shaded triangles.
      if (b.mFillStyle.mFill.pTexture == NULL)
      {
         mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
         techniquePassIndex = 0;
         if(b.mVertexFormat == GRenderer::Vertex_XY16iC32)
         {
            shader = BFlashRender::PS_CxformGauraudNoAddAlpha;                        
         }
         else
         {
            shader = BFlashRender::PS_CxformGauraud;
         }         
      }
      // We have a textured or multi-textured gouraud case.
      else
      {   

         mpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
         // Set texture to stage 0;
         if (!ApplyFillTexture(b.mFillStyle.mFill, 0))
         {
            BDEBUG_ASSERT(0);
            return;
         }

         if ((b.mFillStyle.mGouraudType == GRenderer::GFill_1TextureColor) ||
             (b.mFillStyle.mGouraudType == GRenderer::GFill_1Texture))
         {
            techniquePassIndex = 2;
            shader = BFlashRender::PS_CxformGauraudTexture;

         }
         else
         {
            techniquePassIndex = 4;
            shader = BFlashRender::PS_Cxform2Texture;

            // Set second texture to stage 1; matrix to constants 5 and 6
            
            if (!ApplyFillTexture(b.mFillStyle.mFill2, 1))
            {
               BDEBUG_ASSERT(0);
               return;
            }
         }

      }


      if ((b.mBlendMode == GRenderer::Blend_Multiply) ||
         (b.mBlendMode == GRenderer::Blend_Darken) )
      { 
         shader = (BFlashRender::BFlashRenderPixelShaderType)(shader + (BFlashRender::PS_CxformGauraudMultiply - BFlashRender::PS_CxformGauraud));

         // For indexing to work, these should hold:
         GCOMPILER_ASSERT( (BFlashRender::PS_Cxform2Texture - BFlashRender::PS_CxformGauraud) ==
            (BFlashRender::PS_CxformMultiply2Texture - BFlashRender::PS_CxformGauraudMultiply));
         GCOMPILER_ASSERT( (BFlashRender::PS_CxformGauraudMultiply - BFlashRender::PS_CxformGauraud) ==
            (BFlashRender::PS_Cxform2Texture - BFlashRender::PS_CxformGauraud + 1) );

         switch (shader)
         {
            case PS_None:
               techniquePassIndex = -1;
               break;
            case PS_SolidColor:
            case PS_CxformGauraud:
            case PS_CxformGauraudNoAddAlpha:
               techniquePassIndex = 0;
               break;

            case PS_CxformTextureMultiply:
            case PS_CxformGauraudMultiply:
            case PS_CxformGauraudMultiplyNoAddAlpha:
               techniquePassIndex = 1;
               break;

            case PS_CxformTexture:
            case PS_CxformGauraudTexture:
               techniquePassIndex = 2;
               break;

            case PS_CxformGauraudMultiplyTexture:
               techniquePassIndex = 3;
               break;
            
            case PS_TextTexture:
               BASSERT(0);
               break;
                                    
            case PS_Cxform2Texture:
               techniquePassIndex = 4;
               break;

            case PS_CxformMultiply2Texture:
               techniquePassIndex = 5;
               break;
         }         
      }
   }
}

   if (techniquePassIndex == -1)
      return;

   //trace("DrawTriangleBatch: Technique: %d", techniquePassIndex);
   
   mpDevice->SetVertexDeclaration(mVertexDecls[VD_UberVertex]);
   BASSERT(mpDynamicVB1);
   BASSERT(mpDynamicVB2);
   BASSERT(mpDynamicIB1);
   mpDevice->SetStreamSource(0, mpDynamicVB1, b.mRenderStream0VertexOffset, sizeof(BFlashUberVertex));
   mpDevice->SetStreamSource(1, mpDynamicVB2, 0, sizeof(BFlashUberParameterVertex));
   mpDevice->SetIndices(mpDynamicIB1);
      
   int baseVertexIndex = 0;
   int startIndex      = b.mRenderStream0IndexOffset + b.mIndexStartIndex;
   if (mRenderMode & GViewport::View_AlphaComposite)
   {
      mTechniqueAC.beginRestoreDefaultState();
         mTechniqueAC.beginPass(techniquePassIndex);
            mTechniqueAC.commit();
            mpDevice->DrawIndexedVertices(D3DPT_TRIANGLELIST, baseVertexIndex, startIndex, b.mIndexCount);            
         mTechniqueAC.endPass();
      mTechniqueAC.end();
   }
   else
   {
      mTechnique.beginRestoreDefaultState();
         mTechnique.beginPass(techniquePassIndex);
            mTechnique.commit();
            mpDevice->DrawIndexedVertices(D3DPT_TRIANGLELIST, baseVertexIndex, startIndex, b.mIndexCount);
         mTechnique.endPass();
      mTechnique.end();
   }

   mpDevice->SetVertexDeclaration(NULL);
   mpDevice->SetStreamSource(0, NULL, 0, 0);
   mpDevice->SetStreamSource(1, NULL, 0, 0);
   mpDevice->SetPixelShader(NULL);
   mpDevice->SetVertexShader(NULL);
   mpDevice->SetIndices(NULL); 
   mVDeclIndex   = VD_None;
   mVShaderIndex = VS_None;
   mPShaderIndex = PS_None;

   #ifndef BUILD_FINAL
      gFlashManager.mWorkerStats.mDrawCallsPerFrame++;
      gFlashManager.mWorkerStats.mTriangles += (b.mIndexCount / 3);
      gFlashManager.mWorkerStats.mPrimitives++;
   #endif
}

//============================================================================
// GRendererXbox360* GRendererXbox360::CreateRenderer()
// Factory.
//============================================================================
GRendererXbox360* GRendererXbox360::CreateRenderer()
{
   return new BFlashRender();
}

//============================================================================
//============================================================================
BFlashRender* BFlashRender::createRenderer()
{
   return new BFlashRender();
}

//============================================================================
//============================================================================
void BFlashRender::setEnableBatching(bool bEnable)
{
   mbBatchingEnabled = bEnable;
}

//============================================================================
//============================================================================
void BFlashRender::setEnableWireframe(bool bEnable)
{
   mbWireframeEnabled = bEnable;
}

