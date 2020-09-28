/** 

  Update shader
  
  Sylvain Lefebvre - 2004-07-26 - (c) Microsoft Corp.

  - copy the index buffer into the application buffer. flag patch boundaries
    for faster magnification.
    
  NOTE: patch boundary detection does not work with anisometric synthesis

*/

//-----------------------------------------------------------------------------

// previous pass texture
texture Previous;

// previous pass sampler
sampler S_Previous = sampler_state
{
  Texture   = (Previous);
  MipFilter = None;
  MinFilter = None;
  MagFilter = None;
};

// exemplar texture
texture Exemplar;

// exemplar sampler
sampler S_Exemplar = sampler_state
{
  Texture   = (Exemplar);
  MipFilter = None;
  MinFilter = None;
  MagFilter = None;
};

//-----------------------------------------------------------------------------

// from vertices
struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 TexCoords   : TEXCOORD1;
};

//-----------------------------------------------------------------------------
/*
void get_ij_clr(float2 tc,
                out float2 _ij,
                out float2 _clr)
{
  float4 l=tex2D(S_Previous,tc);
  _ij = l.xy;
  _clr= l.zw;
}
*/
//-----------------------------------------------------------------------------

float4 QuadSize;

float4 WindowPos;

float4 UpdTrl;

float2 BufferSize=float2(256.0,256.0);

float2 InvExemplarRes;

//-----------------------------------------------------------------------------

float4 ps_update(VS_OUTPUT      In) : COLOR
{
/*
  float2 ij;
  float2 clr;
  get_ij_clr( (In.TexCoords + WindowPos.xy) / QuadSize.xy,
              ij,clr);
  return (float4(ij,0.0,1.0));
*/

  float4 ij00_raw=tex2D(S_Previous,(In.TexCoords              ) / QuadSize.xy);
  float2 ij00=ij00_raw.xy;
  
  float2 ij01=tex2D(S_Previous,(In.TexCoords + float2(0,1)) / QuadSize.xy);
  float2 ij11=tex2D(S_Previous,(In.TexCoords + float2(1,1)) / QuadSize.xy);
  float2 ij10=tex2D(S_Previous,(In.TexCoords + float2(1,0)) / QuadSize.xy);

  float2 d0 = frac(ij10 - ij00 - InvExemplarRes.xy);
  float2 d1 = frac(ij01 - ij00 - InvExemplarRes.yx);
  float2 d2 = frac(ij11 - ij00 - InvExemplarRes.xx);
  float   m = max(dot(d0,d0),max(dot(d1,d1),dot(d2,d2)));

//  if (m > 0.8/65536.0) return (float4(ij00_raw.xy,1.0,1.0));
//  else                 return (float4(ij00_raw.xy,0.0,0.0));

  // DEBUG
  return (float4(ij00_raw));

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

  float2 lpos = float2(In.Pos.x,In.Pos.y);
  float2  pos = lpos*WindowPos.zw;
  pos         = float2(pos.x,pos.y) - float2(UpdTrl.x,UpdTrl.y);
  pos         = pos/BufferSize;
  pos         = float2(pos.x,-pos.y)*2.0 + float2(-1.0,1.0);
  pos         = pos + float2(-0.5,0.5)/BufferSize;
  o.Pos       = float4(pos,0.0,1.0);
  o.TexCoords = In.Tex1*WindowPos.zw + WindowPos.xy;

  return (o);
}

//-----------------------------------------------------------------------------

technique t_update
{
  pass P0
  {
    VertexShader = compile vs_2_0 vs_multexcoord();
	PixelShader  = compile ps_3_0 ps_update();
  }  
}

//-----------------------------------------------------------------------------
