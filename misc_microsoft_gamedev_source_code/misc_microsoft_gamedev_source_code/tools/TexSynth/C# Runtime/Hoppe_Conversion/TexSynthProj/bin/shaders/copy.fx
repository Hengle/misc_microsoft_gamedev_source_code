/** 

  Copy from source window to destination window
  Parameters are 'host' buffer sizes and windows coordinates
  
  NOT intended to do resampling !! Make sure SrcWindow.zw == DstWindow.zw
  
  Sylvain Lefebvre 2004-08-03

*/

//-----------------------------------------------------------------------------

// texture
texture Tex;

// texture sampler
sampler S_Tex = sampler_state
{
  Texture   = <Tex>;
  MipFilter = None;
  MinFilter = POINT;
  MagFilter = POINT;
};

float4 SrcWindow;
float4 DstWindow;
float4 SrcBufferSize; // xy = size    zw = 1/size
float4 DstBufferSize;

//-----------------------------------------------------------------------------

// from vertices
struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 Tex1        : TEXCOORD1;
};

//-----------------------------------------------------------------------------

float4 ps_copy(VS_OUTPUT In) : COLOR
{
  float2 tc     = In.Tex1;
  return (tex2D( S_Tex, tc) );
}


//-----------------------------------------------------------------------------


struct VS_INPUT
{
    float4 Pos       : POSITION;
    float2 Tex1      : TEXCOORD1;
};

VS_OUTPUT vs_copy(VS_INPUT In)
{
  VS_OUTPUT o;
  
  o.Pos       = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0)/DstWindow.zw ,0.0,1.0);
  o.Tex1      = (In.Tex1 * SrcWindow.zw + SrcWindow.xy) * SrcBufferSize.zw;

  return (o);
}

//-----------------------------------------------------------------------------

technique t_copy
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_copy();
    PixelShader  = compile ps_2_0 ps_copy();
  }  
}

//-----------------------------------------------------------------------------
