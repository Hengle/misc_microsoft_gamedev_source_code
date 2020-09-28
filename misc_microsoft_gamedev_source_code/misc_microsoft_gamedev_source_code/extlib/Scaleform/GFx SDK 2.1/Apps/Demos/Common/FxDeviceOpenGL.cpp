/**********************************************************************

Filename    :   FxDeviceOpengl.cpp
Content     :   OpennGL Device Implementation
Created     :   Jan, 2008
Authors     :   
Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "FxDeviceOpenGL.h"

// glOrtho parameter
#define     OVERSIZE            1.0f

FxDeviceOpenGL::FxDeviceOpenGL(FxApp* papp)
    :FxDevice(papp)
{ 
    Type=OpenGL;
}


void FxDeviceOpenGL::SetWireframe(bool wireframeEnable)
{
    if (wireframeEnable)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
}

void FxDeviceOpenGL::Clear(unsigned int color)
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
void FxDeviceOpenGL::Push2DRenderView()
{   
    glColorMask(1,1,1,1);   // enable framebuffer writes
    glDisable(GL_TEXTURE_2D);
    glPopAttrib();
    glDisableClientState(GL_VERTEX_ARRAY);

    // Save viewport.
    glGetIntegerv(GL_VIEWPORT, ViewportSave);
    glViewport(0,0, pApp->GetWidth(),pApp->GetHeight());

    // Configure 1-to-1 coord->window transformation.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, pApp->GetWidth(),  pApp->GetHeight(), 0, -100000.0, 100000.0); 
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void FxDeviceOpenGL::Pop2DRenderView()
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
void  FxDeviceOpenGL::FillRect(SInt l, SInt t, SInt r, SInt b, unsigned int color)
{
    float   rgba[4];
    Color32ToFloat(rgba, color);
    glColor4f(rgba[0], rgba[1], rgba[2], rgba[3]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glRecti(l,t, r, b);
}

bool FxDeviceOpenGL::InitRenderer()
{
    // Change the LOD BIAS values to tweak text blurriness.

    ///TODO we need some how to get a access to App settings
/*
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
*/
    // Turn on line smoothing.  Antialiased lines can be used to
    // smooth the outsides of shapes.
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST); // GL_NICEST, GL_FASTEST, GL_DONT_CARE

    // Setup matrices.
    glMatrixMode(GL_PROJECTION);
    glOrtho(-OVERSIZE, OVERSIZE, OVERSIZE, -OVERSIZE, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    return true;
}

void FxDeviceOpenGL::PrepareRendererForFrame()
{
    // Draw on back buffer.
    glDrawBuffer(GL_BACK);
}
