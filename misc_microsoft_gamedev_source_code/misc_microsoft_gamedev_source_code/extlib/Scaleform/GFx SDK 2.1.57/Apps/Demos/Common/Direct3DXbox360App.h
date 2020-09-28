/**********************************************************************

Filename    :   Direct3DXbox360App.h
Content     :   Simple Xbox 360 Application class
Created     :   
Authors     :   Michael Antonov
Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_D3DXBOX360APP_H
#define INC_D3DXBOX360APP_H

// ***** XBOX Includes

// XBox title libraries - for input.
#include <xtl.h>

// GFx Includes
#include "FxApp.h"
#include "GTypes.h"

#include <d3d9.h>
#include <d3dx9.h>
#include "GRendererXbox360.h"

class FxXboxIMEEvent : public FxAppIMEEvent
{
};

// ***** Application class
class FxDeviceDirect3D;
class Direct3DXboxApp: public FxApp
{
public:

    typedef GRendererXbox360  renderer_type;
    typedef IDirect3DDevice9  device_type;
    typedef D3DVIEWPORT9      viewport_type;

    // **** Public Interface

    Direct3DXboxApp();
    ~Direct3DXboxApp();

    virtual bool CreateRenderer();

    
    // Creates a window & initializes the application class
    // Returns 0 if window creation/initialization failed (the app should quit).
    virtual bool    SetupWindow(const char *pname, int width, int height,
                                const SetupWindowParams& extraParams = SetupWindowParams());

    // This call cleans up the application and kills the window.
    // If not called explicitly, it will be called from the destructor.
    virtual void    KillWindow();

    // Message processing function to be called in the 
    // application loops until it returns 0, to indicate application being closed.
    virtual bool    ProcessMessages();

    // Changes/sets window title
    virtual void    SetWindowTitle(const char *ptitle);

    // Presents the data (swaps the back and front buffers)
     virtual void    PresentFrame();

    // Resets the direct3D, return 1 if successful.
    // On successful reset, this function will call InitRenderer again.
    virtual bool     RecreateRenderer();
    
    virtual void     SleepMilliSecs(unsigned int ms);

    // *** Overrides

    // This override is called from SetupWindow to initialize OpenGL/Direct3D view
    virtual bool     InitRenderer();
    // Should/can be called every frame to prepare the render, user function
    virtual void     PrepareRendererForFrame();


    // Clears the entire back buffer.
    virtual void     Clear(unsigned int color);

    // Initialize and restore 2D rendering view.
    // Push2DRenderView must be called before using 2D functions below,
    // while Pop2DRenderView MUST be called after it is finished.
    virtual void     Push2DRenderView();
    virtual void     Pop2DRenderView();

    // Draw a filled + blended rectangle.
    virtual void     FillRect(int l, int t, int r, int b, unsigned int color);
    // Draw a text string (specify top-left corner of characters, NOT baseline)

    // API-independent toggle for wireframe rendering.
    virtual void     SetWireframe(bool wireframeEnable);
    virtual void    SwitchFSAA(bool on_off);

    virtual void    RedrawMouseCursor();
    virtual void    ShowCursorInstantly(bool show);

    // Sizing; by default, re-initalizes the renderer
    virtual void    OnSize(int w, int h) {}
    // Called when sizing begins and ends.
    virtual void    OnSizeEnter(bool enterSize) {}
    // Handle dropped file
    virtual void    OnDropFiles(char *path) {}

    virtual bool UpdateFiles(char* filename, bool prev);

    virtual GRenderer* GetRenderer();

    virtual DisplayStatus  CheckDisplayStatus() const;

    FxDeviceDirect3D*      pFxDevice;
       // *** Implementation


    bool            HideCursor;

    // Requested 3D state
    bool            PredicatedTiling;

    LPDIRECT3D9             pD3D;
    IDirect3DDevice9*       pDevice;
    D3DPRESENT_PARAMETERS   PresentParams;
    IDirect3DTexture9*      pFrameBuffer[2];
    unsigned int            CurBuffer;
    IDirect3DTexture9*      pBackBuffer;
    unsigned int            TileWidth, TileHeight, NumTiles;
    D3DRECT                 Tiles[8];
    IDirect3DSurface9*      pTilingRT;
    IDirect3DSurface9*      pTilingDepth;

    // Client size

    HWND                    hWND;

    // Input state and pad, if any.
    XINPUT_STATE            InputState; 

    // Raw shaders
    IDirect3DVertexShader9* pVShaderCoordCopy;
    IDirect3DPixelShader9*  pPShaderConst;          

    // Viewport un-scale values: first two rows in matrix
    float                   ViewportFactorX[4];
    float                   ViewportFactorY[4];

    // mouse position
    Float                   MouseX, MouseY;
    Float                   MouseXadj, MouseYadj;

    GPtr<GRendererXbox360>  pRenderer;
    void SetRendererDependentVideoMode();
    void CheckTilingSetup(bool forceTiling = 0);
    void SetupTilingBuffers();
};


#endif // INC_D3DXBOX360APP_H
