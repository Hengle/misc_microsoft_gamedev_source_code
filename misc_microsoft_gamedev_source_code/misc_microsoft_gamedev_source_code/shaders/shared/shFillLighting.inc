// File: shFillLighting.inc

float3 computeSHFillLighting(float3 n)
{
   float4 normal = float4(n, 1.0f);
   
   float3 linearConstantColor;
   linearConstantColor.x = dot(normal, gSHFillAr);
   linearConstantColor.y = dot(normal, gSHFillAg);
   linearConstantColor.z = dot(normal, gSHFillAb);
   
   float4 r2 = normal.xyzz * normal.yzzx;
   
   float3 firstQuadraticColor;
   firstQuadraticColor.x = dot(r2, gSHFillBr);
   firstQuadraticColor.y = dot(r2, gSHFillBg);
   firstQuadraticColor.z = dot(r2, gSHFillBb);
   
   normal.x = normal.x * normal.x;
   normal.y = normal.y * normal.y;
   normal.x = normal.x - normal.y;
   
   float3 finalQuadraticColor = gSHFillC * normal.x;
   
   return linearConstantColor + firstQuadraticColor + finalQuadraticColor;
}
