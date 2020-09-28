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
#include "FxDeviceOpengl.h"

// Video mode
#define  APP_COLOR_BITS         32
#define  APP_DEPTH_BITS         24
#define  APP_STENCIL_BITS       8
#define  APP_ACCUM_BITS         0

//#define  WWND_CLASS_NAME         L"OGL_Window_Class"

OpenGLWin32App::OpenGLWin32App()
{
    pFxDevice = new FxDeviceOpenGL(this); 
    hDC         = 0;
    hGLRC       = 0;
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
}


bool OpenGLWin32App::CreateRenderer()
{
    pRenderer = *GRendererOGL::CreateRenderer();
    if (pRenderer)
        pRenderer->SetDependentVideoMode();
    return pRenderer.GetPtr() != NULL;
}

// Create and show the window
bool OpenGLWin32App::SetupWindowDevice()
{
    // If we're in fullscreen mode, set up the display
    if (FullScreen)
    {
        // Set up the device mode structure
        DEVMODE screenSettings;
        memset(&screenSettings,0,sizeof(screenSettings));
        
        screenSettings.dmSize       = sizeof(screenSettings);
        screenSettings.dmPelsWidth  = GetWidth();
        screenSettings.dmPelsHeight = GetHeight();
        screenSettings.dmBitsPerPel = APP_COLOR_BITS;    // bits per pixel
        screenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        
        // Attempt to switch to the resolution and bit depth we've selected
        if (ChangeDisplaySettings(&screenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            // If we can't get go fullscreen, the user can choose a windowed mode
            if (MessageBoxA(NULL,
                    "Cannot switch to fullscreen mode at specified resolution.\n"
                    "Would you like to try windowed mode instead?",
                    "Resolution Change Error",
                MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
            {
                FullScreen = 0;
            }
            else
                return false;
        }
    }
    
    // get a device context
    if ((hDC = GetDC(hWND))==0)
    {
        MessageBoxA(NULL,"Unable to create device context", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);      
        return false;
    }
    
    if (!SetupPixelFormat())
        return false;
        
    
    // Create OpenGL rendering context
    if ((hGLRC = wglCreateContext(hDC))==0)
    {
        MessageBoxA(NULL, "Unable to create OpenGL rendering context", "OpenGLApp Error",MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    // Make the rendering context the active one
    if (!wglMakeCurrent(hDC, hGLRC))
    {       
        MessageBoxA(NULL,"Unable to activate OpenGL rendering context", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    // Create a font (must be fixed width).
    // Use Courier New by default.
    HFONT hfont = CreateFontA(18,0, 0, 0, 800, 0,0,0,
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

    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    SetVSync(VSync);

    p_glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) wglGetProcAddress ("glBindRenderbufferEXT");
    p_glDeleteRenderBuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) wglGetProcAddress ("glDeleteRenderbuffersEXT");
    p_glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) wglGetProcAddress ("glGenRenderbuffersEXT");
    p_glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress ("glBindFramebufferEXT");
    p_glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glDeleteFramebuffersEXT");
    p_glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glGenFramebuffersEXT");
    p_glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) wglGetProcAddress ("glFramebufferTexture2DEXT");
    p_glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) wglGetProcAddress ("glFramebufferRenderbufferEXT");
    p_glFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) wglGetProcAddress ("glFramebufferAttachmentParameterivEXT");
    p_glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) wglGetProcAddress("glRenderbufferStorageEXT");
    p_glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) wglGetProcAddress("glCheckFramebufferStatusEXT");
    p_glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) wglGetProcAddress("glGenerateMipmapEXT");
    return true;
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
        MessageBoxA(NULL, "Can't choose an appropriate pixel format", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    // Set pixel format to device context
    if(!SetPixelFormat(hDC, pixelFormat, &pfd))
    {
        MessageBoxA(NULL, "Unable to set pixel format", "OpenGLApp Error", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }
    
    return 1;
}
        

// Deletes the DC, RC, and Window, and restores the original display.
void OpenGLWin32App::KillWindowDevice()
{
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
}


// Presents the data (swaps the back and front buffers)
void    OpenGLWin32App::PresentFrame()
{
    ::SwapBuffers(hDC);
}

// Draw a text string (specify top-left corner of characters, NOT baseline)
void    OpenGLWin32App::DrawText(int x, int y, const char *ptext, unsigned int color)
{
    if (!ptext || ptext[0] == 0)
        return;

    // Set color
    float   rgba[4];
    FxDeviceOpenGL::Color32ToFloat(rgba, color);
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
void    OpenGLWin32App::CalcDrawTextSize(int *pw, int *ph, const char *ptext)
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

// Sizing; by default, re-initalizes the renderer
void    OpenGLWin32App::ResizeWindow(int w, int h)
{
    if (pRenderer)
        pRenderer->ResetVideoMode();
    SetWidth(w);
    SetHeight(h);

    if (pRenderer)
        pRenderer->SetDependentVideoMode();

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

GRenderer* OpenGLWin32App::GetRenderer()
{
    return pRenderer.GetPtr();   
}


void OpenGLWin32App::SwitchFSAA(bool on_off)
{
    if (pRenderer && FSAASupported && FSAntialias != on_off)
    {
        FSAntialias = on_off;
        pRenderer->ResetVideoMode();
        pRenderer->SetDependentVideoMode();
    }
}

void OpenGLWin32App::SetVSync(bool isEnabled)
{
    PFNWGLSWAPINTERVALEXTPROC wglSwapInterval = NULL;
    wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress("wglSwapIntervalEXT");
    if ( wglSwapInterval ) wglSwapInterval(isEnabled ? 1 : 0);
    VSync = isEnabled;
}

FxApp::DisplayStatus  OpenGLWin32App::CheckDisplayStatus() const
{
    if (!pRenderer)
        return FxApp::DisplayStatus_Unavailable;
    return (FxApp::DisplayStatus)pRenderer->CheckDisplayStatus();
}
