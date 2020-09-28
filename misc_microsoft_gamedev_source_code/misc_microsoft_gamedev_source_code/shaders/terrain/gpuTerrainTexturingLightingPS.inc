


bool gLocalLightingEnabled		: register(ENABLE_LOCAL_LIGHTS_REG);
bool gLocalShadowingEnabled   : register(ENABLE_LOCAL_SHADOWING_REG);
int gNumLights					   : register(NUM_LOCAL_LIGHTS_REG);

bool gExtendedLocalLightingEnabled : register(ENABLE_EXTENDED_LOCAL_LIGHTS_REG);
int gNumExtendedLights        : register(NUM_EXTENDED_LOCAL_LIGHTS_REG);
float4 gExtendedLocalLightingParams : register(EXTENDED_LOCAL_LIGHTING_PARAMS_REG);

float gSpecPower = 25.0f;
float gAODiffuseIntensity = 0.0f;

static float gSpecToDiffuseRatio = 3.14f; // The artists should not be able to control the specular to diffuse ratio (the actual ratio is Pi).

float4 gSpecOnlyDir_Dir_Inten = float4(0,1,0,25);	//XYZ = dir, W = Intensity
float4 gSpecOnlyDir_Col_ShadScl = float4(0,0,0,1);	//XYZ = col, W = Shadow scalar

bool gEnableLightBuffering;
float4x4 gWorldToLightBuf;
sampler LightBufferColorSampler	= sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;
   MipFilter = NONE;

   AddressU = CLAMP;
   AddressV = CLAMP;
   AddressW = CLAMP;

   SEPARATEZFILTERENABLE = FALSE;
};

sampler LightBufferVectorSampler	= sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;
   MipFilter = NONE;

   AddressU = CLAMP;
   AddressV = CLAMP;
   AddressW = CLAMP;

   SEPARATEZFILTERENABLE = FALSE;
};

void sampleLightBuffer(
   in float3 worldPos, in float3 worldNormal, in float3 worldReflect, in float specPower,
   out float3 diffuseSum,
   out float3 specSum)
{   
   float3 lightBufUVW = mul(float4(worldPos.x, worldPos.y, worldPos.z, 1.0f), gWorldToLightBuf);
   float4 lightColorSample = tex3D(LightBufferColorSampler, lightBufUVW);
   float3 lightColor = lightColorSample * 12.0f;

   float4 lightVecSample = tex3D(LightBufferVectorSampler, lightBufUVW);
      
   float3 lightVec = lightVecSample - float3(.5f, .5f, .5f);
   float3 lightNorm = tryNormalize(lightVec);
              
   float nDotL = saturate(dot(lightNorm, worldNormal));

   float3 diffuseContrib = nDotL * lightColor;
   diffuseSum = diffuseContrib;
   
   float rDotL = max(0.0, dot(worldReflect, lightNorm));
   specSum = lightColorSample.w * rDotL * recipFixedFunc(specPower - (specPower - 1.0f) * rDotL) * diffuseContrib * 3.14f;
}

static void localLightingExtended(float3 worldPos, float3 worldNormal, float3 worldReflect, float specPower, out float3 diffuseSum, out float3 specSum)
{
   diffuseSum = 0.0;
   specSum = 0.0;
   
   for (int i = 0; i < gNumExtendedLights; i++)
      omniIlluminateExtended(gExtendedLocalLightingParams.x + i * 4, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
}   

static void localLighting(float3 worldPos, float3 worldNormal, float3 worldReflect, float specPower, out float3 diffuseSum, out float3 specSum)
{
   diffuseSum = 0.0;
   specSum = 0.0;
   
   if (gLocalShadowingEnabled)
   {   
      for (int i = 0; i < gNumLights; i++)
         omniIlluminateShadowed(i, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }
   else
   {
      for (int i = 0; i < gNumLights; i++)
         omniIlluminate(i, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }         
}   

void localDiffuseLighting(float3 worldPos, float3 worldNormal, out float3 diffuseSum)
{
   diffuseSum = 0.0;
   
   if (gLocalShadowingEnabled)
   {   
      for (int i = 0; i < gNumLights; i++)
         omniIlluminateShadowedDiffuse(i, worldNormal, worldPos, diffuseSum);   
   }
   else
   {
      for (int i = 0; i < gNumLights; i++)
         omniIlluminateDiffuse(i, worldNormal, worldPos, diffuseSum);   
   }         
}   
static void computeLighting(
   PS_INPUT IN,
   float3 normal,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   out float3 worldNormal,
   out float3 worldReflect,
   out float3 ambientSum,
   out float3 diffuseSum, 
   out float3 specSum )
{
	diffuseSum = 0;
	specSum = 0;
		
	worldNormal = normalize(normal.x * IN.tangent + normal.y * IN.binormal + normal.z * IN.normal);
	   
	worldReflect = reflect(normalize(IN.worldPos - gWorldCameraPos), worldNormal);
	
   float nDotL;
   computeDirectionalLighting(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecPower, nDotL, diffuseSum, specSum);

   float3 specOnlySum;
   computeDirectionalSpecularLighting(worldNormal, worldReflect, gSpecOnlyDir_Dir_Inten.xyz, gSpecOnlyDir_Dir_Inten.w, specOnlySum);
   specOnlySum *= gSpecOnlyDir_Col_ShadScl.xyz;
         
   if ((dirShadowingEnabled) && (nDotL > 0.0f))
   {
      float3 shadowLevelColor;
      float shadowFactor = calcDirShadowFactor(IN.uv0, IN.shadowMapCoords0, shadowLevelColor);   

#ifdef DEBUG_SHADOWBUFFER
      diffuseSum += shadowLevelColor*1.0f;
#endif

      diffuseSum *= shadowFactor;
      specSum *= shadowFactor;
      specOnlySum *= lerp(1,shadowFactor,gSpecOnlyDir_Col_ShadScl.w);
   }
         
   diffuseSum += IN.addLocalLight * 2;
 
   diffuseSum = lerp(diffuseSum, diffuseSum * IN.ao_alpha.x, gAODiffuseIntensity);
   ambientSum = computeSHFillLighting(worldNormal);
   specSum += specOnlySum;
   
   if (gLocalLightingEnabled)
   {
      float3 localDiffuseSum, localSpecSum;
      localLighting(IN.worldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   } 

   if (gExtendedLocalLightingEnabled)
   {
      float3 localDiffuseSum, localSpecSum;
      localLightingExtended(IN.worldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   }
   
   if (gEnableLightBuffering)
   {
      float3 localDiffuseSum, localSpecSum;
      sampleLightBuffer(IN.worldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   }
   
}    

static void computeLighting_NLL(
   PS_INPUT IN,
   float3 normal,
   uniform bool dirShadowingEnabled,
   uniform bool specEnabled,
   out float3 worldNormal,
   out float3 worldReflect,
   out float3 ambientSum,
   out float3 diffuseSum, 
   out float3 specSum)
{
	diffuseSum = 0;
	specSum = 0;
	float3 specOnlySum = float3(0,0,0);
			
	worldNormal = normalize(normal.x * IN.tangent + normal.y * IN.binormal + normal.z * IN.normal);
	   
	worldReflect = reflect(normalize(IN.worldPos - gWorldCameraPos), worldNormal);
	
   float nDotL;
	if(specEnabled)
	{
		computeDirectionalLighting(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecPower, nDotL, diffuseSum, specSum);
		computeDirectionalSpecularLighting(worldNormal, worldReflect, gSpecOnlyDir_Dir_Inten.xyz, gSpecOnlyDir_Dir_Inten.w, specOnlySum);
		specOnlySum *= gSpecOnlyDir_Col_ShadScl.xyz;
	}
	else
	{
		computeDirectionalDiffuseLighting(worldNormal, gDirLightVecToLightWorld, nDotL, diffuseSum);
	}
		
   
  

   if ((dirShadowingEnabled) && (nDotL > 0.0f))
   {
      float3 shadowLevelColor;
      float shadowFactor = calcDirShadowFactor(IN.uv0, IN.shadowMapCoords0, shadowLevelColor);   

#ifdef DEBUG_SHADOWBUFFER
      diffuseSum += shadowLevelColor*1.0f;
#endif

      diffuseSum *= shadowFactor;
      specSum *= shadowFactor;
      specOnlySum *= lerp(1,shadowFactor,gSpecOnlyDir_Col_ShadScl.w);
   }
         
   diffuseSum += IN.addLocalLight * 2;
    
   diffuseSum = lerp(diffuseSum, diffuseSum * IN.ao_alpha.x, gAODiffuseIntensity);

   if (gEnableLightBuffering)
   {
      float3 localDiffuseSum, localSpecSum;
      sampleLightBuffer(IN.worldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   }
   
   ambientSum = computeSHFillLighting(worldNormal);
   specSum +=specOnlySum;
}    

//-----------------------------------------------------------------------------
static float calcFresnelReflectCoeff(PS_INPUT IN,float3 worldNormal,out float3 refrVector)
{ 
/*
	const float fresPower = 3.5f;
	const float fresScale = 10.f;
	const float fresBias = 0.f;
	//etaRatio is an offset for chromatic dispersion
	const float3 etaRatio = float3(0.5f,0.5f,0.5f);
*/
	
	float3 I		= normalize(IN.worldPos - gWorldCameraPos);
	
	refrVector		= refract(I,worldNormal,0.75f);
							
	//return saturate(fresBias + fresScale * pow(1 + dot(I,worldNormal),fresPower));
	return saturate(20*pow(1+dot(I,worldNormal),3.5f));
	
}

//-----------------------------------------------------------------------------
float4 mypsA(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	float4 normal = float4(0,0,1,1);

	computeLighting(IN,normal,dirShadowingEnabled,localLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
    diffuseSum += ambientSum*IN.ao_alpha.x;
       
	float3 result = diffuseSum * albedo;

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);

   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy)); 
}
//-----------------------------------------------------------------------------
float4 mypsAN(PS_INPUT IN,  uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;

	computeLighting(IN,normal,dirShadowingEnabled, localLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
    diffuseSum += ambientSum*IN.ao_alpha.x;
       
	float3 result = diffuseSum * albedo;

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);

   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy)); 
}

//-----------------------------------------------------------------------------
float4 mypsANS(PS_INPUT IN,  uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;

	computeLighting(IN,normal,dirShadowingEnabled, localLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
    diffuseSum += ambientSum*IN.ao_alpha.x;
       
	float3 result = diffuseSum * albedo;
	specSum *= gSpecToDiffuseRatio;
	result += specSum * specular;

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);

   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy)); 
}

//-----------------------------------------------------------------------------
float4 mypsANSE(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular,float4 self)
{

	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;

	computeLighting(IN,normal,dirShadowingEnabled, localLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
	   
    diffuseSum += ambientSum*IN.ao_alpha.x;
    
	float3 result = diffuseSum * albedo;

	result += self;
	
	specSum *= gSpecToDiffuseRatio;	
	result += specSum * specular;
	
	result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);
 
	return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy));  
}
//-----------------------------------------------------------------------------
float4 mypsANSR(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular,float3 envMask)
{
   float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
   computeLighting(IN,normal,dirShadowingEnabled, localLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
   diffuseSum += ambientSum*IN.ao_alpha.x;
      					
   float3 result = (diffuseSum * albedo) + specSum * specular * gSpecToDiffuseRatio;

   float envMaskGrayscale = dot(envMask, float3(.212671, .715160, .072169));	   
   
   [branch]
   if (envMaskGrayscale > 1.0/255.0)
   {
	  float dirShadowFactor = lerp(gDirShadowDarkness, 1.0, saturate(dot(IN.normal, gDirLightVecToLightWorld) * 16.0));
      float3 refrVector = float3(0,0,0);	
      float reflFactor = (calcFresnelReflectCoeff(IN,worldNormal,refrVector));
      float3 envReflected = getEnvMapColor(worldReflect);
       //CLM we only need refraction on ice maps...
     // float3 envRefracted = getEnvMapColor(refrVector);
      result += /*lerp(envRefracted,envReflected,reflFactor)*/ envReflected * envMask * lerp(.1, 1.0, dirShadowFactor);
   }

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);
   
   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy));
}

//-----------------------------------------------------------------------------
float4 mypsFull(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular,float4 self,float3 envMask)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	computeLighting(IN,normal, dirShadowingEnabled, localLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
	diffuseSum += ambientSum*IN.ao_alpha.x;
			
	float3 result = (diffuseSum * albedo) + (specSum * specular * gSpecToDiffuseRatio) + self;
				
	//reflection/refraction & fresnel
	float envMaskGrayscale = dot(envMask, float3(.212671, .715160, .072169));	   
	
	[branch]
	if (envMaskGrayscale > 1.0/255.0)
	{
	   float dirShadowFactor = lerp(gDirShadowDarkness, 1.0, saturate(dot(IN.normal, gDirLightVecToLightWorld) * 16.0));
		
	   float3 refrVector = float3(0,0,0);	
	   float reflFactor = (calcFresnelReflectCoeff(IN,worldNormal,refrVector));
	   float3 envReflected = getEnvMapColor(worldReflect);
	    //CLM we only need refraction on ice maps...
     // float3 envRefracted = getEnvMapColor(refrVector);
      result += /*lerp(envRefracted,envReflected,reflFactor)*/ envReflected * envMask * lerp(.1, 1.0, dirShadowFactor);
	}
	
	result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);
				   
	return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy));
}


//-----------------------------------------------------------------------------
float4 mypsA_NLL(PS_INPUT IN,   uniform bool dirShadowingEnabled, float4 albedo)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	float4 normal = float4(0,0,1,1);

	computeLighting_NLL(IN,normal,dirShadowingEnabled,false,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
	float3 result = (diffuseSum+ambientSum*IN.ao_alpha.x) * albedo;

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);

   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy)); 
}
//-----------------------------------------------------------------------------
float4 mypsAN_NLL(PS_INPUT IN,  uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;

	computeLighting_NLL(IN,normal,dirShadowingEnabled,false, worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
    diffuseSum += ambientSum*IN.ao_alpha.x;
       
	float3 result = diffuseSum * albedo;

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);

   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy)); 
}
//-----------------------------------------------------------------------------
float4 mypsANS_NLL(PS_INPUT IN,  uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;

	computeLighting_NLL(IN,normal,dirShadowingEnabled,true,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
    diffuseSum += ambientSum*IN.ao_alpha.x;
       
	float3 result = diffuseSum * albedo + (specSum*gSpecToDiffuseRatio) * specular;
	
	 result = computeBlackmap(result, IN.worldPos);
    result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);

   return  float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy)); 
}

//-----------------------------------------------------------------------------
float4 mypsANSE_NLL(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular,float4 self)
{

	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;

	computeLighting_NLL(IN,normal,dirShadowingEnabled,true,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
	   
    diffuseSum += ambientSum*IN.ao_alpha.x;
    
	float3 result = (diffuseSum * albedo) + self + ((specSum*gSpecToDiffuseRatio) * specular);

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);
 
	return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy));  
}
//-----------------------------------------------------------------------------
float4 mypsANSR_NLL(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular,float3 envMask)
{
   float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
   computeLighting_NLL(IN,normal,dirShadowingEnabled,true,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
   diffuseSum += ambientSum*IN.ao_alpha.x;
      					
   float3 result = (diffuseSum * albedo) + specSum * specular * gSpecToDiffuseRatio;

   float envMaskGrayscale = dot(envMask, float3(.212671, .715160, .072169));	   
   
   [branch]
   if (envMaskGrayscale > 1.0/255.0)
   {
	  float dirShadowFactor = lerp(gDirShadowDarkness, 1.0, saturate(dot(IN.normal, gDirLightVecToLightWorld) * 16.0));
      float3 refrVector = float3(0,0,0);	
      float reflFactor = (calcFresnelReflectCoeff(IN,worldNormal,refrVector));
      float3 envReflected = getEnvMapColor(worldReflect);
      //CLM we only need refraction on ice maps...
     // float3 envRefracted = getEnvMapColor(refrVector);
      result += /*lerp(envRefracted,envReflected,reflFactor)*/ envReflected * envMask * lerp(.1, 1.0, dirShadowFactor);
   }

   result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);
   
   return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy));
}

//-----------------------------------------------------------------------------
float4 mypsFull_NLL(PS_INPUT IN,   uniform bool dirShadowingEnabled, uniform bool localLightingEnabled, float4 albedo, float3 normal, float3 specular,float4 self,float3 envMask)
{
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	computeLighting_NLL(IN,normal, dirShadowingEnabled,true,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
   
	diffuseSum += ambientSum*IN.ao_alpha.x;
			
	float3 result = (diffuseSum * albedo) + (specSum * specular * gSpecToDiffuseRatio) + self;
				
	//reflection/refraction & fresnel
	float envMaskGrayscale = dot(envMask, float3(.212671, .715160, .072169));	   
	
	[branch]
	if (envMaskGrayscale > 1.0/255.0)
	{
	   float dirShadowFactor = lerp(gDirShadowDarkness, 1.0, saturate(dot(IN.normal, gDirLightVecToLightWorld) * 16.0));
		
	   float3 refrVector = float3(0,0,0);	
	   float reflFactor = (calcFresnelReflectCoeff(IN,worldNormal,refrVector));
	   float3 envReflected = getEnvMapColor(worldReflect);
	  //CLM we only need refraction on ice maps...
     // float3 envRefracted = getEnvMapColor(refrVector);
      result += /*lerp(envRefracted,envReflected,reflFactor)*/ envReflected * envMask * lerp(.1, 1.0, dirShadowFactor);
	}
	
	result = computeBlackmap(result, IN.worldPos);
   result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);
				   
	return float4(result, IN.ao_alpha.y * giveDynamicAlphaValue(IN.QNuv0.xy));
}