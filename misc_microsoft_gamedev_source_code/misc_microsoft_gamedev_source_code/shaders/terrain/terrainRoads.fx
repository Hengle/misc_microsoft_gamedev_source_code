// File: terrainRoads.fx
#define RENDERING_TERRAIN
#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"
#include "..\shared\pcf.inc"
#include "..\shared\dirLighting.inc"
#include "..\shared\localLighting.inc"
#include "..\shared\fogHelpers.inc"
#include "..\shared\shFillLighting.inc"

#include "gpuTerrainVS.inc"
#include "gpuTerrainPS.inc"


//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------
struct VS_INPUT
{
   float4   pos		: POSITION;
   float2   uv0		: TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 hposition          : POSITION;
    float3 tangent            : TANGENT;
    float3 binormal           : BINORMAL;
    float3 normal             : NORMAL0;    
    float2 uv0	               : TEXCOORD0;
    
     float3 shadowMapCoords0 : TEXCOORD2;
     
    float3 worldPos         : TEXCOORD5;
    
    // ZW contains Z/Planar fog density
    float4 ao_alpha           : TEXCOORD6;
};


//----------------------------------------------------------------------------- 
void getTerrainDataAtPos(float3 gPos, out float3 pos, out float3x3 TBN)
{
	float2 uvVal = float2(	(gPos.z /g_terrainVals.y) * g_terrainVals.x,
							(gPos.x /g_terrainVals.y) * g_terrainVals.x);
	float3 offset = tex2Dlod( vertSampler_pos,   float4(uvVal.x,uvVal.y,0.f,0.f) );
	offset = ((offset) *g_posCompRange)-g_posCompMin;
		
	pos = (float3( gPos.x , offset.y+0.01f, gPos.z )) ;
  
	float4 norm = tex2Dlod( vertSampler_basis,   float4(uvVal.x,uvVal.y,0.f,0.f) );
	GiveTBNFromNormal(norm,TBN); 
}
//-----------------------------------------------------------------------------
VS_OUTPUT myvsChunk( VS_INPUT IN )
{
   VS_OUTPUT OUT;

	OUT.uv0 = IN.uv0;
   float3x3 TBN;
   
   float3 pos;
   getTerrainDataAtPos(IN.pos,pos, TBN);
   
  /* 
   float3 pos = VIndexToPosPATCH(IN.index,IN.vQuadID,IN.vUV,
								OUT.uv0, 
								OUT.QNuv0, 
								TBN, 
								OUT.ao_alpha.x,
								OUT.ao_alpha.y,
								OUT.addLocalLight);
         */
   OUT.ao_alpha.x =1;
   OUT.ao_alpha.y =0;
   OUT.ao_alpha.z = computeRadialFogDensity(pos);
   OUT.ao_alpha.w = computePlanarFogDensity(pos);

	OUT.tangent = TBN[0];
    OUT.binormal = TBN[1];
    OUT.normal = TBN[2];
   
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);
	OUT.worldPos =  pos;
	
	OUT.shadowMapCoords0 = mul( float4(pos.x,pos.y,pos.z,1.f ), gDirShadowWorldToTex);
	return OUT;
}
	
	
sampler gRoadAlbedo = sampler_state
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
sampler gRoadNormal = sampler_state
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
sampler gRoadSpecular = sampler_state
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

float gSpecPower = 25.0f;
static float gSpecToDiffuseRatio = 3.14f; // The artists should not be able to control the specular to diffuse ratio (the actual ratio is Pi).

bool gLocalLightingEnabled		: register(ENABLE_LOCAL_LIGHTS_REG);
bool gLocalShadowingEnabled   : register(ENABLE_LOCAL_SHADOWING_REG);
int gNumLights					   : register(NUM_LOCAL_LIGHTS_REG);

bool gExtendedLocalLightingEnabled : register(ENABLE_EXTENDED_LOCAL_LIGHTS_REG);
int gNumExtendedLights        : register(NUM_EXTENDED_LOCAL_LIGHTS_REG);
float4 gExtendedLocalLightingParams : register(EXTENDED_LOCAL_LIGHTING_PARAMS_REG);

//-----------------------------------------------------------------------------

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
static void Lighting(
	float3 worldPos,
	float3 vertNormal,
	float3 vertTang,
	float3 vertBiNorm,
	float3 texnormal,
	float2 uv0,
	float3 shadowMapCoords0,
   uniform bool dirShadowingEnabled,
   uniform bool localLightingEnabled,
   out float3 worldNormal,
   out float3 worldReflect,
   out float3 ambientSum,
   out float3 diffuseSum, 
   out float3 specSum)
{
	
	diffuseSum = 0;
	specSum = 0;
		
	worldNormal = normalize(texnormal.x * vertTang + texnormal.y * vertBiNorm + texnormal.z * vertNormal);
	   
	worldReflect = reflect(normalize(worldPos - gWorldCameraPos), worldNormal);
	
   float nDotL;
   computeDirectionalLighting(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecPower, nDotL, diffuseSum, specSum);
   
    if ((dirShadowingEnabled) && (nDotL > 0.0f))
   {
      float3 shadowLevelColor;
      float shadowFactor = calcDirShadowFactor(uv0, shadowMapCoords0, shadowLevelColor);   

#ifdef DEBUG_SHADOWBUFFER
      diffuseSum += shadowLevelColor*1.0f;
#endif

      diffuseSum *= shadowFactor;
      specSum *= shadowFactor;
   }
   
   ambientSum = computeSHFillLighting(worldNormal);
   
   if (localLightingEnabled)
   {
      float3 localDiffuseSum, localSpecSum;
      localLighting(worldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   } 
   
}

//-----------------------------------------------------------------------------
[maxtempreg(30)]
float4 mypsMain_ANS(VS_OUTPUT IN ) : COLOR
{  
   float4 albedo	= tex2D(gRoadAlbedo, IN.uv0);
   float3 normal	= unpackDXNNormalScaled(tex2D(gRoadNormal,IN.uv0).xyz);
   float4 specular	= tex2D(gRoadSpecular, IN.uv0);
  
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	Lighting(IN.worldPos,IN.normal,IN.tangent,IN.binormal,normal,
						IN.uv0,IN.shadowMapCoords0,
						true,gLocalLightingEnabled,
						worldNormal,worldReflect,ambientSum,diffuseSum,specSum);
   
    diffuseSum += ambientSum;
       
	float3 result = diffuseSum * albedo;
	specSum *= gSpecToDiffuseRatio;
	result += specSum * specular;
    result = computeFog(result, IN.ao_alpha.z, IN.ao_alpha.w);


	//return float4(IN.normal.x,IN.normal.y,IN.normal.z,1);
   return float4(result, 1); 
   
  // return albedo;//mypsA(IN,true,gLocalLightingEnabled,albedo);
}


//-----------------------------------------------------------------------------

float4 gVisControl0 : register(VIS_CONTROL_0_REG);
float4 gVisControl1 : register(VIS_CONTROL_1_REG);
float4 gVisControl2 : register(VIS_CONTROL_2_REG);
float4 gVisControl3 : register(VIS_CONTROL_3_REG);

float4 psMainVis(VS_OUTPUT IN) : COLOR                
{  
   
    float4 albedo	= tex2D(gRoadAlbedo, IN.uv0);
   float3 normal	= unpackDXNNormalScaled(tex2D(gRoadNormal,IN.uv0).xyz);
   float4 spec	= tex2D(gRoadSpecular, IN.uv0);
  
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	Lighting(IN.worldPos,IN.normal,IN.tangent,IN.binormal,normal,
						IN.uv0,IN.shadowMapCoords0,
						true,gLocalLightingEnabled,
						worldNormal,worldReflect,ambientSum,diffuseSum,specSum);
   	
	
	float3 self    = float3(0,0,0);//getUniqueSelfColor(IN.uv0,1).xyz;
	float3 envMask = float3(0,0,0);//getUniqueEnvMaskColor(IN.uv0,1);
	
   
   
   float3 ao      = float3(1,1,1);//float3(IN.ao_alpha.x, IN.ao_alpha.x, IN.ao_alpha.x); 
   float3 alpha      = float3(1,1,1);//float3(IN.ao_alpha.y, IN.ao_alpha.y, IN.ao_alpha.y); 
            
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