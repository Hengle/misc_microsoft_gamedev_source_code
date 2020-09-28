/**********************************************************************

Filename    :   OpenGLWin32App.cpp
Content     :   Simple OpenGL Win32 Application class implemenation
Created     :   
Authors     :   Michael Antonov
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "OpenGLWin32App.h"
#include "GStd.h"

// Video mode
#define  APP_COLOR_BITS         32
#define  APP_DEPTH_BITS         24
#define  APP_STENCIL_BITS       8
#define  APP_ACCUM_BITS         0

#define  WWND_CLASS_NAME         L"OGL_Window_Class"



OpenGLWin32App::OpenGLWin32App()
{
    Created     = 0;
    Active      = 0;    
    QuitFlag    = 0;
    ExitCode    = 0;
    CursorHidden  = 0;
    CursorHiddenSaved = 0;
    CursorDisabled = 0;
    CursorIsOutOfClientArea = 0;

    FullScreen  = 0;
    BitDepth    = 0;
    FSAntialias = 0;

    SupportDropFiles    = 0;

    SizableWindow = 0;

    hDC         = 0;
    hWND        = 0;
    hGLRC       = 0;
    hInstance   = 0;

    Width       = 0;
    Height      = 0;
    // Set later on in CreateWindow
    FSAASupported= 0;
    TextListsCreated = 0;

    // Clear GL function pointers
    pglMultiTexCoord2f     = NULL;
    pglActiveTexture       = NULL;
    pglClientActiveTexture = NULL;
    pglPointParameterfvEXT = NULL;
    pwglSwapIntervalEXT    = NULL;

}

OpenGLWin32App::~OpenGLWin32App()
{
    if (Created)
        KillWindow();
}


// Global callback function to be called on window create
LRESULT CALLBACK OpenGLGeneralWindowProc(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam)
{
    OpenGLWin32App  *papp;  
    
    // The first message to ever come in sets the long value to class pointer
    if (iMsg==WM_NCCREATE)
    {
        papp = (OpenGLWin32App*) ((LPCREATESTRUCT)lParam)->lpCreateParams;

        if (!papp)
            return DefWindowProcW(hwnd,iMsg,wParam,lParam);
#ifdef GFC_64BIT_POINTERS
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)papp);
#else
        SetWindowLong(hwnd, 0, (LONG)(size_t)papp);
#endif
        papp->hWND = hwnd;
    }
    
    // use size_t to quiet /Wp64 warning
    if ((papp=((OpenGLWin32App*)(size_t)GetWindowLongPtr(hwnd,0)))==0)
        return DefWindowProcW(hwnd,iMsg,wParam,lParam);

    // Call member
    return papp->MemberWndProc(iMsg, wParam, lParam);
}



// Create and show the window
bool OpenGLWin32App::SetupWindow(const char *pname, SInt width, SInt height)
{
    if (Created)
        return 0;
    
    hInstance = GetModuleHandle(NULL);
    Width     = width;
    Height    = height;

    // Initialize the window class structure
    WNDCLASSEXW  wc; 
    
    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc    = OpenGLGeneralWindowProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(OpenGLWin32App*);    // will need to store class pointer here
    wc.hInstance      = hInstance;
    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);    // default icon
    wc.hIconSm        = LoadIcon(NULL, IDI_WINLOGO);        // windows logo small icon
    // set hCursor to NULL; otherwise cursor will be reverted back each time mouse moved.
    wc.hCursor        = NULL; //LoadCursor(NULL, IDC_ARROW);        // default arrow
    wc.hbrBackground  = NULL;     // no background needed
    wc.lpszMenuName   = NULL;     // no menu
    wc.lpszClassName  = WWND_CLASS_NAME; 
    hCursor = LoadCursor(NULL, IDC_ARROW);
    
    // Register the windows class
    if (!RegisterClassExW(&wc))
    {
        MessageBox(NULL,"Unable to register the window class", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);  
        Width = Height = 0;
        return 0;
    }   
    
    DWORD dwExStyle;
    DWORD dwStyle;
    int   xpos = 0, ypos = 0;
    
    // Set the window style depending on FullScreen state
    if (FullScreen)
    {
        dwExStyle   = WS_EX_APPWINDOW;
        dwStyle     = WS_POPUP;   // FullScreen gets no borders or title bar
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

        if (SizableWindow)
            dwStyle = WS_OVERLAPPEDWINDOW;
        else        
        {
            // need WS_CAPTION for GL offsetting to work correctly.
            dwStyle = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
        }

        xpos = ypos = CW_USEDEFAULT;
    }

    if (CursorDisabled)
        ShowCursorInstantly(false);
    
    // Initalize the rendering window to have width & height match the client area  
    RECT  windowRect;
    windowRect.left     = 0;
    windowRect.right    = (LONG) Width;
    windowRect.top      = 0;
    windowRect.bottom   = (LONG) Height;    
    // Account for borders and other style options
    AdjustWindowRectEx(&windowRect, dwStyle, 0, dwExStyle);

    wchar_t wpname[256];
    if (::MultiByteToWideChar(CP_ACP, 0, pname, (int)gfc_strlen(pname) + 1, wpname, sizeof(wpname)) == 0)
        wpname[0] = 0;

    // Create our window
    hWND = CreateWindowExW(
            dwExStyle,
            WWND_CLASS_NAME,
            wpname,             // Window name
            dwStyle |           // Window style required for OpenGL
            WS_CLIPCHILDREN |
            WS_CLIPSIBLINGS,
            xpos, ypos,
            windowRect.right - windowRect.left, // width
            windowRect.bottom - windowRect.top, // height
            NULL,               // hParent
            NULL,               // hMenu
            hInstance,          // Application instance
            (LPVOID) this );    // Our pointer as an extra parameter
        
    if (!hWND)
    {
        MessageBox(NULL, "Unable to create window", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    // Check the client rect was changed by Windows (if, for example, too big window
    // was created).
    RECT clientRect;
    ::GetClientRect(hWND, &clientRect);
    if (clientRect.right - clientRect.left != width ||
        clientRect.bottom - clientRect.top != height)
    {
        // Calc new Width & Height
        Width = clientRect.right - clientRect.left;
        Height = clientRect.bottom - clientRect.top;
    }

    if (SupportDropFiles)       
        ::DragAcceptFiles(hWND, 1); 

    
    // If we're in fullscreen mode, set up the display
    if (FullScreen)
    {
        // Set up the device mode structure
        DEVMODE screenSettings;
        memset(&screenSettings,0,sizeof(screenSettings));
        
        screenSettings.dmSize       = sizeof(screenSettings);
        screenSettings.dmPelsWidth  = Width;
        screenSettings.dmPelsHeight = Height;
        screenSettings.dmBitsPerPel = APP_COLOR_BITS;    // bits per pixel
        screenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        
        // Attempt to switch to the resolution and bit depth we've selected
        if (ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            // If we can't get go fullscreen, the user can choose a windowed mode
            if (MessageBox(NULL,
                    "Cannot switch to fullscreen mode at specified resolution.\n"
                    "Would you like to try windowed mode instead?",
                    "Resolution Change Error",
                MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
            {
                FullScreen = 0;
            }
            else
            {
                DestroyWindow(hWND);
                hWND = NULL;
                return 0;
            }
        }
    }
    
    // get a device context
    if ((hDC = GetDC(hWND))==0)
    {
        DestroyWindow(hWND);
        hWND = NULL;
        MessageBox(NULL,"Unable to create device context", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);      
        return 0;
    }
    
    if (!SetupPixelFormat())
    {
        DestroyWindow(hWND);
        hWND = NULL;
        return 0;
    }
        
    
    // Create OpenGL rendering context
    if ((hGLRC = wglCreateContext(hDC))==0)
    {
        DestroyWindow(hWND);
        hWND = NULL;
        MessageBox(NULL, "Unable to create OpenGL rendering context", "OpenGLApp Error",MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    // Make the rendering context the active one
    if (!wglMakeCurrent(hDC, hGLRC))
    {       
        MessageBox(NULL,"Unable to activate OpenGL rendering context", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    // show the window in the foreground, and set the keyboard focus to it
    ShowWindow(hWND, SW_SHOW);
    SetForegroundWindow(hWND);
    SetFocus(hWND);


    // Create a font (must be fixed width).
    // Use Courier New by default.
    HFONT hfont = CreateFont(18,0, 0, 0, 800, 0,0,0,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                        FIXED_PITCH | FF_DONTCARE, "Courier New");
         
    // Make the system font the device context's selected font.
    SelectObject (hDC, hfont ? hfont : GetStockObject (SYSTEM_FIXED_FONT));
    // Get metrics to measure text sizes later on.
    GetTextMetrics(hDC, &TextMetric);
 
    // Create the bitmap display lists.
    // We're making images of glyphs 0 through TextDisplayListCharCount.
    // The display list numbering starts at TextDisplayListBase.
    TextListsCreated = (wglUseFontBitmaps (hDC, 0, TextDisplayListCharCount, TextDisplayListBase) != 0);

    // Release font used for lists.
    SelectObject (hDC, GetStockObject (SYSTEM_FONT));
    if (hfont)
        ::DeleteObject(hfont);

    // set up the perspective for the current screen size
//  ResizeScene(g_screenWidth, g_screenHeight);
    Created = 1;

    if (!InitRenderer())
    {       
        KillWindow();
        return 0;
    }       

    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    p_glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) wglGetProcAddress ("glBindRenderbufferEXT");
    p_glDeleteRenderBuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) wglGetProcAddress ("glDeleteRenderBuffersEXT");
    p_glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) wglGetProcAddress ("glGenRenderbuffersEXT");
    p_glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress ("glBindFramebufferEXT");
    p_glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glDeleteFramebuffersEXT");
    p_glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glGenFramebuffersEXT");
    p_glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) wglGetProcAddress ("glFramebufferTexture2DEXT");
    p_glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) wglGetProcAddress ("glFramebufferRenderbufferEXT");
    p_glFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) wglGetProcAddress ("glFramebufferAttachmentParameterivEXT");
    p_glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) wglGetProcAddress("glRenderbufferStorageEXT");
    p_glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) wglGetProcAddress("glCheckFramebufferStatusEXT");
    return 1;
}

// Resets the direct3D, return 1 if successful.
// On successful reset, this function will call InitRenderer again.
bool    OpenGLWin32App::RecreateRenderer()
{
    // Nothing to do on OpenGL
    return InitRenderer();
}

    

// Sets up the pixel format according to the settings
bool OpenGLWin32App::SetupPixelFormat()
{   
    PIXELFORMATDESCRIPTOR pfd;  
    memset (&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    
    pfd.nSize =         sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion =      1;
    pfd.dwFlags =       PFD_DRAW_TO_WINDOW |  // Window drawing support
                        PFD_SUPPORT_OPENGL |  // OpenGL support
                        PFD_DOUBLEBUFFER;
    pfd.iPixelType =    PFD_TYPE_RGBA;
    pfd.cColorBits =    APP_COLOR_BITS;
    pfd.cDepthBits =    APP_DEPTH_BITS;
    pfd.cStencilBits =  APP_STENCIL_BITS;
    pfd.cAccumBits =    APP_ACCUM_BITS;
    pfd.iLayerType =    PFD_MAIN_PLANE;
    
    GLuint  pixelFormat;
    
    // Find the closest available pixel format
    if ((pixelFormat = ChoosePixelFormat(hDC, &pfd))==0)
    {
        MessageBox(NULL, "Can't choose an appropriate pixel format", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    // Set pixel format to device context
    if(!SetPixelFormat(hDC, pixelFormat, &pfd))
    {
        MessageBox(NULL, "Unable to set pixel format", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    return 1;
}
        

// Deletes the DC, RC, and Window, and restores the original display.
void OpenGLWin32App::KillWindow()
{
    if (!Created)
        return;

    // Delete text display lists.
    if (TextListsCreated)
    {
        glDeleteLists(TextDisplayListBase, TextDisplayListCharCount);
        TextListsCreated = 0;
    }

    // restore the original display if we're in fullscreen mode
    if (FullScreen)
    {
        ChangeDisplaySettings(NULL, 0);
        ShowCursor(1);
    }
    
    // If we have an RC, release & delete the RC
    if (hGLRC)
    {       
        wglMakeCurrent(NULL,NULL);                      
        wglDeleteContext(hGLRC);
        hGLRC = NULL;
    }   
    // Release the DC if we have one
    if (hDC)
    {
        ReleaseDC(hWND, hDC);       
        hDC = NULL;
    }
    // Destroy the window
    if (hWND)       
    {
        DestroyWindow(hWND);
        hWND = NULL;
    }
    
    // Unregister our class to make name reusable
    UnregisterClassW(WWND_CLASS_NAME, hInstance);
    hInstance = NULL;

    Created = 0;
    return;
}


// Message processing function to be called in the 
// application loops until this returns 0.
bool    OpenGLWin32App::ProcessMessages()
{
    MSG msg;

    if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            // On WM_QUIT message, quit the application by setting Quit flag
            ExitCode = (SInt)msg.wParam;
            QuitFlag = 1;           
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return !QuitFlag;       
}


// Sleeps for the specified number of milliseconds or till message.
void   OpenGLWin32App::SleepTillMessage(UInt32 ms)
{
    ::MsgWaitForMultipleObjects(0,0,0, ms, QS_ALLEVENTS);
}

// Changes/sets window title
void    OpenGLWin32App::SetWindowTitle(const char *ptitle)
{
    wchar_t wptitle[256];
    if (::MultiByteToWideChar(CP_ACP, 0, ptitle, (int)gfc_strlen(ptitle) + 1, wptitle, sizeof(wptitle)) == 0)
        wptitle[0] = 0;
    ::SetWindowTextW(hWND, wptitle);
}

// Presents the data (swaps the back and front buffers)
void    OpenGLWin32App::PresentFrame()
{
    ::SwapBuffers(hDC);
}


static void Color32ToFloat(float *prgba, UInt32 color)
{
    float scalar = 1.0f / 255.0f;
    prgba[3] =  (color >> 24) * scalar;
    prgba[0] =  ((color >> 16) & 0xFF) * scalar;
    prgba[1] =  ((color >> 8) & 0xFF) * scalar;
    prgba[2] =  (color & 0xFF) * scalar;
}


void    OpenGLWin32App::Clear(UInt32 color)
{   
    float   rgba[4];
    GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    if (scissorEnabled)
        glDisable(GL_SCISSOR_TEST);
    Color32ToFloat(rgba, color);
    glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    if (scissorEnabled)
        glEnable(GL_SCISSOR_TEST);
}

// Initialize and restore 2D rendering view.
void    OpenGLWin32App::Push2DRenderView()
{   
    glColorMask(1,1,1,1);   // enable framebuffer writes
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
    glDisableClientState(GL_VERTEX_ARRAY);

    // Save viewport.
    glGetIntegerv(GL_VIEWPORT, ViewportSave);
    glViewport(0,0, Width, Height);

    // Configure 1-to-1 coord->window transformation.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, Width,  Height, 0, -100000.0, 100000.0); 
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void    OpenGLWin32App::Pop2DRenderView()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    // Restore viewport.
    glViewport(ViewportSave[0], ViewportSave[1], ViewportSave[2], ViewportSave[3]);
    glPushAttrib(GL_ALL_ATTRIB_BITS);
}

// Draw a filled + blended rectangle.
void    OpenGLWin32App::FillRect(SInt l, SInt t, SInt r, SInt b, UInt32 color)
{
    float   rgba[4];
    Color32ToFloat(rgba, color);
    glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glRecti(l,t, r, b);
}


// Draw a text string (specify top-left corner of characters, NOT baseline)
void    OpenGLWin32App::DrawText(SInt x, SInt y, const char *ptext, UInt32 color)
{
    if (!ptext || ptext[0] == 0)
        return;

    // Set color
    float   rgba[4];
    Color32ToFloat(rgba, color);
    glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);

    // Set display list base.
    glListBase(TextDisplayListBase);        
    
    // Adjust Y, so that text starts at Top and not baseline (default for wglText lists).
    y += TextMetric.tmAscent;

    // Display text a line at a time.
    SInt    i, lineStart;
    
    for (i=0, lineStart = 0; ptext[i]!=0; i++)
    {
        if (ptext[i] == '\n')
        {
            glRasterPos2i(x, y);
            glCallLists(i - lineStart, GL_UNSIGNED_BYTE, ptext + lineStart);
            y += TextMetric.tmHeight + TextMetric.tmExternalLeading;
            lineStart = i+1;
        }
    }
    // And the last line.
    glRasterPos2i(x, y);
    glCallLists(i - lineStart, GL_UNSIGNED_BYTE, ptext + lineStart);

}


// Compute text size that will be generated by DrawText
void    OpenGLWin32App::CalcDrawTextSize(SInt *pw, SInt *ph, const char *ptext)
{
    // Use fixed height.
    if (!ptext || ptext[0] == 0)
    {   
        *pw = *ph = 0;
        return;
    }

    SInt    lines            = 0;
    SInt    maxLineCharCount = 0;
    SInt    lineCharCount    = 0;
    SInt    i;

    // Count lines and newlines
    for (i=0; ptext[i]!=0; i++, lineCharCount++)
    {
        if (ptext[i] =='\n')
        {
            maxLineCharCount = (maxLineCharCount > lineCharCount) ? maxLineCharCount : lineCharCount;
            lines++;
            lineCharCount = -1; // don't count '\n' as a character.
            continue;
        }
    }

    maxLineCharCount = (maxLineCharCount > lineCharCount) ? maxLineCharCount : lineCharCount;
    // At least one line.
    lines++;

    // Return size, only correct for fixed-width fonts.
    *pw = maxLineCharCount * TextMetric.tmAveCharWidth;
    *ph = lines * TextMetric.tmHeight + (lines-1) * TextMetric.tmExternalLeading;
}


// API-independednt toggle for wireframe rendering.
void    OpenGLWin32App::SetWireframe(bool wireframeEnable)
{
    if (wireframeEnable)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  
}


// *** Overrides

// This override is called to initialize OpenGL from setup window
bool    OpenGLWin32App::InitRenderer()
{
    return 1;
}
// Should/can be called every frame to prepare the render, user function
void    OpenGLWin32App::PrepareRendererForFrame()
{   
}
    
// Message processing overrides
void    OpenGLWin32App::OnKey(UInt key, UInt info, bool downFlag)
{
    GUNUSED3(key,info,downFlag);
}
void    OpenGLWin32App::OnChar(UInt32 wcharCode, UInt info)
{
    GUNUSED2(wcharCode,info);
}
void    OpenGLWin32App::OnMouseButton(UInt button, bool downFlag, SInt x, SInt y)
{
    GUNUSED2(button,downFlag);
    GUNUSED2(x,y);
}
void    OpenGLWin32App::OnMouseWheel(SInt zdelta, SInt x, SInt y)
{
    GUNUSED3(zdelta,x,y);
}
void    OpenGLWin32App::OnMouseMove(SInt x, SInt y)
{
    GUNUSED2(x,y);
}


// Sizing; by default, re-initalizes the renderer
void    OpenGLWin32App::OnSize(SInt w, SInt h)
{
    Width  = w;
    Height = h;

    /*

    // Set the viewport to the new dimensions
    glViewport(0, 0, width, height);
    
    // select the projection matrix and clear it out
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // set the perspective with the appropriate aspect ratio
    gluPerspective(APP_FOV, (GLfloat)width/(GLfloat) ((height != 0) ? height : 1), 0.1f, 1000.0f);
    
    // select modelview matrix
    glMatrixMode(GL_MODELVIEW);

    */
}

// Called when sizing begins and ends.
void    OpenGLWin32App::OnSizeEnter(bool enterSize)
{
    GUNUSED(enterSize);
}

// Handle dropped files
void    OpenGLWin32App::OnDropFiles(char *path)
{
    GUNUSED(path);
}

void    OpenGLWin32App::SetCursor(HCURSOR cursor)
{
    hCursor = cursor;
    ::SetCursor(hCursor);
}

void OpenGLWin32App::ResetCursor()
{
    CursorIsOutOfClientArea = 0;
    CursorHidden = 0;
    CursorHiddenSaved = 0;

    if (!CursorDisabled)
    {
        ShowCursorInstantly(true);
        SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    else
        ShowCursorInstantly(false);
}

void OpenGLWin32App::ShowCursorInstantly(bool show)
{
    if (show)
    {
        while(::ShowCursor(TRUE) < 0)
            ;
    }
    else
    {
        while(::ShowCursor(FALSE) >= 0)
            ;
    }
}

void OpenGLWin32App::ShowCursor(bool show)
{
    if (CursorDisabled) 
        return;
    if (show)
    {
        if (CursorHidden)
        {
            CursorHidden = !CursorHidden;
            if (!CursorIsOutOfClientArea) 
                ShowCursorInstantly(true);
        }
    }
    else
    {
        if (!CursorHidden)
        {
            CursorHidden = !CursorHidden;
            if (!CursorIsOutOfClientArea) 
                ShowCursorInstantly(false);
        }
    }
}

// *** Window procedure

// Mousewheel support
#include    <winuser.h>
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120
#endif

// MouseLeave support
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE                   0x02A3
#endif

// XButton WM_ message support
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_XBUTTONDBLCLK                0x020D
#define WM_NCXBUTTONDOWN                0x00AB
#define WM_NCXBUTTONUP                  0x00AC
#define WM_NCXBUTTONDBLCLK              0x00AD
// XButton values are WORD flags
#define GET_XBUTTON_WPARAM(wp)          (HIWORD(wp))
#define XBUTTON1                        0x0001
#define XBUTTON2                        0x0002
#endif
// XButton VK_ codes
#ifndef VK_XBUTTON1
#define VK_XBUTTON1                     0x05
#define VK_XBUTTON2                     0x06
#endif

LRESULT OpenGLWin32App::MemberWndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_CREATE:
            break;
            
        case WM_ACTIVATE:           
            if (!HIWORD(wParam))                
                Active = 1; // Window was restored or maximized
            else                                
                Active = 0; // Window was minimized
            return 0;
            
        case WM_SYSCOMMAND:         
            // Look for screen-saver and power-save modes
            switch (wParam)
            {
                case SC_SCREENSAVE:     // Screen-saver is starting
                case SC_MONITORPOWER:   // Monitor is going to power-save mode
                    // Prevent either from happening by returning 0
                    return 0;
                default:
                    break;
            }
            break;

        case WM_DROPFILES:
            {
                UInt itemCount = ::DragQueryFile((HDROP)wParam, 0xFFFFFFFF,0,0);
                if (itemCount)
                {
                    // Get name
                    char    buffer[512];
                    buffer[0] = 0;
                    ::DragQueryFile((HDROP)wParam, 0, buffer, 512);
                    ::DragFinish((HDROP)wParam);

                    // Inform user about the drop
                    OnDropFiles(buffer);
                }
            }
            return 0;
            
        case WM_CLOSE:
            // window is being closed
            // send WM_QUIT to message queue
            PostQuitMessage(0); 
            return 0;

        case WM_SIZE:
            OnSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_ENTERSIZEMOVE:  
            OnSizeEnter(1); 
            return 0;
        case WM_EXITSIZEMOVE:   
            OnSizeEnter(0); 
            return 0;

        case WM_KEYDOWN:        OnKey((UInt)wParam, (UInt)lParam, 1);   return 0;
        case WM_KEYUP:          OnKey((UInt)wParam, (UInt)lParam, 0);   return 0;
        case WM_CHAR:          
            {
                UInt32 wcharCode = (UInt32)wParam;
                OnChar(wcharCode, (UInt)lParam);
            }
            break;

        case WM_MOUSEMOVE:      OnMouseMove((SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam)); return 0;
        // Mouse wheel support
        case WM_MOUSEWHEEL:
            {
                // Nonclient position must be adjusted to be inside the window
                POINT   wcl = {0,0};
                ::ClientToScreen(hWND, &wcl);
                SInt x = SInt(SInt16(LOWORD(lParam))) - wcl.x;
                SInt y = SInt(SInt16(HIWORD(lParam))) - wcl.y;
                OnMouseWheel((SInt(SInt16(HIWORD(wParam)))*128)/WHEEL_DELTA, x, y);
                return 0;
            }
        case WM_SETCURSOR:
            if ((HWND)wParam == hWND)
            {
                if (CursorDisabled)
                    break;

                if(LOWORD(lParam) == HTCLIENT)
                {
                    if (CursorIsOutOfClientArea)
                    {
                        bool cursorWasHidden = CursorHiddenSaved;
                        CursorIsOutOfClientArea = false;
                        CursorHiddenSaved = false;
                        if (cursorWasHidden && CursorHidden) 
                            ShowCursorInstantly(false);
                    }
                    ::SetCursor(hCursor);
                    return 1;
                }
                else if (!CursorIsOutOfClientArea)
                {
                    CursorIsOutOfClientArea = true;
                    CursorHiddenSaved = CursorHidden;
                    if (CursorHidden) 
                        ShowCursorInstantly(true);
                }
            }
            break;
        // Mouse button support
        case WM_LBUTTONDOWN:    OnMouseButton(0, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_LBUTTONUP:      OnMouseButton(0, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_RBUTTONDOWN:    OnMouseButton(1, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_RBUTTONUP:      OnMouseButton(1, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_MBUTTONDOWN:    OnMouseButton(2, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        case WM_MBUTTONUP:      OnMouseButton(2, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));  return 0;
        // XButton support
        case WM_XBUTTONDOWN:    OnMouseButton(2+GET_XBUTTON_WPARAM(wParam), 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));   return 0;
        case WM_XBUTTONUP:      OnMouseButton(2+GET_XBUTTON_WPARAM(wParam), 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam));   return 0;
            
        /*
        case WM_CHAR:           
            switch (toupper(wParam))
            {
                case VK_ESCAPE:
                {                   
                    PostQuitMessage(0);
                    return 0;
                }
                default:
                    break;
            };
            break;
        */
            
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(hWND, &ps);
                EndPaint(hWND, &ps);
            }
            break;
            
        default:
            break;
    }
    
    return DefWindowProcW(hWND, message, wParam, lParam);
}


