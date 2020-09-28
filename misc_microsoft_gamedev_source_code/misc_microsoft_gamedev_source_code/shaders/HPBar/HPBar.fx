#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

//-- Diffuse Texture
texture diffuseTextureArray;
sampler gDiffuseSampler = sampler_state
{
   Texture    = (diffuseTextureArray);

   AddressU   = WRAP;
   AddressV   = WRAP;
   AddressW   = WRAP;
   
   MipFilter  = LINEAR;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;    
   
   SEPARATEZFILTERENABLE = TRUE; 
   MinFilterZ = POINT;
   MagFilterZ = POINT;
};

texture maskTextureArray;
sampler gMaskSampler = sampler_state
{
   Texture    = (maskTextureArray);
   AddressU   = WRAP;
   AddressV   = WRAP;
   AddressW   = WRAP;
   
   MipFilter  = LINEAR;
   MinFilter  = LINEAR;
   MagFilter  = LINEAR;    
   
   SEPARATEZFILTERENABLE = TRUE; 
   MinFilterZ = POINT;
   MagFilterZ = POINT;
};

const float2 cOffset[4]   = {float2(-0.5f, -0.5f), float2(0.5f, -0.5f), float2(0.5f, 0.5f), float2(-0.5f, 0.5f)};
const float2 cCornerUV[4] = {float2( 0,  1), float2(  1, 1), float2(1 , 0), float2( 0, 0)};

struct VertexOut
{
    float4 Position   : POSITION0;
    float3 UV         : TEXCOORD0;
    float4 Color      : COLOR0;
};

float4 gAmmoBarColor = float4(0,0,0,0);
float4 gShieldBarColor = float4(0,0,0,0);


//============================================================================
//============================================================================
int getCornerIndex(int vertIndex)
{
    int iDiv = (vertIndex + 0.5f) / 4;
    int iMod = vertIndex - (iDiv * 4);
    return iMod;
}

//============================================================================
//============================================================================
float4 fetchPosition(int index)
{
    float4 pos;
    asm
    {
        vfetch pos, index, position0
    };   
    return pos;
}

//============================================================================
//============================================================================
float2 fetchSize(int index)
{
    float4 size;
    asm
    {
        vfetch size, index, texcoord0
    };   
    return size.xy;
}

//============================================================================
//============================================================================
float4 fetchColor0(int index)
{
    float4 color;
    asm
    {
        vfetch color, index, color0
    };   
    return color;
}

//============================================================================
//============================================================================
float4 fetchColor1(int index)
{
    float4 color;
    asm
    {
        vfetch color, index, color1
    };   
    return color;
}
//============================================================================
//============================================================================
float4 fetchColor2(int index)
{
    float4 color;
    asm
    {
        vfetch color, index, color2
    };   
    return color;
}

//============================================================================
//============================================================================
float4 fetchColor3(int index)
{
    float4 color;
    asm
    {
        vfetch color, index, color3
    };   
    return color;
}

//============================================================================
//============================================================================
float2 fetchOffset(int index)
{
    int cornerIndex = getCornerIndex(index);
    return cOffset[cornerIndex];
}

//============================================================================
//============================================================================
float4 fetchTexCoord0(int index)
{
    float4 t;
    asm
    {
        vfetch t, index, texcoord0
    };   
    return t;
}

//============================================================================
//============================================================================
float4 fetchTexCoord1(int index)
{
    float4 t;
    asm
    {
        vfetch t, index, texcoord1
    };   
    return t;
}

//============================================================================
//============================================================================
float4 fetchTexCoord2(int index)
{
    float4 t;
    asm
    {
        vfetch t, index, texcoord2
    };   
    return t;
}

//============================================================================
//============================================================================
float4 fetchTexCoord3(int index)
{
    float4 t;
    asm
    {
        vfetch t, index, texcoord3
    };   
    return t;
}

//============================================================================
//============================================================================
float4 fetchTexCoord4(int index)
{
    float4 t;
    asm
    {
        vfetch t, index, texcoord4
    };   
    return t;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float4 ComputeTexCoords(int index)
{
   int cornerIndex = getCornerIndex(index);
   return float4(cCornerUV[cornerIndex], cCornerUV[cornerIndex]);
}

//============================================================================
//============================================================================
float4 computerVertexPosition(float4 pos, float2 offset, float2 size)
{   
    float4 finalPos = mul(pos, gWorldToProj);
    finalPos.xy += offset * size;

    return finalPos;
}

//============================================================================
//============================================================================
float4 sampleTexture(sampler s, float2 uv, float index)
{
   return tex3D(s, float3(uv, index));
}

//============================================================================
//============================================================================
VertexOut vsHPBar(in int Index : INDEX)
{
    VertexOut Out;

    int vertexIndex   = (Index + 0.5f) / 4;
    float4 InPosition = fetchPosition(vertexIndex);
    float2 InSize     = fetchSize(vertexIndex);
    float4 InColor    = fetchColor0(vertexIndex);
    float2 InOffset   = fetchOffset(Index);
    float3 InUV       = float3(InOffset.xy + 0.5f, InPosition.w);

    InPosition.w = 1.0f;
    Out.Position = computerVertexPosition(InPosition, InOffset, InSize);
    Out.UV       = InUV;
    Out.Color    = InColor;
    return Out;
}

//============================================================================
//============================================================================
float4 psHPBar(float3 UV : TEXCOORD0, float4 Color : COLOR0) : COLOR0
{
    float2 UVdiff = abs(UV.xy - float2(0.5f, 0.5f));
    float  k = dot(UVdiff, UVdiff) * 4.0f;
    float4 outColor;
    if (UV.x > UV.z)
	    outColor = float4(0.2f, 0.2f, 0.2f, 1.0f - k);
	else
        outColor = float4(Color.rgb, 1.0f - k);

    return outColor;
}


//============================================================================
//============================================================================
void vsSquadBar(in  int     Index : INDEX,
                out float4 oPos  : POSITION0,
                out float3 oTex0 : TEXCOORD0,
                out float3 oTex1 : TEXCOORD1,
                out float4 oTex2 : TEXCOORD2,
                out float4 oTex3 : TEXCOORD3,
                out float4 oTex4 : TEXCOORD4,
                out float4 oColor0: COLOR0,
                out float4 oColor1: COLOR1,
                out float4 oColor2: COLOR2,
                out float4 oColor3: COLOR3
                )
{   
   int vertexIndex    = (Index + 0.5f) / 4;
   float4 InPosition  = fetchPosition(vertexIndex);      
   float2 InOffset    = fetchOffset(Index);
   float4 InHPDims    = fetchTexCoord0(vertexIndex);
   float4 InShieldDims= fetchTexCoord1(vertexIndex);
   float4 InAmmoDims  = fetchTexCoord2(vertexIndex);
   float4 InStatus    = fetchTexCoord3(vertexIndex);
   float2 InSize      = fetchTexCoord4(vertexIndex).xy;
   float4 InColor0    = fetchColor0(vertexIndex);
   float4 InColor1    = fetchColor1(vertexIndex);
   float4 InColor2    = fetchColor2(vertexIndex);
   float4 InColor3    = fetchColor3(vertexIndex);


   //-- uvs
   oTex0.xy = ComputeTexCoords(Index);
   oTex0.z  = InPosition.w; // texture array index

#if 1
   InPosition.w = 1.0f;
   float3 rightV   = gViewToWorld._m00_m01_m02 * ((InOffset.x * InSize.x)/* + In2DOffset.x*/);
   float3 upV      = gViewToWorld._m10_m11_m12 * ((InOffset.y * InSize.y)/* + In2DOffset.y*/);   
   float4 finalPos = float4(InPosition + rightV.xyz + upV.xyz, 1.0f);   
   oPos =  mul(finalPos, gWorldToProj);   
#else
   InPosition.w = 1.0f;
   oPos   = computerVertexPosition(InPosition, InOffset, InSize);
#endif

   //-- status
   oTex1 = InStatus.xyz;
         
   oTex2  = InHPDims;
   oTex3  = InShieldDims;
   oTex4  = InAmmoDims;
   oColor0 = InColor0;   
   oColor1 = InColor1;   
   oColor2 = InColor2;   
   oColor3 = InColor3;   
}

//============================================================================
//============================================================================
void psSquadBar(in  float3 inUV0       : TEXCOORD0, //uv
                in  float3 inStatus    : TEXCOORD1,
                in  float4 inHPDims    : TEXCOORD2,
                in  float4 inShieldDims: TEXCOORD3,
                in  float4 inAmmoDims  : TEXCOORD4,
                in  float4 inColor0    : COLOR0,
                in  float4 inColor1    : COLOR1,
                in  float4 inColor2    : COLOR2,
                in  float4 inColor3    : COLOR3,
                out float4 oColor      : COLOR0)                
{
   float4 diffuse = sampleTexture(gDiffuseSampler, inUV0.xy, inUV0.z) * inColor0;
   float4 mask    = sampleTexture(gMaskSampler, inUV0.xy, inUV0.z);
   if ((inUV0.x > inHPDims.x) && (inUV0.x < inHPDims.z) && (inUV0.y > inHPDims.y) && (inUV0.y < inHPDims.w))   
   {      
      float4 barColor;
      if (inUV0.x > inStatus.x)
         barColor = float4(0.1f,0.1f,0.1f,0.5f);
      else
         barColor = inColor1;        

      oColor = lerp(diffuse, barColor, mask.a);            
   }
   else if ((inUV0.x > inShieldDims.x) && (inUV0.x < inShieldDims.z) && (inUV0.y > inShieldDims.y) && (inUV0.y < inShieldDims.w))
   {
      float4 barColor;
      if (inUV0.x > inStatus.y)
         barColor = float4(0.1f,0.1f,0.1f,0.5f);
      else
         barColor = inColor2;        

      oColor = lerp(diffuse, barColor, mask.a);            
   }
   else if ((inUV0.x > inAmmoDims.x) && (inUV0.x < inAmmoDims.z) && (inUV0.y > inAmmoDims.y) && (inUV0.y < inAmmoDims.w))
   {
      float4 barColor;
      if (inUV0.x > inStatus.z)
         barColor = float4(0.1f,0.1f,0.1f,0.5f);
      else
         barColor = inColor3;

      oColor = lerp(diffuse, barColor, mask.a);            
   }
   else
   {
      oColor = diffuse;
   }        
}

//============================================================================
//============================================================================
technique renderHPBar
{
    pass
    {
      VertexShader = compile vs_3_0 vsHPBar();
      PixelShader = compile ps_3_0 psHPBar();

      ZEnable = false;
      AlphaBlendEnable = true;
      AlphaTestEnable = true;
      SrcBlend = srcalpha;
      DestBlend = invsrcalpha;
      BlendOp = add;
      AlphaRef = 0;
      AlphaFunc = greater;
      CullMode = none;
    }
}

//============================================================================
//============================================================================
technique renderSquadHPBar
{
    pass
    {
      VertexShader = compile vs_3_0 vsSquadBar();
      PixelShader = compile ps_3_0 psSquadBar();

      ZEnable = false;
      AlphaBlendEnable = true;
      AlphaTestEnable = true;
      SrcBlend = srcalpha;
      DestBlend = invsrcalpha;
      BlendOp = add;
      AlphaRef = 0;
      AlphaFunc = greater;
      CullMode = none;
    }
}
