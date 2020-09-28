/**********************************************************************

Filename    :   hlsl.cpp
Content     :   Direct3D10 hlsl shaders
Created     :   
Authors     :   Michael Antonov & Andrew Reisse

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define GRENDERER_VSHADER_PROFILE "vs_4_0"
#define GRENDERER_PSHADER_PROFILE "ps_4_0"

static const char* pStripVShaderText =
"cbuffer VSConstants\n"
"{\n"
"  float4 mvp[4];\n"
"  float4 texgen[2];\n"
"}\n"
"void main(int4 pos        : POSITION,\n"
"          out float2 otc0 : TEXCOORD0,\n"
"          out float4 opos : SV_Position)\n"
"{\n"
"  opos = pos;\n"
"  opos.x = dot(pos, mvp[0]);\n"
"  opos.y = dot(pos, mvp[1]);\n"
"  otc0.x = dot(pos, texgen[0]);\n"
"  otc0.y = dot(pos, texgen[1]);\n"
"}\n"
;

static const char* pGlyphVShaderText =
"cbuffer VSConstants\n"
"{\n"
"  float4 mvp[4];\n"
"}\n"
"void main(float4 pos      : POSITION,\n"
"          float2 tc0      : TEXCOORD0,\n"
"          float4 color    : COLOR0,\n"
"          out float4 ocolor : COLOR0,\n"
"          out float2 otc0 : TEXCOORD0,\n"
"          out float4 opos : SV_Position)\n"
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
"cbuffer VSConstants\n"
"{\n"
"  float4 mvp[4];\n"
"  float4 texgen[2];\n"
"}\n"
"void main(int4 pos          : POSITION,\n"
"          float4 color      : COLOR,\n"
"          out float4 ocolor : COLOR,\n"
"          out float2 otc0   : TEXCOORD0,\n"
"          out float4 opos   : SV_Position)\n"
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
"cbuffer VSConstants\n"
"{\n"
"  float4 mvp[4];\n"
"  float4 texgen[2];\n"
"}\n"
"void main(int4 pos          : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0,\n"
"          out float4 opos   : SV_Position)\n"
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
"cbuffer VSConstants\n"
"{\n"
"  float4 mvp[4];\n"
"  float4 texgen[4];\n"
"}\n"
"void main(int4 pos          : POSITION,\n"
"          float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : COLOR,\n"
"          out float4 ofactor: COLOR1,\n"
"          out float2 otc0   : TEXCOORD0,\n"
"          out float2 otc1   : TEXCOORD1,\n"
"          out float4 opos   : SV_Position)\n"
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


#define GRENDERER_SHADER_VERSION    0x0101

static const char* pSource_PS_SolidColor =
"cbuffer PSConstants\n"
"{\n"
"  float4 color;\n"
"}\n"
"void main(out float4 ocolor : SV_Target)\n"
"{ ocolor = color;\n}\n";

static const char* pSource_PS_CxformTexture =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"sampler samp;\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  float4 color = tex0.Sample(samp, tc0);\n"
"  ocolor = color * cxmul + cxadd;\n"
"}\n"
;

static const char* pSource_PS_CxformTextureMultiply =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"sampler samp;\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  float4 color = tex0.Sample(samp, tc0);\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

static const char* pSource_PS_TextTexture =
"cbuffer PSConstants\n"
"{\n"
"  float4 color;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"sampler samp;\n"
"void main(float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  color.a = color.a * tex0.Sample(samp, tc0).a;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pSource_PS_TextTextureColor =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0 : register(t0);\n"
"sampler samp : register(s0);\n"
"void main(float4 color      : COLOR,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  color.a = color.a * tex0.Sample(samp, tc0).a;\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pSource_PS_CxformGauraud =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = color;\n"
"}\n"
;

// Same, for Multiply blend version.
static const char* pSource_PS_CxformGauraudMultiply =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          out float4 ocolor : SV_Target)\n"
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
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"void main(float4 color      : COLOR,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  ocolor = color * cxmul + cxadd;\n"
"}\n"
;

static const char* pSource_PS_CxformGauraudMultiplyNoAddAlpha =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"void main(float4 color      : COLOR,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

static const char* pSource_PS_CxformGauraudTexture =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"sampler samp;\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  color = lerp (color, tex0.Sample(samp, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = color;\n"
"}\n"
;

static const char* pSource_PS_CxformGauraudMultiplyTexture =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"sampler samp;\n"
"void main(float4 color      : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  color = lerp (color, tex0.Sample(samp, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  color.a = color.a * factor.a;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;

static const char* pSource_PS_Cxform2Texture =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"Texture2D tex1;\n"
"sampler samp;\n"
"void main(float4 color0     : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  float4 color = lerp (tex1.Sample(samp, tc1), tex0.Sample(samp, tc0), factor.b);\n"
"  ocolor = color * cxmul + cxadd;\n"
"}\n"
;

static const char* pSource_PS_CxformMultiply2Texture =
"cbuffer PSConstants\n"
"{\n"
"  float4 cxmul;\n"
"  float4 cxadd;\n"
"}\n"
"Texture2D tex0;\n"
"Texture2D tex1;\n"
"sampler samp;\n"
"void main(float4 color0     : COLOR,\n"
"          float4 factor     : COLOR1,\n"
"          float2 tc0        : TEXCOORD0,\n"
"          float2 tc1        : TEXCOORD1,\n"
"          out float4 ocolor : SV_Target)\n"
"{\n"
"  float4 color = lerp (tex1.Sample(samp, tc1), tex0.Sample(samp, tc0), factor.b);\n"
"  color = color * cxmul + cxadd;\n"
"  ocolor = lerp (1, color, color.a);\n"
"}\n"
;
