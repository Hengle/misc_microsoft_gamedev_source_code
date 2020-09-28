//============================================================================
//
// File: renderToTextureXbox.cpp
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "renderToTextureXbox.h"
#include "renderDraw.h"
#include "BD3D.h"

//============================================================================
// BRenderToTextureHelperXbox::BRenderToTextureHelperXbox
//============================================================================
BRenderToTextureHelperXbox::BRenderToTextureHelperXbox() :
   mWidth(0),
   mHeight(0),
   mFormat(D3DFMT_UNKNOWN),
   mDepthFormat(D3DFMT_UNKNOWN),
   mMultisample(D3DMULTISAMPLE_NONE),
   mpColorSurf(NULL),
   mpDepthSurf(NULL),
   mpSavedColorSurf(NULL),
   mpSavedDepthSurf(NULL),
   mTotalEDRAMUsed(0),
   mEDRAMBase(0)
{
   Utils::ClearObj(mSavedViewport);
   Utils::ClearObj(mSavedScissorRect);
}

//============================================================================
// BRenderToTextureHelperXbox::BRenderToTextureHelperXbox
//============================================================================
BRenderToTextureHelperXbox::BRenderToTextureHelperXbox(uint width, uint height, D3DFORMAT format, D3DFORMAT depthFormat, D3DMULTISAMPLE_TYPE multisample) :
   mWidth(static_cast<uint16>(width)),
   mHeight(static_cast<uint16>(height)),
   mFormat(format),
   mDepthFormat(depthFormat),
   mMultisample(multisample),
   mpColorSurf(NULL),
   mpDepthSurf(NULL),
   mpSavedColorSurf(NULL),
   mpSavedDepthSurf(NULL),
   mTotalEDRAMUsed(0),
   mEDRAMBase(0)
{
   Utils::ClearObj(mSavedViewport);
   Utils::ClearObj(mSavedScissorRect);
}

//============================================================================
// BRenderToTextureHelperXbox::~BRenderToTextureHelperXbox
//============================================================================
BRenderToTextureHelperXbox::~BRenderToTextureHelperXbox()
{
   destroyDeviceObjects();
}

//============================================================================
// BRenderToTextureHelperXbox::set
//============================================================================
void BRenderToTextureHelperXbox::set(uint width, uint height, D3DFORMAT format, D3DFORMAT depthFormat, D3DMULTISAMPLE_TYPE multisample)
{
   destroyDeviceObjects();
   
   mWidth = static_cast<uint16>(width);
   mHeight = static_cast<uint16>(height);
   mFormat = format;
   mDepthFormat = depthFormat;
   mMultisample = multisample;
   mpColorSurf = NULL;
   mpDepthSurf = NULL;
   mpSavedColorSurf = NULL;
   mpSavedDepthSurf = NULL;
   Utils::ClearObj(mSavedViewport);
   Utils::ClearObj(mSavedScissorRect);
   
   mTotalEDRAMUsed = 0;
   mEDRAMBase = 0;
}

//============================================================================
// BRenderToTextureHelperXbox::createDeviceObjects
//============================================================================
bool BRenderToTextureHelperXbox::createDeviceObjects(int baseAddr)
{
   destroyDeviceObjects();
   
   mEDRAMBase = baseAddr;
   
   D3DSURFACE_PARAMETERS surfaceParams;
   Utils::ClearObj(surfaceParams);

   surfaceParams.Base = baseAddr;

   HRESULT hres = gRenderDraw.createRenderTarget(mWidth, mHeight, mFormat, mMultisample, 0, FALSE, &mpColorSurf, &surfaceParams);
   if (FAILED(hres))
      return false;      

   mTotalEDRAMUsed = XGSurfaceSize(mWidth, mHeight, mFormat, mMultisample);
   if (D3DFMT_UNKNOWN != mDepthFormat)
   {
      surfaceParams.Base = mTotalEDRAMUsed;
      
      mTotalEDRAMUsed += XGSurfaceSize(mWidth, mHeight, mDepthFormat, mMultisample);

      hres = BD3D::mpDev->CreateDepthStencilSurface(mWidth, mHeight, mDepthFormat, mMultisample, 0, FALSE, &mpDepthSurf, &surfaceParams);
      if (FAILED(hres))
         return false;
   }            
      
   return true;         
}

//============================================================================
// BRenderToTextureHelperXbox::destroyDeviceObjects
//============================================================================
void BRenderToTextureHelperXbox::destroyDeviceObjects(void)
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
// BRenderToTextureHelperXbox::begin
//============================================================================
void BRenderToTextureHelperXbox::begin(IDirect3DTexture9* pColor)
{
   BDEBUG_ASSERT(mpColorSurf);

   BD3D::mpDev->GetRenderTarget(0, &mpSavedColorSurf);
   BD3D::mpDev->GetDepthStencilSurface(&mpSavedDepthSurf);
   BD3D::mpDev->GetViewport(&mSavedViewport);
   BD3D::mpDev->GetScissorRect(&mSavedScissorRect);
   
   BD3D::mpDev->SetRenderTarget(0, mpColorSurf);
   BD3D::mpDev->SetDepthStencilSurface(mpDepthSurf);
}

//============================================================================
// BRenderToTextureHelperXbox::resolve
//============================================================================
void BRenderToTextureHelperXbox::resolve(IDirect3DTexture9* pColor)
{
   BDEBUG_ASSERT(mpSavedColorSurf);
   
   BD3D::mpDev->Resolve(
      D3DRESOLVE_RENDERTARGET0,
      NULL,
      pColor,
      NULL,
      0,
      0,
      NULL,
      0.0f,
      0,
      NULL);
}

//============================================================================
// BRenderToTextureHelperXbox::end
//============================================================================
void BRenderToTextureHelperXbox::end(void)
{  
   BDEBUG_ASSERT(mpSavedColorSurf);
   
   BD3D::mpDev->SetRenderTarget(0, mpSavedColorSurf);
   mpSavedColorSurf->Release();
   mpSavedColorSurf = NULL;
   
   BD3D::mpDev->SetDepthStencilSurface(mpSavedDepthSurf);
   if (mpSavedDepthSurf)
   {
      mpSavedDepthSurf->Release();
      mpSavedDepthSurf = NULL;
   }
   
   BD3D::mpDev->SetViewport(&mSavedViewport);         
   BD3D::mpDev->SetScissorRect(&mSavedScissorRect);
}


