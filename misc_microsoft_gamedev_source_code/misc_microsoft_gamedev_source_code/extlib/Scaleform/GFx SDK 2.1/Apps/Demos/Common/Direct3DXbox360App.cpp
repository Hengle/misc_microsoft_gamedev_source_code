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
#include "FxDeviceDirect3D.h"
#include "GStd.h"
#include <direct.h>
#include "FileFindWin32.h"

#include "xam.h"

// Video mode (D3DFMT_X8R8G8B8, D3DFMT_R5G6B5.. )
#define  APP_COLOR_FORMAT           D3DFMT_X8R8G8B8
#define  APP_DEPTHSTENCIL_FORMAT    D3DFMT_D24S8


Direct3DXboxApp::Direct3DXboxApp(): FxApp()
{
    Console     = true;  // this is console
    pFxDevice = new FxDeviceDirect3D(this);

    // Requested 3D state   
    FullScreen  = 1;
    hWND        = 0;

    // Clear GL function pointers
    pD3D        = Direct3DCreate9( D3D_SDK_VERSION );
    pDevice     = 0;
    CurBuffer   = 0;
    // Set later on in CreateWindow
    FSAASupported= 0;
    PredicatedTiling= 0;

    CursorHidden = 1;
    CursorDisabled = 1;


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
    delete pFxDevice;
}


bool Direct3DXboxApp::CreateRenderer()
{
    pRenderer = *renderer_type::CreateRenderer();
    SetRendererDependentVideoMode();
    return pRenderer.GetPtr() != NULL;
}

void Direct3DXboxApp::SetRendererDependentVideoMode()
{
    if (pRenderer)
    {
        pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 
                    PredicatedTiling ? renderer_type::VMConfig_SupportTiling : 0, hWND);
    }
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
bool Direct3DXboxApp::SetupWindow(const char *pname, int width, int height,
                                  const SetupWindowParams& extraParams )
{
    if (!pD3D)
        return 0;
    if (Created)
        return 0;


    Width     = width;
    Height    = height;
    PredicatedTiling = extraParams.ForceTiling || (Width > 1024) || (Height > 1024);

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

    if (PredicatedTiling)
    {
        // Command buffer for tiling must be big enough for entire scene
        d3dpp.RingBufferParameters.SecondarySize = 16 * 1024 * 1024;
        d3dpp.DisableAutoFrontBuffer    = 1;
    }

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

    if (PredicatedTiling)
    {
        pDevice->CreateTexture(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, 1, 0, D3DFMT_LE_X8R8G8B8, D3DPOOL_DEFAULT,
            &pFrameBuffer[0], 0);

        pDevice->CreateTexture(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, 1, 0, D3DFMT_LE_X8R8G8B8, D3DPOOL_DEFAULT,
            &pFrameBuffer[1], 0);
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


GRenderer* Direct3DXboxApp::GetRenderer()
{
    return pRenderer.GetPtr();
}

FxApp::DisplayStatus  Direct3DXboxApp::CheckDisplayStatus() const
{
    if (!pRenderer)
        return FxApp::DisplayStatus_Unavailable;
    return (FxApp::DisplayStatus)pRenderer->CheckDisplayStatus();
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

    for (UInt i = 0; i < 2; i++)
        if (pFrameBuffer[i])
        {
            pFrameBuffer[i]->Release();
            pFrameBuffer[i] = 0;
        }
    
    Created = 0;
    return;
}

void Direct3DXboxApp::SwitchFSAA(bool on_off)
{
    if (pRenderer && FSAASupported && FSAntialias != on_off)
    {
        FSAntialias = on_off;
        // On XBox360 there is not enough EDRAM to do FSAA in higher res without tiling.
        if (FSAntialias && (Width <= 640 && Height <= 480))
        {
            PresentParams.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
            PresentParams.SwapEffect      = D3DSWAPEFFECT_DISCARD; // Discard required
        }
        else
        {
            PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
            PresentParams.SwapEffect      = D3DSWAPEFFECT_DISCARD;
        }
        pRenderer->ResetVideoMode();
        RecreateRenderer();
        SetRendererDependentVideoMode();
    }

}


// Message processing function to be called in the 
// application loops until this returns 0.

// Button bit to key maping
struct XBoxButtonToKey
{
    DWORD          Button;
    FxApp::PadKeyCode Key;
};

XBoxButtonToKey XBoxButtonToKey_Map[] =
{
    { XINPUT_GAMEPAD_BACK,          FxApp::Pad_Back },
    { XINPUT_GAMEPAD_START,         FxApp::Pad_Start},
    { XINPUT_GAMEPAD_A,             FxApp::Pad_A},
    { XINPUT_GAMEPAD_B,             FxApp::Pad_B},
    { XINPUT_GAMEPAD_X,             FxApp::Pad_X},
    { XINPUT_GAMEPAD_Y,             FxApp::Pad_Y},
    { XINPUT_GAMEPAD_RIGHT_SHOULDER,FxApp::Pad_R1},
    { XINPUT_GAMEPAD_LEFT_SHOULDER, FxApp::Pad_L1},
    { XINPUT_GAMEPAD_RIGHT_THUMB,	FxApp::Pad_RT},
    { XINPUT_GAMEPAD_LEFT_THUMB,	FxApp::Pad_LT},
    { XINPUT_GAMEPAD_DPAD_UP,       FxApp::Pad_Up},
    { XINPUT_GAMEPAD_DPAD_DOWN,     FxApp::Pad_Down},
    { XINPUT_GAMEPAD_DPAD_LEFT,     FxApp::Pad_Left},
    { XINPUT_GAMEPAD_DPAD_RIGHT,    FxApp::Pad_Right},
    { 0,                            FxApp::VoidPadKey }
};

static struct 
{
    WORD winKey;
    FxApp::KeyCode appKey;
} KeyCodeMap[] = 
{
    {VK_BACK,    FxApp::Backspace},
    {VK_TAB,     FxApp::Tab},
    {VK_CLEAR,   FxApp::ClearKey},
    {VK_RETURN,  FxApp::Return},
    {VK_SHIFT,   FxApp::Shift},
    {VK_CONTROL, FxApp::Control},
    {VK_MENU,    FxApp::Alt},
    {VK_PAUSE,   FxApp::Pause},
    {VK_CAPITAL, FxApp::CapsLock},
    {VK_ESCAPE,  FxApp::Escape},
    {VK_SPACE,   FxApp::Space},
    {VK_PRIOR,   FxApp::PageUp},
    {VK_NEXT,    FxApp::PageDown},
    {VK_END,     FxApp::End},
    {VK_HOME,    FxApp::Home},
    {VK_LEFT,    FxApp::Left},
    {VK_UP,      FxApp::Up},
    {VK_RIGHT,   FxApp::Right},
    {VK_DOWN,    FxApp::Down},
    {VK_INSERT,  FxApp::Insert},
    {VK_DELETE,  FxApp::Delete},
    {VK_HELP,    FxApp::Help},
    {VK_NUMLOCK, FxApp::NumLock},
    {VK_SCROLL,  FxApp::ScrollLock},

    {VK_OEM_1,     FxApp::Semicolon},
    {VK_OEM_PLUS,  FxApp::Equal},
    {VK_OEM_COMMA, FxApp::Comma},
    {VK_OEM_MINUS, FxApp::Minus},
    {VK_OEM_PERIOD,FxApp::Period},
    {VK_OEM_2,     FxApp::Slash},
    {VK_OEM_3,     FxApp::Bar},
    {VK_OEM_4,     FxApp::BracketLeft},
    {VK_OEM_5,     FxApp::Backslash},
    {VK_OEM_6,     FxApp::BracketRight},
    {VK_OEM_7,     FxApp::Quote}
};

bool    Direct3DXboxApp::ProcessMessages()
{   
    XINPUT_STATE newState;
    SInt mbutton = -1;
	SInt rthumbbutton = -1;

    if (XInputGetState(0, &newState) == ERROR_SUCCESS)
    {
        // Send out fake keystrokes for buttons
        for (int i=0; XBoxButtonToKey_Map[i].Button != 0; i++)
        {
            if (!(InputState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button) &&
                (newState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button))
            {
                OnPad(XBoxButtonToKey_Map[i].Key, 1);
            }
            else if ((InputState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button) &&
                     !(newState.Gamepad.wButtons & XBoxButtonToKey_Map[i].Button))
            {
                OnPad(XBoxButtonToKey_Map[i].Key, 0);
            }
        }       

        if (newState.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bLeftTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnPad(FxApp::Pad_L2, 1);
        else if (newState.Gamepad.bLeftTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnPad(FxApp::Pad_L2, 0);

        if (newState.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnPad(FxApp::Pad_R2, 1);
        else if (newState.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD &&
            InputState.Gamepad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
            OnPad(FxApp::Pad_R2, 0);

        if ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) !=
            (InputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
            mbutton = (newState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? 1 : 0;

		if ((newState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) !=
            (InputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB))
            rthumbbutton = (newState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 1 : 0;

        if (newState.Gamepad.sThumbRX > 0)
        {
            newState.Gamepad.sThumbRX -= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbRX < 0)
                newState.Gamepad.sThumbRX = 0;
        }
        else if (newState.Gamepad.sThumbRX < 0)
        {
            newState.Gamepad.sThumbRX += XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbRX > 0)
                newState.Gamepad.sThumbRX = 0;
        }
        if (newState.Gamepad.sThumbRY > 0)
        {
            newState.Gamepad.sThumbRY -= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbRY < 0)
                newState.Gamepad.sThumbRY = 0;
        }
        else if (newState.Gamepad.sThumbRY < 0)
        {
            newState.Gamepad.sThumbRY += XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE + 200;
            if (newState.Gamepad.sThumbRY > 0)
                newState.Gamepad.sThumbRY = 0;
        }
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

            OnMouseMove ((SInt)MouseX, (SInt)MouseY, 0);
        }
        if (mbutton != -1)
            OnMouseButton(0, mbutton ? 1 : 0, (SInt)MouseX, (SInt)MouseY, 0);
        if (rthumbbutton != -1)
            OnMouseButton(1, rthumbbutton ? 1 : 0, (SInt)MouseX, (SInt)MouseY, 0);

        memcpy(&InputState, &newState, sizeof(XINPUT_STATE));
    }           
    FxXboxIMEEvent eventBefore;
    bool ret = OnIMEEvent(eventBefore);
    XINPUT_KEYSTROKE key;
    XInputGetKeystroke(0, XINPUT_FLAG_KEYBOARD, &key);
    if (ret)
    {
        FxXboxIMEEvent eventKey;
        if (OnIMEEvent(eventKey))
            return !QuitFlag;
    }
    if( key.Flags & XINPUT_KEYSTROKE_KEYDOWN )
    {
        KeyCode kc = VoidSymbol;
        if (key.VirtualKey >= 'A' && key.VirtualKey <= 'Z')
        {
            kc = (KeyCode) ((key.VirtualKey - 'A') + A);
        }
        else if (key.VirtualKey >= VK_F1 && key.VirtualKey <= VK_F15)
        {
            kc = (KeyCode) ((key.VirtualKey - VK_F1) + F1);
        }
        else if (key.VirtualKey >= VK_NUMPAD0 && key.VirtualKey <= VK_DIVIDE)
        {
            kc = (KeyCode) ((key.VirtualKey - VK_NUMPAD0) + KP_0);
        }
        else 
        {
            for (int i = 0; KeyCodeMap[i].winKey != 0; i++)
            {
                if (key.VirtualKey == (UInt)KeyCodeMap[i].winKey)
                {
                    kc = KeyCodeMap[i].appKey;
                    break;
                }
            }
        }
        unsigned int mods = 0;
        if (key.Flags & XINPUT_KEYSTROKE_CTRL)
            mods |= KM_Control;
        if (key.Flags & XINPUT_KEYSTROKE_SHIFT)
            mods |= KM_Shift;
        if (key.Flags & XINPUT_KEYSTROKE_ALT)
            mods |= KM_Alt;
        OnKey(kc, 0, key.Unicode, mods, 1);
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
    if (!pDevice)
        return;

    if (PredicatedTiling)
    {
        UInt32 used, rem;
        pDevice->QueryBufferSpace(&used, &rem);
        pDevice->EndTiling(0, NULL, pFrameBuffer[CurBuffer], NULL, 0, 0, NULL);

        pDevice->SynchronizeToPresentationInterval();
        pDevice->Swap(pFrameBuffer[!CurBuffer], NULL);
        CurBuffer = !CurBuffer;
    }
    else
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
    if (PredicatedTiling)
    {
        // 4 tiles if Height>1024, otherwise 2 (vertical split only)
        UInt    twidth = ((Width/2)+31) & ~31;
        UInt    theight = GTL::gmin(Height, 1024);
        D3DRECT tiles[4] = {{0, 0,      twidth, theight}, {twidth, 0,       Width, theight},
                            {0, theight,twidth, Height},  {twidth, theight, Width, Height}};
        D3DVECTOR4 color = {128,0,0,255};

        pDevice->BeginTiling(0, theight==Height ? 2 : 4, tiles, &color, 0, 0);
    }
}

void Direct3DXboxApp::SleepMilliSecs(unsigned int ms)
{
    ::Sleep(ms);
}

void Direct3DXboxApp::Clear(unsigned int color) 
{ 
    if(pFxDevice)
        pFxDevice->Clear(color);  
}

void Direct3DXboxApp::Push2DRenderView() 
{ 
    if(pFxDevice)
        pFxDevice->Push2DRenderView(); 
}
void Direct3DXboxApp::Pop2DRenderView() 
{ 
    if(pFxDevice)
        pFxDevice->Pop2DRenderView(); 
}

void Direct3DXboxApp::RedrawMouseCursor()
{
    ShowCursorInstantly(!CursorHidden);
}

void Direct3DXboxApp::ShowCursorInstantly(bool show)
{
    if( !pDevice || !pFxDevice || !show)
        return;

    const float white[] = {1,1,1,1};
    float mcursor[15];

    memset(mcursor, 0, sizeof(float)*15);
    mcursor[0] = MouseX;
    mcursor[1] = MouseY;
    mcursor[3] = MouseX+6;
    mcursor[4] = MouseY+24;
    mcursor[6] = MouseX+12;
    mcursor[7] = MouseY+21;
    mcursor[9] = MouseX+18;
    mcursor[10] = MouseY+24;
    mcursor[12] = MouseX;
    mcursor[13] = MouseY;

    Push2DRenderView();

    pDevice->SetPixelShaderConstantF(0, white, 1);
    pDevice->SetPixelShader(pPShaderConst);
    pDevice->SetVertexShader(pVShaderCoordCopy);

    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);

    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    pDevice->SetTexture( 0, 0 );
    //pDevice->SetStreamSource( 0, pCursorVertexBuffer, 0, sizeof(float) * 3 );
    pDevice->SetFVF( D3DFVF_XYZ );

    pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, mcursor, sizeof(float) * 3);

    //pDevice->SetStreamSource( 0, 0, 0, 0 );

    Pop2DRenderView();
}

void Direct3DXboxApp::FillRect(int l, int t, int r, int b, unsigned int color)  
{ 
    if(pFxDevice)
        pFxDevice->FillRect(l, t, r, b, color); 
}

void Direct3DXboxApp::SetWireframe(bool wireframeEnable)  
{ 
    if(pFxDevice)
        pFxDevice->SetWireframe(wireframeEnable);  
}

bool Direct3DXboxApp::UpdateFiles(char* filename, bool prev)
{
    WIN32_FIND_DATA f;
    char    path[256];
    char*   pfilename=filename;
    SPInt len = strlen(pfilename);
	for (SPInt i=len; i>0; i--) 
    {
		if (pfilename[i]=='/' || pfilename[i]=='\\') {
			pfilename = pfilename+i+1;
				break;
			}
    }
    gfc_strcpy(path,sizeof(path),filename);
    unsigned int path_length =(unsigned int) strlen(filename)-strlen(pfilename);
    path[path_length]=0; //current directory

    //  if( _getcwd( &path[0], 256 ))
    {
        gfc_strcat(path,256,"*.*");
        if (FindNextFileInList(&f, path, pfilename, prev))
        {
            gfc_strcpy(&filename[path_length], 256-path_length, f.cFileName);
            return true;
        }
    }
    return false;
}
