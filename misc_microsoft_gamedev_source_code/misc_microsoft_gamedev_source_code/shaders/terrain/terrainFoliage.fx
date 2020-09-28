// File: terrainRoads.fx
#define RENDERING_TERRAIN
#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"
#include "..\shared\pcf.inc"
#include "..\shared\dirLighting.inc"
#include "..\shared\localLighting.inc"
#include "..\shared\fogHelpers.inc"
#include "..\shared\shFillLighting.inc"
#include "..\shared\blackmap.inc"
#include "gpuTerrainBlackmap.inc"

#include "gpuTerrainVS.inc"
#include "gpuTerrainPS.inc"


//-----------------------------------------------------------------------------
#define cNumBladesPerNode 64



float gBacksideShadowScalar = 1.0f;

#define M_PI 3.14159265358979323846
#define fmodp(x,n) ((n)*frac((x)/(n)))

float2 our_rand(float2 ij)
{
  float2 xy0    = ij/M_PI;
  float2 xym    = fmodp(xy0.xy,257.0)+1.0;
  float2 xym2   = frac(xym*xym);
  float2 result = xym2;

  return (result);
}
float2 rotate_2d( float2 input, float theta )
{
	float2 temp;
	float sintheta = sin( theta );
	float costheta = cos( theta );
	temp.x = input.x * costheta + input.y * sintheta;
	temp.y = input.y * costheta - input.x * sintheta;
	return temp;
}

static float SampleSM(float3 shadowMapCoords, float shadowArrayIndex)
{
	float3 texCoords = float3(shadowMapCoords.x, shadowMapCoords.y, shadowArrayIndex);
	float4 depth;
	asm
	{
		tfetch3D depth.x___, texCoords, gDirShadowMapTexture, MinFilter=point, MagFilter=point,UseComputedLOD = false, UseRegisterLOD = false
	};
	float fAvgZ = depth.x;//+0.2;//CLM ADDED 0.2F HERE!!
	if( shadowMapCoords.z <= fAvgZ )
      return 1.0f;
                           
   float variance = g_fEpsilonVSM;
   float mean     = fAvgZ;
   float d        = shadowMapCoords.z - mean;
   float p_max    = variance / ( variance + d*d );

   return lerp(gDirShadowDarkness.x, 1.0, p_max);

}

static float calcDirShadowFactorNoFilter(float2 texCoords, float3 shadowMapCoords0, out float3 color)
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

   color = gColors[x];
         
   return SampleSM(shadowMapCoords, shadowArrayIndex);
}


bool gLocalLightingEnabled			: register(ENABLE_LOCAL_LIGHTS_REG);
bool gLocalShadowingEnabled			= false;// : register(ENABLE_LOCAL_SHADOWING_REG);
int gNumLights						: register(NUM_LOCAL_LIGHTS_REG);

bool gExtendedLocalLightingEnabled	: register(ENABLE_EXTENDED_LOCAL_LIGHTS_REG);
int gNumExtendedLights				: register(NUM_EXTENDED_LOCAL_LIGHTS_REG);
float4 gExtendedLocalLightingParams : register(EXTENDED_LOCAL_LIGHTING_PARAMS_REG);

float gSpecPower = 50.0f;
static float gSpecToDiffuseRatio = 3.14f; // The artists should not be able to control the specular to diffuse ratio (the actual ratio is Pi).





static void localLighting(float3 worldPos, float3 worldNormal, float3 worldReflect, float specPower, out float3 diffuseSum, out float3 specSum)
{
   diffuseSum = 0.0;
   specSum = 0.0;
   
   /*if (gLocalShadowingEnabled)
   {   
      for (int i = 0; i < gNumLights; i++)
         omniIlluminateShadowedFoliage(i, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }
   else*/
   {
      for (int i = 0; i < gNumLights; i++)
         omniIlluminate(i, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }         
}   
static void Lighting(
	float3 worldPos,
	float3 vertNormal,
	float2 uv0,
	float3 shadowMapCoords0,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   out float3 ambientSum,
   out float3 diffuseSum)
{
	
	diffuseSum = 0;

		
	float3 worldNormal = vertNormal;
	float3 worldReflect = reflect(normalize(worldPos - gWorldCameraPos), worldNormal);
	
   float nDotL;
   //computeDirectionalLighting(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecPower, nDotL, diffuseSum, specSum);
   computeDirectionalDiffuseLighting(worldNormal, gDirLightVecToLightWorld, nDotL, diffuseSum);
   
   
    if ((dirShadowingEnabled) && (nDotL > 0.0f))
   {
      float3 shadowLevelColor;
      float shadowFactor = calcDirShadowFactorNoFilter(uv0, shadowMapCoords0, shadowLevelColor);//calcDirShadowFactor(uv0, shadowMapCoords0, shadowLevelColor,true);   

      diffuseSum *= shadowFactor;
   }
   
   ambientSum = computeSHFillLighting(worldNormal);
   
   if (localLightingEnabled)
   {
      float3 localDiffuseSum, localSpecSum;
      localLighting(worldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
  } 
  
  diffuseSum = clamp(diffuseSum,(1-gBacksideShadowScalar),1.0f);
   
   diffuseSum += ambientSum;
}



//--------------------------------------
//Gerstner wave functions
//--------------------------------------
 float2 cWaveVector = float2(1,1);
 float cAmplitude=0.05f;
 float cFrequency=1;
 float cWaveLength=4;
 uniform float gAnimTime=0;

	
float3 wavePos(float2 inXZ)
{
	float SinDataI=dot(cWaveVector,inXZ)-(cFrequency*gAnimTime);
	float SinDataO = sin(SinDataI)*cAmplitude;

	float k=2*M_PI/cWaveLength;
  
	float3 xvec;
	xvec.xz =-float2((cWaveVector.x/k)*SinDataO,
					 (cWaveVector.y/k)*SinDataO );

 
	xvec.y=(cos(SinDataI)*cAmplitude);
  
	return xvec;
}

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------
struct VS_INPUT
{
	int		index		: INDEX;
	float3  pos			: POSITION;
};

struct VS_OUTPUT
{
    float4 hposition			: POSITION;  
    float4 uv0					: TEXCOORD0;// ZW contains Z/Planar fog density
    float4 diffuseSum			: TEXCOORD1;
    float4 worldPos				: TEXCOORD3;
};

sampler gFoliagePositions = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP ;
};
sampler gFoliageNormals = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};
sampler gFoliageUVs = sampler_state
{
    MipFilter = NONE;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};
uniform float gRCPNumBlades=0.25f;
uniform float gNumVertsPerBlade=10;
//----------------------------------------------------------------------------- 
float3 getTerrainDataAtPos(float2 gPos)
{
	float2 uvVal = float2(	gPos.y,gPos.x);
	float3 offset = tex2Dlod( vertSampler_pos,   float4(uvVal.x,uvVal.y,0.f,0.f) );
	offset = ((offset) *g_posCompRange)-g_posCompMin;
		
	return offset; 
}


void unpack_4_16(int inVal, out int a4, out int b16)
{
	a4 = (inVal / 65536);
	b16 = inVal-(int)(a4 * 65536);
}

//-----------------------------------------------------------------------------
VS_OUTPUT myvsChunk( VS_INPUT IN )
{
   VS_OUTPUT OUT;

	float3 pos = IN.pos;
   
	int iIndex = IN.index;
	int bladeID =0;
	unpack_4_16(IN.index,bladeID,iIndex);
	int P = floor((iIndex + 0.1f) % 10.0f);
	int bladeIndex = (int)(iIndex / 10.0f);
	
	float2 pixelCoord = float2(P + (bladeID*gNumVertsPerBlade),1);
	
	float4 pos0;
	float4 norm0;
	float2 uv0;
    asm 
    {
		tfetch2D pos0, pixelCoord, gFoliagePositions, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = false, OffsetX = 0.0, OffsetY = 0.0
		tfetch2D norm0, pixelCoord, gFoliageNormals, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = false, OffsetX = 0.0, OffsetY = 0.0
	};
	pos = pos0;
	uv0.x = pos0.w;
	uv0.y = 1-norm0.w;
	
	
	//RAND
	float2 rnd = our_rand(float2(bladeIndex,bladeIndex));
	
	//POSITION
	//gridspace translation
	float2 trns = float2((int)(bladeIndex / cNumBladesPerNode) + 0.5f,(int)(bladeIndex % cNumBladesPerNode));
	trns += g_terrainVals.zw;
	trns += ((rnd*2)-1) * 0.9;
	
	float2 rotation = rotate_2d(pos.xz,rnd.y * 360);
	
		//NORMALS
	norm0.xz = rotate_2d(norm0.xz,rnd.y * 360);
	norm0.xyz = normalize(norm0.xyz);
	//norm0 = ((norm0 * 2.0f) - 1.0f);
	

	pos.xz = rotation;
	pos.xz += trns * g_terrainVals.y;
	pos.y *= max(0.25f,rnd.x);	//random scaling
	pos +=getTerrainDataAtPos(trns * g_terrainVals.x);	//add terrain offset at the end..


	
	//ANIMATE
	//float waveScalar = saturate(floor(P * 0.5f) * 0.25);
	//pos.xz += wavePos(trns).y * norm0 * waveScalar;
	
	
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);
	OUT.worldPos =  float4(pos.x,pos.y,pos.z,1.f );
	
 
 	
    
    //UVs
    OUT.uv0.xy = uv0.xy;
	OUT.uv0.z = computeRadialFogDensity(pos);
	OUT.uv0.w = computePlanarFogDensity(pos);
   
	
	
	//LIGHTING
	
	//determine what normal to use for lighting..
	float3 worldCameraVec = normalize(gWorldCameraPos - pos);
    if(dot(worldCameraVec, norm0) <0)
		norm0 = -norm0;
   
	float4 shadowMapCoords0 = mul( float4(pos.x,pos.y,pos.z,1.f ), gDirShadowWorldToTex);
	float3 worldNormal,worldReflect,ambientSum,diffuseSum;
	Lighting(pos,norm0.xyz,uv0,shadowMapCoords0,
						true,gLocalLightingEnabled,
						ambientSum,diffuseSum);
   
    OUT.diffuseSum.xyz = diffuseSum;
    
    float vecToCam=length(gWorldCameraPos - pos);
    float alphaDistScalar = 1-(clamp(vecToCam-400,0,500)/100.0f);
    
    
    OUT.diffuseSum.w = alphaDistScalar;
    
	return OUT;
}
	
	
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------


sampler gFoliageAlbedo = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = POINT;//LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};
sampler gFoliageNormal = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = POINT;//LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};
sampler gFoliageSpecular = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};
sampler gFoliageOpacity = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------


//----------------------------------------------------------------
//----------------------------------------------------------------
[maxtempreg(30)]
float4 mypsMain_ANS(VS_OUTPUT IN ) : COLOR
{  
   float4 albedo	= tex2D(gFoliageAlbedo, IN.uv0);
 //  float3 normal	= float3(0,0,1);	//unpackDXNNormalScaled(tex2D(gFoliageNormal,IN.uv0).xyz);
										// float4 specular	= tex2D(gFoliageSpecular, IN.uv0);
   float opacity	= tex2D(gFoliageOpacity, IN.uv0);
  
  
	float3 result = IN.diffuseSum.xyz * albedo;// +IN.specSum;
	result = computeBlackmap(result, IN.worldPos);
	result = computeFog(result, IN.uv0.z, IN.uv0.w);
	return float4(result, opacity * IN.diffuseSum.w); 

}
//----------------------------------------------------------------
//----------------------------------------------------------------
float4 gVisControl0 : register(VIS_CONTROL_0_REG);
float4 gVisControl1 : register(VIS_CONTROL_1_REG);
float4 gVisControl2 : register(VIS_CONTROL_2_REG);
float4 gVisControl3 : register(VIS_CONTROL_3_REG);

float4 psMainVis(VS_OUTPUT IN) : COLOR                
{  
   
    float4 albedo	= tex2D(gFoliageAlbedo, IN.uv0);
   float3 normal	= float3(0,0,1);//unpackDXNNormalScaled(tex2D(gFoliageNormal,IN.uv0).xyz);
   float4 spec	= float4(0,0,0,0);//tex2D(gFoliageSpecular, IN.uv0);
  float opacity	= tex2D(gFoliageOpacity, IN.uv0);
  
	float3 worldNormal=0,worldReflect=0,ambientSum=0,diffuseSum=0, specSum=0;

	diffuseSum = IN.diffuseSum;
	specSum = 0;//IN.specSum;
	float3 self    = float3(0,0,0);//getUniqueSelfColor(IN.uv0,1).xyz;
	float3 envMask = float3(0,0,0);//getUniqueEnvMaskColor(IN.uv0,1);
	
   
   
   float3 ao      = float3(1,1,1);//float3(IN.ao_alpha.x, IN.ao_alpha.x, IN.ao_alpha.x); 
   float3 alpha      = float3(opacity,opacity,opacity);//float3(IN.ao_alpha.y, IN.ao_alpha.y, IN.ao_alpha.y); 
            
   float3 env     = float3(0,0,0);//unpackDXT5H(texCUBEbias(EnvSampler, float4(worldReflect,2)), gEnvMapHDR);
         
   
   
   
   
   

   float4 selectA = gVisControl0;
   float4 selectB = gVisControl1;
   float4 selectC = gVisControl2;
   float4 selectD = gVisControl3;
         
   float3 result = 
      selectA.x * albedo +
      selectA.y * ao +
      selectA.z * 0 +
      selectA.w * 0 + 
      
      selectB.x * self +
      selectB.y * envMask +
      selectB.z * (env * envMask) +
      selectB.w * spec +
      
      selectC.x * alpha +
      selectC.y * ambientSum +
      selectC.z * diffuseSum +
      selectC.w * specSum +
      
      selectD.x * (.5 + .5 * worldNormal) +
      selectD.y * (.5 + .5 * normal) +
      selectD.z * 0 +
      selectD.w * gSpecPower;

   return float4(result, 1);
}  
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void myvsChunkShadow( 
   in VS_INPUT IN, 
   out float4 outPos : POSITION, 
   out float4 outDepth : TEXCOORD0 )
{

	float3 pos = IN.pos;
   
	int iIndex = IN.index;
	int bladeID =0;
	unpack_4_16(IN.index,bladeID,iIndex);
	int P = floor((iIndex + 0.1f) % 10.0f);
	int bladeIndex = (int)(iIndex / 10.0f);
	
	float2 pixelCoord = float2(P + (bladeID*gNumVertsPerBlade),1);
	
	float4 pos0;
	float4 norm0;
	float2 uv0;
    asm 
    {
		tfetch2D pos0, pixelCoord, gFoliagePositions, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = false, OffsetX = 0.0, OffsetY = 0.0
		tfetch2D norm0, pixelCoord, gFoliageNormals, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = false, OffsetX = 0.0, OffsetY = 0.0
	};
	pos = pos0;
	uv0.x = pos0.w;
	uv0.y = 1-norm0.w;
	
	
	//RAND
	float2 rnd = our_rand(float2(bladeIndex,bladeIndex));
	
	//POSITION
	//gridspace translation
	float2 trns = float2((int)(bladeIndex / cNumBladesPerNode) + 0.5f,(int)(bladeIndex % cNumBladesPerNode));
	trns += g_terrainVals.zw;
	trns += ((rnd*2)-1) * 0.9;
	
	float2 rotation = rotate_2d(pos.xz,rnd.y * 360);
	
	//NORMALS
	norm0.xz = rotate_2d(norm0.xz,rnd.y * 360);
	norm0 = normalize(norm0);
	//norm0 = ((norm0 * 2.0f) - 1.0f);
	

	pos.xz = rotation;
	pos.xz += trns * g_terrainVals.y;
	pos.y *= max(0.25f,rnd.x);	//random scaling
	pos +=getTerrainDataAtPos(trns * g_terrainVals.x);	//add terrain offset at the end..
	
	//ANIMATE
//	float waveScalar = saturate(floor(P * 0.5f) * 0.25);
//	pos.xz += wavePos(trns).y * norm0 * waveScalar;
	
	outPos = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);

	outDepth.xy = uv0.xy;
	outDepth.z = outPos.z;
	
	float vecToCam=length(gWorldCameraPos - pos);
    float alphaDistScalar = 1-(clamp(vecToCam-400,0,500)/100.0f);
    outDepth.w = alphaDistScalar;
}

float4 mypsChunkShadow(in float4 inDepth : TEXCOORD0) : COLOR0
{
	float4 v = encodeDepth(inDepth.z);
	v.w	= tex2D(gFoliageOpacity, inDepth.xy);
//	clip(opacity*inDepth.w-0.6);
   return v;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
technique FullyTextured
{
	pass ANS
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANS();
    } 
    pass ANSE
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANS();
    } 
    pass ANSR
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANS();
    } 
    pass ANSFull
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANS();
    } 
    //============VISUALIZATION
    pass VIS
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 psMainVis();
    } 
}

technique ShadowTechnique
{
    pass LOD0
    {
		VertexShader = compile vs_3_0 myvsChunkShadow();
		PixelShader  = compile ps_3_0 mypsChunkShadow();
    } 
}