// File: dxtPack.fx

#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

//#define USE_ASM_DXT_PACK_SHADER
//#define SHADER_DEBUGGING

// The C++ code overrides these sampler settings after commit() is called!
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
};

float4x4 gWorldViewProjMatrix : register(c0);

sampler FullScreenQuadFetchConstant : register(vf0);

// Fullscreen Quad

void vsFullScreenQuad(
   int Index : INDEX,
   out float4 OutPosition : POSITION,
   out float4 OutTex0 : TEXCOORD0)
{
   float4 InPosition;
   float4 InTex0;
     
   asm
   {
     vfetch_full InPosition, Index, FullScreenQuadFetchConstant, DataFormat=FMT_32_32_FLOAT, PrefetchCount=4, Stride=4
     vfetch_mini InTex0, DataFormat=FMT_32_32_FLOAT, Offset=2
   };

   InPosition.z = 0.0;
   InPosition.w = 1.0;

   OutPosition = mul(InPosition, gWorldViewProjMatrix);

   OutTex0 = InTex0;
}

float4 const01 = {0, 1, 0, 0};

float4 exportAddress : register(c0); 
float4 gTexDim : register(c1); // 1/width, width, .5/width, rowPitch
float4 gTexLOD : register(c2); // 1/width, width, .5/width, rowPitch
bool gTiledTexture : register(b0);
sampler DstIndexFetchConstant : register(vf0);

float4 psDummy(void) : COLOR0
{
   return 0.0;
}

static float dist2(float3 a, float3 b)
{
   float3 x = a - b;
   return dot(x, x);
}

static float pack565(float3 c)
{
   c *= float3(31.999, 63.999, 31.999);
   c = floor(c);
   return dot(c, float3(2048.0, 32.0, 1.0));
}

static float3 unpack565(float c)
{
   float t = c * 1.0/32.0;

   float b = frac(t) * 32.0;
         
   t = floor(t) * 1.0/64.0;
   
   float g = frac(t) * 64.0;
   
   t = floor(t);
   
   float r = t;
   
   return float3(r*1.0/31.0, g*1.0/63.0, b*1.0/31.0);
}

static float4 packDXT5H(float4 color, float hdrScale)
{
	float mxx = max(color.r,color.g);
	mxx = max(mxx,color.b);
	float imul = sqrt(saturate(mxx / hdrScale));
	float multiplier = imul + 1.0f/255.0f;
	multiplier = multiplier * multiplier * hdrScale;

	float4 packed = float4(saturate(color.xyz / multiplier),imul);

	packed.xyz = sqrt( float3(1.0, 1.0, 1.0) - min(packed.xyz,float3(1.0, 1.0, 1.0)));

	return packed;
}

[maxtempreg(32)]
#ifdef SHADER_DEBUGGING
void vsDXT1Pack(in int index : INDEX, uniform bool doDownSample, out float4 dummyPos : POSITION) 
#else
void vsDXT1Pack(in int index : INDEX, uniform bool doDownSample) 
#endif
{
//       0           1
//   
// Z B BBBB BGGG   GGGR RRRR High
// Y G BBBB BGGG   GGGR RRRR Low

// X R 0011 2233   4455 6677 Scanline 0-1 Selectors
// W A 8899 AABB   CCDD EEFF Scanline 2-3 Selectors 

// 00 11 22 33
// 44 55 66 77
// 88 99 AA BB
// CC DD EE FF

   float3 c[16];
   float dstBlockIndex;
   
   [isolate]
   {
      float cellY = floor(index * gTexDim.x + gTexDim.z);
      float cellX = index - (cellY * gTexDim.y);
            
      if (gTiledTexture)
      {
         float4 dstIndex4;
         asm
         {
            vfetch_full dstIndex4, index, DstIndexFetchConstant, DataFormat=FMT_16_16, PrefetchCount=1, Stride=1, NumFormat=integer
         };
         dstBlockIndex = dstIndex4.x;
      }
      else
      {
         dstBlockIndex = cellX + cellY * gTexDim.w;
      }
              
	 if (!doDownSample)      
	 {
		  float2 pixelCoord = float2(cellX * 4.0, cellY * 4.0);
	      
		  [isolate]
		  {      
			 float4 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
			 asm 
			 {
				setTexLOD gTexLOD
				tfetch2D x0, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 0.0
				tfetch2D x1, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 0.0
				tfetch2D x2, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 0.0
				tfetch2D x3, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 0.0
	            
				tfetch2D x4, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 1.0
				tfetch2D x5, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 1.0
				tfetch2D x6, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 1.0
				tfetch2D x7, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 1.0
	            
				tfetch2D x8, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 2.0
				tfetch2D x9, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 2.0
				tfetch2D x10, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 2.0
				tfetch2D x11, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 2.0
	            
				tfetch2D x12, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 3.0
				tfetch2D x13, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 3.0
				tfetch2D x14, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 3.0
				tfetch2D x15, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 3.0
			 };
	               
			 c[0] = x0; c[1] = x1; c[2] = x2; c[3] = x3;
			 c[4] = x4; c[5] = x5; c[6] = x6; c[7] = x7;
			 c[8] = x8; c[9] = x9; c[10] = x10; c[11] = x11;
			 c[12] = x12; c[13] = x13; c[14] = x14; c[15] = x15;
		  }
      }   
      else   
      {
		  float2 pixelCoord = float2(cellX * 8.0, cellY * 8.0);
	      
		  [isolate]
		  {      
			 float4 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
			 asm 
			 {
				setTexLOD gTexLOD
				tfetch2D x0, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 1.0
				tfetch2D x1, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 1.0
				tfetch2D x2, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 1.0
				tfetch2D x3, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 1.0
	            
				tfetch2D x4, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 3.0
				tfetch2D x5, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 3.0
				tfetch2D x6, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 3.0
				tfetch2D x7, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 3.0
	            
				tfetch2D x8, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 5.0
				tfetch2D x9, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 5.0
				tfetch2D x10, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 5.0
				tfetch2D x11, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 5.0
	            
				tfetch2D x12, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 7.0
				tfetch2D x13, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 7.0
				tfetch2D x14, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 7.0
				tfetch2D x15, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 7.0
			 };
	               
			 c[0] = x0; c[1] = x1; c[2] = x2; c[3] = x3;
			 c[4] = x4; c[5] = x5; c[6] = x6; c[7] = x7;
			 c[8] = x8; c[9] = x9; c[10] = x10; c[11] = x11;
			 c[12] = x12; c[13] = x13; c[14] = x14; c[15] = x15;
		  }
      }      
   }      

   float3 col0, col1, col2, col3;
   float4 output;
   {      
      float3 meanColor;
      [isolate]
      {
         meanColor = c[0];
         
         [unroll]
         for (int i = 1; i < 16; i++)
            meanColor += c[i];

         meanColor *= (1.0 / 16.0);
      }
         
      float3 axis;

      [isolate]
      {      
         float3 v = 0.0;

         [unroll]
         for (int i = 0; i < 16; i++)
         {   
            c[i] -= meanColor; 
            
            float3 a = c[i] * c[i].x; 
            float3 b = c[i] * c[i].y; 
            float3 c = c[i] * c[i].z;
            
            float3 n = tryNormalize(i ? v : c[0]);
               
            v.x += dot(a, n); 
            v.y += dot(b, n); 
            v.z += dot(c, n);
         }   
         
         axis = tryNormalize(v);
      }  

      [branch]
      if (dot(axis, axis) < .5)
      {
         float maxB = -100;
         float3 maxC = 0;

         [unroll]                  
         for (int i = 0; i < 16; i++)
         {
            float3 rgbToLum = float3(.213, .715, .072);
            float b = dot(rgbToLum, c[i]);
            
            maxC = (b > maxB) ? c[i] : maxC;
            
            maxB = max(maxB, b);
         }
               
         axis = tryNormalize(maxC);
      }

      [isolate]
      {
         float l = dot(c[0], axis);
         float h = l;
         
         [unroll]
         for (int i = 1; i < 16; i++)
         {
            float d = dot(c[i], axis);
            l = min(l, d);
            h = max(h, d);
         }
         
         col0 = l * axis; // min
         col3 = h * axis; // max
      }      
      
      float3 minColor = saturate(col0 + meanColor);
      float3 maxColor = saturate(col3 + meanColor);
            
      output.g = pack565(minColor);
      output.b = pack565(maxColor);
      
      if (output.b < output.g)
         swap(output.g, output.b);
      
      col0 = unpack565(output.g) - meanColor;
      col3 = unpack565(output.b) - meanColor;
      
      col1 = lerp(col0, col3, 1.0/3.0);
      col2 = lerp(col0, col3, 2.0/3.0);
   }  
   
   float4 altOutput = 0.0;    
   
   [isolate]
   {
      float3 x0 = c[0], x1 = c[1], x2 = c[2], x3 = c[3];
      float s0, s1, s2, s3;
      
      //1
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      {
         float4 m = min(min(min(d, e), f), g);

         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));
                
         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));
                
         mask = (m != g);
         result *= mask;

         output.r = dot(result, float4(1.0f, 4.0f, 16.0f, 64.0f));		
      }         

   }

   [isolate]
   {		
      float3 x0 = c[4], x1 = c[5], x2 = c[6], x3 = c[7];
      float s0, s1, s2, s3;
      
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      {
         float4 m = min(min(min(d, e), f), g);

         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));

         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));

         mask = (m != g);
         result *= mask;

         output.r += dot(result, float4(256.0, 1024.0, 4096, 16384.0));		
      }
   }		

   [isolate]
   {
      float3 x0 = c[8], x1 = c[9], x2 = c[10], x3 = c[11];
      float s0, s1, s2, s3;
      
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      {
         float4 m = min(min(min(d, e), f), g);
         
         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));

         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));

         mask = (m != g);
         result *= mask;

         output.a = dot(result, float4(1.0f, 4.0f, 16.0f, 64.0f));		
      }

   }		

   [isolate]
   {
      float3 x0 = c[12], x1 = c[13], x2 = c[14], x3 = c[15];
      float s0, s1, s2, s3;
      
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      {
         float4 m = min(min(min(d, e), f), g);
         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));

         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));

         mask = (m != g);
         result *= mask;

         output.a += dot(result, float4(256.0, 1024.0, 4096, 16384.0));		
      }
              
   }		
   
   if (output.g == output.b)
      output.ar = 0.0;
                  
   asm 
   {
      alloc export = 1
      mad eA, dstBlockIndex, const01, exportAddress
      mov eM0, output
   };

#ifdef SHADER_DEBUGGING	
   dummyPos = float4(0,0,0,1);
#endif	
}

[maxtempreg(32)]
#ifdef SHADER_DEBUGGING
void vsDXT5Pack(in int index : INDEX, uniform bool doDXT5H, uniform bool doDownSample,out float4 dummyPos : POSITION)
#else
void vsDXT5Pack(in int index : INDEX, uniform bool doDXT5H, uniform bool doDownSample) 
#endif
{
//       0           1
//   
// Z B BBBB BGGG   GGGR RRRR High
// Y G BBBB BGGG   GGGR RRRR Low

// X R 0011 2233   4455 6677 Scanline 0-1 Selectors
// W A 8899 AABB   CCDD EEFF Scanline 2-3 Selectors 

// 00 11 22 33
// 44 55 66 77
// 88 99 AA BB
// CC DD EE FF
     
   float cellY = floor(index * gTexDim.x + gTexDim.z);
   float cellX = index - (cellY * gTexDim.y);
   
   float dstBlockIndex;
   if (gTiledTexture)
   {
      float4 dstIndex4;
      asm
      {
         vfetch_full dstIndex4, index, DstIndexFetchConstant, DataFormat=FMT_16_16, PrefetchCount=1, Stride=1, NumFormat=integer
      };
      dstBlockIndex = dstIndex4.y;
   }
   else
   {
      dstBlockIndex = cellX + cellY * gTexDim.w;
   }

   float2 cellUV = float2(cellX, cellY);
   
   float4 c[16];
   if (!doDownSample)
   { 
		float2 pixelCoord = float2(cellX * 4.0, cellY * 4.0);   
	   [isolate]
	   {      
		  float4 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
		  asm 
		  {
			 setTexLOD gTexLOD
			 tfetch2D x0, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 0.0
			 tfetch2D x1, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 0.0
			 tfetch2D x2, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 0.0
			 tfetch2D x3, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 0.0
	         
			 tfetch2D x4, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 1.0
			 tfetch2D x5, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 1.0
			 tfetch2D x6, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 1.0
			 tfetch2D x7, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 1.0
	         
			 tfetch2D x8, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 2.0
			 tfetch2D x9, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 2.0
			 tfetch2D x10, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 2.0
			 tfetch2D x11, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 2.0
	         
			 tfetch2D x12, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 3.0
			 tfetch2D x13, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 3.0
			 tfetch2D x14, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 3.0
			 tfetch2D x15, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 3.0
		  };
	            
		  c[0] = x0; c[1] = x1; c[2] = x2; c[3] = x3;
		  c[4] = x4; c[5] = x5; c[6] = x6; c[7] = x7;
		  c[8] = x8; c[9] = x9; c[10] = x10; c[11] = x11;
		  c[12] = x12; c[13] = x13; c[14] = x14; c[15] = x15;
	   }   
   }
   else   
    {
		  float2 pixelCoord= float2(cellX * 8.0, cellY * 8.0);

	   [isolate]
	   {      
		  float4 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
		  asm 
		  {
			 setTexLOD gTexLOD
			 tfetch2D x0, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 1.0
			 tfetch2D x1, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 1.0
			 tfetch2D x2, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 1.0
			 tfetch2D x3, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 1.0
	         
			 tfetch2D x4, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 3.0
			 tfetch2D x5, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 3.0
			 tfetch2D x6, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 3.0
			 tfetch2D x7, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 3.0
	         
			 tfetch2D x8, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 5.0
			 tfetch2D x9, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 5.0
			 tfetch2D x10, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 5.0
			 tfetch2D x11, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 5.0
	         
			 tfetch2D x12, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 7.0
			 tfetch2D x13, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 7.0
			 tfetch2D x14, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 7.0
			 tfetch2D x15, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 7.0
		  };
	            
		  c[0] = x0; c[1] = x1; c[2] = x2; c[3] = x3;
		  c[4] = x4; c[5] = x5; c[6] = x6; c[7] = x7;
		  c[8] = x8; c[9] = x9; c[10] = x10; c[11] = x11;
		  c[12] = x12; c[13] = x13; c[14] = x14; c[15] = x15;
	   }
	      
	}
      
   if(doDXT5H)
   {
	float hdrScale = 16;
	   c[0] = packDXT5H(c[0], hdrScale);
	   c[1] = packDXT5H(c[1], hdrScale);
	   c[2] = packDXT5H(c[2], hdrScale);
	   c[3] = packDXT5H(c[3], hdrScale);
	   c[4] = packDXT5H(c[4], hdrScale);
	   c[5] = packDXT5H(c[5], hdrScale);
	   c[6] = packDXT5H(c[6], hdrScale);
	   c[7] = packDXT5H(c[7], hdrScale);
	   c[8] = packDXT5H(c[8], hdrScale);
	   c[9] = packDXT5H(c[9], hdrScale);
	   c[10] = packDXT5H(c[10], hdrScale);
	   c[11] = packDXT5H(c[11], hdrScale);
	   c[12] = packDXT5H(c[12], hdrScale);
	   c[13] = packDXT5H(c[13], hdrScale);
	   c[14] = packDXT5H(c[14], hdrScale);
	   c[15] = packDXT5H(c[15], hdrScale);
   }
   
   float4 meanColor;
   float maxAlpha;
   [isolate]
   {
      meanColor = c[0];
      maxAlpha = c[0].a;
      
      [unroll]
      for (int i = 1; i < 16; i++)
      {
         meanColor += c[i];
         maxAlpha = max(maxAlpha, c[i].a);
      }

      meanColor *= (1.0 / 16.0);
   }
   
   float3 axis;
   [isolate]
   {      
      float3 v = 0.0;

      [unroll]
      for (int i = 0; i < 16; i++)
      {   
         c[i].rgb = c[i] - meanColor; 
         
         float3 a = c[i] * c[i].x; 
         float3 b = c[i] * c[i].y; 
         float3 c = c[i] * c[i].z;
         
         float3 n = tryNormalize(i ? v : c[0]);
            
         v.x += dot(a, n); 
         v.y += dot(b, n); 
         v.z += dot(c, n);
      }   
      
      axis = tryNormalize(v);
   }      
   
   [branch]
   if (dot(axis, axis) < .5)
   {
      float maxB = -100;
      float3 maxC = 0;

      [unroll]                  
      for (int i = 0; i < 16; i++)
      {
         float3 rgbToLum = float3(.213, .715, .072);
         float b = dot(rgbToLum, c[i]);
         
         maxC = (b > maxB) ? c[i] : maxC;
         
         maxB = max(maxB, b);
      }
            
      axis = tryNormalize(maxC);
   }
   
   float3 col0, col3;
   [isolate]
   {
      float l = dot(c[0], axis);
      float h = l;
      
      [unroll]
      for (int i = 1; i < 16; i++)
      {
         float d = dot(c[i], axis);
         l = min(l, d);
         h = max(h, d);
      }
      
      col0 = l * axis; // min
      col3 = h * axis; // max
   }      
   
   float3 minColor = saturate(col0 + meanColor);
   float3 maxColor = saturate(col3 + meanColor);
   
   float4 output;
   output.g = pack565(minColor);
   output.b = pack565(maxColor);
   
   if (output.b < output.g)
      swap(output.g, output.b);
   
   col0 = unpack565(output.g) - meanColor;
   col3 = unpack565(output.b) - meanColor;
   
   float3 col1 = lerp(col0, col3, 1.0/3.0);
   float3 col2 = lerp(col0, col3, 2.0/3.0);
   
   // DXT5A
   //         0           1
   // Z B HHHH HHHH   LLLL LLLL
   // Y G 0001 1122   2333 4445
   // X R 5566 6777   8889 99AA
   // W A ABBB CCCD   DDEE EFFF

   // 0 1 2 3
   // 4 5 6 7
   // 8 9 A B
   // C D E F
      
   float minA = saturate(meanColor.a - (maxAlpha - meanColor.a));
   float maxA = saturate(maxAlpha);
   float4 d = maxA - minA;    
   float4 alphaABCD = minA + d * float4(0.0/7.0, 1.0/7.0, 2.0/7.0, 3.0/7.0);
   float4 alphaEFGH = minA + d * float4(4.0/7.0, 5.0/7.0, 6.0/7.0, 7.0/7.0);

   float4 alphaOutput = 0.0;
   float highAlphaByte = floor(255.0 * maxA);
   float lowAlphaByte = floor(255.0 * minA);
   float areAlphaBytesNotEqual = (highAlphaByte != lowAlphaByte);
   alphaOutput.b = highAlphaByte + (lowAlphaByte * 256.0);

   [isolate]
   {
      float3 x0 = c[0], x1 = c[1], x2 = c[2], x3 = c[3];
      float s0, s1, s2, s3;
      
      //1
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      [isolate]
      {
         float4 m = min(min(min(d, e), f), g);

         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));
                
         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));
                
         mask = (m != g);
         result *= mask;

         output.r = dot(result, float4(1.0, 4.0, 16.0, 64.0));
      }
      
      // alpha 0
      [isolate]        
      {
         float4 alpha4 = float4(c[0].a, c[1].a, c[2].a, c[3].a);
         
         float4 d0 = abs(alpha4 - alphaABCD.x); 
         float4 d1 = abs(alpha4 - alphaABCD.y); 
         float4 d2 = abs(alpha4 - alphaABCD.z); 
         float4 d3 = abs(alpha4 - alphaABCD.w); 
         float4 d4 = abs(alpha4 - alphaEFGH.x); 
         float4 d5 = abs(alpha4 - alphaEFGH.y); 
         float4 d6 = abs(alpha4 - alphaEFGH.z); 
         float4 d7 = abs(alpha4 - alphaEFGH.w); 

         float4 alphaMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (alphaMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (alphaMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (alphaMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (alphaMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (alphaMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (alphaMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (alphaMin != d7);
         result *= mask;

// Z B HHHH HHHH   LLLL LLLL
// Y G 0001 1122   2333 4445
// X R 5566 6777   8889 99AA
// W A ABBB CCCD   DDEE EFFF

//result.xyzw = float4(1,1,1,1);

         alphaOutput.g = dot(result, float4(1.0, 8.0, 64.0, 512.0));
      }
   }

   [isolate]
   {		
      float3 x0 = c[4], x1 = c[5], x2 = c[6], x3 = c[7];
      float s0, s1, s2, s3;
      
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      [isolate]
      {
         float4 m = min(min(min(d, e), f), g);

         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));

         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));

         mask = (m != g);
         result *= mask;

         output.r += dot(result, float4(256.0, 1024.0, 4096, 16384.0));		
      }         

      // alpha 1
      [isolate]        
      {
         float4 alpha4 = float4(c[4].a, c[5].a, c[6].a, c[7].a);
         
         float4 d0 = abs(alpha4 - alphaABCD.x); 
         float4 d1 = abs(alpha4 - alphaABCD.y); 
         float4 d2 = abs(alpha4 - alphaABCD.z); 
         float4 d3 = abs(alpha4 - alphaABCD.w); 
         float4 d4 = abs(alpha4 - alphaEFGH.x); 
         float4 d5 = abs(alpha4 - alphaEFGH.y); 
         float4 d6 = abs(alpha4 - alphaEFGH.z); 
         float4 d7 = abs(alpha4 - alphaEFGH.w); 

         float4 alphaMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (alphaMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (alphaMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (alphaMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (alphaMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (alphaMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (alphaMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (alphaMin != d7);
         result *= mask;

// Z B HHHH HHHH   LLLL LLLL
// Y G 0001 1122   2333 4445
// X R 5566 6777   8889 99AA
// W A ABBB CCCD   DDEE EFFF

//result.xyzw = float4(1,1,1,1);

         alphaOutput.g += result.x * 4096.0 + fmod(result.y, 2.0) * 32768.0;
         
         alphaOutput.r = floor(result.y * .5) + (result.z * 4.0) + (result.w * 32.0);
      }
   }		

   [isolate]
   {
      float3 x0 = c[8], x1 = c[9], x2 = c[10], x3 = c[11];
      float s0, s1, s2, s3;
      
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);

      [isolate]
      {
         float4 m = min(min(min(d, e), f), g);

         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));

         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));

         mask = (m != g);
         result *= mask;

         output.a = dot(result, float4(1.0, 4.0, 16.0, 64.0));		
      }         

      // alpha 2
      [isolate]
      {
         float4 alpha4 = float4(c[8].a, c[9].a, c[10].a, c[11].a);
         
         float4 d0 = abs(alpha4 - alphaABCD.x); 
         float4 d1 = abs(alpha4 - alphaABCD.y); 
         float4 d2 = abs(alpha4 - alphaABCD.z); 
         float4 d3 = abs(alpha4 - alphaABCD.w); 
         float4 d4 = abs(alpha4 - alphaEFGH.x); 
         float4 d5 = abs(alpha4 - alphaEFGH.y); 
         float4 d6 = abs(alpha4 - alphaEFGH.z); 
         float4 d7 = abs(alpha4 - alphaEFGH.w); 

         float4 alphaMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (alphaMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (alphaMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (alphaMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (alphaMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (alphaMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (alphaMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (alphaMin != d7);
         result *= mask;

// Z B HHHH HHHH   LLLL LLLL
// Y G 0001 1122   2333 4445
// X R 5566 6777   8889 99AA
// W A ABBB CCCD   DDEE EFFF

//result.xyzw = float4(1,1,1,1);

         alphaOutput.r += (result.x * 256.0) + (result.y * 2048.0) + (fmod(result.z, 4.0) * 16384.0);
         alphaOutput.a = floor(result.z * .25) + (result.w * 2.0);
      }
   }		

   [isolate]
   {
      float3 x0 = c[12], x1 = c[13], x2 = c[14], x3 = c[15];
      float s0, s1, s2, s3;
      
      float4 d;
      d.x = dist2(col0, x0);
      d.y = dist2(col0, x1);
      d.z = dist2(col0, x2);
      d.w = dist2(col0, x3);

      //3	   
      float4 e;
      e.x = dist2(col1, x0);
      e.y = dist2(col1, x1);
      e.z = dist2(col1, x2);
      e.w = dist2(col1, x3);

      //2	   
      float4 f;
      f.x = dist2(col2, x0);
      f.y = dist2(col2, x1);
      f.z = dist2(col2, x2);
      f.w = dist2(col2, x3);

      //0	   
      float4 g;
      g.x = dist2(col3, x0);
      g.y = dist2(col3, x1);
      g.z = dist2(col3, x2);
      g.w = dist2(col3, x3);
      
      [isolate]
      {
         float4 m = min(min(min(d, e), f), g);

         float4 result = float4(1,1,1,1);

         float4 mask = (m == e);
         result = result * (1.0 - mask) + (mask * float4(3,3,3,3));

         mask = (m == f);
         result = result * (1.0 - mask) + (mask * float4(2,2,2,2));

         mask = (m != g);
         result *= mask;

         output.a += dot(result, float4(256.0, 1024.0, 4096, 16384.0));		
      }         

      // alpha 2
      [isolate]
      {
         float4 alpha4 = float4(c[12].a, c[13].a, c[14].a, c[15].a);
         
         float4 d0 = abs(alpha4 - alphaABCD.x); 
         float4 d1 = abs(alpha4 - alphaABCD.y); 
         float4 d2 = abs(alpha4 - alphaABCD.z); 
         float4 d3 = abs(alpha4 - alphaABCD.w); 
         float4 d4 = abs(alpha4 - alphaEFGH.x); 
         float4 d5 = abs(alpha4 - alphaEFGH.y); 
         float4 d6 = abs(alpha4 - alphaEFGH.z); 
         float4 d7 = abs(alpha4 - alphaEFGH.w); 

         float4 alphaMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (alphaMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (alphaMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (alphaMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (alphaMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (alphaMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (alphaMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (alphaMin != d7);
         result *= mask;

// Z B HHHH HHHH   LLLL LLLL
// Y G 0001 1122   2333 4445
// X R 5566 6777   8889 99AA
// W A ABBB CCCD   DDEE EFFF

//result.xyzw = float4(4,1,1,1);

         alphaOutput.a += dot(result, float4(16.0, 128.0, 1024.0, 8192.0));
      }
   }		

   if (output.g == output.b)
      output.ar = 0.0;
                                
   alphaOutput.arg *= areAlphaBytesNotEqual;

   float dstExportIndex0 = dstBlockIndex * 2.0 + 1;
   float dstExportIndex1 = dstBlockIndex * 2.0;

   // export
   asm 
   {
      alloc export = 1
      mad eA, dstExportIndex0, const01, exportAddress
      mov eM0, output

      alloc export = 1
      mad eA, dstExportIndex1, const01, exportAddress
      mov eM0, alphaOutput
   };

#ifdef SHADER_DEBUGGING	
   dummyPos = float4(0,0,0,1);
#endif	
}

[maxtempreg(32)]
#ifdef SHADER_DEBUGGING
void vsDXNPack(in int index : INDEX, uniform bool doDownSample, out float4 dummyPos : POSITION)
#else
void vsDXNPack(in int index : INDEX, uniform bool doDownSample) 
#endif
{
//       0           1
//   
// Z B BBBB BGGG   GGGR RRRR High
// Y G BBBB BGGG   GGGR RRRR Low

// X R 0011 2233   4455 6677 Scanline 0-1 Selectors
// W A 8899 AABB   CCDD EEFF Scanline 2-3 Selectors 

// 00 11 22 33
// 44 55 66 77
// 88 99 AA BB
// CC DD EE FF
     
   float cellY = floor(index * gTexDim.x + gTexDim.z);
   float cellX = index - (cellY * gTexDim.y);
   
   float dstBlockIndex;
   if (gTiledTexture)
   {
      float4 dstIndex4;
      asm
      {
         vfetch_full dstIndex4, index, DstIndexFetchConstant, DataFormat=FMT_16_16, PrefetchCount=1, Stride=1, NumFormat=integer
      };
      dstBlockIndex = dstIndex4.y;
   }
   else
   {
      dstBlockIndex = cellX + cellY * gTexDim.w;
   }

   float2 cellUV = float2(cellX, cellY);
    
   
   float2 c[16];
   if (!doDownSample)
   {
	   float2 pixelCoord = float2(cellX * 4.0, cellY * 4.0);  
	   [isolate]
	   {      
		  float4 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
		  asm 
		  {
			 setTexLOD gTexLOD
			 tfetch2D x0, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 0.0
			 tfetch2D x1, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 0.0
			 tfetch2D x2, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 0.0
			 tfetch2D x3, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 0.0
	         
			 tfetch2D x4, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 1.0
			 tfetch2D x5, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 1.0
			 tfetch2D x6, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 1.0
			 tfetch2D x7, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 1.0
	         
			 tfetch2D x8, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 2.0
			 tfetch2D x9, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 2.0
			 tfetch2D x10, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 2.0
			 tfetch2D x11, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 2.0
	         
			 tfetch2D x12, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 0.0, OffsetY = 3.0
			 tfetch2D x13, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 3.0
			 tfetch2D x14, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 2.0, OffsetY = 3.0
			 tfetch2D x15, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 3.0
		  };
	            
		  c[0] = x0; c[1] = x1; c[2] = x2; c[3] = x3;
		  c[4] = x4; c[5] = x5; c[6] = x6; c[7] = x7;
		  c[8] = x8; c[9] = x9; c[10] = x10; c[11] = x11;
		  c[12] = x12; c[13] = x13; c[14] = x14; c[15] = x15;
	   }      
	}
	else
	{
		float2 pixelCoord = float2(cellX * 8.0, cellY * 8.0);  
	   [isolate]
	   {      
		  float4 x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
		  asm 
		  {
			 setTexLOD gTexLOD
			 tfetch2D x0, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 1.0
			 tfetch2D x1, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 1.0
			 tfetch2D x2, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 1.0
			 tfetch2D x3, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 1.0
	         
			 tfetch2D x4, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 3.0
			 tfetch2D x5, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 3.0
			 tfetch2D x6, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 3.0
			 tfetch2D x7, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 3.0
	         
			 tfetch2D x8, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 5.0
			 tfetch2D x9, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 5.0
			 tfetch2D x10, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 5.0
			 tfetch2D x11, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 5.0
	         
			 tfetch2D x12, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 1.0, OffsetY = 7.0
			 tfetch2D x13, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 3.0, OffsetY = 7.0
			 tfetch2D x14, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 5.0, OffsetY = 7.0
			 tfetch2D x15, pixelCoord, gPointSampler0, UnnormalizedTextureCoords = true, UseComputedLOD = false, UseRegisterLOD = true, OffsetX = 7.0, OffsetY = 7.0
		  };
	            
		  c[0] = x0; c[1] = x1; c[2] = x2; c[3] = x3;
		  c[4] = x4; c[5] = x5; c[6] = x6; c[7] = x7;
		  c[8] = x8; c[9] = x9; c[10] = x10; c[11] = x11;
		  c[12] = x12; c[13] = x13; c[14] = x14; c[15] = x15;
	   }      
	}
	
   
   // DXT5A
   //         0           1
   // Z B HHHH HHHH   LLLL LLLL
   // Y G 0001 1122   2333 4445
   // X R 5566 6777   8889 99AA
   // W A ABBB CCCD   DDEE EFFF

   // 0 1 2 3
   // 4 5 6 7
   // 8 9 A B
   // C D E F
   
   float2 maxC = c[0];
   float2 minC = c[0];
     
   [unroll]
   for (int i = 1; i < 16; i++)
   {
      minC = min(minC, c[i]);
      maxC = max(maxC, c[i]);  
   }
   
   float d = maxC.r - minC.r;    
   float4 redABCD = minC.r + d * float4(0.0/7.0, 1.0/7.0, 2.0/7.0, 3.0/7.0);
   float4 redEFGH = minC.r + d * float4(4.0/7.0, 5.0/7.0, 6.0/7.0, 7.0/7.0);

   float4 redOutput = 0.0;
   float highRedByte = floor(255.0 * maxC.r);
   float lowRedByte = floor(255.0 * minC.r);
   float areRedBytesNotEqual = (highRedByte != lowRedByte);
   redOutput.b = highRedByte + (lowRedByte * 256.0);
   
   d = maxC.g - minC.g;
   float4 greenABCD = minC.g + d * float4(0.0/7.0, 1.0/7.0, 2.0/7.0, 3.0/7.0);
   float4 greenEFGH = minC.g + d * float4(4.0/7.0, 5.0/7.0, 6.0/7.0, 7.0/7.0);

   float4 greenOutput = 0.0;
   float highGreenByte = floor(255.0 * maxC.g);
   float lowGreenByte = floor(255.0 * minC.g);
   float areGreenBytesNotEqual = (highGreenByte != lowGreenByte);
   greenOutput.b = highGreenByte + (lowGreenByte * 256.0);

   [isolate]
   {
      // red 0
      [isolate]        
      {
         float4 red4 = float4(c[0].r, c[1].r, c[2].r, c[3].r);
         
         float4 d0 = abs(red4 - redABCD.x); 
         float4 d1 = abs(red4 - redABCD.y); 
         float4 d2 = abs(red4 - redABCD.z); 
         float4 d3 = abs(red4 - redABCD.w); 
         float4 d4 = abs(red4 - redEFGH.x); 
         float4 d5 = abs(red4 - redEFGH.y); 
         float4 d6 = abs(red4 - redEFGH.z); 
         float4 d7 = abs(red4 - redEFGH.w); 

         float4 redMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (redMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (redMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (redMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (redMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (redMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (redMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (redMin != d7);
         result *= mask;

// Z B HHHH HHHH   LLLL LLLL
// Y G 0001 1122   2333 4445
// X R 5566 6777   8889 99AA
// W A ABBB CCCD   DDEE EFFF

//result.xyzw = float4(1,1,1,1);

         redOutput.g = dot(result, float4(1.0, 8.0, 64.0, 512.0));
      }
      
      // green 0
      [isolate]        
      {
         float4 green4 = float4(c[0].g, c[1].g, c[2].g, c[3].g);
         
         float4 d0 = abs(green4 - greenABCD.x); 
         float4 d1 = abs(green4 - greenABCD.y); 
         float4 d2 = abs(green4 - greenABCD.z); 
         float4 d3 = abs(green4 - greenABCD.w); 
         float4 d4 = abs(green4 - greenEFGH.x); 
         float4 d5 = abs(green4 - greenEFGH.y); 
         float4 d6 = abs(green4 - greenEFGH.z); 
         float4 d7 = abs(green4 - greenEFGH.w); 

         float4 greenMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (greenMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (greenMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (greenMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (greenMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (greenMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (greenMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (greenMin != d7);
         result *= mask;

         greenOutput.g = dot(result, float4(1.0, 8.0, 64.0, 512.0));
      }
   }

   [isolate]
   {		
      // red 1
      [isolate]        
      {
         float4 red4 = float4(c[4].r, c[5].r, c[6].r, c[7].r);
         
         float4 d0 = abs(red4 - redABCD.x); 
         float4 d1 = abs(red4 - redABCD.y); 
         float4 d2 = abs(red4 - redABCD.z); 
         float4 d3 = abs(red4 - redABCD.w); 
         float4 d4 = abs(red4 - redEFGH.x); 
         float4 d5 = abs(red4 - redEFGH.y); 
         float4 d6 = abs(red4 - redEFGH.z); 
         float4 d7 = abs(red4 - redEFGH.w); 

         float4 redMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (redMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (redMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (redMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (redMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (redMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (redMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (redMin != d7);
         result *= mask;

         redOutput.g += result.x * 4096.0 + fmod(result.y, 2.0) * 32768.0;
         
         redOutput.r = floor(result.y * .5) + (result.z * 4.0) + (result.w * 32.0);
      }
      
      // green 1
      [isolate]        
      {
         float4 green4 = float4(c[4].g, c[5].g, c[6].g, c[7].g);
         
         float4 d0 = abs(green4 - greenABCD.x); 
         float4 d1 = abs(green4 - greenABCD.y); 
         float4 d2 = abs(green4 - greenABCD.z); 
         float4 d3 = abs(green4 - greenABCD.w); 
         float4 d4 = abs(green4 - greenEFGH.x); 
         float4 d5 = abs(green4 - greenEFGH.y); 
         float4 d6 = abs(green4 - greenEFGH.z); 
         float4 d7 = abs(green4 - greenEFGH.w); 

         float4 greenMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (greenMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (greenMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (greenMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (greenMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (greenMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (greenMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (greenMin != d7);
         result *= mask;

         greenOutput.g += result.x * 4096.0 + fmod(result.y, 2.0) * 32768.0;
         
         greenOutput.r = floor(result.y * .5) + (result.z * 4.0) + (result.w * 32.0);
      }
   }		

   [isolate]
   {
      // red 2
      [isolate]
      {
         float4 red4 = float4(c[8].r, c[9].r, c[10].r, c[11].r);
         
         float4 d0 = abs(red4 - redABCD.x); 
         float4 d1 = abs(red4 - redABCD.y); 
         float4 d2 = abs(red4 - redABCD.z); 
         float4 d3 = abs(red4 - redABCD.w); 
         float4 d4 = abs(red4 - redEFGH.x); 
         float4 d5 = abs(red4 - redEFGH.y); 
         float4 d6 = abs(red4 - redEFGH.z); 
         float4 d7 = abs(red4 - redEFGH.w); 

         float4 redMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (redMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (redMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (redMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (redMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (redMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (redMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (redMin != d7);
         result *= mask;

         redOutput.r += (result.x * 256.0) + (result.y * 2048.0) + (fmod(result.z, 4.0) * 16384.0);
         redOutput.a = floor(result.z * .25) + (result.w * 2.0);
      }
      
      // green 2
      [isolate]
      {
         float4 green4 = float4(c[8].g, c[9].g, c[10].g, c[11].g);
         
         float4 d0 = abs(green4 - greenABCD.x); 
         float4 d1 = abs(green4 - greenABCD.y); 
         float4 d2 = abs(green4 - greenABCD.z); 
         float4 d3 = abs(green4 - greenABCD.w); 
         float4 d4 = abs(green4 - greenEFGH.x); 
         float4 d5 = abs(green4 - greenEFGH.y); 
         float4 d6 = abs(green4 - greenEFGH.z); 
         float4 d7 = abs(green4 - greenEFGH.w); 

         float4 greenMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (greenMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (greenMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (greenMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (greenMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (greenMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (greenMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (greenMin != d7);
         result *= mask;

         greenOutput.r += (result.x * 256.0) + (result.y * 2048.0) + (fmod(result.z, 4.0) * 16384.0);
         greenOutput.a = floor(result.z * .25) + (result.w * 2.0);
      }
   }		

   [isolate]
   {
      // red 2
      [isolate]
      {
         float4 red4 = float4(c[12].r, c[13].r, c[14].r, c[15].r);
         
         float4 d0 = abs(red4 - redABCD.x); 
         float4 d1 = abs(red4 - redABCD.y); 
         float4 d2 = abs(red4 - redABCD.z); 
         float4 d3 = abs(red4 - redABCD.w); 
         float4 d4 = abs(red4 - redEFGH.x); 
         float4 d5 = abs(red4 - redEFGH.y); 
         float4 d6 = abs(red4 - redEFGH.z); 
         float4 d7 = abs(red4 - redEFGH.w); 

         float4 redMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (redMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (redMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (redMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (redMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (redMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (redMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (redMin != d7);
         result *= mask;

         redOutput.a += dot(result, float4(16.0, 128.0, 1024.0, 8192.0));
      }
      
      // green 2
      [isolate]
      {
         float4 green4 = float4(c[12].g, c[13].g, c[14].g, c[15].g);
         
         float4 d0 = abs(green4 - greenABCD.x); 
         float4 d1 = abs(green4 - greenABCD.y); 
         float4 d2 = abs(green4 - greenABCD.z); 
         float4 d3 = abs(green4 - greenABCD.w); 
         float4 d4 = abs(green4 - greenEFGH.x); 
         float4 d5 = abs(green4 - greenEFGH.y); 
         float4 d6 = abs(green4 - greenEFGH.z); 
         float4 d7 = abs(green4 - greenEFGH.w); 

         float4 greenMin = min(min(min(min(min(min(min(d0, d1), d2), d3), d4), d5), d6), d7);
         
         float4 result = 1.0;
         
         float4 mask = (greenMin == d1);
         result = result * (1.0 - mask) + (mask * 7.0);

         mask = (greenMin == d2);
         result = result * (1.0 - mask) + (mask * 6.0);

         mask = (greenMin == d3);
         result = result * (1.0 - mask) + (mask * 5.0);

         mask = (greenMin == d4);
         result = result * (1.0 - mask) + (mask * 4.0);

         mask = (greenMin == d5);
         result = result * (1.0 - mask) + (mask * 3.0);

         mask = (greenMin == d6);
         result = result * (1.0 - mask) + (mask * 2.0);

         mask = (greenMin != d7);
         result *= mask;

         greenOutput.a += dot(result, float4(16.0, 128.0, 1024.0, 8192.0));
      }
   }		

   redOutput.arg *= areRedBytesNotEqual;
   greenOutput.arg *= areGreenBytesNotEqual;

   float dstExportIndex0 = dstBlockIndex * 2.0 + 1;
   float dstExportIndex1 = dstBlockIndex * 2.0;

   // export
   asm 
   {
      alloc export = 1
      mad eA, dstExportIndex0, const01, exportAddress
      mov eM0, greenOutput

      alloc export = 1
      mad eA, dstExportIndex1, const01, exportAddress
      mov eM0, redOutput
   };

#ifdef SHADER_DEBUGGING	
   dummyPos = float4(0,0,0,1);
#endif	
}

technique DXT1Pack
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT1Pack(false);

      ViewPortEnable = false;
      ZEnable = false;
      ZWriteEnable = false;
      CullMode = none;
   }
}

technique DXT5Pack
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT5Pack(false, false);
	}
}	

technique DXT5HPack
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT5Pack(true, false);
	}
}	

technique DXNPack
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXNPack(false);
	}
}
//====================================DOWNSAMPLED VERSIONS

technique DXT1PackDS
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT1Pack(true);

      ViewPortEnable = false;
      ZEnable = false;
      ZWriteEnable = false;
      CullMode = none;
   }
}

technique DXT5PackDS
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT5Pack(false, true);
	}
}	

technique DXT5HPackDS
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT5Pack(true, true);
	}
}	

technique DXNPackDS
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXNPack(true);
	}
}

