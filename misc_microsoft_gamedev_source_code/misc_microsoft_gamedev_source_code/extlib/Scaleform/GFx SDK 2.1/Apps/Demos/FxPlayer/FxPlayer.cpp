/**********************************************************************

 Filename    :   FxPlayer.cpp
 Content     :   Sample SWF/GFX file player leveraging GFxPlayer API
 Created     :
 Authors     :   Michael Antonov, Maxim Didenko
 Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

 This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
 THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include "GTimer.h"
#include "GFxEvent.h"
#include "GFxPlayer.h"
#include "GFxFontLib.h"
//#include "GFxFontProviderFT2.h"

// Progressive loading loads content on background threads by using
// GFxTaskManager. Enable it only on systems with thread support.
#if !defined(GFC_NO_THREADSUPPORT)
    #define FXPLAYER_PROGRESSIVE_LOADING  1    
    #include "GFxTaskManager.h"    
#endif

// FxPlayerConfig defines FxPlayerCommand enumeration with command mappings
// to keyboard/gamepad buttons and provides a family of macros based on the
// platform we are compiling for. Important macros defines here include:
//
//  - FXPLAYER_APP              - Base application class we will use.
//  - FXPLAYER_APP_TITLE        - Title-bar message.
//  - FXPLAYER_FILEDIRECTORY    - Platform specific sample file directory.
//  - FXPLAYER_FILENAME         - Initial file to load, if any.
//  - FXPLAYER_VIEWWIDTH/HEIGHT - Size of window or video-mode.
//
#include "FxPlayerConfig.h"

// Other player implementation helper classes.
#include "FxPlayerSettings.h"
#include "FxPlayerLog.h"
#include "FxPlayerAlloc.h"

// FxPlayer HUD SWF file is compiled in as a static array here,
// so that we always have it available even if executable is moved.
#include "../../Bin/FxPlayer/fxplayer.swf.h"



// ***** Player Application class

class   FxPlayerApp : public FXPLAYER_APP
{
public:
     // Loaded movie data
    GFxLoader           Loader;
    GFxMovieInfo        MovieInfo;
    GPtr<GFxMovieDef>   pMovieDef;
    GPtr<GFxMovieView>  pMovie;

    bool                ReloadMovie;

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
    GPtr<GFxRenderConfig> pRenderConfig;
    GPtr<GFxRenderStats>  pRenderStats;

    // Selected playback settings
    FxPlayerSettings    Settings;

    GFxValue            TransientValue;

    // View width and height
    SInt                ViewWidth, ViewHeight;

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
    //bool                ControlKeyDown;

    // This variable is set when the movie is paused in the player.
    bool                Paused;
    // Store playstate when paused, so that we can restore it.
    GFxMovie::PlayState PausedState;

    // Last FPS and stats
    Float               LastFPS;
    GRenderer::Stats    LastStats;
    UInt                LastFrame; // Frame reported by HUD
    // This flag is set when UpdateHud needs to be called
    bool                NeedHudUpdate;
    // Hud text, blended over the player
    char                HudText[2048];
    char                MessageText[1024];
    GPtr<GFxMovieView>  pHud;
    GViewport           HudViewport;
    GRenderer::Matrix   UserMatrix;

    //HUD Memory 
    UPInt               HudMemoryAllocated;
    UPInt               HudMemoryUsed;
    UInt                HudMemoryCount;

    // Curve error
    Float               CurvePixelError;

    // Width, height during sizing
    SInt                SizeWidth, SizeHeight;
    bool                SizingEntered;

    FxPlayerApp();
    ~FxPlayerApp();

    // Called from main() after settings are initialized to execute
    // most of the program logic. Responsible for setting up the window,
    // loading movies and containing the message loop.
    SInt            Run();

    // Load a new movie from a file and initialize timing
    bool            LoadMovie(const char *pfilename);

    // Helper function to update HUD.
    // Uses LastFPS and LastStats; those variables must be updated separately.
    void            UpdateHud();
    // Updates the view size based on the ScaleEnable flag and window size.
    void            UpdateViewSize();

    void            ResetUserMatrix();
    void            UpdateUserMatrix();
    GSizeF          GetMovieScaleSize();

    // *** Overrides

    // Sizing; by default, re-initalizes the renderer
    virtual void    OnSize(int w, int h);
    virtual void    OnSizeEnter(bool enterSize);
    virtual void    OnDropFiles(char *path);

    // Input
    virtual void    OnKey(KeyCode, unsigned char asciiCode, unsigned int wcharCode, unsigned int mods, bool downFlag);
    virtual void    OnPad(PadKeyCode, bool downFlag);
    virtual bool    OnIMEEvent(const FxAppIMEEvent&);
    virtual void    OnChar(UInt32 wcharCode, UInt info);
    virtual void    OnMouseButton(unsigned int button, bool downFlag, int x, int y, 
        unsigned int mods);
    virtual void    OnMouseWheel(int zdelta, int x, int y, unsigned int mods);
    virtual void    OnMouseMove(int x, int y, int unsigned mods);


    // Helper used to convert key codes and route them to GFxPlayer
    void            KeyEvent(GFxKey::Code keyCode, unsigned char asciiCode, unsigned int wcharCode, unsigned int mods, bool down);

    virtual void    OnUpdateSystemCliboard(const wchar_t* text);

    void ProcessCommand(FxPlayerCommand cmd);

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
#if defined(GFC_OS_WIN32)
// Older window do not define this.
#ifndef IDC_HAND
#define IDC_HAND IDC_ARROW
#endif
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
#endif
        default:
            break;
        }
    }
};

// "fscommand" callback, handles notification callbacks from ActionScript.
class FxPlayerFSCallback : public GFxFSCommandHandler
{
public:
    virtual void Callback(GFxMovieView* pmovie, const char* pcommand, const char* parg)
    {
        GPtr<GFxLog> plog = pmovie->GetLog();
        if (plog)
        {
            plog->LogMessage("FsCallback: '");
            plog->LogMessage(pcommand);
            plog->LogMessage("' '");
            plog->LogMessage(parg);
            plog->LogMessage("'\n");
        }

        FxPlayerApp* pApp = (FxPlayerApp*)pmovie->GetUserData();
        FxPlayerSettings& Settings = pApp->Settings;

        if (gfc_strcmp(pcommand, "GFxApplyLanguage") == 0)
        {
            if (parg != NULL)
            {
                Settings.RequestedLanguage = parg;
                int idx = Settings.GetFontConfigIndexByName(parg);
                if (idx != -1)
                {
                    Settings.FontConfigIndex = idx;
                    FontConfig* pconfig = Settings.FontConfigs[Settings.FontConfigIndex];
                    pconfig->Apply(&pApp->Loader);
                }
                else
                {
                    if (plog)
                    {
                        plog->LogError("\nLanguage support for '%s' not found!\n\n", parg);
                    }
                }
                pApp->ReloadMovie = true;
            }
        }
    }
};

// File opener class.
class FxPlayerFileOpener : public GFxFileOpener 
{
public:
    virtual GFile* OpenFile(const char *purl) 
    {
        if (!strcmp(purl, "  fxplayer.swf"))
            return new GMemoryFile(purl, fxplayer_swf, sizeof(fxplayer_swf));
        // Buffered file wrapper is faster to use because it optimizes seeks.
#ifdef FXPLAYER_FILEOPENER
        return new GBufferedFile(GPtr<GFile>(*FXPLAYER_FILEOPENER(purl)));
#else
        return new GBufferedFile(GPtr<GSysFile>(*new GSysFile(purl)));
#endif
    }
};


// ***** Main function implementation

int GCDECL main(int argc, char *argv[])
{
    FxPlayerApp::InitMain();

#ifdef FXPLAYER_USE_SYSALLOCATOR
    // Must set custom memory hooks before
    // creating FxPlayerApp or any other GFx objects
    SysAllocator Allocator;
    GMemory::SetAllocator(&Allocator);
    GMemory::SetBlockAllocator(&Allocator);
#endif

    int res = 1;

    setlocale (LC_ALL, ""); //!AB: need for correct work of towlower/towupper
    {
        FxPlayerApp app;

        if (!app.Settings.ParseCommandLine(argc, argv))
        return 1;

        res = app.Run();

        // Wait for all threads to finish; this must be done so that memory
        // allocator and all destructors finalize correctly.
        // Note that we MUST call it here before app instance is destroyed,
        // so that we pRenderer is destroyed from the main thread only.
#ifndef GFC_NO_THREADSUPPORT
        GThread::FinishAllThreads();
#endif
    }

    GMemory::DetectMemoryLeaks();
    return res;
}


// ***** FxPlayerApp Implementation

FxPlayerApp::FxPlayerApp()
{
    ReloadMovie     = false;

    Wireframe       = 0;
    // Scale toggle, on by default
    ScaleEnable     = 1;
    ClippingEnable  = 1;
    Paused          = 0;
    PausedState     = GFxMovie::Playing;

    // Clear timing
    SpeedScale      = 1.0f;
    FrameCounter    = 0;
    TessTriangles   = 0;

    TimeStartTicks  = 0;
    TimeTicks       = 0;
    NextTicksTime   = 0;
    LastLoggedFps   = 0;
    MovieStartTicks = 0;
    MovieLastTicks  = 0;
    MovieTicks      = 0;

    LastFPS         = 0.0f;
    LastFrame       = 0;
    NeedHudUpdate   = 1;
    HudText[0]      = 0;

    ViewWidth       = 0;
    ViewHeight      = 0;

    Zoom = 1.0;
    Move = GPointF(0.0);

    MouseX = 0;
    MouseY = 0;
    MouseTracking = None;
    //ControlKeyDown  = 0;

    SizingEntered = 0;

    CurvePixelError = 1.0f;
}

FxPlayerApp::~FxPlayerApp() 
{
}

//  Sample translator implementation that can be used for testing.
/*
 class TranslatorImpl : public GFxTranslator
 {
     virtual UInt    GetCaps() const         { return Cap_ReturnHtml | Cap_StripTrailingNewLines; }
     virtual bool    Translate(GFxWStringBuffer *pbuffer, const wchar_t *pkey)
     {
         // Translated strings start with $$sign
         if (pkey[0] == '$')
         {
             size_t l = gfc_wcslen(pkey);
             pbuffer->Resize(l + 10);
             wchar_t* pb = pbuffer->GetBuffer();

             gfc_wcscpy(pb, pbuffer->GetLength(), L"<font color='#00FF33'>*TR[<font color='#FF3300'>");
             gfc_wcscat(pb, pbuffer->GetLength(), pkey+1);
             gfc_wcscat(pb, pbuffer->GetLength(), L"</font>]</font>");
             return true;
         }
         return false;
     }
 };*/


#if defined(GFC_OS_WIN32)

// Install system-specific clipboard implementation on Win32. If this is not done
// the clipboard will still work in FxPlayer, but it will be impossible to paste
// text to external applications.
class FxPlayerTextClipboard : public GFxTextClipboard
{
public:
    void OnTextStore(const wchar_t* ptext, UPInt len)
    {
        // store the text in a system clipboard.
        if (OpenClipboard(NULL))
        {
            // Empty the Clipboard. This also has the effect
            // of allowing Windows to free the memory associated
            // with any data that is in the Clipboard
            EmptyClipboard();

            HGLOBAL hClipboardData;
            hClipboardData = GlobalAlloc(GMEM_DDESHARE, (len+1)*sizeof(wchar_t));
            GASSERT(sizeof(wchar_t) == 2);

            // Calling GlobalLock returns a pointer to the 
            // data associated with the handle returned from 
            // GlobalAlloc
            wchar_t * pchData;
            pchData = (wchar_t*)GlobalLock(hClipboardData);
            gfc_wcscpy(pchData, len+1, ptext);

            // Once done, unlock the memory. 
            // don't call GlobalFree because Windows will free the 
            // memory automatically when EmptyClipboard is next 
            // called. 
            GlobalUnlock(hClipboardData);

            // Now, set the Clipboard data by specifying that 
            // Unicode text is being used and passing the handle to
            // the global memory.
            SetClipboardData(CF_UNICODETEXT, hClipboardData);

            // Finally, close the Clipboard which has the effect of 
            // unlocking it so that other  apps can examine or modify its contents.
            CloseClipboard();
        }
    }
};

#endif


SInt FxPlayerApp::Run() 
{

    // Set the verbose flags.
    UInt verboseFlags = 0;

    if (Settings.VerboseParse)
        verboseFlags |= GFxParseControl::VerboseParse;
    if (Settings.VerboseParseShape)
        verboseFlags |= GFxParseControl::VerboseParseShape;
    if (Settings.VerboseParseAction)
        verboseFlags |= GFxParseControl::VerboseParseAction;

    //Set App VSync
    VSync = Settings.VSync;

    // File callback.
    GPtr<GFxFileOpener> pfileOpener = *new FxPlayerFileOpener;
    Loader.SetFileOpener(pfileOpener);

#ifdef GFC_OS_MAC
    // Keep images for renderer changes
    GPtr<GFxImageCreator> pImageCreator = *new GFxImageCreator(1);
    Loader.SetImageCreator(pImageCreator);
#endif

	// Task Manager.
#ifdef FXPLAYER_PROGRESSIVE_LOADING
    GPtr<GFxThreadedTaskManager> ptaskManager = *new GFxThreadedTaskManager;
    //ptaskManager->AddWorkerThreads(GFxTask::Type_IO, 2);
    Loader.SetTaskManager(ptaskManager);
#endif

    // Set log, but only if not quiet
    if (!Settings.Quiet) 
        Loader.SetLog(GPtr<GFxLog>(*new GFxPlayerLog()));
#ifdef GFC_OS_WIN32    
    Loader.SetTextClipboard(GPtr<GFxTextClipboard>(*new FxPlayerTextClipboard()));
#endif    
    //Loader.SetTranslator(GPtr<GFxTranslator>(*new TranslatorImpl()));

    // Disabling FontCacheManager
    //---------------------------
    //Loader.SetFontCacheManager(0);
    //---------------------------

    // Configuring the font cache manager
    //---------------------------
    //GFxFontCacheManager::TextureConfig fontCacheConfig;
    //fontCacheConfig.TextureWidth   = 1024;
    //fontCacheConfig.TextureHeight  = 1024;
    //fontCacheConfig.MaxNumTextures = 1;
    //fontCacheConfig.MaxSlotHeight  = 48;
    //fontCacheConfig.SlotPadding    = 2;
    //Loader.GetFontCacheManager()->SetTextureConfig(fontCacheConfig);
    //Loader.GetFontCacheManager()->EnableDynamicCache(true);
    //Loader.GetFontCacheManager()->SetMaxRasterScale(1.0f);
    //---------------------------

#ifdef FXPLAYER_FONTCACHE_SMALL
    GFxFontCacheManager::TextureConfig fontCacheConfig;
    fontCacheConfig.TextureWidth   = 512;
    fontCacheConfig.TextureHeight  = 256;
    fontCacheConfig.MaxNumTextures = 1;
    fontCacheConfig.MaxSlotHeight  = 32;
    //fontCacheConfig.SlotPadding    = 2;
    Loader.GetFontCacheManager()->SetTextureConfig(fontCacheConfig);
    Loader.GetFontCacheManager()->EnableDynamicCache(true);
    Loader.GetFontCacheManager()->SetMaxRasterScale(1.0f);
#endif


    // Configuring the glyph packer
    //-----------------------------
    // Creating the packer
    //Loader.SetFontPackParams(GPtr<GFxFontPackParams>(*new GFxFontPackParams()));
    //
    // Optional TextureConfig
    //GFxFontPackParams::TextureConfig fontPackConfig;
    //fontPackConfig....
    //Loader.GetFontPackParams()->SetTextureConfig(fontPackConfig);
    //Loader.GetFontPackParams()->SetGlyphCountLimit(1000);
    //Loader.SetFontPackParams(0);
    //-----------------------------


    // Create a system font provider on Windows. If this is not done,
    // system font characters will be displayed as squares.
    if (!Settings.NoSystemFont) 
    {
#ifdef GFC_OS_WIN32    	
        GPtr<GFxFontProviderWin32> fontProvider = *new GFxFontProviderWin32(::GetDC(0));
        Loader.SetFontProvider(fontProvider);
#endif        

        // An example of using FreeType-2 font provider
        //GPtr<GFxFontProviderFT2> fontProvider = *new GFxFontProviderFT2;
        //fontProvider->MapFontToFile("Times New Roman", 0,                      "C:\\WINDOWS\\Fonts\\times.ttf");
        //fontProvider->MapFontToFile("Times New Roman", GFxFont::FF_Bold,       "C:\\WINDOWS\\Fonts\\timesbd.ttf");
        //fontProvider->MapFontToFile("Times New Roman", GFxFont::FF_Italic,     "C:\\WINDOWS\\Fonts\\timesi.ttf");
        //fontProvider->MapFontToFile("Times New Roman", GFxFont::FF_BoldItalic, "C:\\WINDOWS\\Fonts\\timesbi.ttf");

        //fontProvider->MapFontToFile("Arial",           0,                     "C:\\WINDOWS\\Fonts\\arial.ttf");
        //fontProvider->MapFontToFile("Arial",           GFxFont::FF_Bold,      "C:\\WINDOWS\\Fonts\\arialbd.ttf");
        //fontProvider->MapFontToFile("Arial",           GFxFont::FF_Italic,    "C:\\WINDOWS\\Fonts\\ariali.ttf");
        //fontProvider->MapFontToFile("Arial",           GFxFont::FF_BoldItalic,"C:\\WINDOWS\\Fonts\\arialbi.ttf");

        //fontProvider->MapFontToFile("Verdana",         0,                     "C:\\WINDOWS\\Fonts\\verdana.ttf");
        //fontProvider->MapFontToFile("Verdana",         GFxFont::FF_Bold,      "C:\\WINDOWS\\Fonts\\verdanab.ttf");
        //fontProvider->MapFontToFile("Verdana",         GFxFont::FF_Italic,    "C:\\WINDOWS\\Fonts\\verdanai.ttf");
        //fontProvider->MapFontToFile("Verdana",         GFxFont::FF_BoldItalic,"C:\\WINDOWS\\Fonts\\verdanaz.ttf");
        //Loader.SetFontProvider(fontProvider);

    }

	// Apply font configuration on a loader
    if (Settings.FontConfigIndex != -1) 
    {
        FontConfig* pconfig = Settings.GetCurrentFontConfig();
        if (pconfig)
            pconfig->Apply(&Loader);
    } 
    else 
    {
        // Create and load a file into GFxFontLib if requested.
        if (!Settings.FontLibFile.IsEmpty()) 
        {
            GPtr<GFxFontLib> fontLib = *new GFxFontLib;
            Loader.SetFontLib(fontLib);
            GPtr<GFxMovieDef> pmovieDef = *Loader.CreateMovie(Settings.FontLibFile.ToCStr());
            fontLib->AddFontsFrom(pmovieDef);
        }
    }

    
#ifdef   FXPLAYER_MEMORY_TRACKSIZES
    GAllocator* palloc = GMemory::GetAllocator();
    const GAllocatorStats* pstats = palloc->GetStats();
    //memory usage before Hud is loaded
    UPInt memAllocated = pstats->GetSizeAllocated();
    UPInt memUsed      = pstats->GetSizeUsed();
    UInt  memCount     = pstats->AllocCount - pstats->FreeCount;
#endif

    // Load movie for the stats display
    // GFxLoader::LoadWaitCompletion should be set from threaded loading for HUD movie. 
    // Otherwise HUD will no be shown because is Advance method gets call only once when the
    // movie is not ready yet
    if (!Settings.NoHud)
    {

        GPtr<GFxMovieDef> pHudDef = *Loader.CreateMovie("  fxplayer.swf",
            GFxLoader::LoadAll|GFxLoader::LoadKeepBindData|GFxLoader::LoadWaitCompletion);
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

#ifdef   FXPLAYER_MEMORY_TRACKSIZES
        HudMemoryAllocated = pstats->GetSizeAllocated() - memAllocated;
        HudMemoryUsed      = pstats->GetSizeUsed() - memUsed ;
        HudMemoryCount     = pstats->AllocCount - pstats->FreeCount - memCount;
#endif
    }
    GPtr<GFxParseControl> pparseControl = *new GFxParseControl(verboseFlags);
    Loader.SetParseControl(pparseControl);

    if(strlen(Settings.FileName) == 0)
        gfc_strcpy(Settings.FileName, sizeof(Settings.FileName), FXPLAYER_FILEPATH);
        
	bool loadMovie = strlen(Settings.FileName)>0;

	// Get info about the width & height of the movie.
	if (!loadMovie || !Loader.GetMovieInfo(Settings.FileName, &MovieInfo)) 
    {
        // Loader.GetMovieInfo will print error message unless we are in quiet mode
		if (Settings.Quiet)
			fprintf(stderr, "Error: Failed to get info about '%s'\n", Settings.FileName);

		ViewWidth = FXPLAYER_VIEWWIDTH;
		ViewHeight = FXPLAYER_VIEWHEIGHT;
        
        loadMovie = 0;
		//return 1;
	} 
    else 
    {
		ViewWidth = (SInt) (MovieInfo.Width * Settings.ScaleX);
		ViewHeight = (SInt) (MovieInfo.Height * Settings.ScaleY);
	}

	if (Settings.DoRender) 
    {
		// Set options based on arguments
		FullScreen = Settings.FullScreen;
        FSAntialias = (Settings.AAMode == FxPlayerSettings::AAMode_FSAA) ? 1 : 0;
		BitDepth = Settings.BitDepth;
        VMCFlags = Settings.VMCFlags;

		if (FullScreen) 
        {
            Settings.ScaleX = ((Float)FXPLAYER_VIEWWIDTH)  / ViewWidth;
            Settings.ScaleY = ((Float)FXPLAYER_VIEWHEIGHT) / ViewHeight;
            ViewWidth       = FXPLAYER_VIEWWIDTH;
            ViewHeight      = FXPLAYER_VIEWHEIGHT;
		}

		// Enable file drop.
		SupportDropFiles = 1;
		SizableWindow = 1;
		SInt w = ViewWidth, h = ViewHeight;
		if (!SetupWindow(FXPLAYER_APP_TITLE, ViewWidth, ViewHeight))
			return 1;

		// It is important to initialize these sizes, in case OnSizeEnter gets called.
		SizeWidth = GetWidth();
		SizeHeight = GetHeight();

		// Width & Height might be changed by SetupWindow call above
		if (w != GetWidth() || h != GetHeight())
			UpdateViewSize();

		// Create renderer
		if (!CreateRenderer()) 
			return 1;

		// Set renderer on loader so that it is also applied to all children.
		pRenderConfig = *new GFxRenderConfig(GetRenderer());
		Loader.SetRenderConfig(pRenderConfig);

		// Create a renderer stats object since we will be tracking statistics.
		pRenderStats = *new GFxRenderStats();
		Loader.SetRenderStats(pRenderStats);

	}

#ifdef GFC_USE_IME
    //!Create IMEManager after the application window has been set up since 
    // IME manager needs valid window handle as parameter.
    GFxIMEManager* pimemanager = IMEHelper::CreateManager(this);
    if (pimemanager)
    {
        pimemanager->SetIMEMoviePath("IMECandidateList.swf");
        Loader.SetIMEManager(pimemanager);
        // Loader keeps the object from this point
        pimemanager->Release();
    }
#endif

	// Load movie and initialize timing.
	if (loadMovie && !LoadMovie(Settings.FileName)) 
    {
		//return 1;
	}

#ifdef GFC_USE_IME
	 if(pMovie)
         pMovie->HandleEvent(GFxEvent::SetFocus);    
#endif

    // Reset NeedHudUpdate because it might have been cleared by LoadMovie,
    // but it also controls HUD Invoke which we must call initially.
    NeedHudUpdate = true;

	while (1)
    {
		if (ReloadMovie) 
        {
			ResetCursor();
			// Reload file but with different bindings.
			if (!LoadMovie(Settings.FileName)) 
            {
				return 1;
			}

			NeedHudUpdate = 1;
			ReloadMovie = false;
		}

		TimeTicks = GTimer::GetTicks()/1000;

		if (Settings.DoRender && !Settings.FastForward)
			MovieTicks = TimeTicks;
		else
			// Simulate time.
			MovieTicks = MovieLastTicks + (UInt32) (1000.0f / MovieInfo.FPS);

		int deltaTicks = (int)(MovieTicks - MovieLastTicks);
		float deltaT = deltaTicks / 1000.f;

		MovieLastTicks = MovieTicks;

		// Check auto exit timeout counter.
		if ((Settings.ExitTimeout > 0) && (MovieTicks - MovieStartTicks > (UInt32) (Settings.ExitTimeout * 1000)))
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
            RecreateRenderer();
        }

//#if defined(FXPLAYER_RENDER_DIRECT3D) && !defined(GFC_OS_XBOX360)
//			// prevent lost device from stopping resize handling
		if (!GetWidth() && !GetHeight() && SizeHeight && SizeWidth)
		    OnSize(SizeWidth, SizeHeight);
//#endif

        // Potential out-of bounds range is not a problem here,
        // because it will be adjusted for inside of the player.
        if (pMovie) 
        {
            pMovie->SetViewport(GetWidth(), GetHeight(), (GetWidth()-ViewWidth)/2, 
                               (GetHeight()-ViewHeight)/2, ViewWidth, ViewHeight);
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
            // Renderer-specific preparation (Disable depth testing)
            PrepareRendererForFrame();
            GetRenderer()->BeginFrame();

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

            RedrawMouseCursor();

            if (pMovie && (LastFrame != pMovie->GetCurrentFrame()))
                NeedHudUpdate = 1;

            // Get stats every frame
            GRenderer::Stats renderStats;
            GetRenderer()->GetRenderStats(&renderStats, 1);
            // If ballpark triangle count changed, need update
            if (((renderStats.Triangles >> 11) != (LastStats.Triangles >> 11))
                || (renderStats.Primitives != LastStats.Primitives))
                NeedHudUpdate = 1;
            LastStats = renderStats;
            
            if (NeedHudUpdate && (!pMovie || Settings.HudState != FxPlayerSettings::Hud_Hidden) && pHud) 
                UpdateHud();

            // Draw the HUD screen if it is displayed.
            if ((!pMovie || Settings.HudState != FxPlayerSettings::Hud_Hidden) && pHud ) 
            {
                SetWireframe(0);
                GRenderer::Matrix m;
                GetRenderer()->SetUserMatrix(m);
                if (pHud)
                    pHud->Display();
                GetRenderer()->SetUserMatrix(UserMatrix);

                GetRenderer()->GetRenderStats(&renderStats, 1);
            }

            // Flip buffers to display the scene.
            PresentFrame();
            // Inform the renderer that our frame ended, this is required on some
            // platforms such as XBox 360 with predicated tiling so that start
            // reusing resources locked in the previous frame (dynamic font cache, etc).
            GetRenderer()->EndFrame();

            if (!pMovie || (!Settings.MeasurePerformance && !Settings.FastForward)) 
            {

                if (pMovie) 
                {
                    TimeTicks = GTimer::GetTicks() / 1000;
                    //TimeTicks = timeGetTime();
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
                    float delta = (TimeTicks - LastLoggedFps) / 1000.f;
                    char buff[512];
                    LastFPS = (delta > 0) ? FrameCounter / delta : 0.0f;
                    TessTriangles = pRenderStats->GetTessStatistics();

                    // Display frame rate in title
                    gfc_sprintf(buff, 512, FXPLAYER_APP_TITLE " (fps:%3.1f)", LastFPS);

                    if(VSync)
                        gfc_strcat(buff, 512, " VSync");
                   
                    SetWindowTitle(buff);

                    // Update HUD
                    if (Settings.HudState!=FxPlayerSettings::Hud_Hidden)
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

    // Releasing the movie will automatically shut down its
    // background loading task, if any.
    pMovie = 0;

    // If we started worker threads in the renderer we must end them here;
    // otherwise FinishAllThreads will lock up waiting indefinitely.
#ifdef FXPLAYER_PROGRESSIVE_LOADING
    ptaskManager->RequestShutdown();    
#endif

    return 0;
}

/*
// Example of installing custom image creator. You may want to do this if you want to
// create a custom GFxImageInfoBase derived class, such as the one which loads files
// directly on demand instead of pre-loading them on startup. Not that the actual
// implementation of default GFxImageCreator is more involved since it creates texture
// directly for renderers that don't require backup image.

#include "GFxImageResource.h"
class CustomImageCreator : public GFxImageCreator
{
public:
    GImageInfoBase* CreateImage(const GFxImageCreateInfo &info)
    {   
        GPtr<GImage> pimage;

        switch(info.Type)
        {
        case GFxImageCreateInfo::Input_Image:
            // We have to pass correct size; it is required at least
            // when we are initializing image info with a texture.
            return new GImageInfo(info.pImage, info.pImage->Width, info.pImage->Height);

        case GFxImageCreateInfo::Input_File:
            {
                // If we got here, we are responsible for loading an image file.
                GPtr<GFile> pfile  = *info.pFileOpener->OpenFile(info.pFileInfo->FileName.ToCStr());
                if (!pfile)
                    return 0;

                // Load an image into GImage object.
                pimage = *GFxImageCreator::LoadBuiltinImage(pfile, info.pFileInfo->Format, info.Use);
                if (!pimage)
                    return 0;
                return new GImageInfo(pimage, info.pFileInfo->TargetWidth, 
                                              info.pFileInfo->TargetHeight);
            }        
            break;

            // No input - empty image info.
        case GFxImageCreateInfo::Input_None:
        default:
            break;
        }
        return new GImageInfo();
    }
};
*/

/*
 // An example of ExternalInterface handler implementation
 class CustomEIHandler : public GFxExternalInterface
 {
 public:
     void Callback(GFxMovieView* pmovieView, const char* methodName, const GFxValue* args, UInt argCount)
     {
         GUNUSED(pmovieView);
         printf("\nCallback! %s, nargs = %d\n", (methodName)?methodName:"(null)", argCount);
         for (UInt i = 0; i < argCount; ++i)
         {
             printf("  arg(%d) = ", i);
             switch(args[i].GetType())
             {
                 case GFxValue::VT_String: printf("%s", args[i].GetString()); break;
                 case GFxValue::VT_Number: printf("%f", args[i].GetNumber()); break;
                 case GFxValue::VT_Boolean: printf("%s", (args[i].GetBool())?"true":"false"); break;
             // etc...
             }
             printf("\n");
         }
         wchar_t localBuf[100];
         gfc_wcscpy(localBuf, sizeof(localBuf)/sizeof(localBuf[0]), L"Oppa!");     // just to demonstrate how to return local wide char strings
         pmovieView->SetExternalInterfaceRetVal(GFxValue(localBuf));
         // or, the most simple way to return a value is as follows:
         //pmovieView->SetExternalInterfaceRetVal(GFxValue("Oppa!"));
     }
 };
 */

// Load a new movie from a file and initialize timing
bool FxPlayerApp::LoadMovie(const char *pfilename) 
{
    // Try to load the new movie
    GPtr<GFxMovieDef> pnewMovieDef;
    GPtr<GFxMovieView> pnewMovie;
    GFxMovieInfo newMovieInfo;

    ResetCursor();
    ResetUserMatrix();

    // Get info about the width & height of the movie.
    if (!Loader.GetMovieInfo(pfilename, &newMovieInfo, 0,GFxLoader::LoadKeepBindData)) 
    {
        fprintf(stderr, "Error: Failed to get info about %s\n", pfilename);
        return 0;
    }

    UInt loadConstants = GFxLoader::LoadAll;
	/*
    GPtr<GFxImageCreator> imageCreator = *new CustomImageCreator;
    Loader.SetImageCreator(imageCreator);
    */

    // Load the actual new movie and crate instance.
    // Don't use library: this will ensure that the memory is released.
    pnewMovieDef = *Loader.CreateMovie(pfilename, loadConstants
                                                  |GFxLoader::LoadKeepBindData
                                                  |GFxLoader::LoadWaitFrame1);
    if (!pnewMovieDef) 
    {
        fprintf(stderr, "Error: Failed to create a movie from '%s'\n", pfilename);
        return 0;
    }

    pnewMovie = *pnewMovieDef->CreateInstance(false);
    if (!pnewMovie) 
    {
        fprintf(stderr, "Error: Failed to create movie instance\n");
        return 0;
    }

    // If this succeeded, replace the old movie with the new one.
    pMovieDef = pnewMovieDef;
    pMovie = pnewMovie;
    memcpy(&MovieInfo, &newMovieInfo, sizeof(GFxMovieInfo));

    // Copy short filename (i.e. after last '/'),
    {
        Settings.RequestedLanguage = "";
        gfc_strcpy(Settings.FileName, sizeof(Settings.FileName), pfilename);
        SPInt len = strlen(pfilename);
        for (SPInt i=len; i>0; i--) 
        {
            if (pfilename[i]=='/' || pfilename[i]=='\\') 
            {
                pfilename = pfilename+i+1;
                break;
            }
        }
        gfc_strcpy(Settings.ShortFileName, sizeof(Settings.ShortFileName), pfilename);
    }

	// Set a reference to the app
	pMovie->SetUserData(this);

	// This should only be true if this is the GFxPlayer application
	// Make sure to comment this out or set the value to false in your game
	pMovie->SetVariable("_global.gfxPlayer", GFxValue(true));

    if (!CursorDisabled)
        pMovie->EnableMouseSupport(1);

	const char* language = (Settings.FontConfigIndex == -1) ? "Default"
                                : Settings.FontConfigs[Settings.FontConfigIndex]->ConfigName.ToCStr();
	pMovie->SetVariable("_global.gfxLanguage", GFxValue(language));

	GFxValue reqLang = (Settings.RequestedLanguage.GetLength() > 0) ? Settings.RequestedLanguage.ToCStr()
                                                                    : GFxValue();
	pMovie->SetVariable("_global.gfxRequestedLanguage", GFxValue(reqLang));

	// Set ActionScript verbosity / etc.
	GPtr<GFxActionControl> pactionControl = *new GFxActionControl();
	pactionControl->SetVerboseAction(Settings.VerboseAction);
	pactionControl->SetActionErrorSuppress(Settings.NoActionErrors);
    pactionControl->SetLogRootFilenames(Settings.LogRootFilenames);
    pactionControl->SetLogChildFilenames(Settings.LogChildFilenames);
    pactionControl->SetLongFilenames(Settings.LogLongFilename);
	pMovie->SetActionControl(pactionControl);

	// Install handlers.
	pMovie->SetFSCommandHandler(GPtr<GFxFSCommandHandler>(*new FxPlayerFSCallback()));
	pMovie->SetUserEventHandler(GPtr<GFxUserEventHandler>(*new FxPlayerUserEventHandler(this)));

	// setting ExternalInterface handler
	//GPtr<CustomEIHandler> pei = *new CustomEIHandler();
	//pMovie->SetExternalInterface(pei);


    //!AB, set active movie..this causes OnMovieFocus to get called on the IMEManager
    pMovie->HandleEvent(GFxEvent::SetFocus);

	// init first frame
	pMovie->Advance(0.0f, 0);

	// Renderer
	if (Settings.DoRender) 
    {
		if (Settings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
			pRenderConfig->SetRenderFlags(pRenderConfig->GetRenderFlags()| GFxRenderConfig::RF_EdgeAA);
	}

	if (Settings.DoSound) 
    { 
        // No built-in sound support currently in GFx. Customers
        // can play their own sound throug fscommand() callbacks.
	}

	// Disable pause.
	Paused = 0;

	// Init timing for the new piece.
	FrameCounter = 0;
	// Time ticks: always rely on a timer
	TimeStartTicks = GTimer::GetTicks()/1000;
	//TimeStartTicks  = timeGetTime();
	NextTicksTime = TimeStartTicks;
	LastLoggedFps = TimeStartTicks;
	// Movie logical ticks: either timer or setting controlled
	MovieStartTicks
			= (Settings.DoRender && !Settings.FastForward) ? TimeStartTicks : 0;
	MovieLastTicks = MovieStartTicks;
	// Set the Hud to update
    if (pHud)
    {
        MessageText[0]=0;
        pHud->Invoke("_root.setMessageText", "%s", MessageText); 
    }
	NeedHudUpdate = 1;

	// Update the view
	UpdateViewSize();
	return 1;
}

// Called when sizing begins and ends.
void FxPlayerApp::OnSizeEnter(bool enterSize) 
{

    // When leaving size, adjust to new widtj/height.
    if (!enterSize) 
    {
        SizingEntered = 0;

        if (GetRenderer() && ((SizeWidth != GetWidth()) || (SizeHeight != GetHeight()))) 
        {
            ResizeWindow(SizeWidth, SizeHeight);
            UpdateViewSize();
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
void FxPlayerApp::OnSize(SInt w, SInt h) 
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
    NeedHudUpdate = 1;
}

void FxPlayerApp::ResetUserMatrix() 
{
    Move = GPointF(0.0f);
    Zoom = 1.0f;
    UpdateUserMatrix();
}

void FxPlayerApp::UpdateUserMatrix() 
{
    if (!GetRenderer())
        return;
    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x * GFxMovie::GetRenderPixelScale(),
                        Move.y * GFxMovie::GetRenderPixelScale());
    GetRenderer()->SetUserMatrix(m);
    UserMatrix = m;
    NeedHudUpdate = 1;
}

GSizeF FxPlayerApp::GetMovieScaleSize() 
{
    GSizeF scale;
    scale.Width = (pMovie->GetVisibleFrameRect().Width() / ViewWidth);
    scale.Height = (pMovie->GetVisibleFrameRect().Height() / ViewHeight);
    return scale;
}

// Updates the view size based on the ScaleEnable flag and window size.
void FxPlayerApp::UpdateViewSize() 
{
    if (ScaleEnable) 
    {
        SInt width = GTL::gmax(GetWidth(), 4);
        SInt height= GTL::gmax(GetHeight(), 4);

        if (ClippingEnable) 
        {
            // Determine movie size and location based on the aspect ratio
            float hw;
            if (MovieInfo.Height != 0 && MovieInfo.Width != 0)
                hw = (Float) MovieInfo.Height / (Float) MovieInfo.Width;
            else
                hw = (Float) height / (Float) width;
            if (width * hw > height) 
            {
                // Use height as the basis for aspect ratio
                ViewWidth = (SInt)((float) height / hw);
                ViewHeight = height;
            } 
            else 
            {
                // Use width
                ViewWidth = width;
                ViewHeight = (SInt) (width * hw);
            }
        } 
        else 
        {
            ViewWidth = width;
            ViewHeight = height;
        }
        // Compute input scale
        Settings.ScaleX = (Float) ViewWidth / (Float) MovieInfo.Width;
        Settings.ScaleY = (Float) ViewHeight / (Float) MovieInfo.Height;
    } 
    else 
    {
        // No scaling, just center the image
        ViewWidth = MovieInfo.Width;
        ViewHeight = MovieInfo.Height;
        Settings.ScaleX = Settings.ScaleY = 1.0f;
    }
}

// Helper function to update HUD.
// Uses LastFPS and LastStats; those variables must be updated separately.
void FxPlayerApp::UpdateHud() 
{
    HudViewport = GViewport(GetWidth(), GetHeight(),
        SInt(GetWidth()*GetSafeArea()),
        SInt(GetHeight()*GetSafeArea()),
        SInt(GetWidth() - 2*GetWidth()*GetSafeArea()),
        SInt(GetHeight() - 2*GetHeight()*GetSafeArea()));

    pHud->SetViewport(HudViewport);

    if (!pMovie) 
    {
        gfc_strcpy(HudText, sizeof(HudText), "");
#ifdef FXPLAYER_FILEPATH
        if (IsConsole())
            gfc_strcpy(MessageText, sizeof(MessageText), "Copy a SWF/GFX file to\n" FXPLAYER_FILEPATH);
        else
#endif
            gfc_strcpy(MessageText, sizeof(MessageText), "Drag and drop SWF/GFX file here");
        
        pHud->Invoke("_root.setHudText", "%s", HudText);
        pHud->Invoke("_root.setMessageText", "%s", MessageText); 
        pHud->Invoke("_root.setHudSize", "%d %d", 0, 0);
        
        NeedHudUpdate = 0;
        LastFrame = 0;
        return;
    }
    else
    {
        MessageText[0]=0;
        LastFrame = pMovie->GetCurrentFrame();
    }
       
    // Stroke type
    UInt32 stroke = GFxRenderConfig::RF_StrokeCorrect;
    bool optTriangles = false;

    if (pRenderConfig) 
    {
        stroke = (pRenderConfig->GetRenderFlags() & GFxRenderConfig::RF_StrokeMask);
        optTriangles = (pRenderConfig->GetRenderFlags() & GFxRenderConfig::RF_OptimizeTriangles) != 0;
    }

    // Display a custom message if stroke is #ifdef-ed out.
#ifndef GFC_NO_FXPLAYER_STROKER
    char * pstrokeType = "Correct";
    if (stroke == GFxRenderConfig::RF_StrokeNormal)
        pstrokeType = "Normal";
#else
    char * pstrokeType = "Correct [#disabled]";
    if (stroke == GFxRenderConfig::RF_StrokeNormal)
        pstrokeType = "Normal [#disabled]";
#endif

    if (stroke == GFxRenderConfig::RF_StrokeHairline)
        pstrokeType = "Hairline";

    // AA Type
    char * pAAType = "Edge AA";

    switch (Settings.AAMode) 
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
    GAllocator* palloc = GMemory::GetAllocator();
    const GAllocatorStats* pstats = palloc->GetStats();
#endif

    // Update hud text
    if (Settings.HudState == FxPlayerSettings::Hud_Stats) 
    {
        gfc_sprintf(
            HudText,
            2048,
            "Playback Info (F1/F2)\n"
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
            "AA Mode:   %s %s\n",

            Settings.ShortFileName,
            MovieInfo.Version,
            MovieInfo.Width,
            MovieInfo.Height,
            MovieInfo.FPS,
            LastFrame,
            MovieInfo.FrameCount,
            Paused ? " (Paused)" : "",
            LastFPS,
    #ifdef   FXPLAYER_MEMORY_TRACKSIZES
            (pstats->GetSizeAllocated() - HudMemoryAllocated +1023) / 1024,
            (pstats->GetSizeUsed() - HudMemoryUsed + 1023) / 1024,
            (pstats->AllocCount - pstats->FreeCount - HudMemoryCount),
    #endif
            TessTriangles,
            LastStats.Triangles,
            LastStats.Primitives,
            LastStats.Lines,
            pAAType, (optTriangles ? "*Opt" : ""));

    if (!IsConsole())
        gfc_sprintf(HudText+strlen(HudText), 2048-gfc_strlen(HudText),
            "CurveErr:  %3.1f (Ctrl - or +)\n"
            "Stroke:    %s\n"
            "Zoom:      %3.2f\n"
            //"Move:      x:%3.1f y:%3.1f"
            "FontCfg:   %s",

            CurvePixelError,
            pstrokeType,
            Zoom,
            //Move.x/20, Move.y/20,
            (Settings.FontConfigIndex == -1) ? "Default"
                    : Settings.FontConfigs[Settings.FontConfigIndex]->ConfigName.ToCStr() );
    }

    if (Settings.HudState == FxPlayerSettings::Hud_Help) 
    {
        gfc_strcpy(HudText, 2048, "Keys:\n");
        if (IsConsole())
        {
            for (int i = 0; FxPlayerCommandPadKeyMap[i].keyCode != FxApp::VoidPadKey; i++)
                gfc_strcat(HudText, 2048, FxPlayerCommandPadKeyMap[i].helpText);
        }
        else
        {
            for (int i = 0; FxPlayerCommandKeyMap[i].keyCode != FxApp::VoidSymbol; i++)
                gfc_strcat(HudText, 2048, FxPlayerCommandKeyMap[i].helpText);
        }
    }
    pHud->Invoke("_root.setHudText", "%s", HudText);
    NeedHudUpdate = 0;
}

// Handle dropped file
void FxPlayerApp::OnDropFiles(char *path) 
{
    // unload the translator
    if (Settings.FontConfigIndex >= 0)
        Loader.SetTranslator(NULL);

    if (Settings.LoadDefaultFontConfigFromPath(path) ) 
    {
        // set the font config
        FontConfig* pconfig = Settings.GetCurrentFontConfig();
        if (pconfig)
            pconfig->Apply(&Loader);
    }

    LoadMovie(path);

    //!AB, set active movie
 //   if (pMovie)
 //       pMovie->HandleEvent(GFxEvent::SetFocus);

    BringMainWindowOnTop();
}

void FxPlayerApp::ProcessCommand(FxPlayerCommand cmd)
{
    switch (cmd) 
    {
        case FPC_Quit:
            QuitFlag = 1;
            return;

        case FPC_CurveToleranceUp: // 219 '['
            CurvePixelError = GTL::gmin(10.0f, CurvePixelError + 0.5f);
            pRenderConfig->SetMaxCurvePixelError(CurvePixelError);
            NeedHudUpdate = 1;
            break;

        case FPC_CurveToleranceDown: // 221 ']':
            CurvePixelError = GTL::gmax(0.5f, CurvePixelError - 0.5f);
            pRenderConfig->SetMaxCurvePixelError(CurvePixelError);
            NeedHudUpdate = 1;
            break;

        case FPC_Wireframe:
            // Toggle wireframe.
            Wireframe = !Wireframe;
            break;
        
        case FPC_VSync:
            // Toggle VSync
            SetVSync(!VSync);
            break;

			// Switch to a next stroke type.
        case FPC_StrokeMode: 
            {
                UInt32 rf = pRenderConfig->GetRenderFlags();
                UInt32 stroke = rf & GFxRenderConfig::RF_StrokeMask;

                switch (stroke) 
                {
                case GFxRenderConfig::RF_StrokeHairline:
                    stroke = GFxRenderConfig::RF_StrokeNormal;
                    break;
                case GFxRenderConfig::RF_StrokeNormal:
                    stroke = GFxRenderConfig::RF_StrokeCorrect;
                    break;
                case GFxRenderConfig::RF_StrokeCorrect:
                    stroke = GFxRenderConfig::RF_StrokeHairline;
                    break;
                }
                pRenderConfig->SetRenderFlags((rf & ~GFxRenderConfig::RF_StrokeMask) | stroke);
                NeedHudUpdate = 1;
            }
            break;

        case FPC_Pause:
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

        case FPC_Restart:
            Paused = 0;
            pMovie->Restart();
            break;

        case FPC_StepForward_1:
            pMovie->GotoFrame(pMovie->GetCurrentFrame()-1);
        onkey_finish_seek: 
            Paused = 1;
            NeedHudUpdate = 1;
            pMovie->SetPlayState(GFxMovie::Playing);
            // Ensure tag actions are executed. This may change frame state to Stopped.
            pMovie->Advance(0.0f);
            PausedState = pMovie->GetPlayState();
            break;

        case FPC_StepBack_1:
            pMovie->GotoFrame(pMovie->GetCurrentFrame()+1);
            goto onkey_finish_seek;

        case FPC_StepBack_10: //PRIOR
            pMovie->GotoFrame(GTL::gmax<UInt>(0, pMovie->GetCurrentFrame()-10) );
            goto onkey_finish_seek;

        case FPC_StepForward_10: // NEXT
            pMovie->GotoFrame(
            GTL::gmin<UInt>(pMovie->GetCurrentFrame()+10, MovieInfo.FrameCount-1));
            goto onkey_finish_seek;

        case FPC_InfoHelp:
            // Toggle info hud.
            switch(Settings.HudState)
            {
                case FxPlayerSettings::Hud_Hidden: Settings.HudState = FxPlayerSettings::Hud_Help; break;
                case FxPlayerSettings::Hud_Stats: Settings.HudState = FxPlayerSettings::Hud_Help; break;
                case FxPlayerSettings::Hud_Help: Settings.HudState = FxPlayerSettings::Hud_Hidden; break;
            }
            NeedHudUpdate = 1;
            break;

        case FPC_InfoStats:
            // Toggle info stats.
            switch(Settings.HudState)
            {
                case FxPlayerSettings::Hud_Hidden: Settings.HudState = FxPlayerSettings::Hud_Stats; break;
                case FxPlayerSettings::Hud_Stats: Settings.HudState = FxPlayerSettings::Hud_Hidden; break;
                case FxPlayerSettings::Hud_Help: Settings.HudState = FxPlayerSettings::Hud_Stats; break;
            }
            NeedHudUpdate = 1;
            break;
        
        case FPC_CycleHud:
            // Toggle info stats.
            switch(Settings.HudState)
            {
                case FxPlayerSettings::Hud_Hidden: Settings.HudState = FxPlayerSettings::Hud_Stats; break;
                case FxPlayerSettings::Hud_Stats: Settings.HudState = FxPlayerSettings::Hud_Help; break;
                case FxPlayerSettings::Hud_Help: Settings.HudState = FxPlayerSettings::Hud_Hidden; break;
            }
            NeedHudUpdate = 1;
            break;

        case FPC_AntialiasingMode:
            {
                bool renderChange = 0;
                bool edgeChange = 0;

                switch(Settings.AAMode)
                {
                    case FxPlayerSettings::AAMode_None:
                        Settings.AAMode = FxPlayerSettings::AAMode_EdgeAA;
                        edgeChange = 1;
                        break;
                    case FxPlayerSettings::AAMode_EdgeAA:
                        if(FSAASupported)
                            Settings.AAMode = FxPlayerSettings::AAMode_FSAA;
                        else
                            Settings.AAMode = FxPlayerSettings::AAMode_None;
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
                    SwitchFSAA(Settings.AAMode == FxPlayerSettings::AAMode_FSAA);
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

        case FPC_Fullscreen:
            pRenderConfig->SetRenderer(0);
            SwitchFullScreenMode();
            pRenderConfig->SetRenderer(GetRenderer());
            UpdateViewSize();
            SetWindowTitle(FXPLAYER_APP_TITLE);
            break;

        case FPC_ScaledDisplay:
            // Toggler scale
            ScaleEnable = !ScaleEnable;
            UpdateViewSize();
            break;

        case FPC_StageClipping:
            // Toggler clipping
            ClippingEnable = !ClippingEnable;
            UpdateViewSize();
            break;

        case FPC_FastForward:
            Settings.FastForward = !Settings.FastForward;
            break;

        case FPC_Background:
            // toggle background color.
            Settings.Background = !Settings.Background;
            break;

        case FPC_FastMode:
            Settings.MeasurePerformance = !Settings.MeasurePerformance;
            pRenderStats->GetTessStatistics(); // Clear stats
            LastFPS = 0;
            NeedHudUpdate = 1;

            if (!Settings.MeasurePerformance)
                SetWindowTitle(FXPLAYER_APP_TITLE);
            break;

        case FPC_ResetUserMatrix:
            ResetUserMatrix();
            break;

        case FPC_FontConfig:
            // Switch international font.
            if ((Settings.FontConfigIndex != -1) && (Settings.FontConfigs.size() > 1))
            {
                // Apply different settings and reload file.

                Settings.FontConfigIndex++;
                Settings.FontConfigIndex %= (SInt)Settings.FontConfigs.size();
                FontConfig* pconfig = Settings.GetCurrentFontConfig();
                if (pconfig)
                {
                    pconfig->Apply(&Loader);
                    Settings.RequestedLanguage = pconfig->ConfigName;
                }
                else
                    Settings.RequestedLanguage = "";
                ResetCursor();
                // Reload file but with different bindings.
                LoadMovie(pMovieDef->GetFileURL());
                NeedHudUpdate = 1;
            }
            break;

        case FPC_StageCulling:
            // Toggle viewport culling.
            if (pMovie && pRenderConfig)
            {
                UInt32 rendererFlags = pRenderConfig->GetRenderFlags();
                pRenderConfig->SetRenderFlags(rendererFlags ^ GFxRenderConfig::RF_NoViewCull);
            }
            break;

        case FPC_TriangleOptimization:
            if (pMovie && pRenderConfig)
            {
                UInt32 rendererFlags = pRenderConfig->GetRenderFlags();
                pRenderConfig->SetRenderFlags(rendererFlags ^ GFxRenderConfig::RF_OptimizeTriangles);
            }
            NeedHudUpdate = 1;
            break;

#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10) && !defined(GFC_OS_XBOX360)
        case FPC_DynamicTextures:
            VMCFlags = (VMCFlags & ~renderer_type::VMConfig_UseDynamicTex) |
                ((~(VMCFlags & renderer_type::VMConfig_UseDynamicTex)) & renderer_type::VMConfig_UseDynamicTex);
            pRenderer->ResetVideoMode();
            SetRendererDependentVideoMode();
            break;
#endif
        case FPC_LoadNextFile:
        case FPC_LoadPrevFile:
            if(UpdateFiles(Settings.FileName, cmd == FPC_LoadPrevFile))
                LoadMovie(Settings.FileName);
            
            break;

        case FPC_ShowMouseCursor:
            if (CursorDisabled && pMovie)
                pMovie->EnableMouseSupport(1);
            else
                CursorHidden = !CursorHidden;
            ShowCursor(!CursorHidden);
            CursorDisabled = false;
            break;

        case KEY_Return:
        case KEY_Escape:
            KeyEvent(cmd == KEY_Return ? GFxKey::Return : GFxKey::Escape, 0, 0, 0, true);
            break;

        default:
        	break;
        } // switch(keyCode)
}

bool FxPlayerApp::OnIMEEvent(const FxAppIMEEvent& event)
{
#ifndef GFC_NO_IME_SUPPORT
    return IMEHelper::OnEvent(event, pMovie);
#else
    GUNUSED(event);
    return false;
#endif
}

void FxPlayerApp::OnPad(PadKeyCode keyCode, bool downFlag)
{
    switch(keyCode)
    {
    case FxApp::Pad_Left:
        KeyEvent(GFxKey::Left, 0,0,0, downFlag);
        return;
    case FxApp::Pad_Right:
        KeyEvent(GFxKey::Right, 0,0,0, downFlag);
        return;
    case FxApp::Pad_Up:
        KeyEvent(GFxKey::Up, 0,0,0, downFlag);
        return;
    case FxApp::Pad_Down:
        KeyEvent(GFxKey::Down, 0,0,0, downFlag);
        return;
    default:
        break;
    }

    if (!pMovie || !downFlag)
        return;

    for (int i = 0; FxPlayerCommandPadKeyMap[i].keyCode != FxApp::VoidPadKey; i++)
    {
        if (keyCode == FxPlayerCommandPadKeyMap[i].keyCode)
        {
            ProcessCommand(FxPlayerCommandPadKeyMap[i].cmd);
            break;
        }
    }
}

void FxPlayerApp::OnKey(KeyCode keyCode, unsigned char asciiCode,
                        unsigned int wcharCode, unsigned int mods, bool downFlag) 
{
    if (!pMovie)
        return;

    bool ctrl = false;

    if (!Settings.NoControlKeys) 
    {
        ctrl = mods & FxApp::KM_Control;        

        if (keyCode == FxApp::Control) 
            return;
        // TODO ???
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
            ProcessCommand(FPC_InfoHelp);
        if (keyCode == FxApp::F2 && downFlag)
            ProcessCommand(FPC_InfoStats);
    }

    if (ctrl && downFlag) 
    {
	    // Handle Ctrl-Key combinations
        for (int i = 0; FxPlayerCommandKeyMap[i].keyCode != FxApp::VoidSymbol; i++)
        {
            if (keyCode == FxPlayerCommandKeyMap[i].keyCode)
            {
                ProcessCommand(FxPlayerCommandKeyMap[i].cmd);
                break;
            }
        }
    }
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

void FxPlayerApp::OnChar(UInt32 wcharCode, UInt info) 
{
    GUNUSED(info);
    if (pMovie && wcharCode) 
    {
        GFxCharEvent event(wcharCode);
        pMovie->HandleEvent(event);
    }
}

void FxPlayerApp::OnMouseButton(unsigned int button, bool downFlag, int x, int y, 
                                unsigned int mods)
{
    bool ControlKeyDown = mods & FxApp::KM_Control;
    if (!pMovie)
        return;

    if (!CursorDisabled)
        pMovie->EnableMouseSupport(1);
    CursorDisabled = false;

    MouseX = x;
    MouseY = y;

    // Adjust x, y to viewport.
    GSizeF s = GetMovieScaleSize();
    Float mX = ((x - (GetWidth()-ViewWidth)/2)) * s.Width;
    Float mY = ((y - (GetHeight()-ViewHeight)/2)) * s.Height;

    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x, Move.y);
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
    } else {
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

void FxPlayerApp::OnMouseWheel(int zdelta, int x, int y, unsigned int mods)
{
    bool ControlKeyDown = mods & FxApp::KM_Control;
    if (ControlKeyDown)// && MouseTracking == None)
    {
        ZoomStart = Zoom;

        Float dZoom = Zoom;
        Zoom += 0.02f * (zdelta/128) * Zoom;

        if (Zoom < 0.02f)
            Zoom = dZoom;

        dZoom -= Zoom;

        GSizeF s = GetMovieScaleSize();
        Float mX = ((x - (GetWidth()-ViewWidth)/2)) * s.Width;
        Float mY = ((y - (GetHeight()-ViewHeight)/2)) * s.Height;
        GRenderer::Matrix m;
        m.AppendScaling(ZoomStart);
        m.AppendTranslation(Move.x, Move.y);
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

    GSizeF s = GetMovieScaleSize();

    // Adjust x, y to viewport.
    Float mX = ((x - (GetWidth()-ViewWidth)/2)) * s.Width;
    Float mY = ((y - (GetHeight()-ViewHeight)/2)) * s.Height;

    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x, Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    GFxMouseEvent event(GFxEvent::MouseWheel, 0, x, y, (Float)((zdelta/WHEEL_DELTA)*3));
    pMovie->HandleEvent(event);
}

void FxPlayerApp::OnMouseMove(int x, int y, unsigned int mods)
{
    GUNUSED(mods);
    Float dX = (Float) MouseX - x;
    Float dY = (Float) MouseY - y;
    MouseX = x;
    MouseY = y;

    // Used by NotifyMouseState in the main loop
    if (!pMovie)
        return;

    GSizeF s = GetMovieScaleSize();

    if (MouseTracking == Zooming) 
    {
        Float dZoom = Zoom;
        Zoom += 0.01f * dY * Zoom;

        if (Zoom < 0.02f)
            Zoom = dZoom;

        dZoom -= Zoom;

        Float mX = ((MouseDownX - (GetWidth()-ViewWidth)/2)) * s.Width;
        Float mY = ((MouseDownY - (GetHeight()-ViewHeight)/2)) * s.Height;
        GRenderer::Matrix m;
        m.AppendScaling(ZoomStart);
        m.AppendTranslation(MoveStart.x, MoveStart.y);
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
    Float mX = ((x - (GetWidth()-ViewWidth)/2)) * s.Width;
    Float mY = ((y - (GetHeight()-ViewHeight)/2)) * s.Height;

    GRenderer::Matrix m;
    m.AppendScaling(Zoom);
    m.AppendTranslation(Move.x, Move.y);
    GRenderer::Point p = m.TransformByInverse(GRenderer::Point(mX, mY));

    x = (int) (p.x / s.Width);
    y = (int) (p.y / s.Height);

    GFxMouseEvent event(GFxEvent::MouseMove, 0, x, y, 0.0f);
    pMovie->HandleEvent(event);

    /*
    // Mouse HitTest debugging logic

    Double xmouse = pMovie->GetVariableDouble("_root._xmouse");
    Double ymouse = pMovie->GetVariableDouble("_root._ymouse");

    int hitTest = pMovie->HitTest(x, y, GFxMovieView::HitTest_Shapes);

    pMovie->GetLog()->LogMessage("pMovie->HitTest: %d\n", hitTest);

    x = (SInt)(float(x) * MovieInfo.Width) / ViewWidth;
    y = (SInt)(float(y) * MovieInfo.Height) / ViewHeight;

    const char* pi = pMovie->Invoke("_root.hitTest", "%f%f%d", xmouse, ymouse, 1);
    pMovie->GetLog()->LogMessage("_root.hitTest: %s\n", pi);
    */
}

void FxPlayerApp::OnUpdateSystemCliboard(const wchar_t* text) 
{
    GPtr<GFxTextClipboard> pclipBoard = Loader.GetTextClipboard();
    if (pclipBoard)
        pclipBoard->SetText(text);
}
