#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

struct VertexIn
{
   float4 Position   : POSITION;
   float4 Diffuse    : COLOR0;
   float2 TexCoord   : TEXCOORD0;
};

struct VertexOut
{
   float4 Position   : POSITION;
   float2 TexCoord   : TEXCOORD0;
};

VertexOut vsMain(in VertexIn In)
{
   VertexOut Out;
   Out.Position = In.Position;
   Out.TexCoord = In.TexCoord;
   return Out;
}

sampler gCurFrame = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

sampler gPrevFrame = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

float4 gBlendParams;

float4 psMain(VertexOut In) : COLOR                
{
   float4 cur = tex2D(gCurFrame, In.TexCoord);
   float4 prev = tex2D(gPrevFrame, In.TexCoord + gBlendParams.xy);

   float3 filtered;
   
   float3 linearCur = cur * cur;
   float3 linearPrev = prev * prev;
      
   filtered.rgb = lerp(linearPrev.rgb, linearCur.rgb, gBlendParams.w);

   filtered.r = sqrt(filtered.r);
   filtered.g = sqrt(filtered.g);
   filtered.b = sqrt(filtered.b);
   
   float blurMask = min(cur.w, prev.w);
   filtered.rgb = lerp(lerp(prev, cur, gBlendParams.z), filtered, blurMask);
   
//   return float4(filtered + (1-cur.w)*.5, 1);
   return float4(filtered, 1);
//return .5*(cur-prev)+.5f;
} 

technique
{
    pass
    {
      VertexShader = compile vs_3_0 vsMain();
      PixelShader = compile ps_3_0 psMain();

      viewportenable = false;
      cullmode = none;
      zenable = false;
      alphablendenable = false;
    }
}

