/**********************************************************************

Filename    :   FxPlayerConfig.h
Content     :   Sample SWF/GFX file player leveraging GFxPlayer API
Created     :   January 10, 2008
Authors     :   Michael Antonov, Maxim Didenko

Copyright   :   (c) 2005-2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FxPlayerConfig_H
#define INC_FxPlayerConfig_H

// Adds the word "Debug" to the application
// title if in debug build mode
#ifdef GFC_BUILD_DEBUG
#define GFC_DEBUG_STRING    " " GFC_BUILD_STRING
#else
#define GFC_DEBUG_STRING
#endif

// This includes the appropriate system-specific header files
// and defines the following macros:
//
//  - FXPLAYER_APP              - Base application class we will use.
//  - FXPLAYER_APP_TITLE        - Title-bar message.
//  - FXPLAYER_FILEDIRECTORY    - Platform specific sample file directory.
//  - FXPLAYER_FILENAME         - Initial file to load, if any.
//  - FXPLAYER_VIEWWIDTH        - Default width of window or video-mode.
//  - FXPLAYER_VIEWHEIGHT       - Default height of window or video-mode. 
//  - FXPLAYER_FONT_SIZE        - Scaling size for HUD font on some platforms.
//  - FXPLAYER_FILEOPENER       - System-specific GFxFileOpener constructor.

#if defined(FXPLAYER_X11)
    // OpenGL application class
    #include "../Common/OpenGLX11App.h"
    #undef None
    #define FXPLAYER_APP    OpenGLX11App
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer OpenGL v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
    #define WHEEL_DELTA 1
    #define FXPLAYER_FILEPATH           ""
    #define FXPLAYER_VIEWWIDTH          1280
    #define FXPLAYER_VIEWHEIGHT         1024
    #include <unistd.h>

#elif defined(GFC_OS_WIN32)
    #ifdef  FXPLAYER_RENDER_DIRECT3D
      #ifndef GFC_D3D_VERSION
      #define GFC_D3D_VERSION 9
      #endif
      #undef FXPLAYER_RENDER
      #if (GFC_D3D_VERSION == 9)
        // Direct3D application class
        #include "Direct3DWin32App.h"
        #define FXPLAYER_APP    Direct3DWin32App
        // Window name
        #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer D3D9 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
      #elif (GFC_D3D_VERSION == 8)
        // Direct3D application class
        #include "Direct3DWin32App.h"
        #define FXPLAYER_APP    Direct3DWin32App
        // Window name
        #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer D3D8 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
      #elif (GFC_D3D_VERSION == 10)
        // Direct3D application class
        #include "Direct3DWin32App.h"
        #define FXPLAYER_APP    Direct3DWin32App
        // Window name
        #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer D3D10 v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
      #endif
    #else
        // OpenGL application class
        #include "OpenGLWin32App.h"
        #define FXPLAYER_APP    OpenGLWin32App
        // Window name
        #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer OpenGL v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
    #endif
    #define FXPLAYER_FILEPATH           ""
    #define FXPLAYER_VIEWWIDTH          1024
    #define FXPLAYER_VIEWHEIGHT         768
    #include <zmouse.h> // for WHEEL_DELTA macro
    #include "GFxFontProviderWin32.h"

#elif defined(GFC_OS_MAC)
    // OpenGL application class
    #include "../Common/OpenGLMacApp.h"
    #undef None
    #define FXPLAYER_APP    OpenGLMacApp
    // Window name
    #define     FXPLAYER_APP_TITLE  "Scaleform GFxPlayer OpenGL v" GFC_FX_VERSION_STRING GFC_DEBUG_STRING
    #define WHEEL_DELTA 1
    #define FXPLAYER_FILEPATH           ""
    #define FXPLAYER_VIEWWIDTH          1024
    #define FXPLAYER_VIEWHEIGHT         768
    #include <unistd.h>

#elif defined(GFC_OS_XBOX360)
    // Direct3D application class
    #include "Direct3DXbox360App.h"
    #define FXPLAYER_APP                Direct3DXboxApp
    #define FXPLAYER_APP_TITLE          "Scaleform GFxPlayer XBox 360 v" GFC_FX_VERSION_STRING
    #define FXPLAYER_RENDER_DIRECT3D
    // - FXPLAYER_FILENAME must be located in this path
    #define FXPLAYER_FILEDIRECTORY      "D:\\Samples\\"
    //#define FXPLAYER_FILENAME           "GFx_GDC_06.swf"
    #define FXPLAYER_FILENAME           "Mouse.swf"
    #define FXPLAYER_FILEPATH           FXPLAYER_FILEDIRECTORY FXPLAYER_FILENAME
    // Resolution at which the player will start, normally 640x480.
    // Specify 1280 and 720 for 720p res.
    #define FXPLAYER_VIEWWIDTH          640
    #define FXPLAYER_VIEWHEIGHT         480
    #define FXPLAYER_FONT_SIZE          10
    #define WHEEL_DELTA 1

#elif defined (GFC_OS_PS3)
  #if defined(FXPLAYER_RENDER_GCM)
    #include "../Common/GcmPS3App.h"
    #define FXPLAYER_APP                GcmPS3App
  #else
    #include "../Common/OpenGLPS3App.h"
    #define FXPLAYER_APP                OpenGLPS3App
  #endif
    #include <sys/paths.h>
    #define FXPLAYER_APP_TITLE          "Scaleform GFxPlayer PS3 v" GFC_FX_VERSION_STRING
    #define FXPLAYER_FILEDIRECTORY      REMOTE_PATH
    #define FXPLAYER_FILENAME           "flash.swf"
    #define FXPLAYER_FILEPATH           FXPLAYER_FILEDIRECTORY FXPLAYER_FILENAME
    #define FXPLAYER_VIEWWIDTH          640
    #define FXPLAYER_VIEWHEIGHT         480
    #define FXPLAYER_FONT_SIZE          10
    #define WHEEL_DELTA 1

#elif defined (GFC_OS_PS2)
   #include "../Common/PS2App.h"
   #define FXPLAYER_APP                PS2App
   // The path that will be searched for files 
   // - FXPLAYER_FILENAME must be located in this path
   #define FXPLAYER_FILEDIRECTORY      "host0:/usr/local/sce/FxPlayer/"
   #define FXPLAYER_FILENAME           "flash.swf"
   #define FXPLAYER_FILEPATH           FXPLAYER_FILEDIRECTORY FXPLAYER_FILENAME
   #define FXPLAYER_APP_TITLE          "Scaleform GFxPlayer PS2 v" GFC_FX_VERSION_STRING
   #define FXPLAYER_VIEWWIDTH          640
   #define FXPLAYER_VIEWHEIGHT         448
   #define FXPLAYER_FONTCACHE_SMALL
   #define FXPLAYER_FONT_SIZE          10
   #define FXPLAYER_FILEOPENER(purl)   GFilePS2Open(purl)
   #define WHEEL_DELTA 1

#elif defined (GFC_OS_PSP)
   #include "../Common/PSPApp.h"
   #define FXPLAYER_APP                PSPApp
   // The path that will be searched for files 
   // - FXPLAYER_FILENAME must be located in this path
   #define FXPLAYER_FILEDIRECTORY      "host0:/usr/local/psp/FxPlayer/"
   #define FXPLAYER_FILENAME           "flash.swf"
   #define FXPLAYER_FILEPATH           FXPLAYER_FILEDIRECTORY FXPLAYER_FILENAME
   #define FXPLAYER_VIEWWIDTH          640
   #define FXPLAYER_VIEWHEIGHT         480
   #define WHEEL_DELTA 1
   #define FXPLAYER_APP_TITLE          "Scaleform GFxPlayer PSP v" GFC_FX_VERSION_STRING
   #define FXPLAYER_FONTCACHE_SMALL
   #define FXPLAYER_FONT_SIZE          10
   #define FXPLAYER_FILEOPENER(purl)   GFilePSPOpen(purl)

#elif defined (GFC_OS_WII)
   #include "../Common/WiiApp.h"
   #define FXPLAYER_APP                WiiApp
   // The path that will be searched for files 
   // - FXPLAYER_FILENAME must be located in this path
   #define FXPLAYER_FILEDIRECTORY      "FxPlayer/"
   #define FXPLAYER_FILENAME           "flash.swf"
   #define FXPLAYER_FILEPATH           FXPLAYER_FILEDIRECTORY FXPLAYER_FILENAME
   #define FXPLAYER_VIEWWIDTH          640
   #define FXPLAYER_VIEWHEIGHT         480
   #define WHEEL_DELTA 1
   #define FXPLAYER_APP_TITLE          "Scaleform GFxPlayer Wii v" GFC_FX_VERSION_STRING
   #define FXPLAYER_FONTCACHE_SMALL
   #define FXPLAYER_FONT_SIZE          10
   #define FXPLAYER_FILEOPENER(purl)   GFileWiiDvdOpen(purl)

#endif

#ifndef GFC_NO_IME_SUPPORT
    class GFxIMEManager;
    template<class TApp>
    class TIMEHelper
    {
    public:
        static GFxIMEManager* CreateManager(TApp* app) { return NULL; }
        static bool OnEvent(const FxAppIMEEvent&, GFxMovieView*) { return false; }
    };

    #if defined(GFC_OS_WIN32)
        #ifdef GFC_USE_IME
            #include "GFxIMEManagerWin32.h"
        #endif // GFC_USE_IME
        template<>
        class TIMEHelper<FXPLAYER_APP>
        {
        public:
            static GFxIMEManager* CreateManager(FXPLAYER_APP* app) 
            { 
                GUNUSED(app);
                GFxIMEManager* pimemanager = NULL;
                #ifdef GFC_USE_IME
                    pimemanager = new GFxIMEManagerWin32(app->hWND);
                #endif // GFC_USE_IME
                return pimemanager; 
            }
            static bool OnEvent(const FxAppIMEEvent& event, GFxMovieView* pmovie) 
            { 
                if (!pmovie)
                    return false;
                const FxWin32IMEEvent& appevent = static_cast<const FxWin32IMEEvent&>(event);
                if (appevent.Preprocess)
                {
                    GFxIMEWin32Event ev(GFxIMEEvent::IME_PreProcessKeyboard,
                                        (UPInt)appevent.hWND, appevent.message, appevent.wParam, appevent.lParam, 0);
                    pmovie->HandleEvent(ev);
                    return true;
                }
                GFxIMEWin32Event ev(GFxIMEWin32Event::IME_Default,
                                    (UPInt)appevent.hWND, appevent.message, appevent.wParam, appevent.lParam, true);
                UInt handleEvtRetVal = pmovie->HandleEvent(ev);
                return (handleEvtRetVal & GFxMovieView::HE_NoDefaultAction) != 0;
            }
        };
    #endif // GFC_OS_WIN32
    typedef  TIMEHelper<FXPLAYER_APP> IMEHelper;
#endif // GFC_NO_IME_SUPPORT

// Define commands handled by FxPlayerApp::ProcessCommand(). These are mapped
// to different keyboard keys and/or gamepad buttons based on platform.
enum FxPlayerCommand
{
    FPC_Unknown,
    FPC_Quit,
    FPC_ScaledDisplay,
    FPC_Wireframe,
    FPC_AntialiasingMode,
    FPC_FastMode,
    FPC_FastForward,
    FPC_VSync,
    FPC_Pause,
    FPC_FontConfig,
    FPC_Restart,
    FPC_StageClipping,
    FPC_StageCulling,
    FPC_TriangleOptimization,
    FPC_StepBack_1,
    FPC_StepForward_1,
    FPC_StepBack_10,
    FPC_StepForward_10,
    FPC_CurveToleranceDown,
    FPC_CurveToleranceUp,
    FPC_InfoHelp,
    FPC_InfoStats,
    FPC_CycleHud,
    FPC_Fullscreen,
    FPC_DynamicTextures,
    FPC_StrokeMode,
    FPC_Background,
    FPC_ResetUserMatrix,
    FPC_LoadNextFile,
    FPC_LoadPrevFile,
    FPC_ShowMouseCursor,
    KEY_Return,
    KEY_Escape
};

struct FxPlayerCommandKeyEntry {
    FxApp::KeyCode      keyCode;
    FxPlayerCommand     cmd;
    const char*         helpText;
} FxPlayerCommandKeyMap[] = 
{
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX360) || defined(FXPLAYER_X11) || defined(GFC_OS_MAC)
    { FxApp::S, FPC_ScaledDisplay,            "  CTRL S          Toggle scaled display\n" },
#if !defined(FXPLAYER_RENDER_DIRECT3D) || (GFC_D3D_VERSION != 10)
    { FxApp::W, FPC_Wireframe,                "  CTRL W          Toggle wireframe\n" },
#endif
    { FxApp::A, FPC_AntialiasingMode,         "  CTRL A          Toggle antialiasing mode\n" },
#if !defined(GFC_OS_XBOX360)
    { FxApp::U, FPC_Fullscreen,               "  CTRL U          Toggle fullscreen\n" },
#endif
#if defined(FXPLAYER_RENDER_DIRECT3D) && (GFC_D3D_VERSION != 10) && !defined(GFC_OS_XBOX360)
    { FxApp::V, FPC_DynamicTextures,          "  CTRL V          Toggle dynamic textures\n" },
#endif
    { FxApp::F, FPC_FastMode,                 "  CTRL F          Toggle fast mode (FPS)\n" },
    { FxApp::G, FPC_FastForward,              "  CTRL G          Toggle fast forward\n" },
    { FxApp::Y, FPC_VSync,                    "  CTRL Y          Toggle VSync\n" },
    { FxApp::P, FPC_Pause,                    "  CTRL P          Toggle pause\n" },
    { FxApp::N, FPC_FontConfig,               "  CTRL N          Next font config\n" },
    { FxApp::R, FPC_Restart,                  "  CTRL R          Restart the movie\n" },
    { FxApp::D, FPC_StageClipping,            "  CTRL D          Toggle stage clipping\n" },
    { FxApp::C, FPC_StageCulling,             "  CTRL C          Toggle stage culling\n" },
    { FxApp::Z, FPC_ResetUserMatrix,          "  CTRL Z          Reset Zoom\n" },
    { FxApp::O, FPC_TriangleOptimization,     "  CTRL O          Toggle triangle optimization\n" },
    { FxApp::T, FPC_StrokeMode,               "  CTRL T          Switch stroke type\n" },
    { FxApp::Right, FPC_StepBack_1,           "  CTRL Right      Step back one frame\n" },
    { FxApp::Left, FPC_StepForward_1,         "  CTRL Left       Step forward one frame\n" },
    { FxApp::PageUp, FPC_StepBack_10,         "  CTRL PageUp     Step back 10 frames\n" },
    { FxApp::PageDown, FPC_StepForward_10,    "  CTRL PageDown   Step forward 10 frames\n" },
    { FxApp::Minus, FPC_CurveToleranceDown,   "  CTRL -          Curve tolerance down\n" },
    { FxApp::Equal, FPC_CurveToleranceUp,     "  CTRL +          Curve tolerance up\n" },
    { FxApp::BracketLeft, FPC_LoadPrevFile,   "  CTRL [          Load previous file\n"},
    { FxApp::BracketRight, FPC_LoadNextFile,  "  CTRL ]          Load next file\n"},
    { FxApp::H, FPC_InfoHelp,                 "  F1              Toggle Info Help\n" },
    { FxApp::I, FPC_InfoStats,                "  F2              Toggle Info Stats\n" },
    { FxApp::Q, FPC_Quit,                     "  CTRL Q          Quit" },
#endif
    { FxApp::VoidSymbol, FPC_Unknown, "Unknown" }
};

struct FxPlayerCommandPadKeyEntry {
    FxApp::PadKeyCode   keyCode;
    FxPlayerCommand     cmd;
    const char*         helpText;
} FxPlayerCommandPadKeyMap[] = 
{
#if defined(GFC_OS_XBOX360)
    { FxApp::Pad_A,     KEY_Return,          "" },
    { FxApp::Pad_Y,     FPC_Wireframe,       "  Y          Toggle wireframe\n" }, 
    { FxApp::Pad_X,     FPC_CycleHud,        "  X          Toggle HUD\n" },
    { FxApp::Pad_B,     FPC_FastForward,     "  B          Toggle Fast Forward\n" },
    { FxApp::Pad_Start, FPC_Pause,           "  Start      Toggle pause\n" },
    { FxApp::Pad_Back,  FPC_Restart,         "  Back       Restart the movie\n" },
    { FxApp::Pad_L1,    FPC_LoadPrevFile,    "  LB         Previous file\n" },
    { FxApp::Pad_R1,    FPC_LoadNextFile,    "  RB         Next file\n" },
    { FxApp::Pad_L2,    FPC_AntialiasingMode,"  LT         Toggle anti-aliasing\n" },
    { FxApp::Pad_R2,    FPC_ShowMouseCursor, "  RT         Enable mouse; Show/Hide cursor" },

#elif defined(GFC_OS_PS3) || defined(GFC_OS_PS2)
    { FxApp::Pad_X,     KEY_Return,          "" },
    { FxApp::Pad_T,     FPC_Wireframe,       "  Triangle   Toggle wireframe\n" }, 
    { FxApp::Pad_S,     FPC_CycleHud,        "  Square     Toggle HUD\n" },
    { FxApp::Pad_O,     FPC_FastForward,     "  Circle     Toggle Fast Forward\n" },
    { FxApp::Pad_Start, FPC_Pause,           "  Start      Toggle pause\n" },
    { FxApp::Pad_Back,  FPC_Restart,         "  Select     Restart the movie\n" },
    { FxApp::Pad_L1,    FPC_LoadPrevFile,    "  L1         Previous file\n" },
    { FxApp::Pad_R1,    FPC_LoadNextFile,    "  R1         Next file\n" },
    { FxApp::Pad_L2,    FPC_AntialiasingMode,"  L2         Toggle anti-aliasing\n" },
    { FxApp::Pad_R2,    FPC_ShowMouseCursor, "  R2         Enable mouse; Show/Hide cursor" },

#elif defined(GFC_OS_PSP)
    { FxApp::Pad_X,     KEY_Return,          "" },
    { FxApp::Pad_T,     FPC_Wireframe,       "  Triangle   Toggle wireframe\n" }, 
    { FxApp::Pad_S,     FPC_CycleHud,        "  Square     Toggle HUD\n" },
    { FxApp::Pad_O,     FPC_FastForward,     "  Circle     Toggle Fast Forward\n" },
    { FxApp::Pad_Start, FPC_Pause,           "  Start      Toggle pause\n" },
    { FxApp::Pad_Select,FPC_AntialiasingMode,"  Select     Toggle anti-aliasing\n" },
    { FxApp::Pad_L1,    FPC_LoadPrevFile,    "  L1         Previous file\n" },
    { FxApp::Pad_R1,    FPC_LoadNextFile,    "  R1         Next file\n" },

#elif defined(GFC_OS_WII)
    { FxApp::Pad_A,     KEY_Return,          "" },
    { FxApp::Pad_B,     KEY_Escape,          "" },
    { FxApp::Pad_C,     FPC_FastForward,     "  C          Toggle Fast Forward\n" },
    { FxApp::Pad_Home,  FPC_Pause,           "  Home       Toggle pause\n" },
    { FxApp::Pad_1,     FPC_CycleHud,        "  1          Toggle HUD\n" },
    { FxApp::Pad_2,     FPC_AntialiasingMode,"  2          Toggle anti-aliasing\n" },
    { FxApp::Pad_Minus, FPC_LoadPrevFile,    "  -          Previous file\n" },
    { FxApp::Pad_Plus,  FPC_LoadNextFile,    "  +          Next file\n" },

#endif
    { FxApp::VoidPadKey, FPC_Unknown, "Unknown" }
};



#endif // INC_FxPlayerConfig_H
