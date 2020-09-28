#include "..\shared\intrinsics.inc"

#include "..\shared\helpers.inc"
#include "particlehelpers.inc"

//----------------------------------------------------------------------------
// Samplers
//----------------------------------------------------------------------------


//============================================================================
// Pixel Shader Samplers
//============================================================================

//-- Diffuse Texture
texture diffuseTextureArray;
sampler gDiffuseArraySampler : register(s0) = sampler_state
{
   Texture    = (diffuseTextureArray);

   AddressU   = WRAP;
   AddressV   = WRAP;
   AddressW   = WRAP;
   
   MipFilter  = LINEAR;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;  
   MinMipLevel= 2; 
   
   SeparateZFilterEnable = TRUE; 
   MinFilterZ = POINT;
   MagFilterZ = POINT;
};

texture diffuse2TextureArray;
sampler gDiffuseArraySampler2 : register(s1) = sampler_state
{
   Texture    = (diffuse2TextureArray);

   AddressU   = WRAP;
   AddressV   = WRAP;
   AddressW   = WRAP;
   
   MipFilter  = LINEAR;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;  
   MinMipLevel= 2; 
   
   SeparateZFilterEnable = TRUE; 
   MinFilterZ = POINT;
   MagFilterZ = POINT;
};

texture diffuse3TextureArray;
sampler gDiffuseArraySampler3 : register(s2) = sampler_state
{
   Texture    = (diffuse3TextureArray);

   AddressU   = WRAP;
   AddressV   = WRAP;
   AddressW   = WRAP;
   
   MipFilter  = LINEAR;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;  
   MinMipLevel= 2; 
   
   SeparateZFilterEnable = TRUE; 
   MinFilterZ = POINT;
   MagFilterZ = POINT;
};

texture intensityTextureArray;
sampler gIntensityArraySampler : register(s3) = sampler_state
{
   Texture    = (intensityTextureArray);

   AddressU   = WRAP;
   AddressV   = WRAP;
   AddressW   = WRAP;
   
   MipFilter  = LINEAR;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;  
   MinMipLevel= 2; 
   
   SeparateZFilterEnable = TRUE; 
   MinFilterZ = POINT;
   MagFilterZ = POINT;
};

sampler gDepthTextureSampler : register(s4) = sampler_state
{
    MinFilter = POINT;
    MagFilter = POINT;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler gLightBufferSampler : register(s5) = sampler_state
{
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;   
   
   SeparateZFilterEnable = TRUE; 
};

texture randomTexture;
sampler gRandomTextureSampler : register(s6) = sampler_state
{
   Texture    = (randomTexture);
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = POINT;
   MagFilter  = POINT;     
};

//============================================================================
// Vertex Shader Samplers
//============================================================================
texture colorProgressionTexture;
sampler gColorProgressionSampler : register(s0) = sampler_state
{
   Texture    = (colorProgressionTexture);
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = POINT;
   MagFilter  = POINT;     
};

texture scaleProgressionTexture;
sampler gScaleProgressionSampler : register(s1) = sampler_state
{
   Texture    = (scaleProgressionTexture);
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = POINT;
   MagFilter  = POINT;     
};

texture intensityProgressionTexture;
sampler gIntensityProgressionSampler : register(s2) = sampler_state
{
   Texture    = (intensityProgressionTexture);
   AddressU   = CLAMP;
   AddressV   = CLAMP;
   AddressW   = CLAMP;
   
   MipFilter  = NONE;
   MinFilter  = POINT;
   MagFilter  = POINT;     
};

sampler gHeightfieldSampler : register(s3) = sampler_state
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

bool gLightBufferingEnabled;
float gLightBufferIntensityScale;
float4 gWorldToLightBufCols[3];
 
sampler gOffsetSampler;

float gZOffsetScale;


//-- parameters
float4 gTextureAnimationData; // x = UV Frame Width 
                              // y = UV FrameHeight;
                              // z = Texture Frames Per Row
                              // w = Texture Num Frames
float4 gTimeData;             // x = current Time
                              // y = Animated Texture Frame Per Second Time   
                                                          
float4x4 gEmitterMatrix;
float4x4 gOrientMatrix;

float4   gBeamForward;
float4   gBeamProgressionAlphaSelector = float4(0,0,0,0);

float4x4 gLastUpdateEmitterMatrix;
float  gEmitterAttraction;
float4 gProgressionTextureV;
float  gEmitterOpacity;
                             
float4 gTextureStageCounts;
bool   gTextureAnimationToggle;
bool   gIntensityMaskToggle;


bool   gMultiTextureEnable3Layers = false;
float4  gMultiTextureBlendMultiplier;
float4  gMultiTextureBlendMultiplier2;

float4 gPalletteColor[8] : register(c0);
float  gPalletteColorCount;
bool   gPalletteColorToggle;

//-- UV Animation
float4 gUVVelocity0;
float4 gUVVelocity1;
float4 gUVRandomOffsetSelector0;
float4 gUVRandomOffsetSelector1;

//-- Vertex Color
float4 gVertex1Color;
float4 gVertex2Color;
float4 gVertex3Color;
float4 gVertex4Color;
float4 gTintColor;


//-- Terrain Patch variables
float4x4 gWorldToHeightfield;
// x - yScale
// y - yOfs
// z - yLowLimit
// w - yHighLimit
float4 gHeightfieldYScaleOfs;
float4 gTerrainDecalYOffset;

float4 gSoftParams;
float4 gSoftParams2;

//----------------------------------------------------------------------------
// Helper Functions
//----------------------------------------------------------------------------
void ComputeZDepth(in float zDepth, in float2 zOffsetTextureUV, out float finalViewDepth)
{
   //-- look up our zOffset Texture Sample
   //-- bring our texture sample into -0.5f to 0.5f range
   float4 zOffsetTextureSample = tex2D(gOffsetSampler, zOffsetTextureUV) - 0.5f;

   //-- View depth
   //-- V = Z + (TextureSample * zOffsetScale)
   float viewDepth = max(0, zDepth + (-zOffsetTextureSample.x * gZOffsetScale));
   //-- outdepth
   //-- oDepth = (V * Q + R) / V
   //-- where Q = matrix._m22
   
   //-- where R = matrix._m32	
   finalViewDepth = max(0, ((viewDepth * gProjToScreen._m22) + gProjToScreen._m32) / viewDepth);
}

// The corner_vectors constant data is used to build the four corners of a quad.
const float2 cCorner[4]              = {float2(-1,  1), float2( -1,-1), float2(1 ,-1), float2( 1, 1)};
const float2 cUpCorner[4]            = {float2(-1,  1), float2( -1,-1), float2(1 , 1), float2( 1, -1)};
const float2 cCornerUV[4]            = {float2( 0,  0), float2(  0, 1), float2(1 , 1), float2( 1, 0)};
const float2 cStretchCornerUV[4]     = {float2( 0,  0), float2(  0, 1), float2(0 , 1), float2( 0, 0)};
const float2 cTrailCorner[4] = {float2(-1, 1), float2(  1,1), float2(1, -1),  float2(-1, -1)};
const float  cTrailCornerMultiplier[4] = {-1,1,1,-1};
const float  cTrailCrossCornerMultiplier[4] = {1,-1,-1, 1};
const float4 cCornerColor[4] = {float4(1,0,0,1), float4(0,1,0,1), float4(0,0,1,1), float4(1,1,1,1)};


const float  cVertexPattern[4] = {0,0,1,1};
const float2 cCorner8[8] = {float2(-1,0), float2(-1,-1), float2(1,-1), float2(1,0), 
                            float2(-1,1), float2(-1,0), float2(1,0), float2(1,1)};

const float2 cCornerUV8[8] = {float2(0,0.5), float2(0,0), float2(1,0), float2(1,0.5),
                              float2(0,1), float2(0,0.5), float2(1,0.5), float2(1,1)};

const float2 cStretchCornerUV8[8] = {float2(0,0.5), float2(0,1), float2(0,1), float2(0,0.5),
                                     float2(0,0), float2(0,0.5), float2(0,0.5), float2(0,0)};

const float cTrailCornerMultiplier8[8] = {0,1,1,0,-1,0,0,-1};
const float cTrailCrossCornerMultiplier8[8] = {0,1,1,0,-1,0,0,-1};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchPosition(int index)
{   
   float4 pos;
   asm
   {
      vfetch pos, index, position0
   };   
   return pos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord0(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord0;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord1(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord1;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord2(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord2;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchTexCoord3(int index)
{
   float4 texCoord;
   asm
   {
      vfetch texCoord, index, texcoord3;
   };
   return texCoord;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchColor0(int index)
{
   float4 color;
   asm
   {
      vfetch color, index, color0;
   };
   return color;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchColor1(int index)
{
   float4 color;
   asm
   {
      vfetch color, index, color1;
   };
   return color;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 fetchVelocity(int index)
{
   float4 velocity;
   asm
   {
      vfetch velocity, index, texcoord2;
   };
   return velocity;
}

//----------------------------------------------------------------------------
// vertIndex should be the index passed in by the vertex shader
//----------------------------------------------------------------------------
int getCornerIndex(int vertIndex)
{
   int iDiv = (vertIndex+0.5f) / 4;  // add 0.5f to avoid rounding errors
   int iMod = vertIndex - (iDiv * 4);
   return iMod;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int getCornerIndex8(int vertIndex)
{
   int iDiv = (vertIndex+0.5f) / 8;  // add 0.5f to avoid rounding errors
   int iMod = vertIndex - (iDiv * 8);
   return iMod;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeCorner(int vertIndex)
{   
   int cornerIndex = getCornerIndex(vertIndex);
   return cCorner[cornerIndex];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeCorner8(int vertIndex)
{   
   int cornerIndex = getCornerIndex8(vertIndex);
   return cCorner8[cornerIndex];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeUpCorner(int vertIndex)
{   
   int cornerIndex = getCornerIndex(vertIndex);
   return cUpCorner[cornerIndex];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeTrailCorner(int vertIndex)
{   
   int cornerIndex = getCornerIndex(vertIndex);
   return cTrailCorner[cornerIndex];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeTexCoords(int index)
{
   int cornerIndex = getCornerIndex(index);
   return float4(cCornerUV[cornerIndex], cCornerUV[cornerIndex]);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeTexCoords8(int index)
{
   int cornerIndex = getCornerIndex8(index);
   return float4(cCornerUV8[cornerIndex], cCornerUV8[cornerIndex]);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeStretchTexCoords(int index, int vertexIndex)
{
   #ifdef FETCH_OCTSTRIP
   int cornerIndex = getCornerIndex8(index);   
   float4 uv = float4(cStretchCornerUV8[cornerIndex], cStretchCornerUV8[cornerIndex]);
   #else
   int cornerIndex = getCornerIndex(index);   
   float4 uv = float4(cStretchCornerUV[cornerIndex], cStretchCornerUV[cornerIndex]);
   #endif

   //-- stretched uv's  gTimeData.z == vertex count
   uv.x = saturate(vertexIndex * gTimeData.z);
   uv.z = saturate(vertexIndex * gTimeData.z);
   return uv;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeStretchTexCoords8(int index, int vertexIndex)
{
   int cornerIndex = getCornerIndex8(index);   
   float4 uv = float4(cStretchCornerUV8[cornerIndex], cStretchCornerUV8[cornerIndex]);
   //-- stretched uv's  gTimeData.z == vertex count
   uv.x = saturate(vertexIndex * gTimeData.z);
   uv.z = saturate(vertexIndex * gTimeData.z);
   return uv;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeAnimatedFrameIndex(float birthTime)
{  
   //float life = max(0.0f, (gTimeData.x - birthTime));
   float life = birthTime;
   float curFrame = floor(life * gTimeData.y);
   float frame0 = fmod(curFrame, gTextureAnimationData.w);

   /*
   float frame1 = fmod(curFrame+1.0f, gTextureAnimationData.w);
   float timePerFrame = 1.0f / gTimeData.y;
   float curFrameAlpha = frac(life * gTimeData.y) / timePerFrame;
   */

   return float4(frame0, frame0, 0.0f, 0.0f);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeAnimatedFrameRowColumn(int frameIndex)
{
   // gTextureAnimationData.z == Texture Frames Per Row
   // compute which row we are on
   int frameRow = frameIndex / gTextureAnimationData.z;
   //-- the column is the is modulus of the row   
   int frameColumn = frameIndex % gTextureAnimationData.z;               
   return float2(frameRow, frameColumn);
}
            
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeAnimatedTexCoords(int index, float2 frameRowColumn)
{
#ifdef FETCH_OCTSTRIP
   int cornerIndex = getCornerIndex8(index);
   float2 uvSelector = cCornerUV8[cornerIndex]; //-- used in lerp to see whether we want u1 or v1
#else
   int cornerIndex = getCornerIndex(index);
   float2 uvSelector = cCornerUV[cornerIndex]; //-- used in lerp to see whether we want u1 or v1
#endif
   
   //-- Map the UVs.
   float u0 = frameRowColumn.y * gTextureAnimationData.x;
   float u1 = u0 + gTextureAnimationData.x;

   float v0 = frameRowColumn.x * gTextureAnimationData.y;
   float v1 = v0 + gTextureAnimationData.y;
      
   float2 uv;
   uv.x = lerp(u0, u1, uvSelector.x);
   uv.y = lerp(v0, v1, uvSelector.y);
   return uv;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeTumble(float2 pos, float angle)
{
   // Compute sin and cos of the rotation angle.
	float fSin, fCos;
	sincos(angle, fSin, fCos);
	
	// Rotate the corner vector by the rotation angle.
	float2 vCornerRotated;
	vCornerRotated.x = pos.x * fCos - pos.y * fSin;
	vCornerRotated.y = pos.x * fSin + pos.y * fCos;
	
   return vCornerRotated;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeBillBoardCorner(float4 pos, float2 scale, float tumbleAngle, int index)
{
   float2 corner   = ComputeCorner(index);
   
   //-- apply scale;
   corner *= scale;
   
   //-- rotate the corner if the particle is tumbling (rotating)
   corner = ComputeTumble(corner, tumbleAngle);
   
   //-- now create the vertex pos in world space   
   float3 rightV   = gViewToWorld._m00_m01_m02 * corner.x;
   float3 upV      = gViewToWorld._m10_m11_m12 * corner.y;   
   float4 finalPos = float4(pos.xyz + rightV.xyz + upV.xyz, 1.0f);
   
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeLitColor(int index, float tumbleAngle)
{
   //return float4(1,1,1,1);
   
   float2 corner   = ComputeCorner(index);         
   //-- rotate the corner if the particle is tumbling (rotating)
   corner = ComputeTumble(corner, tumbleAngle);

   //-- range compress from -1..1 to 0..1
   corner = (corner * 0.5f) + float2(0.5f,0.5f);

   float4 litColor1 = lerp(gVertex1Color, gVertex2Color, corner.x);
   float4 litColor2 = lerp(gVertex3Color, gVertex4Color, corner.x);
   float4 finalLitColor = lerp(litColor2, litColor1, corner.y);

   return finalLitColor;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeUpFacingCorner(float4 pos, float2 scale, float tumbleAngle, int index)
{
   float2 corner = ComputeCorner(index);
   //-- apply scale;
   corner *= scale;   
   corner = ComputeTumble(corner, tumbleAngle);

   return float4(pos.x+corner.x, pos.y, pos.z+corner.y, 1.0f);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeVelocityAlignedCorner(float4 pos, float3 forward, float2 scale, float tumbleAngle, int index)
{
   float2 corner;   
   float3 forwardV = normalize(forward);   
   float3 upV;
   float3 rightV;
      
   if (abs(forwardV.y) >= 0.99999f)
   {
      corner = ComputeCorner(index);   
      //-- apply scale;
      corner *= scale;   
      corner = ComputeTumble(corner, tumbleAngle);

      return float4(pos.x+corner.x, pos.y, pos.z+corner.y, 1.0f);
   }
   
   corner = ComputeCorner(index);   
   //-- apply scale;
   corner *= scale;   
   corner = ComputeTumble(corner, tumbleAngle);

   rightV = normalize(cross(float3(0,1,0), forwardV));   
   upV    = normalize(cross(forwardV, rightV));
   rightV *= corner.x;
   upV    *= corner.y;
   return float4(pos.xyz + rightV.xyz + upV.xyz, 1.0f);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeAxialCorner(float4 pos, float4 axis, float2 scale, float tumbleAngle, int index)
{
   float2 corner = ComputeCorner(index);
   
   //-- apply scale;
   corner *= scale; 
     
   //-- apply tumble
   corner = ComputeTumble(corner, tumbleAngle);
       
   float3 upV       = normalize(float3(axis.xyz));
   float3 cameraDir = normalize(pos-gWorldCameraPos);
   float3 rightV    = normalize(cross(upV, cameraDir));

   upV   *= corner.x;
   rightV*= corner.y;

   float4 finalPos = float4(pos.xyz + rightV.xyz + upV.xyz, 1);
   return finalPos;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeTrailCornerPos(float3 pos, float3 forward, float2 scale, int index)
{  
   forward = normalize(forward);

   #ifdef FETCH_OCTSTRIP
   int cornerIndex = getCornerIndex8(index);   
   float cornerMultiplier = cTrailCornerMultiplier8[cornerIndex];
   #else
   int cornerIndex = getCornerIndex(index);   
   float cornerMultiplier = cTrailCornerMultiplier[cornerIndex];
   #endif
  
   float3 cameraPos = gViewToWorld._m30_m31_m32;
   float3 cameraDir = cameraPos-pos;
   cameraDir = normalize(cameraDir);
   float3 rightV = normalize(cross(cameraDir, forward));
   
   float3 finalPos;
   finalPos = (rightV*cornerMultiplier*scale.x) + pos;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeTrailCornerPos8(float3 pos, float3 forward, float2 scale, int index)
{  
   forward = normalize(forward);
   int cornerIndex = getCornerIndex8(index);   
   float cornerMultiplier = cTrailCornerMultiplier8[cornerIndex];
  
   float3 cameraPos = gViewToWorld._m30_m31_m32;
   float3 cameraDir = cameraPos-pos;
   cameraDir = normalize(cameraDir);
   float3 rightV = normalize(cross(cameraDir, forward));   
   
   float3 finalPos;
   finalPos = (rightV*cornerMultiplier*scale.x) + pos;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeTrailCrossVertical(float3 pos, float3 forward, float3 up, float2 scale, int index)
{
   #ifdef FETCH_OCTSTRIP
      int cornerIndex = getCornerIndex8(index);
      float cornerMultiplier = cTrailCrossCornerMultiplier8[cornerIndex];      
   #else
      int cornerIndex = getCornerIndex(index);
      float cornerMultiplier = cTrailCrossCornerMultiplier[cornerIndex];
   #endif
   
   up *= cornerMultiplier;
   up *= scale.y;
   
   float3 finalPos = pos + up;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeTrailCrossVertical8(float3 pos, float3 forward, float3 up, float2 scale, int index)
{
   int cornerIndex = getCornerIndex8(index);
   float cornerMultiplier = cTrailCrossCornerMultiplier8[cornerIndex];
   
   up *= cornerMultiplier;
   up *= scale.y;
   
   float3 finalPos = pos + up;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeTrailCrossHorizontal(float3 pos, float3 forward, float3 up, float2 scale, int index)
{
   #ifdef FETCH_OCTSTRIP
      int cornerIndex = getCornerIndex8(index);
      float cornerMultiplier = cTrailCrossCornerMultiplier8[cornerIndex];
   #else
      int cornerIndex = getCornerIndex(index);
      float cornerMultiplier = cTrailCrossCornerMultiplier[cornerIndex];
   #endif

   float3 rightV = cross(forward, up);
   
   rightV *= cornerMultiplier;
   rightV *= scale.x;
   
   float3 finalPos = pos + rightV;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeTrailCrossHorizontal8(float3 pos, float3 forward, float3 up, float2 scale, int index)
{
   int cornerIndex = getCornerIndex8(index);
   float cornerMultiplier = cTrailCrossCornerMultiplier8[cornerIndex];
   float3 rightV = cross(forward, up);
   
   rightV *= cornerMultiplier;
   rightV *= scale.x;
   
   float3 finalPos = pos + rightV;
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float3 ComputeBeam(float3 pos, float3 forward, float scale, int index)
{
   int cornerIndex = getCornerIndex(index);
   float cornerMultiplier = cTrailCrossCornerMultiplier[cornerIndex];
   float3 rightV = cross(float3(0,1,0), forward);
   float3 upV = cross(forward, rightV);
   
   rightV *= cornerMultiplier;
   rightV *= scale;
   
   float3 finalPos = (pos * rightV) + (pos * upV) + (pos * forward);
   return finalPos;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float4 interpolate(float4 a, float4 b, float4 c, float4 d, float4 weights)
{
    return a * weights.w + b * weights.z + c * weights.y + d * weights.x;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float3 interpolate(float3 a, float3 b, float3 c, float3 d, float4 weights)
{
    return a * weights.w + b * weights.z + c * weights.y + d * weights.x;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float2 interpolate(float2 a, float2 b, float2 c, float2 d, float4 weights)
{
    return a * weights.w + b * weights.z + c * weights.y + d * weights.x;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float2 heightfieldDepthToY(float2 s)
{
   return s * gHeightfieldYScaleOfs.x + float2(gHeightfieldYScaleOfs.y, gHeightfieldYScaleOfs.y);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeTerrainPatch(float2 inUV, int inQuadID, float4 inPos, float4 inScale, float inTumbleAngle, out float2 fixedUV)
{
   bool invertNormals;   
   float4 weights = calcWeights(inUV, inQuadID, invertNormals, fixedUV);
             
   float4 empty = float4(0,0,0,0);
   float4 p0 = ComputeUpFacingCorner(empty, inScale, inTumbleAngle, 0);
   float4 p1 = ComputeUpFacingCorner(empty, inScale, inTumbleAngle, 1);
   float4 p2 = ComputeUpFacingCorner(empty, inScale, inTumbleAngle, 3);
   float4 p3 = ComputeUpFacingCorner(empty, inScale, inTumbleAngle, 2);
      
   float3 p = interpolate(p0, p1, p2, p3, weights);
   float4 worldPos = float4(p, 0.0) + float4(inPos.x, inPos.y, inPos.z, 1.0);
         
   // conform to terrain.
   {
      float4 centerHeightfieldPos = mul(float4(inPos.x, inPos.y, inPos.z, 1.0), gWorldToHeightfield);
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
      if (abs(inPos.y - centerLoHiY.y) < abs(inPos.y - centerY))
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

   worldPos.y += gTerrainDecalYOffset.x;

   fixedUV.xy = saturate(float2(1.0f,1.0f) - fixedUV.yx);      

   return worldPos;
}

//----------------------------------------------------------------------------
// Pseudo Code
// XMVectorMultiply(XMVectorMultiplyAdd(randomV, varience, gVectorOne), baseValue)
//----------------------------------------------------------------------------
float ComputeVariance(float randomValue, float variance, float baseValue)
{
   float finalValue = (randomValue * variance) + 1.0f;
   finalValue *= baseValue;   
}

//----------------------------------------------------------------------------
// Pseudo Code
// XMVectorMultiply(XMVectorMultiplyAdd(randomV, varience, gVectorOne), baseValue)
//----------------------------------------------------------------------------
float4 ComputeVariance4(float4 randomValue, float4 variance, float4 baseValue)
{
   float4 finalValue = (randomValue * variance) + 1.0f;
   finalValue *= baseValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float ComputeTextureZ(float oneOverStageCount, float textureArrayIndex)
{   
   float textureArrayZ = (0.49999f * oneOverStageCount) + (textureArrayIndex * oneOverStageCount);
   return textureArrayZ;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 ComputeUVOffset(float lifeInSeconds, float2 uvVelocity)
{
   float scalar = lifeInSeconds;
   float2 uvOffset;
   uvOffset.x = lifeInSeconds * uvVelocity.x;
   uvOffset.y = lifeInSeconds * uvVelocity.y;
   return uvOffset;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Internal(in  int Index
#ifdef TERRAIN_PATCH
                        ,in  float2 inUV
                        ,in  int    inQuadID
#endif
                        ,out float4 OutPosition
                        ,out float4 OutTexCoord0
                        ,out float4 OutTexCoord1
                        ,out float4 OutTexCoord2
                        ,out float4 OutTexCoord3
                        ,out float4 OutColor0
                        ,out float3 OutLightBufPos
                        ,out float  OutViewZ
                        ,uniform bool PremultiplyAlpha)
{

#ifdef TERRAIN_PATCH
   int vertexIndex = Index;
#else

   #ifdef FETCH_OCTSTRIP
      int cornerIndex = getCornerIndex(Index);
      int vertexIndex = cVertexPattern[cornerIndex] + floor((Index+0.5f)/8);      
   #else
      #ifdef FETCH_QUADSTRIP   
         int vertexIndex = (floor((Index+0.5f)/2)) - (floor((Index+0.5f)/4));  //--add in 0.5f to avoid rounding errors
      #else // normal quad fetching
         int vertexIndex = (Index+0.5f) / 4;
      #endif
   #endif

#endif

   float4 InPosition = fetchPosition(vertexIndex);
   float4 InScale    = fetchTexCoord1(vertexIndex);
   float4 InColor0   = fetchColor0(vertexIndex);
   float4 textureZ   = fetchColor1(vertexIndex);
   float4 InLife     = fetchTexCoord3(vertexIndex);
   float4 InVelocity = fetchVelocity(vertexIndex);
   
#ifdef BEAM
   float4 progressionAlpha = InLife.xxxx;
   // beams can query progressions by distance and or lifespan so we need to figure out which one to use
   progressionAlpha = lerp(float4(InLife.xxxx), float4(InLife.zzzz), gBeamProgressionAlphaSelector);
#endif

   
   //-- retrieve the tumble Angle
   float tumbleAngle = InPosition.w;
   float lifeInSeconds = InVelocity.w;   

   //-- bring the z Value back into float world 
   textureZ *= 255.0f;

   //-- compute the z Values for the texture lookups
   textureZ.x = ComputeTextureZ(gTextureStageCounts.x, textureZ.x); // Diffuse 0
   textureZ.y = ComputeTextureZ(gTextureStageCounts.y, textureZ.y); // Diffuse 1
   textureZ.z = ComputeTextureZ(gTextureStageCounts.z, textureZ.z); // Diffuse 2
   textureZ.w = ComputeTextureZ(gTextureStageCounts.w, textureZ.w); // Intensity 

   //-- pass through the z Values
   OutTexCoord3 = textureZ;
   
   //-- fixup the w component
   InPosition.w = 1.0f;   
   
#if  (!defined(UPFACING)) && (!defined(VELOCITYALIGNED))
   //-- transform the particle pos by the emitter matrix for particles that are
   //-- tied to the emitter.
   //-- its the identity if its not tied to the emitter      
   InPosition = mul(InPosition, gEmitterMatrix);
#endif

   //-- Intensity
   OutTexCoord2.rgb = InLife.yyy;
   OutTexCoord2.a   = 1.0f;
#ifdef BEAM
   OutTexCoord2.rgb *= tex2Dlod(gIntensityProgressionSampler, float4(progressionAlpha.z, gProgressionTextureV.z, 0.0f, 0.0f));
#else
   OutTexCoord2.rgb *= tex2Dlod(gIntensityProgressionSampler, float4(InLife.x, gProgressionTextureV.z, 0.0f, 0.0f));
#endif

   if (gPalletteColorToggle > 0)
   {
      //-- range expand the random value so its between 0 and 1;      
      float randomValue = saturate((InScale.w * 0.5f) + 0.5f);
      float lookupValue = lerp(0.0f, gPalletteColorCount, randomValue)+0.5f;
      InColor0.rgb *= gPalletteColor[lookupValue].rgb;
   }
   
   //-- get the color progression
#ifdef BEAM
   InColor0.rgb  *= tex2Dlod(gColorProgressionSampler, float4(progressionAlpha.x, gProgressionTextureV.x, 0.0f, 0.0f)).rgb;
   InColor0.a    *= tex2Dlod(gColorProgressionSampler, float4(progressionAlpha.y, gProgressionTextureV.x, 0.0f, 0.0f)).a;
#else
   InColor0   *= tex2Dlod(gColorProgressionSampler, float4(InLife.x, gProgressionTextureV.x, 0.0f, 0.0f));
#endif

   InColor0.a *= gEmitterOpacity;

   //-- get scale progression
#ifdef BEAM
   // first get the scale using the life time to do an over all scale
   float4 InScaleProgression = tex2Dlod(gScaleProgressionSampler, float4(InLife.x, gProgressionTextureV.y, 0.0f, 0.0f));
   // then fetch using the distance alpha and scale the overal scale by the distance scale
   InScaleProgression *= tex2Dlod(gScaleProgressionSampler, float4(InLife.z, gProgressionTextureV.y, 0.0f, 0.0f)).yyyy;
   InScale.xyz *= InScaleProgression.xyz;
#else
   float4 InScaleProgression = tex2Dlod(gScaleProgressionSampler, float4(InLife.x, gProgressionTextureV.y, 0.0f, 0.0f));
   InScale.xyz *= InScaleProgression.xyz;     
#endif

#ifdef TERRAIN_PATCH
   float2 fixedUV;
   OutPosition = ComputeTerrainPatch(inUV, inQuadID, InPosition, InScale, tumbleAngle, fixedUV);   
   InColor0 *= ComputeLitColor(Index, tumbleAngle);
#endif 
                 
#ifdef BILLBOARD
   OutPosition   =  ComputeBillBoardCorner(InPosition, InScale, tumbleAngle, Index);
   InColor0 *= ComputeLitColor(Index, tumbleAngle);
#endif

#ifdef UPFACING   
   OutPosition = ComputeUpFacingCorner(InPosition, InScale, tumbleAngle, Index);
   OutPosition = mul(OutPosition, gEmitterMatrix);
   InColor0 *= ComputeLitColor(Index, tumbleAngle);
#endif

#ifdef VELOCITYALIGNED         
   OutPosition = ComputeVelocityAlignedCorner(InPosition, InVelocity.xyz, InScale, tumbleAngle, Index);
   OutPosition = mul(OutPosition, gEmitterMatrix);
   InColor0 *= ComputeLitColor(Index, tumbleAngle);
#endif

#ifdef AXIAL   
   InVelocity.w = 0.0f;
   InVelocity = mul(InVelocity, gEmitterMatrix);
   OutPosition = ComputeAxialCorner(InPosition, InVelocity, InScale, tumbleAngle, Index);
   InColor0 *= ComputeLitColor(Index, tumbleAngle);
#endif

   float3 forward = float3(0,0,1);
#ifdef TRAIL_COMPUTE_FORWARD
   
   //-- if its the first trail poly we use the next vertex position to figure out our 
   //-- forward vector
   if (vertexIndex <= 0)
   {
      //-- fetch the last position for a trail
      float4 nextPosition = fetchPosition(vertexIndex+1);
      forward = normalize(nextPosition.xyz - InPosition.xyz);
   } 
   else //-- we had a previous position use that to compute our forward vector
   {
      float4 InLastPosition = fetchPosition(max(0, vertexIndex-1));
      forward = normalize(InPosition.xyz - InLastPosition.xyz);
   }
#endif

#ifdef TRAIL
   OutPosition.xyz = ComputeTrailCornerPos(InPosition, forward, InScale.xy, Index);
   /*
   #ifdef FETCH_OCTSTRIP
      OutPosition.xyz = ComputeTrailCornerPos8(InPosition, forward, InScale.xy, Index);
   #else
      OutPosition.xyz = ComputeTrailCornerPos(InPosition, forward, InScale.xy, Index);
   #endif
   */
   OutPosition.w   = 1.0f; 
#endif 

#ifdef TRAILCROSS_PASS1
   float4 up = fetchTexCoord2(vertexIndex);
   OutPosition.xyz = ComputeTrailCrossVertical(InPosition, forward, up.xyz, InScale.xy, Index);
   /*
   #ifdef FETCH_OCTSTRIP
      OutPosition.xyz = ComputeTrailCrossVertical8(InPosition, forward, up.xyz, InScale.xy, Index);
   #else
      OutPosition.xyz = ComputeTrailCrossVertical(InPosition, forward, up.xyz, InScale.xy, Index);
   #endif
   */
   OutPosition.w = 1.0f;
#endif

#ifdef TRAILCROSS_PASS2
   float4 up = fetchTexCoord2(vertexIndex);
   OutPosition.xyz = ComputeTrailCrossHorizontal(InPosition, forward, up.xyz, InScale.xy, Index);
   /*
   #ifdef FETCH_OCTSTRIP
      OutPosition.xyz = ComputeTrailCrossHorizontal8(InPosition, forward, up.xyz, InScale.xy, Index);
   #else
      OutPosition.xyz = ComputeTrailCrossHorizontal(InPosition, forward, up.xyz, InScale.xy, Index);
   #endif
   */
   OutPosition.w = 1.0f;
#endif      

#if (defined(BEAM)) && (defined(BEAM_VERTICAL)) && (!defined(BEAM_HORIZONTAL))   
   float4 up = float4(0,1,0,0);
   OutPosition.xyz = ComputeTrailCrossVertical(InPosition, gBeamForward, up.xyz, InScale.xy, Index);
   OutPosition.w = 1.0f;
#endif

#if (defined(BEAM)) && (!defined(BEAM_VERTICAL)) && (defined(BEAM_HORIZONTAL))   
   float4 up = float4(0,1,0,0);
   OutPosition.xyz = ComputeTrailCrossHorizontal(InPosition, gBeamForward, up.xyz, InScale.xy, Index);
   OutPosition.w = 1.0f;
#endif

#if (defined(BEAM)) && (!defined(BEAM_VERTICAL)) && (!defined(BEAM_HORIZONTAL))
   OutPosition.xyz = ComputeTrailCornerPos(InPosition, gBeamForward, InScale.xy, Index);
   OutPosition.w   = 1.0f;    
#endif

   float4 worldPos = float4(OutPosition.x, OutPosition.y, OutPosition.z, 1.0f);
   OutLightBufPos.x = dot(worldPos, gWorldToLightBufCols[0]);
   OutLightBufPos.y = dot(worldPos, gWorldToLightBufCols[1]);
   OutLightBufPos.z = dot(worldPos, gWorldToLightBufCols[2]);

   // z value in viewspace 
   OutViewZ = mul(OutPosition, gWorldToView).z;
   
   //-- transform position from world to projection space
   OutPosition   = mul(OutPosition, gWorldToProj);
   
   float2 uvDiffuse0Offset  = ComputeUVOffset(lifeInSeconds, gUVVelocity0.xy);
   float2 uvDiffuse1Offset  = ComputeUVOffset(lifeInSeconds, gUVVelocity0.zw);
   float2 uvDiffuse2Offset  = ComputeUVOffset(lifeInSeconds, gUVVelocity1.xy);
   float2 uvIntensityOffset = ComputeUVOffset(lifeInSeconds, gUVVelocity1.zw);   

#ifdef TRAIL_STRETCHED_TEXTURE
   OutTexCoord0 = ComputeStretchTexCoords(Index, vertexIndex);
   /*
   #ifdef FETCH_OCTSTRIP
   OutTexCoord0 = ComputeStretchTexCoords8(Index, vertexIndex);
   #else
   OutTexCoord0 = ComputeStretchTexCoords(Index, vertexIndex);
   #endif
   */
   
   OutTexCoord1 = OutTexCoord0;
   OutTexCoord0.xy += uvDiffuse0Offset;
   OutTexCoord0.zw += uvDiffuse1Offset;
   OutTexCoord1.xy += uvDiffuse2Offset;
   OutTexCoord1.zw += uvIntensityOffset;      
#else
   //-- Compute TexCoords
   if (gTextureAnimationToggle > 0) //-- compute animated texcoords
   {
      float4 frameIndexAndAlpha = ComputeAnimatedFrameIndex(InScale.z); // InScale.z == birth Time of particle
      float2 frameRowColumn     = ComputeAnimatedFrameRowColumn(frameIndexAndAlpha.x);
      OutTexCoord0.xy           = ComputeAnimatedTexCoords(Index, frameRowColumn);
      OutTexCoord0.zw           = 0.0f;      
      OutTexCoord1              = ComputeTexCoords(Index);      
   }
   else //-- non animated texcoords
   {
      #ifdef TERRAIN_PATCH
         OutTexCoord0   = float4(fixedUV, fixedUV);
      #else
         #ifdef FETCH_OCTSTRIP   
         OutTexCoord0   = ComputeTexCoords8(Index);
         #else
         OutTexCoord0   = ComputeTexCoords(Index);
         #endif
      #endif

      OutTexCoord1 = OutTexCoord0;
      OutTexCoord0.x += lerp(0.0f, InScale.w, gUVRandomOffsetSelector0.x);
      OutTexCoord0.y += lerp(0.0f, InScale.w, gUVRandomOffsetSelector0.y);
      OutTexCoord0.z += lerp(0.0f, InScale.w, gUVRandomOffsetSelector0.z);
      OutTexCoord0.w += lerp(0.0f, InScale.w, gUVRandomOffsetSelector0.w);
      
      OutTexCoord1.x += lerp(0.0f, InScale.w, gUVRandomOffsetSelector1.x);
      OutTexCoord1.y += lerp(0.0f, InScale.w, gUVRandomOffsetSelector1.y);
      OutTexCoord1.z += lerp(0.0f, InScale.w, gUVRandomOffsetSelector1.z);
      OutTexCoord1.w += lerp(0.0f, InScale.w, gUVRandomOffsetSelector1.w);
      
      OutTexCoord0.xy += uvDiffuse0Offset;
      OutTexCoord0.zw += uvDiffuse1Offset;
      OutTexCoord1.xy += uvDiffuse2Offset;
      OutTexCoord1.zw += uvIntensityOffset;
   }
#endif

   InColor0 *= gTintColor;
   
   //-- do we want to premultiply the alpha (this is done usually for additive blending)
   if (PremultiplyAlpha)
   {
      InColor0.rgb *= InColor0.a;
      //InColor0.a   = 1.0f;
   }   
      
   //-- pass through color   
   OutColor0 = InColor0;
}

//----------------------------------------------------------------------------
// Shaders
//----------------------------------------------------------------------------
void ParticleVS(  in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                  in  float2 inUV           : BARYCENTRIC,
                  in  int    inQuadID       : QUADID,   
#endif
                  out float4 OutPosition    : POSITION,
                  out float4 OutTexCoord0   : TEXCOORD0,
                  out float4 OutTexCoord1   : TEXCOORD1,
                  out float4 OutTexCoord2   : TEXCOORD2,
                  out float4 OutTexCoord3   : TEXCOORD3,
                  out float4 OutColor0      : COLOR0,
                  out float3 OutLightBufPos : TEXCOORD4,
                  out float  OutViewZ       : TEXCOORD5,
                  uniform bool PremultiplyAlpha)
{
   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,OutPosition
                       ,OutTexCoord0
                       ,OutTexCoord1
                       ,OutTexCoord2
                       ,OutTexCoord3
                       ,OutColor0
                       ,OutLightBufPos
                       ,OutViewZ
                       ,PremultiplyAlpha);
}

//----------------------------------------------------------------------------
// Shaders
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Single_Simple(   in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                                 in  float2 inUV           : BARYCENTRIC,
                                 in  int    inQuadID       : QUADID,   
#endif
                                 out float4 OutPosition    : POSITION,
                                 out float4 OutTexCoord0   : TEXCOORD0,
                                 out float4 OutTexCoord1   : TEXCOORD1,                  
                                 out float4 OutColor0      : COLOR0,                  
                                 uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = localTexCoord2;
   OutColor0 = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Single_Simple_LightBuffer(in  int Index          : INDEX,
                                          #ifdef TERRAIN_PATCH
                                          in  float2 inUV           : BARYCENTRIC,
                                          in  int    inQuadID       : QUADID,   
                                          #endif
                                          out float4 OutPosition    : POSITION,
                                          out float4 OutTexCoord0   : TEXCOORD0,
                                          out float4 OutTexCoord1   : TEXCOORD1,
                                          out float3 OutTexCoord2   : TEXCOORD2,
                                          out float4 OutColor0      : COLOR0,
                                          uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = localTexCoord2;
   OutTexCoord2 = localLightBufPos;
   OutColor0    = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Single_Simple_IntensityMask(   in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                                 in  float2 inUV           : BARYCENTRIC,
                                 in  int    inQuadID       : QUADID,   
#endif
                                 out float4 OutPosition    : POSITION,
                                 out float4 OutTexCoord0   : TEXCOORD0,
                                 out float4 OutTexCoord1   : TEXCOORD1,
                                 out float4 OutTexCoord2   : TEXCOORD2,
                                 out float4 OutColor0      : COLOR0,
                                 uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = localTexCoord2;
   OutTexCoord2 = float4(localTexCoord1.zw, localTexCoord3.w, 0.0f);
   OutColor0    = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Single_Simple_LightBuffer_IntensityMask(   in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                                 in  float2 inUV           : BARYCENTRIC,
                                 in  int    inQuadID       : QUADID,   
#endif
                                 out float4 OutPosition    : POSITION,
                                 out float4 OutTexCoord0   : TEXCOORD0,
                                 out float4 OutTexCoord1   : TEXCOORD1,
                                 out float4 OutTexCoord2   : TEXCOORD2,
                                 out float3 OutTexCoord3   : TEXCOORD3,
                                 out float4 OutColor0      : COLOR0,
                                 uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = localTexCoord2;
   OutTexCoord2 = float4(localTexCoord1.zw, localTexCoord3.w, 0.0f);
   OutTexCoord3 = localLightBufPos;
   OutColor0    = localColor0;
}

//============================================================================
// Multi 2
//============================================================================
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Multi2_Simple(   in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                                 in  float2 inUV           : BARYCENTRIC,
                                 in  int    inQuadID       : QUADID,
#endif
                                 out float4 OutPosition    : POSITION,
                                 out float4 OutTexCoord0   : TEXCOORD0,
                                 out float4 OutTexCoord1   : TEXCOORD1,
                                 out float4 OutTexCoord2   : TEXCOORD2,
                                 out float4 OutColor0      : COLOR0,
                                 uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = float4(localTexCoord0.zw, localTexCoord3.y, localOutViewZ);
   OutTexCoord2 = localTexCoord2;
   OutColor0 = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Multi2_Simple_LightBuffer(in  int Index          : INDEX,
                                          #ifdef TERRAIN_PATCH
                                          in  float2 inUV           : BARYCENTRIC,
                                          in  int    inQuadID       : QUADID,   
                                          #endif
                                          out float4 OutPosition    : POSITION,
                                          out float4 OutTexCoord0   : TEXCOORD0,
                                          out float4 OutTexCoord1   : TEXCOORD1,
                                          out float4 OutTexCoord2   : TEXCOORD2,
                                          out float3 OutTexCoord3   : TEXCOORD3,
                                          out float4 OutColor0      : COLOR0,
                                          uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = float4(localTexCoord0.zw, localTexCoord3.y, localOutViewZ);
   OutTexCoord2 = localTexCoord2;
   OutTexCoord3 = localLightBufPos;
   OutColor0    = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Multi2_Simple_IntensityMask(   in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                                 in  float2 inUV           : BARYCENTRIC,
                                 in  int    inQuadID       : QUADID,   
#endif
                                 out float4 OutPosition    : POSITION,
                                 out float4 OutTexCoord0   : TEXCOORD0,
                                 out float4 OutTexCoord1   : TEXCOORD1,
                                 out float4 OutTexCoord2   : TEXCOORD2,
                                 out float4 OutTexCoord3   : TEXCOORD3,
                                 out float4 OutColor0      : COLOR0,
                                 uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = float4(localTexCoord0.zw, localTexCoord3.y, localOutViewZ);
   OutTexCoord2 = localTexCoord2;
   OutTexCoord3 = float4(localTexCoord1.zw, localTexCoord3.w, localOutViewZ);
   OutColor0    = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleVS_Multi2_Simple_LightBuffer_IntensityMask(   in  int    Index          : INDEX,
#ifdef TERRAIN_PATCH
                                 in  float2 inUV           : BARYCENTRIC,
                                 in  int    inQuadID       : QUADID,   
#endif
                                 out float4 OutPosition    : POSITION,
                                 out float4 OutTexCoord0   : TEXCOORD0,
                                 out float4 OutTexCoord1   : TEXCOORD1,
                                 out float4 OutTexCoord2   : TEXCOORD2,
                                 out float4 OutTexCoord3   : TEXCOORD3,
                                 out float3 OutTexCoord4   : TEXCOORD4,
                                 out float4 OutColor0      : COLOR0,
                                 uniform bool PremultiplyAlpha)
{   

   float4 localPos;
   float4 localTexCoord0;
   float4 localTexCoord1;
   float4 localTexCoord2;
   float4 localTexCoord3;
   float4 localColor0;
   float3 localLightBufPos;
   float  localOutViewZ;

   ParticleVS_Internal(Index
   #ifdef TERRAIN_PATCH
                       ,inUV
                       ,inQuadID
   #endif
                       ,localPos
                       ,localTexCoord0
                       ,localTexCoord1
                       ,localTexCoord2
                       ,localTexCoord3
                       ,localColor0
                       ,localLightBufPos
                       ,localOutViewZ
                       ,PremultiplyAlpha);

   OutPosition  = localPos;
   OutTexCoord0 = float4(localTexCoord0.xy, localTexCoord3.x, localOutViewZ);
   OutTexCoord1 = float4(localTexCoord0.zw, localTexCoord3.y, localOutViewZ);
   OutTexCoord2 = localTexCoord2;
   OutTexCoord3 = float4(localTexCoord1.zw, localTexCoord3.w, localOutViewZ);
   OutTexCoord4 = localLightBufPos;

   OutColor0    = localColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 sampleTexture(sampler arraySampler, float2 uv, float index)
{
   return tex3D(arraySampler, float3(uv, index));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 sampleTextureF3(sampler arraySampler, float3 uvIndex)
{
   return tex3D(arraySampler, uvIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ComputeIncomingLight(in float3 inLightBufPos, in float3 inColor, out float3 outColor)
{
   float3 incomingLight = tex3D(gLightBufferSampler, inLightBufPos) * gLightBufferIntensityScale;
   outColor = inColor + (inColor * incomingLight);      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ComputeIntensityMask(in float3 inUV, in float3 inColor, out float3 outColor)
{
   float4 intensityMaskSample = sampleTextureF3(gIntensityArraySampler, inUV.xyz);
   intensityMaskSample.rgb *= 16.0f;
   outColor.xyz = inColor * intensityMaskSample;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal(  in  float4 InTexCoord0 : TEXCOORD0
                         ,in  float4 InTexCoord1 : TEXCOORD1
                         ,in  float4 InTexCoord2 : TEXCOORD2
                         ,in  float4 InTexCoord3 : TEXCOORD3
                         ,in  float4 InColor0    : COLOR0
                         ,in  float3 InLightBufPos  : TEXCOORD4
                         ,out float4 OutColor    : COLOR0
                         ,uniform bool bMultiTextureEnable
                         ,uniform bool bMultiTextureEnable3
                         ,uniform bool bBlendModeAdditive
                       )
{
   if (bMultiTextureEnable)
   {      
      OutColor = float4(0,0,0,0);
      if (bMultiTextureEnable3)
      {         
         float4 diffuseLayer1 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);            
         float4 diffuseLayer2 = sampleTexture(gDiffuseArraySampler2, InTexCoord0.zw, InTexCoord3.y);
         float4 diffuseLayer3 = sampleTexture(gDiffuseArraySampler3, InTexCoord1.xy, InTexCoord3.z);
                  
         OutColor.a = 1.0f;
         OutColor.rgb += ((diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a)) * gMultiTextureBlendMultiplier.x;
         //OutColor.rgb += (diffuseLayer1.rgb * diffuseLayer2.rgb) * gMultiTextureBlendMultiplier.x;

         //-- do alpha (lerp)
         OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;      

         //-- do multiply
         OutColor.rgb += (OutColor.rgb * (diffuseLayer3.rgb * diffuseLayer3.a)) * gMultiTextureBlendMultiplier.z;
         //OutColor.rgb += (OutColor.rgb * diffuseLayer3.rgb) * gMultiTextureBlendMultiplier.z;
         //-- do alpha lerp         
         OutColor.rgb = lerp(OutColor.rgb, diffuseLayer3.rgb, diffuseLayer3.a * gMultiTextureBlendMultiplier.a);         
      }
      else
      {         
         float4 diffuseLayer1 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);            
         float4 diffuseLayer2 = sampleTexture(gDiffuseArraySampler2, InTexCoord0.zw, InTexCoord3.y);         
         
         OutColor.a   = 1.0f;

         //-- do multiply
         OutColor.rgb += (diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a) * gMultiTextureBlendMultiplier.x;         
         //-- do alpha (lerp)
         OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;
         
      }
      OutColor *= InColor0;            
      //-- fix me and do linear conversion into c code
      OutColor.rgb = saturate(OutColor.rgb);
      OutColor.xyz *= OutColor.xyz;

      if (gIntensityMaskToggle)
      {
         float4 intensityMaskSample = sampleTexture(gIntensityArraySampler, InTexCoord1.zw, InTexCoord3.w);
         intensityMaskSample.rgb *= 16.0f;
         InTexCoord2.rgb *= intensityMaskSample.rgb;         
           
         if (bBlendModeAdditive)
            OutColor *= intensityMaskSample.a;
         else
            OutColor.a *= intensityMaskSample.a;
      }
   }
   else
   {
      float4 textureSample0 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);
      OutColor = textureSample0;
      
      /* disabled cross fade for right now
      if (gTextureAnimationToggle)
      {             
         float4 textureSample1 = sampleTexture(gDiffuseArraySampler, InTexCoord1.xy, InTexCoord3.x);
         OutColor =  lerp(textureSample0, textureSample1, InTexCoord1.w);
      }
      */

      OutColor *= InColor0;

      //-- fix me and do linear conversion into c code
      OutColor.rgb = saturate(OutColor.rgb);
      OutColor.xyz *= OutColor.xyz;

      if (gIntensityMaskToggle)
      {
         float4 intensityMaskSample = sampleTexture(gIntensityArraySampler, InTexCoord1.zw, InTexCoord3.w) * 16.0f;
         InTexCoord2 *= intensityMaskSample;
      }
   }
   
   if (gLightBufferingEnabled)
   {
      ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);

      //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
      //OutColor.xyz += OutColor * incomingLight;
   }         
    
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;   
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Single(  in  float4 InTexCoord0    : TEXCOORD0
                                ,in  float4 InTexCoord1    : TEXCOORD1
                                ,in  float4 InTexCoord2    : TEXCOORD2
                                ,in  float4 InTexCoord3    : TEXCOORD3
                                ,in  float4 InColor0       : COLOR0
                                ,in  float3 InLightBufPos  : TEXCOORD4
                                ,out float4 OutColor       : COLOR0
                                ,uniform bool bUseLightBuffer
                                ,uniform bool bUseIntensityMask
                       )
{
   
   float4 textureSample0 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);
   OutColor = textureSample0;         

   OutColor *= InColor0;

   //-- fix me and do linear conversion into c code
   OutColor.rgb = saturate(OutColor.rgb);
   OutColor.xyz *= OutColor.xyz;

   if (bUseIntensityMask)
   {
      float4 intensityMaskSample = sampleTexture(gIntensityArraySampler, InTexCoord1.zw, InTexCoord3.w) * 16.0f;
      InTexCoord2 *= intensityMaskSample;
   }
   
   if (bUseLightBuffer)
   {
      ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);      
   }         
    
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;   
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Single_Diffuse(in float4 InTexCoord0, in float4 InColor0, out float4 OutColor)
{
   float4 textureSample0 = sampleTextureF3(gDiffuseArraySampler, InTexCoord0.xyz);
   OutColor = textureSample0;         
   OutColor *= InColor0;

   //-- fix me and do linear conversion into c code
   OutColor.rgb = saturate(OutColor.rgb);
   OutColor.xyz *= OutColor.xyz;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Single_Simple(  in  float4 InTexCoord0 : TEXCOORD0
                                       ,in  float4 InTexCoord1 : TEXCOORD1
                                       ,in  float4 InColor0    : COLOR0
                                       ,out float4 OutColor    : COLOR0
                       )
{   
   ParticlePSInternal_Single_Diffuse(InTexCoord0, InColor0, OutColor);             
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord1.xyz;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Single_LightBuffer(  in  float4 InTexCoord0    : TEXCOORD0
                                            ,in  float4 InTexCoord1    : TEXCOORD1
                                            ,in  float3 InLightBufPos  : TEXCOORD2
                                            ,in  float4 InColor0       : COLOR0
                                            ,out float4 OutColor       : COLOR0
                       )
{
   ParticlePSInternal_Single_Diffuse(InTexCoord0, InColor0, OutColor);
   
   ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);
   //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
   //OutColor.xyz += OutColor * incomingLight;
             
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord1.xyz;      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Single_IntensityMask(  in  float4 InTexCoord0    : TEXCOORD0
                                              ,in  float4 InTexCoord1    : TEXCOORD1
                                              ,in  float4 InTexCoord2    : TEXCOORD2
                                              ,in  float4 InColor0       : COLOR0
                                              ,out float4 OutColor       : COLOR0
                       )
{
 
   ParticlePSInternal_Single_Diffuse(InTexCoord0, InColor0, OutColor);
   
   ComputeIntensityMask(InTexCoord2.xyz, OutColor.xyz, OutColor.xyz);
   
   //float4 intensityMaskSample = sampleTextureF3(gIntensityArraySampler, InTexCoord2.xyz) * 16.0f;
   //OutColor.xyz *= intensityMaskSample;
             
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord1.xyz;      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Single_LightBuffer_IntensityMask(  in  float4 InTexCoord0    : TEXCOORD0
                                                          ,in  float4 InTexCoord1    : TEXCOORD1
                                                          ,in  float4 InTexCoord2    : TEXCOORD2
                                                          ,in  float3 InLightBufPos  : TEXCOORD3
                                                          ,in  float4 InColor0       : COLOR0
                                                          ,out float4 OutColor       : COLOR0
                       )
{
   ParticlePSInternal_Single_Diffuse(InTexCoord0, InColor0, OutColor);
   
   ComputeIntensityMask(InTexCoord2.xyz, OutColor.xyz, OutColor.xyz);
   //float4 intensityMaskSample = sampleTextureF3(gIntensityArraySampler, InTexCoord2.xyz) * 16.0f;
   //OutColor.xyz *= intensityMaskSample;

   ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);
   //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
   //OutColor.xyz += OutColor * incomingLight;
             
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord1.xyz;         
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi(  in  float4 InTexCoord0 : TEXCOORD0
                               ,in  float4 InTexCoord1 : TEXCOORD1
                               ,in  float4 InTexCoord2 : TEXCOORD2
                               ,in  float4 InTexCoord3 : TEXCOORD3                         
                               ,in  float4 InColor0    : COLOR0
                               ,in  float3 InLightBufPos  : TEXCOORD4
                               ,out float4 OutColor    : COLOR0                 
                               ,uniform bool bMultiTextureEnable3
                               ,uniform bool bBlendModeAdditive
                               ,uniform bool bUseLightBuffer
                               ,uniform bool bUseIntensityMask
                               
                       )
{
   
   OutColor = float4(0,0,0,0);
   if (bMultiTextureEnable3)
   {         
      float4 diffuseLayer1 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);            
      float4 diffuseLayer2 = sampleTexture(gDiffuseArraySampler2, InTexCoord0.zw, InTexCoord3.y);
      float4 diffuseLayer3 = sampleTexture(gDiffuseArraySampler3, InTexCoord1.xy, InTexCoord3.z);
               
      OutColor.a = 1.0f;
      OutColor.rgb += ((diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a)) * gMultiTextureBlendMultiplier.x;      

      //-- do alpha (lerp)
      OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;      

      //-- do multiply
      OutColor.rgb += (OutColor.rgb * (diffuseLayer3.rgb * diffuseLayer3.a)) * gMultiTextureBlendMultiplier.z;

      //-- do alpha lerp         
      OutColor.rgb = lerp(OutColor.rgb, diffuseLayer3.rgb, diffuseLayer3.a * gMultiTextureBlendMultiplier.a);         
   }
   else
   {         
      float4 diffuseLayer1 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);            
      float4 diffuseLayer2 = sampleTexture(gDiffuseArraySampler2, InTexCoord0.zw, InTexCoord3.y);         
      
      OutColor.a   = 1.0f;

      //-- do multiply
      OutColor.rgb += (diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a) * gMultiTextureBlendMultiplier.x;         
      //-- do alpha (lerp)
      OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;
      
   }

   OutColor *= InColor0;            
   //-- fix me and do linear conversion into c code
   OutColor.rgb = saturate(OutColor.rgb);
   OutColor.xyz *= OutColor.xyz;

   if (bUseIntensityMask)
   {
      float4 intensityMaskSample = sampleTexture(gIntensityArraySampler, InTexCoord1.zw, InTexCoord3.w);
      intensityMaskSample.rgb *= 16.0f;
      InTexCoord2.rgb *= intensityMaskSample.rgb;         
        
      if (bBlendModeAdditive)
         OutColor *= intensityMaskSample.a;
      else
         OutColor.a *= intensityMaskSample.a;
   }
   
   if (bUseLightBuffer)
   {
      ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);
      //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
      //OutColor.xyz += OutColor * incomingLight;
   }         
    
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi2(  in  float4 InTexCoord0 : TEXCOORD0
                                ,in  float4 InTexCoord1 : TEXCOORD1
                                ,in  float4 InTexCoord2 : TEXCOORD2
                                ,in  float4 InTexCoord3 : TEXCOORD3                         
                                ,in  float4 InColor0    : COLOR0
                                ,in  float4 InLightBufPos  : TEXCOORD4
                                ,out float4 OutColor    : COLOR0                                                 
                                ,uniform bool bBlendModeAdditive
                                ,uniform bool bUseLightBuffer
                                ,uniform bool bUseIntensityMask
                               
                       )
{
   
   OutColor = float4(0,0,0,0);               
   float4 diffuseLayer1 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);            
   float4 diffuseLayer2 = sampleTexture(gDiffuseArraySampler2, InTexCoord0.zw, InTexCoord3.y);         
      
   OutColor.a   = 1.0f;
   //-- do multiply
   OutColor.rgb += (diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a) * gMultiTextureBlendMultiplier.x;         
   //-- do alpha (lerp)
   OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;
      
   OutColor *= InColor0;            
   //-- fix me and do linear conversion into c code
   OutColor.rgb = saturate(OutColor.rgb);
   OutColor.xyz *= OutColor.xyz;

   if (bUseIntensityMask)
   {
      float4 intensityMaskSample = sampleTexture(gIntensityArraySampler, InTexCoord1.zw, InTexCoord3.w);
      intensityMaskSample.rgb *= 16.0f;
      InTexCoord2.rgb *= intensityMaskSample.rgb;         
        
      if (bBlendModeAdditive)
         OutColor *= intensityMaskSample.a;
      else
         OutColor.a *= intensityMaskSample.a;
   }
   
   if (bUseLightBuffer)
   {
      ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);
      //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
      //OutColor.xyz += OutColor * incomingLight;
   }         
    
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi2_Diffuse(in float4 InTexCoord0, in float4 InTexCoord1, in float4 InColor0, out float4 OutColor)
{
   float4 diffuseLayer1 = sampleTextureF3(gDiffuseArraySampler,  InTexCoord0.xyz);
   float4 diffuseLayer2 = sampleTextureF3(gDiffuseArraySampler2, InTexCoord1.xyz);
         
   OutColor = float4(0,0,0,1);

   //-- do multiply
   OutColor.rgb += (diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a) * gMultiTextureBlendMultiplier.x;
   //-- do alpha (lerp)
   OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;
      
   OutColor *= InColor0;
   //-- fix me and do linear conversion into c code
   OutColor.rgb = saturate(OutColor.rgb);
   OutColor.xyz *= OutColor.xyz;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi2_Simple( in  float4 InTexCoord0 : TEXCOORD0
                                      ,in  float4 InTexCoord1 : TEXCOORD1
                                      ,in  float4 InTexCoord2 : TEXCOORD2
                                      ,in  float4 InColor0    : COLOR0
                                      ,out float4 OutColor    : COLOR0
                       )
{       
   ParticlePSInternal_Multi2_Diffuse(InTexCoord0, InTexCoord1, InColor0, OutColor);
   /*
   float4 diffuseLayer1 = sampleTextureF3(gDiffuseArraySampler,  InTexCoord0.xyz);
   float4 diffuseLayer2 = sampleTextureF3(gDiffuseArraySampler2, InTexCoord1.xyz);
         
   OutColor.a   = 1.0f;
   //-- do multiply
   OutColor.rgb += (diffuseLayer1.rgb * diffuseLayer1.a) * (diffuseLayer2.rgb * diffuseLayer2.a) * gMultiTextureBlendMultiplier.x;
   //-- do alpha (lerp)
   OutColor.rgb += lerp(diffuseLayer1.rgb, diffuseLayer2.rgb, diffuseLayer2.a) * gMultiTextureBlendMultiplier.y;
      
   OutColor *= InColor0;
   //-- fix me and do linear conversion into c code
   OutColor.rgb = saturate(OutColor.rgb);
   OutColor.xyz *= OutColor.xyz;
   */
       
   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi2_Simple_LightBuffer( in  float4 InTexCoord0   : TEXCOORD0
                                                  ,in  float4 InTexCoord1   : TEXCOORD1
                                                  ,in  float4 InTexCoord2   : TEXCOORD2
                                                  ,in  float3 InLightBufPos : TEXCOORD3
                                                  ,in  float4 InColor0      : COLOR0
                                                  ,out float4 OutColor      : COLOR0
                       )
{   
   ParticlePSInternal_Multi2_Diffuse(InTexCoord0, InTexCoord1, InColor0, OutColor);
   
   //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
   //OutColor.xyz += OutColor * incomingLight;
   ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);

   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi2_Simple_IntensityMask( in  float4 InTexCoord0   : TEXCOORD0
                                                    ,in  float4 InTexCoord1   : TEXCOORD1
                                                    ,in  float4 InTexCoord2   : TEXCOORD2
                                                    ,in  float4 InTexCoord3   : TEXCOORD3
                                                    ,in  float4 InColor0      : COLOR0
                                                    ,out float4 OutColor      : COLOR0
                                                    ,uniform bool bBlendModeAdditive
                       )
{      
   float4 intensityMaskSample = sampleTextureF3(gIntensityArraySampler, InTexCoord3.xyz);
   intensityMaskSample.rgb *= 16.0f;
   InTexCoord2.rgb *= intensityMaskSample.rgb;         

   ParticlePSInternal_Multi2_Diffuse(InTexCoord0, InTexCoord1, InColor0, OutColor);
              
   if (bBlendModeAdditive)
      OutColor *= intensityMaskSample.a;
   else
      OutColor.a *= intensityMaskSample.a;  

   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;   
   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePSInternal_Multi2_Simple_LightBuffer_IntensityMask( in  float4 InTexCoord0   : TEXCOORD0
                                                    ,in  float4 InTexCoord1   : TEXCOORD1
                                                    ,in  float4 InTexCoord2   : TEXCOORD2
                                                    ,in  float4 InTexCoord3   : TEXCOORD3
                                                    ,in  float3 InLightBufPos : TEXCOORD4
                                                    ,in  float4 InColor0      : COLOR0
                                                    ,out float4 OutColor      : COLOR0
                                                    ,uniform bool bBlendModeAdditive
                       )
{         
   //ComputeIntensityMask(InTexCoord3.xyz, InTexCoord2, InTexCoord2);
   float4 intensityMaskSample = sampleTextureF3(gIntensityArraySampler, InTexCoord3.xyz);
   intensityMaskSample.rgb *= 16.0f;   
   InTexCoord2.rgb *= intensityMaskSample.rgb;         

   ParticlePSInternal_Multi2_Diffuse(InTexCoord0, InTexCoord1, InColor0, OutColor);
   
   //float3 incomingLight = tex3D(gLightBufferSampler, InLightBufPos) * gLightBufferIntensityScale;
   //localColor.xyz += localColor * incomingLight;
   ComputeIncomingLight(InLightBufPos, OutColor.xyz, OutColor.xyz);
              
   if (bBlendModeAdditive)
      OutColor *= intensityMaskSample.a;
   else
      OutColor.a *= intensityMaskSample.a;    

   //-- multiply intensity into rgb but not alpha
   OutColor.xyz *= InTexCoord2.xyz;     
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticlePS(  in  float4 InTexCoord0 : TEXCOORD0
                 ,in  float4 InTexCoord1 : TEXCOORD1
                 ,in  float4 InTexCoord2 : TEXCOORD2
                 ,in  float4 InTexCoord3 : TEXCOORD3
                 ,in  float4 InColor0    : COLOR0
                 ,in  float3 InWorldPos  : TEXCOORD4
                 ,out float4 OutColor    : COLOR0
                 ,uniform bool bMultiTextureEnable
                 ,uniform bool bBlendModeAdditive
               )
{
   ParticlePSInternal(InTexCoord0,
                      InTexCoord1,
                      InTexCoord2,
                      InTexCoord3,
                      InColor0,
                      InWorldPos,
                      OutColor,
                      bMultiTextureEnable,
                      (bMultiTextureEnable && gMultiTextureEnable3Layers) ? true : false,
                      bBlendModeAdditive);                             
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ComputeAlphaFade( in float2 inScreenPos
                      ,in float inViewZ
                      ,in float4 inColor
                      ,out float4 outColor
                      ,uniform bool bBlendModeAdditive)
{
   float4 depthSample;  
   float2 screenUV = inScreenPos.xy + gSoftParams.zw;
   screenUV *= gSoftParams2.x; 
   //screenUV *= 2.0f;
   asm 
   {
      tfetch2D depthSample, screenUV, gDepthTextureSampler, UnnormalizedTextureCoords = true
   };
         
   float curZ        = 1.0f / ((depthSample.r * gSoftParams.x) + gSoftParams.y);
   float scale       = gSoftParams2.y;
   float alphaFade   = saturate(scale * (curZ - inViewZ));

   outColor = inColor;
   if (bBlendModeAdditive)
      outColor.rgb *= alphaFade;
   else
      outColor.a *= alphaFade;   
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SoftParticlePS(  in  float4 InTexCoord0 : TEXCOORD0
                 ,in  float4 InTexCoord1 : TEXCOORD1
                 ,in  float4 InTexCoord2 : TEXCOORD2
                 ,in  float4 InTexCoord3 : TEXCOORD3 
                 ,in  float4 InColor0    : COLOR0
                 ,in  float3 InWorldPos  : TEXCOORD4
                 ,in  float  InViewZ     : TEXCOORD5
                 ,in  float2 InScreenPos : VPOS0
                 ,out float4 OutColor    : COLOR0
                 ,uniform bool bMultiTextureEnable
                 ,uniform bool bBlendModeAdditive
               )
{
   float4 localColor;
   ParticlePSInternal(InTexCoord0,
                      InTexCoord1,
                      InTexCoord2,
                      InTexCoord3,
                      InColor0,
                      InWorldPos,
                      localColor,
                      bMultiTextureEnable,
                      (bMultiTextureEnable && gMultiTextureEnable3Layers) ? true : false,
                      bBlendModeAdditive);   

   
   ComputeAlphaFade(InScreenPos, InViewZ, localColor, OutColor, bBlendModeAdditive);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SoftParticlePSInternal_Single(  in  float4 InTexCoord0    : TEXCOORD0
                                    ,in  float4 InTexCoord1    : TEXCOORD1
                                    ,in  float4 InTexCoord2    : TEXCOORD2
                                    ,in  float4 InTexCoord3    : TEXCOORD3
                                    ,in  float4 InColor0       : COLOR0
                                    ,in  float3 InLightBufPos  : TEXCOORD4
                                    ,in  float2 InScreenPos    : VPOS0
                                    ,out float4 OutColor       : COLOR0
                                    ,uniform bool bUseLightBuffer
                                    ,uniform bool bUseIntensityMask
                                    ,uniform bool bBlendModeAdditive
                       )
{
   float4 localColor;
   ParticlePSInternal_Single(InTexCoord0, InTexCoord1, InTexCoord2, InTexCoord3, InColor0, InLightBufPos, localColor, bUseLightBuffer, bUseIntensityMask);

   ComputeAlphaFade(InScreenPos, InTexCoord0.w, localColor, OutColor, bBlendModeAdditive);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SoftParticlePSInternal_Single_Simple( in  float4 InTexCoord0 : TEXCOORD0
                                          ,in  float4 InTexCoord1 : TEXCOORD1
                                          ,in  float4 InColor0    : COLOR0
                                          ,in  float2 InScreenPos : VPOS0
                                          ,out float4 OutColor    : COLOR0
                                          ,uniform bool bBlendModeAdditive
                       )
{
   float4 localColor;
   ParticlePSInternal_Single_Simple(InTexCoord0, InTexCoord1, InColor0, localColor);

   ComputeAlphaFade(InScreenPos, InTexCoord0.w, localColor, OutColor, bBlendModeAdditive);   
   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SoftParticlePSInternal_Single_LightBuffer(  in  float4 InTexCoord0    : TEXCOORD0
                                            ,in  float4 InTexCoord1    : TEXCOORD1
                                            ,in  float3 InLightBufPos  : TEXCOORD2
                                            ,in  float4 InColor0       : COLOR0
                                            ,in  float2 InScreenPos : VPOS0
                                            ,out float4 OutColor       : COLOR0
                                            ,uniform bool bBlendModeAdditive
                       )
{
   float4 localColor;
   ParticlePSInternal_Single_LightBuffer(InTexCoord0, InTexCoord1, InLightBufPos, InColor0, localColor);

   ComputeAlphaFade(InScreenPos, InTexCoord0.w, localColor, OutColor, bBlendModeAdditive);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SoftParticlePSInternal_Single_IntensityMask(  in  float4 InTexCoord0    : TEXCOORD0
                                              ,in  float4 InTexCoord1    : TEXCOORD1
                                              ,in  float4 InTexCoord2    : TEXCOORD2
                                              ,in  float4 InColor0       : COLOR0
                                              ,in  float2 InScreenPos : VPOS0
                                              ,out float4 OutColor       : COLOR0
                                              ,uniform bool bBlendModeAdditive
                       )
{   
   float4 localColor;
   ParticlePSInternal_Single_IntensityMask(InTexCoord0, InTexCoord1, InTexCoord2, InColor0, localColor);
   ComputeAlphaFade(InScreenPos, InTexCoord0.w, localColor, OutColor, bBlendModeAdditive);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SoftParticlePSInternal_Single_LightBuffer_IntensityMask(  in  float4 InTexCoord0    : TEXCOORD0
                                              ,in  float4 InTexCoord1    : TEXCOORD1
                                              ,in  float4 InTexCoord2    : TEXCOORD2
                                              ,in  float3 InLightBufPos  : TEXCOORD3
                                              ,in  float4 InColor0       : COLOR0
                                              ,in  float2 InScreenPos : VPOS0
                                              ,out float4 OutColor       : COLOR0
                                              ,uniform bool bBlendModeAdditive
                       )
{   
   float4 localColor;
   ParticlePSInternal_Single_LightBuffer_IntensityMask(InTexCoord0, InTexCoord1, InTexCoord2, InLightBufPos, InColor0, localColor);
   ComputeAlphaFade(InScreenPos, InTexCoord0.w, localColor, OutColor, bBlendModeAdditive);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParticleDistortionPS(  in  float4 InTexCoord0 : TEXCOORD0                 
                           ,in  float4 InTexCoord1 : TEXCOORD1
                           ,in  float4 InTexCoord2 : TEXCOORD2                 
                           ,in  float4 InTexCoord3 : TEXCOORD3
                           ,in  float4 InColor0    : COLOR0
                           ,out float4 OutColor    : COLOR0                 
               )
{
   float4 textureSample0 = sampleTexture(gDiffuseArraySampler, InTexCoord0.xy, InTexCoord3.x);
   //-- range expand
   OutColor = (textureSample0 * 2.0f) - 1.0f;
   
   float4 intensityMaskSample;
   if (gIntensityMaskToggle)
   {
      intensityMaskSample = sampleTexture(gIntensityArraySampler, InTexCoord1.zw, InTexCoord3.w) * 16.0f;
      InTexCoord2 *= intensityMaskSample;
   }
   
   //-- multiply intensity into rgb but not alpha
   OutColor.xy *= InTexCoord2.xy;// * 8.0f;
   OutColor.xyz *= InColor0.a;
   
   //-- set output alpha to non-zero if the vector is non-zero so alpha test works properly
   OutColor.a = abs(OutColor.x) + abs(OutColor.y);
}

//----------------------------------------------------------------------------
// Techniques
//----------------------------------------------------------------------------
technique particleDefault
{   
    pass AlphaBlend
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_Simple();
    }

    pass AlphaBlend_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_LightBuffer();
    }

    pass AlphaBlend_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_IntensityMask();
    }

    pass AlphaBlend_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer_IntensityMask(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_LightBuffer_IntensityMask();
    }
    
    pass Additive
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_Simple();      
    }

    pass Additive_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_LightBuffer();
    }

    pass Additive_IntensityMask
    {            
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_IntensityMask();            
    }

    pass Additive_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer_IntensityMask(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_LightBuffer_IntensityMask();      
    }
        
    /*
    pass Subtractive
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Single_Simple();      
    }

    pass Subtractive_LightBuffer
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Single_LightBuffer();
    }

    pass Subtractive_IntensityMask
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Single_IntensityMask();      
    }

    pass Subtractive_LightBuffer_IntensityMask
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer_IntensityMask(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Single_LightBuffer_IntensityMask();      
    }
    */

    pass Distortion
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 ParticleDistortionPS();            
    }
    
    /*
    pass PremultipliedAlpha
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_Simple();
    }

    pass PremultipliedAlpha_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_LightBuffer();
    }

    pass PremultipliedAlpha_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_IntensityMask();
    }

    pass PremultipliedAlpha_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer_IntensityMask(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Single_LightBuffer_IntensityMask();
    }
    */
}

//----------------------------------------------------------------------------
// Techniques
//----------------------------------------------------------------------------
technique particleMultiTexture
{
    pass AlphaBlend
    {
      VertexShader      = compile vs_3_0 ParticleVS(false);
      PixelShader       = compile ps_3_0 ParticlePS(true, false);
    }
    
    pass Additive
    {
      VertexShader      = compile vs_3_0 ParticleVS(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePS(true, true);
    }
    
    pass Subtractiven
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 ParticlePS(true, false);      
    }

    pass Distortion
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 ParticleDistortionPS();            
    }
    
    pass PremultipliedAlpha
    {
      VertexShader      = compile vs_3_0 ParticleVS(true);
      PixelShader       = compile ps_3_0 ParticlePS(true, false);
    }

    //== Multi 2
    pass Multi2_AlphaBlend
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple();
    }

    pass Multi2_AlphaBlend_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer();
    }

    pass Multi2_AlphaBlend_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_IntensityMask(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_IntensityMask(false);
    }

    pass Multi2_AlphaBlend_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer_IntensityMask(false);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer_IntensityMask(false);
    }
    
    pass Multi2_Additive
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple();      
    }

    pass Multi2_Additive_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer();
    }

    pass Multi2_Additive_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_IntensityMask(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_IntensityMask(true);      
    }

    pass Multi2_Additive_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer_IntensityMask(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer_IntensityMask(true);      
    }
        
    /*
    pass Multi2_Subtractive
    {
      VertexShader = compile vs_3_0 ParticleVS_Multi2_Simple(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Multi2_Simple();      
    }

    pass Multi2_Subtractive_LightBuffer
    {
      VertexShader = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer();
    }

    pass Multi2_Subtractive_IntensityMask
    {
      VertexShader = compile vs_3_0 ParticleVS_Multi2_Simple_IntensityMask(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Multi2_Simple_IntensityMask(false);      
    }

    pass Multi2_Subtractive_LightBuffer_IntensityMask
    {
      VertexShader = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer_IntensityMask(false);
      PixelShader  = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer_IntensityMask(false);      
    }
    */

    pass Multi2_Distortion
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 ParticleDistortionPS();            
    }
    
    /*
    pass Multi2_PremultipliedAlpha
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple();
    }

    pass Multi2_PremultipliedAlpha_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer();
    }

    pass Multi2_PremultipliedAlpha_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_IntensityMask(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_IntensityMask(false);
    }

    pass Multi2_PremultipliedAlpha_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Multi2_Simple_LightBuffer_IntensityMask(true);
      PixelShader       = compile ps_3_0 ParticlePSInternal_Multi2_Simple_LightBuffer_IntensityMask(false);
    }
    */
}

//----------------------------------------------------------------------------
//Soft Particles
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Techniques
//----------------------------------------------------------------------------
technique softParticleDefault
{
    pass AlphaBlend
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple(false);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_Simple(false);
    }

    pass AlphaBlend_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(false);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_LightBuffer(false);
    }

    pass AlphaBlend_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(false);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_IntensityMask(false);
    }

    pass AlphaBlend_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer_IntensityMask(false);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_LightBuffer_IntensityMask(false);       
    }
    
    pass Additive
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_Simple(true);      
    }

    pass Additive_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_LightBuffer(true);
    }

    pass Additive_IntensityMask
    {            
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_IntensityMask(true);      
    }

    pass Additive_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer_IntensityMask(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_LightBuffer_IntensityMask(true);      
    }
        
    /*
    pass Subtractive
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple(false);
      PixelShader  = compile ps_3_0 SoftParticlePSInternal_Single_Simple(false);      
    }

    pass Subtractive_LightBuffer
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(false);
      PixelShader  = compile ps_3_0 SoftParticlePSInternal_Single_LightBuffer(false);
    }

    pass Subtractive_IntensityMask
    {
      VertexShader = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(false);
      PixelShader  = compile ps_3_0 SoftParticlePSInternal_Single_IntensityMask(false);      
    }

    pass Subtractive_LightBuffer_IntensityMask
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 SoftParticlePSInternal_Single(true, true, false);      
    }
    */

    pass Distortion
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 ParticleDistortionPS();            
    }
    
    /*
    pass PremultipliedAlpha
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple(true);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_Simple(false);
    }

    pass PremultipliedAlpha_LightBuffer
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_LightBuffer(true);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_LightBuffer(false);
    }

    pass PremultipliedAlpha_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS_Single_Simple_IntensityMask(true);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single_IntensityMask(false);
    }

    pass PremultipliedAlpha_LightBuffer_IntensityMask
    {
      VertexShader      = compile vs_3_0 ParticleVS(true);
      PixelShader       = compile ps_3_0 SoftParticlePSInternal_Single(true, true, false);
    } 
    */
}

//----------------------------------------------------------------------------
// Techniques
//----------------------------------------------------------------------------
technique softParticleMultiTexture
{
    pass AlphaBlend
    {
      VertexShader      = compile vs_3_0 ParticleVS(false);
      PixelShader       = compile ps_3_0 SoftParticlePS(true, false);      
    }
    
    pass Additive
    {
      VertexShader      = compile vs_3_0 ParticleVS(true); //-- set to true to premultiply the rgb by the alpha
      PixelShader       = compile ps_3_0 SoftParticlePS(true, true);            
    }
    
    pass Subtractiven
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 SoftParticlePS(true, false);            
    }

    pass Distortion
    {
      VertexShader = compile vs_3_0 ParticleVS(false);
      PixelShader  = compile ps_3_0 ParticleDistortionPS();            
    }
    
    pass PremultipliedAlpha
    {
      VertexShader      = compile vs_3_0 ParticleVS(true);
      PixelShader       = compile ps_3_0 SoftParticlePS(true, false);
    }
}