//==============================================================================
// tonemapManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
// rg FIXME: Instead of calling the tiled AA manager directly, pass the 
// width/height and radiance surface as params.
//
//==============================================================================
#include "xgameRender.h"
#include "tonemapManager.h"

// xsystem
#include "asyncFileManager.h"

// xgameRender
#include "render.h"
#include "tiledAA.h"
#include "vertexTypes.h"
#include "blurHelper.h"

// xrender
#include "renderDraw.h"
#include "gpuHeap.h"

// xcore
#include "reloadManager.h"
#include "consoleOutput.h"

#define FXL_EFFECT_FILENAME "tonemap\\tonemap.bin"
#define MAX_BLOOM_FILTER_SAMPLES 32

//==============================================================================
// Globals
//==============================================================================
BToneMapManager gToneMapManager;

enum eTechniques
{
   cTechniqueFinal,
   cTechniqueFinalDOF,
   cTechniqueReduct4LogSum,
   cTechniqueReduct4LinSum,
   cTechniqueReduct4LinRGBAve,
   cTechniqueReduct2LinRGBAve,
   cTechniqueReduct1,
   cTechniqueBrightMask,
   cTechniqueBloomFilter,
   cTechniqueFilterAdaptation,
   cTechniqueFillDepthStencilSurface,
   cTechniqueGaussBlur5x5,
   cTechniqueRadialBlur,
   cTechniqueFillColorSurface,
};

//==============================================================================
//==============================================================================
bool BToneMapParams::directionalOrRadialBlurring() const
{
   return ((mBlurFactor > 0.0f) && (mBlurX != 0.0f || mBlurY != 0.0f || mRadialBlur)) ? true : false;
}

//==============================================================================
//==============================================================================
bool BToneMapParams::radialBlurring() const
{
   return ((mBlurFactor > 0.0f) && mRadialBlur) ? true : false;
}

//==============================================================================
// BToneMapParams::enforceLimits
//==============================================================================
void BToneMapParams::enforceLimits(void)
{
   mMiddleGrey = Math::Clamp(mMiddleGrey, 0.0f, 2.0f);
   mBrightMaskThresh = Math::Clamp(mBrightMaskThresh, 0.0f, 1.0f);
   
   mBloomIntensity = Math::Clamp(mBloomIntensity, 0.0f, 16.0f);
   mBloomSigma = Math::Clamp(mBloomSigma, .001f, 16.0f);
   mAdaptationRate = Math::Clamp(mAdaptationRate, .001f, 1.0f);

   mLogAveMin = Math::Clamp(mLogAveMin, .0001f, 32.0f);
   mLogAveMax = Math::Clamp(mLogAveMax, .0001f, 32.0f);
   if (mLogAveMin > mLogAveMax)
      std::swap(mLogAveMin, mLogAveMax);
   
   mWhitePointMin = Math::Clamp(mWhitePointMin, .001f, 32.0f);
   mWhitePointMax = Math::Clamp(mWhitePointMax, .001f, 32.0f);
   if (mWhitePointMin > mWhitePointMax)
      std::swap(mWhitePointMin, mWhitePointMax);
      
   float distances[3] = { mDOFNearBlurPlaneDist, mDOFFocalPlaneDist, mDOFFarBlurPlaneDist };
   Utils::BubbleSort(distances, distances + 3);
   
   mDOFNearBlurPlaneDist = distances[0];
   mDOFFocalPlaneDist = distances[1];
   mDOFFarBlurPlaneDist = distances[2];
   mDOFMaxBlurriness = Math::Clamp(mDOFMaxBlurriness, .125f, 4.0f);
}

//==============================================================================
// BToneMapManager::BToneMapManager
//==============================================================================
BToneMapManager::BToneMapManager() :
   BEventReceiver(),
   mInitialized(false),
   mDOFEnabled(true),
   mPostEffectsEnabled(true),
   mpAveLumTex64(NULL), mpAveLumSurf64(NULL),
   mpAveLumTex16(NULL), mpAveLumSurf16(NULL),
   mpRadTex4(NULL), mpRadSurf4(NULL),
   mpRadTex2(NULL), mpRadSurf2(NULL),
   mpAveLumSurf1(NULL),
   mpBrightMaskSurf(NULL),
   mWidthDiv16(0), mHeightDiv16(0),
   mWidthDiv64(0), mHeightDiv64(0),
   mCurLumTexIndex(0),
   mpDOFBlurTex(NULL),
   mpDOFBlurSurf(NULL),
   mCurrentViewportIndex(0)
{
   Utils::ClearObj(mpAveLumTex1);
   Utils::ClearObj(mpBloomBuf);
}

//==============================================================================
// BToneMapManager::~BToneMapManager
//==============================================================================
BToneMapManager::~BToneMapManager()
{
}

//==============================================================================
// BToneMapManager::init
//==============================================================================
void BToneMapManager::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   deinit();
   
   eventReceiverInit(cThreadIndexRender);   
   
   commandListenerInit();
   
   mInitialized = true;
   mDOFEnabled = true;
   mPostEffectsEnabled = true;
}

//==============================================================================
// BToneMapManager::deinit
//==============================================================================
void BToneMapManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (!mInitialized)
      return;
   
   gRenderThread.blockUntilGPUIdle();
   
   commandListenerDeinit();
         
   eventReceiverDeinit();
            
   mInitialized = false;
}

//==============================================================================
// BToneMapManager::getParams
//==============================================================================
const BToneMapParams& BToneMapManager::getParams(uint viewportIndex) const
{
   ASSERT_MAIN_OR_WORKER_THREAD

   BASSERT(viewportIndex < cNumViewports);
   
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      return mParams[viewportIndex];
   else
      return mSimParams[viewportIndex];
}

//==============================================================================
// BToneMapManager::setParams
//==============================================================================
struct BToneMapParamsPayload
{
   BToneMapParams mParams;
   uint mViewportIndex;
};
void BToneMapManager::setParams(const BToneMapParams& params, uint viewportIndex)
{
   ASSERT_MAIN_OR_WORKER_THREAD
   
   BDEBUG_ASSERT(mInitialized);
   BASSERT(viewportIndex < cNumViewports);
   
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      mParams[viewportIndex] = params;
   else
   {
      BToneMapParamsPayload payload;
      payload.mParams = params;
      payload.mViewportIndex = viewportIndex;

      gRenderThread.submitCommand(*this, cRenderCommandSetParams, payload);
      mSimParams[viewportIndex] = params;      
   }
}

//==============================================================================
// BToneMapManager::beginFrame
//==============================================================================
void BToneMapManager::beginFrame(void)
{
}

//==============================================================================
// BToneMapManager::loadEffect
//==============================================================================
void BToneMapManager::loadEffect(void)
{
   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

   pPacket->setFilename(FXL_EFFECT_FILENAME);
   pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
   pPacket->setReceiverHandle(mEventHandle);
   pPacket->setSynchronousReply(true);
   pPacket->setDiscardOnClose(true);

   gAsyncFileManager.submitRequest(pPacket);
}

//==============================================================================
// BToneMapManager::initEffectParams
//==============================================================================
void BToneMapManager::initEffectParams(void)
{
   // FIXME: Change params to manual register update!
}

//==============================================================================
// BToneMapManager::receiveEvent
//==============================================================================
bool BToneMapManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         loadEffect();
         
         BString fullFilename;
         gFileManager.constructQualifiedPath(gRender.getEffectCompilerDefaultDirID(), FXL_EFFECT_FILENAME, fullFilename);
                           
         BReloadManager::BPathArray paths;
         paths.pushBack(fullFilename);
         gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle); 
         
         break;
      }
      case cEventClassClientRemove:
      {
         mToneMapEffect.clear();
            
         gReloadManager.deregisterClient(mEventHandle);
            
         break;
      }
      case cEventClassAsyncFile:
      {
         BAsyncFileManager::BRequestPacket* pFileRequestPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket*>(event.mpPayload);
         
         mToneMapEffect.clear();
            
         HRESULT hres = E_FAIL;
         if (pFileRequestPacket->getSucceeded())
            hres = mToneMapEffect.createFromCompiledData(BD3D::mpDev, (void*)pFileRequestPacket->getData());
         
         if (FAILED(hres))
         {
            BFATAL_FAIL("BToneMapManager::receiveEvent: BFXLEffect::createFromCompiledData failed!");
         }
         
         gConsoleOutput.output(cMsgResource, "BToneMapManager: Load successful: %s", pFileRequestPacket->getFilename().c_str());
         
         initEffectParams();
                           
         break;
      }
      case cEventClassReloadNotify:
      {
         loadEffect();      
         
         break;
      }
   }
   
   return false;      
}

//==============================================================================
// BToneMapManager::clearTexture
//==============================================================================
void BToneMapManager::clearTexture(IDirect3DTexture9* pTex)
{
   XGTEXTURE_DESC desc;
   
   XGGetTextureDesc(pTex, 0, &desc);

   D3DLOCKED_RECT lockedRect;
   pTex->LockRect(0, &lockedRect, NULL, 0);

   memset(lockedRect.pBits, 0, desc.SlicePitch);

   pTex->UnlockRect(0);
}   

//==============================================================================
// BToneMapManager::initDeviceData
//==============================================================================
void BToneMapManager::initDeviceData(void)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   for (uint i = 0; i < 2; i++)
   {
      HRESULT status = BD3D::mpDev->CreateTexture(1, 1, 1, 0, D3DFMT_G32R32F, 0, &mpAveLumTex1[i], NULL);
      BVERIFY(SUCCEEDED(status));

      clearTexture(mpAveLumTex1[i]);
   }
}

//==============================================================================
// BToneMapManager::allocateWorkTextures
//==============================================================================
void BToneMapManager::allocateWorkTextures(void)
{
   mWidthDiv2 = Math::Max<uint>(1, gTiledAAManager.getWidth() / 2);
   mHeightDiv2 = Math::Max<uint>(1, gTiledAAManager.getHeight() / 2);

   mWidthDiv4 = Math::Max<uint>(1, mWidthDiv2 / 2);
   mHeightDiv4 = Math::Max<uint>(1, mHeightDiv2 / 2);

   mWidthDiv16 = Math::Max<uint>(1, mWidthDiv4 / 4);
   mHeightDiv16 = Math::Max<uint>(1, mHeightDiv4 / 4);

   mWidthDiv64 = Math::Max<uint>(1, mWidthDiv16 / 4);
   mHeightDiv64 = Math::Max<uint>(1, mHeightDiv16 / 4);
         
   // Textures
   HRESULT status;
   
   status = gGPUFrameHeap.createTexture(mWidthDiv2, mHeightDiv2, 1, 0, D3DFMT_A16B16G16R16F_EXPAND, 0, &mpRadTex2, NULL);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createTexture(mWidthDiv4, mHeightDiv4, 1, 0, D3DFMT_A16B16G16R16F_EXPAND, 0, &mpRadTex4, NULL);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createTexture(mWidthDiv16, mHeightDiv16, 1, 0, D3DFMT_A16B16G16R16F_EXPAND, 0, &mpAveLumTex16, NULL);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createTexture(mWidthDiv64, mHeightDiv64, 1, 0, D3DFMT_A16B16G16R16F_EXPAND, 0, &mpAveLumTex64, NULL);
   BVERIFY(SUCCEEDED(status));

   for (uint i = 0; i < cNumTempBufs; i++)
   {
      if (mParams[mCurrentViewportIndex].mQuarterResBlooms)
         status = gGPUFrameHeap.createTexture(mWidthDiv4, mHeightDiv4, 1, 0, D3DFMT_A2R10G10B10, 0, &mpBloomBuf[i], NULL);
      else
         status = gGPUFrameHeap.createTexture(mWidthDiv2, mHeightDiv2, 1, 0, D3DFMT_A2R10G10B10, 0, &mpBloomBuf[i], NULL);
      BVERIFY(SUCCEEDED(status));
   }
   
   // Surfaces
   D3DSURFACE_PARAMETERS surfParams;
   Utils::ClearObj(surfParams);

   status = gGPUFrameHeap.createRenderTarget(mWidthDiv2, mHeightDiv2, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, 0, FALSE, &mpRadSurf2, &surfParams);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createRenderTarget(mWidthDiv4, mHeightDiv4, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, 0, FALSE, &mpRadSurf4, &surfParams);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createRenderTarget(mWidthDiv16, mHeightDiv16, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, 0, FALSE, &mpAveLumSurf16, &surfParams);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createRenderTarget(mWidthDiv64, mHeightDiv64, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, 0, FALSE, &mpAveLumSurf64, &surfParams);
   BVERIFY(SUCCEEDED(status));

   status = gGPUFrameHeap.createRenderTarget(1, 1, D3DFMT_G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE, &mpAveLumSurf1, &surfParams);
   BVERIFY(SUCCEEDED(status));

   if (mParams[mCurrentViewportIndex].mQuarterResBlooms)
      status = gGPUFrameHeap.createRenderTarget(mWidthDiv4, mHeightDiv4, D3DFMT_A2R10G10B10, D3DMULTISAMPLE_NONE, 0, FALSE, &mpBrightMaskSurf, &surfParams);
   else
      status = gGPUFrameHeap.createRenderTarget(mWidthDiv2, mHeightDiv2, D3DFMT_A2R10G10B10, D3DMULTISAMPLE_NONE, 0, FALSE, &mpBrightMaskSurf, &surfParams);
   BVERIFY(SUCCEEDED(status));
   
   // Purposely doesn't check mDOFEnabled because it could be set/reset at any time.
   // Also, screen blurring needs this buffer
   if (mParams[mCurrentViewportIndex].mDOFEnabled || (mParams[mCurrentViewportIndex].mBlurFactor > 0.0f))
   {
      status = gGPUFrameHeap.createTexture(mWidthDiv4, mHeightDiv4, 1, 0, D3DFMT_A16B16G16R16F_EXPAND, 0, &mpDOFBlurTex, NULL);
      BVERIFY(SUCCEEDED(status));

      status = gGPUFrameHeap.createRenderTarget(mWidthDiv4, mHeightDiv4, D3DFMT_A16B16G16R16F, D3DMULTISAMPLE_NONE, 0, FALSE, &mpDOFBlurSurf, &surfParams);
      BVERIFY(SUCCEEDED(status));
   }
   
   D3DSURFACE_DESC desc;
   mpRadSurf2->GetDesc(&desc);
   BDEBUG_ASSERT(desc.Width >= (gTiledAAManager.getWidth() / 2));
   BDEBUG_ASSERT(desc.Height >= (gTiledAAManager.getHeight() / 2));      
}

//==============================================================================
// BToneMapManager::releaseWorkTextures
//==============================================================================
void BToneMapManager::releaseWorkTextures(void)
{
   gRenderDraw.unsetTextures();
   
   BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);
   BD3D::mpDev->SetDepthStencilSurface(BD3D::mpDevDepthStencil);

   // Textures
   gGPUFrameHeap.releaseD3DResource(mpRadTex2);
   mpRadTex2 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpRadTex4);
   mpRadTex4 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpAveLumTex16);
   mpAveLumTex16 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpAveLumTex64);
   mpAveLumTex64 = NULL;
   
   for (uint i = 0; i < cNumTempBufs; i++)
   {
      gGPUFrameHeap.releaseD3DResource(mpBloomBuf[i]);
      mpBloomBuf[i] = NULL;
   }
   
   // Surfaces
   gGPUFrameHeap.releaseD3DResource(mpAveLumSurf1);
   mpAveLumSurf1 = NULL;

   gGPUFrameHeap.releaseD3DResource(mpAveLumSurf64);
   mpAveLumSurf64 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpAveLumSurf16);
   mpAveLumSurf16 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpRadSurf4);
   mpRadSurf4 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpRadSurf2);
   mpRadSurf2 = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpBrightMaskSurf);
   mpBrightMaskSurf = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpDOFBlurTex);
   mpDOFBlurTex = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpDOFBlurSurf);
   mpDOFBlurSurf = NULL;
}

//==============================================================================
// BToneMapManager::deinitDeviceData
//==============================================================================
void BToneMapManager::deinitDeviceData(void)
{
   mToneMapEffect.clear();
         
   for (uint i = 0; i < 2; i++)
   {
      if (mpAveLumTex1[i])
      {
         mpAveLumTex1[i]->Release();
         mpAveLumTex1[i] = NULL;
      }
   }   
}

//==============================================================================
// BToneMapManager::frameBegin
//==============================================================================
void BToneMapManager::frameBegin(void)
{
}

//==============================================================================
// struct BQuadVert
//==============================================================================
struct BQuadVert
{
   XMFLOAT2 pos;
   XMFLOAT2 uv0;
   XMFLOAT2 uv1;
   
   BQuadVert() { }
   BQuadVert(float x, float y, float u0, float v0, float u1, float v1) { pos.x = x; pos.y = y; uv0.x = u0; uv0.y = v0; uv1.x = u1; uv1.y = v1; }
};

static const DWORD g_dwQuadGridSizeX  = 8; // 1280 / 8 = 160
static const DWORD g_dwQuadGridSizeY  = 1; //  720 / 1 = 720
static const DWORD g_dwNumQuadsInGrid = g_dwQuadGridSizeX*g_dwQuadGridSizeY;

//==============================================================================
// BToneMapManager::renderQuad
//==============================================================================
void BToneMapManager::renderQuad(
   int x, int y, int width, int height, 
   float ofsX, float ofsY, 
   bool grid, 
   float uLo0, float uHi0, float vLo0, float vHi0,
   float uLo1, float uHi1, float vLo1, float vHi1)
{
#ifdef BUILD_DEBUG   
   DWORD val;
   BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &val);
   BDEBUG_ASSERT(val == FALSE);
#endif   

   XMMATRIX matrix = XMMatrixIdentity();
   BD3D::mpDev->SetVertexShaderConstantF(0, reinterpret_cast<float*>(&matrix), 4);

   // Dummy vertex decl
   BD3D::mpDev->SetVertexDeclaration(BTLVertex::msVertexDecl);

   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);

   const uint numVerts = grid ? 3 * (g_dwQuadGridSizeX * g_dwQuadGridSizeY) : 3;
   BQuadVert* pVB = static_cast<BQuadVert*>(gRenderDraw.lockDynamicVB(numVerts, sizeof(BQuadVert)));

   BD3D::mpDev->SetVertexFetchConstant(0, gRenderDraw.getDynamicVB(), 0);

   if (grid)
   {
      FLOAT fGridDimX = 1.0f / (FLOAT)g_dwQuadGridSizeX;
      FLOAT fGridDimY = 1.0f / (FLOAT)g_dwQuadGridSizeY;
      FLOAT fGridDimU = 1.0f / (FLOAT)g_dwQuadGridSizeX;
      FLOAT fGridDimV = 1.0f / (FLOAT)g_dwQuadGridSizeY;
      FLOAT T  = 0.0f;
      FLOAT V0 = 0.0f;
      
      for( DWORD iy=0; iy<g_dwQuadGridSizeY; iy++ )
      {
         FLOAT L  = 0.0f;
         FLOAT U0 = 0.0f;
         for( DWORD ix=0; ix<g_dwQuadGridSizeX; ix++ )
         {
            FLOAT R = L + fGridDimX;
            FLOAT B = T + fGridDimY;
            FLOAT U1 = U0 + fGridDimU;
            FLOAT V1 = V0 + fGridDimV;

            *pVB++ = BQuadVert( x + ofsX + L * width, y + ofsY + T * height, Math::Lerp(uLo0, uHi0, U0), Math::Lerp(vLo0, vHi0, V0), Math::Lerp(uLo1, uHi1, U0), Math::Lerp(vLo1, vHi1, V0) );
            *pVB++ = BQuadVert( x + ofsX + R * width, y + ofsY + T * height, Math::Lerp(uLo0, uHi0, U1), Math::Lerp(vLo0, vHi0, V0), Math::Lerp(uLo1, uHi1, U1), Math::Lerp(vLo1, vHi1, V0) );
            *pVB++ = BQuadVert( x + ofsX + L * width, y + ofsY + B * height, Math::Lerp(uLo0, uHi0, U0), Math::Lerp(vLo0, vHi0, V1), Math::Lerp(uLo1, uHi1, U0), Math::Lerp(vLo1, vHi1, V1) );
            
            L  += fGridDimX;
            U0 += fGridDimU;
         }
         
         T  += fGridDimY;
         V0 += fGridDimV;
      }
   }
   else
   {
      pVB->pos = XMFLOAT2(x + ofsX, y + ofsY);
      pVB->uv0 = XMFLOAT2(uLo0, vLo0);
      pVB->uv1 = XMFLOAT2(uLo1, vLo1);
      pVB++;

      pVB->pos = XMFLOAT2(x + width + ofsX, y + ofsY);
      pVB->uv0 = XMFLOAT2(uHi0, vLo0);
      pVB->uv1 = XMFLOAT2(uHi1, vLo1);
      pVB++;

      pVB->pos = XMFLOAT2(x + ofsX, y + height + ofsY);
      pVB->uv0 = XMFLOAT2(uLo0, vHi0);
      pVB->uv1 = XMFLOAT2(uLo1, vHi1);
   }      

   gRenderDraw.unlockDynamicVB();

   BD3D::mpDev->DrawVertices(D3DPT_RECTLIST, 0, numVerts);
}

//==============================================================================
// BToneMapManager::reduct4
//==============================================================================
void BToneMapManager::reduct4(
   IDirect3DSurface9* pDstSurf, IDirect3DTexture9* pDstTex, 
   IDirect3DTexture9* pSrc, uint srcWidth, uint srcHeight, 
   uint dstWidth, uint dstHeight, bool grid, uint techniqueIndex)
{
   SCOPEDSAMPLE(Reduct4);
   
   BD3D::mpDev->SetRenderTarget(0, pDstSurf);
   
   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(techniqueIndex);
         
   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, pSrc);
  
   D3DSURFACE_DESC levelDesc;
   pSrc->GetLevelDesc(0, &levelDesc); 
   float ooWidth = 1.0f / levelDesc.Width;
   float ooHeight = 1.0f / levelDesc.Height;
            
   BVec4 mul(2.0f * ooWidth, 2.0f * ooHeight, 0, 0);
   BVec4 add(-1.0f * ooWidth, -1.0f * ooHeight, 0, 1);
   
   BVec4 v[4];
   v[0] = BVec4::multiply(mul, BVec4(0, 0, 0, 0)) + add;
   v[1] = BVec4::multiply(mul, BVec4(1, 0, 0, 0)) + add;
   v[2] = BVec4::multiply(mul, BVec4(0, 1, 0, 0)) + add;
   v[3] = BVec4::multiply(mul, BVec4(1, 1, 0, 0)) + add;
   
   BD3D::mpDev->SetPixelShaderConstantF(0, v[0].getPtr(), 4);
  
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = srcWidth / (float)levelDesc.Width;
   float vHi = srcHeight / (float)levelDesc.Height;
   renderQuad(0, 0, dstWidth, dstHeight, -.5f, -.5f, grid, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);
      
   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, pDstTex, NULL, 0, 0, &clearColor, 0.0f, 0, NULL);
}

//==============================================================================
// BToneMapManager::reduct2
//==============================================================================
void BToneMapManager::reduct2(
   IDirect3DSurface9* pDstSurf, IDirect3DTexture9* pDstTex, 
   IDirect3DTexture9* pSrc, uint srcWidth, uint srcHeight, 
   uint dstWidth, uint dstHeight, bool grid, uint techniqueIndex)
{
   SCOPEDSAMPLE(Reduct2);

   BD3D::mpDev->SetRenderTarget(0, pDstSurf);

   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(techniqueIndex);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, pSrc);
   
   D3DSURFACE_DESC levelDesc;
   pSrc->GetLevelDesc(0, &levelDesc);
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = srcWidth / (float)levelDesc.Width;
   float vHi = srcHeight / (float)levelDesc.Height;
   renderQuad(0, 0, dstWidth, dstHeight, -.5f, -.5f, grid, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);

   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, pDstTex, NULL, 0, 0, &clearColor, 0.0f, 0, NULL);
}

//==============================================================================
// BToneMapManager::reduct1
//==============================================================================
void BToneMapManager::reduct1(
   IDirect3DSurface9* pDstSurf, IDirect3DTexture9* pDstTex, 
   IDirect3DTexture9* pSrc, uint srcWidth, uint srcHeight, 
   uint techniqueIndex)
{
   SCOPEDSAMPLE(Reduct1);
   
   BD3D::mpDev->SetRenderTarget(0, pDstSurf);//mpAveLumSurf1);
      
   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(techniqueIndex);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, pSrc);
   
   D3DSURFACE_DESC levelDesc;
   pSrc->GetLevelDesc(0, &levelDesc);
      
   float ooWidth = 1.0f / levelDesc.Width;
   float ooHeight = 1.0f / levelDesc.Height;
   
   BVec4 v[4];
   v[0].set(2.0f * ooWidth, 2.0f * ooHeight, 0, 0);
   v[1].set(ooWidth, ooHeight, 0.0f, 1.0f);
   v[2].set(1.0f / ((mWidthDiv16 * mHeightDiv16) * 4.0f));
   v[3].set(mParams[mCurrentViewportIndex].mLogAveMin, mParams[mCurrentViewportIndex].mLogAveMax, mParams[mCurrentViewportIndex].mWhitePointMin, mParams[mCurrentViewportIndex].mWhitePointMax);
   BD3D::mpDev->SetPixelShaderConstantF(0, v[0].getPtr(), 4);
         
   int ic[8] = { (srcWidth + 1) / 2, 0, 1, 0,    (srcHeight + 1) / 2, 0, 1, 0 };
   BD3D::mpDev->SetPixelShaderConstantI(0, ic, 2);

   renderQuad(0, 0, 1, 1);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);
   
   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);
      
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, pDstTex, NULL, 0, 0, &clearColor, 0.0f, 0, NULL);
}

//==============================================================================
// BToneMapManager::filterAdaptation
//==============================================================================
void BToneMapManager::filterAdaptation(bool resetAdapation, bool disableAdaptation)
{
   SCOPEDSAMPLE(FilterAdaptation);

   BD3D::mpDev->SetRenderTarget(0, mpAveLumSurf1);

   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueFilterAdaptation);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   const uint prevLumTexIndex = mCurLumTexIndex ^ 1;
   BD3D::mpDev->SetTexture(0, mpAveLumTex1[mCurLumTexIndex]);
   BD3D::mpDev->SetTexture(1, mpAveLumTex1[prevLumTexIndex]);

   float ar = mParams[mCurrentViewportIndex].mAdaptationRate;
   if (resetAdapation)
      ar = 1.0f;
   else if (disableAdaptation)
      ar = 0.0f;
      
   BVec4 v(ar);
   BD3D::mpDev->SetPixelShaderConstantF(0, v.getPtr(), 1);
   
   renderQuad(0, 0, 1, 1);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);
   BD3D::mpDev->SetTexture(1, NULL);

   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);

   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpAveLumTex1[mCurLumTexIndex], NULL, 0, 0, &clearColor, 0.0f, 0, NULL);
}

//==============================================================================
// BToneMapManager::computeAdaptation
//==============================================================================
void BToneMapManager::computeAdaptation(bool resetAdapation, bool disableAdaptation)
{
   reduct2(
      mpRadSurf2, mpRadTex2, 
      gTiledAAManager.getColorTexture(), gTiledAAManager.getWidth(), gTiledAAManager.getHeight(), 
      mWidthDiv2, mHeightDiv2, true, cTechniqueReduct2LinRGBAve);
   
   reduct2(
      mpRadSurf4, mpRadTex4, 
      mpRadTex2, mWidthDiv2, mHeightDiv2, 
      mWidthDiv4, mHeightDiv4, true, cTechniqueReduct2LinRGBAve);
   
   reduct4(
      mpAveLumSurf16, mpAveLumTex16, 
      mpRadTex4, mWidthDiv4, mHeightDiv4, mWidthDiv16, 
      mHeightDiv16, false, cTechniqueReduct4LogSum);
   
   reduct4(
      mpAveLumSurf64, mpAveLumTex64, 
      mpAveLumTex16, mWidthDiv16, mHeightDiv16, 
      mWidthDiv64, mHeightDiv64, false, cTechniqueReduct4LinSum);
      
   reduct1(
      mpAveLumSurf1, mpAveLumTex1[mCurLumTexIndex], 
      mpAveLumTex64, mWidthDiv64, mHeightDiv64, 
      cTechniqueReduct1);
   
   filterAdaptation(resetAdapation, disableAdaptation);
}

//==============================================================================
// gaussianDistribution
//==============================================================================
static FLOAT gaussianDistribution( FLOAT x, FLOAT y, FLOAT rho )
{
   return expf( -(x*x + y*y)/(2*rho*rho) ) / sqrtf( 2*XM_PI*rho*rho );
}

//==============================================================================
// getSampleOffsetsGaussBlur5x5
//==============================================================================
static VOID getSampleOffsetsGaussBlur5x5( 
   DWORD dwTexWidth, DWORD dwTexHeight,
   XMVECTOR* pvTexCoordOffsets,
   XMVECTOR* pvSampleWeights,
   FLOAT fMultiplier,
   float fSigma )
{
   FLOAT tu = 1.0f / (FLOAT)dwTexWidth;
   FLOAT tv = 1.0f / (FLOAT)dwTexHeight;

   XMVECTOR vWhite = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );

   FLOAT fTotalWeight = 0.0f;
   DWORD index=0;
   for( int x = -2; x <= 2; x++ )
   {
      for( int y = -2; y <= 2; y++ )
      {
         // Exclude pixels with a block distance greater than 2. This will
         // create a kernel which approximates a 5x5 kernel using only 13
         // sample points instead of 25; this is necessary since 2.0 shaders
         // only support 16 texture grabs.
         if( fabs((FLOAT)x) + fabs((FLOAT)y) > 2.0f )
            continue;

         // Get the unscaled Gaussian intensity for this offset
         pvTexCoordOffsets[index].x = (FLOAT)x * tu;
         pvTexCoordOffsets[index].y = (FLOAT)y * tv;
         pvTexCoordOffsets[index].z = 0.0f;
         pvTexCoordOffsets[index].w = 0.0f;

         pvSampleWeights[index] = vWhite * gaussianDistribution( (FLOAT)x, (FLOAT)y, fSigma );

         fTotalWeight += pvSampleWeights[index].x;

         index++;
      }
   }

   // Divide the current weight by the total weight of all the samples; Gaussian
   // blur kernels add to 1.0f to ensure that the intensity of the image isn't
   // changed when the blur occurs. An optional multiplier variable is used to
   // add or remove image intensity during the blur.
   for( DWORD i=0; i<index; i++ )
   {
      pvSampleWeights[i] /= fTotalWeight;
      pvSampleWeights[i] *= fMultiplier;
   }
}

//==============================================================================
// getSampleOffsetsDirectionalBlur
//==============================================================================
static VOID getSampleOffsetsDirectionalBlur( 
   DWORD dwTexWidth, DWORD dwTexHeight,
   XMVECTOR* pvTexCoordOffsets,
   XMVECTOR* pvSampleWeights,
   float fMaxXOffset, float fMaxYOffset )
{
   //FLOAT tu = 1.0f / (FLOAT)dwTexWidth;
   //FLOAT tv = 1.0f / (FLOAT)dwTexHeight;

   XMVECTOR vWhite = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );

   FLOAT fTotalWeight = 0.0f;
   DWORD index=0;
   uint numSamples = 13;
   for (uint i = 0; i < numSamples; i++)
   {
      pvTexCoordOffsets[index].x = fMaxXOffset * (i / float(numSamples));
      pvTexCoordOffsets[index].y = fMaxYOffset * (i / float(numSamples));
      pvTexCoordOffsets[index].z = 0.0f;
      pvTexCoordOffsets[index].w = 0.0f;

      //pvSampleWeights[index] = vWhite * gaussianDistribution( (FLOAT)x, (FLOAT)y, fSigma );
      pvSampleWeights[index] = vWhite * float(numSamples - i);

      fTotalWeight += pvSampleWeights[index].x;
      index++;
   }

   // Divide the current weight by the total weight of all the samples; Gaussian
   // blur kernels add to 1.0f to ensure that the intensity of the image isn't
   // changed when the blur occurs. An optional multiplier variable is used to
   // add or remove image intensity during the blur.
   for( DWORD i=0; i<index; i++ )
   {
      pvSampleWeights[i] /= fTotalWeight;
   }
}

//==============================================================================
// BToneMapManager::DOFBlur
//==============================================================================
void BToneMapManager::DOFBlur(void)
{
   if (((!mParams[mCurrentViewportIndex].mDOFEnabled) || (!mDOFEnabled)) && (mParams[mCurrentViewportIndex].mBlurFactor <= 0.0f || !mPostEffectsEnabled))
      return;

   SCOPEDSAMPLE(DOFBlur);

   XMVECTOR blurWeights[16];
   XMVECTOR blurOffsets[16];

   // Calculate blur weights/offsets.  Radial blur calcs offsets in shader, but uses directional weights.
   if (mPostEffectsEnabled && mParams[mCurrentViewportIndex].directionalOrRadialBlurring())
      getSampleOffsetsDirectionalBlur(mWidthDiv4, mHeightDiv4, blurOffsets, blurWeights, mParams[mCurrentViewportIndex].mBlurX, mParams[mCurrentViewportIndex].mBlurY);
   else
      getSampleOffsetsGaussBlur5x5(mWidthDiv4, mHeightDiv4, blurOffsets, blurWeights, 1.0f, mParams[mCurrentViewportIndex].mDOFMaxBlurriness);

   BD3D::mpDev->SetRenderTarget(0, mpDOFBlurSurf);

   // Get technique
   BFXLEffectTechnique technique;
   if (mPostEffectsEnabled && mParams[mCurrentViewportIndex].radialBlurring())
      technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueRadialBlur);
   else
      technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueGaussBlur5x5);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, mpRadTex4);

   // Set constants
   if (mPostEffectsEnabled && mParams[mCurrentViewportIndex].radialBlurring())
   {
      BVec4 radialBlurParams(mParams[mCurrentViewportIndex].mBlurX, mParams[mCurrentViewportIndex].mBlurY, mParams[mCurrentViewportIndex].mRadialBlurScale, mParams[mCurrentViewportIndex].mRadialBlurMax);
      BD3D::mpDev->SetPixelShaderConstantF(0, radialBlurParams.getPtr(), 1);
      BD3D::mpDev->SetPixelShaderConstantF(16, (const float*)blurWeights, 16);
   }
   else
   {
      BD3D::mpDev->SetPixelShaderConstantF(0, (const float*)blurOffsets, 16);
      BD3D::mpDev->SetPixelShaderConstantF(16, (const float*)blurWeights, 16);
   }

   renderQuad(0, 0, mWidthDiv4, mHeightDiv4, -.5f, -.5f, false);

   technique.endPass();

   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);   
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpDOFBlurTex, NULL, 0, 0, &clearColor, 0.0f, 0, NULL);

   BD3D::mpDev->SetTexture(0, NULL);
}

//==============================================================================
// BToneMapManager::finalPass
//==============================================================================
void BToneMapManager::finalPass(IDirect3DTexture9* pDistortionTexture, uint xOfs, uint yOfs, bool unswizzleRadianceBuf)
{
   SCOPEDSAMPLE(FinalPass);
         
   BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);
   
   if ((gTiledAAManager.getWidth() < BD3D::mD3DPP.BackBufferWidth) || (gTiledAAManager.getHeight() < BD3D::mD3DPP.BackBufferHeight))
   {
      BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
   }
         
   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex((mParams[mCurrentViewportIndex].mDOFEnabled && mDOFEnabled && (!mParams[mCurrentViewportIndex].directionalOrRadialBlurring() || !mPostEffectsEnabled)) ? cTechniqueFinalDOF : cTechniqueFinal);

   D3DSURFACE_DESC colorTextureDesc;
   gTiledAAManager.getColorTexture()->GetLevelDesc(0, &colorTextureDesc);
   
   mToneMapEffect.getParam("gTextureWidthHeight") = BVec2((float)colorTextureDesc.Width, (float)colorTextureDesc.Height);
   mToneMapEffect.getParam("gTextureInvWidthHeight") = BVec2(1.0f / colorTextureDesc.Width, 1.0f / colorTextureDesc.Height);
      
   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, gTiledAAManager.getColorTexture());
   BD3D::mpDev->SetTexture(1, mpAveLumTex1[mCurLumTexIndex]);
   BD3D::mpDev->SetTexture(2, mpBloomBuf[2]);
   BD3D::mpDev->SetTexture(3, mpBloomBuf[0]);
         
   // This sets a dummy texture if distortion is disabled to avoid PIX errors.
   BD3D::mpDev->SetTexture(4, pDistortionTexture ? pDistortionTexture : gTiledAAManager.getColorTexture());
   
   BD3D::mpDev->SetTexture(5, mpRadTex4);
   
   // This sets a dummy texture if DOF is disabled to avoid PIX errors.
   BD3D::mpDev->SetTexture(6, ((mParams[mCurrentViewportIndex].mDOFEnabled && mDOFEnabled) || (mParams[mCurrentViewportIndex].mBlurFactor > 0.0f && mPostEffectsEnabled)) ? mpDOFBlurTex : gTiledAAManager.getColorTexture());
         
   BD3D::mpDev->SetTexture(7, gTiledAAManager.getDepthTexture());
   
   const XMMATRIX& screenToView = gRenderDraw.getWorkerSceneMatrixTracker().getMatrix(cMTScreenToView);
   
   BVec4 v(mParams[mCurrentViewportIndex].mMiddleGrey, mParams[mCurrentViewportIndex].mColorTransformFactor, mParams[mCurrentViewportIndex].mBlurFactor, 0);
   BD3D::mpDev->SetPixelShaderConstantF(0, v.getPtr(), 1);
   
   BVec4 w(screenToView._34, screenToView._44, mParams[mCurrentViewportIndex].mDOFFocalPlaneDist, mParams[mCurrentViewportIndex].mDOFNearBlurPlaneDist);
   BD3D::mpDev->SetPixelShaderConstantF(1, w.getPtr(), 1);
   
   BVec4 z(
      mParams[mCurrentViewportIndex].mDOFFarBlurPlaneDist, 
      1.0f / Math::Max(.0000125f, (mParams[mCurrentViewportIndex].mDOFFocalPlaneDist - mParams[mCurrentViewportIndex].mDOFNearBlurPlaneDist)), 
      1.0f / Math::Max(.0000125f, (mParams[mCurrentViewportIndex].mDOFFarBlurPlaneDist - mParams[mCurrentViewportIndex].mDOFFocalPlaneDist)), 0.0f);
   BD3D::mpDev->SetPixelShaderConstantF(2, z.getPtr(), 1);

   // Color transform matrix
   BD3D::mpDev->SetPixelShaderConstantF(4, reinterpret_cast<float*>(&mParams[mCurrentViewportIndex].mRTransform), 1);
   BD3D::mpDev->SetPixelShaderConstantF(5, reinterpret_cast<float*>(&mParams[mCurrentViewportIndex].mGTransform), 1);
   BD3D::mpDev->SetPixelShaderConstantF(6, reinterpret_cast<float*>(&mParams[mCurrentViewportIndex].mBTransform), 1);

   const BOOL bDistortionEnabled = (pDistortionTexture != NULL);//gPSManager.isDistortionEnabled();
   BD3D::mpDev->SetPixelShaderConstantB(0, &bDistortionEnabled, 1);

   // Color transform, blur bools
   BOOL desat = FALSE;
   BOOL blur = FALSE;
   if (mPostEffectsEnabled)
   {
      desat = mParams[mCurrentViewportIndex].mColorTransformFactor > 0.0f ? TRUE : FALSE;
      blur = mParams[mCurrentViewportIndex].mBlurFactor > 0.0f ? TRUE : FALSE;
   }
   
   BD3D::mpDev->SetPixelShaderConstantB(1, &desat, 1);
   BD3D::mpDev->SetPixelShaderConstantB(2, &blur, 1);
  
   BOOL bUnswizzleRadianceBuffer = unswizzleRadianceBuf ? TRUE : FALSE;
   BD3D::mpDev->SetPixelShaderConstantB(3, &bUnswizzleRadianceBuffer, 1);

   D3DSURFACE_DESC levelDesc;
   gTiledAAManager.getColorTexture()->GetLevelDesc(0, &levelDesc);
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = gTiledAAManager.getWidth() / (float)levelDesc.Width;
   float vHi = gTiledAAManager.getHeight() / (float)levelDesc.Height;
   renderQuad(xOfs, yOfs, gTiledAAManager.getWidth(), gTiledAAManager.getHeight(), -.5f, -.5f, true, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);
   BD3D::mpDev->SetTexture(1, NULL);
}

//==============================================================================
// BToneMapManager::brightMask
//==============================================================================
void BToneMapManager::brightMask(void)
{
   SCOPEDSAMPLE(BrightMask);

   BD3D::mpDev->SetRenderTarget(0, mpBrightMaskSurf);

   IDirect3DTexture9* pSrcTex = mpRadTex2;
   
   uint widthDiv = mWidthDiv2;
   uint heightDiv = mHeightDiv2;
   if (mParams[mCurrentViewportIndex].mQuarterResBlooms)
   {
      pSrcTex = mpRadTex4;
      widthDiv = mWidthDiv4;
      heightDiv = mHeightDiv4;
   }
   
   D3DSURFACE_DESC levelDesc;
   pSrcTex->GetLevelDesc(0, &levelDesc);
   if ((widthDiv < levelDesc.Width) || (heightDiv < levelDesc.Height)) 
      BD3D::mpDev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
            
   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueBrightMask);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, pSrcTex);
   BD3D::mpDev->SetTexture(1, mpAveLumTex1[mCurLumTexIndex]);

   BVec4 v[2];
   v[0].set(mParams[mCurrentViewportIndex].mMiddleGrey, 0, 0, 0);
   v[1].set(mParams[mCurrentViewportIndex].mBrightMaskThresh, .5f);
   
   BD3D::mpDev->SetPixelShaderConstantF(0, v[0].getPtr(), 2);
   
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = widthDiv / (float)levelDesc.Width;
   float vHi = heightDiv / (float)levelDesc.Height;
   renderQuad(0, 0, widthDiv, heightDiv, -.5f, -.5f, false, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);
   
   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpBloomBuf[0], NULL, 0, 0, &clearColor, 0.0f, 0, NULL);
}

//==============================================================================
// BToneMapManager::bloomFilter
//==============================================================================
void BToneMapManager::bloomFilter(void)
{
   SCOPEDSAMPLE(BloomFilter);
   
   const int cMaxSamples = MAX_BLOOM_FILTER_SAMPLES;

   BVec4 horizontalSamples[cMaxSamples];
   BVec4 verticalSamples[cMaxSamples];
   BVec4 scales[cMaxSamples];
   Utils::ClearObj(horizontalSamples);
   Utils::ClearObj(verticalSamples);
   Utils::ClearObj(scales);
   
   D3DSURFACE_DESC levelDesc;
   mpBloomBuf[0]->GetLevelDesc(0, &levelDesc);
   const uint textureWidth = levelDesc.Width;
   const uint textureHeight = levelDesc.Height;
         
   BBlurHelper blurHelper;
   const uint sampleCount = blurHelper.computeBilinearBloomSamples(mParams[mCurrentViewportIndex].mBloomSigma, mParams[mCurrentViewportIndex].mBloomIntensity, scales, horizontalSamples, verticalSamples, cMaxSamples, textureWidth, textureHeight);
   BDEBUG_ASSERT((sampleCount >= 1) && (sampleCount <= cMaxSamples));
         
   BD3D::mpDev->SetRenderTarget(0, mpBrightMaskSurf);
   
   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueBloomFilter);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, mpBloomBuf[0]);
   
   BD3D::mpDev->SetPixelShaderConstantF(0, horizontalSamples[0].getPtr(), cMaxSamples);
   BD3D::mpDev->SetPixelShaderConstantF(cMaxSamples, scales[0].getPtr(), cMaxSamples);
   
   const int ic[4] = { (sampleCount + 3) / 4, 0, 4, 0 };
   BD3D::mpDev->SetPixelShaderConstantI(0, ic, 1);

   uint widthDiv = mWidthDiv2;
   uint heightDiv = mHeightDiv2;
   if (mParams[mCurrentViewportIndex].mQuarterResBlooms)
   {
      widthDiv = mWidthDiv4;
      heightDiv = mHeightDiv4;
   }
  
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = widthDiv / (float)textureWidth;
   float vHi = heightDiv / (float)textureHeight;
   renderQuad(0, 0, widthDiv, heightDiv, -.5f, -.5f, false, uLo, uHi, vLo, vHi);

   technique.endPass();

   D3DVECTOR4 clearColor;
   Utils::ClearObj(clearColor);   
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpBloomBuf[1], NULL, 0, 0, &clearColor, 0.0f, 0, NULL);
     
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, mpBloomBuf[1]);

   BD3D::mpDev->SetPixelShaderConstantF(0, verticalSamples[0].getPtr(), cMaxSamples);
   
   renderQuad(0, 0, widthDiv, heightDiv, -.5f, -.5f, false, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();

   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpBloomBuf[2], NULL, 0, 0, &clearColor, 0.0f, 0, NULL);

   BD3D::mpDev->SetTexture(0, NULL);
}

//==============================================================================
// BToneMapManager::tonemap
//==============================================================================
void BToneMapManager::tonemap(
   IDirect3DTexture9* pDistortionTexture, 
   bool resetAdapation, 
   bool disableAdaptation, 
   uint xOfs, uint yOfs, 
   bool unswizzleRadianceBuf, 
   uint viewportIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   SCOPEDSAMPLE(Tonemap);

   mCurrentViewportIndex = viewportIndex;
   
   if (!mToneMapEffect.getEffect())
      return;
     
   allocateWorkTextures();
         
   mToneMapEffect.updateIntrinsicParams();
   
   BD3D::mpDev->SetDepthStencilSurface(NULL);

   computeAdaptation(resetAdapation, disableAdaptation);
         
   brightMask();
   
   bloomFilter();
   
   DOFBlur();

   finalPass(pDistortionTexture, xOfs, yOfs, unswizzleRadianceBuf);
      
   mCurLumTexIndex ^= 1;
   
   BD3D::mpDev->SetDepthStencilSurface(BD3D::mpDevDepthStencil);
   
   releaseWorkTextures();
}

//==============================================================================
// BToneMapManager::fillDepthStencilSurface
// rg FIXME: Move this to tiled AA manager.
//==============================================================================
void BToneMapManager::fillDepthStencilSurface(IDirect3DSurface9* pDstColor, IDirect3DSurface9* pDstDepth, uint width, uint height, bool unswizzleRadianceBuf)
{
   SCOPEDSAMPLE(FillDepthStencilSurface);
   
   BD3D::mpDev->SetRenderTarget(0, pDstColor);
   BD3D::mpDev->SetDepthStencilSurface(pDstDepth);
   
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueFillDepthStencilSurface);

   D3DSURFACE_DESC depthTextureDesc;
   gTiledAAManager.getColorTexture()->GetLevelDesc(0, &depthTextureDesc);

   mToneMapEffect.getParam("gTextureWidthHeight") = BVec2((float)depthTextureDesc.Width, (float)depthTextureDesc.Height);
   mToneMapEffect.getParam("gTextureInvWidthHeight") = BVec2(1.0f / depthTextureDesc.Width, 1.0f / depthTextureDesc.Height);
   
   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();

   BD3D::mpDev->SetTexture(0, gTiledAAManager.getDepthTexture());

   BOOL bUnswizzleRadianceBuffer = unswizzleRadianceBuf ? TRUE : FALSE;
   BD3D::mpDev->SetPixelShaderConstantB(3, &bUnswizzleRadianceBuffer, 1);

   D3DSURFACE_DESC srcTexLevelDesc;
   gTiledAAManager.getDepthTexture()->GetLevelDesc(0, &srcTexLevelDesc);
   
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = gTiledAAManager.getWidth() / (float)srcTexLevelDesc.Width;
   float vHi = gTiledAAManager.getHeight() / (float)srcTexLevelDesc.Height;
   renderQuad(0, 0, width, height, -.5f, -.5f, true, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();

   BD3D::mpDev->SetTexture(0, NULL);
}

//==============================================================================
// BToneMapManager::fillColorSurface
//==============================================================================
void BToneMapManager::fillColorSurface(IDirect3DSurface9* pDstColor, IDirect3DSurface9* pDstDepth, uint width, uint height, bool unswizzleRadianceBuf)
{
   SCOPEDSAMPLE(FillDepthColorSurface);

   BD3D::mpDev->SetRenderTarget(0, pDstColor);

   BFXLEffectTechnique technique = mToneMapEffect.getTechniqueFromIndex(cTechniqueFillColorSurface);

   D3DSURFACE_DESC colorTextureDesc;
   gTiledAAManager.getColorTexture()->GetLevelDesc(0, &colorTextureDesc);

   mToneMapEffect.getParam("gTextureWidthHeight") = BVec2((float)colorTextureDesc.Width, (float)colorTextureDesc.Height);
   mToneMapEffect.getParam("gTextureInvWidthHeight") = BVec2(1.0f / colorTextureDesc.Width, 1.0f / colorTextureDesc.Height);

   technique.beginRestoreDefaultState();
   technique.beginPass(0);
   technique.commit();
      
   BD3D::mpDev->SetTexture(0, gTiledAAManager.getColorTexture());

   BOOL bUnswizzleRadianceBuffer = unswizzleRadianceBuf ? TRUE : FALSE;
   BD3D::mpDev->SetPixelShaderConstantB(3, &bUnswizzleRadianceBuffer, 1);
         
   D3DSURFACE_DESC srcTexLevelDesc;
   gTiledAAManager.getColorTexture()->GetLevelDesc(0, &srcTexLevelDesc);
   
   float uLo = 0.0f;
   float vLo = 0.0f;
   float uHi = gTiledAAManager.getWidth() / (float)srcTexLevelDesc.Width;
   float vHi = gTiledAAManager.getHeight() / (float)srcTexLevelDesc.Height;
   renderQuad(0, 0, width, height, -.5f, -.5f, true, uLo, uHi, vLo, vHi);

   technique.endPass();
   technique.end();
     
   BD3D::mpDev->SetTexture(0, NULL);
}

//==============================================================================
// BToneMapManager::processCommand
//==============================================================================
void BToneMapManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cRenderCommandSetParams:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BToneMapParamsPayload));
         const BToneMapParamsPayload* pPayload = reinterpret_cast<const BToneMapParamsPayload*>(pData);
         mParams[pPayload->mViewportIndex] = pPayload->mParams;
         break;
      }
   }
}

//==============================================================================
// BToneMapManager::frameEnd
//==============================================================================
void BToneMapManager::frameEnd(void)
{
}
