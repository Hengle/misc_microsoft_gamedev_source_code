#ifndef __FX__PACK__
#define __FX__PACK__
  
float pack8(float2 ab)
{
  float2 i=(ab*255.0);
  return (dot(i,float2(1.0,1.0/256.0)));
}
  
float2 unpack8(float v)
{
  float f=frac(v);
  return float2((v-f),f*256.0) / 255.0;
}
  
void unpack8_vect(float4 v,out float2 _o0,out float2 _o1,out float2 _o2,out float2 _o3)
{
  float4 f  = frac(v);
  float4 t0 = float4(v.xy-f.xy , f.xy*256.0) / 255.0;
  float4 t1 = float4(v.zw-f.zw , f.zw*256.0) / 255.0;
  _o0       = t0.xz;
  _o1       = t0.yw;
  _o2       = t1.xz;
  _o3       = t1.yw;
}


#endif
