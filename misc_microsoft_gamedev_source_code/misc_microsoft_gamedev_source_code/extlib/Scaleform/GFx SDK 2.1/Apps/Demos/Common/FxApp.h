/**********************************************************************

Filename    :   FxApp.h
Content     :   Base FxApp class declaration. System specific app
                classes derive from this in their own files.
Created     :   Jan 10, 2008
Authors     :   Maxim Didenko, Dmitry Polenur, Michael Antonov

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_FxApp_H
#define INC_FxApp_H

#include "GTypes.h"
#include "GRefCount.h"


// Base class for IME Events passed to OnIMEEvent. The actual data
// is in platform specific derived class such as FxWin32IMEEvent.
struct FxAppIMEEvent { };


// ***** FxApp

// FxApp is a base application class which has a number of platform
// specific implementations, such as Direct3DWin32App and GcmPS3App.
// While the user application (FxPlayerApp, etc) usually derives from
// the platform specific classes, it uses functions declared here
// for portability.

class GRenderer;
class FxApp 
{
public:
    FxApp();
    virtual ~FxApp();


    virtual bool CreateRenderer() = 0;
    // *** Initialization

    // Each subclass must provide a static InitMain function, and all apps must call it 
    // before allocating memory or using graphics features (including memory
    // allocation from static constructors).

    static void     InitMain() {}

    
    // *** Window Setup / Message APIs

    // Describes extra parameters for window setup which are
    // necessary on some platforms.
    struct SetupWindowParams
    {
        SetupWindowParams()
            : X(0), Y(0),
              FullScreen (false), ForceTiling(false) {}
        int  X,Y;
        bool FullScreen;
        bool ForceTiling;
    };

    // Creates a window & initializes the application class.
    // Returns 0 if window creation/initialization failed (the app should quit).
    virtual bool    SetupWindow(const char *pname, int width, int height, 
                                const SetupWindowParams& extraParams = SetupWindowParams()) = 0;

    // Message processing function to be called in the
    // application loops until it returns 0, to indicate application being closed.
    virtual bool    ProcessMessages() { return true; }
    // Sleeps for the specified number of milliseconds or till message.
    virtual void    SleepTillMessage(unsigned int /*ms*/) { }
    virtual void    SleepMilliSecs(unsigned int ms)     = 0;

    virtual void    BringMainWindowOnTop() { }

    // This call cleans up the application and kills the window.
    // If not called explicitly, it will be called from the destructor.
    virtual void    KillWindow()                        = 0;

    // Changes/sets window title
    virtual void    SetWindowTitle(const char *ptitle)  = 0;   

    // Amount (multiple of size) to reduce viewport by to fit in visible area on TVs.
    // This is a floating point value such as 0.1 for 10% safe frame border.
    virtual Float   GetSafeArea() { return 0; }

    virtual GRenderer* GetRenderer() = 0;

    // Presents the data (swaps the back and front buffers).
    virtual void    PresentFrame()                      = 0;

    // On successful reset, this function will call InitRenderer again.
    virtual bool    RecreateRenderer()                  = 0;

    // This override is called from SetupWindow to initialize OpenGL/Direct3D view
    virtual bool    InitRenderer()                      = 0;
    // Should/can be called every frame to prepare the render, user function
    virtual void    PrepareRendererForFrame()           = 0;

    // This method is used to switch between full and non-full screen mode
    virtual void    SwitchFullScreenMode() {}


    // *** Input Constants and 'On' Functions

    // These key codes match Flash-defined and GFxKey::Code values exactly.
    // we have these codes here because we can not (MAC reason) have a dependency
    // on any GFx header files
    enum KeyCode
    {
        VoidSymbol      = 0,

        // A through Z and numbers 0 through 9.
        A               = 65,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        Num0            = 48,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,

        // Numeric keypad.
        KP_0            = 96,
        KP_1,
        KP_2,
        KP_3,
        KP_4,
        KP_5,
        KP_6,
        KP_7,
        KP_8,
        KP_9,
        KP_Multiply,
        KP_Add,
        KP_Enter,
        KP_Subtract,
        KP_Decimal,
        KP_Divide,

        // Function keys.
        F1              = 112,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,

        // Other keys.
        Backspace       = 8,
        Tab,
        ClearKey       = 12,
        Return,
        Shift           = 16,
        Control,
        Alt,
        Pause,
        CapsLock        = 20, // Toggle
        Escape          = 27,
        Space           = 32,
        PageUp,
        PageDown,
        End             = 35,
        Home,
        Left,
        Up,
        Right,
        Down,
        Insert          = 45,
        Delete,
        Help,
        NumLock         = 144, // Toggle
        ScrollLock      = 145, // Toggle

        Semicolon       = 186,
        Equal           = 187,
        Comma           = 188, // Platform specific?
        Minus           = 189,
        Period          = 190, // Platform specific?
        Slash           = 191,
        Bar             = 192,
        BracketLeft     = 219,
        Backslash       = 220,
        BracketRight    = 221,
        Quote           = 222,

        // Total number of keys.
        KeyCount
    };

    enum PadKeyCode
    {
        VoidPadKey,
        Pad_Back,
        Pad_Start,
        Pad_A,
        Pad_B,
        Pad_X,
        Pad_Y,
        Pad_R1,  // RightShoulder;
        Pad_L1,  // LeftShoulder;
        Pad_R2,  // RightTrigger;
        Pad_L2,  // LeftTrigger;
        Pad_Up,
        Pad_Down,
        Pad_Right,
        Pad_Left,
        Pad_Plus,
        Pad_Minus,
        Pad_1,
        Pad_2,
        Pad_H,
        Pad_C,
        Pad_Z,
        Pad_O,
        Pad_T,
        Pad_S,
        Pad_Select,
        Pad_Home,
        Pad_RT,  // RightThumb;
        Pad_LT   // LeftThumb;
    };

    enum KeyModifiers
    {
        KM_Control = 0x1,
        KM_Alt     = 0x2,
        KM_Shift   = 0x4,
        KM_Num     = 0x8,
        KM_Caps    = 0x10,
        KM_Scroll  = 0x20
    };

    // Input overrides, invoked during the ProcessMessages call.
    virtual void    OnKey(KeyCode /*keyCode*/, unsigned char /*asciiCode*/, unsigned int /*wcharCode*/, 
                          unsigned int /*mods*/, bool /*downFlag*/)     { };
    virtual void    OnPad(PadKeyCode /*PadkeyCode*/, bool /*downFlag*/) { }

    // IME events
    virtual bool    OnIMEEvent(const FxAppIMEEvent&) { return false; }

    // Mouse events
    virtual void    OnMouseButton(unsigned int /*button*/, bool /*downFlag*/, int /*x*/, int /*y*/, 
                                  unsigned int /*mods*/) {  }
    virtual void    OnMouseMove(int /*x*/, int /*y*/, int unsigned /*mods*/) {}
    virtual void    OnMouseWheel(int /*zdelta*/, int /*x*/, int /*y*/, unsigned int /*mods*/) { }

    virtual void    ResizeWindow(int /*w*/, int /*h*/) { }
    // Sizing; by default, re-initalizes the renderer
    virtual void    OnSize(int w, int h)   { ResizeWindow(w,h); }
    // Called when sizing begins and ends.
    virtual void    OnSizeEnter(bool /*enterSize*/) { }
    // Handle dropped files.
    virtual void    OnDropFiles(char* /*path*/)     { }

    virtual void    OnUpdateSystemCliboard(const wchar_t* /*text*/) { }

    virtual bool    UpdateFiles(char* /*filename*/, bool /* prev*/) { return 0; }

    // Query display status
    enum DisplayStatus
    {
        DisplayStatus_Ok            = 0,
        DisplayStatus_NoModeSet     = 1,    // Video mode 
        DisplayStatus_Unavailable   = 2,    // Display is unavailable for rendering; check status again later
        DisplayStatus_NeedsReset    = 3     // May be returned in Dependent mode to indicate external action being required
    };
    virtual DisplayStatus  CheckDisplayStatus() const = 0;


    // *** Simple Rendering APIs

    // Clears the entire back buffer.
    virtual void    Clear(unsigned int color) = 0;

    // Initialize and restore 2D rendering view.
    // Push2DRenderView must be called before using 2D functions below,
    // while Pop2DRenderView MUST be called after it is finished.
    virtual void    Push2DRenderView() = 0;
    virtual void    Pop2DRenderView() = 0;    

	// Draw a filled + blended rectangle.
    virtual void    FillRect(int l, int t, int r, int b, unsigned int color) = 0;
    // Draw a text string (specify top-left corner of characters, NOT baseline)
    virtual void    DrawText(int /*x*/, int /*y*/, const char* /*ptext*/, unsigned int /*color*/)  {}
    // Compute text size that will be generated by DrawText
    virtual void    CalcDrawTextSize(int* /*pw*/, int* /*ph*/, const char* /*ptext*/) {}
	
    // API-independent toggle for wireframe rendering.
    virtual void    SetWireframe(bool wireframeEnable) = 0;
  
    virtual void    SwitchFSAA(bool /*on_off*/) {}
    virtual void    SetVSync(bool /*isEnabled*/){}  


    // *** Window Size / State Query

    int             GetWidth()  const { return Width; }
    int             GetHeight() const { return Height; }
    void            SetWidth(int w)   { Width=w; }
    void            SetHeight(int h)  { Height=h; }

    bool            IsConsole() const { return Console; }


    // *** Mouse and Cursor functions

    virtual void    StartMouseCapture() {}
    virtual void    EndMouseCapture()   {}

     // Draw a mouse cursor (usually after the scene). Used only on consoles.
    virtual void    RedrawMouseCursor() {}
    
    virtual void    ShowCursorInstantly(bool /*show*/) {}
    void            ShowCursor(bool show = true);
    void            ResetCursor();


    // Cursor states
    bool            CursorHidden;
    bool            CursorHiddenSaved;
    bool            CursorDisabled;
    bool            CursorIsOutOfClientArea;

    // Boolean states for the window and app
    bool            Created;
    bool            Active;
    bool            QuitFlag;
    bool            LockOnSize;
    int             ExitCode;

    // Set to true if this platform is a console; should be
    // set in the constructor for the particular platform.
    bool            Console;
	
    // Set this if the window is to accept drag and drop
    bool            SupportDropFiles;
    bool            SizableWindow;

    // Requested 3D state
    bool            FullScreen;
    bool            FSAntialias;
    bool            VSync;
    unsigned int    BitDepth;

    // This flag will be set after SetupWindow to indicate that FSAA is available.
    bool            FSAASupported;

    int             Width;
    int             Height;

    unsigned int    VMCFlags;

    // Old width and height saved during FullScreen mode
    int             OldWindowX, OldWindowY;
    int             OldWindowWidth, OldWindowHeight;

    static FxApp*   pApp; 
};

#endif //INC_FxApp_H
