
float4 mvp[4] : register(c0);
float4 texgen[4] : register(c4);

void VShaderStrip(float4 pos : POSITION,
                  out float4 opos : POSITION,
                  out float2 otc0 : TEXCOORD0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
}

void VShaderGlyph(float4 pos      : POSITION,
          float2 tc0      : TEXCOORD0,
          float4 color    : COLOR0,
          out float4 opos : POSITION,
          out float2 otc0 : TEXCOORD0,
          out float4 ocolor : COLOR0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0 = tc0;
  ocolor = color.bgra;
}

// Edge AA VShaders (pass along color channels)
void VShaderStripXY16iC32(float4 pos        : POSITION,
          float4 color      : COLOR,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float2 otc0   : TEXCOORD0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
  ocolor = color;
}

void VShaderStripXY16iCF32(float4 pos        : POSITION,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1,
          out float2 otc0   : TEXCOORD0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
  ocolor = color;
  ofactor = factor;
}

// Two-texture shader version
void VShaderStripXY16iCF32_T2(float4 pos        : POSITION,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1,
          out float2 otc0   : TEXCOORD0,
          out float2 otc1   : TEXCOORD1)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
  otc1.x = dot(pos, texgen[2]);
  otc1.y = dot(pos, texgen[3]);
  ocolor = color;
  ofactor = factor;
}

void VShaderGlyphSzc(float4 pos      : POSITION,
          float2 tc0      : TEXCOORD0,
          float4 color    : COLOR0,
          out float4 opos : POSITION,
          out float2 otc0 : TEXCOORD0,
          out float4 ocolor : COLOR0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0 = tc0;
  ocolor = color;
}

void VShaderStripXY16iC32Szc(float4 pos        : POSITION,
          float4 color      : COLOR,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float2 otc0   : TEXCOORD0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
  ocolor = color.bgra;
}

void VShaderStripXY16iCF32Szc(float4 pos        : POSITION,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1,
          out float2 otc0   : TEXCOORD0)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
  ocolor = color.bgra;
  ofactor = factor.bgra;
}

// Two-texture shader version
void VShaderStripXY16iCF32Szc_T2(float4 pos        : POSITION,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1,
          out float2 otc0   : TEXCOORD0,
          out float2 otc1   : TEXCOORD1)
{
  opos = pos;
  opos.x = dot(pos, mvp[0]);
  opos.y = dot(pos, mvp[1]);
  otc0.x = dot(pos, texgen[0]);
  otc0.y = dot(pos, texgen[1]);
  otc1.x = dot(pos, texgen[2]);
  otc1.y = dot(pos, texgen[3]);
  ocolor = color.bgra;
  ofactor = factor.bgra;
}

void VShaderStripXY32fCF32(float4 pos        : POSITION,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1)
{
  opos = pos;
  ocolor = color;
  ofactor = factor;
}

void VShaderStripXYUV32fCF32(float4 pos        : POSITION,
          float2 tc0        : TEXCOORD0,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1,
          out float2 otc0   : TEXCOORD0)
{
  opos = pos;
  ocolor = color;
  ofactor = factor;
  otc0 = tc0;
}

void VShaderStripXYUVUV32fCF32(float4 pos        : POSITION,
          float2 tc0        : TEXCOORD0,
          float2 tc1        : TEXCOORD1,
          float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 opos   : POSITION,
          out float4 ocolor : COLOR,
          out float4 ofactor: COLOR1,
          out float2 otc0   : TEXCOORD0,
          out float2 otc1   : TEXCOORD1)
{
  opos = pos;
  ocolor = color;
  ofactor = factor;
  otc0 = tc0;
  otc1 = tc1;
}

float4 color : register(c0);
float4 cxmul : register(c2);
float4 cxadd : register(c3);
sampler tex0 : register(s0);
sampler tex1 : register(s1);

void PS_SolidColor(out float4 ocolor : COLOR)
{ 
  ocolor = color;
}

void PS_CxformTexture(float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  float4 color = tex2D(tex0, tc0);
  ocolor = color * cxmul + cxadd;
}

void PS_CxformTextureMultiply(float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  float4 color = tex2D(tex0, tc0);
  color = color * cxmul + cxadd;
  ocolor = lerp (1, color, color.a);
}

void PS_TextTextureColor(float2 tc0        : TEXCOORD0,
          float4 color      : COLOR,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  color.a = color.a * tex2D(tex0, tc0).a;
  ocolor = color;
}

void PS_CxformGauraud(float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  ocolor = color;
}

// Same, for Multiply blend version.
void PS_CxformGauraudMultiply(float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  ocolor = lerp (1, color, color.a);
}

// The difference from above is that we don't have separate EdgeAA alpha channel;
// it is instead pre-multiplied into the color alpha (VertexXY16iC32). So we
// don't do an EdgeAA multiply in the end.
void PS_CxformGauraudNoAddAlpha(float4 color      : COLOR,
          out float4 ocolor : COLOR)
{
  ocolor = color * cxmul + cxadd;
}

void PS_CxformGauraudMultiplyNoAddAlpha(float4 color      : COLOR,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  ocolor = lerp (1, color, color.a);
}

void PS_CxformGauraudTexture(float4 color      : COLOR,
          float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  color = lerp (color, tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  ocolor = color;
}

void PS_CxformGauraudMultiplyTexture(float4 color      : COLOR,
          float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  color = lerp (color, tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  ocolor = lerp (1, color, color.a);
}

void PS_Cxform2Texture(float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          float2 tc1        : TEXCOORD1,
          out float4 ocolor : COLOR)
{
  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);
  ocolor = color * cxmul + cxadd;
}

void PS_CxformMultiply2Texture(float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          float2 tc1        : TEXCOORD1,
          out float4 ocolor : COLOR)
{
  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  ocolor = lerp (1, color, color.a);
}

void PS_AcSolidColor(out float4 ocolor : COLOR)
{
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformTexture(float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  float4 color = tex2D(tex0, tc0);
  color = color * cxmul + cxadd;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformTextureMultiply(float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  float4 color = tex2D(tex0, tc0);
  color = color * cxmul + cxadd;
  color = lerp (1, color, color.a);
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcTextTexture(float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  color.a = color.a * tex2D(tex0, tc0).a;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcTextTextureColor(float2 tc0        : TEXCOORD0,
          float4 color      : COLOR,
          out float4 ocolor : COLOR)
{
  color.a = color.a * tex2D(tex0, tc0).a;
  color = color * cxmul + cxadd;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformGauraud(float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

// Same, for Multiply blend version.
void PS_AcCxformGauraudMultiply(float4 color      : COLOR,
          float4 factor     : COLOR1,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  color = lerp (1, color, color.a);
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformGauraudNoAddAlpha(float4 color      : COLOR,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformGauraudMultiplyNoAddAlpha(float4 color      : COLOR,
          out float4 ocolor : COLOR)
{
  color = color * cxmul + cxadd;
  color = lerp (1, color, color.a);
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformGauraudTexture(float4 color      : COLOR,
          float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  color = lerp (color, tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformGauraudMultiplyTexture(float4 color      : COLOR,
          float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          out float4 ocolor : COLOR)
{
  color = lerp (color, tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  color.a = color.a * factor.a;
  color = lerp (1, color, color.a);
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxform2Texture(float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          float2 tc1        : TEXCOORD1,
          out float4 ocolor : COLOR)
{
  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}

void PS_AcCxformMultiply2Texture(float4 factor     : COLOR1,
          float2 tc0        : TEXCOORD0,
          float2 tc1        : TEXCOORD1,
          out float4 ocolor : COLOR)
{
  float4 color = lerp (tex2D(tex1, tc1), tex2D(tex0, tc0), factor.b);
  color = color * cxmul + cxadd;
  color = lerp (1, color, color.a);
  ocolor.rgb = color * color.a;
  ocolor.a = color.a;
}
