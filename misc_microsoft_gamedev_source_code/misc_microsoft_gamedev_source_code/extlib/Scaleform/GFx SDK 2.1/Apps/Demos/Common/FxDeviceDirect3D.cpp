/**********************************************************************

Filename    :   FxDeviceDirect3D.h
Content     :   OpennGL Device  
Created     :   Jan, 2008
Authors     :   
Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "FxDeviceDirect3D.h"
#ifdef GFC_OS_WIN32 //can be used for xbox
    #include <windows.h>
#endif //GFC_OS_WIN32 

 FxDeviceDirect3D::FxDeviceDirect3D(FxApp* papp):FxDevice(papp) 
 {
     Type =Direct3D; 
     In2DView        = 0;
     WasInScene      = 0;
#if defined (GFC_OS_XBOX360)
    // For now initialize to 0.
    ViewportFactorX[0] = ViewportFactorX[1] = 
    ViewportFactorX[2] = ViewportFactorX[3] = 0.0f;
    ViewportFactorY[0] = ViewportFactorY[1] = 
    ViewportFactorY[2] = ViewportFactorY[3] = 0.0f;
#endif 
 }

void    FxDeviceDirect3D::Clear(unsigned int color)
{
    if (!GetD3DDevice())
        return;

    // D3D Clear relies on viewport, so change it if necessary to clear the entire buffer.
    viewport_type    vpSave;
    viewport_type    vp = { 0,0, pApp->GetWidth(), pApp->GetHeight(), 0.0f, 0.0f };
#if (GFC_D3D_VERSION == 10)
    if (!In2DView)
    {
        UInt n = 1;
        GetD3DDevice()->RSGetViewports(&n, &vpSave);
        GetD3DDevice()->RSSetViewports(1, &vp);
    }
    float rgba[4];
    Color32ToFloat(rgba, color);
    GetD3DDevice()->ClearRenderTargetView(((Direct3DWin32App*)pApp)->pRenderTarget, rgba);

    if (!In2DView)
        GetD3DDevice()->RSSetViewports(1, &vpSave);
#else
    if (!In2DView)
    {
        GetD3DDevice()->GetViewport(&vpSave);
        GetD3DDevice()->SetViewport(&vp);
    }
    GetD3DDevice()->Clear(0,0, D3DCLEAR_TARGET, color, 1.0f, 0);

    if (!In2DView)
        GetD3DDevice()->SetViewport(&vpSave);
#endif
}


// Initialize and restore 2D rendering view.
void    FxDeviceDirect3D::Push2DRenderView()
{
    if  (In2DView)
    {
        ::OutputDebugStringA("GFC Warning: Direct3DWin32App::Push2DRenderView failed - already in view.");
        return;
    }
    In2DView = 1;

    if (!GetD3DDevice())
        return;
    viewport_type    vp = { 0,0, pApp->GetWidth(), pApp->GetHeight(), 0.0f, 0.0f };
#if (GFC_D3D_VERSION == 10)
    UInt n = 1;
    GetD3DDevice()->RSGetViewports(&n, &ViewportSave);
    GetD3DDevice()->RSSetViewports(1, &vp);
#else
#if defined (GFC_OS_XBOX360)
    // Viewport unscale matrix
    ViewportFactorX[0] = 2.0f / (float) pApp->GetWidth();
    ViewportFactorY[1] = -2.0f / (float) pApp->GetHeight();
    ViewportFactorX[3] = -1.0f;
    ViewportFactorY[3] = 1.0f;
#endif

    GetD3DDevice()->GetViewport(&ViewportSave);
    GetD3DDevice()->SetViewport(&vp);

#if defined (GFC_OS_XBOX360)
    // Set constants
    GetD3DDevice()->SetVertexShaderConstantF(0, ViewportFactorX, 1);
    GetD3DDevice()->SetVertexShaderConstantF(1, ViewportFactorY, 1);
#endif

    // Begin scene if necessary.
    if (GetD3DDevice()->BeginScene() == D3DERR_INVALIDCALL)
        WasInScene = 1;
    else
        WasInScene = 0;
#endif
    // Set some states ? TBD.
}


void    FxDeviceDirect3D::Pop2DRenderView()
{
    if (!In2DView)
    {
        ::OutputDebugStringA("GFC Warning: Direct3DWin32App::Pop2DRenderView failed - not in view.");
        return;
    }
    In2DView = 0;

    // Restore viewport.
#if (GFC_D3D_VERSION == 10)
    GetD3DDevice()->RSSetViewports(1,&ViewportSave);
#else
    GetD3DDevice()->SetViewport(&ViewportSave);

    // End scene if we began it earlier.
    if (!WasInScene)
    {
        GetD3DDevice()->EndScene();
        WasInScene = 0;
    }
#endif
}

#if (GFC_D3D_VERSION != 10)
// Non-transformed vertex for a rectangle.
struct XYZWVertex
{
#ifdef GFC_OS_WIN32
    enum { FVF = D3DFVF_XYZRHW };
#endif
    float x, y, z, w;
    void    SetValue2D(float _x, float _y)  { x=_x; y=_y; z=0.0f; w=1.0f; }
    void    SetValue2D(int _x, int _y)      { x=(float)_x; y=(float)_y; z=0.0f; w=1.0f; }
};
#endif
// Draw a filled + blended rectangle.
void  FxDeviceDirect3D::FillRect(int l, int t, int r, int b, unsigned int color)
{
#if (GFC_D3D_VERSION != 10)
    if (!GetD3DDevice())
        return;

    XYZWVertex tv[4];
    // reverse 'Z' - pattern for strip
    tv[0].SetValue2D(r, t);
    tv[1].SetValue2D(l, t);
    tv[2].SetValue2D(r, b);
    tv[3].SetValue2D(l, b);

    GetD3DDevice()->SetTexture( 0, 0 );
#ifdef GFC_OS_XBOX360
    GetD3DDevice()->SetVertexShader(((Direct3DXboxApp*)pApp)->pVShaderCoordCopy);
    GetD3DDevice()->SetPixelShader(((Direct3DXboxApp*)pApp)->pPShaderConst);

    // Set color. This is compatible with our byte order def.
    float rgba[4] = 
    {
        (color & 0xFF)              / 255.0f, // Red
        ((color & 0xFF00) >> 8)     / 255.0f, // Green
        ((color & 0xFF0000) >> 16)  / 255.0f, // Blue
        ((color & 0xFF000000) >> 24)/ 255.0f, // Alpha
    };
    GetD3DDevice()->SetPixelShaderConstantF(0, rgba, 1);
#else //GFC_OS_XBOX360
#if (GFC_D3D_VERSION == 8)
    GetD3DDevice()->SetVertexShader(XYZWVertex::FVF);
#else
    GetD3DDevice()->SetFVF( XYZWVertex::FVF );
    GetD3DDevice()->SetVertexShader(0);
#endif

    // Standard blending
    GetD3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    GetD3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    GetD3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    // Use factor
    GetD3DDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    GetD3DDevice()->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    GetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    GetD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
    GetD3DDevice()->SetRenderState(D3DRS_TEXTUREFACTOR, color);

    GetD3DDevice()->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    GetD3DDevice()->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    GetD3DDevice()->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP,  2, tv, sizeof(XYZWVertex) );
#endif
#else
    GUNUSED4(l,t,r,b);
    GUNUSED(color);
#endif
}

void FxDeviceDirect3D::SetWireframe(bool wireframeEnable)
{   
    if (!GetD3DDevice())
        return;
#if (GFC_D3D_VERSION != 10)
    if (wireframeEnable)
        GetD3DDevice()->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    else
        GetD3DDevice()->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
#else
    GUNUSED(wireframeEnable);
#endif
}

bool FxDeviceDirect3D::InitRenderer()
{
    return true;
}
void FxDeviceDirect3D::PrepareRendererForFrame()
{
}
