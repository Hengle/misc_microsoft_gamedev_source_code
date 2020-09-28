/**********************************************************************

Filename    :   GRendererOGLImpl.cpp
Content     :   OpenGL Sample renderer implementation
Created     :   
Authors     :   Andrew Reisse, Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GImage.h"
#include "GAtomic.h"
#include "GTLTypes.h"

#if defined(GFC_OS_WIN32)
    #include <windows.h>
#endif

#include "GRendererOGL.h"
#include "GRendererCommonImpl.h"

#include <string.h> // for memset()

#ifdef GFC_OS_PS3
  #include <PSGL/psgl.h>
  #include <PSGL/psglu.h>
  #include <cell/fs/cell_fs_file_api.h>
  #include <sys/paths.h>
  #define GFC_GL_NO_ALPHA_TEXTURES
#elif defined(GFC_OS_MAC)
  #include <OpenGL/OpenGL.h>
  #include <OpenGL/gl.h>
#elif defined(GFC_OS_WIN32)
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

// ***** Classes implemented
class GRendererOGLImpl;
class GTextureOGLImpl;


// Define this macro if mask should be implemented through
// z-buffer instead of stencil. Will overwrite z-buffer contents.
// #define  GFC_ZBUFFER_MASKING


// Pixel shaders
enum PixelShaderType
{
    PS_None                 = 0,
    PS_TextTexture,              // not actually present
    PS_CxformTexture,
    PS_CxformTextureMultiply,

    PS_CxformGouraud,
    PS_CxformGouraudNoAddAlpha,
    PS_CxformGouraudTexture,
    PS_Cxform2Texture,

    // Multiplies - must come in same order as other gourauds
    PS_CxformGouraudMultiply,
    PS_CxformGouraudMultiplyNoAddAlpha,
    PS_CxformGouraudMultiplyTexture,
    PS_CxformMultiply2Texture,

    // premultiplied alpha
    PS_AcTextTexture,
    PS_AcCxformTexture,
    PS_AcCxformTextureMultiply,

    PS_AcCxformGouraud,
    PS_AcCxformGouraudNoAddAlpha,
    PS_AcCxformGouraudTexture,
    PS_AcCxform2Texture,

    PS_AcCxformGouraudMultiply,
    PS_AcCxformGouraudMultiplyNoAddAlpha,
    PS_AcCxformGouraudMultiplyTexture,
    PS_AcCxformMultiply2Texture,

    PS_Count,
    PS_AcOffset = PS_AcCxformTexture-PS_CxformTexture,
};

/* *** Fixed Function Pipeline Support Notes

Some shader behavior can be replicated with fixed
function pipeline texture stages. However, there a number of FF limitations that
make this challenging or impossible.

Specifically:

1. Cxforms are supported as long as 3 texture units and GL_COMBINE are available.
If only 2 texture units, Blend_Multiply and Blend_Darken won't be correct.
Without GL_COMBINE, only the modulate portion is supported.

2. EdgeAA is difficult to support due to their being only one
color per vertex (secondary color is not usable for texture combines).
NoAddAlpha and 2Texture versions could be supported, but there are no Cap
flags for that. A complex use of lookup tables in textures might work for all cases,
but is not implemented.

*/


// ***** GTextureOGLImpl implementation


// GTextureOGLImpl declaration
class GTextureOGLImpl : public GTextureOGL
{
public:

    // Renderer
    GRendererOGLImpl *  pRenderer;
    SInt                Width, Height;
    // GL texture id
    GLuint              TextureId;
    GLenum              TextureFmt, TextureData;
    bool                DeleteTexture;

    GTextureOGLImpl(GRendererOGLImpl *prenderer);
    ~GTextureOGLImpl();

    // Obtains the renderer that create TextureInfo 
    virtual GRenderer*  GetRenderer() const;        
    virtual bool        IsDataValid() const;
    
    // Creates a texture based on parameters    
    bool                InitTextureAlpha(GImageBase* pim);

    virtual bool        InitTexture(UInt texID, SInt width, SInt height, bool deleteTexture);
    virtual bool        InitTexture(GImageBase* pim, int targetWidth, int targetHeight);
    virtual bool        InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                    int targetWidth , int targetHeight);
    virtual bool        InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight);
    virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim);

    // Helper function
    // Creates a texture id ans sets filter parameters
    void                InitTextureId(GLint minFilter);
    // Releases texture and clears vals
    virtual void        ReleaseTextureId();

    // Remove texture from renderer, notifies of renderer destruction
    void                RemoveFromRenderer();

    virtual UInt        GetGLTexture() const { return TextureId; }
};


#if (GFC_BYTE_ORDER == GFC_BIG_ENDIAN) && !defined(GFC_OS_PS3)
#define SWAPCOLOR(color) ((color >> 24) | ((color >> 8) & 0xff00) | ((color & 0xff00) << 8) | ((color & 0xff) << 24))
#else
#define SWAPCOLOR(color) color
#endif

// Vertex buffer structure used for glyphs.
struct GGlyphVertex
{
    float x,y,z,w;
    float u,v;
    GColor color;

    void SetVertex2D(float xx, float yy, float uu, float vv, GColor c)
    {
        x = xx; y = yy; u = uu; v = vv;
        color.Raw = SWAPCOLOR(c.Raw);
    }
};

class GBufferNode : public GRendererNode, public GNewOverrideBase
{
public:
    GLuint                      buffer;
    GRenderer::CachedData       *pcache;

    GBufferNode() : GRendererNode() { buffer = 0; pcache = 0; }
    GBufferNode(GBufferNode *p, GRenderer::CachedData *d) : GRendererNode(p) { buffer = 0; pcache = d; }
};

// ***** Renderer Implementation


class GRendererOGLImpl : public GRendererOGL
{
public:
    // Some renderer state.

    bool                Initialized;
    SInt                RenderMode;
    
    // Output size.
    Float               DisplayWidth;
    Float               DisplayHeight;
    
    GRenderer::Matrix   UserMatrix;
    GRenderer::Matrix   CurrentMatrix;
    GRenderer::Cxform   CurrentCxform;

    // Link list of all allocated textures
    GRendererNode       Textures;
    GLock               TexturesLock;

    Stats               RenderStats;

    // Vertex data pointer
    GBufferNode         BufferObjects;
    const void*         pVertexData;
    const void*         pIndexData;
    GLuint              VertexArray, IndexArray;
    VertexFormat        VertexFmt;
    GLenum              IndexFmt;

    // Stencil counter - for increment/decrement compares.
    SInt                StencilCounter;
    bool                DrawingMask;

    // Current sample mode
    BitmapSampleMode        SampleMode;

    // Current blend mode
    BlendType               BlendMode;
    GTL::garray<BlendType>  BlendModeStack;

    // Buffer used to pass along glyph vertices
    enum { GlyphVertexBufferSize = 6 * 48 };
    GGlyphVertex            GlyphVertexBuffer[GlyphVertexBufferSize];

    // Linked list used for buffer cache testing, otherwise holds no data.
    CacheNode               CacheList;

    SInt                    CurrentShader;

    GLint                   MaxTexUnits;
    bool                    UseCombine, UseBuffers, UseAcBlend;
    bool                    UseShaders;

#ifdef GFC_GL_RUNTIME_LINK
    // Extensions
    PFNGLBLENDEQUATIONPROC                       p_glBlendEquation;
    PFNGLBLENDFUNCSEPARATEPROC                   p_glBlendFuncSeparate;
    PFNGLDRAWRANGEELEMENTSPROC                   p_glDrawRangeElements;
    PFNGLMULTITEXCOORD4FVPROC                    p_glMultiTexCoord4fv;
    PFNGLCLIENTACTIVETEXTUREPROC                 p_glClientActiveTexture;
    PFNGLACTIVETEXTUREPROC                       p_glActiveTexture;
    PFNGLENABLEVERTEXATTRIBARRAYPROC             p_glEnableVertexAttribArray;
    PFNGLDISABLEVERTEXATTRIBARRAYPROC            p_glDisableVertexAttribArray;
    PFNGLVERTEXATTRIBPOINTERPROC                 p_glVertexAttribPointer;
    PFNGLVERTEXATTRIB4FPROC                      p_glVertexAttrib4f;
    PFNGLDELETEOBJECTARBPROC                     p_glDeleteObjectARB;
    PFNGLCREATESHADEROBJECTARBPROC               p_glCreateShaderObjectARB;
    PFNGLSHADERSOURCEARBPROC                     p_glShaderSourceARB;
    PFNGLCOMPILESHADERARBPROC                    p_glCompileShaderARB;
    PFNGLCREATEPROGRAMOBJECTARBPROC              p_glCreateProgramObjectARB;
    PFNGLATTACHOBJECTARBPROC                     p_glAttachObjectARB;
    PFNGLLINKPROGRAMARBPROC                      p_glLinkProgramARB;
    PFNGLUSEPROGRAMOBJECTARBPROC                 p_glUseProgramObjectARB;
    PFNGLGETOBJECTPARAMETERIVARBPROC             p_glGetObjectParameterivARB;
    PFNGLGETINFOLOGARBPROC                       p_glGetInfoLogARB;
    PFNGLGETUNIFORMLOCATIONARBPROC               p_glGetUniformLocationARB;
    PFNGLUNIFORM4FARBPROC                        p_glUniform4fARB;
    PFNGLUNIFORM1IARBPROC                        p_glUniform1iARB;

    PFNGLGENPROGRAMSARBPROC                      p_glGenProgramsARB;
    PFNGLDELETEPROGRAMSARBPROC                   p_glDeleteProgramsARB;
    PFNGLPROGRAMSTRINGARBPROC                    p_glProgramStringARB;
    PFNGLBINDPROGRAMARBPROC                      p_glBindProgramARB;
    PFNGLPROGRAMENVPARAMETER4DARBPROC            p_glProgramLocalParameter4dARB;
    PFNGLGENBUFFERSARBPROC                       p_glGenBuffers;
    PFNGLDELETEBUFFERSARBPROC                    p_glDeleteBuffers;
    PFNGLBINDBUFFERARBPROC                       p_glBindBuffer;
    PFNGLBUFFERDATAARBPROC                       p_glBufferData;

    PFNGLCOMPRESSEDTEXIMAGE2DPROC                p_glCompressedTexImage2D;

    void glBlendEquation (GLenum e) const { if (p_glBlendEquation) p_glBlendEquation (e); }
    void glBlendFuncSeparate (GLenum sc, GLenum dc, GLenum sa, GLenum da) const
    {
        if (p_glBlendFuncSeparate)
            p_glBlendFuncSeparate (sc,dc,sa,da);
        else
            ::glBlendFunc (sc, dc);
    }
    void glDrawRangeElements (GLenum p, GLuint s, GLuint e, GLsizei c, GLenum t, const void *a) const
    {
        if (p_glDrawRangeElements) p_glDrawRangeElements(p,s,e,c,t,a);
        else glDrawElements(p,c,t,a);
    }
    void glMultiTexCoord4fv(GLenum e, const GLfloat *v) const { p_glMultiTexCoord4fv(e,v); }
    void glActiveTexture(GLenum e) const { p_glActiveTexture(e); }
    void glClientActiveTexture(GLenum e) const { p_glClientActiveTexture(e); }
    void glEnableVertexAttribArrayARB(GLuint i) const { if (p_glEnableVertexAttribArray) p_glEnableVertexAttribArray(i); }
    void glDisableVertexAttribArrayARB(GLuint i) const { if (p_glDisableVertexAttribArray) p_glDisableVertexAttribArray(i); }
    void glVertexAttribPointerARB(GLuint i, GLint c, GLenum t, GLboolean n, GLsizei s, const GLvoid *a) const
    {
        if (p_glVertexAttribPointer)
            p_glVertexAttribPointer(i,c,t,n,s,a);
    }
    void glVertexAttrib4fARB(GLuint i, GLfloat a, GLfloat b, GLfloat c, GLfloat d) { p_glVertexAttrib4f(i,a,b,c,d); }
    void glDeleteObjectARB (GLhandleARB o) const { if (p_glDeleteObjectARB) p_glDeleteObjectARB(o); }

    GLhandleARB  glCreateShaderObjectARB (GLenum t) const { if (p_glCreateShaderObjectARB) return p_glCreateShaderObjectARB(t); }
    void glShaderSourceARB (GLhandleARB s, GLsizei n, const GLcharARB* *t, const GLint *l) const { p_glShaderSourceARB(s,n,t,l); }

    void  glCompileShaderARB (GLhandleARB s) const { p_glCompileShaderARB(s); }
    GLhandleARB  glCreateProgramObjectARB (void) const { if (p_glCreateProgramObjectARB) return p_glCreateProgramObjectARB(); }
    void  glAttachObjectARB (GLhandleARB o, GLhandleARB s) const { p_glAttachObjectARB(o,s); }
    void  glLinkProgramARB (GLhandleARB o) const { p_glLinkProgramARB(o); }
    void  glUseProgramObjectARB (GLhandleARB o) const { if (p_glUseProgramObjectARB) p_glUseProgramObjectARB(o); }
    void  glGetObjectParameterivARB (GLhandleARB o, GLenum e, GLint *v) const { p_glGetObjectParameterivARB(o,e,v); }
    void  glGetInfoLogARB (GLhandleARB o, GLsizei m, GLsizei *l, GLcharARB *t) const
    {
        if (p_glGetInfoLogARB)
            p_glGetInfoLogARB (o,m,l,t);
    }

    GLint glGetUniformLocationARB (GLhandleARB o, const GLcharARB *n) const { return p_glGetUniformLocationARB(o,n); }
    void  glUniform4fARB (GLint i, GLfloat a, GLfloat b, GLfloat c, GLfloat d) const { p_glUniform4fARB(i,a,b,c,d); }
    void  glUniform1iARB (GLint i, GLint a) const { p_glUniform1iARB(i,a); }

    void  glProgramStringARB (GLenum t, GLenum f, GLsizei l, const GLvoid *s) const { p_glProgramStringARB(t,f,l,s); }
    void  glBindProgramARB (GLenum t, GLuint o) const { p_glBindProgramARB(t,o); }
    void  glDeleteProgramsARB (GLsizei n, const GLuint *o) const { p_glDeleteProgramsARB(n,o); }
    void  glGenProgramsARB (GLsizei n, GLuint *o) const { p_glGenProgramsARB(n,o); }
    void  glProgramLocalParameter4dARB (GLenum t, GLuint i, GLdouble x, GLdouble y, GLdouble z, GLdouble w) const
    {
        p_glProgramLocalParameter4dARB(t, i, x,y,z,w);
    }
    void  glGenBuffers(GLsizei n, GLuint *o) { p_glGenBuffers(n,o); }
    void  glDeleteBuffers(GLsizei n, GLuint *o) { p_glDeleteBuffers(n,o); }
    void  glBindBuffer(GLenum t, GLuint b) { p_glBindBuffer(t,b); }
    void  glBufferData(GLenum t, GLsizeiptr s, const GLvoid *p, GLenum u) { p_glBufferData(t,s,p,u);}

    void  glCompressedTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
    {
        if (p_glCompressedTexImage2D)
            p_glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
        else
        {
            GFC_DEBUG_WARNING(1, "GRendererOGL texture creation failed - glCompressedTexImage2D ext not available");
        }
    }

#else
    void glBlendEquation (GLenum e) const { ::glBlendEquation (e); }
    void glActiveTexture(GLenum e) const { ::glActiveTexture(e); }
#endif

    // all OGL calls should be done on the main thread, but textures can be released from 
    // different threads so we collect system resources and release them on the main thread
    GTL::garray<GLuint> ResourceReleasingQueue;
    GLock               ResourceReleasingQueueLock;

    // this method is called from GTextureXXX destructor to put a system resource into releasing queue
    void AddResourceForReleasing(GLuint textureId)
    {
        if (textureId == 0) return;
        GLock::Locker guard(&ResourceReleasingQueueLock);
        ResourceReleasingQueue.push_back(textureId);
    }

    // this method is called from GetTexture, EndDisplay and destructor to actually release
    // collected system resources
    void ReleaseQueuedResources()
    {
        GLock::Locker guard(&ResourceReleasingQueueLock);
        for (UInt i = 0; i < ResourceReleasingQueue.size(); ++i)
            glDeleteTextures(1, &ResourceReleasingQueue[i]);
        ResourceReleasingQueue.clear();
    }

     // ****

    GRendererOGLImpl()
    {       
        Initialized = 0;

        SampleMode  = Sample_Linear;
        RenderMode  = 0;
        pVertexData = 0;
        pIndexData  = 0;
        VertexFmt   = Vertex_None;
        IndexFmt    = GL_UNSIGNED_SHORT;

        StencilCounter = 0;

        BlendModeStack.set_size_policy(GTL::garray<BlendType>::Buffer_NoShrink);

        // Glyph buffer z, w components are always 0 and 1.
        UInt i;
        for(i = 0; i < GlyphVertexBufferSize; i++)
        {
            GlyphVertexBuffer[i].z = 0.0f;
            GlyphVertexBuffer[i].w = 1.0f;
        }
    }

    ~GRendererOGLImpl()
    {
        Clear();
    }

    void Clear()
    {
        // Remove/notify all textures
        {
        GLock::Locker guard(&TexturesLock);
        while (Textures.pFirst != &Textures)
            ((GTextureOGLImpl*)Textures.pFirst)->RemoveFromRenderer();
        }

        CacheList.ReleaseList();

        for (GRendererNode *p = BufferObjects.pFirst; p; )
        {
            GBufferNode *pn = (GBufferNode*)p;
#ifdef GL_ARB_vertex_buffer_object
            if (pn->buffer)
                glDeleteBuffers(1, &pn->buffer);
#endif
            if (pn->pcache)
                pn->pcache->ReleaseDataByRenderer();
            p = p->pNext;
            if (p == BufferObjects.pLast)
                p->pNext = 0;
            if (pn != &BufferObjects)
                delete pn;
        }
        BufferObjects.pFirst = NULL;
        ReleaseQueuedResources();
    }

    void ReleaseResources()
    {
        Clear();
    }


    // Utility.  Mutates *width, *height and *data to create the
    // next mip level.
    static void MakeNextMiplevel(int* width, int* height, UByte* data, GLenum format = GL_ALPHA)
    {
        GASSERT(width);
        GASSERT(height);
        GASSERT(data);
        GASSERT_ON_RENDERER_MIPMAP_GEN;

        int newW = *width >> 1;
        int newH = *height >> 1;
        if (newW < 1) newW = 1;
        if (newH < 1) newH = 1;
        
        if (newW * 2 != *width || newH * 2 != *height)
        {
            // Image can not be shrunk Along (at least) one
            // of its dimensions, so no need to do
            // resampling.  Technically we should, but
            // it's pretty useless at this point.  Just
            // change the image dimensions and leave the
            // existing pixels.
        }
        else if (format == GL_ALPHA || format == GL_LUMINANCE)
        {
            // Resample.  Simple average 2x2 --> 1, in-place.
            for (int j = 0; j < newH; j++) 
            {
                UByte*  out = ((UByte*) data) + j * newW;
                UByte*  in  = ((UByte*) data) + (j << 1) * *width;
                for (int i = 0; i < newW; i++) 
                {
                    int a = (*(in + 0) + *(in + 1) + *(in + 0 + *width) + *(in + 1 + *width));
                    *(out) = UByte(a >> 2);
                    out++;
                    in += 2;
                }
            }
        }
        else
        {
            // Resample.  Simple average 2x2 --> 1, in-place.
            for (int j = 0; j < newH; j++) 
            {
                UByte*  out = ((UByte*) data) + j * (4 * newW);
                UByte*  in = ((UByte*) data) + (j << 1) * (4 * *width);
                for (int i = 0; i < newW; i++) 
                {
                    int r,g,b,a;
                    r = ( *(in + 0) + *(in + 4) + *(in + 0 + (*width * 4)) + *(in + 4 + (*width * 4)) );
                    g = ( *(in + 1) + *(in + 5) + *(in + 1 + (*width * 4)) + *(in + 5 + (*width * 4)) );
                    b = ( *(in + 2) + *(in + 6) + *(in + 2 + (*width * 4)) + *(in + 6 + (*width * 4)) );
                    a = ( *(in + 3) + *(in + 7) + *(in + 3 + (*width * 4)) + *(in + 7 + (*width * 4)) );
                    out[0] = UByte(r >> 2);
                    out[1] = UByte(g >> 2);
                    out[2] = UByte(b >> 2);
                    out[3] = UByte(a >> 2);
                    out += 4;
                    in += 8;
                }
            }
        }

        // Set parameters to reflect the new size
        *width = newW;
        *height = newH;
    }

    virtual void DisableShaders () { }
    virtual void SetPixelShader (PixelShaderType ps, SInt pass = 0) { GUNUSED2(ps,pass); }
    virtual void ApplyPShaderCxform(const Cxform &cxform) const { GUNUSED(cxform); }
    virtual bool SetVertexProgram (VertexFormat vf, SInt numtex) { GUNUSED2(vf,numtex); return 0; }
    virtual void SetVertexProgState () { }
    virtual void SetTexgenState (SInt stageIndex, const FillTexture& fill) { GUNUSED2(stageIndex,fill); }
    virtual void SetTextureState (SInt stageIndex, const FillTexture& fill) { GUNUSED2(stageIndex,fill); }
    virtual void SetTextureState (SInt stageIndex, const GTexture *pTexture) { GUNUSED2(stageIndex,pTexture); }
    virtual void VertexAttribArray (SInt attr, GLint size, GLenum type, GLboolean norm, GLsizei stride, GLvoid* array)
    {
        GUNUSED4(attr,size,type,norm); GUNUSED2(stride,array);
    }
    virtual void DisableVertexAttribArrays () { }

    // Fill helper function:
    // Applies fill texture by setting it to the specified stage, initializing samplers and vertex constants
    virtual void    ApplyFillTexture(const FillTexture &fill, UInt stageIndex, bool useVS = 0)
    {
        GASSERT (fill.pTexture != 0);
        if (fill.pTexture == 0) return; // avoid crash in release build

        GTextureOGLImpl* ptexture = ((GTextureOGLImpl*)fill.pTexture);

        glActiveTexture(GL_TEXTURE0 + stageIndex);
        glBindTexture(GL_TEXTURE_2D, ptexture->TextureId);
        glEnable(GL_TEXTURE_2D);

        if (fill.WrapMode == Wrap_Clamp)
        {   
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else
        {
            GASSERT(fill.WrapMode == Wrap_Repeat);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        ApplySampleMode(fill.SampleMode, 0); //!AB, why useMipmaps is false here?

        if (useVS)
            SetTexgenState (stageIndex, fill);
        else
        {
            // Set up the bitmap GRenderer::Matrix for texgen.
            Float   InvWidth = 1.0f / ptexture->Width;
            Float   InvHeight = 1.0f / ptexture->Height;
            const GRenderer::Matrix&    m = fill.TextureMatrix;
            Float   p[4] = { 0, 0, 0, 0 };

            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
            p[0] = m.M_[0][0] * InvWidth;
            p[1] = m.M_[0][1] * InvWidth;
            p[3] = m.M_[0][2] * InvWidth;
            glTexGenfv(GL_S, GL_OBJECT_PLANE, p);

            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
            p[0] = m.M_[1][0] * InvHeight;
            p[1] = m.M_[1][1] * InvHeight;
            p[3] = m.M_[1][2] * InvHeight;
            glTexGenfv(GL_T, GL_OBJECT_PLANE, p);
        }

        glActiveTexture(GL_TEXTURE0);
    }

    class GFxFillStyle
    {
    public:
        enum FillMode
        {
            FM_None,
            FM_Color,
            FM_Bitmap,
            FM_Gouraud
        };

        FillMode                Mode;
        GouraudFillType         GouraudType;
        GColor                  Color;
        FillTexture             Fill, Fill2;

        GRenderer::Cxform       BitmapColorTransform;
        bool                    HasNonzeroAdditiveCxform;
        
        GFxFillStyle()
        {
            Mode            = FM_None;
            GouraudType     = GFill_Color;
            Fill.pTexture   = 0;
            Fill2.pTexture  = 0;
            HasNonzeroAdditiveCxform = false;
        }

        // Push our style into OpenGL.
        void    Apply(GRendererOGLImpl *prenderer) const
        {
            GASSERT(Mode != FM_None);

            prenderer->DisableShaders();

            if (prenderer->MaxTexUnits)
            {
                prenderer->glActiveTexture(GL_TEXTURE1);
                glDisable(GL_TEXTURE_2D);
                prenderer->glActiveTexture(GL_TEXTURE2);
                glDisable(GL_TEXTURE_2D);
                prenderer->glActiveTexture(GL_TEXTURE0);
            }

            if (Mode == FM_Color)
            {
                prenderer->ApplyColor(Color, (prenderer->UseAcBlend == 0) && (prenderer->RenderMode & GViewport::View_AlphaComposite));
            
                if ((Color.GetAlpha() == 0xFF) && (prenderer->BlendMode <= Blend_Normal))
                    glDisable(GL_BLEND);
                else
                    glEnable(GL_BLEND);

                glDisable(GL_TEXTURE_2D);
            }
            else if (Mode == FM_Bitmap)
            {
                if (Fill.pTexture == NULL)
                {
                    glDisable(GL_TEXTURE_2D);

                    if ((Color.GetAlpha() == 0xFF) && (prenderer->BlendMode <= Blend_Normal))
                        glDisable(GL_BLEND);
                    else
                        glEnable(GL_BLEND);

                    prenderer->ApplyColor(Color, (prenderer->UseAcBlend == 0) && (prenderer->RenderMode & GViewport::View_AlphaComposite));
                }
                else if (prenderer->UseShaders)
                {
                    glEnable(GL_BLEND);

                    prenderer->ApplyFillTexture(Fill, 0);

                    if (prenderer->BlendMode == Blend_Multiply || prenderer->BlendMode == Blend_Darken)
                        prenderer->SetPixelShader(PS_CxformTextureMultiply);
                    else
                        prenderer->SetPixelShader(PS_CxformTexture);

                    prenderer->ApplyPShaderCxform(BitmapColorTransform);
                    prenderer->SetTextureState (0, Fill);
                }
#ifdef GL_ARB_texture_env_combine
                else if (prenderer->MaxTexUnits >= 2 && prenderer->UseCombine)
                {
                    glEnable(GL_BLEND);
                    prenderer->ApplyFillTexture(Fill, 0);

                    glColor4f(BitmapColorTransform.M_[0][0],
                        BitmapColorTransform.M_[1][0],
                        BitmapColorTransform.M_[2][0],
                        BitmapColorTransform.M_[3][0]);

                    Float   sca[4] = { 
                        BitmapColorTransform.M_[0][1] / 255.0f, 
                        BitmapColorTransform.M_[1][1] / 255.0f,
                        BitmapColorTransform.M_[2][1] / 255.0f, 
                        BitmapColorTransform.M_[3][1] / 255.0f };

                    Float   sc1[4] = {1.0f, 1.0f, 1.0f, 1.0f};

                    if ((prenderer->BlendMode == Blend_Multiply || prenderer->BlendMode == Blend_Darken)
                        && prenderer->MaxTexUnits >= 3)
                    {
                        prenderer->glActiveTexture(GL_TEXTURE2);
                        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, ((GTextureOGLImpl*)Fill.pTexture)->TextureId);
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
                        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
                        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_INTERPOLATE);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GL_SRC_ALPHA);
                        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, sc1);
                    }

                    if (prenderer->RenderMode & GViewport::View_AlphaComposite
                        && prenderer->MaxTexUnits >= ((prenderer->BlendMode == Blend_Multiply || prenderer->BlendMode == Blend_Darken) ? 4 : 3))
                    {
                        if (prenderer->BlendMode == Blend_Multiply || prenderer->BlendMode == Blend_Darken)
                            prenderer->glActiveTexture(GL_TEXTURE3);
                        else
                            prenderer->glActiveTexture(GL_TEXTURE2);

                        glEnable(GL_TEXTURE_2D);
                        glBindTexture(GL_TEXTURE_2D, ((GTextureOGLImpl*)Fill.pTexture)->TextureId);
                        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
                        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
                        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_ALPHA);
                        glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
                    }

                    prenderer->glActiveTexture(GL_TEXTURE1);
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, ((GTextureOGLImpl*)Fill.pTexture)->TextureId);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
                    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_CONSTANT);
                    glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
                    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
                    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
                    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
                    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
                    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, sca);
                    prenderer->glActiveTexture(GL_TEXTURE0);
                }
#endif
                else // modulate part of transform (add in second pass with combine and <2 tex units only)
                {
                    glEnable(GL_BLEND);
                    prenderer->ApplyFillTexture(Fill, 0);

                    glColor4f(BitmapColorTransform.M_[0][0],
                        BitmapColorTransform.M_[1][0],
                        BitmapColorTransform.M_[2][0],
                        BitmapColorTransform.M_[3][0]);
                }
            }
            else if (Mode == FM_Gouraud)
            {
                PixelShaderType shader = PS_None;
                bool            useVS = prenderer->SetVertexProgram(prenderer->VertexFmt, 
                        GouraudType == GFill_2Texture || GouraudType == GFill_2TextureColor ? 2 : 1);

                // No texture: generate color-shaded triangles.
                if (Fill.pTexture == NULL)
                {
                    glEnable(GL_BLEND);

                    if (prenderer->VertexFmt == Vertex_XY16iC32)
                    {
                        // Cxform Alpha Add can not be non-zero is this state because 
                        // cxform blend equations can not work correctly.
                        // If we hit this assert, it means Vertex_XY16iCF32 should have been used.
                        GASSERT (BitmapColorTransform.M_[3][1] < 1.0f);

                        shader = PS_CxformGouraudNoAddAlpha;                        
                    }
                    else
                    {
                        shader = PS_CxformGouraud;
                    }
                }
                // We have a textured or multi-textured gouraud case.
                else
                {   
                    glEnable(GL_BLEND);

                    prenderer->ApplyFillTexture(Fill, 0, useVS);

                    if ((GouraudType == GFill_1TextureColor) ||
                        (GouraudType == GFill_1Texture))
                    {
                        shader = PS_CxformGouraudTexture;
                    }
                    else
                    {
                        shader = PS_Cxform2Texture;
                        prenderer->ApplyFillTexture(Fill2, 1, useVS);
                    }

                }

                if ((prenderer->BlendMode == Blend_Multiply) ||
                    (prenderer->BlendMode == Blend_Darken) )
                { 
                    shader = (PixelShaderType)(shader + (PS_CxformGouraudMultiply - PS_CxformGouraud));

                    // For indexing to work, these should hold:
                    GCOMPILER_ASSERT( (PS_Cxform2Texture - PS_CxformGouraud) ==
                        (PS_CxformMultiply2Texture - PS_CxformGouraudMultiply));
                    GCOMPILER_ASSERT( (PS_CxformGouraudMultiply - PS_CxformGouraud) ==
                        (PS_Cxform2Texture - PS_CxformGouraud + 1) );
                }
                prenderer->SetPixelShader(shader);
                prenderer->ApplyPShaderCxform(BitmapColorTransform);

                if (Fill.pTexture)
                {
                    prenderer->SetTextureState (0, Fill);
                    if (useVS) prenderer->SetTexgenState (0, Fill);

                    if (Fill2.pTexture &&
                        (GouraudType == GFill_2TextureColor) ||
                        (GouraudType == GFill_2Texture))
                    {
                        prenderer->SetTextureState (1, Fill2);
                        if (useVS) prenderer->SetTexgenState (1, Fill2);
                    }
                }
            }
        }


        // Return true if we need to do a second pass to make
        // a valid color.
        bool    NeedsSecondPass(GRendererOGLImpl *prenderer) const
        {
            if (Mode == FM_Bitmap && prenderer->UseCombine && prenderer->MaxTexUnits < 2)
            {
                return HasNonzeroAdditiveCxform;
            }
            else
            {
                return false;
            }
        }

        // Set OpenGL state for a necessary second pass.
        void    ApplySecondPass() const
        {
            // The additive color also seems to be modulated by the texture. So,
            // maybe we can fake this in one pass using using the mean value of 
            // the colors: c0*t+c1*t = ((c0+c1)/2) * t*2
            // not sure what the alpha component of the color is for.

#ifdef GL_ARB_texture_env_combine
            glEnable(GL_BLEND);

            glColor4f(
                BitmapColorTransform.M_[0][1] / 255.0f,
                BitmapColorTransform.M_[1][1] / 255.0f,
                BitmapColorTransform.M_[2][1] / 255.0f,
                BitmapColorTransform.M_[3][1] / 255.0f
                );

            glEnable(GL_TEXTURE_2D);            
            
            // Alpha needs to come from texture.
            // Add-color needs to come from above.
            // Need to use COMBINE functions (GL 1.4 +).
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE); // not GL_MODULATE
            glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
            glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
            glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
            glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
#endif
        }

        void    CleanupSecondPass(GRendererOGLImpl *pRenderer) const
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            pRenderer->glBlendEquation(GL_FUNC_ADD);

            // Restore modulate.
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }


        void    Disable()               { Mode = FM_None; Fill.pTexture = 0; }

        void    SetColor(GColor color)  { Mode = FM_Color; Color = color; }

        void    SetCxform(const Cxform& colorTransform)
        {
            BitmapColorTransform = colorTransform;

            if ( BitmapColorTransform.M_[0][1] > 1.0f ||
                BitmapColorTransform.M_[1][1] > 1.0f ||
                BitmapColorTransform.M_[2][1] > 1.0f ||
                BitmapColorTransform.M_[3][1] > 1.0f )         
                HasNonzeroAdditiveCxform = true;            
            else            
                HasNonzeroAdditiveCxform = false;
        }

        void    SetBitmap(const FillTexture* pft, const Cxform& colorTransform)
        {
            Mode            = FM_Bitmap;
            Fill            = *pft;
            Color           = GColor(0xFFFFFFFF);

            SetCxform(colorTransform);
        }

        // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
        // The specified textures are applied to vertices {0, 1, 2} of each triangle based
        // on factors of Complex vertex. Any or all subsequent pointers can be NULL, in which case
        // texture is not applied and vertex colors used instead.
        void    SetGouraudFill(GouraudFillType gfill,
            const FillTexture *ptexture0,
            const FillTexture *ptexture1,
            const FillTexture *ptexture2, const Cxform& colorTransform)
        {

            // Texture2 is not yet used.
            if (ptexture0 || ptexture1 || ptexture2)
            {
                const FillTexture *p = ptexture0;
                if (!p) p = ptexture1;              

                SetBitmap(p, colorTransform);
                // Used in 2 texture mode
                if (ptexture1)
                    Fill2 = *ptexture1;             
            }
            else
            {
                SetCxform(colorTransform);              
                Fill.pTexture = 0;  
            }

            Mode        = GFxFillStyle::FM_Gouraud;
            GouraudType = gfill;
        }

        bool    IsValid() const { return Mode != FM_None; }
    };


    // Style state.
    enum StyleIndex
    {
        FILL_STYLE = 0,     
        LINE_STYLE,
        STYLE_COUNT
    };
    GFxFillStyle    CurrentStyles[STYLE_COUNT];


    // Given an image, returns a Pointer to a GTexture struct
    // that can later be passed to FillStyleX_bitmap(), to set a
    // bitmap fill style.
    GTextureOGL*   CreateTexture()
    {
        ReleaseQueuedResources();
        GLock::Locker guard(&TexturesLock);
        return new GTextureOGLImpl(this);
    }


    // Helper function to query renderer capabilities.
    bool        GetRenderCaps(RenderCaps *pcaps)
    {
        if (!Initialized)
        {
            GFC_DEBUG_WARNING(1, "GRendererOGL::GetRenderCaps fails - video mode not set");
            return 0;
        }
        
        pcaps->CapBits      = Cap_Index16 | Cap_Index32 | Cap_NestedMasks;
        pcaps->BlendModes   = (1<<Blend_None) | (1<<Blend_Normal) |
                              (1<<Blend_Multiply) | (1<<Blend_Lighten) | (1<<Blend_Darken) |
                              (1<<Blend_Add) | (1<<Blend_Subtract);
        pcaps->VertexFormats= (1<<Vertex_None) | (1<<Vertex_XY16i) | (1<<Vertex_XY16iC32) | (1<<Vertex_XY16iCF32);

        if (UseShaders)
            pcaps->CapBits |= Cap_CxformAdd | Cap_FillGouraud | Cap_FillGouraudTex;
        else if (UseCombine)
            pcaps->CapBits |= Cap_CxformAdd;

        GLint maxTextureSize = 64;
        glGetIntegerv (GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        pcaps->MaxTextureSize = maxTextureSize;
        return 1;
    }


    // Set up to render a full frame from a movie and fills the
    // background.  Sets up necessary transforms, to scale the
    // movie to fit within the given dimensions.  Call
    // EndDisplay() when you're done.
    //
    // The Rectangle (viewportX0, viewportY0, 
    // viewportX0 + viewportWidth, viewportY0 + viewportHeight) 
    // defines the window coordinates taken up by the movie.
    //
    // The Rectangle (x0, y0, x1, y1) defines the pixel
    // coordinates of the movie that correspond to the viewport bounds.
    void    BeginDisplay(
        GColor backgroundColor, const GViewport &viewport,
        Float x0, Float x1, Float y0, Float y1)

    {
        RenderMode = (viewport.Flags & GViewport::View_AlphaComposite);

        BlendModeStack.clear();
        BlendModeStack.reserve(16);
        BlendMode = Blend_None;
        ApplyBlendMode(BlendMode); 

        DisplayWidth = fabsf(x1 - x0);
        DisplayHeight = fabsf(y1 - y0);

        glViewport(viewport.Left, viewport.BufferHeight-viewport.Top-viewport.Height, 
                   viewport.Width, viewport.Height);

        if (viewport.Flags & GViewport::View_UseScissorRect)
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(viewport.ScissorLeft, viewport.BufferHeight-viewport.ScissorTop-viewport.ScissorHeight, 
                      viewport.ScissorWidth, viewport.ScissorHeight);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

#ifdef GFC_OS_PS3
        if (viewport.Flags & GViewport::View_IsRenderTexture)
            glOrthof(x0, x1, y0, y1, -1, 1);
        else
            glOrthof(x0, x1, y1, y0, -1, 1);
#else
        if (viewport.Flags & GViewport::View_IsRenderTexture)
            glOrtho(x0, x1, y0, y1, -1, 1);
        else
            glOrtho(x0, x1, y1, y0, -1, 1);
#endif

        glDisable(GL_CULL_FACE);

        // No lighting or depth testing.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_ALPHA_TEST);
        glStencilMask(0xffffffff);

        glEnable(GL_BLEND);

        for (int i = 0; i < MaxTexUnits; i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glDisable(GL_TEXTURE_2D);
        }
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);

        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

#ifdef GL_ARB_vertex_buffer_object
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif

        DisableShaders();

        // Sample Mode
        SampleMode = Sample_Linear;

        BlendMode = Blend_Normal;

        // Stencil counter starts out at 0.
        StencilCounter = 0;
        DrawingMask = 0;

        // Clear the background, if background color has alpha > 0.
        if (backgroundColor.GetAlpha() > 0)
        {
            // Draw a big quad.
            if (backgroundColor.GetAlpha() == 0xFF)
                glDisable(GL_BLEND);
            else
                glEnable(GL_BLEND);

            ApplyColor(backgroundColor, (UseAcBlend == 0) && (RenderMode & GViewport::View_AlphaComposite));

            GLfloat bgVertBuffer[8];
            bgVertBuffer[0] = x0; bgVertBuffer[1] = y0;
            bgVertBuffer[2] = x1; bgVertBuffer[3] = y0;
            bgVertBuffer[4] = x1; bgVertBuffer[5] = y1;
            bgVertBuffer[6] = x0; bgVertBuffer[7] = y1;

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, bgVertBuffer);
            glDrawArrays(GL_QUADS, 0, 4); 
            glDisableClientState(GL_VERTEX_ARRAY);
        }
    }


    // Clean up after rendering a frame.  Client program is still
    // responsible for calling glSwapBuffers() or whatever.
    void    EndDisplay()
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        ReleaseQueuedResources();
    }


    // Set the current transform for mesh & line-strip rendering.
    void    SetMatrix(const GRenderer::Matrix& m)
    {
        CurrentMatrix = m;
    }

    void    SetUserMatrix(const GRenderer::Matrix& m)
    {
        UserMatrix = m;
    }

    // Set the current color transform for mesh & line-strip rendering.
    void    SetCxform(const GRenderer::Cxform& cx)
    {
        CurrentCxform = cx;
    }

    struct BlendModeDesc
    {
        GLenum op, src, dest;
    };

    struct BlendModeDescAlpha
    {
        GLenum op, srcc, srca, destc, desta;
    };

    void    ApplyBlendMode(BlendType mode)
    {
        static BlendModeDesc modes[15] =
        {
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // None
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Normal
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Layer

            { GL_FUNC_ADD,              GL_DST_COLOR,           GL_ZERO                 }, // Multiply
            // (For multiply, should src be pre-multiplied by its inverse alpha?)

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Screen *??

            { GL_MAX,                   GL_SRC_ALPHA,           GL_ONE                  }, // Lighten
            { GL_MIN,                   GL_SRC_ALPHA,           GL_ONE                  }, // Darken

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Difference *??

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE                  }, // Add
            { GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA,           GL_ONE                  }, // Subtract

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Invert *??

            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Alpha *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Erase *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // Overlay *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,           GL_ONE_MINUS_SRC_ALPHA  }, // HardLight *??
        };

    // Blending into alpha textures with premultiplied colors
        static BlendModeDescAlpha acmodes[15] =
        {
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // None
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Normal
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Layer

            { GL_FUNC_ADD,              GL_DST_COLOR,  GL_DST_ALPHA,  GL_ZERO,                GL_ZERO                 }, // Multiply

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Screen *??

            { GL_MAX,                   GL_SRC_ALPHA,  GL_SRC_ALPHA,  GL_ONE,                 GL_ONE                  }, // Lighten *??
            { GL_MIN,                   GL_SRC_ALPHA,  GL_SRC_ALPHA,  GL_ONE,                 GL_ONE                  }, // Darken *??

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Difference

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Add
            { GL_FUNC_REVERSE_SUBTRACT, GL_SRC_ALPHA,  GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Subtract

            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Invert *??

            { GL_FUNC_ADD,              GL_ZERO,       GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Alpha *??
            { GL_FUNC_ADD,              GL_ZERO,       GL_ZERO,       GL_ONE,                 GL_ONE                  }, // Erase *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // Overlay *??
            { GL_FUNC_ADD,              GL_SRC_ALPHA,  GL_ONE,        GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA  }, // HardLight *??
        };

        // For debug build
        GASSERT(((UInt) mode) < 15);
        // For release
        if (((UInt) mode) >= 15)
            mode = Blend_None;

        if (RenderMode & GViewport::View_AlphaComposite)
        {
            if (UseAcBlend)
            {
                glBlendFuncSeparate(acmodes[mode].srcc, acmodes[mode].destc, acmodes[mode].srca, acmodes[mode].desta);
                glBlendEquation(acmodes[mode].op);
            }
            else
            {
                glBlendFunc(modes[mode].src == GL_SRC_ALPHA ? GL_ONE : modes[mode].src, modes[mode].dest);
                glBlendEquation(modes[mode].op);
                if (!DrawingMask)
                {
                    if (BlendMode == Blend_Add)
                        glColorMask(1,1,1,0);
                    else
                        glColorMask(1,1,1,1);
                }
            }
        }
        else
        {
            glBlendFunc(modes[mode].src, modes[mode].dest);
            glBlendEquation(modes[mode].op);
        }
    }

    // Pushes a Blend mode onto renderer.
    virtual void    PushBlendMode(BlendType mode)
    {
        // Blend modes need to be applied cumulatively, ideally through an extra RT texture.
        // If the nested clip has a "Multiply" effect, and its parent has an "Add", the result
        // of Multiply should be written to a buffer, and then used in Add.

        // For now we only simulate it on one level -> we apply the last effect on top of stack.
        // (Note that the current "top" element is BlendMode, it is not actually stored on stack).
        // Although incorrect, this will at least ensure that parent's specified effect
        // will be applied to children.

        BlendModeStack.push_back(BlendMode);

        if ((mode > Blend_Layer) && (BlendMode != mode))
        {
            BlendMode = mode;
            ApplyBlendMode(BlendMode);
        }
    }

    // Pops a blend mode, restoring it to the previous one. 
    virtual void    PopBlendMode()
    {
        if (BlendModeStack.size() != 0)
        {                       
            // Find the next top interesting mode.
            BlendType   newBlendMode = Blend_None;
            
            for (SInt i = (SInt)BlendModeStack.size()-1; i>=0; i--)
                if (BlendModeStack[i] > Blend_Layer)
                {
                    newBlendMode = BlendModeStack[i];
                    break;
                }

            BlendModeStack.pop_back();

            if (newBlendMode != BlendMode)
            {
                BlendMode = newBlendMode;
                ApplyBlendMode(BlendMode);
            }

            return;
        }
        
        // Stack was empty..
        GFC_DEBUG_WARNING(1, "GRendererGL::PopBlendMode - blend mode stack is empty");
    }
    

    void ApplySampleMode(BitmapSampleMode mode, bool useMipmaps)
    {
        GLint filter = (mode == Sample_Point) ?
                            GL_NEAREST :
                            (useMipmaps ? GL_LINEAR_MIPMAP_LINEAR  : GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (filter == GL_NEAREST) ? GL_NEAREST : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);              
    }
    
    // multiply current GRenderer::Matrix with GL GRenderer::Matrix
    void    ApplyMatrix(const GRenderer::Matrix& m1)
    {
        GRenderer::Matrix m(UserMatrix);
        m *= m1;

        Float   mat[16];
        memset(&mat[0], 0, sizeof(mat));
        mat[0] = m.M_[0][0];
        mat[1] = m.M_[1][0];
        mat[4] = m.M_[0][1];
        mat[5] = m.M_[1][1];
        mat[10] = 1;
        mat[12] = m.M_[0][2];
        mat[13] = m.M_[1][2];
        mat[15] = 1;
        glMultMatrixf(mat);
    }

    // Set the given color.
    void ApplyColor(const GColor c, int alpha = 0)
    {
        if (alpha || 
            (BlendMode == Blend_Multiply) ||
            (BlendMode == Blend_Darken))
        {
            const float alpha = c.GetAlpha() * (1.0f / 65025.0f);
            glColor4f(c.GetRed() * alpha, c.GetGreen() * alpha, c.GetBlue() * alpha, 
                      c.GetAlpha() * (1.0f / 255.0f));
        }
        else
            glColor4ub(c.GetRed(), c.GetGreen(), c.GetBlue(), c.GetAlpha());
    }

    // Don't fill on the {0 == left, 1 == right} side of a path.
    void    FillStyleDisable()
    {
        CurrentStyles[FILL_STYLE].Disable();
    }
    // Don't draw a line on this path.
    void    LineStyleDisable()
    {
        CurrentStyles[LINE_STYLE].Disable();
    }

    // Set fill style for the left interior of the shape.  If
    // enable is false, turn off fill for the left interior.
    void    FillStyleColor(GColor color)
    {
        CurrentStyles[FILL_STYLE].SetColor(CurrentCxform.Transform(color));
    }

    // Set the line style of the shape.  If enable is false, turn
    // off lines for following curve segments.
    void    LineStyleColor(GColor color)
    {
        CurrentStyles[LINE_STYLE].SetColor(CurrentCxform.Transform(color));
    }

    void    FillStyleBitmap(const FillTexture *pfill)
    {       
        CurrentStyles[FILL_STYLE].SetBitmap(pfill, CurrentCxform);
    }
    
    // Sets the interpolated color/texture fill style used for shapes with EdgeAA.
    void    FillStyleGouraud(GouraudFillType gfill,
                             const FillTexture *ptexture0,
                             const FillTexture *ptexture1,
                             const FillTexture *ptexture2)
    {
        CurrentStyles[FILL_STYLE].SetGouraudFill(gfill, ptexture0, ptexture1, ptexture2, CurrentCxform);
    }

    void    SetVertexData(const void* pvertices, int numVertices, VertexFormat vf, CacheProvider *pcache)
    {
        pVertexData = pvertices;
        VertexFmt   = vf;
        VertexArray = 0;

#ifdef GL_ARB_vertex_buffer_object
        if (pcache && UseBuffers)
        {
            CachedData *pcd = pcache->GetCachedData(this);
            if (pcd)
            {
                VertexArray = ((GBufferNode*)pcd->GetRendererData())->buffer;
                return;
            }

            GLsizeiptr size;
            switch (vf)
            {
            case Vertex_XY16i:      size = 2 * sizeof(GLshort); break;
            case Vertex_XY32f:      size = 2 * sizeof(GLfloat); break;
            case Vertex_XY16iC32:   size = 2 * sizeof(GLshort) + 4; break;
            case Vertex_XY16iCF32:  size = 2 * sizeof(GLshort) + 8; break;
            default: return;
            }

            GBufferNode *pva;
            pcd = pcache->CreateCachedData(Cached_Vertex,this,0);

            if (!pcd->GetRendererData())
            {
                GLsizeiptr vasize = (size * numVertices + 15) & ~15;
                pva = new GBufferNode(&BufferObjects, pcd);
                pcd->SetRendererData(pva);
                glGenBuffers(1, &pva->buffer);

                glBindBuffer(GL_ARRAY_BUFFER, pva->buffer);
                glBufferData(GL_ARRAY_BUFFER, vasize, pvertices, GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                VertexArray = pva->buffer;
            }
        }
#endif
        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Vertex, (pvertices!=0),
                                    numVertices, (((pvertices!=0)&&numVertices) ? *((SInt16*)pvertices) : 0) );
    }

    void    SetIndexData(const void* pindices, int numIndices, IndexFormat idxf, CacheProvider *pcache)
    {
        pIndexData  = pindices;
        IndexArray  = 0;
        switch(idxf)
        {
            case Index_None:    IndexFmt = 0; break;
            case Index_16:      IndexFmt = GL_UNSIGNED_SHORT; break;
            case Index_32:      IndexFmt = GL_UNSIGNED_INT; break;
        }

#ifdef GL_ARB_vertex_buffer_object
        if (pcache && UseBuffers)
        {
            CachedData *pcd = pcache->GetCachedData(this);
            if (pcd)
            {
                IndexArray = ((GBufferNode*)pcd->GetRendererData())->buffer;
                return;
            }

            GLsizeiptr size;
            switch (idxf)
            {
            case Index_16:      size = sizeof(GLshort); break;
            case Index_32:      size = sizeof(GLint); break;
            default: return;
            }

            GBufferNode *pva;
            pcd = pcache->CreateCachedData(Cached_Index,this,0);

            if (!pcd->GetRendererData())
            {
                pva = new GBufferNode(&BufferObjects, pcd);
                pcd->SetRendererData(pva);
                glGenBuffers(1, &pva->buffer);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pva->buffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * numIndices, pindices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                IndexArray = pva->buffer;
            }
        }
#endif

        // Test cache buffer management support.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_Index, (pindices!=0),
                                    numIndices, (((pindices!=0)&&numIndices) ? *((SInt16*)pindices) : 0) );
    }

    void    ReleaseCachedData(CachedData *pdata, CachedDataType type)
    {
        // Releases cached data that was allocated from the cache providers.
        CacheList.ReleaseCachedData(pdata, type);

#ifdef GL_ARB_vertex_buffer_object
        if (pdata->GetRendererData())
        {
            GBufferNode *pn = (GBufferNode*)pdata->GetRendererData();
            glDeleteBuffers(1, &pn->buffer);
            pn->RemoveNode();
            delete pn;
            pdata->SetRendererData(0);
        }
#endif
    }


    void    DrawIndexedTriList(int baseVertexIndex, int minVertexIndex, int numVertices,
                               int startIndex, int triangleCount)
    {       
        GUNUSED2(minVertexIndex, numVertices);

        if (!pVertexData || !pIndexData || !IndexFmt) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(!pVertexData, "GRendererOGL::DrawIndexedTriList failed, vertex data not specified");
            GFC_DEBUG_WARNING(!pIndexData, "GRendererOGL::DrawIndexedTriList failed, index data not specified");
            GFC_DEBUG_WARNING(!IndexFmt, "GRendererOGL::DrawIndexedTriList failed, index buffer format not specified");
            return;
        }

        // Set up current style.
        CurrentStyles[FILL_STYLE].Apply(this);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        ApplyMatrix(CurrentMatrix);

        SetVertexProgState(); // for Cg matrices

        const void* pindices = (UByte*)(IndexArray ? 0 : pIndexData) + startIndex *
                                ((IndexFmt == GL_UNSIGNED_SHORT) ? sizeof(UInt16) : sizeof(UInt32));
        const void *pVertexBase = pVertexData;

#ifdef GL_ARB_vertex_buffer_object
        if (VertexArray)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VertexArray);
            pVertexBase = 0;
        }
        if (IndexArray)
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexArray);
#endif

        // Send the tris to OpenGL
        glEnableClientState(GL_VERTEX_ARRAY);

        // Gouraud colors
        if (VertexFmt == Vertex_XY16iC32)
        {
            glVertexPointer(2, GL_SHORT, sizeof(VertexXY16iC32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iC32));
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof (VertexXY16iC32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iC32) + 2 * sizeof(UInt16));
        }
        else if (VertexFmt == Vertex_XY16iCF32)
        {
            glVertexPointer(2, GL_SHORT, sizeof(VertexXY16iCF32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iCF32));

            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(4, GL_UNSIGNED_BYTE, sizeof (VertexXY16iCF32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iCF32) + 2 * sizeof(UInt16));

            VertexAttribArray (0, 4, GL_UNSIGNED_BYTE, 1, sizeof (VertexXY16iCF32),
                ((UByte*)pVertexBase) + baseVertexIndex * sizeof(VertexXY16iCF32) + 2 * sizeof(UInt16) + 4);
        }
        else
        {
            glVertexPointer(2, GL_SHORT, sizeof(SInt16) * 2,
                ((UByte*)pVertexBase) + baseVertexIndex * 2 * sizeof(SInt16));
        }

        glDrawRangeElements(GL_TRIANGLES, minVertexIndex, 
            numVertices+minVertexIndex, 3*triangleCount, IndexFmt, pindices);

        RenderStats.Triangles += triangleCount;
        RenderStats.Primitives++;

        if (CurrentStyles[FILL_STYLE].NeedsSecondPass(this))
        {
            CurrentStyles[FILL_STYLE].ApplySecondPass();
            glDrawRangeElements(GL_TRIANGLES, minVertexIndex, 
                numVertices+minVertexIndex, 3*triangleCount, IndexFmt, pindices);
            CurrentStyles[FILL_STYLE].CleanupSecondPass(this);

            RenderStats.Triangles += triangleCount;
            RenderStats.Primitives++;
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        DisableVertexAttribArrays ();

#ifdef GL_ARB_vertex_buffer_object
        if (VertexArray) glBindBuffer(GL_ARRAY_BUFFER, 0);
        if (IndexArray) glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#endif

        glPopMatrix();
    }



    // Draw the line strip formed by the sequence of Points.
    void    DrawLineStrip(int baseVertexIndex, int lineCount)
    {       
        if (!pVertexData) // Must have vertex data.
        {
            GFC_DEBUG_WARNING(!pVertexData, "GRendererOGL::DrawLineStrip failed, vertex data not specified");           
            return;
        }

        // Set up current style.
        CurrentStyles[LINE_STYLE].Apply(this);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        ApplyMatrix(CurrentMatrix);

        const void *pVertexBase = pVertexData;

#ifdef GL_ARB_vertex_buffer_object
        if (VertexArray)
        {
            glBindBuffer(GL_ARRAY_BUFFER, VertexArray);
            pVertexBase = 0;
        }
#endif

        // Send the line-strip to OpenGL
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_SHORT, sizeof(SInt16) * 2,
                        ((UByte*)pVertexBase) + baseVertexIndex * 2 * sizeof(SInt16));
        glDrawArrays(GL_LINE_STRIP, 0, lineCount + 1);
        glDisableClientState(GL_VERTEX_ARRAY);

        RenderStats.Lines += lineCount;
        RenderStats.Primitives++;

#ifdef GL_ARB_vertex_buffer_object
        if (VertexArray) glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
        glPopMatrix();
    }


    // Draw a set of rectangles textured with the given bitmap, with the given color.
    // Apply given transform; ignore any currently set transforms.  
    //
    // Intended for textured glyph rendering.
    void    DrawBitmaps(BitmapDesc* pbitmapList, int listSize, int startIndex, int count,
                        const GTexture* pti, const Matrix& m,
                        CacheProvider *pcache)
    {
        if (!pbitmapList || !pti)
            return;

        // Test cache buffer management support.
        // Optimized implementation could use this spot to set vertex buffer and/or initialize offset in it.
        // Note that since bitmap lists are usually short, it would be a good idea to combine many of them
        // into one buffer, instead of creating individual buffers for each pbitmapList data instance.
        CacheList.VerifyCachedData( this, pcache, CacheNode::Buffer_BitmapList, (pbitmapList!=0),
                                    listSize, (((pbitmapList!=0)&&listSize) ? ((SInt)pbitmapList->Coords.Left) : 0) );

        glEnable(GL_BLEND);
        DisableShaders();

        if (MaxTexUnits)
        {
            glActiveTexture(GL_TEXTURE3);
            glDisable(GL_TEXTURE_2D);
            glActiveTexture(GL_TEXTURE2);
            glDisable(GL_TEXTURE_2D);

            glActiveTexture(GL_TEXTURE1);

            if (UseShaders)
            {
                SetPixelShader(PS_TextTexture);
                SetTextureState(0, pti);
            }
#ifdef GL_ARB_texture_env_combine
            else if (UseCombine && RenderMode & GViewport::View_AlphaComposite)
            {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, ((GTextureOGLImpl*)pti)->TextureId);
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
                glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PREVIOUS);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_ALPHA);
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
            }
            else
#endif
                glDisable(GL_TEXTURE_2D);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ((GTextureOGLImpl*)pti)->TextureId);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);

        // Always use linear mipmap sampling for text.
        ApplySampleMode(Sample_Linear, 1);
        
        // Custom matrix per call.
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        ApplyMatrix(m);
        ApplyPShaderCxform(CurrentCxform);

        SInt ibitmap = 0, ivertex = 0;
        GCOMPILER_ASSERT((GlyphVertexBufferSize%6) == 0);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(4, GL_FLOAT, sizeof(GGlyphVertex), GlyphVertexBuffer);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(GGlyphVertex), &GlyphVertexBuffer[0].color);
        glTexCoordPointer(2, GL_FLOAT, sizeof(GGlyphVertex), ((UByte*)GlyphVertexBuffer) + sizeof(Float)*4);

        if (UseShaders)
        while (ibitmap < count)
        {
            for(ivertex = 0; (ivertex < GlyphVertexBufferSize) && (ibitmap<count); ibitmap++, ivertex+= 6)
            {
                BitmapDesc &  bd = pbitmapList[ibitmap + startIndex];
                GGlyphVertex* pv = GlyphVertexBuffer + ivertex;

                // Triangle 1.
                pv[0].SetVertex2D(bd.Coords.Left, bd.Coords.Top,    bd.TextureCoords.Left, bd.TextureCoords.Top, bd.Color);
                pv[1].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[2].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                // Triangle 2.
                pv[3].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, bd.Color);
                pv[4].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, bd.Color);
                pv[5].SetVertex2D(bd.Coords.Right, bd.Coords.Bottom,bd.TextureCoords.Right, bd.TextureCoords.Bottom, bd.Color);
            }

            // Draw the generated triangles.
            if (ivertex)
            {
                glDrawArrays(GL_TRIANGLES, 0, ivertex);
                RenderStats.Primitives++;
            }
        }
        else
            while (ibitmap < count)
            {
                for(ivertex = 0; (ivertex < GlyphVertexBufferSize) && (ibitmap<count); ibitmap++, ivertex+= 6)
                {
                    BitmapDesc &  bd = pbitmapList[ibitmap + startIndex];
                    GGlyphVertex* pv = GlyphVertexBuffer + ivertex;
#if GFC_BYTE_ORDER == GFC_LITTLE_ENDIAN
                    GColor c1 = CurrentCxform.Transform(bd.Color);
                    GColor Color (c1.GetRed() | (c1.GetGreen() << 8) | (c1.GetBlue() << 16) | (c1.GetAlpha() << 24));
#else
                    GColor Color = CurrentCxform.Transform(bd.Color);
#endif

                    // Triangle 1.
                    pv[0].SetVertex2D(bd.Coords.Left, bd.Coords.Top,    bd.TextureCoords.Left, bd.TextureCoords.Top, Color);
                    pv[1].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, Color);
                    pv[2].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, Color);
                    // Triangle 2.
                    pv[3].SetVertex2D(bd.Coords.Left, bd.Coords.Bottom, bd.TextureCoords.Left, bd.TextureCoords.Bottom, Color);
                    pv[4].SetVertex2D(bd.Coords.Right, bd.Coords.Top,   bd.TextureCoords.Right, bd.TextureCoords.Top, Color);
                    pv[5].SetVertex2D(bd.Coords.Right, bd.Coords.Bottom,bd.TextureCoords.Right, bd.TextureCoords.Bottom, Color);
                }

                // Draw the generated triangles.
                if (ivertex)
                {
                    glDrawArrays(GL_TRIANGLES, 0, ivertex);
                    RenderStats.Primitives++;
                }
            }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        RenderStats.Triangles += count * 2;

#ifdef GL_ARB_texture_env_combine
        if (UseCombine && RenderMode & GViewport::View_AlphaComposite)
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif

        glPopMatrix();
    }
    
    void BeginSubmitMask(SubmitMaskMode maskMode)
    {
        DrawingMask = 1;
        glColorMask(0,0,0,0);                       // disable framebuffer writes

#ifndef GFC_ZBUFFER_MASKING
        glEnable(GL_STENCIL_TEST);

        switch(maskMode)
        {
        case Mask_Clear:
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glStencilFunc(GL_ALWAYS, 1, 1);             // Always pass, 1 bit plane, 1 as mask
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // 
            StencilCounter = 1;
            break;

        case Mask_Increment:
            glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
            StencilCounter++;
            break;

        case Mask_Decrement:
            glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
            StencilCounter--;
            break;
        }

#else
        glEnable(GL_DEPTH_TEST);                    // enable the depth test and the depth write        
        glDepthMask(GL_TRUE);

        if (maskMode == Mask_Clear)
        {
            glClearDepth(1.0);                      // clear the depth buffer to the farthest value
            glClear(GL_DEPTH_BUFFER_BIT);
            glDepthFunc(GL_ALWAYS);                 // always write on the depth buffer
        }
        else
        {
            // Don't modify buffer for increment/decrement.
            glDepthFunc(GL_NEVER);
        }

#endif
    }
    
    void EndSubmitMask()
    {        
        DrawingMask = 0;
        glColorMask(1,1,1,1);                       // Enable frame-buffer writes

#ifndef GFC_ZBUFFER_MASKING
        // We draw only where the (stencil == StencilCounter); i.e. where the mask was drawn.
        // Don't change the stencil buffer    
        glStencilFunc(GL_EQUAL, StencilCounter, 0xFF);      
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);             
#else
        glEnable(GL_DEPTH_TEST);                    // Enable the depth test and the depth write
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_EQUAL);                      // We draw only where the mask was drawn with whatever depth is assumed by your driver.
                                                    // According to the spec, this should be 0.0. If it is 1.0, change the clear depth above.
#endif 
    }
    
    void DisableMask()
    {   
#ifndef GFC_ZBUFFER_MASKING
        glDisable(GL_STENCIL_TEST);
        StencilCounter = 0;
#else
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
#endif
    }


        
    virtual void        GetRenderStats(Stats *pstats, bool resetStats)
    {
        if (pstats)
            memcpy(pstats, &RenderStats, sizeof(Stats));
        if (resetStats)
            RenderStats.Clear();
    }

    static bool CheckExtension (const char *exts, const char *name)
    {
        const char *p = strstr(exts, name);
        return (p && (p[strlen(name)] == 0 || p[strlen(name)] == ' '));
    }

    // Specify pixel format (?)
    // (we can also get it from the window)
    // Specify OpenGL context ?
    virtual bool                SetDependentVideoMode()
    {
        if (Initialized)
            return 1;

#ifdef GFC_GL_RUNTIME_LINK
        p_glBlendEquation           = (PFNGLBLENDEQUATIONPROC) GFC_GL_RUNTIME_LINK ("glBlendEquation");
        p_glBlendFuncSeparate       = (PFNGLBLENDFUNCSEPARATEPROC) GFC_GL_RUNTIME_LINK ("glBlendFuncSeparate");
        p_glDrawRangeElements       = (PFNGLDRAWRANGEELEMENTSPROC) GFC_GL_RUNTIME_LINK("glDrawRangeElements");
        p_glMultiTexCoord4fv        = (PFNGLMULTITEXCOORD4FVPROC) GFC_GL_RUNTIME_LINK("glMultiTexCoord4fv");
        p_glActiveTexture           = (PFNGLACTIVETEXTUREPROC) GFC_GL_RUNTIME_LINK("glActiveTexture");
        p_glClientActiveTexture     = (PFNGLCLIENTACTIVETEXTUREPROC) GFC_GL_RUNTIME_LINK("glClientActiveTexture");
        p_glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) GFC_GL_RUNTIME_LINK("glEnableVertexAttribArrayARB");
        p_glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) GFC_GL_RUNTIME_LINK("glDisableVertexAttribArrayARB");
        p_glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC) GFC_GL_RUNTIME_LINK("glVertexAttribPointerARB");
        p_glVertexAttrib4f          = (PFNGLVERTEXATTRIB4FPROC) GFC_GL_RUNTIME_LINK("glVertexAttrib4fARB");
        p_glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC) GFC_GL_RUNTIME_LINK ("glDeleteObjectARB");
        p_glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC) GFC_GL_RUNTIME_LINK ("glCreateShaderObjectARB");
        p_glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC) GFC_GL_RUNTIME_LINK ("glShaderSourceARB");
        p_glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC) GFC_GL_RUNTIME_LINK ("glCompileShaderARB");
        p_glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC) GFC_GL_RUNTIME_LINK ("glCreateProgramObjectARB");
        p_glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC) GFC_GL_RUNTIME_LINK ("glAttachObjectARB");
        p_glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC) GFC_GL_RUNTIME_LINK ("glLinkProgramARB");
        p_glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC) GFC_GL_RUNTIME_LINK ("glUseProgramObjectARB");
        p_glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC) GFC_GL_RUNTIME_LINK ("glGetInfoLogARB");
        p_glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC) GFC_GL_RUNTIME_LINK ("glGetUniformLocationARB");
        p_glUniform4fARB            = (PFNGLUNIFORM4FARBPROC) GFC_GL_RUNTIME_LINK ("glUniform4fARB");
        p_glUniform1iARB            = (PFNGLUNIFORM1IARBPROC) GFC_GL_RUNTIME_LINK ("glUniform1iARB");
        p_glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) GFC_GL_RUNTIME_LINK("glGetObjectParameterivARB");
        p_glProgramStringARB        = (PFNGLPROGRAMSTRINGARBPROC) GFC_GL_RUNTIME_LINK ("glProgramStringARB");
        p_glBindProgramARB          = (PFNGLBINDPROGRAMARBPROC) GFC_GL_RUNTIME_LINK ("glBindProgramARB");
        p_glProgramLocalParameter4dARB = (PFNGLPROGRAMENVPARAMETER4DARBPROC) GFC_GL_RUNTIME_LINK ("glProgramLocalParameter4dARB");
        p_glGenProgramsARB          = (PFNGLGENPROGRAMSARBPROC) GFC_GL_RUNTIME_LINK("glGenProgramsARB");
        p_glDeleteProgramsARB       = (PFNGLDELETEPROGRAMSARBPROC) GFC_GL_RUNTIME_LINK("glDeleteProgramsARB");
        p_glGenBuffers              = (PFNGLGENBUFFERSARBPROC) GFC_GL_RUNTIME_LINK("glGenBuffersARB");
        p_glDeleteBuffers           = (PFNGLDELETEBUFFERSARBPROC) GFC_GL_RUNTIME_LINK("glDeleteBuffersARB");
        p_glBindBuffer              = (PFNGLBINDBUFFERARBPROC) GFC_GL_RUNTIME_LINK("glBindBufferARB");
        p_glBufferData              = (PFNGLBUFFERDATAARBPROC) GFC_GL_RUNTIME_LINK("glBufferDataARB");
        p_glCompressedTexImage2D    = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) GFC_GL_RUNTIME_LINK("glCompressedTexImage2D");
#endif

        UseShaders = 0;
        UseBuffers = 0;
        UseAcBlend = 0;

        const char *glexts = (const char *) glGetString(GL_EXTENSIONS);

        MaxTexUnits = 0;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS, &MaxTexUnits);
        if (MaxTexUnits <= 1)
            MaxTexUnits = 0;
        UseCombine = CheckExtension(glexts, "GL_ARB_texture_env_combine");
        UseAcBlend = CheckExtension(glexts, "GL_EXT_blend_func_separate");
        //UseBuffers = CheckExtension(glexts, "GL_ARB_vertex_buffer_object");

        Initialized = 1;
        return 1;
    }
                                        
    // Returns back to original mode (cleanup)                                                      
    virtual bool                ResetVideoMode()
    {
        ReleaseQueuedResources();
        return 1;
    }

    virtual DisplayStatus       CheckDisplayStatus() const
    {
        return Initialized ? DisplayStatus_Ok : DisplayStatus_NoModeSet;
    }

    inline void SetCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
    {
        glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);   
    }
    
};  // end class GRendererOGLImpl




// ***** GTextureOGL implementation

GTextureOGLImpl::GTextureOGLImpl(GRendererOGLImpl *prenderer)
    : GTextureOGL(&prenderer->Textures)
{
    pRenderer   = prenderer;
    Width       = 
    Height      = 0;
    TextureId   = 0;
}

GTextureOGLImpl::~GTextureOGLImpl()
{
    ReleaseTextureId(); 
    if (!pRenderer)
        return;
    GLock::Locker guard(&pRenderer->TexturesLock);
    if (pFirst)
        RemoveNode();
}

// Obtains the renderer that create TextureInfo 
GRenderer*  GTextureOGLImpl::GetRenderer() const
    { return pRenderer; }
bool        GTextureOGLImpl::IsDataValid() const
    { return (TextureId > 0);   }

// Remove texture from renderer, notifies renderer destruction
void    GTextureOGLImpl::RemoveFromRenderer()
{
    pRenderer = 0;
    if (AddRef_NotZero())
    {
        ReleaseTextureId();
        CallHandlers(ChangeHandler::Event_RendererReleased);
        if (pNext) // We may have been released by user
            RemoveNode();
        Release();
    } else {
        if (pNext) // We may have been released by user
            RemoveNode();
    }
}

// Creates a texture id ans sets filter parameters, initializes Width/Height
void    GTextureOGLImpl::InitTextureId(GLint minFilter)
{
    // Create the texture.
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, (GLuint*)&TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);   

    DeleteTexture = 1;
}

// Releases texture and clears values
void    GTextureOGLImpl::ReleaseTextureId()
{
    if (TextureId > 0)
    {
        if (DeleteTexture)
        {
            if (pRenderer)
                pRenderer->AddResourceForReleasing(TextureId);
            else
                glDeleteTextures(1, &TextureId);
        }
        TextureId = 0;
    }
    Width = Height = 0;
}


// Forward declarations for sampling/mipmaps
void    HardwareResample(int bytesPerPixel, int srcWidth, int srcHeight, UByte* psrcData, int dstWidth, int dstHeight);
void    SoftwareResample(int bytesPerPixel, int srcWidth, int srcHeight, int srcPitch, UByte* psrcData, int dstWidth, int dstHeight);
void    GenerateMipmaps(unsigned internalFormat, unsigned inputFormat, int bytesPerPixel, GImageBase* pim);


bool GTextureOGLImpl::InitTexture(UInt texID, SInt width, SInt height, bool deleteTexture)
{
    ReleaseTextureId();

    if (texID)
    {
        Width = width;
        Height = height;
        TextureId = texID;
        DeleteTexture = deleteTexture;
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool GTextureOGLImpl::InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                  int targetWidth , int targetHeight)
{
    ReleaseTextureId();

    GLint   internalFormat=0;
    GLint   datatype = GL_UNSIGNED_BYTE;
    //bool    resample = 0;

    if (format == GImage::Image_ARGB_8888)
    {
        internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
    }
    else if (format == GImage::Image_RGB_888)
    {
        internalFormat  = GL_RGB;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
    }
    else if (format == GImage::Image_A_8)
    {
#ifdef GFC_GL_NO_ALPHA_TEXTURES
        internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
#else
        internalFormat  = GL_ALPHA;
#endif
    }
    else
        GASSERT(0);

    InitTextureId(GL_LINEAR);
    TextureFmt = internalFormat;
    TextureData = datatype;

    Width   = (targetWidth == 0) ? width : targetWidth;
    Height  = (targetHeight == 0) ? height: targetHeight;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, datatype, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmaps);

    for (int i = 0; i < mipmaps; i++)
    {
        width >>= 1;
        height >>= 1;
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;
        glTexImage2D(GL_TEXTURE_2D, i+1, internalFormat, width, height, 0, internalFormat, datatype, 0);
    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

// NOTE: This function destroys pim's data in the process of making mipmaps.
bool GTextureOGLImpl::InitTexture(GImageBase* pim, int targetWidth, int targetHeight)
{   
    // Delete old data
    ReleaseTextureId();
    if (!pim)
    {
        // Kill texture     
        CallHandlers(ChangeHandler::Event_DataChange);
        return 1;
    }

    // Determine format
    UInt    bytesPerPixel=0;
    GLint   internalFormat=0;
    GLint   datatype = GL_UNSIGNED_BYTE;
    bool    resample = 0;

    if (pim->Format == GImage::Image_ARGB_8888)
    {
        bytesPerPixel   = 4;
        internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
#endif
    }
    else if (pim->Format == GImage::Image_RGB_888)
    {
        bytesPerPixel   = 3;
        internalFormat  = GL_RGB;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
        resample        = 1;
#endif
    }
    else if (pim->Format == GImage::Image_A_8)
    {
        return InitTextureAlpha(pim);
    }
    else if (pim->Format == GImage::Image_DXT1)
    {
        bytesPerPixel   = 1;
        internalFormat  = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    }
    else if (pim->Format == GImage::Image_DXT3)
    {
        bytesPerPixel   = 1;
        internalFormat  = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    }
    else if (pim->Format == GImage::Image_DXT5)
    {
        bytesPerPixel   = 1;
        internalFormat  = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    }
    else
    { // Unsupported format
        GASSERT(0);
    }


    // Create the texture.
    InitTextureId(GL_LINEAR);
    TextureFmt = internalFormat;
    TextureData = datatype;

    Width   = (targetWidth == 0) ? pim->Width : targetWidth;
    Height  = (targetHeight == 0) ? pim->Height: targetHeight;

    UInt    w = 1; while (w < pim->Width) { w <<= 1; }
    UInt    h = 1; while (h < pim->Height) { h <<= 1; }

    if (w != pim->Width || h != pim->Height || resample)
    {
        GASSERT_ON_RENDERER_RESAMPLING;

        // Faster/simpler software bilinear rescale.
        SoftwareResample(bytesPerPixel, pim->Width, pim->Height, pim->Pitch, pim->pData, w, h);
    }
    else
    {
        // Use original image directly.
        UInt level = 0;
        do 
        {
            UInt mipW, mipH;
            const UByte* pdata = pim->GetMipMapLevelData(level, &mipW, &mipH);
            if (pdata == 0) //????
            {
                GFC_DEBUG_WARNING(1, "GRendererOGL: can't find mipmap level in texture");
                break;
            }
            if (pim->IsDataCompressed())
            {
                pRenderer->SetCompressedTexImage2D(GL_TEXTURE_2D, level, internalFormat, mipW, mipH, 0, 
                    GImage::GetMipMapLevelSize(pim->Format, mipW, mipH), pdata);
            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, level, internalFormat, mipW, mipH, 0, internalFormat, datatype,
                    pdata);
            }
        } while(++level < pim->MipMapCount);

    }

    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

bool    GTextureOGLImpl::InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight)    
{
    GUNUSED3(pfilename,targetWidth,targetHeight);
    return false;
}

bool    GTextureOGLImpl::InitTextureAlpha(GImageBase* pim)
{
    GASSERT(pim);
    int width  = pim->Width;
    int height = pim->Height;
    UByte* data = pim->pData;

    bool    retVal = 1;
    // Delete old data
    ReleaseTextureId(); 
    if (!data)
    {
        // Kill texture
    empty_texture:      
        CallHandlers(ChangeHandler::Event_DataChange);
        return retVal;
    }

    GASSERT(width > 0);
    GASSERT(height > 0);

    InitTextureId(GL_LINEAR_MIPMAP_LINEAR);
    // Copy data to avoid destruction
#ifdef GFC_GL_NO_ALPHA_TEXTURES
    UByte *pnewData = (UByte*)GALLOC(width * height * 4);
    if (!pnewData)
    {
        retVal = 0;
        ReleaseTextureId();
        goto empty_texture;
    }       

    int len = width * height * 4;
    for( int i = 0, j = 0; i < len; i += 4, j++ )
    {
        pnewData[i] = 255;
        pnewData[i+1] = 255;
        pnewData[i+2] = 255;
        pnewData[i+3] = data[j];
    }
#else
    UByte *pnewData = 0;
    if (pim->MipMapCount <= 1)
    {
        pnewData = (UByte*)GALLOC(width * height);
        if (!pnewData)
        {
            retVal = 0;
            ReleaseTextureId();
            goto empty_texture;
        }       
        memcpy(pnewData, data, width*height);
    }
    else
        pnewData = data;
#endif

    Width = width;
    Height = height;

#ifdef GFC_BUILD_DEBUG
    // must use power-of-two dimensions
    int w = 1; while (w < width) { w <<= 1; }
    int h = 1; while (h < height) { h <<= 1; }
    GASSERT(w == width);
    GASSERT(h == height);
#endif // DEBUG

#ifdef GFC_GL_NO_ALPHA_TEXTURES
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pnewData);

    // Build mips.
    int level = 1;
    while (width > 1 || height > 1)
    {
        GRendererOGLImpl::MakeNextMiplevel(&width, &height, pnewData, GL_RGBA);
        glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pnewData);
        level++;
    }
    TextureFmt = GL_RGBA;
    TextureData = GL_UNSIGNED_INT_8_8_8_8;
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pnewData);

    int level = 1;
    if (pim->MipMapCount <= 1)
    {
        // Build mips.
        while (width > 1 || height > 1)
        {
            GRendererOGLImpl::MakeNextMiplevel(&width, &height, pnewData);
            glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pnewData);
            level++;
        }
    }
    else
    {
        while(level < int(pim->MipMapCount)) 
        {
            UInt mipW, mipH;
            const UByte* pmipdata = pim->GetMipMapLevelData(level, &mipW, &mipH);
            if (pmipdata == 0) //????
            {
                GFC_DEBUG_WARNING(1, "GRendererOGL: can't find mipmap level in texture");
                break;
            }
            glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA, mipW, mipH, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 
                pmipdata);
            ++level;
        }
    }
    TextureFmt = GL_ALPHA;
    TextureData = GL_UNSIGNED_BYTE;
#endif

    if (pnewData != data)
        GFREE(pnewData);    
    CallHandlers(ChangeHandler::Event_DataChange);
    return 1;
}

void GTextureOGLImpl::Update(int level, int n, const UpdateRect *rects, const GImageBase *pim)
{
    GLenum datatype = GL_UNSIGNED_BYTE;
    GLenum internalFormat = GL_RGBA; // avoid warning.
    bool   convert = 0;

    if (pim->Format == GImage::Image_ARGB_8888)
    {
        internalFormat  = GL_RGBA;
#ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
//        if (TextureFmt != internalFormat || TextureData != datatype)
#endif
    }
    else if (pim->Format == GImage::Image_RGB_888)
        internalFormat  = GL_RGB;
    else if (pim->Format == GImage::Image_A_8)
    {
#ifdef GFC_GL_NO_ALPHA_TEXTURES
        convert         = 1;
        internalFormat  = GL_RGBA;
# ifdef GFC_OS_PS3
        datatype        = GL_UNSIGNED_INT_8_8_8_8;
# endif
#else
        internalFormat = GL_ALPHA;
#endif
    }

    glBindTexture(GL_TEXTURE_2D, TextureId);

#if (defined(GL_UNPACK_ROW_LENGTH) && defined(GL_UNPACK_ALIGNMENT))
    if (!convert && pim->Pitch == pim->Width * pim->GetBytesPerPixel())
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pim->Width);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (int i = 0; i < n; i++)
            glTexSubImage2D(GL_TEXTURE_2D, level,
                rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
                internalFormat, datatype, pim->pData + pim->Pitch * rects[i].src.Top + pim->GetBytesPerPixel() * rects[i].src.Left);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    else if (!convert && pim->Pitch == ((3 + pim->Width * pim->GetBytesPerPixel()) & ~3))
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pim->Width);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        for (int i = 0; i < n; i++)
            glTexSubImage2D(GL_TEXTURE_2D, level,
                rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
                internalFormat, datatype, pim->pData + pim->Pitch * rects[i].src.Top + pim->GetBytesPerPixel() * rects[i].src.Left);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    else
#endif
    if (convert) // convert alpha to rgba
    {
        int s = rects[0].src.Width() * rects[0].src.Height();
        for (int i = 0; i < n; i++)
            if (rects[i].src.Width() * rects[i].src.Height() > s)
                s = rects[i].src.Width() * rects[i].src.Height();
        UByte *pdata = (UByte*)GALLOC(s * 4);

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < rects[i].src.Height(); j++)
                for (int k = 0; k < rects[i].src.Width(); k++)
                {
                    pdata[k*4+j*rects[i].src.Width()*4+0] = 255;
                    pdata[k*4+j*rects[i].src.Width()*4+1] = 255;
                    pdata[k*4+j*rects[i].src.Width()*4+2] = 255;
                    pdata[k*4+j*rects[i].src.Width()*4+3] =
                        pim->pData[pim->Pitch * (j + rects[i].src.Top) +
                                   pim->GetBytesPerPixel() * (k + rects[i].src.Left)];
                }

            glTexSubImage2D(GL_TEXTURE_2D, level,
                            rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
                            internalFormat, datatype, pdata);
        }

        GFREE(pdata);
    }
    else
    {
        int s = rects[0].src.Width() * rects[0].src.Height();
        for (int i = 0; i < n; i++)
            if (rects[i].src.Width() * rects[i].src.Height() > s)
                s = rects[i].src.Width() * rects[i].src.Height();
        UByte *pdata = (UByte*)GALLOC(s * pim->GetBytesPerPixel());

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < rects[i].src.Height(); j++)
                memcpy(pdata + j * rects[i].src.Width() * pim->GetBytesPerPixel(),
                   pim->pData + pim->Pitch * (j + rects[i].src.Top) + pim->GetBytesPerPixel() * rects[i].src.Left,
                   rects[i].src.Width() * pim->GetBytesPerPixel());

            glTexSubImage2D(GL_TEXTURE_2D, level,
                rects[i].dest.x, rects[i].dest.y, rects[i].src.Width(), rects[i].src.Height(),
                internalFormat, datatype, pdata);
        }

        GFREE(pdata);
    }

    CallHandlers(ChangeHandler::Event_DataChange);
}



// GTextureOGLImpl implementation

// Creates an OpenGL texture of the specified dst dimensions, from a
// resampled version of the given src image.  Does a bilinear
// resampling to create the dst image.
void    SoftwareResample(
                int bytesPerPixel,
                int srcWidth,
                int srcHeight,
                int srcPitch,
                UByte* psrcData,
                int dstWidth,
                int dstHeight)
{
    UByte* rescaled = 0;

    unsigned   internalFormat = 0;
    unsigned   inputFormat    = 0;
    GLint      dataType       = GL_UNSIGNED_BYTE;

    switch(bytesPerPixel)
    {
    case 3:
#ifdef GFC_OS_PS3
        dataType       = GL_UNSIGNED_INT_8_8_8_8;
        inputFormat    = GL_RGBA;
        internalFormat = GL_RGBA;

        rescaled = (UByte*)GALLOC(dstWidth * dstHeight * 4);

        GRenderer::ResizeImage(&rescaled[0], dstWidth, dstHeight, dstWidth * 4,
                                psrcData,    srcWidth, srcHeight, srcPitch,
                                GRenderer::ResizeRgbToRgba);
#else
        inputFormat    = GL_RGB;
        internalFormat = GL_RGB;
        rescaled = (UByte*)GALLOC(dstWidth * dstHeight * bytesPerPixel);

        GRenderer::ResizeImage(&rescaled[0], dstWidth, dstHeight, dstWidth * 3,
                                psrcData,    srcWidth, srcHeight, srcPitch,
                                GRenderer::ResizeRgbToRgb);
#endif
        break;

    case 4:
        rescaled = (UByte*)GALLOC(dstWidth * dstHeight * bytesPerPixel);
        inputFormat    = GL_RGBA;
        internalFormat = GL_RGBA;
#ifdef GFC_OS_PS3
        dataType       = GL_UNSIGNED_INT_8_8_8_8;
#endif
        GRenderer::ResizeImage(&rescaled[0], dstWidth, dstHeight, dstWidth * 4,
                                psrcData,    srcWidth, srcHeight, srcPitch,
                                GRenderer::ResizeRgbaToRgba);
        break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, dstWidth, dstHeight, 
                 0, inputFormat, dataType, &rescaled[0]);

    GFREE(rescaled);
}


#ifdef GFC_OS_PS3
#include "GRendererOGLImplPS3.cpp"
#elif defined(GL_ARB_fragment_program)
#include "GRendererOGLImplAsm.cpp"
#endif

// Factory.
GRendererOGL*   GRendererOGL::CreateRenderer()
{
#ifdef GFC_OS_PS3
    return new GRendererOGLImplPS3;
#elif defined(GL_ARB_fragment_program)
    return new GRendererOGLImplAsm;
#else
    return new GRendererOGLImpl;
#endif
}

