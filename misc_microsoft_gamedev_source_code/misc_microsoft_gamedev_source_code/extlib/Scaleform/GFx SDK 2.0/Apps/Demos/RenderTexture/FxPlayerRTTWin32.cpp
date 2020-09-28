/**********************************************************************

Filename    :   RenderTexture.cpp
Content     :   Sample SWF file player leveraging GFxPlayer API
Created     :   
Authors     :   Michael Antonov, Andrew Reisse
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

// Define this to use Direct3D instead of OpenGL
#define FXPLAYER_RENDER_DIRECT3D

// This option specifies that external image files (DDS, etc) will be
// preloaded instead of loaded on demand during playback.
#define FXPLAYER_PRELOAD_IMAGES

// GFx includes
#include "GFile.h"
#include "GImage.h"
#include "GFxPlayer.h"
#include "GFxLoader.h"
#include "GFxLog.h"
#include "GMath.h"
#include "GImageInfo.h"
#include "GFxImageResource.h"
#include "GStd.h"
// Adds the word "Debug" to the application
// title if in debug build mode
#ifdef GFC_BUILD_DEBUG
#define GFC_DEBUG_STRING    " " GFC_BUILD_STRING
#else
#define GFC_DEBUG_STRING    
#endif

#ifdef  FXPLAYER_RENDER_DIRECT3D
// Direct3D application class
#include "Direct3DWin32App.h"
#include "GRendererD3D9.h"
#define FXPLAYER_RENDER GRendererD3D9
#define FXPLAYER_APP    Direct3DWin32App
// Window name
#define     FXPLAYER_APP_TITLE  "Scaleform GFx Render Texture Sample D3D9 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING

#include <d3dx9.h>

#else
// OpenGL application class
#include "OpenGLWin32App.h"
#include "GRendererOGL.h"
#define FXPLAYER_RENDER GRendererOGL
#define FXPLAYER_APP    OpenGLWin32App
// Window name
#define     FXPLAYER_APP_TITLE  "Scaleform GFx Render Texture Sample OpenGL v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
#endif

#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <zmouse.h> // for WHEEL_DELTA macro


#define     GFC_2PI             (2*3.1415926f)


// ***** SysAllocator - Sample custom allocator for GFxPlayer

// Enable the memory tracking in the GFxPlayer
// Comment this line out to prevent memory tracking
#define GFC_MEMORY_TRACKSIZES

#include "GAllocator.h"

    // If not using our allocator, memory tracking is only available
    // if GMemory was compiled with GFC_MEMORY_TRACKSIZES.
    #ifdef GFC_MEMORY_TRACKSIZES
        #define FXPLAYER_MEMORY_TRACKSIZES
    #endif


// ***** Player Settings class

// Settings class stores playback settings determined
// based on the command-line parameters
class   FxPlayerSettings
{

public:

    enum AAModeType
    {
        AAMode_None,        // No anti-aliasing is used.
        AAMode_EdgeAA,      // Edge AA is used (if supported by GRenderer).
        AAMode_FSAA         // HW Full-screen AA is used (if supported by device).
    };


    UInt        BitDepth;
    Float       ScaleX, ScaleY;
    Float       TexLodBias;
    AAModeType  AAMode;
    bool        Background;
    bool        MeasurePerformance; 
    bool        FullScreen;

    enum    HudStateType
    {
        Hud_Hidden,
        Hud_Stats,
        Hud_Help
    };

    // Display Hud at startup
    HudStateType    HudState;

    // Verbose options
    bool    VerboseParse;
    bool    VerboseParseShape;
    bool    VerboseParseAction;
    bool    VerboseAction;
    bool    Quiet;
    bool    NoActionErrors;
    
    // Rendering state
    bool    DoLoop;
    bool    DoRender;
    bool    DoSound;

    // Set to play movie as fast as possible
    bool    FastForward;

    Float   ExitTimeout;    

    // PlaybackFile
    char    FileName[256];


    FxPlayerSettings()
    {
        // Default values
        ScaleX = ScaleY     = 1.0f;
        TexLodBias          = -0.5f;
        AAMode              = AAMode_EdgeAA;
        BitDepth            = 32;
        Background          = 1;
        MeasurePerformance  = 0;
        FullScreen          = 0;
        HudState            = Hud_Hidden;

        VerboseParse        = 0;
        VerboseParseShape   = 0;
        VerboseParseAction  = 0;
        VerboseAction       = 0;
        Quiet               = 0;
        NoActionErrors      = 0;

        DoLoop              = 1;
        DoRender            = 1;
        DoSound             = 0;

        FastForward         = 0;

        ExitTimeout         = 0.0f;        

        // Clear file
        FileName[0]         = 0;
    }

    // Initializes settings based on the command line.
    // Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
    bool        ParseCommandLine(int argc, char *argv[]);

    // Displays Playback / Usage information
    static void PrintUsage();
};



// ***** Player Application class

class   FxPlayerApp : public FXPLAYER_APP
{
public:
    typedef FxPlayerSettings::AAModeType AAModeType;

    // Loaded movie data
    GFxLoader           Loader;
    GFxMovieInfo        MovieInfo;
    GPtr<GFxMovieDef>   pMovieDef;
    GPtr<GFxMovieView>  pMovie;
    
    // Movie timing state
    float       SpeedScale;         // Advance speed, def 1.0f
    SInt        FrameCounter;       // Frames rendered, for FPS
    UInt        TessTriangles;      // Tess triangles for log.
    // Time ticks: always rely on a timer, for FPS
    UInt32      TimeStartTicks;     // Ticks during the start of playback
    UInt32      TimeTicks;          // Current ticks
    UInt32      LastLoggedFps;      // Time ticks during last FPS log
    UInt32      NextTicksTime;      // Ticks when next advance should be called.
    // Movie logical ticks: either timer or setting controlled
    UInt32      MovieStartTicks;
    UInt32      MovieLastTicks;
    UInt32      MovieTicks;    
    
    // Renderer we use
	
	GPtr<GFxRenderConfig>   pRenderConfig;
	GPtr<GFxRenderStats>  pRenderStats;
    GPtr<FXPLAYER_RENDER> pRenderer;

    // Selected playback settings
    FxPlayerSettings    Settings;

    // View width and height
    SInt                ViewWidth, ViewHeight;

    // Running antialiased or not (for lines)
    //AAModeType            AAMode;
    // Set if wireframe ins enabled.
    bool                Wireframe;

    // Scale toggle, on by default
    bool                ScaleEnable;
	bool                ClippingEnable;
    // Viewport transformation
    Float               Zoom;
    Float               ZoomStart;
    GPointF             Move;
    GPointF             MoveStart;

    enum TrackingState
    {
        None,
        Zooming,
        Moving,
    };
    TrackingState       MouseTracking;
    SInt                MouseX;
    SInt                MouseY;
    SInt                MouseDownX;
    SInt                MouseDownY;
    bool                ControlKeyDown;

    // This variable is set when the movie is paused in the player.
    bool                Paused;
    // Store playstate when paused, so that we can restore it.
    GFxMovie::PlayState PausedState;

    // Last FPS and stats
    Float               LastFPS;
    GRenderer::Stats    LastStats;
    UInt                LastFrame; // Frame reported by HUD
    // This flag is set when UpdateHudText needs to be called
    bool                NeedHudUpdate;
    // Hud text, blended over the player
    char                HudText[2048];
    
    // Curve error
    Float               CurvePixelError;

    // Width, height during sizing
    SInt                SizeWidth, SizeHeight;
    bool                SizingEntered;

    // Old width and height saved during FullScreen mode
    SInt                OldWindowX, OldWindowY;
    SInt                OldWindowWidth, OldWindowHeight;

#ifdef FXPLAYER_RENDER_DIRECT3D
    GPtr<IDirect3DTexture9>  pRenderTexture;
    GPtr<IDirect3DSurface9>  pStencilSurface;
    GPtr<IDirect3DVertexBuffer9>     pCubeVertexBuffer;
#else
    GLuint                  RenderTextureId;
    GLuint                  RenderTextureFbo;
#endif
    SInt                    RTWidth, RTHeight;
    Float                   MeshRotation;
    UInt                    LastRotationTick;
    bool                    CubeWireframe;

    static FxPlayerApp      *pApp;

    FxPlayerApp();
    ~FxPlayerApp();

    // Called from main() after settings are initialized to execute 
    // most of the program logic. Responsible for setting up the window,
    // loading movies and containing the message loop.
    SInt            Run();

    // Load a new movie from a file and initialize timing
    bool            LoadMovie(char *pfilename);
    

    // Helper function to update HUD.
    // Uses LastFPS and LastStats; those variables must be updated separately.
    void            UpdateHudText();
    // Updates the view size based on the ScaleEnable flag and window size.
    void            UpdateViewSize();

    void            ResetUserMatrix();
    void            UpdateUserMatrix();
    GSizeF          GetMovieScaleSize();

    bool            SetupRTTexture();
    void            SetupMatrices();
    void            RenderMesh();

    // *** Overrides

    // Sizing; by default, re-initalizes the renderer
    virtual void    OnSize(SInt w, SInt h);     
    virtual void    OnSizeEnter(bool enterSize);
    virtual void    OnDropFiles(char *path);

    // Input
    virtual void    OnKey(UInt keyCode, UInt info, bool downFlag);
    virtual void    OnChar(UInt32 wcharCode, UInt info);
    virtual void    OnMouseButton(UInt button, bool downFlag, SInt x, SInt y);
    virtual void    OnMouseWheel(SInt zdelta, SInt x, SInt y);
    virtual void    OnMouseMove(SInt x, SInt y);
    // Override to initialize OpenGL viewport
    virtual bool    InitRenderer();
    virtual void    PrepareRendererForFrame();

    // Helper used to convert key codes and route them to GFxPlayer
    void            KeyEvent(UInt key, UInt info, bool down);


    // *** Static callbacks

	class FxPlayerRTTFileOpener : public GFxFileOpener
	{
	public:
		virtual GFile* OpenFile(const char *pfilename)
		{
			return new GSysFile(pfilename);
		}
	};
	// "fscommand" callback, handles notification callbacks from ActionScript.
	class FxPlayerRTTFSCommandHandler : public GFxFSCommandHandler
	{
	public:
		virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg);
	};

    // event user handler
    static void     GCDECL UserEventHandler(GFxMovieView* movie, const GFxEvent& event, void* puserData);
};

class GFxPlayerLog : public GFxLog
{
public: 
    // We override this function in order to do custom logging.
    virtual void    LogMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
    {
        // Output log to console
#ifdef GFC_OS_WIN32
        char    formatBuff[2048];

        gfc_vsprintf(formatBuff, 2048, pfmt, argList);
        ::OutputDebugStringA(formatBuff);
#else
        vprintf(pfmt, argList);
#endif
    }
};

// ***** Main function implementation

int GCDECL main(int argc, char *argv[])
{
    int res = 1;

    setlocale (LC_ALL, ""); //!AB: need for correct work of towlower/towupper
    {
        FxPlayerApp app;

        if (!app.Settings.ParseCommandLine(argc, argv))                 
            return 1;

        res = app.Run();    
    }
    
    GMemory::DetectMemoryLeaks();
    return res;
}



// ***** FxPlayerApp Implementation

FxPlayerApp::FxPlayerApp()
{   
    Wireframe   = 0;
    // Scale toggle, on by default
    ScaleEnable = 1;
    Paused      = 0;
    PausedState = GFxMovie::Playing;

    // Clear timing 
    SpeedScale          = 1.0f;
    FrameCounter        = 0;
    TessTriangles       = 0;

    TimeStartTicks      = 0;
    TimeTicks           = 0;
    NextTicksTime       = 0;
    LastLoggedFps       = 0;    
    MovieStartTicks     = 0;
    MovieLastTicks      = 0;
    MovieTicks          = 0;

    LastFPS             = 0.0f;
    LastFrame           = 0;
    NeedHudUpdate       = 1;
    HudText[0]          = 0;

    ViewWidth   = 0; 
    ViewHeight  = 0;

    Zoom = 1.0;
    Move = GPointF(0.0);

    MouseTracking   = None;
    ControlKeyDown  = 0;

    SizingEntered = 0;

    CurvePixelError = 1.0f;

    // No old pos, save during FullScreen mode
    OldWindowX = OldWindowY = 0;
    OldWindowWidth = OldWindowHeight = 0;

    MeshRotation            = 0;
    LastRotationTick        = 0;
    CubeWireframe           = 0;
    pApp = this;
}

FxPlayerApp *FxPlayerApp::pApp = 0;

FxPlayerApp::~FxPlayerApp()
{
#ifdef FXPLAYER_RENDER_DIRECT3D
    if (pDevice)
        pDevice->SetStreamSource( 0, 0, 0, 0);
#endif

    pApp = 0;
}

/*

class GRTImageInfo : public GImageInfo
{
    bool isext;

public:
    GRTImageInfo(GImage *pim, UInt targetWidth = 0, UInt targetHeight = 0) : GImageInfo(pim, targetWidth, targetHeight) { isext = 0; } 
    //virtual ~GRTImageInfo();

    UInt            GetWidth() const { return GImageInfo::GetWidth(); }
    UInt            GetHeight() const { return GImageInfo::GetHeight(); }
    void            SetTexture(GTexture *ptex) { pTexture = ptex; isext = 1; ptex->AddChangeHandler(this); }
    //void            SetSize(UInt tw, UInt th) { TargetHeight = th; TargetWidth = tw; }

    // GTexture::ChangeHandler implementation           
    virtual void    OnChange(GRenderer* prenderer, EventType changeType);
    //virtual bool    Recreate(GRenderer* prenderer);
};

void    GRTImageInfo::OnChange(GRenderer* prenderer, EventType changeType)
{
    GImageInfo::OnChange(prenderer, changeType);

    if (pTexture && isext && changeType == Event_DataLost)
    {
        pTexture->RemoveChangeHandler(this);
        isext = 0;
    }       
}
*/
     

struct Vertex
{
#ifdef FXPLAYER_RENDER_DIRECT3D
    enum
    {
        FVF = D3DFVF_XYZ | D3DFVF_TEX1
    };
#endif

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


#ifdef FXPLAYER_RENDER_DIRECT3D

void    FxPlayerApp::SetupMatrices()
{
    // For our world matrix, we will just leave it as the identity
    D3DXMATRIXA16 matWorld;
    D3DXMatrixRotationY( &matWorld, MeshRotation ); 
    pDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the 
    // origin, and define "up" to be in the y-direction.

    D3DXVECTOR3 vEyePt( 0.0f, 4.0f, -5.5f );
    D3DXVECTOR3 vLookatPt( -1.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpVec( -0.1f, 1.0f, 0.0f );

    D3DXMATRIXA16 matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookatPt, &vUpVec );
    pDevice->SetTransform( D3DTS_VIEW, &matView );


    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DXToRadian( 45.0f ), 
        /*640.0f / 480.0f*/1.0f, 0.1f, 100.0f );
    pDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

// Rendering
void    FxPlayerApp::RenderMesh()
{
	if (!pRenderTexture)
		return;

    IDirect3DSurface9 *poldSurface      = 0;
    IDirect3DSurface9 *poldDepthSurface = 0;
    IDirect3DSurface9 *psurface         = 0;

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


	static D3DCOLOR clearColor = D3DCOLOR_ARGB(90,0,0,255);

    pDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 
					clearColor, 1.0f, 0 );

    UInt    ticks = timeGetTime();

    // 1/10 revolution per second
    float dt = 0.0f;
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

    // Setup the world, view, and projection matrices
    SetupMatrices();

    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    pDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    pDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    pDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    //pDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

    D3DVIEWPORT9 vp;
    vp.X        = 0;
    vp.Y        = 0;
    vp.Width    = RTWidth;
    vp.Height   = RTHeight;
    vp.MinZ     = 0.0f;
    vp.MaxZ     = 1.0f;
    pDevice->SetViewport(&vp);

    pDevice->BeginScene();

	pDevice->SetPixelShader(0);
	pDevice->SetVertexShader(0);

    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    // Blending 
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    //pDevice->SetRenderState(D3DRS_BLENDFACTOR, 0x60606060);

    pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

    pDevice->SetRenderState(D3DRS_FILLMODE, CubeWireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

    pDevice->SetTexture( 0, 0 );
    pDevice->SetStreamSource( 0, pCubeVertexBuffer, 0, sizeof(Vertex) );
    pDevice->SetFVF( Vertex::FVF );

    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255,180,0,0));
    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  0, 2 );
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255,180,180,0));
    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  4, 2 );
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255,0,180,180));
    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  8, 2 );

    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255,0,180,0));
    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 12, 2 );
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255,0,0,180));
    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 16, 2 );
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255,180,0,180));
    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 20, 2 );

    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

    pDevice->EndScene();

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

// also recreates if lost
bool    FxPlayerApp::SetupRTTexture()
{
    if (FAILED( pDevice->CreateTexture(         
        RTWidth,RTHeight,0,
        D3DUSAGE_RENDERTARGET|D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, 
        D3DPOOL_DEFAULT, &pRenderTexture.GetRawRef(), 0) ))
        return 0;

    pDevice->CreateDepthStencilSurface( RTWidth,RTHeight, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0,
        TRUE, &pStencilSurface.GetRawRef(), 0);

    if (pMovie)
    {
		GFxResource*	  pres = pMovie->GetMovieDef()->GetResource("texture1");
		GFxImageResource* pimageRes = 0;
		if (pres && pres->GetResourceType() == GFxResource::RT_Image)
			pimageRes = (GFxImageResource*)pres;

		if (pimageRes)    
		{
			// We know that the imageInfo is GImageInfo since we didn't override image creator.
			GImageInfo* pimageInfo = (GImageInfo*)pimageRes->GetImageInfo();

			if (pimageInfo)
			{
				GPtr<GTextureD3D9> ptexture = *((GRendererD3D9*)pRenderer)->CreateTexture();
				ptexture->InitTexture(pRenderTexture, pimageInfo->GetWidth(), pimageInfo->GetHeight());
				pimageInfo->SetTexture(ptexture);
			}		
		}
    }

    return 1;
}

#else


void    FxPlayerApp::RenderMesh()
{
    UInt    ticks = timeGetTime();

    // 1/10 revolution per second
    float dt = 0.0f;
    {
        float t  = (float)((double)fmod((double)ticks, 20000.0) / 20000.0) * 360;
        float lt = (float)((double)fmod((double)LastRotationTick, 20000.0) / 20000.0) * 360;
        dt = t - lt;
    }

    LastRotationTick    = ticks;
    MeshRotation        += dt;

    if (MeshRotation > 360)
        MeshRotation -= 360;
    if (MeshRotation < 0.0f)
        MeshRotation += 360;


    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, RenderTextureFbo);
    glViewport(0,0,RTWidth,RTHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1.0, 0.1, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0,-4,-5.5, 1,0,0, -0.1,1,0);
    glRotated(MeshRotation, 0,1,0);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glColorMask(1,1,1,1);
    glClearColor(0,0,1,0.4f);
    glClearDepth(1);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.1, 1);
    glPolygonMode(GL_FRONT_AND_BACK, CubeWireframe ? GL_LINE : GL_FILL);
    glLineWidth(6.0);
    glEnable(GL_LINE_SMOOTH);

    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), g_cubeVertices);
    glEnableClientState(GL_VERTEX_ARRAY);

    glColor4ub(180,0,0,255);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glColor4ub(180,180,0,255);
    glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
    glColor4ub(0,180,180,255);
    glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
    glColor4ub(0,180,0,255);
    glDrawArrays(GL_TRIANGLE_STRIP,12, 4);
    glColor4ub(0,0,180,255);
    glDrawArrays(GL_TRIANGLE_STRIP,16, 4);
    glColor4ub(180,0,180,255);
    glDrawArrays(GL_TRIANGLE_STRIP,20, 4);

    glDisableClientState(GL_VERTEX_ARRAY);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1);
    glDisable(GL_LINE_SMOOTH);
}

bool    FxPlayerApp::SetupRTTexture()
{
    GLuint rbs[2];

    // Create texture and framebuffer
    glGenFramebuffersEXT(1, &RenderTextureFbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, RenderTextureFbo);

    glGenTextures(1, &RenderTextureId);
    glBindTexture(GL_TEXTURE_2D, RenderTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RTWidth,RTHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, RenderTextureId, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffersEXT(2, rbs);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbs[0]);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, RTWidth,RTHeight);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbs[0]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    if (pMovie)
    {
		GFxResource*	  pres = pMovie->GetMovieDef()->GetResource("texture1");
		GFxImageResource* pimageRes = 0;
		if (pres && pres->GetResourceType() == GFxResource::RT_Image)
			pimageRes = (GFxImageResource*)pres;

		if (pimageRes)
		{
            // We know that the imageInfo is GImageInfo since we didn't override image creator.
            GImageInfo* pimageInfo = (GImageInfo*)pimageRes->GetImageInfo();
            if (pimageInfo)
            {
                GPtr<GTextureOGL> ptexture = *((GRendererOGL*)pRenderer)->CreateTexture();
                ptexture->InitTexture(RenderTextureId, pimageInfo->GetWidth(), pimageInfo->GetHeight());
                pimageInfo->SetTexture(ptexture);
            }
		}
    }

    return 1;
}

#endif


SInt    FxPlayerApp::Run()
{   

    UByte       verboseFlags = 0;

    // Set the verbose flags
	GPtr<GFxParseControl> pparseControl = *new GFxParseControl();
	pparseControl->SetParseFlags(Settings.VerboseParse ? GFxParseControl::VerboseParse : 0);
	Loader.SetParseControl(pparseControl);

	GPtr<GFxFileOpener> pfileOpener = *new FxPlayerRTTFileOpener;
	Loader.SetFileOpener(pfileOpener); 

	GPtr<GFxFSCommandHandler> pcommandHandler = *new FxPlayerRTTFSCommandHandler;
	Loader.SetFSCommandHandler(pcommandHandler);


    // Set log, but only if not quiet
    if (!Settings.Quiet)
        Loader.SetLog(GPtr<GFxLog>(*new GFxPlayerLog()));

    if (strlen(Settings.FileName)==0)
        gfc_strcpy(Settings.FileName, 256, "Window_Texture.swf");

    bool loadMovie = strlen(Settings.FileName)>0;

    // Get info about the width & height of the movie.
    if (!loadMovie || !Loader.GetMovieInfo(Settings.FileName, &MovieInfo))
    {
        if (loadMovie)
            fprintf(stderr, "Error: failed to get info about %s\n", Settings.FileName);

        ViewWidth   = 1024;
        ViewHeight  = 768;      

        //return 1;
    }
    else
    {
        ViewWidth   = (SInt) (MovieInfo.Width * Settings.ScaleX);
        ViewHeight  = (SInt) (MovieInfo.Height * Settings.ScaleY);
    }

    // Leave as 0 until switching.
    // This Will cause border-dependent calc during full-screen toggle.
    OldWindowWidth  = 0;
    OldWindowHeight = 0;

    if (Settings.DoRender)
    {
        // Set options based on arguments
        FullScreen      = Settings.FullScreen;
        FSAntialias     = (Settings.AAMode == FxPlayerSettings::AAMode_FSAA) ? 1 : 0;
        BitDepth        = Settings.BitDepth;        

        if (FullScreen)
        {
            Settings.ScaleX = 1024.0f / ViewWidth;
            Settings.ScaleY = 768.0f / ViewHeight;
            ViewWidth       = 1024;
            ViewHeight      = 768;
        }

        
        // Enable file drop.
        SupportDropFiles = 1;
        SizableWindow    = 1;
        if (!SetupWindow(FXPLAYER_APP_TITLE, ViewWidth, ViewHeight))
            return 1;


        // It is important to initialize these sizes, in case OnSizeEnter gets called.
        SizeWidth  = Width;
        SizeHeight = Height;    

        // Create renderer      
        if (pRenderer = *FXPLAYER_RENDER::CreateRenderer())
        {
    #ifdef  FXPLAYER_RENDER_DIRECT3D
            pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
    #else
            pRenderer->SetDependentVideoMode();
    #endif          
        }
        else
        {
            return 1;
        }
    }

#ifndef FXPLAYER_RENDER_DIRECT3D
    if (!strcmp((const char *) glGetString(GL_EXTENSIONS), "EXT_framebuffer_object"))
    {
#ifdef GFC_OS_WIN32
        MessageBox(hWND, "EXT_framebuffer_object is required.", "Missing GL Extension", MB_OK);
#endif
        return 0;
    }
#endif

    RTWidth = 256;
    RTHeight = 256;
    SetupRTTexture();

#ifdef FXPLAYER_RENDER_DIRECT3D
    pDevice->CreateVertexBuffer( 24*sizeof(Vertex),0, Vertex::FVF,
        D3DPOOL_MANAGED, &pCubeVertexBuffer.GetRawRef(), NULL );
    void *pVertices = NULL;
    pCubeVertexBuffer->Lock( 0, sizeof(g_cubeVertices), (void**)&pVertices, 0 );
    memcpy( pVertices, g_cubeVertices, sizeof(g_cubeVertices) );
    pCubeVertexBuffer->Unlock();
#endif

	pRenderConfig = *new GFxRenderConfig(pRenderer);
	Loader.SetRenderConfig(pRenderConfig);

	// Create a renderer stats object since we will be tracking statistics.
	pRenderStats = *new GFxRenderStats();
	Loader.SetRenderStats(pRenderStats);

    // Load movie and initialize timing.
    if (loadMovie && !LoadMovie(Settings.FileName)) 
    {
        //return 1;
    }

    
    while (1)
    {   
        TimeTicks = timeGetTime();

        if (Settings.DoRender && !Settings.FastForward)
            MovieTicks = TimeTicks;
        else // Simulate time.          
            MovieTicks = MovieLastTicks + (UInt32) (1000.0f / MovieInfo.FPS);
        
        int     deltaTicks  = MovieTicks - MovieLastTicks;
        float   deltaT      = deltaTicks / 1000.f;
        
        MovieLastTicks = MovieTicks;

        // Check auto exit timeout counter.
        if ((Settings.ExitTimeout > 0) &&
            (MovieTicks - MovieStartTicks > (UInt32) (Settings.ExitTimeout * 1000)) )   
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
#ifdef FXPLAYER_RENDER_DIRECT3D
                pRenderTexture = 0;                    
                pStencilSurface= 0;                    
#endif

                RecreateRenderer(); 
                SetupRTTexture();
            }
        }

        // Potential out-of bounds range is not a problem here,
        // because it will be adjusted for inside of the player.
        if (pMovie)
        {
            pMovie->SetViewport(Width,Height, (Width-ViewWidth)/2, (Height-ViewHeight)/2,
                                ViewWidth, ViewHeight);
            pMovie->SetBackgroundAlpha(Settings.Background ? 1.0f : 0.05f);

            Float timeTillNextTicks;

            if (!Paused)            
                timeTillNextTicks = pMovie->Advance(deltaT * SpeedScale, 0);
            else
                timeTillNextTicks = 0.05f;

            NextTicksTime = TimeTicks + (UInt32)(timeTillNextTicks * 1000.0f);
            if (NextTicksTime < TimeTicks) // wrap-around check.
                NextTicksTime = TimeTicks;
        }

        if (Settings.DoRender)
        {
            RenderMesh();

            // Renderer-specific preparation (Disable depth testing)
            PrepareRendererForFrame();
            
            // Clear the entire buffer.
            Clear(GColor::Blue);
                
        // Enable wireframe if requested.
        if (Wireframe)
            SetWireframe(1);
        }

        if (pMovie)
            pMovie->Display(); 

        FrameCounter++;

        
        if (Settings.DoRender)
        {           
            SetWireframe(0);

            if (pMovie && (LastFrame != pMovie->GetCurrentFrame()))
                NeedHudUpdate = 1;

            // Get stats every frame
            GRenderer::Stats    renderStats;
            pRenderer->GetRenderStats(&renderStats, 1);
            // If ballpark triangle count changed, need update
            if (((renderStats.Triangles >> 11) != (LastStats.Triangles >> 11)) ||
                (renderStats.Primitives != LastStats.Primitives))
                NeedHudUpdate = 1;
            LastStats = renderStats;

            if (NeedHudUpdate)
                UpdateHudText();

            // Draw the HUD screen if it is displayed.
            if ((Settings.HudState != FxPlayerSettings::Hud_Hidden) && HudText[0])
            {   
                Push2DRenderView();

                SInt tw, th;
                CalcDrawTextSize(&tw, &th, HudText);
                // Fill background.
                FillRect(10-5,10-5, 10 + tw + 5, 10 + th + 5, GColor::DarkGray|GColor::Alpha50);
                // And draw text.
                DrawText(10,10, HudText, UInt32(GColor::Yellow| GColor::Alpha100) );
                Pop2DRenderView();
            }
            
            // Otherwise, if therer is no movie playing, display a drop message.
            else if (!pMovie)
            {
                Push2DRenderView();
                const char  *pmessage = "Drag and Drop SWF File Here";
                // Draw message in the center.
                SInt tw, th;
                CalcDrawTextSize(&tw, &th, pmessage);
                DrawText(Width/2 - tw/2, Height/2 - th/2, pmessage, UInt32(GColor::Yellow| GColor::Alpha100) );
                Pop2DRenderView();              
            }


            // Flip buffers to display the scene
            PresentFrame();

            if (!pMovie || (!Settings.MeasurePerformance && !Settings.FastForward))
            {
                // Don't hog the CPU.
                MSG winMsg;               

                if (pMovie)
                {                    
                      TimeTicks = timeGetTime();
                      if (TimeTicks < NextTicksTime)                      
                          SleepTillMessage(NextTicksTime - TimeTicks);                      
                }
                else
                {
                    // No movie, just wait for it.
                    if (!PeekMessage(&winMsg,0,0,0,0))
                        ::Sleep(32);
                }
            }
            else
            {
                // Log the frame rate every second or so.
                if (TimeTicks - LastLoggedFps > 1000)
                {
                    float   delta = (TimeTicks - LastLoggedFps) / 1000.f;

                    char buff[512];

                    LastFPS = (delta > 0) ? FrameCounter / delta : 0.0f;    

                    TessTriangles = pRenderStats->GetTessStatistics();
                    
                    // Display frame rate in title
                    gfc_sprintf(buff, 512, FXPLAYER_APP_TITLE " (fps:%3.1f)", LastFPS);
                    SetWindowTitle(buff);

                    // Update HUD
                    NeedHudUpdate = 1;          

                    LastLoggedFps = TimeTicks;
                    FrameCounter = 0;
                }
            }
        }

        // If we're reached the end of the movie, exit.
        if (!Settings.DoLoop && pMovie &&
            (pMovie->GetCurrentFrame() + 1 == pMovieDef->GetFrameCount()) )
            break;          
    }


    // Release logic? -> TBD
    
    pMovie      = 0;

    return 0;
}

/*
GImageInfoBase* GFxImageCreateCallback(const GFxImageCreateInfo& info, Handle userHandle)
{
    GUNUSED(userHandle);
    switch(info.Type)
    {
        case GFxImageCreateInfo::Input_None:
            return new GRTImageInfo(0);
        case GFxImageCreateInfo::Input_Image:
            return new GRT ImageInfo(info.pImage);

        case GFxImageCreateInfo::Input_File:             
            return new GRTImageInfo(0, info.FileInfo.TargetWidth, info.FileInfo.TargetHeight);
    }
    return 0;
}
*/


// Load a new movie from a file and initialize timing
bool    FxPlayerApp::LoadMovie(char *pfilename)
{
    // Try to load the new movie
    GPtr<GFxMovieDef>   pnewMovieDef;
    GPtr<GFxMovieView>  pnewMovie;
    GFxMovieInfo        newMovieInfo;

    // Get info about the width & height of the movie.
    if (!Loader.GetMovieInfo(pfilename, &newMovieInfo))
    {
        fprintf(stderr, "Error: failed to get info about %s\n", pfilename);
        return 0;
    }

    UInt loadConstants = GFxLoader::LoadAll;    
    
    // Load the actual new movie and crate instance.
    // Don't use library: this will ensure that the memory is released.
    pnewMovieDef = *Loader.CreateMovie(pfilename, loadConstants);
    if (!pnewMovieDef)
    {
        fprintf(stderr, "Error: failed to create a movie from '%s'\n", pfilename);
        return 0;
    }
    
    pnewMovie = *pnewMovieDef->CreateInstance(false);    
    if (!pnewMovie)
    {
        fprintf(stderr, "Error: failed to create movie instance\n");
        return 0;
    }

    // If this succeeded, replace the old movie with the new one.
    pMovieDef   = pnewMovieDef;
    pMovie      = pnewMovie;
    memcpy(&MovieInfo, &newMovieInfo, sizeof(GFxMovieInfo));

	GPtr<GFxActionControl> pactionControl = *new GFxActionControl();
	pactionControl->SetVerboseAction(Settings.VerboseAction);
	pactionControl->SetActionErrorSuppress(Settings.NoActionErrors);
	pMovie->SetActionControl(pactionControl);

    // init first frame
    pMovie->Advance(0.0f, 0);

    // Renderer
	if (Settings.DoRender)
	{
		pMovie->SetRenderConfig(pRenderConfig);
		if (Settings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
			pRenderConfig->SetRenderFlags(pRenderConfig->GetRenderFlags() | GFxRenderConfig::RF_EdgeAA);
	}
    
    // Copy short filename (i.e. after last '/'),
    // but only if the source isn't the same buffer.
    if (Settings.FileName != pfilename)
    {   
        SPInt len = strlen(pfilename);
        for (SPInt i=len; i>0; i--)
        {
            if (pfilename[i]=='/' || pfilename[i]=='\\')
            {
                pfilename = pfilename+i+1;
                break;
            }
        }
        gfc_strcpy(Settings.FileName, 256, pfilename);
    }
    

    // Disable pause.
    Paused          = 0;

    // Init timing for the new piece.
    FrameCounter    = 0;
    // Time ticks: always rely on a timer
    TimeStartTicks  = timeGetTime(); 
    NextTicksTime   = TimeStartTicks;
    LastLoggedFps   = TimeStartTicks;
    // Movie logical ticks: either timer or setting controlled
    MovieStartTicks = (Settings.DoRender && !Settings.FastForward) ? TimeStartTicks : 0;
    MovieLastTicks  = MovieStartTicks;
    // Set the Hud to update
    NeedHudUpdate = 1;

    // Update the view
    UpdateViewSize();
	GFxResource*	  pres = pMovie->GetMovieDef()->GetResource("texture1");
	GFxImageResource* pimageRes = 0;
	if (pres && pres->GetResourceType() == GFxResource::RT_Image)
		pimageRes = (GFxImageResource*)pres;

	if (pimageRes)    
    {
#ifdef FXPLAYER_RENDER_DIRECT3D
        GPtr<GTextureD3D9> pTexture = *((GRendererD3D9*)pRenderer)->CreateTexture();
        pTexture->InitTexture(pRenderTexture, pimageRes->GetWidth(), pimageRes->GetHeight());
#else
        GPtr<GTextureOGL> pTexture = *((GRendererOGL*)pRenderer)->CreateTexture();
        pTexture->InitTexture(RenderTextureId, pimageRes->GetWidth(), pimageRes->GetHeight());
#endif

		// Store the original image info's width and height.
        GImageInfo* pimageInfo = (GImageInfo*)pimageRes->GetImageInfo();
        // Convert image to texture; keep image dimensions as target size.
        pimageInfo->SetTexture(pTexture.GetPtr(),
                               pimageInfo->GetWidth(), pimageInfo->GetHeight());
    }

    return 1;
}


// Called when sizing begins and ends.
void    FxPlayerApp::OnSizeEnter(bool enterSize)
{
    
    // When leaving size, adjust to new widtj/height.
    if (!enterSize)
    {
        SizingEntered = 0;

        if (pRenderer && ((SizeWidth != Width) || (SizeHeight != Height)))
        {
#ifdef FXPLAYER_RENDER_DIRECT3D
            pRenderTexture = 0;
            pStencilSurface= 0;
#endif

            pRenderer->ResetVideoMode();

            // Call original on
            FXPLAYER_APP::OnSize(SizeWidth, SizeHeight);

            // Update view based on the new window size and scale settings.
            UpdateViewSize();

        #ifdef  FXPLAYER_RENDER_DIRECT3D
            pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
            SetupRTTexture();
        #else
            pRenderer->SetDependentVideoMode();
        #endif  
			
        }
    }
    else
    {
        // Set SizingEntered flag so that we can differentiate size-grip
        // resize from instantaneous resize (maximize).
        SizingEntered = 1;
    }
}

// Sizing; by default, re-initalizes the renderer
void    FxPlayerApp::OnSize(SInt w, SInt h)
{
    if (pRenderer)
    {
        SizeWidth = w;
        SizeHeight= h;

        if (!SizingEntered &&
            (pRenderer->CheckDisplayStatus()==FXPLAYER_RENDER::DisplayStatus_Ok))
        {
            // Commit sizing immediately if it was due to maximize.
            OnSizeEnter(0);
        }
        else
        {
#ifdef FXPLAYER_RENDER_DIRECT3D
            if (pDevice && !(FSAntialias && FSAASupported))
                pDevice->Present( NULL, NULL, NULL, NULL );
#endif
        }       
    }
}

void    FxPlayerApp::ResetUserMatrix()
{
    Move = GPointF(0.0f);
    Zoom = 1.0f;
    UpdateUserMatrix();
}

void    FxPlayerApp::UpdateUserMatrix()
{
    if (!pRenderer)
        return;
    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x * GFxMovie::GetRenderPixelScale(), Move.y * GFxMovie::GetRenderPixelScale());
    pRenderer->SetUserMatrix(m);
    UpdateHudText();
}

GSizeF  FxPlayerApp::GetMovieScaleSize()
{
    GSizeF  scale;
    scale.Width     = (pMovie->GetVisibleFrameRect().Width() / ViewWidth);
    scale.Height    = (pMovie->GetVisibleFrameRect().Height() / ViewHeight);
    return scale;
}

// Updates the view size based on the ScaleEnable flag and window size.
void    FxPlayerApp::UpdateViewSize()
{
    if (ScaleEnable)
    {
        SInt width = GTL::gmax(Width, 4);
        SInt height= GTL::gmax(Height, 4);

        // Determine movie size and location based on the aspect ratio  
        float hw = (Float) MovieInfo.Height / (Float) MovieInfo.Width;
        if (width * hw > height)
        {
            // Use height as the basis for aspect ratio
            ViewWidth   = (SInt)((float) height / hw);
            ViewHeight  = height;
        }
        else
        {
            // Use width
            ViewWidth   = width;
            ViewHeight  = (SInt) (width * hw);
        }

        // Compute input scale
        Settings.ScaleX = (Float) ViewWidth / (Float) MovieInfo.Width;
        Settings.ScaleY = (Float) ViewHeight / (Float) MovieInfo.Height;
    }
    else
    {
        // No scaling, just center the image
        ViewWidth   = MovieInfo.Width;
        ViewHeight  = MovieInfo.Height;
        Settings.ScaleX = Settings.ScaleY = 1.0f;
    }
}


/// Helper function to update HUD.
// Uses LastFPS and LastStats; those variables must be updated separately.
// Helper function to update HUD.
// Uses LastFPS and LastStats; those variables must be updated separately.
void    FxPlayerApp::UpdateHudText()
{   
	if (pMovie)
		LastFrame = pMovie->GetCurrentFrame();
	else
		LastFrame = 0;

	// Stroke type
	UInt32  stroke       = GFxRenderConfig::RF_StrokeCorrect;
	bool    optTriangles = false;

	if (pRenderConfig)
	{
		stroke = (pRenderConfig->GetRenderFlags() & GFxRenderConfig::RF_StrokeMask);
		optTriangles = (pRenderConfig->GetRenderFlags() & GFxRenderConfig::RF_OptimizeTriangles) != 0;
	}

	// Display a custom message if stroke is #ifdef-ed out.
#ifndef GFC_NO_FXPLAYER_STROKER
	char *  pstrokeType = "Correct";
	if (stroke == GFxRenderConfig::RF_StrokeNormal)
		pstrokeType = "Normal";
#else
	char *  pstrokeType = "Correct [#disabled]";
	if (stroke == GFxRenderConfig::RF_StrokeNormal)
		pstrokeType = "Normal [#disabled]";
#endif

	if (stroke == GFxRenderConfig::RF_StrokeHairline)
		pstrokeType = "Hairline";


	// AA Type
	char * pAAType = "Edge AA";

	switch(Settings.AAMode)
	{
	case FxPlayerSettings::AAMode_None:
		pAAType = "None";
		break;
	case FxPlayerSettings::AAMode_EdgeAA:

		// Display a custom message if edge AA is #ifdef-ed out.
#ifndef GFC_NO_FXPLAYER_EDGEAA
		pAAType = "Edge AA";
		{
			// Report if EdgeAA is not supported by renderer.
			GRenderer::RenderCaps caps;
			caps.CapBits = 0;
			if (pRenderer)
				pRenderer->GetRenderCaps(&caps);
			if (!(caps.CapBits & GRenderer::Cap_FillGouraud))
				pAAType = "Edge AA (Not Supported)";
			else if (!(caps.CapBits & GRenderer::Cap_FillGouraudTex))
				pAAType = "Edge AA (Limited)";
		}
#else
		pAAType = "Edge AA [#disabled]";
#endif

		break;
	case FxPlayerSettings::AAMode_FSAA:
		if (FSAASupported)
			pAAType = "HW FSAA";
		else
			pAAType = "HW FSAA (Not Supported)";
		break;
	}

#ifdef   FXPLAYER_MEMORY_TRACKSIZES
	GAllocator*            palloc = GMemory::GetAllocator();
	const GAllocatorStats* pstats = palloc->GetStats();
#endif

	// Update hud text
	gfc_sprintf(HudText, 2048,
		"Playback Info (F1/F2)\n\n"
		"Filename:  %s\n"
		"           SWF %d (%dx%d@%.f)\n"
		"           %d/%d frames%s\n"
		"FPS:       %3.1f\n"
#ifdef   FXPLAYER_MEMORY_TRACKSIZES     
		"MemUse:    %dK / %dK @ %d\n"
#endif
		"NewTess:   %d Tri/s\n"
		"Triangles: %d @ %d DP\n"
		"Lines:     %d\n"
		"CurveErr:  %3.1f (Ctrl - or +)\n"
		"Stroke:    %s\n"
		"Zoom:      %3.2f\n"
		//"Move:      x:%3.1f y:%3.1f"
		"AA Mode:   %s %s",

		Settings.FileName,
		MovieInfo.Version,
		MovieInfo.Width, MovieInfo.Height, MovieInfo.FPS,
		LastFrame, MovieInfo.FrameCount, Paused ? " (Paused)" : "",
		LastFPS,
#ifdef   FXPLAYER_MEMORY_TRACKSIZES     
		(pstats->GetSizeAllocated()+1023) / 1024,
		(pstats->GetSizeUsed()+1023) / 1024, 
		(pstats->AllocCount - pstats->FreeCount),
#endif
		TessTriangles,
		LastStats.Triangles, LastStats.Primitives,
		LastStats.Lines,
		CurvePixelError,
		pstrokeType,
		Zoom,
		//Move.x/20, Move.y/20,
		pAAType,
		optTriangles ? "*Opt" : ""
		);

	if (Settings.HudState == FxPlayerSettings::Hud_Help)
	{
		gfc_strcat(HudText, 2048,
			"\n\n"          
			"Keys:\n"           
			"  CTRL S          Toggle scaled display\n"
			"  CTRL W          Toggle wireframe\n"
			"  CTRL A          Toggle antialiasing mode\n"
#ifdef FXPLAYER_RENDER_DIRECT3D
			"  CTRL U          Toggle fullscreen\n"
#endif          
			"  CTRL F          Toggle fast mode (FPS)\n"
			"  CTRL G          Toggle fast forward\n"
			"  CTRL P          Toggle pause\n"
			"  CTRL R          Restart the movie\n"
			"  CTRL D          Toggle stage clipping\n"
			"  CTRL C          Toggle stage culling\n"
			"  CTRL O          Toggle triangle optimization\n"
			"  CTRL Right      Step back one frame\n"
			"  CTRL Left       Step forward one frame\n"
			"  CTRL PageUp     Step back 10 frames\n"
			"  CTRL PageDown   Step forward 10 frames\n"
			"  CTRL -,+        Curve tolerance down, up\n"
			"  F1              Toggle Info Help\n"
			"  F2              Toggle Info Stats\n"
			"  CTRL Q          Quit"
			);
	}

	NeedHudUpdate = 0;
}

// Handle dropped file
void    FxPlayerApp::OnDropFiles(char *path)
{
    ShowCursor(true);
    SetCursor(::LoadCursor(NULL, IDC_ARROW));
    ResetUserMatrix();
    LoadMovie(path);
    ::BringWindowToTop(hWND); 
    ::SetForegroundWindow(hWND);
}

void    FxPlayerApp::OnKey(UInt keyCode, UInt info, bool downFlag)
{   
	if (!pMovie)
		return;

	//  GFxLog* plog = pMovie->GetLog();
	bool    ctrl = ControlKeyDown;

	if (keyCode == VK_CONTROL)
	{
		ControlKeyDown = downFlag;
		return;
	}
    if (keyCode == VK_MENU && downFlag) 
        ControlKeyDown = false; // to enable Ctrl-Alt-... combinations to work

	if (keyCode == VK_ESCAPE && downFlag)
	{
		// Cancel mouse manipulation
		if (MouseTracking != None)
		{
			MouseTracking = None;
			::ReleaseCapture();
			Zoom = ZoomStart;
			Move = MoveStart;
			UpdateUserMatrix();
			return;
		}
	}


	if (keyCode == VK_F1 && downFlag)
		goto toggle_hud;
	if (keyCode == VK_F2 && downFlag)
		goto toggle_stats;

	// Handle Ctrl-Key combinations
	if (ctrl && downFlag)
	{

		switch(keyCode)
		{
		case 'Q':
			QuitFlag = 1;
			return;

			// minus
		case 109: // 219 '[' 
		case 189: // 219 '['
			CurvePixelError = GTL::gmin(10.0f, CurvePixelError + 0.5f);
			pRenderConfig->SetMaxCurvePixelError(CurvePixelError);
			UpdateHudText();
			break;

			// plus
		case 107: // 221 ']':           
		case 187: // 221 ']':           
			CurvePixelError = GTL::gmax(0.5f, CurvePixelError - 0.5f);
			pRenderConfig->SetMaxCurvePixelError(CurvePixelError);
			UpdateHudText();
			break;

		case 'W':
			// Toggle wireframe.
			Wireframe = !Wireframe;
			break;

			// Switch to a next stroke type.
		case 'T':
			{                   
				UInt32  rf      = pRenderConfig->GetRenderFlags();
				UInt32  stroke  = rf & GFxRenderConfig::RF_StrokeMask;

				switch(stroke)
				{                 
				case GFxRenderConfig::RF_StrokeHairline:  stroke = GFxRenderConfig::RF_StrokeNormal; break;
				case GFxRenderConfig::RF_StrokeNormal:    stroke = GFxRenderConfig::RF_StrokeCorrect; break;
				case GFxRenderConfig::RF_StrokeCorrect:   stroke = GFxRenderConfig::RF_StrokeHairline; break;
				}
				pRenderConfig->SetRenderFlags((rf & ~GFxRenderConfig::RF_StrokeMask) | stroke);
				UpdateHudText();
			}
			break;

		case 'P':               
			// Toggle paused state.
			NeedHudUpdate = 1;
			Paused = !Paused;
			// Note that toggling play state through pMovie->SetPlayState would only pause root clip,
			// not children. So instead we just set the global variable above which prevents Advance.
			// However, we may still need to save/restore state in frame.
			if (Paused)
				PausedState = pMovie->GetPlayState();
			else                
				pMovie->SetPlayState(PausedState);
			break;

		case 'R':
			Paused = 0;
			pMovie->Restart();
			break;

		case VK_LEFT:               
			pMovie->GotoFrame(pMovie->GetCurrentFrame()-1);
onkey_finish_seek:
			Paused = 1;
			NeedHudUpdate = 1;
			pMovie->SetPlayState(GFxMovie::Playing);
			// Ensure tag actions are executed. This may change frame state to Stopped.
			pMovie->Advance(0.0f); 
			PausedState = pMovie->GetPlayState();
			break;
		case VK_RIGHT:
			pMovie->GotoFrame(pMovie->GetCurrentFrame()+1);
			goto onkey_finish_seek;

		case VK_PRIOR:              
			pMovie->GotoFrame(
				GTL::gmax<UInt>(0, pMovie->GetCurrentFrame()-10) );
			goto onkey_finish_seek;
		case VK_NEXT:
			pMovie->GotoFrame(
				GTL::gmin<UInt>(pMovie->GetCurrentFrame()+10, MovieInfo.FrameCount-1));
			goto onkey_finish_seek;

		case 'H':
			// Toggle info hud.
toggle_hud:
			switch(Settings.HudState)
			{
			case FxPlayerSettings::Hud_Hidden:  Settings.HudState = FxPlayerSettings::Hud_Help;     break;
			case FxPlayerSettings::Hud_Stats:   Settings.HudState = FxPlayerSettings::Hud_Help;     break;
			case FxPlayerSettings::Hud_Help:    Settings.HudState = FxPlayerSettings::Hud_Hidden;   break;
			}
			NeedHudUpdate = 1;
			break;

		case 'I':
			// Toggle info stats.
toggle_stats:
			switch(Settings.HudState)
			{
			case FxPlayerSettings::Hud_Hidden:  Settings.HudState = FxPlayerSettings::Hud_Stats;    break;
			case FxPlayerSettings::Hud_Stats:   Settings.HudState = FxPlayerSettings::Hud_Hidden;   break;
			case FxPlayerSettings::Hud_Help:    Settings.HudState = FxPlayerSettings::Hud_Stats;    break;
			}
			NeedHudUpdate = 1;
			break;

		case 'A':
			{
				bool    renderChange = 0;
				bool    edgeChange   = 0;

				switch(Settings.AAMode)
				{
				case FxPlayerSettings::AAMode_None:
					Settings.AAMode = FxPlayerSettings::AAMode_EdgeAA;
					edgeChange = 1;
					break;
				case FxPlayerSettings::AAMode_EdgeAA:
					Settings.AAMode = FxPlayerSettings::AAMode_FSAA;
					edgeChange = renderChange = 1;
					break;
				case FxPlayerSettings::AAMode_FSAA:
					Settings.AAMode = FxPlayerSettings::AAMode_None;
					renderChange = 1;
					break;
				}
				NeedHudUpdate = 1;

				// FSAA toggle
#ifdef  FXPLAYER_RENDER_DIRECT3D
				if (pRenderer && FSAASupported && renderChange)
				{           
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

					pRenderTexture = 0;
					pStencilSurface = 0;

					pRenderer->ResetVideoMode();

					// Call original on
					RecreateRenderer();         

					pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
					SetupRTTexture();
				}

#endif

				if (edgeChange && pRenderer && pMovie)
				{
					UInt32 rendererFlags = pRenderConfig->GetRenderFlags() & ~GFxRenderConfig::RF_EdgeAA;
					if (Settings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
						rendererFlags |= GFxRenderConfig::RF_EdgeAA;
					pRenderConfig->SetRenderFlags(rendererFlags);
				}

			}
			break;

		case 'U':

			// TODO : Should pull this into the Direct3D App class
#ifdef  FXPLAYER_RENDER_DIRECT3D
			if (pRenderer)
			{   
restore_video_mode:
				FullScreen = !FullScreen;

				SInt    x = OldWindowX, y = OldWindowY;                 
				SInt    w = OldWindowWidth, h = OldWindowHeight;                    

				if (FullScreen)
				{
					// Save window size & location
					RECT r;
					::GetWindowRect(hWND, &r);                  
					OldWindowWidth  = r.right - r.left;
					OldWindowHeight = r.bottom - r.top;
					OldWindowX      = r.left;
					OldWindowY      = r.top;
					// New location for full-screen
					x = y = 0;
					w = 1024; h = 768;
				}

				if (!w || !h)
				{
					w = 1024; h = 768;
				}

				if (FSAntialias && FSAASupported)
				{
					PresentParams.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
					PresentParams.SwapEffect      = D3DSWAPEFFECT_DISCARD; // Discard required
				}
				else
				{
					PresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
					PresentParams.SwapEffect      = FullScreen ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
				}

				// Release our references to D3D resources. 
				pRenderTexture = 0;
				pStencilSurface = 0;

				pRenderer->ResetVideoMode();

				// Set new D3D mode, location & size. Recover if that fails.
				if (!ConfigureWindow(x, y, w, h, FullScreen))
					goto restore_video_mode;

				UpdateViewSize();

				pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
				SetupRTTexture();
			}

#endif

			break;

		case 'S':
			// Toggler scale
			ScaleEnable = !ScaleEnable;
			UpdateViewSize();
			break;

		case 'D':
			// Toggler clipping
			ClippingEnable = !ClippingEnable;
			UpdateViewSize();
			break;

		case 'G':
			Settings.FastForward = !Settings.FastForward;
			break;

		case 'B':               
			// toggle background color.
			Settings.Background = !Settings.Background;             
			break;

		case 'F':               
			Settings.MeasurePerformance = !Settings.MeasurePerformance;
			pRenderStats->GetTessStatistics(); // Clear stats
			LastFPS = 0;
			NeedHudUpdate = 1;

			if (!Settings.MeasurePerformance)
				SetWindowTitle(FXPLAYER_APP_TITLE);
			break;

		case 'Z':                
			ResetUserMatrix();
			break;

		case 'C':
			// Toggle viewport culling.
			if (pMovie && pRenderConfig)
			{
				UInt32 rendererFlags = pRenderConfig->GetRenderFlags();
				pRenderConfig->SetRenderFlags(rendererFlags ^ GFxRenderConfig::RF_NoViewCull);
			}
			break;

		case 'O':
			if (pMovie && pRenderConfig)
			{
				UInt32 rendererFlags = pRenderConfig->GetRenderFlags();
				pRenderConfig->SetRenderFlags(rendererFlags ^ GFxRenderConfig::RF_OptimizeTriangles);
			}
			UpdateHudText();
			break;
		} // switch(keyCode)
	} // if (ctrl)

	else
	{ // if (!ctrl)

		// Inform the player about keystroke
		if (!ctrl)
			KeyEvent(keyCode, info, downFlag);
	}
}

// Helper used to convert key codes and route them to GFxPlayer
void    FxPlayerApp::KeyEvent(UInt key, UInt info, bool down)
{
    GFxKey::Code    c((GFxKey::Code)key);
    //AB, in the case key is not found in the table let give a chance to 
    // plain key code, not just return GFxKey::VoidSymbol;

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
        static struct //!AB added static modifier
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
        if (pMovie)
        {
            // get the ASCII code, if possible.
            UByte asciiCode = 0;
            UINT uScanCode = (info >> 16) & 0xFF; // fetch the scancode
            BYTE ks[256];
            WORD charCode;

            // Get the current keyboard state
            ::GetKeyboardState(ks);

            if (::ToAscii(key, uScanCode, ks, &charCode, 0) > 0) 
            {
                //!AB, what to do if ToAscii returns > 1?
                asciiCode = LOBYTE (charCode);
            }

            GFxKeyEvent event(down ? GFxEvent::KeyDown : GFxKeyEvent::KeyUp, c, asciiCode);
            event.SpecialKeysState.SetShiftPressed((::GetKeyState(VK_SHIFT) & 0x8000) ? 1: 0);
            event.SpecialKeysState.SetCtrlPressed((::GetKeyState(VK_CONTROL) & 0x8000) ? 1: 0);
            event.SpecialKeysState.SetAltPressed((::GetKeyState(VK_MENU) & 0x8000) ? 1: 0);
            event.SpecialKeysState.SetNumToggled((::GetKeyState(VK_NUMLOCK) & 0x1) ? 1: 0);
            event.SpecialKeysState.SetCapsToggled((::GetKeyState(VK_CAPITAL) & 0x1) ? 1: 0);
            event.SpecialKeysState.SetScrollToggled((::GetKeyState(VK_SCROLL) & 0x1) ? 1: 0);
            pMovie->HandleEvent(event);         
        }
    }
}

void    FxPlayerApp::OnChar(UInt32 wcharCode, UInt info)
{
    if (pMovie && wcharCode)
    {
        GFxCharEvent event(wcharCode);
        pMovie->HandleEvent(event);
    }
}

void    FxPlayerApp::OnMouseButton(UInt button, bool downFlag, SInt x, SInt y)
{
    if (!pMovie)
        return;

    MouseX = x;
    MouseY = y;

    // Adjust x, y to viewport.
    GSizeF  s = GetMovieScaleSize();
    Float   mX = ((x - (Width-ViewWidth)/2)) * s.Width;
    Float   mY = ((y - (Height-ViewHeight)/2)) * s.Height;

    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x,Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    // Update mouse state
    if (downFlag)
    {
        MouseDownX = MouseX;
        MouseDownY = MouseY;

        ::SetCapture(hWND);

        if (button==0 && ControlKeyDown)
            MouseTracking = Zooming;
        else if (button==1 && ControlKeyDown)
            MouseTracking = Moving;

        if (MouseTracking != None)
        {
            ZoomStart = Zoom;
            MoveStart = Move;
            return;
        }

        GFxMouseEvent event(GFxEvent::MouseDown, button, x, y, 0.0f);
        pMovie->HandleEvent(event);
    }
    else
    {
        ::ReleaseCapture();

        if (MouseTracking != None)
        {
            MouseTracking = None;
            return;
        }

        GFxMouseEvent event(GFxEvent::MouseUp, button, x, y, 0.0f);
        pMovie->HandleEvent(event);
    }
}

void    FxPlayerApp::OnMouseWheel(SInt zdelta, SInt x, SInt y)
{
    if (ControlKeyDown)// && MouseTracking == None)
    {
        ZoomStart = Zoom;

        Float dZoom = Zoom;
        Zoom += 0.02f * (zdelta/128) * Zoom;

        if (Zoom < 0.02f)
            Zoom = dZoom;

        dZoom -= Zoom;

        GSizeF  s = GetMovieScaleSize();
        Float   mX = ((x - (Width-ViewWidth)/2)) * s.Width;
        Float   mY = ((y - (Height-ViewHeight)/2)) * s.Height;
        GRenderer::Matrix m;
        m.AppendScaling(ZoomStart);
        m.AppendTranslation(Move.x,Move.y);
        GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));
        mX = (Float) (int) (p.x / s.Width);
        mY = (Float) (int) (p.y / s.Height);

        Move.x += s.Width * dZoom * mX;
        Move.y += s.Height * dZoom * mY;

        UpdateUserMatrix();
        return;
    }

    if (!pMovie)
        return;

    GSizeF  s = GetMovieScaleSize();

    // Adjust x, y to viewport.
    Float mX = ((x - (Width-ViewWidth)/2)) * s.Width;
    Float mY = ((y - (Height-ViewHeight)/2)) * s.Height;

    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x,Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    GFxMouseEvent event(GFxEvent::MouseWheel, 0, x, y, (Float)((zdelta/WHEEL_DELTA)*3));
    pMovie->HandleEvent(event);
}

void    FxPlayerApp::OnMouseMove(SInt x, SInt y)
{   
    Float dX = (Float) MouseX - x;
    Float dY = (Float) MouseY - y;
    MouseX = x;
    MouseY = y;

    // Used by NotifyMouseState in the main loop
    if (!pMovie)
        return;

    GSizeF  s = GetMovieScaleSize();

    if (MouseTracking == Zooming)
    {
        Float dZoom = Zoom;
        Zoom += 0.01f * dY * Zoom;

        if (Zoom < 0.02f)
            Zoom = dZoom;

        dZoom -= Zoom;

        Float mX = ((MouseDownX - (Width-ViewWidth)/2)) * s.Width;
        Float mY = ((MouseDownY - (Height-ViewHeight)/2)) * s.Height;
        GRenderer::Matrix m;
        m.AppendScaling(ZoomStart);
        m.AppendTranslation(MoveStart.x,MoveStart.y);
        GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));
        mX = (Float) (int) (p.x / s.Width);
        mY = (Float) (int) (p.y / s.Height);

        Move.x += s.Width * dZoom * mX;
        Move.y += s.Height * dZoom * mY;

        UpdateUserMatrix();
        return;
    }
    if (MouseTracking == Moving)
    {   
        Move.x -= s.Width * dX;
        Move.y -= s.Height * dY;

        UpdateUserMatrix();
        return;
    }
    

    // Adjust x, y to viewport.
    Float mX = ((x - (Width-ViewWidth)/2)) * s.Width;
    Float mY = ((y - (Height-ViewHeight)/2)) * s.Height;

    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x,Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    GFxMouseEvent event(GFxEvent::MouseMove, 0, x, y, 0.0f);
    pMovie->HandleEvent(event);
}


// *** Static Callbacks

void    FxPlayerApp::FxPlayerRTTFSCommandHandler::Callback(GFxMovieView* pmovie, const char* command, const char* args)
{
	GFxLog *plog = pmovie->GetLog();
	if (plog)
	{
		plog->LogMessage("FsCallback: '");
		plog->LogMessage(command);
		plog->LogMessage("' '");
		plog->LogMessage(args);
		plog->LogMessage("'\n");
	}

	if (!strcmp(command, "wireframe") && pApp)
		pApp->CubeWireframe = !strcmp(args, "1");
	else if (!strcmp(command, "exit") && pApp)
		pApp->QuitFlag = 1;
}
// Older window do not define this.
#ifndef IDC_HAND
#define IDC_HAND IDC_ARROW
#endif

void    FxPlayerApp::UserEventHandler(GFxMovieView* pmovie, const GFxEvent& event, void* puserData)
{
#ifdef FXPLAYER_RENDER_DIRECT3D
    FxPlayerApp* papp = reinterpret_cast<FxPlayerApp*>(puserData);
    switch(event.Type)
    {
    case GFxEvent::DoShowMouse:
        papp->ShowCursor(true);
        break;
    case GFxEvent::DoHideMouse:
        papp->ShowCursor(false);
        break;
    case GFxEvent::DoSetMouseCursor:
        {
            const GFxMouseCursorEvent& mcEvent = static_cast<const GFxMouseCursorEvent&>(event);
            switch(mcEvent.CursorShape)
            {
            case GFxMouseCursorEvent::ARROW:
                papp->SetCursor(::LoadCursor(NULL, IDC_ARROW));
                break;
            case GFxMouseCursorEvent::HAND:
                papp->SetCursor(::LoadCursor(NULL, IDC_HAND));
                break;
            case GFxMouseCursorEvent::IBEAM:
                papp->SetCursor(::LoadCursor(NULL, IDC_IBEAM));
                break;
            }
        }
        break;
    }
#endif
}

#ifdef  FXPLAYER_RENDER_DIRECT3D

// ***** D3D9 Specific

// Override to initialize D3D settings..
bool    FxPlayerApp::InitRenderer()
{
    return 1;
}
void FxPlayerApp::PrepareRendererForFrame()
{
}


#else

// ***** OpenGL Specific

// Override to initialize OpenGL viewport
bool    FxPlayerApp::InitRenderer()
{

    // Change the LOD BIAS values to tweak text blurriness.
    if (Settings.TexLodBias != 0.0f) 
    {
#ifdef FIX_I810_LOD_BIAS    
        // If 2D textures weren't previously enabled, enable them now and force 
        // the driver to notice the update, then disable them again.
        if (!glIsEnabled(GL_TEXTURE_2D))
        {
            // Clearing a mask of zero *should* have no side effects, but coupled
            // with enabling GL_TEXTURE_2D it works around a segmentation fault         
            // in the driver for the Intel 810 chip.
            glEnable(GL_TEXTURE_2D);
            glClear(0);
            glDisable(GL_TEXTURE_2D);
        }
#endif // FIX_I810_LOD_BIAS
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, Settings.TexLodBias);
    }

    // Turn on line smoothing.  Antialiased lines can be used to
    // smooth the outsides of shapes.
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); // GL_NICEST, GL_FASTEST, GL_DONT_CARE

    // Setup matrices.
    glMatrixMode(GL_PROJECTION);
    glOrtho(-1, 1, 1, -1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    return 1;
}

void FxPlayerApp::PrepareRendererForFrame()
{
    // Draw on back buffer.
    glDrawBuffer(GL_BACK);
}

#endif


// ***** FxPlayerSettings Implementation


// Initializes settings based on the command line.
// Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
bool    FxPlayerSettings::ParseCommandLine(int argc, char *argv[])
{
    if (argc <= 1)
        return 1;

    for (int arg = 1; arg < argc; arg++)
    {
        if (argv[arg][0] == '/')
        {
            // Looks like an option.

            if (argv[arg][1] == '?')
            {
                // Help.
                PrintUsage();
                return 0;
            }

            else if (argv[arg][1] == 's')
            {
                // Scale.
                arg++;
                if (arg < argc)                 
                    ScaleX = ScaleY = GTL::gclamp<float>((float) atof(argv[arg]), 0.01f, 100.f);
                else
                {
                    fprintf(stderr, "/s option requires a scale value.\n");
                    return 0;
                }
            }

            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 'a'))
            {
                // Disable antialiasing - '/na'
                AAMode = AAMode_None;
            }
            else if ((argv[arg][1] == 'f') && (argv[arg][2] == 's') && (argv[arg][3] == 'a'))
            {
                // Force HW FASS - '/fsa'
                AAMode = AAMode_FSAA;
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
                        fprintf(stderr, "/b - specified bit depth must be 16 or 32.\n");
                        return 0;
                    }
                }
                else
                {
                    fprintf(stderr, "/b requires bit depth, ex: \"-b 16\"  or \"-b 32\".\n");
                    return 0;
                }
            }            

            else if (argv[arg][1] == 'p')
            {
                // Enable frame-rate/performance logging.
                MeasurePerformance = 1;
            }


            else if (argv[arg][1] == 'i')
            {
                // Display Info hud screen at at startup.
                HudState = Hud_Stats;
            }
            
            else if (argv[arg][1] == 'f')
            {
                // Enable fast-forward playback
                if (argv[arg][2] == 'f')
                    FastForward = 1;
                else if (argv[arg][2] == 0)
                {
                    FullScreen = 1;
                }
            }
            
            else if (argv[arg][1] == '1')
            {
                // Play once; don't loop.
                DoLoop = 0;
            }

            else if (argv[arg][1] == 'r')
            {
                // Set rendering on/off.
                arg++;
                if (arg < argc)
                {
                    const int render_arg = atoi(argv[arg]);
                    switch (render_arg) 
                    {
                        case 0:
                            // Disable both
                            DoRender    = 0;
                            DoSound     = 0;
                            break;
                        case 1:
                            // Enable both
                            DoRender    = 1;
                            DoSound     = 1;
                            break;
                        case 2:
                            // Disable just sound
                            DoRender    = 1;
                            DoSound     = 0;
                            break;

                        default:
                            fprintf(stderr, "/r requires a value of 0, 1 or 2 (%d is invalid).\n",
                                render_arg);
                            return 0;
                    }
                } 
                else 
                {
                    fprintf(stderr, "/r requires a value of 0/1 to disable/enable rendering.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 't')
            {
                // Set timeout.
                arg++;
                if (arg < argc)                 
                    ExitTimeout = (float) atof(argv[arg]);
                else
                {
                    fprintf(stderr, "/t requires a timeout value, in seconds.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'v')
            {
                if (Quiet)
                {
                    fprintf(stderr, "Quiet option /q conflicts with %s verbose option.\n", argv[arg]);
                    return 0;
                }

                // Be verbose; i.e. print log messages to stdout.
                if (argv[arg][2] == 'a')
                {
                    // Enable spew re: action.
                    VerboseAction = 1;                  
                }
                else if ((argv[arg][2] == 'p') && (argv[arg][3] == 0))
                {
                    // Enable parse spew.
                    VerboseParse = 1;
                }               
                else if ((argv[arg][2] == 'p') && (argv[arg][3] == 's'))
                {
                    // Enable parse shape.
                    VerboseParseShape = 1;
                }
                else if ((argv[arg][2] == 'p') && (argv[arg][3] == 'a'))
                {
                    // Enable parse action.
                    VerboseParseAction = 1;
                }
                else
                {
                    fprintf(stderr, "Unknown /v option type.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'q' && argv[arg][2] == 'a' && argv[arg][3] == 'e')
            {
                // Quiet. Opposite to verbose, will not disply any messages.
                NoActionErrors = 1;

                if (VerboseAction)
                {
                    fprintf(stderr, "Quiet option /qse conflicts with specified verbose options.\n");
                    return 0;
                }
            }
            else if (argv[arg][1] == 'q')
            {
                // Quiet. Opposite to verbose, will not disply any messages.
                Quiet = 1;

                if (VerboseAction || VerboseParseAction || VerboseParseShape || VerboseParse)
                {
                    fprintf(stderr, "Quiet option /q conflicts with specified verbose options.\n");
                    return 0;
                }
            }

            else if (argv[arg][1] == 'm')
            {
                if (argv[arg][2] == 'l')
                {
                    arg++;
                    TexLodBias = (float) atof(argv[arg]);
                    //printf("Texture LOD Bais is no %f\n", tex_lod_bias);
                }
                else
                {
                    fprintf(stderr, "Unknown /m option type.\n");
                    return 0;
                }
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
        printf("Note: No input file specified. Use /? option for help. \n");
        return 1;
    }

    return 1;
}

    
// Brief instructions.
void    FxPlayerSettings::PrintUsage()
{

    printf(
        "GFxPlayer - a sample SWF file player for the GFx library.\n"
        "\n"
        "Copyright (c) 2006 Scaleform Corp. All Rights Reserved.\n"
        "Contact sales@scaleform.com for licensing information.\n"
        "\n"
        "Usage:        gfxplayer [options] movie_file.swf\n"        
        "Options:\n"
        "  /?          Display this help info.\n"
        "  /s <factor> Scale the movie window size by the specified factor.\n"
        "  /na, /fsa   Use no anti-aliasing; use FullScreen HW AA.\n"
        "  /f          Run in full-screen mode.\n"
        "  /i          Display Info Hud on startup.\n"
        "  /vp         Verbose parse - print SWF parse log.\n"
        "  /vps        Verbose parse shape - print SWF shape parse log.\n"
        "  /vpa        Verbose parse action - print SWF actions during parse.\n"
        "  /va         Verbose Actions - display ActionScript execution log.\n"
        "  /q          Quiet. Do not display errors or trace statements.\n"
        "  /qae        Suppress ActionScript errors.\n"
        "  /ml <bias>  Specify the texture LOD bias (float, default -0.5).\n"
        "  /p          Performance test - run without delay and log FPS.\n"
        "  /ff         Fast forward - run one SWF frame per update.\n"
        "  /1          Play once; exit when/if movie reaches the last frame.\n"
        "  /r <0|1>    0 disables rendering  (for batch tests).\n"
        "              1 enables rendering (default setting).\n"
        //"              2 enables rendering & disables sound\n"
        "  /t <sec>    Timeout and exit after the specified number of seconds.\n"
        "  /b <bits>   Bit depth of output window (16 or 32, default is 16).\n"
        "\n"
        "Keys:\n"               
        "  CTRL S          Toggle scaled vs. centered display.\n"
        "  CTRL W          Toggle wireframe.\n"
        "  CTRL A          Toggle FSAA.\n"
        "  CTRL U          Toggle FullScreen.\n"
        //"  CTRL L          Toggle line anti-aliasing.\n"
        "  CTRL F          Toggle fast performance mode (FPS).\n"
        "  CTRL P          Toggle pause.\n"
        "  CTRL R          Restart the movie.\n"
        "  CTRL Right      Step backward one frame.\n"
        "  CTRL Left       Step forward one frame.\n"
        "  CTRL PageUp     Step back 10 frames.\n"
        "  CTRL PageDown   Step forward 10 frames.\n"
        "  CTRL -,+        Curve tolerance down, up.\n"
        "  CTRL Q,ESC      Quit.\n"
        "  F1              Toggle Info Help.\n"
        "  F2              Toggle Info Stats.\n"


//      "  CTRL-T          Debug.  Test the SetVariable() function\n"
//      "  CTRL-G          Debug.  Test the GetVariable() function\n"
//      "  CTRL-M          Debug.  Test the Invoke() function\n"
//      "  CTRL-B          Toggle background color fill/blending.\n"
        );
}
