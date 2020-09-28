// File: lightBuffer.fx

#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

float4x4 gLightToWorld;
float4x4 gWorldToLight;
float4 gSlicePlane;
float4 gLightBufValues0; // width, height, scale, 1.0/scale
float4 gLightBufValues1; // 1/width, 1/height, normSlice, sliceY

struct VertexIn
{
   float4 mPosRadius          : TEXCOORD0;
   float4 mColor              : TEXCOORD1;
   float4 mLightParams        : TEXCOORD2; // omniMul, omniAdd, spotMul, spotAdd
   float4 mSpotAtDecayDist    : TEXCOORD3; 
};

struct VertexToPixel
{
   float4 mPosition           : POSITION;
   float3 mWorldPos           : TEXCOORD0;
   float4 mPosRadius          : TEXCOORD1;
   float4 mColor              : TEXCOORD2;
   float4 mLightParams        : TEXCOORD3; // omniMul, omniAdd, spotMul, spotAdd
   float4 mSpotAtDecayDist    : TEXCOORD4;   
};

VertexToPixel vsAccumLight(int vertexIndex : INDEX)
{
   int fetchIndex = vertexIndex / 4;
   int cornerIndex = vertexIndex - (fetchIndex * 4);
   
   VertexToPixel result;
   
   float4 posRadius, color, lightParams, spotAtDecayDist;
   asm
   {
      vfetch posRadius,        fetchIndex, texcoord0
      vfetch color,            fetchIndex, texcoord1
      vfetch lightParams,      fetchIndex, texcoord2
      vfetch spotAtDecayDist,  fetchIndex, texcoord3
   };
   
   result.mPosRadius = posRadius;
   result.mColor = color;
   result.mLightParams = lightParams;
   result.mSpotAtDecayDist = spotAtDecayDist;
      
   float4 omniPos = float4(result.mPosRadius.x, result.mPosRadius.y, result.mPosRadius.z, 1.0f);
   float omniRadius = result.mPosRadius.w;
      
   float3 cornerCoord;
   if (cornerIndex == 0)
      cornerCoord = float3(-1.0f, 0.0f, -1.0f);
   else if (cornerIndex == 1)
      cornerCoord = float3(1.0f, 0.0f, -1.0f);
   else if (cornerIndex == 2)
      cornerCoord = float3(1.0f, 0.0f, 1.0f);
   else
      cornerCoord = float3(-1.0f, 0.0f, 1.0f);
      
   float4 transformedPlane = gSlicePlane;
   transformedPlane.w += dot(-float3(omniPos.x, omniPos.y, omniPos.z), float3(transformedPlane.x, transformedPlane.y, transformedPlane.z));
   
   if (transformedPlane.w < 0.0f)
      transformedPlane *= -1.0f;
   
   if (transformedPlane.w >= omniRadius)
   {
      result.mPosition = 0.0f;
      result.mWorldPos = 0.0f;
   }
   else
   {
      float3 circlePos = (float3(transformedPlane.x, transformedPlane.y, transformedPlane.z) * transformedPlane.w) + omniPos;
      float circleRadius = sqrt(max(0.0f, omniRadius * omniRadius - transformedPlane.w * transformedPlane.w));
      
      float3 worldCorner = circlePos + float3(circleRadius, 0.0f, circleRadius) * cornerCoord;
      float4 lightCorner = mul(float4(worldCorner.x, worldCorner.y, worldCorner.z, 1.0f), gWorldToLight);
      
      result.mPosition = float4(2.0f * lightCorner.x - 1.0f, -(2.0f * lightCorner.y - 1.0f), 0.0f, 1.0f);
      result.mWorldPos = worldCorner;
   }
   
   return result;
}

void psAccumLight(
   in VertexToPixel vals,
   out float4 OutColor : COLOR0,
   out float4 OutVector : COLOR1)
{
   float3 worldPos = vals.mWorldPos;
   float3 lightPos = float3(vals.mPosRadius.x, vals.mPosRadius.y, vals.mPosRadius.z);
   float omniMul = vals.mLightParams.x;
   float omniAdd = vals.mLightParams.y;
   float spotMul = vals.mLightParams.z;
   float spotAdd = vals.mLightParams.w;
   float3 spotAt = float3(vals.mSpotAtDecayDist.x, vals.mSpotAtDecayDist.y, vals.mSpotAtDecayDist.z);
   float decayDist = vals.mSpotAtDecayDist.w;
        
   float3 lightVec = lightPos - worldPos;   
   float lightVecLen2 = dot(lightVec, lightVec);
   float ooLightDist = recipSqrtClamp(lightVecLen2);
   float dist = ooLightDist * lightVecLen2;

   float3 lightNorm = lightVec * ooLightDist;  
   
   float spotAngle = -dot(spotAt, lightNorm);

   float2 atten;
   atten.x = saturate(spotAngle * spotMul + spotAdd);
   atten.y = saturate(dist * omniMul + omniAdd);
   
   atten = atten * atten * (-2.0 * atten + 3.0);
         
   float overallAtten = atten.x * atten.y * saturate(decayDist * ooLightDist);
   float3 diffuseContrib = overallAtten * vals.mColor * gLightBufValues0.z;
   
   OutColor = float4(diffuseContrib.x, diffuseContrib.y, diffuseContrib.z, vals.mColor.w);
   
   float scale = overallAtten;
   if (scale > 0.0f)
      scale = max(scale, .000075f);
   OutVector.xyz = lightNorm * 4.0f * scale;
   OutVector.w = .125f;
}

void vsRenormAndPack(
   in float4 inPosition : POSITION,
   out float4 outPosition : POSITION,
   out float2 texcoords : TEXCOORD0)
{
   outPosition = float4(inPosition.x, inPosition.y, 0.0f, 1.0f);
   texcoords = float2(inPosition.z, inPosition.w);
}

sampler LightBufferVectorSampler	= sampler_state
{
   MinFilter = POINT;
   MagFilter = POINT;
   MipFilter = NONE;

   AddressU = CLAMP;
   AddressV = CLAMP;
   AddressW = CLAMP;

   SeparateZFilterEnable = FALSE;
};

float4 psRenormAndPack(in float2 texcoords : TEXCOORD0) : COLOR
{
   float3 sample = tex2D(LightBufferVectorSampler, texcoords);
   
   sample = tryNormalize(sample);
   
   //if (dot(sample, sample) < .0125f)
   //   sample = float3(0.0f, 1.0f, 0.0f);
   
   sample = sample * .5f + float3(.5f, .5f, .5f);
   
   return float4(sample.x, sample.y, sample.z, 1.0f);
}   

technique AccumLightTechnique
{
    pass
    {
      VertexShader = compile vs_3_0 vsAccumLight();
      PixelShader = compile ps_3_0 psAccumLight();

      ViewPortEnable             = true;
      ZEnable                    = false;
      ZWriteEnable               = false;
      CullMode                   = None;
      AlphaBlendEnable           = true;
      SrcBlend                   = One;
      DestBlend                  = One;
      AlphaTestEnable            = false;
      Halfpixeloffset            = true;
      SeparateAlphaBlendEnable   = true;
      SrcBlendAlpha              = One;
      DestBlendAlpha             = One;
    }
}

technique RenormAndPackTechnique
{
    pass
    {
      VertexShader = compile vs_3_0 vsRenormAndPack();
      PixelShader = compile ps_3_0 psRenormAndPack();

      ViewPortEnable             = false;
      ZEnable                    = false;
      ZWriteEnable               = false;
      CullMode                   = None;
      AlphaBlendEnable           = false;
      SrcBlend                   = One;
      DestBlend                  = zero;
      AlphaTestEnable            = false;
      Halfpixeloffset            = true;
      SeparateAlphaBlendEnable   = false;
      SrcBlendAlpha              = One;
      DestBlendAlpha             = zero;
    }
}
