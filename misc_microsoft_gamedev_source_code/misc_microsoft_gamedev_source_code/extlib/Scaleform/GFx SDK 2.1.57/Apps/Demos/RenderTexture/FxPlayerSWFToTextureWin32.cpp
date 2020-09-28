/**********************************************************************

Filename    :   RenderTexture.cpp
Content     :   Sample SWF file player leveraging GFxPlayer API
Created     :
Authors     :   Michael Antonov, Andrew Reisse
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

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
#include "GFxRenderConfig.h"
#include "GTimer.h"
// Helper include for simple matrix functions
//?#include "MathLib.h"
#include "gstd.h"
// Adds the word "Debug" to the application
// title if in debug build mode
#ifdef GFC_BUILD_DEBUG
#define GFC_DEBUG_STRING    " " GFC_BUILD_STRING
#else
#define GFC_DEBUG_STRING
#endif


#ifdef  FXPLAYER_RENDER_DIRECT3D
  #ifndef GFC_D3D_VERSION
  #define GFC_D3D_VERSION 9
  #endif
  #if (GFC_D3D_VERSION == 9)
    // Direct3D application class
    #include "Direct3DWin32App.h"
    #define FXPLAYER_APP    Direct3DWin32App
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFx Render Texture Sample D3D9 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
    #define FXPLAYER_USE_SWF_HUD 
  #elif (GFC_D3D_VERSION == 8)
    // Direct3D application class
    #include "Direct3DWin32App.h"
    #define FXPLAYER_APP    Direct3DWin32App
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFx Render Texture Sample D3D8 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
    #define FXPLAYER_USE_SWF_HUD
  #elif (GFC_D3D_VERSION == 10)
    // Direct3D application class
    #include "Direct3DWin32App.h"
    #define FXPLAYER_APP    Direct3DWin32App
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer D3D10 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
    #define FXPLAYER_USE_SWF_HUD
    #include "MathLib.h"
  #endif
#else
    // OpenGL application class
    #include "OpenGLWin32App.h"
    #define FXPLAYER_APP    OpenGLWin32App
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFx Render Texture Sample OpenGL v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
#endif

#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <zmouse.h> // for WHEEL_DELTA macro

#include "../../Bin/FxPlayer/fxplayer.swf.h"

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
    UInt32      VMCFlags;

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
        VMCFlags            = 0;

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

#ifdef FXPLAYER_RENDER_DIRECT3D

#if (GFC_D3D_VERSION == 9)
#define IDirect3DDeviceX        IDirect3DDevice9
#define IDirect3DTextureX       IDirect3DTexture9
#define IDirect3DSurfaceX       IDirect3DSurface9
#define IDirect3DIndexBufferX   IDirect3DIndexBuffer9
#define IDirect3DVertexBufferX  IDirect3DVertexBuffer9
#define D3DVIEWPORTx            D3DVIEWPORT9

// Used for functions that have an extra last argument of NULL in D3D9 only
#define NULL9                   , NULL

#elif (GFC_D3D_VERSION == 8)
#define IDirect3DDeviceX        IDirect3DDevice8
#define IDirect3DTextureX       IDirect3DTexture8
#define IDirect3DSurfaceX       IDirect3DSurface8
#define IDirect3DIndexBufferX   IDirect3DIndexBuffer8
#define IDirect3DVertexBufferX  IDirect3DVertexBuffer8
#define D3DVIEWPORTx            D3DVIEWPORT8

// Used for functions that have an extra last argument of NULL in D3D9 only
#define NULL9
#endif
#endif

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
    GFxMovieInfo        BGInfo;
    GPtr<GFxMovieDef>   pBGDef;
    GPtr<GFxMovieView>  pBG;

    // Movie timing state
    float       SpeedScale;         // Advance speed, def 1.0f
    SInt        FrameCounter;       // Frames rendered, for FPS
    UInt        TessTriangles;      // Tess triangles for log.
    // Time ticks: always rely on a timer, for FPS
    UInt64      TimeStartTicks;     // Ticks during the start of playback
    UInt64      TimeTicks;          // Current ticks
    UInt64      LastLoggedFps;      // Time ticks during last FPS log
    UInt64      NextTicksTime;      // Ticks when next advance should be called.
    // Movie logical ticks: either timer or setting controlled
    UInt64      MovieStartTicks;
    UInt64      MovieLastTicks;
    UInt64      MovieTicks;

    // Renderer we use
    GPtr<GFxRenderConfig>   pRenderConfig;
    GPtr<GFxRenderStats>  pRenderStats;
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
        Tilting,
        Centering,
    };
    TrackingState       MouseTracking;
    TrackingState       TextureTilt;
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
    char                MessageText[1024];
    GPtr<GFxMovieView>  pHud;
    GViewport           HudViewport;
    GRenderer::Matrix   UserMatrix;

    // Curve error
    Float               CurvePixelError;

    // Width, height during sizing
    SInt                SizeWidth, SizeHeight;
    bool                SizingEntered;

    // Old width and height saved during FullScreen mode
    SInt                OldWindowX, OldWindowY;
    SInt                OldWindowWidth, OldWindowHeight;

#ifdef FXPLAYER_RENDER_DIRECT3D
#if (GFC_D3D_VERSION == 10)
    GPtr<ID3D10Texture2D>           pRenderTexture;
    GPtr<ID3D10RenderTargetView>    pRenderTextureView;
    GPtr<ID3D10ShaderResourceView>  pRenderTextureSV;
    GPtr<ID3D10DepthStencilView>    pRTDepthStencil;
    GPtr<ID3D10Texture2D>           pRTDepthStencilBuf;
    GPtr<ID3D10Buffer>              pCubeVertexBuffer;
    GPtr<ID3D10Buffer>              VShaderConst;
    GPtr<ID3D10DepthStencilState>   pDepthTest;
    GPtr<ID3D10RasterizerState>     pRSFill;
    GPtr<ID3D10BlendState>          pBlendState;
    GPtr<ID3D10VertexShader>        p3DVShader;
    GPtr<ID3D10PixelShader>         pTex2dShader;
    GPtr<ID3D10InputLayout>         p3DILayout;
#else
    GPtr<IDirect3DTextureX>  pRenderTexture;
    GPtr<IDirect3DSurfaceX>  pStencilSurface;
    GPtr<IDirect3DVertexBufferX>     pCubeVertexBuffer;
#endif
#else
    GLuint                  RenderTextureId;
    GLuint                  RenderTextureFbo;
    bool                    PackedDepthStencil;
#endif
    SInt                    RTWidth, RTHeight;
    Float                   MeshRotationX;
    Float                   MeshRotationZ;
    UInt64                  LastRotationTick;
    Float                   InvMV[16];
    Float                   Proj[16], InvProj[16];
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
    void            RenderMovie();
    void            RenderMovieTexture();

    // *** Overrides

    // Sizing; by default, re-initalizes the renderer
    virtual void    OnSize(int w, int h); 
    virtual void    OnSizeEnter(bool enterSize);
    virtual void    OnDropFiles(char *path);

    // Input
    virtual void    OnKey(KeyCode keyCode, unsigned char asciiCode, unsigned int wcharCode, 
        unsigned int mods, bool downFlag) ;
    virtual void    OnChar(UInt32 wcharCode, UInt info);
    virtual void    OnMouseButton(unsigned int button, bool downFlag, int x, int y, 
        unsigned int mods);
    virtual void    OnMouseWheel(int zdelta, int x, int y, unsigned int mods);
    virtual void    OnMouseMove(int x, int y, int unsigned mods);
    // Override to initialize OpenGL viewport
    virtual bool    InitRenderer();
    virtual void    PrepareRendererForFrame();

    // Helper used to convert key codes and route them to GFxPlayer
    void            KeyEvent(GFxKey::Code keyCode, unsigned char asciiCode, unsigned int wcharCode, unsigned int mods, bool down);


    // *** Static callbacks

    // ***** GFx State classes (callback functionality)
    // File opener class.
    class FxPlayerRFTFileOpener : public GFxFileOpener
    {
    public:
        virtual GFile* OpenFile(const char *pfilename)
        {
#ifdef FXPLAYER_USE_SWF_HUD
            if (!strcmp(pfilename, "  fxplayer.swf"))
                return new GMemoryFile(pfilename, fxplayer_swf, sizeof(fxplayer_swf));
#endif

            return new GSysFile(pfilename);
        }
    };
    // "fscommand" callback, handles notification callbacks from ActionScript.
    class FxPlayerRFTFSCommandHandler : public GFxFSCommandHandler
    {
    public:
        virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg);
    };
    // event user handler
    static void     GCDECL UserEventHandler(GFxMovieView* movie, const GFxEvent& event, void* puserData);
};


class FxPlayerUserEventHandler : public GFxUserEventHandler
{
    FxPlayerApp* pApp;
public:

    FxPlayerUserEventHandler(FxPlayerApp *papp)
    {
        pApp = papp;
    }

    virtual void HandleEvent(GFxMovieView* pmovie, const GFxEvent& event)
    {
        GUNUSED(pmovie);
        switch(event.Type)
        {
        case GFxEvent::DoShowMouse:
            pApp->ShowCursor(true);
            break;
        case GFxEvent::DoHideMouse:
            pApp->ShowCursor(false);
            break;
        case GFxEvent::DoSetMouseCursor:
            {
                const GFxMouseCursorEvent& mcEvent = static_cast<const GFxMouseCursorEvent&>(event);
                switch(mcEvent.CursorShape)
                {
                case GFxMouseCursorEvent::ARROW:
                    pApp->SetCursor(::LoadCursor(NULL, IDC_ARROW));
                    break;
                case GFxMouseCursorEvent::HAND:
                    pApp->SetCursor(::LoadCursor(NULL, IDC_HAND));
                    break;
                case GFxMouseCursorEvent::IBEAM:
                    pApp->SetCursor(::LoadCursor(NULL, IDC_IBEAM));
                    break;
                }
            }
            break;
        }
    }
};


class GFxPlayerLog : public GFxLog
{
public:
    // We override this function in order to do custom logging.
    virtual void    LogMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
    {
        GUNUSED(messageType);
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
    MessageText[0]      = 0;

    ViewWidth   = 0;
    ViewHeight  = 0;

    Zoom = 1.0;
    Move = GPointF(0.0);

    MouseTracking   = None;
    TextureTilt     = None;
    ControlKeyDown  = 0;

    SizingEntered = 0;

    CurvePixelError = 1.0f;

    // No old pos, save during FullScreen mode
    OldWindowX = OldWindowY = 0;
    OldWindowWidth = OldWindowHeight = 0;

    MeshRotationX           = 0;
    MeshRotationZ           = 0;
    LastRotationTick        = 0;
    CubeWireframe           = 0;
    pApp = this;
}

FxPlayerApp *FxPlayerApp::pApp = 0;

FxPlayerApp::~FxPlayerApp()
{
#ifdef FXPLAYER_RENDER_DIRECT3D
    if (pDevice)
#if (GFC_D3D_VERSION == 9)
        pDevice->SetStreamSource( 0, 0, 0, 0);
#elif (GFC_D3D_VERSION == 8)
        pDevice->SetStreamSource( 0, 0, 0);
#elif (GFC_D3D_VERSION == 10)
    {
        UInt n = 0;
        ID3D10Buffer *buf = 0;
        ID3D10ShaderResourceView* srs[2] = {0,0};

        pDevice->OMSetDepthStencilState(0, 0);
        pDevice->VSSetConstantBuffers(0, 1, &buf);
        pDevice->PSSetShaderResources(0, 2, srs);
        pDevice->RSSetState(0);
        pDevice->OMSetBlendState(0, 0, 0xffffffff);
        pDevice->OMSetDepthStencilState(0, 0);
        pDevice->IASetIndexBuffer(0, DXGI_FORMAT_UNKNOWN, 0);
        pDevice->IASetInputLayout(0);
        pDevice->IASetVertexBuffers(0, 1, &buf, &n, &n);
        pDevice->PSSetShader(0);
        pDevice->VSSetShader(0);
    }
#endif
#endif

    pApp = 0;
}

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
    //  virtual bool    Recreate(GRenderer* prenderer);
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


struct Vertex
{
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
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
    {-1.0f, 1.0f, 0.0f,  0.0f,0.0f },
    { 1.0f, 1.0f, 0.0f,  1.0f,0.0f },
    {-1.0f,-1.0f, 0.0f,  0.0f,1.0f },
    { 1.0f,-1.0f, 0.0f,  1.0f,1.0f },
};

#if (GFC_D3D_VERSION != 10)

#define M(x,r,c) x[(r)*4+(c)]
#define Mt(x,c,r) x[(r)*4+(c)]

static void MatrixMult(Float *O, const Float *A, const float *B)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(O, i,j) = M(A, i,0) * M(B, 0,j) + M(A, i,1) * M(B, 1,j) + M(A, i,2) * M(B, 2,j) + M(A, i,3) * M(B, 3,j);
}

static void VectorMult(Float *O, const Float *A, Float x, Float y, Float z, Float w)
{
    O[0] = Mt(A,0,0) * x + Mt(A,0,1) * y + Mt(A,0,2) * z + Mt(A,0,3) * w;
    O[1] = Mt(A,1,0) * x + Mt(A,1,1) * y + Mt(A,1,2) * z + Mt(A,1,3) * w;
    O[2] = Mt(A,2,0) * x + Mt(A,2,1) * y + Mt(A,2,2) * z + Mt(A,2,3) * w;
    O[3] = Mt(A,3,0) * x + Mt(A,3,1) * y + Mt(A,3,2) * z + Mt(A,3,3) * w;
}

static void VectorMult(Float *O, const Float *A, const Float *v)
{
    O[0] = Mt(A,0,0) * v[0] + Mt(A,0,1) * v[1] + Mt(A,0,2) * v[2] + Mt(A,0,3) * v[3];
    O[1] = Mt(A,1,0) * v[0] + Mt(A,1,1) * v[1] + Mt(A,1,2) * v[2] + Mt(A,1,3) * v[3];
    O[2] = Mt(A,2,0) * v[0] + Mt(A,2,1) * v[1] + Mt(A,2,2) * v[2] + Mt(A,2,3) * v[3];
    O[3] = Mt(A,3,0) * v[0] + Mt(A,3,1) * v[1] + Mt(A,3,2) * v[2] + Mt(A,3,3) * v[3];
}

static void VectorInvHomog(Float *v)
{
    v[0] *= v[3];
    v[1] *= v[3];
    v[2] *= v[3];
    v[3] = 1;
}

static void MakePerspective(Float *P, Float fov, Float aspect, Float z0, Float z1)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(P,i,j) = 0;

    Float dz = z1-z0;
    Float sinfov = sin(fov*0.5f);
    Float cotfov = cos(fov*0.5f) / sinfov;

    M(P,0,0) = cotfov/aspect;
    M(P,1,1) = cotfov;
    M(P,2,2) = -(z0 + z1) / dz;
    M(P,2,3) = -1;
    M(P,3,2) = -2 * z0 * z1 / dz;
}

static void MakeIdentity(Float *P)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(P,i,j) = 0;

    M(P,0,0) = 1;
    M(P,1,1) = 1;
    M(P,2,2) = 1;
    M(P,3,3) = 1;
}

static void MakeRotateX(Float *P, Float angle)
{
    Float s = sin(angle);
    Float c = cos(angle);

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(P,i,j) = 0;

    M(P,1,1) = c;
    M(P,2,2) = c;
    M(P,2,1) = -s;
    M(P,1,2) = s;
    M(P,0,0) = 1;
    M(P,3,3) = 1;
}

static void MakeRotateZ(Float *P, Float angle)
{
    Float s = sin(angle);
    Float c = cos(angle);

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(P,i,j) = 0;

    M(P,0,0) = c;
    M(P,2,2) = c;
    M(P,2,0) = -s;
    M(P,0,2) = s;
    M(P,1,1) = 1;
    M(P,3,3) = 1;
}

static void Translate(Float *P, Float x, Float y, Float z)
{
    for (int i = 0; i < 4; i++)
        M(P, 3,i) += x * M(P, 0,i) + y * M(P, 1,i) + z * M(P, 2,i);
}

static Float Cofactor(const Float *A, int I, int J)
{
    const int subs[4][3] = {{1,2,3},{0,2,3},{0,1,3},{0,1,2}};
#define SubM(m,i,j) M(m,subs[I][i],subs[J][j])

    Float a = SubM(A, 0,0) * SubM(A, 1,1) * SubM(A, 2,2);
    a      += SubM(A, 1,0) * SubM(A, 2,1) * SubM(A, 0,2);
    a      += SubM(A, 2,0) * SubM(A, 0,1) * SubM(A, 1,2);
    a      -= SubM(A, 0,0) * SubM(A, 2,1) * SubM(A, 1,2);
    a      -= SubM(A, 1,0) * SubM(A, 0,1) * SubM(A, 2,2);
    a      -= SubM(A, 2,0) * SubM(A, 1,1) * SubM(A, 0,2);

    return ((I + J) & 1) ? -a : a;
#undef SubM
}

static void MatrixInverse(Float *O, const Float *A)
{
    Float C[16];
    Float det = 0;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(C,i,j) = Cofactor(A, i,j);

    for (int i = 0; i < 4; i++)
        det += M(C, 0,i) * M(A, 0,i);

    det = 1.0f / det;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            M(O, j,i) = det * M(C, i,j);
}

#undef M
#endif

#ifdef FXPLAYER_RENDER_DIRECT3D
#if (GFC_D3D_VERSION == 10)
void    FxPlayerApp::SetupMatrices()
{
    Float mv[16], rot[16], lap[16], mat[16], mvp[16], mvpt[16];

    MakeIdentity(lap);
    Translate(lap, 0, 0, -2.5f * Zoom);
    MakeRotateX(rot, MeshRotationX * 3.141592f * 0.00555555f);
    MatrixMult(mat, rot, lap);
    MakeRotateZ(rot, MeshRotationZ * 3.141592f * 0.00555555f);
    MatrixMult(mv, rot, mat);

    MakePerspective(Proj, 3.141592f * 0.25f, Float(RTWidth)/Float(RTHeight), 0.1f, 100);

    MatrixInverse(InvProj, Proj);
    MatrixInverse(InvMV, mv);
    MatrixMult(mvp, mv, Proj);
    MatrixTranspose(mvpt, mvp);

    pDevice->UpdateSubresource(VShaderConst, 0, NULL, mvpt, 0, 0);
}

void    FxPlayerApp::RenderMovie()
{
    if (!pRenderTexture)
        return;

    pDevice->OMSetRenderTargets(1, &pRenderTextureView.GetRawRef(), pRTDepthStencil);

    D3D10_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width    = RTWidth;
    vp.Height   = RTHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pDevice->RSSetViewports(1, &vp);

    const float clear[] = {0,0,0,0};
    pDevice->ClearRenderTargetView(pRenderTextureView, clear);
    pDevice->ClearDepthStencilView(pRTDepthStencil, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);

    if (pMovie)
    {
        pMovie->SetViewport(RTWidth,RTHeight,0,0,RTWidth,RTHeight, GViewport::View_RenderTextureAlpha);
        pMovie->SetBackgroundAlpha(0);
        pMovie->Display();
    }

    pDevice->OMSetRenderTargets(1, &pRenderTarget, pDepthStencil);
}

void    FxPlayerApp::RenderMovieTexture()
{
    UInt64    ticks = GTimer::GetTicks() / 1000;

    if (TextureTilt == Centering)
    {
        float dt = 0.0f;
        float t  = (float)((double)fmod((double)ticks, 7500.0) / 7500.0) * 360;
        float lt = (float)((double)fmod((double)LastRotationTick, 7500.0) / 7500.0) * 360;
        dt       = t - lt;

        LastRotationTick    = ticks;
        if (MeshRotationX > 0.0f)
        {
            MeshRotationX        -= dt;
            if (MeshRotationX < 0.0f)
                MeshRotationX = 0.0f;
        }
        else if (MeshRotationX < 0.0f)
        {
            MeshRotationX        += dt;
            if (MeshRotationX > 0.0f)
                MeshRotationX = 0.0f;
        }

        if (MeshRotationZ > 0.0f)
        {
            MeshRotationZ        -= dt;
            if (MeshRotationZ < 0.0f)
                MeshRotationZ = 0.0f;
        }
        else if (MeshRotationZ < 0.0f)
        {
            MeshRotationZ        += dt;
            if (MeshRotationZ > 0.0f)
                MeshRotationZ = 0.0f;
        }

        if (MeshRotationZ == 0.0f && MeshRotationX == 0.0f)
        {
            TextureTilt = None;
        }
    }
    else if (TextureTilt == None && (ticks - LastRotationTick) >= 20)
    {
        float tiltMax   = 10.5f;
        float tiltDelta = 0.2f;

        if (MeshRotationZ > -tiltMax && MeshRotationX == 0 || MeshRotationX < -tiltMax)
            MeshRotationZ -= tiltDelta;

        if (MeshRotationX < tiltMax && MeshRotationZ < -tiltMax)
            MeshRotationX += tiltDelta;

        if (MeshRotationZ < tiltMax && MeshRotationX > tiltMax)
            MeshRotationZ += tiltDelta;

        if (MeshRotationX > -tiltMax && MeshRotationZ > tiltMax)
            MeshRotationX -= tiltDelta;

        LastRotationTick = ticks;
    }
    else if (TextureTilt != None)
        LastRotationTick = ticks;

    MeshRotationX = fmod(MeshRotationX, 360.0f);
    MeshRotationZ = fmod(MeshRotationZ, 360.0f);

    if (MeshRotationX > 45.0f)
        MeshRotationX = 45.0f;
    if (MeshRotationX < -45.0f)
        MeshRotationX = -45.0f;

    if (MeshRotationZ > 45.0f)
        MeshRotationZ = 45.0f;
    if (MeshRotationZ < -45.0f)
        MeshRotationZ = -45.0f;

    // Setup the world, view, and projection matrices
    SetupMatrices();

    D3D10_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width    = Width;
    vp.Height   = Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    pDevice->RSSetViewports(1, &vp);

    const float one[] = {1,1,1,1};
    pDevice->OMSetBlendState(pBlendState, one, 0xfffffff);
    pDevice->RSSetState(pRSFill);
    pDevice->OMSetDepthStencilState(pDepthTest, 0);
    pDevice->VSSetShader(p3DVShader);
    pDevice->VSSetConstantBuffers(0, 1, &VShaderConst.GetRawRef());
    pDevice->IASetInputLayout(p3DILayout);
    pDevice->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    pDevice->PSSetShader(pTex2dShader);
    //pDevice->PSSetSamplers(0, 1, &pTex2dSampler.GetRawRef());
    pDevice->PSSetShaderResources(0, 1, &pRenderTextureSV.GetRawRef());
    UInt stride = sizeof(Vertex);
    UInt offset = 0;
    pDevice->IASetVertexBuffers(0, 1, &pCubeVertexBuffer.GetRawRef(), &stride, &offset);

    pDevice->Draw(4, 0);

    ID3D10ShaderResourceView* srs[2] = {0,0};

    pDevice->PSSetShaderResources(0, 2, srs);
    pDevice->PSSetShader(0);
}

bool FxPlayerApp::SetupRTTexture()
{
    D3D10_TEXTURE2D_DESC texdesc;
    texdesc.Width = RTWidth;
    texdesc.Height = RTHeight;
    texdesc.MipLevels = 1;
    texdesc.ArraySize = 1;
    texdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texdesc.SampleDesc.Count = 1;
    texdesc.SampleDesc.Quality = 0;
    texdesc.Usage = D3D10_USAGE_DEFAULT;
    texdesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    texdesc.CPUAccessFlags = 0;
    texdesc.MiscFlags = 0;
    pDevice->CreateTexture2D(&texdesc, NULL, &pRenderTexture.GetRawRef());

    pDevice->CreateRenderTargetView(pRenderTexture, NULL, &pRenderTextureView.GetRawRef());
    pDevice->CreateShaderResourceView(pRenderTexture, NULL, &pRenderTextureSV.GetRawRef());

    texdesc.Width = RTWidth;
    texdesc.Height = RTHeight;
    texdesc.MipLevels = 1;
    texdesc.ArraySize = 1;
    texdesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    texdesc.SampleDesc.Count = 1;
    texdesc.SampleDesc.Quality = 0;
    texdesc.Usage = D3D10_USAGE_DEFAULT;
    texdesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    texdesc.CPUAccessFlags = 0;
    texdesc.MiscFlags = 0;
    pDevice->CreateTexture2D(&texdesc, NULL, &pRTDepthStencilBuf.GetRawRef());

    D3D10_DEPTH_STENCIL_VIEW_DESC dsv;
    dsv.Format = texdesc.Format;
    dsv.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    pDevice->CreateDepthStencilView( pRTDepthStencilBuf, &dsv, &pRTDepthStencil.GetRawRef());

    return 1;
}

#else // 9 or 8
void    FxPlayerApp::SetupMatrices()
{
    Float mv[16], rot[16], lap[16], mat[16];

    MakeIdentity(lap);
    Translate(lap, 0, 0, -2.5f * Zoom);
    MakeRotateX(rot, MeshRotationX * 3.141592f * 0.00555555f);
    MatrixMult(mat, rot, lap);
    MakeRotateZ(rot, MeshRotationZ * 3.141592f * 0.00555555f);
    MatrixMult(mv, rot, mat);

    MakePerspective(Proj, 3.141592f * 0.25f, Float(RTWidth)/Float(RTHeight), 0.1f, 100);

    MatrixInverse(InvProj, Proj);
    MatrixInverse(InvMV, mv);

    D3DXMATRIXA16 matWorld;
    D3DXMatrixIdentity(&matWorld);
    pDevice->SetTransform( D3DTS_WORLD, &matWorld );

    pDevice->SetTransform( D3DTS_VIEW, (D3DMATRIX*) mv );
    pDevice->SetTransform( D3DTS_PROJECTION, (D3DMATRIX*) Proj );
}

// Rendering
void    FxPlayerApp::RenderMovie()
{
    IDirect3DSurfaceX *poldSurface      = 0;
    IDirect3DSurfaceX *poldDepthSurface = 0;
    IDirect3DSurfaceX *psurface         = 0;

    pRenderTexture->GetSurfaceLevel(0, &psurface);

#if (GFC_D3D_VERSION == 9)
    // Save both RT and depth-stencil.
    pDevice->GetRenderTarget(0, &poldSurface);
    pDevice->GetDepthStencilSurface(&poldDepthSurface);
    pDevice->Clear( 0, NULL, D3DCLEAR_TARGET,
        D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0 );

    // Set texture as render target
    if (!FAILED(pDevice->SetRenderTarget(0, psurface )))
    {
        // Set stencil; this will disable it if not available.
        pDevice->SetDepthStencilSurface(pStencilSurface);
    }

#elif (GFC_D3D_VERSION == 8)
    // Save both RT and depth-stencil.
    pDevice->GetRenderTarget(&poldSurface);
    pDevice->GetDepthStencilSurface(&poldDepthSurface);

    // Set texture as render target
    pDevice->SetRenderTarget(psurface, pStencilSurface);
#endif

    D3DVIEWPORTx vp;
    vp.X        = 0;
    vp.Y        = 0;
    vp.Width    = RTWidth;
    vp.Height   = RTHeight;
    vp.MinZ     = 0.0f;
    vp.MaxZ     = 1.0f;
    pDevice->SetViewport(&vp);

    pDevice->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
        D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0 );

    if (pMovie)
    {   
        pMovie->SetViewport(RTWidth, RTHeight, 0, 0, RTWidth, RTHeight, GViewport::View_RenderTextureAlpha);
        pMovie->SetBackgroundAlpha(0);
        pMovie->Display();
    }

    // Restore the render target
#if (GFC_D3D_VERSION == 9)
    pDevice->SetRenderTarget(0, poldSurface);
    pDevice->SetDepthStencilSurface(poldDepthSurface);

#elif (GFC_D3D_VERSION == 8)
    pDevice->SetRenderTarget(poldSurface, poldDepthSurface);
#endif

    if (psurface)
        psurface->Release();
    if (poldSurface)
        poldSurface->Release();
    if (poldDepthSurface)
        poldDepthSurface->Release();

    // Need to do this so that mipmaps are updated.
    pRenderTexture->AddDirtyRect(0);
}

void    FxPlayerApp::RenderMovieTexture()
{
    UInt64    ticks = GTimer::GetTicks() / 1000;

    if (TextureTilt == Centering)
    {
        float dt = 0.0f;
        float t  = (float)((double)fmod((double)ticks, 7500.0) / 7500.0) * 360;
        float lt = (float)((double)fmod((double)LastRotationTick, 7500.0) / 7500.0) * 360;
        dt       = t - lt;

        LastRotationTick    = ticks;
        if (MeshRotationX > 0.0f)
        {
            MeshRotationX        -= dt;
            if (MeshRotationX < 0.0f)
                MeshRotationX = 0.0f;
        }
        else if (MeshRotationX < 0.0f)
        {
            MeshRotationX        += dt;
            if (MeshRotationX > 0.0f)
                MeshRotationX = 0.0f;
        }

        if (MeshRotationZ > 0.0f)
        {
            MeshRotationZ        -= dt;
            if (MeshRotationZ < 0.0f)
                MeshRotationZ = 0.0f;
        }
        else if (MeshRotationZ < 0.0f)
        {
            MeshRotationZ        += dt;
            if (MeshRotationZ > 0.0f)
                MeshRotationZ = 0.0f;
        }

        if (MeshRotationZ == 0.0f && MeshRotationX == 0.0f)
        {
            TextureTilt = None;
        }
    }
    else if (TextureTilt == None && (ticks - LastRotationTick) >= 20)
    {
        float tiltMax   = 10.5f;
        float tiltDelta = 0.2f;

        if (MeshRotationZ > -tiltMax && MeshRotationX == 0 || MeshRotationX < -tiltMax)
            MeshRotationZ -= tiltDelta;

        if (MeshRotationX < tiltMax && MeshRotationZ < -tiltMax)
            MeshRotationX += tiltDelta;

        if (MeshRotationZ < tiltMax && MeshRotationX > tiltMax)
            MeshRotationZ += tiltDelta;

        if (MeshRotationX > -tiltMax && MeshRotationZ > tiltMax)
            MeshRotationX -= tiltDelta;

        LastRotationTick = ticks;
    }
    else if (TextureTilt != None)
        LastRotationTick = ticks;

    MeshRotationX = fmod(MeshRotationX, 360.0f);
    MeshRotationZ = fmod(MeshRotationZ, 360.0f);

    if (MeshRotationX > 45.0f)
        MeshRotationX = 45.0f;
    if (MeshRotationX < -45.0f)
        MeshRotationX = -45.0f;

    if (MeshRotationZ > 45.0f)
        MeshRotationZ = 45.0f;
    if (MeshRotationZ < -45.0f)
        MeshRotationZ = -45.0f;


    // Setup the world, view, and projection matrices
    SetupMatrices();

    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

#if (GFC_D3D_VERSION == 9)
    pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

#elif (GFC_D3D_VERSION == 8)
    pDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
    pDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
    pDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
#endif

    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    D3DVIEWPORTx vp;
    vp.X        = 0;
    vp.Y        = 0;
    vp.Width    = Width;
    vp.Height   = Height;
    vp.MinZ     = 0.0f;
    vp.MaxZ     = 1.0f;
    pDevice->SetViewport(&vp);

    pDevice->BeginScene();

    pDevice->SetPixelShader(0);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

    // Blending
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    pDevice->SetTexture( 0, pRenderTexture );

#if (GFC_D3D_VERSION == 9)
    pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);

    pDevice->SetStreamSource( 0, pCubeVertexBuffer, 0, sizeof(Vertex) );
    pDevice->SetFVF( Vertex::FVF );
    pDevice->SetVertexShader(0);

#elif (GFC_D3D_VERSION == 8)
    pDevice->SetStreamSource( 0, pCubeVertexBuffer, sizeof(Vertex) );
    pDevice->SetVertexShader( Vertex::FVF );
#endif

    pDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  0, 2 );

    // Unbind render texture, so it can be updated.
    pDevice->SetTexture( 0, 0 );


    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    pDevice->EndScene();
}

// also recreates if lost
bool    FxPlayerApp::SetupRTTexture()
{
#if (GFC_D3D_VERSION == 9)
    if (FAILED( pDevice->CreateTexture(
        RTWidth,RTHeight,0,
        D3DUSAGE_RENDERTARGET|D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT, &pRenderTexture.GetRawRef(), 0) ))
        return 0;

    pDevice->CreateDepthStencilSurface( RTWidth,RTHeight, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0,
        TRUE, &pStencilSurface.GetRawRef(), NULL);

#elif (GFC_D3D_VERSION == 8)
    if (FAILED( pDevice->CreateTexture(
        RTWidth,RTHeight,1,
        D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8,
        D3DPOOL_DEFAULT, &pRenderTexture.GetRawRef()) ))
        return 0;

    pDevice->CreateDepthStencilSurface( RTWidth,RTHeight, D3DFMT_D24S8, D3DMULTISAMPLE_NONE,
        &pStencilSurface.GetRawRef());
#endif

    return 1;
}

#endif
#else

void    FxPlayerApp::RenderMovie()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, RenderTextureFbo);

    GLenum  x = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);
    if (x != GL_FRAMEBUFFER_COMPLETE_EXT)
        abort();

    glViewport(0,0, RTWidth, RTHeight);
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (pMovie)
    {   
        pMovie->SetViewport(RTWidth,RTHeight,0,0,RTWidth,RTHeight, GViewport::View_RenderTextureAlpha);
        pMovie->SetBackgroundAlpha(0);
        pMovie->Display();

    }
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    glBindTexture(GL_TEXTURE_2D, RenderTextureId);
    glGenerateMipmapEXT(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

}

void    FxPlayerApp::RenderMovieTexture()
{
    UInt64    ticks = GTimer::GetTicks() / 1000;

    if (TextureTilt == Centering)
    {
        float dt = 0.0f;
        float t  = (float)((double)fmod((double)ticks, 7500.0) / 7500.0) * 360;
        float lt = (float)((double)fmod((double)LastRotationTick, 7500.0) / 7500.0) * 360;
        dt = t - lt;
    
        LastRotationTick    = ticks;
        if (MeshRotationX > 0.0f)
        {
            MeshRotationX        -= dt;
            if (MeshRotationX < 0.0f)
                MeshRotationX = 0.0f;
        }
        else if (MeshRotationX < 0.0f)
        {
            MeshRotationX        += dt;
            if (MeshRotationX > 0.0f)
                MeshRotationX = 0.0f;
        }

        if (MeshRotationZ > 0.0f)
        {
            MeshRotationZ        -= dt;
            if (MeshRotationZ < 0.0f)
                MeshRotationZ = 0.0f;
        }
        else if (MeshRotationZ < 0.0f)
        {
            MeshRotationZ        += dt;
            if (MeshRotationZ > 0.0f)
                MeshRotationZ = 0.0f;
        }

        if (MeshRotationZ == 0.0f && MeshRotationX == 0.0f)
        {
            TextureTilt = None;
        }
    }
    else if (TextureTilt == None && (ticks - LastRotationTick) >= 20)
    {
        float tiltMax   = 10.5f;
        float tiltDelta = 0.2f;

        if (MeshRotationZ > -tiltMax && MeshRotationX == 0 || MeshRotationX < -tiltMax)
            MeshRotationZ -= tiltDelta;

        if (MeshRotationX < tiltMax && MeshRotationZ < -tiltMax)
            MeshRotationX += tiltDelta;

        if (MeshRotationZ < tiltMax && MeshRotationX > tiltMax)
            MeshRotationZ += tiltDelta;

        if (MeshRotationX > -tiltMax && MeshRotationZ > tiltMax)
            MeshRotationX -= tiltDelta;
        LastRotationTick = ticks;
    }
    else if (TextureTilt != None)
        LastRotationTick = ticks;

    if (MeshRotationX > 45.0f)
        MeshRotationX = 45.0f;
    if (MeshRotationX < -45.0f)
        MeshRotationX = -45.0f;

    if (MeshRotationZ > 45.0f)
        MeshRotationZ = 45.0f;
    if (MeshRotationZ < -45.0f)
        MeshRotationZ = -45.0f;

    glViewport(0,0,Width,Height);

    Float mv[16], rot[16], lap[16], mat[16];

    MakeIdentity(lap);
    Translate(lap, Move.x, Move.y, -2.5f * Zoom);
    MakeRotateX(rot, MeshRotationX * 3.141592f * 0.00555555f);
    MatrixMult(mat, rot, lap);
    MakeRotateZ(rot, MeshRotationZ * 3.141592f * 0.00555555f);
    MatrixMult(mv, rot, mat);

    MakePerspective(Proj, 3.141592f * 0.25f, Float(RTWidth)/Float(RTHeight), 0.1f, 100);

    MatrixInverse(InvProj, Proj);
    MatrixInverse(InvMV, mv);

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(Proj);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(mv);

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    glDisable(GL_VERTEX_PROGRAM_ARB);
    glColorMask(1,1,1,1);
    glDepthRange(0.1, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, RenderTextureId);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glEnable(GL_BLEND); 
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), g_cubeVertices);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &g_cubeVertices[0].tu);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glBindTexture(GL_TEXTURE_2D, 0);
}

bool    FxPlayerApp::SetupRTTexture()
{
    GLuint rbs[2];

    // Create texture and framebuffer
    glGenFramebuffersEXT(1, &RenderTextureFbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, RenderTextureFbo);

    glGenTextures(1, &RenderTextureId);
    glBindTexture(GL_TEXTURE_2D, RenderTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RTWidth,RTHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGenerateMipmapEXT(GL_TEXTURE_2D);
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, RenderTextureId, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffersEXT(2, rbs);

    GLenum status = 0;

    if (PackedDepthStencil)
    {
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbs[0]);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, 0x84f9, RTWidth,RTHeight);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbs[0]);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbs[0]);
        status = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);
    }

    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbs[0]);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, RTWidth,RTHeight);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rbs[1]);
        glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX, RTWidth,RTHeight);

        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbs[0]);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rbs[1]);
        status = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);
    }

    if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        // Try with no stencil buffer
        glDeleteRenderbuffersEXT(1, &rbs[1]);
        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    status = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    return (status == GL_FRAMEBUFFER_COMPLETE_EXT);
}

#endif


SInt    FxPlayerApp::Run()
{

//    UByte       verboseFlags = 0;


//    Loader.SetVerboseParse(verboseFlags);
    GPtr<GFxParseControl> pparseControl = *new GFxParseControl();
    pparseControl->SetParseFlags(Settings.VerboseParse ? GFxParseControl::VerboseParse : 0);
    Loader.SetParseControl(pparseControl);

    GPtr<GFxFileOpener> pfileOpener = *new FxPlayerRFTFileOpener;
    Loader.SetFileOpener(pfileOpener);

    GPtr<GFxFSCommandHandler> pcommandHandler = *new FxPlayerRFTFSCommandHandler;
    Loader.SetFSCommandHandler(pcommandHandler);


    // Set log, but only if not quiet
    if (!Settings.Quiet)
        Loader.SetLog(GPtr<GFxLog>(*new GFxPlayerLog()));

    // Load movie for the stats display
    GPtr<GFxMovieDef> pHudDef = *Loader.CreateMovie("  fxplayer.swf",
                                                    GFxLoader::LoadAll|GFxLoader::LoadOrdered
                                                    |GFxLoader::LoadKeepBindData|GFxLoader::LoadWaitCompletion);
    if (pHudDef)
    {
        pHud = *pHudDef->CreateInstance();
        if (pHud)
        {
            pHud->SetBackgroundAlpha(0);
#ifdef FXPLAYER_FONT_SIZE
            pHud->Invoke("_root.setHudFontSize", "%d", FXPLAYER_FONT_SIZE);
#endif
        }
    }

    if (strlen(Settings.FileName)==0)
#ifdef GFC_D3D_VERSION
        gfc_strcpy(Settings.FileName, 256, "3DWindow.swf");
#else
        gfc_strcpy(Settings.FileName, 256, "Mouse.swf");           
#endif

    bool loadMovie = strlen(Settings.FileName)>0;

    // Get info about the width & height of the movie.
    if (!loadMovie || !Loader.GetMovieInfo(Settings.FileName, &MovieInfo))
    {
        if (loadMovie)
            fprintf(stderr, "Error: failed to get info about %s\n", Settings.FileName);

        //return 1;
    }

    ViewWidth   = 1024;
    ViewHeight  = 1024;

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
        VMCFlags        = Settings.VMCFlags;

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
        if (!CreateRenderer())
        {
            return 1;
        }
    }

    // Set renderer on loader so that it is also applied to all children.
    pRenderConfig = *new GFxRenderConfig(GetRenderer());
    Loader.SetRenderConfig(pRenderConfig);

    // Create a renderer stats object since we will be tracking statistics.
    pRenderStats = *new GFxRenderStats();
    Loader.SetRenderStats(pRenderStats);

#ifndef FXPLAYER_RENDER_DIRECT3D
    const char *glexts = (const char *) glGetString(GL_EXTENSIONS);

    if (!CheckExtension(glexts, "EXT_framebuffer_object"))
    {
#ifdef GFC_OS_WIN32
        MessageBox(hWND, "EXT_framebuffer_object is required.", "Missing GL Extension", MB_OK);
#endif
        return 0;
    }

    PackedDepthStencil = CheckExtension(glexts, "EXT_packed_depth_stencil");
#endif

    RTWidth = 1024;
    RTHeight = 1024;
    SetupRTTexture();

#ifdef FXPLAYER_RENDER_DIRECT3D
#if (GFC_D3D_VERSION == 10)
    D3D10_BUFFER_DESC bd;
    memset(&bd, 0, sizeof(D3D10_BUFFER_DESC));
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
    bd.ByteWidth = sizeof(g_cubeVertices);
    D3D10_SUBRESOURCE_DATA vd;
    vd.pSysMem = g_cubeVertices;
    vd.SysMemPitch = vd.SysMemSlicePitch = 0;
    pDevice->CreateBuffer(&bd, &vd, &pCubeVertexBuffer.GetRawRef());

    memset(&bd, 0, sizeof(D3D10_BUFFER_DESC));
    bd.Usage = D3D10_USAGE_DEFAULT;
    bd.BindFlags = D3D10_BIND_CONSTANT_BUFFER;

    bd.ByteWidth = sizeof(float) * 16;
    pDevice->CreateBuffer(&bd, NULL, &VShaderConst.GetRawRef());

    GPtr<ID3D10Blob> pshader;
    GPtr<ID3D10Blob> pmsg;
    HRESULT hr;

    static const char *p3dVShaderText = 
        "cbuffer VSConstants {\n"
        "  float4x4 mvp;\n"
        "}\n"
        "void main(float4     pos  : POSITION,\n"
        "          float2     tc0  : TEXCOORD0,\n"
        "          out float2 otc0 : TEXCOORD0,\n"
        "          out float4 opos : SV_Position)\n"
        "{ opos = mul(pos, mvp); otc0 = tc0; }\n"
        ;

    hr = D3D10CompileShader(p3dVShaderText, strlen(p3dVShaderText), NULL,
        NULL, NULL, "main", "vs_4_0", 0, &pshader.GetRawRef(), &pmsg.GetRawRef());
    if (FAILED(hr))
    {
        GFC_DEBUG_WARNING1(1, "VertexShader errors:\n %s ", pmsg->GetBufferPointer() );
        return 0;
    }
    static D3D10_INPUT_ELEMENT_DESC VertexDecl[] =
    {
        {"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D10_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0},
    };
    pDevice->CreateVertexShader(pshader->GetBufferPointer(), pshader->GetBufferSize(), &p3DVShader.GetRawRef());
    pDevice->CreateInputLayout(VertexDecl, 2, pshader->GetBufferPointer(), pshader->GetBufferSize(), &p3DILayout.GetRawRef());

    static const char *pTex2dShaderText = 
        "Texture2D tex : register(t0);\n"
        "SamplerState samp\n"
        "{\n"
        "  Filter = MIN_MAG_MIP_LINEAR;\n"
        "};\n"
        "void main(float2 tc0        : TEXCOORD0,\n"
        "          out float4 ocolor : SV_Target)\n"
        "{ ocolor = tex.Sample(samp, tc0);\n}\n";

    hr = D3D10CompileShader(pTex2dShaderText, strlen(pTex2dShaderText), NULL,
        NULL, NULL, "main", "ps_4_0", 0, &pshader.GetRawRef(), &pmsg.GetRawRef());
    if (FAILED(hr))
    {
        GFC_DEBUG_WARNING1(1, "PixelShader errors:\n %s ", pmsg->GetBufferPointer() );
        return 0;
    }
    pDevice->CreatePixelShader(pshader->GetBufferPointer(), pshader->GetBufferSize(), &pTex2dShader.GetRawRef());

    D3D10_DEPTH_STENCIL_DESC ds;
    memset(&ds, 0, sizeof(D3D10_DEPTH_STENCIL_DESC));
    ds.DepthFunc = D3D10_COMPARISON_ALWAYS;
    ds.DepthEnable = 0;
    ds.DepthWriteMask = D3D10_DEPTH_WRITE_MASK_ZERO;
    ds.FrontFace.StencilDepthFailOp = ds.BackFace.StencilDepthFailOp = D3D10_STENCIL_OP_KEEP;
    ds.FrontFace.StencilFailOp = ds.BackFace.StencilFailOp = D3D10_STENCIL_OP_KEEP;
    ds.FrontFace.StencilPassOp = ds.BackFace.StencilPassOp = D3D10_STENCIL_OP_KEEP;
    ds.FrontFace.StencilFunc = ds.BackFace.StencilFunc = D3D10_COMPARISON_ALWAYS;
    pDevice->CreateDepthStencilState(&ds, &pDepthTest.GetRawRef());

    D3D10_RASTERIZER_DESC rs;
    memset(&rs, 0, sizeof(D3D10_RASTERIZER_DESC));
    rs.FillMode = D3D10_FILL_SOLID;
    rs.CullMode = D3D10_CULL_NONE;
    rs.ScissorEnable = 0;
    rs.MultisampleEnable = 0;
    rs.AntialiasedLineEnable = 1;
    pDevice->CreateRasterizerState(&rs, &pRSFill.GetRawRef());

    const D3D10_BLEND_DESC blend =     { 0, {1}, D3D10_BLEND_ONE, D3D10_BLEND_INV_SRC_ALPHA, D3D10_BLEND_OP_ADD,
        D3D10_BLEND_ONE,       D3D10_BLEND_INV_SRC_ALPHA, D3D10_BLEND_OP_ADD, {D3D10_COLOR_WRITE_ENABLE_ALL} };
    pDevice->CreateBlendState(&blend, &pBlendState.GetRawRef());

#else
    pDevice->CreateVertexBuffer( 24*sizeof(Vertex),0, Vertex::FVF,
        D3DPOOL_MANAGED, &pCubeVertexBuffer.GetRawRef() NULL9);
    void *pVertices = NULL;
#if (GFC_D3D_VERSION == 9)
    pCubeVertexBuffer->Lock( 0, sizeof(g_cubeVertices), (void**)&pVertices, 0 );
#elif (GFC_D3D_VERSION == 8)
    pCubeVertexBuffer->Lock( 0, sizeof(g_cubeVertices), (BYTE**)&pVertices, 0 );
#endif
    memcpy( pVertices, g_cubeVertices, sizeof(g_cubeVertices) );
    pCubeVertexBuffer->Unlock();
#endif
#endif

    // Load movie and initialize timing.
    if (loadMovie && !LoadMovie(Settings.FileName))
    {
        //return 1;
    }


    while (1)
    {
        TimeTicks = GTimer::GetTicks()/1000;

        if (Settings.DoRender && !Settings.FastForward)
            MovieTicks = TimeTicks;
        else // Simulate time.
            MovieTicks = MovieLastTicks + (UInt32) (1000.0f / MovieInfo.FPS);

        int     deltaTicks  = (int)(MovieTicks - MovieLastTicks);
        float   deltaT      = deltaTicks / 1000.f;

        MovieLastTicks = MovieTicks;

        // Check auto exit timeout counter.
        if ((Settings.ExitTimeout > 0) &&
            (MovieTicks - MovieStartTicks > (UInt32) (Settings.ExitTimeout * 1000)) )
            break;

        // Process messages and exit if necessary.
        if (!ProcessMessages())
            break;

        // *** Advance/Render movie

        // This is technically necessary only for D3D
        DisplayStatus status = CheckDisplayStatus();
        if (status == DisplayStatus_Unavailable) 
        {
            SleepMilliSecs(10);
            continue;
        }
        if (status == DisplayStatus_NeedsReset) 
        {
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
            if (pRenderTexture)
                pRenderTexture->Release();
            if (pStencilSurface)
                pStencilSurface->Release();
#endif
            RecreateRenderer();
            SetupRTTexture();
        }
#ifdef FXPLAYER_RENDER_DIRECT3D
        // prevent lost device from stopping resize handling
        if (!Width && !Height && SizeHeight && SizeWidth)
            OnSize(SizeWidth, SizeHeight);
#endif

        // Potential out-of bounds range is not a problem here,
        // because it will be adjusted for inside of the player.
        if (pMovie)
        {
            Float timeTillNextTicks;

            if (!Paused)
            {
                timeTillNextTicks = pMovie->Advance(deltaT * SpeedScale, 0);
                if (pBG)
                    pBG->Advance(deltaT * SpeedScale, 0);
            }
            else
                timeTillNextTicks = 0.05f;

            NextTicksTime = TimeTicks + (UInt32)(timeTillNextTicks * 1000.0f);
            if (NextTicksTime < TimeTicks) // wrap-around check.
                NextTicksTime = TimeTicks;
        }

        if (Settings.DoRender)
        {
            if (Wireframe)
                SetWireframe(1);
            RenderMovie();
            SetWireframe(0);

            // Renderer-specific preparation (Disable depth testing)
            PrepareRendererForFrame();
            GetRenderer()->BeginFrame();

            // Clear the entire buffer.
            Clear(GColor(233,236,226,255).ToColor32());

            if (pBG)
            {
                pBG->SetViewport(Width, Height, 0,0, Width, Height);
                pBG->SetBackgroundAlpha(0);
                pBG->Display();
            }

            RenderMovieTexture();
        }

        FrameCounter++;


        if (Settings.DoRender)
        {
            SetWireframe(0);

            if (pMovie && (LastFrame != pMovie->GetCurrentFrame()))
                NeedHudUpdate = 1;

            // Get stats every frame
            GRenderer::Stats    renderStats;
            GetRenderer()->GetRenderStats(&renderStats, 1);
            // If ballpark triangle count changed, need update
            if (((renderStats.Triangles >> 11) != (LastStats.Triangles >> 11)) ||
                (renderStats.Primitives != LastStats.Primitives))
                NeedHudUpdate = 1;
            LastStats = renderStats;

            if (NeedHudUpdate && pHud)
            {
                UpdateHudText();
                HudViewport = GViewport(GetWidth(), GetHeight(),
                    SInt(GetWidth()*GetSafeArea()),
                    SInt(GetHeight()*GetSafeArea()),
                    SInt(GetWidth() - 2*GetWidth()*GetSafeArea()),
                    SInt(GetHeight() - 2*GetHeight()*GetSafeArea()));

                pHud->SetViewport(HudViewport);
                pHud->Invoke("_root.setHudText", "%s", HudText);
                pHud->Invoke("_root.setMessageText", "%s", MessageText);
                if (!pMovie)
                    pHud->Invoke("_root.setHudSize", "%d %d", 0, 0);
            }

            // Draw the HUD screen if it is displayed.
            if ((!pMovie || Settings.HudState != FxPlayerSettings::Hud_Hidden) && HudText[0])
            {
                SetWireframe(0);
                GRenderer::Matrix m;
                GetRenderer()->SetUserMatrix(m);
                pHud->Display();
                GetRenderer()->SetUserMatrix(UserMatrix);

                GetRenderer()->GetRenderStats(&renderStats, 1);
            }

            // Flip buffers to display the scene
            PresentFrame();

            GetRenderer()->EndFrame();

            if (!pMovie || (!Settings.MeasurePerformance && !Settings.FastForward))
            {
                if (pMovie) 
                {
                    TimeTicks = GTimer::GetTicks() / 1000;
                    if (TimeTicks < NextTicksTime)
                        SleepTillMessage((unsigned int)(NextTicksTime - TimeTicks));
                } 
                else 
                {
                    // Don't hog the CPU.
                    SleepTillMessage(200);
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
            return new GRTImageInfo(info.pImage);

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
    if (!Loader.GetMovieInfo(pfilename, &newMovieInfo, 0, GFxLoader::LoadKeepBindData))
    {
        fprintf(stderr, "Error: failed to get info about %s\n", pfilename);
        return 0;
    }

    //Loader.SetImageCreateCallback(GFxImageCreateCallback, 0);

    UInt loadConstants = GFxLoader::LoadAll;

    // Load the actual new movie and crate instance.
    // Don't use library: this will ensure that the memory is released.
    pnewMovieDef = *Loader.CreateMovie(pfilename, loadConstants);
    pBGDef = *Loader.CreateMovie("3DWindowBackground.swf", loadConstants);
    if (!pnewMovieDef)
    {
        fprintf(stderr, "Error: failed to create a movie from '%s'\n", pfilename);
        return 0;
    }

    pnewMovie = *pnewMovieDef->CreateInstance(false);
    if (pBGDef)
        pBG = *pBGDef->CreateInstance(false);
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
    pMovie->SetUserEventHandler(GPtr<GFxUserEventHandler>(*new FxPlayerUserEventHandler(this)));

    // init first frame
    pMovie->Advance(0.0f, 0);
    if (pBG)
        pBG->Advance(0.0f, 0);

    // Renderer
    if (Settings.DoRender)
    {
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
    TimeStartTicks  = GTimer::GetTicks() / 1000;
    NextTicksTime   = TimeStartTicks;
    LastLoggedFps   = TimeStartTicks;
    // Movie logical ticks: either timer or setting controlled
    MovieStartTicks = (Settings.DoRender && !Settings.FastForward) ? TimeStartTicks : 0;
    MovieLastTicks  = MovieStartTicks;
    // Set the Hud to update
    NeedHudUpdate = 1;

    // Update the view
    UpdateViewSize();

    return 1;
}


// Called when sizing begins and ends.
void    FxPlayerApp::OnSizeEnter(bool enterSize)
{

    // When leaving size, adjust to new widtj/height.
    if (!enterSize)
    {
        SizingEntered = 0;

        if (GetRenderer() && ((SizeWidth != Width) || (SizeHeight != Height)))
        {
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
            if (pRenderTexture)
                pRenderTexture->Release();
            if (pStencilSurface)
                pStencilSurface->Release();
#endif
            ResizeWindow(SizeWidth, SizeHeight);
            UpdateViewSize();
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
            SetupRTTexture();
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
    if (GetRenderer())
    {
        SizeWidth = w;
        SizeHeight= h;

        if (!SizingEntered && CheckDisplayStatus() == DisplayStatus_Ok) 
        {
            // Commit sizing immediately if it was due to maximize.
            OnSizeEnter(0);
        }
        else
        {
            if (!(FSAntialias && FSAASupported))
                PresentFrame();           
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
    if (!GetRenderer())
        return;
    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x * GFxMovie::GetRenderPixelScale(), Move.y * GFxMovie::GetRenderPixelScale());
    GetRenderer()->SetUserMatrix(m);
    UserMatrix = m;
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


// Helper function to update HUD.
// Uses LastFPS and LastStats; those variables must be updated separately.
// Helper function to update HUD.
// Uses LastFPS and LastStats; those variables must be updated separately.
void    FxPlayerApp::UpdateHudText()
{
    if (!pMovie) 
    {
        gfc_strcpy(HudText, sizeof(HudText), "");
#ifdef FXPLAYER_FILEPATH
        if (IsConsole())
            gfc_strcpy(MessageText, sizeof(MessageText), "Copy a SWF/GFX file to\n" FXPLAYER_FILEPATH);
        else
#endif
            gfc_strcpy(MessageText, sizeof(MessageText), "Drag and drop SWF/GFX file here");
        return;
    }
    else
        MessageText[0]=0;

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
            if (GetRenderer())
                GetRenderer()->GetRenderCaps(&caps);
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
            "  CTRL V          Toggle dynamic textures\n"
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
    BringMainWindowOnTop();
}

void    FxPlayerApp::OnKey(KeyCode keyCode, unsigned char asciiCode, unsigned int wcharCode, 
                                unsigned int mods, bool downFlag)
{
    if (!pMovie)
        return;

    //  GFxLog* plog = pMovie->GetLog();
    bool    ctrl = ControlKeyDown;

    if (keyCode == FxApp::Control)
    {
        ControlKeyDown = downFlag;
        return;
    }
    //if (keyCode == VK_MENU && downFlag)
    //    ControlKeyDown = false; // to enable Ctrl-Alt-... combinations to work

    if (keyCode == FxApp::Escape && downFlag)
    {
        // Cancel mouse manipulation
        if (MouseTracking != None)
        {
            MouseTracking = None;
            EndMouseCapture();
            Zoom = ZoomStart;
            Move = MoveStart;
            UpdateUserMatrix();
            return;
        }
    }

    if (keyCode == FxApp::F1 && downFlag)
        goto toggle_hud;
    if (keyCode == FxApp::F2 && downFlag)
        goto toggle_stats;

    // Handle Ctrl-Key combinations
    if (ctrl && downFlag)
    {

        switch(keyCode)
        {
        case FxApp::Q:
            QuitFlag = 1;
            return;

            // minus
        case FxApp::Minus: // 219 '['
            CurvePixelError = GTL::gmin(10.0f, CurvePixelError + 0.5f);
            pRenderConfig->SetMaxCurvePixelError(CurvePixelError);
            UpdateHudText();
            break;

            // plus
        case FxApp::KP_Add: // 221 ']':
        //case 187: // 221 ']':
            CurvePixelError = GTL::gmax(0.5f, CurvePixelError - 0.5f);
            pRenderConfig->SetMaxCurvePixelError(CurvePixelError);
            UpdateHudText();
            break;

        case FxApp::W:
            // Toggle wireframe.
            Wireframe = !Wireframe;
            break;

            // Switch to a next stroke type.
        case FxApp::T:
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

        case FxApp::P:
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

        case FxApp::R:
            Paused = 0;
            pMovie->Restart();
            break;

        case FxApp::Left:
            pMovie->GotoFrame(pMovie->GetCurrentFrame()-1);
onkey_finish_seek:
            Paused = 1;
            NeedHudUpdate = 1;
            pMovie->SetPlayState(GFxMovie::Playing);
            // Ensure tag actions are executed. This may change frame state to Stopped.
            pMovie->Advance(0.0f);
            PausedState = pMovie->GetPlayState();
            break;
        case FxApp::Right:
            pMovie->GotoFrame(pMovie->GetCurrentFrame()+1);
            goto onkey_finish_seek;

        case FxApp::PageUp:
            pMovie->GotoFrame(
                GTL::gmax<UInt>(0, pMovie->GetCurrentFrame()-10) );
            goto onkey_finish_seek;
        case FxApp::PageDown:
            pMovie->GotoFrame(
                GTL::gmin<UInt>(pMovie->GetCurrentFrame()+10, MovieInfo.FrameCount-1));
            goto onkey_finish_seek;

        case FxApp::H:
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

        case FxApp::I:
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

                if (renderChange) 
                {
                    // FSAA toggle
#if defined(FXPLAYER_RENDER_DIRECT3D) 
    #if(GFC_D3D_VERSION != 10)
                    pRenderTexture = 0;
                    pStencilSurface = 0;
    #else
                    pRenderTexture = 0;
                    pDepthStencilBuf = 0;
                    pDepthStencil = 0;
    #endif
#endif
                    SwitchFSAA(Settings.AAMode == FxPlayerSettings::AAMode_FSAA);
                    SetupRTTexture();
                }
                if (edgeChange && pMovie)
                {
                    UInt32 rendererFlags = pRenderConfig->GetRenderFlags() & ~GFxRenderConfig::RF_EdgeAA;
                    if (Settings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
                        rendererFlags |= GFxRenderConfig::RF_EdgeAA;
                    pRenderConfig->SetRenderFlags(rendererFlags);
                }

            }
            break;

        case FxApp::U:
#if defined(FXPLAYER_RENDER_DIRECT3D) 
    #if(GFC_D3D_VERSION != 10)
            pRenderTexture = 0;
            pStencilSurface = 0;
    #else
            pRenderTexture = 0;
            pDepthStencilBuf = 0;
            pDepthStencil = 0;
    #endif
            SwitchFullScreenMode();
            UpdateViewSize();
            SetWindowTitle(FXPLAYER_APP_TITLE);
            SetupRTTexture();
#endif

            break;

        case FxApp::S:
            // Toggler scale
            ScaleEnable = !ScaleEnable;
            UpdateViewSize();
            break;

        case FxApp::D:
            // Toggler clipping
            ClippingEnable = !ClippingEnable;
            UpdateViewSize();
            break;

        case FxApp::G:
            Settings.FastForward = !Settings.FastForward;
            break;

        case FxApp::B:
            // toggle background color.
            Settings.Background = !Settings.Background;
            break;

        case FxApp::F:
            Settings.MeasurePerformance = !Settings.MeasurePerformance;
            pRenderStats->GetTessStatistics(); // Clear stats
            LastFPS = 0;
            NeedHudUpdate = 1;

            if (!Settings.MeasurePerformance)
                SetWindowTitle(FXPLAYER_APP_TITLE);
            break;

        case FxApp::Z:
            ResetUserMatrix();
            break;

        case FxApp::C:
            // Toggle viewport culling.
            if (pMovie && pRenderConfig)
            {
                UInt32 rendererFlags = pRenderConfig->GetRenderFlags();
                pRenderConfig->SetRenderFlags(rendererFlags ^ GFxRenderConfig::RF_NoViewCull);
            }
            break;

        case FxApp::O:
            if (pMovie && pRenderConfig)
            {
                UInt32 rendererFlags = pRenderConfig->GetRenderFlags();
                pRenderConfig->SetRenderFlags(rendererFlags ^ GFxRenderConfig::RF_OptimizeTriangles);
            }
            UpdateHudText();
            break;

#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10) && !defined(GFC_OS_XBOX360)
        case 'V':
            Settings.VMCFlags = (Settings.VMCFlags & ~RENDERER::VMConfig_UseDynamicTex) |
                ((~(Settings.VMCFlags & ~RENDERER::VMConfig_UseDynamicTex)) & RENDERER::VMConfig_UseDynamicTex);
            pRenderTexture = 0;
            pStencilSurface = 0;
            pRenderer->ResetVideoMode();
            pRenderer->SetDependentVideoMode(pDevice, &PresentParams, Settings.VMCFlags, hWND);
            SetupRTTexture();
            break;
#endif
        } // switch(keyCode)
    } // if (ctrl)

    else
    { // if (!ctrl)

        // Inform the player about keystroke
        if (!ctrl)
            KeyEvent((GFxKey::Code)keyCode, asciiCode, wcharCode, mods, downFlag);
    }
}


// Helper used to convert key codes and route them to GFxPlayer
void FxPlayerApp::KeyEvent(GFxKey::Code keyCode, unsigned char asciiCode,
                           unsigned int wcharCode, unsigned int mods, bool down)
{
    if (keyCode != GFxKey::VoidSymbol) 
    {
        if (pMovie) 
        {
            GFxKeyEvent event(down ? GFxEvent::KeyDown : GFxKeyEvent::KeyUp, keyCode, asciiCode, wcharCode);
            event.SpecialKeysState.SetShiftPressed(mods & FxApp::KM_Shift ? 1 : 0);
            event.SpecialKeysState.SetCtrlPressed(mods & FxApp::KM_Control ? 1 : 0);
            event.SpecialKeysState.SetAltPressed(mods & FxApp::KM_Alt ? 1 : 0);
            event.SpecialKeysState.SetNumToggled(mods & FxApp::KM_Num ? 1 : 0);
            event.SpecialKeysState.SetCapsToggled(mods & FxApp::KM_Caps ? 1 : 0);
            event.SpecialKeysState.SetScrollToggled(mods & FxApp::KM_Scroll ? 1 : 0);
            pMovie->HandleEvent(event);
        }
    }
}

void    FxPlayerApp::OnChar(UInt32 wcharCode, UInt info)
{
    GUNUSED(info);
    if (pMovie && wcharCode)
    {
        GFxCharEvent event(wcharCode);
        pMovie->HandleEvent(event);
    }
}

void    FxPlayerApp::OnMouseButton(unsigned int button, bool downFlag, int x, int y, 
                                   unsigned int mods)
{
    GUNUSED(mods);
    if (!pMovie)
        return;

    Float fx = (x - Width*0.5f) / (Width*0.5f);
    Float fy = (y - Height*0.5f) / (Height*0.5f);

    Float x1p[4], x1o[4];
    Float x2p[4], x2o[4];
    Float wz1[4], wz2[4];

    VectorMult(wz1, Proj, 0, 0, -0.5, 1);
    VectorMult(wz2, Proj, 0, 0, -1, 1);

    VectorMult(x1p, InvProj, fx*wz1[3], fy*wz1[3], wz1[2]*wz1[3], wz1[3]);
    VectorMult(x2p, InvProj, fx*wz2[3], fy*wz2[3], wz2[2]*wz1[3], wz2[3]);
    VectorInvHomog(x1p);
    VectorInvHomog(x2p);

    VectorMult(x1o, InvMV, x1p);
    VectorMult(x2o, InvMV, x2p);

    Float dx = x2o[0]-x1o[0];
    Float dy = x2o[1]-x1o[1];
    Float dz = x2o[2]-x1o[2];
    Float mz = -x1o[2]/dz;

    fx = x1o[0] + mz * dx;
    fy = x1o[1] + mz * dy;

    x = SInt(fx * RTWidth*0.5f + RTWidth*0.5f);
    y = SInt(fy * RTHeight*0.5f + RTHeight*0.5f);

    // Adjust x, y to viewport.
    GSizeF  s = GetMovieScaleSize();
    Float mX = x * s.Width;
    Float mY = y * s.Height;

    GRenderer::Matrix m;
    //m.AppendScaling(Zoom);
    //m.AppendTranslation(Move.x,Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    // Update mouse state
    if (downFlag)
    {
        MouseDownX = MouseX;
        MouseDownY = MouseY;

        StartMouseCapture();

        if (button==0 && ControlKeyDown)
            MouseTracking = Zooming;
        else if (button==1)
        {
            MouseTracking = Tilting;
            TextureTilt   = Tilting;
        }
        //else if (button==1 && ControlKeyDown)
        //    MouseTracking = Moving;

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
        EndMouseCapture();

        if (MouseTracking != None)
        {
            MouseTracking = None;
            return;
        }

        GFxMouseEvent event(GFxEvent::MouseUp, button, x, y, 0.0f);
        pMovie->HandleEvent(event);
    }
}

void    FxPlayerApp::OnMouseWheel(int zdelta, int x, int y, unsigned int mods)
{
    GUNUSED(mods);
    if (ControlKeyDown)// && MouseTracking == None)
    {
        ZoomStart = Zoom;

        Float dZoom = Zoom;
        Zoom += 0.02f * (zdelta/128) * Zoom;

        if (Zoom < 0.02f)
            Zoom = dZoom;

        dZoom -= Zoom;

        //GSizeF  s = GetMovieScaleSize();
        //Float   mX = ((x - (Width-ViewWidth)/2)) * s.Width;
        //Float   mY = ((y - (Height-ViewHeight)/2)) * s.Height;
        //GRenderer::Matrix m;
        //m.AppendScaling(ZoomStart);
        //m.AppendTranslation(Move.x,Move.y);
        //GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));
        //mX = (Float) (int) (p.x / s.Width);
        //mY = (Float) (int) (p.y / s.Height);

        //Move.x += s.Width * dZoom * mX;
        //Move.y += s.Height * dZoom * mY;

        //UpdateUserMatrix();
        return;
    }

    if (!pMovie)
        return;

    GSizeF  s = GetMovieScaleSize();

    Float fx = (x - Width*0.5f) / (Width*0.5f);
    Float fy = (y - Height*0.5f) / (Height*0.5f);

    Float x1p[4], x1o[4];
    Float x2p[4], x2o[4];
    Float wz1[4], wz2[4];

    VectorMult(wz1, Proj, 0, 0, -0.5, 1);
    VectorMult(wz2, Proj, 0, 0, -1, 1);

    VectorMult(x1p, InvProj, fx*wz1[3], fy*wz1[3], wz1[2]*wz1[3], wz1[3]);
    VectorMult(x2p, InvProj, fx*wz2[3], fy*wz2[3], wz2[2]*wz1[3], wz2[3]);
    VectorInvHomog(x1p);
    VectorInvHomog(x2p);

    VectorMult(x1o, InvMV, x1p);
    VectorMult(x2o, InvMV, x2p);

    Float dx = x2o[0]-x1o[0];
    Float dy = x2o[1]-x1o[1];
    Float dz = x2o[2]-x1o[2];
    Float mz = -x1o[2]/dz;

    fx = x1o[0] + mz * dx;
    fy = x1o[1] + mz * dy;

    x = SInt(fx * RTWidth*0.5f + RTWidth*0.5f);
    y = SInt(fy * RTHeight*0.5f + RTHeight*0.5f);

    // Adjust x, y to viewport.
    Float mX = x * s.Width;
    Float mY = y * s.Height;

    GRenderer::Matrix m;
    //m.AppendScaling(Zoom);
    //m.AppendTranslation(Move.x,Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    GFxMouseEvent event(GFxEvent::MouseWheel, 0, x, y, (Float)((zdelta/WHEEL_DELTA)*3));
    pMovie->HandleEvent(event);
}

void    FxPlayerApp::OnMouseMove(int x, int y, int unsigned mods)
{
    GUNUSED(mods);
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
        Zoom += 0.01f * -dY * Zoom;

        if (Zoom < 0.02f)
            Zoom = dZoom;

        dZoom -= Zoom;

        //Float mX = ((MouseDownX - (Width-ViewWidth)/2)) * s.Width;
        //Float mY = ((MouseDownY - (Height-ViewHeight)/2)) * s.Height;
        //GRenderer::Matrix m;
        //m.AppendScaling(ZoomStart);
        //m.AppendTranslation(MoveStart.x,MoveStart.y);
        //GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));
        //mX = (Float) (int) (p.x / s.Width);
        //mY = (Float) (int) (p.y / s.Height);

        //Move.x += s.Width * dZoom * mX;
        //Move.y += s.Height * dZoom * mY;

        //UpdateUserMatrix();
        return;
    }
    if (MouseTracking == Moving)
    {
        //Move.x -= s.Width * dX;
        //Move.y -= s.Height * dY;

        //UpdateUserMatrix();
        Move.x -= dX  / (Width*0.5f);
        Move.y += dY  / (Height*0.5f);
        return;
    }
    if (MouseTracking == Tilting)
    {
        MeshRotationZ += 0.25f * dX;
        MeshRotationX -= 0.25f * dY;
        return;
    }

    Float fx = (x - Width*0.5f) / (Width*0.5f);
    Float fy = (y - Height*0.5f) / (Height*0.5f);

    Float x1p[4], x1o[4];
    Float x2p[4], x2o[4];
    Float wz1[4], wz2[4];

    VectorMult(wz1, Proj, 0, 0, -0.5, 1);
    VectorMult(wz2, Proj, 0, 0, -1, 1);

    VectorMult(x1p, InvProj, fx*wz1[3], fy*wz1[3], wz1[2]*wz1[3], wz1[3]);
    VectorMult(x2p, InvProj, fx*wz2[3], fy*wz2[3], wz2[2]*wz1[3], wz2[3]);
    VectorInvHomog(x1p);
    VectorInvHomog(x2p);

    VectorMult(x1o, InvMV, x1p);
    VectorMult(x2o, InvMV, x2p);

    Float dx = x2o[0]-x1o[0];
    Float dy = x2o[1]-x1o[1];
    Float dz = x2o[2]-x1o[2];
    Float mz = -x1o[2]/dz;

    fx = x1o[0] + mz * dx;
    fy = x1o[1] + mz * dy;

    x = SInt(fx * RTWidth*0.5f + RTWidth*0.5f);
    y = SInt(fy * RTHeight*0.5f + RTHeight*0.5f);



/*
    // Adjust x, y to viewport.
    Float mX = x * s.Width;
    Float mY = y * s.Height;

    GRenderer::Matrix m;
    //m.AppendScaling(Zoom);
    //m.AppendTranslation(Move.x,Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);
*/
    GFxMouseEvent event(GFxEvent::MouseMove, 0, x, y, 0.0f);
    pMovie->HandleEvent(event);
}


// *** Static Callbacks
/*
GFile*  FxPlayerApp::FileOpener(const char* url)
{
    // Buffered file wrapper is faster to use because it optimizes seeks.
    return new GBufferedFile(GPtr<GSysFile>(*new GSysFile(url)));
}
*/
// For handling notification callbacks from ActionScript.

void    FxPlayerApp::FxPlayerRFTFSCommandHandler::Callback(GFxMovieView* pmovie, const char* command, const char* args)
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
        pApp->Wireframe = !strcmp(args, "1");
    else if (!strcmp(command, "center") && pApp)
        pApp->TextureTilt = Centering;
    else if (!strcmp(command, "quit") && pApp)
        pApp->QuitFlag = 1;
}

// Older window do not define this.
#ifndef IDC_HAND
#define IDC_HAND IDC_ARROW
#endif

void    FxPlayerApp::UserEventHandler(GFxMovieView* pmovie, const GFxEvent& event, void* puserData)
{
    GUNUSED(pmovie);

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
#else
    GUNUSED2(event, puserData);
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
//#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
//            else if ((argv[arg][1] == 'd') && (argv[arg][2] == 'y') && (argv[arg][3] == 'n'))
//            {
//                VMCFlags |= FXPLAYER_RENDER::VMConfig_UseDynamicTex;
//            }
//#endif
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
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
        "  /dyn        Use dynamic textures (D3D)"
#endif
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
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10)
        "  CTRL V          Toggle dynamic textures\n"
#endif
        "  F1              Toggle Info Help.\n"
        "  F2              Toggle Info Stats.\n"


//      "  CTRL-T          Debug.  Test the SetVariable() function\n"
//      "  CTRL-G          Debug.  Test the GetVariable() function\n"
//      "  CTRL-M          Debug.  Test the Invoke() function\n"
//      "  CTRL-B          Toggle background color fill/blending.\n"
        );
}
