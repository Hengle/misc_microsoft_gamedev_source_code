//============================================================================
// File: nearLayerManager.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "xgameRender.h"
#include "nearLayerManager.h"
#include "gpuHeap.h"
#include "renderDraw.h"
#include "render.h"
#include "primDraw2D.h"
#include "camera.h"
#include "inputsystem.h"
#include "effectIntrinsicManager.h"

BNearLayerManager gNearLayerManager;

//============================================================================
// BNearLayerManager::BNearLayerManager
//============================================================================
BNearLayerManager::BNearLayerManager() :
   mWidth(1024),
   mHeight(576),
   mRenderFormat(D3DFMT_A2B10G10R10F_EDRAM),
   mDepthFormat(D3DFMT_D24S8),
   mTextureFormat(D3DFMT_A16B16G16R16F_EXPAND),
   mMultisampleType(D3DMULTISAMPLE_2_SAMPLES),
   mpRenderTarget(NULL),
   mpDepthStencil(NULL),
   mpTexture(NULL),
   mEnabled(false),
   mFullScreenDraw(true)
{
}

//============================================================================
// BNearLayerManager::~BNearLayerManager
//============================================================================
BNearLayerManager::~BNearLayerManager()
{
}

//============================================================================
// BNearLayerManager::allocateBuffer
//============================================================================
void BNearLayerManager::allocateBuffer()
{
   if (mpTexture)
      return;
         
   HRESULT hres = gGPUFrameHeap.createTexture(mWidth, mHeight, 1, 0, mTextureFormat, 0, &mpTexture, NULL);
   BVERIFY(SUCCEEDED(hres));
         
   //gEffectIntrinsicManager.set(cIntrinsicReflectionTexture, &mpReflectionBuffer, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BNearLayerManager::releaseBuffer
//============================================================================
void BNearLayerManager::releaseBuffer()
{
   if (!mpTexture)
      return;
      
   gRenderDraw.unsetTextures();      
   
   gGPUFrameHeap.releaseD3DResource(mpTexture);
   mpTexture = NULL;
               
   //gEffectIntrinsicManager.set(cIntrinsicReflectionTexture, &mpReflectionBuffer, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BNearLayerManager::renderBegin
//============================================================================
void BNearLayerManager::renderBegin()
{
   ASSERT_THREAD(cThreadIndexRender);

   // Alloc temp render target / depth buffer
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);
   surfaceParams.Base = 0;

   HRESULT hres = gGPUFrameHeap.createRenderTarget(mWidth, mHeight, mRenderFormat, mMultisampleType, 0, FALSE, &mpRenderTarget, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));
   
   surfaceParams.Base = XGSurfaceSize(mWidth, mHeight, mRenderFormat, mMultisampleType);
   hres = gGPUFrameHeap.createRenderTarget(mWidth, mHeight, mDepthFormat, mMultisampleType, 0, FALSE, &mpDepthStencil, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));

   // Set up renderering matrices and viewport
   setMatricesAndViewport();
   
   // Set up lighting

   // Clear color and depth
   gRenderDraw.clear(D3DCLEAR_TARGET0|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0x00000000);
}

//============================================================================
// BNearLayerManager::renderEnd
//============================================================================
void BNearLayerManager::renderEnd()
{
   // Resolve render target to texure
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpTexture, NULL, 0, 0, NULL, 1.0f, 0, NULL);

   // Reset render stuff
   //gRenderDraw.getWorkerActiveVolumeCuller().disableExclusionPlanes();
   //gRenderDraw.getWorkerActiveVolumeCuller().disableInclusionPlanes();
   gRenderDraw.resetWorkerActiveMatricesAndViewport();

   // Release render target / depth buffer
   gGPUFrameHeap.releaseD3DResource(mpRenderTarget);
   mpRenderTarget = NULL;
   
   gGPUFrameHeap.releaseD3DResource(mpDepthStencil);
   mpDepthStencil = NULL;
}

//============================================================================
//============================================================================
void BNearLayerManager::setMatricesAndViewport()
{
   BMatrixTracker&  matrixTracker = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport = gRenderDraw.getWorkerActiveRenderViewport();

   //============================================================================
   // Set matrices
   XMMATRIX worldToView;// = matrixTracker.getMatrix(cMTWorldToView);
   worldToView = XMMatrixIdentity();
   matrixTracker.setMatrix(cMTWorldToView, worldToView);

   //matrixTracker.setMatrix(cMTViewToProj, viewToProj);

   //============================================================================
   // Set viewport
   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = mWidth;
   viewport.Height = mHeight;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;
   
   matrixTracker.setViewport(viewport);
        
   renderViewport.setSurf(0, mpRenderTarget);
   renderViewport.setDepthStencilSurf(mpDepthStencil);
   renderViewport.setViewport(viewport);

   //============================================================================
   // Set matrix tracker and viewport so that they update dependencies
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);

   // Set volume culler
   //BVolumeCuller& volumeCuller = gRenderDraw.getWorkerActiveVolumeCuller();
   //volumeCuller.setBasePlanes(cullingFrustumNoClip);
}

//============================================================================
// BNearLayerManager::composite
//============================================================================
void BNearLayerManager::composite()
{
   ASSERT_THREAD(cThreadIndexRender);

   if (!mEnabled)
      return;

   if (!mpTexture)
      return;

   // Render quad with reflection texture
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);      
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);      
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);      
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);      

   BD3D::mpDev->SetTexture(0, mpTexture);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
      
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_SEPARATEZFILTERENABLE, TRUE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTERZ, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTERZ, D3DTEXF_POINT);
   
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 1);


   int rectWidth = mWidth / 2;
   int rectHeight = mHeight / 2;
   int xOfs = 50;
   int yOfs = gRender.getHeight() - 20 - rectHeight;
   if (mFullScreenDraw)
   {
      rectWidth = gRender.getWidth();
      rectHeight = gRender.getHeight();
      xOfs = 0;
      yOfs = 0;
   }

   BPrimDraw2D::drawSolidRect2D(xOfs, yOfs, xOfs+rectWidth, yOfs+rectHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF, cPosTex1VS, cTex1PS);
}
