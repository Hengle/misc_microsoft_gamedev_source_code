/**********************************************************************

Filename    :   Direct3DWin32App.cpp
Content     :   Simple Direct3D Win32 Application class implemenation
Created     :   
Authors     :   Michael Antonov
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "Direct3DWin32App.h"
#include "GStd.h"


// Video mode (D3DFMT_X8R8G8B8, D3DFMT_R5G6B5.. )
#define  APP_COLOR_FORMAT           D3DFMT_UNKNOWN
#define  APP_DEPTHSTENCIL_FORMAT    D3DFMT_D24S8

#define  WWND_CLASS_NAME            L"Direct3D_Window_Class"

// Define an earlier SDK version so that the 
// app will be more likely to run Direct3D 9b.
#ifdef D3D_DEBUG_INFO
    #define DIRECT3DAPP_D3D_SDK_VERSION     (31 | 0x80000000)
#else
    #define DIRECT3DAPP_D3D_SDK_VERSION     31
#endif 


Direct3DWin32App::Direct3DWin32App()
{
    Created     = 0;
    Active      = 0;
    QuitFlag    = 0;
    LockOnSize  = 0;
    ExitCode    = 0;

    // Cursor variables
    CursorHidden  = 0;
    CursorHiddenSaved = 0;
    CursorIsOutOfClientArea = 0;
    CursorDisabled = 0;

    // Requested 3D state   
    FullScreen  = 0;
    FSAntialias = 0;
    BitDepth    = 0;

    SupportDropFiles    = 0;
    SizableWindow = 0;

    hWND        = 0;
    hInstance   = 0;

    Width       = 0;
    Height      = 0;

    // Clear GL function pointers
    pD3D        = Direct3DCreate9( DIRECT3DAPP_D3D_SDK_VERSION );
    pDevice     = 0;
    // Set later on in CreateWindow
    FSAASupported= 0;

    In2DView        = 0;
    WasInScene      = 0;
    pTextFont       = 0;
    pTextFontSprite = 0;
    
    if (!pD3D)
        ::MessageBox(NULL,"Unable to create Direct3D9 interface", "Direct3D Error", 
                     MB_OK | MB_ICONEXCLAMATION);
}

Direct3DWin32App::~Direct3DWin32App()
{
    if (Created)
        KillWindow();

    if (pD3D)
        pD3D->Release();
}


// Global callback function to be called on window create
LRESULT CALLBACK Direct3DGeneralWindowProc(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam)
{
    Direct3DWin32App    *papp;  
    
    // The first message to ever come in sets the long value to class pointer
    if (iMsg==WM_NCCREATE)
    {
        papp = (Direct3DWin32App*) ((LPCREATESTRUCT)lParam)->lpCreateParams;
		
        if (!papp)
            return DefWindowProcW(hwnd,iMsg,wParam,lParam);        
#ifdef GFC_64BIT_POINTERS
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)papp);
#else
        SetWindowLong(hwnd, 0, (LONG)(size_t)papp);
#endif
        papp->hWND = hwnd;
    }
    
	// use size_t to quiet /Wp64 warning
    if ( (papp=((Direct3DWin32App*)(size_t)GetWindowLongPtr(hwnd,0)))==0 )
        return DefWindowProcW(hwnd,iMsg,wParam,lParam);

    // Call member
    return papp->MemberWndProc(iMsg, wParam, lParam);
}



// Create and show the window
bool Direct3DWin32App::SetupWindow(const char *pname, SInt width, SInt height)
{
    if (!pD3D)
        return 0;
    if (Created)
        return 0;
    
    hInstance = GetModuleHandle(NULL);
    Width     = width;
    Height    = height;

    // Initialize the window class structure
    WNDCLASSEXW  wc; 
    
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc    = Direct3DGeneralWindowProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(Direct3DWin32App*);  // will need to store class pointer here
    wc.hInstance      = hInstance;
    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);    // default icon
    wc.hIconSm        = LoadIcon(NULL, IDI_WINLOGO);        // windows logo small icon
    // set hCursor to NULL; otherwise cursor will be reverted back each time mouse moved.
    wc.hCursor        = NULL; //LoadCursor(NULL, IDC_ARROW);        // default arrow
    wc.hbrBackground  = NULL;     // no background needed
    wc.lpszMenuName   = NULL;     // no menu
    wc.lpszClassName  = WWND_CLASS_NAME;
    hCursor = LoadCursor(NULL, IDC_ARROW);
    
    // Register the windows class
    if (!RegisterClassExW(&wc))
    {
        MessageBox(NULL,"Unable to register the window class", "Direct3DApp Error", 
                   MB_OK | MB_ICONEXCLAMATION);    
        Width = Height = 0;
        return 0;
    }
    
    DWORD dwExStyle;
    DWORD dwStyle;
    int   xpos = 0, ypos = 0;
    
    // Set the window style depending on FullScreen state
    if (FullScreen)
    {
        dwExStyle   = WS_EX_APPWINDOW;
        dwStyle     = WS_POPUP;   // FullScreen gets no borders or title bar
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

        if (SizableWindow)
            dwStyle = WS_OVERLAPPEDWINDOW;
        else
            dwStyle = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
        
        xpos = ypos = CW_USEDEFAULT;
    }

    if (CursorDisabled)
        ShowCursorInstantly(false);
    
    // Initalize the rendering window to have width & height match the client area  
    RECT  windowRect;
    windowRect.left     = 0;
    windowRect.right    = (LONG) Width;
    windowRect.top      = 0;
    windowRect.bottom   = (LONG) Height;    
    // Account for borders and other style options
    AdjustWindowRectEx(&windowRect, dwStyle, 0, dwExStyle);

    wchar_t wpname[256];
    if (::MultiByteToWideChar(CP_ACP, 0, pname, (int)gfc_strlen(pname) + 1, wpname, sizeof(wpname)) == 0)
        wpname[0] = 0;

    // Create our window
    hWND = CreateWindowExW(
            dwExStyle,
            WWND_CLASS_NAME,
            wpname,              // Window name
            dwStyle | 
            WS_CLIPCHILDREN |
            WS_CLIPSIBLINGS,
            xpos, ypos,
            windowRect.right - windowRect.left, // width
            windowRect.bottom - windowRect.top, // height
            NULL,               // hParent
            NULL,               // hMenu
            hInstance,          // Application instance
            (LPVOID) this );    // Our pointer as an extra parameter
        
    if (!hWND)
    {
        MessageBox(NULL, "Unable to create window", "Direct3DApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    // Check the client rect was changed by Windows (if, for example, too big window
    // was created).
    RECT clientRect;
    ::GetClientRect(hWND, &clientRect);
    if (clientRect.right - clientRect.left != width ||
        clientRect.bottom - clientRect.top != height)
    {
        // Calc new Width & Height
        Width = clientRect.right - clientRect.left;
        Height = clientRect.bottom - clientRect.top;
    }


    if (SupportDropFiles)       
        ::DragAcceptFiles(hWND, 1); 


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

    // Set up the structure used to create the D3DDevice
    D3DPRESENT_PARAMETERS &d3dpp = PresentParams;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed                  = !FullScreen;    
    d3dpp.BackBufferFormat          = format;
    d3dpp.EnableAutoDepthStencil    = 1;
    d3dpp.AutoDepthStencilFormat    = APP_DEPTHSTENCIL_FORMAT;
    d3dpp.BackBufferHeight          = Height;
    d3dpp.BackBufferWidth           = Width;
    d3dpp.PresentationInterval      = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Enable multisampling if it is supported
    if (SUCCEEDED(pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                        ((format == D3DFMT_UNKNOWN) ? D3DFMT_X8R8G8B8 : format),
                        !FullScreen, D3DMULTISAMPLE_4_SAMPLES, NULL)))  
        FSAASupported = 1;

    if (FSAntialias && FSAASupported)
    {
        d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;       
    }

    // Discard required for FSAA
    d3dpp.SwapEffect                = (FullScreen || (FSAntialias && FSAASupported)) ?
                                     D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;

    // Support both SW and HW vertex processing.
    D3DCAPS9    caps;
    pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    DWORD   vp = ((caps.VertexShaderVersion < D3DVS_VERSION(1,1)) || !(caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
                    ? D3DCREATE_SOFTWARE_VERTEXPROCESSING : D3DCREATE_HARDWARE_VERTEXPROCESSING;
    
    // Create the D3DDevice
    if( FAILED(pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWND,
                                   vp, &d3dpp, &pDevice ) ) )
    {
        ::MessageBox(NULL, 
            "Unable to create Direct3D Device", "Direct3DApp Error",MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    // show the window in the foreground, and set the keyboard focus to it
    ShowWindow(hWND, SW_SHOW);
    SetForegroundWindow(hWND);
    SetFocus(hWND);

    
    // Create a font.
    D3DXCreateFont(pDevice, 18,0, 800, 1, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                FIXED_PITCH | FF_DONTCARE, "Courier New", &pTextFont);
    if (pTextFont)
        pTextFont->PreloadCharacters(31, 128);
    D3DXCreateSprite(pDevice, &pTextFontSprite);

    
    Created = 1;
    if (!InitRenderer())
    {       
        KillWindow();
        return 0;
    }       
    
    return 1;
}


// Resets the direct3D, return 1 if successful.
// On successful reset, this function will call InitRenderer again.
bool    Direct3DWin32App::RecreateRenderer()
{   
    if (!pDevice)
        return 0;

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

    return InitRenderer();
}

// Deletes the DC, RC, and Window, and restores the original display.
void Direct3DWin32App::KillWindow()
{
    if (!Created)
        return;

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

    if (pDevice)
        pDevice->Release();
    
    // Destroy the window
    if (hWND)       
    {
        DestroyWindow(hWND);
        hWND = NULL;
    }
    
    // Unregister our class to make name reusable
    UnregisterClassW(WWND_CLASS_NAME, hInstance);
    hInstance = NULL;

    Created = 0;
    return;
}




void    Direct3DWin32App::Clear(UInt32 color)
{   
    if (!pDevice)
        return;

    // D3D Clear relies on viewport, so change it if necessary to clear the entire buffer.
    D3DVIEWPORT9    vpSave;
    D3DVIEWPORT9    vp = { 0,0, Width, Height, 0.0f, 0.0f };

    if (!In2DView)
    {
        pDevice->GetViewport(&vpSave);
        pDevice->SetViewport(&vp);
    }       
    pDevice->Clear(0,0, D3DCLEAR_TARGET, color, 1.0f, 0);

    if (!In2DView)
        pDevice->SetViewport(&vpSave);  
}

// Initialize and restore 2D rendering view.
void    Direct3DWin32App::Push2DRenderView()
{   
    if  (In2DView)
    {
        ::OutputDebugString("GFC Warning: Direct3DWin32App::Push2DRenderView failed - already in view.");
        return;
    }
    In2DView = 1;

    if (!pDevice)
        return;

    // Save viewport.
    pDevice->GetViewport(&ViewportSave);

    // Set new viewport.
    D3DVIEWPORT9    vp = { 0,0, Width, Height, 0.0f, 0.0f };
    pDevice->SetViewport(&vp);

    // Begin scene if necessary.
    if (pDevice->BeginScene() == D3DERR_INVALIDCALL)
        WasInScene = 1;
    else
        WasInScene = 0;

    // Set some states ? TBD.
}


void    Direct3DWin32App::Pop2DRenderView()
{
    if (!In2DView)
    {
        ::OutputDebugString("GFC Warning: Direct3DWin32App::Pop2DRenderView failed - not in view.");
        return;
    }
    In2DView = 0;

    // Restore viewport.
    pDevice->SetViewport(&ViewportSave);

    // End scene if we began it earlier.
    if (!WasInScene)
    {
        pDevice->EndScene();
        WasInScene = 0;
    }
}


// Non-transformed vertex for a rectangle.
struct XYZWVertex
{
    enum { FVF = D3DFVF_XYZRHW };

    float x, y, z, w;
    void    SetValue2D(float _x, float _y)  { x=_x; y=_y; z=0.0f; w=1.0f; }
    void    SetValue2D(int _x, int _y)      { x=(float)_x; y=(float)_y; z=0.0f; w=1.0f; }
};

// Draw a filled + blended rectangle.
void    Direct3DWin32App::FillRect(SInt l, SInt t, SInt r, SInt b, UInt32 color)
{
    if (!pDevice)
        return;

    XYZWVertex tv[4];
    // reverse 'Z' - pattern for strip
    tv[0].SetValue2D(r, t);
    tv[1].SetValue2D(l, t);
    tv[2].SetValue2D(r, b);
    tv[3].SetValue2D(l, b);
    
    pDevice->SetTexture( 0, 0 );
    pDevice->SetFVF( XYZWVertex::FVF );

    // Standard blending
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    // Use factor
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);

    pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    pDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP,  2, tv, sizeof(XYZWVertex) );
}


// Draw a text string (specify top-left corner of characters, NOT baseline)
void    Direct3DWin32App::DrawText(SInt x, SInt y, const char *ptext, UInt32 color)
{
    if (!ptext || ptext[0] == 0)
        return;
    if (!pTextFont || !pTextFontSprite)
        return;

    // Draw text (we don't need to specify width/height since we use NOCLIP).
    RECT r = { x,y, 0,0 };
    pTextFontSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_DONOTSAVESTATE);
    pTextFont->DrawTextA(pTextFontSprite, ptext, -1, &r, DT_LEFT| DT_TOP |DT_NOCLIP, color);
    pTextFontSprite->End();
}

void    Direct3DWin32App::CalcDrawTextSize(SInt *pw, SInt *ph, const char *ptext)
{   
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
}


// API-independednt toggle for wireframe rendering.
void    Direct3DWin32App::SetWireframe(bool wireframeEnable)
{
    if (!pDevice)
        return;
    if (wireframeEnable)
        pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
    else
        pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
}


// Message processing function to be called in the 
// application loops until this returns 0.
bool    Direct3DWin32App::ProcessMessages()
{
    MSG msg;

    if(PeekMessage(&msg, 0, NULL, NULL, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // On WM_QUIT message, quit the application by setting Quit flag
            ExitCode = (SInt)msg.wParam;
            QuitFlag = 1;           
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return !QuitFlag;       
}

// Sleeps for the specified number of milliseconds or till message.
void    Direct3DWin32App::SleepTillMessage(UInt32 ms)
{
    ::MsgWaitForMultipleObjects(0,0,0, ms, QS_ALLEVENTS);
}

// Changes/sets window title
void    Direct3DWin32App::SetWindowTitle(const char *ptitle)
{
    wchar_t wptitle[256];
    if (::MultiByteToWideChar(CP_ACP, 0, ptitle, (int)gfc_strlen(ptitle) + 1, wptitle, sizeof(wptitle)) == 0)
        wptitle[0] = 0;
    ::SetWindowTextW(hWND, wptitle);
}

// Presents the data (swaps the back and front buffers)
void    Direct3DWin32App::PresentFrame()
{
    if (pDevice)
        pDevice->Present( NULL, NULL, NULL, NULL );
}

// *** Overrides

// This override is called to initialize Direct3D from setup window
bool    Direct3DWin32App::InitRenderer()
{
    return 1;
}
// Should/can be called every frame to prepare the render, user function
void    Direct3DWin32App::PrepareRendererForFrame()
{   
}
    
// Message processing overrides
void    Direct3DWin32App::OnKey(UInt key, UInt info, bool downFlag)
{
    GUNUSED3(key,info,downFlag);
}
void    Direct3DWin32App::OnChar(UInt32 wcharCode, UInt info)
{
    GUNUSED2(wcharCode,info);
}
void    Direct3DWin32App::OnMouseButton(UInt button, bool downFlag, SInt x, SInt y)
{
    GUNUSED4(button,downFlag,x,y);
}
void    Direct3DWin32App::OnMouseWheel(SInt zdelta, SInt x, SInt y)
{
    GUNUSED3(zdelta,x,y);
}
void    Direct3DWin32App::OnMouseMove(SInt x, SInt y)
{
    GUNUSED2(x,y);
}


// Sizing; by default, re-initalizes the renderer
void    Direct3DWin32App::OnSize(SInt w, SInt h)
{
    Width  = w;
    Height = h;

    // Minimized/zero size? Do nothing.
    if ((Width == 0) && (Height == 0))
        return;
    if (!pDevice)
        return;

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
}

// Called when sizing begins and ends.
void    Direct3DWin32App::OnSizeEnter(bool enterSize)
{
    GUNUSED(enterSize);
}

// Handle dropped files
void    Direct3DWin32App::OnDropFiles(char *path)
{
    GUNUSED(path);
}


// Returns 1 if recreate succeeded, 0 if it failed. If reset is not specified, function will never fail.
bool    Direct3DWin32App::ConfigureWindow(SInt x, SInt y, SInt w, SInt h, bool fullScreen, bool recreateRenderer)
{
    // Change the settings.
    FullScreen  = fullScreen;
    Width       = w;
    Height      = h;
    PresentParams.Windowed          = !FullScreen;  
    PresentParams.BackBufferFormat  = FullScreen ? D3DFMT_X8R8G8B8 : D3DFMT_UNKNOWN;
    PresentParams.BackBufferHeight  = Height;
    PresentParams.BackBufferWidth   = Width;
    
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
    LockOnSize = 0;

    // And recreate the renderer if necessary.
    return recreateRenderer ? RecreateRenderer() : 1;
}

void Direct3DWin32App::SetCursor(HCURSOR cursor)
{
    hCursor = cursor;
    ::SetCursor(hCursor);
}

void Direct3DWin32App::ResetCursor()
{
    CursorIsOutOfClientArea = 0;
    CursorHidden = 0;
    CursorHiddenSaved = 0;

    if (!CursorDisabled)
    {
        ShowCursorInstantly(true);
        SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    else
        ShowCursorInstantly(false);
}

void Direct3DWin32App::ShowCursorInstantly(bool show)
{
    if (show)
    {
        while(::ShowCursor(TRUE) < 0)
            ;
    }
    else
    {
        while(::ShowCursor(FALSE) >= 0)
            ;
    }
}

void Direct3DWin32App::ShowCursor(bool show)
{
    if (CursorDisabled) 
        return;
    if (show)
    {
        if (CursorHidden)
        {
            CursorHidden = !CursorHidden;
            if (!CursorIsOutOfClientArea) 
                ShowCursorInstantly(true);
        }
    }
    else
    {
        if (!CursorHidden)
        {
            CursorHidden = !CursorHidden;
            if (!CursorIsOutOfClientArea) 
                ShowCursorInstantly(false);
        }
    }
}

// *** Window procedure

// Mousewheel support
#include    <winuser.h>
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120
#endif

// MouseLeave support
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE                   0x02A3
#endif

// XButton WM_ message support
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_XBUTTONDBLCLK                0x020D
#define WM_NCXBUTTONDOWN                0x00AB
#define WM_NCXBUTTONUP                  0x00AC
#define WM_NCXBUTTONDBLCLK              0x00AD
// XButton values are WORD flags
#define GET_XBUTTON_WPARAM(wp)          (HIWORD(wp))
#define XBUTTON1                        0x0001
#define XBUTTON2                        0x0002
#endif
// XButton VK_ codes
#ifndef VK_XBUTTON1
#define VK_XBUTTON1                     0x05
#define VK_XBUTTON2                     0x06
#endif

#include <stdio.h>

LRESULT Direct3DWin32App::MemberWndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_CREATE:
            break;
            
        case WM_ACTIVATE:           
            if (!HIWORD(wParam))                
                Active = 1; // Window was restored or maximized
            else                                
                Active = 0; // Window was minimized
            return 0;
            
        case WM_SYSCOMMAND:         
            // Look for screen-saver and power-save modes
            switch (wParam)
            {
                case SC_SCREENSAVE:     // Screen-saver is starting
                case SC_MONITORPOWER:   // Monitor is going to power-save mode
                    // Prevent either from happening by returning 0
                    return 0;
                default:
                    break;
            }
            break;

        case WM_DROPFILES:
            {
                UInt itemCount = ::DragQueryFile((HDROP)wParam, 0xFFFFFFFF,0,0);
                if (itemCount)
                {
                    // Get name
                    char    buffer[512];
                    buffer[0] = 0;
                    ::DragQueryFile((HDROP)wParam, 0, buffer, 512);
                    ::DragFinish((HDROP)wParam);

                    // Inform user about the drop
                    OnDropFiles(buffer);
                }
            }
            return 0;
            
        case WM_CLOSE:
            // Window is being closed. // Send WM_QUIT to a message queue.
            PostQuitMessage(0);
            return 0;           

        case WM_SIZE:
            if (!LockOnSize)
                OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_ENTERSIZEMOVE:  
            OnSizeEnter(1); 
            return 0;
        case WM_EXITSIZEMOVE:   
            OnSizeEnter(0); 
            return 0;

        case WM_KEYDOWN:        OnKey((UInt)wParam, (UInt)lParam, 1);   return 0;
        case WM_KEYUP:          OnKey((UInt)wParam, (UInt)lParam, 0);   return 0;
        case WM_CHAR:          
            {
                UInt32 wcharCode = (UInt32)wParam;
                OnChar(wcharCode, (UInt)lParam);
            }
            break;

        case WM_MOUSEMOVE:      OnMouseMove((SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam)); return 0;
        // Mouse wheel support
        case WM_MOUSEWHEEL:
            {
                // Nonclient position must be adjusted to be inside the window
                POINT   wcl = {0,0};
                ::ClientToScreen(hWND, &wcl);
                SInt x = SInt(SInt16(LOWORD(lParam))) - wcl.x;
                SInt y = SInt(SInt16(HIWORD(lParam))) - wcl.y;
                OnMouseWheel((SInt(SInt16(HIWORD(wParam)))*128)/WHEEL_DELTA, x, y);
                return 0;
            }
        case WM_SETCURSOR:
            if ((HWND)wParam == hWND)
            {
                if (CursorDisabled)
                    break;

                if(LOWORD(lParam) == HTCLIENT)
                {
                    if (CursorIsOutOfClientArea)
                    {
                        bool cursorWasHidden = CursorHiddenSaved;
                        CursorIsOutOfClientArea = false;
                        CursorHiddenSaved = false;
                        if (cursorWasHidden && CursorHidden) 
                            ShowCursorInstantly(false);
                    }
                    ::SetCursor(hCursor);
                    return 1;
                }
                else if (!CursorIsOutOfClientArea)
                {
                    CursorIsOutOfClientArea = true;
                    CursorHiddenSaved = CursorHidden;
                    if (CursorHidden) 
                        ShowCursorInstantly(true);
                }
            }
            break;
        // Mouse button support
        case WM_LBUTTONDOWN:    OnMouseButton(0, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_LBUTTONUP:      OnMouseButton(0, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_RBUTTONDOWN:    OnMouseButton(1, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_RBUTTONUP:      OnMouseButton(1, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_MBUTTONDOWN:    OnMouseButton(2, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_MBUTTONUP:      OnMouseButton(2, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        // XButton support
        case WM_XBUTTONDOWN:    OnMouseButton(2+GET_XBUTTON_WPARAM(wParam), 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));   return 0;
        case WM_XBUTTONUP:      OnMouseButton(2+GET_XBUTTON_WPARAM(wParam), 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));   return 0;

        /*
        case WM_CHAR:           
            switch (toupper(wParam))
            {
                case VK_ESCAPE:
                {                   
                    PostQuitMessage(0);
                    return 0;
                }
                default:
                    break;
            };
            break;
        */
            
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(hWND, &ps);
                EndPaint(hWND, &ps);
            }
            break;
            
        default:
            break;
    }
    
    return DefWindowProcW(hWND, message, wParam, lParam);
}


