/**********************************************************************

Filename    :   FxDevice.h
Content     :   Base class for FxDevice  
Created     :   Jan, 2008
Authors     :   Maxim Didenko, Dmitry Polenur

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FxDevice_H
#define INC_FxDevice_H

#include "GTypes.h"
#include "GMemory.h"
#include "FxApp.h"

class FxDevice : public GNewOverrideBase
{
public:
    FxDevice(FxApp* papp ) : pApp(papp){}
    virtual ~FxDevice();
    enum DeviceTypes
    {
        Undefined,
        Direct3D,
        OpenGL,
    };

    DeviceTypes Type;

    virtual void    Clear(unsigned int color) = 0;
    virtual void    Push2DRenderView() = 0;
    virtual void    Pop2DRenderView() = 0;

    // Draw a filled + blended rectangle.
    virtual void    FillRect(int l, int t, int r, int b, unsigned int color) = 0;
    virtual void    SetWireframe(bool wireframeEnable) = 0;

    virtual bool    InitRenderer() = 0;
    virtual void    PrepareRendererForFrame() = 0;

    FxApp*         pApp;

    static void Color32ToFloat(float *prgba, unsigned int color)
    {
        float scalar = 1.0f / 255.0f;
        prgba[3] =  (color >> 24) * scalar;
        prgba[0] =  ((color >> 16) & 0xFF) * scalar;
        prgba[1] =  ((color >> 8) & 0xFF) * scalar;
        prgba[2] =  (color & 0xFF) * scalar;
    }

};

#endif // INC_FxDevice_H
