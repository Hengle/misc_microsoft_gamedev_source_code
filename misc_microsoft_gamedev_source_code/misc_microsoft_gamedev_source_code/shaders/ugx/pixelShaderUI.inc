static void computeDirectionalLightingUI(float3 normal, float3 reflect, float3 vecToDirLight, float specPower, out float nDotL, out float3 diffuseSum, out float3 specSum)
{
   nDotL = saturate(dot(normal, vecToDirLight));

   diffuseSum = gTintColor * nDotL;
   
   float rDotL = saturate(dot(reflect, vecToDirLight));
   
   float specContrib = rDotL * recipFixedFunc(specPower - specPower * rDotL + rDotL);
      
   specSum = diffuseSum * specContrib;  
}   

static void computeLightingUI(
   inout PixelIn In,
   uniform bool dirLightingEnabled,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   uniform bool useUVSelectors,
   uniform bool useUVOfs,
   out float4 tangentNormal,
   out float3 worldNormal,
   out float3 worldReflect,
   out float3 ambientSum,
   out float3 diffuseSum, 
   out float3 specSum,
   out float dirShadowFactor,
   out float3 viewVector)
{
   if ((gTwoSided) && (In.VFace >= 0.0))
   {
      In.Normal = -In.Normal;
#ifdef BUMP      
      //In.Tangent = -In.Tangent;
      //In.Binormal = -In.Binormal;
#endif      
   }
   
#ifdef BUMP   
   float4 normal = unpackDXNNormal(tex2D(BumpSampler, selectUV(In.UV, In.FogDensities, BUMP_UV_SELECTOR, BUMP_UV_OFS, useUVSelectors, useUVOfs)));
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
      computeDirectionalLightingUI(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecColorPower.w, nDotL, diffuseSum, specSum);

#ifdef DIR_SHADOWING   
      if (dirShadowingEnabled)
      {
         float3 shadowLevelColor;
         dirShadowFactor = calcDirShadowFactor(In.UV, In.ShadowMapCoords0, shadowLevelColor);

#ifdef DEBUG_SHADOWBUFFER
         diffuseSum += shadowLevelColor * .1f;
#endif   
                  
         diffuseSum *= dirShadowFactor;
         specSum *= dirShadowFactor;
      }
#endif      
   }
           
   if (localLightingEnabled)
   {
      float3 localDiffuseSum, localSpecSum;

      localLighting(In.WorldPos.w, worldNormal, worldReflect, In.WorldPos, gSpecColorPower.w, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   }

   ambientSum = computeSHFillLighting(worldNormal);
}   

static float4 psMainInternalUI(
   PixelIn In,
   uniform bool dirLightingEnabled,
   uniform bool localLightingEnabled,
   uniform bool specMapEnabled,
   uniform bool opacityMapEnabled,
   uniform bool selfMapEnabled,
   uniform bool envMapEnabled,
   uniform bool envMapMaskEnabled,
   uniform bool useUVSelectors,
   uniform bool useUVOfs
   )
{
   float4 uv0 = In.UV;
   float4 uv1 = In.FogDensities;

   float4 tangentNormal;
   float3 worldNormal, worldReflect, ambientSum, diffuseSum, specSum, viewVector;     
   float dirShadowFactor;
   computeLightingUI(In, 
      dirLightingEnabled, 
      false, 
      localLightingEnabled, 
      useUVSelectors, 
      useUVOfs, 
      tangentNormal, 
      worldNormal, 
      worldReflect, 
      ambientSum, 
      diffuseSum, 
      specSum, 
      dirShadowFactor, 
      viewVector);

   diffuseSum += ambientSum.rgb;// * gTintColor.rgb;
                                  
   float4 albedo = tex2D(DiffuseSampler, selectUV(uv0, uv1, DIFFUSE_UV_SELECTOR, DIFFUSE_UV_OFS, true, true));
   float3 result = albedo.xyz * diffuseSum.xyz;
   

   specSum *= 3.0f;

   result += specSum;
   
   result = saturate(result);

   float3 specMap = gSpecColorPower;
   if (envMapEnabled)
   {      
      dirShadowFactor = lerp(gDirShadowDarkness, 1.0, saturate(dot(In.Normal, gDirLightVecToLightWorld) * 16.0));
      
      float bias = gEnvVisControl.y;// + (1.0-dirShadowFactor) * 2.0;
      float3 env = unpackDXT5H(texCUBEbias(EnvSampler, float4(worldReflect, bias)), gHDRTexScale.y);
      float r = 0.5;
      float fresnelApprox = r + (1.0 - r) * pow(saturate(1.0 - dot(worldNormal, viewVector)), 4.0);
            
      env *= gEnvVisControl.w * lerp(.1, 1.0, dirShadowFactor);
                  
      if (envMapMaskEnabled)
         env *= tex2D(EnvMaskSampler, selectUV(uv0, uv1, ENVMASK_UV_SELECTOR, ENVMASK_UV_OFS, true, true));
      else
         env *= specMap;
      
      result = lerp(result, env, fresnelApprox);
   }

   float ooGamma = 1.0f/2.1f;
   float eps = 1.0f / 4096.0f;

   result.r = pow(result.r + eps, ooGamma);
   result.g = pow(result.g + eps, ooGamma);
   result.b = pow(result.b + eps, ooGamma);

   if (selfMapEnabled)
      result += tex2D(SelfSampler, selectUV(uv0, uv1, SELF_UV_SELECTOR, SELF_UV_OFS, useUVSelectors, useUVOfs));
            
   float opacity = gTintColor.w * albedo.w;
   return float4(result, opacity);
} 

float4 psMainUI(
   PixelIn In,
   uniform bool dirLighting,     
   uniform bool localLighting,
   uniform bool specMapAnd,      uniform bool specMapOr,
   uniform bool opacityMapAnd,   uniform bool opacityMapOr,
   uniform bool selfMapAnd,      uniform bool selfMapOr,
   uniform bool envMapAnd,       uniform bool envMapOr,
   uniform bool envMaskMapAnd,   uniform bool envMaskMapOr,   
   uniform bool useUVSelectors,
   uniform bool useUVOfs
   ) : COLOR                
{  
   return psMainInternalUI(In,
      dirLighting    && gDirLightEnabled,
      localLighting  && gLocalPixelLightingEnabled,
      !specMapOr     ? (specMapAnd       && gSpecMapEnabled)      : true,
      !opacityMapOr  ? (opacityMapAnd    && gOpacityMapEnabled)   : true,
      !selfMapOr     ? (selfMapAnd       && gSelfMapEnabled)      : true,
      !envMapOr      ? (envMapAnd        && gEnvMapEnabled)       : true,
      !envMaskMapOr  ? (envMaskMapAnd    && gEnvMaskMapEnabled)   : true,
      useUVSelectors,
      useUVOfs);      
}


