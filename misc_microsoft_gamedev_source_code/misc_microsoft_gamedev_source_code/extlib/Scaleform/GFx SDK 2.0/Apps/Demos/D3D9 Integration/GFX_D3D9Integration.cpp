/*********************************************************************

Filename    :   GFX_D3D9Integration.cpp
Content     :   GFxPlayer Integration demo on top of D3D Matrices
Created     :   September 25, 2005
Authors     :   Michael Antonov
Copyright   :   (c) 2005 Scaleform Corp. All Rights Reserved.

This demo provides an example of how GFx can be used on
top of a Direct3D application. To make integration possible this
demo configures GRendererD3D9 in a dependent mode.

For GFx integration, the following modifications have
been made:
 1. The WinMain function was modified to load the sample SWF file
    and configure it for rendering.
 2. The Render function was modified to Display the movie on
    top ot 3D content (triangle).
 3. Video mode switching and device-recreation logic was introduced,
    to demostrate handling of lost devices and mode changes.

The rest the original Matrices triangle rendering code and most
of D3D9 initialization logic has been left untouched.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


//-----------------------------------------------------------------------------
// File: Matrices.cpp
//
// Desc: Now that we know how to create a device and render some 2D vertices,
//       this tutorial goes the next step and renders 3D geometry. To deal with
//       3D geometry we need to introduce the use of 4x4 matrices to transform
//       the geometry with translations, rotations, scaling, and setting up our
//       camera.
//
//       Geometry is defined in model space. We can move it (translation),
//       rotate it (rotation), or stretch it (scaling) using a world transform.
//       The geometry is then said to be in world space. Next, we need to
//       position the camera, or eye point, somewhere to look at the geometry.
//       Another transform, via the view matrix, is used, to position and
//       rotate our view. With the geometry then in view space, our last
//       transform is the projection transform, which "projects" the 3D scene
//       into our 2D viewport.
//
//       Note that in this tutorial, we are introducing the use of D3DX, which
//       is a set of helper utilities for D3D. In this case, we are using some
//       of D3DX's useful matrix initialization functions. To use D3DX, simply
//       include <d3dx9.h> and link with d3dx9.lib.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

// GFx includes
#include "GFile.h"
#include "GFxPlayer.h"
#include "GFxLoader.h"
#include "GRendererD3D9.h"

// Undefine new if necessary
#undef new

#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

D3DPRESENT_PARAMETERS   g_d3dpp;
LPDIRECT3DSTATEBLOCK9   g_pStateBlock   = 0;
HWND                    hWindow         = 0;

LPDIRECT3D9             g_pD3D       = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       g_pd3dDevice = NULL; // Our rendering device
LPDIRECT3DVERTEXBUFFER9 g_pVB        = NULL; // Buffer to hold vertices

// A structure for our custom vertex type
struct CUSTOMVERTEX
{
    FLOAT x, y, z;      // The untransformed, 3D position for the vertex
    DWORD color;        // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)


//-----------------------------------------------------------------------------
// Global variables - GFx
//-----------------------------------------------------------------------------
GPtr<GFxMovieDef>       pMovieDef;
GPtr<GFxMovieView>      pMovie;
GPtr<GRendererD3D9>     pRenderer;
GPtr<GFxRenderConfig>   pRenderConfig;
UInt32                  MovieLastTime;
// Global, illustrates whether we are rendering UI in wireframe or not
bool                    WireframeUI = 0;

// Describe supported modes (toggled with radio buttons in Flash).
struct VideoMode
{
    enum {
        DefaultModeIndex    = 0,
        ModeCount           = 3
    };

    char*       pName;
    UInt        Width, Height;
    bool        FullScreen;
    D3DFORMAT   Format;
};

// This is an array of hard-coded "video modes" that we can set
VideoMode VideoModes[VideoMode::ModeCount] = 
{
        {"640x480", 640, 480, 0, D3DFMT_UNKNOWN},
        {"800x600", 800, 600, 0, D3DFMT_UNKNOWN},
        {"640x480 Full Screen", 640, 480, 1, D3DFMT_X8R8G8B8}
};




//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    if( NULL == ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    // Set the default video mode on initialization
    VideoMode &mode= VideoModes[VideoMode::DefaultModeIndex];

    // Set up the structure used to create the D3DDevice    
    ZeroMemory( &g_d3dpp, sizeof(g_d3dpp) );
    g_d3dpp.Windowed                = mode.FullScreen ? FALSE : TRUE;
    g_d3dpp.SwapEffect              = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat        = mode.Format;  
    g_d3dpp.BackBufferWidth         = mode.Width;
    g_d3dpp.BackBufferHeight        = mode.Height;
    g_d3dpp.EnableAutoDepthStencil  = 1;
    g_d3dpp.AutoDepthStencilFormat  = D3DFMT_D24S8;
    //g_d3dpp.MultiSampleQuality        = D3DMULTISAMPLE_4_SAMPLES;

    // Create the D3DDevice
    if( FAILED( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &g_d3dpp, &g_pd3dDevice ) ) )
    {
        return E_FAIL;
    }

    // Create a state block.
    g_pd3dDevice->CreateStateBlock(D3DSBT_ALL, &g_pStateBlock);

    // Turn off culling, so we see the front and back of the triangle
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // Turn off D3D lighting, since we are providing our own vertex colors
    g_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    // No ZBuffer.
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Creates the scene geometry
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
    // Initialize three vertices for rendering a triangle
    CUSTOMVERTEX g_Vertices[] =
    {
        { -1.0f,-1.0f, 0.0f, 0xffff0000, },
        {  1.0f,-1.0f, 0.0f, 0xff0000ff, },
        {  0.0f, 1.0f, 0.0f, 0xffffffff, },
    };

    // Create the vertex buffer.
    if( FAILED( g_pd3dDevice->CreateVertexBuffer( 3*sizeof(CUSTOMVERTEX),
                                                  0, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT, &g_pVB, NULL ) ) )
    {
        return E_FAIL;
    }

    // Fill the vertex buffer.
    VOID* pVertices;
    if( FAILED( g_pVB->Lock( 0, sizeof(g_Vertices), (void**)&pVertices, 0 ) ) )
        return E_FAIL;
    memcpy( pVertices, g_Vertices, sizeof(g_Vertices) );
    g_pVB->Unlock();

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( g_pVB != NULL )
        g_pVB->Release();

    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();

    if (g_pStateBlock)
    {
        g_pStateBlock->Release();
        g_pStateBlock = 0;
    }

}



//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{
    // For our world matrix, we will just rotate the object about the y-axis.
    D3DXMATRIXA16 matWorld;

    // Set up the rotation matrix to generate 1 full rotation (2*PI radians) 
    // every 3000 ms. To avoid the loss of precision inherent in very high 
    // floating point numbers, the system time is modulated by the rotation 
    // period before conversion to a radian angle.
    UINT  iTime  = timeGetTime() % 3000;
    FLOAT fAngle = iTime * (2.0f * D3DX_PI) / 3000.0f;
    D3DXMatrixRotationY( &matWorld, fAngle );
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    D3DXVECTOR3 vEyePt( 0.0f, 3.0f,-5.0f );
    D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    g_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perpsective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f );
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}


//-----------------------------------------------------------------------------
// Name: ResetD3D()
// Desc: Resets the D3D device and recreates triangle geometry
//-----------------------------------------------------------------------------
HRESULT ResetD3D()
{   
    // Release D3D geometry so that reset can succeed
    if( g_pVB != NULL )
    {
        g_pVB->Release();
        g_pVB = 0;
    }
    if (g_pStateBlock)
    {
        g_pStateBlock->Release();
        g_pStateBlock = 0;
    }

    // Set new mode
    if( FAILED(g_pd3dDevice->Reset(&g_d3dpp) ))
        return E_FAIL;

    // Turn off culling, so we see the front and back of the triangle
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    // Turn off D3D lighting, since we are providing our own vertex colors
    g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);    
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Create a state block.
    g_pd3dDevice->CreateStateBlock(D3DSBT_ALL, &g_pStateBlock);

    InitGeometry();
    SetupMatrices();
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    // Clear the backbuffer to a black color
    g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(200,200,200), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( g_pd3dDevice->BeginScene() ) )
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();

        // Render the vertex buffer contents
        g_pd3dDevice->SetStreamSource( 0, g_pVB, 0, sizeof(CUSTOMVERTEX) );
        g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
        g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 1 );

        // End the scene
        g_pd3dDevice->EndScene();
    }

    
    // *** GFx Rendering    
    if (pMovie)
    {
        if (g_pStateBlock)
            g_pStateBlock->Capture();
        
        g_pd3dDevice->SetRenderState(D3DRS_FILLMODE,
                        WireframeUI ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

        UInt32  time    = timeGetTime();
        Float   delta   = ((Float)(time - MovieLastTime)) / 1000.0f;
        // Advance time and display the movie.
        pMovie->Advance(delta);
        pMovie->Display();

        MovieLastTime = time;

        if (g_pStateBlock)
            g_pStateBlock->Apply();
    }


    // Present the backbuffer contents to the display
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}




// Helper used to convert VK key codes and route them to GFxPlayer.
void    HandleKeyEvent(UInt key, bool down)
{
    if (!pMovie)
        return;

    GFxKey::Code    c(GFxKey::VoidSymbol);

    if (key >= 'A' && key <= 'Z')
    {
        c = (GFxKey::Code) ((key - 'A') + GFxKey::A);
    }
    else if (key >= VK_F1 && key <= VK_F15)
    {
        c = (GFxKey::Code) ((key - VK_F1) + GFxKey::F1);
    }
    else if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9)
    {
        c = (GFxKey::Code) ((key - VK_NUMPAD0) + GFxKey::KP_0);
    }
    else
    {
        // many keys don't correlate, so just use a look-up table.
        struct
        {
            int         vk;
            GFxKey::Code    gs;
            } table[] =
        {
                { VK_RETURN,    GFxKey::Return },
                { VK_ESCAPE,    GFxKey::Escape },
                { VK_LEFT,      GFxKey::Left },
                { VK_UP,        GFxKey::Up },
                { VK_RIGHT,     GFxKey::Right },
                { VK_DOWN,      GFxKey::Down },

                // TODO: fill this out some more
                { 0, GFxKey::VoidSymbol }
        };

        for (int i = 0; table[i].vk != 0; i++)
        {
            if (key == (UInt)table[i].vk)
            {
                c = table[i].gs;
                break;
            }
        }
    }

    if (c != GFxKey::VoidSymbol)
    {
        if (pMovie)
        {
            GFxKeyEvent event(down ? GFxEvent::KeyDown : GFxKeyEvent::KeyUp, c);
            pMovie->HandleEvent(event);         
        }
    }
}



//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            return 0;

        case WM_SETCURSOR:
            if (LOWORD(lParam)==HTCLIENT)
            {
                ::SetCursor(::LoadCursor(0, IDC_ARROW));
                return 0;
            }
            break;
            

        // *** The following logic is used to route input to GFxPlayer.

        case WM_MOUSEMOVE:
            if (pMovie)
            {
                GFxMouseEvent event(GFxEvent::MouseMove, 0,
                                    (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));              
                pMovie->HandleEvent(event);
            }           
            return 0;

        case WM_LBUTTONDOWN:
            if (pMovie)
            {
                ::SetCapture(hWnd);
                GFxMouseEvent event(GFxEvent::MouseDown, 0,
                                    (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));
                pMovie->HandleEvent(event);
            }           
            return 0;
        case WM_LBUTTONUP:
            if (pMovie)
            {
                ::ReleaseCapture();
                GFxMouseEvent event(GFxEvent::MouseUp, 0,
                                    (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));
                pMovie->HandleEvent(event);
            }           
            return 0;

        case WM_KEYDOWN:
            HandleKeyEvent((UInt)wParam, 1);
            return 0;
        case WM_KEYUP:          
            HandleKeyEvent((UInt)wParam, 0);
            return 0;
    }


    return DefWindowProc( hWnd, msg, wParam, lParam );
}

class FxPlayerFileOpener : public GFxFileOpener
{
public:
	virtual GFile* OpenFile(const char *pfilename)
	{
		return new GSysFile(pfilename);
	}
};

// Sets a new video mode based on the specified index in mode array.
void    SelectVideoMode(SInt index)
{
    if ((UInt)index >= VideoMode::ModeCount)
        return;
    if (!pMovie || !pRenderer)
        return;

    // Resolution change        
    VideoMode   &mode = VideoModes[index];
    pRenderer->ResetVideoMode();

    // Set new mode parameters
    g_d3dpp.Windowed        = mode.FullScreen ? FALSE : TRUE;
    g_d3dpp.BackBufferWidth = mode.Width;
    g_d3dpp.BackBufferHeight= mode.Height;
    g_d3dpp.BackBufferFormat= mode.Format;

    // Reset D3D device
    if (!SUCCEEDED( ResetD3D() ))
    {
        ::MessageBox(0, "Failed to set requested video mode", "D3D Error", MB_OK);
        exit(1);
    }

    // Set the window flags and size appropriately
    RECT r = { 0,0, mode.Width, mode.Height };
    LONG style = mode.FullScreen ? WS_POPUP : WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
    AdjustWindowRect(&r, style, 0);
    
    ::SetWindowLong(hWindow, GWL_STYLE, style| WS_VISIBLE);     
    ::SetWindowPos(hWindow, 0, 0,0, r.right-r.left,r.bottom-r.top,
                    SWP_NOMOVE|SWP_NOZORDER|SWP_FRAMECHANGED);

    // Set the new dependent mode.
    pRenderer->SetDependentVideoMode(g_pd3dDevice, &g_d3dpp, 0, hWindow);       

    pMovie->SetViewport(mode.Width, mode.Height, 0,0, mode.Width, mode.Height);
}

class FxPlayerFSCommandHandler : public GFxFSCommandHandler
{
public:
	virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg);
};

// FSComand callback. Used to handle video mode change requests from the SWF file.
void    FxPlayerFSCommandHandler::Callback(GFxMovieView* pmovie, const char* command, const char* args)
{
    if (!pMovie)
        return;

    if (!strcmp(command, "setMode"))
    {
        if (args && args[0]!=0 && args[1]==0)
        {
            // Set video mode
            SInt index = args[0] - '0';
            SelectVideoMode(index);
        }       
    }
    else if (!strcmp(command, "wireframe"))
    {
        if (args && args[0]!=0 && args[1]==0)
        {
            // Set wireframe
            if ((args[0] - '0') > 0)
                WireframeUI = 1;
            else
                WireframeUI = 0;            
        }       
    }

    else if (!strcmp(command, "exit"))
    {
        // Close window
        ::PostMessage(hWindow, WM_CLOSE, 0, 0);
    }
}

// File loader callback.
GFile* GFxLoader_FileOpenCallback(const char* ppathOrUrl)
{
    return new GSysFile(ppathOrUrl);
}



//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, INT )
{
    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "D3D Tutorial", NULL };
    RegisterClassEx( &wc );

    // Load Window.swf
    if (!lpCmdLine || !lpCmdLine[0])
        lpCmdLine = "Window.swf";

    VideoMode &mode= VideoModes[VideoMode::DefaultModeIndex];

    // Create the application's window
    RECT r = { 100,100, 100 + mode.Width, 100 + mode.Height };
    ::AdjustWindowRect(&r, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, 0);

    HWND hWnd = CreateWindow( "D3D Tutorial", "GFxPlayer D3D9 Integration",
                              WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
                              r.left, r.top, r.right-r.left, r.bottom - r.top,
                              GetDesktopWindow(), NULL, wc.hInstance, NULL );
    hWindow = hWnd;

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    {
        // Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );
         
            // Init GFC.
            GFxLoader loader;
			GPtr<GFxFileOpener> pfileOpener = *new FxPlayerFileOpener;
			loader.SetFileOpener(pfileOpener); 
			
			GPtr<GFxFSCommandHandler> pcommandHandler = *new FxPlayerFSCommandHandler;
			loader.SetFSCommandHandler(pcommandHandler);

            // For D3D, it is good to override image creator to keep image data,
            // so that it can be restored in case of a lost device.
            GPtr<GFxImageCreator> pimageCreator = *new GFxImageCreator(1);
            loader.SetImageCreator(pimageCreator);


            if (!(pMovieDef = *loader.CreateMovie(lpCmdLine)))
            {
    load_error_cleanup:
                pMovie = 0;
                pMovieDef = 0;
                Cleanup();
                UnregisterClass( "D3D Tutorial", wc.hInstance );
                return 1;
            }
			
            if (!(pMovie = *pMovieDef->CreateInstance()))
                goto load_error_cleanup;            

            // Create renderer.
            if (!(pRenderer = *GRendererD3D9::CreateRenderer()))
                goto load_error_cleanup;

			// Configure renderer in "Dependent mode", honoring externally
			// configured settings.
			pRenderer->SetDependentVideoMode(g_pd3dDevice, &g_d3dpp, 0, hWnd);	
			
			// Set renderer on loader so that it is also applied to all children.
			pRenderConfig = *new GFxRenderConfig(pRenderer, GFxRenderConfig::RF_EdgeAA);			
			loader.SetRenderConfig(pRenderConfig);            		

            // Background Alpha = 0 for transparent background,
            // and playback view spanning the entire window.
            pMovie->SetBackgroundAlpha(0.0f);
            pMovie->SetViewport(mode.Width, mode.Height, 0,0, mode.Width, mode.Height);

            // Store initial timing, so that we can determine
            // how much to advance Flash playback.
            MovieLastTime = timeGetTime();

            // Enter the message loop
            MSG msg;
            ZeroMemory( &msg, sizeof(msg) );
            while( msg.message!=WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                {
                    // Check for lost D3D Devices.
                    if (pRenderer)
                    {                       
                        GRendererD3D9::DisplayStatus status = pRenderer->CheckDisplayStatus();
                        if (status == GRendererD3D9::DisplayStatus_Unavailable)
                            { ::Sleep(10); continue; }
                        if (status == GRendererD3D9::DisplayStatus_NeedsReset)
                        {           
                            if (!SUCCEEDED(ResetD3D()))
                                continue;
                        }
                    }

                    Render();
                }
            }

            pMovie = 0;
            pMovieDef = 0;
            pRenderer = 0;
			pRenderConfig = 0;

        }
    }

    UnregisterClass( "D3D Tutorial", wc.hInstance );
    return 0;
}



