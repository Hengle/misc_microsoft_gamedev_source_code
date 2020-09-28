// File: pcf.inc

float PCF(float3 texcoord, float ndcZ, sampler shadowMapArraySampler)
{
   float4 weights;
   float4 depth;

   asm
   {
      getWeights3D weights, texcoord, shadowMapArraySampler, MagFilter = linear, MinFilter = linear, MipFilter = linear, VolMagFilter = point, VolMinFilter = point
      tfetch3D depth.x___, texcoord, shadowMapArraySampler, OffsetX = -0.5, OffsetY = -0.5
      tfetch3D depth._x__, texcoord, shadowMapArraySampler, OffsetX =  0.5, OffsetY = -0.5
      tfetch3D depth.__x_, texcoord, shadowMapArraySampler, OffsetX = -0.5, OffsetY =  0.5
      tfetch3D depth.___x, texcoord, shadowMapArraySampler, OffsetX =  0.5, OffsetY =  0.5
   };

   float4 lerpDot = float4((1-weights.y)*(1-weights.x), (1-weights.y)*(weights.x), (weights.y)*(1-weights.x), (weights.y)*(weights.x));
   
//   float4 x = ndcZ < depth;

   float4 x = saturate((depth - float4(ndcZ, ndcZ, ndcZ, ndcZ)) * 20000.0f);

   return dot(x, lerpDot);
}


