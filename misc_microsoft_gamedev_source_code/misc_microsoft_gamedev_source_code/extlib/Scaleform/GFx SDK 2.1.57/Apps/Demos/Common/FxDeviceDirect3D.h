/**********************************************************************

Filename    :   FxDeviceDirect3D.h
Content     :   OpennGL Device  
Created     :   Jan, 2008
Authors     :   
Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FXDEVICED3D_H
#define INC_FXDEVICED3D_H

#include "FxDevice.h"

#if defined(GFC_OS_XBOX360)
    #include "Direct3DXbox360App.h"
    #define APP      Direct3DXboxApp
#elif defined(GFC_OS_WIN32)
    #include <windows.h>
    #include "Direct3DWin32App.h"
    #define APP      Direct3DWin32App
#else
    #error Only Win32 of Xbox360 are supported
#endif


class FxDeviceDirect3D : public FxDevice
{
public:
    typedef APP::renderer_type   renderer_type;
    typedef APP::device_type     device_type;
    typedef APP::viewport_type   viewport_type;

    FxDeviceDirect3D(FxApp* papp);
    virtual         ~FxDeviceDirect3D() {}
   
    virtual void    Clear(unsigned int color);
    virtual void    Push2DRenderView();
    virtual void    Pop2DRenderView();
    virtual void    FillRect(int l, int t, int r, int b, unsigned int color);

    virtual void    SetWireframe(bool wireframeEnable);

    virtual bool    InitRenderer();
    virtual void    PrepareRendererForFrame();

    // Text/ 2D view rendering support functionality
    bool                    In2DView;
    bool                    WasInScene; 

    device_type*   GetD3DDevice() 
    {
        device_type* pdevice = static_cast<APP*>(pApp)->pDevice;
        GASSERT(pdevice);
        return pdevice;
    }
    viewport_type           ViewportSave;
#if defined (GFC_OS_XBOX360)
    // Viewport un-scale values: first two rows in matrix
    float                   ViewportFactorX[4];
    float                   ViewportFactorY[4];
#endif

};

#endif //NC_FXDEVICED3D_H