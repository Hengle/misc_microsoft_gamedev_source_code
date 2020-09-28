//==============================================================================
//PIXEL SHADER
//==============================================================================

#include "gpuTerrainshaderRegs.inc"

sampler UniqueAlbedoSampler = sampler_state
{
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP ;
    AddressW = CLAMP ;
    MAXANISOTROPY = 3;
};
sampler UniqueNormalSampler = sampler_state
{
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP ;
    AddressW = CLAMP ;
    MAXANISOTROPY = 3;
};

sampler UniqueSpecularSampler = sampler_state
{
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP ;
    AddressW = CLAMP ;
    MAXANISOTROPY = 3;
};
sampler UniqueEnvMaskSampler = sampler_state
{
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP ;
    AddressW = CLAMP ;
    MAXANISOTROPY = 3;
};
sampler UniqueSelfSampler = sampler_state
{
    MipFilter = LINEAR;
    MinFilter = ANISOTROPIC;
    MagFilter = ANISOTROPIC;
    AddressU = CLAMP;
    AddressV = CLAMP ;
    AddressW = CLAMP ;
    MAXANISOTROPY = 3;
};
sampler EnvSampler	= sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;

};

bool gPackedSelfTexture =false;
float gSelfMapFadeoutScalar = 1.0f;
float gBumpPower = 1.0f;
float gEnvMapHDR = 10.0f;

static float3 unpackDXNNormalScaled(float3 v)
{
   v.xy = (v * 2.0) - 1.0;
   v.z = sqrt(1.0f - v.x * v.x - v.y * v.y);
   
   v.xy *= gBumpPower;
   v = normalize(v);
   
   return v;
}

//-------------------------------------------
float4 getUniqueAlbedoColor(float2 uv0)
{
   return tex2D(UniqueAlbedoSampler,uv0);
}
//-------------------------------------------
float4 getUniqueNormalColor(float2 uv0, float fadeLerp)
{
   float3 nrm=unpackDXNNormalScaled(tex2D(UniqueNormalSampler,uv0).xyz);
   return lerp(float4(0,0,1,1),float4(nrm.x,nrm.y,nrm.z,1), fadeLerp);
}
//-------------------------------------------
float4 getUniqueSpecularColor(float2 uv0, float fadeLerp)
{
   return lerp(float4(0,0,0,0),tex2D(UniqueSpecularSampler,uv0), fadeLerp);
}
//-------------------------------------------
float3 getUniqueEnvMaskColor(float2 uv0, float fadeLerp)
{
   return lerp(float4(0,0,0,0),tex2D(UniqueEnvMaskSampler,uv0), fadeLerp);
}
//-------------------------------------------
float4 getUniqueSelfColor(float2 uv0, float fadeLerp)
{
	float4 target = tex2D(UniqueSelfSampler,uv0);
	if(gPackedSelfTexture)
		target= float4(unpackDXT5H(target, 16),1)* fadeLerp;
		
	return target ;
}
//-------------------------------------------
float3 getEnvMapColor(float3 uvVec)
{
	return unpackDXT5H(texCUBEbias(EnvSampler, float4(uvVec,2)), gEnvMapHDR);
}
//-------------------------------------------