#include "..\shared\intrinsics.inc"
#include "..\shared\helpers.inc"

float4x4 scaledScreenToWorld;
uniform float foggedColorTintValue = 0.6f;

sampler depthTexture = sampler_state
{
    MinFilter = POINT;
    MagFilter = POINT;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler colorTexture = sampler_state
{
    MinFilter = POINT;
    MagFilter = POINT;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler visibilityTexture = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler prevVisibilityTexture = sampler_state
{
    MinFilter = LINEAR;
    MagFilter = LINEAR;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct visibilityVertexIn
{
    float4 Position   : POSITION0;
    float2 UV         : TEXCOORD0;
};

struct visibilityVertexOut
{
    float4 Position   : POSITION0;
    float2 UV         : TEXCOORD0;
    float4 UVPosition : TEXCOORD1;
};

visibilityVertexOut vsGenerateVisibility(in visibilityVertexIn In)
{
    visibilityVertexOut Out;
    Out.Position   = In.Position;
    Out.UV         = In.UV;
    Out.UVPosition = In.Position;
    return Out;
}


float4 psGenerateVisibility(float2 UV : TEXCOORD0, float4 screenPos : TEXCOORD1, uniform bool forceSkirtToFogged) : COLOR0
{
    float depthSample = tex2D(depthTexture, UV).r;
    screenPos.z = depthSample;

    float4 worldPos;
    worldPos = mul(scaledScreenToWorld, screenPos);
    worldPos /= worldPos.w;
    float2 visTexUV;
    visTexUV.x = worldPos.x;
    visTexUV.y = 1.0f - worldPos.z;
    
    float3 color = tex2D(colorTexture, UV);        
            
    if (screenPos.z >= 1.0)
       return float4(color, 1.0);
        
    float visFactor = tex2D(visibilityTexture, visTexUV).a;

    if (forceSkirtToFogged)
    {
      if ( (visTexUV.x >= .999f) || (visTexUV.y >= .999f) ||
            (visTexUV.x < 0.0f) || (visTexUV.y < 0.0f) )
         visFactor = 1.0f;
    }         
            
    if (visFactor < 7.0/255.0)
      return float4(color, 1.0);
      
    visFactor = (visFactor - 7.0/255.0) * (255.0/248.0);
                        
    float3 foggedColor = color * foggedColorTintValue;
    float3 foggedColorGray = dot(foggedColor, float3(.212671, .715160, .072169));
    foggedColor = lerp(foggedColor, foggedColorGray, .6);
    
    return float4(lerp(color, foggedColor, visFactor), 1.0);
}

void vsQuadPass(in visibilityVertexIn In, out float4 Pos : POSITION, out float2 UV : TEXCOORD0)
{
    Pos = In.Position;
    UV = In.UV;
}

float4 blurVisibilityOffsets[4];
float4 blurVisibilityWeights;

// X axis blur
float4 psBlurVisibilityX(float2 UV : TEXCOORD0) : COLOR0
{
   float4 s;
   s.x = tex2D(visibilityTexture, UV + blurVisibilityOffsets[0]).a;
   s.y = tex2D(visibilityTexture, UV + blurVisibilityOffsets[1]).a;
   s.z = tex2D(visibilityTexture, UV + blurVisibilityOffsets[2]).a;
   s.w = tex2D(visibilityTexture, UV + blurVisibilityOffsets[3]).a;
   
   float4 prev;
   asm
   {
      tfetch2D prev, UV, prevVisibilityTexture, MinFilter=point, MagFilter=point, MipFilter=point
   };
   
   float curV = dot(s, blurVisibilityWeights);
   
   float prevV = prev.a * 15.0f/16.0f;
            
   return float4(prevV, prevV, prevV, curV);
}

// Y axis blur, then lerp from previous frame
float4 psBlurVisibilityY(float2 UV : TEXCOORD0) : COLOR0
{
    float4 s;
    s.x = tex2D(prevVisibilityTexture, UV + blurVisibilityOffsets[0]).a;
    s.y = tex2D(prevVisibilityTexture, UV + blurVisibilityOffsets[1]).a;
    s.z = tex2D(prevVisibilityTexture, UV + blurVisibilityOffsets[2]).a;
    s.w = tex2D(prevVisibilityTexture, UV + blurVisibilityOffsets[3]).a;
   
    float curV = dot(s, blurVisibilityWeights);

    //curV *= 1.0f/16.0f;
                
    return curV;
}

technique generateVisibility
{
    pass
    {
      VertexShader = compile vs_3_0 vsGenerateVisibility();
      PixelShader = compile ps_3_0 psGenerateVisibility(true);

      ViewPortEnable       = false;
      ZEnable              = false;
      ZWriteEnable         = false;
      CullMode             = None;
      AlphaBlendEnable     = false;
      AlphaTestEnable      = false;
    }
}

technique blurVisibilityX
{
    pass
    {
      VertexShader = compile vs_3_0 vsQuadPass();
      PixelShader = compile ps_3_0 psBlurVisibilityX();

      ViewPortEnable       = false;
      ZEnable              = false;
      ZWriteEnable         = false;
      CullMode             = None;
      AlphaBlendEnable     = false;
      AlphaTestEnable      = false;
    }
}

technique blurVisibilityY
{
    pass
    {
      VertexShader = compile vs_3_0 vsQuadPass();
      PixelShader = compile ps_3_0 psBlurVisibilityY();

      ViewPortEnable       = false;
      ZEnable              = false;
      ZWriteEnable         = false;
      CullMode             = None;
      // rg - disabling smooth transitioning for now, it's busted
      //AlphaBlendEnable     = true;
      AlphaBlendEnable     = false;
      SrcBlend             = One;
      DestBlend            = One;
      AlphaTestEnable      = false;
    }
}
