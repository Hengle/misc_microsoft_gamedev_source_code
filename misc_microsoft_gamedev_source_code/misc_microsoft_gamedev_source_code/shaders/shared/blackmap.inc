// File: blackmap.inc

void computeBlackmapScalar(
   out float brightness, 
   out float backgroundLerp,
   in float3 worldPos, 
   sampler blackmapSampler, 
   in float4 blackmapParams0, in float4 blackmapParams1, in float4 blackmapParams2)
{
   float2 visTexUV = float2(worldPos.x * blackmapParams2.x, 1.0 - worldPos.z * blackmapParams2.x);
                              
   float visFactor = tex2D(blackmapSampler, visTexUV).a;
   
   float2 loDist = float2(blackmapParams1.y, blackmapParams1.z) - float2(worldPos.x, worldPos.z);
   float2 hiDist = float2(worldPos.x, worldPos.z) - float2(blackmapParams2.y, blackmapParams2.z);
   float2 dist = float2(1.0f, 1.0f) - saturate(max(loDist, hiDist) * blackmapParams2.w);
   backgroundLerp = dist.x * dist.y;
            
   float layer0 = 1.0;
   float layer1 = blackmapParams0.w;
   float layer2 = blackmapParams1.x;
      
   if (visFactor <= .5)
      brightness = lerp(layer0, layer1, visFactor * 2.0f);
   else
      brightness = lerp(layer1, layer2, (visFactor - .5f) * 2.0f);
}

float3 computeBlackmapColor(
   float3 color,
   in float3 worldPos, 
   sampler blackmapSampler, 
   sampler unexploredSampler,
   in float4 blackmapParams0, in float4 blackmapParams1, in float4 blackmapParams2)
{
   float2 visTexUV = float2(worldPos.x * blackmapParams2.x, 1.0 - worldPos.z * blackmapParams2.x);
   float4 unexploredMask = tex2D(unexploredSampler, float2(worldPos.x, worldPos.z) * blackmapParams1.w);
                                 
   float visFactor = tex2D(blackmapSampler, visTexUV).a;
   
   float2 loDist = float2(blackmapParams1.y, blackmapParams1.z) - float2(worldPos.x, worldPos.z);
   float2 hiDist = float2(worldPos.x, worldPos.z) - float2(blackmapParams2.y, blackmapParams2.z);
   float2 dist = float2(1.0f, 1.0f) - saturate(max(loDist, hiDist) * blackmapParams2.w);
   float backgroundLerp = dist.x * dist.y;
            
   float3 layer0 = color;
   float3 layer1 = color * blackmapParams0.w;
   float3 layer2 = lerp(color, unexploredMask, unexploredMask.a) * blackmapParams1.x;
   //float3 layer2 = color * unexploredMask * blackmapParams1.x;
      
   if (visFactor <= .5)
      color = lerp(layer0, layer1, visFactor * 2.0f);
   else
      color = lerp(layer1, layer2, (visFactor - .5f) * 2.0f);
      
   return lerp(float3(blackmapParams0.x, blackmapParams0.y, blackmapParams0.z), color, backgroundLerp);
}