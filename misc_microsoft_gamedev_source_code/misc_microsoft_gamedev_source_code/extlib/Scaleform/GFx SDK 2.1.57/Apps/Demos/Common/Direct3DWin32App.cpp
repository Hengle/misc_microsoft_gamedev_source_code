/**********************************************************************

Filename    :   Direct3DWin32App.cpp
Content     :   Simple Direct3D Win32 Application class implemenation
Created     :   January 10, 2008
Authors     :   Michael Antonov

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "Direct3DWin32App.h"
#include "FxDeviceDirect3D.h"
//#define GFC_D3D_VERSION 10



// Define an earlier SDK version so that the
// app will be more likely to run Direct3D 9b.
#ifndef GFC_D3D_VERSION
#define GFC_D3D_VERSION 9
#endif

#if (GFC_D3D_VERSION == 8)
#ifdef D3D_DEBUG_INFO
    #define DIRECT3DAPP_D3D_SDK_VERSION     (220 | 0x80000000)
#else
    #define DIRECT3DAPP_D3D_SDK_VERSION     220
#endif
// Used for functions that have an extra last argument of NULL in D3D9 only
#define NULL9
#elif (GFC_D3D_VERSION == 9)
#ifdef D3D_DEBUG_INFO
    #define DIRECT3DAPP_D3D_SDK_VERSION     (31 | 0x80000000)
#else
    #define DIRECT3DAPP_D3D_SDK_VERSION     31
#endif
// Used for functions that have an extra last argument of NULL in D3D9 only
#define NULL9                   , NULL
#endif

// Video mode (D3DFMT_X8R8G8B8, D3DFMT_R5G6B5.. )
#if (GFC_D3D_VERSION == 10)
    #define  APP_COLOR_FORMAT           DXGI_FORMAT_R8G8B8A8_UNORM
    #define  APP_DEPTHSTENCIL_FORMAT 
#else
    #define  APP_COLOR_FORMAT           D3DFMT_UNKNOWN
    #define  APP_DEPTHSTENCIL_FORMAT    D3DFMT_D24S8
#endif

bool Direct3DWin32App::CreateRenderer()
{
    pRenderer = *renderer_type::CreateRenderer();
    SetRendererDependentVideoMode();
    return pRenderer.GetPtr() != NULL;
}

void Direct3DWin32App::SetRendererDependentVideoMode()
{
    if (!pRenderer)
        return;
#if (GFC_D3D_VERSION == 8) || (GFC_D3D_VERSION == 9)
    #if defined(GFC_OS_XBOX360)
        pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 
            PredicatedTiling ? renderer_type::VMConfig_SupportTiling : 0, hWND);
    #else
        pRenderer->SetDependentVideoMode(pDevice, &PresentParams, VMCFlags, hWND);
    #endif
#else
    VMCFlags = (VMCFlags & ~GRendererD3D10::VMConfig_Multisample) | 
        (FSAntialias ? GRendererD3D10::VMConfig_Multisample : 0);
    pRenderer->SetDependentVideoMode(pDevice, VMCFlags);
#endif
}

FxApp::DisplayStatus  Direct3DWin32App::CheckDisplayStatus() const
{
    if (!pRenderer)
        return FxApp::DisplayStatus_Unavailable;
    return (FxApp::DisplayStatus)pRenderer->CheckDisplayStatus();
}

Direct3DWin32App::Direct3DWin32App() : FxWin32App() 
{
    pFxDevice = new FxDeviceDirect3D(this);
#if (GFC_D3D_VERSION == 8)
    pD3D        = Direct3DCreate8( DIRECT3DAPP_D3D_SDK_VERSION );
#elif (GFC_D3D_VERSION == 9)
    pD3D        = Direct3DCreate9( DIRECT3DAPP_D3D_SDK_VERSION );
#endif
    pDevice         = 0;
#if (GFC_D3D_VERSION == 10)
    pRenderTarget    = 0;
    pDxgi            = 0;
    pDevice          = 0;
    pSwapChain       = 0;
    pDepthStencil    = 0;
    pDepthStencilBuf = 0;
    // Set later on in CreateWindow
    FSAASupported    = 0;
    FSAASamples      = 4;

#else
    pTextFont       = 0;
    pTextFontSprite = 0;
    if (!pD3D)
        ::MessageBoxA(NULL,"Unable to create Direct3D interface", "Direct3D Error",
                     MB_OK | MB_ICONEXCLAMATION);
#endif
}

Direct3DWin32App::~Direct3DWin32App()
{
    if (pRenderer)
        pRenderer->ReleaseResources();
#if (GFC_D3D_VERSION != 10)
    if (pD3D)
        pD3D->Release();
#endif
}

void Direct3DWin32App::SwitchFullScreenMode()
{
    if (pRenderer)
        pRenderer->ResetVideoMode();

restore_video_mode:
    FullScreen = !FullScreen;

    SInt x = OldWindowX, y = OldWindowY;
    SInt w = OldWindowWidth, h = OldWindowHeight;

    if (FullScreen)
    {
        // Save window size & location
        RECT r;
        ::GetWindowRect(hWND, &r);
        OldWindowWidth = r.right - r.left;
        OldWindowHeight = r.bottom - r.top;
        OldWindowX = r.left;
        OldWindowY = r.top;
        // New location for full-screen
        x = y = 0;
        w = 1024; h = 768;
    }

    if (!w || !h)
    {
        w = 1024; h = 768;
    }

    if (!ConfigureWindow(x, y, w, h, FullScreen))
        goto restore_video_mode;

#if (GFC_D3D_VERSION == 8) || (GFC_D3D_VERSION == 9)
    if (FSAntialias && FSAASupported)
    {
        PresentParams.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
        PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD; // Discard required
    }
    else
    {
        PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
        PresentParams.SwapEffect = FullScreen ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
    }
#endif
    SetRendererDependentVideoMode();
}

void Direct3DWin32App::SwitchFSAA(bool on_off)
{
    if (pRenderer && FSAASupported && FSAntialias != on_off)
    {
        FSAntialias = on_off;
#if (GFC_D3D_VERSION == 8) || (GFC_D3D_VERSION == 9)
        if (FSAntialias)
        {
            PresentParams.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
            PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD; // Discard required
        }
        else
        {
            PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
            PresentParams.SwapEffect = FullScreen ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
        }
        pRenderer->ResetVideoMode();
        RecreateRenderer();
#else
        ChangeFSAA();
        VMCFlags = (VMCFlags & ~GRendererD3D10::VMConfig_Multisample)
            | (FSAntialias ? GRendererD3D10::VMConfig_Multisample : 0);
        pRenderer->ResetVideoMode();
#endif
        SetRendererDependentVideoMode();
    }

}

void    Direct3DWin32App::SetVSync(bool isEnabled)
{
#if (GFC_D3D_VERSION == 9)
    PresentParams.PresentationInterval = (isEnabled ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
    RecreateRenderer();
    VSync = isEnabled;
#else
    GUNUSED(isEnabled);
#endif
}
// Create and show the window
bool Direct3DWin32App::SetupWindowDevice()
{
#if (GFC_D3D_VERSION == 10)
    UInt32 flags = 0;
#ifdef GFC_BUILD_DEBUG
    flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

    memset(&SwapChainDesc, 0, sizeof(SwapChainDesc));
    SwapChainDesc.BufferCount = 1;
    SwapChainDesc.BufferDesc.Width = Width;
    SwapChainDesc.BufferDesc.Height = Height;
    SwapChainDesc.BufferDesc.Format = APP_COLOR_FORMAT;
    SwapChainDesc.BufferDesc.RefreshRate.Numerator = 85;
    SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.OutputWindow = hWND;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.Windowed = !FullScreen;

    HRESULT hr = D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, flags, D3D10_SDK_VERSION, &pDevice);
    if (FAILED(hr))
        return 0;

    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pDxgi);
    if (FAILED(hr))
        return 0;

    if (SUCCEEDED(pDevice->CheckMultisampleQualityLevels(APP_COLOR_FORMAT, FSAASamples, &FSAAQuality)) && FSAAQuality > 0)
        FSAASupported = 1;

    if (FSAASupported && FSAntialias)
    {
        SwapChainDesc.SampleDesc.Count = FSAASamples;
        SwapChainDesc.SampleDesc.Quality = FSAAQuality-1;
    }
    hr = pDxgi->CreateSwapChain(pDevice, &SwapChainDesc, &pSwapChain);
    if( FAILED(hr ) )
        return 0;

    if (FullScreen)
        ConfigureWindow(0, 0, Width, Height, 1, 0);
    else
        CreateBuffers();
#else //(GFC_D3D_VERSION == 10)
    if (!pD3D)
        return 0;
 
    D3DFORMAT format = FullScreen ?
            ((APP_COLOR_FORMAT == D3DFMT_UNKNOWN) ? D3DFMT_X8R8G8B8 : APP_COLOR_FORMAT) : APP_COLOR_FORMAT;

    if (BitDepth)
    {
        // Use 16-bit bit depth if allowed
        if (BitDepth == 16)
        {
            if (pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                        D3DFMT_R5G6B5, D3DFMT_R5G6B5, !FullScreen) == D3D_OK)
                format = D3DFMT_R5G6B5;
            else if (pD3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                                        D3DFMT_X1R5G5B5, D3DFMT_X1R5G5B5, !FullScreen) == D3D_OK)
                format = D3DFMT_X1R5G5B5;
        }
    }

    if (format == D3DFMT_UNKNOWN)
    {
        D3DDISPLAYMODE mode;
        pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);

        format = mode.Format;
    }

    // Set up the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS &d3dpp = PresentParams;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed                  = !FullScreen;
    d3dpp.BackBufferFormat          = format;
    d3dpp.EnableAutoDepthStencil    = 1;
    d3dpp.AutoDepthStencilFormat    = APP_DEPTHSTENCIL_FORMAT;
    d3dpp.BackBufferHeight          = GetHeight();
    d3dpp.BackBufferWidth           = GetWidth();
#if (GFC_D3D_VERSION == 9)
    d3dpp.PresentationInterval      = (VSync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
#endif

    // Enable multisampling if it is supported
    if (SUCCEEDED(pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                        ((format == D3DFMT_UNKNOWN) ? D3DFMT_X8R8G8B8 : format),
                        !FullScreen, D3DMULTISAMPLE_4_SAMPLES NULL9)))
        FSAASupported = 1;

    if (FSAntialias && FSAASupported)
    {
        d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
    }

    // Discard required for FSAA
    d3dpp.SwapEffect                = (FullScreen || (FSAntialias && FSAASupported)) ?
                                     D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;

    // Support both SW and HW vertex processing.
#if (GFC_D3D_VERSION == 8)
    D3DCAPS8    caps;
#else
    D3DCAPS9    caps;
#endif

    pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    DWORD   vp = ((caps.VertexShaderVersion < D3DVS_VERSION(1,1)) || !(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
                    ? D3DCREATE_SOFTWARE_VERTEXPROCESSING : D3DCREATE_HARDWARE_VERTEXPROCESSING;

    // Create the D3DDevice
    HRESULT hr = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWND,
        vp, &d3dpp, &pDevice );
    
    if( FAILED(hr ) )
    {
        ::MessageBoxA(NULL,
            "Unable to create Direct3D Device", "Direct3DApp Error",MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

#if (GFC_D3D_VERSION == 9)
    // Create a font.
    D3DXCreateFontA(pDevice, 18,0, 800, 1, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FIXED_PITCH | FF_DONTCARE, "Courier New", &pTextFont);
    if (pTextFont)
        pTextFont->PreloadCharacters(31, 128);
    D3DXCreateSprite(pDevice, &pTextFontSprite);
#endif
#endif // else (GFC_D3D_VERSION == 10)
    return 1;
}


// Resets the direct3D, return 1 if successful.
// On successful reset, this function will call InitRenderer again.
bool    Direct3DWin32App::RecreateRenderer()
{
    if (!pDevice)
        return 0;
#if (GFC_D3D_VERSION != 10)
    if (pTextFont)
        pTextFont->OnLostDevice();
    if (pTextFontSprite)
        pTextFontSprite->OnLostDevice();

    if(FAILED(pDevice->Reset(&PresentParams)))
        return 0;

    if (pTextFont)
        pTextFont->OnResetDevice();
    if (pTextFontSprite)
        pTextFontSprite->OnResetDevice();
#endif
    return InitRenderer();
}

// Deletes the DC, RC, and Window, and restores the original display.
void Direct3DWin32App::KillWindowDevice()
{
#if (GFC_D3D_VERSION == 10)
    if (pSwapChain)
        pSwapChain->SetFullscreenState(0, NULL);
    ReleaseBuffers();
    if (pSwapChain)
        pSwapChain->Release();
    if (pDxgi)
        pDxgi->Release();
    pSwapChain = 0;
    pDevice = 0;
    pDxgi = 0;
#else
    if (pTextFont)
    {
        pTextFont->Release();
        pTextFont = 0;
    }
    if (pTextFontSprite)
    {
        pTextFontSprite->Release();
        pTextFontSprite = 0;
    }
#endif

    if (pDevice)
        pDevice->Release();

}

GRenderer* Direct3DWin32App::GetRenderer()
{
    return pRenderer.GetPtr();
}


// Draw a text string (specify top-left corner of characters, NOT baseline)
void    Direct3DWin32App::DrawText(int x, int y, const char *ptext, unsigned int color)
{
#if (GFC_D3D_VERSION == 9)
    if (!ptext || ptext[0] == 0)
        return;
    if (!pTextFont || !pTextFontSprite)
        return;

    // Draw text (we don't need to specify width/height since we use NOCLIP).
    RECT r = { x,y, 0,0 };
    pTextFontSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_DONOTSAVESTATE);
    pTextFont->DrawTextA(pTextFontSprite, ptext, -1, &r, DT_LEFT| DT_TOP |DT_NOCLIP, color);
    pTextFontSprite->End();
#else
    GUNUSED4(x,y,ptext,color);
#endif
}

void    Direct3DWin32App::CalcDrawTextSize(SInt *pw, SInt *ph, const char *ptext)
{
#if (GFC_D3D_VERSION == 9)
    if (!ptext || ptext[0] == 0)
    {
        *pw = *ph = 0;
        return;
    }

    // Compute text size.
    RECT r = { 0,0, 0,0 };
    pTextFont->DrawTextA(pTextFontSprite, ptext, -1, &r, DT_LEFT| DT_TOP | DT_CALCRECT, 0);
    *pw = r.right;
    *ph = r.bottom;
#else
    GUNUSED3(pw,ph,ptext);
#endif
}


// Presents the data (swaps the back and front buffers)
void    Direct3DWin32App::PresentFrame()
{
#if (GFC_D3D_VERSION == 10)
    if (pSwapChain)
        pSwapChain->Present(0, 0);
#else
    if (pDevice)
        pDevice->Present( NULL, NULL, NULL, NULL );
#endif
}


// Sizing; by default, re-initalizes the renderer
void    Direct3DWin32App::ResizeWindow(int w, int h)
{
    if (pRenderer)
        pRenderer->ResetVideoMode();

    Width  = w;
    Height = h;

    // Minimized/zero size? Do nothing.
    if (pDevice && (Width||Height))
    {

#if (GFC_D3D_VERSION == 10)
        ReleaseBuffers();
        pSwapChain->ResizeBuffers(2, Width, Height, APP_COLOR_FORMAT, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
        CreateBuffers();
#else
        PresentParams.BackBufferHeight  = Height;
        PresentParams.BackBufferWidth   = Width;

        // Avoid debug breakpoints: do not reset device unless it is ready.
        if (pDevice->TestCooperativeLevel() == D3DERR_DEVICELOST)
            return;

        if (pTextFont)
            pTextFont->OnLostDevice();
        if (pTextFontSprite)
            pTextFontSprite->OnLostDevice();

        if(FAILED(pDevice->Reset(&PresentParams)))
            return;

        if (pTextFont)
            pTextFont->OnResetDevice();
        if (pTextFontSprite)
            pTextFontSprite->OnResetDevice();
#endif
    }
    SetRendererDependentVideoMode();
}

#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#undef new
#endif

// Returns 1 if recreate succeeded, 0 if it failed. If reset is not specified, function will never fail.
bool    Direct3DWin32App::ConfigureWindow(SInt x, SInt y, SInt w, SInt h, bool fullScreen, bool recreateRenderer)
{
    // Change the settings.
    FullScreen  = fullScreen;
    Width       = w;
    Height      = h;
#if (GFC_D3D_VERSION != 10)
    PresentParams.Windowed          = !FullScreen;
    PresentParams.BackBufferFormat  = FullScreen ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;
    PresentParams.BackBufferHeight  = Height;
    PresentParams.BackBufferWidth   = Width;
#endif

#if (GFC_D3D_VERSION == 8)
    if (PresentParams.BackBufferFormat == D3DFMT_UNKNOWN)
    {
        D3DDISPLAYMODE mode;
        pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);

        PresentParams.BackBufferFormat = mode.Format;
    }
#endif

    // New window location & size.
    LockOnSize = 1; // Avoid OnSize() calls.
    DWORD style = WS_VISIBLE |
            (FullScreen ? WS_POPUP :
            (SizableWindow ? WS_OVERLAPPEDWINDOW : WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU));
    if (::GetWindowLong(hWND, GWL_STYLE) != (LONG)style)
        ::SetWindowLong(hWND, GWL_STYLE, style);

    RECT wr = { x, y, x+w, y+h };
    ::AdjustWindowRect(&wr, style, 0);
    ::SetWindowPos(hWND, FullScreen ? HWND_TOPMOST : HWND_NOTOPMOST,
                   x,y, wr.right - wr.left, wr.bottom - wr.top, 0);

#if (GFC_D3D_VERSION == 10)
    DXGI_MODE_DESC mode;

    if (FullScreen)
    {
        IDXGIOutput *pDisplay;
        pSwapChain->GetContainingOutput(&pDisplay);
        UInt n = 0;
        pDisplay->GetDisplayModeList(APP_COLOR_FORMAT, 0, &n, NULL);
        DXGI_MODE_DESC *modes = new DXGI_MODE_DESC[n];
        pDisplay->GetDisplayModeList(APP_COLOR_FORMAT, 0, &n, modes);
        UInt modei = ~0u;

        for (UInt i = 0; i < n; i++)
        {
            if (modes[i].Width == UInt(w) && modes[i].Height == UInt(h) && 
                (modei == ~0u || float(modes[i].RefreshRate.Numerator)/float(modes[i].RefreshRate.Denominator) >
                float(modes[modei].RefreshRate.Numerator)/float(modes[modei].RefreshRate.Denominator)))
                modei = i;
        }
        pDisplay->Release();
        mode = modes[modei];
        delete[] modes;

        if (modei == ~0)
            return 0;
    }
    else
    {
        mode.Width = w;
        mode.Height = h;
        mode.RefreshRate.Numerator = 85;
        mode.RefreshRate.Denominator = 1;
        mode.Format = APP_COLOR_FORMAT;
        mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        mode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    }

    ReleaseBuffers();

    HRESULT hr;
    hr = pSwapChain->ResizeBuffers(2, Width, Height, APP_COLOR_FORMAT, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (FAILED(hr))
        return 0;
    CreateBuffers();
    pSwapChain->SetFullscreenState(FullScreen, NULL);
    hr = pSwapChain->ResizeTarget(&mode);
    if (FAILED(hr))
        return 0;
#endif
    LockOnSize = 0;
    // And recreate the renderer if necessary.
    return recreateRenderer ? RecreateRenderer() : 1;
}
#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif

#if (GFC_D3D_VERSION == 10)
bool    Direct3DWin32App::CreateBuffers()
{
    ID3D10Texture2D* pBackBuffer;
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&pBackBuffer);
    if( FAILED(hr) )
        return 0;

    D3D10_RENDER_TARGET_VIEW_DESC rtv;
    rtv.Texture2D.MipSlice = 0;
    rtv.Format = APP_COLOR_FORMAT;
    rtv.ViewDimension = SwapChainDesc.SampleDesc.Quality ? D3D10_RTV_DIMENSION_TEXTURE2DMS : D3D10_RTV_DIMENSION_TEXTURE2D;
    hr = pDevice->CreateRenderTargetView(pBackBuffer, &rtv, &pRenderTarget);
    pBackBuffer->Release();
    if( FAILED(hr) )
        return 0;

    D3D10_TEXTURE2D_DESC dsd;
    dsd.Width = Width;
    dsd.Height = Height;
    dsd.MipLevels = 1;
    dsd.ArraySize = 1;
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsd.SampleDesc.Count = SwapChainDesc.SampleDesc.Count;
    dsd.SampleDesc.Quality = SwapChainDesc.SampleDesc.Quality;
    dsd.Usage = D3D10_USAGE_DEFAULT;
    dsd.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    dsd.CPUAccessFlags = 0;
    dsd.MiscFlags = 0;
    hr = pDevice->CreateTexture2D(&dsd, NULL, &pDepthStencilBuf);
    if( FAILED(hr) )
        return 0;

    D3D10_DEPTH_STENCIL_VIEW_DESC dsv;
    dsv.Format = dsd.Format;
    dsv.ViewDimension = SwapChainDesc.SampleDesc.Quality ? D3D10_DSV_DIMENSION_TEXTURE2DMS : D3D10_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    hr = pDevice->CreateDepthStencilView( pDepthStencilBuf, &dsv, &pDepthStencil);
    if( FAILED(hr) )
        return 0;

    pDevice->ClearDepthStencilView(pDepthStencil, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 0, 0);
    pDevice->OMSetRenderTargets(1, &pRenderTarget, pDepthStencil);

    // Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = Width;
    vp.Height = Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pDevice->RSSetViewports(1, &vp);

    return 1;
}

void Direct3DWin32App::ReleaseBuffers()
{
    ID3D10RenderTargetView *rt = 0;
    pDevice->OMSetRenderTargets(1, &rt, 0);

    if (pDepthStencilBuf)
        pDepthStencilBuf->Release();
    if (pDepthStencil)
        pDepthStencil->Release();
    if (pRenderTarget)
        pRenderTarget->Release();

    pDepthStencil = 0;
    pDepthStencilBuf = 0;
    pRenderTarget = 0;
}
void Direct3DWin32App::ChangeFSAA()
{
    if (!FSAASupported || !pSwapChain)
        return;

    LockOnSize = 1;
    pSwapChain->SetFullscreenState(0, NULL);
    ReleaseBuffers();
    pSwapChain->Release();
    pSwapChain = 0;

    if (FSAntialias)
    {
        SwapChainDesc.SampleDesc.Count = FSAASamples;
        SwapChainDesc.SampleDesc.Quality = FSAAQuality-1;
    }
    else
    {
        SwapChainDesc.SampleDesc.Count = 1;
        SwapChainDesc.SampleDesc.Quality = 0;
    }

    SwapChainDesc.BufferDesc.Width = Width;
    SwapChainDesc.BufferDesc.Height = Height;
    SwapChainDesc.Windowed = 1;
    if (FAILED(pDxgi->CreateSwapChain(pDevice, &SwapChainDesc, &pSwapChain)))
        return;
    if (FullScreen)
        ConfigureWindow(0, 0, Width, Height, 1, 0);
    else
        CreateBuffers();
    LockOnSize = 0;
}
#endif //GFC_D3D_VERSION == 10
