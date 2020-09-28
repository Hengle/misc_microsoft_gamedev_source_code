/**********************************************************************

Filename    :   FxApp.cpp
Content     :   Base FxApp class implementation.
Created     :   Jan, 2008
Authors     :   Maxim Didenko, Dmitry Polenur

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "FxApp.h"
#include <stddef.h>


FxApp* FxApp::pApp = NULL;

FxApp::FxApp() 
{ 
    Created     = 0;
    Active      = 0;
    QuitFlag    = 0;
    LockOnSize  = 0;
    ExitCode    = 0;

    Console     = false;

    // Requested 3D state
    FullScreen  = 0;
    FSAntialias = 0;
    BitDepth    = 0;
    VSync       = 0;

    SupportDropFiles    = 0;
    SizableWindow = 0;

    Width       = 0;
    Height      = 0;

    // Set later on in CreateWindow
    FSAASupported= 0;

    VMCFlags = 0;

    // No old pos, save during FullScreen mode
    OldWindowX = OldWindowY = 0;
    OldWindowWidth = OldWindowHeight = 0;

    // Cursor variables
    CursorHidden  = 0;
    CursorHiddenSaved = 0;
    CursorIsOutOfClientArea = 0;
    CursorDisabled = 0;

    pApp = this; 
}

FxApp::~FxApp() 
{
}

void FxApp::ResetCursor()
{
    CursorIsOutOfClientArea = 0;
    CursorHidden = 0;
    CursorHiddenSaved = 0;

    if (!CursorDisabled)
    {
        ShowCursorInstantly(true);
//        SetCursor(::LoadCursor(NULL, IDC_ARROW));
    }
    else
        ShowCursorInstantly(false);
}

void FxApp::ShowCursor(bool show)
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
