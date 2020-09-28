/**********************************************************************

Filename    :   hlsl.cpp
Content     :   Direct3D9 hlsl shaders
Created     :   
Authors     :   Michael Antonov & Andrew Reisse

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define GRENDERER_VSHADER_PROFILE "vs_1_1"
#define GRENDERER_PSHADER_PROFILE "ps_1_1"

static const char* pStripVShaderText =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[2] : register(c4);\n"
"void main(float4 pos      : POSITION,\n"
"          out float4 opos : POSITION,\n"
"          out float2 otc0 : TEXCOORD0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"}\n"
;

static const char* pGlyphVShaderText =
"float4 mvp[4] : register(c0);\n"
"void main(float4 pos      : POSITION,\n"
"          float2 tc0      : TEXCOORD0,\n"
"          float4 color    : COLOR0,\n"
"          out float4 opos : POSITION,\n"
"          out float2 otc0 : TEXCOORD0,\n"
"          out float4 ocolor : COLOR0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0 = tc0;\n"
"  ocolor = color.bgra;\n"
"}\n"
;

// Edge AA VShaders (pass along color channels)
static const char* pStripVShaderXY16iC32Text =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[2] : register(c4);\n"
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float2 otc0   : TEXCOORD0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"  ocolor = color;\n"
"}\n"
;
static const char* pStripVShaderXY16iCF32Text =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[2] : register(c4);\n"
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"  ocolor = color;\n"
"  ofactor = factor;\n"
"}\n"
;
// Two-texture shader version
static const char* pStripVShaderXY16iCF32_T2Text =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[4] : register(c4);\n"
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0,\n"
"          out float2 otc1   : TEXCOORD1)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"  otc1.x = dot(pos, texgen[2]);\n"
"  otc1.y = dot(pos, texgen[3]);\n"
"  ocolor = color;\n"
"  ofactor = factor;\n"
"}\n"
;

static const char* pGlyphVShaderSzcText =
"float4 mvp[4] : register(c0);\n"
"void main(float4 pos      : POSITION,\n"
"          float2 tc0      : TEXCOORD0,\n"
"          float4 color    : COLOR0,\n"
"          out float4 opos : POSITION,\n"
"          out float2 otc0 : TEXCOORD0,\n"
"          out float4 ocolor : COLOR0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0 = tc0;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pStripVShaderXY16iC32SzcText =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[2] : register(c4);\n"
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float2 otc0   : TEXCOORD0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"  ocolor = color.bgra;\n"
"}\n"
;
static const char* pStripVShaderXY16iCF32SzcText =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[2] : register(c4);\n"
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"  ocolor = color.bgra;\n"
"  ofactor = factor.bgra;\n"
"}\n"
;
// Two-texture shader version
static const char* pStripVShaderXY16iCF32Szc_T2Text =
"float4 mvp[4] : register(c0);\n"
"float4 texgen[4] : register(c4);\n"
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0,\n"
"          out float2 otc1   : TEXCOORD1)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"  otc1.x = dot(pos, texgen[2]);\n"
"  otc1.y = dot(pos, texgen[3]);\n"
"  ocolor = color.bgra;\n"
"  ofactor = factor.bgra;\n"
"}\n"
;

static const char* pStripVShaderXY32fCF32Text =
"void main(float4 pos        : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1)\n"
"{\n"
"  opos = pos;\n"
"  ocolor = color;\n"
"  ofactor = factor;\n"
"}\n"
;

static const char* pStripVShaderXYUV32fCF32Text =
"void main(float4 pos        : POSITION,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0)\n"
"{\n"
"  opos = pos;\n"
"  ocolor = color;\n"
"  ofactor = factor;\n"
"  otc0 = tc0;\n"
"}\n"
;

static const char* pStripVShaderXYUVUV32fCF32Text =
"void main(float4 pos        : POSITION,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 opos   : POSITION,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0,\n"
"          out float2 otc1   : TEXCOORD1)\n"
"{\n"
"  opos = pos;\n"
"  ocolor = color;\n"
"  ofactor = factor;\n"
"  otc0 = tc0;\n"
"  otc1 = tc1;\n"
"}\n"
;


#define GRENDERER_SHADER_VERSION    0x0101

static const char* pSource_PS_SolidColor =
"float4 color : register(c0);\n"
"void main(out float4 ocolor : COLOR)\n"
"{ ocolor = color;\n}\n";

static const char* pSource_PS_CxformTexture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = tex2D(tex0, tc0);\n"
"  ocolor = color * cxmul + cxadd;\n"
"}\n"
;

static const char* pSource_PS_CxformTextureMultiply =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = tex2D(tex0, tc0);\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

static const char* pSource_PS_TextTexture =
"float4 color : register(c0);\n"
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color.a = color.a * tex2D(tex0, tc0).a;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pSource_PS_TextTextureColor =
"sampler tex0 : register(s0);\n"
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color.a = color.a * tex2D(tex0, tc0).a;\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pSource_PS_CxformGauraud =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = color;\n"
"}\n"
;

// Same, for Multiply blend version.
static const char* pSource_PS_CxformGauraudMultiply =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

// The difference from above is that we don't have separate EdgeAA alpha channel;
// it is instead pre-multiplied into the color alpha (VertexXY16iC32). So we
// don't do an EdgeAA multiply in the end.
static const char* pSource_PS_CxformGauraudNoAddAlpha =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  ocolor = color * cxmul + cxadd;\n"
"}\n"
;

static const char* pSource_PS_CxformGauraudMultiplyNoAddAlpha =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

static const char* pSource_PS_CxformGauraudTexture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = lerp (color, tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pSource_PS_CxformGauraudMultiplyTexture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = lerp (color, tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

static const char* pSource_PS_Cxform2Texture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"sampler tex1 : register(s1);\n"
"void main(float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);\n"
"  ocolor = color * cxmul + cxadd;\n"
"}\n"
;

static const char* pSource_PS_CxformMultiply2Texture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"sampler tex1 : register(s1);\n"
"void main(float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;


static const char* pSource_PS_AcSolidColor =
"float4 color : register(c0);\n"
"void main(out float4 ocolor : COLOR)\n"
"{\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformTexture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = tex2D(tex0, tc0);\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformTextureMultiply =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = tex2D(tex0, tc0);\n"
"  color = color * cxmul + cxadd;\n"
"  color = lerp (1, color, color.a);\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcTextTexture =
"float4 color : register(c0);\n"
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color.a = color.a * tex2D(tex0, tc0).a;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcTextTextureColor =
"sampler tex0 : register(s0);\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color.a = color.a * tex2D(tex0, tc0).a;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformGauraud =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

// Same, for Multiply blend version.
static const char* pSource_PS_AcCxformGauraudMultiply =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  color = lerp (1, color, color.a);\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformGauraudNoAddAlpha =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformGauraudMultiplyNoAddAlpha =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"void main(float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  color = lerp (1, color, color.a);\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformGauraudTexture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = lerp (color, tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformGauraudMultiplyTexture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  color = lerp (color, tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  color = lerp (1, color, color.a);\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxform2Texture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"sampler tex1 : register(s1);\n"
"void main(float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;

static const char* pSource_PS_AcCxformMultiply2Texture =
"float4 cxmul : register(c2);\n"
"float4 cxadd : register(c3);\n"
"sampler tex0 : register(s0);\n"
"sampler tex1 : register(s1);\n"
"void main(float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          out float4 ocolor : COLOR)\n"
"{\n"
"  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color = lerp (1, color, color.a);\n"
"  ocolor.rgb = color * color.a;\n"
"  ocolor.a = color.a;\n"
"}\n"
;
