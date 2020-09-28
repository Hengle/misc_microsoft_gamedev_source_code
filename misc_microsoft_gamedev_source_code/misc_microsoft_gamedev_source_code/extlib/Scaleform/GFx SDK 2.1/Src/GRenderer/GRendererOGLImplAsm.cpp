/**********************************************************************

Filename    :   GRendererOGLImplAsm.cpp
Content     :   OpenGL renderer implementation - ARB_fragment_program
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


static const char *p1VS_CFTex = 
"!!ARBvp1.0\n"
"OPTION ARB_position_invariant;\n"
"MOV    result.color, vertex.color;\n"
"MOV    result.color.secondary, vertex.attrib[4];\n"
"DP4    result.texcoord[0].x, vertex.position, vertex.attrib[1];\n"
"DP4    result.texcoord[0].y, vertex.position, vertex.attrib[2];\n"
"MOV    result.texcoord[2], vertex.texcoord[2];\n"
"MOV    result.texcoord[3], vertex.texcoord[3];\n"
"END\n";

static const char *p1VS_CFTex2 = 
"!!ARBvp1.0\n"
"OPTION ARB_position_invariant;\n"
"MOV    result.color, vertex.color;\n"
"MOV    result.color.secondary, vertex.attrib[4];\n"
"DP4    result.texcoord[0].x, vertex.position, vertex.attrib[1];\n"
"DP4    result.texcoord[0].y, vertex.position, vertex.attrib[2];\n"
"DP4    result.texcoord[1].x, vertex.position, vertex.attrib[6];\n"
"DP4    result.texcoord[1].y, vertex.position, vertex.attrib[7];\n"
"MOV    result.texcoord[2], vertex.texcoord[2];\n"
"MOV    result.texcoord[3], vertex.texcoord[3];\n"
"END\n";

static const char *p1PS_TextTexture = 
"!!ARBfp1.0\n"
"TEMP texr;\n"
"TEX    texr, fragment.texcoord[0], texture[0], 2D;\n"
"MUL    texr.a, fragment.color.a, texr.a;\n"
"MOV    texr.rgb, fragment.color.bgra;\n"
"MAD    result.color, texr, fragment.texcoord[2], fragment.texcoord[3];\n"
"END\n";

static const char *p1PS_CxformTexture = 
"!!ARBfp1.0\n"
"TEMP texr;\n"
"TEX    texr, fragment.texcoord[0], texture[0], 2D;\n"
"MAD    result.color, texr, fragment.texcoord[2], fragment.texcoord[3];\n"
"END\n";

static const char *p1PS_CxformTextureMultiply = 
"!!ARBfp1.0\n"
"TEMP texr;\n"
"TEX    texr, fragment.texcoord[0], texture[0], 2D;\n"
"MAD    texr, texr, fragment.texcoord[2], fragment.texcoord[3];\n"
"LRP    result.color, texr.a, texr, 1;\n"
"END\n";

static const char *p1PS_CxformGouraud = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"MOV    result.color, t;\n"
"END\n";

static const char *p1PS_CxformGouraudMultiply = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"LRP    result.color, t.a, t, 1;\n"
"END\n";

static const char *p1PS_CxformGouraudNoAddAlpha = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    result.color, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"END\n";

static const char *p1PS_CxformGouraudMultiplyNoAddAlpha = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"LRP    result.color, t.a, t, 1;\n"
"END\n";

static const char *p1PS_CxformGouraudTexture = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"TEX    t, fragment.texcoord[0], texture[0], 2D;\n"
"LRP    t, fragment.color.secondary.b, t, fragment.color;\n"
"MAD    t, t, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    result.color.a, t.a, fragment.color.secondary.a;\n"
"MOV    result.color.rgb, t;\n"
"END\n";

static const char *p1PS_CxformGouraudMultiplyTexture = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"TEX    t, fragment.texcoord[0], texture[0], 2D;\n"
"LRP    t, fragment.color.secondary.b, t, fragment.color;\n"
"MAD    t, t, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"LRP    result.color, t.a, t, 1;\n"
"END\n";

static const char *p1PS_Cxform2Texture = 
"!!ARBfp1.0\n"
"TEMP   t1, t2;\n"
"TEX    t1, fragment.texcoord[0], texture[0], 2D;\n"
"TEX    t2, fragment.texcoord[1], texture[1], 2D;\n"
"LRP    t1, fragment.color.secondary.b, t1, t2;\n"
"MAD    result.color, t1, fragment.texcoord[2], fragment.texcoord[3];\n"
"END\n";

static const char *p1PS_CxformMultiply2Texture = 
"!!ARBfp1.0\n"
"TEMP   t1, t2;\n"
"TEX    t1, fragment.texcoord[0], texture[0], 2D;\n"
"TEX    t2, fragment.texcoord[1], texture[1], 2D;\n"
"LRP    t1, fragment.color.secondary.b, t1, t2;\n"
"MAD    t1, t1, fragment.texcoord[2], fragment.texcoord[3];\n"
"LRP    result.color, t1.a, t1, 1;\n"
"END\n";

// alpha compositing

static const char *p1PS_AcTextTexture = 
"!!ARBfp1.0\n"
"TEMP texr;\n"
"TEX    texr, fragment.texcoord[0], texture[0], 2D;\n"
"MUL    texr.a, fragment.color.a, texr.a;\n"
"MOV    texr.rgb, fragment.color.bgra;\n"
"MAD    texr, texr, fragment.texcoord[2], fragment.texcoord[3];\n"
"MOV    result.color.a, texr.a;\n"
"MUL    result.color.rgb, texr, texr.a;\n"
"END\n";

static const char *p1PS_AcCxformTexture = 
"!!ARBfp1.0\n"
"TEMP texr;\n"
"TEX    texr, fragment.texcoord[0], texture[0], 2D;\n"
"MAD    texr, texr, fragment.texcoord[2], fragment.texcoord[3];\n"
"MOV    result.color.a, texr.a;\n"
"MUL    result.color.rgb, texr, texr.a;\n"
"END\n";

static const char *p1PS_AcCxformTextureMultiply = 
"!!ARBfp1.0\n"
"TEMP texr;\n"
"TEX    texr, fragment.texcoord[0], texture[0], 2D;\n"
"MAD    texr, texr, fragment.texcoord[2], fragment.texcoord[3];\n"
"LRP    texr, texr.a, texr, 1;\n"
"END\n";

static const char *p1PS_AcCxformGouraud = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"MOV    result.color.a, t.a;\n"
"MUL    result.color.rgb, t, t.a;\n"
"END\n";

static const char *p1PS_AcCxformGouraudMultiply = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"LRP    t, t.a, t, 1;\n"
"END\n";

static const char *p1PS_AcCxformGouraudNoAddAlpha = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"MOV    result.color.a, t.a;\n"
"MUL    result.color.rgb, t, t.a;\n"
"END\n";

static const char *p1PS_AcCxformGouraudMultiplyNoAddAlpha = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"MAD    t, fragment.color, fragment.texcoord[2], fragment.texcoord[3];\n"
"LRP    t, t.a, t, 1;\n"
"END\n";

static const char *p1PS_AcCxformGouraudTexture = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"TEX    t, fragment.texcoord[0], texture[0], 2D;\n"
"LRP    t, fragment.color.secondary.b, t, fragment.color;\n"
"MAD    t, t, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"MUL    t.rgb, t, t.a;\n"
"MOV    result.color, t;\n"
"END\n";

static const char *p1PS_AcCxformGouraudMultiplyTexture = 
"!!ARBfp1.0\n"
"TEMP   t;\n"
"TEX    t, fragment.texcoord[0], texture[0], 2D;\n"
"LRP    t, fragment.color.secondary.b, t, fragment.color;\n"
"MAD    t, t, fragment.texcoord[2], fragment.texcoord[3];\n"
"MUL    t.a, t.a, fragment.color.secondary.a;\n"
"LRP    t, t.a, t, 1;\n"
"MOV    result.color.a, t.a;\n"
"MUL    result.color.rgb, t, t.a;\n"
"END\n";

static const char *p1PS_AcCxform2Texture = 
"!!ARBfp1.0\n"
"TEMP   t1, t2;\n"
"TEX    t1, fragment.texcoord[0], texture[0], 2D;\n"
"TEX    t2, fragment.texcoord[1], texture[1], 2D;\n"
"LRP    t1, fragment.color.secondary.b, t1, t2;\n"
"MAD    t1, t1, fragment.texcoord[2], fragment.texcoord[3];\n"
"MOV    result.color.a, t1.a;\n"
"MUL    result.color.rgb, t1, t1.a;\n"
"END\n";

static const char *p1PS_AcCxformMultiply2Texture = 
"!!ARBfp1.0\n"
"TEMP   t1, t2;\n"
"TEX    t1, fragment.texcoord[0], texture[0], 2D;\n"
"TEX    t2, fragment.texcoord[1], texture[1], 2D;\n"
"LRP    t1, fragment.color.secondary.b, t1, t2;\n"
"MAD    t1, t1, fragment.texcoord[2], fragment.texcoord[3];\n"
"LRP    t1, t1.a, t1, 1;\n"
"MOV    result.color.a, t1.a;\n"
"MUL    result.color.rgb, t1, t1.a;\n"
"END\n";


class GRendererOGLImplAsm : public GRendererOGLImpl
{
    GLuint      VScftex[2];
    GLuint      FShaders[PS_Count];

public:
    GRendererOGLImplAsm()
    {
        VScftex[0] = 0;
    }

    virtual void DisableShaders ()
    {
        if (UseShaders)
        {
            glDisable(GL_FRAGMENT_PROGRAM_ARB);
            glDisable(GL_VERTEX_PROGRAM_ARB);
        }
    }

    virtual void SetPixelShader (PixelShaderType psin, SInt pass = 0)
    {
        GUNUSED(pass);
        int ps = psin;
        if ((UseAcBlend == 0) && (RenderMode & GViewport::View_AlphaComposite))
            ps += int(PS_AcOffset);
        CurrentShader = ps;
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FShaders[ps]);
    }

    virtual bool SetVertexProgram (VertexFormat vf, SInt numtex)
    {
        if (vf == Vertex_XY16iCF32)
        {
            glEnable(GL_VERTEX_PROGRAM_ARB);
            glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VScftex[numtex-1]);
            return 1;
        }
        return 0;
    }

    virtual void VertexAttribArray (SInt attr, GLint size, GLenum type, GLboolean norm, GLsizei stride, GLvoid* array)
    {
        GUNUSED(attr);
        glEnableVertexAttribArrayARB(4);
        glVertexAttribPointerARB(4, size, type, norm, stride, array);
    }

    virtual void DisableVertexAttribArrays ()
    {
        if (UseShaders)
            glDisableVertexAttribArrayARB(4);
    }

    virtual void SetTexgenState (SInt stageIndex, const FillTexture& fill)
    {
        GTextureOGLImpl* ptexture = ((GTextureOGLImpl*)fill.pTexture);
        Float   invWidth = 1.0f / ptexture->Width;
        Float   invHeight = 1.0f / ptexture->Height;
        const GRenderer::Matrix&    m = fill.TextureMatrix;
        Float   p[4] = { 0, 0, 0, 0 };

        GASSERT(stageIndex < 3);
        p[0] = m.M_[0][0] * invWidth;
        p[1] = m.M_[0][1] * invWidth;
        p[3] = m.M_[0][2] * invWidth;
        glVertexAttrib4fARB(stageIndex*5 + 1, p[0],p[1],p[2],p[3]);
        p[0] = m.M_[1][0] * invHeight;
        p[1] = m.M_[1][1] * invHeight;
        p[3] = m.M_[1][2] * invHeight;
        glVertexAttrib4fARB(stageIndex*5 + 2, p[0],p[1],p[2],p[3]);
    }

    virtual void    ApplyPShaderCxform(const Cxform &cxform) const
    {
        Float   scm[4] = { cxform.M_[0][0], cxform.M_[1][0],
            cxform.M_[2][0], cxform.M_[3][0] };
        Float   sca[4] = { cxform.M_[0][1] / 255.0f, cxform.M_[1][1] / 255.0f,
            cxform.M_[2][1] / 255.0f, cxform.M_[3][1] / 255.0f };

        // store cxform in texcoords for convenience
        glMultiTexCoord4fv (GL_TEXTURE2, scm);
        glMultiTexCoord4fv (GL_TEXTURE3, sca);
    }

    virtual void InitFragShaderAsm (int i, const char *src)
    {
        glGenProgramsARB(1, &FShaders[i]);
        glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, FShaders[i]);
        glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(strlen(src)), src);
        GLint err;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
        if (err != -1)
        {
            GFC_DEBUG_WARNING(1, const_cast<char*>(src));
            GFC_DEBUG_WARNING(1, (char *) glGetString (GL_PROGRAM_ERROR_STRING_ARB));
        }
    }

    ~GRendererOGLImplAsm()
    {
        Clear();
    }

    void Clear()
    {
        if (UseShaders)
        {
            glDeleteProgramsARB(2, VScftex);
            glDeleteProgramsARB(PS_Count, FShaders);
        }
        UseShaders = false;
    }
    

    void ReleaseResources()
    {
        Clear();
        GRendererOGLImpl::ReleaseResources();
    }

    virtual bool                SetDependentVideoMode()
    {
        if (!GRendererOGLImpl::SetDependentVideoMode())
            return 0;

        // Already initialized, return
        if (VScftex[0])
          return 1;

        const char *glexts = (const char *) glGetString(GL_EXTENSIONS);

        if (CheckExtension(glexts, "GL_ARB_vertex_program") && CheckExtension(glexts, "GL_ARB_fragment_program"))
        {
            glGenProgramsARB(2, VScftex);
            glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VScftex[0]);
            glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(strlen(p1VS_CFTex)), p1VS_CFTex);
            GLint err;
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
            if (err != -1)
            {
                GFC_DEBUG_WARNING(1, (char *) glGetString (GL_PROGRAM_ERROR_STRING_ARB));
            }
            glBindProgramARB(GL_VERTEX_PROGRAM_ARB, VScftex[1]);
            glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(strlen(p1VS_CFTex2)), p1VS_CFTex2);
            glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
            if (err != -1)
            {
                GFC_DEBUG_WARNING(1, (char *) glGetString (GL_PROGRAM_ERROR_STRING_ARB));
            }

            InitFragShaderAsm(PS_TextTexture, p1PS_TextTexture);
            InitFragShaderAsm(PS_CxformTexture, p1PS_CxformTexture);
            InitFragShaderAsm(PS_CxformTextureMultiply, p1PS_CxformTextureMultiply);
            InitFragShaderAsm(PS_CxformGouraud, p1PS_CxformGouraud);
            InitFragShaderAsm(PS_CxformGouraudMultiply, p1PS_CxformGouraudMultiply);
            InitFragShaderAsm(PS_CxformGouraudNoAddAlpha, p1PS_CxformGouraudNoAddAlpha);
            InitFragShaderAsm(PS_CxformGouraudMultiplyNoAddAlpha, p1PS_CxformGouraudMultiplyNoAddAlpha);
            InitFragShaderAsm(PS_CxformGouraudTexture, p1PS_CxformGouraudTexture);
            InitFragShaderAsm(PS_CxformGouraudMultiplyTexture, p1PS_CxformGouraudMultiplyTexture);
            InitFragShaderAsm(PS_Cxform2Texture, p1PS_Cxform2Texture);
            InitFragShaderAsm(PS_CxformMultiply2Texture, p1PS_CxformMultiply2Texture);

            if (!UseAcBlend)
            {
            InitFragShaderAsm(PS_AcTextTexture, p1PS_AcTextTexture);
            InitFragShaderAsm(PS_AcCxformTexture, p1PS_AcCxformTexture);
            InitFragShaderAsm(PS_AcCxformTextureMultiply, p1PS_AcCxformTextureMultiply);
            InitFragShaderAsm(PS_AcCxformGouraud, p1PS_AcCxformGouraud);
            InitFragShaderAsm(PS_AcCxformGouraudMultiply, p1PS_AcCxformGouraudMultiply);
            InitFragShaderAsm(PS_AcCxformGouraudNoAddAlpha, p1PS_AcCxformGouraudNoAddAlpha);
            InitFragShaderAsm(PS_AcCxformGouraudMultiplyNoAddAlpha, p1PS_AcCxformGouraudMultiplyNoAddAlpha);
            InitFragShaderAsm(PS_AcCxformGouraudTexture, p1PS_AcCxformGouraudTexture);
            InitFragShaderAsm(PS_AcCxformGouraudMultiplyTexture, p1PS_AcCxformGouraudMultiplyTexture);
            InitFragShaderAsm(PS_AcCxform2Texture, p1PS_AcCxform2Texture);
            InitFragShaderAsm(PS_AcCxformMultiply2Texture, p1PS_AcCxformMultiply2Texture);
            }

            UseShaders = 1;
            UseCombine = 0;
        }

        return 1;
    }
};
