//============================================================================
//
//  renderViewport.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// rendertarget/BRenderViewport set flags
enum 
{ 
   cSetRenderTarget  = 1, 
   cSetDepthStencil  = 2,
   cSetViewport      = 4, 
   cSetScissor       = 8,
   cSetAll           = 255
};

#define BRENDERVIEWPORT_BACKBUFFER_PTR ((IDirect3DSurface9*)0x1)
#define BRENDERVIEWPORT_DEPTHSTENCIL_PTR ((IDirect3DSurface9*)0x2)

// Must be bitwise-copyable!
class BRenderViewport
{
public:
   BRenderViewport();
   
   BRenderViewport(
      IDirect3DSurface9* pSurf, 
      IDirect3DSurface9* pDepthStencil);
      
   BRenderViewport(
      IDirect3DSurface9* pSurf, 
      IDirect3DSurface9* pDepthStencil, 
      const D3DVIEWPORT9& viewport);
               
   BRenderViewport(const BRenderViewport& rhs);
      
   IDirect3DSurface9* getSurf(uint index = 0) const { return mpSurfs[debugRangeCheck<uint, uint>(index, NumSurfaces)]; }
   void setSurf(uint index, IDirect3DSurface9* pSurf) { mpSurfs[debugRangeCheck<uint, uint>(index, NumSurfaces)] = pSurf; }
   
   void setDepthStencilSurf(IDirect3DSurface9* pSurf) { mpDepthStencil = pSurf; }
   IDirect3DSurface9* getDepthStencilSurf(void) const { return mpDepthStencil; }
         
   const D3DVIEWPORT9& getViewport(void) const { return mViewport; }
   void setViewport(const D3DVIEWPORT9& viewport) { mViewport = viewport; }

   // Callable from the worker thread.   
   void setToDevice(uint setFlags = cSetAll) const;
   
   void setScissorEnabled(bool enabled) { mScissorEnabled = enabled; }
   bool getScissorEnabled(void) const { return mScissorEnabled; }
   
   void setScissorRect(const RECT& rect) { mScissorRect = rect; }
   const RECT& getScissorRect(void) const { return mScissorRect; }
                                 
protected:
   enum { NumSurfaces = 4 };
   IDirect3DSurface9* mpSurfs[NumSurfaces];
   IDirect3DSurface9* mpDepthStencil;
      
   D3DVIEWPORT9 mViewport;
   
   RECT mScissorRect;
   bool mScissorEnabled;
};
