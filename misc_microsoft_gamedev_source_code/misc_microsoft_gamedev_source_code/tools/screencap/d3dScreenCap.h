// File: d3dScreenCap.h
#pragma once

#include <d3d9.h>

class BD3DScreenCapture
{
   BD3DScreenCapture(const BD3DScreenCapture&);
   BD3DScreenCapture& operator= (const BD3DScreenCapture&);
   
public:
   BD3DScreenCapture();
   ~BD3DScreenCapture();
   
   HRESULT init(void);
   void deinit(void);
   
   bool getInitialized(void) const { return NULL != mpD3DDevice; }
   uint getWidth(void) const { return mWidth; }
   uint getHeight(void) const { return mHeight; }
   double getCaptureTime(void) const { return mCaptureTime; }
   
   HRESULT capture(void);
   bool getBits(const DWORD*& pData, uint& pitch);
   
private:
   HWND                       mhWnd;           
   IDirect3D9*                mpD3D;
   IDirect3DDevice9*          mpD3DDevice;
   IDirect3DSurface9*         mpSurface;
         
   uint                       mWidth;
   uint                       mHeight;
   double                     mCaptureTime;
      
   HRESULT reset(void);
};   