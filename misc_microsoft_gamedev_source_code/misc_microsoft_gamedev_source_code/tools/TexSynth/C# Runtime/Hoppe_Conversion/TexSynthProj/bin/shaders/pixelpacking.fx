/** 

  Tools for pixel packing
  
  Sylvain Lefebvre 2004-07-03

*/

//-----------------------------------------------------------------------------

#include "coordinates.fx"

// buffer
texture Buffer;

// exemplar sampler
sampler S_Buffer = sampler_state
{
  Texture   = <Buffer>;
  MipFilter = None;
  MinFilter = Point;
  MagFilter = Point;
};

// Window pos (x,y,w,h)
float4 WindowPos;

// Buffer quad
float4 BufferQuad;

// Region to be unpacked
float4 Region;

//-----------------------------------------------------------------------------

// from vertices
struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 Tex1        : TEXCOORD1;
  float2 TexNrm      : TEXCOORD2;
};

#define fmodp(x,n) ((n)*frac((x)/(n)))

//-----------------------------------------------------------------------------

float4 ps_pixelpacking(VS_OUTPUT In) : COLOR
{
  // in which part are we rendering ?
  float2 part   = (In.TexNrm > float2(0.5,0.5)) ? float2(1.0,1.0) : float2(0.0,0.0);

  // must also take into account position of corner
  //float2 corner = fmodp(WindowPos.xy, 2.0);
  //part          = fmod(part+corner , 2.0);

  float2 rij    = fmodp(In.Tex1 , (Region.zw/2.0) );

  float2 pij    = floor(rij) * 2.0 + part;
  
  float4 prev   = tex2D(S_Buffer,(pij*BufferQuad.zw)+(0.5*BufferQuad.zw)); //+0.5 for np2 buffers

  return (prev);
}


//-----------------------------------------------------------------------------


float4 ps_pixelunpacking(VS_OUTPUT In) : COLOR
{
  float2 tc     = In.Tex1;

  float2 rij    = floor(tc);
  
  float4 prev   = tex2D(S_Buffer,pix_unpack2pack(rij,BufferQuad));

  return (prev);
}


//-----------------------------------------------------------------------------


struct VS_INPUT
{
    float4 Pos       : POSITION;
    float2 Tex1      : TEXCOORD1;
};

VS_OUTPUT vs_multexcoord(VS_INPUT In)
{
  VS_OUTPUT o;
  
  o.Pos       = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0)/Region.zw ,0.0,1.0);
  o.Tex1      = In.Tex1*Region.zw + Region.xy;
  o.TexNrm    = In.Tex1;

  return (o);
}

//-----------------------------------------------------------------------------

technique t_pixelpack
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_multexcoord();
    PixelShader  = compile ps_2_0 ps_pixelpacking();
  }  
}

//-----------------------------------------------------------------------------

technique t_pixelunpack
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_multexcoord();
    PixelShader  = compile ps_2_0 ps_pixelunpacking();
  }  
}

//-----------------------------------------------------------------------------
