#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

sampler gDiffuseSampler = sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;

   AddressU = CLAMP;
   AddressV = CLAMP;
   AddressW = CLAMP;
};

sampler gMaskSampler = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

const float2 cOffset[4]   = {float2(-0.5f, -0.5f), float2(0.5f, -0.5f), float2(0.5f, 0.5f), float2(-0.5f, 0.5f)};
const float2 cCornerUV[4] = {float2( 0,  1), float2(  1, 1), float2(1 , 0), float2( 0, 0)};

//============================================================================
//============================================================================
int getCornerIndex(int vertIndex)
{
    int iDiv = (vertIndex + 0.5f) / 4;
    int iMod = vertIndex - (iDiv * 4);
    return iMod;
}

//============================================================================
//============================================================================
float2 fetchOffset(int index)
{
    int cornerIndex = getCornerIndex(index);
    return cOffset[cornerIndex];
}

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
float4 fetchTexCoord1(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord1;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeTexCoords(int index)
{
   int cornerIndex = getCornerIndex(index);
   return float4(cCornerUV[cornerIndex], cCornerUV[cornerIndex]);
}

//============================================================================
//============================================================================
float4 computeVertexPosition(float4 pos, float2 offset, float2 size)
{
    float4 finalPos = mul(pos, gWorldToProj);
    finalPos.xy += offset * size;

    return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void pieVS ( in  int Index          : INDEX
            ,out float4 OutPosition : POSITION
            ,out float2 OutTex0     : TEXCOORD0
            ,out float4 OutColor    : COLOR0 )
{
   float4 InPosition = fetchPosition(Index);
   float4 InOffset   = fetchTexCoord0(Index);
   float2 InTex1     = fetchTexCoord1(Index).xy;
   float4 InColor    = fetchColor0(Index);


   float3 rightV   = gViewToWorld._m00_m01_m02 * InOffset.x;
   float3 upV      = gViewToWorld._m10_m11_m12 * InOffset.y;   
   float4 finalPos = float4(InPosition + rightV.xyz - upV.xyz, 1.0f);

   OutPosition = mul(finalPos, gWorldToProj);   
   OutColor    = InColor;
   OutTex0     = InTex1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void piePS ( in  float4 InColor0    : COLOR0
            ,in  float2 InTex0      : TEXCOORD0
            ,out float4 OutColor    : COLOR0 )
{
   OutColor = InColor0 * tex2D(gMaskSampler, InTex0);   
}

//============================================================================
//============================================================================
void spriteVS(in  int     Index : INDEX,
              out float4 oPos   : POSITION0,
              out float2 oTex0  : TEXCOORD0,
              out float4 oColor0: COLOR0,
              uniform bool b2DSprite
             )
{   
   int vertexIndex    = (Index + 0.5f) / 4;
   float4 InPosition  = fetchPosition(vertexIndex);      
   float2 InOffset    = fetchOffset(Index);   
   float2 InSize      = fetchTexCoord0(vertexIndex).xy;
   float2 In2DOffset  = fetchTexCoord1(vertexIndex).xy;
   float4 InColor0    = fetchColor0(vertexIndex);   

   //-- uvs
   oTex0.xy = ComputeTexCoords(Index);   
   InPosition.w = 1.0f;
   
   if (b2DSprite)
   {      
      float3 rightV   = float3(1,0,0) * ((InOffset.x * InSize.x)+ In2DOffset.x);
      float3 upV      = float3(0,1,0) * ((InOffset.y * InSize.y)+ In2DOffset.y);         
      float4 finalPos = float4(InPosition + rightV.xyz + upV.xyz, 1.0f);   
      oPos = finalPos;
   }
   else
   {
      float3 rightV   = gViewToWorld._m00_m01_m02 * ((InOffset.x * InSize.x) + In2DOffset.x);
      float3 upV      = gViewToWorld._m10_m11_m12 * ((InOffset.y * InSize.y) + In2DOffset.y);   
      float4 finalPos = float4(InPosition + rightV.xyz + upV.xyz, 1.0f);   
      oPos =  mul(finalPos, gWorldToProj);   
   }

   //oPos   = computeVertexPosition(InPosition, InOffset, InSize);   
   oColor0 = InColor0;      
}

//============================================================================
//============================================================================
void spritePS( in  float4 InColor0 : COLOR0
              ,in  float2 InTex0   : TEXCOORD0
              ,out float4 OutColor : COLOR0)
{
   OutColor = tex2D(gMaskSampler, InTex0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique Pie
{
   pass Pie
   {
      VertexShader = compile vs_3_0 pieVS();
      PixelShader  = compile ps_3_0 piePS();
   }      
}

technique Sprite
{
   pass Sprite
   {
      VertexShader = compile vs_3_0 spriteVS(false);
      PixelShader  = compile ps_3_0 spritePS();
   }
}

technique Sprite2D
{
   pass Sprite2D
   {

      VertexShader = compile vs_3_0 spriteVS(true);
      PixelShader  = compile ps_3_0 spritePS();

      ViewportEnable = FALSE;
   }
}