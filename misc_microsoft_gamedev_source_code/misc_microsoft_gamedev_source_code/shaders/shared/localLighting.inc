// File: localLighting.inc

#include "localLightingRegs.inc"

// This param should be set to manual register update!
float4 gLightData[NUM_LOCAL_LIGHT_PSHADER_CONSTANTS] : register(c0);

// You may be wondering why I did it this way. It's because other methods caused horrible codegen.
#define lightPos     gLightData[lightIndex]
#define omniMul      gLightData[lightIndex].w

#define lightColor   gLightData[lightIndex + 1]
#define omniAdd      gLightData[lightIndex + 1].w

#define decayDist    gLightData[lightIndex + 2].x
#define spotMul      gLightData[lightIndex + 2].y
#define spotAdd      gLightData[lightIndex + 2].z
#define shadowIndex  gLightData[lightIndex + 2].w

#define spotAt       gLightData[lightIndex + 3]
#define specInten    gLightData[lightIndex + 3].w

#define shadowCol0   gLightData[lightIndex + 4]
#define shadowCol1   gLightData[lightIndex + 5]
#define shadowCol2   gLightData[lightIndex + 6]

#define shadowUVBoundsIndex   gLightData[lightIndex + 7].x
#define shadowFadeFactor      gLightData[lightIndex + 7].y

// Extended
#define lightPosExt   lightParams0
#define omniMulExt    lightParams0.w

#define lightColorExt lightParams1
#define omniAddExt    lightParams1.w

#define decayDistExt  lightParams2.x
#define spotMulExt    lightParams2.y
#define spotAddExt    lightParams2.z

#define spotAtExt     lightParams3
#define specIntenExt  lightParams3.w

static float4 UVBoundsTable[5] =
{
   float4( 0.0f, 0.0f, 1.0f, 1.0f ),
   float4( 0.0f, 0.0f,  .5f,  .5f ),
   float4(  .5f, 0.0f, 1.0f,  .5f ),
   float4( 0.0f,  .5f,  .5f, 1.0f ),
   float4(  .5f, 0.5f, 1.0f, 1.0f )
};

static float2 PCFOffsets1[4] =
{
   float2( 3.591659E-05 , 7.231401E-04  ),
   float2( 5.948633E-04 , 3.5039E-04  ),
   float2( 6.088773E-04 , 4.708898E-04  ),
   float2( 6.689834E-04 , 2.728689E-04  ),
};

static float2 PCFOffsets2[4] =
{
   float2( 5.600713E-04 , 6.843841E-05 ),
   float2( 1.464225E-04 , 2.288123E-04 ),
   float2( 2.128248E-04 , 3.938494E-04 ),
   float2( 3.106788E-04 , 3.071751E-04 ),
};

static void omniIlluminateShadowed(
  in int lightIndex,
  in float3 worldNorm,
  in float3 worldReflect,
  in float3 worldPos,
  in float specPower,
  inout float3 diffuseSum,
  inout float3 specSum)
{
   float3 lightVec = lightPos - worldPos;   
   float lightVecLen2 = dot(lightVec, lightVec);
   float ooLightDist = recipSqrtClamp(lightVecLen2);
   float dist = ooLightDist * lightVecLen2;

   float3 lightNorm = lightVec * ooLightDist;  
   
   float spotAngle = -dot(spotAt, lightNorm);

   float3 atten;
   atten.x = saturate(dot(lightNorm, worldNorm));
   atten.y = saturate(spotAngle * spotMul + spotAdd);
   atten.z = saturate(dist * omniMul + omniAdd);
   
   atten = atten * atten * (-2.0 * atten + 3.0);
   
   atten.x *= atten.z;
   atten.x *= saturate(decayDist * ooLightDist);	
   atten.x *= atten.y;

   float rDotL = max(0.0, dot(worldReflect, lightNorm));
   float specContrib = rDotL * recipFixedFunc(specPower - (specPower - 1.0f) * rDotL) * specInten;
   float3 diffuseContrib = atten.x * lightColor;

#if 0   
   if ((shadowIndex >= 0.0f) && (atten.x > 0.0f))  
   {
      float4 p = float4(worldPos.x, worldPos.y, worldPos.z, 1.0);
      
      float3 shadowPos;
      shadowPos.x = dot(p, shadowCol0);
      shadowPos.y = dot(p, shadowCol1);
      float w = dot(p, shadowCol2);
      shadowPos.z = shadowIndex;
      
     
      float oow = recipClamp(w);
      
      shadowPos.xy = shadowPos * oow;

      float z = (w * 1.007874 - 1.007874) * oow;
  
      float shadowFactor = PCF(shadowPos, z, gLocalShadowMapTexture);
            
      diffuseContrib *= shadowAtten;
   } 
#endif
   if ((shadowIndex != -1.0f) && (atten.x > 0.0f))  
   {
      float4 p = float4(worldPos.x, worldPos.y, worldPos.z, 1.0);
      
      float3 shadowPos;
      shadowPos.x = dot(p, shadowCol0);
      shadowPos.y = dot(p, shadowCol1);
      float w = dot(p, shadowCol2);
      
      float z;
      [branch]
      if (shadowIndex < -1.0f)
      {
         shadowPos.z = -shadowIndex - 1.0f;
         
         float3 viewPos = float3(shadowPos.x, shadowPos.y, w);
         if (w < 0.0f)
         {
            //viewPos.x = -viewPos.x;
            viewPos.z = -viewPos.z;
            shadowPos.z += (1.0f / MAX_LOCAL_SHADOW_BUFFER_SLICES);
         }
         
         float3 normViewPos = viewPos;
         float oolen = 1.0f / sqrt(dot(normViewPos, normViewPos));
         normViewPos = normViewPos * oolen;

         normViewPos.z += 1.0f;

         viewPos.xy = normViewPos * (1.0f / normViewPos.z);
         					
         shadowPos.xy = viewPos * float2(.5f, -.5f) + float2(.5f, .5f);
                     	
         z = 1.0f / oolen;
         
         z = (z - DUAL_PARABOLOID_NEAR_PLANE) / (DUAL_PARABOLOID_FAR_PLANE * DUAL_PARABOLOID_NEAR_PLANE);
      }
      else
      {
         shadowPos.z = shadowIndex;
         
         float oow = recipClamp(w);
         shadowPos.xy = shadowPos * oow;
         
         z = (w * 1.007874 - 1.007874) * oow;
         
         float4 UVBounds = UVBoundsTable[shadowUVBoundsIndex];
         shadowPos.xy = max(shadowPos.xy, UVBounds.xy);
         shadowPos.xy = min(shadowPos.xy, UVBounds.zw);
      }
      
      float shadowFactor = PCF(shadowPos, z, gLocalShadowMapTexture);
      diffuseContrib *= lerp(shadowFactor, 1.0f, shadowFadeFactor);

#if 0      
      float4 depth;
      asm
      {
         tfetch3D depth.x___, shadowPos, gLocalShadowMapTexture
      };
      
      diffuseContrib = 1.0f-depth.x;
#endif      
   }      
  
   diffuseSum += diffuseContrib;
   specSum += diffuseContrib * specContrib;
}  

void omniIlluminateShadowedDiffuse(
  in int lightIndex,
  in float3 worldNorm,
  in float3 worldPos,
  inout float3 diffuseSum)
{
   float3 lightVec = lightPos - worldPos;   
   float lightVecLen2 = dot(lightVec, lightVec);
   float ooLightDist = recipSqrtClamp(lightVecLen2);
   float dist = ooLightDist * lightVecLen2;

   float3 lightNorm = lightVec * ooLightDist;  
   
   float spotAngle = -dot(spotAt, lightNorm);

   float3 atten;
   atten.x = saturate(dot(lightNorm, worldNorm));
   atten.y = saturate(spotAngle * spotMul + spotAdd);
   atten.z = saturate(dist * omniMul + omniAdd);
   
   atten = atten * atten * (-2.0 * atten + 3.0);
   
   atten.x *= atten.z;
   atten.x *= saturate(decayDist * ooLightDist);	
   atten.x *= atten.y;

   float3 diffuseContrib = atten.x * lightColor;
   
   if ((shadowIndex != -1.0f) && (atten.x > 0.0f))  
   {
      float4 p = float4(worldPos.x, worldPos.y, worldPos.z, 1.0);
      
      float3 shadowPos;
      shadowPos.x = dot(p, shadowCol0);
      shadowPos.y = dot(p, shadowCol1);
      float w = dot(p, shadowCol2);
      
      float z;
      [branch]
      if (shadowIndex < -1.0f)
      {
         shadowPos.z = -shadowIndex - 1.0f;
         
         float3 viewPos = float3(shadowPos.x, shadowPos.y, w);
         if (w < 0.0f)
         {
            viewPos.xz = -viewPos;
            shadowPos.z += (1.0f / MAX_LOCAL_SHADOW_BUFFER_SLICES);
         }
         
         float3 normViewPos = viewPos;
         float oolen = 1.0f / sqrt(dot(normViewPos, normViewPos));
         normViewPos = normViewPos * oolen;

         normViewPos.z += 1.0f;

         viewPos.xy = normViewPos * (1.0f / normViewPos.z);
         					
         shadowPos.xy = viewPos * float2(.5f, -.5f) + float2(.5f, .5f);
                     	
         z = 1.0f / oolen;
         
         z = (z - DUAL_PARABOLOID_NEAR_PLANE) / (DUAL_PARABOLOID_FAR_PLANE * DUAL_PARABOLOID_NEAR_PLANE);
      }
      else
      {
         shadowPos.z = shadowIndex;
         
         float oow = recipClamp(w);
         shadowPos.xy = shadowPos * oow;
         
         z = (w * 1.007874 - 1.007874) * oow;
         
         float4 UVBounds = UVBoundsTable[shadowUVBoundsIndex];
         shadowPos.xy = max(shadowPos.xy, UVBounds.xy);
         shadowPos.xy = min(shadowPos.xy, UVBounds.zw);
      }
              
      float shadowFactor = PCF(shadowPos, z, gLocalShadowMapTexture);
      diffuseContrib *= lerp(shadowFactor, 1.0f, shadowFadeFactor);

#if 0      
      float4 depth;
      asm
      {
         tfetch3D depth.x___, shadowPos, gLocalShadowMapTexture
      };
      
      diffuseContrib = 1.0f-depth.x;
#endif      
   }
  
   diffuseSum += diffuseContrib;
}  

static void omniIlluminate(
  in int lightIndex,
  in float3 worldNorm,
  in float3 worldReflect,
  in float3 worldPos,
  in float specPower,
  inout float3 diffuseSum,
  inout float3 specSum)
{
   float3 lightVec = lightPos - worldPos;   
   float lightVecLen2 = dot(lightVec, lightVec);
   float ooLightDist = recipSqrtClamp(lightVecLen2);
   float dist = ooLightDist * lightVecLen2;

   float3 lightNorm = lightVec * ooLightDist;  
   
   float spotAngle = -dot(spotAt, lightNorm);

   float3 atten;
   atten.x = saturate(dot(lightNorm, worldNorm));
   atten.y = saturate(spotAngle * spotMul + spotAdd);
   atten.z = saturate(dist * omniMul + omniAdd);
   
   atten = atten * atten * (-2.0 * atten + 3.0);
   
   atten.x *= atten.z;
   atten.x *= saturate(decayDist * ooLightDist);	
   atten.x *= atten.y;
   
   float3 diffuseContrib = atten.x * lightColor;
         
   float rDotL = max(0.0, dot(worldReflect, lightNorm));
   float specContrib = rDotL * recipFixedFunc(specPower - (specPower - 1.0f) * rDotL) * specInten;
   
   diffuseSum += diffuseContrib;
   specSum += diffuseContrib * specContrib;
}

static void omniIlluminateDiffuse(
  in int lightIndex,
  in float3 worldNorm,
  in float3 worldPos,
  inout float3 diffuseSum)
{
   float3 lightVec = lightPos - worldPos;   
   float lightVecLen2 = dot(lightVec, lightVec);
   float ooLightDist = recipSqrtClamp(lightVecLen2);
   float dist = ooLightDist * lightVecLen2;

   float3 lightNorm = lightVec * ooLightDist;  
   
   float spotAngle = -dot(spotAt, lightNorm);

   float3 atten;
   atten.x = saturate(dot(lightNorm, worldNorm));
   atten.y = saturate(spotAngle * spotMul + spotAdd);
   atten.z = saturate(dist * omniMul + omniAdd);
   
   atten = atten * atten * (-2.0 * atten + 3.0);
   
   diffuseSum += lightColor * (atten.x * atten.z * saturate(decayDist * ooLightDist) * atten.y);

#if 0   
   atten.x *= atten.z;
   atten.x *= saturate(decayDist * ooLightDist);	
   atten.x *= atten.y;
      
   float3 diffuseContrib = atten.x * lightColor;
   diffuseSum += diffuseContrib;
#endif      
}

#define lightPos0     gLightData[lightIndex    ]
#define lightPos1     gLightData[lightIndex + 1]
#define lightPos2     gLightData[lightIndex + 2]
#define lightPos3     gLightData[lightIndex + 3]

#define lightColor0   gLightData[lightIndex + 4]
#define lightColor1   gLightData[lightIndex + 5]
#define lightColor2   gLightData[lightIndex + 6]
#define lightColor3   gLightData[lightIndex + 7]

#define omniMul0123   gLightData[lightIndex + 8]
#define omniAdd0123   gLightData[lightIndex + 9]
#define decayDist0123 gLightData[lightIndex + 10]

static void omniOnlyIlluminateDiffuse4(
  in int lightIndex,
  in float3 worldNorm,
  in float3 worldPos,
  inout float3 diffuseSum)
{
   float3 lightVec0 = lightPos0 - worldPos;   
   float3 lightVec1 = lightPos1 - worldPos;   
   float3 lightVec2 = lightPos2 - worldPos;   
   float3 lightVec3 = lightPos3 - worldPos;   
   
   float lightVecLen20 = dot(lightVec0, lightVec0);
   float lightVecLen21 = dot(lightVec1, lightVec1);
   float lightVecLen22 = dot(lightVec2, lightVec2);
   float lightVecLen23 = dot(lightVec3, lightVec3);
   
   float4 ooLightDist;
   ooLightDist.x = recipSqrtClamp(lightVecLen20);
   ooLightDist.y = recipSqrtClamp(lightVecLen21);
   ooLightDist.z = recipSqrtClamp(lightVecLen22);
   ooLightDist.w = recipSqrtClamp(lightVecLen23);
   
   float4 dist;
   dist.x = ooLightDist.x * lightVecLen20;
   dist.y = ooLightDist.y * lightVecLen21;
   dist.z = ooLightDist.z * lightVecLen22;
   dist.w = ooLightDist.w * lightVecLen23;
   
   dist = dist * omniMul0123 + omniAdd0123;
   
   float3 lightNorm0 = lightVec0 * ooLightDist.x; 
   float3 lightNorm1 = lightVec1 * ooLightDist.y;  
   float3 lightNorm2 = lightVec2 * ooLightDist.z;  
   float3 lightNorm3 = lightVec3 * ooLightDist.w;  
      
   float4 atten01;
   atten01.x = saturate(dot(lightNorm0, worldNorm));
   atten01.y = saturate(dist.x);
   
   atten01.z = saturate(dot(lightNorm1, worldNorm));
   atten01.w = saturate(dist.y);
   
   float4 atten23;
   atten23.x = saturate(dot(lightNorm2, worldNorm));
   atten23.y = saturate(dist.z);
   
   atten23.z = saturate(dot(lightNorm3, worldNorm));
   atten23.w = saturate(dist.w);
   
   atten01 = atten01 * atten01 * (-2.0 * atten01 + 3.0);
   atten23 = atten23 * atten23 * (-2.0 * atten23 + 3.0);
   
   float4 decay0123 = saturate(decayDist0123 * ooLightDist);
   
   diffuseSum += lightColor0 * (atten01.x * atten01.y * decay0123.x);
   diffuseSum += lightColor1 * (atten01.z * atten01.w * decay0123.y);
   diffuseSum += lightColor2 * (atten23.x * atten23.y * decay0123.z);
   diffuseSum += lightColor3 * (atten23.z * atten23.w * decay0123.w);
}

#undef lightPos     
#undef omniMul      
#undef lightColor   
#undef omniAdd      
#undef decayDist    
#undef spotMul      
#undef spotAdd      
#undef shadowIndex  
#undef spotAt       
#undef specInten    
#undef shadowCol0   
#undef shadowCol1   
#undef shadowCol2
#undef shadowUVBoundsIndex

static float2 computeLightUV(float lightIndex)
{
   float2 result;
   result.y = floor(lightIndex * (1.0f/128.0f));
   result.x = lightIndex - result.y * 128.0f;
   return result;
}

sampler ExtendedLocalLightSampler = sampler_state
{
    MinFilter = POINT;
    MagFilter = POINT;
    MipFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

static void omniIlluminateExtended(
  in int lightIndex,
  in float3 worldNorm,
  in float3 worldReflect,
  in float3 worldPos,
  in float specPower,
  inout float3 diffuseSum,
  inout float3 specSum)
{
   float2 lightUV0 = computeLightUV(lightIndex);
   float2 lightUV1 = computeLightUV(lightIndex + 1);
   float2 lightUV2 = computeLightUV(lightIndex + 2);
   float2 lightUV3 = computeLightUV(lightIndex + 3);
   
   float4 lightParams0, lightParams1, lightParams2, lightParams3;
   asm
   {
      tfetch2D lightParams0, lightUV0, ExtendedLocalLightSampler, UnnormalizedTextureCoords = true
      tfetch2D lightParams1, lightUV1, ExtendedLocalLightSampler, UnnormalizedTextureCoords = true
      tfetch2D lightParams2, lightUV2, ExtendedLocalLightSampler, UnnormalizedTextureCoords = true
      tfetch2D lightParams3, lightUV3, ExtendedLocalLightSampler, UnnormalizedTextureCoords = true
   };

   float3 lightVec = lightPosExt - worldPos;   
   float lightVecLen2 = dot(lightVec, lightVec);
   float ooLightDist = recipSqrtClamp(lightVecLen2);
   float dist = ooLightDist * lightVecLen2;

   float3 lightNorm = lightVec * ooLightDist;  
   
   float spotAngle = -dot(spotAtExt, lightNorm);

   float3 atten;
   atten.x = saturate(dot(lightNorm, worldNorm));
   atten.y = saturate(spotAngle * spotMulExt + spotAddExt);
   atten.z = saturate(dist * omniMulExt + omniAddExt);
   
   atten = atten * atten * (-2.0 * atten + 3.0);
   
   atten.x *= atten.z;
   atten.x *= saturate(decayDistExt * ooLightDist);	
   atten.x *= atten.y;
   
   float3 diffuseContrib = atten.x * lightColorExt;
         
   float rDotL = max(0.0, dot(worldReflect, lightNorm));
   float specContrib = rDotL * recipFixedFunc(specPower - (specPower - 1.0f) * rDotL) * specIntenExt;
   
   diffuseSum += diffuseContrib;
   specSum += diffuseContrib * specContrib;
}