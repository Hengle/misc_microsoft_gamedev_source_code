/*
	Init Shader
	Colt "MainRoach" McAnlis - Cleaned up 01.02.07
*/

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

// from vertices
struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 TexCoords   : TEXCOORD0;
  float2 TexCoords1   : TEXCOORD1;
};

//-----------------------------------------------------------------------------

#include "rand.fx"

//-----------------------------------------------------------------------------

// Exemplar size (in pixels)
float ExemplarSize=1.0;

// Random scale
float Scale=1.0;

// Window pos (x,y,w,h)
float4 WindowPos;

// Level size
float LevelSize=1.0;

// Local frames texture resolution
float4 LocalFramesRes;

//-----------------------------------------------------------------------------

#include "output.fx"

#include "coordinates.fx"

#define fmodp(x,n) ((n)*frac((x)/(n)))

//-----------------------------------------------------------------------------

// Init algorithm

float4 ps_init(VS_OUTPUT      In) : COLOR
{  

  float2 tc   = In.TexCoords;
  float2 rij  = tc;
  float2 aij  = floor(relative2absolute(rij,WindowPos));
  
  float2 tij  = aij;

#ifdef TOROIDAL
  float2 rndij = fmodp(rand(fmodp(floor(tij),LocalFramesRes.xy/ExemplarSize),0.0),ExemplarSize) - ExemplarSize/2.0;
  float2 ij    = round(rndij*Scale);
#else
  float2 rndij = fmodp(rand(floor(tij),0.0),ExemplarSize) - ExemplarSize/2.0;
  float2 ij    = round(rndij*Scale);
#endif

  float2 txy       = frac(0.5 + ij / ExemplarSize) + .5 / ExemplarSize; 
                                                  // .5/ExemplarSize only with 
                                                  // stack and non warp
  float2 oxy       = txy;

  return float4(oxy,0,0);

}

//-----------------------------------------------------------------------------

struct VS_INPUT
{
    float4 Pos       : POSITION;
    float2 Tex0      : TEXCOORD0;
    float2 Tex1      : TEXCOORD1;
};

VS_OUTPUT vs_multexcoord(VS_INPUT In)
{
  VS_OUTPUT o;
  
  o.Pos       = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0)/WindowPos.zw ,0.0,1.0);
  o.TexCoords = In.Tex1*WindowPos.zw;
  o.TexCoords1 = In.Tex0;

  return (o);
}

//-----------------------------------------------------------------------------

technique t_init_std
{
  pass P0
  {
    VertexShader = compile vs_3_0 vs_multexcoord();
    PixelShader  = compile ps_3_0 ps_init();
  }  
}

//-----------------------------------------------------------------------------
