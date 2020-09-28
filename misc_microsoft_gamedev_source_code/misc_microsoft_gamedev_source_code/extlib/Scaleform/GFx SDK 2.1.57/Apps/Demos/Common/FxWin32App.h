/**********************************************************************

Filename    :   FxWin32App.h
Content     :   Shared base Win32 implementation for FxApp class.
Created     :   Jan, 2008
Authors     :   Michael Antonov, Maxim Didenko, Dmitry Polenur

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FxWin32App_H
#define INC_FxWin32App_H

#include <windows.h>
#include "FxApp.h"
#include "GStd.h"
#include "FxDevice.h"


struct FxWin32IMEEvent : public FxAppIMEEvent
{
    FxWin32IMEEvent(bool preprocess = false) : Preprocess(preprocess) {}
    UINT        message;
    WPARAM      wParam;
    LPARAM      lParam;
    HWND        hWND;
    bool        Preprocess;
};


class FxWin32App : public FxApp 
{
public:
    FxWin32App();
    virtual ~FxWin32App();
    // Creates a window & initializes the application class
    // Returns 0 if window creation/initialization failed (the app should quit).
    virtual bool    SetupWindow(const char *pname, int width, int height,
                                const SetupWindowParams& extra_pars = SetupWindowParams());

    // This call cleans up the application and kills the window.
    // If not called explicitly, it will be called from the destructor.
    virtual void    KillWindow();

    // Message extraParams function to be called in the 
    // application loops until it returns 0, to indicate application being closed.
    virtual bool    ProcessMessages();

    // Sleeps for the specified number of milliseconds or till message.
    virtual void    SleepTillMessage(unsigned int ms);

    virtual void    SleepMilliSecs(unsigned int ms);

    // Changes/sets window title
    virtual void    SetWindowTitle(const char *ptitle);

    virtual void    OnCreate();
    virtual void    OnDestroy();
   
    // Set cursor
    void            SetCursor(HCURSOR cursor);

    // *** Overrides
    virtual void    ShowCursorInstantly(bool show);

    // This override is called from SetupWindow to initialize OpenGL/Direct3D view
    virtual bool    InitRenderer();
    // Should/can be called every frame to prepare the render, user function
    virtual void    PrepareRendererForFrame();

    virtual bool    UpdateFiles(char* filename, bool prev);

    virtual void    BringMainWindowOnTop();

    // Clears the entire back buffer.
    virtual void    Clear(unsigned int color) { pFxDevice->Clear(color);  }
    // Initialize and restore 2D rendering view.
    // Push2DRenderView must be called before using 2D functions below,
    // while Pop2DRenderView MUST be called after it is finished.
    virtual void    Push2DRenderView()      { pFxDevice->Push2DRenderView(); }
    virtual void    Pop2DRenderView()       { pFxDevice->Pop2DRenderView(); }

    // Draw a filled + blended rectangle.
    virtual void    FillRect(int l, int t, int r, int b, unsigned int color)  { pFxDevice->FillRect(l, t, r, b, color); }

    // API-independent toggle for wireframe rendering.
    virtual void    SetWireframe(bool wireframeEnable)  { pFxDevice->SetWireframe(wireframeEnable);  }
    
    virtual void    StartMouseCapture();
    virtual void    EndMouseCapture();

    // OnChar event specific for Win32
    virtual void    OnChar(UInt32 wcharCode, UInt info) { GUNUSED2(wcharCode,info); }

    // Member window procedure, used so that it can access locals
    virtual LRESULT MemberWndProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void    OnDrawClipboard(WPARAM wParam, LPARAM lParam);
    virtual void    OnChangeCBChain(WPARAM wParam, LPARAM lParam);

    // Called from a SetupWindow to initialize specific graphic device 
    virtual bool    SetupWindowDevice() { return true; }
    // Called from a KillWindow to shutdown specific graphic device 
    virtual void    KillWindowDevice() {}


    HWND            hWND;
    HINSTANCE       hInstance;
    HCURSOR         hCursor;

    // Clipboard related
    HWND            hWNDNextViewer;

    FxDevice *      pFxDevice; 
};

#endif //INC_FXWIN32APP_H
