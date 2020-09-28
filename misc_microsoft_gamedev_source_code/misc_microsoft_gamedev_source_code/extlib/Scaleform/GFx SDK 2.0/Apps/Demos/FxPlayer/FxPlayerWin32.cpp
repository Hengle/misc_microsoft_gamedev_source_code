/**********************************************************************

Filename    :   FxPlayerWin32.cpp
Content     :   Sample SWF/GFX file player leveraging GFxPlayer API
Created     :
Authors     :   Michael Antonov
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
#include "GFxString.h"
#include "GFxRenderConfig.h"
#include "GStd.h"
#include "FontConfigParser.h"

#include "GFxFontLib.h"
#include "GFxFontProviderWin32.h"


#include "GFxFontlib.h"
#include "GFxFontProviderWin32.h"
//#include "GFxFontProviderFT2.h"

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
    #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer D3D9 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING

    #include <d3dx9.h>

#else
    // OpenGL application class
    #include "OpenGLWin32App.h"
    #include "GRendererOGL.h"
    #define FXPLAYER_RENDER GRendererOGL
    #define FXPLAYER_APP    OpenGLWin32App
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer OpenGL v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
#endif

// glOrtho parameter
#define     OVERSIZE            1.0f

// Standard includes
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <zmouse.h> // for WHEEL_DELTA macro

// ***** SysAllocator - Sample custom allocator for GFxPlayer

// Enable the memory tracking in the GFxPlayer
// Comment this line out to prevent memory tracking
#define GFC_MEMORY_TRACKSIZES

#include "GAllocator.h"

// Define this macro to enable the use of a custom sample
// SysAllocator class provided below.
//#define  FXPLAYER_USE_SYSALLOCATOR

#ifndef FXPLAYER_USE_SYSALLOCATOR

    // If not using our allocator, memory tracking is only available
    // if GMemory was compiled with GFC_MEMORY_TRACKSIZES.
    #ifdef GFC_MEMORY_TRACKSIZES
        #define FXPLAYER_MEMORY_TRACKSIZES
    #endif

#else

// We want to track memory use.
#define FXPLAYER_MEMORY_TRACKSIZES

// Sample custom allocator. Delegates allocation to standard library
// malloc/free/realloc functions and adds memory counters on top.
class SysAllocator : public GAllocator
{
public:

    GAllocatorStats  Stats;

    SysAllocator()
    {
        GMemUtil::Set(&Stats, 0, sizeof(GAllocatorStats));
    }
    const GAllocatorStats* GetStats() const
    {
        return &Stats;
    }

    void    Free(void *pmem)
    {
        // System allocator & tracking option
#ifdef  FXPLAYER_MEMORY_TRACKSIZES
        // Have to check for null - by standard
        if (!pmem) return;
        Stats.FreeCount++;
        UPInt sizeFreed = *(((UPInt*)pmem)-1);
        Stats.FreeSize += sizeFreed;
        pmem = (((UPInt*)pmem)-1);
#endif
        free(pmem);
    }

    void*   Alloc(UPInt size)
    {
#ifdef FXPLAYER_MEMORY_TRACKSIZES
        void *pret;

        Stats.AllocCount++;
        pret = malloc(size+sizeof(UPInt));
        if (pret)
        {
            Stats.AllocSize += size;
            *((UPInt*)pret) = size;
            return ((UPInt*)pret)+1;
        }
        return 0;

#else // NO MEMORY TRACKING
        return malloc(size);
#endif
    }

    void*   Realloc(void *pmem, UPInt size)
    {
#ifdef  FXPLAYER_MEMORY_TRACKSIZES
        Stats.ReallocCount++;
        UPInt oldSize = pmem ? *((UPInt*)pmem-1) : 0;
        void *pret =  realloc(pmem ? ((UPInt*)pmem-1) : NULL,size + sizeof(UPInt));
        if (pret)
        {
            if (oldSize>size)
                Stats.FreeSize += (oldSize - size);
            else
                Stats.AllocSize += (size - oldSize);
            *((UPInt*)pret) = size;
            return ((UPInt*)pret) + 1;
        }
        return 0;
#else
        return realloc(pmem,size);
#endif
    }

};

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

    // Set to disable system font.
    bool        NoSystemFont;
    bool        NoControlKeys;
    // If not empty, specifies a file to load int FontLib.
    GFxString   FontLibFile;
    // Set to disable automatic load of fontconfig.txt
    bool        NoFontConfigLoad;

    // Font configurations, if specified.
    FontConfigSet   FontConfigs;
    // Index of currently applied FontConfig, -1 for none.
    SInt            FontConfigIndex;

    // Set to play movie as fast as possible
    bool    FastForward;

    Float   ExitTimeout;

    // PlaybackFile
    char    FileName[256];

    // FontConfigFile
    GFxString   FontConfigFilePath;
    GFileStat   FontConfigFileStats;


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

        NoSystemFont        = 0;
        NoControlKeys       = 0;
        NoFontConfigLoad    = 0;

        // No font config used by default.
        FontConfigIndex     = -1;

        FastForward         = 0;

        ExitTimeout         = 0.0f;

        // Clear file
        FileName[0]         = 0;

        
    }

    // Initializes settings based on the command line.
    // Returns 1 if parse was successful, otherwise 0 if usage should be displayed.
    bool        ParseCommandLine(int argc, char *argv[]);

    void        LoadFontConfigs(ConfigParser *parser);
    FontConfig*        GetCurrentFontConfig();

    bool            LoadDefaultFontConfigFromPath(char* path);

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
    GPtr<FXPLAYER_RENDER> pRenderer;
    GPtr<GFxRenderConfig> pRenderConfig;
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
    void            UpdateHudText();
    // Updates the view size based on the ScaleEnable flag and window size.
    void            UpdateViewSize();

    void            ResetUserMatrix();
    void            UpdateUserMatrix();
    GSizeF          GetMovieScaleSize();

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
};



/*  GFxPlayerLog - Output log with message counter filtering.

    This class implements GFxLog by outputting messages to screen or a redirected
    file; it receives messages by overriding the LogMessageVarg virtual function.

    If repetitive script error messages arrive, they are combined into one and a
    counter is displayed next to a message. Such message counting and filtering
    is done DisplayCountedMessage. If counting of messages is not necessary,
    vprintf(pfmt, argList) could be used instead of DisplayCountedMessage.
*/

class GFxPlayerLog : public GFxLog
{
public:

    // Variables used for repeated message detection.
    enum { Log_MaxBuffer = 512 };
    char    LastMessageBuffer[Log_MaxBuffer];
    int     LastMessageLength;
    int     LastMessageCounter;
    int     CounterStrLength;
    bool    LastMessageHadNewline;
    bool    IsFileMode;

    // Colors used.
    enum ConsoleColor
    {
        ConsoleColor_Gray   = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE,
        ConsoleColor_White  = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY,
        ConsoleColor_Yellow = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY,
        ConsoleColor_Cyan   = FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY
    };
    // Console output handle and active colors.
    HANDLE          hOutput;
    ConsoleColor    Color, LastColor;


    GFxPlayerLog()
    {
        LastMessageLength = 0;
        LastMessageHadNewline = 1;

        // We do custom processing if we are in console (screen-output) mode.
        //  - In screen mode we display the message immediately and use '\b'
        //    character to overwrite the message counter as duplicate messages arrive.
        //  - In file mode, '\b' doesn't work, so we wait until the next message
        //    before outputting the counter.
        IsFileMode = 1;
        hOutput    = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD Mode;
        if (GetConsoleMode(hOutput, &Mode))
            IsFileMode = 0;
        // Default colors.
        Color = LastColor = ConsoleColor_Gray;
    }

    ~GFxPlayerLog()
    {
        // Finish last counted log line, if any.
        if (LastMessageLength && LastMessageHadNewline)
        {
            if (IsFileMode && (LastMessageCounter > 1))
                DisplayCounter(LastMessageCounter);
            printf("\n");
        }
        SetTextColor(ConsoleColor_Gray);
        UpdateColor();
    }

    static bool MessageNeedsCounting(LogMessageType messageType)
    {
        return (messageType == Log_ScriptError) || (messageType == Log_ScriptWarning);
    }

    // Delayed color update - useful to reduce overhead of SetConsoleTextAttribute calls.
    void SetTextColor(ConsoleColor attributes)
    {
        Color = attributes;
    }
    void UpdateColor()
    {
        if (Color != LastColor)
        {
            ::SetConsoleTextAttribute(hOutput, (WORD)Color);
            LastColor = Color;
        }
    }

    // Displays counter message and returns the number of characters in it.
    static int  DisplayCounter(int counter, int backspaceCount = 0)
    {
        char    backspaceBuff[16], counterBuff[32];
        char*   pbackspace;
        int     i, counterLength;

        // Concatenate backspaces to output string; it's too slow to display them
        // individually through printf.
        for (i = backspaceCount, pbackspace = backspaceBuff; i > 0; pbackspace++,  i--)
            *pbackspace = '\b';
        *pbackspace = 0;

        counterLength = (int)gfc_sprintf(counterBuff, 32, "%s (x%d)", backspaceBuff, counter);
        printf(counterBuff);
        return counterLength - backspaceCount;
    }

    void    DisplayCountedMessage(LogMessageType messageType, const char* pfmt, va_list argList)
    {
        UpdateColor();

        // If previous message exist and we don't need counting, just flush and output.
        if (!MessageNeedsCounting(messageType))
        {
            if (LastMessageLength && LastMessageHadNewline)
            {
                printf("\n");
                LastMessageLength = 0;
            }
            // Output new message.
            vprintf(pfmt, argList);
            return;
        }

        // Messages might need counting, so do so.
        char    messageBuffer[Log_MaxBuffer];
        int     messageLength = (int)gfc_vsprintf(messageBuffer, Log_MaxBuffer-1, pfmt, argList);

        if (messageLength > 0)
        {
            messageBuffer[messageLength] = 0;

            if (!LastMessageHadNewline)
            {
                // If last message didn't have a newline, just concatenate directly.
                // This is a part of the previous message and we don't count these.
                printf("%s", messageBuffer);
                if (messageBuffer[messageLength-1] == '\n')
                {
                    LastMessageHadNewline = 1;
                    LastMessageLength = 0;
                }
            }
            else
            {
                bool haveNewline = 0;
                if (messageBuffer[messageLength-1] == '\n')
                {
                    messageBuffer[messageLength-1] = 0;
                    haveNewline = 1;
                }

                // Consider counting logic.
                if (!LastMessageLength || !haveNewline)
                {
                    // No last message, just output us as-is.
                    printf("%s", messageBuffer);
                    gfc_strcpy(LastMessageBuffer, Log_MaxBuffer, messageBuffer);
                    LastMessageLength       = messageLength;
                    LastMessageCounter      = 1;
                    LastMessageHadNewline   = haveNewline;
                    CounterStrLength        = 0;
                }
                else
                {
                    // We had last message: see if they are the same.
                    if (gfc_strcmp(LastMessageBuffer, messageBuffer) == 0)
                    {
                        // Strings match, so erase old count and update counter.
                        LastMessageCounter++;

                        // Update and display new counter.
                        if (!IsFileMode)
                            CounterStrLength = DisplayCounter(LastMessageCounter, CounterStrLength);
                    }
                    else
                    {
                        // No match. Output newline followed by the new string.
                        if (IsFileMode && (LastMessageCounter > 1))
                            DisplayCounter(LastMessageCounter);

                        printf("\n%s", messageBuffer);
                        gfc_strcpy(LastMessageBuffer, Log_MaxBuffer, messageBuffer);
                        LastMessageLength       = messageLength;
                        LastMessageCounter      = 1;
                        LastMessageHadNewline   = 1;
                        CounterStrLength        = 0;
                    }
                }
            }
        }
    }


    // We override this function in order to do custom logging.
    virtual void    LogMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
    {
        // Output log to console.
        // Highlight script messages for readability.
        if (messageType & Log_Channel_Script)
        {
            if (messageType == Log_ScriptMessage)
                SetTextColor(ConsoleColor_Yellow);
            else // brighter white
                SetTextColor(ConsoleColor_White);
        }
        else if (messageType == Log_Message)
        {   // FsCommand only, player does not generate messages
            // (only errors, warnings, and parse statements).
            SetTextColor(ConsoleColor_Cyan);
        }

        // Non-counted alternative: vprintf(pfmt, argList).
        DisplayCountedMessage(messageType, pfmt, argList);

        // Back to normal.
        if ((messageType & Log_Channel_Script) || (messageType == Log_Message))
            SetTextColor(ConsoleColor_Gray);
    }
};


// File opener class.
class FxPlayerFileOpener : public GFxFileOpener
{
public:
    virtual GFile* OpenFile(const char *purl)
    {
        // Buffered file wrapper is faster to use because it optimizes seeks.
        return new GBufferedFile(GPtr<GSysFile>(*new GSysFile(purl)));
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
    }
};

// Older window do not define this.
#ifndef IDC_HAND
#define IDC_HAND IDC_ARROW
#endif

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




// ***** Main function implementation

int GCDECL main(int argc, char *argv[])
{
#ifdef FXPLAYER_USE_SYSALLOCATOR
    // Must set custom memory hooks before
    // creating FxPlayerApp or any other GFx objects
    SysAllocator    Allocator;
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
    ClippingEnable = 1;
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

    MouseX = 0;
    MouseY = 0;
    MouseTracking   = None;
    ControlKeyDown  = 0;

    SizingEntered = 0;

    CurvePixelError = 1.0f;

    // No old pos, save during FullScreen mode
    OldWindowX = OldWindowY = 0;
    OldWindowWidth = OldWindowHeight = 0;
}

FxPlayerApp::~FxPlayerApp()
{
}

//  Optional translator implementation that can be used for testing.
/*
class TranslatorImpl : public GFxTranslator
{
    virtual UInt    GetCaps() const         { return Cap_ReturnHtml | Cap_StripTrailingNewLines; }
    virtual bool    Translate(GFxWStringBuffer *pbuffer, const wchar_t *pkey)
    {
        // Translated strings start with $$sign
        if (pkey[0] == '$')
        {
            size_t l = wcslen(pkey);
            pbuffer->Resize(l + 10);
            wchar_t* pb = pbuffer->GetBuffer();

            wcscpy(pb, L"<font color='#00FF33'>*TR[<font color='#FF3300'>");
            wcscat(pb, pkey+1);
            wcscat(pb, L"</font>]</font>");
            return true;
        }
        return false;
    }
};*/

SInt    FxPlayerApp::Run()
{

    // Set the verbose flags.
    UInt       verboseFlags = 0;

    if (Settings.VerboseParse)
        verboseFlags |= GFxParseControl::VerboseParse;
    if (Settings.VerboseParseShape)
        verboseFlags |= GFxParseControl::VerboseParseShape;
    if (Settings.VerboseParseAction)
        verboseFlags |= GFxParseControl::VerboseParseAction;

    GPtr<GFxParseControl> pparseControl = *new GFxParseControl(verboseFlags);
    Loader.SetParseControl(pparseControl);

    // File callback.
    GPtr<GFxFileOpener> pfileOpener = *new FxPlayerFileOpener;
    Loader.SetFileOpener(pfileOpener);


    // Set log, but only if not quiet
    if (!Settings.Quiet)
    {
        Loader.SetLog(GPtr<GFxLog>(*new GFxPlayerLog()));
    }
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
        GPtr<GFxFontProviderWin32> fontProvider = *new GFxFontProviderWin32(::GetDC(0));
        Loader.SetFontProvider(fontProvider);


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
        SInt w = ViewWidth, h = ViewHeight;
        if (!SetupWindow(FXPLAYER_APP_TITLE, ViewWidth, ViewHeight))
            return 1;


        // It is important to initialize these sizes, in case OnSizeEnter gets called.
        SizeWidth  = Width;
        SizeHeight = Height;

        // Width & Height might be changed by SetupWindow call above
        if (w != Width || h != Height)
            UpdateViewSize();

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

        // Set renderer on loader so that it is also applied to all children.
        pRenderConfig = *new GFxRenderConfig(pRenderer);
        Loader.SetRenderConfig(pRenderConfig);

        // Create a renderer stats object since we will be tracking statistics.
        pRenderStats = *new GFxRenderStats();
        Loader.SetRenderStats(pRenderStats);

    }

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
            RecreateRenderer();
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

            // Otherwise, if there is no movie playing, display a drop message.
            else if (!pMovie)
            {
                Push2DRenderView();
                const char  *pmessage = "Drag and Drop SWF/GFX File Here";
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
            return new GImageInfo();
        case GFxImageCreateInfo::Input_Image:
            return new GImageInfo(info.pImage);

        case GFxImageCreateInfo::Input_File:

#ifdef FXPLAYER_PRELOAD_IMAGES
            // VisitImages will do the pre-loading.
            return new GImageInfo(0, info.FileInfo.TargetWidth, info.FileInfo.TargetHeight);
#else
            if (info.FileInfo.Format == GFxFileConstants::Image_TGA)
                return new GImageInfo(0, info.FileInfo.TargetWidth, info.FileInfo.TargetHeight);
            else
                return new GImageFileInfo(info.FileInfo.TargetWidth, info.FileInfo.TargetHeight);
#endif

    }
    return 0;
}

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
        wcscpy(localBuf, L"Oppa!");     // just to demonstrate how to return local wide char strings
        pmovieView->SetExternalInterfaceRetVal(GFxValue(localBuf));
        // or, the most simple way to return a value is as follows:
        //pmovieView->SetExternalInterfaceRetVal(GFxValue("Oppa!"));
    }
};
*/

// Load a new movie from a file and initialize timing
bool    FxPlayerApp::LoadMovie(const char *pfilename)
{
    // Try to load the new movie
    GPtr<GFxMovieDef>   pnewMovieDef;
    GPtr<GFxMovieView>  pnewMovie;
    GFxMovieInfo        newMovieInfo;

    // Get info about the width & height of the movie.
    if (!Loader.GetMovieInfo(pfilename, &newMovieInfo, 0, GFxLoader::LoadKeepBindData))
    {
        fprintf(stderr, "Error: Failed to get info about %s\n", pfilename);
        return 0;
    }

    UInt loadConstants = GFxLoader::LoadAll;
    /*
    if (newMovieInfo.IsStripped())
    {
        Loader.SetImageCreateCallback(GFxImageCreateCallback, 0);
        loadConstants &= (~GFxLoader::LoadImageData);
    }
    else
    {
        // For normal '.swf' files use default image implementation.
        Loader.SetImageCreateCallback(0, 0);
    }
    */

    // Load the actual new movie and crate instance.
    // Don't use library: this will ensure that the memory is released.
    pnewMovieDef = *Loader.CreateMovie(pfilename, loadConstants|GFxLoader::LoadOrdered|GFxLoader::LoadKeepBindData);
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
    pMovieDef   = pnewMovieDef;
    pMovie      = pnewMovie;
    memcpy(&MovieInfo, &newMovieInfo, sizeof(GFxMovieInfo));

    // This should only be true if this is the GFxPlayer application
    // Make sure to comment this out or set the value to false in your game
    pMovie->SetVariable("_global.gfxPlayer", GFxValue(true));


    // Set ActionScript verbosity / etc.
    GPtr<GFxActionControl> pactionControl = *new GFxActionControl();
    pactionControl->SetVerboseAction(Settings.VerboseAction);
    pactionControl->SetActionErrorSuppress(Settings.NoActionErrors);
    pMovie->SetActionControl(pactionControl);

    // Install handlers.
    pMovie->SetFSCommandHandler(GPtr<GFxFSCommandHandler>(*new FxPlayerFSCallback()));
    pMovie->SetUserEventHandler(GPtr<GFxUserEventHandler>(*new FxPlayerUserEventHandler(this)));

    // setting ExternalInterface handler
    //GPtr<CustomEIHandler> pei = *new CustomEIHandler();
    //pMovie->SetExternalInterface(pei);

    // init first frame
    pMovie->Advance(0.0f, 0);

    // Renderer
    if (Settings.DoRender)
    {
        if (Settings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
            pRenderConfig->SetRenderFlags(pRenderConfig->GetRenderFlags() | GFxRenderConfig::RF_EdgeAA);
    }

    if (Settings.DoSound)
    {   // TBD:
        //  GFxSoundPlayer* psound = NULL;
        //      psound = GFxSoundPlayer::CreatePlayer ();
        //      pMovie->SetSoundPlayer(psound);
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
            pRenderer->ResetVideoMode();

            // Call original on
            FXPLAYER_APP::OnSize(SizeWidth, SizeHeight);

            // Update view based on the new window size and scale settings.
            UpdateViewSize();

        #ifdef  FXPLAYER_RENDER_DIRECT3D
            pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
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
                ViewWidth   = (SInt)((float) height / hw);
                ViewHeight  = height;
            }
            else
            {
                // Use width
                ViewWidth   = width;
                ViewHeight  = (SInt) (width * hw);
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
        ViewWidth   = MovieInfo.Width;
        ViewHeight  = MovieInfo.Height;
        Settings.ScaleX = Settings.ScaleY = 1.0f;
    }
}


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
        "AA Mode:   %s %s\n"
        "FontCfg:   %s",

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
        optTriangles ? "*Opt" : "",
        (Settings.FontConfigIndex == -1) ? "Default" :
            Settings.FontConfigs[Settings.FontConfigIndex]->ConfigName.ToCStr()
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
            "  CTRL N          Next font config\n"
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
    // unload the translator
    if (Settings.FontConfigIndex >= 0)
        Loader.SetTranslator(NULL);

    if ( Settings.LoadDefaultFontConfigFromPath(path) )
    {
        // set the font config
        FontConfig* pconfig = Settings.GetCurrentFontConfig();
        if (pconfig)        
            pconfig->Apply(&Loader);
    }


    ResetCursor();
    ResetUserMatrix();
    LoadMovie(path);
    ::BringWindowToTop(hWND);
    ::SetForegroundWindow(hWND);
}

void    FxPlayerApp::OnKey(UInt keyCode, UInt info, bool downFlag)
{
    if (!pMovie)
        return;

    bool    ctrl = false;
    if (!Settings.NoControlKeys)
    {
        ctrl = ControlKeyDown;
        //  GFxLog* plog = pMovie->GetLog();

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
    }

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
                        pRenderer->ResetVideoMode();

                        // Call original on
                        RecreateRenderer();

                        pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
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
                    pRenderer->ResetVideoMode();

                    // Set new D3D mode, location & size. Recover if that fails.
                    if (!ConfigureWindow(x, y, w, h, FullScreen))
                        goto restore_video_mode;

                    UpdateViewSize();

                    pRenderer->SetDependentVideoMode(pDevice, &PresentParams, 0, hWND);
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

            
            case 'N':
                // Switch international font.
                if ((Settings.FontConfigIndex != -1) &&
                    (Settings.FontConfigs.size() > 1))
                {
                    // Apply different settings and reload file.

                    Settings.FontConfigIndex++;
                    Settings.FontConfigIndex %= (SInt)Settings.FontConfigs.size();
                    FontConfig* pconfig = Settings.GetCurrentFontConfig();
                    if (pconfig)
                        pconfig->Apply(&Loader);
                    ResetCursor();
                    // Reload file but with different bindings.
                    LoadMovie(pMovieDef->GetFileURL());
                    NeedHudUpdate = 1;
                }                
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
    GUNUSED(info);
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
    glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
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

            else if ((argv[arg][1] == 'f') && (argv[arg][2] == 'c'))
            {
                // Font Config file use - '/fc filename'
                arg++;
                // Load the font config file                
                ConfigParser parser(argv[arg]);
                if (!parser.IsValid())
                {
                    fprintf(stderr, "/fc - cannot find specified font config.\n" );
                    return 0;
                }
                LoadFontConfigs(&parser);
                NoFontConfigLoad = 1; // disable autoloading
            }

            else if (argv[arg][1] == 'n' && argv[arg][2] == 'f' && argv[arg][3] == 'c')
            {
                // Disables the player from looking for the fontconfig.txt file in the movie's dir - '/nfc'
                NoFontConfigLoad = 1;
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
                {
                    FastForward = 1;
                }
                else if (argv[arg][2] == 0)
                {
                    FullScreen = 1;
                }
                else if (argv[arg][2] == 'l')
                {
                    // Install fontlib file: /fl filename.swf                
                    arg++;
                    if (arg < argc)
                    {
                        FontLibFile = argv[arg];
                    }
                    else
                    {
                        fprintf(stderr, "/fl option requires a font source SWF/GFX file name.\n");
                        return 0;
                    }
                }
            }

            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 's') && (argv[arg][3] == 'f'))
            {
                // No System Font: /nsf.
                NoSystemFont = 1;
            }            

            else if ((argv[arg][1] == 'n') && (argv[arg][2] == 'c') && (argv[arg][3] == 'k'))
            {
                // No Control Keys: /nck.
                NoControlKeys = 1;
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
                // Quiet. Opposite to verbose, will not display any messages.
                NoActionErrors = 1;

                if (VerboseAction)
                {
                    fprintf(stderr, "Quiet option /qse conflicts with specified verbose options.\n");
                    return 0;
                }
            }
            else if (argv[arg][1] == 'q')
            {
                // Quiet. Opposite to verbose, will not display any messages.
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
            {
                gfc_strcpy(FileName, 256, argv[arg]);
            }
        }
    }

    if (FileName[0])
    {
        // if filename is set, then we try to load the font config
        LoadDefaultFontConfigFromPath(FileName);
    }

    if (!FileName[0])
    {
        printf("Note: No input file specified. Use /? option for help. \n");
        return 1;
    }

    return 1;
}

void    FxPlayerSettings::LoadFontConfigs(ConfigParser *parser)
{
    FontConfigs.Parse(parser);
    if (FontConfigs.size() > 0)
    {
        FontConfigIndex = 0;
    }
    else
    {
        FontConfigIndex = -1;
    }
}


FontConfig*  FxPlayerSettings::GetCurrentFontConfig()
{
    // we are skipping over invalid fontconfigs until one is found.
    // else return NULL.

	if (FontConfigIndex == -1)
		return NULL;

    FontConfig* fc = NULL;
    SInt sIdx = FontConfigIndex; 
    bool ok = false;

    while (!ok)
    {
        ok = true;
        fc = FontConfigs[FontConfigIndex];
        // check if all fontlib files exist
        for (UInt i=0; i < fc->FontLibFiles.size(); i++)
        {        
            // check if file exists
            GSysFile file(fc->FontLibFiles[i]);
            if (!file.IsValid())
            {
                ok = false;
                fprintf(stderr, "Fontlib file '%s' cannot be found. Skipping config '%s'..\n", fc->FontLibFiles[i].ToCStr(), fc->ConfigName.ToCStr());
                break;
            }
        }

        if (!ok)
        {
            FontConfigIndex++;
            FontConfigIndex %= (SInt)FontConfigs.size();
            if (FontConfigIndex == sIdx)
                return NULL;
        }
    }

    return FontConfigs[FontConfigIndex];
}   


bool  FxPlayerSettings::LoadDefaultFontConfigFromPath(char *path)
{
    if (!NoFontConfigLoad)
    {
        // load fontconfig.txt if it exists in the movie's path
        GFxString fontConfigFilePath;

        if (GFxURLBuilder::IsPathAbsolute(path))
        {
            fontConfigFilePath.AppendString(path);
            GFxURLBuilder::ExtractFilePath(&fontConfigFilePath);
        }

        fontConfigFilePath += "fontconfig.txt";  
        bool maintainIndex = false;

        // store font config file related info
        if (FontConfigFilePath.GetLength() == 0)   // if no file was previously loaded
        {
            GFileStat fileStats;
            if ( GSysFile::GetFileStat(&fileStats, fontConfigFilePath.ToCStr()) )
            {
                FontConfigFilePath = fontConfigFilePath;
                FontConfigFileStats = fileStats;
            }
        }
        else // if the file was previously loaded and is modified
        {
            if (fontConfigFilePath == FontConfigFilePath)
            {
                // if modified time is not the same, then reload config file
                GFileStat fileStats;
                if ( GSysFile::GetFileStat(&fileStats, fontConfigFilePath.ToCStr()) )
                {
                    if ( !(fileStats == FontConfigFileStats) )
                    {
                        FontConfigFileStats = fileStats;
                        maintainIndex = true;
                    }
                }
            }
        }       

        // parse the config file
        ConfigParser parser(fontConfigFilePath.ToCStr());
        SInt oldIdx = FontConfigIndex;
        LoadFontConfigs(&parser);

        // try to maintain previous font config index
        if ( maintainIndex && 
            (FontConfigIndex == 0) && 
            (oldIdx != -1) )
        {        
            FontConfigIndex = oldIdx;
            FontConfigIndex %= (SInt)FontConfigs.size();
        }

        return true;
    }
    return false;
}



// Brief instructions.
void    FxPlayerSettings::PrintUsage()
{

    printf(
        "GFxPlayer - a sample SWF/GFX file player for the GFx library.\n"
        "\n"
        "Copyright (c) 2006-2007 Scaleform Corp. All Rights Reserved.\n"
        "Contact sales@scaleform.com for licensing information.\n"
        "\n"
        "Usage:        gfxplayer [options] movie_file.swf\n"
        "Options:\n"
        "  /?          Display this help info.\n"
        "  /s <factor> Scale the movie window size by the specified factor.\n"
        "  /na, /fsa   Use no anti-aliasing; use fullscreen HW AA.\n"
        "  /f          Run in full-screen mode.\n"
        "  /nsf        No system font - disables GFxFontProviderWin32.\n"
        "  /fc <fname> Load a font config file.\n"
        "  /nfc        Disable autoloading of font config file in movie's path.\n"
        "  /fl <fname> Specifies a SWF/GFX file to load into GFxFontLib.\n"
        "  /i          Display info HUD on startup.\n"
        "  /vp         Verbose parse - print SWF parse log.\n"
        "  /vps        Verbose parse shape - print SWF shape parse log.\n"
        "  /vpa        Verbose parse action - print SWF actions during parse.\n"
        "  /va         Verbose actions - display ActionScript execution log.\n"
        "  /q          Quiet. Do not display errors or trace statements.\n"
        "  /qae        Suppress ActionScript errors.\n"
        "  /ml <bias>  Specify the texture LOD bias (float, default -0.5).\n"
        "  /p          Performance test - run without delay and log FPS.\n"
        "  /ff         Fast forward - run one frame per update.\n"
        "  /1          Play once; exit when/if movie reaches the last frame.\n"
        "  /r <0|1>    0 disables rendering  (for batch tests).\n"
        "              1 enables rendering (default setting).\n"
        //"              2 enables rendering & disables sound\n"
        "  /t <sec>    Timeout and exit after the specified number of seconds.\n"
        "  /b <bits>   Bit depth of output window (16 or 32, default is 16).\n"
        "  /nck        Disable all player related control keys.\n"
        "\n"
        "Keys:\n"
        "  CTRL N          Cycle through loaded font configs.\n"
        "  CTRL S          Toggle scaled display\n"
        "  CTRL W          Toggle wireframe\n"
        "  CTRL A          Toggle antialiasing mode\n"
        "  CTRL U          Toggle fullscreen\n"
        "  CTRL F          Toggle fast mode (FPS)\n"
        "  CTRL G          Toggle fast forward\n"
        "  CTRL P          Toggle pause\n"
        "  CTRL R          Restart the movie\n"
        "  CTRL D          Toggle stage clipping\n"
        "  CTRL C          Toggle stage culling\n"
        "  CTRL O          Toggle triangle optimization\n"
        "  CTRL Right      Step backward one frame\n"
        "  CTRL Left       Step forward one frame\n"
        "  CTRL PageUp     Step back 10 frames\n"
        "  CTRL PageDown   Step forward 10 frames\n"
        "  CTRL -,+        Curve tolerance down, up\n"
        "  F1              Toggle info help\n"
        "  F2              Toggle info stats\n"
        "  CTRL Q          Quit\n"
        );
}


