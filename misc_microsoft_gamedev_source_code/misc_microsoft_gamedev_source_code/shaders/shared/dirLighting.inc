// File: dirLighting.inc

static const float g_fEpsilonVSM = 0.000000155f;

static float PCFFilter3x3(float3 shadowMapCoords, float shadowArrayIndex, uniform bool fastFiltering)
{
   float4 weights;
   
   float3 texCoords = float3(shadowMapCoords.x, shadowMapCoords.y, shadowArrayIndex);

   float fAvgZ;
	float fAvgZ2;
   
   if (!fastFiltering)
   {
      float3 s, t, u;
      
      asm
      {
         getWeights3D weights, texCoords, gDirShadowMapTexture, MagFilter = linear, MinFilter = linear, MipFilter = linear, VolMagFilter = point, VolMinFilter = point
         
         tfetch3D s.x___, texCoords, gDirShadowMapTexture, OffsetX = -1.5, OffsetY = -1.5, MinFilter=point, MagFilter=point
         tfetch3D s._x__, texCoords, gDirShadowMapTexture, OffsetX = -1.5, OffsetY =  -.5, MinFilter=point, MagFilter=point
         tfetch3D s.__x_, texCoords, gDirShadowMapTexture, OffsetX = -1.5, OffsetY =   .5, MinFilter=point, MagFilter=point
         
         tfetch3D t.x___, texCoords, gDirShadowMapTexture, OffsetX = -.5, OffsetY = -1.5, MinFilter=point, MagFilter=point
         tfetch3D t._x__, texCoords, gDirShadowMapTexture, OffsetX = -.5, OffsetY =  -.5, MinFilter=point, MagFilter=point
         tfetch3D t.__x_, texCoords, gDirShadowMapTexture, OffsetX = -.5, OffsetY =   .5, MinFilter=point, MagFilter=point
         
         tfetch3D u.x___, texCoords, gDirShadowMapTexture, OffsetX = .5, OffsetY = -1.5, MinFilter=point, MagFilter=point
         tfetch3D u._x__, texCoords, gDirShadowMapTexture, OffsetX = .5, OffsetY = -.5, MinFilter=point, MagFilter=point
         tfetch3D u.__x_, texCoords, gDirShadowMapTexture, OffsetX = .5, OffsetY =  .5, MinFilter=point, MagFilter=point
      };

#if 0
      float3 c0 = lerp(s, t, weights.x);
	   float3 c1 = lerp(t, u, weights.x);
	   float x00 = lerp(c0.x, c0.y, weights.y);
	   float x01 = lerp(c0.y, c0.z, weights.y);
	   float x10 = lerp(c1.x, c1.y, weights.y);
	   float x11 = lerp(c1.y, c1.z, weights.y);
	   fAvgZ = .25 * (x00 + x01 + x10 + x11);
      
      c0 = lerp(s*s, t*t, weights.x);
	   c1 = lerp(t*t, u*u, weights.x);
	   x00 = lerp(c0.x, c0.y, weights.y);
	   x01 = lerp(c0.y, c0.z, weights.y);
	   x10 = lerp(c1.x, c1.y, weights.y);
	   x11 = lerp(c1.y, c1.z, weights.y);
	   fAvgZ2 = .25 * (x00 + x01 + x10 + x11);
#else
      float3 c0 = lerp(s, t, weights.x);
	   float3 c1 = lerp(t, u, weights.x);
      float3 d = float3(.25 - .25 * weights.y, .25, .25 * weights.y);
      
      fAvgZ = dot(c0, d) + dot(c1, d);
      
      c0 = lerp(s*s, t*t, weights.x);
	   c1 = lerp(t*t, u*u, weights.x);
      fAvgZ2 = dot(c0, d) + dot(c1, d);	   
#endif      
   }	   
   else
   {
      float4 depth;
         
      asm
      {
         getWeights3D weights, texCoords, gDirShadowMapTexture, MagFilter = linear, MinFilter = linear, MipFilter = linear, VolMagFilter = point, VolMinFilter = point
               
         tfetch3D depth.x___, texCoords, gDirShadowMapTexture, OffsetX = -0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point
         tfetch3D depth._x__, texCoords, gDirShadowMapTexture, OffsetX =  0.5, OffsetY = -0.5, MinFilter=point, MagFilter=point
         tfetch3D depth.__x_, texCoords, gDirShadowMapTexture, OffsetX = -0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point
         tfetch3D depth.___x, texCoords, gDirShadowMapTexture, OffsetX =  0.5, OffsetY =  0.5, MinFilter=point, MagFilter=point
      };
      
      float4 lerpDot = float4((1-weights.y)*(1-weights.x), (1-weights.y)*(weights.x), (weights.y)*(1-weights.x), (weights.y)*(weights.x));
      
	   fAvgZ = dot(lerpDot, depth);
	   fAvgZ2 = dot(lerpDot, depth*depth);
   }	   
      
   if( shadowMapCoords.z <= fAvgZ )
      return 1.0f;
                           
   float variance = fAvgZ2 - (fAvgZ * fAvgZ);
      
   variance       = saturate(variance + g_fEpsilonVSM);

   float mean     = fAvgZ;
   float d        = shadowMapCoords.z - mean;
   float p_max    = variance / ( variance + d*d );

   return lerp(gDirShadowDarkness.x, 1.0, smoothstep(0.43f, 1.0f, p_max));
}

#define DS_WIDTH                          gDirShadowParams0.x
#define DS_HALF_PLUS_HALF_OVER_WIDTH      gDirShadowParams0.y
#define DS_FOUR_OVER_WIDTH                gDirShadowParams0.z
#define DS_INV_WIDTH                      gDirShadowParams0.w

#define DS_PASSES_MINUS_1                 gDirShadowParams1.x
#define DS_INV_PASSES                     gDirShadowParams1.y
#define DS_INV_2_PASSES                   gDirShadowParams1.z
#define DS_CSM_SCALE                      gDirShadowParams1.w

static const float3 gColors[8] = 
{
   float3(0,1,0),
   float3(0,0,1),
   float3(1,0,1),
   float3(1,1,0),
   
   float3(1,1,1),
   float3(0,1,1),
   float3(0,0,1),
   float3(1,.5,.5)
};

static float calcDirShadowFactor(float2 texCoords, float3 shadowMapCoords0, out float3 color, uniform bool fastFiltering = false)
{
   float x = floor((max(abs(shadowMapCoords0.x), abs(shadowMapCoords0.y))) * DS_CSM_SCALE);
      
   if (x < 1.0f)
      x = 0.0f;
   else 
      x = floor(min(log2(x) + 1.0, DS_PASSES_MINUS_1));
      
   float xyScale = max(pow(2.0f, (DS_PASSES_MINUS_1 - x)), 1.0);
   float3 shadowMapCoords = shadowMapCoords0 * float3(xyScale, xyScale, 1.0);

   float2 scaledCoords = abs(shadowMapCoords);
   
   float shadowArrayIndex = x * DS_INV_PASSES;
   shadowArrayIndex += DS_INV_2_PASSES;
   
   shadowMapCoords.x = (shadowMapCoords.x * .5) + DS_HALF_PLUS_HALF_OVER_WIDTH;
   shadowMapCoords.y = (shadowMapCoords.y * -.5) + DS_HALF_PLUS_HALF_OVER_WIDTH;
   shadowMapCoords.z = saturate(shadowMapCoords.z * gDirShadowZScales[x].x + gDirShadowZScales[x].y);

   if ((x >= DS_PASSES_MINUS_1) && (max(scaledCoords.x, scaledCoords.y) > .999))
   {
      color = float3(2.0f, 2.0f, 2.0f);
      return 1.0f;
   }
   
   color = gColors[x];
         
   return PCFFilter3x3(shadowMapCoords, shadowArrayIndex, fastFiltering);
}

static void computeDirectionalLighting(float3 normal, float3 reflect, float3 vecToDirLight, float specPower, out float nDotL, out float3 diffuseSum, out float3 specSum)
{
   nDotL = saturate(dot(normal, vecToDirLight));

   diffuseSum = gDirLightColor * nDotL;
   
   float rDotL = saturate(dot(reflect, vecToDirLight));
   
   float specContrib = rDotL * recipFixedFunc(specPower - specPower * rDotL + rDotL);
   //float specContrib = pow(rDotL, specPower);
      
   specSum = diffuseSum * specContrib;  
}   

static void computeDirectionalDiffuseLighting(float3 normal, float3 vecToDirLight, out float nDotL, out float3 diffuseSum)
{
   nDotL = saturate(dot(normal, vecToDirLight));

   diffuseSum = gDirLightColor * nDotL;
}   

static void computeDirectionalSpecularLighting(float3 normal, float3 reflect, float3 vecToDirLight, float specPower, out float3 specSum)
{
   float rDotL = saturate(dot(reflect, vecToDirLight));
   
   float specContrib = rDotL * recipFixedFunc(specPower - specPower * rDotL + rDotL);
   //float specContrib = pow(rDotL, specPower);
      
   specSum = specContrib;  
}  

#ifdef RENDERING_TERRAIN
static const float depthBias = .00001f;
static const float slopeScaleBias = 1.2;
#else
static const float depthBias = .00001f;
static const float slopeScaleBias = 1.2;
#endif

sampler gGradientPointSampler0 : register(s0) = sampler_state
{
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};
   
static float4 encodeDepth(float d)
{
   // MIKEY - this function was optimized thanks to using a true 16-bit
   // destination texture in the code.
   
//#ifdef RENDERING_TERRAIN
#if 1
   float4 grad;
   float4 t = float4(d, d, 0, 0);
   asm
   {  
      getGradients grad, t, gGradientPointSampler0, FetchValidOnly = false
   };

#if 0 // ORIGINAL
   d += depthBias + clamp(max(grad.x, grad.y), 0.0, .004f) * slopeScaleBias;
#else // MIKEY - Replace clamp(x,0,y<1) with min(saturate(x),y)       
   d += depthBias + min(saturate(max(grad.x, grad.y)), .004f) * slopeScaleBias;
#endif

#else   
   d += depthBias;
#endif

#if 0 // ORIGINAL - spends many instructions to break the float into 8-bit MSB and LSB
      // values for a G8R8 render target
   d = saturate(d);
         
   d = floor(d * 65535.0);
      
   float4 v;
   v.x = frac(d * 1.0/256.0) * 256.0;
   v.y = (d - v.x) * 1.0/256.0;
   v.z = 0;
   v.w = 1.0;
            
   [isolate]
   {
      v.xy *= 1.0/255.0f;
   }
#else // MIKEY - optimized to just return the full floating point value for a G16R16_EDRAM
      // render target
   float4 v = float4( d, 0, 0, 1 );
#endif
           
   return v;
}
