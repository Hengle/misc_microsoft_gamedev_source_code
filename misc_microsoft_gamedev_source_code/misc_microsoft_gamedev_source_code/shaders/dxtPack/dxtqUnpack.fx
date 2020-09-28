// File: dxtqUnpack.fx

#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

//#define SHADER_DEBUGGING

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
};

float4x4 gWorldViewProjMatrix : register(c0);

sampler VertexFetchConstant0 : register(vf0);
sampler VertexFetchConstant1 : register(vf1);
   
float4 const01 = {0, 1, 0, 0};
float4 exportAddress : register(c0); 

float4 gUnpackConst : register(c1);

float4 psDummy(void) : COLOR0
{
   return 0.0;
}

static void fetchIndexPairFromStream(sampler streamSampler, int index, out float index0, out float index1)
{
   // 0  1  2  3  4  5  6  7  8  9  A  B
   // X  Y  Z  X  Y  Z  X  Y  Z  X  Y  Z
   // 10 10 10|10 10 10|10 10 10|10 10 10
   // C  S  C |S  C  S |C  S  C |S  C  S
   // 0     1     2     0     1     2
   // 0     1     2     3     4     5
   
   float vertexIndex = floor((index * 2.0) / 3.0);
   float typeIndex = index - floor(index / 3.0) * 3.0;
   
   float4 v0, v1;
   asm
   {
      vfetch_full v0, vertexIndex, streamSampler, DataFormat=FMT_2_10_10_10, PrefetchCount=2, Stride=1, NumFormat=integer
      vfetch_mini v1, DataFormat=FMT_2_10_10_10, Offset=1, NumFormat=integer
   };
   
   index0 = v0.x;
   index1 = v0.y;
   if (typeIndex > 0.0)
   {
      if (typeIndex > 1.0)
      {
         index0 = v0.y;
         index1 = v0.z;
      }
      else 
      {
         index0 = v0.z;
         index1 = v1.x;
      }
   }      
}

[maxtempreg(32)]
#ifdef SHADER_DEBUGGING
void vsDXT1QUnpack(in int index : INDEX, out float4 dummyPos : POSITION) 
#else
void vsDXT1QUnpack(in int index : INDEX) 
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

   // Read color index from vertex stream 0
   // Read color selector index from vertex stream 0
   // Read color value from color codebook texture
   // Read color selector value from color codebook texture
   // Memexport
   
   // Vertex stream: D3DFMT_A2B10G10R10/D3DDECLTYPE_UDEC4
   // Textures: D3DFMT_G16R16
   
   // 0  1  2  3  4  5  6  7  8  9  A  B
   // X  Y  Z  X  Y  Z  X  Y  Z  X  Y  Z
   // 10 10 10|10 10 10|10 10 10|10 10 10
   // C  S  C |S  C  S |C  S  C |S  C  S
   // 0     1     2     0     1     2
   // 0     1     2     3     4     5
   
   float colorIndex, colorSelectorIndex;
   fetchIndexPairFromStream(VertexFetchConstant0, index, colorIndex, colorSelectorIndex);
   
   float colorSelectorPixel = gUnpackConst.x + colorSelectorIndex;
   
   float4 colorValues;
   float4 colorSelectorValues;
   asm
   {
      tfetch1D colorValues, colorIndex, gPointSampler0, UnnormalizedTextureCoords=true, MinFilter=point, MagFilter=point, MipFilter=point, UseComputedLOD=false
      tfetch1D colorSelectorValues, colorSelectorPixel, gPointSampler0, UnnormalizedTextureCoords=true, MinFilter=point, MagFilter=point, MipFilter=point, UseComputedLOD=false
   };
   
   float4 output;
   output.z = colorValues.x;
   output.y = colorValues.y;
   output.x = colorSelectorValues.x;
   output.w = colorSelectorValues.y;

   float dstBlockIndex = index;
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

technique DXT1QUnpack
{
   pass 
   {
      PixelShader = compile ps_3_0 psDummy();
      VertexShader = compile vs_3_0 vsDXT1QUnpack();

      ViewPortEnable = false;
      ZEnable = false;
      ZWriteEnable = false;
      CullMode = none;
   }
}
