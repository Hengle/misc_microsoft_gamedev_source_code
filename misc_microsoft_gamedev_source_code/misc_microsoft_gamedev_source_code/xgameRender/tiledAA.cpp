//============================================================================
//
//  File: tiledAA.cpp
//
//  Copyright (c) 2006 Ensemble Studios
//
// rg [6/14/06] - Initial implementation
//============================================================================
#include "xgameRender.h"
#include "tiledAA.h"
#include "primDraw2D.h"

#include "render.h"
#include "renderDraw.h"
#include "vertexTypes.h"

#include "deviceStateDumper.h"

//============================================================================
// Globals
//============================================================================
BTiledAAManager gTiledAAManager;

namespace
{
   //============================================================================
   // setRect
   //============================================================================
   void setRect(RECT& rect, int left, int top, int right, int bottom)
   {
      rect.left = left;
      rect.top = top;
      rect.right = right;
      rect.bottom = bottom;
   }
}

//============================================================================
// BTiledAAManager::BTiledAAManager
//============================================================================
BTiledAAManager::BTiledAAManager()
{
   clear();
}

//============================================================================
// BTiledAAManager::clear
//============================================================================
void BTiledAAManager::clear(void)
{
   mWidth = 0;
   mHeight = 0;
   mTileMaxWidth = 0;
   mTileMaxHeight = 0;
   mNumTiles = 0;
   mViewportLeft = 0;
   mViewportTop = 0;
   mpColorSurf = NULL;
   mpDepthSurf = NULL;
   mpColorTexture = NULL;
   mpDepthTexture = NULL;
   mMultisampleType = D3DMULTISAMPLE_NONE;
   mEDRAMColorFormat = D3DFMT_A8R8G8B8;
   mTexColorFormat = D3DFMT_A8R8G8B8;
   mDepthFormat = D3DFMT_D24S8;
      
   Utils::ClearObj(mTiles);
   
   mIsTiling = false;
   mCurTileIndex = -1;
   mTotalEDRAMUsed = 0;
   
   mTilingEnabled = true;
   mNoTilingWidth = 0;
   mNoTilingHeight = 0;
   mNoTilingMultisample = D3DMULTISAMPLE_NONE;
   
   Utils::ClearObj(mAliasedColorTexture);
}   

//============================================================================
// BTiledAAManager::~BTiledAAManager
//============================================================================
BTiledAAManager::~BTiledAAManager()
{
}

//============================================================================
// BTiledAAManager::init
//============================================================================
void BTiledAAManager::init(uint width, uint height, uint numTiles, D3DFORMAT edramColorFormat, D3DFORMAT texColorFormat, D3DFORMAT depthFormat)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   deinit();
      
   mWidth = width;
   mHeight = height;
     
   mEDRAMColorFormat = edramColorFormat;
   mTexColorFormat = texColorFormat;
   mDepthFormat = depthFormat;
   
   createD3DTextures();
                  
   mTilingEnabled = true;
   mNoTilingWidth = 0;
   mNoTilingHeight = 0;
   mNoTilingMultisample = D3DMULTISAMPLE_NONE;
   Utils::ClearObj(mNoTilingRect);
   mNoTilingSurfaces.destroyDeviceObjects();
      
   setNumTiles(numTiles);
     
   commandListenerInit();
}

//============================================================================
// BTiledAAManager::createD3DTextures
//============================================================================
void BTiledAAManager::createD3DTextures(void)
{
   if (!mpColorTexture)
   {
      HRESULT status = gRenderDraw.createTexture(mWidth, mHeight, 1, 0, mTexColorFormat, 0, &mpColorTexture, NULL);
      if (!SUCCEEDED(status))
      {
         BFATAL_FAIL("Out of memory");
      }
   }

   if (!mpDepthTexture)
   {
      BDEBUG_ASSERT(!mpDepthTexture);
      HRESULT status = gRenderDraw.createTexture(mWidth, mHeight, 1, 0, mDepthFormat, 0, &mpDepthTexture, NULL);
      if (!SUCCEEDED(status))
      {
         BFATAL_FAIL("Out of memory");
      }
   }
}

//============================================================================
// BTiledAAManager::releaseD3DTextures
//============================================================================
void BTiledAAManager::releaseD3DTextures(void)
{
   if (mpColorTexture)
   {
      gRenderDraw.releaseD3DResource(mpColorTexture);
      mpColorTexture = NULL;
   }

   if (mpDepthTexture)
   {
      gRenderDraw.releaseD3DResource(mpDepthTexture);
      mpDepthTexture = NULL;
   }
}

//============================================================================
// BTiledAAManager::releaseD3DSurfaces
//============================================================================
void BTiledAAManager::releaseD3DSurfaces(void)
{
   if (mpColorSurf)
   {
      gRenderDraw.releaseD3DResource(mpColorSurf);
      mpColorSurf = NULL;
   }

   if (mpDepthSurf)
   {
      gRenderDraw.releaseD3DResource(mpDepthSurf);
      mpDepthSurf = NULL;
   }
}

//============================================================================
// BTiledAAManager::deinit
//============================================================================
void BTiledAAManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   if (!isInitialized())
      return;

   gRenderThread.blockUntilGPUIdle();         

   commandListenerDeinit();

   releaseD3DSurfaces();
   releaseD3DTextures();
   
   mNoTilingSurfaces.destroyDeviceObjects();

   clear();
}

//============================================================================
// BTiledAAManager::setNumTiles
//============================================================================
void BTiledAAManager::setNumTiles(uint numTiles)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BDEBUG_ASSERT(gRenderThread.getInitialized());

   if (numTiles == mNumTiles)
      return;
   
   gRenderThread.blockUntilGPUIdle();

   releaseD3DSurfaces();
   
   numTiles = Math::Clamp<uint>(numTiles, 1, 4);
   Utils::ClearObj(mTiles);
   
   const bool verticalTiles = false;
      
   // Important: The smallest tile should be the last!
   
   switch (numTiles)
   {
      case 1:
      {
         mNumTiles = 1;
         mMultisampleType = D3DMULTISAMPLE_NONE;
         mTileMaxWidth = mWidth;
         mTileMaxHeight = mHeight;
         
         mViewportLeft = 0;
         mViewportTop = 0;
         
         setRect(mTiles[0], 0, 0, mWidth, mHeight);
         
         break;
      }
      case 2:
      {
         mNumTiles = 2;
         mMultisampleType = D3DMULTISAMPLE_2_SAMPLES;

         mViewportLeft = 0;
         mViewportTop = 0;

         if (verticalTiles)
         {
            const uint halfWidth = mWidth / 2;
                        
            mTileMaxWidth = (Math::Max(halfWidth, mWidth - halfWidth) + 31) & ~31;
            mTileMaxHeight = mHeight;
                                                                                                   
            setRect(mTiles[0], 0, 0, halfWidth, mHeight);
            setRect(mTiles[1], halfWidth, 0, mWidth, mHeight);
         }
         else
         {
            const uint halfHeight = (mHeight / 2) & ~31;
            
            mTileMaxWidth = mWidth;
            mTileMaxHeight = Math::Max(halfHeight, mHeight - halfHeight);
            
            setRect(mTiles[0], 0, halfHeight, mWidth, mHeight);
            setRect(mTiles[1], 0, 0, mWidth, halfHeight);
         }            
                  
         break;
      }
      case 3:
      case 4:
      {
         const DWORD edramBPP = (XGBitsPerPixelFromFormat(mDepthFormat) + XGBitsPerPixelFromFormat(mEDRAMColorFormat)) / 8;
         mNumTiles = 3;
   
         mMultisampleType = (edramBPP == 8) ? D3DMULTISAMPLE_4_SAMPLES : D3DMULTISAMPLE_2_SAMPLES; 
         
         const uint oneThirdHeight = ((mHeight / 3) + 31) & ~31;

         mTileMaxWidth = mWidth;
         mTileMaxHeight = Math::Max(oneThirdHeight, mHeight - oneThirdHeight * 2);

         setRect(mTiles[2], 0, oneThirdHeight * 2, mWidth, mHeight);
         setRect(mTiles[1], 0, oneThirdHeight, mWidth, oneThirdHeight * 2);
         setRect(mTiles[0], 0, 0, mWidth, oneThirdHeight);
      }
   }

   D3DSURFACE_PARAMETERS colorParams;
   Utils::ClearObj(colorParams);
         
   HRESULT status = gRenderDraw.createRenderTarget(mTileMaxWidth, mTileMaxHeight, mEDRAMColorFormat, mMultisampleType, 0, 0, &mpColorSurf, &colorParams);
   BVERIFY(SUCCEEDED(status));
   
   D3DSURFACE_PARAMETERS depthParams;
   Utils::ClearObj(depthParams);
   depthParams.Base = XGSurfaceSize(mTileMaxWidth, mTileMaxHeight, mEDRAMColorFormat, mMultisampleType);
   status = gRenderDraw.createDepthStencilSurface(mTileMaxWidth, mTileMaxHeight, mDepthFormat, mMultisampleType, 0, 0, &mpDepthSurf, &depthParams);
   BVERIFY(SUCCEEDED(status));
   
   mTotalEDRAMUsed = depthParams.Base + XGSurfaceSize(mTileMaxWidth, mTileMaxHeight, mDepthFormat, mMultisampleType);
}

//============================================================================
// BTiledAAManager::getNumTiles
//============================================================================
uint BTiledAAManager::getNumTiles(void) const 
{ 
   return mTilingEnabled ? mNumTiles : 1; 
}   

//============================================================================
// BTiledAAManager::getWidth
//============================================================================
uint BTiledAAManager::getWidth(void) const 
{ 
   return mTilingEnabled ? mWidth : mNoTilingWidth; 
}

//============================================================================
// BTiledAAManager::getHeight
//============================================================================
uint BTiledAAManager::getHeight(void) const 
{ 
   return mTilingEnabled ? mHeight : mNoTilingHeight; 
}

//============================================================================
// BTiledAAManager::enableTiling
//============================================================================
void BTiledAAManager::enableTiling(bool enabled, uint noTilingWidth, uint noTilingHeight, D3DMULTISAMPLE_TYPE multisampleType)
{
   mTilingEnabled = enabled;
   
   if (!mTilingEnabled)
   {
      mNoTilingWidth = noTilingWidth;
      mNoTilingHeight = noTilingHeight;
      mNoTilingMultisample = multisampleType;
      setRect(mNoTilingRect, 0, 0, noTilingWidth, noTilingHeight);
            
      if ((!mNoTilingSurfaces.getColorSurf()) || 
          (mNoTilingSurfaces.getFormat() != mEDRAMColorFormat) || 
          ((uint)mNoTilingSurfaces.getWidth() != noTilingWidth) || 
          ((uint)mNoTilingSurfaces.getHeight() != noTilingHeight) ||
          (mNoTilingSurfaces.getMultisample() != mNoTilingMultisample))
      {
         mNoTilingSurfaces.set(noTilingWidth, noTilingHeight, mEDRAMColorFormat, D3DFMT_D24S8, multisampleType);
         mNoTilingSurfaces.createDeviceObjects();
         
         BDEBUG_ASSERT(mNoTilingSurfaces.getTotalEDRAMUsed() <= mTotalEDRAMUsed);
      }
   }
}

//============================================================================
// BTiledAAManager::getTileRect
//============================================================================
const RECT& BTiledAAManager::getTileRect(uint tileIndex) const 
{ 
   if (!mTilingEnabled)
   {
      BDEBUG_ASSERT(!tileIndex); 
      return mNoTilingRect;
   }
   else
   {
      BDEBUG_ASSERT(tileIndex < mNumTiles); 
      return mTiles[tileIndex]; 
   }
}      

//============================================================================
// BTiledAAManager::getTileProjMatrix
//============================================================================
XMMATRIX BTiledAAManager::getTileProjMatrix(uint tileIndex) const 
{ 
   if (!mTilingEnabled)
   {
      BDEBUG_ASSERT(!tileIndex);
      
      return gRenderDraw.getWorkerSceneMatrixTracker().getMatrix(cMTViewToProj);
   }
   else
   {
      BDEBUG_ASSERT(tileIndex < mNumTiles); 
         
      return mTileProjMatrices[tileIndex]; 
   }
}

//============================================================================
// BTiledAAManager::getTileVolumeCuller
//============================================================================
const BVolumeCuller& BTiledAAManager::getTileVolumeCuller(uint tileIndex) const 
{ 
   if (!mTilingEnabled)
   {
      return gRenderDraw.getWorkerSceneVolumeCuller();
   }
   else
   {
      BDEBUG_ASSERT(tileIndex < mNumTiles); 
      return mTileVolumeCullers[tileIndex]; 
   }
}

//============================================================================
// BTiledAAManager::beginFrame
//============================================================================
void BTiledAAManager::beginFrame(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(isInitialized());
   
   if (mTilingEnabled)
   {
      for (uint tileIndex = 0; tileIndex < mNumTiles; tileIndex++)
      {
         float l = (mTiles[tileIndex].left   / (float)mWidth) * 2.0f - 1.0f;
         float r = (mTiles[tileIndex].right  / (float)mWidth) * 2.0f - 1.0f;
         float b = (mTiles[tileIndex].bottom / (float)mHeight) * -2.0f + 1.0f;
         float t = (mTiles[tileIndex].top    / (float)mHeight) * -2.0f + 1.0f;
         
         XMMATRIX tileMatrix = XMMatrixOrthographicOffCenterLH(
            l,
            r,
            b,
            t,
            0.0f,
            1.0f);

         XMMATRIX newProjMatrix = XMMatrixMultiply(gRenderDraw.getWorkerSceneMatrixTracker().getMatrix(cMTViewToProj), tileMatrix);
         
         mTileProjMatrices[tileIndex] = newProjMatrix;
         mTileVolumeCullers[tileIndex].setBasePlanes(XMMatrixMultiply(gRenderDraw.getWorkerSceneMatrixTracker().getMatrix(cMTWorldToView), newProjMatrix));
      }      
   }      
}

//============================================================================
// BTiledAAManager::beginTiling
//============================================================================
void BTiledAAManager::beginTiling(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(isInitialized() && !mIsTiling);
         
   mIsTiling = true;
   mCurTileIndex = -1;
   
   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, TRUE);
}

//============================================================================
// BTiledAAManager::getTileViewport
//============================================================================
void BTiledAAManager::getTileViewport(uint tileIndex, D3DVIEWPORT9& viewport) const
{
   if (!mTilingEnabled)
   {
      BDEBUG_ASSERT(!tileIndex);

      viewport.X = 0;
      viewport.Y = 0;
      viewport.Width = mNoTilingWidth;
      viewport.Height = mNoTilingHeight;
   }
   else
   {
      viewport.X = mViewportLeft;
      viewport.Y = mViewportTop;
      viewport.Width = mTiles[tileIndex].right - mTiles[tileIndex].left;
      viewport.Height = mTiles[tileIndex].bottom - mTiles[tileIndex].top;
   }  
   
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f; 
}

//============================================================================
// BTiledAAManager::beginTile
//============================================================================
void BTiledAAManager::beginTile(uint tileIndex, const D3DVECTOR4* pClearColor, DWORD stencil)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(isInitialized() && mIsTiling && tileIndex < mNumTiles);
         
   mCurTileIndex = tileIndex;

   BMatrixTracker& matrixTracker = gRenderDraw.getWorkerActiveMatrixTracker();         
   BRenderViewport& renderViewport  = gRenderDraw.getWorkerActiveRenderViewport();   
   
   D3DVECTOR4 zero;
   if (!pClearColor)
   {
      Utils::ClearObj(zero);
      pClearColor = &zero;
   }
   
   D3DVIEWPORT9 viewport;
   getTileViewport(tileIndex, viewport);
   matrixTracker.setViewport(viewport);
   
   if (!mTilingEnabled)
   {
      BDEBUG_ASSERT(!tileIndex);
      
      matrixTracker.setMatrix(cMTViewToProj, gRenderDraw.getWorkerSceneMatrixTracker().getMatrix(cMTViewToProj));
                        
      renderViewport.setSurf(0, mNoTilingSurfaces.getColorSurf());
      renderViewport.setDepthStencilSurf(mNoTilingSurfaces.getDepthSurf());
      renderViewport.setViewport(viewport);
     
      gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
      gRenderDraw.setWorkerActiveRenderViewport(renderViewport);     
      gRenderDraw.setWorkerActiveVolumeCuller(gRenderDraw.getWorkerSceneVolumeCuller());
      
      BD3D::mpDev->ClearF(D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER, NULL, pClearColor, 1.0f, stencil);
   }
   else
   {
      matrixTracker.setMatrix(cMTViewToProj, mTileProjMatrices[tileIndex]);
                  
      renderViewport.setSurf(0, mpColorSurf);
      renderViewport.setDepthStencilSurf(mpDepthSurf);
      renderViewport.setViewport(viewport);
      
      gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
      gRenderDraw.setWorkerActiveRenderViewport(renderViewport);      
      gRenderDraw.setWorkerActiveVolumeCuller(mTileVolumeCullers[tileIndex]);
      
      if (tileIndex == 0)
         BD3D::mpDev->ClearF(D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER, NULL, pClearColor, 1.0f, stencil);
   }      
}

//============================================================================
// BTiledAAManager::endTileDepth
//============================================================================
void BTiledAAManager::endTileDepth(uint tileIndex, const D3DRESOLVE_PARAMETERS* pDepthResolveParams, bool resolveNoTilingDepthSurf, DWORD clearStencil)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   BDEBUG_ASSERT(isInitialized() && mIsTiling && tileIndex < mNumTiles);

#ifdef BUILD_DEBUG   
   DWORD val;
   BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &val);
   BDEBUG_ASSERT(val == TRUE);
#endif   

   if (!mTilingEnabled)
   {
      BDEBUG_ASSERT(!tileIndex);
      
      D3DPOINT dstPoint;
      dstPoint.x = 0;
      dstPoint.y = 0;
      
      D3DRECT rect;
      rect.x1 = 0;
      rect.y1 = 0;
      rect.x2 = mNoTilingWidth;
      rect.y2 = mNoTilingHeight;
      
      if (resolveNoTilingDepthSurf)
      {
         BDEBUG_ASSERT(mNoTilingWidth <= mWidth && mNoTilingHeight <= mHeight);
                          
         BD3D::mpDev->Resolve(
            D3DRESOLVE_DEPTHSTENCIL | D3DRESOLVE_FRAGMENT0,
            &rect,
            mpDepthTexture,
            &dstPoint,
            0,
            0,
            NULL,
            1.0f,
            clearStencil,
            pDepthResolveParams);  
      }
   }
   else
   {
      D3DRECT srcRect;
      srcRect.x1 = mViewportLeft;
      srcRect.y1 = mViewportTop;
      srcRect.x2 = mViewportLeft + (mTiles[tileIndex].right - mTiles[tileIndex].left);
      srcRect.y2 = mViewportTop + (mTiles[tileIndex].bottom - mTiles[tileIndex].top);

      D3DPOINT dstPoint;
      
      dstPoint.x = mTiles[tileIndex].left;
      dstPoint.y = mTiles[tileIndex].top;

      BD3D::mpDev->Resolve(
         D3DRESOLVE_DEPTHSTENCIL | D3DRESOLVE_FRAGMENT0,
         &srcRect,
         mpDepthTexture,
         &dstPoint,
         0,
         0,
         NULL,
         1.0f,
         clearStencil,
         pDepthResolveParams);  
   }         
}

//============================================================================
// BTiledAAManager::endTileScene
//============================================================================
void BTiledAAManager::endTileScene(uint tileIndex, const D3DRESOLVE_PARAMETERS* pColorResolveParams, const D3DVECTOR4* pClearColor, bool resolveNoTilingColorSurf, DWORD clearStencil)
{
   ASSERT_THREAD(cThreadIndexRender);
         
   BDEBUG_ASSERT(isInitialized() && mIsTiling && tileIndex < mNumTiles);

#ifdef BUILD_DEBUG   
   DWORD val;
   BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &val);
   BDEBUG_ASSERT(val == TRUE);
#endif   

   if (!mTilingEnabled)
   {
      BDEBUG_ASSERT(!tileIndex);
      
      D3DPOINT dstPoint;
      dstPoint.x = 0;
      dstPoint.y = 0;
      
      D3DRECT rect;
      rect.x1 = 0;
      rect.y1 = 0;
      rect.x2 = mNoTilingWidth;
      rect.y2 = mNoTilingHeight;
      
      if (resolveNoTilingColorSurf)
      {
         BDEBUG_ASSERT(mNoTilingWidth <= mWidth && mNoTilingHeight <= mHeight);
                  
         BD3D::mpDev->Resolve(
            D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS,
            &rect,
            mpColorTexture,
            &dstPoint,
            0,
            0,
            NULL,
            1.0f,
            clearStencil,
            pColorResolveParams);           
      }
   }
   else
   {
      D3DRECT srcRect;
      srcRect.x1 = mViewportLeft;
      srcRect.y1 = mViewportTop;
      srcRect.x2 = mViewportLeft + (mTiles[tileIndex].right - mTiles[tileIndex].left);
      srcRect.y2 = mViewportTop + (mTiles[tileIndex].bottom - mTiles[tileIndex].top);

      D3DPOINT dstPoint;
      dstPoint.x = mTiles[tileIndex].left;
      dstPoint.y = mTiles[tileIndex].top;

      D3DVECTOR4 zero;
      if (!pClearColor)
      {
         Utils::ClearObj(zero);
         pClearColor = &zero;
      }
       
      // This assumes tile 0 is equal/larger than tile 1!!
      BDEBUG_ASSERT( (mTiles[0].bottom - mTiles[0].top) >= (mTiles[1].bottom - mTiles[1].top) );
      BDEBUG_ASSERT( (mTiles[0].right - mTiles[0].left) >= (mTiles[1].right - mTiles[1].left) );
      BD3D::mpDev->Resolve(
         D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS | ((tileIndex < (mNumTiles - 1)) ? (D3DRESOLVE_CLEARRENDERTARGET | D3DRESOLVE_CLEARDEPTHSTENCIL) : 0),
         &srcRect,
         mpColorTexture,
         &dstPoint,
         0,
         0,
         pClearColor,
         1.0f,
         clearStencil,
         pColorResolveParams);  
   }         
}

//============================================================================
// BTiledAAManager::endTiling
//============================================================================
void BTiledAAManager::endTiling(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(isInitialized() && mIsTiling);
   
   mIsTiling = false;
   mCurTileIndex = -1;
        
   gRenderDraw.setWorkerActiveMatrixTracker(gRenderDraw.getWorkerSceneMatrixTracker());
   gRenderDraw.setWorkerActiveRenderViewport(gRenderDraw.getWorkerSceneRenderViewport());
   gRenderDraw.setWorkerActiveVolumeCuller(gRenderDraw.getWorkerSceneVolumeCuller());
   
   BD3D::mpDev->SetRenderState(D3DRS_HALFPIXELOFFSET, FALSE);
}

//============================================================================
// BTiledAAManager::blitColorTexture
//============================================================================
void BTiledAAManager::blitColorTexture(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   BDEBUG_ASSERT(isInitialized() && !mIsTiling);

   SCOPEDSAMPLE(BTiledAAManagerBlitColorTexture);
      
   BD3D::mpDev->SetTexture(0, mpColorTexture);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
   
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);
   
   BPrimDraw2D::drawSolidRect2D(0, 0, mWidth, mHeight, 0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFF, 0);
   
   BD3D::mpDev->SetTexture(0, NULL);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
}

//==============================================================================
// BTiledAAManager::resolveBackbuffer
//==============================================================================
void BTiledAAManager::resolveBackbuffer(IDirect3DTexture9* pTextureToOverwrite)
{  
   BDEBUG_ASSERT(pTextureToOverwrite);

   BD3D::mpDev->SetRenderTarget(0, BD3D::mpDevBackBuffer);

   XGTEXTURE_DESC desc;
   XGGetTextureDesc(pTextureToOverwrite, 0, &desc);

   UINT baseSize, mipSize;
   uint totalSize = XGSetTextureHeader(
      BD3D::mD3DPP.BackBufferWidth,
      BD3D::mD3DPP.BackBufferHeight,
      1, 0, BD3D::mD3DPP.BackBufferFormat, 
      0, 0, 0, 0, 
      &mAliasedColorTexture, 
      &baseSize, 
      &mipSize);
   totalSize;
   BDEBUG_ASSERT(desc.SlicePitch >= totalSize);
   XGOffsetResourceAddress(&mAliasedColorTexture, (VOID*)(pTextureToOverwrite->Format.BaseAddress << 12));

   BD3D::mpDev->Resolve(
      D3DRESOLVE_RENDERTARGET0 | D3DRESOLVE_ALLFRAGMENTS,
      NULL,
      &mAliasedColorTexture,
      NULL,
      0,
      0,
      NULL,
      1.0f,
      0,
      NULL);
}                   

//============================================================================
// BTiledAAManager::initDeviceData
//============================================================================
void BTiledAAManager::initDeviceData(void)
{
}

//============================================================================
// BTiledAAManager::frameBegin
//============================================================================
void BTiledAAManager::frameBegin(void)
{
}

//============================================================================
// BTiledAAManager::processCommand
//============================================================================
void BTiledAAManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
}

//============================================================================
// BTiledAAManager::frameEnd
//============================================================================
void BTiledAAManager::frameEnd(void)
{
}

//============================================================================
// BTiledAAManager::deinitDeviceData
//============================================================================
void BTiledAAManager::deinitDeviceData(void)
{
   
}

//============================================================================
// BTiledAAManager::beginLevelLoad
//============================================================================
void BTiledAAManager::beginLevelLoad(void)
{
   releaseD3DTextures();
}

//============================================================================
// BTiledAAManager::endLevelLoad
//============================================================================
void BTiledAAManager::endLevelLoad(void)
{
   createD3DTextures();
}

