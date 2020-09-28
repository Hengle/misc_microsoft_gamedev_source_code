#include "skinning.inc"

bool gTex1 : register(TEX1_REG);
bool gTex2 : register(TEX2_REG);

sampler gHeightfieldSampler : register(s0) = sampler_state
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
                                              
float4x4 gWorldToHeightfield : register(WORLD_TO_HEIGHTFIELD_REG);
// x - yScale
// y - yOfs
// z - yLimit (+ and -)
// w - yOffset
float4 gHeightfieldYScaleOfs : register(HEIGHTFIELD_Y_SCALE_OFS_REG);


struct VertexOut
{                                             
    float4 Position           : POSITION;                

    float3 Normal             : NORMAL0;
    
#ifdef BUMP    
    float3 Tangent            : TANGENT;
    float3 Binormal           : BINORMAL;
#endif    

    float4 UV                 : TEXCOORD0;    

    // W contains overall AO factor
    float4 PixelXForm         : COLOR0;
    
    // WorldPos.W contains instance's light array index.
    float4 WorldPos           : TEXCOORD1;

#ifdef DIR_SHADOWING    
    float3 ShadowMapCoords0   : TEXCOORD2;
#endif    

    // XY = fog params, ZW = UV2
    float4 FogDensities       : TEXCOORD5;
        
    float3 AmbientSum         : TEXCOORD6;
};                                             

static float2 heightfieldDepthToY(float2 s)
{
   return s * gHeightfieldYScaleOfs.x + float2(gHeightfieldYScaleOfs.y, gHeightfieldYScaleOfs.y);
}

static float4 fetchUVOffsets0(float matrixIndex)
{
   float4 uvOffsets;
   
   asm
   {
      vfetch uvOffsets, matrixIndex, texcoord4, UseTextureCache=true
   };
        
   return uvOffsets;
}

static float4 fetchUVOffsets1(float matrixIndex)
{
   float4 uvOffsets;
   
   asm
   {
      vfetch uvOffsets, matrixIndex, texcoord5, UseTextureCache=true
   };
        
   return uvOffsets;
}

static float fetchOverallAO(float matrixIndex)
{
   float4 overallAO;
   
   asm
   {
      vfetch overallAO, matrixIndex, texcoord6, UseTextureCache=true
   };
        
   return overallAO.x;
}

VertexOut vsMain(int Index : INDEX, uniform bool rigid, uniform bool tex1, uniform bool tex2, uniform bool terrainConform)
{                                              
   int meshIndex = (Index + .5f) * gInstanceControl.y;
   float fetchIndex = Index - meshIndex * gInstanceControl.z;

   float4 Position;
   float4 Normal;
   float4 Tangent;
   float4 UV0;
   float4 UV1 = 0;
   float4 UV2 = 0;
   int4 InIndices;
   float4 InWeights;

   asm
   {
      vfetch Position, fetchIndex, position

      vfetch Normal, fetchIndex, normal
    
#ifdef BUMP    
      vfetch Tangent, fetchIndex, tangent
#endif    

      vfetch UV0, fetchIndex, texcoord0    
   };

   if ((tex1) && (gTex1))
   {
      asm
      {
         vfetch UV1, fetchIndex, texcoord1    
      };
   }
   
   if ((tex2) && (gTex2))
   {
      asm
      {
         vfetch UV2, fetchIndex, texcoord2 
      };
   }

   if (!rigid)
   {
      asm
      {
         vfetch InIndices, fetchIndex, blendindices
         vfetch InWeights, fetchIndex, blendweight
      };         
   }
      
   float instanceBoneMatrices = gInstanceData[meshIndex].w;
        
   float3 bindToWorldRot0;
   float3 bindToWorldRot1;
   float3 bindToWorldRot2;
   float3 bindToWorldTran;

   if (rigid)
      RetrieveMatrix(gInstanceControl.x + instanceBoneMatrices + 1.0, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);
   else
      CalcBindToWorldMatrix(instanceBoneMatrices + 1.0, InIndices, InWeights, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);

   float4 worldPos = mulVec(Position, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);
   worldPos.w = 1;
   
   // Terrain Conform
   if (terrainConform)
   {
      float4 centerHeightfieldPos = mul(float4(bindToWorldTran, 1.0), gWorldToHeightfield);
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
      if (abs(bindToWorldTran.y - centerLoHiY.y) < abs(bindToWorldTran.y - centerY))
         centerY = centerLoHiY.y;
               
      float sampleY = sampleLoHiY.x;
      if (abs(centerY - sampleLoHiY.y) < abs(centerY - sampleLoHiY.x))
         sampleY = sampleLoHiY.y;
         
     // if (sampleY < (centerY - gHeightfieldYScaleOfs.z))
     //    sampleY = centerY - gHeightfieldYScaleOfs.z;
     // else if (sampleY > (centerY + gHeightfieldYScaleOfs.z))
     //    sampleY = centerY + gHeightfieldYScaleOfs.z;

      float yOffset = gHeightfieldYScaleOfs.w;
      worldPos.y = sampleY + yOffset;
   }

   VertexOut Out;                                      

   Out.Position = mul(worldPos, gWorldToProj);

   Out.WorldPos.xyz = worldPos;
   Out.WorldPos.w = meshIndex * gInstanceControl.w;

#ifdef DIR_SHADOWING   
   Out.ShadowMapCoords0 = mul(worldPos, gDirShadowWorldToTex);
#endif   

   Out.Normal = normalize(mulVecNoTran(Normal, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2));
      
#ifdef BUMP
   Out.Tangent = normalize(mulVecNoTran(Tangent, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2));
   Out.Binormal = cross(Out.Tangent, Out.Normal);
         
   float3 tangent = Tangent;   
   float tangentLenSqr = dot(tangent, tangent);

   if (tangentLenSqr < 0.5f)
   {
      //Out.Tangent *= 2.0f;
      //Out.Binormal *= -2.0f;
      
      Out.Binormal *= -1.0f;
   }
#endif
   
   Out.UV.xy = UV0;
   Out.UV.zw = UV1.xyxy;
   
   Out.UV += fetchUVOffsets0(instanceBoneMatrices);
      
   Out.PixelXForm.xyz = gInstanceData[meshIndex];
   Out.PixelXForm.w = fetchOverallAO(instanceBoneMatrices); // also called "AO tint value" in C++
   
   Out.FogDensities.x = computeRadialFogDensity(worldPos);
   Out.FogDensities.y = computePlanarFogDensity(worldPos);
   
   // Output 3rd UV
   float4 UVOffsets1 = fetchUVOffsets1(instanceBoneMatrices);
   Out.FogDensities.z = UV2.x + UVOffsets1.x;
   Out.FogDensities.w = UV2.y + UVOffsets1.y;
   
   Out.AmbientSum = computeSHFillLighting(Out.Normal);

   return Out;                                
}                                            

struct ShadowVertexOut
{                                             
    float4 Position           : POSITION;                
    float  Depth              : TEXCOORD0;
};            

ShadowVertexOut vsShadowMain(int Index : INDEX, uniform bool rigid, uniform bool dualParaboloid)
{                                              
   int meshIndex = (Index + .5f) * gInstanceControl.y;
   float fetchIndex = Index - meshIndex * gInstanceControl.z;

   float4 Position;
   int4 InIndices;
   float4 InWeights;

   asm
   {
      vfetch Position, fetchIndex, position
   };

   if (!rigid)
   {
      asm
      {    
         vfetch InIndices, fetchIndex, blendindices
         vfetch InWeights, fetchIndex, blendweight
      };         
   }
      
   float instanceBoneMatrices = gInstanceData[meshIndex].w;
        
   float3 bindToWorldRot0;
   float3 bindToWorldRot1;
   float3 bindToWorldRot2;
   float3 bindToWorldTran;
   
   if (rigid)
      RetrieveMatrix(gInstanceControl.x + instanceBoneMatrices + 1.0, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);
   else
      CalcBindToWorldMatrix(instanceBoneMatrices + 1.0, InIndices, InWeights, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);

   ShadowVertexOut result;
   float4 worldPos = mulVec(Position, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);
   worldPos.w = 1;
         
   result.Position = mul(worldPos, gWorldToProj);
   result.Depth = result.Position.z;
   
   if (dualParaboloid)
   {
      result.Depth = result.Position.z;
   
      float3 normViewPos = result.Position;
      float oolen = 1.0f / sqrt(mul(normViewPos, normViewPos));
      float len = 1.0f / oolen;
      normViewPos = normViewPos * oolen + float3(0.0f, 0.0f, 1.0f);

      float ooz = 1.0f / normViewPos.z;
      result.Position.x = normViewPos.x * ooz;
      result.Position.y = normViewPos.y * ooz;
      result.Position.z = (len - DUAL_PARABOLOID_NEAR_PLANE) / (DUAL_PARABOLOID_FAR_PLANE - DUAL_PARABOLOID_NEAR_PLANE);
      result.Position.w = 1.0f;
   }
         
   return result;
}                                            
                                        
struct AlphaShadowVertexOut
{                                             
    float4 Position           : POSITION;                
    float4 UV                 : TEXCOORD0;    
    float  Depth              : TEXCOORD1;
    float4 UV2                : TEXCOORD5;
};              

AlphaShadowVertexOut vsShadowMainAlphaTest(int Index : INDEX, uniform bool rigid, uniform bool tex1, uniform bool tex2, uniform bool dualParaboloid)
{                                              
   int meshIndex = (Index + .5f) * gInstanceControl.y;
   float fetchIndex = Index - meshIndex * gInstanceControl.z;

   float4 Position;
   int4 InIndices;
   float4 InWeights;
   float4 UV0;
   float4 UV1 = 0.0;
   float4 UV2 = 0.0;

   asm
   {
      vfetch Position, fetchIndex, position
      vfetch UV0, fetchIndex, texcoord0    
   };
   
   if ((tex1) && (gTex1))
   {
      asm
      {
         vfetch UV1, fetchIndex, texcoord1
      };
   }
   
   if ((tex2) && (gTex2))
   {
      asm
      {
         vfetch UV2, fetchIndex, texcoord2 
      };
   }

   if (!rigid)
   {   
      asm
      {
         vfetch InIndices, fetchIndex, blendindices
         vfetch InWeights, fetchIndex, blendweight
      };         
   }      
      
   float instanceBoneMatrices = gInstanceData[meshIndex].w;
        
   float3 bindToWorldRot0;
   float3 bindToWorldRot1;
   float3 bindToWorldRot2;
   float3 bindToWorldTran;
   
   if (rigid)
      RetrieveMatrix(gInstanceControl.x + instanceBoneMatrices + 1.0, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);
   else
      CalcBindToWorldMatrix(instanceBoneMatrices + 1.0, InIndices, InWeights, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);

   float4 worldPos = mulVec(Position, bindToWorldRot0, bindToWorldRot1, bindToWorldRot2, bindToWorldTran);
   worldPos.w = 1;

   AlphaShadowVertexOut result;
   result.Position =  mul(worldPos, gWorldToProj);
      
   result.UV.xy = UV0;
   result.UV.zw = UV1.xyxy;
   
   result.Depth = result.Position.z;
   
   // Output 3rd UV
   result.UV2 = float4(0.0, 0.0, UV2.x, UV2.y);
   
   if (dualParaboloid)
   {
      result.Depth = result.Position.z;
   
      float3 normViewPos = result.Position;
      float oolen = 1.0f / sqrt(mul(normViewPos, normViewPos));
      float len = 1.0f / oolen;
      normViewPos = normViewPos * oolen + float3(0.0f, 0.0f, 1.0f);

      float ooz = 1.0f / normViewPos.z;
      result.Position.x = normViewPos.x * ooz;
      result.Position.y = normViewPos.y * ooz;
      result.Position.z = (len - DUAL_PARABOLOID_NEAR_PLANE) / (DUAL_PARABOLOID_FAR_PLANE - DUAL_PARABOLOID_NEAR_PLANE);
      result.Position.w = 1.0f;
   }
   
   return result;
}                                            


