/**********************************************************************

Filename    :   Direct3DXbox360App.cpp
Content     :   Simple XBox 360 Application class implemenation
Created     :   
Authors     :   Michael Antonov
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "Direct3DXbox360App.h"

#include "GRefCount.h"

// Video mode (D3DFMT_X8R8G8B8, D3DFMT_R5G6B5.. )
#define  APP_COLOR_FORMAT           D3DFMT_X8R8G8B8
#define  APP_DEPTHSTENCIL_FORMAT    D3DFMT_D24S8


Direct3DXboxApp::Direct3DXboxApp()
{
    Created     = 0;
    Active      = 0;
    HideCursor  = 0;
    QuitFlag    = 0;
    ExitCode    = 0;

    // Requested 3D state   
    FullScreen  = 1;
    FSAntialias = 0;
    BitDepth    = 0;

    SupportDropFiles    = 0;
    SizableWindow = 0;

    Width       = 0;
    Height      = 0;
    hWND        = 0;

    // Clear GL function pointers
    pD3D        = Direct3DCreate9( D3D_SDK_VERSION );
    pDevice     = 0;
    // Set later on in CreateWindow
    FSAASupported= 0;

    In2DView        = 0;
    WasInScene      = 0;    

    // Raw shaders
    pVShaderCoordCopy = 0;
    pPShaderConst     = 0;          

    // For now initialize to 0.
    ViewportFactorX[0] = ViewportFactorX[1] = 
    ViewportFactorX[2] = ViewportFactorX[3] = 0.0f;
    ViewportFactorY[0] = ViewportFactorY[1] = 
    ViewportFactorY[2] = ViewportFactorY[3] = 0.0f;

    MouseX = MouseY = 100;
}

Direct3DXboxApp::~Direct3DXboxApp()
{
    if (Created)
        KillWindow();

    if (pD3D)
        pD3D->Release();
}


//-------------------------------------------------------------------------------------
// Vertex shader that copies the 2D output directly.
//-------------------------------------------------------------------------------------
const char* g_strVertexShaderProgram = 
"float4 xTransform : register(c0);"
"float4 yTransform : register(c1);"
" float4 main( float4 In : POSITION)  : POSITION"  
" {                                            "
" float4 res;"
//" res.x = dot(In, xTransform);"
//" res.y = dot(In, yTransform);"
" res.x = In.x * xTransform.x + xTransform.w;" // Un-scale viewport
" res.y = In.y * yTransform.y + yTransform.w;"
" res.zw= In.zw;"
"     return res;                              "  // Transfer color
" }                                            ";

//-------------------------------------------------------------------------------------
// Pixel shader - fills in a constant color
//-------------------------------------------------------------------------------------
const char* g_strPixelShaderProgram = 
" float4 solidColor : register(c0);            "
"                                              "  
" float4 main() : COLOR                        "  
" {                                            "  
"     return solidColor;                       "  // Output color
" }                                            "; 



// Create and show the window
bool Direct3DXboxApp::SetupWindow(const char *pname, SInt width, SInt height)
{
    if (!pD3D)
        return 0;
    if (Created)
        return 0;


    Width     = width;
    Height    = height;

    D3DFORMAT format = APP_COLOR_FORMAT;

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
    d3dpp.Windowed                  = 0;    
    d3dpp.BackBufferFormat          = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount           = 1;
    d3dpp.EnableAutoDepthStencil    = TRUE;
    d3dpp.AutoDepthStencilFormat    = D3DFMT_D24S8;
    d3dpp.BackBufferHeight          = Height;
    d3dpp.BackBufferWidth           = Width;
    d3dpp.SwapEffect                = D3DSWAPEFFECT_DISCARD;
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
    else
        d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;

    // Create the D3DDevice
    if( FAILED(pD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL,
                                   D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pDevice ) ) )
    {
        return 0;
    }

    // Compile simple shaders.


    // Compile vertex shader.
    GPtr<ID3DXBuffer> pVertexShaderCode;
    GPtr<ID3DXBuffer> pVertexErrorMsg;
    HRESULT hr = D3DXCompileShader( g_strVertexShaderProgram, 
        (UINT)strlen( g_strVertexShaderProgram ),
        NULL, 
        NULL, 
        "main", 
        "vs_2_0", 
        0, 
        &pVertexShaderCode.GetRawRef(), 
        &pVertexErrorMsg.GetRawRef(), 
        NULL );
    if( FAILED(hr) )
    {
        if( pVertexErrorMsg )
            OutputDebugString( (char*)pVertexErrorMsg->GetBufferPointer() );
        return 0;
    }    

    // Create vertex shader.
    pDevice->CreateVertexShader( (DWORD*)pVertexShaderCode->GetBufferPointer(), 
        &pVShaderCoordCopy );

    // Compile pixel shader.
    GPtr<ID3DXBuffer> pPixelShaderCode;
    GPtr<ID3DXBuffer> pPixelErrorMsg;
    hr = D3DXCompileShader( g_strPixelShaderProgram, 
        (UINT)strlen( g_strPixelShaderProgram ),
        NULL, 
        NULL, 
        "main", 
        "ps_2_0", 
        0, 
        &pPixelShaderCode.GetRawRef(), 
        &pPixelErrorMsg.GetRawRef(),
        NULL );
    if( FAILED(hr) )
    {
        if( pPixelErrorMsg )
            OutputDebugString( (char*)pPixelErrorMsg->GetBufferPointer() );
        return 0;
    }

    // Create pixel shader.
    pDevice->CreatePixelShader( (DWORD*)pPixelShaderCode->GetBufferPointer(), 
        &pPShaderConst );


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
bool    Direct3DXboxApp::RecreateRenderer()
{   
    if (!pDevice)
        return 0;

    if(FAILED(pDevice->Reset(&PresentParams)))
        return 0;

    return InitRenderer();
}

// Deletes the DC, RC, and Window, and restores the original display.
void Direct3DXboxApp::KillWindow()
{
    if (!Created)
        return;

    if (pDevice)
        pDevice->Release();
    
    Created = 0;
    return;
}



void    Direct3DXboxApp::Clear(UInt32 color)
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
void    Direct3DXboxApp::Push2DRenderView()
{   
    if  (In2DView)
    {
        ::OutputDebugString("GFC Warning: Direct3DXboxApp::Push2DRenderView failed - already in view.");
        return;
    }
    In2DView = 1;

    if (!pDevice)
        return;

    // Viewport unscale matrix
    ViewportFactorX[0] = 2.0f / (float) Width;
    ViewportFactorY[1] = -2.0f / (float) Height;
    ViewportFactorX[3] = -1.0f;
    ViewportFactorY[3] = 1.0f;

    // On Xbox360, there should be some way to disable viewport scaling;
    // i.e. use GPU_VTECONTROL. However, that seems very low-level...


    // Save viewport.
    pDevice->GetViewport(&ViewportSave);

    // Set new viewport.
    D3DVIEWPORT9    vp = { 0,0, Width, Height, 0.0f, 0.0f };
    pDevice->SetViewport(&vp);

    // Set constants
    pDevice->SetVertexShaderConstantF(0, ViewportFactorX, 1);
    pDevice->SetVertexShaderConstantF(1, ViewportFactorY, 1);

    // Begin scene if necessary.
    if (pDevice->BeginScene() == D3DERR_INVALIDCALL)
        WasInScene = 1;
    else
        WasInScene = 0;

    // Set some states ? TBD.
}


void    Direct3DXboxApp::Pop2DRenderView()
{
    if (!In2DView)
    {
        ::OutputDebugString("GFC Warning: Direct3DXboxApp::Pop2DRenderView failed - not in view.");
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
    float x, y, z, w;
    void    SetValue2D(float _x, float _y)  { x=_x; y=_y; z=0.0f; w=1.0f; }
    void    SetValue2D(int _x, int _y)      { x=(float)_x; y=(float)_y; z=0.0f; w=1.0f; }
};

// Draw a filled + blended rectangle.
void    Direct3DXboxApp::FillRect(SInt l, SInt t, SInt r, SInt b, UInt32 color)
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
    pDevice->SetVertexShader(pVShaderCoordCopy);
    pDevice->SetPixelShader(pPShaderConst);

    // Set color. This is compatible with our byte order def.
    float rgba[4] = 
    {
        (color & 0xFF)              / 255.0f, // Red
        ((color & 0xFF00) >> 8)     / 255.0f, // Green
        ((color & 0xFF0000) >> 16)  / 255.0f, // Blue
        ((color & 0xFF000000) >> 24)/ 255.0f, // Alpha
    };
    pDevice->SetPixelShaderConstantF(0, rgba, 1);

    // Standard blending    
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD); 
    pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    // Flipping the y coordinate may cull up our strip...
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // Sometimes the rectangle disappears, based on SWF file contents. Why??
        
    pDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP,  2, tv, sizeof(XYZWVertex) );
}


// Draw a text string (specify top-left corner of characters, NOT baseline)
void    Direct3DXboxApp::DrawText(SInt x, SInt y, const char *ptext, UInt32 color)
{
}

void    Direct3DXboxApp::CalcDrawTextSize(SInt *pw, SInt *ph, const char *ptext)
{   
}


// API-independednt toggle for wireframe rendering.
void    Direct3DXboxApp::SetWireframe(bool wireframeEnable)
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

// Button bit to key maping
struct XBoxButtonToKey
{
    DWORD   Button;
    UInt    Key;
};

XBoxButtonToKey XBoxButtonToKey_Map[] =
{
    { XINPUT_GAMEPAD_BACK,          VK_BACK     },
    { XINPUT_GAMEPAD_START,         VK_PAUSE    },
    { XINPUT_GAMEPAD_A,             'A'         },
    { XINPUT_GAMEPAD_B,             'B'         },
    { XINPUT_GAMEPAD_X,             'X'         },
    { XINPUT_GAMEPAD_Y,             'Y'         },
    { XINPUT_GAMEPAD_RIGHT_SHOULDER,'N'         },
    { XINPUT_GAMEPAD_LEFT_SHOULDER, 'P'         },
    { XINPUT_GAMEPAD_DPAD_UP,       VK_UP       },
    { XINPUT_GAMEPAD_DPAD_DOWN,     VK_DOWN     },
    { XINPUT_GAMEPAD_DPAD_LEFT,     VK_LEFT     },
    { XINPUT_GAMEPAD_DPAD_RIGHT,    VK_RIGHT    },
    { 0, 0 }
};

bool    Direct3DXboxApp::ProcessMessages()
{   
    XINPUT_STATE newState;
    SInt mbutton = -1;

    if (XInputGetState(0, &newState) == ERROR_SUCCESS)
    {
        // Send out fake keystrokes for buttons
        for (int i=0; XBoxButtonToKey_Map[i].Button != 0; i++)
        {
            if (!(InputState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button) &&
                (newState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button))
            {
                OnKey(XBoxButtonToKey_Map[i].Key, 1);
            }
            else if ((InputState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button) &&
                     !(newState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button))
            {
                OnKey(XBoxButtonToKey_Map[i].Key, 0);
            }

        }       

        if (newState.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bLeftTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnKey('L', 1);
        else if (newState.Gamepad.bLeftTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnKey('L', 0);

        if (newState.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnKey('R', 1);
        else if (newState.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnKey('R', 0);

        if ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) !=
            (InputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
            mbutton = (newState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? 1 : 0;

        if (newState.Gamepad.sThumbLX > 0)
        {
            newState.Gamepad.sThumbLX -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbLX < 0)
                newState.Gamepad.sThumbLX = 0;
        }
        else if (newState.Gamepad.sThumbLX < 0)
        {
            newState.Gamepad.sThumbLX += XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbLX > 0)
                newState.Gamepad.sThumbLX = 0;
        }
        if (newState.Gamepad.sThumbLY > 0)
        {
            newState.Gamepad.sThumbLY -= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbLY < 0)
                newState.Gamepad.sThumbLY = 0;
        }
        else if (newState.Gamepad.sThumbLY < 0)
        {
            newState.Gamepad.sThumbLY += XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbLY > 0)
                newState.Gamepad.sThumbLY = 0;
        }

        MouseXadj = Float(newState.Gamepad.sThumbLX) * 0.02f/Width;
        MouseYadj = -Float(newState.Gamepad.sThumbLY) * 0.02f/Width;

        if (MouseXadj || MouseYadj)
        {
            MouseX += MouseXadj;
            MouseY += MouseYadj;
            if (MouseX < 0) MouseX = 0;
            if (MouseX >= Width) MouseX = (Float)Width;
            if (MouseY < 0) MouseY = 0;
            if (MouseY >= Height) MouseY = (Float)Height;

            OnMouseMove ((SInt)MouseX, (SInt)MouseY);
        }
        if (mbutton != -1)
            OnMouseButton(0, mbutton ? 1 : 0, (SInt)MouseX, (SInt)MouseY);

        memcpy(&InputState, &newState, sizeof(XINPUT_STATE));
    }           

    return !QuitFlag;
}


// Changes/sets window title
void    Direct3DXboxApp::SetWindowTitle(const char *ptitle)
{
}

// Presents the data (swaps the back and front buffers)
void    Direct3DXboxApp::PresentFrame()
{
    if (pDevice)
        pDevice->Present( NULL, NULL, NULL, NULL );
}

// *** Overrides

// This override is called to initialize Direct3D from setup window
bool    Direct3DXboxApp::InitRenderer()
{
    return 1;
}
// Should/can be called every frame to prepare the render, user function
void    Direct3DXboxApp::PrepareRendererForFrame()
{   
}
