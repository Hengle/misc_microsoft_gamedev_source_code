// File: gpuTerrainBlackmap.inc

sampler gBlackmapSampler = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler gBlackmapUnexploredSampler = sampler_state
{
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

bool gBlackmapEnabled = false;
// background, layer1 brightness
float4 gBlackmapParams0; 
// layer2 brightness, 1.0/32.0, 32.0, 31/32.0
float4 gBlackmapParams1;
// x = 1.0/worldSizeX, 1.0/worldSizeZ
float4 gBlackmapParams2;

float3 computeBlackmap(float3 color, float3 worldPos)
{
   if (!gBlackmapEnabled)
      return color;
   
   //float brightness;
   //float backgroundLerp;   
   //computeBlackmapInternal(brightness, backgroundLerp, worldPos, gUnexploredSampler, gBlackmapParams0, gBlackmapParams1, gBlackmapParams2);
   
   //return lerp(gBlackmapParams0, color * brightness, backgroundLerp);
   
   return computeBlackmapColor(color, worldPos, gBlackmapSampler, gBlackmapUnexploredSampler, gBlackmapParams0, gBlackmapParams1, gBlackmapParams2);
}

