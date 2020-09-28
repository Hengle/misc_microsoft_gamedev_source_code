/** 

  Upsampling shader
  
  Sylvain Lefebvre 2004-07-03 - (c) Microsoft Corp.
                   2004-08-03 pixel packing
                   2005-03-08 renamed from Extrapolation to Upsampling
*/

//-----------------------------------------------------------------------------

//#define ANISOSYNTH  // enables anisometric synthesis 
                    // TODO: for perf measurement, do not
                    //       use when not performing aniso syn.

// #define ANISOSYNTH

//-----------------------------------------------------------------------------

#ifdef COMPRESS_CLR
  #define TEX2D_EX(S,uv) tex2Dlod(S,float4(uv + IsLevel0*0.5/ExemplarSize,0,0))
#else
  #define TEX2D_EX(S,uv) tex2D(S,uv)
#endif

//-----------------------------------------------------------------------------

// exemplar constraint texture
texture ExemplarCstr;

// sampler
sampler S_ExemplarCstr = sampler_state
{
  Texture   = <ExemplarCstr>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
};

// previous level texture
texture Previous;

// sampler
sampler S_Previous = sampler_state
{
  Texture   = <Previous>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
};

// constraint map
texture CstrMap;

// sampler
sampler S_CstrMap = sampler_state
{
  Texture   = <CstrMap>;
  MipFilter = Linear;
  MinFilter = Linear;
  MagFilter = Linear;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// postionning map
texture PositionMap0;

// sampler
sampler S_PositionMap0 = sampler_state
{
  Texture   = <PositionMap0>;
  MipFilter = Point;
  MinFilter = POINT;
  MagFilter = POINT;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// postionning map
texture PositionMap1;

// sampler
sampler S_PositionMap1 = sampler_state
{
  Texture   = <PositionMap1>;
  MipFilter = Point;
  MinFilter = POINT;
  MagFilter = POINT;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// exemplar randomness map
texture ExRndMap;

// sampler
sampler S_ExRndMap = sampler_state
{
  Texture   = <ExRndMap>;
  MipFilter = Linear;
  MinFilter = Linear;
  MagFilter = Linear;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// permutation table
texture PermutationTable;

// sampler
sampler S_PermutationTable = sampler_state
{
  Texture   = <PermutationTable>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// local frames
texture LocalFrames;

// sampler
sampler S_LocalFrames = sampler_state
{
  Texture   = <LocalFrames>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear;
  AddressU  = Clamp;
  AddressV  = Clamp;
};

// inverse local frames
texture LocalFramesInverse;

// sampler
sampler S_LocalFramesInverse = sampler_state
{
  Texture   = <LocalFramesInverse>;
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Linear;
  AddressU  = Clamp;
  AddressV  = Clamp;
};

// similarity set
uniform texture SimilaritySet;

sampler S_SimilaritySet = sampler_state
{
  Texture   = (SimilaritySet);
  MipFilter = Point;
  MinFilter = Point;
  MagFilter = Point;
  AddressU  = Wrap;
  AddressV  = Wrap;
};

// exemplar size
float ExemplarSize;

// randomness threshold
float Randomness;

// randomness scaling
float Scale;

// level 0 flag - used to handle special case with compress_clr
float IsLevel0;

// level id
float LevelId;

// time
float Time = 0.0;

// local frames texture resolution (w,h,1/w,1/h)
float4 LocalFramesRes;

// constraint threshold
float CstrThreshold;

// constraint map scale
float CstrMapScale;

// Neighren offset (stack exemplars)
float NeighOffset;

// Window pos (x,y,w,h)
float4 WindowPos;

// Parent window pos
float4 ParentWindowPos;

// Parent quad size
float4 ParentQuadSize;

// Parent at t-1 quad size
float4 Parent_T_1_QuadSize;

// Current at t-1 quad size
float4 Current_T_1_QuadSize;

// Scale for position map lookups
float  PositionMapScale;

// Data associated with position map (drag n drop)
float4 PositionData;

// Pixel packing index
float2 PixPackIndex;

// Cut avoidance (intorduces randomness on cuts)
float CutAvoidance = 0.0f;

//-----------------------------------------------------------------------------

float2 decode_simset_2(float2 k,float2 u)
{
  return k*(255.01/ExemplarSize)-(128.0/ExemplarSize) + u;
}

//-----------------------------------------------------------------------------

// from vertices
struct VS_OUTPUT
{
  float4 Pos         : POSITION;
  float2 Tex1        : TEXCOORD1;
};

//-----------------------------------------------------------------------------

#include "coordinates.fx"

#include "lookups.fx"

#include "output.fx"

#include "rand.fx"

#define fmodp(x,n) ((n)*frac((x)/(n)))

//-----------------------------------------------------------------------------

// Upsampling algorithm   -   packed version

float4 ps_upsamplingJitter(VS_OUTPUT In) : COLOR
{

  float2 tc  = In.Tex1;
  float2 rij = floor(tc)*2.0 + PixPackIndex;
  float2 aij   = relative2absolute(rij,WindowPos);
  float2 lftc  = (aij*NeighOffset+.5)*LocalFramesRes.zw;
  float2 tij = aij;



  float2 mij=fmodp(aij,2.0);

  float2 prev_rij=absolute2relative( floor(aij*0.5) , ParentWindowPos);
  float3 prev_xyerr;
  prev_xyerr = lookup_S_err(pix_unpack2pack(prev_rij,ParentQuadSize));

  float2 prev_xy = prev_xyerr.xy;
  float  err     = prev_xyerr.z;
  

  prev_xy  = (prev_xy * ExemplarSize); // floor removed for anisometric synthesis

  // upsampling
#ifdef ANISOSYNTH

float2x2 J     = tex2D(S_LocalFrames,lftc);
  float2 xy    = prev_xy + 
    mul(J , floor( (mij - 0.5)*NeighOffset ) + .5*IsLevel0 ) - .5*IsLevel0;
                 //                             ^^^^^^^^^^^^
                 // + .5*IsLevel0 since finest lvl mask is -1,0 (not centered)
#else
  float2 xy    = prev_xy + floor( (mij - 0.5)*NeighOffset );
#endif


  float2 r=floor(tij);
  
#ifdef TOROIDAL
  r=fmodp(r,LocalFramesRes.xy/NeighOffset);
#endif

  float2 rndij = fmodp(rand(r,0.0),ExemplarSize) - ExemplarSize/2.0;
  xy += round(rndij*Scale); 
  

  // done
  float2 txy = frac(xy / ExemplarSize);

  return (float4(txy,err,err));
  
}


float4 ps_upsampling(VS_OUTPUT In) : COLOR
{

  float2 tc  = In.Tex1;
  float2 rij = floor(tc)*2.0 + PixPackIndex;
  float2 aij   = relative2absolute(rij,WindowPos);
  float2 lftc  = (aij*NeighOffset+.5)*LocalFramesRes.zw;
  float2 tij = aij;



  float2 mij=fmodp(aij,2.0);

  float2 prev_rij=absolute2relative( floor(aij*0.5) , ParentWindowPos);
  float3 prev_xyerr;
  prev_xyerr = lookup_S_err(pix_unpack2pack(prev_rij,ParentQuadSize));

  float2 prev_xy = prev_xyerr.xy;
  float  err     = prev_xyerr.z;
  

  prev_xy  = (prev_xy * ExemplarSize); // floor removed for anisometric synthesis

  // upsampling
#ifdef ANISOSYNTH

float2x2 J     = tex2D(S_LocalFrames,lftc);
  float2 xy    = prev_xy + 
    mul(J , floor( (mij - 0.5)*NeighOffset ) + .5*IsLevel0 ) - .5*IsLevel0;
                 //                             ^^^^^^^^^^^^
                 // + .5*IsLevel0 since finest lvl mask is -1,0 (not centered)
#else
  float2 xy    = prev_xy + floor( (mij - 0.5)*NeighOffset );
#endif
  // done
  float2 txy = frac(xy / ExemplarSize);

  return (float4(txy,err,err));
  
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
  
  o.Pos       = float4(float2(In.Pos.x,-In.Pos.y) + float2(-1.0,1.0)/WindowPos.zw ,0.0,1.0);
  o.Tex1      = In.Tex1*WindowPos.zw;

  return (o);
}

//-----------------------------------------------------------------------------

technique upsampling_jitter
{
  pass P0
  {
    VertexShader = compile vs_3_0 vs_multexcoord();
    PixelShader  = compile ps_3_0 ps_upsamplingJitter();
    
  }  
}
technique upsampling
{
  pass P0
  {
    VertexShader = compile vs_3_0 vs_multexcoord();
    PixelShader  = compile ps_3_0 ps_upsampling();
    
  }  
}
//-----------------------------------------------------------------------------
