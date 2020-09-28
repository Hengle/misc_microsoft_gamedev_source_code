// File: dirShadowing.fx
#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

// 32-bit float to fraction alias trick:
// z += float2(1.0f, 1.0f);
// z *= exp2(-126.0f);

sampler gPointSampler0 : register(s0) = sampler_state
{
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gPointSampler1 : register(s1) = sampler_state
{
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gPointSampler2 : register(s2) = sampler_state
{
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gPointSampler3 : register(s3) = sampler_state
{
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gPointSampler4 : register(s4) = sampler_state
{
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLinearSampler0 : register(s0) = sampler_state
{
    MipFilter = POINT;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLinearSampler1 : register(s1) = sampler_state
{
    MipFilter = POINT;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLinearSampler2 : register(s2) = sampler_state
{
    MipFilter = POINT;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLinearSampler3 : register(s3) = sampler_state
{
    MipFilter = POINT;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

sampler gLinearSampler4 : register(s4) = sampler_state
{
    MipFilter = POINT;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    MaxAnisotropy = 1;
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

// Fullscreen Quad

void vsFullScreenQuad(
    in float2 InPosition : POSITION,
    in float2 InTex0 : TEXCOORD0,
    in float2 InTex1 : TEXCOORD1,
    out float4 OutPosition : POSITION,
    out float2 OutTex0 : TEXCOORD0,
    out float2 OutTex1 : TEXCOORD1)
{
   OutPosition = float4(InPosition, 0, 1);
   OutTex0 = InTex0;
   OutTex1 = InTex1;
}

#if 0
static float encode(float v)
{
   v = saturate(v);
   
   if (v < .5)
   {
      return (v * 2.0) * 31.9990234375;
   }
   else
   {
      // 0x8000   0xFFFF    0x0000  0x7FFF
      //    -32          0.0        32.0
   
      return (2.0 * (v - .5)) * 31.9990234375 + -32.0;
   }
}
#endif

float spow(float a, float b)
{
   //if (a < 0.0)
   //   return -pow(-a, b);
   //else
   //   return pow(a, b);  
   
   return sign(a) * pow(abs(a), b);
}

static float4 encodeXY(float4 v, bool encodeVariance)
{
   if (encodeVariance)
   {
      v.y = (v.y - (v.x * v.x));
      v.y = spow(v.y, 1.0/2.0) * .5 + .5;
   }
      
   // a8 r8 g8 b8
   // rl rh gl gh
   
   v = saturate(v);
   
   float x = floor(v.x * 65535.0 + .5f);
   float y = floor(v.y * 65535.0 + .5f);

   v.y = floor(x * 1.0/256.0);
   v.x = frac(x * 1.0/256.0)*256.0;

   v.w = floor(y * 1.0/256.0);
   v.z = frac(y * 1.0/256.0)*256.0;
      
   [isolate]
   {
      v *= 1.0/255.0f;
   }
           
   return v;
}

// Vertical filter: Resample from (width, height) to (width, height/2)
float4 psVSMVFilter5(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
 // Fetch a row of 5 pixels from the D24S8 depth map
   float4 t0;
   float4 t1;
   float4 t2;
   float4 t3;
   float4 t4;
   
   asm
   {
     tfetch2D t0.x___, tex0, gPointSampler0, OffsetY = -2.0, MinFilter=point, MagFilter=point
     tfetch2D t1.x___, tex0, gPointSampler0, OffsetY = -1.0, MinFilter=point, MagFilter=point
     tfetch2D t2.x___, tex0, gPointSampler0, OffsetY =  0.0, MinFilter=point, MagFilter=point
     tfetch2D t3.x___, tex0, gPointSampler0, OffsetY = +1.0, MinFilter=point, MagFilter=point
     tfetch2D t4.x___, tex0, gPointSampler0, OffsetY = +2.0, MinFilter=point, MagFilter=point
   };
   
   // 1 4 6 4 1

   float z = (t0 + t1 * 4.0f + t2 * 6.0f + t3 * 4.0f + t4) * (1.0f/16.0f);
   
   t0 *= t0;
   t1 *= t1;
   t2 *= t2;
   t3 *= t3;
   t4 *= t4;
   
   float z2 = (t0 + t1 * 4.0f + t2 * 6.0f + t3 * 4.0f + t4) * (1.0f/16.0f);
               
   return float4( z, z2, 0, 0 );
}

// Horizontal filter: Resample from (width, height/2) to (width/2, height/2)
float4 psVSMHFilter5(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
   // Fetch a row of 5 pixels from the G32R32 depth map
   float4 t0;
   float4 t1;
   float4 t2;
   float4 t3;
   float4 t4;
   
   asm
   {
     tfetch2D t0.xy__, tex0, gPointSampler0, OffsetX = -2.0, MinFilter=point, MagFilter=point
     tfetch2D t1.xy__, tex0, gPointSampler0, OffsetX = -1.0, MinFilter=point, MagFilter=point
     tfetch2D t2.xy__, tex0, gPointSampler0, OffsetX =  0.0, MinFilter=point, MagFilter=point
     tfetch2D t3.xy__, tex0, gPointSampler0, OffsetX = +1.0, MinFilter=point, MagFilter=point
     tfetch2D t4.xy__, tex0, gPointSampler0, OffsetX = +2.0, MinFilter=point, MagFilter=point
   };
   
   // 1 4 6 4 1

   float2 z = saturate((t0 + t1 * 4.0f + t2 * 6.0f + t3 * 4.0f + t4) * (1.0f/16.0f));
  
   return encodeXY(float4( z.x, z.y, 0, 0 ), true);
}

// Vertical filter: Resample from (width, height) to (width, height/2)
float4 psVSMVFilter3(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
 // Fetch a row of 3 pixels from the D24S8 depth map
   float4 t0;
   float4 t1;
   float4 t2;
   
   asm
   {
     tfetch2D t0.x___, tex0, gPointSampler0, OffsetY = -1.0, MinFilter=point, MagFilter=point
     tfetch2D t1.x___, tex0, gPointSampler0, OffsetY =  0.0, MinFilter=point, MagFilter=point
     tfetch2D t2.x___, tex0, gPointSampler0, OffsetY = +1.0, MinFilter=point, MagFilter=point
   };
   
   // 1 3 1

   float z = (t0 + t1 * 3.0f + t2) * (1.0f/5.0f);
   
   t0 *= t0;
   t1 *= t1;
   t2 *= t2;
      
   float z2 = (t0 + t1 * 3.0f + t2) * (1.0f/5.0f);
            
   return float4( z, z2, 0, 0 );
}

// Horizontal filter: Resample from (width, height/2) to (width/2, height/2)
float4 psVSMHFilter3(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
   // Fetch a row of 3 pixels from the G32R32 depth map
   float4 t0;
   float4 t1;
   float4 t2;
   
   asm
   {
     tfetch2D t0.xy__, tex0, gPointSampler0, OffsetX = -1.0, MinFilter=point, MagFilter=point
     tfetch2D t1.xy__, tex0, gPointSampler0, OffsetX =  0.0, MinFilter=point, MagFilter=point
     tfetch2D t2.xy__, tex0, gPointSampler0, OffsetX = +1.0, MinFilter=point, MagFilter=point
   };
   
   // 1 3 1

   float2 z = saturate((t0 + t1 * 3.0f + t2) * (1.0f/5.0f));
                           
   return encodeXY(float4( z.x, z.y, 0, 0 ), true);
}

float4 psVSMVFilter1(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
   float4 t0;
   
   asm
   {
     tfetch2D t0.x___, tex0, gPointSampler0, OffsetY =  0.0, MinFilter=point, MagFilter=point
   };
   
   // 1 3 1

   float z = t0.x;
   
   t0 *= t0;
         
   float z2 = t0.x;
            
   return float4( z, z2, 0, 0 );
}

float4 psVSMHFilter1(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
   float4 t0;
   
   asm
   {
     tfetch2D t0.xy__, tex0, gPointSampler0, OffsetX =  0.0, MinFilter=point, MagFilter=point
   };
   
   // 1 3 1
                           
   return encodeXY(float4( t0.x, t0.y, 0, 0 ), true);
}

// 1/25 3/25 1/25
// 3/25 9/25 3/25
// 1/25 3/25 1/25

float4 psVSM2DFilter3(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
   float3 s;
   float3 t;
   float3 u;
   
   asm
   {
     tfetch2D s.x__, tex0, gPointSampler0, OffsetX = -1.0, OffsetY = -1.0, MinFilter=point, MagFilter=point
     tfetch2D s._x_, tex0, gPointSampler0, OffsetX =  0.0, OffsetY = -1.0, MinFilter=point, MagFilter=point
     tfetch2D s.__x, tex0, gPointSampler0, OffsetX =  1.0, OffsetY = -1.0, MinFilter=point, MagFilter=point
     
     tfetch2D t.x__, tex0, gPointSampler0, OffsetX = -1.0, OffsetY =  0.0, MinFilter=point, MagFilter=point
     tfetch2D t._x_, tex0, gPointSampler0, OffsetX =  0.0, OffsetY =  0.0, MinFilter=point, MagFilter=point
     tfetch2D t.__x, tex0, gPointSampler0, OffsetX =  1.0, OffsetY =  0.0, MinFilter=point, MagFilter=point
     
     tfetch2D u.x__, tex0, gPointSampler0, OffsetX = -1.0, OffsetY =  1.0, MinFilter=point, MagFilter=point
     tfetch2D u._x_, tex0, gPointSampler0, OffsetX =  0.0, OffsetY =  1.0, MinFilter=point, MagFilter=point
     tfetch2D u.__x, tex0, gPointSampler0, OffsetX =  1.0, OffsetY =  1.0, MinFilter=point, MagFilter=point
   };
   
   float d = dot(s, float3(1.0/25.0, 3.0/25.0, 1.0/25.0)) + dot(t, float3(3.0/25.0, 9.0/25.0, 3.0/25.0)) + dot(u, float3(1.0/25.0, 3.0/25.0, 1.0/25.0));
   float d2 = dot(s*s, float3(1.0/25.0, 3.0/25.0, 1.0/25.0)) + dot(t*t, float3(3.0/25.0, 9.0/25.0, 3.0/25.0)) + dot(u*u, float3(1.0/25.0, 3.0/25.0, 1.0/25.0));
	
	return encodeXY(float4(d, d2, 0, 0), true);
}

float4 psVSM2DFilter1(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1) : COLOR
{
   float d;
   
   asm
   {
     tfetch2D d.x, tex0, gPointSampler0, OffsetX =  0.0, OffsetY =  0.0, MinFilter=point, MagFilter=point
   };
   
	return encodeXY(float4(d, d * d, 0, 0), true);
}

technique VSMVFilter
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSMVFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
   
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSMVFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
   
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSMVFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique VSMHFilter
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSMHFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
   
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSMHFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
   
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSMHFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}


technique VSM2DFilter
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSM2DFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
   
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuad();
      PixelShader = compile ps_3_0 psVSM2DFilter1();

      ZEnable = false;
      ViewPortEnable = false;
      ZWriteEnable = false;
      ZFunc = always;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}