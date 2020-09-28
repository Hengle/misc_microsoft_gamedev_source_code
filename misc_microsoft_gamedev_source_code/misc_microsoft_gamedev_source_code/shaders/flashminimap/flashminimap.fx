#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

#define BLACK_MAP

sampler diffuseSampler : register (s0);

sampler maskSampler = sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
   AddressW  = CLAMP;
};

sampler backgroundSampler = sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
   AddressW  = CLAMP;
};

sampler visTexture = sampler_state
{
   MinFilter = LINEAR;
   MagFilter = LINEAR;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
   AddressW  = CLAMP;
};

sampler visPointTexture = sampler_state
{
   MinFilter = POINT;
   MagFilter = POINT;
   AddressU  = CLAMP;
   AddressV  = CLAMP;
   AddressW  = CLAMP;
};

float4x4 gTransform;
float    gRotationAngle;
float4   gColor;
float4   gMinimapFogColor;
float4   gFogScalar;

const float2 cCorner[4]   = {float2(-1,  1), float2( -1,-1), float2(1 ,-1), float2( 1, 1)};
const float2 cCornerUV[4] = {float2( 0,  0), float2(  0, 1), float2(1 , 1), float2( 1, 0)};

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
float2 ComputeCorner(int vertIndex)
{   
   int cornerIndex = getCornerIndex(vertIndex);
   return cCorner[cornerIndex];
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
float4 ComputeBillBoardCorner(float4 pos, float2 scale, int index)
{   
   float2 corner = ComputeCorner(index); 
   corner.xy *= scale.xy;
   return float4(pos.x+corner.x, pos.y+corner.y, 0, 1);         
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float2 computeRotation(float2 uv, float angle)
{
   // Compute sin and cos of the rotation angle.
	float fSin, fCos;
	sincos(angle, fSin, fCos);
	
	// Rotate the corner vector by the rotation angle.
	float2 vRotated = uv;
   vRotated.x = uv.x * fCos - uv.y * fSin;   			
   vRotated.y = uv.x * fSin + uv.y * fCos;
   return vRotated;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void iconVS( in  float4 inPos    : POSITION0,
             in  float2 inSize   : PSIZE0,             
             in  float4 inTex0   : TEXCOORD0,
             in  float4 inColor0 : COLOR0,
                                       
             out float4 outPos   : POSITION0,
             out float4 outColor0: COLOR0,
             out float4 outTex0  : TEXCOORD0,
             out float  outSize  : PSIZE0
           )
{
   outPos    = mul(inPos, gTransform);
   outSize   = inSize.x;
   outTex0   = inTex0;
   outColor0 = inColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void mapPS( in  float2 inSpriteUV : SPRITETEXCOORD,
            in  float4 inTex0     : TEXCOORD0,
            in  float4 inColor0   : COLOR0,
            out float4 outColor0  : COLOR0
           )
{
   float2 uv;
   //inTex0.xy = computeRotation(inTex0.xy, gRotationAngle, float4(inTex0.z,inTex0.w,0,0));
   float2 offset;
   offset.x = lerp(-inTex0.z, inTex0.z, inSpriteUV.x);
   offset.y = lerp(-inTex0.w, inTex0.w, inSpriteUV.y);

   offset = computeRotation(offset, -gRotationAngle);
   uv = inTex0 + offset;

   float4 maskSample = tex2D(maskSampler, inSpriteUV);

   float4 textureSample;
   textureSample = tex2D(diffuseSampler, uv);
   
   float4 visTextureSample = tex2D(visTexture, uv);   

   float4 darkenedSample = textureSample;
   if (uv.x >=0.0f && uv.x <1.0f && uv.y >=0.0f && uv.y <=1.0f)
   {       
#ifdef BLACK_MAP      
      darkenedSample.rgb *= 1.0f - visTextureSample.a;
      textureSample.rgb = lerp(darkenedSample.rgb, textureSample.rgb, gFogScalar.x);
#else      
      darkenedSample.rgb *= gFogScalar.x;
      if (visTextureSample.a >= 0.005f)
         textureSample = darkenedSample;
#endif 

      float4 overlay = tex2D(backgroundSampler, uv);// * inColor0;
      textureSample.rgb = lerp(textureSample.rgb, overlay.rgb, overlay.a);
   }
   else
   {
#ifdef BLACK_MAP
      textureSample.rgb = lerp(float3(0,0,0), darkenedSample.rgb, gFogScalar.y);
#else
      textureSample.rgb *= gFogScalar.y;
#endif
   }


   //textureSample.rgb = 1.0f - visTextureSample.a;
   textureSample *= maskSample.a;      
   outColor0 = textureSample; // * inColor0;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void iconPS( in  float2 inSpriteUV : SPRITETEXCOORD,
             in  float4 inTex0     : TEXCOORD0,
             in  float4 inColor0   : COLOR0,
             out float4 outColor0  : COLOR0
           )
{
   float2 uv;
   uv.x = inTex0.x + (inSpriteUV.x*inTex0.z);
   uv.y = inTex0.y + (inSpriteUV.y*inTex0.w);
   float4 textureSample = tex2D(diffuseSampler, uv);
   outColor0 = inColor0 * 1.25f * textureSample;// * inColor0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void generateVisVS(in  float4 inPos    : POSITION0,
                   in  float2 inSize   : PSIZE0,
                   out float4 outPos   : POSITION0,
                   out float  outSize  : PSIZE0,
                   uniform bool bUseSizeY
                   )
{
   outPos  = mul(inPos, gTransform);
   if (bUseSizeY)
      outSize = inSize;
   else
      outSize = inSize.x;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void generateVisPS(in      float2 inSpriteUV : SPRITETEXCOORD,
                   out     float4 outColor0  : COLOR0,
                   uniform bool   bUseTexture,
                   uniform bool   bUsePointTexture                         
                   )
{
   if (bUseTexture)
   {
      float4 textureSample;
      if (bUsePointTexture)
         textureSample = tex2D(visPointTexture, inSpriteUV);
      else
         textureSample = tex2D(visTexture, inSpriteUV);

      outColor0 = textureSample * gColor;
   }
   else
   {
      outColor0 = gColor;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void generateVisibilityVS(in  int Index             : INDEX,                          
                          out float4 OutPosition    : POSITION0,
                          out float4 OutTexCoord0   : TEXCOORD0)
{
   int vertexIndex = (Index+0.5f) / 4;

   float4 InPosition = fetchPosition(vertexIndex);   
   float4 InScale    = fetchTexCoord0(vertexIndex);

   /*
   float2 corner   = ComputeCorner(Index); 
   corner.xy *= InScale.xy;

   OutPosition = float4(InPosition.x+corner.x, InPosition.y+corner.y, 0, 1);
   */

   OutPosition  = ComputeBillBoardCorner(InPosition, InScale, Index);   
   OutPosition  = mul(OutPosition, gTransform);      
   OutTexCoord0 = ComputeTexCoords(Index);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void generateVisibilityPS(in  float4 InTexCoord0 : TEXCOORD0,
                          out float4 OutColor0   : COLOR0,
                          uniform bool bUseTexture,
                          uniform bool bUsePointTexture )              
{   
   if (bUseTexture)
   {
      float4 textureSample;
      if (bUsePointTexture)
         textureSample = tex2D(visPointTexture, InTexCoord0);
      else
         textureSample = tex2D(visTexture, InTexCoord0);

      OutColor0 = textureSample * gColor;
   }
   else
   {
      OutColor0 = gColor;
   }  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique iconTechnique
{
   pass Default
   {
      VertexShader             = compile vs_3_0 iconVS();
      PixelShader              = compile ps_3_0 iconPS();

      PointSpriteEnable        = true;
      PointSize_Min            = 1.0;
      PointSize_Max            = 256.0;
      ViewPortEnable           = false;
      ZEnable                  = false;
      AlphaBlendEnable         = true;
      BlendOp                  = Add;
      SrcBlend                 = SrcAlpha;
      DestBlend                = InvSrcAlpha;
      AlphaTestEnable          = true;
      AlphaFunc                = Greater;
      AlphaRef                 = 0.0;
      CullMode                 = None;
      HighPrecisionBlendEnable = true;
   }

   pass Map
   {
      VertexShader             = compile vs_3_0 iconVS();
      PixelShader              = compile ps_3_0 mapPS();

      PointSpriteEnable        = true;
      PointSize_Min            = 1.0;
      PointSize_Max            = 256.0;
      ViewPortEnable           = false;
      ZEnable                  = false;
      AlphaBlendEnable         = true;
      BlendOp                  = Add;
      SrcBlend                 = SrcAlpha;
      DestBlend                = InvSrcAlpha;
      AlphaTestEnable          = true;
      AlphaFunc                = Greater;
      AlphaRef                 = 0.0;
      CullMode                 = None;
      HighPrecisionBlendEnable = true;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
technique visibilityTechnique
{
   pass 
   {
      VertexShader      = compile vs_3_0 generateVisVS(false);
      PixelShader       = compile ps_3_0 generateVisPS(true, false);

      PointSpriteEnable = true;
      PointSize_Min     = 1.0;
      PointSize_Max     = 256.0;
      ViewPortEnable    = false;
      ZEnable           = false;
      AlphaBlendEnable  = false;
      AlphaTestEnable   = false;
      CullMode          = None;
   }

   pass
   {
      VertexShader      = compile vs_3_0 generateVisVS(false);
      PixelShader       = compile ps_3_0 generateVisPS(true, true);

      PointSpriteEnable = true;
      PointSize_Min     = 1.0;
      PointSize_Max     = 256.0;
      ViewPortEnable    = false;
      ZEnable           = false;
      AlphaBlendEnable  = false;
      AlphaTestEnable   = true;
      AlphaFunc         = Greater;
      AlphaRef          = 0.0;
      CullMode          = None;
    }

   pass
   {
      VertexShader      = compile vs_3_0 generateVisVS(false);
      PixelShader       = compile ps_3_0 generateVisPS(true, true);

      PointSpriteEnable = true;
      PointSize_Min     = 1.0;
      PointSize_Max     = 256.0;
      ViewPortEnable    = false;
      ZEnable           = false;
      BlendOp           = Max;
      SrcBlend          = One;
      DestBlend         = One;
      AlphaBlendEnable  = true;
      AlphaTestEnable   = true;
      AlphaFunc         = Greater;
      AlphaRef          = 0.0;
      CullMode          = None;
   }

   //square visibility
   pass
   {
      VertexShader      = compile vs_3_0 generateVisVS(true);
      PixelShader       = compile ps_3_0 generateVisPS(false, false);

      PointSpriteEnable = true;
      PointSize_Min     = 1.0;
      PointSize_Max     = 256.0;
      ViewPortEnable    = false;
      ZEnable           = false;
      AlphaBlendEnable  = false;
      AlphaTestEnable   = true;
      AlphaFunc         = Greater;
      AlphaRef          = 0.0;
      CullMode          = None;
   }


   pass
   {
      VertexShader      = compile vs_3_0 generateVisVS(true);
      PixelShader       = compile ps_3_0 generateVisPS(false, false);

      PointSpriteEnable = true;
      PointSize_Min     = 1.0;
      PointSize_Max     = 256.0;
      ViewPortEnable    = false;
      ZEnable           = false;
      BlendOp           = Max;
      SrcBlend          = One;
      DestBlend         = One;
      AlphaBlendEnable  = true;
      AlphaTestEnable   = true;
      AlphaFunc         = Greater;
      AlphaRef          = 0.0;
      CullMode          = None;
   }

   //square blocker
   pass
   {
      VertexShader      = compile vs_3_0 generateVisibilityVS();
      PixelShader       = compile ps_3_0 generateVisibilityPS(false, false);

      //PointSpriteEnable = true;
      //PointSize_Min     = 1.0;
      //PointSize_Max     = 256.0;
      ViewPortEnable    = false;
      ZEnable           = false;
      BlendOp           = Max;
      SrcBlend          = One;
      DestBlend         = One;
      AlphaBlendEnable  = true;
      AlphaTestEnable   = true;
      AlphaFunc         = Greater;
      AlphaRef          = 0.0;
      CullMode          = None;
   }
}
