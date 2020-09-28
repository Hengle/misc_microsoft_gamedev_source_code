//============================================================================
//
//  BD3D.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

const uint cMaxD3DTextureSamplers = 26;

// This low-level class should only be accessed from the worker thread!
class BD3D
{
public:
   static IDirect3D9*             mpD3D;
   static IDirect3DDevice9*       mpDev;
   static D3DPRESENT_PARAMETERS   mD3DPP; 
   static IDirect3DTexture9*      mpDevFrontBuffer;
   static IDirect3DSurface9*      mpDevBackBuffer;
   static IDirect3DSurface9*      mpDevDepthStencil;
   
   struct BCreateDeviceParams
   {
      BCreateDeviceParams() {  clear(); }
            
      void        clear(void);
      
      uint        mBackBufferWidth;
      uint        mBackBufferHeight;
      D3DFORMAT   mBackBufferFormat;
      D3DFORMAT   mFrontBufferFormat;
      D3DFORMAT   mDepthFormat;
      DWORD       mPresentInterval;
      uint        mPresentImmediateThreshold;
      uint        mPresentImmediateThresholdSD;

      D3DRECT     mScalerSourceRect;
      uint        mScaledOutputWidth;
      uint        mScaledOutputHeight;
      bool        mHasVideoScalerParams;
      bool        mBuffer2Frames;
      int         mAspectRatioMode;
   };

   // BRenderThread will call init() and deinit() from the worker thread. 
   static bool init(const BCreateDeviceParams& params);
   static bool deinit(void);

   static void checkHResult(HRESULT hres, const char* pMsg = NULL);
   
   template<class T> static void safeRelease(T*& p)   
   {  
      if (p) 
      { 
         p->Release(); 
         p = NULL; 
      }
   }

   template<class T> static void safeReleaseVec(T& vec)
   {
      for (T::iterator it = vec.begin(); it != vec.end(); ++it)
         safeRelease(*it);
   }
}; // class BD3D
