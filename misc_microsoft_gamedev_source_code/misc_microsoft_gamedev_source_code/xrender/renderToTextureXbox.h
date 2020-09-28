//============================================================================
//
// File: renderToTextureXbox.h
//
//  Copyright (c) 2006 Ensemble Studios
//
//============================================================================
#pragma once

//============================================================================
// class BRenderToTextureHelperXbox
//============================================================================
class BRenderToTextureHelperXbox
{
public:
   BRenderToTextureHelperXbox();
   BRenderToTextureHelperXbox(uint width, uint height, D3DFORMAT format, D3DFORMAT depthFormat = D3DFMT_UNKNOWN, D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE);
   ~BRenderToTextureHelperXbox();
   
   void set(uint width, uint height, D3DFORMAT format, D3DFORMAT depthFormat = D3DFMT_UNKNOWN, D3DMULTISAMPLE_TYPE multisample = D3DMULTISAMPLE_NONE);

   bool createDeviceObjects(int baseAddr = 0);
   void destroyDeviceObjects(void);
   
   void begin(IDirect3DTexture9* pColor);
   void resolve(IDirect3DTexture9* pColor);
   void end(void);

   int getWidth(void) const { return mWidth; }
   int getHeight(void) const { return mHeight; }
   D3DMULTISAMPLE_TYPE getMultisample(void) const { return mMultisample; }
   D3DFORMAT getFormat(void) const { return mFormat; }
   D3DFORMAT getDepthFormat(void) const { return mDepthFormat; }
   IDirect3DSurface9* getColorSurf(void) const { return mpColorSurf; }
   IDirect3DSurface9* getDepthSurf(void) const { return mpDepthSurf; }
   
   uint getEDRAMBase(void) const { return mEDRAMBase; }
   uint getTotalEDRAMUsed(void) const { return mTotalEDRAMUsed; }

private:
   uint16 mWidth;
   uint16 mHeight;
   D3DFORMAT mFormat;
   D3DFORMAT mDepthFormat;
   D3DMULTISAMPLE_TYPE mMultisample;

   IDirect3DSurface9* mpColorSurf;
   IDirect3DSurface9* mpDepthSurf;
   
   IDirect3DSurface9* mpSavedColorSurf;
   IDirect3DSurface9* mpSavedDepthSurf;
   D3DVIEWPORT9 mSavedViewport;
   RECT mSavedScissorRect;
   
   uint mEDRAMBase;
   uint mTotalEDRAMUsed;
}; // class BRenderToTextureHelperXbox

typedef BRenderToTextureHelperXbox BRenderToTextureHelper;