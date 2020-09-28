#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

#define BMINIMAP_NO_BLACKMAP

float4x4 transform;
float4   colorScale;
float    textureAtlasScale;

sampler linearTexture = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

sampler linearStencil = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler pointTexture = sampler_state
{
    MinFilter = POINT;
    MagFilter = POINT;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct PTVertexIn
{
   float4 Position   : POSITION0;
   float2 UV         : TEXCOORD0;
};

struct PTVertexOut
{
   float4 Position   : POSITION0;
   float2 UV         : TEXCOORD0;
};

struct PSDVertexIn
{
   float4 Position   : POSITION0;
   float  Size       : PSIZE0;
   float4 Diffuse    : COLOR0;
};

struct PDVertexIn
{
   float4 Position   : POSITION0;
   float4 Diffuse    : COLOR0;
};

struct PDVertexOut
{
   float4 Position   : POSITION0;
   float4 Diffuse    : COLOR0;
};

struct PSDVertexOut
{
   float4 Position   : POSITION0;
   float  Size       : PSIZE0;
   float4 Diffuse    : COLOR0;
};

struct PSVertexIn
{
   float4 Position   : POSITION0;
   float  Size       : PSIZE0;
};

struct PSVertexOut
{
   float4 Position   : POSITION0;
   float  Size       : PSIZE0;
};

struct PSTDVertexIn
{
   float4 Position   : POSITION0;
   float2 Size       : PSIZE0;
   float2 UV         : TEXCOORD0;
   float4 Diffuse    : COLOR0;
};

struct PSTDVertexOut
{
   float4 Position   : POSITION0;
   float  Size       : PSIZE0;
   float3 UV         : TEXCOORD0;
   float4 Diffuse    : COLOR0;
};

PTVertexOut vsPT(in PTVertexIn In)
{
   PTVertexOut Out;
   Out.Position = mul(In.Position, transform);
   Out.UV       = In.UV;
   return Out;
}

PSDVertexOut vsPSD(in PSDVertexIn In)
{
   PSDVertexOut Out;
   Out.Position = mul(In.Position, transform);
   Out.Size     = In.Size;
   Out.Diffuse  = In.Diffuse;
   return Out;
}

PSTDVertexOut vsPSTD(in PSTDVertexIn In)
{
   PSTDVertexOut Out;
   Out.Position = mul(In.Position, transform);
   Out.Size     = In.Size.x;
   Out.UV.xy    = In.UV.xy;
   Out.UV.z     = In.Size.y * textureAtlasScale;
   Out.Diffuse  = In.Diffuse;
   return Out;
}

PDVertexOut vsPD(in PDVertexIn In)
{
   PDVertexOut Out;
   Out.Position = mul(In.Position, transform);
   Out.Diffuse  = In.Diffuse;
   return Out;
}

PSVertexOut vsPS(in PSVertexIn In)
{
   PSVertexOut Out;
   Out.Position = mul(In.Position, transform);
   Out.Size     = In.Size;
   return Out;
}

float4 psD(float4 inDiffuse : COLOR0) : COLOR0
{   
   return inDiffuse * colorScale;
}

float4 psT(float2 inUV : TEXCOORD0) : COLOR0
{
   return tex2D(linearTexture, inUV) * colorScale;
}

float4 psTT(float2 inUV : TEXCOORD0) : COLOR0
{
#if defined(BMINIMAP_NO_BLACKMAP)
   return min(tex2D(linearTexture, inUV), float4(1.0f, 1.0f, 1.0f, 0.5f)) * tex2D(linearStencil, inUV) * colorScale;
#else
   return tex2D(linearTexture, inUV) * tex2D(linearStencil, inUV) * colorScale;
#endif
}

float4 pst(float2 inUV : SPRITETEXCOORD) : COLOR0
{
   return tex2D(linearTexture, inUV) * colorScale;
}

float4 pstd(float2 inUV : SPRITETEXCOORD, float3 inUV2 : TEXCOORD0, float4 inDiffuse : COLOR0) : COLOR0
{
   float2 uv = inUV2.xy + (inUV.xy * inUV2.z);
   return tex2D(linearTexture, uv) * inDiffuse * colorScale;
}

float4 pst_point(float2 inUV : SPRITETEXCOORD) : COLOR0
{
   return tex2D(pointTexture, inUV) * colorScale;
}

technique coloredBlendedPointSprite
{
    pass
    {
      VertexShader = compile vs_3_0 vsPSD();
      PixelShader = compile ps_3_0 psD();

      PointSpriteEnable = true;
      PointSize_Min = 1.0;
      PointSize_Max = 256.0;
      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      CullMode = None;
      HighPrecisionBlendEnable = true;
    }
}

technique coloredBlendedSprite
{
    pass
    {
      VertexShader = compile vs_3_0 vsPD();
      PixelShader = compile ps_3_0 psD();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
      HighPrecisionBlendEnable = true;
    }
}

technique texturedBlendedSprite
{
    pass
    {
      VertexShader = compile vs_3_0 vsPT();
      PixelShader = compile ps_3_0 psT();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
      HighPrecisionBlendEnable = true;
    }
}

technique texturedAdditiveBlendedSprite
{
    pass
    {
      VertexShader = compile vs_3_0 vsPT();
      PixelShader = compile ps_3_0 psT();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
      HighPrecisionBlendEnable = true;
    }
}

technique texturedBlendedPointSprite
{
    pass
    {
      VertexShader = compile vs_3_0 vsPS();
      PixelShader = compile ps_3_0 pst();

      PointSpriteEnable = true;
      PointSize_Min = 1.0;
      PointSize_Max = 256.0;
      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
      HighPrecisionBlendEnable = true;
    }
}

technique texturedBlendedColoredPointSprite
{
    pass
    {
      VertexShader = compile vs_3_0 vsPSTD();
      PixelShader = compile ps_3_0 pstd();

      PointSpriteEnable = true;
      PointSize_Min = 1.0;
      PointSize_Max = 256.0;
      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
      HighPrecisionBlendEnable = true;
    }
}

technique visibilityGenerator
{
    pass
    {
      VertexShader = compile vs_3_0 vsPS();
      PixelShader = compile ps_3_0 pst();

      PointSpriteEnable = true;
      PointSize_Min = 1.0;
      PointSize_Max = 256.0;
      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = false;
      CullMode = None;
   }

    pass
    {
      VertexShader = compile vs_3_0 vsPS();
      PixelShader = compile ps_3_0 pst_point();

      PointSpriteEnable = true;
      PointSize_Min = 1.0;
      PointSize_Max = 256.0;
      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = false;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
    }

    pass
    {
      VertexShader = compile vs_3_0 vsPS();
      PixelShader = compile ps_3_0 pst_point();

      PointSpriteEnable = true;
      PointSize_Min = 1.0;
      PointSize_Max = 256.0;
      ViewPortEnable = false;
      ZEnable = false;
      BlendOp = Max;
      SrcBlend = One;
      DestBlend = One;
      AlphaBlendEnable = true;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
    }
}

technique drawVisibility
{
    pass
    {
      VertexShader = compile vs_3_0 vsPT();
      PixelShader = compile ps_3_0 psTT();

      ViewPortEnable = false;
      ZEnable = false;
      AlphaBlendEnable = true;
      BlendOp = Add;
      SrcBlend = SrcAlpha;
      DestBlend = InvSrcAlpha;
      AlphaTestEnable = true;
      AlphaFunc = Greater;
      AlphaRef = 0.0;
      CullMode = None;
    }
}

