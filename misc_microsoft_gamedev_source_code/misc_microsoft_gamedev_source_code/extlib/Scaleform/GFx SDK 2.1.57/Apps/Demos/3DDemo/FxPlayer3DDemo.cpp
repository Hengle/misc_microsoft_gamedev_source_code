/**********************************************************************

Filename    :   FxPlayer3DDemo.cpp
Content     :   Sample SWF file player leveraging GFxPlayer API
Created     :   August 24, 2005
Authors     :   Michael Antonov
Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "FxPlayer3DDemo.h"

// Standard includes
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>

// Window name
#define FXPLAYER_APP_TITLE      "Scaleform GFxPlayer 3D Demo"

// glOrtho parameter
#define OVERSIZE                1.0f

#define GFC_2PI                 (2*3.1415926f)


// ***** Contained flash playback class

// Create a play stream without a texture
FxPlayStream::FxPlayStream(FxPlayerApp* papp, GFxLoader &loader, const char *pfileName)
{
    // Init vars
    gfc_strcpy(FileName, 256, pfileName);    
    pDevice         = 0;
    pRenderTexture  = 0;
    pStencilSurface = 0;    
    SpeedScale      = 1.0f;
    FrameFrequency  = 0.8f;
    Transparent     = 0;
    x = y = 0;
    // Clear mouse state; mouse off screen
    xMouse = yMouse = -1000;
    MouseButtons    = 0;
    ForceRender     = 0;
    Width           = 0;
    Height          = 0;

    pApp = papp;

    // Get info about the width & height of the movie.
    if (!loader.GetMovieInfo(FileName, &MovieInfo))
        return;
    // Load the actual movie.
    pMovieDef = *loader.CreateMovie(FileName);
    if (!pMovieDef)     
        return;
    // And crate instance.
    pMovie = *pMovieDef->CreateInstance();
    if (!pMovie)    
        return;
    
    // View
    Width   = MovieInfo.Width;
    Height  = MovieInfo.Height;

    // Init timing
    StartTicks          = timeGetTime();
    LastAdvanceTicks    = StartTicks;
    NextAdvanceTicks    = 0;
    AdvanceCounter      = 0;
}

FxPlayStream::~FxPlayStream()
{
    if (pDevice)
        pDevice->Release();
    if (pRenderTexture)
        pRenderTexture->Release();
    if (pStencilSurface)
        pStencilSurface->Release();
}

// Sets the renderer for the movie
void    FxPlayStream::SetRenderConfig(GFxRenderConfig *prenderConfig)
{
    if (!pMovie)
        return;
    pRenderConfig = prenderConfig;
    //pMovie->SetRendererInfo(prenderer);
    LPDIRECT3DDEVICE9 pnewDevice = 
        ((GRendererD3D9*)prenderConfig->GetRenderer())->GetDirect3DDevice();
    pnewDevice->AddRef();
    if (pDevice)
        pDevice->Release();    
    pDevice = pnewDevice;    
}

// Set the viewport. Only necessary for non-stransparent rendering.
void    FxPlayStream::SetViewport(SInt vx, SInt vy, SInt width, SInt height, bool transparent)
{
    x = vx;
    y = vy; 
    Width   = (width <= 0) ? MovieInfo.Width : width;
    Height  = (height<= 0) ? MovieInfo.Height : height;
    Transparent = transparent;
}


// Create surfaces for individual rendering
bool    FxPlayStream::CreateBuffers(SInt w, SInt h, bool createStencil)
{
    if (!pMovie || !pRenderConfig)
        return 0;
    ReleaseBuffers();

    GASSERT(pDevice);
    //LPDIRECT3DDEVICE9 pDevice = pRenderer->GetDirect3DDevice();
    Width = w;
    Height= h;

    if (!pRenderTexture)
    {
        if (FAILED( pDevice->CreateTexture(         
                            w,h,0,
                            D3DUSAGE_RENDERTARGET|D3DUSAGE_AUTOGENMIPMAP, D3DFMT_X8R8G8B8, 
                            D3DPOOL_DEFAULT, &pRenderTexture, 0) ))
         return 0;
    }

    if (createStencil)
    {
        pDevice->CreateDepthStencilSurface( w,h, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0,
                                            TRUE, &pStencilSurface, 0);
    }

    return 1;
}

// Releases buffers, used to handle reset.
void    FxPlayStream::ReleaseBuffers()
{
    if (pRenderTexture)
    {
        pRenderTexture->Release();
        pRenderTexture = 0;
    }
    if (pStencilSurface)
    {
        pStencilSurface->Release();
        pStencilSurface = 0;
    }
}



void    FxPlayStream::AdvanceAndRender(UInt32 ticks)
{
    // Note: second condition can occur in first frame of movie if it was just loaded,
    // and the cached ticks value is passed from a loop
    if ((ticks == -1) || (LastAdvanceTicks > ticks))
        ticks = timeGetTime();

    // Advance the frame if necessary.
    bool    advanced = 0;

    if (ticks > NextAdvanceTicks)
    {
        int     deltaTicks  = ticks - LastAdvanceTicks;
        float   deltaT      = deltaTicks / 1000.f;

        // Compute next advance ticks, a bit higher rate then necessary
        NextAdvanceTicks = ticks + (UInt32) ((FrameFrequency *1000.0f) / MovieInfo.FPS);
        LastAdvanceTicks = ticks;

        pMovie->NotifyMouseState(xMouse, yMouse, MouseButtons);

        if (pApp->pTextureMovie != this || pApp->IsTextureMovieAnimating())
        {
            AdvanceCounter++;
            advanced = 1;
            // Advance the movie
            pMovie->Advance(deltaT * SpeedScale);
        }

        // Position slider if they are in this clip.
        if(pApp->pTextureMovie != this && !gfc_stricmp(FileName, "HudTop.swf"))
        {
/*            if (pApp->pTextureMovie && pApp->IsTextureMovieAnimating())
            {
                GFxMovieView*   pv = pApp->pTextureMovie->GetMovie();
                GFxMovieDef*    pd = pv ? pv->GetMovieDef() : 0;
                if (pd)
                {
                    Float count = (float)pd->GetFrameCount();
                    Float frame = (float)pv->GetCurrentFrame();
                    if (pMovie->IsAvailable("controls.sliders.sliderPlaytrack.setSlider"))
                        pMovie->Invoke("controls.sliders.sliderPlaytrack.setSlider", "%d", UInt((frame/count)*100) );
                }
            }
*/
            if (pApp->IsMeshRotating())
            {
                Float rot = pApp->MeshRotation / GFC_2PI;
                if (pMovie->IsAvailable("controls.sliders.sliderRotation.setSlider"))
                    pMovie->Invoke("controls.sliders.sliderRotation.setSlider", "%d", UInt(rot*100) );
            }

        }

    }

    // Rendering does not need to take place if we are rendering to texture
    // and the movie have not advanced.
    if (ForceRender)
        ForceRender = 0;
    else if (pRenderTexture && !advanced)
        return;

    // Surface storage pointers, used for render textures
    IDirect3DSurface9 *poldSurface      = 0;
    IDirect3DSurface9 *poldDepthSurface = 0;
    IDirect3DSurface9 *psurface         = 0;

    // Draw Flash to the renderer texture       
    if (pRenderTexture)
    {           
        // Save both RT and depth-stencil.
        pDevice->GetRenderTarget(0, &poldSurface);
        pDevice->GetDepthStencilSurface(&poldDepthSurface);

        // Get our surface & set it as RT
        pRenderTexture->GetSurfaceLevel(0, &psurface);                  
                    
        if (!FAILED(pDevice->SetRenderTarget(0, psurface )))
        {
            // Set stencil; this will disable it if not available.
            pDevice->SetDepthStencilSurface(pStencilSurface);           
        }
    }


    // Render.

    // Render flash contents here
    pMovie->SetViewport(Width, Height, x, y, Width, Height);
                        
    // Must clear after changing viewport.
    pMovie->SetBackgroundAlpha(Transparent ? 0.0f : 1.0f);

    // Renderer-specific preparation (Disable depth testing)
    pMovie->Display();

    // Restore states.
    pDevice->SetVertexShader(0);
    //pDevice->SetPixelShader(0);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    // Enable depth testing for 3D
    pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

    // Restore RT state.
    if (pRenderTexture)
    {
        // Restore the render target
        pDevice->SetRenderTarget(0, poldSurface);
        pDevice->SetDepthStencilSurface(poldDepthSurface);

        if (psurface)
            psurface->Release();
        if (poldSurface)
            poldSurface->Release();
        if (poldDepthSurface)
            poldDepthSurface->Release();
                
        // Need to do this so that mipmaps are updated.
        pRenderTexture->AddDirtyRect(0);    
    }

}




// *** Cube Data

struct Vertex
{
    enum
    {
        FVF = D3DFVF_XYZ | D3DFVF_TEX1
    };

    float x, y, z;
    float tu, tv;
};

Vertex g_cubeVertices[] =
{
    {-1.0f, 1.0f,-1.0f,  0.0f,0.0f },
    { 1.0f, 1.0f,-1.0f,  1.0f,0.0f },
    {-1.0f,-1.0f,-1.0f,  0.0f,1.0f },
    { 1.0f,-1.0f,-1.0f,  1.0f,1.0f },
    
    {-1.0f, 1.0f, 1.0f,  1.0f,0.0f },
    {-1.0f,-1.0f, 1.0f,  1.0f,1.0f },
    { 1.0f, 1.0f, 1.0f,  0.0f,0.0f },
    { 1.0f,-1.0f, 1.0f,  0.0f,1.0f },
    
    {-1.0f, 1.0f, 1.0f,  0.0f,0.0f },
    { 1.0f, 1.0f, 1.0f,  1.0f,0.0f },
    {-1.0f, 1.0f,-1.0f,  0.0f,1.0f },
    { 1.0f, 1.0f,-1.0f,  1.0f,1.0f },
    
    {-1.0f,-1.0f, 1.0f,  0.0f,0.0f },
    {-1.0f,-1.0f,-1.0f,  1.0f,0.0f },
    { 1.0f,-1.0f, 1.0f,  0.0f,1.0f },
    { 1.0f,-1.0f,-1.0f,  1.0f,1.0f },

    { 1.0f, 1.0f,-1.0f,  0.0f,0.0f },
    { 1.0f, 1.0f, 1.0f,  1.0f,0.0f },
    { 1.0f,-1.0f,-1.0f,  0.0f,1.0f },
    { 1.0f,-1.0f, 1.0f,  1.0f,1.0f },
    
    {-1.0f, 1.0f,-1.0f,  1.0f,0.0f },
    {-1.0f,-1.0f,-1.0f,  1.0f,1.0f },
    {-1.0f, 1.0f, 1.0f,  0.0f,0.0f },
    {-1.0f,-1.0f, 1.0f,  0.0f,1.0f }
};



// ***** FxPlayerApp Implementation

FxPlayerApp::FxPlayerApp()
{
    QuitFlag                = 0;

    Antialiased             = 1;

    ModelShown              = 0;
    UseCubeModel            = 1;

    // Clear mouse state; mouse off screen.
    xMouse = yMouse         = -1000;
    MouseButtons            = 0;

    ActiveMesh              = 0;    
    pCubeVertexBuffer       = 0;

    HUDWireframe            = 0;
    MovieWireframe          = 0;
    MovieAnimationDragging  = 0;
    MovieAnimationPaused    = 0;

    MeshWireframe           = 0;
    MeshRotationDragging    = 0;
    MeshRotationPaused      = 0;
    MeshRotation            = 0.0f;
    LastRotationTick        = 0;

    OldWindowX = OldWindowY = 0;
}

FxPlayerApp::~FxPlayerApp()
{
}



SInt    FxPlayerApp::Run()
{   

    //loader.SetVerboseParse(GFxLoader::VerboseParse|GFxLoader::VerboseParseShape);
    GPtr<GFxParseControl> pparseControl = *new GFxParseControl();
    pparseControl->SetParseFlags(Settings.VerboseParse ? GFxParseControl::VerboseParse : 0);
    Loader.SetParseControl(pparseControl);
    
    GPtr<GFxFileOpener> pfileOpener = *new Fx3DDemoFileOpener;
    Loader.SetFileOpener(pfileOpener); 

    GPtr<GFxFSCommandHandler> pcommadHandler = *new Fx3DDemoFSCommandHandler;
    Loader.SetFSCommandHandler(pcommadHandler);


    // Setup window and create renderer.
    if (!SetupWindow(FXPLAYER_APP_TITLE, 800, 600))
        return 1;

    // Create renderer      
    if (pRenderer = *FXPLAYER_RENDER::CreateRenderer())
    {
#ifdef  FXPLAYER_RENDER_DIRECT3D
        pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
#else
        pRenderer->SetDependentVideoMode(hWND);
#endif
    }
    else
    {
        return 1;
    }

    // Set renderer on loader so that it is also applied to all children.
    pRenderConfig = *new GFxRenderConfig(pRenderer);
    Loader.SetRenderConfig(pRenderConfig);

    
    // Load the bottom movie.
    {
        GPtr<FxPlayStream> pmainMovie = *new FxPlayStream(this, Loader, Settings.FileName);

        if (!pmainMovie || !pmainMovie->IsValid())
        {
            // Must have main movie to start
            return 1;
        }
        pmainMovie->SetRenderConfig(pRenderConfig);
     //   pmainMovie->GetMovie()->SetFSCommandCallback(&FsCallback);
        pmainMovie->GetMovie()->SetUserData(pmainMovie.GetPtr());

        // No buffers, just viewport.
        pmainMovie->SetViewport(0,0, 800, 600);     
        // 2x more advances per frame, for smoother animation and UI control
        pmainMovie->SetFrameFrequency(0.5f); 

        // Add to list, from now we go.
        BottomMovies.push_back(pmainMovie);
    }



    // FPS logging
    UInt32  ticks;
    UInt32  startTicks      = timeGetTime();    
    int     frameCounter    = 0;
    int     lasLoggedFps    = startTicks;

    // Always fill speed
    //Settings.MeasurePerformance = 1;
    

    while (!QuitFlag)
    {       
        ticks = timeGetTime();      

        // Check auto exit timeout counter.
        if ((Settings.ExitTimeout > 0) &&
            (ticks - startTicks > (UInt32) (Settings.ExitTimeout * 1000)) ) 
            break;
        
        // Process messages and exit if necessary.
        if (!ProcessMessages())
            break;

        if (::IsIconic(hWND))
        {
            ::Sleep(10);
            continue;
        }
        
        // *** Advance/Render movie

        if (pRenderer)
        {
            // This is technically necessary only for D3D
            FXPLAYER_RENDER::DisplayStatus status = pRenderer->CheckDisplayStatus();
            if (status == FXPLAYER_RENDER::DisplayStatus_Unavailable)
            {
                ::Sleep(10);
                continue;
            }
            if (status == FXPLAYER_RENDER::DisplayStatus_NeedsReset)
            {
                // Need to release RT Textures first                
                if (pTextureMovie)              
                    pTextureMovie->ReleaseBuffers();
                
                if (RecreateRenderer())
                {
                    if (pTextureMovie)
                        pTextureMovie->CreateBuffers(512,512, 1);
                }
            }
        }

        // Clear the backbuffer and the zbuffer
        pDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 
                        D3DCOLOR_XRGB(0,0,255), 1.0f, 0 );

        
        UInt i;
        
        // Display bottom movies
        if (HUDWireframe)
            pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

        for (i=0; i<BottomMovies.size(); i++)
        {
            BottomMovies[i]->NotifyMouseState(xMouse, yMouse, MouseButtons);
            BottomMovies[i]->AdvanceAndRender(ticks);
        }

        if (HUDWireframe)
            pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

        // Update texture movie
        if (pTextureMovie)
        {
            if (MovieWireframe)
                pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

            pTextureMovie->AdvanceAndRender(ticks);

            if (MovieWireframe)
                pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        }

        
        // Render animation to a renderer texture   
        frameCounter++;

        
        // Draw 3D Mesh
        if (ModelShown)
        {
            if (MeshWireframe)
                pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

            RenderMesh(ticks);

            if (MeshWireframe)
                pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        }

        // Display top movies
        if (HUDWireframe)
            pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
        for (i=0; i<TopMovies.size(); i++)
        {
            TopMovies[i]->NotifyMouseState(xMouse, yMouse, MouseButtons);
            TopMovies[i]->AdvanceAndRender(ticks);
        }
        if (HUDWireframe)
            pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);


        // Flip buffers to display the scene
        PresentFrame();

        
        // Flush the draw driver, this synchronizes input and rendering, avoiding swimming
        if (pDevice)
        {           
            IDirect3DQuery9 *pquery = 0;

            if (!(FAILED(pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &pquery))))
            {
                HRESULT hres;
                BOOL    data[1] = {0};
                pquery->Issue(D3DISSUE_END);            
                do  {
                    hres = pquery->GetData(data, sizeof(BOOL), D3DGETDATA_FLUSH);
                } while (hres == S_FALSE);          
                pquery->Release();
            }
        }

        if (Settings.MeasurePerformance == 0)
        {
            // Don't hog the CPU.
            MSG winMsg;
            if (!PeekMessage(&winMsg,0,0,0,0))
                ::Sleep(Settings.SleepDelay);
        }
        else
        {
            // Log the frame rate every second or so.
            if (ticks - lasLoggedFps > 1000)
            {
                float   delta = (ticks - lasLoggedFps) / 1000.f;

                char buff[512];
                
                if (delta > 0)
                    gfc_sprintf(buff, 512, FXPLAYER_APP_TITLE " (fps:%3.1f)", frameCounter / delta);
                else                        
                    gfc_sprintf(buff, 512, FXPLAYER_APP_TITLE " (fps:*inf*)\n");
                SetWindowTitle(buff);

                lasLoggedFps = ticks;
                frameCounter = 0;
                //advanceCounter = 0;
            }
        }
        
        RemovedMovies.clear();

        // If we're reached the end of the movie, exit.
        if (!Settings.DoLoop &&
            (pTextureMovie->GetMovie()->GetCurrentFrame() + 1 == 
              pTextureMovie->GetMovie()->GetFrameCount()) )
            break;          
    }


    // Release 3D data
    CleanupGeometry();

    pTextureMovie = 0;
    //pMovie2 = 0;

    TopMovies.clear();
    BottomMovies.clear();

    return 0;
}

void    FxPlayerApp::OnKey(KeyCode keyCode, unsigned char asciiCode, unsigned int wcharCode, 
                           unsigned int mods, bool downFlag) 
{   
    GUNUSED4(mods, wcharCode, downFlag, asciiCode);

    //GFxLog* plog = Loader.GetLog();
    bool    ctrl = (::GetKeyState(VK_LCONTROL)<0) ||
                   (::GetKeyState(VK_RCONTROL)<0);

    // Escape quits the app
    if (keyCode == VK_ESCAPE)
    {
        QuitFlag = 1;
        return;
    }

    // Handle Ctrl-Key combinations
    if (ctrl && downFlag)
    {

        switch(keyCode)
        {
            case 'Q':
                QuitFlag = 1;
                return;                     
                
            case 'A':  
                {

                switch(Settings.AAMode)
                {
                case FxPlayerSettings::AAMode_None:
                    Settings.AAMode = FxPlayerSettings::AAMode_EdgeAA;
                    break;
                case FxPlayerSettings::AAMode_EdgeAA:
                    Settings.AAMode = FxPlayerSettings::AAMode_FSAA;
                    break;
                case FxPlayerSettings::AAMode_FSAA:
                    Settings.AAMode = FxPlayerSettings::AAMode_None;
                    break;
                }
                
                UInt32 renderFlags = pRenderConfig->GetRenderFlags() & ~GFxRenderConfig::RF_EdgeAA;
                if (Settings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
                    renderFlags |= GFxRenderConfig::RF_EdgeAA;
                pRenderConfig->SetRenderFlags(renderFlags);
                
                goto restore_video_mode_AA;
                
                break;
                }
            case 'U':
            // Toggle full-screen.
                if (pRenderer)
                {   
            restore_video_mode:
                    FullScreen = !FullScreen;
            restore_video_mode_AA:

                    RECT r;
                    ::GetWindowRect(hWND, &r);
                    SInt    x = r.left, y = r.top;
                    SInt    w = 800, h = 600; // always 8x6

                    if (keyCode != 'A')
                    {
                        if (FullScreen)
                        {
                            // Switching to full-screen: save window size & location.
                            OldWindowX      = r.left;
                            OldWindowY      = r.top;
                            x = y = 0;
                        }
                        else
                        {
                            // Switching back to window.
                            x = OldWindowX;
                            y = OldWindowY;
                        }
                    }               
                    
                    FSAntialias = (Settings.AAMode == FxPlayerSettings::AAMode_FSAA);     
                    if (FSAntialias)
                    {
                        PresentParams.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
                        PresentParams.SwapEffect      = D3DSWAPEFFECT_DISCARD; // Discard required
                    }
                    else
                    {
                        PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
                        PresentParams.SwapEffect      = FullScreen ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
                    }

                    pRenderer->ResetVideoMode();

                    // Need to release RT Textures first        
                    if (pTextureMovie)
                        pTextureMovie->ReleaseBuffers();

                    // New window mode, location & size.
                    if (!ConfigureWindow(x, y, w, h, FullScreen))
                    {
                        // If mode change failed, switch back
                        if (keyCode != 'A')
                            goto restore_video_mode;
                        else
                        {
                            FSAntialias = 0;
                            goto restore_video_mode_AA;
                        }
                    }

                    // Re-configure our renderer-depended state.
                    pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
                    if (pTextureMovie)
                        pTextureMovie->CreateBuffers(512,512, 1);
        
                }
                break;


            case 'W':
                HUDWireframe = !HUDWireframe;
                break;

        } // switch(keyCode)


    } // if (ctrl)

    else
    { // if (!ctrl)
        
        // Inform the player about keystroke
        if (!ctrl)
            KeyEvent(keyCode, downFlag);
    }
}



// Helper used to convert key codes and route them to GFxPlayer
void    FxPlayerApp::KeyEvent(UInt key, bool down)
{
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

                // @@ TODO fill this out some more
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
        if (pTextureMovie)
        {
            GFxKeyEvent event(down ? GFxEvent::KeyDown : GFxKeyEvent::KeyUp, c);
            pTextureMovie->GetMovie()->HandleEvent(event);  
        }           
    }
}



void    FxPlayerApp::OnMouseButton(unsigned int button, bool downFlag, int x, int y, unsigned int mods)
{
    GUNUSED3(x,y,mods);
    // Update mouse state
    if (downFlag)
        MouseButtons |= 1 << button;
    else
        MouseButtons &= ~(1 << button); 
}

void    FxPlayerApp::OnMouseMove(int x, int y, int unsigned mods)
{ 
    GUNUSED(mods);
    xMouse = x;
    yMouse = y;
    // Used by NotifyMouseState in the main loop    
}


// *** Static Callbacks


// Helper - finds a movie playing based on filename (or none 0)
// Considers bottom and top layers + texture
FxPlayStream*   FxPlayerApp::FindMovieStream(const char *pfilename)
{
    UInt i;

    // Search bottom
    for (i=0; i<BottomMovies.size(); i++)
        if (!gfc_stricmp(BottomMovies[i]->GetFileName(), pfilename))
            return BottomMovies[i];
    // Search top
    for (i=0; i<TopMovies.size(); i++)          
        if (!gfc_stricmp(TopMovies[i]->GetFileName(), pfilename))
            return TopMovies[i];

    // And check texture movie
    if (pTextureMovie && pTextureMovie->GetFileName())
        if (!gfc_stricmp(pTextureMovie->GetFileName(), pfilename))
            return pTextureMovie;
    return 0;               
}
    
            
    
// Fx3DDemoFSCommandHandler

// For handling notification callbacks from ActionScript.
void    Fx3DDemoFSCommandHandler::Callback(GFxMovieView* pmovie, const char* command, const char* args)
{
    FxPlayStream*   pstream = (FxPlayStream*) pmovie->GetUserData();    
    FxPlayerApp*    papp    = pstream ? pstream->GetApp() : 0;
    if (!papp || !command)
        return;

    /*
    ::OutputDebugString("FsCmd: ");
    ::OutputDebugString(command);
    if (args && args[0])
    {
        ::OutputDebugString(" ");
        ::OutputDebugString(args);
    }
    ::OutputDebugString("\n");
    */

    // Process commands
    if (!strcmp(command, "loadFlashBottom") || !strcmp(command, "loadFlashTop"))
    {
        // Flash loading
        if (!args)
            return;

        GPtr<FxPlayStream> pmovie = *new FxPlayStream(papp, papp->Loader, args);
        if (!pmovie || !pmovie->IsValid())
        {
            // No movie
            char msg[MAX_PATH];                 
            gfc_sprintf(msg, MAX_PATH, "Could not load the SWF movie file: %s", args);
            MessageBox(NULL, msg, "GFxPlayer.exe", MB_OK);
            return;
        }

        pmovie->SetRenderConfig(papp->pRenderConfig);
       // pmovie->GetMovie()->SetFSCommandCallback(&FsCallback);
        pmovie->GetMovie()->SetUserData(pmovie.GetPtr());

        //pmovie->GetMovie()->SetVerboseAction(1);

        // No buffers, just viewport.
        pmovie->SetViewport(0,0, 800, 600);     
        // 2x more advances per frame, for smoother animation and UI control.
        pmovie->SetFrameFrequency(0.5f); 

        // Add to list, from now we go.
        if (!strcmp(command, "loadFlashBottom"))
            papp->BottomMovies.push_back(pmovie);
        else
            papp->TopMovies.push_back(pmovie);

    }


    else if (!strcmp(command, "unloadFlashBottom") ||
            !strcmp(command, "unloadFlashTop") ||
            !strcmp(command, "unloadFlash") )
    {
        if (!args)
        {
            // No argument -> release all movies in a category
            if (!strcmp(command, "unloadFlashBottom"))
                papp->BottomMovies.clear();
            else if (!strcmp(command, "unloadFlashTop"))
                papp->TopMovies.clear();
            return;
        }

        // Unload a specific file
        if (!strcmp(command, "unloadFlashBottom") || !strcmp(command, "unloadFlash"))
        {
            for (UInt i=0; i<papp->BottomMovies.size(); i++)
                if (!gfc_stricmp(papp->BottomMovies[i]->GetFileName(), args))
                {
                    papp->RemovedMovies.push_back(papp->BottomMovies[i]);
                    papp->BottomMovies.remove(i);
                    return;
                }
            // if not "unloadFlash" bail
            if (strcmp(command, "unloadFlash"))
                return;
        }
        
        // Must be "unloadFlash" or "unloadFlashTop"        
        for (UInt i=0; i<papp->TopMovies.size(); i++)
            if (!gfc_stricmp(papp->TopMovies[i]->GetFileName(), args))
            {
                papp->RemovedMovies.push_back(papp->TopMovies[i]);
                papp->TopMovies.remove(i);
                return;
            }

    }
    else if (!strcmp(command, "showModel"))
    {
        papp->ModelShown = 1;
    }
    else if (!strcmp(command, "hideModel"))
    {
        papp->ModelShown = 0;
    }
    else if (!strcmp(command, "exit"))
    {
        papp->QuitFlag = 1;
    }
    else if (!strcmp(command, "movieSelect"))
    {
        if (!args)
        {
            // Unload
            if (papp->pTextureMovie)
                papp->pTextureMovie = 0;
            return;         
        }
        
        // Same movie? leave playing
        if (papp->pTextureMovie && papp->pTextureMovie->GetFileName())
            if (!gfc_stricmp(papp->pTextureMovie->GetFileName(), args))
                return;

        // Store old texture for older substitution check,
        // but no need to AddRef
        GPtr<FxPlayStream> poldMovie    = papp->pTextureMovie;
        //LPDIRECT3DTEXTURE9 poldTexture  =
        //    papp->pTextureMovie ? papp->pTextureMovie->GetRenderTexture() : 0;
        
        // Initialize movie stream.
        papp->pTextureMovie = *new FxPlayStream(papp, papp->Loader, args);
        if (!papp->pTextureMovie || !papp->pTextureMovie->IsValid())
        {
            papp->pTextureMovie = poldMovie;
            char msg[MAX_PATH];                 
            gfc_sprintf(msg, MAX_PATH, "Could not load the SWF movie file: %s", args);
            MessageBox(NULL, msg, "GFxPlayer.exe", MB_OK);
            return;
        }

        // Renderer 
        papp->pTextureMovie->SetRenderConfig(papp->pRenderConfig);
        // Need separate buffers for a rendered cube
        if (!papp->pTextureMovie->CreateBuffers(512,512, 1))
        {
            papp->pTextureMovie = poldMovie;

            char msg[MAX_PATH];                 
            gfc_sprintf(msg, MAX_PATH, "Could not create SWF movie file buffers: %s", args);
            MessageBox(NULL, msg, "GFxPlayer.exe", MB_OK);
            return;
        }

        papp->RemovedMovies.push_back(poldMovie);

        // Action       
        // FsCommand ? no callbacks on texture movies
        // pTextureMovie->GetMovie()->SetFSCommandCallback(&FsCallback);
    }

    else if (!strcmp(command, "modelSelect"))
    {
        // This should select a mesh
        if (!args || !args[0])
            return;

        if (!strcmp(args, "cube"))
        {
            papp->UseCubeModel = 1;
        }
        else
        {       
            // If mesh already loaded, select it
            SInt index = papp->FindMeshIndex(args);
            if (index != -1)
            {
                papp->UseCubeModel = 0;
                papp->ActiveMesh = index;
                return;
            }

            // Load the mesh. If mesh failed to load either stay using old mesh,
            // or switch to a cube model.
            FxPlayerApp::MeshDesc mesh;
            if (!mesh.LoadMesh(papp, args))
            {
                // Leave things the way they were
                return;
            }

            // Mesh ok, add it ans switch to it.
            papp->ActiveMesh = (SInt)papp->LoadedMeshes.size();
            papp->LoadedMeshes.push_back(mesh);
            papp->UseCubeModel = 0;
        }

    }
    else if (!strcmp(command, "modelWireframe"))
    {
        bool state = 1;
        if (args && (!gfc_stricmp(args,"false") || !strcmp(args,"0")))
            state = 0;
        papp->MeshWireframe = state;
    }
    else if (!strcmp(command, "movieWireframe"))
    {
        bool state = 1;
        if (args && (!gfc_stricmp(args,"false") || !strcmp(args,"0")))
            state = 0;
        papp->MovieWireframe = state;
    }

    else
    {
        // Handle a complex command that has a colon in it
        char        commandOp[512];
        const char* commandArg = 0;
        if (strlen(command) > 512)
            return;

        // Command with a colon component
        UInt i;
        for (i=0; command[i] != 0; i++)
        {
            if (command[i] == ':')
            {
                memcpy(commandOp, command, i);
                commandOp[i] = 0;
                commandArg = command + i + 1;
                break;
            }
        }
        // Colon not found ? bail
        if (command[i] != ':')
            return;

        // Process complex command
        if (!strcmp(commandOp, "gotoAndPlay") && commandArg[0])
        {
            // Find the clip that this applies to
            FxPlayStream *pmovie = papp->FindMovieStream(commandArg);
            if (!pmovie || !args || !args[0])
                return;

            // Separate a function name from frame number
            char            argPath[512 + 20];
            const char*     argFrame = 0;
            if (strlen(args) > 512)
                return;
            argPath[0] = 0;

            // Command with a comma component
            UInt i;
            for (i=0; args[i] != 0; i++)
            {
                if (args[i] == ':')
                {
                    memcpy(argPath, args, i);
                    argPath[i] = 0;
                    argFrame = args + i + 1;
                    break;
                }
            }
                        
            if (args[i] == ':')
            {
                // path was found
                
                // does not work right due to path lookup bug in G-SWF
                gfc_strcat(argPath, 532, ".gotoAndPlay");
                pmovie->GetMovie()->Invoke(argPath, "%s", argFrame);
            }
            else
            {           
                // path not found
                gfc_strcpy(argPath, 532, "gotoAndPlay");
                pmovie->GetMovie()->Invoke(argPath, "%s", args);
            }

        // Done handling gotoAndPlay
        return;
        }

        else if (!strcmp(commandOp, "sliderBeginDrag") && commandArg[0])
        {
            // Begin dragging the rotation/playback slider (pause animation)
            if (!strcmp(commandArg, "sliderRotation"))
            {
                // Pause the 3D mesh rotation and jump to the drag location
                papp->MeshRotationDragging      = 1;
            }
            else if (!strcmp(commandArg, "sliderPlaytrack"))
            {
                // Pause the flash animation, giving user control.
                papp->MovieAnimationDragging    = 1;
            }
            if (args && strlen(args)>0)
                goto updatepos;
        }
        
        else if (!strcmp(commandOp, "sliderEndDrag") && commandArg[0])
        {
            // Stop dragging the rotation/playback slider (resume animation)
            if (!strcmp(commandArg, "sliderRotation"))
            {
                // Resume the 3D mesh rotation
                papp->MeshRotationDragging      = 0;
            }
            else if (!strcmp(commandArg, "sliderPlaytrack"))
            {
                // Resume the flash animation, giving user control.
                papp->MovieAnimationDragging    = 0;

                if (papp->pTextureMovie && !papp->MovieAnimationPaused)
                {
                    GFxMovieView*   pv = papp->pTextureMovie->GetMovie();
                    if (pv)
                        pv->SetPlayState( papp->MovieAnimationPaused ? 
                                          GFxMovie::Stopped : GFxMovie::Playing );
                }
            }
        }
        
        else if (!strcmp(commandOp, "sliderGripPos") && commandArg[0])
        {

        updatepos:
            Float pos = (float)atoi(args);

            // Set animation/rotation to a given place
            if (!strcmp(commandArg, "sliderRotation"))
            {
                papp->MeshRotation = (pos/100) * GFC_2PI;
            }
            else if (!strcmp(commandArg, "sliderPlaytrack"))
            {
                if (papp->pTextureMovie)
                {
                    GFxMovieView*   pv = papp->pTextureMovie->GetMovie();
                    GFxMovieDef*    pd = pv ? pv->GetMovieDef() : 0;
                    if (pd)
                    {
                        Float count = (float)pd->GetFrameCount();
                        Float frame = (pos/100) * count;
                        pv->GotoFrame(UInt(frame));
                        papp->pTextureMovie->ForceRender = 1;
                    }
                }
            }
        }

        // Play/pause
        else if (!strcmp(commandOp, "buttonPlay") && commandArg[0])
        {
            bool state = 1;

            if (args && (!gfc_stricmp(args,"false") || !strcmp(args,"0")))
                state = 0;

            // Set animation/rotation to a given place
            if (!strcmp(commandArg, "modelRotation"))
            {
                papp->MeshRotationPaused    = !state;
            }
            else if (!strcmp(commandArg, "moviePlayback"))
            {
                papp->MovieAnimationPaused  = !state;

                if (papp->pTextureMovie)
                {
                    GFxMovieView*   pv = papp->pTextureMovie->GetMovie();
                    if (pv)
                        pv->SetPlayState( state ? GFxMovie::Playing : GFxMovie::Stopped );
                }
            }
        }

    }

    // end FxPlayerApp::FsCallback
}


// Override to initialize OpenGL viewport
bool    FxPlayerApp::InitRenderer()
{
    // Turn off culling
    pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    // Turn off D3D lighting, since we are providing our own vertex colors
    pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

    return InitGeometry();  
}

void FxPlayerApp::PrepareRendererForFrame()
{
    // Disable depth testing.
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
}




// Mesh loading/cleanup
bool    FxPlayerApp::InitGeometry()
{

    // *** Init Cube

    if (!pCubeVertexBuffer)
    {
        pDevice->CreateVertexBuffer( 24*sizeof(Vertex),0, Vertex::FVF,
                                 D3DPOOL_MANAGED, &pCubeVertexBuffer, NULL );
        void *pVertices = NULL;
        pCubeVertexBuffer->Lock( 0, sizeof(g_cubeVertices), (void**)&pVertices, 0 );
        memcpy( pVertices, g_cubeVertices, sizeof(g_cubeVertices) );
        pCubeVertexBuffer->Unlock();
    }
    
    // *** Init mesh
    // No need to init mesh here, it is done explicitly by the swf.
    
    return 1;
}


// Loads mesh from a filename
bool    FxPlayerApp::MeshDesc::LoadMesh(FxPlayerApp *papp, const char *pfilename)
{
    if (!pfilename)
        return 0;
    gfc_strcpy(FileName, 256, pfilename);

    LPDIRECT3DDEVICE9   pdevice = papp->pDevice;
    LPD3DXBUFFER        pD3DXMtrlBuffer;

    const char* meshname= pfilename;
    const char* meshpic = meshname;

    char meshfile[MAX_PATH];
    char meshtexture[MAX_PATH]; 
    gfc_sprintf(meshfile, MAX_PATH, "%s.x", meshname);
    gfc_sprintf(meshtexture, MAX_PATH, "%s_texture.bmp", meshpic);



    // Load the mesh from the specified file
    if( FAILED( D3DXLoadMeshFromX( meshfile, D3DXMESH_SYSTEMMEM, 
                                   pdevice, NULL, 
                                   &pD3DXMtrlBuffer, NULL, &dwNumMaterials, 
                                   &pMesh ) ) )
    {
        char msg[MAX_PATH];
        gfc_sprintf(msg, MAX_PATH, "Could not find the mesh: %s", meshfile);
        MessageBox(NULL, msg, "GFxPlayer.exe", MB_OK);
        return 0;
    }

    // We need to extract the material properties and texture names from the 
    // pD3DXMtrlBuffer
    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    pMeshMaterials      = (D3DMATERIAL9*) GALLOC(sizeof(D3DMATERIAL9)*dwNumMaterials);
    pMeshTextures       = (LPDIRECT3DTEXTURE9*) GALLOC(sizeof(LPDIRECT3DTEXTURE9)*dwNumMaterials);
    pMovieTextureTags   = (UByte*) GALLOC(sizeof(UByte)*dwNumMaterials);

    memset(pMovieTextureTags, 0, sizeof(UByte)*dwNumMaterials);

    for( DWORD i=0; i<dwNumMaterials; i++ )
    {
        // Copy the material
        pMeshMaterials[i] = d3dxMaterials[i].MatD3D;
        
        // Set the ambient color for the material (D3DX does not do this)
        pMeshMaterials[i].Ambient = pMeshMaterials[i].Diffuse;

        pMeshTextures[i] = NULL;
        if( !d3dxMaterials[i].pTextureFilename || lstrlen(d3dxMaterials[i].pTextureFilename) == 0 )
            continue;

        // If this is a tiger texture, substitute it with a render target
        if (gfc_stricmp(d3dxMaterials[i].pTextureFilename, meshtexture) == 0)
        {
            // Tag as movie texture
            // We don't do this coz texture may be recreated: papp->pTextureMovie->GetRenderTexture()
            pMeshTextures[i]        = 0; 
            pMovieTextureTags[i]    = 1;                
            continue;
        }

        // Create the texture
        if( FAILED( D3DXCreateTextureFromFile( pdevice, 
                                               d3dxMaterials[i].pTextureFilename, 
                                               &pMeshTextures[i] ) ) )
        {
            char msg[MAX_PATH];                 
            gfc_sprintf(msg, MAX_PATH, "Could not find the texture: %s", d3dxMaterials[i].pTextureFilename);
            MessageBox(NULL, msg, "GFxPlayer.exe", MB_OK);
        }
    }

    // Done with the material buffer
    pD3DXMtrlBuffer->Release();

    return 1;


}


void    FxPlayerApp::CleanupGeometry()
{

    if (pCubeVertexBuffer)
    {
        pCubeVertexBuffer->Release();
        pCubeVertexBuffer = 0;
    }

    // Release all of these meshes
    for (UInt i=0; i<LoadedMeshes.size(); i++)
    {
        MeshDesc &mesh = LoadedMeshes[i];

        if (mesh.pMeshMaterials)
        {
            GFREE(mesh.pMeshMaterials);
            mesh.pMeshMaterials = 0;
        }
        if( mesh.pMeshTextures )
        {
            for( DWORD j = 0; j < mesh.dwNumMaterials; j++ )
            {
                if( mesh.pMeshTextures[j] )
                    mesh.pMeshTextures[j]->Release();
            }
            GFREE(mesh.pMeshTextures);
            mesh.pMeshTextures = 0;
        }
        if ( mesh.pMovieTextureTags)
        {
            GFREE(mesh.pMovieTextureTags);
            mesh.pMovieTextureTags = 0;
        }
        if( mesh.pMesh != NULL )
        {
            mesh.pMesh->Release();
            mesh.pMesh = 0;
        }
    }
    
}

// Rendering
void    FxPlayerApp::RenderMesh(UInt ticks)
{
    if (ticks == -1)
        ticks = timeGetTime();
    
    // 1/10 revolution per second
    float dt = 0.0f;
    if (IsMeshRotating())
    {
        float t  = (float)((double)fmod((double)ticks, 20000.0) / 20000.0) * GFC_2PI;
        float lt = (float)((double)fmod((double)LastRotationTick, 20000.0) / 20000.0) * GFC_2PI;
        dt = t - lt;
    }

    LastRotationTick    = ticks;
    MeshRotation        += dt;

    if (MeshRotation > GFC_2PI)
        MeshRotation -= GFC_2PI;
    if (MeshRotation < 0.0f)
        MeshRotation += GFC_2PI;

    if (UseCubeModel || !LoadedMeshes.size())
    {
        // Setup the world, view, and projection matrices
        SetupMatrices();

        pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

        pDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        pDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        pDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
        //pDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);


        D3DVIEWPORT9 vp;
        vp.X        = 0;
        vp.Y        = 0;
        vp.Width    = Width;
        vp.Height   = Height;
        vp.MinZ     = 0.0f;
        vp.MaxZ     = 1.0f;
        pDevice->SetViewport(&vp);

        pDevice->BeginScene();

        // Blending 
        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
        pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
        pDevice->SetRenderState(D3DRS_BLENDFACTOR, 0x60606060);

        // Don't need Z for blending
        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

        pDevice->SetTexture( 0, pTextureMovie ? pTextureMovie->GetRenderTexture() : 0 );
        pDevice->SetStreamSource( 0, pCubeVertexBuffer, 0, sizeof(Vertex) );
        pDevice->SetFVF( Vertex::FVF );

        pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  0, 2 );
        pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  4, 2 );
        pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  8, 2 );

        pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 12, 2 );
        pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 16, 2 );
        pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 20, 2 );

        // Unbind render texture, so it can be updated.
        pDevice->SetTexture( 0, 0 );

        pDevice->EndScene();
    }
    else
    {
        // *** Draw model

        if (ActiveMesh >= (SInt)LoadedMeshes.size())
            return;

        // Enable X
        pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
        pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

        pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        pDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE );

        MeshDesc &mesh = LoadedMeshes[ActiveMesh];

        // Begin the scene
        if( SUCCEEDED( pDevice->BeginScene() ) )
        {
            LPDIRECT3DTEXTURE9 prenderTexture = pTextureMovie ? pTextureMovie->GetRenderTexture() : 0;          

            SetupLights();

            // Setup the world, view, and projection matrices
            SetupMatrices();

            bool hasAlpha = 0;

            // Meshes are divided into subsets, one for each material. Render them in a loop
            for( DWORD i=0; i<mesh.dwNumMaterials; i++ )
            {
                D3DMATERIAL9* pmat = &mesh.pMeshMaterials[i];
                if (pmat->Diffuse.a==1.0)
                {
                    // Set the material and texture for this subset
                    pDevice->SetMaterial( pmat );

                    if (mesh.pMovieTextureTags[i])
                        pDevice->SetTexture(0, prenderTexture);
                    else                
                        pDevice->SetTexture(0, mesh.pMeshTextures[i]);
                    // Draw the mesh subset
                    mesh.pMesh->DrawSubset( i );
                }
                else 
                {
                    hasAlpha = 1;
                }
            }

            if (hasAlpha)
            {
                // Blending 
                pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
                pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
                pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
                pDevice->SetRenderState(D3DRS_BLENDFACTOR, 0x60606060);
                // Don't need Z for blending
                pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

                // Meshes are divided into subsets, one for each material. Render them in a loop
                for( DWORD i=0; i<mesh.dwNumMaterials; i++ )
                {
                    D3DMATERIAL9* pmat = &mesh.pMeshMaterials[i];
                    if (pmat->Diffuse.a!=1.0)
                    {
                        // Set the material and texture for this subset
                        pDevice->SetMaterial( &mesh.pMeshMaterials[i] );

                        if (mesh.pMovieTextureTags[i])
                            pDevice->SetTexture(0, prenderTexture);
                        else                
                            pDevice->SetTexture(0, mesh.pMeshTextures[i]);
                        
                        // Draw the mesh subset
                        mesh.pMesh->DrawSubset( i );
                    }
                }

                pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            }

            // End the scene
            pDevice->EndScene();
        }

        // Disable lighting for the rest
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);  
    }

}



void    FxPlayerApp::SetupLights()
{
    // Set up a material. The material here just has the diffuse and ambient
    // colors set to yellow. Note that only one material can be used at a time.
    D3DMATERIAL9 mtrl;
    ZeroMemory( &mtrl, sizeof(D3DMATERIAL9) );
    mtrl.Diffuse.r = mtrl.Ambient.r = 1.0f;
    mtrl.Diffuse.g = mtrl.Ambient.g = 1.0f;
    mtrl.Diffuse.b = mtrl.Ambient.b = 0.0f;
    mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
    pDevice->SetMaterial( &mtrl );

    // Set up a white, directional light, with an oscillating direction.
    // Note that many lights may be active at a time (but each one slows down
    // the rendering of our scene). However, here we are just using one. Also,
    // we need to set the D3DRS_LIGHTING render state to enable lighting
    D3DLIGHT9 light;
    ZeroMemory( &light, sizeof(D3DLIGHT9) );
    light.Type       = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r  = 1.0f;
    light.Diffuse.g  = 1.0f;
    light.Diffuse.b  = 1.0f;
    light.Range      = 10000.0f;
    
    
    //D3DXVECTOR3 vecDir = D3DXVECTOR3(cosf(timeGetTime()/350.0f),
    //                     1.0f,
    //                    sinf(timeGetTime()/350.0f) );

    D3DXVECTOR3 vecDir = D3DXVECTOR3(1.0f, -5.0f, 1.0f);
    D3DXVec3Normalize( (D3DXVECTOR3*)&light.Direction, &vecDir );

    pDevice->SetLight( 0, &light );
    pDevice->LightEnable( 0, TRUE );
    pDevice->SetRenderState( D3DRS_LIGHTING, TRUE );

    //pDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE );
    //pDevice->SetRenderState( D3DRS_SPECULARENABLE, TRUE );

    // Finally, turn on some ambient light.
    pDevice->SetRenderState( D3DRS_AMBIENT, 0x00202020 );
}


void    FxPlayerApp::SetupMatrices()
{

    // For our world matrix, we will just leave it as the identity
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY( &matWorld, MeshRotation ); 
    pDevice->SetTransform( D3DTS_WORLD, &matWorld );

    if (UseCubeModel)
    {   
        // Set up our view matrix. A view matrix can be defined given an eye point,
        // a point to lookat, and a direction for which way is up. Here, we set the
        // eye five units back along the z-axis and up three units, look at the 
        // origin, and define "up" to be in the y-direction.
        //D3DXVECTOR3 vEyePt( 0.0f, 3.0f, -5.0f );
        //D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
        //D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );

        D3DXVECTOR3 vEyePt( 0.0f, 4.0f, -5.0f );
        D3DXVECTOR3 vLookatPt( -1.0f, 0.0f, 0.0f );
        D3DXVECTOR3 vUpVec( -0.1f, 1.0f, 0.0f );

        D3DXMATRIXA16 matView;
        D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
        pDevice->SetTransform( D3DTS_VIEW, &matView );


        D3DXMATRIX matProj;
        D3DXMatrixPerspectiveFovLH( &matProj, D3DXToRadian( 45.0f ), 
                                    640.0f / 480.0f, 0.1f, 100.0f );
        pDevice->SetTransform( D3DTS_PROJECTION, &matProj );
    }

    else
    {
         // Set up our view matrix. A view matrix can be defined given an eye point,
        // a point to lookat, and a direction for which way is up. Here, we set the
        // eye five units back along the z-axis and up three units, look at the 
        // origin, and define "up" to be in the y-direction.
        D3DXVECTOR3 vEyePt( 0.0f, 4.0f, -7.0f );
        D3DXVECTOR3 vLookatPt( -1.0f, 0.0f, 0.0f );
        D3DXVECTOR3 vUpVec( -0.1f, 1.0f, 0.0f );

        D3DXMATRIXA16 matView;
        D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
        pDevice->SetTransform( D3DTS_VIEW, &matView );  

        // For the projection matrix, we set up a perspective transform (which
        // transforms geometry from 3D view space to 2D viewport space, with
        // a perspective divide making objects smaller in the distance). To build
        // a perspective transform, we need the field of view (1/4 pi is common),
        // the aspect ratio, and the near and far clipping planes (which define at
        // what distances geometry should be no longer be rendered).
        D3DXMATRIXA16 matProj;
        D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f );
        pDevice->SetTransform( D3DTS_PROJECTION, &matProj );
    }


}


SInt    FxPlayerApp::FindMeshIndex(const char* pfilename)
{
    for(UInt i=0; i<LoadedMeshes.size(); i++)
        if (!gfc_stricmp(LoadedMeshes[i].FileName, pfilename))
            return i;
    return -1;
}





// ***** FxPlayerSettings Implementation


FxPlayerSettings::FxPlayerSettings()
{
    // Default values
    Scale               = 1.0f;         
    Antialiased         = 1;
    BitDepth            = 16;
    Background          = 1;
    MeasurePerformance  = 0;    

    VerboseAction       = 0;
    VerboseParse        = 0;

    AAMode              = AAMode_FSAA;

    DoLoop              = 1;

    ExitTimeout         = 0.0f;
    SleepDelay          = 10;   

    FullScreen          = 0;

    // Clear file
    FileName[0]         = 0;
}


// Initializes settings based on the command line.
// Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
bool    FxPlayerSettings::ParseCommandLine(int argc, char *argv[])
{
    for (int arg = 1; arg < argc; arg++)
    {
        if (argv[arg][0] == '/')
        {
            // Looks like an option.

            if (argv[arg][1] == 'f')
            {
                // Full-sceen demo.
                FullScreen = 1;
            }

            if (argv[arg][1] == '?')
            {
                // Help.
                return 0;
            }

            else if (argv[arg][1] == 's')
            {
                // Scale.
                arg++;
                if (arg < argc)                 
                    Scale = GTL::gclamp<float>((float) atof(argv[arg]), 0.01f, 100.f);                  
                else
                {
                    fprintf(stderr, "/s arg must be followed by a scale value\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'a')
            {
                // Set antialiasing on or off.
                arg++;
                if (arg < argc)                 
                    Antialiased = atoi(argv[arg]) ? 1 : 0;                  
                else
                {
                    fprintf(stderr, "/a arg must be followed by 0 or 1 to disable/enable antialiasing\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'b')
            {
                // Set default bit depth.
                arg++;
                if (arg < argc)
                {
                    BitDepth = atoi(argv[arg]);
                    if (BitDepth != 16 && BitDepth != 32)
                    {
                        fprintf(stderr, "Command-line supplied bit depth %d, but it must be 16 or 32", BitDepth);
                        return 0;
                    }
                }
                else
                {
                    fprintf(stderr, "/b arg must be followed by 16 or 32 to set bit depth\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'd')
            {
                // Set a delay
                arg++;
                if (arg < argc)
                {
                    SleepDelay = atoi(argv[arg]);
                }
                else
                {
                    fprintf(stderr, "/d arg must be followed by number of milli-seconds to del in the main loop\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'p')
            {
                // Enable frame-rate/performance logging.
                MeasurePerformance = 1;
            }
            else if (argv[arg][1] == '1')
            {
                // Play once; don't loop.
                DoLoop = 0;
            }

            else if (argv[arg][1] == 't')
            {
                // Set timeout.
                arg++;
                if (arg < argc)                 
                    ExitTimeout = (float) atof(argv[arg]);
                else
                {
                    fprintf(stderr, "/t must be followed by an exit timeout, in seconds\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'v')
            {
                // Be verbose; i.e. print log messages to stdout.           
                if (argv[arg][2] == 'a')
                {
                    // Enable spew re: action.
                    VerboseAction = 1;                  
                }
                else if (argv[arg][2] == 'p')
                {
                    // Enable parse spew.
                    VerboseParse = 1;                   
                }
                // ...
            }
            
        }
        else
        {
            if (argv[arg])
                gfc_strcpy(FileName, 256, argv[arg]);            
        }
    }

    if (!FileName[0])
    {
        // Pick HudBottom by default.
        gfc_strcpy(FileName, 256, "HudBottom.swf");
    }

    return 1;   
}

    
// Brief instructions.
void    FxPlayerSettings::PrintUsage()
{
    /*
    // Not useful, since we are running without console.
    printf(
        "GFxPlayer3Ddemo - a SWF + 3D demo built on the GFxPlayer.\n"
        "\n"
        "Copyright (c) 2005 Scaleform Corp. All Rights Reserved.\n"
        "Contact sales@scaleform.com for licensing information.\n"
        "\n"
        "Usage:        gfxplayer3ddemo [options] [movie_file.swf]\n"        
        "Options:\n"
        "  /?          Display this help info.\n"
        "  /s <factor> Scale the movie window size by the specified factor.\n"
        "  /d <msecs>  Number of milliseconds to delay in player loop.\n"
        "  /a          Turn on full screen antialiasing.\n"
        "  /f          Run in full-screen mode.\n"
        "  /vp         Verbose parse - print SWF parse log.\n"
        "  /va         Verbose Actions - display ActionScript execution log.\n"
        "  /q          Quiet. Do not display errors or trace statements.\n"     
        "  /p          Performance test - run without delay and log FPS.\n"
        "  /ff         Fast forward - run one SWF frame per update.\n"
        "  /1          Play once; exit when/if movie reaches the last frame.\n"     
        "  /t <sec>    Timeout and exit after the specified number of seconds.\n"
        "  /b <bits>   Bit depth of output window (16 or 32, default is 16).\n"
        "\n"
        "Keys:\n"
        "  CTRL W          Toggle HUD wireframe.\n"
        "  CTRL Q,ESC      Quit.\n"
        );
    */
}



// ***** Main function implementation

int main(int argc, char *argv[])
{
    int res = 1;
    {
        FxPlayerApp app;
        if (!app.Settings.ParseCommandLine(argc, argv))
        {
            app.Settings.PrintUsage();
            return 1;
        }

        app.FullScreen = app.Settings.FullScreen;
        app.Antialiased = app.Settings.Antialiased;
        app.FSAntialias = app.Antialiased;
        res = app.Run();    
    }
    
    GMemory::DetectMemoryLeaks();
    return res;
}
