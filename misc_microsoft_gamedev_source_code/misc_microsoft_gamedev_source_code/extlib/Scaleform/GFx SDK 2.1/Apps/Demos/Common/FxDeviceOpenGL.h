/**********************************************************************

Filename    :   FxDeviceOpengl.h
Content     :   OpennGL Device  
Created     :   Jan, 2008
Authors     :   
Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_FXDEVICEOGL_H
#define INC_FXDEVICEOGL_H

#include "FxDevice.h"
#ifdef GFC_OS_PS3
  #include <PSGL/psgl.h>
  #include <PSGL/psglu.h>
#elif defined(GFC_OS_MAC)
  #include <OpenGL/OpenGL.h>
  #include <OpenGL/gl.h>
#elif defined(GFC_OS_WIN32)
  #include <windows.h>
  #include <gl/gl.h>
  #include "glext.h"
  #define GFC_GL_RUNTIME_LINK(x) wglGetProcAddress(x)
#else                                          
  // Use GLX to link gl extensions at runtime; comment out if not using GLX
  #define GFC_GLX_RUNTIME_LINK

  #ifdef GFC_GLX_RUNTIME_LINK
    #include <GL/glx.h>
    #include <GL/glxext.h>
    #define GFC_GL_RUNTIME_LINK(x) glXGetProcAddressARB((const GLubyte*) (x))
  #else
  #define GL_GLEXT_PROTOTYPES
  #endif

  #include <GL/gl.h>
  #include <GL/glext.h>
#endif

class FxDeviceOpenGL : public FxDevice
{
 public:
    FxDeviceOpenGL(FxApp* papp);
    virtual    ~FxDeviceOpenGL() {}
    
    GLint       ViewportSave[4];
   
    virtual void    Clear(unsigned int color);
    virtual void    Push2DRenderView();
    virtual void    Pop2DRenderView();
    virtual void    FillRect(int l, int t, int r, int b, unsigned int color);

    virtual void    SetWireframe(bool wireframeEnable);

    virtual bool    InitRenderer();
    virtual void    PrepareRendererForFrame();

};

#endif //INC_FXDEVICEOGL_H
