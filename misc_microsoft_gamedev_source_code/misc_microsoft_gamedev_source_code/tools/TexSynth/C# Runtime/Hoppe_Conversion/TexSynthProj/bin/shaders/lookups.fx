
#include "output.fx"

// Functions for lookups

// lookup index of previous pixel
float2 lookup_S(float2 tc)
{
  float2 l = tex2D(S_Previous,tc);
  return (l);
}

// lookup index of previous pixel
float2 lookup_S(sampler S,float2 tc)
{
  float2 l = tex2D(S,tc);
  return (l);
}

// lookup index of previous pixel and error channel
float3 lookup_S_err(sampler S,float2 tc)
{
  float4 l = tex2D(S_Previous,tc);
  //return (l.xyz);
  return float3(l.xy,0);
}

// lookup index of previous pixel and error channel
float3 lookup_S_err(float2 tc)
{
  return lookup_S_err(S_Previous,tc);
}

