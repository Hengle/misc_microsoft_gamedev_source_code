//============================================================================
// File: waterManager.cpp
//  
// Copyright (c) 2007, Ensemble Studios
//============================================================================

#include "xgameRender.h"
#include "waterManager.h"
#include "gpuHeap.h"
#include "renderDraw.h"
#include "render.h"
#include "primDraw2D.h"
#include "camera.h"
#include "inputsystem.h"
#include "effectIntrinsicManager.h"

BWaterManager gWaterManager;

//============================================================================
// BWaterManager::BWaterManager
//============================================================================
BWaterManager::BWaterManager() :
   mWidth(256),
   mHeight(256),
   mRenderFormat(D3DFMT_A2B10G10R10F_EDRAM),
   mDepthFormat(D3DFMT_D24S8),
   mTextureFormat(D3DFMT_A16B16G16R16F_EXPAND),
   mMultisampleType(D3DMULTISAMPLE_4_SAMPLES),
   mpRenderTarget(NULL),
   mpDepthStencil(NULL),
   mpReflectionBuffer(NULL),
   mUpdateReflectionBuffer(false)
   #ifndef BUILD_FINAL
      ,mRenderMode(cRenderAll),
      mDebugDraw(false),
      mFullScreenDebugDraw(false)
   #endif
{
   mReflectionPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
}

//============================================================================
// BWaterManager::~BWaterManager
//============================================================================
BWaterManager::~BWaterManager()
{
}

//============================================================================
// BWaterManager::allocateReflectionBuffer
//============================================================================
void BWaterManager::allocateReflectionBuffer()
{
   if (mpReflectionBuffer)
      return;
         
   HRESULT hres = gGPUFrameHeap.createTexture(mWidth, mHeight, 1, 0, mTextureFormat, 0, &mpReflectionBuffer, NULL);
   BVERIFY(SUCCEEDED(hres));
         
   gEffectIntrinsicManager.set(cIntrinsicReflectionTexture, &mpReflectionBuffer, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BWaterManager::releaseReflectionBuffer
//============================================================================
void BWaterManager::releaseReflectionBuffer()
{
   if (!mpReflectionBuffer)
      return;
      
   gRenderDraw.unsetTextures();      
   
   gGPUFrameHeap.releaseD3DResource(mpReflectionBuffer);
   mpReflectionBuffer = NULL;
               
   gEffectIntrinsicManager.set(cIntrinsicReflectionTexture, &mpReflectionBuffer, cIntrinsicTypeTexturePtr);
}

//============================================================================
// BWaterManager::reflectionRenderBegin
//============================================================================
void BWaterManager::reflectionRenderBegin()
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

   // Update world / projection matrices and viewport for reflection
   setReflectionMatricesAndViewport(mReflectionPlane);

   // Clear color and depth
   gRenderDraw.clear(D3DCLEAR_TARGET0|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0x00000000);
}

//============================================================================
// BWaterManager::reflectionRenderEnd
//============================================================================
void BWaterManager::reflectionRenderEnd()
{
   // Resolve render target to texure
   BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, mpReflectionBuffer, NULL, 0, 0, NULL, 1.0f, 0, NULL);

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
void calcScreenSpaceBBox(XMVECTOR min, XMVECTOR max, XMMATRIX WorldToScreen, int vpWidth, int vpHeight, XMVECTOR &ws_min_min, XMVECTOR &ws_min_max, XMVECTOR &ws_max_min, XMVECTOR &ws_max_max)
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(BOcclusionManager_calcScreenSpaceBBox)

   XMVECTOR cubeSets[8] = 
   {
      XMVectorSet(min.x,min.y,min.z,1),
      XMVectorSet(max.x,min.y,min.z,1),
      XMVectorSet(min.x,max.y,min.z,1),
      XMVectorSet(max.x,max.y,min.z,1),
      XMVectorSet(min.x,min.y,max.z,1),
      XMVectorSet(max.x,min.y,max.z,1),
      XMVectorSet(min.x,max.y,max.z,1),
      XMVectorSet(max.x,max.y,max.z,1)
   };

   XMVECTOR zVec = XMVectorSet(0,0,0,0);
   XMVECTOR oVec = XMVectorSet(1,1,1,1);
   XMVECTOR points[8];


   XMVECTOR minBounds = XMVectorSet(0,0,0,1);
   XMVECTOR maxBounds = XMVectorSet((float)vpWidth,(float)vpHeight,0,1);
   bool invalidProj = false;
   //project to screenspace
   for(int i = 0; i < 8; i ++)
   {
      points[i] = cubeSets[i];

      
      points[i] = XMVector4Transform(points[i],WorldToScreen);
      points[i] = XMVectorMultiply(points[i],XMVectorReciprocal(XMVectorSplatW(points[i])));
      

      //if any point is clipped and becomes degenerate, just default out (ie we're so inside the volume, it doesn't matter..)
      //CLM this tends to happen a great deal with 'large' reflection planes..
      if(XMVector4Greater(XMVectorSplatZ(points[i]),oVec) || XMVector4Less(XMVectorSplatZ(points[i]),zVec))
      {
         invalidProj = true;
         break;
         
      }
   }
  
   if(!invalidProj)
   {
      minBounds = points[0];
      maxBounds = points[0];
      for(int i=1;i<8;i++)
      {
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

      //CLAMP!
      minBounds.x = Math::Clamp((float)floor(minBounds.x),0.0f,(float)vpWidth);
      minBounds.y = Math::Clamp((float)floor(minBounds.y),0.0f,(float)vpHeight);

      maxBounds.x = Math::Clamp((float)ceil(maxBounds.x),0.0f,(float)vpWidth);
      maxBounds.y = Math::Clamp((float)ceil(maxBounds.y),0.0f,(float)vpHeight);
   }
   




   //now, transform these points BACK to world space on the near plane.
   XMVECTOR det;
   XMMATRIX iWorldToScreen  = XMMatrixInverse(&det, WorldToScreen);

   ws_max_min = XMVector4Transform(XMVectorSet(minBounds.x,minBounds.y,minBounds.z,1),iWorldToScreen);
   ws_max_min =XMVectorMultiply(ws_max_min,XMVectorReciprocal(XMVectorSplatW(ws_max_min)));

   ws_max_max = XMVector4Transform(XMVectorSet(minBounds.x,maxBounds.y,minBounds.z,1),iWorldToScreen);
   ws_max_max =XMVectorMultiply(ws_max_max,XMVectorReciprocal(XMVectorSplatW(ws_max_max)));

   ws_min_min = XMVector4Transform(XMVectorSet(maxBounds.x,minBounds.y,minBounds.z,1),iWorldToScreen);
   ws_min_min =XMVectorMultiply(ws_min_min,XMVectorReciprocal(XMVectorSplatW(ws_min_min)));

   ws_min_max = XMVector4Transform(XMVectorSet(maxBounds.x,maxBounds.y,minBounds.z,1),iWorldToScreen);
   ws_min_max =XMVectorMultiply(ws_min_max,XMVectorReciprocal(XMVectorSplatW(ws_min_max)));
}

//============================================================================
//============================================================================
#ifndef BUILD_FINAL
XMVECTOR ws_min_min0;
XMVECTOR ws_min_max0;
XMVECTOR ws_max_min0;
XMVECTOR ws_max_max0;
XMVECTOR ws_min_min_line;
XMVECTOR ws_min_max_line;
XMVECTOR ws_max_min_line;
XMVECTOR ws_max_max_line;
#endif

void BWaterManager::setReflectionMatricesAndViewport(XMVECTOR reflectionPlane)
{
   BMatrixTracker&  matrixTracker = gRenderDraw.getWorkerActiveMatrixTracker();
   BRenderViewport& renderViewport = gRenderDraw.getWorkerActiveRenderViewport();
   
   BFrustum origCamFrust = matrixTracker.getWorldFrustum();

   XMVECTOR camPos = matrixTracker.getWorldCamPos();
   XMMATRIX savedWorldToProjection = matrixTracker.getMatrix(cMTWorldToProj);
   XMMATRIX savedWorldToScreen = matrixTracker.getMatrix(cMTWorldToScreen);
   XMMATRIX savedViewToProjection = matrixTracker.getMatrix(cMTViewToProj);


   // Make sure reflection plane is normalized
   reflectionPlane = XMPlaneNormalize(reflectionPlane);
   XMVECTOR planeDot = XMPlaneDot(reflectionPlane, camPos);
   bool camBehindPlane = planeDot.x < 0.0f;

   //============================================================================
   // Calculate new World To View matrix reflected about the reflection plane
   XMMATRIX worldToView = matrixTracker.getMatrix(cMTWorldToView);
   XMMATRIX reflectMtx = XMMatrixReflect(reflectionPlane);

   worldToView = XMMatrixMultiply(reflectMtx, worldToView);
   worldToView._11 *= -1.0f;
   worldToView._21 *= -1.0f;
   worldToView._31 *= -1.0f;
   worldToView._41 *= -1.0f;

   matrixTracker.setMatrix(cMTWorldToView, worldToView);


   //============================================================================
   // Calculate the inclusion planes for the volume culler (before we mess with the oblique plane..)
   // take the 'best' reflection object bounds and project them to the reflected viewspace
   // from there, clip the current frustum to the bounds of the projected object
   // and set those as the inclusion only planes.
   XMVECTOR newPlanes[6];
   {
      XMVECTOR ws_min_min;
      XMVECTOR ws_min_max;
      XMVECTOR ws_max_min;
      XMVECTOR ws_max_max;

      const Plane* planes = matrixTracker.getWorldFrustum().getPlanes();
      XMVECTOR camPosRefl = matrixTracker.getWorldCamPos();
   


      calcScreenSpaceBBox(mReflectionObjMin, mReflectionObjMax, matrixTracker.getMatrix(cMTWorldToScreen), matrixTracker.getViewport().Width,  matrixTracker.getViewport().Height, ws_min_min, ws_min_max, ws_max_min, ws_max_max);
      
      newPlanes[BFrustum::cLeftPlane] = XMPlaneFromPoints(camPosRefl,ws_min_max,ws_min_min);
      newPlanes[BFrustum::cRightPlane] = XMPlaneFromPoints(camPosRefl,ws_max_min,ws_max_max);
      newPlanes[BFrustum::cBottomPlane] = XMPlaneFromPoints(camPosRefl,ws_min_min,ws_max_min);
      newPlanes[BFrustum::cTopPlane] = XMPlaneFromPoints(camPosRefl,ws_max_max,ws_min_max);
      newPlanes[BFrustum::cNearPlane] = XMLoadFloat4((XMFLOAT4*)planes[BFrustum::cNearPlane].equation().getPtr());
      newPlanes[BFrustum::cFarPlane] = XMLoadFloat4((XMFLOAT4*)planes[BFrustum::cFarPlane].equation().getPtr());

      
#ifndef BUILD_FINAL
     {
        ws_min_min0=ws_min_min;
        ws_min_max0=ws_min_max;
        ws_max_min0=ws_max_min;
        ws_max_max0=ws_max_max;

        //debug drawing..
        XMVECTOR dirl = XMVector4Normalize(ws_min_min - camPosRefl); 
        ws_min_min_line = ws_min_min + (dirl * 1000.0f); ws_min_min_line.w=1;

        dirl = XMVector4Normalize(ws_min_max - camPosRefl); 
        ws_min_max_line = ws_min_max + (dirl * 1000.0f); ws_min_max_line.w=1;

        dirl = XMVector4Normalize(ws_max_min - camPosRefl); 
        ws_max_min_line = ws_max_min + (dirl * 100.0f); ws_max_min_line.w=1;

        dirl = XMVector4Normalize(ws_max_max - camPosRefl); 
        ws_max_max_line = ws_max_max + (dirl * 1000.0f); ws_max_max_line.w=1;
     }
#endif

   }




   // Calculate the reflected projection frustum before we set up the oblique
   // clipping plane.  We use this in the volume culler below
   BFrustum cullingFrustumNoClip;
   cullingFrustumNoClip.set(matrixTracker.getMatrix(cMTWorldToProj));

   //============================================================================
   // Set projection matrix to include near plane at the water plane - "Oblique Frustum Clipping"
   // This will clip out stuff below the water from rendering to the reflection buffer

   // Get inverse transpose of current worldToProjection (taking the reflection into account) so that we can
   // transform the clip plane into projection space
   XMVECTOR det;
   XMMATRIX worldToProjection = XMMatrixMultiply(matrixTracker.getMatrix(cMTWorldToView), savedViewToProjection);
   worldToProjection = XMMatrixInverse(&det, worldToProjection);
   worldToProjection = XMMatrixTranspose(worldToProjection);

   // Make clip plane in world space
   static float clipOffset = -0.1f;
   XMVECTOR clipPlane = camBehindPlane ? reflectionPlane : -reflectionPlane;
   clipPlane.w += clipOffset;

   // TODO - Fix this up so that we can use the clipped planes for culling
   // instead of removing these at the end of the function to use just the
   // reflected frustum.
   #if 0

      // This version is from "Oblique View Frustums for Mirrors and Portals" in
      // Game Programming Gems 5.  The intent is to fix up the far plane by
      // applying scale to the near-clip plane.  It's close, but not quite working

      // Note that there are some assumptions in the article about the projection
      // and camera space clip plane having the same x and y signedness.  This
      // is fairly safe with standard projection matrices (of which we use),
      // but not with wacky ones.
      
      // Get camera space clip plane
      XMMATRIX m = savedViewToProjection;
      XMMATRIX worldToViewInvT = matrixTracker.getMatrix(cMTWorldToView);
      worldToViewInvT = XMMatrixInverse(&det, worldToViewInvT);
      worldToViewInvT = XMMatrixTranspose(worldToViewInvT);

      XMVECTOR camClipPlane = XMVector4Transform(clipPlane, worldToViewInvT);

      // Make sure it points the right direction
      if (camClipPlane.w > 0.0f)
         camClipPlane = -camClipPlane;

      // Calculate Q
      XMVECTOR q;
      float signX = (camClipPlane.x > 0.0f) ? 1.0f : (camClipPlane.x < 0.0f ? -1.0f : 0.0f);
      float signY = (camClipPlane.y > 0.0f) ? 1.0f : (camClipPlane.y < 0.0f ? -1.0f : 0.0f);
      q.x = (signX - m._31) / m._11;
      q.y = (signY - m._32) / m._22;
      q.z = 1.0f;
      q.w = (1.0f - m._33) / m._43;

      float scale = 1.0f / XMVector4Dot(camClipPlane, q).x;
      XMVECTOR c = camClipPlane * scale;

      // Replace 3rd column
      XMMATRIX viewToProj = savedViewToProjection;
      viewToProj._13 = c.x;
      viewToProj._23 = c.y;
      viewToProj._33 = c.z;
      viewToProj._43 = c.w;
      matrixTracker.setMatrix(cMTViewToProj, viewToProj);

   #else

      // Transform clipPlane into projection space
      XMVECTOR projClipPlane;
      projClipPlane = XMVector4Transform(clipPlane, worldToProjection);

      // If projClipPlane.w not positive, the clip plane is perpendicular to the
      // current near plane, so we won't mess with it
      if (projClipPlane.w >= cFloatCompareEpsilon)
      {
         projClipPlane = XMVector4Normalize(projClipPlane);

         // Flip plane to point away from eye
         if (projClipPlane.w > 0)
         {
            projClipPlane = -projClipPlane;
         }

         // Put projection space clip plane in Z column
         XMMATRIX matClipProj = XMMatrixIdentity();
         matClipProj(0, 2) = projClipPlane.x;
         matClipProj(1, 2) = projClipPlane.y;
         matClipProj(2, 2) = projClipPlane.z;
         matClipProj(3, 2) = projClipPlane.w;

         // Multiply into projection matrix
         XMMATRIX viewToProj = XMMatrixMultiply(savedViewToProjection, matClipProj);
         matrixTracker.setMatrix(cMTViewToProj, viewToProj);
      }

   #endif

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

   BVolumeCuller& volumeCuller = gRenderDraw.getWorkerActiveVolumeCuller();
   //============================================================================
   // Fix up volume culler here. gRenderDraw.setWorkerActiveMatrixTracker(matrixTracker);
   // will set the active volume culler's base planes to the matrix tracker's
   // world frustum.  The math currently doesn't play nicely with an oblique
   // near clipping plane, so we have to fix it up here

   // TODO - Look at volume culler's frustum math to see if it will work better
   // with an oblique near plane.  If the alternate projection matrix setting
   // worked above, we shouldn't need to do this fixup
   
   volumeCuller.setBasePlanes(cullingFrustumNoClip);

    volumeCuller.enableInclusionPlanes(newPlanes ,6);
}

//============================================================================
//============================================================================
// Debug functions
//============================================================================
//============================================================================

#ifndef BUILD_FINAL

//============================================================================
// BWaterManager::debugHandleInpit
//============================================================================
void BWaterManager::debugHandleInput(long event, long controlType, bool altPressed, bool shiftPressed, bool controlPressed)
{
   if (event == cInputEventControlStart)
   {
      switch (controlType)
      {
         case cKeyR:
         {
            if (altPressed)
            {
               mFullScreenDebugDraw = !mFullScreenDebugDraw;
               
               gConsoleOutput.status("Water fullscreen debug draw %s", mFullScreenDebugDraw ? "Enabled" : "Disabled");
            }
            else if (controlPressed)
            {
               // Go to next render mode
               switch (mRenderMode)
               {
                  case cRenderNone:
                     mRenderMode = cRenderAll;
                     break;
                  case cRenderAll:
                     mRenderMode = cRenderTerrain;
                     break;
                  case cRenderTerrain:
                     mRenderMode = cRenderUnits;
                     break;
                  case cRenderUnits:
                     mRenderMode = cRenderNone;
                     break;
               }
               
               gConsoleOutput.status("Water mode: %u", mRenderMode);
            }
            else if (shiftPressed)
            {
               // Toggle render formats
               if (mRenderFormat == D3DFMT_A8R8G8B8)
                  mRenderFormat = D3DFMT_A2B10G10R10F_EDRAM;
               else if (mRenderFormat == D3DFMT_A2B10G10R10F_EDRAM)
                  mRenderFormat = D3DFMT_A8R8G8B8;

               if (mTextureFormat == D3DFMT_A8R8G8B8)
                  mTextureFormat = D3DFMT_A16B16G16R16F_EXPAND;
               else if (mTextureFormat == D3DFMT_A16B16G16R16F_EXPAND)
                  mTextureFormat = D3DFMT_A8R8G8B8;

               gConsoleOutput.status("Changed water texture format");
               
               break;
            }
            else
            {
               mDebugDraw = !mDebugDraw;
               
               gConsoleOutput.status("Water debug draw %s", mDebugDraw ? "Enabled" : "Disabled");
            }
               
            break;
         }
      }
   }
}

//============================================================================
// BWaterManager::debugDraw
//============================================================================
void BWaterManager::debugDraw(ATG::Font& font)
{
   ASSERT_THREAD(cThreadIndexRender);

   if (!mDebugDraw)
      return;

   // Render text
   static float x = 60.0f;
   static float y = 400.0f;
   static DWORD textColor = 0xffffffff;

   BString bufferFormatStr, modeStr;
   if (mTextureFormat == D3DFMT_A8R8G8B8)
      bufferFormatStr.set("A8R8G8B8");
   else if (mTextureFormat == D3DFMT_A16B16G16R16F_EXPAND)
      bufferFormatStr.set("16F_EXPAND");

   switch (mRenderMode)
   {
      case cRenderNone:
         modeStr.set("None");
         break;
      case cRenderAll:
         modeStr.set("All");
         break;
      case cRenderTerrain:
         modeStr.set("Terrain Only");
         break;
      case cRenderUnits:
         modeStr.set("Units Only");
         break;
   }

   BUString str;
   str.format(L"Reflection Buffer = %S, Mode = %S", bufferFormatStr.getPtr(), modeStr.getPtr());
   font.DrawText(x, y, textColor, str.getPtr());

   if (!mpReflectionBuffer)
      return;

   // Render quad with reflection texture
   BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);      
   BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);      
   BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);      
   BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);      

   BD3D::mpDev->SetTexture(0, mpReflectionBuffer);
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
   if (mFullScreenDebugDraw)
   {
      rectWidth = gRender.getWidth();
      rectHeight = gRender.getHeight();
      xOfs = 0;
      yOfs = 0;
   }

   BPrimDraw2D::drawSolidRect2D(xOfs, yOfs, xOfs+rectWidth, yOfs+rectHeight, 1.0f, 0.0f, 0.0f, 1.0f, 0xFFFFFFFF, 0xFFFFFFFF, cPosTex1VS, cTex1PS);



   BPrimDraw2D::drawLine(ws_min_min0,ws_min_max0, 0xFFFF00FF, 0xFFFF0000);
   BPrimDraw2D::drawLine(ws_max_min0,ws_max_max0, 0xFFFF00FF, 0xFFFF0000);
   BPrimDraw2D::drawLine(ws_min_min0,ws_max_min0, 0xFFFF00FF, 0xFFFF00FF);
   BPrimDraw2D::drawLine(ws_min_max0,ws_max_max0, 0xFFFF0000, 0xFFFF0000);


   BPrimDraw2D::drawLine(ws_min_min0,ws_min_min_line, 0xFFFF00FF, 0xFFFF00FF);
   BPrimDraw2D::drawLine(ws_max_min0,ws_max_min_line, 0xFFFF00FF, 0xFFFF00FF);
   BPrimDraw2D::drawLine(ws_min_max0,ws_min_max_line, 0xFFFF0000, 0xFFFF0000);
   BPrimDraw2D::drawLine(ws_max_max0,ws_max_max_line, 0xFFFF0000, 0xFFFF0000);

   BPrimDraw2D::drawLine(mReflectionObjMin,mReflectionObjMax, 0xFFFFFF00, 0xFFFFFF00);
   
   

}

#endif
