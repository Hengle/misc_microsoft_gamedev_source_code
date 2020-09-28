#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

sampler gDiffuseSampler = sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;
   AddressU  = CLAMP;
   AddressV  = CLAMP;   
};

const float  cCornerMultiplier[4] = {1,-1,-1, 1};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchPosition(int index)
{
   float4 pos;
   asm
   {
      vfetch pos, index, position0
   };
   return pos;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchColor0(int index)
{
   float4 color;
   asm
   {
      vfetch color, index, color0;
   };
   return color;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord0(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord0;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int getCornerIndex(int vertIndex)
{
   int iDiv = (vertIndex+0.5f) / 4; // add 0.5f to avoid rounding errors
   int iMod = vertIndex - (iDiv * 4);
   return iMod;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeCornerPos(float3 pos, float scale, int index)
{
   int cornerIndex = getCornerIndex(index);
   float cornerMultiplier = cCornerMultiplier[cornerIndex];   
   float3 offset = float3(0,1,0);   
   if (pos.z > 0.0f)
   {
      offset = float3(1,0,0);
   }   
   pos.z = 0.0f;
   
         
   offset *= cornerMultiplier;
   offset *= scale;
   
   float3 finalPos = pos + offset;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void graphVS(in  int    Index       : INDEX,
             out float4 OutPosition : POSITION,
             out float2 OutTex0     : TEXCOORD0,
             out float4 OutColor0   : COLOR0)
{
   // quad strip   
   int vertexIndex = (floor((Index+0.5f)/2));

   //int vertexIndex = (floor((Index+0.5f)/2)) - (floor((Index+0.5f)/4));  //--add in 0.5f to avoid rounding errors
   float4 InPosition = fetchPosition(vertexIndex);
   float4 InColor0   = fetchColor0(vertexIndex);
   float4 InTexCoord0 = fetchTexCoord0(vertexIndex);

   OutPosition.xyz = ComputeCornerPos(InPosition, 2.0f, Index);      
   OutPosition.w = 1.0f;

   OutColor0 = InColor0;
   OutTex0   = InTexCoord0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void graphVS2(in  int    Index       : INDEX,
              out float4 OutPosition : POSITION,
              out float2 OutTex0     : TEXCOORD0,
              out float4 OutColor0   : COLOR0)
{   
   float4 InPosition  = fetchPosition(Index);
   float4 InColor0    = fetchColor0(Index);
   float4 InTexCoord0 = fetchTexCoord0(Index);

   OutPosition = float4(InPosition.x, InPosition.y, 0, 1.0f);      
   OutColor0   = InColor0;
   OutTex0     = InTexCoord0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void graphPS( in  float4 InColor0  : COLOR0,
              in  float2 InTex0    : TEXCOORD0,
              out float4 OutColor0 : COLOR0,
              uniform bool bSampleTexture)
{
   OutColor0 = InColor0;

   if (bSampleTexture)
   {
      float4 diffuseSample = tex2D(gDiffuseSampler, InTex0);
      OutColor0 *= diffuseSample;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique graphDefault
{
   pass
   {
      VertexShader = compile vs_3_0 graphVS();
      PixelShader  = compile ps_3_0 graphPS(false);
   }

   pass
   {
      VertexShader = compile vs_3_0 graphVS2();
      PixelShader  = compile ps_3_0 graphPS(false);
   }

   pass
   {
      VertexShader = compile vs_3_0 graphVS2();
      PixelShader  = compile ps_3_0 graphPS(true);
   }
}