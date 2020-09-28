// File: helpers.inc

static float4 min4(float4 q, float4 t)
{
   float4 result;
   asm
   {
      min result, q, t
   };
   return result;
}

static float4 max4(float4 q, float4 t)
{
   float4 result;
   asm
   {
      max result, q, t
   };
   return result;
}

static float4 cndeq(float4 q, float4 t, float4 s)
{
   float4 result;
   asm
   {
      cndeq result, q, t, s
   };
   return result;
}

static float4 cndgt(float4 q, float4 t, float4 s)
{
   float4 result;
   asm
   {
      cndgt result, q, t, s
   };
   return result;
}

static float4 cndge(float4 q, float4 t, float4 s)
{
   float4 result;
   asm
   {
      cndge result, q, t, s
   };
   return result;
}

static float4 max4(float4 q)
{
   float4 result;
   asm
   {
      max4 result, q
   };
   return result;
}

static float4 sge(float4 q, float4 t)
{
   float4 result;
   asm
   {
      sge result, q, t
   };
   return result;
}

static float4 sgt(float4 q, float4 t)
{
   float4 result;
   asm
   {
      sgt result, q, t
   };
   return result;
}

static float4 sne(float4 q, float4 t)
{
   float4 result;
   asm
   {
      sne result, q, t
   };
   return result;
}

static float truncate(float f)
{
   float4 result;
   
   asm
   {
      truncs result, f
   };
   
   return result.w;
}

static float4 truncate(float4 f)
{
   float4 result;
   
   asm
   {
      truncs result, f
   };
   
   return result;
}

static float dp2add(float2 dot1, float2 dot2, float addend)
{
   float4 result;
   
   asm
   {
      dp2add result, dot1, dot2, addend
   };
   
   return result.x;
}

static float recipClamp(float x)
{
   float4 result;

   asm
   {
      rcpc result, x
   };
   
   return result.x;
}

static float recipFixedFunc(float x)
{
   float4 result;
   
   asm
   {
      rcpf result, x
   };
   
   return result.x;
}

static float recipSqrtClamp(float x)
{
   float4 result;
   
   asm
   {
      rsqc result, x
   };
   
   return result.x;
}

static float recipSqrtFixedFunc(float x)
{
   float4 result;
   
   asm
   {
      rsqf result, x
   };
   
   return result.x;
}

static float logClamp(float x)
{
   float4 result;
   
   asm
   {
      logc result, x
   };
   
   return result.x;
}

static float4 mulVec4x3(float4 v, float4x3 m)
{
   return float4(
      dot(v, m._m00_m10_m20_m30),
      dot(v, m._m01_m11_m21_m31),
      dot(v, m._m02_m12_m22_m32),
      1);
}

static float3 mulVec4x3NoTran(float3 v, float4x3 m)
{
   return float3(
      dot(v, m._m00_m10_m20),
      dot(v, m._m01_m11_m21),
      dot(v, m._m02_m12_m22));
}

static float4 mulVec(float4 v, float3 rot0, float3 rot1, float3 rot2, float3 tran)
{
   float3 temp = float3(v.x * rot0 + v.y * rot1 + v.z * rot2 + tran);
   
   return float4(temp, 1);
}

static float3 mulVecNoTran(float3 v, float3 rot0, float3 rot1, float3 rot2)
{
   return float3(v.x * rot0 + v.y * rot1 + v.z * rot2);
}

static float4 deswizzleNormal(float4 swizzled)
{
   return swizzled.wyzx;
}

static float3 unpackDXNNormal(float3 v)
{
   v.xy = (v * 2.0) - 1.0;
   v.z = sqrt(1.0f - v.x * v.x - v.y * v.y);
   return v;
}

static float4 unpackDXNNormal(float4 v)
{
   v.xy = (v * 2.0) - 1.0;
   v.z = sqrt(saturate(1.0f - v.x * v.x - v.y * v.y));
   return v;
}

static float PCF(float4 texcoord, float ndcZ, sampler shadowMapArraySampler)
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

   return dot(ndcZ < depth, lerpDot);
}

static float3 unpackDXT5H(float4 packed, float hdrScale)
{
   packed.w = packed.w + 1.0f/255.0f;
   
   packed *= packed;
         
   float3 unpacked = saturate(float3(1.0, 1.0, 1.0) - packed) * packed.w * hdrScale;
   
   return unpacked;
}


static float3 tryNormalize(float3 v)
{
   return v * recipSqrtFixedFunc(dot(v, v));
}

static void swap(inout float a, inout float b)
{
   float temp = a;
   a = b;
   b = temp;
}

static void swap3(inout float3 a, inout float3 b)
{
   float3 temp = a;
   a = b;
   b = temp;
}

static void swap4(inout float4 a, inout float4 b)
{
   float4 temp = a;
   a = b;
   b = temp;
}

static float srgbToLinear(float color)
{
   color = saturate(color) * saturate(color);
   return color;
}

static float3 srgbToLinear(float3 color)
{
   color.rgb = saturate(color) * saturate(color);
   return color;
}

static float4 srgbToLinear(float4 color)
{
   color.rgb = saturate(color) * saturate(color);
   return color;
}

static float linstep(float l, float h, float s)
{
   return saturate((s - l) / (h - l));
}

