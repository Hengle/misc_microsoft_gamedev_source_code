
// xgamereander
#include "xgameRender.h"
#include "occlusion.h"
#include "configsgamerender.h"

// xrender
#include "primDraw2D.h"
#include "render.h"
#include "renderThread.h"
#include "gpuHeap.h"
#include "renderDraw.h"

// xcore
#include "math\vmxIntersection.h"

// xgame
#include "inputsystem.h"

// terrain
#include "terrainheightfield.h"


BOcclusionManager gOcclusionManager;
const int cAreaCutoffAmt = 1000;

//============================================================================
// BOcclusionManager::BOcclusionManager
//============================================================================
BOcclusionManager::BOcclusionManager():
mBufferWidth(128),//256),
mBufferHeight(80),//160),//256),
mNumXMVectorsX(64),
mNumXMVectorsY(64),
mRenderFormat(D3DFMT_R32F),
mDepthFormat(D3DFMT_D24S8),
mTextureFormat(D3DFMT_R32F),
mpRenderTarget(NULL),
mpDepthStencil(NULL),
mpOcclusionBufferTexture(NULL),
mpOcclusionBuffer(NULL),
mOcclusionTestEnabled(false),
#ifndef BUILD_FINAL
mDebugDraw(false),
#endif
mpRenderTargetPointer(0)
{

}

//============================================================================
// BOcclusionManager::~BOcclusionManager
//============================================================================
BOcclusionManager::~BOcclusionManager()
{

}
//============================================================================
// BOcclusionManager::allocateOcclusionBuffer
//============================================================================
void BOcclusionManager::allocateOcclusionBuffer()
{
   if(mpOcclusionBuffer)
      return;

   //Buffer width & height MUST be multiple of 4 for speed inprovements
   BDEBUG_ASSERT(mBufferWidth %4 ==0 && mBufferHeight %4 ==0);

   mNumXMVectorsX = mBufferWidth>>2;
   mNumXMVectorsY = mBufferHeight;
   mpOcclusionBuffer =new  XMVECTOR[mNumXMVectorsX * mNumXMVectorsY ];

   gRenderDraw.createTexture(mBufferWidth, mBufferHeight, 1, 0, mTextureFormat, 0, &mpOcclusionBufferTexture, NULL);
   BVERIFY(mpOcclusionBufferTexture);
}
//============================================================================
// BOcclusionManager::destroy
//============================================================================
void BOcclusionManager::destroy()
{
   // Block for safety. 
   gRenderThread.blockUntilGPUIdle();

   releaseOcclusionBuffer();
}

//============================================================================
// BOcclusionManager::releaseOcclusionBuffer
//============================================================================
void BOcclusionManager::releaseOcclusionBuffer()
{
  if(!mpOcclusionBuffer)
     return;

  delete [] mpOcclusionBuffer;
  mpOcclusionBuffer = NULL;

  if (mpOcclusionBufferTexture)
  {
     gRenderDraw.releaseD3DResource(mpOcclusionBufferTexture);
     mpOcclusionBufferTexture = NULL;
  }
}
//============================================================================
// BOcclusionManager::preGenerateOcclusionBuffer
//============================================================================
void BOcclusionManager::preGenerateOcclusionBuffer()
{ 
  
   // Alloc temp render target / depth buffer
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);
   surfaceParams.Base = 0;

   //create RT, DEPTH and resolve targets
   HRESULT hres = gGPUFrameHeap.createRenderTarget(mBufferWidth, mBufferHeight, mRenderFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &mpRenderTarget, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));

   surfaceParams.Base = XGSurfaceSize(mBufferWidth, mBufferHeight, mRenderFormat, mMultisampleType);
   hres = gGPUFrameHeap.createRenderTarget(mBufferWidth, mBufferHeight, mDepthFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &mpDepthStencil, &surfaceParams);
   BVERIFY(SUCCEEDED(hres));

   


   //keep this around!
   BMatrixTracker&  matrixTracker = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport = gRenderDraw.getWorkerActiveRenderViewport();

   mCameraPos = matrixTracker.getWorldCamPos();

   // Set viewport
   D3DVIEWPORT9 viewport;
   viewport.X = 0;
   viewport.Y = 0;
   viewport.Width = mBufferWidth;
   viewport.Height = mBufferHeight;
   viewport.MinZ = 0.0f;
   viewport.MaxZ = 1.0f;

   matrixTracker.setViewport(viewport);

   renderViewport.setSurf(0, mpRenderTarget);
   renderViewport.setDepthStencilSurf(mpDepthStencil);
   renderViewport.setViewport(viewport);

   // Set matrix tracker and viewport so that they update dependencies
   gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   gRenderDraw.setWorkerActiveRenderViewport(renderViewport);

   mWorldToScreen = matrixTracker.getMatrix(cMTWorldToScreen);

   // Clear color and depth
   gRenderDraw.clear(D3DCLEAR_TARGET0|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0xFFFFFFFF);
}
//============================================================================
// BOcclusionManager::postGenerateOcclusionBuffer
//============================================================================
void BOcclusionManager::postGenerateOcclusionBuffer()
{
   gRenderDraw.resetWorkerActiveMatricesAndViewport();

   // Release render target / depth buffer
   gGPUFrameHeap.releaseD3DResource(mpRenderTarget);
   mpRenderTarget = NULL;

   gGPUFrameHeap.releaseD3DResource(mpDepthStencil);
   mpDepthStencil = NULL;


}

//============================================================================
// BOcclusionManager::generateOcclusionBuffer
//============================================================================
void BOcclusionManager::generateOcclusionBuffer()
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLE(BOcclusionManager_generateOcclusionBuffer)
  
   //CLM we do this at the beginning of this process to ensure that we're not
   //clearing out the buffer while someone is trying to use it!
   if(mDoClearOcclusionBuffer)
      clearOcclusionBufferInternal();

   if(!mOcclusionTestEnabled)
      return;

   preGenerateOcclusionBuffer();

   renderTerrainToBuffer();
   renderOccluderObjects();

   resolvePackDepthBuffer();

  postGenerateOcclusionBuffer();
}

//============================================================================
// BOcclusionManager::resolvePackDepthBuffer
//============================================================================
void BOcclusionManager::resolvePackDepthBuffer()
{
   ASSERT_RENDER_THREAD

      if(!mpOcclusionBuffer)
         allocateOcclusionBuffer();

   // Resolve render target to texure
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpOcclusionBufferTexture, NULL, 0, 0, NULL, 1.0f, 0, NULL);

   //place them in VMX optomized 2d representation
   if(!mpRenderTargetPointer)
   {
      D3DLOCKED_RECT rect;
      mpOcclusionBufferTexture->LockRect(0,&rect,0,0);
      mpRenderTargetPointer= (float*)rect.pBits;
      mpOcclusionBufferTexture->UnlockRect(0);
   }
   
   int idx=0;
   for(int y=0; y < mNumXMVectorsY; y++)
   {
      for(int x=0; x < mNumXMVectorsX; x++)
      {
         const uint ofs = XGAddress2DTiledOffset(x*4, y, mBufferWidth, sizeof(float));
         
         mpOcclusionBuffer[idx++] = XMVectorSet(   mpRenderTargetPointer[ofs+0],
                                                   mpRenderTargetPointer[ofs+1],
                                                   mpRenderTargetPointer[ofs+2],
                                                   mpRenderTargetPointer[ofs+3]);
      }
   }
}

//============================================================================
// BOcclusionManager::renderTerrainToBuffer
//============================================================================
void BOcclusionManager::renderTerrainToBuffer()
{
   ASSERT_RENDER_THREAD

   //this should actually render our decal rep
   gTerrainHeightField.renderHeightfieldForOcclusion();
}

//============================================================================
// BOcclusionManager::renderOccluderObjects
//============================================================================
void BOcclusionManager::renderOccluderObjects()
{
   ASSERT_RENDER_THREAD

   //the sim should mark objects as :
   // occluders : never hidden, but hides others
   // occludees : can be hidden, but never hides others
   // dynamic : can be occluder or occludee
   //we render occluders and dynamics into the depth buffer at this point.
}
//============================================================================
// BOcclusionManager::clearOcclusionBufferInternal
//============================================================================
void BOcclusionManager::clearOcclusionBufferInternal()
{
   mDoClearOcclusionBuffer = false;
   for(int y=0; y < mNumXMVectorsY*mNumXMVectorsX; y++)
         mpOcclusionBuffer[y] = XMVectorSplatOne();
}
//============================================================================
// BOcclusionManager::calcScreenSpaceBBox
//============================================================================
void BOcclusionManager::calcScreenSpaceBBox(BVector min, BVector max, XMMATRIX transform, XMVECTOR &frontDist, XMVECTOR &minBounds, XMVECTOR &maxBounds)
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(BOcclusionManager_calcScreenSpaceBBox)

   XMVECTOR* points = new XMVECTOR[8];
   points[0] = XMVector4Transform(XMVector4Transform(XMVectorSet(min.x,min.y,min.z,1),transform),mWorldToScreen);
   points[0] = XMVectorMultiply(points[0],XMVectorReciprocal(XMVectorSplatW(points[0])));

   points[1] = XMVector4Transform(XMVector4Transform(XMVectorSet(max.x,min.y,min.z,1),transform),mWorldToScreen);
   points[1] = XMVectorMultiply(points[1],XMVectorReciprocal(XMVectorSplatW(points[1])));

   points[2] = XMVector4Transform(XMVector4Transform(XMVectorSet(min.x,max.y,min.z,1),transform),mWorldToScreen);
   points[2] = XMVectorMultiply(points[2],XMVectorReciprocal(XMVectorSplatW(points[2])));

   points[3] = XMVector4Transform(XMVector4Transform(XMVectorSet(max.x,max.y,min.z,1),transform),mWorldToScreen);
   points[3] = XMVectorMultiply(points[3],XMVectorReciprocal(XMVectorSplatW(points[3])));

   points[4] = XMVector4Transform(XMVector4Transform(XMVectorSet(min.x,min.y,max.z,1),transform),mWorldToScreen);
   points[4] = XMVectorMultiply(points[4],XMVectorReciprocal(XMVectorSplatW(points[4])));

   points[5] = XMVector4Transform(XMVector4Transform(XMVectorSet(max.x,min.y,max.z,1),transform),mWorldToScreen);
   points[5] = XMVectorMultiply(points[5],XMVectorReciprocal(XMVectorSplatW(points[5])));

   points[6] = XMVector4Transform(XMVector4Transform(XMVectorSet(min.x,max.y,max.z,1),transform),mWorldToScreen);
   points[6] = XMVectorMultiply(points[6],XMVectorReciprocal(XMVectorSplatW(points[6])));

   points[7] = XMVector4Transform(XMVector4Transform(XMVectorSet(max.x,max.y,max.z,1),transform),mWorldToScreen);
   points[7] = XMVectorMultiply(points[7],XMVectorReciprocal(XMVectorSplatW(points[7])));



   XMVECTOR closestPt = points[0];
   XMVECTOR closestLn = XMVectorSplatZ(points[0]);
   minBounds = points[0];
   maxBounds = points[0];
   for(int i=1;i<8;i++)
   {
      //calc closest point to camera
      XMVECTOR ln = XMVectorSplatZ(points[i]);
      if(XMVector4Less(ln,closestLn))
      {  
         closestLn = XMVectorSplatZ(ln);
         closestPt = points[i];
      }

      //calc min values Needs to be VMXd..
      if(XMVector4Less(XMVectorSplatX(points[i]),XMVectorSplatX(minBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 4, 1, 2, 3 );
         minBounds = XMVectorPermute(  minBounds, points[i],control );
      }
      else if(XMVector4Greater(XMVectorSplatX(points[i]),XMVectorSplatX(maxBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 4, 1, 2, 3 );
         maxBounds = XMVectorPermute(  maxBounds, points[i],control );
      }

      if(XMVector4Less(XMVectorSplatY(points[i]),XMVectorSplatY(minBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 0, 5, 2, 3 );
         minBounds = XMVectorPermute(  minBounds, points[i],control );
      }
      else if(XMVector4Greater(XMVectorSplatY(points[i]),XMVectorSplatY(maxBounds)))
      {
         XMVECTOR control = XMVectorPermuteControl( 0, 5, 2, 3 );
         maxBounds = XMVectorPermute(  maxBounds, points[i],control );
      }
   }
   delete[] points;

   frontDist= XMVectorSplatZ(closestPt);
}
//============================================================================
// BOcclusionManager::testOcclusion
//============================================================================
BOcclusionManager::eOcclusionResult BOcclusionManager::testOcclusion(BVector min, BVector max,XMMATRIX transform)
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(BOcclusionManager_testOcclusion)

   if(!gOcclusionManager.isOcclusionTestEnabled())
      return eOR_FullyVisible;

   const XMMATRIX ident = XMMatrixIdentity();

   //convert min & max to bufferspace to define our area to test within.
   XMVECTOR mnWorld = XMVectorSet(min.x,min.y,min.z,1);
   XMVECTOR mxWorld = XMVectorSet(max.x,max.y,max.z,1);

   {
      SCOPEDSAMPLE(BOcclusionManager_testOcclusion_areaEarlyOut)

      //early out if the chunk is just massive?
      float area =0;
      BVMXIntersection::calculateBoxArea(area, mCameraPos, mnWorld, mxWorld, ident, mWorldToScreen);
      if(area > cAreaCutoffAmt)
         return eOR_Indeterminate;
   }

   //splat our closest z value to screen
   XMVECTOR testVec;
   XMVECTOR mnScreen;
   XMVECTOR mxScreen;
   calcScreenSpaceBBox(mnWorld,mxWorld, transform, testVec, mnScreen, mxScreen);

   //calculate our bounds to test
   //convert from screnspace to XMVector space
   int minX = Math::Clamp<int>((int)floor(mnScreen.x * 0.25f),0,mBufferWidth);
   int minY = Math::Clamp<int>((int)floor(mnScreen.y),0,mBufferHeight);
   int maxX = Math::Clamp<int>((int)ceil(mxScreen.x * 0.25f),0,mBufferWidth);
   int maxY = Math::Clamp<int>((int)ceil(mxScreen.y),0,mBufferHeight);

    
   //consts used for testing

   XMVECTOR oneInc = XMVectorSplatOne();
   XMVECTOR falInc = XMVectorReplicate(0);// = XMVectorSplatZero();
   {
      SCOPEDSAMPLE(BOcclusionManager_testOcclusion_Loop)

      Utils::BPrefetchState prefetch = Utils::BeginPrefetch(mpOcclusionBuffer, 3);
      for(int y =minY;y < maxY;y++)
      {
         for(int x =minX;x < maxX;x++)
         {
            int idx = x + mNumXMVectorsX * y;
            prefetch = Utils::UpdatePrefetch(prefetch, &mpOcclusionBuffer[idx], 3);

            //we only care if we're fully invisible
            if(XMVector4Greater(testVec,mpOcclusionBuffer[idx]))
            {
               falInc = XMVectorAdd(falInc, oneInc);
            }
         }
      }
   }
   
   if(XMVector4Equal(falInc, XMVectorReplicate((float)((maxY-minY) * (maxX-minX)))))
      return eOR_FullyHidden;

   //Might be fully visible, but we don't care right now..
   return eOR_Indeterminate;

}

//============================================================================
//============================================================================
// Debug functions
//============================================================================
//============================================================================

#ifndef BUILD_FINAL

//============================================================================
// BOcclusionManager::debugDraw
//============================================================================
void BOcclusionManager::debugDraw(ATG::Font& font)
{
   ASSERT_THREAD(cThreadIndexRender);

   if (!mDebugDraw || !mpOcclusionBuffer)
      return;

   D3DLOCKED_RECT pLockedRect;
   uint pLen=0;

   
   IDirect3DTexture9* depthTex = gRenderDraw.lockDynamicTexture(mBufferWidth,mBufferHeight,D3DFMT_LIN_A8R8G8B8, &pLockedRect, &pLen);
   byte* pDat = (byte*)pLockedRect.pBits;

   int invBuf = mNumXMVectorsX-1;
   for ( int y=0; y <  mNumXMVectorsY; y++)
   {
      for ( int x=0; x < mNumXMVectorsX; x++)
      {
         int pixC = y*(mBufferWidth*4) + ((invBuf-x)*16);
         int vmxInd=y*mNumXMVectorsX + x;

         BVector v = mpOcclusionBuffer[vmxInd];
         byte b = (byte)(255 * v.w);
         pDat[pixC++] = b;
         pDat[pixC++] = b;
         pDat[pixC++] = b;
         pDat[pixC++] = b;

         b = (byte)(255 * v.z);
         pDat[pixC++] = b;
         pDat[pixC++] = b;
         pDat[pixC++] = b;
         pDat[pixC++] = b;

         b = (byte)(255 * v.y);
         pDat[pixC++] =  b;
         pDat[pixC++] =  b;
         pDat[pixC++] =  b;
         pDat[pixC++] =  b;

         b = (byte)(255 * v.x);
         pDat[pixC++] = b;
         pDat[pixC++] = b;
         pDat[pixC++] = b;
         pDat[pixC++] = b;
      }
   }

      
   gRenderDraw.unlockDynamicTexture();

   BD3D::mpDev->SetTexture(0, depthTex);

   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
   BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);

   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);    
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);


   int rectWidth = mBufferWidth;// / 2;
   int rectHeight = mBufferHeight;//    / 2;
   int xOfs = 50;
   int yOfs = gRender.getHeight() - 20 - rectHeight;

   BPrimDraw2D::drawSolidRect2D(xOfs, yOfs, xOfs+rectWidth, yOfs+rectHeight, 1.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF, cPosTex1VS, cTex1PS);

   BD3D::mpDev->SetTexture(0, NULL);
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, TRUE);
   BD3D::mpDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

}

#endif