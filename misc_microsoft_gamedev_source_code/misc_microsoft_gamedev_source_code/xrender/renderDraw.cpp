//============================================================================
//
//  renderDraw.cpp
//  
//  Copyright (c) 2006-2008, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "renderDraw.h"
#include "renderThread.h"
#include "effectIntrinsicManager.h"
#include "consoleOutput.h"
#include "writeTGA.h"
#include "writeTiff.h"
#include "bfileStream.h"
#include "colorUtils.h"
#include "xexception\xexception.h"
#include "dynamicGPUBuffer.h"
#include "JPEGCodec\JPGCodec.h"
#include "file\win32FileUtils.h"

// For now we're not going to have a global dynamic buffer.
const uint cDynamicBufferSize = 0;

BRenderDraw gRenderDraw;

__declspec(thread) BRenderDraw::BThreadData* BRenderDraw::mpThreadData = NULL;

struct BRDClearData
{
   DWORD Count;
   CONST D3DRECT *pRects;
   DWORD Flags;
   D3DCOLOR Color;
   float Z;
   DWORD Stencil;
};

struct BRDClearFData
{
   D3DVECTOR4 Color;
   CONST D3DRECT *pRect;
   DWORD Flags;
   float Z;
   DWORD Stencil;
};

struct BRDPresentData
{
   RECT SourceRect;
   bool HasSourceRect : 1;

   RECT DestRect;
   bool HasDestRect : 1;
};

struct BRDSetRenderTargetData
{
   DWORD RenderTargetIndex;
   IDirect3DSurface9 *pRenderTarget;
};

struct BRDSetDepthStencilSurfaceData
{
   IDirect3DSurface9 *pZStencilSurface;
};

struct BRDSetRenderStateData
{
   D3DRENDERSTATETYPE State;
   DWORD Value;
};

struct BRDSetSamplerStateData
{
   DWORD Sampler;
   D3DSAMPLERSTATETYPE Type;
   DWORD Value;
};

struct BRDSetScissorRectData
{
   RECT Rect;
};

struct BRDSetViewportData
{
   D3DVIEWPORT9 Viewport;
};

struct BRDSetTextureData
{
   DWORD Sampler;
   IDirect3DBaseTexture9 *pTexture;
};

struct BRDSetTextureByHandleData
{
   DWORD mSampler;
   BManagedTextureHandle mTextureHandle;
   eDefaultTexture mDefaultTexture;
};

struct BRDSetStreamSourceData
{
   UINT StreamNumber;
   IDirect3DVertexBuffer9 *pStreamData;
   UINT OffsetInBytes;
   UINT Stride;
};

struct BRDSetIndicesData
{
   IDirect3DIndexBuffer9 *pIndexData;
};

struct BRDDrawVerticesData
{
   D3DPRIMITIVETYPE PrimitiveType;
   UINT StartVertex;
   UINT VertexCount;
};

struct BRDDrawVerticesUPData
{
   D3DPRIMITIVETYPE PrimitiveType;
   UINT VertexCount;
   CONST void *pVertexStreamZeroData;
   UINT VertexStreamZeroStride;
};

struct BRDDrawIndexedVerticesData
{
   D3DPRIMITIVETYPE PrimitiveType;
   INT BaseVertexIndex;
   UINT StartIndex;
   UINT IndexCount;
};

struct BRDDrawIndexedVerticesUPData
{
   D3DPRIMITIVETYPE PrimitiveType;
   UINT MinVertexIndex;
   UINT NumVertices;
   UINT IndexCount;
   CONST void *pIndexData;
   D3DFORMAT IndexDataFormat;
   CONST void *pVertexStreamZeroData;
   UINT VertexStreamZeroStride;
};

struct BRDSetPixelShaderData
{
   IDirect3DPixelShader9 *pShader;
};

struct BRDSetVertexShaderData
{
   IDirect3DVertexShader9 *pShader;
};

struct BRDSetPixelShaderConstantFData
{
   UINT StartRegister;
   CONST float *pConstantData;
   DWORD Vector4fCount;
};

struct BRDSetVertexShaderConstantFData
{
   UINT StartRegister;
   CONST float *pConstantData;
   DWORD Vector4fCount;
};

struct BRDSetVertexDeclarationData
{
   IDirect3DVertexDeclaration9 *pDecl;
};

struct BRDSetFVFData
{
   DWORD FVF;
};

#pragma warning(push)
#pragma warning(disable:4324)
struct BRDResolveData
{
   DWORD Flags;
   D3DRECT SourceRect;
   IDirect3DBaseTexture9 *pDestTexture;
   D3DPOINT DestPoint;
   UINT DestLevel;
   UINT DestSliceOrFace;
   D3DVECTOR4 ClearColor;
   float ClearZ;
   DWORD ClearStencil;
   D3DRESOLVE_PARAMETERS Parameters;
   bool HasSourceRect : 1;
   bool HasDestPoint : 1;
   bool HasClearColor : 1;
   bool HasResolveParameters : 1;
};
#pragma warning(pop)

struct BRDInvalidateGPUCacheData
{
   void *pBaseAddress;
   DWORD Size;
   DWORD Flags;
};

BRenderDraw::BRenderDraw() :
   mpStateBlock(NULL),
   mCommandListenerHandle(cInvalidCommandListenerHandle),
   mRSStackTop(0),
   mpDynamicGPUBuffer(NULL)
{
   Utils::ClearObj(mDisplayMode);
   Utils::ClearObj(mPresentParams);
   Utils::ClearObj(mRSStack);
}

void BRenderDraw::initViewportState(BViewportState& viewportState)
{
   viewportState.mNumViewports = 1;
   viewportState.mVerticalSplit = FALSE;

   BViewportDesc& desc = viewportState.mDesc[0];
   desc.mXOfs = 0;
   desc.mYOfs = 0;
   desc.mWidth = BD3D::mD3DPP.BackBufferWidth;
   desc.mHeight = BD3D::mD3DPP.BackBufferHeight;
   desc.mRenderViewport = mMainActiveRenderViewport;
   desc.mVolumeCuller = mMainActiveVolumeCuller;
   desc.mMatrixTracker = mMainActiveMatrixTracker;
}

bool BRenderDraw::init(void)
{
   ASSERT_MAIN_THREAD
   
   mRSStackTop = 0;
   Utils::ClearObj(mRSStack);

   mMainActiveVolumeCuller.clear();
   mMainSceneVolumeCuller.clear();
   mWorkerActiveVolumeCuller.clear();
   mWorkerSceneVolumeCuller.clear();
   
   mMainActiveMatrixTracker.clear();
   mMainSceneMatrixTracker.clear();
   mWorkerActiveMatrixTracker.clear();
   mWorkerSceneMatrixTracker.clear();
   
   mMainActiveRenderViewport.setSurf(0, getDevBackBuffer());
   mMainActiveRenderViewport.setDepthStencilSurf(getDevDepthStencil());
   
   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = BD3D::mD3DPP.BackBufferWidth;
   viewport.Height = BD3D::mD3DPP.BackBufferHeight;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;
   mMainActiveRenderViewport.setViewport(viewport);
   
   mMainActiveMatrixTracker.setViewport(viewport);
   mMainSceneMatrixTracker = mMainActiveMatrixTracker;
   mWorkerActiveMatrixTracker = mMainActiveMatrixTracker;
   mWorkerSceneMatrixTracker = mMainActiveMatrixTracker;
      
   mMainSceneRenderViewport = mMainActiveRenderViewport;
   mMainSceneRenderViewport = mMainActiveRenderViewport;

   mWorkerActiveRenderViewport = mMainActiveRenderViewport;
   mWorkerSceneRenderViewport = mMainActiveRenderViewport;
         
   delete mpThreadData;
   mpThreadData = new BThreadData;
   
   if (mCommandListenerHandle == cInvalidCommandListenerHandle)
   {
      mCommandListenerHandle = gRenderThread.registerCommandListener(this);
   }
   
   initViewportState(mpThreadData->mViewportState);

   // Now block until the worker thread calls initDeviceData(). This also executes a memory barrier, so we can read the data written by the worker thread.
   gRenderThread.blockUntilWorkerIdle();
                        
   return true;
}

bool BRenderDraw::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   if (mCommandListenerHandle != cInvalidCommandListenerHandle)
   {
      gRenderThread.freeCommandListener(mCommandListenerHandle);
      mCommandListenerHandle = cInvalidCommandListenerHandle;
   }

   // Now block until the worker thread calls deinitDeviceData(). This also executes a memory barrier, so we can read the data written by the worker thread.
   gRenderThread.blockUntilWorkerIdle();
  
   return true;
}

void BRenderDraw::initDeviceData(void)
{
   ASSERT_RENDER_THREAD
   
   BD3D::checkHResult(BD3D::mpDev->CreateStateBlock(D3DSBT_ALL, &mpStateBlock));
   BD3D::mpDev->GetDisplayMode(0, &mDisplayMode);
   
   mPresentParams = BD3D::mD3DPP;
   
   delete mpThreadData;
   mpThreadData = new BThreadData;
            
   if (cDynamicBufferSize)
   {
      BDEBUG_ASSERT(!mpDynamicGPUBuffer);
      mpDynamicGPUBuffer = ALIGNED_NEW(BDynamicGPUBuffer, gRenderHeap);
      mpDynamicGPUBuffer->init(cDynamicBufferSize);
   }
   
   initViewportState(mpThreadData->mViewportState);
}

void BRenderDraw::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD
   if (mpStateBlock)
   {
      mpStateBlock->Release();
      mpStateBlock = NULL;
   }
   
   if (mpDynamicGPUBuffer)
   {
      ALIGNED_DELETE(mpDynamicGPUBuffer, gRenderHeap);
      mpDynamicGPUBuffer = NULL;
   }
}

void BRenderDraw::workerUnsetResources(DWORD flags)
{
   // Unset resources in case they point into CPU frame storage.
   if (flags & cUnsetIndices)
      BD3D::mpDev->SetIndices(NULL);

   if (flags & cUnsetStreams)
   {
      for (uint i = 0; i < D3DMAXSTREAMS; i++)
         BD3D::mpDev->SetStreamSource(i, NULL, 0, 0);
   }         

   if (flags & cUnsetTextures)
   {
      for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
         BD3D::mpDev->SetTexture(i, NULL);
   }         

   if (flags & cUnsetShaders)
   {
      BD3D::mpDev->SetVertexShader(NULL);
      BD3D::mpDev->SetPixelShader(NULL);
   }

   if (flags & cUnsetRenderTargets)
   {
      BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);

      for (uint i = 1; i < 4; i++)
         BD3D::mpDev->SetRenderTarget(i, NULL);
   }         

   if (flags & cUnsetDepthStencil)
      BD3D::mpDev->SetDepthStencilSurface(BD3D::mpDevDepthStencil);      
   
   if (flags & cUnsetStateBlock)
      mpStateBlock->Capture();
}

void BRenderDraw::frameBegin(void)
{
}

void BRenderDraw::frameEnd(void)
{
   workerUnsetResources();
   
   if (mpDynamicGPUBuffer)
   {
      BASSERT(!mpDynamicGPUBuffer->getLocked());
      
      mpDynamicGPUBuffer->flushPendingFences();
   }
}

static DWORD floatToDWORD(float f)
{
   return *reinterpret_cast<const DWORD*>(&f);
}

void BRenderDraw::workerSetDefaultRenderStates(IDirect3DDevice9* pDev)
{
   if (!pDev)
      pDev = BD3D::mpDev;

   // Not bothering with the inline version to cut down code bloat.
   //#define SET_RS(s, v) { DWORD d; BD3D::mpDev->GetRenderState(s, &d); if (d != v) { trace("state 0x%X is 0x%X not 0x%X", s, d, v); } BD3D::mpDev->SetRenderState(s, v); }
   #define SET_RS(s, v) pDev->SetRenderState(s, v);
   
   SET_RS(D3DRS_ZENABLE, TRUE)
   SET_RS(D3DRS_ZFUNC, D3DCMP_LESSEQUAL)
   SET_RS(D3DRS_ZWRITEENABLE, TRUE)
   SET_RS(D3DRS_FILLMODE, D3DFILL_SOLID)
   SET_RS(D3DRS_CULLMODE, D3DCULL_CCW)
   SET_RS(D3DRS_ALPHABLENDENABLE, FALSE)
   SET_RS(D3DRS_SEPARATEALPHABLENDENABLE, FALSE)
   SET_RS(D3DRS_BLENDFACTOR, 0xFFFFFFFF)
   SET_RS(D3DRS_SRCBLEND, D3DBLEND_ONE)
   SET_RS(D3DRS_DESTBLEND, D3DBLEND_ZERO)
   SET_RS(D3DRS_BLENDOP, D3DBLENDOP_ADD)
   SET_RS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE)
   SET_RS(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO)
   SET_RS(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD)
   SET_RS(D3DRS_ALPHATESTENABLE, FALSE)
   SET_RS(D3DRS_ALPHAREF, 0)
   SET_RS(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS)
   SET_RS(D3DRS_STENCILENABLE, FALSE)
   SET_RS(D3DRS_TWOSIDEDSTENCILMODE, FALSE)
   SET_RS(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP)
   SET_RS(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP)
   SET_RS(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP)
   SET_RS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS)
   SET_RS(D3DRS_STENCILREF, 0)
   SET_RS(D3DRS_STENCILMASK, 0xFF)
   SET_RS(D3DRS_STENCILWRITEMASK, 0xFF)
   SET_RS(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP)
   SET_RS(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP)
   SET_RS(D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP)
   SET_RS(D3DRS_CCW_STENCILFUNC,  D3DCMP_ALWAYS)
   SET_RS(D3DRS_CCW_STENCILREF, 0)
   SET_RS(D3DRS_CCW_STENCILMASK, 0xFF)
   SET_RS(D3DRS_CCW_STENCILWRITEMASK, 0xFF)
   SET_RS(D3DRS_CLIPPLANEENABLE, 0)
   SET_RS(D3DRS_POINTSIZE, floatToDWORD(1.0f))
   SET_RS(D3DRS_POINTSIZE_MIN, floatToDWORD(1.0f))
   SET_RS(D3DRS_POINTSPRITEENABLE, FALSE)
   SET_RS(D3DRS_POINTSIZE_MAX, floatToDWORD(64.0f))
   SET_RS(D3DRS_SCISSORTESTENABLE, FALSE)
   SET_RS(D3DRS_SLOPESCALEDEPTHBIAS, 0)
   SET_RS(D3DRS_DEPTHBIAS, 0)
   SET_RS(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL)
   SET_RS(D3DRS_COLORWRITEENABLE1, D3DCOLORWRITEENABLE_ALL)
   SET_RS(D3DRS_COLORWRITEENABLE2, D3DCOLORWRITEENABLE_ALL)
   SET_RS(D3DRS_COLORWRITEENABLE3, D3DCOLORWRITEENABLE_ALL)
//   SET_RS(D3DRS_SRGBWRITEENABLE, 0)
   SET_RS(D3DRS_TESSELLATIONMODE, D3DTM_CONTINUOUS)
   SET_RS(D3DRS_MINTESSELLATIONLEVEL, floatToDWORD(1.0f))
   SET_RS(D3DRS_MAXTESSELLATIONLEVEL, floatToDWORD(1.0f))
   
   SET_RS(D3DRS_VIEWPORTENABLE, TRUE)
   SET_RS(D3DRS_HIGHPRECISIONBLENDENABLE, FALSE)
   SET_RS(D3DRS_HIGHPRECISIONBLENDENABLE1, FALSE)
   SET_RS(D3DRS_HIGHPRECISIONBLENDENABLE2, FALSE)
   SET_RS(D3DRS_HIGHPRECISIONBLENDENABLE3, FALSE)
   SET_RS(D3DRS_HALFPIXELOFFSET, FALSE)
   SET_RS(D3DRS_PRIMITIVERESETENABLE, FALSE)
   SET_RS(D3DRS_PRIMITIVERESETINDEX, 0xFFFF)
   SET_RS(D3DRS_ALPHATOMASKENABLE, FALSE)
   SET_RS(D3DRS_ALPHATOMASKOFFSETS, 135)
   SET_RS(D3DRS_GUARDBAND_X, floatToDWORD(2.0f))
   SET_RS(D3DRS_GUARDBAND_Y, floatToDWORD(2.0f))
   SET_RS(D3DRS_DISCARDBAND_X, floatToDWORD(1.0f))
   SET_RS(D3DRS_DISCARDBAND_Y, floatToDWORD(1.0f))
   SET_RS(D3DRS_HISTENCILENABLE, FALSE)
   SET_RS(D3DRS_HISTENCILWRITEENABLE, FALSE)
   SET_RS(D3DRS_HISTENCILFUNC, D3DHSCMP_EQUAL)
   SET_RS(D3DRS_HISTENCILREF, 0)
      
   #undef SET_RENDER_STATE
}

void BRenderDraw::workerSetDefaultSamplerStates(IDirect3DDevice9* pDev)
{
   if (!pDev)
      pDev = BD3D::mpDev;
   
   for (uint sampler = 0; sampler < cMaxD3DTextureSamplers; sampler++)
   {
      //#define SET_SS(s, v) { DWORD d; pDev->GetSamplerState(sampler, s, &d); if (d != v) { trace("sampler 0x%X state 0x%X is 0x%X not 0x%X", sampler, s, d, v); } pDev->SetSamplerState(sampler, s, v); }
      #define SET_SS(s, v) pDev->SetSamplerState(sampler, s, v); 
         
      SET_SS(D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP)
      SET_SS(D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP)
      SET_SS(D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP)
      SET_SS(D3DSAMP_BORDERCOLOR, 0)
      SET_SS(D3DSAMP_MAGFILTER, D3DTEXF_POINT)
      SET_SS(D3DSAMP_MINFILTER, D3DTEXF_POINT)
      SET_SS(D3DSAMP_MIPFILTER, D3DTEXF_NONE)
      SET_SS(D3DSAMP_MIPMAPLODBIAS, 0)
      SET_SS(D3DSAMP_MAXMIPLEVEL, 0)
      SET_SS(D3DSAMP_MAXANISOTROPY, 1)
      SET_SS(D3DSAMP_MAGFILTERZ, D3DTEXF_POINT)
      SET_SS(D3DSAMP_MINFILTERZ, D3DTEXF_POINT)
      SET_SS(D3DSAMP_SEPARATEZFILTERENABLE, FALSE)
      SET_SS(D3DSAMP_MINMIPLEVEL, 13)
      SET_SS(D3DSAMP_TRILINEARTHRESHOLD, D3DTRILINEAR_IMMEDIATE)
      SET_SS(D3DSAMP_ANISOTROPYBIAS, 0x80000000)
      SET_SS(D3DSAMP_HGRADIENTEXPBIAS, 0)
      SET_SS(D3DSAMP_VGRADIENTEXPBIAS, 0)
      SET_SS(D3DSAMP_WHITEBORDERCOLORW, FALSE)
      SET_SS(D3DSAMP_POINTBORDERENABLE, TRUE)
   }
}

void BRenderDraw::checkForFailOrMeltdown(void)
{
#ifndef BUILD_FINAL
   if ((gAssertionSystem.getIsHandlingFailure()) || (X_Exception::GetHandlingMeltdown()))
   {
      for ( ; ; )
      {
         Sleep(100);
 
         if ((!gAssertionSystem.getIsHandlingFailure()) && (!X_Exception::GetHandlingMeltdown()))
            break;
      }
   }
#endif          
}

void BRenderDraw::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD
   switch (header.mType)
   {
      case cRDSetSceneRenderViewport:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderViewport));
         mWorkerSceneRenderViewport = *reinterpret_cast<const BRenderViewport*>(pData);
         break;
      }
      case cRDSetActiveRenderViewport:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderViewport));
         mWorkerActiveRenderViewport = *reinterpret_cast<const BRenderViewport*>(pData);
         mWorkerActiveRenderViewport.setToDevice();
         break;
      }
      case cRDSetSceneMatrixTracker:
      {  
         BDEBUG_ASSERT(header.mLen == BMatrixTracker::getSerializeSize());
         mWorkerSceneMatrixTracker.deserialize(pData);
         break;
      }
      case cRDSetActiveMatrixTracker:
      {
         BDEBUG_ASSERT(header.mLen == BMatrixTracker::getSerializeSize());
         mWorkerActiveMatrixTracker.deserialize(pData);
         break;
      }
      case cRDSetSceneVolumeCuller:
      {
         const uint size = mWorkerSceneVolumeCuller.deserialize(pData);
         size;
         BDEBUG_ASSERT(header.mLen == size);
         break;
      }
      case cRDSetActiveVolumeCuller:
      {
         const uint size = mWorkerActiveVolumeCuller.deserialize(pData);
         size;
         BDEBUG_ASSERT(header.mLen == size);
         break;
      }
      case cRDStateBlockCapture:
      {
         mpStateBlock->Capture();
         break;
      }
      case cRDStateBlockApply:
      {
         mpStateBlock->Apply();
         break;
      }
      case cRDSetRenderTarget:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetRenderTargetData));
         const BRDSetRenderTargetData* p = reinterpret_cast<const BRDSetRenderTargetData*>(pData);
         BD3D::mpDev->SetRenderTarget(p->RenderTargetIndex, p->pRenderTarget);
         break;
      }
      case cRDSetDepthStencilSurface:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetDepthStencilSurfaceData));
         const BRDSetDepthStencilSurfaceData* p = reinterpret_cast<const BRDSetDepthStencilSurfaceData*>(pData);
         BD3D::mpDev->SetDepthStencilSurface(p->pZStencilSurface);
         break;
      }
      case cRDSetRenderState:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetRenderStateData));
         const BRDSetRenderStateData* p = reinterpret_cast<const BRDSetRenderStateData*>(pData);
         BD3D::mpDev->SetRenderState(p->State, p->Value);
         break;
      }
      case cRDSetSamplerState:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetSamplerStateData));
         const BRDSetSamplerStateData* p = reinterpret_cast<const BRDSetSamplerStateData*>(pData);
         BD3D::mpDev->SetSamplerState(p->Sampler, p->Type, p->Value);
         break;
      }
      case cRDSetScissorRect:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetScissorRectData));
         const BRDSetScissorRectData* p = reinterpret_cast<const BRDSetScissorRectData*>(pData);
         BD3D::mpDev->SetScissorRect(&p->Rect);
         break;
      }
      case cRDSetViewport:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetViewportData));
         const BRDSetViewportData* p = reinterpret_cast<const BRDSetViewportData*>(pData);
         BD3D::mpDev->SetViewport(&p->Viewport);
         break;
      }
      case cRDSetTexture:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetTextureData));
         const BRDSetTextureData* p = reinterpret_cast<const BRDSetTextureData*>(pData);
         BD3D::mpDev->SetTexture(p->Sampler, p->pTexture);
         break;
      }
      case cRDSetStreamSource:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetStreamSourceData));
         const BRDSetStreamSourceData* p = reinterpret_cast<const BRDSetStreamSourceData*>(pData);
         BD3D::mpDev->SetStreamSource(p->StreamNumber, p->pStreamData, p->OffsetInBytes, p->Stride);
         break;
      }      
      case cRDClearStreamSource:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(uint));
         BD3D::mpDev->SetStreamSource(*reinterpret_cast<const uint*>(pData), 0, 0, 0);
         break;
      }      
      case cRDSetIndices:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetIndicesData));
         const BRDSetIndicesData* p = reinterpret_cast<const BRDSetIndicesData*>(pData);
         BD3D::mpDev->SetIndices(p->pIndexData);
         break;
      }
      case cRDDrawVertices:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDDrawVerticesData));
         const BRDDrawVerticesData* p = reinterpret_cast<const BRDDrawVerticesData*>(pData);
         BD3D::mpDev->DrawVertices(p->PrimitiveType, p->StartVertex, p->VertexCount);
         break;
      }
      case cRDDrawVerticesUP:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDDrawVerticesUPData));
         const BRDDrawVerticesUPData* p = reinterpret_cast<const BRDDrawVerticesUPData*>(pData);
         BD3D::mpDev->DrawVerticesUP(p->PrimitiveType, p->VertexCount, p->pVertexStreamZeroData, p->VertexStreamZeroStride);
         break;
      }
      case cRDDrawIndexedVertices:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDDrawIndexedVerticesData));
         const BRDDrawIndexedVerticesData* p = reinterpret_cast<const BRDDrawIndexedVerticesData*>(pData);
         BD3D::mpDev->DrawIndexedVertices(p->PrimitiveType, p->BaseVertexIndex, p->StartIndex, p->IndexCount);
         break;
      }
      case cRDDrawIndexedVerticesUP:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDDrawIndexedVerticesUPData));
         const BRDDrawIndexedVerticesUPData* p = reinterpret_cast<const BRDDrawIndexedVerticesUPData*>(pData);
         BD3D::mpDev->DrawIndexedVerticesUP(
            p->PrimitiveType, p->MinVertexIndex, p->NumVertices, 
            p->IndexCount, 
            p->pIndexData, p->IndexDataFormat, 
            p->pVertexStreamZeroData, p->VertexStreamZeroStride);
         break;
      }
      case cRDSetPixelShader:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetPixelShaderData));
         const BRDSetPixelShaderData* p = reinterpret_cast<const BRDSetPixelShaderData*>(pData);
         BD3D::mpDev->SetPixelShader(p->pShader);
         break;
         break;
      }
      case cRDSetVertexShader:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetVertexShaderData));
         const BRDSetVertexShaderData* p = reinterpret_cast<const BRDSetVertexShaderData*>(pData);
         BD3D::mpDev->SetVertexShader(p->pShader);
         break;
      }
      case cRDSetPixelShaderConstantF:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetPixelShaderConstantFData));
         const BRDSetPixelShaderConstantFData* p = reinterpret_cast<const BRDSetPixelShaderConstantFData*>(pData);
         BD3D::mpDev->SetPixelShaderConstantF(p->StartRegister, p->pConstantData, p->Vector4fCount);
         break;
      }
      case cRDSetVertexShaderConstantF:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetVertexShaderConstantFData));
         const BRDSetVertexShaderConstantFData* p = reinterpret_cast<const BRDSetVertexShaderConstantFData*>(pData);
         BD3D::mpDev->SetVertexShaderConstantF(p->StartRegister, p->pConstantData, p->Vector4fCount);
         break;
      }
      case cRDSetVertexDeclaration:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetVertexDeclarationData));
         const BRDSetVertexDeclarationData* p = reinterpret_cast<const BRDSetVertexDeclarationData*>(pData);
         BD3D::mpDev->SetVertexDeclaration(p->pDecl);
         break;
      }
      case cRDSetFVF:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDSetFVFData));
         const BRDSetFVFData* p = reinterpret_cast<const BRDSetFVFData*>(pData);
         BD3D::mpDev->SetFVF(p->FVF);
         break;
      }
      case cRDResolve:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDResolveData));
         
         const BRDResolveData* p = reinterpret_cast<const BRDResolveData*>(pData);
         
         D3DVECTOR4 clearColor;
         if (p->HasClearColor)
            memcpy(&clearColor, &p->ClearColor, sizeof(D3DVECTOR4));
            
         BD3D::mpDev->Resolve(
            p->Flags,
            p->HasSourceRect ? &p->SourceRect : NULL,
            p->pDestTexture,
            p->HasDestPoint ? &p->DestPoint : NULL,
            p->DestLevel,
            p->DestSliceOrFace,
            p->HasClearColor ? &clearColor : NULL,
            p->ClearZ,
            p->ClearStencil,
            p->HasResolveParameters ? &p->Parameters : NULL);
            
         break;
      }
      case cRDUnsetAll:
      {
         BDEBUG_ASSERT(header.mLen == 0);
         BD3D::mpDev->UnsetAll();
         break;
      }
      case cRDPresent:
      {
         gEventDispatcher.dispatchSynchronousEvents();
         
         checkForFailOrMeltdown();
         
         const BRDPresentData* p = reinterpret_cast<const BRDPresentData*>(pData);
         {
            SCOPEDSAMPLE(Present)
            BD3D::mpDev->Present(p->HasSourceRect ? &p->SourceRect : NULL, p->HasDestRect ? &p->DestRect : NULL, 0, 0);
         }
         
         gRenderThread.processPresent();
         
         break;
      }
      case cRDBeginScene:
      {
         BD3D::mpDev->BeginScene();
         BDEBUG_ASSERT(!mpThreadData->mBegunScene);
         mpThreadData->mBegunScene = true;
         break;
      }
      case cRDEndScene:
      {
         BD3D::mpDev->EndScene();
         BDEBUG_ASSERT(mpThreadData->mBegunScene);
         mpThreadData->mBegunScene = false;
         break;
      }
      case cRDClear:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDClearData));
         const BRDClearData* p = reinterpret_cast<const BRDClearData*>(pData);
         BD3D::mpDev->Clear(p->Count, p->pRects, p->Flags, p->Color, p->Z, p->Stencil);
                          
         break;
      }
      case cRDClearF:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDClearFData));
         const BRDClearFData* p = reinterpret_cast<const BRDClearFData*>(pData);
         
         // Make a local copy due to alignment
         D3DVECTOR4 clearColor;
         memcpy(&clearColor, &p->Color, sizeof(D3DVECTOR4));
         BD3D::mpDev->ClearF(p->Flags, p->pRect, &clearColor, p->Z, p->Stencil);

         break;
      }
      case cRDReleaseResource:     //(D3DResource* pResource);
      {
         BDEBUG_ASSERT(header.mLen == sizeof(D3DResource*));
         D3DResource* pResource = *reinterpret_cast<D3DResource* const*>(pData);
         //BDEBUG_ASSERT(!pResource->IsSet(BD3D::mpDev));
         pResource->Release();
         break;
      }
      case cRDReleaseVertexShader: //(IDirect3DVertexShader9* pResource);
      {
         BDEBUG_ASSERT(header.mLen == sizeof(IDirect3DVertexShader9*));
         IDirect3DVertexShader9* pResource = *reinterpret_cast<IDirect3DVertexShader9* const*>(pData);
         //BDEBUG_ASSERT(!pResource->IsSet(BD3D::mpDev));
         pResource->Release();
         break;
      }
      case cRDReleasePixelShader:  //(IDirect3DPixelShader9* pResource);
      {
         BDEBUG_ASSERT(header.mLen == sizeof(IDirect3DPixelShader9*));
         IDirect3DPixelShader9* pResource = *reinterpret_cast<IDirect3DPixelShader9* const*>(pData);
         //BDEBUG_ASSERT(!pResource->IsSet(BD3D::mpDev));
         pResource->Release();
         break;
      }
      case cRDReleaseVertexDecl:   //(IDirect3DVertexDeclaration9* pResource)
      {
         BDEBUG_ASSERT(header.mLen == sizeof(IDirect3DVertexDeclaration9*));
         IDirect3DVertexDeclaration9* pResource = *reinterpret_cast<IDirect3DVertexDeclaration9* const*>(pData);
         //BDEBUG_ASSERT(!pResource->IsSet(BD3D::mpDev));
         pResource->Release();
         break;
      }
      case cRDInvalidateGPUCache:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRDInvalidateGPUCacheData));
         const BRDInvalidateGPUCacheData* p = reinterpret_cast<const BRDInvalidateGPUCacheData*>(pData);
         
         BD3D::mpDev->InvalidateGpuCache(p->pBaseAddress, p->Size, p->Flags);
         break;
      }
      case cRDSetDefaultRenderStates:
      {
         workerSetDefaultRenderStates();
         break;
      }
      case cRDSetDefaultSamplerStates:
      {
         workerSetDefaultSamplerStates();
         break;
      }
      case cRDUnsetResources:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(DWORD));
         workerUnsetResources(*reinterpret_cast<const DWORD*>(pData));
         break;
      }
      case cRDUnsetTextures:
      {
         workerUnsetTextures();
         break;
      }
      case cRDSaveScreenshot:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(const char* const*));
         const char* pFilename = *reinterpret_cast<const char* const*>(pData);
         workerSaveScreenshot(pFilename);
         break;
      }
      default:
      {
         BVERIFY(false);
         break;
      }
   }
}

IDirect3DBaseTexture9* BRenderDraw::getDevFrontBuffer(void) const
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDevFrontBuffer;
}

IDirect3DSurface9* BRenderDraw::getDevBackBuffer(void) const
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDevBackBuffer;
}

IDirect3DSurface9* BRenderDraw::getDevDepthStencil(void) const
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDevDepthStencil;
}

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createArrayTexture(
   UINT Width,
   UINT Height,
   UINT ArraySize,
   UINT Levels,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DArrayTexture9 **ppArrayTexture,
   HANDLE *pUnusedSharedHandle)
{
   BDEBUG_ASSERT(BD3D::mpDev);

   return BD3D::mpDev->CreateArrayTexture(
      Width,
      Height,
      ArraySize,
      Levels,
      Usage,
      Format,
      UnusedPool,
      ppArrayTexture,
      pUnusedSharedHandle);
}                           

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createCubeTexture(
   UINT EdgeLength,
   UINT Levels,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DCubeTexture9 **ppCubeTexture,
   HANDLE *pUnusedSharedHandle)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateCubeTexture(
      EdgeLength,
      Levels,
      Usage,
      Format,
      UnusedPool,
      ppCubeTexture,
      pUnusedSharedHandle);
}                          

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createDepthStencilSurface(
   UINT Width,
   UINT Height,
   D3DFORMAT Format,
   D3DMULTISAMPLE_TYPE MultiSample,
   DWORD UnusedMultisampleQuality,
   BOOL UnusedDiscard,
   IDirect3DSurface9 **ppSurface,
   CONST D3DSURFACE_PARAMETERS *pParameters)
{
   BDEBUG_ASSERT(BD3D::mpDev);   
   return BD3D::mpDev->CreateDepthStencilSurface(Width,
      Height,
      Format,
      MultiSample,
      UnusedMultisampleQuality,
      UnusedDiscard,
      ppSurface,
      pParameters);
}                                  

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createIndexBuffer(
   UINT Length,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DIndexBuffer9 **ppIndexBuffer,
   HANDLE *pUnusedSharedHandle)
{

   BDEBUG_ASSERT(BD3D::mpDev);   
   return BD3D::mpDev->CreateIndexBuffer(
      Length,
      Usage,
      Format,
      UnusedPool,
      ppIndexBuffer,
      pUnusedSharedHandle);
}                          

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createPixelShader(
   CONST DWORD *pFunction,
   IDirect3DPixelShader9 **ppShader)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreatePixelShader(
      pFunction,
      ppShader);
}                          

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createRenderTarget(
   UINT Width,
   UINT Height,
   D3DFORMAT Format,
   D3DMULTISAMPLE_TYPE MultiSample,
   DWORD UnusedMultisampleQuality,
   BOOL UnusedLockable,
   IDirect3DSurface9 **ppSurface,
   CONST D3DSURFACE_PARAMETERS *pParameters)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   HRESULT hres = BD3D::mpDev->CreateRenderTarget(
      Width,
      Height,
      Format,
      MultiSample,
      UnusedMultisampleQuality,
      UnusedLockable,
      ppSurface,
      pParameters);
   
   if (FAILED(hres))
      return hres;
   
   #if _XDK_VER == 5426
   (*ppSurface)->Identifier = 0;
   (*ppSurface)->ReadFence = 0;
   (*ppSurface)->Fence = 0;
   #endif
           
   return hres;      
}                           

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createLineTexture(
                          UINT Width,
                          UINT Levels,
                          DWORD Usage,
                          D3DFORMAT Format,
                          D3DPOOL UnusedPool,
                          IDirect3DLineTexture9 **ppTexture,
                          HANDLE *pUnusedSharedHandle)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateLineTexture(Width, Levels, Usage, Format, UnusedPool, ppTexture, pUnusedSharedHandle);
}                          

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createTexture(
                                     UINT Width,
                                     UINT Height,
                                     UINT Levels,
                                     DWORD Usage,
                                     D3DFORMAT Format,
                                     D3DPOOL UnusedPool,
                                     IDirect3DTexture9 **ppTexture,
                                     HANDLE *pUnusedSharedHandle)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateTexture(
      Width,
      Height,
      Levels,
      Usage,
      Format,
      UnusedPool,
      ppTexture,
      pUnusedSharedHandle);
}                      

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createVertexBuffer(
   UINT Length,
   DWORD Usage,
   DWORD UnusedFVF,
   D3DPOOL UnusedPool,
   IDirect3DVertexBuffer9 **ppVertexBuffer,
   HANDLE *pUnusedSharedHandle)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateVertexBuffer(
      Length,
      Usage,
      UnusedFVF,
      UnusedPool,
      ppVertexBuffer,
      pUnusedSharedHandle);
}                           

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createVertexDeclaration(
   CONST D3DVERTEXELEMENT9 *pVertexElements,
   IDirect3DVertexDeclaration9 **ppVertexDeclaration)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateVertexDeclaration(
      pVertexElements,
      ppVertexDeclaration);
}                                

// releaseD3DResource must be called to release the returned object.   
HRESULT BRenderDraw::createVertexShader(
   CONST DWORD *pFunction,
   IDirect3DVertexShader9 **ppShader)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateVertexShader(
      pFunction,
      ppShader);
}                          

// releaseD3DResource must be called to release the returned object.
HRESULT BRenderDraw::createVolumeTexture(
   UINT Width,
   UINT Height,
   UINT Depth,
   UINT Levels,
   DWORD Usage,
   D3DFORMAT Format,
   D3DPOOL UnusedPool,
   IDirect3DVolumeTexture9 **ppVolumeTexture,
   HANDLE *pUnusedSharedHandle)
{
   BDEBUG_ASSERT(BD3D::mpDev);
   return BD3D::mpDev->CreateVolumeTexture(
      Width,
      Height,
      Depth,
      Levels,
      Usage,
      Format,
      UnusedPool,
      ppVolumeTexture,
      pUnusedSharedHandle);
}                            

void BRenderDraw::releaseD3DResource(D3DResource* pResource, bool releaseInWorkerThread)
{
   if (!pResource)
      return;
   
   BDEBUG_ASSERT(BD3D::mpDev);
      
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      pResource->Release();
   else
   {
      ASSERT_MAIN_THREAD
      
      if (releaseInWorkerThread)   
      {
         gRenderThread.submitCommand(mCommandListenerHandle, cRDReleaseResource, pResource);
      }
      else
      {
         // The docs say we must insert a fence into the command buffer before releasing a resource from another thread.
         gRenderThread.submitCommand(cRCCControl, cRCKickPushBuffer);
         gRenderThread.blockUntilWorkerIdle();
         pResource->Release();
      }
   }      
}

void BRenderDraw::releaseD3DResource(IDirect3DVertexShader9* pResource, bool releaseInWorkerThread)
{
   if (!pResource)
      return;
      
   BDEBUG_ASSERT(BD3D::mpDev);      
      
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      pResource->Release();
   else
   {
      ASSERT_MAIN_THREAD
      
      if (releaseInWorkerThread)   
      {
         gRenderThread.submitCommand(mCommandListenerHandle, cRDReleaseVertexShader, pResource);
      }
      else
      {
         // The docs say we must insert a fence into the command buffer before releasing a resource from another thread.
         gRenderThread.submitCommand(cRCCControl, cRCKickPushBuffer);
         gRenderThread.blockUntilWorkerIdle();
         pResource->Release();
      }
   }      
}

void BRenderDraw::releaseD3DResource(IDirect3DPixelShader9* pResource, bool releaseInWorkerThread)
{
   if (!pResource)
      return;
   
   BDEBUG_ASSERT(BD3D::mpDev);
      
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      pResource->Release();
   else
   {
      ASSERT_MAIN_THREAD
      
      if (releaseInWorkerThread)   
      {
         gRenderThread.submitCommand(mCommandListenerHandle, cRDReleasePixelShader, pResource);
      }
      else
      {
         // The docs say we must insert a fence into the command buffer before releasing a resource from another thread.
         gRenderThread.submitCommand(cRCCControl, cRCKickPushBuffer);
         gRenderThread.blockUntilWorkerIdle();
         pResource->Release();
      }
   }      
}

void BRenderDraw::releaseD3DResource(IDirect3DVertexDeclaration9* pResource, bool releaseInWorkerThread)
{
   if (!pResource)
      return;
   
   BDEBUG_ASSERT(BD3D::mpDev);
      
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      pResource->Release();
   else
   {
      ASSERT_MAIN_THREAD
      
      if (releaseInWorkerThread)   
      {
         gRenderThread.submitCommand(mCommandListenerHandle, cRDReleaseVertexDecl, pResource);
      }
      else
      {
         // The docs say we must insert a fence into the command buffer before releasing a resource from another thread.
         gRenderThread.submitCommand(cRCCControl, cRCKickPushBuffer);
         gRenderThread.blockUntilWorkerIdle();
         pResource->Release();
      }
   }      
}

void BRenderDraw::stateBlockCapture(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      mpStateBlock->Capture();
   else
      gRenderThread.submitCommand(mCommandListenerHandle, cRDStateBlockCapture);
}

void BRenderDraw::stateBlockApply(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      mpStateBlock->Apply();   
   else
      gRenderThread.submitCommand(mCommandListenerHandle, cRDStateBlockApply);
}

void BRenderDraw::setRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9 *pRenderTarget)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetRenderTarget(RenderTargetIndex, pRenderTarget);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetRenderTargetData data;
      data.RenderTargetIndex = RenderTargetIndex;
      data.pRenderTarget = pRenderTarget;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetRenderTarget, data);
   }      
}

void BRenderDraw::setDepthStencilSurface(IDirect3DSurface9 *pZStencilSurface)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetDepthStencilSurface(pZStencilSurface);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetDepthStencilSurfaceData data;
      data.pZStencilSurface = pZStencilSurface;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetDepthStencilSurface, data);
   }      
}

void BRenderDraw::setRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetRenderState(State, Value);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      
      BRDSetRenderStateData data;
      data.State = State;
      data.Value = Value;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetRenderState, data);
   }
}

void BRenderDraw::setSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetSamplerState(Sampler, Type, Value);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      
      BRDSetSamplerStateData data;
      data.Sampler = Sampler;
      data.Type = Type;
      data.Value = Value;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetSamplerState, data);
   }      
}

void BRenderDraw::setScissorRect(CONST RECT *pRect)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetScissorRect(pRect);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      
      BRDSetScissorRectData data;
      data.Rect = *checkNull(pRect);
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetScissorRect, data);
   }      
}

void BRenderDraw::setViewport(CONST D3DVIEWPORT9* pViewport)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetViewport(pViewport);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetViewportData data;
      data.Viewport = *checkNull(pViewport);
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetViewport, data);
   }      
}

void BRenderDraw::setTexture(DWORD Sampler, IDirect3DBaseTexture9 *pTexture)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetTexture(Sampler, pTexture);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetTextureData data;
      data.Sampler = Sampler;
      data.pTexture = pTexture;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetTexture, data);
   }      
}

void BRenderDraw::setTextureByHandle(DWORD Sampler, BManagedTextureHandle textureHandle, eDefaultTexture defaultTexture)
{
   ASSERT_MAIN_OR_WORKER_THREAD
      
      //CLM [11.11.08] - this isn't a proper validity check.
      // The texture manager can return a unique handle to a texture that doesn't get loaded
      // on the render thread. So then the sim thread will pass in a handle which it thinks is uniuqe, and valid
      // but is actually not. We've bubbled up the extra isValidHandle() call at the caller levels (above this)
      // but if this keeps happening, we may want to propigate it to this.
   if (textureHandle == cInvalidManagedTextureHandle)
      textureHandle = gD3DTextureManager.getDefaultTextureHandle(defaultTexture);
   
   gD3DTextureManager.setManagedTextureByHandle(textureHandle, Sampler);
}

void BRenderDraw::setStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetStreamSourceData data;
      data.StreamNumber = StreamNumber;
      data.pStreamData = pStreamData;
      data.OffsetInBytes = OffsetInBytes;
      data.Stride = Stride;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetStreamSource, data);
   }      
}

void BRenderDraw::clearStreamSource(UINT StreamNumber)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetStreamSource(StreamNumber, 0, 0, 0);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      gRenderThread.submitCommand(mCommandListenerHandle, cRDClearStreamSource, StreamNumber);
   }      
}

void BRenderDraw::setIndices(IDirect3DIndexBuffer9 *pIndexData)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetIndices(pIndexData);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetIndicesData data;
      data.pIndexData = pIndexData;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetIndices, data);
   }
}

void BRenderDraw::drawVertices(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT VertexCount)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->DrawVertices(PrimitiveType, StartVertex, VertexCount);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDDrawVerticesData data;
      data.PrimitiveType = PrimitiveType;
      data.StartVertex = StartVertex;
      data.VertexCount = VertexCount;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDDrawVertices, data);
   }      
}

void BRenderDraw::drawVerticesUP(D3DPRIMITIVETYPE PrimitiveType, UINT VertexCount, CONST void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->DrawVerticesUP(PrimitiveType, VertexCount, pVertexStreamZeroData, VertexStreamZeroStride);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      const uint vertexLen = VertexCount * VertexStreamZeroStride;
      void* pVertices = gRenderThread.allocateFrameStorage(vertexLen);
      BVERIFY(pVertices);
      Utils::FastMemCpy(pVertices, pVertexStreamZeroData, vertexLen);

      BRDDrawVerticesUPData data;
      data.PrimitiveType = PrimitiveType;
      data.VertexCount = VertexCount;
      data.pVertexStreamZeroData = pVertices;
      data.VertexStreamZeroStride = VertexStreamZeroStride;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDDrawVerticesUP, data);
   }      
}

void BRenderDraw::drawIndexedVertices(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex,UINT StartIndex,UINT IndexCount)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->DrawIndexedVertices(PrimitiveType, BaseVertexIndex, StartIndex, IndexCount);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDDrawIndexedVerticesData data;
      data.PrimitiveType = PrimitiveType;
      data.BaseVertexIndex = BaseVertexIndex;
      data.StartIndex = StartIndex;
      data.IndexCount = IndexCount;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDDrawIndexedVertices, data);
   }      
}

void BRenderDraw::drawIndexedVerticesUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT IndexCount, CONST void *pIndexData, D3DFORMAT IndexDataFormat, CONST void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->DrawIndexedVerticesUP(PrimitiveType, MinVertexIndex, NumVertices, IndexCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      const uint vertexLen = NumVertices * VertexStreamZeroStride;
      void* pVertices = gRenderThread.allocateFrameStorage(vertexLen);
      BVERIFY(pVertices);
      Utils::FastMemCpy(pVertices, pVertexStreamZeroData, vertexLen);

      const uint indexLen = IndexCount * ((IndexDataFormat == D3DFMT_INDEX16) ? 2 : 4);
      void* pIndices = gRenderThread.allocateFrameStorage(indexLen);
      Utils::FastMemCpy(pIndices, pIndexData, indexLen);

      BRDDrawIndexedVerticesUPData data;
      data.PrimitiveType = PrimitiveType;
      data.MinVertexIndex = MinVertexIndex;
      data.NumVertices = NumVertices;
      data.IndexCount = IndexCount;
      data.pIndexData = pIndices;
      data.IndexDataFormat = IndexDataFormat;
      data.pVertexStreamZeroData = pVertices;
      data.VertexStreamZeroStride = VertexStreamZeroStride;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDDrawIndexedVerticesUP, data);
   }      
}

void BRenderDraw::setPixelShader(IDirect3DPixelShader9 *pShader)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetPixelShader(pShader);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetPixelShaderData data;
      data.pShader = pShader;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetPixelShader, data);
   }      
}

void BRenderDraw::setVertexShader(IDirect3DVertexShader9 *pShader)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetVertexShader(pShader);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetVertexShaderData data;
      data.pShader = pShader;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetVertexShader, data);
   }      
}

void BRenderDraw::setPixelShaderConstantF(UINT StartRegister, CONST float *pConstantData, DWORD Vector4fCount)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
   else
   {
      BDEBUG_ASSERT(pConstantData);
      
      const uint len = Vector4fCount * sizeof(float) * 4;
      float* pMem = (float*)gRenderThread.allocateFrameStorage(len);
      BVERIFY(pMem);
      Utils::FastMemCpy(pMem, pConstantData, len);

      BRDSetVertexShaderConstantFData data;
      data.StartRegister = StartRegister;
      data.pConstantData = pMem;
      data.Vector4fCount = Vector4fCount;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetPixelShaderConstantF, data);
   }      
}

void BRenderDraw::setVertexShaderConstantF(UINT StartRegister, CONST float *pConstantData, DWORD Vector4fCount)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BDEBUG_ASSERT(pConstantData);
      const uint len = Vector4fCount * sizeof(float) * 4;
      float* pMem = (float*)gRenderThread.allocateFrameStorage(len);
      BVERIFY(pMem);
      Utils::FastMemCpy(pMem, pConstantData, len);

      BRDSetVertexShaderConstantFData data;
      data.StartRegister = StartRegister;
      data.pConstantData = pMem;
      data.Vector4fCount = Vector4fCount;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetVertexShaderConstantF, data);
   }      
}

void BRenderDraw::setVertexDeclaration(IDirect3DVertexDeclaration9 *pDecl)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetVertexDeclaration(pDecl);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetVertexDeclarationData data;
      data.pDecl = pDecl;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetVertexDeclaration, data);
   }      
}

void BRenderDraw::setFVF(DWORD fvf)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->SetFVF(fvf);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDSetFVFData data;
      data.FVF = fvf;
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetFVF, data);
   }      
}

void BRenderDraw::resolve(
                            DWORD Flags,
                            CONST D3DRECT *pSourceRect,
                            IDirect3DBaseTexture9 *pDestTexture,
                            CONST D3DPOINT *pDestPoint,
                            UINT DestLevel,
                            UINT DestSliceOrFace,
                            CONST D3DVECTOR4 *pClearColor,
                            float ClearZ,
                            DWORD ClearStencil,
                            CONST D3DRESOLVE_PARAMETERS *pParameters)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->Resolve(Flags, pSourceRect, pDestTexture, pDestPoint, DestLevel, DestSliceOrFace, pClearColor, ClearZ, ClearStencil, pParameters);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      BDEBUG_ASSERT(pDestTexture);

      BRDResolveData data;
      data.Flags = Flags;

      data.HasSourceRect = false;
      data.HasSourceRect = false;
      data.HasDestPoint = false;
      data.HasClearColor = false;
      data.HasResolveParameters = false;

      if (pSourceRect)
      {
         data.SourceRect = *pSourceRect;
         data.HasSourceRect = true;
      }

      data.pDestTexture = pDestTexture;
      if (pDestPoint)
      {
         data.DestPoint = *pDestPoint;
         data.HasDestPoint = true;
      }

      data.DestLevel = DestLevel;
      data.DestSliceOrFace = DestSliceOrFace;
      if (pClearColor)
      {
         data.ClearColor = *pClearColor;
         data.HasClearColor = true;
      }

      data.ClearZ = ClearZ;
      data.ClearStencil = ClearStencil;
      if (pParameters)
      {
         data.Parameters = *pParameters;
         data.HasResolveParameters = true;
      }

      gRenderThread.submitCommand(mCommandListenerHandle, cRDResolve, data);
   }      
}

void BRenderDraw::unsetAll(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->UnsetAll();
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      gRenderThread.submitCommand(mCommandListenerHandle, cRDUnsetAll);
   }      
}

void BRenderDraw::dummyPresent(void)
{
   ASSERT_RENDER_THREAD
   
   gEventDispatcher.dispatchSynchronousEvents();

   checkForFailOrMeltdown();
   
   gRenderThread.processPresent();
}

void BRenderDraw::present(CONST RECT *pSourceRect, CONST RECT *pDestRect)
{
   ASSERT_MAIN_OR_WORKER_THREAD;
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
   {
      gEventDispatcher.dispatchSynchronousEvents();
      
      checkForFailOrMeltdown();
      
      {
         SCOPEDSAMPLE(Present)
         BD3D::mpDev->Present(pSourceRect, pDestRect, NULL, NULL);
      }
      
      gRenderThread.processPresent();
   }
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDPresentData data;
      data.HasSourceRect = false;
      data.HasDestRect = false;

      if (pSourceRect)
      {
         data.SourceRect = *pSourceRect;
         data.HasSourceRect = true;
      }

      if (pDestRect)
      {
         data.DestRect = *pDestRect;
         data.HasDestRect = true;
      }

      gRenderThread.submitCommand(mCommandListenerHandle, cRDPresent, data);
   }      
}

void BRenderDraw::clear(
                          DWORD Count,
                          CONST D3DRECT *pRects,
                          DWORD Flags,
                          D3DCOLOR Color,
                          float Z,
                          DWORD Stencil)
{
   ASSERT_MAIN_OR_WORKER_THREAD;
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->Clear(Count, pRects, Flags, Color, Z, Stencil);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());

      BRDClearData data;

      data.Count = Count;
      data.pRects = NULL;

      if (Count)
      {
         D3DRECT* pDstRects = gRenderThread.allocateFrameStorageObj<D3DRECT, false>(Count);
         BVERIFY(pDstRects);
         Utils::FastMemCpy(pDstRects, pRects, sizeof(D3DRECT)*Count);
         data.pRects = pDstRects;
      }

      data.Flags = Flags;
      data.Color = Color;
      data.Z = Z;
      data.Stencil = Stencil;

      gRenderThread.submitCommand(mCommandListenerHandle, cRDClear, data);
   }      
}           

void BRenderDraw::clearF(DWORD Flags, CONST D3DRECT *pRect, CONST D3DVECTOR4 *pColor, float Z, DWORD Stencil)
{
   ASSERT_MAIN_OR_WORKER_THREAD;
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->ClearF(Flags, pRect, pColor, Z, Stencil);
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      BDEBUG_ASSERT(pColor);

      BRDClearFData data;
      
      data.pRect = NULL;

      if (pRect)
      {
         const uint Count = 1;
         D3DRECT* pDstRects = gRenderThread.allocateFrameStorageObj<D3DRECT, false>(Count);
         BVERIFY(pDstRects);
         Utils::FastMemCpy(pDstRects, pRect, sizeof(D3DRECT)*Count);
         data.pRect = pDstRects;
      }

      data.Flags = Flags;
      data.Color = *pColor;
      data.Z = Z;
      data.Stencil = Stencil;

      gRenderThread.submitCommand(mCommandListenerHandle, cRDClearF, data);
   }      
}

void BRenderDraw::setDefaultRenderStates(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      workerSetDefaultRenderStates();
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetDefaultRenderStates);  
   }      
}

void BRenderDraw::setDefaultSamplerStates(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      workerSetDefaultSamplerStates();
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSetDefaultSamplerStates);
   }      
}

void BRenderDraw::beginScene(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
   {
      BD3D::mpDev->BeginScene();
   }
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      gRenderThread.submitCommand(mCommandListenerHandle, cRDBeginScene);
   }      
   
   BDEBUG_ASSERT(!mpThreadData->mBegunScene);
   mpThreadData->mBegunScene = true;
}

void BRenderDraw::endScene(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
   {
      BD3D::mpDev->EndScene();
   }
   else
   {
      BDEBUG_ASSERT(gRenderThread.getInFrame());
      gRenderThread.submitCommand(mCommandListenerHandle, cRDEndScene);
   }      
   
   BDEBUG_ASSERT(mpThreadData->mBegunScene);
   mpThreadData->mBegunScene = false;
}

void BRenderDraw::kickPushBuffer(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->InsertFence();
   else
      gRenderThread.submitCommand(cRCCControl, cRCKickPushBuffer);
}

BRenderViewport& BRenderDraw::getMainSceneRenderViewport(void) 
{ 
   ASSERT_MAIN_THREAD
   // If this assert fires, you've called this method from the wrong spot. Most likely, you're calling this from the game's update() method, 
   // which isn't allowed because you would be using the previous frame's values.
   BDEBUG_ASSERT(gRenderThread.getInFrame()); 
   return mMainSceneRenderViewport; 
}

BRenderViewport& BRenderDraw::getMainActiveRenderViewport(void) 
{ 
   ASSERT_MAIN_THREAD
   // If this assert fires, you've called this method from the wrong spot. Most likely, you're calling this from the game's update() method, 
   // which isn't allowed because you would be using the previous frame's values.
   BDEBUG_ASSERT(gRenderThread.getInFrame()); 
   return mMainActiveRenderViewport; 
}

BMatrixTracker& BRenderDraw::getMainSceneMatrixTracker(void) 
{ 
   ASSERT_MAIN_THREAD
   // If this assert fires, you've called this method from the wrong spot. Most likely, you're calling this from the game's update() method, 
   // which isn't allowed because you would be using the previous frame's values.   
   BDEBUG_ASSERT(gRenderThread.getInFrame()); 
   return mMainSceneMatrixTracker; 
}

BMatrixTracker& BRenderDraw::getMainActiveMatrixTracker(void) 
{ 
   ASSERT_MAIN_THREAD
   // If this assert fires, you've called this method from the wrong spot. Most likely, you're calling this from the game's update() method, 
   // which isn't allowed because you would be using the previous frame's values.   
   BDEBUG_ASSERT(gRenderThread.getInFrame()); 
   return mMainActiveMatrixTracker; 
}

BVolumeCuller& BRenderDraw::getMainSceneVolumeCuller(void) 
{ 
   ASSERT_MAIN_THREAD
   // If this assert fires, you've called this method from the wrong spot. Most likely, you're calling this from the game's update() method, 
   // which isn't allowed because you would be using the previous frame's values.
//   BDEBUG_ASSERT(gRenderThread.getInFrame()); 
   return mMainSceneVolumeCuller; 
}         

BVolumeCuller& BRenderDraw::getMainActiveVolumeCuller(void) 
{ 
   ASSERT_MAIN_THREAD
   // If this assert fires, you've called this method from the wrong spot. Most likely, you're calling this from the game's update() method, 
   // which isn't allowed because you would be using the previous frame's values.
   BDEBUG_ASSERT(gRenderThread.getInFrame()); 
   return mMainActiveVolumeCuller; 
}         

void BRenderDraw::setMainSceneMatrixTracker(const BMatrixTracker& tracker)
{
   ASSERT_MAIN_THREAD
   
   if (&mMainSceneMatrixTracker != &tracker)
      mMainSceneMatrixTracker = tracker;
   
   uchar* pDst = (uchar*)gRenderThread.submitCommandBegin(mCommandListenerHandle, cRDSetSceneMatrixTracker, BMatrixTracker::getSerializeSize());
   
   tracker.serialize(pDst);
   
   gRenderThread.submitCommandEnd(BMatrixTracker::getSerializeSize());
      
   mMainSceneVolumeCuller.setBasePlanes(mMainSceneMatrixTracker.getWorldFrustum());
   setMainSceneVolumeCuller(mMainSceneVolumeCuller);
}

void BRenderDraw::updateEffectIntrinsics(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   
   struct 
   {
      eEffectIntrinsicIndex mIntrinsic;
      eMTMatrix mMatrix;
   } matrices[] = 
   { 
     { cIntrinsicWorldToView,    cMTWorldToView  },
     { cIntrinsicViewToProj,     cMTViewToProj   },
     { cIntrinsicProjToScreen,   cMTProjToScreen },
     { cIntrinsicViewToWorld,    cMTViewToWorld  },
     { cIntrinsicScreenToProj,   cMTScreenToProj },
     { cIntrinsicWorldToProj,    cMTWorldToProj  },
     { cIntrinsicScreenToView,   cMTScreenToView },
     // This is a special case that use's the scene's matrix tracker
     { cIntrinsicSceneWorldToProj,  cMTInvalidMatrix }   
   };
   const uint cNumMatrices = sizeof(matrices) / sizeof(matrices[0]);
      
   uint lowestIntrinsic = UINT_MAX;
   uint highestIntrinsic = 0;
   
   const bool renderThread = (GetCurrentThreadId() == gRenderThread.getWorkerThreadId());
//-- FIXING PREFIX BUG ID 7101
   const BMatrixTracker& matrixTracker = renderThread ? mWorkerActiveMatrixTracker : mMainActiveMatrixTracker;
//--
//-- FIXING PREFIX BUG ID 7102
   const BMatrixTracker& sceneMatrixTracker = renderThread ? mWorkerSceneMatrixTracker : mMainSceneMatrixTracker;
//--
   
   for (uint matrixIndex = 0; matrixIndex < cNumMatrices; matrixIndex++)
   {
      // This a hacked special case where we send the scene's world to proj instead
      // of the active world to proj
      if (matrices[matrixIndex].mIntrinsic == cIntrinsicSceneWorldToProj)
      {
         // Send scene world to proj
         gEffectIntrinsicManager.set(
            matrices[matrixIndex].mIntrinsic, 
            &sceneMatrixTracker.getMatrix44(cMTWorldToProj, false),
            cIntrinsicTypeFloat4x4,
            1,
            false);
      }
      else
      {
         gEffectIntrinsicManager.set(
            matrices[matrixIndex].mIntrinsic, 
            &matrixTracker.getMatrix44(matrices[matrixIndex].mMatrix, false),
            cIntrinsicTypeFloat4x4,
            1,
            false);
      }
         
      lowestIntrinsic = Math::Min<uint>(lowestIntrinsic, matrices[matrixIndex].mIntrinsic);
      highestIntrinsic = Math::Max<uint>(highestIntrinsic, matrices[matrixIndex].mIntrinsic);
   }
      
   gEffectIntrinsicManager.set(cIntrinsicWorldCameraPos, &matrixTracker.getWorldCamPosVec4(), cIntrinsicTypeFloat3, 1, false);
   
   if (!renderThread)
   {
      lowestIntrinsic = Math::Min<uint>(lowestIntrinsic, cIntrinsicWorldCameraPos);
      highestIntrinsic = Math::Max<uint>(highestIntrinsic, cIntrinsicWorldCameraPos);
      
      gEffectIntrinsicManager.updateRenderThread((eEffectIntrinsicIndex)lowestIntrinsic, (eEffectIntrinsicIndex)highestIntrinsic);
   }
}

void BRenderDraw::setMainActiveMatrixTracker(const BMatrixTracker& tracker)
{
   ASSERT_MAIN_THREAD
   
   if (&tracker != &mMainActiveMatrixTracker)
      mMainActiveMatrixTracker = tracker;

   uchar* pDst = (uchar*)gRenderThread.submitCommandBegin(mCommandListenerHandle, cRDSetActiveMatrixTracker, BMatrixTracker::getSerializeSize());
   
   tracker.serialize(pDst);
   
   gRenderThread.submitCommandEnd(BMatrixTracker::getSerializeSize());
   
   updateEffectIntrinsics();
   
   mMainActiveVolumeCuller.setBasePlanes(mMainActiveMatrixTracker.getWorldFrustum());
   setMainActiveVolumeCuller(mMainActiveVolumeCuller);
}

void BRenderDraw::setMainSceneRenderViewport(const BRenderViewport& viewport)
{
   ASSERT_MAIN_THREAD
   mMainSceneRenderViewport = viewport;
   gRenderThread.submitCommand(mCommandListenerHandle, cRDSetSceneRenderViewport, sizeof(viewport), &viewport);
}

void BRenderDraw::setMainActiveRenderViewport(const BRenderViewport& viewport)
{
   ASSERT_MAIN_THREAD
   
   if (&mMainActiveRenderViewport != &viewport)
      mMainActiveRenderViewport = viewport;
      
   gRenderThread.submitCommand(mCommandListenerHandle, cRDSetActiveRenderViewport, sizeof(viewport), &viewport);
}

void BRenderDraw::setMainSceneVolumeCuller(const BVolumeCuller& volumeCuller)
{
   ASSERT_MAIN_THREAD
   
   if (&mMainSceneVolumeCuller != &volumeCuller)
      mMainSceneVolumeCuller = volumeCuller;
   
   // This is not submitting with alignment, I'm assuming the data will be bitwise copied.
   const uint size = volumeCuller.getSerializeSize();
   
   uchar* pDst = (uchar*)gRenderThread.submitCommandBegin(mCommandListenerHandle, cRDSetSceneVolumeCuller, size);
   
   volumeCuller.serialize(pDst);
   
   gRenderThread.submitCommandEnd(size);
}

void BRenderDraw::setMainActiveVolumeCuller(const BVolumeCuller& volumeCuller)
{
   ASSERT_MAIN_THREAD
   
   if (&mMainActiveVolumeCuller != &volumeCuller)
      mMainActiveVolumeCuller = volumeCuller;
      
   const uint size = volumeCuller.getSerializeSize();      

   // This is not submitting with alignment, I'm assuming the data will be bitwise copied.
   uchar* pDst = (uchar*)gRenderThread.submitCommandBegin(mCommandListenerHandle, cRDSetActiveVolumeCuller, size);
   
   volumeCuller.serialize(pDst);
   
   gRenderThread.submitCommandEnd(size);
}

BRenderViewport& BRenderDraw::getWorkerSceneRenderViewport(void) 
{  
   return mWorkerSceneRenderViewport; 
}

BRenderViewport& BRenderDraw::getWorkerActiveRenderViewport(void) 
{ 
   return mWorkerActiveRenderViewport; 
}

void BRenderDraw::setWorkerActiveRenderViewport(const BRenderViewport& renderViewport) 
{ 
   ASSERT_RENDER_THREAD 
   mWorkerActiveRenderViewport = renderViewport; 
   mWorkerActiveRenderViewport.setToDevice();
}

BMatrixTracker& BRenderDraw::getWorkerSceneMatrixTracker(void) 
{ 
   return mWorkerSceneMatrixTracker; 
}

BMatrixTracker& BRenderDraw::getWorkerActiveMatrixTracker(void) 
{ 
   return mWorkerActiveMatrixTracker; 
}

void BRenderDraw::setWorkerActiveMatrixTracker(const BMatrixTracker& matrixTracker) 
{ 
   ASSERT_RENDER_THREAD 
   
   if (&mWorkerActiveMatrixTracker != &matrixTracker)
      mWorkerActiveMatrixTracker = matrixTracker; 
   
   updateEffectIntrinsics();
   
   mWorkerActiveVolumeCuller.setBasePlanes(mWorkerActiveMatrixTracker.getWorldFrustum());
   setWorkerActiveVolumeCuller(mWorkerActiveVolumeCuller);
}

BVolumeCuller& BRenderDraw::getWorkerSceneVolumeCuller(void) 
{ 
   return mWorkerSceneVolumeCuller; 
}

BVolumeCuller& BRenderDraw::getWorkerActiveVolumeCuller(void) 
{ 
   return mWorkerActiveVolumeCuller; 
}  
    
void BRenderDraw::setWorkerActiveVolumeCuller(const BVolumeCuller& volumeCuller) 
{ 
   ASSERT_RENDER_THREAD
   mWorkerActiveVolumeCuller = volumeCuller; 
}

void BRenderDraw::resetMainActiveMatricesAndViewport(void)
{
   setMainActiveMatrixTracker(getMainSceneMatrixTracker());
   setMainActiveRenderViewport(getMainSceneRenderViewport());
   setMainActiveVolumeCuller(getMainSceneVolumeCuller());
}

void BRenderDraw::resetWorkerActiveMatricesAndViewport(void)
{
   setWorkerActiveMatrixTracker(getWorkerSceneMatrixTracker());
   setWorkerActiveRenderViewport(getWorkerSceneRenderViewport());
   setWorkerActiveVolumeCuller(getWorkerSceneVolumeCuller());
}

uint BRenderDraw::getNumViewports() const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   return mpThreadData->mViewportState.mNumViewports;
}

BOOL BRenderDraw::getVerticalSplit() const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   return mpThreadData->mViewportState.mVerticalSplit;
}

int BRenderDraw::getViewportIndex() const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   return mpThreadData->mViewportState.mViewportIndex;
}

const BRenderDraw::BViewportDesc& BRenderDraw::getViewportDesc(uint viewportIndex) const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(viewportIndex < cMaxViewports);
   return mpThreadData->mViewportState.mDesc[viewportIndex];
}

void BRenderDraw::workerSetViewportIndex(void* pData)
{
   mpThreadData->mViewportState.mViewportIndex = (int)pData;
}

void BRenderDraw::setViewportIndex(int viewportIndex)
{
   BDEBUG_ASSERT(Math::IsInRange(viewportIndex, -1, cMaxViewports - 1));

   mpThreadData->mViewportState.mViewportIndex = viewportIndex;
   
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
      gRenderThread.submitCallback(workerSetViewportIndex, (void*)viewportIndex);
   }
}

void BRenderDraw::workerSetViewports(void* pData)
{
   BViewportState* pState = static_cast<BViewportState*>(pData);
   
   memcpy(&mpThreadData->mViewportState, pState, sizeof(BViewportState));
   
   HEAP_DELETE(pState, gRenderHeap);
}

void BRenderDraw::setViewports(uint numViewports, BOOL verticalSplit, const BViewportDesc* pViewports) const
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT((numViewports > 0) && (numViewports <= cMaxViewports));
   
   mpThreadData->mViewportState.mNumViewports = numViewports;
   mpThreadData->mViewportState.mVerticalSplit = verticalSplit;
   for (uint i = 0; i < numViewports; i++)
      mpThreadData->mViewportState.mDesc[i] = pViewports[i];
   
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
//-- FIXING PREFIX BUG ID 7103
      const BViewportState* pViewportState = new(gRenderHeap) BViewportState(mpThreadData->mViewportState);
//--
            
      gRenderThread.submitCallback(workerSetViewports, pViewportState);
   }
}

void BRenderDraw::invalidateGPUCache(
   void *pBaseAddress,
   DWORD Size,
   DWORD Flags)
{   
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      BD3D::mpDev->InvalidateGpuCache(pBaseAddress, Size, Flags);
   else
   {
      BRDInvalidateGPUCacheData data;
      data.pBaseAddress = pBaseAddress;
      data.Size = Size;
      data.Flags = Flags;
      
      gRenderThread.submitCommand(mCommandListenerHandle, cRDInvalidateGPUCache, data);
   }      
}

// Creates a dynamic VB for streaming purposes from CPU and GPU frame storage.
// This VB's lifetime can ONLY be for the current frame! 
IDirect3DVertexBuffer9* BRenderDraw::createDynamicVB(uint lengthInBytes)
{
   IDirect3DVertexBuffer9* pVB;
   uchar* pBuffer;
   
   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
   {
      pVB = gRenderThread.allocateFrameStorageObj<IDirect3DVertexBuffer9, false>();
      pBuffer = (uchar*)gRenderThread.allocateGPUFrameStorage(lengthInBytes, sizeof(DWORD));
   }
   else
   {
      ASSERT_RENDER_THREAD
      
      pVB = gRenderThread.workerAllocateFrameStorageObj<IDirect3DVertexBuffer9, false>();
      pBuffer = (uchar*)gRenderThread.workerAllocateGPUFrameStorage(lengthInBytes, sizeof(DWORD));
   }
   
   if (!pVB)
   {
      BFATAL_FAIL("Out of CPU frame storage");
   }
   if (!pBuffer)
   {
      BFATAL_FAIL("Out of GPU frame storage");
   }
                
   XGSetVertexBufferHeader(lengthInBytes, 0, 0, 0, pVB);

   XGOffsetResourceAddress(pVB, pBuffer); 
   
   return pVB;
}

// Creates a dynamic IB for streaming purposes from CPU and GPU frame storage.
// This IB's lifetime can ONLY be for the current frame!
IDirect3DIndexBuffer9* BRenderDraw::createDynamicIB(uint lengthInBytes, D3DFORMAT indexType)
{
   IDirect3DIndexBuffer9* pIB;
   uchar* pBuffer;
   
   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
   {
      pIB = gRenderThread.allocateFrameStorageObj<IDirect3DIndexBuffer9, false>();
      pBuffer = (uchar*)gRenderThread.allocateGPUFrameStorage(lengthInBytes, sizeof(DWORD));
   }
   else
   {
      ASSERT_RENDER_THREAD
      
      pIB = gRenderThread.workerAllocateFrameStorageObj<IDirect3DIndexBuffer9, false>();
      pBuffer = (uchar*)gRenderThread.workerAllocateGPUFrameStorage(lengthInBytes, sizeof(DWORD));
   }
   
   if (!pIB)
   {
      BFATAL_FAIL("Out of CPU frame storage");
   }
   if (!pBuffer)
   {
      BFATAL_FAIL("Out of GPU frame storage");
   }
   
   XGSetIndexBufferHeader(lengthInBytes, 0, indexType, 0, 0, pIB );
   XGOffsetResourceAddress(pIB, pBuffer); 
   
   return pIB;
}

IDirect3DTexture9* BRenderDraw::createDynamicTexture(uint width, uint height, D3DFORMAT format, uint* pLen)
{
   const bool mainThread = (GetCurrentThreadId() == gRenderThread.getMainThreadId());

   IDirect3DTexture9* pTex;
   if (mainThread)
      pTex = gRenderThread.allocateFrameStorageObj<IDirect3DTexture9, false>();
   else
   {
      ASSERT_RENDER_THREAD

      pTex = gRenderThread.workerAllocateFrameStorageObj<IDirect3DTexture9, false>();
   }

   if (!pTex)
      return NULL;

   const uint size = XGSetTextureHeader(
      width,
      height,
      1, 
      0,
      format,
      0,
      0,
      XGHEADER_CONTIGUOUS_MIP_OFFSET,
      0,
      pTex,
      NULL,
      NULL ); 
   
   if (pLen)
      *pLen = size;      

   void* pData;
   if (mainThread)      
      pData = (uchar*)gRenderThread.allocateGPUFrameStorage(size, 4096);   
   else
      pData = (uchar*)gRenderThread.workerAllocateGPUFrameStorage(size, 4096);   

   if (!pData)
   {
      return NULL;
   }

   XGOffsetResourceAddress(pTex, pData);       
   
   return pTex;      
}

IDirect3DTexture9* BRenderDraw::lockDynamicTexture(uint width, uint height, D3DFORMAT format, D3DLOCKED_RECT* pLockedRect, uint* pLen, bool lineTexture)
{
   BDEBUG_ASSERT(pLockedRect);
   BDEBUG_ASSERT(NULL == mpThreadData->mpCurDynamicTextureData);   
   
   const bool mainThread = (GetCurrentThreadId() == gRenderThread.getMainThreadId());
            
   if (mainThread)
      mpThreadData->mpCurDynamicTexture = gRenderThread.allocateFrameStorageObj<IDirect3DTexture9, false>();
   else
   {
      ASSERT_RENDER_THREAD
      
      mpThreadData->mpCurDynamicTexture = gRenderThread.workerAllocateFrameStorageObj<IDirect3DTexture9, false>();
   }
   
   if (!mpThreadData->mpCurDynamicTexture)
   {
      BFATAL_FAIL("Out of CPU frame storage");
   }

   if (lineTexture)
   {
      BDEBUG_ASSERT(1 == height);

      // rg [7/11/06] - July XDK changed this, not currently supporting.
       
      BVERIFY(false);
#if 0      
      mpThreadData->mCurDynamicTextureSize = XGSetLineTextureHeader(
         width,
         1, 
         0,
         format,
         0,
         0,
         XGHEADER_CONTIGUOUS_MIP_OFFSET,
         mpThreadData->mpCurDynamicTexture,
         NULL,
         NULL ); 
#endif         
   }
   else
   {
      mpThreadData->mCurDynamicTextureSize = XGSetTextureHeader(
         width,
         height,
         1, 
         0,
         format,
         0,
         0,
         XGHEADER_CONTIGUOUS_MIP_OFFSET,
         0,
         mpThreadData->mpCurDynamicTexture,
         NULL,
         NULL ); 
   }         
   
   if (mainThread)      
      mpThreadData->mpCurDynamicTextureData = (uchar*)gRenderThread.allocateGPUFrameStorage(mpThreadData->mCurDynamicTextureSize, 4096);   
   else
      mpThreadData->mpCurDynamicTextureData = (uchar*)gRenderThread.workerAllocateGPUFrameStorage(mpThreadData->mCurDynamicTextureSize, 4096);   
   
   if (!mpThreadData->mpCurDynamicTextureData)
   {
      BFATAL_FAIL("Out of GPU frame storage");
   }
      
   XGOffsetResourceAddress( mpThreadData->mpCurDynamicTexture, mpThreadData->mpCurDynamicTextureData );       
      
   mpThreadData->mpCurDynamicTexture->LockRect(0, pLockedRect, NULL, mainThread ? D3DLOCK_NO_GPU_CACHE_INVALIDATE : 0);
   
   if (pLen)
      *pLen = mpThreadData->mCurDynamicTextureSize;
      
   return mpThreadData->mpCurDynamicTexture;      
}

void BRenderDraw::unlockDynamicTexture(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(NULL != mpThreadData->mpCurDynamicTextureData);
   
   mpThreadData->mpCurDynamicTexture->UnlockRect(0);
 
   const bool mainThread = (GetCurrentThreadId() == gRenderThread.getMainThreadId());
   if (mainThread)
      invalidateGPUCache(mpThreadData->mpCurDynamicTextureData, mpThreadData->mCurDynamicTextureSize);
   
   mpThreadData->mpCurDynamicTextureData = NULL;   
}

void* BRenderDraw::lockDynamicVB(uint numVerts, uint vertSize)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(NULL == mpThreadData->mpCurDynamicVBData);
   
   mpThreadData->mCurDynamicVBSize = numVerts * vertSize;
   mpThreadData->mpCurDynamicVB = createDynamicVB(mpThreadData->mCurDynamicVBSize);
   
   mpThreadData->mpCurDynamicVB->Lock(0, 0, &mpThreadData->mpCurDynamicVBData, 0);
   return mpThreadData->mpCurDynamicVBData;
}

void BRenderDraw::unlockDynamicVB(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(mpThreadData->mpCurDynamicVBData);
   
   mpThreadData->mpCurDynamicVB->Unlock();
   
   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
      invalidateGPUCache(mpThreadData->mpCurDynamicVBData, mpThreadData->mCurDynamicVBSize);
         
   mpThreadData->mpCurDynamicVBData = NULL;
}

void* BRenderDraw::lockDynamicIB(uint numIndices, D3DFORMAT indexType)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(NULL == mpThreadData->mpCurDynamicIBData);

   mpThreadData->mCurDynamicIBSize = numIndices * ((indexType == D3DFMT_INDEX16) ? sizeof(WORD) : sizeof(DWORD));
   mpThreadData->mpCurDynamicIB = createDynamicIB(mpThreadData->mCurDynamicIBSize, indexType);
   
   mpThreadData->mpCurDynamicIB->Lock(0, 0, &mpThreadData->mpCurDynamicIBData, 0);
   return mpThreadData->mpCurDynamicIBData;
}

void BRenderDraw::unlockDynamicIB(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   BDEBUG_ASSERT(mpThreadData->mpCurDynamicIBData);

   mpThreadData->mpCurDynamicIB->Unlock();
   
   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
      invalidateGPUCache(mpThreadData->mpCurDynamicIBData, mpThreadData->mCurDynamicIBSize);
   
   mpThreadData->mpCurDynamicIBData = NULL;
}

void BRenderDraw::unsetResources(DWORD flags)
{
   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
      gRenderThread.submitCommand(mCommandListenerHandle, cRDUnsetResources, sizeof(uint), &flags);
   else
      workerUnsetResources(flags);
}

void BRenderDraw::workerUnsetTextures(void)
{
   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
      BD3D::mpDev->SetTexture(i, NULL);
}

void BRenderDraw::unsetTextures(void)
{
   ASSERT_MAIN_OR_WORKER_THREAD   

   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
      gRenderThread.submitCommand(mCommandListenerHandle, cRDUnsetTextures);
   else
      workerUnsetTextures();
}

bool BRenderDraw::workerSaveScreenshot(const char* pFilename)
{
   BString filename(pFilename);
   filename.toLower();
      
   const bool status = (filename.contains(".jpg") || filename.contains(".jpeg")) ? workerSaveScreenshotJPEG(pFilename) : workerSaveScreenshotTGA(pFilename);
      
   if (!status)
      gConsoleOutput.output(cMsgConsole, "Error: Unable to save LDR screenshot!");
   else
      gConsoleOutput.output(cMsgConsole, "Saved LDR screenshot to: %s", pFilename);
   
   return status;
}   

bool BRenderDraw::workerSaveScreenshotTGA(const char* pFilename)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(BD3D::mpDev);
   
   BD3D::mpDev->BlockUntilIdle();
   
   IDirect3DTexture9* pFrontBuf;
   BD3D::mpDev->GetFrontBuffer(&pFrontBuf);
   pFrontBuf->Release();
   
   D3DRESOURCETYPE resType = pFrontBuf->GetType();
   resType;
   BASSERT(D3DRTYPE_TEXTURE == resType);

   D3DSURFACE_DESC desc;
   pFrontBuf->GetLevelDesc(0, &desc);
   BASSERT((desc.Format == D3DFMT_LE_X8R8G8B8) || (desc.Format == D3DFMT_LE_X2R10G10B10));
      
   volatile const DWORD* pSurf = reinterpret_cast<volatile const DWORD*>(pFrontBuf->Format.BaseAddress << 12);
   
   BFixedStringMaxPath tgaFilename;
   tgaFilename.set(pFilename);
      
   BFileSystemStream tgaStream;
   if (!tgaStream.open(-1, tgaFilename.c_str(), cSFWritable | cSFSeekable | cSFEnableBuffering))
      return false;
      
   BTGAWriter tgaWriter;
   if (!tgaWriter.open(tgaStream, desc.Width, desc.Height, cTGAImageTypeBGRA))
      return false;
      
   BDynamicArray<BRGBAColor> tgaScanlineBuf(desc.Width);
   
   for (uint y = 0; y < desc.Height; y++)
   {
      for (uint x = 0; x < desc.Width; x++)
      {
         const uint ofs = XGAddress2DTiledOffset(x, y, desc.Width, sizeof(DWORD));

         DWORD c = pSurf[ofs];
         EndianSwitchDWords(&c, 1);
         
         uint r, g, b;
                  
         if (desc.Format == D3DFMT_LE_X2R10G10B10)
         {
            const uint round = ((x ^ (y >> 1)) & 1) ? 511 : 0;
            r = (((c >> 20) & 1023) * 255 + round) / 1023;
            g = (((c >> 10) & 1023) * 255 + round) / 1023;
            b = ( (c        & 1023) * 255 + round) / 1023;
         }
         else
         {
            r = (c >> 16) & 0xFF;
            g = (c >> 8) & 0xFF;
            b = c & 0xFF;
         }
         
         //                             r  g  b  a
         tgaScanlineBuf[x] = BRGBAColor(b, g, r, 255);
      }
      
      if (!tgaWriter.writeLine(tgaScanlineBuf.getPtr()))
         return false;
   }
   
   if (!tgaWriter.close())
      return false;
      
   if (desc.Format == D3DFMT_LE_X2R10G10B10)
   {
      BString tiffFilename;
      tiffFilename.set(pFilename);
      
      strPathRemoveExtension(tiffFilename);
      
      tiffFilename += ".tif";
      
      BFileSystemStream tiffStream;
      if (!tiffStream.open(-1, tiffFilename, cSFWritable | cSFSeekable | cSFEnableBuffering))
         return false;

      BTIFFWriter tiffWriter;
      if (!tiffWriter.open(tiffStream, desc.Width, desc.Height, 3, 16))
         return false;

      BDynamicArray<ushort> tiffScanlineBuf(desc.Width * 3);
      
      BDynamicArray<ushort> conversionTable(1024);
      for (uint i = 0; i < 1024; i++)
         conversionTable[i] = (ushort)((i * 65535) / 1023);

      for (uint y = 0; y < desc.Height; y++)
      {
         ushort* pDst = tiffScanlineBuf.getPtr();
         
         for (uint x = 0; x < desc.Width; x++)
         {
            const uint ofs = XGAddress2DTiledOffset(x, y, desc.Width, sizeof(DWORD));

            DWORD c = pSurf[ofs];
            EndianSwitchDWords(&c, 1);
                                    
            pDst[0] = conversionTable[(c >> 20) & 1023];
            pDst[1] = conversionTable[(c >> 10) & 1023];
            pDst[2] = conversionTable[c         & 1023];
            pDst += 3;
         }

         if (!tiffWriter.writeLine(tiffScanlineBuf.getPtr()))
            return false;
      }

      if (!tiffWriter.close())
         return false;
   }
      
   return true;
}

void BRenderDraw::workerGetFrontBuffer(BRGBAImage& image)
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(BD3D::mpDev);
   
   BD3D::mpDev->BlockUntilIdle();

   IDirect3DTexture9* pFrontBuf;
   BD3D::mpDev->GetFrontBuffer(&pFrontBuf);
   pFrontBuf->Release();

   D3DRESOURCETYPE resType = pFrontBuf->GetType();
   resType;
   BASSERT(D3DRTYPE_TEXTURE == resType);

   D3DSURFACE_DESC desc;
   pFrontBuf->GetLevelDesc(0, &desc);
   BASSERT((desc.Format == D3DFMT_LE_X8R8G8B8) || (desc.Format == D3DFMT_LE_X2R10G10B10));

   volatile const DWORD* pSurf = reinterpret_cast<volatile const DWORD*>(pFrontBuf->Format.BaseAddress << 12);
   
   image.setSize(desc.Width, desc.Height);
      
   for (uint y = 0; y < desc.Height; y++)
   {
      for (uint x = 0; x < desc.Width; x++)
      {
         const uint ofs = XGAddress2DTiledOffset(x, y, desc.Width, sizeof(DWORD));

         DWORD c = pSurf[ofs];
         EndianSwitchDWords(&c, 1);

         uint r, g, b;

         if (desc.Format == D3DFMT_LE_X2R10G10B10)
         {
            const uint round = ((x ^ (y >> 1)) & 1) ? 511 : 0;
            r = (((c >> 20) & 1023) * 255 + round) / 1023;
            g = (((c >> 10) & 1023) * 255 + round) / 1023;
            b = ( (c        & 1023) * 255 + round) / 1023;
         }
         else
         {
            r = (c >> 16) & 0xFF;
            g = (c >> 8) & 0xFF;
            b = c & 0xFF;
         }
         
         image.setPixel(x, y, BRGBAColor(r, g, b, 255));
      }
   }
}

bool BRenderDraw::workerSaveScreenshotJPEG(const char* pFilename)
{
   ASSERT_THREAD(cThreadIndexRender);

   BDEBUG_ASSERT(BD3D::mpDev);
   
   BRGBAImage image;
   
   workerGetFrontBuffer(image);

   BByteArray data; 
   if (!BJPEGCodec::compressImageJPEG(image, data, 90, BJPEGCodec::cH2V2, true))
      return false;
      
   BString filename(pFilename);
      
   return BWin32FileUtils::writeFileData(pFilename, data, true);
}   

void BRenderDraw::saveScreenshot(const char* pFilename)
{
   if (gEventDispatcher.getThreadIndex() == cThreadIndexSim)
   {
      gRenderThread.submitCommand(mCommandListenerHandle, cRDSaveScreenshot, sizeof(const char*), &pFilename);
   
      gRenderThread.blockUntilWorkerIdle();
   }
   else
   {
      workerSaveScreenshot(pFilename);
   }
}

//============================================================================
// BRenderDraw::getResourceAddress
//============================================================================
void* BRenderDraw::getResourceAddress(D3DResource* pResource, bool cachedReadOnlyView)
{
   if (!pResource)
      return NULL;

   void* pPhysicalAlloc = NULL;

   switch (pResource->Common & D3DCOMMON_TYPE_MASK)
   {
      case D3DCOMMON_TYPE_VERTEXBUFFER:
      {
         pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<D3DVertexBuffer*>(pResource)->Format.BaseAddress << GPU_VERTEXBUFFER_ADDRESS_SHIFT);
         break;
      }
      case D3DCOMMON_TYPE_TEXTURE:
      {
         pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<D3DBaseTexture*>(pResource)->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT);
         break;
      }
      case D3DCOMMON_TYPE_INDEXBUFFER:
      {
         pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<D3DIndexBuffer*>(pResource)->Address);
         break;
      }
      case D3DCOMMON_TYPE_SURFACE:
      {
         // No physical memory 
         break;
      }
      default:
      {
         BFATAL_FAIL("BRenderDraw::getResourceAddress: Unsupported resource type");
      }
   }

   if (!pPhysicalAlloc)
      return NULL;

   return cachedReadOnlyView ? GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(pPhysicalAlloc) : pPhysicalAlloc;
}

DWORD BRenderDraw::pushRenderState(D3DRENDERSTATETYPE state)
{
   ASSERT_RENDER_THREAD
   
   BDEBUG_ASSERT(mRSStackTop < cRSStackSize);
     
   if (mRSStackTop >= cRSStackSize)   
      return 0;
      
   DWORD value = 0;
   BD3D::mpDev->GetRenderState(state, &value);
   
   mRSStack[mRSStackTop].mState = state;
   mRSStack[mRSStackTop].mValue = value;
   mRSStackTop++;
   
   return value;
}

DWORD BRenderDraw::pushAndSetRenderState(D3DRENDERSTATETYPE state, DWORD newValue)
{
   ASSERT_RENDER_THREAD

   const DWORD prevValue = pushRenderState(state);
   
   BD3D::mpDev->SetRenderState(state, newValue);
   
   return prevValue;
}

void BRenderDraw::popRenderState(D3DRENDERSTATETYPE state)
{
   ASSERT_RENDER_THREAD
   
   BDEBUG_ASSERT(mRSStackTop > 0);
   
   if (mRSStackTop)
   {
      mRSStackTop--;
      
      BDEBUG_ASSERT((D3DRENDERSTATETYPE)mRSStack[mRSStackTop].mState == state);
      
      BD3D::mpDev->SetRenderState((D3DRENDERSTATETYPE)mRSStack[mRSStackTop].mState, mRSStack[mRSStackTop].mValue);
   }
}

void BRenderDraw::setRenderStateStackTop(uint stackTop)
{  
   BDEBUG_ASSERT(stackTop <= mRSStackTop);
   
   for (int i = mRSStackTop - 1; i >= (int)stackTop; i--)
      BD3D::mpDev->SetRenderState((D3DRENDERSTATETYPE)mRSStack[i].mState, mRSStack[i].mValue);
   
   mRSStackTop = stackTop;
}

void BRenderDraw::getTextureLayout(
   D3DBaseTexture*  pTexture,             
   UINT*            pBaseData,            
   UINT*            pBaseSize,            
   XGLAYOUT_REGION* pBaseRegionList,      
   UINT*            pBaseRegionListCount, 
   UINT             BaseRegionAlignment,  
   UINT*            pMipData,             
   UINT*            pMipSize,             
   XGLAYOUT_REGION* pMipRegionList,       
   UINT*            pMipRegionListCount,  
   UINT             MipRegionAlignment)
{
   XGGetTextureLayout(pTexture, pBaseData, pBaseSize, pBaseRegionList, pBaseRegionListCount, BaseRegionAlignment, pMipData, pMipSize, pMipRegionList, pMipRegionListCount, MipRegionAlignment);

   if ((*pBaseRegionListCount == 1) && (*pMipRegionListCount == 0))
   {
      XGTEXTURE_DESC desc;
      XGGetTextureDesc(pTexture, 0, &desc);

      if ( ((desc.Width > 4) && (desc.Height <= 4)) ||
           ((desc.Height > 4) && (desc.Width <= 4)) )
      {
         pBaseRegionList[0].StartOffset = 0;
         pBaseRegionList[0].EndOffset = *pBaseSize;

         BDEBUG_ASSERT(desc.SlicePitch <= *pBaseSize);
      }
   }
}                               

void BRenderDraw::clearTextureData(IDirect3DBaseTexture9* pTex)
{
   BDEBUG_ASSERT(pTex);
   
   enum { cMaxBaseRegions = 256, cMaxMipRegions = 256 };

   XGLAYOUT_REGION baseRegions[cMaxBaseRegions];
   UINT baseRegionCount = cMaxBaseRegions;

   XGLAYOUT_REGION mipRegions[cMaxMipRegions];
   UINT mipRegionCount = cMaxMipRegions;

   UINT baseData = 0;
   UINT baseSize = 0;
   
   UINT mipData = 0;   
   UINT mipSize = 0;

   getTextureLayout(pTex, &baseData, &baseSize, baseRegions, &baseRegionCount, 16, &mipData, &mipSize, mipRegions, &mipRegionCount, 16);
   // If these asserts fire, increase cMaxBaseRegions or cMaxMipRegions
   BDEBUG_ASSERT(baseRegionCount < cMaxBaseRegions);
   BDEBUG_ASSERT(mipRegionCount < cMaxMipRegions);
   
   if (baseSize)
   {
      uchar* pBaseData = (uchar*)baseData;
      for (uint i = 0; i < baseRegionCount; i++)
         memset(pBaseData + baseRegions[i].StartOffset, 0, baseRegions[i].EndOffset - baseRegions[i].StartOffset);
      
      if (pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY)
         Utils::FlushCacheLines(pBaseData, Utils::AlignUpValue(baseSize, 4096));
   }         
      
   if (mipSize)
   {
      uchar* pMipData = (uchar*)mipData;      
      for (uint i = 0; i < mipRegionCount; i++)
         memset(pMipData + mipRegions[i].StartOffset, 0, mipRegions[i].EndOffset - mipRegions[i].StartOffset);
         
      if (pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY)
         Utils::FlushCacheLines(pMipData, Utils::AlignUpValue(mipSize, 4096));
   }         
}

void BRenderDraw::copyTextureData(IDirect3DBaseTexture9* pDstTex, const void* pSrc, uint srcSize)
{
   BDEBUG_ASSERT(pDstTex && pSrc && srcSize);

   enum { cMaxBaseRegions = 256, cMaxMipRegions = 256 };

   XGLAYOUT_REGION baseRegions[cMaxBaseRegions];
   UINT baseRegionCount = cMaxBaseRegions;

   XGLAYOUT_REGION mipRegions[cMaxMipRegions];
   UINT mipRegionCount = cMaxMipRegions;

   UINT baseData = 0;
   UINT baseSize = 0;

   UINT mipData = 0;   
   UINT mipSize = 0;

   getTextureLayout(pDstTex, &baseData, &baseSize, baseRegions, &baseRegionCount, 16, &mipData, &mipSize, mipRegions, &mipRegionCount, 16);
   // If these asserts fire, increase cMaxBaseRegions or cMaxMipRegions
   BDEBUG_ASSERT(baseRegionCount < cMaxBaseRegions);
   BDEBUG_ASSERT(mipRegionCount < cMaxMipRegions);

   BASSERT(srcSize >= (baseSize + mipSize));      
   
   if ((baseRegionCount == 1) && (mipRegionCount == 0))
   {
      XGTEXTURE_DESC desc;
      XGGetTextureDesc(pDstTex, 0, &desc);
      
      if ( ((desc.Width > 4) && (desc.Height <= 4)) ||
           ((desc.Height > 4) && (desc.Width <= 4)) )
      {
         baseRegions[0].StartOffset = 0;
         baseRegions[0].EndOffset = baseSize;
      
         BDEBUG_ASSERT(desc.SlicePitch <= baseSize);
      }
   }
   
   if (baseSize)
   {
      uchar* pBaseData = (uchar*)baseData;
      for (uint i = 0; i < baseRegionCount; i++)
         memcpy(pBaseData + baseRegions[i].StartOffset, static_cast<const uchar*>(pSrc) + baseRegions[i].StartOffset, baseRegions[i].EndOffset - baseRegions[i].StartOffset);

      if (pDstTex->Common & D3DCOMMON_CPU_CACHED_MEMORY)
         Utils::FlushCacheLines(pBaseData, Utils::AlignUpValue(baseSize, 4096));
   }         

   if (mipSize)
   {
      uchar* pMipData = (uchar*)mipData;      
      for (uint i = 0; i < mipRegionCount; i++)
         memcpy(pMipData + mipRegions[i].StartOffset, static_cast<const uchar*>(pSrc) + baseSize + mipRegions[i].StartOffset, mipRegions[i].EndOffset - mipRegions[i].StartOffset);

      if (pDstTex->Common & D3DCOMMON_CPU_CACHED_MEMORY)
         Utils::FlushCacheLines(pMipData, Utils::AlignUpValue(mipSize, 4096));
   }         
}

void BRenderDraw::flushTextureData(IDirect3DBaseTexture9* pDstTex)
{
   BDEBUG_ASSERT(pDstTex);
   
   if ((pDstTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0)
      return;
   
   BDEBUG_ASSERT(pDstTex);

   enum { cMaxBaseRegions = 256, cMaxMipRegions = 256 };

   XGLAYOUT_REGION baseRegions[cMaxBaseRegions];
   UINT baseRegionCount = cMaxBaseRegions;

   XGLAYOUT_REGION mipRegions[cMaxMipRegions];
   UINT mipRegionCount = cMaxMipRegions;

   UINT baseData = 0;
   UINT baseSize = 0;

   UINT mipData = 0;   
   UINT mipSize = 0;

   getTextureLayout(pDstTex, &baseData, &baseSize, baseRegions, &baseRegionCount, 16, &mipData, &mipSize, mipRegions, &mipRegionCount, 16);
   // If these asserts fire, increase cMaxBaseRegions or cMaxMipRegions
   BDEBUG_ASSERT(baseRegionCount < cMaxBaseRegions);
   BDEBUG_ASSERT(mipRegionCount < cMaxMipRegions);
   
   if (baseSize)
      Utils::FlushCacheLines((void*)baseData, Utils::AlignUpValue(baseSize, 4096));

   if (mipSize)
      Utils::FlushCacheLines((void*)mipData, Utils::AlignUpValue(mipSize, 4096));
}

BRenderStateStackState::BRenderStateStackState()
{
   mStackTop = gRenderDraw.getRenderStateStackTop();
}

BRenderStateStackState::~BRenderStateStackState()
{
   gRenderDraw.setRenderStateStackTop(mStackTop);
}
