//============================================================================
//
//  renderViewport.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "renderViewport.h"
#include "BD3D.h"
#include "renderThread.h"

BRenderViewport::BRenderViewport() 
{
   Utils::ClearObj(mpSurfs);
   mpDepthStencil = NULL;
   
   mViewport.X = 0;
   mViewport.Y = 0;
   mViewport.Width = 640;
   mViewport.Height = 480;
   mViewport.MinZ = 0.0f;
   mViewport.MaxZ = 1.0f;
   
   mScissorRect.left = 0;
   mScissorRect.top = 0;
   mScissorRect.right = 640;
   mScissorRect.bottom = 480;
   mScissorEnabled = false;
}

BRenderViewport::BRenderViewport(
   IDirect3DSurface9* pSurf, 
   IDirect3DSurface9* pDepthStencil) 
{
   Utils::ClearObj(mpSurfs);
   mpSurfs[0] = pSurf;

   mpDepthStencil = pDepthStencil;
   
   mViewport.X = mViewport.Y = 0;
   mViewport.Width = 640;
   mViewport.Height = 480;
   mViewport.MinZ = 0.0f;
   mViewport.MaxZ = 1.0f;
   
   mScissorRect.left = 0;
   mScissorRect.top = 0;
   mScissorRect.right = 640;
   mScissorRect.bottom = 480;
   mScissorEnabled = false;
}

BRenderViewport::BRenderViewport(
   IDirect3DSurface9* pSurf, 
   IDirect3DSurface9* pDepthStencil,
   const D3DVIEWPORT9& viewport) 
{
   Utils::ClearObj(mpSurfs);
   mpSurfs[0] = pSurf;

   mpDepthStencil = pDepthStencil;
   mViewport = viewport;
   
   mScissorRect.left = 0;
   mScissorRect.top = 0;
   mScissorRect.right = mViewport.X + mViewport.Width;
   mScissorRect.bottom = mViewport.Y + mViewport.Height;
   mScissorEnabled = false;
}

BRenderViewport::BRenderViewport(const BRenderViewport& rhs)
{
   *this = rhs;
}

void BRenderViewport::setToDevice(uint setFlags) const
{
   ASSERT_RENDER_THREAD
   
   if (setFlags & cSetRenderTarget)
   {
      for (int i = 0; i < NumSurfaces; i++)
      {
         IDirect3DSurface9* pSurf = mpSurfs[i];
                           
         BD3D::mpDev->SetRenderTarget(i, pSurf);
      }
   }
   
   if (setFlags & cSetDepthStencil)
      BD3D::mpDev->SetDepthStencilSurface(mpDepthStencil);
      
   if (setFlags & cSetViewport)
      BD3D::mpDev->SetViewport(&mViewport);
      
   if (setFlags & cSetScissor)      
   {
      BD3D::mpDev->SetScissorRect(&mScissorRect);
      BD3D::mpDev->SetRenderState(D3DRS_SCISSORTESTENABLE, mScissorEnabled ? TRUE : FALSE);
   }
}   
