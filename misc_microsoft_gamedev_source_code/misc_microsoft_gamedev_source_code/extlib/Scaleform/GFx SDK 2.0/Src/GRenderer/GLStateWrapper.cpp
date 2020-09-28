/**********************************************************************

Filename    :   GLStateWrapper.cpp
Content     :   OpenGL wrapper for preserving states
Created     :   September 8, 2006
Authors     :   Andrew Reisse

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


void SPW_ApplyStates();

/* Call SPW_ApplyStates to set the supported GL states back to the values
   set by calls intercepted by the wrapper. Multiple contexts are supported.

   Typically, SPW_ApplyStates() is called after a call to GFxMovieRoot::Display().
*/


#include <PSGL/psgl.h>
#include <map>

#define MAX_TEXTURE_UNITS 16

using namespace std;

struct GLstate
{
    GLclampf  ClearColor[4];
    GLfloat   ClearDepth;
    GLint     ClearStencil;
    GLint     ScissorX, ScissorY;
    GLsizei   ScissorW, ScissorH;
    GLint     ViewportX, ViewportY;
    GLsizei   ViewportW, ViewportH;

    GLenum    LogicOp;
    GLenum    AlphaFunc;
    GLclampf  AlphaRef;
    GLboolean ColorMask[4];

    GLenum    DepthFunc;
    GLuint    DepthMask;
    GLfloat   DepthRange[2];

    GLenum    StencilFunc, StencilOp[3];
    GLint     StencilRef;
    GLuint    StencilMask;

    GLenum    FrontFace, CullFace;
    GLenum    PolyMode[2];
    GLfloat   LineWidth, PointSize;
    GLfloat   PolyOffset[2];

    GLenum    BlendEqnA, BlendEqnC, BlendFnAs, BlendFnCs, BlendFnAd, BlendFnCd;
    GLclampf  BlendColor[4];

    GLenum    ActiveTexture, ClientActiveTexture;

    struct TexState
    {
    GLenum   Binding2D;
    GLfloat  LodBias;
    GLenum   EnvFunc;
    GLclampf EnvColor[4];
    } Tex[MAX_TEXTURE_UNITS];

    struct CgProfileState
    {
    int enabled;
    CGprogram binding;

    CgProfileState() { enabled = 0; binding = 0; }
    };
    map<CGprofile,CgProfileState> CgProfiles;
    map<GLenum,int>               States;

    void init()
    {
    memset (this, 0, (char*)&CgProfiles - (char*)this);
    CgProfiles.clear();
    States.clear();

    ClearDepth = 1;
    LogicOp = GL_COPY;
    AlphaFunc = GL_ALWAYS;
    AlphaRef = 0;
    ColorMask[0] = GL_TRUE;
    ColorMask[1] = GL_TRUE;
    ColorMask[2] = GL_TRUE;
    ColorMask[3] = GL_TRUE;
    DepthFunc = GL_LESS;
    DepthMask = GL_TRUE;
    DepthRange[0] = 0;
    DepthRange[1] = 1;
    StencilFunc = GL_ALWAYS;
    StencilRef = 0;
    StencilMask = ~0;
    StencilOp[0] = GL_KEEP;
    StencilOp[1] = GL_KEEP;
    StencilOp[2] = GL_KEEP;

    FrontFace = GL_CCW;
    CullFace = GL_BACK;
    PolyMode[0] = PolyMode[1] = GL_FILL;
    PointSize = LineWidth = 1;
    BlendEqnA = BlendEqnC = GL_FUNC_ADD;
    BlendFnAs = BlendFnCs = GL_ONE;
    BlendFnAd = BlendFnCd = GL_ZERO;

    ActiveTexture = ClientActiveTexture = GL_TEXTURE0;

    for (int i = 0; i < MAX_TEXTURE_UNITS; i++)
    {
        Tex[i].EnvFunc = GL_MODULATE;
        Tex[i].LodBias = 0;
    }

    States.insert(pair<GLenum,int> (GL_BLEND, 0));
    States.insert(pair<GLenum,int> (GL_DEPTH_TEST, 0));
    States.insert(pair<GLenum,int> (GL_STENCIL_TEST, 0));
    States.insert(pair<GLenum,int> (GL_TEXTURE_2D, 0));
    States.insert(pair<GLenum,int> (GL_TEXTURE_GEN_S, 0));
    States.insert(pair<GLenum,int> (GL_TEXTURE_GEN_T, 0));
    }

    GLstate ()
    {
    init();
    }

    void apply () const;
};

static map<PSGLcontext *, GLstate> Contexts;

static GLstate           *cur = 0;
static GLstate::TexState *curt = 0;

extern "C" {

void spw_psglMakeCurrent(PSGLcontext *ctx, PSGLdevice *dev)
{
    map<PSGLcontext *, GLstate>::iterator i = Contexts.find(ctx);
    if (i == Contexts.end())
    {
    Contexts.insert(pair<PSGLcontext *, GLstate> (ctx, GLstate()));
    i = Contexts.find(ctx);
    }
    cur = &i->second;
    curt = &cur->Tex[cur->ActiveTexture-GL_TEXTURE0];
    psglMakeCurrent(ctx,dev);
}

void spw_psglResetCurrentContext()
{
    cur->init();
    curt = &cur->Tex[cur->ActiveTexture-GL_TEXTURE0];
    psglResetCurrentContext();
}

void spw_psglDestroyContext(PSGLcontext *ctx)
{
    map<PSGLcontext *, GLstate>::iterator i = Contexts.find(ctx);
    if (i != Contexts.end())
    Contexts.erase(i);
    psglDestroyContext(ctx);
}

void spw_glLogicOp (GLenum t)
{
    cur->LogicOp = t;
    glLogicOp(t);
}

void spw_glColorMask (GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    cur->ColorMask[0] = r;
    cur->ColorMask[1] = g;
    cur->ColorMask[2] = b;
    cur->ColorMask[3] = a;
    glColorMask(r,g,b,a);
}

void spw_glDepthFunc (GLenum t)
{
    cur->DepthFunc = t;
    glDepthFunc(t);
}
void spw_glDepthMask (GLboolean t)
{
    cur->DepthMask = t;
    glDepthMask(t);
}
void spw_glDepthRangef (GLfloat a, GLfloat b)
{
    cur->DepthRange[0] = a;
    cur->DepthRange[1] = b;
    glDepthRangef(a,b);
}

void spw_glEnable (GLenum t)
{
    map<GLenum,int>::iterator i = cur->States.find (t);
    if (i == cur->States.end())
    cur->States.insert (pair<GLenum,int> (t, 1));
    else
    i->second = 1;
    glEnable(t);
}
void spw_glDisable (GLenum t)
{
    map<GLenum,int>::iterator i = cur->States.find (t);
    if (i == cur->States.end())
    cur->States.insert (pair<GLenum,int> (t, 0));
    else
    i->second = 0;
    glDisable(t);
}

void spw_glFrontFace (GLenum t)
{
    cur->FrontFace = t;
    glFrontFace(t);
}
void spw_glCullFace (GLenum t)
{
    cur->CullFace = t;
    glCullFace(t);
}

void spw_glPolygonMode (GLenum f, GLenum t)
{
    if (f == GL_FRONT_AND_BACK || f == GL_FRONT)
    cur->PolyMode[0] = t;
    if (f == GL_FRONT_AND_BACK || f == GL_BACK)
    cur->PolyMode[1] = t;
    glPolygonMode(f,t);
}

void spw_glPointSize (GLfloat t)
{
    cur->PointSize = t;
    glPointSize(t);
}
void spw_glLineWidth (GLfloat t)
{
    cur->LineWidth = t;
    glLineWidth(t);
}

void spw_glClearColor (GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    cur->ClearColor[0] = r;
    cur->ClearColor[1] = g;
    cur->ClearColor[2] = b;
    cur->ClearColor[3] = a;
    glClearColor(r,g,b,a);
}
void spw_glClearDepthf (GLfloat s)
{
    cur->ClearDepth = s;
    glClearDepthf(s);
}
void spw_glClearStencil (GLint s)
{
    cur->ClearStencil = s;
    glClearStencil(s);
}

void spw_glScissor (GLint x, GLint y, GLsizei w, GLsizei h)
{
    cur->ScissorX = x;
    cur->ScissorY = y;
    cur->ScissorW = w;
    cur->ScissorH = h;
    glScissor(x,y,w,h);
}

void spw_glViewport (GLint x, GLint y, GLsizei w, GLsizei h)
{
    cur->ViewportX = x;
    cur->ViewportY = y;
    cur->ViewportW = w;
    cur->ViewportH = h;
    glViewport(x,y,w,h);
}

void spw_glAlphaFunc (GLenum t, GLfloat r)
{
    cur->AlphaFunc = t;
    cur->AlphaRef = r;
    glAlphaFunc(t,r);
}

void spw_glStencilFunc (GLenum f, GLint r, GLuint m)
{
    cur->StencilFunc = f;
    cur->StencilRef = r;
    cur->StencilMask = m;
    glStencilFunc(f,r,m);
}

void spw_glStencilOp (GLenum a, GLenum b, GLenum c)
{
    cur->StencilOp[0] = a;
    cur->StencilOp[0] = b;
    cur->StencilOp[0] = c;
    glStencilOp(a,b,c);
}

void spw_glPolygonOffset (GLfloat a, GLfloat b)
{
    cur->PolyOffset[0] = a;
    cur->PolyOffset[1] = b;
    glPolygonOffset (a,b);
}

void spw_glBlendFuncSeparate (GLenum sc, GLenum dc, GLenum sa, GLenum da)
{
    cur->BlendFnAs = sa;
    cur->BlendFnAd = da;
    cur->BlendFnCs = sc;
    cur->BlendFnCd = dc;
    glBlendFuncSeparate (sc,dc,sa,da);
}
void spw_glBlendFunc (GLenum s, GLenum d)
{
    cur->BlendFnAs = s;
    cur->BlendFnAd = d;
    cur->BlendFnCs = s;
    cur->BlendFnCd = d;
    glBlendFunc (s,d);
}

void spw_glBlendEquationSeparate (GLenum c, GLenum a)
{
    cur->BlendEqnA = a;
    cur->BlendEqnC = c;
    glBlendEquationSeparate (c,a);
}
void spw_glBlendEquation (GLenum t)
{
    cur->BlendEqnA = t;
    cur->BlendEqnC = t;
    glBlendEquation (t);
}

void spw_glBlendColor (GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    cur->BlendColor[0] = r;
    cur->BlendColor[1] = g;
    cur->BlendColor[2] = b;
    cur->BlendColor[3] = a;
    glBlendColor(r,g,b,a);
}

void spw_glActiveTexture (GLenum t)
{
    cur->ActiveTexture = t;
    curt = &cur->Tex[t-GL_TEXTURE0];
    glActiveTexture (t);
}

void spw_glClientActiveTexture (GLenum t)
{
    cur->ClientActiveTexture = t;
    glClientActiveTexture (t);
}

void spw_glBindTexture (GLenum t, GLenum o)
{
    if (t == GL_TEXTURE_2D)
    curt->Binding2D = o;
    glBindTexture (t, o);
}

void spw_glTexEnvf (GLenum t, GLenum p, GLfloat v)
{
    if (t == GL_TEXTURE_ENV)
    switch (p)
    {
    case GL_TEXTURE_ENV_MODE:  curt->EnvFunc = (GLenum)v; break;
    }
    else if (t == GL_TEXTURE_FILTER_CONTROL && p == GL_TEXTURE_LOD_BIAS)
    curt->LodBias = v;
    glTexEnvf (t,p,v);
}

void spw_glTexEnvi (GLenum t, GLenum p, GLint v)
{
    if (t == GL_TEXTURE_ENV)
    switch (p)
    {
    case GL_TEXTURE_ENV_MODE:  curt->EnvFunc = v; break;
    }
    else if (t == GL_TEXTURE_FILTER_CONTROL && p == GL_TEXTURE_LOD_BIAS)
    curt->LodBias = v;
    glTexEnvi (t,p,v);
}

void spw_glTexEnvfv (GLenum t, GLenum p, GLfloat *v)
{
    if (t == GL_TEXTURE_ENV)
    switch (p)
    {
    case GL_TEXTURE_ENV_MODE:  curt->EnvFunc = (GLenum)*v; break;
    case GL_TEXTURE_ENV_COLOR: memcpy(curt->EnvColor, v, sizeof(GLfloat) * 4); break;
    }
    else if (t == GL_TEXTURE_FILTER_CONTROL && p == GL_TEXTURE_LOD_BIAS)
    curt->LodBias = *v;
    glTexEnvfv (t,p,v);
}

void spw_glTexEnviv (GLenum t, GLenum p, GLint *v)
{
    if (t == GL_TEXTURE_ENV)
    switch (p)
    {
    case GL_TEXTURE_ENV_MODE:  curt->EnvFunc = *v; break;
    case GL_TEXTURE_ENV_COLOR:
        curt->EnvColor[0] = v[0] / (4.0 * (1<<30));
        curt->EnvColor[1] = v[1] / (4.0 * (1<<30));
        curt->EnvColor[2] = v[2] / (4.0 * (1<<30));
        curt->EnvColor[3] = v[3] / (4.0 * (1<<30));
        break;
    }
    else if (t == GL_TEXTURE_FILTER_CONTROL && p == GL_TEXTURE_LOD_BIAS)
    curt->LodBias = *v;
    glTexEnviv (t,p,v);
}

void spw_cgGLEnableProfile (CGprofile p)
{
    map<CGprofile,GLstate::CgProfileState>::iterator i = cur->CgProfiles.find (p);
    if (i == cur->CgProfiles.end())
    {
    cur->CgProfiles.insert (pair<CGprofile,GLstate::CgProfileState> (p, GLstate::CgProfileState()));
    i = cur->CgProfiles.find (p);
    }
    i->second.enabled = 1;
    cgGLEnableProfile(p);
}
void spw_cgGLDisableProfile (CGprofile p)
{
    map<CGprofile,GLstate::CgProfileState>::iterator i = cur->CgProfiles.find (p);
    if (i == cur->CgProfiles.end())
    {
    cur->CgProfiles.insert (pair<CGprofile,GLstate::CgProfileState> (p, GLstate::CgProfileState()));
    i = cur->CgProfiles.find (p);
    }
    i->second.enabled = 0;
    cgGLDisableProfile(p);
}
void spw_cgGLBindProgram (CGprogram t)
{
    CGprofile p = cgGetProgramProfile(t);
    map<CGprofile,GLstate::CgProfileState>::iterator i = cur->CgProfiles.find (p);
    if (i == cur->CgProfiles.end())
    {
    cur->CgProfiles.insert (pair<CGprofile,GLstate::CgProfileState> (p, GLstate::CgProfileState()));
    i = cur->CgProfiles.find (p);
    }
    i->second.binding = t;
    cgGLBindProgram(t);
}

}

void GLstate::apply () const
{
    for (map<GLenum,int>::const_iterator i = States.begin(); i != States.end(); i++)
    if (i->second)
        glEnable(i->first);
    else
        glDisable(i->first);

    glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
    glClearDepthf(ClearDepth);
    glClearStencil(ClearStencil);
    glScissor(ScissorX, ScissorY, ScissorW, ScissorH);
    glViewport(ViewportX, ViewportY, ViewportW, ViewportH);
    glLogicOp(LogicOp);
    glAlphaFunc(AlphaFunc, AlphaRef);
    glDepthMask(DepthMask);
    glDepthRangef(DepthRange[0], DepthRange[1]);
    glColorMask(ColorMask[0], ColorMask[1], ColorMask[2], ColorMask[3]);
    glStencilFunc(StencilFunc, StencilRef, StencilMask);
    glStencilOp(StencilOp[0], StencilOp[2], StencilOp[3]);
    glPolygonMode(GL_FRONT, PolyMode[0]);
    glPolygonMode(GL_BACK, PolyMode[1]);
    glPolygonOffset(PolyOffset[0], PolyOffset[1]);
    glFrontFace(FrontFace);
    glCullFace(CullFace);
    glLineWidth(LineWidth);
    glPointSize(PointSize);
    glBlendFuncSeparate(BlendFnCs, BlendFnCd, BlendFnAs, BlendFnAd);
    glBlendEquationSeparate(BlendEqnC, BlendEqnA);
    glBlendColor(BlendColor[0], BlendColor[1], BlendColor[2], BlendColor[3]);

    for (int i = 0; i < MAX_TEXTURE_UNITS; i++)
    {
    glActiveTexture(i + GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Tex[i].Binding2D);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, Tex[i].LodBias);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, Tex[i].EnvFunc);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, Tex[i].EnvColor);
    }

    glActiveTexture(ActiveTexture);
    glClientActiveTexture(ClientActiveTexture);

    for (map<CGprofile,CgProfileState>::const_iterator i = CgProfiles.begin(); i != CgProfiles.end(); i++)
    {
    if (i->second.enabled)
        cgGLEnableProfile(i->first);
    else
        cgGLDisableProfile(i->first);
    if (i->second.binding)
        cgGLBindProgram(i->second.binding);
    }
}

void SPW_ApplyStates()
{
    if (Contexts.size() == 1)
    {
    Contexts.begin()->second.apply();
    return;
    }

    PSGLdevice *dev = psglGetCurrentDevice();
    PSGLcontext *ctx = psglGetCurrentContext();

    for (map<PSGLcontext *, GLstate>::iterator i = Contexts.begin(); i != Contexts.end(); i++)
    {
    psglMakeCurrent (i->first, dev);
    i->second.apply();
    }

    psglMakeCurrent(ctx,dev);
}
