/**********************************************************************

Filename    :   asm11.cpp
Content     :   Direct3D9 asm 1.1 shaders
Created     :   
Authors     :   Michael Antonov & Andrew Reisse

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


static const char* pStripVShaderText =
    "vs.1.1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"

    "mov oPos.zw, v0.zw\n"
//    "dp4 oPos.z, v0, c2\n"
//    "dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    ;

static const char* pGlyphVShaderText =
    "vs.1.1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "mov oPos.zw, v0.zw\n"
    //"dp4 oPos.z, v0, c2\n"
    //"dp4 oPos.w, v0, c3\n"
    "mov oT0, v7\n" // copy tex coords
    "mov oD0, v5.bgra\n"
    ;

// Edge AA VShaders (pass along color channels)
static const char* pStripVShaderXY16iC32Text =
    "vs.1.1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "mov oPos.zw, v0.zw\n"
    //"dp4 oPos.z, v0, c2\n"
    //"dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    "mov oD0, v5\n"
    ;
static const char* pStripVShaderXY16iCF32Text =
    "vs.1.1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "mov oPos.zw, v0.zw\n"
    //"dp4 oPos.z, v0, c2\n"
    //"dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    "mov oD0, v5\n"
    "mov oD1, v6\n"
    ;
// Two-texture shader version
static const char* pStripVShaderXY16iCF32_T2Text =
    "vs.1.1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "mov oPos.zw, v0.zw\n"
    //"dp4 oPos.z, v0, c2\n"
    //"dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    "dp4 oT1.x, v0, c6\n" // texgen 2
    "dp4 oT1.y, v0, c7\n"
    "mov oD0, v5\n"
    "mov oD1, v6\n"
    ;

static const char* pGlyphVShaderSzcText =
"vs.1.1\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"mov oPos.zw, v0.zw\n"
//"dp4 oPos.z, v0, c2\n"
//"dp4 oPos.w, v0, c3\n"
"mov oT0, v7\n" // copy tex coords
"mov oD0, v5\n"
;

static const char* pStripVShaderXY16iC32SzcText =
"vs.1.1\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"mov oPos.zw, v0.zw\n"
//"dp4 oPos.z, v0, c2\n"
//"dp4 oPos.w, v0, c3\n"
"dp4 oT0.x, v0, c4\n" // texgen
"dp4 oT0.y, v0, c5\n"
"mov oD0, v5.bgra\n"
;
static const char* pStripVShaderXY16iCF32SzcText =
"vs.1.1\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"mov oPos.zw, v0.zw\n"
//"dp4 oPos.z, v0, c2\n"
//"dp4 oPos.w, v0, c3\n"
"dp4 oT0.x, v0, c4\n" // texgen
"dp4 oT0.y, v0, c5\n"
"mov oD0, v5.bgra\n"
"mov oD1, v6.bgra\n"
;
// Two-texture shader version
static const char* pStripVShaderXY16iCF32Szc_T2Text =
"vs.1.1\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"mov oPos.zw, v0.zw\n"
//"dp4 oPos.z, v0, c2\n"
//"dp4 oPos.w, v0, c3\n"
"dp4 oT0.x, v0, c4\n" // texgen
"dp4 oT0.y, v0, c5\n"
"dp4 oT1.x, v0, c6\n" // texgen 2
"dp4 oT1.y, v0, c7\n"
"mov oD0, v5.bgra\n"
"mov oD1, v6.bgra\n"
;

static const char* pStripVShaderXYUVUV32fCF32Text =
    "vs.1.1\n"
    "mov oPos, v0\n"
    "mov oT0, v7\n"
    "mov oT1, v8\n"
    "mov oD0, v5\n"
    "mov oD1, v6\n"
    ;

static const char* pStripVShaderXYUV32fCF32Text =
"vs.1.1\n"
"mov oPos, v0\n"
"mov oT0, v7\n"
"mov oD0, v5\n"
"mov oD1, v6\n"
;

static const char* pStripVShaderXY32fCF32Text =
"vs.1.1\n"
"mov oPos, v0\n"
"mov oD0, v5\n"
"mov oD1, v6\n"
;


#define GRENDERER_SHADER_VERSION    0x0101

// If any shader strings become longer, make sure the buffer in InitShaders is still big enough.

// *** PS 1.1 Non-Gauraud shaders

// These shaders are identical to PS 2.0 versions below.
static const char* pSource_PS_SolidColor =
    "ps.1.1\n"
    "mov r0, c0\n"              // Set to solid color from c0
    ;

static const char* pSource_PS_CxformTexture =
    "ps.1.1\n"  
    "tex    t0\n"               // Sample texture   
    "mad    r0, t0, c2, c3\n"   // Apply CXForm r0*c2+c3
    ;

static const char* pSource_PS_CxformTextureMultiply =
    "ps.1.1\n"  
    "def    c4, 1,1,1,1\n"

    "tex    t0\n"               // Sample texture
    "mad_sat r0, t0, c2, c3\n"   // Apply CXForm r0*c2+c3
    "lrp    r0, r0.a, r0, c4\n"
    ;

static const char* pSource_PS_TextTextureColor =
    "ps.1.1\n"

    "tex    t0\n"               // Sample texture
    "mov    r0, v0\n"
    "mad    r0, r0, c2, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, t0.a, r0.a\n" // Modulate alpha - c0
    ;

// *** PS 1.1 Gouraud versions: used with EdgeAA

static const char* pSource_PS_CxformGauraud =
    "ps.1.1\n"  
    "mad    r0, v0, c2, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    ;
// Same, for Multiply blend version.
static const char* pSource_PS_CxformGauraudMultiply =
    "ps.1.1\n"
    "def    c4, 1,1,1,1\n"

    "mad    r0, v0, c2, c3\n"   // Apply CXForm r0*c2+c3
    "mul_sat r0.a, r0.a, v1.a\n" 
    "lrp    r0, r0.a, r0, c4\n"
    ;

// The difference from above is that we don't have separate EdgeAA alpha channel;
// it is instead pre-multiplied into the color alpha (VertexXY16iC32). So we
// don't do an EdgeAA multiply in the end.
static const char* pSource_PS_CxformGauraudNoAddAlpha =
    "ps.1.1\n"  
    "mad    r0, v0, c2, c3\n"   // Apply CXForm r0*c2+c3
    ;

static const char* pSource_PS_CxformGauraudMultiplyNoAddAlpha =
    "ps.1.1\n"
    "def    c4, 1,1,1,1\n"

    "mad_sat r0, v0, c2, c3\n"   // Apply CXForm r0*c2+c3    
    "lrp    r0, r0.a, r0, c4\n"
    ;

static const char* pSource_PS_CxformGauraudTexture =
    "ps.1.1\n"

    "tex    t0\n"                   // Sample texture   
    "lrp    r0.rgb, v1, t0, v0\n"   // rgb contains mix factor.
    "+lrp   r0.a, v1.b, t0, v0\n"
    "mad    r0, r0, c2, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    ;

static const char* pSource_PS_CxformGauraudMultiplyTexture =
    "ps.1.1\n"
    "def    c4, 1,1,1,1\n"

    "tex    t0\n"                   // Sample texture   
    "lrp    r0.rgb, v1, t0, v0\n"   // rgb contains mix factor.
    "+lrp   r0.a, v1.b, t0, v0\n"
    "mad    r0, r0, c2, c3\n"       // Apply CXForm r0*c2+c3
    "mul_sat r0.a, r0.a, v1.a\n"
    "lrp    r0, r0.a, r0, c4\n"
    ;

static const char* pSource_PS_Cxform2Texture =
    "ps.1.1\n"
    
    "tex    t0\n"                   // Sample textures
    "tex    t1\n"
    "lrp    r0.rgb, v1, t0, t1\n"   // Mix textures.
    "+lrp   r0.a, v1.b, t0, t1\n"
    "mad    r0, r0, c2, c3\n"       // Apply CXForm r0*c2+c3
    ;

static const char* pSource_PS_CxformMultiply2Texture =
    "ps.1.1\n"
    "def    c4, 1,1,1,1\n"
    
    "tex    t0\n"                   // Sample textures
    "tex    t1\n"
    "lrp    r0.rgb, v1, t0, t1\n"   // Mix textures.
    "+lrp   r0.a, v1.b, t0, t1\n"
    "mad_sat r0, r0, c2, c3\n"       // Apply CXForm r0*c2+c3
    "lrp    r0, r0.a, r0, c4\n"
    ;
