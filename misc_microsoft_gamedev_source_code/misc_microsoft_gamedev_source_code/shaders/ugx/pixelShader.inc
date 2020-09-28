// File: pixelShader.inc
// WARNING: All pixel shaders MUST consume all outputs from their vertex shaders!

//#define DEBUG_SHADOWBUFFER

static float gSpecToDiffuseRatio = 3.14f; 

struct PixelIn                                 
{
    float3 Normal           : NORMAL0;

#ifdef BUMP    
    float3 Tangent          : TANGENT;
    float3 Binormal         : BINORMAL;
#endif

    float4 UV               : TEXCOORD0;

    float4 PixelXForm       : COLOR0;
    
    float4 WorldPos         : TEXCOORD1;

#ifdef DIR_SHADOWING    
    float3 ShadowMapCoords0 : TEXCOORD2;
#endif    

    // X = Z, Y = Planar 
    // Z = UV2.u, W = UV2.v
    float4 FogDensities     : TEXCOORD5;
           
    float VFace             : VFACE;
    
    float3 AmbientSum       : TEXCOORD6;
};                                            

static void localLighting(int arrayOffset, float3 worldNormal, float3 worldReflect, float3 worldPos, float specPower, out float3 diffuseSum, out float3 specSum)
{
   if (gLocalShadowingEnabled)
   {
      diffuseSum = 0;
      specSum = 0;
      
      for (int i = 0; i < gNumPixelLights; i++)
         omniIlluminateShadowed(arrayOffset + i * 8, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }
   else
   {
      diffuseSum = 0;
      specSum = 0;
      
      for (int i = 0; i < gNumPixelLights; i++)
         omniIlluminate(arrayOffset + i * 8, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }         
}   

static void localLightingDiffuse(float arrayOffset, float3 worldNormal, float3 worldPos, out float3 diffuseSum)
{
   if (gLocalShadowingEnabled)
   {
      diffuseSum = 0;
      
      for (int i = 0; i < gNumPixelLights; i++)
         omniIlluminateShadowedDiffuse(arrayOffset + i * 8, worldNormal, worldPos, diffuseSum);   
   }
   else
   {
      diffuseSum = 0;
      
      for (int i = 0; i < gNumPixelLights; i++)
         omniIlluminateDiffuse(arrayOffset + i * 8, worldNormal, worldPos, diffuseSum);   
   }         
}   

// uvA.xy = UV0
// uvA.zw = UV1
// uvB.zw = UV2
static float2 selectUV(float4 uvA, float4 uvB, float selector, bool useUVSelectors)
{
   return useUVSelectors ? ((selector < 0.0) ? uvB.zw : ((selector > 0.0) ? uvA.zw : uvA.xy)) : uvA;
}

static float2 selectUV(float4 uvA, float4 uvB, float selector, float2 uvOfs, bool useUVSelectors, bool useUVOfs)
{
   float2 tex = useUVSelectors ? ((selector < 0.0) ? uvB.zw : ((selector > 0.0) ? uvA.zw : uvA.xy)) : uvA;
   return useUVOfs ? (tex + uvOfs) : tex;
}

float3 computeBlackmap(float3 color, float3 worldPos)
{
   if (!gBlackmapEnabled)
      return color;
   
   float brightness;
   float backgroundLerp;
   computeBlackmapScalar(brightness, backgroundLerp, worldPos, BlackmapSampler, gBlackmapParams0, gBlackmapParams1, gBlackmapParams2);
   return lerp(gBlackmapParams0, color * brightness, backgroundLerp);
}

static void computeLighting(
   inout PixelIn In,
   uniform bool specEnabled,
   uniform bool dirLightingEnabled,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   uniform bool twoSidedEnabled,
   uniform bool useUVSelectors,
   uniform bool useUVOfs,
   uniform bool fastFiltering,
   out float4 tangentNormal,
   out float3 worldNormal,
   out float3 worldReflect,
   out float3 ambientSum,
   out float3 diffuseSum, 
   out float3 specSum,
   out float dirShadowFactor,
   out float3 viewVector)
{
   ambientSum = In.AmbientSum;//computeSHFillLighting(worldNormal);
   
   float4 uv0 = In.UV;
   float4 uv1 = In.FogDensities;
   
   if ((twoSidedEnabled) && (In.VFace >= 0.0))
   {
      In.Normal = -In.Normal;
   }
         
#ifdef BUMP   
   float4 normalSample = tex3D(BumpSampler, float3(selectUV(uv0, uv1, BUMP_UV_SELECTOR, BUMP_UV_OFS, useUVSelectors, useUVOfs), BUMP_W_OFS));
   float4 normal = unpackDXNNormal(normalSample);
   tangentNormal = normal;
   worldNormal = normalize(normal.x * In.Tangent + normal.y * In.Binormal + normal.z * In.Normal);
#else
   tangentNormal = float4(0, 0, 1, 1);
   worldNormal = normalize(In.Normal);
#endif

   viewVector = normalize(gWorldCameraPos - In.WorldPos);
      
   worldReflect = reflect(-viewVector, worldNormal);

   diffuseSum = 0;
   specSum = 0;
   dirShadowFactor = 1.0;

   if (dirLightingEnabled)
   {
      float nDotL;
      if (specEnabled)
         computeDirectionalLighting(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecColorPower.w, nDotL, diffuseSum, specSum);
      else
         computeDirectionalDiffuseLighting(worldNormal, gDirLightVecToLightWorld, nDotL, diffuseSum);

#ifdef DIR_SHADOWING   
      if (dirShadowingEnabled)
      {
         float3 shadowLevelColor;
         dirShadowFactor = calcDirShadowFactor(In.UV, In.ShadowMapCoords0, shadowLevelColor, fastFiltering);

#ifdef DEBUG_SHADOWBUFFER
         diffuseSum += shadowLevelColor;
#endif   
                  
         diffuseSum *= dirShadowFactor;
         specSum *= dirShadowFactor;
      }
#endif      
   }

   if (localLightingEnabled)
   {
      if (specEnabled)
      {
         float3 localDiffuseSum, localSpecSum;
         
         localLighting(In.WorldPos.w, worldNormal, worldReflect, In.WorldPos, gSpecColorPower.w, localDiffuseSum, localSpecSum);
         
         diffuseSum += localDiffuseSum;
         specSum += localSpecSum;
      }
      else
      {
         float3 localDiffuseSum;
         
         localLightingDiffuse(In.WorldPos.w, worldNormal, In.WorldPos, localDiffuseSum);
         
         diffuseSum += localDiffuseSum;
      }
   }
   
   if (gLightBufferingEnabled)
   {
      float3 lightBufUVW;
      
      lightBufUVW.x = dot(float4(In.WorldPos.x, In.WorldPos.y, In.WorldPos.z, 1.0f), gWorldToLightBufCol0);
      lightBufUVW.y = dot(float4(In.WorldPos.x, In.WorldPos.y, In.WorldPos.z, 1.0f), gWorldToLightBufCol1);
      lightBufUVW.z = dot(float4(In.WorldPos.x, In.WorldPos.y, In.WorldPos.z, 1.0f), gWorldToLightBufCol2);
      
      float4 lightColorSample = tex3D(LightBufferColorSampler, lightBufUVW);
      float3 lightColor = lightColorSample * 12.0f;
      float3 lightNorm = tryNormalize(tex3D(LightBufferVectorSampler, lightBufUVW) - float3(.5f, .5f, .5f));
      
      float3 diffuseContrib = saturate(dot(lightNorm, worldNormal)) * lightColor;
      diffuseSum += diffuseContrib;
      
      float rDotL = max(0.0, dot(worldReflect, lightNorm));
      specSum += lightColorSample.w * rDotL * recipFixedFunc(gSpecColorPower.w - (gSpecColorPower.w - 1.0f) * rDotL) * diffuseContrib * 3.14f;
   }
}   
  
static float4 psMainInternal(
   PixelIn In,
   
   uniform bool specEnabled,
   uniform bool dirLightingEnabled,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   
   uniform bool specMapEnabled,
   uniform bool opacityMapEnabled,
   
   uniform bool xformMapEnabled,
   uniform bool emXformMapEnabled,
   
   uniform bool selfMapEnabled,
   
   uniform bool aoMapEnabled,
   
   uniform bool envMapEnabled,
   uniform bool envMapMaskEnabled,
   uniform bool twoSidedEnabled,
   
   uniform bool useUVSelectors,
   uniform bool useUVOfs,

   uniform bool localReflectionEnabled,
   
   uniform bool highlightMapEnabled,
   uniform bool modulateMapEnabled
   
   ) : COLOR0
{
   float4 uv0 = In.UV;
   float4 uv1 = In.FogDensities;

   float opacity = gTintColor.w;
   if (opacityMapEnabled)
   {
      opacity *= tex2D(OpacitySampler, selectUV(uv0, uv1, OPACITY_UV_SELECTOR, OPACITY_UV_OFS, useUVSelectors, useUVOfs)).g;
                        
      clip(opacity - 2.0/255.0);
   }         
                                     
   float4 tangentNormal;
   float3 worldNormal, worldReflect, ambientSum, diffuseSum, specSum, viewVector;     
   float dirShadowFactor;
   
   computeLighting(
      In, 
      specEnabled, dirLightingEnabled, dirShadowingEnabled, localLightingEnabled, twoSidedEnabled, useUVSelectors, useUVOfs, false,
      tangentNormal, worldNormal, worldReflect, ambientSum, diffuseSum, specSum, dirShadowFactor, viewVector);
         
   if (aoMapEnabled)
      ambientSum *= tex2D(AOSampler, selectUV(uv0, uv1, AO_UV_SELECTOR, AO_UV_OFS, useUVSelectors, useUVOfs));

   diffuseSum += ambientSum * In.PixelXForm.w;

   float4 albedo = tex3D(DiffuseSampler, float3(selectUV(uv0, uv1, DIFFUSE_UV_SELECTOR, DIFFUSE_UV_OFS, useUVSelectors, useUVOfs), DIFFUSE_W_OFS));
   
   if (modulateMapEnabled)
      albedo *= tex2D(ModulateSampler, selectUV(uv0, uv1, MODULATE_UV_SELECTOR, MODULATE_UV_OFS, useUVSelectors, useUVOfs));
      
   float xform = albedo.w;
   if (xformMapEnabled)
      xform = tex2D(XFormSampler, selectUV(uv0, uv1, XFORM_UV_SELECTOR, XFORM_UV_OFS, useUVSelectors, useUVOfs)).g;
   albedo.xyz = lerp(albedo * In.PixelXForm, albedo, xform);
   
   if (gLerpTexEnabled)
   {
      float4 worldPosH = float4(In.WorldPos.x, In.WorldPos.y, In.WorldPos.z, 1.0f);
      float4 lerpTex = gLerpTexParams * tex2D(LerpTexSampler, float2(dot(worldPosH, gLerpTexUVXForm0), dot(worldPosH, gLerpTexUVXForm1)));
      albedo.xyz = lerp(albedo.xyz, lerpTex.xyz, lerpTex.w);
   }
   
   float3 result = gTintColor + diffuseSum * albedo;

   if (selfMapEnabled)
   {
      float3 self = unpackDXT5H(tex2D(SelfSampler, selectUV(uv0, uv1, SELF_UV_SELECTOR, SELF_UV_OFS, useUVSelectors, useUVOfs)), gHDRTexScale.x);
      
      if (gEmXFormMapEnabled)
      {
         float3 emXForm = tex2D(EmXFormSampler, selectUV(uv0, uv1, EMXFORM_UV_SELECTOR, EMXFORM_UV_OFS, useUVSelectors, useUVOfs)).g;
         
         self = lerp(self * In.PixelXForm, self, emXForm);
      }
      
      result += self;
   }
   
   if (highlightMapEnabled)
      result += unpackDXT5H(tex2D(HighlightSampler, selectUV(uv0, uv1, HIGHLIGHT_UV_SELECTOR, HIGHLIGHT_UV_OFS, useUVSelectors, useUVOfs)), gHDRTexScale.z);
   
   float3 specMap;
   
   if (specEnabled)
   {
      specSum *= gSpecToDiffuseRatio; 
   
      specMap = specMapEnabled ? (tex2D(SpecularSampler, selectUV(uv0, uv1, SPEC_UV_SELECTOR, SPEC_UV_OFS, useUVSelectors, useUVOfs)) * gSpecColorPower) : gSpecColorPower;
      
      if (gLerpTexEnabled)
         specMap *= (1.0f - gLerpTexParams.w);
         
      result += specSum * specMap;
   }
     
   if (envMapEnabled)
   {
      if (dirLightingEnabled)
         dirShadowFactor = lerp(gDirShadowDarkness, 1.0, saturate(dot(In.Normal, gDirLightVecToLightWorld) * 16.0));
      
      float3 env = unpackDXT5H(texCUBEbias(EnvSampler, float4(worldReflect, gEnvVisControl.y)), gHDRTexScale.y);

      if (localReflectionEnabled)
      {
         if (gLocalReflectionEnabled)
         {
            // Calculate normalized screen space coordinates
            float4 worldPos = float4(In.WorldPos.xyz, 1);
            float4 screenPos = mul(worldPos, gSceneWorldToProj);
            float2 screenSpaceTexCoords = screenPos.xy * recipFixedFunc(screenPos.w);
            screenSpaceTexCoords = 1.0f - (screenSpaceTexCoords * 0.5f + 0.5f);

            float bumpRatio = 0.25f;
            float2 factorPerturb = 1.0 - (tangentNormal.xy * bumpRatio);
            screenSpaceTexCoords *= factorPerturb;
            float4 localRefl = tex2D(gReflectionTexture, screenSpaceTexCoords);
            env = lerp(env, localRefl.rgb, localRefl.a);
         }
      }

      float r = gEnvVisControl.z;
      float fresnelApprox = r + (1.0 - r) * pow(saturate(1.0 - dot(worldNormal, viewVector)), gEnvVisControl.x);
      
      env *= gEnvVisControl.w * lerp(.1, 1.0, dirShadowFactor);
                  
      if (envMapMaskEnabled)
         env *= tex2D(EnvMaskSampler, selectUV(uv0, uv1, ENVMASK_UV_SELECTOR, ENVMASK_UV_OFS, useUVSelectors, useUVOfs));
      else if (specEnabled)
         env *= specMap;
      
      result += env * fresnelApprox;
   }
   
   if (gAddTexEnabled)
   {
      float4 worldPosH = float4(In.WorldPos.x, In.WorldPos.y, In.WorldPos.z, 1.0f);
      float3 addTex = gAddTexParams * tex2D(AddTexSampler, float2(dot(worldPosH, gAddTexUVXForm0), dot(worldPosH, gAddTexUVXForm1)));
      result += addTex * gAddTexParams.w;
   }

   result = computeBlackmap(result, In.WorldPos);
   result = computeFog(result, In.FogDensities.x, In.FogDensities.y);

   return float4(result, opacity);
} 

float4 psMain(
   PixelIn In,
   uniform bool specularEnabled,
   uniform bool dirLightingAnd,  uniform bool dirLightingOr,
   uniform bool dirShadowing,
   uniform bool localLightingAnd, uniform bool localLightingOr,
   uniform bool specMapAnd,      uniform bool specMapOr,
   uniform bool opacityMapAnd,   uniform bool opacityMapOr,
   uniform bool XFormMapAnd,     uniform bool XFormMapOr,
   uniform bool EmXFormMapAnd,   uniform bool EmXFormMapOr,
   uniform bool selfMapAnd,      uniform bool selfMapOr,
   uniform bool AOMapAnd,        uniform bool AOMapOr,
   uniform bool envMapAnd,       uniform bool envMapOr,
   uniform bool envMaskMapAnd,   uniform bool envMaskMapOr,
   uniform bool twoSidedAnd,     uniform bool twoSidedOr,
   
   uniform bool useUVSelectors,
   uniform bool useUVOfs,
   
   uniform bool localReflectionAnd, uniform bool localReflectionOr,
   
   uniform bool highlightMapAnd, uniform bool highlightMapOr,
   
   uniform bool modulateMapAnd, uniform bool modulateMapOr
   
   ) : COLOR                
{  
   return psMainInternal(
      In,
      
      specularEnabled,
      !dirLightingOr  ? (dirLightingAnd    && gDirLightEnabled)   : true,
      dirShadowing,
      !localLightingOr ? (localLightingAnd && gLocalPixelLightingEnabled) : true,
      
      !specMapOr     ? (specMapAnd       && gSpecMapEnabled)      : true,
      !opacityMapOr  ? (opacityMapAnd    && gOpacityMapEnabled)   : true,
      !XFormMapOr    ? (XFormMapAnd      && gXFormMapEnabled)     : true,
      !EmXFormMapOr  ? (EmXFormMapAnd    && gEmXFormMapEnabled)   : true,
      
      !selfMapOr     ? (selfMapAnd       && gSelfMapEnabled)      : true,
      !AOMapOr       ? (AOMapAnd         && gAOMapEnabled)        : true,
      
      !envMapOr      ? (envMapAnd        && gEnvMapEnabled)       : true,
      !envMaskMapOr  ? (envMaskMapAnd    && gEnvMaskMapEnabled)   : true,
      !twoSidedOr    ? (twoSidedAnd      && gTwoSided)            : true,
      
      useUVSelectors,
      useUVOfs,

      !localReflectionOr ? (localReflectionAnd && gLocalReflectionEnabled) : true,
      
      !highlightMapOr ? (highlightMapAnd && gHighlightMapEnabled) : true,
      
      !modulateMapOr ? (modulateMapAnd && gModulateMapEnabled) : true
   );      
}   

float4 psSky(
   PixelIn In,
   uniform bool useUVSelectors, 
   uniform bool useUVOfs) : COLOR0
{
   float4 uv0 = In.UV;
   float4 uv1 = In.FogDensities;
   
   float3 result = unpackDXT5H(tex2D(SelfSampler, selectUV(uv0, uv1, SELF_UV_SELECTOR, SELF_UV_OFS, useUVSelectors, useUVOfs)), gHDRTexScale.x);
   return float4(result, 1.0);
} 

float4 psMainVis(
   PixelIn In,
   uniform bool dirLightingEnabled,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   uniform bool useUVSelectors,
   uniform bool useUVOfs
   ) : COLOR                
{  
   float4 uv0 = In.UV;
   float4 uv1 = In.FogDensities;
      
   float4 tangentNormal;
   float3 worldNormal, worldReflect, ambientSum, diffuseSum, specSum, viewVector;     
   float dirShadowFactor;
   
   computeLighting(In, 
      true, dirLightingEnabled, dirShadowingEnabled, localLightingEnabled, gTwoSided, useUVSelectors, useUVOfs, false, 
      tangentNormal, worldNormal, worldReflect, ambientSum, diffuseSum, specSum, dirShadowFactor, viewVector);
      
   float4 albedo = tex3D(DiffuseSampler, float3(selectUV(uv0, uv1, DIFFUSE_UV_SELECTOR, DIFFUSE_UV_OFS, useUVSelectors, useUVOfs), DIFFUSE_W_OFS));
      
   float3 ao      = gAOMapEnabled       ? tex2D(AOSampler, selectUV(uv0, uv1, AO_UV_SELECTOR, AO_UV_OFS, useUVSelectors, useUVOfs)) : 1;
   float3 xform   = gXFormMapEnabled    ? tex2D(XFormSampler, selectUV(uv0, uv1, XFORM_UV_SELECTOR, XFORM_UV_OFS, useUVSelectors, useUVOfs)).g : albedo.w;
   float3 emXForm = gEmXFormMapEnabled  ? tex2D(EmXFormSampler, selectUV(uv0, uv1, EMXFORM_UV_SELECTOR, EMXFORM_UV_OFS, useUVSelectors, useUVOfs)).g : albedo.w;
   
   float3 self    = gSelfMapEnabled     ? unpackDXT5H(tex2D(SelfSampler, selectUV(uv0, uv1, SELF_UV_SELECTOR, SELF_UV_OFS, useUVSelectors, useUVOfs)), gHDRTexScale.x)           : 0;
               
   float3 env     = gEnvMapEnabled      ? unpackDXT5H(texCUBEbias(EnvSampler, float4(worldReflect, gEnvVisControl.y)), gHDRTexScale.y) : 0;
   env *= gEnvVisControl.w * lerp(.1, 1.0, dirShadowFactor);
      
   float3 spec    = gSpecMapEnabled     ? tex2D(SpecularSampler, selectUV(uv0, uv1, SPEC_UV_SELECTOR, SPEC_UV_OFS, useUVSelectors, useUVOfs)) : 1;
   float3 opacity = gOpacityMapEnabled  ? tex2D(OpacitySampler, selectUV(uv0, uv1, OPACITY_UV_SELECTOR, OPACITY_UV_OFS, useUVSelectors, useUVOfs)).g      : 1;
   
   float3 envMask = gEnvMaskMapEnabled  ? tex2D(EnvMaskSampler, selectUV(uv0, uv1, ENVMASK_UV_SELECTOR, ENVMASK_UV_OFS, useUVSelectors, useUVOfs)) : 1;
   
   float3 highlight = gHighlightMapEnabled ? unpackDXT5H(tex2D(HighlightSampler, selectUV(uv0, uv1, HIGHLIGHT_UV_SELECTOR, HIGHLIGHT_UV_OFS, useUVSelectors, useUVOfs)), gHDRTexScale.z) : 0;
   float3 modulate = gModulateMapEnabled ? tex2D(ModulateSampler, selectUV(uv0, uv1, MODULATE_UV_SELECTOR, MODULATE_UV_OFS, useUVSelectors, useUVOfs)) : 1;
   
   if (gAOMapEnabled)
   {
      ambientSum *= ao;
   }
   
   ambientSum *= In.PixelXForm.w;
         
   float3 result = albedo;
   
   float visMode = gEnvVisControl.x;

   if (visMode >= 1.0)  result = ao;
   if (visMode >= 2.0)  result = xform;
   if (visMode >= 3.0)  result = emXForm;
   if (visMode >= 4.0)  result = self;
      
   if (visMode >= 5.0)  result = envMask;
   if (visMode >= 6.0)  result = env;
   if (visMode >= 7.0)  result = spec;
   //if (visMode >= 8.0)  result = sqrt(opacity);
   if (visMode >= 8.0)  result = opacity;
   
   if (visMode >= 9.0)  result = ambientSum;
   if (visMode >= 10.0) result = diffuseSum;
   if (visMode >= 11.0) result = specSum;
   if (visMode >= 12.0) result = (.5 + .5 * worldNormal);
   
   if (visMode >= 13.0) result = (.5 + .5 * tangentNormal);
   if (visMode >= 14.0) result = gSpecColorPower;
   if (visMode >= 15.0) result = float3(gSpecColorPower.w*1.0f/100.0, gSpecColorPower.w*1.0f/100.0, gSpecColorPower.w*1.0f/100.0);
   if (visMode >= 16.0) result = highlight;
   if (visMode >= 17.0) result = modulate;
      
   return float4(result, 1);
}  

// FIXME: This is also executed for local shadows!
float4 psShadowMain(in float depth : TEXCOORD0, uniform bool overallAlpha) : COLOR                
{
   float4 v = encodeDepth(depth);
   if (overallAlpha)
      v.w = gTintColor.w;
   return v;
}

// FIXME: This is also executed for local shadows!
float4 psShadowMainAlphaTest(
   in float4 uv0 : TEXCOORD0, 
   in float4 uv1 : TEXCOORD5, 
   in float depth : TEXCOORD1,
   uniform bool useUVSelectors, 
   uniform bool useUVOfs) : COLOR                
{
   float4 v = encodeDepth(depth);
   v.w = tex2D(OpacitySampler, selectUV(uv0, uv1, OPACITY_UV_SELECTOR, OPACITY_UV_OFS, useUVSelectors, useUVOfs)).g * gTintColor.w;
   return v;
}

float4 psDPShadowMain(in float depth : TEXCOORD0, uniform bool overallAlpha) : COLOR
{
   float a = 1.0f;
   
   if (overallAlpha)
      a = gTintColor.w;
         
   return (depth < .1f) ? 0.0f : a;
}

float4 psDPShadowMainAlphaTest(
   in float4 uv0 : TEXCOORD0, 
   in float4 uv1 : TEXCOORD5, 
   in float depth : TEXCOORD1,
   uniform bool useUVSelectors, 
   uniform bool useUVOfs) : COLOR                
{
   if (depth < 1.0f)
      return 0.0f;
   else
   {
      float a = tex2D(OpacitySampler, selectUV(uv0, uv1, OPACITY_UV_SELECTOR, OPACITY_UV_OFS, useUVSelectors, useUVOfs)).g * gTintColor.w;
      if (gTintColor.w < .1f)
         return a;
      else
         return (a < .5f) ? 0.0f : 1.0f;
   }
}

float4 psDistortionMain(PixelIn In) : COLOR
{
   float4 uv0 = In.UV;
   float4 uv1 = In.FogDensities;
   
   float2 d = unpackDXNNormal(tex2D(DiffuseSampler, selectUV(uv0, uv1, DISTORTON_UV_SELECTOR, DIFFUSE_UV_OFS, true, true)));
      
   if (abs(d.x) <= 3.0f/255.0f) 
      d.x = 0.0f;
      
   if (abs(d.y) <= 3.0f/255.0f) 
      d.y = 0.0f;
   
   d.xy *= gTintColor.w;
   
   return float4(d.x, d.y, 0.0f, abs(d.x) + abs(d.y));
}