// tonemap.fx
// rg FIXME: Lots of these parameters are manually bound to registers, and set manually. Be sure to change them to manual register update!
#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

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

sampler gPointSampler5 : register(s5) = sampler_state
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

sampler gPointSampler6 : register(s6) = sampler_state
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

sampler gPointSampler7 : register(s7) = sampler_state
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

sampler gLinearSampler5 : register(s5) = sampler_state
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

sampler gLinearSampler6 : register(s6) = sampler_state
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

float4x4 gWorldViewProjMatrix : register(c0);

sampler FullScreenQuadFetchConstant : register(vf0);

bool gUnswizzleTexels : register(b3);
static float2 gUnswizzleGrid[4][4] = 
{
   { float2(0, 0 ), float2(2, 0), float2(1, 0), float2(3, 0) },
   { float2(0, 2 ), float2(2, 2), float2(1, 2), float2(3, 2) },
   
   { float2(0, 1 ), float2(2, 1), float2(1, 1), float2(3, 1) },
   { float2(0, 3 ), float2(2, 3), float2(1, 3), float2(3, 3) }
};

float2 gTextureWidthHeight, gTextureInvWidthHeight;

// Fullscreen Quad

void vsFullScreenQuadTex1(
    int Index : INDEX,
    out float4 OutPosition : POSITION,
    out float4 OutTex0 : TEXCOORD0)
{
    float4 InPosition;
    float4 InTex0;
        
    asm
    {
        vfetch_full InPosition, Index, FullScreenQuadFetchConstant, DataFormat=FMT_32_32_FLOAT, PrefetchCount=6, Stride=6
        vfetch_mini InTex0, DataFormat=FMT_32_32_FLOAT, Offset=2
    };
    
    InPosition.z = 0.0;
    InPosition.w = 1.0;

    OutPosition = mul(InPosition, gWorldViewProjMatrix);

    OutTex0 = InTex0;
}

void vsFullScreenQuadTex2(
    int Index : INDEX,
    out float4 OutPosition : POSITION,
    out float4 OutTex0 : TEXCOORD0,
    out float4 OutTex1 : TEXCOORD1)
{
    float4 InPosition;
    float4 InTex0;
    float4 InTex1;
        
    asm
    {
        vfetch_full InPosition, Index, FullScreenQuadFetchConstant, DataFormat=FMT_32_32_FLOAT, PrefetchCount=6, Stride=6
        vfetch_mini InTex0, DataFormat=FMT_32_32_FLOAT, Offset=2
        vfetch_mini InTex1, DataFormat=FMT_32_32_FLOAT, Offset=4
    };
    
    InPosition.z = 0.0;
    InPosition.w = 1.0;

    OutPosition = mul(InPosition, gWorldViewProjMatrix);

    OutTex0 = InTex0;
    OutTex1 = InTex1;
}

// Tonemap

float4 gMiddleGreyMax : register(c0); // x: middle grey, y: color xform factor, z: blur factor
float3 gRTransform : register(c4);
float3 gGTransform : register(c5);
float3 gBTransform : register(c6);
bool gDistortionEnabled : register(b0);
bool gColorTransformEnabled : register(b1);
bool gScreenBlurEnabled : register(b2);

static float3 tonemap(float3 sample)
{
   float middleGray = gMiddleGreyMax.x;
   float3 adaptationParams = tex2D(gPointSampler1, float2(.5, .5));
   
   float sampleLum = dot(sample, float3(.213, .715, .072));
   float adaptedLum = adaptationParams.r;
   float maxLum = adaptationParams.g;

#if 1
   float l = sampleLum * middleGray / (adaptedLum + 0.00001f);
   float ld = l * (1.0 + (l / (maxLum * maxLum))) / (1.0 + l);
   float3 c = sample * ld / sampleLum;
#else   
   // Same equation as above, but just split up into several discrete sections to understand it better.
   float l = middleGray / (adaptedLum + 0.00001f);
   sampleLum *= l;
   sample *= l;
         
   float3 c = sample;
   c += sample * (sampleLum / (maxLum * maxLum));
   c = c / (1.0 + sampleLum);
#endif   
      
   return c;
}

static float3 tonemapDistort(float2 tex0, float distortMag)
{
   // Linear, not point, sampling due to distortion effect.
   float sampleR = max(0, tex2D(gLinearSampler0, tex0 - distortMag * .4)).r;
   float sampleG = max(0, tex2D(gLinearSampler0, tex0)).g;
   float sampleB = max(0, tex2D(gLinearSampler0, tex0 + distortMag * .4)).b;
   
   float3 sample = float3(sampleR, sampleG, sampleB);
   
   return tonemap(sample);
}   

static float distort(inout float2 tex0, inout float2 tex1)
{
   float distortMag = 0.0;

   if (gDistortionEnabled)
   {
      float2 sample = tex2D(gLinearSampler4, tex0);      
      
      tex0 += sample;
      
      // FIXME: This is not correct because tex1 is scaled differently from tex0! 
      tex1 += sample;
      
      distortMag = length(sample);
   }
   
   return distortMag;
}

float4 gScreenToViewZParams : register(c1);
float4 gDOFParams : register(c2);

static float computeBlurFactor(float depth)
{
   float focalPlaneDist = gScreenToViewZParams.z;
   float nearPlaneDist = gScreenToViewZParams.w;
   float farPlaneDist = gDOFParams.x;
   float invFocalMinusNear = gDOFParams.y;
   float invFarMinusFocal = gDOFParams.z;
      
   // Compute depth blur
   float depthBlur;

   if (depth < focalPlaneDist)
     depthBlur = (focalPlaneDist - depth) * invFocalMinusNear;
   else
     depthBlur = (depth - focalPlaneDist) * invFarMinusFocal;
   
   depthBlur = clamp(depthBlur, 0.0f, 1.0f);
   return depthBlur;
}

float4 psToneMap(float2 tex0 : TEXCOORD0, float2 tex1 : TEXCOORD1, uniform bool enableDOF) : COLOR
{   
   float distortMag = distort(tex0, tex1);
   
   float2 radianceUV = tex0;
   if (gUnswizzleTexels)
   {
      float2 screenPos = floor(tex0 * gTextureWidthHeight.xy);
      float2 gridOfs = screenPos % 4.0;
      float2 gridCoord = floor(screenPos / 4.0);
      gridOfs = gUnswizzleGrid[gridOfs.y][gridOfs.x];
      radianceUV = (gridCoord * 4.0 + gridOfs) * gTextureInvWidthHeight + gTextureInvWidthHeight * .5f;
   }
                 
   float3 sample = max(0, tex2D(gPointSampler0, radianceUV));
         
   float3 blurredSample2;

   if (enableDOF)
   {
      float3 blurredSample1 = tex2D(gLinearSampler5, tex1);
      blurredSample2 = tex2D(gLinearSampler6, tex1);
      
      float depth = tex2D(gPointSampler7, tex0).x;
      float viewZ = 1.0 / (depth * gScreenToViewZParams.x + gScreenToViewZParams.y);
   
      float blurriness = computeBlurFactor(viewZ);
      float blurLerp = min(1.0f, blurriness * 2);
      
      float3 blurred = lerp(blurredSample1, blurredSample2, blurLerp);
      sample = lerp(sample, blurred, blurriness);
   }      
         
   // Screen blur
   if (gScreenBlurEnabled)
   {
      // Get blurred sample if DOF didn't already sample it
      if (!enableDOF)
         blurredSample2 = tex2D(gLinearSampler6, tex1);
   
      sample = lerp(sample, blurredSample2, gMiddleGreyMax.z);
   }
   
   float3 c = tonemap(sample);
   
   float3 bloomSample = tex2D(gLinearSampler2, tex1);
   
   c += bloomSample;
  
   c = saturate(c);
   
   // Color transformation - for desaturate effects
   if (gColorTransformEnabled)
   {
      float3 transformedColor = float3(dot(c, gRTransform), dot(c, gGTransform), dot(c, gBTransform));
      c = lerp(c, transformedColor, gMiddleGreyMax.y);
   }
   
   //c = (c - float3(.5f, .5f, .5f)) * 1.025f + float3(.5f, .5f, .5f);
   //c = saturate(c);
                            
   float ooGamma = 1.0f/2.3f;
   float eps = 1.0f / 4096.0f;

   c.r = pow(c.r + eps, ooGamma);
   c.g = pow(c.g + eps, ooGamma);
   c.b = pow(c.b + eps, ooGamma);

   return float4(c, 1);
}

// Bright Mask

float4 gBrightMaskParams : register(c1);

float4 psBrightMask(float2 tex0 : TEXCOORD0) : COLOR
{
   float3 sample = max(0, tex2D(gLinearSampler0, tex0));
   float3 c = tonemap(sample);
   
   float l = dot(c, float3(.213, .715, .072));
   
   float m = max(0, l - gBrightMaskParams.x) * gBrightMaskParams.y;
   
   return float4(c * m, 1);
}

// Reduct4

float4 gReduct4Add[4] : register(c0);

float4 psReduct4LogSum(float2 tex0 : TEXCOORD0) : COLOR
{
   float3 rgbToLum = float3(.213, .715, .072);
   float eps = 1.0f/16384.0f;
      
   float2 uv00 = tex0 + gReduct4Add[0];
   float2 uv01 = tex0 + gReduct4Add[1];
   float2 uv10 = tex0 + gReduct4Add[2];
   float2 uv11 = tex0 + gReduct4Add[3];
   
   float3 c0 = tex2D(gLinearSampler0, uv00);
   float3 c1 = tex2D(gLinearSampler0, uv01);
   float3 c2 = tex2D(gLinearSampler0, uv10);
   float3 c3 = tex2D(gLinearSampler0, uv11);
   
   float l0 = dot(c0, rgbToLum);
   float l1 = dot(c1, rgbToLum);
   float l2 = dot(c2, rgbToLum);
   float l3 = dot(c3, rgbToLum);
   
   float lSum = log(l0 + eps);
   lSum += log(l1 + eps);
   lSum += log(l2 + eps);
   lSum += log(l3 + eps);
   
   float lMax = max(max(max(l0, l1), l2), l3);
   
   return float4(lSum, lMax, 0, 1);
}

// Reduct4LinSum

float4 psReduct4LinSum(float2 tex0 : TEXCOORD0) : COLOR
{
   float2 uv00 = tex0 + gReduct4Add[0];
   float2 uv01 = tex0 + gReduct4Add[1];
   float2 uv10 = tex0 + gReduct4Add[2];
   float2 uv11 = tex0 + gReduct4Add[3];
   
   float3 c0 = tex2D(gLinearSampler0, uv00);
   float3 c1 = tex2D(gLinearSampler0, uv01);
   float3 c2 = tex2D(gLinearSampler0, uv10);
   float3 c3 = tex2D(gLinearSampler0, uv11);
   
   float lSum = 4.0f * (c0.r + c1.r + c2.r + c3.r);
   float lMax = max(max(max(c0.g, c1.g), c2.g), c3.g);
   
   return float4(lSum, lMax, 0, 1);
}

// Reduct4LinRGBAve

float4 psReduct4LinRGBAve(float2 tex0 : TEXCOORD0) : COLOR
{
   float2 uv00 = tex0 + gReduct4Add[0];
   float2 uv01 = tex0 + gReduct4Add[1];
   float2 uv10 = tex0 + gReduct4Add[2];
   float2 uv11 = tex0 + gReduct4Add[3];
   
   float3 c0 = max(0, tex2D(gLinearSampler0, uv00));
   float3 c1 = max(0, tex2D(gLinearSampler0, uv01));
   float3 c2 = max(0, tex2D(gLinearSampler0, uv10));
   float3 c3 = max(0, tex2D(gLinearSampler0, uv11));
   
   float3 lSum = .25f * (c0 + c1 + c2 + c3);
      
   return float4(lSum, 1);
}

// Reduct2LinRGBAve

float4 psReduct2LinRGBAve(float2 tex0 : TEXCOORD0) : COLOR
{
   return max(0, tex2D(gLinearSampler0, tex0));
}

// Reduct1

float4 gReduct1Mul : register(c0);
float4 gReduct1Add : register(c1);
float4 gReductAveMul : register(c2);
float4 gToneMapLimits : register(c3);

int gReduct1Width : register(i0);
int gReduct1Height : register(i1);

float4 psReduct1(void) : COLOR
{
   float lSum = 0;
   float lMax = 0;
      
   [loop]
   for (int y = 0; y < gReduct1Height; y++)
   {
      [loop]
      for (int x = 0; x < gReduct1Width; x++)
      {
         float4 uv = float4(x, y, 0, 1) * gReduct1Mul + gReduct1Add;
         
         float3 sample = 4.0f * tex2Dlod(gLinearSampler0, uv);
         
         lSum += sample.r;
         lMax = max(lMax, sample.g);
      }
   }
   
   lSum *= gReductAveMul.x;
   
   lSum = exp(lSum);
   
   lSum = clamp(lSum, gToneMapLimits.x, gToneMapLimits.y);
   lMax = clamp(lMax, gToneMapLimits.z, gToneMapLimits.w);
   
   return float4(lSum, lMax, 0, 1);
}

// Bloom horizontal filter
#define MAX_BLOOM_SAMPLES 32
#define BLOOM_TAP_SCALES_REG c32
float4 gBloomFilterTapSamples[MAX_BLOOM_SAMPLES] : register(ps, c0);
float4 gBloomFilterTapScales[MAX_BLOOM_SAMPLES]  : register(ps, BLOOM_TAP_SCALES_REG);
int gBloomNumSamples : register(i0);

float4 psBloomFilter(float2 InTexCoord0 : TEXCOORD0) : COLOR0   
{
   float3 sum = 0;
   
   // Sample 4 at a time - the GPU will increase aL by 4 each iteration.
   for (int i = 0; i < gBloomNumSamples; i++)
   { 
      float2 uv0 = InTexCoord0 + gBloomFilterTapSamples[i  ].xy;
      float2 uv1 = InTexCoord0 + gBloomFilterTapSamples[i+1].xy;
      float2 uv2 = InTexCoord0 + gBloomFilterTapSamples[i+2].xy;
      float2 uv3 = InTexCoord0 + gBloomFilterTapSamples[i+3].xy;
      
      float3 a = tex2D(gLinearSampler0, uv0);
      float3 b = tex2D(gLinearSampler0, uv1);
      float3 c = tex2D(gLinearSampler0, uv2);
      float3 d = tex2D(gLinearSampler0, uv3);
      
      a = gBloomFilterTapScales[i  ].x * a;
      b = gBloomFilterTapScales[i+1].x * b;
      c = gBloomFilterTapScales[i+2].x * c;
      d = gBloomFilterTapScales[i+3].x * d;
      
      sum += a + b + c + d;
   }
   
   return float4(sum, 1);	
}

// Filter adaptation

float4 gFilterAdaptationParams : register(c0);

float4 psFilterAdaptation(void) : COLOR0   
{
   float3 prev = tex2D(gPointSampler1, float2(.5f, .5f));
   float3 cur = tex2D(gPointSampler0, float2(.5f, .5f));

   float3 next = lerp(prev, cur, gFilterAdaptationParams.x);
         
   return float4(next, 1);
}

// Fill depth stencil surface
void psFillDepthStencilSurface(in  float2 tex0 : TEXCOORD0,
                               out float4 oColor: COLOR0,
                               out float  oDepth: DEPTH)
{
   float2 UV = tex0;
   
   if (gUnswizzleTexels)
   {      
      float2 screenPos = floor(tex0 * gTextureWidthHeight.xy);
      float2 gridOfs = screenPos % 4.0;      
      float2 gridCoord = floor(screenPos / 4.0);
      gridOfs = gUnswizzleGrid[gridOfs.y][gridOfs.x];            
      UV = (gridCoord * 4.0 + gridOfs) * gTextureInvWidthHeight + gTextureInvWidthHeight * .5f;
   }   

   float4 depth = tex2D(gPointSampler0, UV);   
   
   oColor = 0;
   oDepth = depth.x;
}

// Fill Color surface
void psFillColorSurface(in  float2 tex0  : TEXCOORD0,
                        out float4 oColor: COLOR0)
{
   float2 UV = tex0;
   
   if (gUnswizzleTexels)
   {      
      float2 screenPos = floor(tex0 * gTextureWidthHeight.xy);
      float2 gridOfs = screenPos % 4.0;      
      float2 gridCoord = floor(screenPos / 4.0);
      gridOfs = gUnswizzleGrid[gridOfs.y][gridOfs.x];            
      UV = (gridCoord * 4.0 + gridOfs) * gTextureInvWidthHeight + .5f * gTextureInvWidthHeight;
   }   
   
   oColor = tex2D(gPointSampler0, UV);
}


// 5v5 Gaussian blur
float4 gGaussBlurOffsets[16] : register(c0);
float4 gGaussBlurWeights[16] : register(c16);
float4 psGaussBlur5x5(in float2 tex0 : TEXCOORD0) : COLOR
{
    float4 vColor = 0.0f;

    for(int i = 0; i < 13; i++)
        vColor += gGaussBlurWeights[i] * tex2D(gPointSampler0, tex0 + gGaussBlurOffsets[i].xy );  
    return vColor;
}

// Radial blur
float4 gRadialBlur : register(c0);
float4 psRadialBlur(in float2 tex0 : TEXCOORD0) : COLOR
{
    float4 vColor = 0.0f;

    // Calculate pixel offset vec from blur pos
    float2 vec = float2(gRadialBlur.x, gRadialBlur.y) - tex0;
    
    // Scale and clamp
    float len = length(vec);
    len = clamp(len * gRadialBlur.z, 0.0f, gRadialBlur.w);
    vec = normalize(vec) * len;
    
    for(int i = 0; i < 13; i++)
    {
       float2 offset = vec * (i / 13.0f);// * gRadialBlur.z;
       //vColor += ((13.0f - i) / 91.0f) * tex2D(gPointSampler0, tex0 + offset);
       vColor += gGaussBlurWeights[i] * tex2D(gPointSampler0, tex0 + offset);
    }
    return vColor;
}

// Techniques

technique Tonemap
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex2();
      PixelShader = compile ps_3_0 psToneMap(false);

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique TonemapDOF
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex2();
      PixelShader = compile ps_3_0 psToneMap(true);

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique Reduct4LogSum
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psReduct4LogSum();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique Reduct4LinSum
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psReduct4LinSum();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique Reduct4LinRGBAve
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psReduct4LinRGBAve();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique Reduct2LinRGBAve
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psReduct2LinRGBAve();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique Reduct1
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psReduct1();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique BrightMask
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psBrightMask();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique BloomFilter
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psBloomFilter();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique FilterAdaptation
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psFilterAdaptation();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique FillDepthStencilSurface
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psFillDepthStencilSurface();

      ViewPortEnable = false;
      ZWriteEnable = true;
      ZFunc = always;
      ColorWriteEnable = 0;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique GaussBlur5x5
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psGaussBlur5x5();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique RadialBlur
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psRadialBlur();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}

technique FillColorSurface
{
   pass 
   {
      VertexShader = compile vs_3_0 vsFullScreenQuadTex1();
      PixelShader = compile ps_3_0 psFillColorSurface();

      ViewPortEnable = false;
      ZWriteEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = none;
   }
}