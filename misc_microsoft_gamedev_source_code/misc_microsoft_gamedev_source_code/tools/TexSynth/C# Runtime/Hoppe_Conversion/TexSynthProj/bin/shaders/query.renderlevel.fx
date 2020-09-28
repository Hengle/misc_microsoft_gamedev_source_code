/** 

  Shader to retrieve level data.
  
  Used by D3DSynthesizer::SynthesisQuery::retrieveLevelIndexMap

  Sylvain Lefebvre 2005-10-27

*/

//-----------------------------------------------------------------------------

// buffer
texture SynthesisBuffer;

// buffer sampler
sampler S_SynthesisBuffer = sampler_state
{
  Texture   = <SynthesisBuffer>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
};


//-----------------------------------------------------------------------------

float4 WorkBufferSize;
float4 SrcWindow;
float4 DstWindow;
float4 SkipBorderTrsfrm;

//-----------------------------------------------------------------------------

struct VS_INPUT
{
    float4 Pos       : POSITION;
    float2 Tex       : TEXCOORD1;
};

struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 Tex         : TEXCOORD1;
  float2 Src         : TEXCOORD2;
};

//-----------------------------------------------------------------------------

float4 ps_renderlevel_no_indirection(VS_OUTPUT In) : COLOR
{
  float2 tc = In.Src;
  return (tex2D(S_SynthesisBuffer,tc));
}


//-----------------------------------------------------------------------------

VS_OUTPUT vs_renderlevel(VS_INPUT In)
{
  VS_OUTPUT o;
  
  o.Pos       = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0)/DstWindow.zw ,0.0,1.0);
  o.Tex       = In.Tex;
  o.Src       = (In.Tex * SrcWindow.zw + SrcWindow.xy) * WorkBufferSize.zw;

  return (o);
}

//-----------------------------------------------------------------------------

technique t_renderlevel_no_indirection
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_renderlevel();
    PixelShader  = compile ps_2_0 ps_renderlevel_no_indirection();
  }  
}


//-----------------------------------------------------------------------------
