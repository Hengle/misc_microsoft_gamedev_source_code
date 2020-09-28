// File: terrainHeightField.fx

#define RENDERING_TERRAIN


#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"
#include "..\shared\pcf.inc"
#include "..\shared\dirLighting.inc"
#include "..\shared\localLighting.inc"
#include "..\shared\fogHelpers.inc"
#include "..\shared\shFillLighting.inc"
#include "..\shared\blackmap.inc"
#include "gpuTerrainBlackmap.inc"

#include "gpuTerrainshaderRegs.inc"


bool gLocalLightingEnabled	: register(ENABLE_LOCAL_LIGHTS_REG);
bool gLocalShadowingEnabled   : register(ENABLE_LOCAL_SHADOWING_REG);
int gNumLights					   : register(NUM_LOCAL_LIGHTS_REG);

bool gExtendedLocalLightingEnabled : register(ENABLE_EXTENDED_LOCAL_LIGHTS_REG);
int gNumExtendedLights        : register(NUM_EXTENDED_LOCAL_LIGHTS_REG);
float4 gExtendedLocalLightingParams : register(EXTENDED_LOCAL_LIGHTING_PARAMS_REG);

sampler gLightBufferColorSampler = sampler_state
{
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;   
   
   SeparateZFilterEnable = TRUE; 
};
sampler gLightBufferVectorSampler = sampler_state
{
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;   
   
   SeparateZFilterEnable = TRUE; 
};
bool gLightBufferingEnabled;
float4 gWorldToLightBufCols[3];

sampler gHeightfieldSampler = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
    
    SeparateZFilterEnable = FALSE;
    MinFilterZ = POINT;
    MagFilterZ = POINT;
};

// x - 1.0/numTrisPerRow
// y - numTrisPerRow
// z - 1.0/width
// w - 1.0/height
float4 gHeightfieldSampleParams;

float4x4 gHeightfieldToProj;

static void vsRenderDebugHeightfield( 
   in int Index : INDEX,
   out float4 OutPosition : POSITION,
   out float4 OutTex0 : TEXCOORD0 ) 
{
   float triIndex = floor((Index + .5) / 3.0);
   float vertIndex = floor(Index - triIndex * 3.0);
   
   float row = floor((triIndex + .5) * gHeightfieldSampleParams.x);
   float col = floor(triIndex - row * gHeightfieldSampleParams.y);
   
   float2 uv;

   if (frac(col * .5))
   {
      uv.x = (vertIndex == 1.0) ? 1.0 : 0.0;
      uv.y = (vertIndex == 2.0) ? 1.0 : 0.0;
   }
   else
   {
      uv.x = (vertIndex < 2.0) ? 1.0 : 0.0;
      uv.y = (vertIndex <  1.0) ? 0.0 : 1.0;
   }

   uv += float2(floor(col * .5), row);
   uv += float2(.5, .5);
   
   float2 normUV = uv * float2(gHeightfieldSampleParams.z, gHeightfieldSampleParams.w);
   
   float4 sample;
   
   asm
   {
      tfetch2D sample.xy, normUV, gHeightfieldSampler, UseComputedLOD = false
   };
   
   float lowHeight = sample.x;
   float highHeight = sample.y;
   
   OutPosition = mul(float4(uv.x-1, uv.y-1, highHeight, 1.0), gHeightfieldToProj);

   OutTex0 = float4(normUV.x, normUV.y, 0, 1);
}

float4 psRenderDebugHeightField(in float2 Tex0 : TEXCOORD0) : COLOR0
{
   return float4(Tex0.x, Tex0.y, 0.0, 1);
}

technique RenderDebugHeightField
{
    pass
    {
		VertexShader = compile vs_3_0 vsRenderDebugHeightfield();
		PixelShader  = compile ps_3_0 psRenderDebugHeightField();
		
      ZWriteEnable = false;
      AlphaBlendEnable = false;
      SrcBlend = one;
      DestBlend = zero;
      AlphaTestEnable = false;
      CullMode = none;
    } 
}

static void vsRenderHeightfieldForOcclusion( 
   in int Index : INDEX,
   out float4 OutPosition : POSITION,
   out float2 outDepth : TEXCOORD0 ) 
{
   float triIndex = floor((Index + .5) / 3.0);
   float vertIndex = floor(Index - triIndex * 3.0);
   
   float row = floor((triIndex + .5) * gHeightfieldSampleParams.x);
   float col = floor(triIndex - row * gHeightfieldSampleParams.y);
   
   float2 uv;

   if (frac(col * .5))
   {
      uv.x = (vertIndex == 1.0) ? 1.0 : 0.0;
      uv.y = (vertIndex == 2.0) ? 1.0 : 0.0;
   }
   else
   {
      uv.x = (vertIndex < 2.0) ? 1.0 : 0.0;
      uv.y = (vertIndex <  1.0) ? 0.0 : 1.0;
   }

   uv += float2(floor(col * .5), row);
   uv += float2(.5, .5);
   
   float2 normUV = uv * float2(gHeightfieldSampleParams.z, gHeightfieldSampleParams.w);
   
   float4 sample;
   
   asm
   {
      tfetch2D sample.xy, normUV, gHeightfieldSampler, UseComputedLOD = false
   };
   
   float lowHeight = sample.x;
   float highHeight = sample.y;
   
   OutPosition = mul(float4(uv.x-1, uv.y-1, highHeight, 1.0), gHeightfieldToProj);
   outDepth = OutPosition.zw;
}

float4 psRenderHeightFieldForOcclusion(in float2 inDepth : TEXCOORD0) : COLOR0
{
   return inDepth.x/ inDepth.y;
}

technique RenderHeightFieldForOcclusion
{
    pass
    {
		VertexShader = compile vs_3_0 vsRenderHeightfieldForOcclusion();
		PixelShader  = compile ps_3_0 psRenderHeightFieldForOcclusion();
		
      ZWriteEnable = true;
      ZEnable = true;
      AlphaBlendEnable = false;
      SrcBlend = one;
      DestBlend = zero;
      AlphaTestEnable = false;
      CullMode = none;
    } 
}


static float4 calcWeights(float2 uv, int quadID, out bool invertNormals, out float2 outUV)
{
    // r.x = (1.0 - u) * (1.0 - v);
    // r.y = u * (1.0 - v);
    // r.z = (1.0 - u) * v;
    // r.w = u * v;
    
    float u = uv.x;
    float v = uv.y;
    float4 r = float4((1.0 - u) * (1.0 - v), u * (1.0 - v), (1.0 - u) * v, u * v);

    if(quadID == 1)         // 1 Swap in x (note: invert normals)
    {   
        invertNormals = true;
        r = r.yxwz; 
        outUV = float2(1.0 - uv.x, uv.y);
    }
    else if(quadID == 2)    // 2 Swap in x and y
    {
        invertNormals = false;
        r = r.wzyx;
        outUV = float2(1.0 - uv.x, 1.0 - uv.y);
    }
    else if(quadID == 3)    // 3 Swap in y (note: invert normals)
    {
        invertNormals = true;
        r = r.zwxy;
        outUV = float2(uv.x, 1.0 - uv.y);
    }
    else    // 0 no swap
    {
        invertNormals = false;
        r = r;
        outUV = uv;
    }
    return r;
}

static float4 interpolate(float4 a, float4 b, float4 c, float4 d, float4 weights)
{
    return a * weights.w + b * weights.z + c * weights.y + d * weights.x;
}

static float3 interpolate(float3 a, float3 b, float3 c, float3 d, float4 weights)
{
    return a * weights.w + b * weights.z + c * weights.y + d * weights.x;
}

static float2 interpolate(float2 a, float2 b, float2 c, float2 d, float4 weights)
{
    return a * weights.w + b * weights.z + c * weights.y + d * weights.x;
}

float4x4 gWorldToHeightfield;
// x - yScale
// y - yOfs
// z - yLowLimit
// w - yHighLimit
float4 gHeightfieldYScaleOfs;

bool gConformToTerrainFlag;

static float2 heightfieldDepthToY(float2 s)
{
   return s * gHeightfieldYScaleOfs.x + float2(gHeightfieldYScaleOfs.y, gHeightfieldYScaleOfs.y);
}

static void vsRenderPatches(
   in    float2   inUV     : BARYCENTRIC,
   in    int      inQuadID : QUADID,
   in    int      inIndex  : INDEX,
   out   float4   outPos   : POSITION,
   out   float3   outUV    : TEXCOORD0,
   out   float4   outTexUV : TEXCOORD1,
   out   float4   outColor : COLOR0)
{
   float4 instancePos;
   float4 instanceAttribs1;
   float4 instanceAttribs2;
   float4 instanceColor;
   float4 instanceTexUV;
     
   asm 
   {
     vfetch instancePos,     inIndex, position
     vfetch instanceAttribs1, inIndex, texcoord0
     vfetch instanceAttribs2, inIndex, texcoord1
     vfetch instanceTexUV, inIndex, texcoord2
     vfetch instanceColor, inIndex, color0
   };

   float3 forward = float3(instanceAttribs1.x, instanceAttribs1.y, instanceAttribs1.z);
   float3 right = float3(instanceAttribs1.w, instanceAttribs2.x, instanceAttribs2.y); 
   float yOffset = instanceAttribs2.z;
   float intensity = instanceAttribs2.w;
            
   bool invertNormals;
   float2 fixedUV;
   float4 weights = calcWeights(inUV, inQuadID, invertNormals, fixedUV);
          
   float3 p0 = -right + -forward;
   float3 p1 =  right + -forward;
   float3 p3 =  right + forward;
   float3 p2 = -right + forward;

   float3 p = interpolate(p0, p1, p2, p3, weights);
   float4 worldPos = float4(p, 0.0) + float4(instancePos.x, instancePos.y, instancePos.z, 1.0);
         
   if (gConformToTerrainFlag)
   {
      float4 centerHeightfieldPos = mul(float4(instancePos.x, instancePos.y, instancePos.z, 1.0), gWorldToHeightfield);
      float4 heightfieldPos = mul(worldPos, gWorldToHeightfield);
   
      float4 centerSample;
      float4 sample;
      asm
      {
         tfetch2D centerSample.xy, centerHeightfieldPos, gHeightfieldSampler, UseComputedLOD = false, UnnormalizedTextureCoords = true   
         tfetch2D sample.xy, heightfieldPos, gHeightfieldSampler, UseComputedLOD = false, UnnormalizedTextureCoords = true
      };
      
      float2 centerLoHiY = heightfieldDepthToY(centerSample);
      float2 sampleLoHiY = heightfieldDepthToY(sample);
      
      float centerY = centerLoHiY.x;
      if (abs(instancePos.y - centerLoHiY.y) < abs(instancePos.y - centerY))
         centerY = centerLoHiY.y;
               
      float sampleY = sampleLoHiY.x;
      if (abs(centerY - sampleLoHiY.y) < abs(centerY - sampleLoHiY.x))
         sampleY = sampleLoHiY.y;
         
      if (sampleY < (centerY - gHeightfieldYScaleOfs.z))
         sampleY = centerY - gHeightfieldYScaleOfs.z;
      else if (sampleY > (centerY + gHeightfieldYScaleOfs.z))
         sampleY = centerY + gHeightfieldYScaleOfs.z;
      
      worldPos.y = sampleY;
   }
   
   worldPos.y += yOffset;
   
   outPos   = mul(worldPos, gWorldToProj);
   outColor.xyz = srgbToLinear(instanceColor) * intensity;
   outColor.w = instanceColor.w;
   outUV    = float3(fixedUV.x, fixedUV.y, inIndex);
   outTexUV = instanceTexUV;
}

sampler gSampler0 : register(s0);

static float4 psRenderPatches(
   in   float3   inUV    : TEXCOORD0,
   in   float4   inTex0  : TEXCOORD1,
   in   float4   inColor : COLOR0) : COLOR0
{
   //return float4(inUV.x, inUV.y, 0, 1);

   float2 uv;
   uv.x = inTex0.x + (inUV.x*inTex0.z);
   uv.y = inTex0.y + (inUV.y*inTex0.w);
   
   float4 t = srgbToLinear(tex2D(gSampler0, uv));
   
   t.xyz *= inColor;
   t *= inColor.a;
   
   return t;
}   


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////



sampler gDiffuseSampler : register(s0);
sampler gNormalSampler : register(s1);
sampler gOpacitySampler : register(s2);
sampler gSpecularSampler : register(s3);

sampler gTerrainAlphaSampler = sampler_state
{
    MipFilter = NONE;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};


float giveDynamicAlphaValue( float2 texCoords )
{
	float    g_dynamicAlphaTexWidth = gHeightfieldSampleParams.x;  

     float imgWidth = g_dynamicAlphaTexWidth-1;
     
     int2 floord = (texCoords * float2(imgWidth,imgWidth)) * float2(0.25f,1);   //floord should be in [0,imgWidth>>2]x[0,imgHeight] space
     
     float2 scaled = floord / float2(imgWidth * 0.25f,imgWidth);
     
     float4 pixel= tex2Dlod( gTerrainAlphaSampler, float4(scaled.x, scaled.y ,0,0) );       // Output color
     
     int xInd = (texCoords * imgWidth)%4;  
     float res =0;
     if(xInd ==0) res = pixel.x;
     if(xInd ==1) res = pixel.y;
     if(xInd ==2) res = pixel.z;
     if(xInd ==3) res = pixel.w;
     
     return res;
 }      
 
 
static void vsRenderPatchesLit(
   in    float2   inUV     : BARYCENTRIC,
   in    int      inQuadID : QUADID,
   in    int      inIndex  : INDEX,
   out   float4   outPos   : POSITION,
   
   out	float3 vertTangent				: TANGENT,
   out	float3 vertBinormal				: BINORMAL,
   out	float3 vertNormal				: NORMAL0, 
    
   out   float3   outUV    : TEXCOORD0,
   out   float4   outTexUV : TEXCOORD1,
    
    out	float3 shadowMapCoords0			: TEXCOORD2,
    out	float3 vertWorldPos				: TEXCOORD5,
    out	float4 ao_alpha					: TEXCOORD6, // ZW contains Z/Planar fog density
    out   float4   outColor : COLOR0)
    
    
    
    
    
    
    
    
{
   float4 instancePos;
   float4 instanceAttribs1;
   float4 instanceAttribs2;
   float4 instanceColor;
   float4 instanceTexUV;
     
   asm 
   {
     vfetch instancePos,     inIndex, position
     vfetch instanceAttribs1, inIndex, texcoord0
     vfetch instanceAttribs2, inIndex, texcoord1
     vfetch instanceTexUV, inIndex, texcoord2
     vfetch instanceColor, inIndex, color0
   };

   float3 forward = float3(instanceAttribs1.x, instanceAttribs1.y, instanceAttribs1.z);
   float3 right = float3(instanceAttribs1.w, instanceAttribs2.x, instanceAttribs2.y); 
   float yOffset = instanceAttribs2.z;
   float intensity = instanceAttribs2.w;
            
   bool invertNormals;
   float2 fixedUV;
   float4 weights = calcWeights(inUV, inQuadID, invertNormals, fixedUV);
          
   float3 p0 = -right + -forward;
   float3 p1 =  right + -forward;
   float3 p3 =  right + forward;
   float3 p2 = -right + forward;

   float3 p = interpolate(p0, p1, p2, p3, weights);
   float4 worldPos = float4(p, 0.0) + float4(instancePos.x, instancePos.y, instancePos.z, 1.0);
   float worldAlpha = 1.0f;
	
   if (gConformToTerrainFlag)
   {
      float4 centerHeightfieldPos = mul(float4(instancePos.x, instancePos.y, instancePos.z, 1.0), gWorldToHeightfield);
      float4 heightfieldPos = mul(worldPos, gWorldToHeightfield);
      float2 alphaPos = heightfieldPos.xy * gHeightfieldSampleParams.zw;
	  worldAlpha = giveDynamicAlphaValue(alphaPos);
   
      float4 centerSample;
      float4 sample;
      asm
      {
         tfetch2D centerSample.xy, centerHeightfieldPos, gHeightfieldSampler, UseComputedLOD = false, UnnormalizedTextureCoords = true   
         tfetch2D sample.xy, heightfieldPos, gHeightfieldSampler, UseComputedLOD = false, UnnormalizedTextureCoords = true
      };
      
      float2 centerLoHiY = heightfieldDepthToY(centerSample);
      float2 sampleLoHiY = heightfieldDepthToY(sample);
      
      float centerY = centerLoHiY.x;
      if (abs(instancePos.y - centerLoHiY.y) < abs(instancePos.y - centerY))
         centerY = centerLoHiY.y;
               
      float sampleY = sampleLoHiY.x;
      if (abs(centerY - sampleLoHiY.y) < abs(centerY - sampleLoHiY.x))
         sampleY = sampleLoHiY.y;
         
         //CLM this causes large patches to be clipped to height restrictions
         //Is it still needed?
    //  if (sampleY < (centerY - gHeightfieldYScaleOfs.z))
    //     sampleY = centerY - gHeightfieldYScaleOfs.z;
    //  else if (sampleY > (centerY + gHeightfieldYScaleOfs.z))
    //     sampleY = centerY + gHeightfieldYScaleOfs.z;
      
      worldPos.y = sampleY;
  }
   
   worldPos.y += yOffset;
   
   outPos   = mul(worldPos, gWorldToProj);
   outColor.xyz = srgbToLinear(instanceColor) * intensity;
   outColor.w = instanceColor.w * worldAlpha;
   outUV    = float3(fixedUV.x, fixedUV.y, inIndex);
   outTexUV = instanceTexUV;
   
	vertNormal = float3(0,1,0);
	vertBinormal = normalize(vertNormal.xzy * float3(0, 1, -1));
    vertTangent = cross(vertBinormal, vertNormal);
   
    shadowMapCoords0 = mul( float4(worldPos.x,worldPos.y,worldPos.z,1.f ), gDirShadowWorldToTex);
	vertWorldPos = worldPos;
   
	ao_alpha = float4(1,1,computeRadialFogDensity(worldPos),computePlanarFogDensity(worldPos));	
}



static void vsRenderRibbonLit(
	in    float4 Position   : POSITION0,
    in    float2 UV         : TEXCOORD0,
	in    float1 alpha     : TEXCOORD1,
   
	out   float4   outPos   : POSITION,
	out	float3 vertTangent				: TANGENT,
	out	float3 vertBinormal				: BINORMAL,
	out	float3 vertNormal				: NORMAL0, 
    
	out   float3   outUV    : TEXCOORD0,
	out   float4   outTexUV : TEXCOORD1,
    
    out	float3 shadowMapCoords0			: TEXCOORD2,
    out	float3 vertWorldPos				: TEXCOORD5,
    out	float4 ao_alpha					: TEXCOORD6, // ZW contains Z/Planar fog density
    out   float4   outColor : COLOR0
  )
 
{
   float4 worldPos = Position;//mul( Position, gWorldToProj);
   
   // Terrain Conform
   float worldAlpha = 1.0f;
   {
      float4 centerHeightfieldPos = mul(worldPos, gWorldToHeightfield);
      float4 heightfieldPos =		mul(worldPos, gWorldToHeightfield);
      float2 alphaPos = heightfieldPos.xy * gHeightfieldSampleParams.zw;
	  worldAlpha = giveDynamicAlphaValue(alphaPos);
      
      float4 centerSample;
      float4 sample;
      asm
      {
         tfetch2D centerSample.xy, centerHeightfieldPos, gHeightfieldSampler, UseComputedLOD = false, UnnormalizedTextureCoords = true   
      };
      
      float2 centerLoHiY = heightfieldDepthToY(centerSample);


      float yOffset = 0;//gHeightfieldYScaleOfs.w;
      worldPos.y = centerLoHiY.y;//sampleY + yOffset;
   }
   
   
   
   //POSITION
	{
		outPos = mul( worldPos, gWorldToProj);
		vertWorldPos = worldPos;  
	}
   
   //TBN
	{
		vertNormal = float3(0,1,0);
		vertBinormal = normalize(vertNormal.xzy * float3(0, 1, -1));
		vertTangent = cross(vertBinormal, vertNormal);
    }
    
    //UVs
    {
		outUV = float3(0,0,0);
		outTexUV = float4(UV.x,1-UV.y,1,1);
    }
    
    //LIGHTING
    {
		shadowMapCoords0 = mul( float4(worldPos.x,worldPos.y,worldPos.z,1.f ), gDirShadowWorldToTex);
		ao_alpha = float4(1,1,computeRadialFogDensity(worldPos),computePlanarFogDensity(worldPos));	
    }
    
    //COLOR
    {
		float intensity = 1.0f;
		outColor.xyz = float3(1,1,1);
		outColor.w = alpha * worldAlpha;
	}
    
}


float gSpecPower = 25.0f;
static float gSpecToDiffuseRatio = 3.14f; // The artists should not be able to control the specular to diffuse ratio (the actual ratio is Pi).

static void localLighting(float3 worldPos, float3 worldNormal, float3 worldReflect, float specPower, out float3 diffuseSum, out float3 specSum)
{
   diffuseSum = 0.0;
   specSum = 0.0;
   
 //  if (gLocalShadowingEnabled)
   {   
  //    for (int i = 0; i < gNumLights; i++)
  //       omniIlluminateShadowed(i, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }
  // else
   {
      for (int i = 0; i < gNumLights; i++)
	     omniIlluminate(i, worldNormal, worldReflect, worldPos, specPower, diffuseSum, specSum);   
   }        
} 
	
static void computeLighting(
	float2 uv0,
	float3 shadowMapCoords0,
	float3 texnormal,
	
	float3 vertWorldPos,
	float3 vertNormal,
	float3 vertTangent,
	float3 vertBinormal,
	
   
	uniform bool dirShadowingEnabled,
	uniform bool localLightingEnabled,
	
	out float3 worldNormal,
	out float3 worldReflect,
	out float3 ambientSum,
	out float3 diffuseSum,
	out float3 specSum)
{
	diffuseSum = 0;
	specSum=0;
	
	worldNormal = normalize(texnormal.x * vertTangent + texnormal.y * vertBinormal + texnormal.z * vertNormal);
	   
	worldReflect = reflect(normalize(vertWorldPos - gWorldCameraPos), worldNormal);
	

   float nDotL;
   computeDirectionalLighting(worldNormal, worldReflect, gDirLightVecToLightWorld, gSpecPower, nDotL, diffuseSum, specSum);

	
	if ((dirShadowingEnabled) && (nDotL > 0.0f))
   {
      float3 shadowLevelColor;
      float shadowFactor = calcDirShadowFactor(uv0, shadowMapCoords0, shadowLevelColor);   

      diffuseSum *= shadowFactor;
      specSum *= shadowFactor;
   }
   
	ambientSum = computeSHFillLighting(worldNormal);
	
	
	if (gLocalLightingEnabled)
   {
      float3 localDiffuseSum, localSpecSum;
      localLighting(vertWorldPos, worldNormal, worldReflect, gSpecPower, localDiffuseSum, localSpecSum);
      
      diffuseSum += localDiffuseSum;
      specSum += localSpecSum;
   } 
   
   if (gLightBufferingEnabled)
   {
      float4 worldPos = float4(vertWorldPos.x, vertWorldPos.y, vertWorldPos.z, 1.0f);
      
      float3 lightBufUVW;
      lightBufUVW.x = dot(worldPos, gWorldToLightBufCols[0]);
      lightBufUVW.y = dot(worldPos, gWorldToLightBufCols[1]);
      lightBufUVW.z = dot(worldPos, gWorldToLightBufCols[2]);
                  
      float3 lightColor = tex3D(gLightBufferColorSampler, lightBufUVW) * 12.0f;
      float3 lightNorm = tryNormalize(tex3D(gLightBufferVectorSampler, lightBufUVW) - float3(.5f, .5f, .5f));
      
      float3 diffuseContrib = saturate(dot(lightNorm, worldNormal)) * lightColor;
      diffuseSum += diffuseContrib;
   }
}



static float4 psRenderPatchesLit(

    in	float3 vertTangent				: TANGENT,
    in	float3 vertBinormal				: BINORMAL,
    in	float3 vertNormal				: NORMAL0,   
    in	float3 inUV						: TEXCOORD0,
    in	float4 inTex0					: TEXCOORD1,
    in	float3 shadowMapCoords0			: TEXCOORD2,
    in	float3 vertWorldPos				: TEXCOORD5,
    in	float4 ao_alpha					: TEXCOORD6,// ZW contains Z/Planar fog density
    in  float4 inColor					: COLOR0             

   ) : COLOR0
{

   float2 uv;
   uv.x = inTex0.x + (inUV.x*inTex0.z);
   uv.y = inTex0.y + (inUV.y*inTex0.w);
   
   float3 albedo = srgbToLinear(tex2D(gDiffuseSampler, uv));
   float3 specular = tex2D(gSpecularSampler, uv);
   float opacity = tex2D(gOpacitySampler, uv);
	float3 texnormal = unpackDXNNormal(tex2D(gNormalSampler,uv));
   
   float3 worldNormal;
   float3 worldReflect;
   float3 ambientSum;
   float3 diffuseSum;
   float3 specSum;
   
   computeLighting(uv,shadowMapCoords0,texnormal,
   vertWorldPos,vertNormal,vertTangent,vertBinormal,
   true,true, 
   worldNormal, worldReflect, ambientSum, diffuseSum, specSum);
   
   diffuseSum += ambientSum;
       
   float3 result = diffuseSum *  albedo;
   specSum *= gSpecToDiffuseRatio;
   result += specSum * specular;

   
   result = computeBlackmap(result, vertWorldPos);
   result = computeFog(result, ao_alpha.z, ao_alpha.w);

   return float4(result, inColor.a * opacity); 

}   


#define VIS_CONTROL_0_REG              c196
#define VIS_CONTROL_1_REG              c197
#define VIS_CONTROL_2_REG              c198
#define VIS_CONTROL_3_REG              c199

float4 gVisControl0 : register(VIS_CONTROL_0_REG);
float4 gVisControl1 : register(VIS_CONTROL_1_REG);
float4 gVisControl2 : register(VIS_CONTROL_2_REG);
float4 gVisControl3 : register(VIS_CONTROL_3_REG);



static float4 psRenderPatchesLitVis(

    in	float3 vertTangent				: TANGENT,
    in	float3 vertBinormal				: BINORMAL,
    in	float3 vertNormal				: NORMAL0,   
    in	float3 inUV						: TEXCOORD0,
    in	float4 inTex0					: TEXCOORD1,
    in	float3 shadowMapCoords0			: TEXCOORD2,
    in	float3 vertWorldPos				: TEXCOORD5,
    in	float4 ao_alpha					: TEXCOORD6,// ZW contains Z/Planar fog density
    in  float4 inColor					: COLOR0             

   ) : COLOR0
{

   float2 uv;
   uv.x = inTex0.x + (inUV.x*inTex0.z);
   uv.y = inTex0.y + (inUV.y*inTex0.w);
   
   float3 albedo = srgbToLinear(tex2D(gDiffuseSampler, uv));
   float3 specular = tex2D(gSpecularSampler, uv);
   float opacity = tex2D(gOpacitySampler, uv);
   float3 texnormal = unpackDXNNormal(tex2D(gNormalSampler,uv));
   
   float3 worldNormal;
   float3 worldReflect;
   float3 ambientSum;
   float3 diffuseSum;
   float3 specSum;
   
   computeLighting(uv,shadowMapCoords0,texnormal,
   vertWorldPos,vertNormal,vertTangent,vertBinormal,
   true,true, 
   worldNormal, worldReflect, ambientSum, diffuseSum, specSum);
   

   float4 selectA = gVisControl0;
   float4 selectB = gVisControl1;
   float4 selectC = gVisControl2;
   float4 selectD = gVisControl3;
         
   float3 result = 
      selectA.x * albedo +
      selectA.y * float3(1,1,1) +
      selectA.z * 0 +
      selectA.w * 0 + 
      
      selectB.x * 0 +
      selectB.y * 0 +
      selectB.z * 0 +
      selectB.w * specular +
      
      selectC.x * opacity +
      selectC.y * ambientSum +
      selectC.z * diffuseSum +
      selectC.w * specSum +
      
      
      selectD.x * (.5 + .5 * worldNormal) +
      selectD.y * (.5 + .5 * texnormal) +
      selectD.z * 0 +
      selectD.w * gSpecPower;

   return float4(result, 1);
   
}   




technique RenderPatches
{
   pass UnlitDecalPatch
   {
      VertexShader = compile vs_3_0 vsRenderPatches();
		PixelShader  = compile ps_3_0 psRenderPatches();
   }
   
   pass LitDecalPatch
   {
		VertexShader = compile vs_3_0 vsRenderPatchesLit();
		PixelShader  = compile ps_3_0 psRenderPatchesLit();
   }
   
   pass VISDecalPatch
    {
		VertexShader = compile vs_3_0 vsRenderPatchesLit();
		PixelShader  = compile ps_3_0 psRenderPatchesLitVis();
    }
    
   pass LitRibbon
   {
		VertexShader = compile vs_3_0 vsRenderRibbonLit();
		PixelShader  = compile ps_3_0 psRenderPatchesLit();
   }
   
   //============VISUALIZATION
    pass VISRibbon
    {
		VertexShader = compile vs_3_0 vsRenderRibbonLit();
		PixelShader  = compile ps_3_0 psRenderPatchesLitVis();
    } 
}