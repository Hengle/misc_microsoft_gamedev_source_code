
#include "pack.fx"

#ifndef __FX_OUTPUT__
#define __FX_OUTPUT__

// Output indices and color

float4 output(float2 ij,float3 clr)
{
  return (float4(ij,clr.xy));//clr.x,pack8(clr.yz)));
}

#endif
