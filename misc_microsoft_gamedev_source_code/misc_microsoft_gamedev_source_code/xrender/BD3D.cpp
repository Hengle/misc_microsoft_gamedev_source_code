//============================================================================
//
//  BD3D.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "BD3D.h"
#include "renderThread.h"

#define D3D_SINGLESTEP 0
#define D3D_DISABLE_CONSTANT_OVERWRITE_CHECK 1

IDirect3D9*             BD3D::mpD3D;
IDirect3DDevice9*       BD3D::mpDev;
D3DPRESENT_PARAMETERS   BD3D::mD3DPP; 
IDirect3DTexture9*      BD3D::mpDevFrontBuffer;
IDirect3DSurface9*      BD3D::mpDevBackBuffer;
IDirect3DSurface9*      BD3D::mpDevDepthStencil;

void BD3D::BCreateDeviceParams::clear(void)
{
   Utils::ClearObj(*this);

   mBackBufferWidth = 1280;
   mBackBufferHeight = 720;
   mBackBufferFormat = D3DFMT_A8R8G8B8;
   mFrontBufferFormat = D3DFMT_LE_X8R8G8B8;
   mDepthFormat = D3DFMT_D24S8;
   mPresentInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   mPresentImmediateThreshold = 40;
   mPresentImmediateThresholdSD = 15;
   mBuffer2Frames = false;
   mAspectRatioMode = 0;
}

bool BD3D::init(const BCreateDeviceParams& params)
{
   ASSERT_RENDER_THREAD
   
   if (BD3D::mpD3D)
      return true;
      
   // Create the D3D object.
   BD3D::mpD3D = Direct3DCreate9( D3D_SDK_VERSION );

   // Set up the structure used to create the D3DDevice.

   ZeroMemory( &BD3D::mD3DPP, sizeof(BD3D::mD3DPP) );

   if (params.mHasVideoScalerParams)
   {
      BD3D::mD3DPP.VideoScalerParameters.ScalerSourceRect = params.mScalerSourceRect;
      BD3D::mD3DPP.VideoScalerParameters.ScaledOutputWidth = params.mScaledOutputWidth;
      BD3D::mD3DPP.VideoScalerParameters.ScaledOutputHeight = params.mScaledOutputHeight;
   }      

   BD3D::mD3DPP.BackBufferWidth        = params.mBackBufferWidth;
   BD3D::mD3DPP.BackBufferHeight       = params.mBackBufferHeight;
   BD3D::mD3DPP.BackBufferFormat       = params.mBackBufferFormat;
   BD3D::mD3DPP.FrontBufferFormat      = params.mFrontBufferFormat;
   BD3D::mD3DPP.MultiSampleType        = D3DMULTISAMPLE_NONE;
   BD3D::mD3DPP.MultiSampleQuality     = 0;
   BD3D::mD3DPP.BackBufferCount        = 1;
   BD3D::mD3DPP.EnableAutoDepthStencil = TRUE;
   BD3D::mD3DPP.AutoDepthStencilFormat = params.mDepthFormat;
   BD3D::mD3DPP.SwapEffect             = D3DSWAPEFFECT_DISCARD;
   BD3D::mD3DPP.PresentationInterval   = params.mPresentInterval; 
   
   DWORD createFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
   if (params.mBuffer2Frames)
      createFlags |= D3DCREATE_BUFFER_2_FRAMES;

   // Create the Direct3D device.
   BD3D::mpD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL, createFlags, &BD3D::mD3DPP, &BD3D::mpDev);   
   
   if (params.mPresentInterval != D3DPRESENT_INTERVAL_IMMEDIATE)
   {
      XVIDEO_MODE videoMode;
      XGetVideoMode(&videoMode);

      uint threshold = params.mPresentImmediateThreshold;
      // Use a lower threshold on low-def screens to prevent tearing.
      if (!videoMode.fIsHiDef)
         threshold = params.mPresentImmediateThresholdSD;
      
      BD3D::mpDev->SetRenderState(D3DRS_PRESENTIMMEDIATETHRESHOLD, threshold);
   }
         
   checkHResult(BD3D::mpDev->GetFrontBuffer(&mpDevFrontBuffer));
   BASSERT(mpDevFrontBuffer);
   mpDevFrontBuffer->Release();
         
   checkHResult(BD3D::mpDev->GetDepthStencilSurface(&mpDevDepthStencil));
   BASSERT(mpDevDepthStencil);
   mpDevDepthStencil->Release();
   
   BD3D::mpDev->GetBackBuffer(0, 0, 0, &mpDevBackBuffer);
   mpDevBackBuffer->Release();

#if defined(BUILD_DEBUG) && !defined(BUILD_CHECKED)
#if D3D_SINGLESTEP   
   D3D__SingleStepper = TRUE;
   trace("Warning: D3D single step enabled!");
#endif
#if D3D_DISABLE_CONSTANT_OVERWRITE_CHECK
   D3D__DisableConstantOverwriteCheck = TRUE;
   trace("Warning: Disabling D3D constant overwrite checking!");
#endif
#endif
         
   return true;
}

bool BD3D::deinit(void)
{
   ASSERT_RENDER_THREAD
   safeRelease(mpDev);
   safeRelease(mpD3D);
   
   Utils::ClearObj(BD3D::mD3DPP); 
   mpDevFrontBuffer = NULL;
   mpDevBackBuffer = NULL;
   mpDevDepthStencil = NULL;
   
   return true;
}

void BD3D::checkHResult(HRESULT hres, const char* pMsg)
{
   ASSERT_RENDER_THREAD
   
   if (FAILED(hres))
   {
      if (pMsg)
      {
         BFATAL_FAIL(pMsg);
      }
      else
      {
         BFATAL_FAIL("BD3D::checkHResult failed!");
      }
   }
}   