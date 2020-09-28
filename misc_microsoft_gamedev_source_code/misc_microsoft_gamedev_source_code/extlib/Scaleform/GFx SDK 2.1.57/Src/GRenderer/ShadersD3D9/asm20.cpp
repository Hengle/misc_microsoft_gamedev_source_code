/**********************************************************************

Filename    :   asm20.cpp
Content     :   Direct3D9 asm 2.0 shaders
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
    "vs_1_1\n"
    "dcl_position v0\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    ;

static const char* pGlyphVShaderText =
    "vs_1_1\n"
    "dcl_position v0\n"
    "dcl_texcoord v1\n"
    "dcl_color0 v2\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "mov oT0, v1\n" // copy tex coords
    "mov oD0, v2.bgra\n"
    ;

// Edge AA VShaders (pass along color channels)
static const char* pStripVShaderXY16iC32Text =
    "vs_1_1\n"
    "dcl_position v0\n"
    "dcl_color0 v1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    "mov oD0, v1\n"
    ;
static const char* pStripVShaderXY16iCF32Text =
    "vs_1_1\n"
    "dcl_position v0\n"
    "dcl_color0 v1\n"
    "dcl_color1 v2\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    "mov oD0, v1\n"
    "mov oD1, v2\n"
    ;
// Two-texture shader version
static const char* pStripVShaderXY16iCF32_T2Text =
    "vs_1_1\n"
    "dcl_position v0\n"
    "dcl_color0 v1\n"
    "dcl_color1 v2\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "dp4 oT0.x, v0, c4\n" // texgen
    "dp4 oT0.y, v0, c5\n"
    "dp4 oT1.x, v0, c6\n" // texgen 2
    "dp4 oT1.y, v0, c7\n"
    "mov oD0, v1\n"
    "mov oD1, v2\n"
    ;

static const char* pGlyphVShaderSzcText =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_texcoord v1\n"
"dcl_color0 v2\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"dp4 oPos.z, v0, c2\n"
"dp4 oPos.w, v0, c3\n"
"mov oT0, v1\n" // copy tex coords
"mov oD0, v2\n"
;

static const char* pStripVShaderXY16iC32SzcText =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_color0 v1\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"dp4 oPos.z, v0, c2\n"
"dp4 oPos.w, v0, c3\n"
"dp4 oT0.x, v0, c4\n" // texgen
"dp4 oT0.y, v0, c5\n"
"mov oD0, v1.bgra\n"
;
static const char* pStripVShaderXY16iCF32SzcText =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_color0 v1\n"
"dcl_color1 v2\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"dp4 oPos.z, v0, c2\n"
"dp4 oPos.w, v0, c3\n"
"dp4 oT0.x, v0, c4\n" // texgen
"dp4 oT0.y, v0, c5\n"
"mov oD0, v1.bgra\n"
"mov oD1, v2.bgra\n"
;
// Two-texture shader version
static const char* pStripVShaderXY16iCF32Szc_T2Text =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_color0 v1\n"
"dcl_color1 v2\n"
"dp4 oPos.x, v0, c0\n"
"dp4 oPos.y, v0, c1\n"
"dp4 oPos.z, v0, c2\n"
"dp4 oPos.w, v0, c3\n"
"dp4 oT0.x, v0, c4\n" // texgen
"dp4 oT0.y, v0, c5\n"
"dp4 oT1.x, v0, c6\n" // texgen 2
"dp4 oT1.y, v0, c7\n"
"mov oD0, v1.bgra\n"
"mov oD1, v2.bgra\n"
;

static const char* pStripVShaderXYUVUV32fCF32Text =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_texcoord0 v1\n"
"dcl_texcoord1 v2\n"
"dcl_color0 v3\n"
"dcl_color1 v4\n"
"mov oPos, v0\n"
"mov oT0, v1\n"
"mov oT1, v2\n"
"mov oD0, v3\n"
"mov oD1, v4\n"
;

static const char* pStripVShaderXYUV32fCF32Text =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_texcoord0 v1\n"
"dcl_color0 v2\n"
"dcl_color1 v3\n"
"mov oPos, v0\n"
"mov oT0, v1\n"
"mov oD0, v2\n"
"mov oD1, v3\n"
;

static const char* pStripVShaderXY32fCF32Text =
"vs_1_1\n"
"dcl_position v0\n"
"dcl_color0 v1\n"
"dcl_color1 v2\n"
"mov oPos, v0\n"
"mov oD0, v1\n"
"mov oD1, v2\n"
;

#define GRENDERER_SHADER_VERSION 0x0200

static const char* pSource_PS_SolidColor =
    "ps_2_0\n"
    "mov oC0, c0\n"             // Set to solid color from c0
    ;

static const char* pSource_PS_CxformTexture =
    "ps_2_0\n"
    "dcl t0.xy\n"
    "dcl_2d s0\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_CxformTextureMultiply =
    "ps_2_0\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "def    c4, 1,1,1,1\n"
    
    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "lrp    r1, r0.a, r0, c4\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_TextTextureColor =
    "ps_2_0\n"
    "dcl    t0.xy\n"    
    "dcl_2d s0\n"
    "dcl    v0.rgba\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, v0\n"           // Text color
    "mov    r2, c2\n"
    "mad    r1, r1, r2, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r1.a, r0.a, r1.a\n"
    "mov    oC0, r1\n"          // Modulate alpha - c0
    ;

// *** PS 2.0 Gouraud versions: used with EdgeAA

static const char* pSource_PS_CxformGauraud =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    
    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "mov    oC0, r0\n"
    ;
// Same, for Multiply blend version.
static const char* pSource_PS_CxformGauraudMultiply =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "def    c4, 1,1,1,1\n"
    
    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "lrp    r1, r0.a, r0, c4\n"
    "mov    oC0, r1\n"
    ;

// The difference from above is that we don't have separate EdgeAA alpha channel;
// it is instead pre-multiplied into the color alpha (VertexXY16iC32). So we
// don't do an EdgeAA multiply in the end.
static const char* pSource_PS_CxformGauraudNoAddAlpha =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
        
    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3    
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_CxformGauraudMultiplyNoAddAlpha =
    "ps_2_0\n"
    "dcl    v0.rgba\n"
    "def    c4, 1,1,1,1\n"
        
    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3    
    "lrp    r1, r0.a, r0, c4\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_CxformGauraudTexture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, v1\n"
    "lrp    r0, r1.b, r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_CxformGauraudMultiplyTexture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "def    c4, 1,1,1,1\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, v1\n"
    "lrp    r0, r1.b, r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "lrp    r1, r0.a, r0, c4\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_Cxform2Texture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "dcl    t1.xy\n"
    "dcl_2d s1\n"   
    
    "texld  r0, t0, s0\n"       // Sample texture 0
    "mov    r1, v1\n"
    "texld  r2, t1, s1\n"       // Sample texture 1
    "lrp    r0, r1.b, r0, r2\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3    
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_CxformMultiply2Texture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "dcl    t1.xy\n"
    "dcl_2d s1\n"
    "def    c4, 1,1,1,1\n"
    
    "texld  r0, t0, s0\n"       // Sample texture 0
    "mov    r1, v1\n"
    "texld  r2, t1, s1\n"       // Sample texture 1
    "lrp    r0, r1.b, r0, r2\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "lrp    r1, r0.a, r0, c4\n"
    "mov    oC0, r1\n"
    ;

// Shaders for alpha composite rendering

static const char* pSource_PS_AcSolidColor =
    "ps_2_0\n"
    "mov    r3.a, c0.a\n"
    "mul    r3.rgb, c0, c0.a\n"
    "mov    oC0, r3\n"
    ;

static const char* pSource_PS_AcCxformTexture =
    "ps_2_0\n"
    "dcl t0.xy\n"
    "dcl_2d s0\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.rgb, r0, r0.a\n"
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_AcCxformTextureMultiply =
    "ps_2_0\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "def    c4, 1,1,1,1\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "lrp    r1, r0.a, r0, c4\n"
    "mul    r1.rgb, r1, r1.a\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_AcTextTexture =
    "ps_2_0\n"
    "dcl    t0.xy\n"    
    "dcl_2d s0\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, c0\n"           // Constant color
    "mul    r1.a, r0.a, c0.a\n"
    "mul    r1.rgb, r1, r1.a\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_AcTextTextureColor =
    "ps_2_0\n"
    "dcl    t0.xy\n"    
    "dcl_2d s0\n"
    "dcl    v0.rgba\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, v0\n"           // Text color
    "mul    r1.a, r0.a, v0.a\n"
    "mov    r0, c2\n"
    "mad    r1, r1, r0, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r1.rgb, r1, r1.a\n"
    "mov    oC0, r1\n"          // Modulate alpha - c0
    ;

// *** PS 2.0 Gouraud versions: used with EdgeAA + alpha compositing

static const char* pSource_PS_AcCxformGauraud =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"

    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "mul    r0.rgb, r0, r0.a\n"
    "mov    oC0, r0\n"
    ;
// Same, for Multiply blend version.
static const char* pSource_PS_AcCxformGauraudMultiply =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "def    c4, 1,1,1,1\n"

    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "lrp    r1, r0.a, r0, c4\n"
    "mul    r1.rgb, r1, r1.a\n"
    "mov    oC0, r1\n"
    ;

// The difference from above is that we don't have separate EdgeAA alpha channel;
// it is instead pre-multiplied into the color alpha (VertexXY16iC32). So we
// don't do an EdgeAA multiply in the end.
static const char* pSource_PS_AcCxformGauraudNoAddAlpha =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  

    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3    
    "mul    r0.rgb, r0, r0.a\n"
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_AcCxformGauraudMultiplyNoAddAlpha =
    "ps_2_0\n"
    "dcl    v0.rgba\n"
    "def    c4, 1,1,1,1\n"

    "mov    r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3    
    "lrp    r1, r0.a, r0, c4\n"
    "mul    r1.rgb, r1, r1.a\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_AcCxformGauraudTexture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, v1\n"
    "lrp    r0, r1.b, r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "mul    r0.rgb, r0, r0.a\n"
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_AcCxformGauraudMultiplyTexture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "def    c4, 1,1,1,1\n"

    "texld  r0, t0, s0\n"       // Sample texture
    "mov    r1, v1\n"
    "lrp    r0, r1.b, r0, v0\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "mul    r0.a, r0.a, v1.a\n"
    "lrp    r1, r0.a, r0, c4\n"
    "mul    r1.rgb, r1, r1.a\n"
    "mov    oC0, r1\n"
    ;

static const char* pSource_PS_AcCxform2Texture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "dcl    t1.xy\n"
    "dcl_2d s1\n"   

    "texld  r0, t0, s0\n"       // Sample texture 0
    "mov    r1, v1\n"
    "texld  r2, t1, s1\n"       // Sample texture 1
    "lrp    r0, r1.b, r0, r2\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3    
    "mul    r0.rgb, r0, r0.a\n"
    "mov    oC0, r0\n"
    ;

static const char* pSource_PS_AcCxformMultiply2Texture =
    "ps_2_0\n"
    "dcl    v0.rgba\n"  
    "dcl    v1.rgba\n"
    "dcl    t0.xy\n"
    "dcl_2d s0\n"
    "dcl    t1.xy\n"
    "dcl_2d s1\n"
    "def    c4, 1,1,1,1\n"

    "texld  r0, t0, s0\n"       // Sample texture 0
    "mov    r1, v1\n"
    "texld  r2, t1, s1\n"       // Sample texture 1
    "lrp    r0, r1.b, r0, r2\n"
    "mov    r1, c2\n"
    "mad    r0, r0, r1, c3\n"   // Apply CXForm r0*c2+c3
    "lrp    r1, r0.a, r0, c4\n"
    "mul    r0.rgb, r1, r1.a\n"
    "mov    oC0, r0\n"
    ;
