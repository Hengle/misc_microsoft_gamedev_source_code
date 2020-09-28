#define RENDERING_TERRAIN
//#define DEBUG_SHADOWBUFFER

#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"
#include "..\shared\pcf.inc"
#include "..\shared\dirLighting.inc"
#include "..\shared\localLighting.inc"
#include "..\shared\fogHelpers.inc"
#include "..\shared\shFillLighting.inc"
#include "..\shared\blackmap.inc"

#include "terrainDynamicAlpha.inc"
#include "gpuTerrainVS.inc"
#include "gpuTerrainPS.inc"
#include "gpuTerrainBlackmap.inc"

//-----------------------------------------------------------------------------
// Vertex Definitions
//-----------------------------------------------------------------------------
struct VS_INPUT
{
    int index		  : INDEX;
    float2 vUV        : BARYCENTRIC;
    int    vQuadID    : QUADID;
};

struct VS_OUTPUT
{
    float4 hposition          : POSITION;
    float3 tangent            : TANGENT;
    float3 binormal           : BINORMAL;
    float3 normal             : NORMAL0;    
    float2 uv0	               : TEXCOORD0;
    float2 QNuv0	            : TEXCOORD1;
    
    
    float3 shadowMapCoords0   : TEXCOORD2;
                
    float3 worldPos           : TEXCOORD5;
    
    // ZW contains Z/Planar fog density
    float4 ao_alpha           : TEXCOORD6;
    float4 addLocalLight	  : TEXCOORD7;
};

float gFadeoutMinDistance = 100;
float gFadeoutMaxDistance = 250;
float gFadeoutDistBias = -10;
float calcFadeoutDist(float3 worldVertPos)
{
	float dst = length(worldVertPos-gWorldCameraPos) - gFadeoutMinDistance;

	return 1-saturate(dst / (gFadeoutMaxDistance-gFadeoutMinDistance+gFadeoutDistBias));
}

static void computeShadowMapCoords(in float3 pos, inout VS_OUTPUT OUT)
{
   OUT.shadowMapCoords0 = mul( float4(pos.x,pos.y,pos.z,1.f ), gDirShadowWorldToTex);
}   
   
//-----------------------------------------------------------------------------
VS_OUTPUT myvsChunk( VS_INPUT IN )
{
   VS_OUTPUT OUT;

   float3x3 TBN;
   
   
   
   float3 pos = VIndexToPosPATCH(IN.index,IN.vQuadID,IN.vUV,
								OUT.uv0, 
								OUT.QNuv0, 
								TBN, 
								OUT.ao_alpha.x,
								OUT.ao_alpha.y,
								OUT.addLocalLight);
         
   OUT.ao_alpha.z = computeRadialFogDensity(pos);
   OUT.ao_alpha.w = computePlanarFogDensity(pos);

	OUT.tangent = TBN[0];
    OUT.binormal = TBN[1];
    OUT.normal = TBN[2];
   
	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);
	OUT.worldPos = pos;
	
	OUT.addLocalLight.w = calcFadeoutDist(pos);
	
	computeShadowMapCoords(pos, OUT);
	
	return OUT;
}

//-----------------------------------------------------------------------------
void myvsChunkShadow( 
   in VS_INPUT IN, 
   out float4 outPos : POSITION, 
   out float3 outDepth : TEXCOORD0 )
{
   float3 pos = VIndexToPosPATCHShadow(IN.index,IN.vQuadID,IN.vUV,outDepth.xy);

	outPos = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);
	
	outDepth.z = outPos.z;
}

float4 mypsChunkShadow(in float3 inDepth : TEXCOORD0) : COLOR0
{
	float4 v = encodeDepth(inDepth.z); 
	v.w = giveDynamicAlphaValue(inDepth.xy);
   return v;
}

void myvsChunkShadowAlpha( 
   in VS_INPUT IN, 
   out float4 outPos : POSITION, 
   out float4 outDepth : TEXCOORD0 )
{
   float4 pos = VIndexToPosPATCHShadowAlpha(IN.index,IN.vQuadID,IN.vUV,outDepth.xy);
   
	outPos = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);
	
	outDepth.z = outPos.z;
	outDepth.w = pos.w;
}
float4 mypsChunkShadowAlpha(in float4 inDepth : TEXCOORD0) : COLOR0
{
	float4 v = encodeDepth(inDepth.z); 
	v.w = inDepth.w;// * giveDynamicAlphaValue(inDepth.xy);
   return v;
}

//-----------------------------------------------------------------------------

struct VS_INPUT_SKIRT
{
    int index		  : INDEX;
    float2 vUV        : BARYCENTRIC;
    int    vQuadID    : QUADID;
};

struct VS_OUTPUT_SKIRT
{
    float4 hposition          : POSITION;  
    float4 QNuv0	            : TEXCOORD1;// ZW contains Z/Planar fog density
    float4 diffuseSum           : TEXCOORD2;
};


VS_OUTPUT_SKIRT myvsChunkSkirt( VS_INPUT_SKIRT IN )
{
   VS_OUTPUT_SKIRT OUT;

   
   float3 normal;
   float alpha;
   float3 pos = VIndexToPosPATCHSkirt(IN.index,IN.vQuadID,IN.vUV,
								OUT.QNuv0.xy, 
								normal,
								alpha);

   // Transform position to world space
   pos = mul(float4(pos.x,pos.y,pos.z,1.f ), g_quadrantMatrix);   
	
   OUT.QNuv0.z = computeRadialFogDensity(pos);
   OUT.QNuv0.w = computePlanarFogDensity(pos);

	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);

	float3 worldNormal = mul( normal,(float3x3)g_quadrantMatrix);
	float nDotL = saturate(dot(worldNormal, gDirLightVecToLightWorld));
	float3 diffuseSum = gDirLightColor * nDotL;
	float3 ambientSum = computeSHFillLighting(worldNormal);
    OUT.diffuseSum.xyz = diffuseSum + ambientSum;
	OUT.diffuseSum.w = alpha;
	
	return OUT;
}

VS_OUTPUT_SKIRT myvsChunkSkirtHiResTex( VS_INPUT_SKIRT IN )
{
   VS_OUTPUT_SKIRT OUT;

   float3 normal;
   float alpha;
   float3 pos = VIndexToPosPATCHSkirtHighResTex(IN.index,IN.vQuadID,IN.vUV,
								OUT.QNuv0.xy, 
								normal,
								alpha);

   // Transform position to world space
   pos = mul(float4(pos.x,pos.y,pos.z,1.f ), g_quadrantMatrix);   
	
   OUT.QNuv0.z = computeRadialFogDensity(pos);
   OUT.QNuv0.w = computePlanarFogDensity(pos);

	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);

	float3 worldNormal = mul( normal,(float3x3)g_quadrantMatrix);
	float nDotL = saturate(dot(worldNormal, gDirLightVecToLightWorld));
	float3 diffuseSum = gDirLightColor * nDotL;
	float3 ambientSum = computeSHFillLighting(worldNormal);
    OUT.diffuseSum.xyz = diffuseSum + ambientSum;
	OUT.diffuseSum.w = alpha;
	return OUT;
}


VS_OUTPUT_SKIRT myvsChunkSkirtLowResTex( VS_INPUT_SKIRT IN )
{
   VS_OUTPUT_SKIRT OUT;

   float3 normal;
   float alpha;
   float3 pos = VIndexToPosPATCHSkirtLowResTex(IN.index,IN.vQuadID,IN.vUV,
								OUT.QNuv0.xy, 
								normal,
								alpha);

   // Transform position to world space
   pos = mul(float4(pos.x,pos.y,pos.z,1.f ), g_quadrantMatrix);   
	
   OUT.QNuv0.z = computeRadialFogDensity(pos);
   OUT.QNuv0.w = computePlanarFogDensity(pos);

	OUT.hposition = mul( float4(pos.x,pos.y,pos.z,1.f ),gWorldToProj);

	float3 worldNormal = mul( normal,(float3x3)g_quadrantMatrix);
	float nDotL = saturate(dot(worldNormal, gDirLightVecToLightWorld));
	float3 diffuseSum = gDirLightColor * nDotL;
	float3 ambientSum = computeSHFillLighting(worldNormal);
    OUT.diffuseSum.xyz = diffuseSum + ambientSum;
    OUT.diffuseSum.w = alpha;

	return OUT;
}

[maxtempreg(30)]
float4 mypsMain_Skirt(VS_OUTPUT_SKIRT IN ) : COLOR
{  
//return float4(IN.QNuv0.yx, 0,1);
   float4 albedo	= getUniqueAlbedoColor(IN.QNuv0.yx);
  
   
   float3 result = IN.diffuseSum.xyz * albedo;

	result = computeFog(result, IN.QNuv0.z, IN.QNuv0.w);
	return float4(result, IN.diffuseSum.w); 
}

[maxtempreg(30)]
float4 mypsMain_SkirtFullFog(VS_OUTPUT_SKIRT IN ) : COLOR
{  
	float3 result = gBlackmapParams0.xyz;
	result = computeFog(result, IN.QNuv0.z, IN.QNuv0.w);
	return float4(result, IN.diffuseSum.w); 
}
//-----------------------------------------------------------------------------
struct PS_INPUT
{
    float3 normal           : NORMAL0;
    float3 tangent          : TANGENT;
    float3 binormal         : BINORMAL;
    float2 uv0	             : TEXCOORD0_CENTROID;
    float2 QNuv0            : TEXCOORD1_CENTROID;
    
    float3 shadowMapCoords0 : TEXCOORD2;
    
    float3 worldPos         : TEXCOORD5;
    
    // ZW contains planar fog density
    float4 ao_alpha         : TEXCOORD6;    
    float4 addLocalLight    : TEXCOORD7;	//w holds fadeout distance   
};

#include "gpuTerrainTexturingLightingPS.inc"

//-----------------------------------------------------------------------------


sampler AlbedoTexAtlas	= sampler_state
{
	MipFilter = NONE;
    
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = CLAMP;
    
    SEPARATEZFILTERENABLE = TRUE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;

};//			: register(ENV_SAMPLER_REG);

//-----------------------------------------------------------------------------
[maxtempreg(31)]
float4 mypsMain_AN(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   return mypsAN(IN,true,gLocalLightingEnabled,albedo,normal);
}
//-----------------------------------------------------------------------------
[maxtempreg(31)]
float4 mypsMain_ANS(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   return mypsANS(IN,true,gLocalLightingEnabled,albedo,normal,specular);
}
//-----------------------------------------------------------------------------
[maxtempreg(31)]
float4 mypsMain_ANSR(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   float3 envMask	= getUniqueEnvMaskColor(IN.uv0,IN.addLocalLight.w);
   return mypsANSR(IN,true,gLocalLightingEnabled,albedo,normal,specular,envMask);
}
//-----------------------------------------------------------------------------
[maxtempreg(31)]
float4 mypsMain_ANSE(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   float4 self		= getUniqueSelfColor(IN.uv0,IN.addLocalLight.w);
   return mypsANSE(IN,true,gLocalLightingEnabled,albedo,normal,specular,self);
   
}
//-----------------------------------------------------------------------------
[maxtempreg(31)]
float4 mypsMain_FULL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   float4 self		= getUniqueSelfColor(IN.uv0,IN.addLocalLight.w);
   float3 envMask	= getUniqueEnvMaskColor(IN.uv0,IN.addLocalLight.w);
   return mypsFull(IN,true,gLocalLightingEnabled,albedo,normal,specular,self,envMask);
   
}
//-----------------------------------------------------------------------------
[maxtempreg(31)]
float4 mypsMain_SmallChunk(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.QNuv0.yx);
  
   return mypsA(IN,true,gLocalLightingEnabled, albedo);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
[maxtempreg(17)]
float4 mypsMain_AN_NLL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   return mypsAN_NLL(IN,true,gLocalLightingEnabled,albedo,normal);
}
//-----------------------------------------------------------------------------
[maxtempreg(19)]
float4 mypsMain_ANS_NLL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   return mypsANS_NLL(IN,true,gLocalLightingEnabled,albedo,normal,specular);
}
//-----------------------------------------------------------------------------
[maxtempreg(24)]
float4 mypsMain_ANSR_NLL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   float3 envMask	= getUniqueEnvMaskColor(IN.uv0,IN.addLocalLight.w);
   return mypsANSR_NLL(IN,true,gLocalLightingEnabled,albedo,normal,specular,envMask);
}
//-----------------------------------------------------------------------------
[maxtempreg(21)]
float4 mypsMain_ANSE_NLL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   float4 self		= getUniqueSelfColor(IN.uv0,IN.addLocalLight.w);
   return mypsANSE_NLL(IN,true,gLocalLightingEnabled,albedo,normal,specular,self);
   
}
//-----------------------------------------------------------------------------
[maxtempreg(26)]
float4 mypsMain_FULL_NLL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.uv0);
   float3 normal	= getUniqueNormalColor(IN.uv0,IN.addLocalLight.w);
   float3 specular	= getUniqueSpecularColor(IN.uv0,IN.addLocalLight.w);
   float4 self		= getUniqueSelfColor(IN.uv0,IN.addLocalLight.w);
   float3 envMask	= getUniqueEnvMaskColor(IN.uv0,IN.addLocalLight.w);
   return mypsFull_NLL(IN,true,gLocalLightingEnabled,albedo,normal,specular,self,envMask);
   
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
[maxtempreg(30)]
float4 mypsMain_SmallChunk_NLL(PS_INPUT IN ) : COLOR
{  
   float4 albedo	= getUniqueAlbedoColor(IN.QNuv0.yx);
  
   //return mypsA_NLL(IN,true,albedo);
   return mypsA(IN, true, true, albedo);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

float4 gVisControl0 : register(VIS_CONTROL_0_REG);
float4 gVisControl1 : register(VIS_CONTROL_1_REG);
float4 gVisControl2 : register(VIS_CONTROL_2_REG);
float4 gVisControl3 : register(VIS_CONTROL_3_REG);

float4 psMainVis(PS_INPUT IN) : COLOR                
{  
 
   
   float2 uv0 = IN.uv0;

   float4 normal  = getUniqueNormalColor(IN.uv0,1);
   
	float3 worldNormal,worldReflect,ambientSum,diffuseSum, specSum;
	computeLighting(IN,normal,true,gLocalLightingEnabled,worldNormal,worldReflect,ambientSum,diffuseSum, specSum);
	
	
	float3 self    = getUniqueSelfColor(IN.uv0,1).xyz;
	float3 envMask = getUniqueEnvMaskColor(IN.uv0,1);
	
   float4 albedo  = getUniqueAlbedoColor(IN.uv0);
   
   float3 ao      = float3(IN.ao_alpha.x, IN.ao_alpha.x, IN.ao_alpha.x); 
   float3 alpha      = float3(IN.ao_alpha.y, IN.ao_alpha.y, IN.ao_alpha.y); 
            
   float3 env     = unpackDXT5H(texCUBEbias(EnvSampler, float4(worldReflect,2)), gEnvMapHDR);
         
   float3 spec    = getUniqueSpecularColor(uv0,1);
   
   
   
   

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


technique FullyTextured
{
	pass AN
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_AN();
    } 
    pass ANS
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANS();
    } 
	pass ANSR
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANSR();
    } 
	pass ANSE
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANSE();
    } 
	pass FULL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_FULL();
    } 
 //============SMALLCHUNK
    pass SMALLCHUNK
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_SmallChunk();
    } 

	 //=================NO LOCAL LIGHTING
	 pass AN_NLL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_AN_NLL();
    } 
	 pass ANS_NLL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANS_NLL();
    } 
	pass ANSR_NLL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANSR_NLL();
    } 
	pass ANSE_NLL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_ANSE_NLL();
    } 
	pass FULL_NLL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_FULL_NLL();
    }
    
    //============SMALLCHUNK
    pass SMALLCHUNK_NLL
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 mypsMain_SmallChunk_NLL();
    } 

    
    //============SKIRT CHUNK
    pass SKIRT_BATCHED
    {
		VertexShader = compile vs_3_0 myvsChunkSkirt();
		PixelShader  = compile ps_3_0 mypsMain_Skirt();
    } 
	pass SKIRT_BATCHED_FOGGED
    {
		VertexShader = compile vs_3_0 myvsChunkSkirt();
		PixelShader  = compile ps_3_0 mypsMain_SkirtFullFog();
    } 
    
     pass SKIRT_HI_RES_TEX
    {
		VertexShader = compile vs_3_0 myvsChunkSkirtHiResTex();
		PixelShader  = compile ps_3_0 mypsMain_Skirt();
    } 
    
       pass SKIRT_LOW_RES_TEX
    {
		VertexShader = compile vs_3_0 myvsChunkSkirtLowResTex();
		PixelShader  = compile ps_3_0 mypsMain_Skirt();
    } 
    pass SKIRT_NONBATCHED_FOGGED
    {
		VertexShader = compile vs_3_0 myvsChunkSkirtHiResTex();
		PixelShader  = compile ps_3_0 mypsMain_SkirtFullFog();
    }
    
    
    //============VISUALIZATION
    pass VIS
    {
		VertexShader = compile vs_3_0 myvsChunk();
		PixelShader  = compile ps_3_0 psMainVis();
    } 
    
    


}

//-----------------------------------------------------------------------------

technique ShadowTechnique
{
    pass LOD0
    {
		VertexShader = compile vs_3_0 myvsChunkShadow();
		PixelShader  = compile ps_3_0 mypsChunkShadow();
    } 
    
     pass LOD0_Alpha
    {
		VertexShader = compile vs_3_0 myvsChunkShadowAlpha();
		PixelShader  = compile ps_3_0 mypsChunkShadowAlpha();
    } 
}