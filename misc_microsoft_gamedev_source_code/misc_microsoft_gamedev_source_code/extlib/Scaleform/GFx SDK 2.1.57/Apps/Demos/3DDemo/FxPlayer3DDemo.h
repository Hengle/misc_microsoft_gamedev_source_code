/**********************************************************************

Filename    :   FxPlayer3DDemo.h
Content     :   Sample SWF file player leveraging GFxPlayer API
Created     :   August 24, 2005
Authors     :   Michael Antonov
Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FXPLAYER3DDEMO_H
#define INC_FXPLAYER3DDEMO_H


// Define this to use Direct3D instead of OpenGL
#ifndef FXPLAYER_RENDER_DIRECT3D
#define FXPLAYER_RENDER_DIRECT3D
#endif


// GFx
#include "GTLTypes.h"
#include "GFile.h"
#include "GFxPlayer.h"
#include "GFxLoader.h"
#include "GFxLog.h"

#ifdef  FXPLAYER_RENDER_DIRECT3D
    // Direct3D application class
    #include "Direct3DWin32App.h"
    #include "GRendererD3D9.h"
    #define FXPLAYER_RENDER GRendererD3D9
    #define FXPLAYER_APP    Direct3DWin32App
#else
    // OpenGL application class
    #include "OpenGLWin32App.h"
    #include "GRendererOGL.h"
    #define FXPLAYER_RENDER GRendererOGL
    #define FXPLAYER_APP    OpenGLWin32App
#endif


// **** Declared Classes

class   FxPlayerApp;
class   FxPlayerSettings;
class   FxPlayStream;



// ***** Player Settings class

// Settings class stores playback settings determined
// based on the comand-line parameters
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
    Float       Scale;  
    bool        Antialiased;    
    bool        Background;
    bool        MeasurePerformance; 

    bool        VerboseAction;
    bool        VerboseParse;

    AAModeType  AAMode;

    bool        DoLoop; 

    Float       ExitTimeout;
    UInt        SleepDelay;

    // Run full-screen: -u option.
    bool        FullScreen;

    // PlaybackFile
    char        FileName[256];


    FxPlayerSettings();

    // Initializes settings based on the command line.
    // Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
    bool        ParseCommandLine(int argc, char *argv[]);

    // Displays Playback / Usage information
    static void PrintUsage();
};



// ***** Contained flash playback class

class   FxPlayStream : public GRefCountBase<FxPlayStream>
{

protected:

    // Renderer we use
    FxPlayerApp*            pApp;
    GPtr<GFxRenderConfig>   pRenderConfig;
    LPDIRECT3DDEVICE9       pDevice;

    // Loaded movie data
    char                FileName[256];
    GFxMovieInfo        MovieInfo;
    GPtr<GFxMovieDef>   pMovieDef;
    GPtr<GFxMovieView>  pMovie;

    // Render target & view     
    LPDIRECT3DTEXTURE9  pRenderTexture;
    LPDIRECT3DSURFACE9  pStencilSurface;

    // Location & size where to display an SWF
    SInt                x, y;
    SInt                Width, Height;
    bool                Transparent;

    // How fast we move 
    Float               SpeedScale;
    // How much of the specified flash frame we use to advance clip.
    // 1.0 - same frame rate as specified; default 0.8 (20% faster).
    Float               FrameFrequency;

    // Mouse notification so far.
    SInt                xMouse, yMouse;
    SInt                MouseButtons;

    // Timing values used during playback
    UInt32              StartTicks;
    UInt32              LastAdvanceTicks;
    UInt32              NextAdvanceTicks;
    int                 AdvanceCounter;


public:

    bool                ForceRender;

    // Create a play stream without a texture
    FxPlayStream(FxPlayerApp* papp, GFxLoader &loader, const char *pfileName);
    ~FxPlayStream();

    // Sets the renderer for the movie
    void    SetRenderConfig(GFxRenderConfig *prenderConfig);

    // Set the viewport. Only necessary for non-stransparent rendering.
    void    SetViewport(SInt x, SInt y, SInt width=-1, SInt height=-1, bool transparent = 1);

    // Create surfaces for individual rendering.
    // This needs to be called if we are going to be displaying in a different location.
    bool    CreateBuffers(SInt w, SInt h, bool createStencil);
    // Releases buffers, used to handle reset.
    void    ReleaseBuffers();

    
    LPDIRECT3DTEXTURE9  GetRenderTexture() const    { return pRenderTexture; }

    GFxMovieView*       GetMovie() const            { return pMovie; }
    FxPlayerApp*        GetApp() const              { return pApp;}
    const char*         GetFileName() const         { return FileName; }


    void                SetFrameFrequency(Float ff)
        { FrameFrequency = ff; }

    // Return 1 if the stream was successfully initialized
    bool    IsValid() const
        { return pMovie.GetPtr() != 0;}

    void    NotifyMouseState (int x, int y, int buttons)
    {       
        xMouse = x;
        yMouse = y;
        MouseButtons = buttons;     
    }

    // Advances the movie playhead based on time and potentially updates it.
    // For custom-buffer movies, buffers will only be updated at the specified rate.
    //
    void    AdvanceAndRender(UInt32 ticks = -1);

};



// ***** Player Application class

class   FxPlayerApp : public FXPLAYER_APP
{
public:
    typedef FxPlayerSettings::AAModeType AAModeType;

    // Selected playback settings
    FxPlayerSettings        Settings;
    // Running antialiased or not
    bool                    Antialiased;

    // Loader
    GFxLoader               Loader; 


    // Top movie stack - above model, the first flash file is loaded here
    GTL::garray<GPtr<FxPlayStream> >    TopMovies;
    // Bottom movie stack - below model, load data here if necessary
    GTL::garray<GPtr<FxPlayStream> >    BottomMovies;

    // Movie playing on the texture cube
    GPtr<FxPlayStream>                  pTextureMovie;

    // Removed movie stack - emptied at the end of each tick
    GTL::garray<GPtr<FxPlayStream> >    RemovedMovies;


    // Renderer we use
    GPtr<FXPLAYER_RENDER>       pRenderer;
    GPtr<GFxRenderConfig>       pRenderConfig;

    // Mouse notification so far.
    SInt                        xMouse, yMouse;
    SInt                        MouseButtons;


    // Flag indicating whether the 3D model is shown or hidden
    bool                        ModelShown;
    // This flag is set if the cube model should be displayed.
    // If it is not set, the mesh is displayed.
    // This should set by default, but SWF file will change the model.
    bool                        UseCubeModel;

    // Close Flag - set when the app should bail
    bool                        QuitFlag;

    // Old window location, saved during full-screen switch.
    SInt                        OldWindowX, OldWindowY;

    // Slider properties

    bool    MovieWireframe;
    bool    MovieAnimationDragging;
    bool    MovieAnimationPaused;

    bool    IsTextureMovieAnimating() const 
        { return !MovieAnimationPaused && !MovieAnimationDragging; }

    bool    HUDWireframe;
    bool    MeshWireframe;
    bool    MeshRotationDragging;
    bool    MeshRotationPaused;
    
    bool    IsMeshRotating() const  
        { return !MeshRotationPaused && !MeshRotationDragging; }


    FxPlayerApp();
    ~FxPlayerApp();


    // Called from main() after settings are initialized to execute 
    // most of the program logic. Responsible for setting up the window,
    // loading movies and containing the message loop.
    SInt            Run();


    // Helper - finds a movie playing based on filename (or none 0)
    // Considers bottom and top layers + texture
    FxPlayStream*   FindMovieStream(const char *pfilename);

    
    // *** Overrides

    // Input
    virtual void    OnKey(KeyCode keyCode, unsigned char asciiCode, unsigned int wcharCode, 
        unsigned int mods, bool downFlag) ;
    virtual void    OnMouseButton(unsigned int button, bool downFlag, int x, int y, 
        unsigned int mods);
    virtual void    OnMouseMove(int x, int y, int unsigned mods);
    // Override to initialize OpenGL viewport
    virtual bool    InitRenderer();
    virtual void    PrepareRendererForFrame();

    // Helper used to convert key codes and route them to GFxPlayer
    void            KeyEvent(UInt key, bool down);


    // *** Mesh rendering

    struct MeshDesc
    {
        // Filename we loaded the mesh from
        char                    FileName[256];

        LPD3DXMESH              pMesh;              // Our mesh object in sysmem
        D3DMATERIAL9*           pMeshMaterials;     // Materials for our mesh
        DWORD                   dwNumMaterials;     // Number of mesh materials
        LPDIRECT3DTEXTURE9*     pMeshTextures;      // Textures for our mesh
        // An array of texture flags/indices
        // Currently, 0 if pMeshTexture is used, and 1 if pTextureMovie->GetRenderTexture() is used.
        UByte*                  pMovieTextureTags;
        

        MeshDesc()
        {
            // Clear mesh variables
            FileName[0]    = 0;
            pMesh          = NULL;
            pMeshMaterials = NULL;
            pMeshTextures  = NULL;
            pMovieTextureTags = 0;
            dwNumMaterials = 0L; 
        }

        MeshDesc(const MeshDesc &src)
        {
            gfc_strcpy(FileName, 256, src.FileName);
            pMesh           = src.pMesh;
            pMeshTextures   = src.pMeshTextures;
            pMeshMaterials  = src.pMeshMaterials;
            dwNumMaterials  = src.dwNumMaterials;       
            pMovieTextureTags = src.pMovieTextureTags;
        }

        ~MeshDesc()
        {
            // We don't want destructor to release anything,
            // since we are copying data without AddRef.            
        }

        // Loads mesh from a filename
        bool    LoadMesh(FxPlayerApp *papp, const char *pfilename);
    };

    Float                   MeshRotation;
    UInt                    LastRotationTick;

    // All of the meshes that have been loaded so far.
    GTL::garray<MeshDesc>   LoadedMeshes;
    // Active mesh - this mesh is displayed if UseCubeModel == 0.
    SInt                    ActiveMesh;

    // Returns an index of a mesh found by name, or -1 for fail.
    SInt        FindMeshIndex(const char* pfilename);

    // Vertex buffer for a cube - fallback & default case.
    LPDIRECT3DVERTEXBUFFER9     pCubeVertexBuffer;

    // Mesh loading/cleanup
    bool                InitGeometry();
    void                CleanupGeometry();
    // Rendering
    void                RenderMesh(UInt ticks = -1);
    void                SetupMatrices();
    void                SetupLights();
    
};

// ***** GFx State classes (callback functionality)


// File opener class.
class Fx3DDemoFileOpener : public GFxFileOpener
{
public:
    virtual GFile* OpenFile(const char *pfilename)
    {
        return new GSysFile(pfilename);
    }
};


// "fscommand" callback, handles notification callbacks from ActionScript.
class Fx3DDemoFSCommandHandler : public GFxFSCommandHandler
{
public:
    virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg);
};



#endif // INC_FXPLAYERDEMO_H
